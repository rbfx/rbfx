// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Matrix3x4.h"

#include <cstdint>

#ifdef DT_POLYREF64
using dtPolyRef = uint64_t;
#else
using dtPolyRef = unsigned int;
#endif

namespace Urho3D
{

class Component;

enum NavmeshPartitionType
{
    NAVMESH_PARTITION_WATERSHED = 0,
    NAVMESH_PARTITION_MONOTONE
};

/// Special area ID. Evaluates to RC_WALKABLE_AREA or 0 depending on polygon slope.
static constexpr unsigned char DeduceAreaId = 0xff;

/// Description of a navigation mesh geometry component, with transform and bounds information.
struct NavigationGeometryInfo
{
    /// Geometry component.
    Component* component_{};
    /// Geometry LOD level if applicable.
    unsigned lodLevel_{};
    /// Transform relative to the navigation mesh root node.
    Matrix3x4 transform_;
    /// Bounding box relative to the navigation mesh root node.
    BoundingBox boundingBox_;
    /// Area ID.
    unsigned char areaId_{DeduceAreaId};
};

using NavigationGeometryInfoVector = ea::vector<NavigationGeometryInfo>;

} // namespace Urho3D
