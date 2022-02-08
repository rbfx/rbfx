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

/// \file

#pragma once

#include <EASTL/hash_set.h>

#include "../Core/Object.h"
#include "../Core/Timer.h"
#include "../Input/Controls.h"
#include "../IO/VectorBuffer.h"
#include "../Network/AbstractConnection.h"
#include "../Scene/ReplicationState.h"

namespace SLNet
{
    class SystemAddress;
    struct AddressOrGUID;
    struct RakNetGUID;
    struct Packet;
    class NatPunchthroughClient;
    class RakPeerInterface;
}

namespace Urho3D
{

class ClockSynchronizer;
class File;
class MemoryBuffer;
class ReplicationManager;
class Node;
class Scene;
class Serializable;
class PackageFile;

/// Queued remote event.
struct RemoteEvent
{
    /// Remote sender node ID (0 if not a remote node event).
    unsigned senderID_;
    /// Event type.
    StringHash eventType_;
    /// Event data.
    VariantMap eventData_;
    /// In order flag.
    bool inOrder_;
};

/// Package file receive transfer.
struct PackageDownload
{
    /// Construct with defaults.
    PackageDownload();

    /// Destination file.
    SharedPtr<File> file_;
    /// Already received fragments.
    ea::hash_set<unsigned> receivedFragments_;
    /// Package name.
    ea::string name_;
    /// Total number of fragments.
    unsigned totalFragments_;
    /// Checksum.
    unsigned checksum_;
    /// Download initiated flag.
    bool initiated_;
};

/// Package file send transfer.
struct PackageUpload
{
    /// Construct with defaults.
    PackageUpload();

    /// Source file.
    SharedPtr<File> file_;
    /// Current fragment index.
    unsigned fragment_;
    /// Total number of fragments.
    unsigned totalFragments_;
};

/// Send modes for observer position/rotation. Activated by the client setting either position or rotation.
enum ObserverPositionSendMode
{
    OPSM_NONE = 0,
    OPSM_POSITION,
    OPSM_POSITION_ROTATION
};

/// %Connection to a remote network host.
class URHO3D_API Connection : public AbstractConnection
{
    URHO3D_OBJECT(Connection, AbstractConnection);

public:
    using AbstractConnection::SendMessage;

    /// Construct with context, RakNet connection address and Raknet peer pointer.
    Connection(Context* context);
    /// Destruct.
    ~Connection() override;
    /// Initialize object state. Should be called immediately after constructor.
    void Initialize(bool isClient, const SLNet::AddressOrGUID& address, SLNet::RakPeerInterface* peer);

    /// Register object with the engine.
    static void RegisterObject(Context* context);

    /// Implement AbstractConnection
    /// @{
    void SendMessageInternal(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes) override;
    ea::string ToString() const override;
    bool IsClockSynchronized() const override;
    unsigned RemoteToLocalTime(unsigned time) const override;
    unsigned LocalToRemoteTime(unsigned time) const override;
    unsigned GetLocalTime() const override;
    unsigned GetLocalTimeOfLatestRoundtrip() const override;
    unsigned GetPing() const override;
    /// @}

    /// Get packet type based on the message parameters
    PacketType GetPacketType(bool reliable, bool inOrder);
    /// Send a remote event.
    void SendRemoteEvent(StringHash eventType, bool inOrder, const VariantMap& eventData = Variant::emptyVariantMap);
    /// Send a remote event with the specified node as sender.
    void SendRemoteEvent(Node* node, StringHash eventType, bool inOrder, const VariantMap& eventData = Variant::emptyVariantMap);
    /// Assign scene. On the server, this will cause the client to load it.
    /// @property
    void SetScene(Scene* newScene);
    /// Assign identity. Called by Network.
    void SetIdentity(const VariantMap& identity);
    /// Set new controls.
    void SetControls(const Controls& newControls);
    /// Set the observer position for interest management, to be sent to the server.
    /// @property
    void SetPosition(const Vector3& position);
    /// Set the observer rotation for interest management, to be sent to the server. Note: not used by the NetworkPriority component.
    /// @property
    void SetRotation(const Quaternion& rotation);
    /// Set the connection pending status. Called by Network.
    void SetConnectPending(bool connectPending);
    /// Set whether to log data in/out statistics.
    /// @property
    void SetLogStatistics(bool enable);
    /// Disconnect. If wait time is non-zero, will block while waiting for disconnect to finish.
    void Disconnect(int waitMSec = 0);
    /// Send scene update messages. Called by Network.
    void SendServerUpdate();
    /// Send latest controls from the client. Called by Network.
    void SendClientUpdate();
    /// Send queued remote events. Called by Network.
    void SendRemoteEvents();
    /// Send package files to client. Called by network.
    void SendPackages();
    /// Send out buffered messages by their type
    void SendBuffer(PacketType type);
    /// Send out all buffered messages
    void SendAllBuffers();
    /// Process pending latest data for nodes and components.
    void ProcessPendingLatestData();
    /// Process a message from the server or client. Called by Network.
    bool ProcessMessage(int msgID, MemoryBuffer& buffer);
    /// Ban this connections IP address.
    void Ban();
    /// Return the RakNet address/guid.
    const SLNet::AddressOrGUID& GetAddressOrGUID() const { return *address_; }
    /// Set the the RakNet address/guid.
    void SetAddressOrGUID(const SLNet::AddressOrGUID& addr);
    /// Return client identity.
    VariantMap& GetIdentity() { return identity_; }

