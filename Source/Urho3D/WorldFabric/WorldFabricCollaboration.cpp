// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "WorldFabricCollaboration.h"

#include <algorithm>

namespace Urho3D
{

namespace
{

bool Contains(const ea::vector<ea::string>& values, const ea::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

void WorldFabricCollaboration::SetError(ea::string* error, const ea::string& message)
{
    lastError_ = message;
    if (error)
        *error = message;
}

bool WorldFabricCollaboration::IsClientKnown(const ea::string& clientId) const
{
    return !clientId.empty() && Contains(clients_, clientId);
}

bool WorldFabricCollaboration::IsLockedByOther(WorldFabricId node, const ea::string& clientId) const
{
    for (const WorldFabricLock& lock : locks_)
    {
        if (lock.node == node && lock.clientId != clientId)
            return true;
    }
    return false;
}

bool WorldFabricCollaboration::AddClient(const ea::string& clientId)
{
    if (clientId.empty() || IsClientKnown(clientId))
    {
        lastError_ = "Collaboration client identifiers must be non-empty and unique.";
        return false;
    }
    clients_.push_back(clientId);
    std::sort(clients_.begin(), clients_.end());
    return true;
}

bool WorldFabricCollaboration::RemoveClient(const ea::string& clientId)
{
    auto it = std::find(clients_.begin(), clients_.end(), clientId);
    if (it == clients_.end())
        return false;
    clients_.erase(it);
    locks_.erase(std::remove_if(locks_.begin(), locks_.end(), [&clientId](const WorldFabricLock& lock)
    {
        return lock.clientId == clientId;
    }), locks_.end());
    return true;
}

bool WorldFabricCollaboration::Lock(WorldFabricId node, const ea::string& clientId)
{
    if (!graph_ || !IsClientKnown(clientId) || node == InvalidWorldFabricId || !graph_->GetNode(node))
    {
        lastError_ = "Cannot lock an unknown WorldFabric node or client.";
        return false;
    }
    for (WorldFabricLock& lock : locks_)
    {
        if (lock.node == node)
        {
            if (lock.clientId == clientId)
                return true;
            lastError_ = "WorldFabric node is already locked by another client.";
            return false;
        }
    }
    WorldFabricLock lock;
    lock.node = node;
    lock.clientId = clientId;
    lock.revision = revision_;
    locks_.push_back(lock);
    return true;
}

bool WorldFabricCollaboration::Unlock(WorldFabricId node, const ea::string& clientId)
{
    for (auto it = locks_.begin(); it != locks_.end(); ++it)
    {
        if (it->node == node)
        {
            if (it->clientId != clientId)
            {
                lastError_ = "Only the lock owner can unlock a WorldFabric node.";
                return false;
            }
            locks_.erase(it);
            return true;
        }
    }
    return false;
}

bool WorldFabricCollaboration::Apply(const WorldFabricOperation& operation, ea::string* error)
{
    if (!graph_)
    {
        SetError(error, "WorldFabric collaboration requires a bound graph.");
        return false;
    }
    if (!IsClientKnown(operation.clientId))
    {
        SetError(error, "WorldFabric operation comes from an unknown client.");
        return false;
    }
    if (IsLockedByOther(operation.node, operation.clientId))
    {
        SetError(error, "WorldFabric operation targets a node locked by another client.");
        return false;
    }

    switch (operation.kind)
    {
    case WorldFabricOperationKind::AddNode:
        {
            const WorldFabricId actual = graph_->AddNode(operation.key, operation.nodeKind, operation.type);
            if (actual == InvalidWorldFabricId || (operation.node != InvalidWorldFabricId && actual != operation.node))
            {
                SetError(error, "WorldFabric AddNode operation produced an unexpected stable identifier.");
                return false;
            }
            return true;
        }
    case WorldFabricOperationKind::RemoveNode:
        if (!graph_->RemoveNode(operation.node))
        {
            SetError(error, "WorldFabric RemoveNode operation referenced an unknown node.");
            return false;
        }
        return true;
    case WorldFabricOperationKind::AddDependency:
        if (!graph_->AddDependency(operation.node, operation.dependency, operation.dependencyKind, operation.label))
        {
            SetError(error, "WorldFabric AddDependency operation was rejected by graph validation.");
            return false;
        }
        return true;
    case WorldFabricOperationKind::RemoveDependency:
        if (!graph_->RemoveDependency(operation.node, operation.dependency, operation.dependencyKind, operation.label))
        {
            SetError(error, "WorldFabric RemoveDependency operation referenced an unknown edge.");
            return false;
        }
        return true;
    case WorldFabricOperationKind::SetMetadata:
        {
            WorldFabricNode* node = graph_->GetNode(operation.node);
            if (!node || operation.metadataKey.empty())
            {
                SetError(error, "WorldFabric SetMetadata operation referenced an unknown node or empty key.");
                return false;
            }
            node->metadata[operation.metadataKey] = operation.metadataValue;
            return true;
        }
    }

    SetError(error, "Unknown WorldFabric collaboration operation.");
    return false;
}

bool WorldFabricCollaboration::Submit(WorldFabricOperation operation, ea::string* error)
{
    if (!Apply(operation, error))
        return false;
    operation.revision = ++revision_;
    history_.push_back(operation);
    return true;
}

bool WorldFabricCollaboration::Merge(const ea::vector<WorldFabricOperation>& operations, ea::string* error)
{
    ea::vector<WorldFabricOperation> ordered = operations;
    std::sort(ordered.begin(), ordered.end(), [](const WorldFabricOperation& lhs, const WorldFabricOperation& rhs)
    {
        if (lhs.revision != rhs.revision)
            return lhs.revision < rhs.revision;
        if (lhs.clientId != rhs.clientId)
            return lhs.clientId < rhs.clientId;
        return static_cast<unsigned>(lhs.kind) < static_cast<unsigned>(rhs.kind);
    });
    for (WorldFabricOperation operation : ordered)
    {
        if (!Submit(operation, error))
            return false;
    }
    return true;
}

} // namespace Urho3D
