// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/Navigation/NavigationUtils.h"

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
    WriteDetourBuffer(dest, buffer.data_.get(), + buffer.dataSize_);
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
        URHO3D_LOGERROR("Tile width should be integer for NavigationMesh rebase: {} * {} = {}",
            tileSize, cellSize, tileSize * cellSize);
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

BoundingBox CalculateBoundingBox(const ea::vector<NavigationGeometryInfo>& geometryList, const Vector3& padding)
{
    BoundingBox result;
    for (unsigned i = 0; i < geometryList.size(); ++i)
        result.Merge(geometryList[i].boundingBox_);

    result.min_ -= padding;
    result.max_ += padding;

    return result;
}

BoundingBox CalculateTileBoundingBox(
    const ea::vector<NavigationGeometryInfo>& geometryList, const BoundingBox& tileColumn)
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

} // namespace Urho3D
