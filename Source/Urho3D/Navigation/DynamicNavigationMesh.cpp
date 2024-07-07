//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Navigation/DynamicNavigationMesh.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Navigation/CrowdAgent.h"
#include "../Navigation/NavArea.h"
#include "../Navigation/NavBuildData.h"
#include "../Navigation/NavigationEvents.h"
#include "../Navigation/NavigationUtils.h"
#include "../Navigation/Obstacle.h"
#include "../Navigation/OffMeshConnection.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <DetourTileCache/DetourTileCache.h>
#include <DetourTileCache/DetourTileCacheBuilder.h>
#include <LZ4/lz4.h>
#include <Recast/Recast.h>

#include <EASTL/shared_array.h>

// DebugNew is deliberately not used because the macro 'free' conflicts with DetourTileCache's LinearAllocator interface
//#include "../DebugNew.h"

namespace Urho3D
{

static const int DEFAULT_MAX_OBSTACLES = 1024;
static const int DEFAULT_MAX_LAYERS = 16;

struct DynamicNavigationMesh::TileCacheData
{
    unsigned char* data;
    int dataSize;
};

struct TileCompressor : public dtTileCacheCompressor
{
    int maxCompressedSize(const int bufferSize) override
    {
        return (int)(bufferSize * 1.05f);
    }

    dtStatus compress(const unsigned char* buffer, const int bufferSize,
        unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) override
    {
        *compressedSize = LZ4_compress_default((const char*)buffer, (char*)compressed, bufferSize, LZ4_compressBound(bufferSize));
        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize,
        unsigned char* buffer, const int maxBufferSize, int* bufferSize) override
    {
        *bufferSize = LZ4_decompress_safe((const char*)compressed, (char*)buffer, compressedSize, maxBufferSize);
        return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
    }
};

struct MeshProcess : public dtTileCacheMeshProcess
{
    DynamicNavigationMesh* owner_;
    ea::vector<Vector3> offMeshVertices_;
    ea::vector<float> offMeshRadii_;
    ea::vector<unsigned short> offMeshFlags_;
    ea::vector<unsigned char> offMeshAreas_;
    ea::vector<unsigned char> offMeshDir_;

    inline explicit MeshProcess(DynamicNavigationMesh* owner) :
        owner_(owner)
    {
    }

    void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) override
    {
        // Update poly flags from areas.
        // \todo Assignment of flags from areas?
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] != RC_NULL_AREA)
                polyFlags[i] = RC_WALKABLE_AREA;
        }

        BoundingBox bounds;
        rcVcopy(&bounds.min_.x_, params->bmin);
        rcVcopy(&bounds.max_.x_, params->bmin);

        // collect off-mesh connections
        ea::vector<OffMeshConnection*> offMeshConnections = owner_->CollectOffMeshConnections(bounds);

        if (offMeshConnections.size() > 0)
        {
            if (offMeshConnections.size() != offMeshRadii_.size())
            {
                Matrix3x4 inverse = owner_->GetNode()->GetWorldTransform().Inverse();
                ClearConnectionData();
                for (unsigned i = 0; i < offMeshConnections.size(); ++i)
                {
                    OffMeshConnection* connection = offMeshConnections[i];
                    Vector3 start = inverse * connection->GetNode()->GetWorldPosition();
                    Vector3 end = inverse * connection->GetEndPoint()->GetWorldPosition();

                    offMeshVertices_.push_back(start);
                    offMeshVertices_.push_back(end);
                    offMeshRadii_.push_back(connection->GetRadius());
                    offMeshFlags_.push_back((unsigned short) connection->GetMask());
                    offMeshAreas_.push_back((unsigned char) connection->GetAreaID());
                    offMeshDir_.push_back((unsigned char) (connection->IsBidirectional() ? DT_OFFMESH_CON_BIDIR : 0));
                }
            }
            params->offMeshConCount = offMeshRadii_.size();
            params->offMeshConVerts = &offMeshVertices_[0].x_;
            params->offMeshConRad = &offMeshRadii_[0];
            params->offMeshConFlags = &offMeshFlags_[0];
            params->offMeshConAreas = &offMeshAreas_[0];
            params->offMeshConDir = &offMeshDir_[0];
        }
    }

    void ClearConnectionData()
    {
        offMeshVertices_.clear();
        offMeshRadii_.clear();
        offMeshFlags_.clear();
        offMeshAreas_.clear();
        offMeshDir_.clear();
    }
};

