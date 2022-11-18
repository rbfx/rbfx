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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/WorkQueue.h"
#include "../Engine/EngineEvents.h"
#include "../IO/FileSystem.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Input/InputEvents.h"
#include "../Network/HttpRequest.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/Protocol.h"
#include "../Network/Transport/DataChannel/DataChannelConnection.h"
#include "../Network/Transport/DataChannel/DataChannelServer.h"
#include "../Replica/BehaviorNetworkObject.h"
#include "../Replica/FilteredByDistance.h"
#include "../Replica/NetworkObject.h"
#include "../Replica/PredictedKinematicController.h"
#include "../Replica/ReplicatedAnimation.h"
#include "../Replica/ReplicatedTransform.h"
#include "../Replica/ReplicationManager.h"
#include "../Replica/StaticNetworkObject.h"
#include "../Replica/TrackedAnimatedModel.h"
#include "../Scene/Scene.h"

#ifdef SendMessage
#undef SendMessage
#endif

#include "../DebugNew.h"

namespace Urho3D
{

Network::Network(Context* context)
    : Object(context)
{
    // Register Network library object factories
    RegisterNetworkLibrary(context_);

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Network, HandleBeginFrame));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Network, HandleRenderUpdate));
    SubscribeToEvent(E_APPLICATIONSTOPPED, URHO3D_HANDLER(Network, HandleApplicationExit));
}

Network::~Network()
{
    assert(clientConnections_.empty());
    assert(!IsServerRunning());
    assert(!serverConnection_);
}

void Network::OnClientConnected(Connection* connection)
{
    connection->Initialize();
    connection->SetIsClient(true);
    connection->SetConnectPending(true);
    clientConnections_.insert({connection->transportConnection_, SharedPtr(connection)});
    URHO3D_LOGINFO("Client {} connected", connection->ToString());

    using namespace ClientConnected;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = connection;
    connection->SendEvent(E_CLIENTCONNECTED, eventData);

    if (serverMaxConnections_ <= clientConnections_.size())
    {
        connection->SendMessage(MSG_CONNECTION_LIMIT_EXCEEDED);
        connection->Disconnect();
    }
}

void Network::OnClientDisconnected(Connection* connection)
{
    // Remove the client connection that corresponds to this MessageConnection
    URHO3D_LOGINFO("Client {} disconnected", connection->ToString());

    using namespace ClientDisconnected;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = connection;
    connection->SendEvent(E_CLIENTDISCONNECTED, eventData);

    clientConnections_.erase(connection->transportConnection_);
}

bool Network::Connect(const URL& url, Scene* scene, const VariantMap& identity)
{
    URHO3D_PROFILE("Connect");

    if (!serverConnection_)
    {
        URHO3D_LOGINFO("Connecting to server {}", url.ToString());
        DataChannelConnection* transportConnection = new DataChannelConnection(context_);
        serverConnection_ = new Connection(context_, transportConnection);
        serverConnection_->SetScene(scene);
        serverConnection_->SetIdentity(identity);
        serverConnection_->SetConnectPending(true);
        serverConnection_->SetIsClient(false);

        WorkQueue* queue = GetSubsystem<WorkQueue>();
        transportConnection->onConnected_ = [this, queue]()
        {
            queue->CallFromMainThread([this](int)
            {
                OnServerConnected(serverConnection_);
            });
        };
        transportConnection->onDisconnected_ = transportConnection->onError_ = [this, queue]()
        {
            queue->CallFromMainThread([this](int)
            {
                OnServerDisconnected(serverConnection_);
            });
        };

        transportConnection->Connect(url);
        return true;
    }
    else if (serverConnection_->IsConnected())
    {
        URHO3D_LOGWARNING("Already connected to server!");
        SendEvent(E_CONNECTIONINPROGRESS);  // TODO: This is not used by the engine/samples and naming is weird. Torch it?
        return false;
    }
    else if (serverConnection_->IsConnectPending())
    {
        URHO3D_LOGWARNING("Connection attempt already in progress!");
        SendEvent(E_CONNECTIONINPROGRESS);
        return false;
    }
    else
    {
        URHO3D_LOGERROR("Failed to connect to server {}.", url.ToString());
        SendEvent(E_CONNECTFAILED);
        return false;
    }
}

