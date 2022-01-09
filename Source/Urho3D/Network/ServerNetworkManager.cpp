//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkObject.h"
#include "../Network/NetworkManager.h"
#include "../Network/ServerNetworkManager.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

unsigned GetIndex(NetworkId networkId)
{
    return NetworkManager::DecomposeNetworkId(networkId).first;
}

}

ServerNetworkManager::ServerNetworkManager(NetworkManagerBase* base, Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , base_(base)
    , scene_(scene)
    , updateFrequency_(network_->GetUpdateFps())
    , physicsSync_(scene_, updateFrequency_, false)
{
    SubscribeToEvent(E_INPUTREADY, [this](StringHash, VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();

        const bool isUpdateNow = network_->IsUpdateNow();
        const float overtime = network_->GetUpdateOvertime();
        const auto resetValue = isUpdateNow ? ea::make_optional(overtime) : ea::nullopt;
        physicsSync_.UpdateClock(timeStep, resetValue);
        if (isUpdateNow)
            BeginNetworkFrame();
    });

    SubscribeToEvent(network_, E_NETWORKUPDATE, [this](StringHash, VariantMap&)
    {
        PrepareNetworkFrame();
        for (auto& [connection, data] : connections_)
            SendUpdate(data);
    });
}

ServerNetworkManager::~ServerNetworkManager()
{
}

void ServerNetworkManager::BeginNetworkFrame()
{
    const float timeStep = 1.0f / updateFrequency_;
    UpdateClocks(timeStep);

    network_->SendEvent(E_BEGINSERVERNETWORKUPDATE);
}

void ServerNetworkManager::PrepareNetworkFrame()
{
    const float timeStep = 1.0f / updateFrequency_;
    CollectObjectsToUpdate(timeStep);
    PrepareDeltaUpdates();
}

void ServerNetworkManager::UpdateClocks(float timeStep)
{
    ++currentFrame_;

    for (auto& [connection, data] : connections_)
    {
        if (!data.synchronized_)
            return;

        data.pingAccumulator_ += timeStep;
        data.clockAccumulator_ += timeStep;
    }
}

void ServerNetworkManager::CollectObjectsToUpdate(float timeStep)
{
    const auto& recentlyAddedObjects = base_->GetRecentlyAddedComponents();
    for (NetworkId networkId : recentlyAddedObjects)
    {
        NetworkObject* networkObject = base_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_ASSERTLOG(0, "Cannot find recently added NetworkObject");
            continue;
        }

        networkObject->SetNetworkMode(NetworkObjectMode::Server);
        networkObject->InitializeOnServer();
    }

    const unsigned maxIndex = base_->GetNetworkIndexUpperBound();
    deltaUpdateMask_.Clear(maxIndex);
    reliableDeltaUpdates_.resize(maxIndex);
    unreliableDeltaUpdates_.resize(maxIndex);

    // Collect objects to update
    base_->UpdateAndSortNetworkObjects(orderedNetworkObjects_);
    for (auto& [connection, data] : connections_)
    {
        if (!data.synchronized_)
            continue;

        data.isComponentReplicated_.resize(maxIndex);
        data.componentsRelevanceTimeouts_.resize(maxIndex);

        data.pendingRemovedComponents_.clear();
        data.pendingUpdatedComponents_.clear();

        // Process removed components first
        for (NetworkId networkId : base_->GetRecentlyRemovedComponents())
        {
            const unsigned index = GetIndex(networkId);
            if (data.isComponentReplicated_[index])
            {
                data.isComponentReplicated_[index] = false;
                data.pendingRemovedComponents_.push_back(networkId);
            }
        }

        // Process active components
        for (NetworkObject* networkObject : orderedNetworkObjects_)
        {
            const NetworkId networkId = networkObject->GetNetworkId();
            const unsigned index = GetIndex(networkId);

            if (!data.isComponentReplicated_[index])
            {
                if (networkObject->IsRelevantForClient(connection))
                {
                    // Begin replication of component, queue snapshot
                    data.componentsRelevanceTimeouts_[index] = settings_.relevanceTimeout_;
                    data.isComponentReplicated_[index] = true;
                    data.pendingUpdatedComponents_.push_back({ networkObject, true });
                }
            }
            else
            {
                data.componentsRelevanceTimeouts_[index] -= timeStep;
                if (data.componentsRelevanceTimeouts_[index] < 0.0f)
                {
                    if (!networkObject->IsRelevantForClient(connection))
                    {
                        // Remove irrelevant component
                        data.isComponentReplicated_[index] = false;
                        data.pendingRemovedComponents_.push_back(networkId);
                        continue;
                    }

                    data.componentsRelevanceTimeouts_[index] = settings_.relevanceTimeout_;
                }

                // Queue non-snapshot update
                deltaUpdateMask_.Set(index);
                data.pendingUpdatedComponents_.push_back({ networkObject, false });
            }
        }
    }
    base_->ClearRecentActions();
}

