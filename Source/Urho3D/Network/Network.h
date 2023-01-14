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

#include "../Core/Object.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Connection.h"

namespace Urho3D
{

class HttpRequest;
class MemoryBuffer;
class Scene;

/// %Network subsystem. Manages client-server communications using the UDP protocol.
class URHO3D_API Network : public Object
{
    URHO3D_OBJECT(Network, Object);

public:
    /// Construct.
    explicit Network(Context* context);
    /// Destruct.
    ~Network() override;

    /// Handle an inbound message.
    void HandleMessage(const SLNet::AddressOrGUID& source, int packetID, int msgID, const char* data, size_t numBytes);
    /// Handle a new client connection.
    void NewConnectionEstablished(const SLNet::AddressOrGUID& connection);
    /// Handle a client disconnection.
    void ClientDisconnected(const SLNet::AddressOrGUID& connection);

    /// Set the data that will be used for a reply to attempts at host discovery on LAN/subnet.
    void SetDiscoveryBeacon(const VariantMap& data);
    /// Scan the LAN/subnet for available hosts.
    void DiscoverHosts(unsigned port);
    /// Set password for the client/server communcation.
    void SetPassword(const ea::string& password);
    /// Set NAT server information.
    void SetNATServerInfo(const ea::string& address, unsigned short port);
    /// Connect to a server using UDP protocol. Return true if connection process successfully started.
    bool Connect(const ea::string& address, unsigned short port, Scene* scene, const VariantMap& identity = Variant::emptyVariantMap);
    /// Disconnect the connection to the server. If wait time is non-zero, will block while waiting for disconnect to finish.
    void Disconnect(int waitMSec = 0);
    /// Start a server on a port using UDP protocol. Return true if successful.
    bool StartServer(unsigned short port, unsigned int maxConnections = 128);
    /// Stop the server.
    void StopServer();
    /// Start NAT punchtrough client to allow remote connections.
    void StartNATClient();
    /// Get local server GUID.
    /// @property{get_guid}
    const ea::string& GetGUID() const { return guid_; }
    /// Attempt to connect to NAT server.
    void AttemptNATPunchtrough(const ea::string& guid, Scene* scene, const VariantMap& identity = Variant::emptyVariantMap);
    /// Broadcast a message with content ID to all client connections.
    void BroadcastMessage(int msgID, bool reliable, bool inOrder, const VectorBuffer& msg, unsigned contentID = 0);
    /// Broadcast a message with content ID to all client connections.
    void BroadcastMessage(int msgID, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes, unsigned contentID = 0);
    /// Broadcast a remote event to all client connections.
    void BroadcastRemoteEvent(StringHash eventType, bool inOrder, const VariantMap& eventData = Variant::emptyVariantMap);
    /// Broadcast a remote event to all client connections in a specific scene.
    void BroadcastRemoteEvent(Scene* scene, StringHash eventType, bool inOrder, const VariantMap& eventData = Variant::emptyVariantMap);
    /// Set network update FPS.
    /// @property
    void SetUpdateFps(unsigned fps);
    /// Set interval of pings by server.
    void SetPingIntervalMs(unsigned interval);
    /// Set max allowed ping by server.
    void SetMaxPingIntervalMs(unsigned interval);
    /// Set number of clock synchronization samples used.
    void SetClockBufferSize(unsigned size);
    /// Set number of ping samples used.
    void SetPingBufferSize(unsigned size);
    /// Set simulated latency in milliseconds. This adds a fixed delay before sending each packet.
    /// @property
    void SetSimulatedLatency(int ms);
    /// Set simulated packet loss probability between 0.0 - 1.0.
    /// @property
    void SetSimulatedPacketLoss(float probability);
    /// Test only. Set whether to send events as server.
    void SetSimulateServerEvents(bool enable) { simulateServerEvents_ = enable; }
    /// Test only. Set whether to send events as client.
    void SetSimulateClientEvents(bool enable) { simulateClientEvents_ = enable; }
    /// Register a remote event as allowed to be received. There is also a fixed blacklist of events that can not be allowed in any case, such as ConsoleCommand.
    void RegisterRemoteEvent(StringHash eventType);
    /// Unregister a remote event as allowed to received.
    void UnregisterRemoteEvent(StringHash eventType);
    /// Unregister all remote events.
    void UnregisterAllRemoteEvents();
    /// Set the package download cache directory.
    /// @property
    void SetPackageCacheDir(const ea::string& path);
    /// Trigger all client connections in the specified scene to download a package file from the server. Can be used to download additional resource packages when clients are already joined in the scene. The package must have been added as a requirement to the scene, or else the eventual download will fail.
    void SendPackageToClients(Scene* scene, PackageFile* package);
    /// Perform an HTTP request to the specified URL. Empty verb defaults to a GET request. Return a request object which can be used to read the response data.
    SharedPtr<HttpRequest> MakeHttpRequest(const ea::string& url, const ea::string& verb = EMPTY_STRING, const ea::vector<ea::string>& headers = ea::vector<ea::string>(), const ea::string& postData = EMPTY_STRING);
    /// Ban specific IP addresses.
    void BanAddress(const ea::string& address);
    /// Return network update FPS.
    /// @property
    unsigned GetUpdateFps() const { return updateFps_; }
    /// Return interval of pings by server.
    unsigned GetPingIntervalMs() const { return pingIntervalMs_; }
    /// Return max allowed ping by server.
    unsigned GetMaxPingIntervalMs() const { return maxPingMs_; }
    /// Return number of clock synchronization samples used.
    unsigned GetClockBufferSize() const { return clockBufferSize_; }
    /// Return number of ping synchronization samples used.
    unsigned GetPingBufferSize() const { return pingBufferSize_; }