void Network::Disconnect(int waitMSec)
{
    if (!serverConnection_)
        return;

    URHO3D_PROFILE("Disconnect");
    serverConnection_->Disconnect();
}

void Network::OnServerConnected(Connection* connection)
{
    connection->Initialize();
    connection->SetConnectPending(false);

    URHO3D_LOGINFO("Connected to server!");

    // Send the identity map now
    VectorBuffer msg;
    msg.WriteVariantMap(connection->GetIdentity());
    connection->SendMessage(MSG_IDENTITY, msg);

    SendEvent(E_SERVERCONNECTED);
}

void Network::OnServerDisconnected(Connection* connection)
{
    // Differentiate between failed connection, and disconnection
    assert(serverConnection_ == connection);
    bool failedConnect = serverConnection_ && serverConnection_->IsConnectPending();
    serverConnection_.Reset();

    if (!failedConnect)
    {
        URHO3D_LOGINFO("Disconnected from server");
        SendEvent(E_SERVERDISCONNECTED);
    }
    else
    {
        URHO3D_LOGERROR("Failed to connect to server");
        SendEvent(E_CONNECTFAILED);
    }
}

bool Network::StartServer(const URL& url, unsigned int maxConnections)
{
    if (IsServerRunning())
        return true;

    URHO3D_PROFILE("StartServer");

    WorkQueue* queue = GetSubsystem<WorkQueue>();
    transportServer_ = MakeShared<DataChannelServer>(context_);
    transportServer_->onConnected_ = [this, queue](NetworkConnection* wtConnection)
    {
        // Hold on to DataChannelConnection reference until callback executes.
        SharedPtr<DataChannelConnection> dataChannel(reinterpret_cast<DataChannelConnection*>(wtConnection));
        queue->CallFromMainThread([this, dataChannel](int)
        {
            OnClientConnected(new Connection(context_, dataChannel));
        });
    };
    transportServer_->onDisconnected_ = [this, queue](NetworkConnection* wtConnection)
    {
        // Similarly, ensure that dataChannel reference is kept until callback finishes executing.
        SharedPtr<DataChannelConnection> dataChannel(reinterpret_cast<DataChannelConnection*>(wtConnection));
        queue->CallFromMainThread([this, dataChannel](int)
        {
            auto it = clientConnections_.find(WeakPtr(dataChannel.Get()));
            if (it != clientConnections_.end())
                OnClientDisconnected(it->second);
        });

    };
    transportServer_->Listen(url);
    URHO3D_LOGINFO("Server is listening on {}.", url.ToString());
    serverMaxConnections_ = maxConnections;
    return true;
}

void Network::StopServer()
{
    for (auto& pair : clientConnections_)
        pair.second->Disconnect();
    clientConnections_.clear();

    if (!transportServer_)
        return;

    URHO3D_PROFILE("StopServer");
    transportServer_->Stop();
    transportServer_ = nullptr;

    URHO3D_LOGINFO("Stopped server");
}

void Network::BroadcastMessage(NetworkMessageId msgID, const VectorBuffer& msg, PacketTypeFlags packetType)
{
    BroadcastMessage(msgID, msg.GetData(), msg.GetSize(), packetType);
}

void Network::BroadcastMessage(NetworkMessageId msgID, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType)
{
    if (!transportServer_)
    {
        URHO3D_LOGERROR("Server not running, can not broadcast messages");
        return;
    }

    for (auto& pair : clientConnections_)
        pair.second->SendMessage(msgID, data, numBytes, packetType);
}

void Network::BroadcastRemoteEvent(StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    for (auto& connection : clientConnections_)
        connection.second->SendRemoteEvent(eventType, inOrder, eventData);
}

void Network::BroadcastRemoteEvent(Scene* scene, StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    for (auto& pair : clientConnections_)
    {
        if (pair.second->GetScene() == scene)
            pair.second->SendRemoteEvent(eventType, inOrder, eventData);
    }
}