class LinearAllocator : public dtTileCacheAlloc
{
public:
    explicit LinearAllocator(unsigned cap) { buffer_.resize(cap); }
    ~LinearAllocator() override { FreeOverflow(); }

    /// Implement dtTileCacheAlloc.
    /// @{
    void reset() override
    {
        maxAllocation_ = ea::max(maxAllocation_, currentAllocation_);
        currentAllocation_ = 0;
        currentOffset_ = 0;

        if (!overflow_.empty())
            buffer_.resize(NextPowerOfTwo(maxAllocation_ * 3 / 2));

        FreeOverflow();
    }

    void* alloc(const size_t size) override
    {
        currentAllocation_ += size;

        if (currentOffset_ + size > buffer_.size())
        {
            void* ptr = dtAlloc(size, DT_ALLOC_TEMP);
            overflow_.push_back(ptr);
            return ptr;
        }

        void* ptr = &buffer_[currentOffset_];
        currentOffset_ += size;
        return ptr;
    }

    void free(void* ptr) override {}
    /// @}

private:
    void FreeOverflow()
    {
        for (void* ptr : overflow_)
            dtFree(ptr);
        overflow_.clear();
    }

    ByteVector buffer_;
    ea::vector<void*> overflow_;
    unsigned currentOffset_{};

    unsigned currentAllocation_{};
    unsigned maxAllocation_{};
};

DynamicNavigationMesh::DynamicNavigationMesh(Context* context) :
    NavigationMesh(context),
    maxLayers_(DEFAULT_MAX_LAYERS)
{
    partitionType_ = NAVMESH_PARTITION_MONOTONE;
    allocator_ = ea::make_unique<LinearAllocator>(32 * 1024);
    compressor_ = ea::make_unique<TileCompressor>();
    meshProcessor_ = ea::make_unique<MeshProcess>(this);
}

DynamicNavigationMesh::~DynamicNavigationMesh()
{
    ReleaseNavigationMesh();
}

void DynamicNavigationMesh::RegisterObject(Context* context)
{
    context->AddFactoryReflection<DynamicNavigationMesh>(Category_Navigation);

    URHO3D_COPY_BASE_ATTRIBUTES(NavigationMesh);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Obstacles", GetMaxObstacles, SetMaxObstacles, unsigned, DEFAULT_MAX_OBSTACLES, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Layers", GetMaxLayers, SetMaxLayers, unsigned, DEFAULT_MAX_LAYERS, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Obstacles", GetDrawObstacles, SetDrawObstacles, bool, false, AM_DEFAULT);
}

bool DynamicNavigationMesh::AllocateMesh(unsigned maxTiles)
{
    if (!NavigationMesh::AllocateMesh(maxTiles * maxLayers_))
        return false;

    dtTileCacheParams tileCacheParams;      // NOLINT(hicpp-member-init)
    memset(&tileCacheParams, 0, sizeof(tileCacheParams));
    // TODO: Fill tileCacheParams.orig
    tileCacheParams.ch = cellHeight_;
    tileCacheParams.cs = cellSize_;
    tileCacheParams.width = tileSize_;
    tileCacheParams.height = tileSize_;
    tileCacheParams.maxSimplificationError = edgeMaxError_;
    tileCacheParams.maxTiles = maxTiles * maxLayers_;
    tileCacheParams.maxObstacles = maxObstacles_;
    // Settings from NavigationMesh
    tileCacheParams.walkableClimb = agentMaxClimb_;
    tileCacheParams.walkableHeight = agentHeight_;
    tileCacheParams.walkableRadius = agentRadius_;

    tileCache_ = dtAllocTileCache();
    if (!tileCache_)
    {
        URHO3D_LOGERROR("Could not allocate tile cache");
        ReleaseNavigationMesh();
        return false;
    }

    if (dtStatusFailed(tileCache_->init(&tileCacheParams, allocator_.get(), compressor_.get(), meshProcessor_.get())))
    {
        URHO3D_LOGERROR("Could not initialize tile cache");
        ReleaseNavigationMesh();
        return false;
    }

    // No need to scan for obstacles here because there are no tiles yet
    return true;
}

