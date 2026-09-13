// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/TransformedSpan.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Replica/ClientReplica.h"
#include "Urho3D/Replica/ProtocolMessages.h"
#include "Urho3D/Replica/ServerReplicator.h"
#include "Urho3D/Scene/TrackedComponent.h"
#include "Urho3D/Container/RefCounted.h"
#include "Urho3D/Network/NetworkDefs.h"

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class Network;
class NetworkObject;
struct NetworkSetting;

/// Part of ReplicationManager used by both client and server, and referenced by components.
class URHO3D_API NetworkObjectRegistry : public ReferencedComponentRegistryBase
{
    URHO3D_OBJECT(NetworkObjectRegistry, ReferencedComponentRegistryBase);

public:
    Signal<void(NetworkObject*)> OnNetworkObjectAdded;
    Signal<void(NetworkObject*)> OnNetworkObjectRemoved;
    using NetworkObjectSpan = TransformedSpan<TrackedComponentBase* const, NetworkObject* const, StaticCaster<NetworkObject* const>>;

    explicit NetworkObjectRegistry(Context* context);

    static void RegisterObject(Context* context);

    /// Process components
    /// @{
    void RemoveAllNetworkObjects();

    void QueueNetworkObjectUpdate(NetworkObject* networkObject);
    void UpdateNetworkObjects();

    void GetSortedNetworkObjects(ea::vector<NetworkObject*>& networkObjects) const;
    /// @}

    NetworkObjectSpan GetNetworkObjects() const { return StaticCastSpan<NetworkObject* const>(GetTrackedComponents()); }
    unsigned GetNetworkIndexUpperBound() const { return GetReferenceIndexUpperBound(); }
    NetworkObject* GetNetworkObject(NetworkId networkId, bool checkVersion = true) const;
    NetworkObject* GetNetworkObjectByIndex(unsigned networkIndex) const;

private:
    ea::vector<bool> networkObjectsDirty_;

protected:
    void OnComponentAdded(TrackedComponentBase* baseComponent) override;
    void OnComponentRemoved(TrackedComponentBase* baseComponent) override;
};

enum ReplicationManagerMode
{
    Standalone,
    Server,
    Client
};

/// Root level scene component that manages Scene replication both on client and server.
/// Local Scene should have an instance of ReplicationManager in order to use NetworkObject-s in standalone mode.
class URHO3D_API ReplicationManager : public NetworkObjectRegistry
{
    URHO3D_OBJECT(ReplicationManager, NetworkObjectRegistry);

public:
    explicit ReplicationManager(Context* context);
    ~ReplicationManager() override;

    static void RegisterObject(Context* context);

    /// Stop whatever client or server logic is going on and continue standalone.
    void StartStandalone();
    /// Start new server from current state.
    void StartServer();
    /// Start new client from specified connection. Removes all existing objects.
    void StartClient(ReplicatedPeerPtr connectionToServer);
    /// Process network message either as client or as server.
    bool ProcessMessage(ReplicatedPeer* connection, NetworkMessageId messageId, MemoryBuffer& messageData);
    /// Process connection dropped. Removes client connection for server, converts scene to standalone for client.
    void DropConnection(ReplicatedPeerPtr connection);

    /// Attributes.
    /// @{
    bool IsFixedUpdateServer() const { return attributes_.isFixedUpdateServer_; }
    void SetFixedUpdateServer(bool fixed) { attributes_.isFixedUpdateServer_ = fixed; }
    bool IsAllowZeroUpdatesOnServer() const { return attributes_.allowZeroUpdatesOnServer_; }
    void SetAllowZeroUpdatesOnServer(bool allow) { attributes_.allowZeroUpdatesOnServer_ = allow; }
    /// @}

    /// Return current state specific to client or server.
    /// @{
    unsigned GetUpdateFrequency() const;
    float GetTraceDurationInSeconds() const;
    unsigned GetTraceDurationInFrames() const;
    const Variant& GetSetting(const NetworkSetting& setting) const;
    ea::string GetDebugInfo() const;
    bool IsStandalone() const { return mode_ == ReplicationManagerMode::Standalone; }
    bool IsServer() const { return mode_ == ReplicationManagerMode::Server; }
    bool IsClient() const { return mode_ == ReplicationManagerMode::Client; }
    ClientReplica* GetClientReplica() const { return client_ ? client_->replica_ : nullptr; }
    ServerReplicator* GetServerReplicator() const { return server_; }
    /// @}

protected:
    void OnSceneSet(Scene* previousScene, Scene* scene) override;
    void OnComponentAdded(TrackedComponentBase* baseComponent) override;
    void OnComponentRemoved(TrackedComponentBase* baseComponent) override;

private:
    void HandleSceneUpdate(StringHash eventType, float timeStep);
    void OnSceneUpdate(float timeStep);
    void OnScenePostUpdate(float timeStep);

    void InitializeObjectsStandalone();
    void Stop();
    bool ProcessMessageOnUninitializedClient(
        ReplicatedPeer* connection, NetworkMessageId messageId, MemoryBuffer& messageData);
    ea::string GetUninitializedClientDebugInfo() const;

    struct StandaloneData
    {
        ea::unordered_set<NetworkId> recentlyAddedObjects_;
    };

    struct ClientData
    {
        ReplicatedPeerWeakPtr peer_;
        ea::optional<MsgSceneClock> initialClock_;
        ea::optional<VariantMap> serverSettings_;
        ea::optional<unsigned> ackMagic_;

        bool IsReadyToInitialize() const { return peer_ && initialClock_ && serverSettings_ && ackMagic_; }

        SharedPtr<ClientReplica> replica_;
    };

    struct Attributes
    {
        bool isFixedUpdateServer_{true};
        bool allowZeroUpdatesOnServer_{};
    } attributes_;

    ReplicationManagerMode mode_{};
    SharedPtr<ServerReplicator> server_;
    ea::optional<ClientData> client_;
    StandaloneData standalone_;
};

}