void Network::SetUpdateFps(unsigned fps)
{
    if (IsServerRunning())
    {
        URHO3D_LOGERROR("Cannot change update frequency of running server. Attempted to change frequency from {} to {}.", updateFps_, fps);
        return;
    }

    updateFps_ = Max(fps, 1);
    updateInterval_ = 1.0f / (float)updateFps_;
    updateAcc_ = 0.0f;
}

void Network::SetPingIntervalMs(unsigned interval)
{
    if (IsServerRunning() || GetServerConnection())
        URHO3D_LOGWARNING("Cannot change ping interval for currently active connections.");

    pingIntervalMs_ = interval;
}

void Network::SetMaxPingIntervalMs(unsigned interval)
{
    if (IsServerRunning() || GetServerConnection())
        URHO3D_LOGWARNING("Cannot change max ping for currently active connections.");

    maxPingMs_ = interval;
}

void Network::SetClockBufferSize(unsigned size)
{
    if (IsServerRunning() || GetServerConnection())
        URHO3D_LOGWARNING("Cannot change sync buffer size for currently active connections.");

    clockBufferSize_ = size;
}

void Network::SetPingBufferSize(unsigned size)
{
    if (IsServerRunning() || GetServerConnection())
        URHO3D_LOGWARNING("Cannot change ping buffer size for currently active connections.");

    pingBufferSize_ = size;
}

void Network::RegisterRemoteEvent(StringHash eventType)
{
    allowedRemoteEvents_.insert(eventType);
}

void Network::UnregisterRemoteEvent(StringHash eventType)
{
    allowedRemoteEvents_.erase(eventType);
}

void Network::UnregisterAllRemoteEvents()
{
    allowedRemoteEvents_.clear();
}

void Network::SetPackageCacheDir(const ea::string& path)
{
    packageCacheDir_ = AddTrailingSlash(path);
}

void Network::SendPackageToClients(Scene* scene, PackageFile* package)
{
    if (!scene)
    {
        URHO3D_LOGERROR("Null scene specified for SendPackageToClients");
        return;
    }
    if (!package)
    {
        URHO3D_LOGERROR("Null package specified for SendPackageToClients");
        return;
    }

    for (auto& pair : clientConnections_)
    {
        if (pair.second->GetScene() == scene)
            pair.second->SendPackageToClient(package);
    }
}

SharedPtr<HttpRequest> Network::MakeHttpRequest(const ea::string& url, const ea::string& verb, const ea::vector<ea::string>& headers,
    const ea::string& postData)
{
    URHO3D_PROFILE("MakeHttpRequest");

    // The initialization of the request will take time, can not know at this point if it has an error or not
    SharedPtr<HttpRequest> request(new HttpRequest(url, verb, headers, postData));
    return request;
}

void Network::BanAddress(const ea::string& address)
{
    //rakPeer_->AddToBanList(address.c_str(), 0);
}

Connection* Network::GetServerConnection() const
{
    return serverConnection_;
}

ea::vector<SharedPtr<Connection>> Network::GetClientConnections() const
{
    ea::vector<SharedPtr<Connection>> connections;
    for (auto&[dataChannel, connection] : clientConnections_)
        connections.push_back(connection);
    return connections;
}

bool Network::IsServerRunning() const
{
    return transportServer_ != nullptr;
}

bool Network::CheckRemoteEvent(StringHash eventType) const
{
    return allowedRemoteEvents_.contains(eventType);
}

