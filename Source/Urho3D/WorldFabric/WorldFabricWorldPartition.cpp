// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "WorldFabricWorldPartition.h"

#include <Urho3D/Scene/WorldPartition.h>

namespace Urho3D
{

unsigned WorldFabricWorldPartition::Synchronize(WorldPartition* partition, WorldFabricGraph& graph)
{
    if (!partition)
        return 0;

    unsigned count = 0;
    for (const ea::string& cellId : partition->GetCellIds())
    {
        const StreamingCell* cell = partition->GetCell(cellId);
        if (!cell)
            continue;

        const StreamingCellDescriptor& descriptor = cell->GetDescriptor();
        const ea::string key = "worldpartition/cell/" + cellId;
        StringVariantMap metadata;
        metadata["cellId"] = Variant(cellId);
        metadata["scenePath"] = Variant(descriptor.scenePath);
        metadata["coordinateX"] = Variant(descriptor.coordinates.x_);
        metadata["coordinateY"] = Variant(descriptor.coordinates.y_);
        metadata["centerX"] = Variant(descriptor.center.x_);
        metadata["centerY"] = Variant(descriptor.center.y_);
        metadata["centerZ"] = Variant(descriptor.center.z_);
        metadata["radius"] = Variant(descriptor.radius);
        metadata["memoryCost"] = Variant(static_cast<int>(descriptor.memoryCost));
        metadata["state"] = Variant(static_cast<int>(cell->GetState()));
        metadata["distanceSquared"] = Variant(cell->GetDistanceSquared());
        metadata["loadRevision"] = Variant(static_cast<int>(cell->GetLoadRevision()));
        metadata["lastError"] = Variant(cell->GetLastError());

        const WorldFabricId cellNode = graph.AddNode(key, WorldFabricNodeKind::SceneCell, "StreamingCell", metadata);
        if (cellNode == InvalidWorldFabricId)
            continue;
        ++count;

        const ea::string sceneKey = "asset/scene/" + descriptor.scenePath;
        const WorldFabricId sceneNode = graph.AddNode(sceneKey, WorldFabricNodeKind::Asset, "Scene", {});
        if (sceneNode != InvalidWorldFabricId && !descriptor.scenePath.empty())
            graph.AddDependency(cellNode, sceneNode, WorldFabricDependencyKind::StreamsWith, "scene");
    }
    return count;
}

} // namespace Urho3D
