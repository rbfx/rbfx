//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <EASTL/hash_set.h>

#include <Urho3D/Core/Object.h>

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

}