void ServerNetworkManager::PrepareDeltaUpdates()
{
    const unsigned maxIndex = base_->GetNetworkIndexUpperBound();
    deltaUpdateBuffer_.Clear();
    for (unsigned index = 0; index < maxIndex; ++index)
    {
        if (!deltaUpdateMask_.NeedAny(index))
            continue;

        NetworkObject* networkObject = base_->GetNetworkObjectByIndex(index);
        PrepareReliableDeltaForObject(index, networkObject);
        PrepareUnreliableDeltaForObject(index, networkObject);
    }
}

void ServerNetworkManager::PrepareReliableDeltaForObject(unsigned index, NetworkObject* networkObject)
{
    const unsigned beginOffset = deltaUpdateBuffer_.Tell();
    if (networkObject->WriteReliableDelta(currentFrame_, deltaUpdateBuffer_))
    {
        const unsigned endOffset = deltaUpdateBuffer_.Tell();
        reliableDeltaUpdates_[index] = { beginOffset, endOffset };
    }
    else
    {
        deltaUpdateBuffer_.Seek(beginOffset);
        deltaUpdateMask_.ResetReliableDelta(index);
    }
}

void ServerNetworkManager::PrepareUnreliableDeltaForObject(unsigned index, NetworkObject* networkObject)
{
    const unsigned beginOffset = deltaUpdateBuffer_.Tell();
    if (networkObject->WriteUnreliableDelta(currentFrame_, deltaUpdateBuffer_))
    {
        const unsigned endOffset = deltaUpdateBuffer_.Tell();
        unreliableDeltaUpdates_[index] = { beginOffset, endOffset };
    }
    else
    {
        deltaUpdateBuffer_.Seek(beginOffset);
        deltaUpdateMask_.ResetUnreliableDelta(index);
    }
}

void ServerNetworkManager::AddConnection(AbstractConnection* connection)
{
    if (connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection {} is already added", connection->ToString());
        assert(0);
    }

    ClientConnectionData& data = connections_[connection];
    data = ClientConnectionData{};

    data.connection_ = connection;
    data.comfirmedPings_.set_capacity(settings_.numInitialPings_);
    data.comfirmedPingsSorted_.set_capacity(settings_.numInitialPings_);
    data.pendingPings_.set_capacity(settings_.maxOngoingPings_);
    data.feedbackDelay_.set_capacity(settings_.numFeedbackDelaySamples_);
    data.feedbackDelaySorted_.set_capacity(settings_.numFeedbackDelaySamples_);

    URHO3D_LOGINFO("Connection {} is added", connection->ToString());
}

void ServerNetworkManager::RemoveConnection(AbstractConnection* connection)
{
    if (!connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection {} is not added", connection->ToString());
        assert(0);
    }

    connections_.erase(connection);
    URHO3D_LOGINFO("Connection {} is removed", connection->ToString());
}

void ServerNetworkManager::SendUpdate(ClientConnectionData& data)
{
    if (!SendSynchronizationMessages(data))
        return;

    SendPingAndClockMessages(data);

    SendRemoveObjectsMessage(data);
    SendAddObjectsMessage(data);
    SendUpdateObjectsReliableMessage(data);
    SendUpdateObjectsUnreliableMessage(data);
}

