// SPDX-License-Identifier: MIT

#include "WorldFabric.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace Urho3D
{

namespace
{

constexpr unsigned long long FnvOffset = 14695981039346656037ull;
constexpr unsigned long long FnvPrime = 1099511628211ull;

void HashByte(unsigned long long& hash, unsigned char byte)
{
    hash ^= byte;
    hash *= FnvPrime;
}

void HashString(unsigned long long& hash, const ea::string& value)
{
    for (char character : value)
        HashByte(hash, static_cast<unsigned char>(character));
    HashByte(hash, 0xff);
}

bool DependencyLess(const WorldFabricDependency& lhs, const WorldFabricDependency& rhs)
{
    if (lhs.dependency != rhs.dependency)
        return lhs.dependency < rhs.dependency;
    if (lhs.node != rhs.node)
        return lhs.node < rhs.node;
    if (lhs.kind != rhs.kind)
        return static_cast<unsigned>(lhs.kind) < static_cast<unsigned>(rhs.kind);
    return lhs.label < rhs.label;
}

} // namespace

WorldFabricId WorldFabricGraph::MakeStableId(const ea::string& key)
{
    unsigned long long hash = FnvOffset;
    HashString(hash, key);
    return hash == InvalidWorldFabricId ? 1 : hash;
}

WorldFabricId WorldFabricGraph::MakeCollisionSafeId(const ea::string& key) const
{
    const WorldFabricId base = MakeStableId(key);
    if (const WorldFabricNode* existing = GetNode(base); !existing || existing->key == key)
        return base;

    for (unsigned suffix = 1; suffix != M_MAX_UNSIGNED; ++suffix)
    {
        ea::string candidate = key;
        candidate += "#";
        candidate += std::to_string(suffix).c_str();
        const WorldFabricId id = MakeStableId(candidate);
        if (!GetNode(id))
            return id;
    }
    return InvalidWorldFabricId;
}

WorldFabricId WorldFabricGraph::AddNode(const ea::string& key, WorldFabricNodeKind kind,
    const ea::string& type, const StringVariantMap& metadata)
{
    if (key.empty())
    {
        lastError_ = "WorldFabric node key must not be empty.";
        return InvalidWorldFabricId;
    }
    const WorldFabricId id = MakeCollisionSafeId(key);
    if (id == InvalidWorldFabricId)
    {
        lastError_ = "Unable to allocate a collision-safe WorldFabric identifier.";
        return InvalidWorldFabricId;
    }
    if (WorldFabricNode* existing = GetNode(id))
    {
        existing->kind = kind;
        existing->type = type;
        existing->metadata = metadata;
        return id;
    }

    WorldFabricNode node;
    node.id = id;
    node.key = key;
    node.kind = kind;
    node.type = type;
    node.metadata = metadata;
    nodes_[id] = node;
    Emit({WorldFabricEventType::NodeAdded, id, InvalidWorldFabricId});
    return id;
}

bool WorldFabricGraph::RemoveNode(WorldFabricId node)
{
    if (!GetNode(node))
        return false;
    nodes_.erase(node);
    dependencies_.erase(node);
    for (auto& entry : dependencies_)
    {
        DependencyList& list = entry.second;
        list.erase(std::remove_if(list.begin(), list.end(),
            [node](const WorldFabricDependency& dependency)
            {
                return dependency.node == node || dependency.dependency == node;
            }), list.end());
    }
    Emit({WorldFabricEventType::NodeRemoved, node, InvalidWorldFabricId});
    return true;
}

WorldFabricNode* WorldFabricGraph::GetNode(WorldFabricId node)
{
    return const_cast<WorldFabricNode*>(static_cast<const WorldFabricGraph*>(this)->GetNode(node));
}

const WorldFabricNode* WorldFabricGraph::GetNode(WorldFabricId node) const
{
    const auto iterator = nodes_.find(node);
    return iterator != nodes_.end() ? &iterator->second : nullptr;
}

ea::vector<WorldFabricNode> WorldFabricGraph::GetNodes() const
{
    ea::vector<WorldFabricNode> result;
    result.reserve(nodes_.size());
    for (const auto& entry : nodes_)
        result.push_back(entry.second);
    std::sort(result.begin(), result.end(), [](const WorldFabricNode& lhs, const WorldFabricNode& rhs)
    {
        return lhs.id < rhs.id;
    });
    return result;
}

bool WorldFabricGraph::HasDependency(const WorldFabricDependency& dependency) const
{
    const auto iterator = dependencies_.find(dependency.node);
    if (iterator == dependencies_.end())
        return false;
    return std::find(iterator->second.begin(), iterator->second.end(), dependency) != iterator->second.end();
}

bool WorldFabricGraph::AddDependency(WorldFabricId node, WorldFabricId dependency,
    WorldFabricDependencyKind kind, const ea::string& label)
{
    if (!GetNode(node) || !GetNode(dependency) || node == dependency)
    {
        lastError_ = "WorldFabric dependency references an invalid or identical node.";
        return false;
    }
    const WorldFabricDependency edge{node, dependency, kind, label};
    if (HasDependency(edge))
        return false;
    dependencies_[node].push_back(edge);
    Emit({WorldFabricEventType::DependencyAdded, node, dependency});
    return true;
}

bool WorldFabricGraph::RemoveDependency(WorldFabricId node, WorldFabricId dependency,
    WorldFabricDependencyKind kind, const ea::string& label)
{
    const auto iterator = dependencies_.find(node);
    if (iterator == dependencies_.end())
        return false;
    const WorldFabricDependency edge{node, dependency, kind, label};
    const auto edgeIterator = std::find(iterator->second.begin(), iterator->second.end(), edge);
    if (edgeIterator == iterator->second.end())
        return false;
    iterator->second.erase(edgeIterator);
    Emit({WorldFabricEventType::DependencyRemoved, node, dependency});
    return true;
}

ea::vector<WorldFabricDependency> WorldFabricGraph::GetDependencies(WorldFabricId node) const
{
    const auto iterator = dependencies_.find(node);
    if (iterator == dependencies_.end())
        return {};
    ea::vector<WorldFabricDependency> result = iterator->second;
    std::sort(result.begin(), result.end(), DependencyLess);
    return result;
}

ea::vector<WorldFabricDependency> WorldFabricGraph::GetDependents(WorldFabricId node) const
{
    ea::vector<WorldFabricDependency> result;
    for (const auto& entry : dependencies_)
    {
        for (const WorldFabricDependency& dependency : entry.second)
        {
            if (dependency.dependency == node)
                result.push_back(dependency);
        }
    }
    std::sort(result.begin(), result.end(), DependencyLess);
    return result;
}

void WorldFabricGraph::SetError(ea::string* error, const ea::string& message) const
{
    lastError_ = message;
    if (error)
        *error = message;
}

bool WorldFabricGraph::VisitForOrder(WorldFabricId node, ea::unordered_map<WorldFabricId, unsigned char>& marks,
    ea::vector<WorldFabricId>& order, ea::string* error) const
{
    const unsigned char mark = marks[node];
    if (mark == 2)
        return true;
    if (mark == 1)
    {
        SetError(error, "WorldFabric dependency graph contains a cycle.");
        return false;
    }
    marks[node] = 1;
    ea::vector<WorldFabricDependency> dependencies = GetDependencies(node);
    for (const WorldFabricDependency& edge : dependencies)
    {
        if (!VisitForOrder(edge.dependency, marks, order, error))
            return false;
    }
    marks[node] = 2;
    order.push_back(node);
    return true;
}

bool WorldFabricGraph::BuildOrder(ea::vector<WorldFabricId>& order, ea::string* error) const
{
    order.clear();
    ea::vector<WorldFabricNode> nodes = GetNodes();
    ea::unordered_map<WorldFabricId, unsigned char> marks;
    for (const WorldFabricNode& node : nodes)
    {
        if (!VisitForOrder(node.id, marks, order, error))
        {
            order.clear();
            return false;
        }
    }
    return true;
}

bool WorldFabricGraph::HasCycles(ea::string* error) const
{
    ea::vector<WorldFabricId> order;
    return !BuildOrder(order, error);
}

bool WorldFabricGraph::Validate(ea::string* error) const
{
    for (const auto& entry : nodes_)
    {
        const WorldFabricNode& node = entry.second;
        if (node.id == InvalidWorldFabricId || node.id != entry.first || node.key.empty())
        {
            SetError(error, "WorldFabric contains a malformed node.");
            return false;
        }
    }
    for (const auto& entry : dependencies_)
    {
        if (!GetNode(entry.first))
        {
            SetError(error, "WorldFabric contains dependencies for a missing node.");
            return false;
        }
        for (unsigned i = 0; i < entry.second.size(); ++i)
        {
            const WorldFabricDependency& dependency = entry.second[i];
            if (!GetNode(dependency.node) || !GetNode(dependency.dependency) || dependency.node == dependency.dependency
                || !HasDependency(dependency))
            {
                SetError(error, "WorldFabric contains an invalid dependency edge.");
                return false;
            }
            for (unsigned j = i + 1; j < entry.second.size(); ++j)
            {
                if (dependency == entry.second[j])
                {
                    SetError(error, "WorldFabric contains a duplicate dependency edge.");
                    return false;
                }
            }
        }
    }
    ea::vector<WorldFabricId> order;
    return BuildOrder(order, error);
}

unsigned long long WorldFabricGraph::ComputeDigest() const
{
    unsigned long long digest = FnvOffset;
    const ea::vector<WorldFabricNode> nodes = GetNodes();
    for (const WorldFabricNode& node : nodes)
    {
        HashByte(digest, static_cast<unsigned char>(node.kind));
        HashString(digest, node.key);
        HashString(digest, node.type);
        ea::vector<ea::string> metadataKeys;
        metadataKeys.reserve(node.metadata.size());
        for (const auto& metadata : node.metadata)
            metadataKeys.push_back(metadata.first);
        std::sort(metadataKeys.begin(), metadataKeys.end());
        for (const ea::string& metadataKey : metadataKeys)
        {
            HashString(digest, metadataKey);
            HashString(digest, node.metadata.find(metadataKey)->second.ToString());
        }
        const ea::vector<WorldFabricDependency> dependencies = GetDependencies(node.id);
        for (const WorldFabricDependency& dependency : dependencies)
        {
            for (unsigned shift = 0; shift < 64; shift += 8)
                HashByte(digest, static_cast<unsigned char>((dependency.dependency >> shift) & 0xff));
            HashByte(digest, static_cast<unsigned char>(dependency.kind));
            HashString(digest, dependency.label);
        }
    }
    return digest;
}

unsigned WorldFabricGraph::Subscribe(EventCallback callback)
{
    if (!callback)
        return 0;
    const unsigned subscription = nextSubscriptionId_++;
    callbacks_[subscription] = ea::move(callback);
    return subscription;
}

bool WorldFabricGraph::Unsubscribe(unsigned subscriptionId)
{
    return callbacks_.erase(subscriptionId) != 0;
}

void WorldFabricGraph::Emit(const WorldFabricEvent& event)
{
    ea::vector<unsigned> subscriptions;
    subscriptions.reserve(callbacks_.size());
    for (const auto& callback : callbacks_)
        subscriptions.push_back(callback.first);
    std::sort(subscriptions.begin(), subscriptions.end());
    for (unsigned subscription : subscriptions)
    {
        const auto iterator = callbacks_.find(subscription);
        if (iterator != callbacks_.end() && iterator->second)
            iterator->second(event);
    }
}

void WorldFabricGraph::Reset()
{
    nodes_.clear();
    dependencies_.clear();
    lastError_.clear();
    Emit({WorldFabricEventType::GraphReset, InvalidWorldFabricId, InvalidWorldFabricId});
}

} // namespace Urho3D