ea::string Network::GetDebugInfo() const
{
    ea::string result;
    /*
    ea::hash_set<ReplicationManager*> replicationManagers;

    const unsigned localTime = Time::GetSystemTime();
    result += Format("Local Time {}\n", localTime);

    if (Connection* connection = GetServerConnection())
    {
        result += Format("Server Connection {}: {}p-{}b/s in, {}p-{}b/s out, Remote Time {}\n",
            connection->ToString(),
            connection->GetPacketsInPerSec(), connection->GetBytesInPerSec(),
            connection->GetPacketsOutPerSec(), connection->GetBytesOutPerSec(),
            connection->LocalToRemoteTime(localTime));

        if (Scene* scene = connection->GetScene())
        {
            if (auto replicationManager = scene->GetComponent<ReplicationManager>())
                replicationManagers.insert(replicationManager);
        }
    }

    for (Connection* connection : GetClientConnections())
    {
        result += Format("Client Connection {}: {}p-{}b/s in, {}p-{}b/s out, Remote Time {}\n",
            connection->ToString(),
            connection->GetPacketsInPerSec(), connection->GetBytesInPerSec(),
            connection->GetPacketsOutPerSec(), connection->GetBytesOutPerSec(),
            connection->LocalToRemoteTime(localTime));

        if (Scene* scene = connection->GetScene())
        {
            if (auto replicationManager = scene->GetComponent<ReplicationManager>())
                replicationManagers.insert(replicationManager);
        }
    }

    for (ReplicationManager* replicationManager : replicationManagers)
        result += replicationManager->GetDebugInfo();
    */

    return result;
}

