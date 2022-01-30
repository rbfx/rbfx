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
#include "../Core/Timer.h"
#include "../IO/Log.h"
#include "../Math/RandomEngine.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Replica/NetworkObject.h"
#include "../Replica/NetworkManager.h"
#include "../Replica/NetworkSettingsConsts.h"
#include "../Replica/ServerReplicator.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

unsigned GetIndex(NetworkId networkId)
{
    return DeconstructStableComponentId(networkId).first;
}

}

SharedReplicationState::SharedReplicationState(NetworkManagerBase* replicationManager)
    : replicationManager_(replicationManager)
{
    URHO3D_ASSERT(replicationManager_);

    replicationManager_->OnNetworkObjectAdded.Subscribe(this, &SharedReplicationState::OnNetworkObjectAdded);
    replicationManager_->OnNetworkObjectRemoved.Subscribe(this, &SharedReplicationState::OnNetworkObjectRemoved);

    for (NetworkObject* networkObject : replicationManager_->GetNetworkObjects())
        recentlyAddedComponents_.emplace(networkObject->GetNetworkId());
}

void SharedReplicationState::OnNetworkObjectAdded(NetworkObject* networkObject)
{
    recentlyAddedComponents_.insert(networkObject->GetNetworkId());
}

void SharedReplicationState::OnNetworkObjectRemoved(NetworkObject* networkObject)
{
    if (recentlyAddedComponents_.erase(networkObject->GetNetworkId()) == 0)
        recentlyRemovedComponents_.insert(networkObject->GetNetworkId());
}

void SharedReplicationState::PrepareForUpdate()
{
    ResetFrameBuffers();
    InitializeNewObjects();

    replicationManager_->UpdateAndSortNetworkObjects(sortedNetworkObjects_);
}

void SharedReplicationState::ResetFrameBuffers()
{
    const unsigned indexUppedBound = GetIndexUpperBound();

    isDeltaUpdateQueued_.clear();
    isDeltaUpdateQueued_.resize(indexUppedBound);

    needReliableDeltaUpdate_.clear();
    needReliableDeltaUpdate_.resize(indexUppedBound);
    reliableDeltaUpdateData_.resize(indexUppedBound);

    needUnreliableDeltaUpdate_.clear();
    needUnreliableDeltaUpdate_.resize(indexUppedBound);
    unreliableDeltaUpdateData_.resize(indexUppedBound);

    deltaUpdateBuffer_.Clear();
}

void SharedReplicationState::InitializeNewObjects()
{
    for (NetworkId networkId : recentlyAddedComponents_)
    {
        NetworkObject* networkObject = replicationManager_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_ASSERTLOG(0, "Cannot find recently added NetworkObject");
            continue;
        }

        networkObject->SetNetworkMode(NetworkObjectMode::Server);
        networkObject->InitializeOnServer();
    }
    recentlyAddedComponents_.clear();
}

void SharedReplicationState::QueueDeltaUpdate(NetworkObject* networkObject)
{
    const unsigned index = GetIndex(networkObject->GetNetworkId());
    isDeltaUpdateQueued_[index] = true;
}

void SharedReplicationState::CookDeltaUpdates(unsigned currentFrame)
{
    recentlyRemovedComponents_.clear();

    for (unsigned i = 0; i < isDeltaUpdateQueued_.size(); ++i)
    {
        if (!isDeltaUpdateQueued_[i])
            continue;

        NetworkObject* networkObject = replicationManager_->GetNetworkObjectByIndex(i);
        URHO3D_ASSERT(networkObject);

        if (networkObject->PrepareReliableDelta(currentFrame))
        {
            const unsigned beginOffset = deltaUpdateBuffer_.Tell();
            networkObject->WriteReliableDelta(currentFrame, deltaUpdateBuffer_);
            const unsigned endOffset = deltaUpdateBuffer_.Tell();

            needReliableDeltaUpdate_[i] = true;
            reliableDeltaUpdateData_[i] = {beginOffset, endOffset};
        }

        if (networkObject->PrepareUnreliableDelta(currentFrame))
        {
            const unsigned beginOffset = deltaUpdateBuffer_.Tell();
            networkObject->WriteUnreliableDelta(currentFrame, deltaUpdateBuffer_);
            const unsigned endOffset = deltaUpdateBuffer_.Tell();

            needUnreliableDeltaUpdate_[i] = true;
            unreliableDeltaUpdateData_[i] = {beginOffset, endOffset};
        }
    }
}

