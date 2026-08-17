// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/WorldFabric/WorldFabric.h>

#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class URHO3D_API WorldFabricOperationKind
{
    AddNode,
    RemoveNode,
    AddDependency,
    RemoveDependency,
    SetMetadata
};

struct URHO3D_API WorldFabricOperation
{
    unsigned long long revision{};
    ea::string clientId;
    WorldFabricOperationKind kind{WorldFabricOperationKind::SetMetadata};
    WorldFabricId node{InvalidWorldFabricId};
    ea::string key;
    ea::string type;
    WorldFabricNodeKind nodeKind{WorldFabricNodeKind::Custom};
    WorldFabricId dependency{InvalidWorldFabricId};
    WorldFabricDependencyKind dependencyKind{WorldFabricDependencyKind::Requires};
    ea::string label;
    ea::string metadataKey;
    Variant metadataValue;
};

struct URHO3D_API WorldFabricLock
{
    WorldFabricId node{InvalidWorldFabricId};
    ea::string clientId;
    unsigned long long revision{};
};

/// Versioned collaboration layer for deterministic graph edits and optimistic merging.
class URHO3D_API WorldFabricCollaboration
{
public:
    explicit WorldFabricCollaboration(WorldFabricGraph* graph = nullptr) : graph_(graph) {}

    void SetGraph(WorldFabricGraph* graph) { graph_ = graph; }
    WorldFabricGraph* GetGraph() const { return graph_; }

    bool AddClient(const ea::string& clientId);
    bool RemoveClient(const ea::string& clientId);
    bool Lock(WorldFabricId node, const ea::string& clientId);
    bool Unlock(WorldFabricId node, const ea::string& clientId);
    bool Submit(WorldFabricOperation operation, ea::string* error = nullptr);
    bool Merge(const ea::vector<WorldFabricOperation>& operations, ea::string* error = nullptr);

    unsigned long long GetRevision() const { return revision_; }
    const ea::vector<WorldFabricOperation>& GetHistory() const { return history_; }
    const ea::vector<WorldFabricLock>& GetLocks() const { return locks_; }
    const ea::string& GetLastError() const { return lastError_; }

private:
    bool Apply(const WorldFabricOperation& operation, ea::string* error);
    bool IsClientKnown(const ea::string& clientId) const;
    bool IsLockedByOther(WorldFabricId node, const ea::string& clientId) const;
    void SetError(ea::string* error, const ea::string& message);

    WorldFabricGraph* graph_{};
    ea::vector<ea::string> clients_;
    ea::vector<WorldFabricOperation> history_;
    ea::vector<WorldFabricLock> locks_;
    unsigned long long revision_{};
    ea::string lastError_;
};

} // namespace Urho3D