//void Network::HandleIncomingPacket(SLNet::Packet* packet, bool isServer)
//{
    //unsigned char packetID = packet->data[0];
    //bool packetHandled = false;
    //
    //// Deal with timestamped backents
    //unsigned dataStart = sizeof(char);
    //if (packetID == ID_TIMESTAMP)
    //{
    //    dataStart += sizeof(SLNet::Time);
    //    packetID = packet->data[dataStart];
    //    dataStart += sizeof(char);
    //}
    //
    //if (packetID == ID_NEW_INCOMING_CONNECTION)
    //{
    //    if (isServer)
    //    {
    //        OnClientConnected(packet->systemAddress);
    //        packetHandled = true;
    //    }
    //}
    //else if (packetID == ID_ALREADY_CONNECTED)
    //{
    //    if (natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
    //        URHO3D_LOGINFO("Already connected to NAT server! ");
    //        if (!isServer)
    //        {
    //            natPunchthroughClient_->OpenNAT(*remoteGUID_, *natPunchServerAddress_);
    //        }
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_CONNECTION_REQUEST_ACCEPTED) // We're a client, our connection as been accepted
    //{
    //    if(natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
    //        URHO3D_LOGINFO("Succesfully connected to NAT punchtrough server! ");
    //        SendEvent(E_NATMASTERCONNECTIONSUCCEEDED);
    //        if (!isServer)
    //        {
    //            natPunchthroughClient_->OpenNAT(*remoteGUID_, *natPunchServerAddress_);
    //        }
    //    } else {
    //        if (!isServer)
    //        {
    //            OnServerConnected(packet->systemAddress);
    //        }
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_NAT_TARGET_NOT_CONNECTED)
    //{
    //    URHO3D_LOGERROR("Target server not connected to NAT master server!");
    //    packetHandled = true;
    //}
    //else if (packetID == ID_CONNECTION_LOST) // We've lost connectivity with the packet source
    //{
    //    if (isServer)
    //    {
    //        ClientDisconnected(packet->systemAddress);
    //    }
    //    else
    //    {
    //        OnServerDisconnected(packet->systemAddress);
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_DISCONNECTION_NOTIFICATION) // We've lost connection with the other side
    //{
    //    if (isServer)
    //    {
    //        ClientDisconnected(packet->systemAddress);
    //    }
    //    else
    //    {
    //        OnServerDisconnected(packet->systemAddress);
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_CONNECTION_ATTEMPT_FAILED) // We've failed to connect to the server/peer
    //{
    //    if (natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
    //        URHO3D_LOGERROR("Connection to NAT punchtrough server failed!");
    //        SendEvent(E_NATMASTERCONNECTIONFAILED);
    //
    //    } else {
    //
    //        if (!isServer)
    //        {
    //            OnServerDisconnected(packet->systemAddress);
    //        }
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_NAT_PUNCHTHROUGH_SUCCEEDED)
    //{
    //    SLNet::SystemAddress remotePeer = packet->systemAddress;
    //    URHO3D_LOGINFO("NAT punchtrough succeeded! Remote peer: " + ea::string(remotePeer.ToString()));
    //    if (!isServer)
    //    {
    //        using namespace NetworkNatPunchtroughSucceeded;
    //        VariantMap eventMap;
    //        eventMap[P_ADDRESS] = remotePeer.ToString(false);
    //        eventMap[P_PORT] = remotePeer.GetPort();
    //        SendEvent(E_NETWORKNATPUNCHTROUGHSUCCEEDED, eventMap);
    //        URHO3D_LOGINFO("Connecting to server behind NAT: " + ea::string(remotePeer.ToString()));
    //        Connect(ea::string(remotePeer.ToString(false)), remotePeer.GetPort(), scene_, identity_);
    //    }
    //    packetHandled = true;
    //}
    //else if (packetID == ID_NAT_PUNCHTHROUGH_FAILED)
    //{
    //    URHO3D_LOGERROR("NAT punchtrough failed!");
    //    SLNet::SystemAddress remotePeer = packet->systemAddress;
    //    using namespace NetworkNatPunchtroughFailed;
    //    VariantMap eventMap;
    //    eventMap[P_ADDRESS] = remotePeer.ToString(false);
    //    eventMap[P_PORT] = remotePeer.GetPort();
    //    SendEvent(E_NETWORKNATPUNCHTROUGHFAILED, eventMap);
    //    packetHandled = true;
    //}
    //else if (packetID == ID_CONNECTION_BANNED) // We're a client and we're on the ban list
    //{
    //    URHO3D_LOGERROR("Connection failed, you're banned!");
    //    SendEvent(E_NETWORKBANNED);
    //    packetHandled = true;
    //}
    //else if (packetID == ID_INVALID_PASSWORD) // We're a client, and we gave an invalid password
    //{
    //    URHO3D_LOGERROR("Invalid password provided for connection!");
    //    SendEvent(E_NETWORKINVALIDPASSWORD);
    //    packetHandled = true;
    //}
    //else if (packetID == ID_DOWNLOAD_PROGRESS) // Part of a file transfer
    //{
    //    //URHO3D_LOGINFO("101010");
    //}
    //else if (packetID == ID_UNCONNECTED_PING)
    //{
    //    packetHandled = true;
    //}
    //else if (packetID == ID_UNCONNECTED_PONG) // Host discovery response
    //{
    //    if (!isServer)
    //    {
    //        using namespace NetworkHostDiscovered;
    //
    //        dataStart += sizeof(SLNet::TimeMS);
    //        VariantMap& eventMap = context_->GetEventDataMap();
    //        if (packet->length > packet->length - dataStart) {
    //            VectorBuffer buffer(packet->data + dataStart, packet->length - dataStart);
    //            VariantMap srcData = buffer.ReadVariantMap();
    //            eventMap[P_BEACON] = srcData;
    //        }
    //        else {
    //            eventMap[P_BEACON] = VariantMap();
    //        }
    //
    //        eventMap[P_ADDRESS] = ea::string(packet->systemAddress.ToString(false));
    //        eventMap[P_PORT] = (int)packet->systemAddress.GetPort();
    //        SendEvent(E_NETWORKHOSTDISCOVERED, eventMap);
    //    }
    //    packetHandled = true;
    //}
    //
    //// Urho3D messages
    //if (packetID >= ID_USER_PACKET_ENUM)
    //{
    //    unsigned int messageID = *(unsigned int*)(packet->data + dataStart);
    //    dataStart += sizeof(unsigned int);
    //
    //    if (isServer)
    //    {
    //        HandleMessage(packet->systemAddress, 0, messageID, (const char*)(packet->data + dataStart), packet->length - dataStart);
    //    }
    //    else
    //    {
    //        MemoryBuffer buffer(packet->data + dataStart, packet->length - dataStart);
    //        bool processed = serverConnection_ && serverConnection_->ProcessMessage(messageID, buffer);
    //        if (!processed)
    //        {
    //            HandleMessage(packet->systemAddress, 0, messageID, (const char*)(packet->data + dataStart), packet->length - dataStart);
    //        }
    //    }
    //    packetHandled = true;
    //}
    //
    //if (!packetHandled && packetID < ea::size(RAKNET_MESSAGEID_STRINGS))
    //    URHO3D_LOGERROR("Unhandled network packet: " + ea::string(RAKNET_MESSAGEID_STRINGS[packetID]));
    //else if (!packetHandled)
    //    URHO3D_LOGERRORF("Unhandled network packet: %i", packetID);