bool DynamicNavigationMesh::RebuildMesh()
{
    if (!NavigationMesh::RebuildMesh())
        return false;

    // Scan for obstacles to insert into us
    ea::vector<Node*> obstacles;
    GetScene()->GetChildrenWithComponent<Obstacle>(obstacles, true);
    for (unsigned i = 0; i < obstacles.size(); ++i)
    {
        auto* obs = obstacles[i]->GetComponent<Obstacle>();
        if (obs && obs->IsEnabledEffective())
            AddObstacle(obs);
    }

    return true;
}

ea::vector<unsigned char> DynamicNavigationMesh::GetTileData(const IntVector2& tileIndex) const
{
    dtCompressedTileRef tiles[MaxLayers];
    const int numTiles = tileCache_->getTilesAt(tileIndex.x_, tileIndex.y_, tiles, maxLayers_);

    VectorBuffer ret;
    for (int i = 0; i < numTiles; ++i)
    {
        const dtCompressedTile* tile = tileCache_->getTileByRef(tiles[i]);
        WriteTile(ret, tileIndex.x_, tileIndex.y_, tile->header->tlayer);
    }
    return ret.GetBuffer();
}

bool DynamicNavigationMesh::IsObstacleInTile(Obstacle* obstacle, const IntVector2& tileIndex) const
{
    const BoundingBox tileBoundingBox = GetTileBoundingBoxColumn(tileIndex);
    const Vector3 obstaclePosition = obstacle->GetNode()->GetWorldPosition();
    return tileBoundingBox.DistanceToPoint(obstaclePosition) < obstacle->GetRadius();
}

bool DynamicNavigationMesh::AddTile(const ea::vector<unsigned char>& tileData)
{
    MemoryBuffer buffer(tileData);
    return ReadTiles(buffer, false);
}

void DynamicNavigationMesh::RemoveTile(const IntVector2& tileIndex)
{
    if (!navMesh_)
        return;

    dtCompressedTileRef existing[MaxLayers];
    const int existingCt = tileCache_->getTilesAt(tileIndex.x_, tileIndex.y_, existing, maxLayers_);
    for (int i = 0; i < existingCt; ++i)
    {
        unsigned char* data = nullptr;
        if (!dtStatusFailed(tileCache_->removeTile(existing[i], &data, nullptr)) && data != nullptr)
            dtFree(data);
    }

    NavigationMesh::RemoveTile(tileIndex);
}

void DynamicNavigationMesh::RemoveAllTiles()
{
    int numTiles = tileCache_->getTileCount();
    for (int i = 0; i < numTiles; ++i)
    {
        const dtCompressedTile* tile = tileCache_->getTile(i);
        assert(tile);
        if (tile->header)
            tileCache_->removeTile(tileCache_->getTileRef(tile), nullptr, nullptr);
    }

    NavigationMesh::RemoveAllTiles();
}

