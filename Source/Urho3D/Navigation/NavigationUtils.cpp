// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Navigation/NavigationUtils.h"

namespace Urho3D
{

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

} // namespace Urho3D