//
//}

void Network::Update(float timeStep)
{
    URHO3D_PROFILE("UpdateNetwork");

    // Check if periodic update should happen now
    updateAcc_ += timeStep;
    updateNow_ = updateAcc_ >= updateInterval_;
    if (updateNow_)
        updateAcc_ = fmodf(updateAcc_, updateInterval_);

    {
        using namespace NetworkInputProcessed;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_TIMESTEP] = timeStep;
        SendEvent(E_NETWORKINPUTPROCESSED, eventData);
    }
}

void Network::PostUpdate(float timeStep)
{
    URHO3D_PROFILE("PostUpdateNetwork");

    // Update periodically on the server
    if (updateNow_ && (IsServerRunning() || simulateServerEvents_))
    {
        SendNetworkUpdateEvent(E_NETWORKUPDATE, true);

        if (IsServerRunning())
        {
            URHO3D_PROFILE("SendServerUpdate");

            // Then send server updates for each client connection
            for (auto& pair : clientConnections_)
            {
                pair.second->SendRemoteEvents();
                pair.second->SendPackages();
                pair.second->SendAllBuffers();
                pair.second->ProcessPackets();
            }
        }

        SendNetworkUpdateEvent(E_NETWORKUPDATESENT, true);
    }

    // Always update on the client
    SendNetworkUpdateEvent(E_NETWORKUPDATE, false);
    if (serverConnection_)
    {
        serverConnection_->SendRemoteEvents();
        serverConnection_->SendAllBuffers();
        serverConnection_->ProcessPackets();
    }
    SendNetworkUpdateEvent(E_NETWORKUPDATESENT, false);
}

void Network::HandleApplicationExit(StringHash eventType, VariantMap& eventData)
{
    if (serverConnection_)
        serverConnection_->Disconnect();
    StopServer();

    // Connection shutdown is triggered. Wait until MsQuic callbacks receive shutdown events and deinitialize all streams and connections.
    // This will result in eventual deletion of Connection objects.
    auto queue = GetSubsystem<WorkQueue>();
    while (serverConnection_ || !clientConnections_.empty())
    {
        // Since we block main thread until all connections close, we must manually invoke queued callbacks, because MsQuic connection and
        // stream callbacks depend on WorkQueue::CallFromMainThread to do object deinitialization (which also sends events) in main thread.
        //queue->ProcessMainThreadTasks();
        std::this_thread::yield();
    }
}

void Network::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    using namespace BeginFrame;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Network::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace RenderUpdate;

    PostUpdate(eventData[P_TIMESTEP].GetFloat());
}

void Network::SendNetworkUpdateEvent(StringHash eventType, bool isServer)
{
    using namespace NetworkUpdate;
    auto& eventData = GetEventDataMap();
    eventData[P_ISSERVER] = isServer;
    SendEvent(eventType, eventData);
}

void RegisterNetworkLibrary(Context* context)
{
    NetworkObjectRegistry::RegisterObject(context);
    ReplicationManager::RegisterObject(context);

    NetworkObject::RegisterObject(context);
    StaticNetworkObject::RegisterObject(context);
    BehaviorNetworkObject::RegisterObject(context);

    NetworkBehavior::RegisterObject(context);
    ReplicatedAnimation::RegisterObject(context);
    ReplicatedTransform::RegisterObject(context);
    TrackedAnimatedModel::RegisterObject(context);
    FilteredByDistance::RegisterObject(context);
#ifdef URHO3D_PHYSICS
    PredictedKinematicController::RegisterObject(context);
#endif

    Connection::RegisterObject(context);
    //DataChannelConnection::RegisterObject(context);
    //DataChannelServer::RegisterObject(context);
}

}
