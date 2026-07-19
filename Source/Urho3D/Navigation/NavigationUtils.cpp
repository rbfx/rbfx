// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Navigation/NavigationUtils.h"

#include "Urho3D/Graphics/Drawable.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/Graphics/TerrainPatch.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Navigation/NavArea.h"
#include "Urho3D/Navigation/OffMeshConnection.h"
#include "Urho3D/Scene/Node.h"
#ifdef URHO3D_PHYSICS
    #include "Urho3D/Physics/CollisionShape.h"
#endif

#include <Detour/DetourAlloc.h>
#include <Recast/Recast.h>

namespace Urho3D
{

DetourAllocation ReadDetourBuffer(Deserializer& source)
{
    const int dataSize = source.ReadInt();
    auto* data = reinterpret_cast<unsigned char*>(dtAlloc(dataSize, DT_ALLOC_PERM));
    if (!data)
        return {};

    source.Read(data, dataSize);
    return DetourAllocation{data, dataSize};
}

void WriteDetourBuffer(Serializer& dest, const DetourAllocation& buffer)
{
    WriteDetourBuffer(dest, buffer.data_.get(), buffer.dataSize_);
}

void WriteDetourBuffer(Serializer& dest, const unsigned char* data, int dataSize)
{
    WriteDetourBuffer(dest, ConstByteSpan{data, data + dataSize});
}

void WriteDetourBuffer(Serializer& dest, const ConstByteSpan& buffer)
{
    dest.WriteInt(buffer.size());
    if (buffer.size() != 0)
        dest.Write(buffer.data(), static_cast<unsigned>(buffer.size()));
}

ea::optional<ea::pair<IntVector2, int>> CalculateTileOffset(const IntVector3& delta, int tileSize, float cellSize)
{
    const int tileEdgeLength = RoundToInt(tileSize * cellSize);
    if (tileEdgeLength <= 0 || !Equals(tileSize * cellSize, static_cast<float>(tileEdgeLength)))
    {
        URHO3D_LOGERROR("Tile width should be integer for NavigationMesh rebase: {} * {} = {}", tileSize, cellSize,
            tileSize * cellSize);
        return ea::nullopt;
    }

    if (delta.x_ % tileEdgeLength != 0 || delta.z_ % tileEdgeLength != 0)
    {
        URHO3D_LOGERROR("Delta must be a multiple of navigation mesh tile width for NavigationMesh rebase: {}/{}",
            delta.ToString(), tileEdgeLength);
        return ea::nullopt;
    }

    const IntVector3 tileDelta = delta / tileEdgeLength;
    return ea::make_pair(IntVector2{tileDelta.x_, tileDelta.z_}, delta.y_);
}

BoundingBox CalculateBoundingBox(const NavigationGeometryInfoVector& geometryList, const Vector3& padding)
{
    BoundingBox result;
    for (unsigned i = 0; i < geometryList.size(); ++i)
        result.Merge(geometryList[i].boundingBox_);

    result.min_ -= padding;
    result.max_ += padding;

    return result;
}

BoundingBox CalculateTileBoundingBox(const NavigationGeometryInfoVector& geometryList, const BoundingBox& tileColumn)
{
    BoundingBox result = tileColumn;
    result.min_.y_ = M_LARGE_VALUE;
    result.max_.y_ = -M_LARGE_VALUE;

    for (const NavigationGeometryInfo& info : geometryList)
    {
        if (info.boundingBox_.IsInside(tileColumn) == OUTSIDE)
            continue;

        result.min_.y_ = ea::min(result.min_.y_, info.boundingBox_.min_.y_);
        result.max_.y_ = ea::max(result.max_.y_, info.boundingBox_.max_.y_);
    }

    // Repair empty bounding box
    if (result.max_.y_ < result.min_.y_)
    {
        result.min_.y_ = 0.0f;
        result.max_.y_ = 0.0f;
    }

    return result;
}

unsigned CalculateMaxTiles(const BoundingBox& boundingBox, int tileSize, float cellSize)
{
    if (!boundingBox.Defined())
        return 0;

    const float tileEdgeLength = tileSize * cellSize;
    const IntVector2 beginTileIndex = VectorFloorToInt(boundingBox.min_.ToXZ() / tileEdgeLength);
    const IntVector2 endTileIndex = VectorFloorToInt(boundingBox.max_.ToXZ() / tileEdgeLength);
    const IntVector2 numTiles = endTileIndex - beginTileIndex + IntVector2::ONE;

    return NextPowerOfTwo(static_cast<unsigned>(numTiles.x_ * numTiles.y_));
}

void DeduceAreaIds(
    float walkableSlopeAngle, const float* vertices, const int* triangles, int numTriangles, unsigned char* areas)
{
    const float walkableThreshold = cosf(walkableSlopeAngle / 180.0f * M_PI);
    for (int i = 0; i < numTriangles; ++i)
    {
        if (areas[i] != DeduceAreaId)
            continue;

        const int* triangle = &triangles[i * 3];
        const Vector3 v0{&vertices[triangle[0] * 3]};
        const Vector3 v1{&vertices[triangle[1] * 3]};
        const Vector3 v2{&vertices[triangle[2] * 3]};
        const Vector3 normal = (v1 - v0).CrossProduct(v2 - v0).Normalized();
        areas[i] = normal.y_ > walkableThreshold ? RC_WALKABLE_AREA : 0;
    }
}

void AppendNavigationGeometry(
    NavigationGeometryInfoVector& geometryList, Node* node, const Matrix3x4& inverseRootTransform, unsigned char areaId)
{
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
            info.transform_ = inverseRootTransform * node->GetWorldTransform() * shapeTransform;
            info.boundingBox_ = shape->GetWorldBoundingBox().Transformed(inverseRootTransform);
            info.areaId_ = areaId;

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
            /// \todo Evaluate whether should handle other types. Now StaticModel & TerrainPatch are supported, others
            /// skipped
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
            info.transform_ = inverseRootTransform * node->GetWorldTransform();
            info.boundingBox_ = drawable->GetWorldBoundingBox().Transformed(inverseRootTransform);
            info.areaId_ = areaId;

            geometryList.push_back(info);
        }
    }
}

void AppendOffMessConnection(NavigationGeometryInfoVector& geometryList, OffMeshConnection* offMeshConnection,
    const Matrix3x4& inverseRootTransform)
{
    const Matrix3x4& transform = offMeshConnection->GetNode()->GetWorldTransform();
    const BoundingBox boundingBox{Sphere(transform.Translation(), offMeshConnection->GetRadius())};

    NavigationGeometryInfo info;
    info.component_ = offMeshConnection;
    info.boundingBox_ = boundingBox.Transformed(inverseRootTransform);

    geometryList.push_back(info);
}

void AppendNavArea(NavigationGeometryInfoVector& geometryList, NavArea* navArea, const Matrix3x4& inverseRootTransform)
{
    NavigationGeometryInfo info;
    info.component_ = navArea;
    info.boundingBox_ = navArea->GetWorldBoundingBox().Transformed(inverseRootTransform);
    geometryList.push_back(info);
}

} // namespace Urho3D
