// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/NetworkObject.h"

namespace Urho3D
{

class PrefabResource;

/// NetworkObject that is replicated on the client from prefab and is not updated afterwards.
/// Note: object position in the hierarchy of NetworkObject-s is still maintained.
class URHO3D_API StaticNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(StaticNetworkObject, NetworkObject);

public:
    explicit StaticNetworkObject(Context* context);
    ~StaticNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Server-only attributes.
    /// @{
    bool IsOriginForDistanceFiltering() const { return isOriginForDistanceFiltering_; }
    void SetOriginForDistanceFiltering(bool isOrigin) { isOriginForDistanceFiltering_ = isOrigin; }
    /// @}

    /// Attribute modification. Don't do that after replication!
    /// @{
    void SetClientPrefab(PrefabResource* prefab);
    PrefabResource* GetClientPrefab() const { return clientPrefab_; }
    /// @}

    /// Implement NetworkObject
    /// @{
    ea::optional<float> CalculateDistanceForFiltering(NetworkObject* otherNetworkObject) override;

    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override;

    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;
    /// @}

protected:
    ResourceRef GetClientPrefabAttr() const;
    void SetClientPrefabAttr(const ResourceRef& value);

private:
    SharedPtr<PrefabResource> clientPrefab_;
    bool isOriginForDistanceFiltering_{true};
};

}; // namespace Urho3D
