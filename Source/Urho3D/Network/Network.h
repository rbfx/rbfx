// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

#include <EASTL/hash_set.h>

namespace Urho3D
{

class Scene;
class NetworkConnection;
class NetworkServer;

/// %Network subsystem. Provides network update scheduling and replication events.
class URHO3D_API Network : public Object
{
    URHO3D_OBJECT(Network, Object);

public:
    /// Construct.
    explicit Network(Context* context);
    /// Destruct.
    ~Network() override;

    /// Create a network connection of the specified type.
    SharedPtr<NetworkConnection> CreateConnection(StringHash type = StringHash::Empty);
    /// Create a network server of the specified type.
    SharedPtr<NetworkServer> CreateServer(StringHash type = StringHash::Empty);

    /// Set network update FPS.
    /// @property
    void SetUpdateFps(unsigned fps);
    /// Return network update FPS.
    /// @property
    unsigned GetUpdateFps() const { return updateFps_; }

    /// Return the amount of time that happened after fixed-time network update.
    float GetUpdateOvertime() const { return updateAcc_; }

    /// Return whether the network is updated on this frame.
    bool IsUpdateNow() const { return updateNow_; }

    /// Return whether any server or connection teardown is in progress.
    bool HasActiveResources() const { return !stoppingServers_.empty() || !disconnectingConnections_.empty(); }

    /// Process incoming messages from connections. Called by HandleBeginFrame.
    void Update(float timeStep);
    /// Send outgoing messages after frame logic. Called by HandleRenderUpdate.
    void PostUpdate(float timeStep);

protected:
    friend class NetworkConnection;
    friend class NetworkServer;

    /// Called by NetworkConnection::Disconnect to notify teardown has started.
    void OnConnectionDisconnecting(NetworkConnection* connection);
    /// Called by NetworkServer::Stop implementation to notify teardown has started.
    void OnServerStopping(NetworkServer* server);

private:
    /// Event handlers.
    /// @{
    void HandleBeginFrame(VariantMap& eventData);
    void HandleRenderUpdate(VariantMap& eventData);
    void HandleClientDisconnected(VariantMap& eventData);
    void HandleServerClientDisconnected(VariantMap& eventData);
    /// @}

    void SendNetworkUpdateEvent(StringHash eventType, bool isServer);

    /// Properties that need connection reset to apply
    /// @{
    unsigned updateFps_{30};
    /// @}
    /// Update time interval.
    float updateInterval_ = 1.0f / updateFps_;
    /// Update time accumulator.
    float updateAcc_ = 0.0f;
    /// Whether the network will be updated on this frame.
    bool updateNow_{};

    /// Servers in teardown — held by shared ptr until all their connections disconnect.
    ea::hash_set<SharedPtr<NetworkServer>> stoppingServers_;
    /// Connections in teardown — held by shared ptr until fully disconnected.
    ea::hash_set<SharedPtr<NetworkConnection>> disconnectingConnections_;
};

/// Register Network library objects.
/// @nobind
void URHO3D_API RegisterNetworkLibrary(Context* context);

} // namespace Urho3D
