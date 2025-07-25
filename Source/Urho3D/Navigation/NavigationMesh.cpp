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

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Model.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/TerrainPatch.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Navigation/CrowdAgent.h"
#include "../Navigation/DynamicNavigationMesh.h"
#include "../Navigation/NavArea.h"
#include "../Navigation/NavBuildData.h"
#include "../Navigation/Navigable.h"
#include "../Navigation/NavigationEvents.h"
#include "../Navigation/NavigationMesh.h"
#include "../Navigation/NavigationUtils.h"
#include "../Navigation/Obstacle.h"
#include "../Navigation/OffMeshConnection.h"
#ifdef URHO3D_PHYSICS
#include "../Physics/CollisionShape.h"
#endif
#include "../Scene/Scene.h"

#include <cfloat>
#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourNavMeshQuery.h>
#include <Recast/Recast.h>

#include <EASTL/numeric.h>

#include "../DebugNew.h"

namespace Urho3D
{

const char* navmeshPartitionTypeNames[] =
{
    "watershed",
    "monotone",
    nullptr
};

static const int DEFAULT_TILE_SIZE = 128;
static const float DEFAULT_CELL_SIZE = 0.3f;
static const float DEFAULT_CELL_HEIGHT = 0.2f;
static const float DEFAULT_AGENT_HEIGHT = 2.0f;
static const float DEFAULT_AGENT_RADIUS = 0.6f;
static const float DEFAULT_AGENT_MAX_CLIMB = 0.9f;
static const float DEFAULT_AGENT_MAX_SLOPE = 45.0f;
static const float DEFAULT_REGION_MIN_SIZE = 8.0f;
static const float DEFAULT_REGION_MERGE_SIZE = 20.0f;
static const float DEFAULT_EDGE_MAX_LENGTH = 12.0f;
static const float DEFAULT_EDGE_MAX_ERROR = 1.3f;
static const float DEFAULT_DETAIL_SAMPLE_DISTANCE = 6.0f;
static const float DEFAULT_DETAIL_SAMPLE_MAX_ERROR = 1.0f;

static const int MAX_POLYS = 2048;

/// Temporary data for finding a path.
struct FindPathData
{
    // Polygons.
    dtPolyRef polys_[MAX_POLYS]{};
    // Polygons on the path.
    dtPolyRef pathPolys_[MAX_POLYS]{};
    // Points on the path.
    Vector3 pathPoints_[MAX_POLYS];
    // Flags on the path.
    unsigned char pathFlags_[MAX_POLYS]{};
};

namespace
{

/// Write dtMeshTile to the stream.
void WriteTile(Serializer& dest, const dtMeshTile* tile)
{
    dest.WriteInt(tile->header->x);
    dest.WriteInt(tile->header->y);
    dest.WriteInt(tile->dataSize);
    dest.Write(tile->data, static_cast<unsigned>(tile->dataSize));
}

/// Return polygon vertices.
ea::pair<ea::array<Vector3, DT_VERTS_PER_POLYGON>, Vector3> GetPolygonVerticesAndCenter(
    const dtMeshTile& tile, const dtPoly& poly)
{
    ea::array<Vector3, DT_VERTS_PER_POLYGON> vertices;
    for (unsigned i = 0; i < poly.vertCount; ++i)
        memcpy(&vertices[i], &tile.verts[poly.verts[i] * 3], sizeof(Vector3));

    const Vector3 center =
        ea::accumulate(vertices.begin(), vertices.begin() + poly.vertCount, Vector3::ZERO) / poly.vertCount;
    return {vertices, center};
}

} // namespace

NavigationMesh::NavigationMesh(Context* context) :
    Component(context),
    navMesh_(nullptr),
    navMeshQuery_(nullptr),
    queryFilter_(new dtQueryFilter()),
    pathData_(new FindPathData()),
    tileSize_(DEFAULT_TILE_SIZE),
    cellSize_(DEFAULT_CELL_SIZE),
    cellHeight_(DEFAULT_CELL_HEIGHT),
    agentHeight_(DEFAULT_AGENT_HEIGHT),
    agentRadius_(DEFAULT_AGENT_RADIUS),
    agentMaxClimb_(DEFAULT_AGENT_MAX_CLIMB),
    agentMaxSlope_(DEFAULT_AGENT_MAX_SLOPE),
    regionMinSize_(DEFAULT_REGION_MIN_SIZE),
    regionMergeSize_(DEFAULT_REGION_MERGE_SIZE),
    edgeMaxLength_(DEFAULT_EDGE_MAX_LENGTH),
    edgeMaxError_(DEFAULT_EDGE_MAX_ERROR),
    detailSampleDistance_(DEFAULT_DETAIL_SAMPLE_DISTANCE),
    detailSampleMaxError_(DEFAULT_DETAIL_SAMPLE_MAX_ERROR),
    padding_(Vector3::ONE),
    partitionType_(NAVMESH_PARTITION_WATERSHED),
    keepInterResults_(false),
    drawOffMeshConnections_(false),
    drawNavAreas_(false)
{
}

NavigationMesh::~NavigationMesh()
{
    ReleaseNavigationMesh();
}

void NavigationMesh::RegisterObject(Context* context)
{
    context->AddFactoryReflection<NavigationMesh>(Category_Navigation);

    URHO3D_ACTION_STATIC_LABEL("Clear!", Clear, "Clears navigation mesh data");
    URHO3D_ACTION_STATIC_LABEL("Rebuild!", Rebuild, "Rebuilds navigation mesh and adjusts maximum number of tiles");
    URHO3D_ACTION_STATIC_LABEL("Allocate!", Allocate, "Allocates empty navigation mesh with specified maximum number of tiles");

    URHO3D_ACCESSOR_ATTRIBUTE("Max Tiles", GetMaxTiles, SetMaxTiles, int, DefaultMaxTiles, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Tile Size", GetTileSize, SetTileSize, int, DEFAULT_TILE_SIZE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Cell Size", GetCellSize, SetCellSize, float, DEFAULT_CELL_SIZE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Cell Height", GetCellHeight, SetCellHeight, float, DEFAULT_CELL_HEIGHT, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Mesh Height Range", GetHeightRange, SetHeightRange, Vector2, Vector2{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Agent Height", GetAgentHeight, SetAgentHeight, float, DEFAULT_AGENT_HEIGHT, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Agent Radius", GetAgentRadius, SetAgentRadius, float, DEFAULT_AGENT_RADIUS, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Agent Max Climb", GetAgentMaxClimb, SetAgentMaxClimb, float, DEFAULT_AGENT_MAX_CLIMB, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Agent Max Slope", GetAgentMaxSlope, SetAgentMaxSlope, float, DEFAULT_AGENT_MAX_SLOPE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Region Min Size", GetRegionMinSize, SetRegionMinSize, float, DEFAULT_REGION_MIN_SIZE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Region Merge Size", GetRegionMergeSize, SetRegionMergeSize, float, DEFAULT_REGION_MERGE_SIZE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Edge Max Length", GetEdgeMaxLength, SetEdgeMaxLength, float, DEFAULT_EDGE_MAX_LENGTH, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Edge Max Error", GetEdgeMaxError, SetEdgeMaxError, float, DEFAULT_EDGE_MAX_ERROR, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Detail Sample Distance", GetDetailSampleDistance, SetDetailSampleDistance, float,
        DEFAULT_DETAIL_SAMPLE_DISTANCE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Detail Sample Max Error", GetDetailSampleMaxError, SetDetailSampleMaxError, float,
        DEFAULT_DETAIL_SAMPLE_MAX_ERROR, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Bounding Box Padding", GetPadding, SetPadding, Vector3, Vector3::ONE, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Navigation Data", GetNavigationDataAttr, SetNavigationDataAttr, ea::vector<unsigned char>,
        Variant::emptyBuffer, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Partition Type", GetPartitionType, SetPartitionType, NavmeshPartitionType, navmeshPartitionTypeNames,
        NAVMESH_PARTITION_WATERSHED, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw OffMeshConnections", GetDrawOffMeshConnections, SetDrawOffMeshConnections, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw NavAreas", GetDrawNavAreas, SetDrawNavAreas, bool, false, AM_DEFAULT);
}

void NavigationMesh::DrawDebugTileGeometry(DebugRenderer* debug, bool depthTest, int tileIndex)
{
    static const Color polygonEdgeColor = 0x7fffff00_argb;
    static const Color polygonLinkColor = 0x7f00ff00_argb;

    if (tileIndex >= navMesh_->getMaxTiles())
        return;

    const dtNavMesh& navMesh = *navMesh_;
    const dtMeshTile& tile = *navMesh.getTile(tileIndex);
    if (!tile.header)
        return;

    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    for (int polyIndex = 0; polyIndex < tile.header->polyCount; ++polyIndex)
    {
        const dtPoly& poly = tile.polys[polyIndex];
        const auto [polyVertices, polyCenter] = GetPolygonVerticesAndCenter(tile, poly);

        for (unsigned i = 0; i < poly.vertCount; ++i)
        {
            const Vector3& firstVertex = polyVertices[i];
            const Vector3& secondVertex = polyVertices[(i + 1) % poly.vertCount];
            debug->AddLine(worldTransform * firstVertex, worldTransform * secondVertex, polygonEdgeColor, depthTest);
        }

        for (unsigned link = poly.firstLink; link != DT_NULL_LINK; link = tile.links[link].next)
        {
            const dtLink& linkData = tile.links[link];

            const dtMeshTile* otherTile = nullptr;
            const dtPoly* otherPoly = nullptr;
            if (!dtStatusSucceed(navMesh.getTileAndPolyByRef(linkData.ref, &otherTile, &otherPoly)))
                continue;

            const auto [_, otherPolyCenter] = GetPolygonVerticesAndCenter(*otherTile, *otherPoly);
            debug->AddLine(worldTransform * polyCenter, worldTransform * otherPolyCenter, polygonLinkColor, depthTest);
        }
    }
}

void NavigationMesh::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!debug || !navMesh_ || !node_)
        return;