bool ServerNetworkManager::SendSynchronizationMessages(ClientConnectionData& data)
{
    AbstractConnection* connection = data.connection_;

    // Initialization phase: send "reliable" pings one by one
    if (!data.comfirmedPings_.full())
    {
        if (data.pendingPings_.empty())
        {
            const unsigned magic = GetMagic(true);
            data.pendingPings_.push_back(ClientPing{ magic });

            connection->SendSerializedMessage(MSG_PING, MsgPingPong{ magic }, NetworkMessageFlag::Reliable);
        }
        return false;
    }

    // Synchronization phase.
    if (!data.synchronized_)
    {
        if (!data.pendingSynchronization_)
        {
            const unsigned magic = GetMagic();
            const unsigned ping = data.averagePing_;

            data.pendingSynchronization_ = magic;

            MsgSynchronize msg{ magic };

            msg.updateFrequency_ = updateFrequency_;
            msg.connectionId_ = connection->GetObjectID();

            msg.numTrimmedClockSamples_ = settings_.numTrimmedClockSamples_;
            msg.numOngoingClockSamples_ = settings_.numOngoingClockSamples_;

            msg.lastFrame_ = currentFrame_;
            msg.ping_ = ping;

            connection->SendSerializedMessage(MSG_SYNCHRONIZE, msg, NetworkMessageFlag::Reliable);
        }
        return false;
    }

    return true;
}

void ServerNetworkManager::SendPingAndClockMessages(ClientConnectionData& data)
{
    AbstractConnection* connection = data.connection_;

    // Send periodic ping
    if (data.pingAccumulator_ >= settings_.pingIntervalMs_ / 1000.0f)
    {
        data.pingAccumulator_ = 0.0f;

        const unsigned magic = GetMagic();
        data.pendingPings_.push_back(ClientPing{ magic });

        connection->SendSerializedMessage(MSG_PING, MsgPingPong{ magic }, NetworkMessageFlag::None);
    }

    // Send periodic clock
    if (data.clockAccumulator_ >= settings_.clockIntervalMs_ / 1000.0f)
    {
        data.clockAccumulator_ = 0.0f;
        connection->SendSerializedMessage(MSG_CLOCK, MsgClock{ currentFrame_, data.averagePing_ }, NetworkMessageFlag::None);
    }
}

void ServerNetworkManager::SendRemoveObjectsMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_REMOVE_OBJECTS,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        if (debugInfo)
        {
            for (NetworkId networkId : data.pendingRemovedComponents_)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkId));
            }
        }

        msg.WriteUInt(currentFrame_);
        for (NetworkId networkId : data.pendingRemovedComponents_)
            msg.WriteUInt(static_cast<unsigned>(networkId));

        const bool sendMessage = !data.pendingRemovedComponents_.empty();
        return sendMessage;
    });
}

void ServerNetworkManager::SendAddObjectsMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_ADD_OBJECTS,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(currentFrame_);

        bool sendMessage = false;
        for (const auto& [networkObject, isSnapshot] : data.pendingUpdatedComponents_)
        {
            if (!isSnapshot)
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());
            msg.WriteVLE(networkObject->GetOwnerConnectionId());

            componentBuffer_.Clear();
            networkObject->WriteSnapshot(currentFrame_, componentBuffer_);
            msg.WriteBuffer(componentBuffer_.GetBuffer());

            if (debugInfo)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

void ServerNetworkManager::SendUpdateObjectsReliableMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_RELIABLE,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(currentFrame_);

        bool sendMessage = false;
        for (const auto& [networkObject, isSnapshot] : data.pendingUpdatedComponents_)
        {
            const unsigned index = GetIndex(networkObject->GetNetworkId());
            if (isSnapshot || !deltaUpdateMask_.NeedReliableDelta(index))
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());

            const auto [beginOffset, endOffset] = reliableDeltaUpdates_[index];
            const unsigned deltaSize = endOffset - beginOffset;
            msg.WriteVLE(deltaSize);
            msg.Write(deltaUpdateBuffer_.GetData() + beginOffset, deltaSize);

            if (debugInfo)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

