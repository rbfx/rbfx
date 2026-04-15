// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <cstdint>

#ifdef DT_POLYREF64
using dtPolyRef = uint64_t;
#else
using dtPolyRef = unsigned int;
#endif

namespace Urho3D
{

enum NavmeshPartitionType
{
    NAVMESH_PARTITION_WATERSHED = 0,
    NAVMESH_PARTITION_MONOTONE
};

/// Special area ID. Evaluates to RC_WALKABLE_AREA or 0 depending on polygon slope.
static constexpr unsigned char DeduceAreaId = 0xff;

} // namespace Urho3D