unsigned SharedReplicationState::GetIndexUpperBound() const
{
    return replicationManager_->GetNetworkIndexUpperBound();
}

ea::optional<ConstByteSpan> SharedReplicationState::GetReliableUpdateByIndex(unsigned index) const
{
    if (!needReliableDeltaUpdate_[index])
        return ea::nullopt;
    return GetSpanData(reliableDeltaUpdateData_[index]);
}

ea::optional<ConstByteSpan> SharedReplicationState::GetUnreliableUpdateByIndex(unsigned index) const
{
    if (!needUnreliableDeltaUpdate_[index])
        return ea::nullopt;
    return GetSpanData(unreliableDeltaUpdateData_[index]);
}

ConstByteSpan SharedReplicationState::GetSpanData(const DeltaBufferSpan& span) const
{
    const auto data = deltaUpdateBuffer_.GetData();
    return {data + span.beginOffset_, span.endOffset_ - span.beginOffset_};
}

ClientSynchronizationState::ClientSynchronizationState(
    NetworkManagerBase* replicationManager, AbstractConnection* connection, const VariantMap& settings)
    : replicationManager_(replicationManager)
    , connection_(connection)
    , settings_(settings)
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , inputDelayFilter_(GetSetting(NetworkSettings::InputDelayFilterBufferSize).GetUInt())
    , inputStats_(GetSetting(NetworkSettings::InputBufferingWindowSize).GetUInt(), InputStatsSafetyLimit)
    , inputBufferFilter_(GetSetting(NetworkSettings::InputBufferingFilterBufferSize).GetUInt())
{
    SetNetworkSetting(settings_, NetworkSettings::ConnectionId, connection_->GetObjectID());
}

void ClientSynchronizationState::BeginNetworkFrame(unsigned currentFrame, float overtime)
{
    const float timeStep = 1.0f / updateFrequency_;
    frame_ = currentFrame;
    frameLocalTime_ = connection_->GetLocalTime() - RoundToInt(overtime * 1000);
    clockTimeAccumulator_ += timeStep;
}

const Variant& ClientSynchronizationState::GetSetting(const NetworkSetting& setting) const
{
    return GetNetworkSetting(settings_, setting);
}

void ClientSynchronizationState::SendMessages()
{
    // Send configuration on startup once
    if (!synchronizationMagic_)
    {
        const unsigned magic = MakeMagic();
        connection_->SendSerializedMessage(MSG_CONFIGURE, MsgConfigure{magic, settings_}, NetworkMessageFlag::Reliable);
        synchronizationMagic_ = magic;
    }

    // Send clock updates
    const float clockInterval = GetSetting(NetworkSettings::PeriodicClockInterval).GetFloat();
    if (clockTimeAccumulator_ >= clockInterval)
    {
        clockTimeAccumulator_ = Fract(clockTimeAccumulator_ / clockInterval) * clockInterval;

        UpdateInputDelay();
        UpdateInputBuffer();

        const MsgSceneClock msg{frame_, frameLocalTime_, inputDelay_ + inputBufferSize_};
        connection_->SendSerializedMessage(MSG_SCENE_CLOCK, msg, NetworkMessageFlag::None);
    }
}

void ClientSynchronizationState::UpdateInputDelay()
{
    const unsigned latestPingTimestamp = connection_->GetLocalTimeOfLatestRoundtrip();
    if (latestProcessedPingTimestamp_ == latestPingTimestamp)
        return;
    latestProcessedPingTimestamp_ = latestPingTimestamp;

    const double inputDelayInFrames = 0.001 * connection_->GetPing() * updateFrequency_;
    inputDelayFilter_.AddValue(CeilToInt(inputDelayInFrames));
    inputDelay_ = inputDelayFilter_.GetStabilizedAverageMaxValue();
}