    /// Return the scene used by this connection.
    /// @property
    Scene* GetScene() const;

    /// Return the client controls of this connection.
    const Controls& GetControls() const { return controls_; }

    /// Return the controls timestamp, sent from client to server along each control update.
    unsigned char GetTimeStamp() const { return timeStamp_; }

    /// Return the observer position sent by the client for interest management.
    /// @property
    const Vector3& GetPosition() const { return position_; }

    /// Return the observer rotation sent by the client for interest management.
    /// @property
    const Quaternion& GetRotation() const { return rotation_; }

    /// Return whether is a client connection.
    /// @property
    bool IsClient() const { return isClient_; }

    /// Return whether is fully connected.
    /// @property
    bool IsConnected() const;

    /// Return whether connection is pending.
    /// @property
    bool IsConnectPending() const { return connectPending_; }

    /// Return whether the scene is loaded and ready to receive server updates.
    /// @property
    bool IsSceneLoaded() const { return sceneLoaded_; }

    /// Return whether to log data in/out statistics.
    /// @property
    bool GetLogStatistics() const { return logStatistics_; }

    /// Return remote address.
    /// @property
    ea::string GetAddress() const;

    /// Return remote port.
    /// @property
    unsigned short GetPort() const { return port_; }

    /// Return the connection's round trip time in milliseconds.
    /// @property
    float GetRoundTripTime() const;

    /// Return the time since last received data from the remote host in milliseconds.
    /// @property
    unsigned GetLastHeardTime() const;

    /// Return bytes received per second.
    /// @property
    unsigned long long GetBytesInPerSec() const;

    /// Return bytes sent per second.
    /// @property
    unsigned long long GetBytesOutPerSec() const;

    /// Return packets received per second.
    /// @property
    int GetPacketsInPerSec() const;

    /// Return packets sent per second.
    /// @property
    int GetPacketsOutPerSec() const;

    /// Return number of package downloads remaining.
    /// @property
    unsigned GetNumDownloads() const;
    /// Return name of current package download, or empty if no downloads.
    /// @property
    const ea::string& GetDownloadName() const;
    /// Return progress of current package download, or 1.0 if no downloads.
    /// @property
    float GetDownloadProgress() const;
    /// Trigger client connection to download a package file from the server. Can be used to download additional resource packages when client is already joined in a scene. The package must have been added as a requirement to the scene the client is joined in, or else the eventual download will fail.
    void SendPackageToClient(PackageFile* package);

    /// Set network simulation parameters. Called by Network.
    void ConfigureNetworkSimulator(int latencyMs, float packetLoss);
    /// Buffered packet size limit, when reached, packet is sent out immediately
    void SetPacketSizeLimit(int limit);