    for (int j = 0; j < navMesh_->getMaxTiles(); ++j)
        DrawDebugTileGeometry(debug, depthTest, j);

    Scene* scene = GetScene();
    if (scene)
    {
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
            for (unsigned i = 0; i < areas_.size(); ++i)
            {
                NavArea* area = areas_[i];
                if (area && area->IsEnabledEffective())
                    area->DrawDebugGeometry(debug, depthTest);
            }
        }
    }
}

void NavigationMesh::SetMeshName(const ea::string& newName)
{
    meshName_ = newName;
}

void NavigationMesh::SetTileSize(int size)
{
    tileSize_ = Max(size, 16);
}

void NavigationMesh::SetCellSize(float size)
{
    cellSize_ = Max(size, M_EPSILON);
}

void NavigationMesh::SetCellHeight(float height)
{
    cellHeight_ = Max(height, M_EPSILON);
}

void NavigationMesh::SetAgentHeight(float height)
{
    agentHeight_ = Max(height, M_EPSILON);
}

void NavigationMesh::SetAgentRadius(float radius)
{
    agentRadius_ = Max(radius, M_EPSILON);
}

void NavigationMesh::SetAgentMaxClimb(float maxClimb)
{
    agentMaxClimb_ = Max(maxClimb, M_EPSILON);
}

void NavigationMesh::SetAgentMaxSlope(float maxSlope)
{
    agentMaxSlope_ = Max(maxSlope, 0.0f);
}

void NavigationMesh::SetRegionMinSize(float size)
{
    regionMinSize_ = Max(size, M_EPSILON);
}

void NavigationMesh::SetRegionMergeSize(float size)
{
    regionMergeSize_ = Max(size, M_EPSILON);
}

void NavigationMesh::SetEdgeMaxLength(float length)
{
    edgeMaxLength_ = Max(length, M_EPSILON);
}

void NavigationMesh::SetEdgeMaxError(float error)
{
    edgeMaxError_ = Max(error, M_EPSILON);
}

void NavigationMesh::SetDetailSampleDistance(float distance)
{
    detailSampleDistance_ = Max(distance, M_EPSILON);
}

void NavigationMesh::SetDetailSampleMaxError(float error)
{
    detailSampleMaxError_ = Max(error, M_EPSILON);
}

void NavigationMesh::SetPadding(const Vector3& padding)
{
    padding_ = padding;
}

bool NavigationMesh::AllocateMesh(unsigned maxTiles)
{
    // Release existing navigation data
    ReleaseNavigationMesh();

    if (!node_)
        return false;

    // Calculate max number of polygons, 22 bits available to identify both tile & polygon within tile
    const unsigned tileBits = LogBaseTwo(maxTiles);
    const unsigned maxPolys = 1u << (22 - tileBits);
    const float tileEdgeLength = tileSize_ * cellSize_;

    dtNavMeshParams params{};
    // TODO: Fill params.orig
    params.tileWidth = tileEdgeLength;
    params.tileHeight = tileEdgeLength;
    params.maxTiles = maxTiles;
    params.maxPolys = maxPolys;

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        URHO3D_LOGERROR("Could not allocate navigation mesh");
        return false;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        URHO3D_LOGERROR("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return false;
    }

    return true;
}

