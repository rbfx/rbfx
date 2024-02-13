// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Matrix3x4.h"

#include <vector>

namespace Urho3D
{

class Component;

/// Description of a navigation mesh geometry component, with transform and bounds information.
struct NavigationGeometryInfo
{
    /// Component.
    Component* component_{};
    /// Geometry LOD level if applicable.
    unsigned lodLevel_{};
    /// Transform relative to the navigation mesh root node.
    Matrix3x4 transform_;
    /// Bounding box relative to the navigation mesh root node.
    BoundingBox boundingBox_;
};

/// Calculate bounding box of given geometry.
URHO3D_API BoundingBox CalculateBoundingBox(
    const ea::vector<NavigationGeometryInfo>& geometryList, const Vector3& padding);

/// Calculate bounding box of given geometry within the given tile volume.
URHO3D_API BoundingBox CalculateTileBoundingBox(
    const ea::vector<NavigationGeometryInfo>& geometryList, const BoundingBox& tileColumn);

/// Calculate maximum number of tiles required to contain the given bounding box.
URHO3D_API unsigned CalculateMaxTiles(const BoundingBox& boundingBox, int tileSize, float cellSize);

} // namespace Urho3D