    /// Current controls.
    Controls controls_;
    /// Controls timestamp. Incremented after each sent update.
    unsigned char timeStamp_;
    /// Identity map.
    VariantMap identity_;

private:
    /// Handle scene loaded event.
    void HandleAsyncLoadFinished(StringHash eventType, VariantMap& eventData);
    /// Process a LoadScene message from the server. Called by Network.
    void ProcessLoadScene(int msgID, MemoryBuffer& msg);
    /// Process a SceneChecksumError message from the server. Called by Network.
    void ProcessSceneChecksumError(int msgID, MemoryBuffer& msg);
    /// Process a scene update message from the server. Called by Network.
    void ProcessSceneUpdate(int msgID, MemoryBuffer& msg);
    /// Process package download related messages. Called by Network.
    void ProcessPackageDownload(int msgID, MemoryBuffer& msg);
    /// Process an Identity message from the client. Called by Network.
    void ProcessIdentity(int msgID, MemoryBuffer& msg);
    /// Process a Controls message from the client. Called by Network.
    void ProcessControls(int msgID, MemoryBuffer& msg);
    /// Process a SceneLoaded message from the client. Called by Network.
    void ProcessSceneLoaded(int msgID, MemoryBuffer& msg);
    /// Process a remote event message from the client or server. Called by Network.
    void ProcessRemoteEvent(int msgID, MemoryBuffer& msg);
    /// Process a node for sending a network update. Recurses to process depended on node(s) first.
    void ProcessNode(unsigned nodeID);
    /// Process a node that the client has not yet received.
    void ProcessNewNode(Node* node);
    /// Process a node that the client has already received.
    void ProcessExistingNode(Node* node, NodeReplicationState& nodeState);
    /// Process a SyncPackagesInfo message from server.
    void ProcessPackageInfo(int msgID, MemoryBuffer& msg);
    /// Process unknown message. All unknown messages are forwarded as an events
    void ProcessUnknownMessage(int msgID, MemoryBuffer& msg);
    /// Check a package list received from server and initiate package downloads as necessary. Return true on success, or false if failed to initialze downloads (cache dir not set).
    bool RequestNeededPackages(unsigned numPackages, MemoryBuffer& msg);
    /// Initiate a package download.
    void RequestPackage(const ea::string& name, unsigned fileSize, unsigned checksum);
    /// Send an error reply for a package download.
    void SendPackageError(const ea::string& name);
    /// Handle scene load failure on the server or client.
    void OnSceneLoadFailed();
    /// Handle a package download failure on the client.
    void OnPackageDownloadFailed(const ea::string& name);
    /// Handle all packages loaded successfully. Also called directly on MSG_LOADSCENE if there are none.
    void OnPackagesReady();

    /// Utility to keep server and client clocks synchronized.
    ea::unique_ptr<ClockSynchronizer> clock_;
    /// Scene.
    WeakPtr<Scene> scene_;
    /// Scene replication and synchronization manager.
    WeakPtr<ReplicationManager> replicationManager_;

    /// Network replication state of the scene.
    SceneReplicationState sceneState_;
    /// Waiting or ongoing package file receive transfers.
    ea::unordered_map<StringHash, PackageDownload> downloads_;
    /// Ongoing package send transfers.
    ea::unordered_map<StringHash, PackageUpload> uploads_;
    /// Pending latest data for not yet received nodes.
    ea::unordered_map<unsigned, ea::vector<unsigned char> > nodeLatestData_;
    /// Pending latest data for not yet received components.
    ea::unordered_map<unsigned, ea::vector<unsigned char> > componentLatestData_;
    /// Node ID's to process during a replication update.
    ea::hash_set<unsigned> nodesToProcess_;
    /// Queued remote events.
    ea::vector<RemoteEvent> remoteEvents_;
    /// Scene file to load once all packages (if any) have been downloaded.
    ea::string sceneFileName_;
    /// Statistics timer.
    Timer statsTimer_;
    /// Remote endpoint port.
    unsigned short port_;
    /// Observer position for interest management.
    Vector3 position_;
    /// Observer rotation for interest management.
    Quaternion rotation_;
    /// Send mode for the observer position & rotation.
    ObserverPositionSendMode sendMode_;
    /// Client connection flag.
    bool isClient_;
    /// Connection pending flag.
    bool connectPending_;
    /// Scene loaded flag.
    bool sceneLoaded_;
    /// Show statistics flag.
    bool logStatistics_;
    /// Address of this connection.
    SLNet::AddressOrGUID* address_;
    /// Raknet peer object.
    SLNet::RakPeerInterface* peer_;
    /// Temporary variable to hold packet count in the next second, x - packets in, y - packets out.
    IntVector2 tempPacketCounter_;
    /// Packet count in the last second, x - packets in, y - packets out.
    IntVector2 packetCounter_;
    /// Packet count timer which resets every 1s.
    Timer packetCounterTimer_;
    /// Last heard timer, resets when new packet is incoming.
    Timer lastHeardTimer_;
    /// Outgoing packet buffer which can contain multiple messages
    ea::unordered_map<int, VectorBuffer> outgoingBuffer_;
    /// Outgoing packet size limit
    int packedMessageLimit_;
};

}
