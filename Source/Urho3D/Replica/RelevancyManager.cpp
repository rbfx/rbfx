// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RelevancyManager.h"

namespace Urho3D
{

void RelevancyManager::SetDefaultRadius(float radius)
{
    defaultRadius_ = Max(radius, 0.0f);
}

void RelevancyManager::SetObserver(unsigned connectionId, const Vector3& position, float radius)
{
    if (RelevancyObserver* observer = FindObserverMutable(connectionId))
    {
        observer->position = position;
        observer->radius = radius >= 0.0f ? radius : defaultRadius_;
        return;
    }

    observers_.push_back({connectionId, position, radius >= 0.0f ? radius : defaultRadius_});
}

bool RelevancyManager::RemoveObserver(unsigned connectionId)
{
    for (auto iter = observers_.begin(); iter != observers_.end(); ++iter)
    {
        if (iter->connectionId == connectionId)
        {
            observers_.erase(iter);
            return true;
        }
    }
    return false;
}

const RelevancyObserver* RelevancyManager::FindObserver(unsigned connectionId) const
{
    for (const RelevancyObserver& observer : observers_)
    {
        if (observer.connectionId == connectionId)
            return &observer;
    }
    return nullptr;
}

RelevancyObserver* RelevancyManager::FindObserverMutable(unsigned connectionId)
{
    for (RelevancyObserver& observer : observers_)
    {
        if (observer.connectionId == connectionId)
            return &observer;
    }
    return nullptr;
}

void RelevancyManager::SetObjectRule(NetworkId objectId, const Vector3& position, float radius, bool alwaysRelevant)
{
    if (RelevancyRule* rule = FindRuleMutable(objectId))
    {
        rule->position = position;
        rule->radius = radius >= 0.0f ? radius : defaultRadius_;
        rule->alwaysRelevant = alwaysRelevant;
        return;
    }

    rules_.push_back({objectId, position, radius >= 0.0f ? radius : defaultRadius_, alwaysRelevant});
}

bool RelevancyManager::RemoveObjectRule(NetworkId objectId)
{
    for (auto iter = rules_.begin(); iter != rules_.end(); ++iter)
    {
        if (iter->objectId == objectId)
        {
            rules_.erase(iter);
            return true;
        }
    }
    return false;
}

bool RelevancyManager::SetAlwaysRelevant(NetworkId objectId, bool alwaysRelevant)
{
    RelevancyRule* rule = FindRuleMutable(objectId);
    if (!rule)
        return false;
    rule->alwaysRelevant = alwaysRelevant;
    return true;
}

RelevancyRule* RelevancyManager::FindRuleMutable(NetworkId objectId)
{
    for (RelevancyRule& rule : rules_)
    {
        if (rule.objectId == objectId)
            return &rule;
    }
    return nullptr;
}

const RelevancyRule* RelevancyManager::FindRule(NetworkId objectId) const
{
    for (const RelevancyRule& rule : rules_)
    {
        if (rule.objectId == objectId)
            return &rule;
    }
    return nullptr;
}

bool RelevancyManager::IsRelevant(unsigned connectionId, NetworkId objectId) const
{
    const RelevancyRule* rule = FindRule(objectId);
    if (!rule || rule->alwaysRelevant)
        return true;

    const RelevancyObserver* observer = FindObserver(connectionId);
    if (!observer)
        return false;

    const float radius = Max(observer->radius, rule->radius);
    return (rule->position - observer->position).LengthSquared() <= radius * radius;
}

ea::vector<NetworkId> RelevancyManager::Filter(unsigned connectionId, const ea::vector<NetworkId>& objectIds) const
{
    ea::vector<NetworkId> result;
    result.reserve(objectIds.size());
    for (const NetworkId objectId : objectIds)
    {
        if (IsRelevant(connectionId, objectId))
            result.push_back(objectId);
    }
    return result;
}

} // namespace Urho3D