void NavigationMesh::Clear()
{
    ReleaseNavigationMesh();
}

bool NavigationMesh::Allocate()
{
    URHO3D_PROFILE("AllocateNavigationMesh");

    if (!AllocateMesh(maxTiles_))
        return false;

    URHO3D_LOGDEBUG("Allocated empty navigation mesh with max {} tiles", maxTiles_);
    SendRebuildEvent();
    return true;
}

void NavigationMesh::SendRebuildEvent()
{
    using namespace NavigationMeshRebuilt;
    VariantMap& buildEventParams = GetContext()->GetEventDataMap();
    buildEventParams[P_NODE] = node_;
    buildEventParams[P_MESH] = this;
    SendEvent(E_NAVIGATION_MESH_REBUILT, buildEventParams);
}

bool NavigationMesh::RebuildMesh()
{
    // Collect geometry and update dimensions
    ea::vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    const BoundingBox boundingBox = CalculateBoundingBox(geometryList, padding_);
    const unsigned maxTiles = CalculateMaxTiles(boundingBox, tileSize_, cellSize_);

    maxTiles_ = maxTiles ? static_cast<int>(maxTiles) : DefaultMaxTiles;

    // Allocate navigation mesh
    if (!AllocateMesh(maxTiles_))
        return false;

    // Build navigation mesh
    const IntVector2 beginTileIndex = GetTileIndex(boundingBox.min_);
    const IntVector2 endTileIndex = GetTileIndex(boundingBox.max_);

    BuildTilesFromGeometry(geometryList, beginTileIndex, endTileIndex);
    return true;
}

bool NavigationMesh::Rebuild()
{
    URHO3D_PROFILE("BuildNavigationMesh");

    if (!RebuildMesh())
        return false;

    URHO3D_LOGDEBUG("Built navigation mesh with max {} tiles", maxTiles_);
    SendRebuildEvent();
    return true;
}

bool NavigationMesh::BuildTilesInRegion(const BoundingBox& boundingBox)
{
    URHO3D_PROFILE("BuildPartialNavigationMesh");

    if (!node_)
        return false;

    ea::vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    const IntVector2 beginTileIndex = GetTileIndex(boundingBox.min_);
    const IntVector2 endTileIndex = GetTileIndex(boundingBox.max_);

    const unsigned numTiles = BuildTilesFromGeometry(geometryList, beginTileIndex, endTileIndex);
    URHO3D_LOGDEBUG("Rebuilt {} tiles of the navigation mesh", numTiles);

    for (const IntVector2& tileIndex : IntRect{beginTileIndex, endTileIndex + IntVector2::ONE})
        SendTileAddedEvent(tileIndex);

    return true;
}

bool NavigationMesh::BuildTiles(const IntVector2& from, const IntVector2& to)
{
    URHO3D_PROFILE("BuildPartialNavigationMesh");

    if (!node_)
        return false;

    if (!navMesh_)
    {
        URHO3D_LOGERROR("Navigation mesh must first be built or allocated before it can be partially rebuilt");
        return false;
    }

    ea::vector<NavigationGeometryInfo> geometryList;
    CollectGeometries(geometryList);

    unsigned numTiles = BuildTilesFromGeometry(geometryList, from, to);
    URHO3D_LOGDEBUG("Rebuilt {} tiles of the navigation mesh", numTiles);

    for (const IntVector2& tileIndex : IntRect{from, to + IntVector2::ONE})
        SendTileAddedEvent(tileIndex);

    return true;
}

ea::vector<IntVector2> NavigationMesh::GetAllTileIndices() const
{
    ea::vector<IntVector2> result;
    for (int i = 0; i < navMesh_->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = const_cast<const dtNavMesh*>(navMesh_)->getTile(i);
        if (!tile->header || !tile->dataSize)
            continue;

        result.emplace_back(tile->header->x, tile->header->y);
    }
    return result;
}

ea::vector<unsigned char> NavigationMesh::GetTileData(const IntVector2& tileIndex) const
{
    const dtMeshTile* tile = navMesh_->getTileAt(tileIndex.x_, tileIndex.y_, 0);
    if (!tile || !tile->header || !tile->dataSize)
        return {};

    VectorBuffer ret;
    WriteTile(ret, tile);
    return ret.GetBuffer();
}

bool NavigationMesh::AddTile(const ea::vector<unsigned char>& tileData)
{
    MemoryBuffer buffer(tileData);
    return ReadTile(buffer, false);
}

bool NavigationMesh::HasTile(const IntVector2& tileIndex) const
{
    if (navMesh_)
        return navMesh_->getTilesAt(tileIndex.x_, tileIndex.y_, static_cast<dtMeshTile const**>(nullptr), 0) > 0;
    return false;
}

BoundingBox NavigationMesh::GetTileBoundingBoxColumn(const IntVector2& tileIndex) const
{
    const Vector2 heightRange = IsHeightRangeValid() ? heightRange_ : Vector2{-M_LARGE_VALUE, M_LARGE_VALUE};
    const float tileEdgeLength = tileSize_ * cellSize_;
    const Vector3 minPosition{tileIndex.x_ * tileEdgeLength, heightRange.x_, tileIndex.y_ * tileEdgeLength};
    const Vector3 maxPosition{(tileIndex.x_ + 1) * tileEdgeLength, heightRange.y_, (tileIndex.y_ + 1) * tileEdgeLength};
    return BoundingBox{minPosition, maxPosition};
}

IntVector2 NavigationMesh::GetTileIndex(const Vector3& position) const
{
    const float tileEdgeLength = tileSize_ * cellSize_;
    return VectorFloorToInt(position.ToXZ() / tileEdgeLength);
}

void NavigationMesh::RemoveTile(const IntVector2& tileIndex)
{
    if (!navMesh_)
        return;

    const dtMeshTile* tilesToRemove[MaxLayers];
    const int numTilesToRemove = navMesh_->getTilesAt(tileIndex.x_, tileIndex.y_, tilesToRemove, MaxLayers);
    for (int i = 0; i < numTilesToRemove; ++i)
    {
        const dtTileRef tileRef = navMesh_->getTileRefAt(tileIndex.x_, tileIndex.y_, tilesToRemove[i]->header->layer);
        navMesh_->removeTile(tileRef, nullptr, nullptr);
    }

    // Send event
    if (numTilesToRemove > 0)
    {
        using namespace NavigationTileRemoved;
        VariantMap& eventData = GetContext()->GetEventDataMap();
        eventData[P_NODE] = GetNode();
        eventData[P_MESH] = this;
        eventData[P_TILE] = tileIndex;
        SendEvent(E_NAVIGATION_TILE_REMOVED, eventData);
    }
}