void DynamicNavigationMesh::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!debug || !navMesh_ || !node_)
        return;

    for (int j = 0; j < navMesh_->getMaxTiles(); ++j)
        DrawDebugTileGeometry(debug, depthTest, j);

    Scene* scene = GetScene();
    if (scene)
    {
        // Draw Obstacle components
        if (drawObstacles_)
        {
            ea::vector<Node*> obstacles;
            scene->GetChildrenWithComponent<Obstacle>(obstacles, true);
            for (unsigned i = 0; i < obstacles.size(); ++i)
            {
                auto* obstacle = obstacles[i]->GetComponent<Obstacle>();
                if (obstacle && obstacle->IsEnabledEffective())
                    obstacle->DrawDebugGeometry(debug, depthTest);
            }
        }

        // Draw OffMeshConnection components
        if (drawOffMeshConnections_)
        {
            ea::vector<Node*> connections;
            scene->GetChildrenWithComponent<OffMeshConnection>(connections, true);
            for (unsigned i = 0; i < connections.size(); ++i)
            {
                auto* connection = connections[i]->GetComponent<OffMeshConnection>();
                if (connection && connection->IsEnabledEffective())
                    connection->DrawDebugGeometry(debug, depthTest);
            }
        }

        // Draw NavArea components
        if (drawNavAreas_)
        {
            ea::vector<Node*> areas;
            scene->GetChildrenWithComponent<NavArea>(areas, true);
            for (unsigned i = 0; i < areas.size(); ++i)
            {
                auto* area = areas[i]->GetComponent<NavArea>();
                if (area && area->IsEnabledEffective())
                    area->DrawDebugGeometry(debug, depthTest);
            }
        }
    }
}

void DynamicNavigationMesh::DrawDebugGeometry(bool depthTest)
{
    Scene* scene = GetScene();
    if (scene)
    {
        auto* debug = scene->GetComponent<DebugRenderer>();
        if (debug)
            DrawDebugGeometry(debug, depthTest);
    }
}

void DynamicNavigationMesh::SetNavigationDataAttr(const ea::vector<unsigned char>& value)
{
    ReleaseNavigationMesh();

    if (value.empty())
        return;

    MemoryBuffer buffer(value);

    // Keep the header the same as the old data format to check for validity.
    buffer.ReadBoundingBox();
    const int unused0 = buffer.ReadInt();
    const int unused1 = buffer.ReadInt();
    const int version = buffer.ReadInt();
    if (unused0 != 0 || unused1 != 0 || version != NavigationDataVersion)
    {
        URHO3D_LOGWARNING("Incompatible navigation data format, please rebuild navigation data");
        return;
    }

    dtNavMeshParams params;     // NOLINT(hicpp-member-init)
    buffer.Read(&params, sizeof(dtNavMeshParams));

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        URHO3D_LOGERROR("Could not allocate navigation mesh");
        return;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        URHO3D_LOGERROR("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return;
    }

    dtTileCacheParams tcParams;     // NOLINT(hicpp-member-init)
    buffer.Read(&tcParams, sizeof(tcParams));

    tileCache_ = dtAllocTileCache();
    if (!tileCache_)
    {
        URHO3D_LOGERROR("Could not allocate tile cache");
        ReleaseNavigationMesh();
        return;
    }
    if (dtStatusFailed(tileCache_->init(&tcParams, allocator_.get(), compressor_.get(), meshProcessor_.get())))
    {
        URHO3D_LOGERROR("Could not initialize tile cache");
        ReleaseNavigationMesh();
        return;
    }

    ReadTiles(buffer, true);
    // \todo Shall we send E_NAVIGATION_MESH_REBUILT here?
}

ea::vector<unsigned char> DynamicNavigationMesh::GetNavigationDataAttr() const
{
    VectorBuffer ret;
    if (navMesh_ && tileCache_)
    {
        ret.WriteBoundingBox(BoundingBox{});
        ret.WriteInt(0);
        ret.WriteInt(0);
        ret.WriteInt(NavigationDataVersion);

        const dtNavMeshParams* params = navMesh_->getParams();
        ret.Write(params, sizeof(dtNavMeshParams));

        const dtTileCacheParams* tcParams = tileCache_->getParams();
        ret.Write(tcParams, sizeof(dtTileCacheParams));

        for (int i = 0; i < navMesh_->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = const_cast<const dtNavMesh*>(navMesh_)->getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;

            WriteTile(ret, tile->header->x, tile->header->y, tile->header->layer);
        }
    }
    return ret.GetBuffer();
}

void DynamicNavigationMesh::SetMaxLayers(unsigned maxLayers)
{
    // Set 3 as a minimum due to the tendency of layers to be constructed inside the hollow space of stacked objects
    // That behavior is unlikely to be expected by the end user
    maxLayers_ = Max(3U, Min(maxLayers, MaxLayers));
}