    /// Return simulated latency in milliseconds.
    /// @property
    int GetSimulatedLatency() const { return simulatedLatency_; }

    /// Return simulated packet loss probability.
    /// @property
    float GetSimulatedPacketLoss() const { return simulatedPacketLoss_; }

    /// Return the amount of time that happened after fixed-time network update.
    float GetUpdateOvertime() const { return updateAcc_; }

    /// Return whether the network is updated on this frame.
    bool IsUpdateNow() const { return updateNow_; }

    /// Return a client or server connection by RakNet connection address, or null if none exist.
    Connection* GetConnection(const SLNet::AddressOrGUID& connection) const;
    /// Return the connection to the server. Null if not connected.
    /// @property
    Connection* GetServerConnection() const;
    /// Return all client connections.
    /// @property
    ea::vector<SharedPtr<Connection> > GetClientConnections() const;
    /// Return whether the server is running.
    /// @property
    bool IsServerRunning() const;
    /// Return whether a remote event is allowed to be received.
    bool CheckRemoteEvent(StringHash eventType) const;
    /// Return aggregated debug info.
    ea::string GetDebugInfo() const;

    /// Return the package download cache directory.
    /// @property
    const ea::string& GetPackageCacheDir() const { return packageCacheDir_; }

    /// Process incoming messages from connections. Called by HandleBeginFrame.
    void Update(float timeStep);
    /// Send outgoing messages after frame logic. Called by HandleRenderUpdate.
    void PostUpdate(float timeStep);

private:
    /// Handle begin frame event.
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    /// Handle render update frame event.
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle server connection.
    void OnServerConnected(const SLNet::AddressOrGUID& address);
    /// Handle server disconnection.
    void OnServerDisconnected(const SLNet::AddressOrGUID& address);
    /// Reconfigure network simulator parameters on all existing connections.
    void ConfigureNetworkSimulator();
    /// All incoming packages are handled here.
    void HandleIncomingPacket(SLNet::Packet* packet, bool isServer);
    /// Return hash of endpoint.
    static unsigned long GetEndpointHash(const SLNet::AddressOrGUID& endpoint);

    void SendNetworkUpdateEvent(StringHash eventType, bool isServer);

    /// Used for testing only
    /// @{
    bool simulateServerEvents_{};
    bool simulateClientEvents_{};
    /// @}

    /// Properties that need connection reset to apply
    /// @{
    unsigned updateFps_{30};
    unsigned pingIntervalMs_{250};
    unsigned maxPingMs_{10000};
    unsigned clockBufferSize_{40};
    unsigned pingBufferSize_{10};
    /// @}

    /// SLikeNet peer instance for server connection.
    SLNet::RakPeerInterface* rakPeer_;
    /// SLikeNet peer instance for client connection.
    SLNet::RakPeerInterface* rakPeerClient_;
    /// Client's server connection.
    SharedPtr<Connection> serverConnection_;
    /// Server's client connections. Key is SLNet::AddressOrGUID hash.
    ea::unordered_map<unsigned long, SharedPtr<Connection> > clientConnections_;
    /// Allowed remote events.
    ea::hash_set<StringHash> allowedRemoteEvents_;
    /// Simulated latency (send delay) in milliseconds.
    int simulatedLatency_;
    /// Simulated packet loss probability between 0.0 - 1.0.
    float simulatedPacketLoss_;
    /// Update time interval.
    float updateInterval_;
    /// Update time accumulator.
    float updateAcc_;
    /// Whether the network will be updated on this frame.
    bool updateNow_{};
    /// Package cache directory.
    ea::string packageCacheDir_;
    /// Whether we started as server or not.
    bool isServer_;
    /// Server/Client password used for connecting.
    ea::string password_;
    /// Scene which will be used for NAT punchtrough connections.
    Scene* scene_;
    /// Client identify for NAT punchtrough connections.
    VariantMap identity_;
    /// NAT punchtrough server information.
    SLNet::SystemAddress* natPunchServerAddress_;
    /// NAT punchtrough client for the server.
    SLNet::NatPunchthroughClient* natPunchthroughServerClient_;
    /// NAT punchtrough client for the client.
    SLNet::NatPunchthroughClient* natPunchthroughClient_;
    /// Remote GUID information.
    SLNet::RakNetGUID* remoteGUID_;
    /// Local server GUID.
    ea::string guid_;
};

/// Register Network library objects.
/// @nobind
void URHO3D_API RegisterNetworkLibrary(Context* context);

}