void NavigationMesh::RemoveAllTiles()
{
    const dtNavMesh* navMesh = navMesh_;
    for (int i = 0; i < navMesh_->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        assert(tile);
        if (tile->header)
            navMesh_->removeTile(navMesh_->getTileRef(tile), nullptr, nullptr);
    }

    // Send event
    using namespace NavigationAllTilesRemoved;
    VariantMap& eventData = GetContext()->GetEventDataMap();
    eventData[P_NODE] = GetNode();
    eventData[P_MESH] = this;
    SendEvent(E_NAVIGATION_ALL_TILES_REMOVED, eventData);
}

Vector3 NavigationMesh::FindNearestPoint(const Vector3& point, const Vector3& extents, const dtQueryFilter* filter,
    dtPolyRef* nearestRef)
{
    if (!InitializeQuery())
        return point;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localPoint = inverse * point;
    Vector3 nearestPoint;

    dtPolyRef pointRef;
    if (!nearestRef)
        nearestRef = &pointRef;
    navMeshQuery_->findNearestPoly(&localPoint.x_, &extents.x_, filter ? filter : queryFilter_.get(), nearestRef, &nearestPoint.x_);
    return *nearestRef ? transform * nearestPoint : point;
}

Vector3 NavigationMesh::MoveAlongSurface(const Vector3& start, const Vector3& end, const Vector3& extents, int maxVisited,
    const dtQueryFilter* filter)
{
    if (!InitializeQuery())
        return end;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return end;

    Vector3 resultPos;
    int visitedCount = 0;
    maxVisited = Max(maxVisited, 0);
    ea::vector<dtPolyRef> visited((unsigned)maxVisited);
    navMeshQuery_->moveAlongSurface(startRef, &localStart.x_, &localEnd.x_, queryFilter, &resultPos.x_, maxVisited ?
        &visited[0] : nullptr, &visitedCount, maxVisited);
    return transform * resultPos;
}

void NavigationMesh::FindPath(ea::vector<Vector3>& dest, const Vector3& start, const Vector3& end, const Vector3& extents,
    const dtQueryFilter* filter)
{
    ea::vector<NavigationPathPoint> navPathPoints;
    FindPath(navPathPoints, start, end, extents, filter);

    dest.clear();
    for (unsigned i = 0; i < navPathPoints.size(); ++i)
        dest.push_back(navPathPoints[i].position_);
}

void NavigationMesh::FindPath(ea::vector<NavigationPathPoint>& dest, const Vector3& start, const Vector3& end,
    const Vector3& extents, const dtQueryFilter* filter)
{
    URHO3D_PROFILE("FindPath");
    dest.clear();

    if (!InitializeQuery())
        return;

    // Navigation data is in local space. Transform path points from world to local
    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.get();
    dtPolyRef startRef;
    dtPolyRef endRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    navMeshQuery_->findNearestPoly(&localEnd.x_, &extents.x_, queryFilter, &endRef, nullptr);

    if (!startRef || !endRef)
        return;

    int numPolys = 0;
    int numPathPoints = 0;

    navMeshQuery_->findPath(startRef, endRef, &localStart.x_, &localEnd.x_, queryFilter, pathData_->polys_, &numPolys,
        MAX_POLYS);
    if (!numPolys)
        return;

    Vector3 actualLocalEnd = localEnd;

    // If full path was not found, clamp end point to the end polygon
    if (pathData_->polys_[numPolys - 1] != endRef)
        navMeshQuery_->closestPointOnPoly(pathData_->polys_[numPolys - 1], &localEnd.x_, &actualLocalEnd.x_, nullptr);

    navMeshQuery_->findStraightPath(&localStart.x_, &actualLocalEnd.x_, pathData_->polys_, numPolys,
        &pathData_->pathPoints_[0].x_, pathData_->pathFlags_, pathData_->pathPolys_, &numPathPoints, MAX_POLYS);

    // Transform path result back to world space
    for (int i = 0; i < numPathPoints; ++i)
    {
        NavigationPathPoint pt;
        pt.position_ = transform * pathData_->pathPoints_[i];
        pt.flag_ = (NavigationPathPointFlag)pathData_->pathFlags_[i];

        // Walk through all NavAreas and find nearest
        unsigned nearestNavAreaID = 0;       // 0 is the default nav area ID
        float nearestDistance = M_LARGE_VALUE;
        for (unsigned j = 0; j < areas_.size(); j++)
        {
            NavArea* area = areas_[j];
            if (area && area->IsEnabledEffective())
            {
                BoundingBox bb = area->GetWorldBoundingBox();
                if (bb.IsInside(pt.position_) == INSIDE)
                {
                    Vector3 areaWorldCenter = area->GetNode()->GetWorldPosition();
                    float distance = (areaWorldCenter - pt.position_).LengthSquared();
                    if (distance < nearestDistance)
                    {
                        nearestDistance = distance;
                        nearestNavAreaID = area->GetAreaID();
                    }
                }
            }
        }
        pt.areaID_ = (unsigned char)nearestNavAreaID;

        dest.push_back(pt);
    }
}

Vector3 NavigationMesh::GetRandomPoint(const dtQueryFilter* filter, dtPolyRef* randomRef)
{
    if (!InitializeQuery())
        return Vector3::ZERO;

    dtPolyRef polyRef;
    Vector3 point(Vector3::ZERO);

    navMeshQuery_->findRandomPoint(filter ? filter : queryFilter_.get(), Random, randomRef ? randomRef : &polyRef, &point.x_);

    return node_->GetWorldTransform() * point;
}

Vector3 NavigationMesh::GetRandomPointInCircle(const Vector3& center, float radius, const Vector3& extents,
    const dtQueryFilter* filter, dtPolyRef* randomRef)
{
    if (randomRef)
        *randomRef = 0;

    if (!InitializeQuery())
        return center;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();
    Vector3 localCenter = inverse * center;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localCenter.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return center;

    dtPolyRef polyRef;
    if (!randomRef)
        randomRef = &polyRef;
    Vector3 point(localCenter);

    navMeshQuery_->findRandomPointAroundCircle(startRef, &localCenter.x_, radius, queryFilter, Random, randomRef, &point.x_);

    return transform * point;
}

