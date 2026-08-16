// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/vector.h>

#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Replica/NetworkId.h>

namespace Urho3D
{

/// Connection-specific interest point used by relevancy filtering.
struct URHO3D_API RelevancyObserver
{
    unsigned connectionId{};
    Vector3 position{Vector3::ZERO};
    float radius{100.0f};
};

/// Per-object relevancy rule used by production interest management.
struct URHO3D_API RelevancyRule
{
    NetworkId objectId{NetworkId::None};
    Vector3 position{Vector3::ZERO};
    float radius{100.0f};
    bool alwaysRelevant{};
};

/// Deterministic interest-management service independent from ReplicationManager transport details.
class URHO3D_API RelevancyManager
{
public:
    /// Set the default radius for observers without an explicit radius.
    void SetDefaultRadius(float radius);
    float GetDefaultRadius() const { return defaultRadius_; }

    /// Register or update a connection's interest point.
    void SetObserver(unsigned connectionId, const Vector3& position, float radius = -1.0f);
    /// Remove a connection observer.
    bool RemoveObserver(unsigned connectionId);
    /// Return an observer, if one exists.
    const RelevancyObserver* FindObserver(unsigned connectionId) const;

    /// Register or update an object's spatial relevance rule.
    void SetObjectRule(NetworkId objectId, const Vector3& position, float radius = -1.0f, bool alwaysRelevant = false);
    /// Remove an object's rule.
    bool RemoveObjectRule(NetworkId objectId);
    /// Mark an object as globally relevant without a distance test.
    bool SetAlwaysRelevant(NetworkId objectId, bool alwaysRelevant = true);

    /// Determine whether an object should be sent to a connection.
    bool IsRelevant(unsigned connectionId, NetworkId objectId) const;
    /// Filter a list of object ids while preserving its original order.
    ea::vector<NetworkId> Filter(unsigned connectionId, const ea::vector<NetworkId>& objectIds) const;

    const ea::vector<RelevancyObserver>& GetObservers() const { return observers_; }
    const ea::vector<RelevancyRule>& GetRules() const { return rules_; }

private:
    RelevancyObserver* FindObserverMutable(unsigned connectionId);
    RelevancyRule* FindRuleMutable(NetworkId objectId);
    const RelevancyRule* FindRule(NetworkId objectId) const;

    float defaultRadius_{100.0f};
    ea::vector<RelevancyObserver> observers_;
    ea::vector<RelevancyRule> rules_;
};

} // namespace Urho3D