void DynamicNavigationMesh::WriteTile(Serializer& dest, int x, int z, int layer) const
{
    // Don't write "void-space" tiles
    const dtCompressedTile* tile = tileCache_->getTileAt(x, z, layer);
    if (!tile || !tile->header || !tile->dataSize)
        return;

    // The header conveniently has the majority of the information required
    dest.Write(tile->header, sizeof(dtTileCacheLayerHeader));
    dest.WriteInt(tile->dataSize);
    dest.Write(tile->data, static_cast<unsigned>(tile->dataSize));
}

bool DynamicNavigationMesh::ReadTiles(Deserializer& source, bool silent)
{
    tileQueue_.clear();
    while (!source.IsEof())
    {
        dtTileCacheLayerHeader header;      // NOLINT(hicpp-member-init)
        source.Read(&header, sizeof(dtTileCacheLayerHeader));
        const int dataSize = source.ReadInt();

        auto* data = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        if (!data)
        {
            URHO3D_LOGERROR("Could not allocate data for navigation mesh tile");
            return false;
        }

        source.Read(data, (unsigned)dataSize);
        if (dtStatusFailed(tileCache_->addTile(data, dataSize, DT_TILE_FREE_DATA, nullptr)))
        {
            URHO3D_LOGERROR("Failed to add tile");
            dtFree(data);
            return false;
        }

        const IntVector2 tileIdx = IntVector2(header.tx, header.ty);
        if (tileQueue_.empty() || tileQueue_.back() != tileIdx)
            tileQueue_.push_back(tileIdx);
    }

    for (unsigned i = 0; i < tileQueue_.size(); ++i)
        tileCache_->buildNavMeshTilesAt(tileQueue_[i].x_, tileQueue_[i].y_, navMesh_);

    // Send event
    if (!silent)
    {
        for (const IntVector2& tileIndex : tileQueue_)
            SendTileAddedEvent(tileIndex);
    }
    return true;
}