float NavigationMesh::GetDistanceToWall(const Vector3& point, float radius, const Vector3& extents, const dtQueryFilter* filter,
    Vector3* hitPos, Vector3* hitNormal)
{
    if (hitPos)
        *hitPos = Vector3::ZERO;
    if (hitNormal)
        *hitNormal = Vector3::DOWN;

    if (!InitializeQuery())
        return radius;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();
    Vector3 localPoint = inverse * point;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localPoint.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return radius;

    float hitDist = radius;
    Vector3 pos;
    if (!hitPos)
        hitPos = &pos;
    Vector3 normal;
    if (!hitNormal)
        hitNormal = &normal;

    navMeshQuery_->findDistanceToWall(startRef, &localPoint.x_, radius, queryFilter, &hitDist, &hitPos->x_, &hitNormal->x_);
    return hitDist;
}

Vector3 NavigationMesh::Raycast(const Vector3& start, const Vector3& end, const Vector3& extents, const dtQueryFilter* filter,
    Vector3* hitNormal)
{
    if (hitNormal)
        *hitNormal = Vector3::DOWN;

    if (!InitializeQuery())
        return end;

    const Matrix3x4& transform = node_->GetWorldTransform();
    Matrix3x4 inverse = transform.Inverse();

    Vector3 localStart = inverse * start;
    Vector3 localEnd = inverse * end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_.get();
    dtPolyRef startRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    if (!startRef)
        return end;

    Vector3 normal;
    if (!hitNormal)
        hitNormal = &normal;
    float t;
    int numPolys;

    navMeshQuery_->raycast(startRef, &localStart.x_, &localEnd.x_, queryFilter, &t, &hitNormal->x_, pathData_->polys_, &numPolys,
        MAX_POLYS);
    if (t == FLT_MAX)
        t = 1.0f;

    return start.Lerp(end, t);
}

void NavigationMesh::DrawDebugGeometry(bool depthTest)
{
    Scene* scene = GetScene();
    if (scene)
    {
        auto* debug = scene->GetComponent<DebugRenderer>();
        if (debug)
            DrawDebugGeometry(debug, depthTest);
    }
}

void NavigationMesh::SetAreaCost(unsigned areaID, float cost)
{
    if (queryFilter_)
        queryFilter_->setAreaCost((int)areaID, cost);
}

float NavigationMesh::GetAreaCost(unsigned areaID) const
{
    if (queryFilter_)
        return queryFilter_->getAreaCost((int)areaID);
    return 1.0f;
}

void NavigationMesh::SetNavigationDataAttr(const ea::vector<unsigned char>& value)
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

    dtNavMeshParams params{};     // NOLINT(hicpp-member-init)
    // TODO: Fill params.orig
    params.tileWidth = buffer.ReadFloat();
    params.tileHeight = buffer.ReadFloat();
    params.maxTiles = buffer.ReadInt();
    params.maxPolys = buffer.ReadInt();

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

    unsigned numTiles = 0;

    while (!buffer.IsEof())
    {
        if (ReadTile(buffer, true))
            ++numTiles;
        else
            return;
    }

    URHO3D_LOGDEBUG("Created navigation mesh with {} tiles from serialized data", numTiles);
    // \todo Shall we send E_NAVIGATION_MESH_REBUILT here?
}

ea::vector<unsigned char> NavigationMesh::GetNavigationDataAttr() const
{
    VectorBuffer ret;

    if (navMesh_)
    {
        ret.WriteBoundingBox(BoundingBox{});
        ret.WriteInt(0);
        ret.WriteInt(0);
        ret.WriteInt(NavigationDataVersion);

        const dtNavMeshParams* params = navMesh_->getParams();
        ret.WriteFloat(params->tileWidth);
        ret.WriteFloat(params->tileHeight);
        ret.WriteInt(params->maxTiles);
        ret.WriteInt(params->maxPolys);

        for (int i = 0; i < navMesh_->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = const_cast<const dtNavMesh*>(navMesh_)->getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;

            WriteTile(ret, tile);
        }
    }

    return ret.GetBuffer();
}

void NavigationMesh::CollectGeometries(ea::vector<NavigationGeometryInfo>& geometryList)
{
    URHO3D_PROFILE("CollectNavigationGeometry");

    // Get Navigable components from child nodes, not from whole scene. This makes it possible to partition
    // the scene into several navigation meshes
    ea::vector<Navigable*> navigables;
    node_->FindComponents<Navigable>(navigables);

    ea::hash_set<Node*> processedNodes;
    for (unsigned i = 0; i < navigables.size(); ++i)
    {
        if (navigables[i]->IsEnabledEffective())
        {
            CollectGeometries(
                geometryList, navigables[i], navigables[i]->GetNode(), processedNodes, navigables[i]->IsRecursive());
        }
    }

    // Get offmesh connections
    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();
    ea::vector<OffMeshConnection*> connections;
    node_->FindComponents<OffMeshConnection>(connections);

    for (unsigned i = 0; i < connections.size(); ++i)
    {
        OffMeshConnection* connection = connections[i];
        if (connection->IsEnabledEffective() && connection->GetEndPoint())
        {
            const Matrix3x4& transform = connection->GetNode()->GetWorldTransform();

            NavigationGeometryInfo info;
            info.component_ = connection;
            info.boundingBox_ = BoundingBox(Sphere(transform.Translation(), connection->GetRadius())).Transformed(inverse);

            geometryList.push_back(info);
        }
    }

    // Get nav area volumes
    ea::vector<NavArea*> navAreas;
    node_->FindComponents<NavArea>(navAreas);
    areas_.clear();
    for (unsigned i = 0; i < navAreas.size(); ++i)
    {
        NavArea* area = navAreas[i];
        if (area->IsEnabledEffective())
        {
            NavigationGeometryInfo info;
            info.component_ = area;
            info.boundingBox_ = area->GetWorldBoundingBox();
            geometryList.push_back(info);
            areas_.push_back(WeakPtr<NavArea>(area));
        }
    }
}