void ServerNetworkManager::SendUpdateObjectsUnreliableMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_UNRELIABLE, NetworkMessageFlag::None,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        bool sendMessage = false;

        msg.WriteUInt(currentFrame_);
        msg.WriteVLE(data.averageFeedbackDelay_);

        for (const auto& [networkObject, isSnapshot] : data.pendingUpdatedComponents_)
        {
            // Skip redundant updates, both if update is empty or if snapshot was already sent
            const unsigned index = GetIndex(networkObject->GetNetworkId());
            if (isSnapshot || !deltaUpdateMask_.NeedUnreliableDelta(index))
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());

            const auto [beginOffset, endOffset] = unreliableDeltaUpdates_[index];
            const unsigned deltaSize = endOffset - beginOffset;
            msg.WriteVLE(deltaSize);
            msg.Write(deltaUpdateBuffer_.GetData() + beginOffset, deltaSize);

            if (debugInfo)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

void ServerNetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    ClientConnectionData& data = GetConnection(connection);

    switch (messageId)
    {
    case MSG_PONG:
    {
        const auto& msg = ReadNetworkMessage<MsgPingPong>(messageData);
        connection->OnMessageReceived(messageId, msg);

        ProcessPong(data, msg);
        break;
    }

    case MSG_SYNCHRONIZE_ACK:
    {
        const auto& msg = ReadNetworkMessage<MsgSynchronizeAck>(messageData);
        connection->OnMessageReceived(messageId, msg);

        ProcessSynchronizeAck(data, msg);
        break;
    }

    case MSG_OBJECTS_FEEDBACK_UNRELIABLE:
    {
        connection->OnMessageReceived(messageId, messageData);

        ProcessObjectsFeedbackUnreliable(data, messageData);
        break;
    }

    default:
        break;
    }
}

void ServerNetworkManager::ProcessPong(ClientConnectionData& data, const MsgPingPong& msg)
{
    const auto iter = ea::find_if(data.pendingPings_.begin(), data.pendingPings_.end(),
        [&](const ClientPing& ping) { return ping.magic_ == msg.magic_; });
    if (iter == data.pendingPings_.end())
    {
        URHO3D_LOGWARNING("Connection {}: Unexpected pong received", data.connection_->ToString(), msg.magic_);
        return;
    }

    ClientPing& pendingPing = *iter;
    const auto pingMs = pendingPing.timer_.GetUSec(false) / 1000 / 2;
    data.pendingPings_.erase(iter);

    if (data.comfirmedPings_.full())
        data.comfirmedPings_.pop_front();
    data.comfirmedPings_.push_back(pingMs);

    RecalculateAvergagePing(data);
    URHO3D_LOGTRACE("Connection {}: Ping of {}ms added to buffer, average is {}ms", data.connection_->ToString(),
        pingMs, data.averagePing_);
}

void ServerNetworkManager::ProcessSynchronizeAck(ClientConnectionData& data, const MsgSynchronizeAck& msg)
{
    if (!data.pendingSynchronization_ || data.pendingSynchronization_ != msg.magic_)
    {
        URHO3D_LOGWARNING(
            "Connection {}: Unexpected synchronization ack received", data.connection_->ToString(), msg.magic_);
        return;
    }

    data.pendingSynchronization_ = ea::nullopt;
    data.synchronized_ = true;
}