int DynamicNavigationMesh::BuildTile(ea::vector<NavigationGeometryInfo>& geometryList, int x, int z, TileCacheData* tiles)
{
    URHO3D_PROFILE("BuildNavigationMeshTile");

    const dtMeshTile* tilesToRemove[MaxLayers];
    const int numTilesToRemove = navMesh_->getTilesAt(x, z, tilesToRemove, MaxLayers);
    for (int i = 0; i < numTilesToRemove; ++i)
    {
        const dtTileRef tileRef = navMesh_->getTileRefAt(x, z, tilesToRemove[i]->header->layer);
        tileCache_->removeTile(tileRef, nullptr, nullptr);
    }

    const BoundingBox tileColumn = GetTileBoundingBoxColumn(IntVector2{x, z});
    const BoundingBox tileBoundingBox =
        IsHeightRangeValid() ? tileColumn : CalculateTileBoundingBox(geometryList, tileColumn);

    DynamicNavBuildData build(allocator_.get());

    rcConfig cfg;   // NOLINT(hicpp-member-init)
    memset(&cfg, 0, sizeof(cfg));
    cfg.cs = cellSize_;
    cfg.ch = cellHeight_;
    cfg.walkableSlopeAngle = agentMaxSlope_;
    cfg.walkableHeight = (int)ceilf(agentHeight_ / cfg.ch);
    cfg.walkableClimb = (int)floorf(agentMaxClimb_ / cfg.ch);
    cfg.walkableRadius = (int)ceilf(agentRadius_ / cfg.cs);
    cfg.maxEdgeLen = (int)(edgeMaxLength_ / cellSize_);
    cfg.maxSimplificationError = edgeMaxError_;
    cfg.minRegionArea = (int)sqrtf(regionMinSize_);
    cfg.mergeRegionArea = (int)sqrtf(regionMergeSize_);
    cfg.maxVertsPerPoly = 6;
    cfg.tileSize = tileSize_;
    cfg.borderSize = cfg.walkableRadius + 3; // Add padding
    cfg.width = cfg.tileSize + cfg.borderSize * 2;
    cfg.height = cfg.tileSize + cfg.borderSize * 2;
    cfg.detailSampleDist = detailSampleDistance_ < 0.9f ? 0.0f : cellSize_ * detailSampleDistance_;
    cfg.detailSampleMaxError = cellHeight_ * detailSampleMaxError_;

    rcVcopy(cfg.bmin, &tileBoundingBox.min_.x_);
    rcVcopy(cfg.bmax, &tileBoundingBox.max_.x_);
    cfg.bmin[0] -= cfg.borderSize * cfg.cs;
    cfg.bmin[1] -= padding_.y_;
    cfg.bmin[2] -= cfg.borderSize * cfg.cs;
    cfg.bmax[0] += cfg.borderSize * cfg.cs;
    cfg.bmax[1] += padding_.y_;
    cfg.bmax[2] += cfg.borderSize * cfg.cs;

    BoundingBox expandedBox(*reinterpret_cast<Vector3*>(cfg.bmin), *reinterpret_cast<Vector3*>(cfg.bmax));
    GetTileGeometry(&build, geometryList, expandedBox);

    if (build.vertices_.empty() || build.indices_.empty())
        return 0; // Nothing to do

    build.heightField_ = rcAllocHeightfield();
    if (!build.heightField_)
    {
        URHO3D_LOGERROR("Could not allocate heightfield");
        return 0;
    }

    if (!rcCreateHeightfield(build.ctx_, *build.heightField_, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs,
        cfg.ch))
    {
        URHO3D_LOGERROR("Could not create heightfield");
        return 0;
    }

    unsigned numTriangles = build.indices_.size() / 3;
    ea::shared_array<unsigned char> triAreas(new unsigned char[numTriangles]);
    memset(triAreas.get(), 0, numTriangles);

    rcMarkWalkableTriangles(build.ctx_, cfg.walkableSlopeAngle, &build.vertices_[0].x_, build.vertices_.size(),
        &build.indices_[0], numTriangles, triAreas.get());
    rcRasterizeTriangles(build.ctx_, &build.vertices_[0].x_, build.vertices_.size(), &build.indices_[0],
        triAreas.get(), numTriangles, *build.heightField_, cfg.walkableClimb);
    rcFilterLowHangingWalkableObstacles(build.ctx_, cfg.walkableClimb, *build.heightField_);

    rcFilterLedgeSpans(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_);
    rcFilterWalkableLowHeightSpans(build.ctx_, cfg.walkableHeight, *build.heightField_);

    build.compactHeightField_ = rcAllocCompactHeightfield();
    if (!build.compactHeightField_)
    {
        URHO3D_LOGERROR("Could not allocate create compact heightfield");
        return 0;
    }
    if (!rcBuildCompactHeightfield(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_,
        *build.compactHeightField_))
    {
        URHO3D_LOGERROR("Could not build compact heightfield");
        return 0;
    }
    if (!rcErodeWalkableArea(build.ctx_, cfg.walkableRadius, *build.compactHeightField_))
    {
        URHO3D_LOGERROR("Could not erode compact heightfield");
        return 0;
    }

    // area volumes
    for (unsigned i = 0; i < build.navAreas_.size(); ++i)
        rcMarkBoxArea(build.ctx_, &build.navAreas_[i].bounds_.min_.x_, &build.navAreas_[i].bounds_.max_.x_,
            build.navAreas_[i].areaID_, *build.compactHeightField_);

    if (this->partitionType_ == NAVMESH_PARTITION_WATERSHED)
    {
        if (!rcBuildDistanceField(build.ctx_, *build.compactHeightField_))
        {
            URHO3D_LOGERROR("Could not build distance field");
            return 0;
        }
        if (!rcBuildRegions(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea,
            cfg.mergeRegionArea))
        {
            URHO3D_LOGERROR("Could not build regions");
            return 0;
        }
    }
    else
    {
        if (!rcBuildRegionsMonotone(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
        {
            URHO3D_LOGERROR("Could not build monotone regions");
            return 0;
        }
    }

    build.heightFieldLayers_ = rcAllocHeightfieldLayerSet();
    if (!build.heightFieldLayers_)
    {
        URHO3D_LOGERROR("Could not allocate height field layer set");
        return 0;
    }

    if (!rcBuildHeightfieldLayers(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.walkableHeight,
        *build.heightFieldLayers_))
    {
        URHO3D_LOGERROR("Could not build height field layers");
        return 0;
    }

    int retCt = 0;
    for (int i = 0; i < build.heightFieldLayers_->nlayers; ++i)
    {
        dtTileCacheLayerHeader header;      // NOLINT(hicpp-member-init)
        header.magic = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;
        header.tx = x;
        header.ty = z;
        header.tlayer = i;

        rcHeightfieldLayer* layer = &build.heightFieldLayers_->layers[i];

        // Tile info.
        rcVcopy(header.bmin, layer->bmin);
        rcVcopy(header.bmax, layer->bmax);
        header.width = (unsigned char)layer->width;
        header.height = (unsigned char)layer->height;
        header.minx = (unsigned char)layer->minx;
        header.maxx = (unsigned char)layer->maxx;
        header.miny = (unsigned char)layer->miny;
        header.maxy = (unsigned char)layer->maxy;
        header.hmin = (unsigned short)layer->hmin;
        header.hmax = (unsigned short)layer->hmax;

        if (dtStatusFailed(
            dtBuildTileCacheLayer(compressor_.get()/*compressor*/, &header, layer->heights, layer->areas/*areas*/, layer->cons,
                &(tiles[retCt].data), &tiles[retCt].dataSize)))
        {
            URHO3D_LOGERROR("Failed to build tile cache layers");
            return 0;
        }
        else
            ++retCt;
    }

    // Send a notification of the rebuild of this tile to anyone interested
    {
        using namespace NavigationAreaRebuilt;
        VariantMap& eventData = GetContext()->GetEventDataMap();
        eventData[P_NODE] = GetNode();
        eventData[P_MESH] = this;
        eventData[P_BOUNDSMIN] = Variant(tileBoundingBox.min_);
        eventData[P_BOUNDSMAX] = Variant(tileBoundingBox.max_);
        SendEvent(E_NAVIGATION_AREA_REBUILT, eventData);
    }

    return retCt;
}

unsigned DynamicNavigationMesh::BuildTilesFromGeometry(
    ea::vector<NavigationGeometryInfo>& geometryList, const IntVector2& from, const IntVector2& to)
{
    unsigned numTiles = 0;

    for (int z = from.y_; z <= to.y_; ++z)
    {
        for (int x = from.x_; x <= to.x_; ++x)
        {
            dtCompressedTileRef existing[MaxLayers];
            const int existingCt = tileCache_->getTilesAt(x, z, existing, maxLayers_);
            for (int i = 0; i < existingCt; ++i)
            {
                unsigned char* data = nullptr;
                if (!dtStatusFailed(tileCache_->removeTile(existing[i], &data, nullptr)) && data != nullptr)
                    dtFree(data);
            }

            TileCacheData tiles[MaxLayers];
            int layerCt = BuildTile(geometryList, x, z, tiles);
            for (int i = 0; i < layerCt; ++i)
            {
                dtCompressedTileRef tileRef;
                int status = tileCache_->addTile(tiles[i].data, tiles[i].dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
                if (dtStatusFailed((dtStatus)status))
                {
                    dtFree(tiles[i].data);
                    tiles[i].data = nullptr;
                }
                else
                {
                    tileCache_->buildNavMeshTile(tileRef, navMesh_);
                    ++numTiles;
                }
            }
        }
    }

    return numTiles;
}

ea::vector<OffMeshConnection*> DynamicNavigationMesh::CollectOffMeshConnections(const BoundingBox& bounds)
{
    ea::vector<OffMeshConnection*> connections;
    node_->GetComponents<OffMeshConnection>(connections, true);
    for (unsigned i = 0; i < connections.size(); ++i)
    {
        OffMeshConnection* connection = connections[i];
        if (!(connection->IsEnabledEffective() && connection->GetEndPoint()))
        {
            // discard this connection
            connections.erase_at(i);
            --i;
        }
    }

    return connections;
}

void DynamicNavigationMesh::ReleaseNavigationMesh()
{
    NavigationMesh::ReleaseNavigationMesh();
    ReleaseTileCache();
}

void DynamicNavigationMesh::ReleaseTileCache()
{
    dtFreeTileCache(tileCache_);
    tileCache_ = nullptr;
}

void DynamicNavigationMesh::UpdateTileCache()
{
    bool upToDate = false;
    do
    {
        tileCache_->update(0, navMesh_, &upToDate);
    } while (!upToDate);
}

void DynamicNavigationMesh::OnSceneSet(Scene* scene)
{
    // Subscribe to the scene subsystem update, which will trigger the tile cache to update the nav mesh
    if (scene)
        SubscribeToEvent(scene, E_SCENESUBSYSTEMUPDATE, URHO3D_HANDLER(DynamicNavigationMesh, HandleSceneSubsystemUpdate));
    else
        UnsubscribeFromEvent(E_SCENESUBSYSTEMUPDATE);
}

void DynamicNavigationMesh::AddObstacle(Obstacle* obstacle, bool silent)
{
    if (tileCache_)
    {
        float pos[3];
        Vector3 obsPos = obstacle->GetNode()->GetWorldPosition();
        rcVcopy(pos, &obsPos.x_);
        dtObstacleRef refHolder;

        // Because dtTileCache doesn't process obstacle requests while updating tiles
        // it's necessary update until sufficient request space is available
        UpdateTileCache();

        if (dtStatusFailed(tileCache_->addObstacle(pos, obstacle->GetRadius(), obstacle->GetHeight(), &refHolder)))
        {
            URHO3D_LOGERROR("Failed to add obstacle");
            return;
        }
        obstacle->obstacleId_ = refHolder;
        assert(refHolder > 0);

        if (!silent)
        {
            using namespace NavigationObstacleAdded;
            VariantMap& eventData = GetContext()->GetEventDataMap();
            eventData[P_NODE] = obstacle->GetNode();
            eventData[P_OBSTACLE] = obstacle;
            eventData[P_POSITION] = obstacle->GetNode()->GetWorldPosition();
            eventData[P_RADIUS] = obstacle->GetRadius();
            eventData[P_HEIGHT] = obstacle->GetHeight();
            SendEvent(E_NAVIGATION_OBSTACLE_ADDED, eventData);
        }
    }
}

void DynamicNavigationMesh::ObstacleChanged(Obstacle* obstacle)
{
    if (tileCache_)
    {
        RemoveObstacle(obstacle, true);
        AddObstacle(obstacle, true);
    }
}

void DynamicNavigationMesh::RemoveObstacle(Obstacle* obstacle, bool silent)
{
    if (tileCache_ && obstacle->obstacleId_ > 0)
    {
        // Because dtTileCache doesn't process obstacle requests while updating tiles
        // it's necessary update until sufficient request space is available
        UpdateTileCache();

        if (dtStatusFailed(tileCache_->removeObstacle(obstacle->obstacleId_)))
        {
            URHO3D_LOGERROR("Failed to remove obstacle");
            return;
        }
        obstacle->obstacleId_ = 0;
        // Require a node in order to send an event
        if (!silent && obstacle->GetNode())
        {
            using namespace NavigationObstacleRemoved;
            VariantMap& eventData = GetContext()->GetEventDataMap();
            eventData[P_NODE] = obstacle->GetNode();
            eventData[P_OBSTACLE] = obstacle;
            eventData[P_POSITION] = obstacle->GetNode()->GetWorldPosition();
            eventData[P_RADIUS] = obstacle->GetRadius();
            eventData[P_HEIGHT] = obstacle->GetHeight();
            SendEvent(E_NAVIGATION_OBSTACLE_REMOVED, eventData);
        }
    }
}

void DynamicNavigationMesh::HandleSceneSubsystemUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace SceneSubsystemUpdate;

    if (tileCache_ && navMesh_ && IsEnabledEffective())
        UpdateTileCache();
}

}