void NavigationMesh::CollectGeometries(ea::vector<NavigationGeometryInfo>& geometryList, Navigable* navigable,
    Node* node, ea::hash_set<Node*>& processedNodes, bool recursive)
{
    // Make sure nodes are not included twice
    if (processedNodes.contains(node))
        return;
    // Exclude obstacles and crowd agents from consideration
    if (node->HasComponent<Obstacle>() || node->HasComponent<CrowdAgent>())
        return;
    processedNodes.insert(node);

    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();

#ifdef URHO3D_PHYSICS
    // Prefer compatible physics collision shapes (triangle mesh, convex hull, box) if found.
    // Then fallback to visible geometry
    ea::vector<CollisionShape*> collisionShapes;
    node->GetComponents<CollisionShape>(collisionShapes);
    bool collisionShapeFound = false;

    for (unsigned i = 0; i < collisionShapes.size(); ++i)
    {
        CollisionShape* shape = collisionShapes[i];
        if (!shape->IsEnabledEffective())
            continue;

        ShapeType type = shape->GetShapeType();
        if ((type == SHAPE_BOX || type == SHAPE_TRIANGLEMESH || type == SHAPE_CONVEXHULL) && shape->GetCollisionShape())
        {
            Matrix3x4 shapeTransform(shape->GetPosition(), shape->GetRotation(), shape->GetSize());

            NavigationGeometryInfo info;
            info.component_ = shape;
            info.transform_ = inverse * node->GetWorldTransform() * shapeTransform;
            info.boundingBox_ = shape->GetWorldBoundingBox().Transformed(inverse);
            info.areaId_ = navigable->GetEffectiveAreaId();

            geometryList.push_back(info);
            collisionShapeFound = true;
        }
    }
    if (!collisionShapeFound)
#endif
    {
        ea::vector<Drawable*> drawables;
        node->FindComponents<Drawable>(drawables, ComponentSearchFlag::Self | ComponentSearchFlag::Derived);

        for (unsigned i = 0; i < drawables.size(); ++i)
        {
            /// \todo Evaluate whether should handle other types. Now StaticModel & TerrainPatch are supported, others skipped
            Drawable* drawable = drawables[i];
            if (!drawable->IsEnabledEffective())
                continue;

            NavigationGeometryInfo info;

            if (drawable->GetType() == StaticModel::GetTypeStatic())
                info.lodLevel_ = static_cast<StaticModel*>(drawable)->GetOcclusionLodLevel();
            else if (drawable->GetType() == TerrainPatch::GetTypeStatic())
                info.lodLevel_ = 0;
            else
                continue;

            info.component_ = drawable;
            info.transform_ = inverse * node->GetWorldTransform();
            info.boundingBox_ = drawable->GetWorldBoundingBox().Transformed(inverse);
            info.areaId_ = navigable->GetEffectiveAreaId();

            geometryList.push_back(info);
        }
    }

    if (recursive)
    {
        const ea::vector<SharedPtr<Node> >& children = node->GetChildren();
        for (unsigned i = 0; i < children.size(); ++i)
            CollectGeometries(geometryList, navigable, children[i], processedNodes, recursive);
    }
}

void NavigationMesh::GetTileGeometry(
    NavBuildData* build, const ea::vector<NavigationGeometryInfo>& geometryList, BoundingBox& box)
{
    Matrix3x4 inverse = node_->GetWorldTransform().Inverse();

    for (unsigned i = 0; i < geometryList.size(); ++i)
    {
        const NavigationGeometryInfo& geometryInfo = geometryList[i];
        if (box.IsInsideFast(geometryInfo.boundingBox_) != OUTSIDE)
        {
            const Matrix3x4& transform = geometryInfo.transform_;

            if (geometryInfo.component_->GetType() == OffMeshConnection::GetTypeStatic())
            {
                auto* connection = static_cast<OffMeshConnection*>(geometryInfo.component_);
                Vector3 start = inverse * connection->GetNode()->GetWorldPosition();
                Vector3 end = inverse * connection->GetEndPoint()->GetWorldPosition();

                build->offMeshVertices_.push_back(start);
                build->offMeshVertices_.push_back(end);
                build->offMeshRadii_.push_back(connection->GetRadius());
                build->offMeshFlags_.push_back((unsigned short) connection->GetMask());
                build->offMeshAreas_.push_back((unsigned char) connection->GetAreaID());
                build->offMeshDir_.push_back((unsigned char) (connection->IsBidirectional() ? DT_OFFMESH_CON_BIDIR : 0));
                continue;
            }
            else if (geometryInfo.component_->GetType() == NavArea::GetTypeStatic())
            {
                auto* area = static_cast<NavArea*>(geometryInfo.component_);
                NavAreaStub stub;
                stub.areaID_ = (unsigned char)area->GetAreaID();
                stub.bounds_ = area->GetWorldBoundingBox();
                build->navAreas_.push_back(stub);
                continue;
            }

#ifdef URHO3D_PHYSICS
            auto* shape = dynamic_cast<CollisionShape*>(geometryInfo.component_);
            if (shape)
            {
                switch (shape->GetShapeType())
                {
                case SHAPE_TRIANGLEMESH:
                    {
                        Model* model = shape->GetModel();
                        if (!model)
                            continue;

                        unsigned lodLevel = shape->GetLodLevel();
                        for (unsigned j = 0; j < model->GetNumGeometries(); ++j)
                            AddTriMeshGeometry(build, model->GetGeometry(j, lodLevel), transform, geometryInfo.areaId_);
                    }
                    break;

                case SHAPE_CONVEXHULL:
                    {
                        auto* data = static_cast<ConvexData*>(shape->GetGeometryData());
                        if (!data)
                            continue;

                        unsigned numVertices = data->vertexCount_;
                        unsigned numIndices = data->indexCount_;
                        unsigned destVertexStart = build->vertices_.size();

                        for (unsigned j = 0; j < numVertices; ++j)
                            build->vertices_.push_back(transform * data->vertexData_[j]);

                        for (unsigned j = 0; j < numIndices; ++j)
                            build->indices_.push_back(data->indexData_[j] + destVertexStart);

                        build->areaIds_.insert(build->areaIds_.end(), numIndices / 3, geometryInfo.areaId_);
                    }
                    break;

                case SHAPE_BOX:
                    {
                        unsigned destVertexStart = build->vertices_.size();

                        build->vertices_.push_back(transform * Vector3(-0.5f, 0.5f, -0.5f));
                        build->vertices_.push_back(transform * Vector3(0.5f, 0.5f, -0.5f));
                        build->vertices_.push_back(transform * Vector3(0.5f, -0.5f, -0.5f));
                        build->vertices_.push_back(transform * Vector3(-0.5f, -0.5f, -0.5f));
                        build->vertices_.push_back(transform * Vector3(-0.5f, 0.5f, 0.5f));
                        build->vertices_.push_back(transform * Vector3(0.5f, 0.5f, 0.5f));
                        build->vertices_.push_back(transform * Vector3(0.5f, -0.5f, 0.5f));
                        build->vertices_.push_back(transform * Vector3(-0.5f, -0.5f, 0.5f));

                        const unsigned indices[] = {
                            0, 1, 2, 0, 2, 3, 1, 5, 6, 1, 6, 2, 4, 5, 1, 4, 1, 0, 5, 4, 7, 5, 7, 6,
                            4, 0, 3, 4, 3, 7, 1, 0, 4, 1, 4, 5
                        };

                        for (unsigned index : indices)
                            build->indices_.push_back(index + destVertexStart);

                        build->areaIds_.insert(build->areaIds_.end(), std::size(indices) / 3, geometryInfo.areaId_);
                    }
                    break;

                default:
                    break;
                }

                continue;
            }
#endif
            auto* drawable = dynamic_cast<Drawable*>(geometryInfo.component_);
            if (drawable)
            {
                const ea::vector<SourceBatch>& batches = drawable->GetBatches();

                for (unsigned j = 0; j < batches.size(); ++j)
                {
                    AddTriMeshGeometry(
                        build, drawable->GetLodGeometry(j, geometryInfo.lodLevel_), transform, geometryInfo.areaId_);
                }
            }
        }
    }
}