void ClientSynchronizationState::UpdateInputBuffer()
{
    inputBufferFilter_.AddValue(inputStats_.GetRecommendedBufferSize());

    const int bufferSizeTweak = GetSetting(NetworkSettings::InputBufferingTweak).GetInt();
    const int newInputBufferSize = bufferSizeTweak + static_cast<int>(inputBufferFilter_.GetStabilizedAverageMaxValue());

    const int minInputBuffer = GetSetting(NetworkSettings::MinInputBuffering).GetUInt();
    const int maxInputBuffer = GetSetting(NetworkSettings::MaxInputBuffering).GetUInt();
    inputBufferSize_ = Clamp(newInputBufferSize, minInputBuffer, maxInputBuffer);
}

void ClientSynchronizationState::ProcessSynchronized(const MsgSynchronized& msg)
{
    if (synchronizationMagic_ != msg.magic_)
    {
        URHO3D_LOGWARNING(
            "Connection {}: Unexpected synchronization acknowledgement received", connection_->ToString());
        return;
    }

    synchronized_ = true;
}

bool ClientSynchronizationState::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_SYNCHRONIZED:
    {
        const auto& msg = ReadNetworkMessage<MsgSynchronized>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessSynchronized(msg);
        return true;
    }
    default:
        return false;
    }
}

void ClientSynchronizationState::OnInputReceived(unsigned inputFrame)
{
    inputStats_.OnInputReceived(inputFrame);
}

unsigned ClientSynchronizationState::MakeMagic() const
{
    return RandomEngine::GetDefaultEngine().GetUInt();
}

ClientReplicationState::ClientReplicationState(
    NetworkManagerBase* replicationManager, AbstractConnection* connection, const VariantMap& settings)
    : ClientSynchronizationState(replicationManager, connection, settings)
{
}

void ClientReplicationState::SendMessages(const SharedReplicationState& sharedState)
{
    ClientSynchronizationState::SendMessages();

    if (IsSynchronized())
    {
        SendRemoveObjects();
        SendAddObjects();
        SendUpdateObjectsReliable(sharedState);
        SendUpdateObjectsUnreliable(sharedState);
    }
}

bool ClientReplicationState::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (ClientSynchronizationState::ProcessMessage(messageId, messageData))
        return true;

    switch (messageId)
    {
    case MSG_OBJECTS_FEEDBACK_UNRELIABLE:
        connection_->OnMessageReceived(messageId, messageData);

        ProcessObjectsFeedbackUnreliable(messageData);
        return true;

    default:
        return false;
    }
}

void ClientReplicationState::ProcessObjectsFeedbackUnreliable(MemoryBuffer& messageData)
{
    if (!IsSynchronized())
    {
        URHO3D_LOGWARNING("Connection {}: Received unexpected feedback", connection_->ToString());
        return;
    }

    const unsigned feedbackFrame = messageData.ReadUInt();
    OnInputReceived(feedbackFrame);

    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        NetworkObject* networkObject = replicationManager_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Connection {}: Received feedback for unknown NetworkObject {}",
                connection_->ToString(), ToString(networkId));
            continue;
        }

        if (networkObject->GetOwnerConnection() != connection_)
        {
            URHO3D_LOGWARNING("Connection {}: Received feedback for NetworkObject {} owned by connection #{}",
                connection_->ToString(), ToString(networkId), networkObject->GetOwnerConnectionId());
            continue;
        }

        componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
        componentBuffer_.Seek(0);
        networkObject->ReadUnreliableFeedback(feedbackFrame, componentBuffer_);
    }
}

void ClientReplicationState::SendRemoveObjects()
{
    connection_->SendGeneratedMessage(MSG_REMOVE_OBJECTS,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        if (debugInfo)
        {
            for (NetworkId networkId : pendingRemovedComponents_)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkId));
            }
        }

        msg.WriteUInt(GetCurrentFrame());
        for (NetworkId networkId : pendingRemovedComponents_)
            msg.WriteUInt(static_cast<unsigned>(networkId));

        const bool sendMessage = !pendingRemovedComponents_.empty();
        return sendMessage;
    });
}