void ServerNetworkManager::ProcessObjectsFeedbackUnreliable(ClientConnectionData& data, MemoryBuffer& messageData)
{
    if (!data.synchronized_)
    {
        URHO3D_LOGWARNING("Connection {}: Received unexpected feedback", data.connection_->ToString());
        return;
    }

    // Input is processed before BeginNetworkFrame, so add 1 to get actual current frame
    const unsigned serverFrame = currentFrame_ + 1;
    const unsigned feedbackFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        NetworkObject* networkObject = base_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Connection {}: Received feedback for unknown NetworkObject {}",
                data.connection_->ToString(), ToString(networkId));
            continue;
        }

        if (networkObject->GetOwnerConnection() != data.connection_)
        {
            URHO3D_LOGWARNING("Connection {}: Received feedback for NetworkObject {} owned by connection #{}",
                data.connection_->ToString(), ToString(networkId), networkObject->GetOwnerConnectionId());
            continue;
        }

        componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
        componentBuffer_.Seek(0);
        networkObject->ReadUnreliableFeedback(serverFrame, feedbackFrame, componentBuffer_);
    }

    if (NetworkTime{feedbackFrame} - NetworkTime{data.latestFeedbackFrame_})
    {
        data.latestFeedbackFrame_ = feedbackFrame;
        const unsigned delay = ea::max(0, static_cast<int>(serverFrame - feedbackFrame));

        if (data.feedbackDelay_.full())
            data.feedbackDelay_.pop_front();
        while (!data.feedbackDelay_.full())
            data.feedbackDelay_.push_back(delay);

        data.feedbackDelaySorted_.assign(data.feedbackDelay_.begin(), data.feedbackDelay_.end());
        ea::sort(data.feedbackDelaySorted_.begin(), data.feedbackDelaySorted_.end());
        const auto iter = data.feedbackDelaySorted_.begin() + data.feedbackDelaySorted_.size() / 2;
        const unsigned newFeedbackDelay = *iter;

        // If delay is decreased just by 1, don't.
        // TODO(network): Use additional percentile to check?
        if (newFeedbackDelay + 1 != data.averageFeedbackDelay_)
            data.averageFeedbackDelay_ = newFeedbackDelay; // TODO(network): Refactor this thing
        //data.averageFeedbackDelay_ = *ea::max_element(data.feedbackDelay_.begin(), data.feedbackDelay_.end());
    }
}

void ServerNetworkManager::SetTestPing(AbstractConnection* connection, unsigned ping)
{
    ClientConnectionData& data = GetConnection(connection);
    data.overridePing_ = ping;
    RecalculateAvergagePing(data);
}

void ServerNetworkManager::SetCurrentFrame(unsigned frame)
{
    currentFrame_ = frame;
}

ClientConnectionData& ServerNetworkManager::GetConnection(AbstractConnection* connection)
{
    const auto iter = connections_.find(connection);
    assert(iter != connections_.end());
    return iter->second;
}

void ServerNetworkManager::RecalculateAvergagePing(ClientConnectionData& data)
{
    if (data.overridePing_)
    {
        data.averagePing_ = *data.overridePing_;
        return;
    }

    if (data.comfirmedPings_.empty())
    {
        data.averagePing_ = M_MAX_UNSIGNED;
        return;
    }

    data.comfirmedPingsSorted_.assign(data.comfirmedPings_.begin(), data.comfirmedPings_.end());
    ea::sort(data.comfirmedPingsSorted_.begin(), data.comfirmedPingsSorted_.end());

    const unsigned numPings = data.comfirmedPingsSorted_.size();
    const auto beginIter = data.comfirmedPingsSorted_.begin();
    const auto endIter = std::prev(data.comfirmedPingsSorted_.end(), ea::min(numPings - 1, settings_.numTrimmedMaxPings_));
    data.averagePing_ = ea::accumulate(beginIter, endIter, 0) / ea::distance(beginIter, endIter);
}

unsigned ServerNetworkManager::GetMagic(bool reliable) const
{
    // TODO: It doesn't look secure.
    const unsigned value = Rand();
    return reliable ? (value | 1u) : (value & ~1u);
}

ea::string ServerNetworkManager::GetDebugInfo() const
{
    ea::string result;

    result += Format("Scene '{}': Time #{}\n",
        !scene_->GetName().empty() ? scene_->GetName() : "Unnamed",
        currentFrame_);

    for (const auto& [connection, data] : connections_)
    {
        result += Format("Connection {}: Ping {}ms, Feedback Delay {} frames\n",
            connection->ToString(), data.averagePing_, data.averageFeedbackDelay_);
    }

    return result;
}

unsigned ServerNetworkManager::GetFeedbackDelay(AbstractConnection* connection) const
{
    const auto iter = connections_.find(connection);
    return iter != connections_.end() ? iter->second.averageFeedbackDelay_ : 0;
}

}