void NavigationMesh::AddTriMeshGeometry(
    NavBuildData* build, Geometry* geometry, const Matrix3x4& transform, unsigned char areaId)
{
    if (!geometry)
        return;

    const unsigned char* vertexData;
    const unsigned char* indexData;
    unsigned vertexSize;
    unsigned indexSize;
    const ea::vector<VertexElement>* elements;

    geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elements);
    if (!vertexData || !indexData || !elements || VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_POSITION) != 0)
        return;

    unsigned srcIndexStart = geometry->GetIndexStart();
    unsigned srcIndexCount = geometry->GetIndexCount();
    unsigned srcVertexStart = geometry->GetVertexStart();
    unsigned srcVertexCount = geometry->GetVertexCount();

    if (!srcIndexCount)
        return;

    unsigned destVertexStart = build->vertices_.size();

    for (unsigned k = srcVertexStart; k < srcVertexStart + srcVertexCount; ++k)
    {
        Vector3 vertex = transform * *((const Vector3*)(&vertexData[k * vertexSize]));
        build->vertices_.push_back(vertex);
    }

    // Copy remapped indices
    if (indexSize == sizeof(unsigned short))
    {
        const unsigned short* indices = ((const unsigned short*)indexData) + srcIndexStart;
        const unsigned short* indicesEnd = indices + srcIndexCount;

        while (indices < indicesEnd)
        {
            build->indices_.push_back(*indices - srcVertexStart + destVertexStart);
            ++indices;
        }
    }
    else
    {
        const unsigned* indices = ((const unsigned*)indexData) + srcIndexStart;
        const unsigned* indicesEnd = indices + srcIndexCount;

        while (indices < indicesEnd)
        {
            build->indices_.push_back(*indices - srcVertexStart + destVertexStart);
            ++indices;
        }
    }

    build->areaIds_.insert(build->areaIds_.end(), srcIndexCount / 3, areaId);
}

bool NavigationMesh::ReadTile(Deserializer& source, bool silent)
{
    const int x = source.ReadInt();
    const int z = source.ReadInt();
    unsigned navDataSize = source.ReadUInt();

    auto* navData = (unsigned char*)dtAlloc(navDataSize, DT_ALLOC_PERM);
    if (!navData)
    {
        URHO3D_LOGERROR("Could not allocate data for navigation mesh tile");
        return false;
    }

    source.Read(navData, navDataSize);
    if (dtStatusFailed(navMesh_->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr)))
    {
        URHO3D_LOGERROR("Failed to add navigation mesh tile");
        dtFree(navData);
        return false;
    }

    if (!silent)
        SendTileAddedEvent(IntVector2(x, z));

    return true;
}

void NavigationMesh::SendTileAddedEvent(const IntVector2& tileIndex)
{
    using namespace NavigationTileAdded;
    VariantMap& eventData = GetContext()->GetEventDataMap();
    eventData[P_NODE] = GetNode();
    eventData[P_MESH] = this;
    eventData[P_TILE] = tileIndex;
    SendEvent(E_NAVIGATION_TILE_ADDED, eventData);
}

