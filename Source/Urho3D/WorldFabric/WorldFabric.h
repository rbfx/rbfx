// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

using WorldFabricId = unsigned long long;
static constexpr WorldFabricId InvalidWorldFabricId = 0;

enum class WorldFabricNodeKind
{
    Entity,
    Component,
    Blueprint,
    RbScript,
    Asset,
    SceneCell,
    RenderResource,
    NetworkObject,
    AudioBus,
    Animation,
    Custom
};

enum class WorldFabricDependencyKind
{
    Requires,
    Produces,
    References,
    Replicates,
    StreamsWith,
    Profiles,
    BuildsFrom
};

enum class WorldFabricEventType
{
    NodeAdded,
    NodeRemoved,
    DependencyAdded,
    DependencyRemoved,
    GraphReset
};

struct URHO3D_API WorldFabricNode
{
    WorldFabricId id{InvalidWorldFabricId};
    ea::string key;
    ea::string type;
    WorldFabricNodeKind kind{WorldFabricNodeKind::Custom};
    StringVariantMap metadata;
};

struct URHO3D_API WorldFabricDependency
{
    WorldFabricId node{InvalidWorldFabricId};
    WorldFabricId dependency{InvalidWorldFabricId};
    WorldFabricDependencyKind kind{WorldFabricDependencyKind::Requires};
    ea::string label;

    bool operator==(const WorldFabricDependency& rhs) const
    {
        return node == rhs.node && dependency == rhs.dependency && kind == rhs.kind && label == rhs.label;
    }
};

struct URHO3D_API WorldFabricEvent
{
    WorldFabricEventType type{WorldFabricEventType::GraphReset};
    WorldFabricId node{InvalidWorldFabricId};
    WorldFabricId dependency{InvalidWorldFabricId};
};

/// Stable, deterministic semantic dependency graph shared by all production systems.
class URHO3D_API WorldFabricGraph
{
public:
    using EventCallback = ea::function<void(const WorldFabricEvent&)>;

    /// Return the stable identifier generated from a semantic key.
    static WorldFabricId MakeStableId(const ea::string& key);

    /// Add a node. Re-adding the same key returns the existing stable identifier.
    WorldFabricId AddNode(const ea::string& key, WorldFabricNodeKind kind,
        const ea::string& type = EMPTY_STRING, const StringVariantMap& metadata = {});
    /// Remove a node and all dependencies touching it.
    bool RemoveNode(WorldFabricId node);
    /// Return a mutable node or nullptr.
    WorldFabricNode* GetNode(WorldFabricId node);
    /// Return an immutable node or nullptr.
    const WorldFabricNode* GetNode(WorldFabricId node) const;
    /// Return all nodes sorted by stable identifier.
    ea::vector<WorldFabricNode> GetNodes() const;

    /// Add a directed dependency: `node` cannot be built before `dependency`.
    bool AddDependency(WorldFabricId node, WorldFabricId dependency,
        WorldFabricDependencyKind kind = WorldFabricDependencyKind::Requires,
        const ea::string& label = EMPTY_STRING);
    /// Remove one exact dependency edge.
    bool RemoveDependency(WorldFabricId node, WorldFabricId dependency,
        WorldFabricDependencyKind kind = WorldFabricDependencyKind::Requires,
        const ea::string& label = EMPTY_STRING);
    /// Return dependencies of a node in deterministic order.
    ea::vector<WorldFabricDependency> GetDependencies(WorldFabricId node) const;
    /// Return reverse dependents of a node in deterministic order.
    ea::vector<WorldFabricDependency> GetDependents(WorldFabricId node) const;

    /// Produce a dependency-first deterministic build/evaluation order.
    bool BuildOrder(ea::vector<WorldFabricId>& order, ea::string* error = nullptr) const;
    /// Return whether the graph contains a cycle.
    bool HasCycles(ea::string* error = nullptr) const;
    /// Validate identifiers, references, duplicate edges and cycles.
    bool Validate(ea::string* error = nullptr) const;
    /// Compute a deterministic content digest for caching and snapshots.
    unsigned long long ComputeDigest() const;

    /// Subscribe to structural graph changes. Returns a non-zero subscription ID.
    unsigned Subscribe(EventCallback callback);
    /// Remove a subscription.
    bool Unsubscribe(unsigned subscriptionId);
    /// Remove all nodes, edges and subscriptions-independent state.
    void Reset();

    const ea::string& GetLastError() const { return lastError_; }

private:
    using DependencyList = ea::vector<WorldFabricDependency>;

    WorldFabricId MakeCollisionSafeId(const ea::string& key) const;
    bool HasDependency(const WorldFabricDependency& dependency) const;
    bool VisitForOrder(WorldFabricId node, ea::unordered_map<WorldFabricId, unsigned char>& marks,
        ea::vector<WorldFabricId>& order, ea::string* error) const;
    void Emit(const WorldFabricEvent& event);
    void SetError(ea::string* error, const ea::string& message) const;

    ea::unordered_map<WorldFabricId, WorldFabricNode> nodes_;
    ea::unordered_map<WorldFabricId, DependencyList> dependencies_;
    ea::unordered_map<unsigned, EventCallback> callbacks_;
    unsigned nextSubscriptionId_{1};
    mutable ea::string lastError_;
};

} // namespace Urho3D