void ClientReplicationState::SendAddObjects()
{
    connection_->SendGeneratedMessage(MSG_ADD_OBJECTS,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(GetCurrentFrame());

        bool sendMessage = false;
        for (const auto& [networkObject, isSnapshot] : pendingUpdatedComponents_)
        {
            if (!isSnapshot)
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());
            msg.WriteVLE(networkObject->GetOwnerConnectionId());

            componentBuffer_.Clear();
            networkObject->WriteSnapshot(GetCurrentFrame(), componentBuffer_);
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

void ClientReplicationState::SendUpdateObjectsReliable(const SharedReplicationState& sharedState)
{
    connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_RELIABLE,
        NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(GetCurrentFrame());

        bool sendMessage = false;
        for (const auto& [networkObject, isSnapshot] : pendingUpdatedComponents_)
        {
            const unsigned index = GetIndex(networkObject->GetNetworkId());
            if (isSnapshot)
                continue;

            const auto updateSpan = sharedState.GetReliableUpdateByIndex(index);
            if (!updateSpan)
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());

            msg.WriteVLE(updateSpan->size());
            msg.Write(updateSpan->data(), updateSpan->size());

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

void ClientReplicationState::SendUpdateObjectsUnreliable(const SharedReplicationState& sharedState)
{
    connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_UNRELIABLE, NetworkMessageFlag::None,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        bool sendMessage = false;

        msg.WriteUInt(GetCurrentFrame());

        for (const auto& [networkObject, isSnapshot] : pendingUpdatedComponents_)
        {
            // Skip redundant updates, both if update is empty or if snapshot was already sent
            const unsigned index = GetIndex(networkObject->GetNetworkId());
            if (isSnapshot)
                continue;

            const auto updateSpan = sharedState.GetUnreliableUpdateByIndex(index);
            if (!updateSpan)
                continue;

            sendMessage = true;
            msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
            msg.WriteStringHash(networkObject->GetType());

            msg.WriteVLE(updateSpan->size());
            msg.Write(updateSpan->data(), updateSpan->size());

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

void ClientReplicationState::UpdateNetworkObjects(SharedReplicationState& sharedState)
{
    if (!IsSynchronized())
        return;

    const float timeStep = 1.0f / updateFrequency_;
    const float relevanceTimeout = GetSetting(NetworkSettings::RelevanceTimeout).GetFloat();

    const unsigned indexUpperBound = sharedState.GetIndexUpperBound();
    isComponentReplicated_.resize(indexUpperBound);
    componentsRelevanceTimeouts_.resize(indexUpperBound);

    pendingRemovedComponents_.clear();
    pendingUpdatedComponents_.clear();

    // Process removed components first
    for (NetworkId networkId : sharedState.GetRecentlyRemovedObjects())
    {
        const unsigned index = GetIndex(networkId);
        if (isComponentReplicated_[index])
        {
            isComponentReplicated_[index] = false;
            pendingRemovedComponents_.push_back(networkId);
        }
    }

    // Process active components
    for (NetworkObject* networkObject : sharedState.GetSortedObjects())
    {
        const NetworkId networkId = networkObject->GetNetworkId();
        const unsigned index = GetIndex(networkId);

        if (!isComponentReplicated_[index])
        {
            if (networkObject->IsRelevantForClient(connection_))
            {
                // Begin replication of component, queue snapshot
                componentsRelevanceTimeouts_[index] = relevanceTimeout;
                isComponentReplicated_[index] = true;
                pendingUpdatedComponents_.push_back({ networkObject, true });
            }
        }
        else
        {
            componentsRelevanceTimeouts_[index] -= timeStep;
            if (componentsRelevanceTimeouts_[index] < 0.0f)
            {
                if (!networkObject->IsRelevantForClient(connection_))
                {
                    // Remove irrelevant component
                    isComponentReplicated_[index] = false;
                    pendingRemovedComponents_.push_back(networkId);
                    continue;
                }

                componentsRelevanceTimeouts_[index] = relevanceTimeout;
            }

            // Queue non-snapshot update
            sharedState.QueueDeltaUpdate(networkObject);
            pendingUpdatedComponents_.push_back({ networkObject, false });
        }
    }
}

ServerReplicator::ServerReplicator(Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
    , replicationManager_(scene->GetComponent<NetworkManager>())
    , updateFrequency_(network_->GetUpdateFps())
    , physicsSync_(scene_, updateFrequency_, true)
    , sharedState_(MakeShared<SharedReplicationState>(replicationManager_))
{
    SetNetworkSetting(settings_, NetworkSettings::UpdateFrequency, updateFrequency_);

    SubscribeToEvent(E_INPUTREADY, [this](StringHash, VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();

        const bool isUpdateNow = network_->IsUpdateNow();
        const float overtime = network_->GetUpdateOvertime();
        OnInputReady(timeStep, isUpdateNow, overtime);
    });

    SubscribeToEvent(network_, E_NETWORKUPDATE, [this](StringHash, VariantMap&)
    {
        OnNetworkUpdate();
    });
}

ServerReplicator::~ServerReplicator()
{
}

void ServerReplicator::OnInputReady(float timeStep, bool isUpdateNow, float overtime)
{
    if (isUpdateNow)
    {
        ++currentFrame_;
        physicsSync_.Synchronize(overtime, currentFrame_);

        for (auto& [connection, data] : connections_)
            data->BeginNetworkFrame(currentFrame_, overtime);

        using namespace BeginServerNetworkFrame;
        auto& eventData = GetEventDataMap();
        eventData[P_FRAME] = currentFrame_;
        network_->SendEvent(E_BEGINSERVERNETWORKFRAME, eventData);
    }
    else
    {
        physicsSync_.Update(timeStep);
    }
}

void ServerReplicator::OnNetworkUpdate()
{
    sharedState_->PrepareForUpdate();
    for (auto& [connection, data] : connections_)
        data->UpdateNetworkObjects(*sharedState_);
    sharedState_->CookDeltaUpdates(currentFrame_);

    for (auto& [connection, data] : connections_)
        data->SendMessages(*sharedState_);
}

void ServerReplicator::AddConnection(AbstractConnection* connection)
{
    if (connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection {} is already added", connection->ToString());
        assert(0);
    }

    connections_.erase(connection);
    const auto& [iter, insterted] = connections_.emplace(
        connection, MakeShared<ClientReplicationState>(replicationManager_, connection, settings_));

    URHO3D_ASSERT(insterted);
    ClientReplicationState& data = *iter->second;

    URHO3D_LOGINFO("Connection {} is added", connection->ToString());
}

void ServerReplicator::RemoveConnection(AbstractConnection* connection)
{
    if (!connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection {} is not added", connection->ToString());
        assert(0);
    }

    connections_.erase(connection);
    URHO3D_LOGINFO("Connection {} is removed", connection->ToString());
}

bool ServerReplicator::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    ClientReplicationState& data = GetConnection(connection);
    return data.ProcessMessage(messageId, messageData);
}

void ServerReplicator::SetCurrentFrame(unsigned frame)
{
    currentFrame_ = frame;
}

ClientReplicationState& ServerReplicator::GetConnection(AbstractConnection* connection)
{
    const auto iter = connections_.find(connection);
    assert(iter != connections_.end());
    return *iter->second;
}

ea::string ServerReplicator::GetDebugInfo() const
{
    ea::string result;

    result += Format("Scene '{}': Time #{}\n",
        !scene_->GetName().empty() ? scene_->GetName() : "Unnamed",
        currentFrame_);

    for (const auto& [connection, data] : connections_)
    {
        result += Format("Connection {}: Ping {}ms, Input delay {}+{} frames\n",
            connection->ToString(), connection->GetPing(), data->GetInputDelay(), data->GetInputBufferSize());
    }

    return result;
}

const Variant& ServerReplicator::GetSetting(const NetworkSetting& setting) const
{
    return GetNetworkSetting(settings_, setting);
}

unsigned ServerReplicator::GetFeedbackDelay(AbstractConnection* connection) const
{
    const auto iter = connections_.find(connection);
    return iter != connections_.end() ? iter->second->GetInputDelay() + iter->second->GetInputBufferSize() : 0;
}

}