bool NavigationMesh::BuildTile(ea::vector<NavigationGeometryInfo>& geometryList, int x, int z)
{
    URHO3D_PROFILE("BuildNavigationMeshTile");

    // Remove previous tile (if any)
    navMesh_->removeTile(navMesh_->getTileRefAt(x, z, 0), nullptr, nullptr);

    const BoundingBox tileColumn = GetTileBoundingBoxColumn(IntVector2{x, z});
    const BoundingBox tileBoundingBox =
        IsHeightRangeValid() ? tileColumn : CalculateTileBoundingBox(geometryList, tileColumn);

    SimpleNavBuildData build;

    rcConfig cfg;       // NOLINT(hicpp-member-init)
    memset(&cfg, 0, sizeof(cfg));
    cfg.cs = cellSize_;
    cfg.ch = cellHeight_;
    cfg.walkableSlopeAngle = agentMaxSlope_;
    cfg.walkableHeight = CeilToInt(agentHeight_ / cfg.ch);
    cfg.walkableClimb = FloorToInt(agentMaxClimb_ / cfg.ch);
    cfg.walkableRadius = CeilToInt(agentRadius_ / cfg.cs);
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
        return true; // Nothing to do

    build.heightField_ = rcAllocHeightfield();
    if (!build.heightField_)
    {
        URHO3D_LOGERROR("Could not allocate heightfield");
        return false;
    }

    if (!rcCreateHeightfield(build.ctx_, *build.heightField_, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs,
        cfg.ch))
    {
        URHO3D_LOGERROR("Could not create heightfield");
        return false;
    }

    const unsigned numTriangles = build.indices_.size() / 3;
    URHO3D_ASSERT(numTriangles == build.areaIds_.size());

    DeduceAreaIds(cfg.walkableSlopeAngle, &build.vertices_[0].x_, &build.indices_[0], numTriangles, &build.areaIds_[0]);

    rcRasterizeTriangles(build.ctx_, &build.vertices_[0].x_, build.vertices_.size(), &build.indices_[0],
        &build.areaIds_[0], numTriangles, *build.heightField_, cfg.walkableClimb);
    rcFilterLowHangingWalkableObstacles(build.ctx_, cfg.walkableClimb, *build.heightField_);

    rcFilterWalkableLowHeightSpans(build.ctx_, cfg.walkableHeight, *build.heightField_);
    rcFilterLedgeSpans(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_);

    build.compactHeightField_ = rcAllocCompactHeightfield();
    if (!build.compactHeightField_)
    {
        URHO3D_LOGERROR("Could not allocate create compact heightfield");
        return false;
    }
    if (!rcBuildCompactHeightfield(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_,
        *build.compactHeightField_))
    {
        URHO3D_LOGERROR("Could not build compact heightfield");
        return false;
    }
    if (!rcErodeWalkableArea(build.ctx_, cfg.walkableRadius, *build.compactHeightField_))
    {
        URHO3D_LOGERROR("Could not erode compact heightfield");
        return false;
    }

    // Mark area volumes
    for (unsigned i = 0; i < build.navAreas_.size(); ++i)
        rcMarkBoxArea(build.ctx_, &build.navAreas_[i].bounds_.min_.x_, &build.navAreas_[i].bounds_.max_.x_,
            build.navAreas_[i].areaID_, *build.compactHeightField_);

    if (this->partitionType_ == NAVMESH_PARTITION_WATERSHED)
    {
        if (!rcBuildDistanceField(build.ctx_, *build.compactHeightField_))
        {
            URHO3D_LOGERROR("Could not build distance field");
            return false;
        }
        if (!rcBuildRegions(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea,
            cfg.mergeRegionArea))
        {
            URHO3D_LOGERROR("Could not build regions");
            return false;
        }
    }
    else
    {
        if (!rcBuildRegionsMonotone(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
        {
            URHO3D_LOGERROR("Could not build monotone regions");
            return false;
        }
    }

    build.contourSet_ = rcAllocContourSet();
    if (!build.contourSet_)
    {
        URHO3D_LOGERROR("Could not allocate contour set");
        return false;
    }
    if (!rcBuildContours(build.ctx_, *build.compactHeightField_, cfg.maxSimplificationError, cfg.maxEdgeLen,
        *build.contourSet_))
    {
        URHO3D_LOGERROR("Could not create contours");
        return false;
    }

    build.polyMesh_ = rcAllocPolyMesh();
    if (!build.polyMesh_)
    {
        URHO3D_LOGERROR("Could not allocate poly mesh");
        return false;
    }
    if (!rcBuildPolyMesh(build.ctx_, *build.contourSet_, cfg.maxVertsPerPoly, *build.polyMesh_))
    {
        URHO3D_LOGERROR("Could not triangulate contours");
        return false;
    }

    build.polyMeshDetail_ = rcAllocPolyMeshDetail();
    if (!build.polyMeshDetail_)
    {
        URHO3D_LOGERROR("Could not allocate detail mesh");
        return false;
    }
    if (!rcBuildPolyMeshDetail(build.ctx_, *build.polyMesh_, *build.compactHeightField_, cfg.detailSampleDist,
        cfg.detailSampleMaxError, *build.polyMeshDetail_))
    {
        URHO3D_LOGERROR("Could not build detail mesh");
        return false;
    }

    // Set polygon flags
    /// \todo Assignment of flags from navigation areas?
    for (int i = 0; i < build.polyMesh_->npolys; ++i)
    {
        if (build.polyMesh_->areas[i] != RC_NULL_AREA)
            build.polyMesh_->flags[i] = 0x1;
    }

    unsigned char* navData = nullptr;
    int navDataSize = 0;

    dtNavMeshCreateParams params;       // NOLINT(hicpp-member-init)
    memset(&params, 0, sizeof params);
    params.verts = build.polyMesh_->verts;
    params.vertCount = build.polyMesh_->nverts;
    params.polys = build.polyMesh_->polys;
    params.polyAreas = build.polyMesh_->areas;
    params.polyFlags = build.polyMesh_->flags;
    params.polyCount = build.polyMesh_->npolys;
    params.nvp = build.polyMesh_->nvp;
    params.detailMeshes = build.polyMeshDetail_->meshes;
    params.detailVerts = build.polyMeshDetail_->verts;
    params.detailVertsCount = build.polyMeshDetail_->nverts;
    params.detailTris = build.polyMeshDetail_->tris;
    params.detailTriCount = build.polyMeshDetail_->ntris;
    params.walkableHeight = agentHeight_;
    params.walkableRadius = agentRadius_;
    params.walkableClimb = agentMaxClimb_;
    params.tileX = x;
    params.tileY = z;
    rcVcopy(params.bmin, build.polyMesh_->bmin);
    rcVcopy(params.bmax, build.polyMesh_->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;

    // Add off-mesh connections if have them
    if (build.offMeshRadii_.size())
    {
        params.offMeshConCount = build.offMeshRadii_.size();
        params.offMeshConVerts = &build.offMeshVertices_[0].x_;
        params.offMeshConRad = &build.offMeshRadii_[0];
        params.offMeshConFlags = &build.offMeshFlags_[0];
        params.offMeshConAreas = &build.offMeshAreas_[0];
        params.offMeshConDir = &build.offMeshDir_[0];
    }

    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        URHO3D_LOGERROR("Could not build navigation mesh tile data");
        return false;
    }

    if (dtStatusFailed(navMesh_->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr)))
    {
        URHO3D_LOGERROR("Failed to add navigation mesh tile");
        dtFree(navData);
        return false;
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
    return true;
}

unsigned NavigationMesh::BuildTilesFromGeometry(
    ea::vector<NavigationGeometryInfo>& geometryList, const IntVector2& from, const IntVector2& to)
{
    unsigned numTiles = 0;

    for (int z = from.y_; z <= to.y_; ++z)
    {
        for (int x = from.x_; x <= to.x_; ++x)
        {
            if (BuildTile(geometryList, x, z))
                ++numTiles;
        }
    }
    return numTiles;
}

bool NavigationMesh::InitializeQuery()
{
    if (!navMesh_ || !node_)
        return false;

    if (navMeshQuery_)
        return true;

    navMeshQuery_ = dtAllocNavMeshQuery();
    if (!navMeshQuery_)
    {
        URHO3D_LOGERROR("Could not create navigation mesh query");
        return false;
    }

    if (dtStatusFailed(navMeshQuery_->init(navMesh_, MAX_POLYS)))
    {
        URHO3D_LOGERROR("Could not init navigation mesh query");
        return false;
    }

    return true;
}

void NavigationMesh::ReleaseNavigationMesh()
{
    dtFreeNavMesh(navMesh_);
    navMesh_ = nullptr;

    dtFreeNavMeshQuery(navMeshQuery_);
    navMeshQuery_ = nullptr;
}

void NavigationMesh::SetPartitionType(NavmeshPartitionType partitionType)
{
    partitionType_ = partitionType;
}

void RegisterNavigationLibrary(Context* context)
{
    Navigable::RegisterObject(context);
    NavigationMesh::RegisterObject(context);
    OffMeshConnection::RegisterObject(context);
    CrowdAgent::RegisterObject(context);
    CrowdManager::RegisterObject(context);
    DynamicNavigationMesh::RegisterObject(context);
    Obstacle::RegisterObject(context);
    NavArea::RegisterObject(context);
}

}
