// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/WorldFabric/WorldFabric.h>

namespace Urho3D
{

class WorldPartition;

/// Projects the live WorldPartition cell registry into World Fabric.
class URHO3D_API WorldFabricWorldPartition
{
public:
    /// Add or update all registered cells and their streaming metadata.
    /// Returns the number of cells projected.
    static unsigned Synchronize(WorldPartition* partition, WorldFabricGraph& graph);
};

} // namespace Urho3D
