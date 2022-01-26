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
#include "../Replica/ServerNetworkManager.h"
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

void SharedReplicationState::PrepareForNewFrame()
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

        const unsigned reliableMask = networkObject->GetReliableDeltaMask(currentFrame);
        if (reliableMask)
        {
            const unsigned beginOffset = deltaUpdateBuffer_.Tell();
            networkObject->WriteReliableDelta(currentFrame, reliableMask, deltaUpdateBuffer_);
            const unsigned endOffset = deltaUpdateBuffer_.Tell();

            needReliableDeltaUpdate_[i] = true;
            reliableDeltaUpdateData_[i] = {beginOffset, endOffset};
        }

        const unsigned unreliableMask = networkObject->GetUnreliableDeltaMask(currentFrame);
        if (unreliableMask)
        {
            const unsigned beginOffset = deltaUpdateBuffer_.Tell();
            networkObject->WriteUnreliableDelta(currentFrame, unreliableMask, deltaUpdateBuffer_);
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

ClientConnectionData::ClientConnectionData(AbstractConnection* connection, const VariantMap& settings)
    : connection_(connection)
    , settings_(settings)
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , inputDelayFilter_(GetSetting(NetworkSettings::InputDelayFilterBufferSize).GetUInt())
    , inputStats_(GetSetting(NetworkSettings::InputBufferingWindowSize).GetUInt(), InputStatsSafetyLimit)
    , inputBufferFilter_(GetSetting(NetworkSettings::InputBufferingFilterBufferSize).GetUInt())
{
    SetNetworkSetting(settings_, NetworkSettings::ConnectionId, connection_->GetObjectID());
}

void ClientConnectionData::UpdateFrame(float timeStep, const NetworkTime& serverTime, float overtime)
{
    serverTime_ = serverTime;
    timestamp_ = connection_->GetLocalTime() - RoundToInt(overtime * 1000);

    clockTimeAccumulator_ += timeStep;
}

void ClientConnectionData::ProcessNetworkObjects(SharedReplicationState& sharedState, float timeStep)
{
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

void ClientConnectionData::SendCommonUpdates()
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

        const MsgSceneClock msg{serverTime_.GetFrame(), timestamp_, inputDelay_ + inputBufferSize_};
        connection_->SendSerializedMessage(MSG_SCENE_CLOCK, msg, NetworkMessageFlag::None);
    }
}

void ClientConnectionData::SendSynchronizedMessages()
{

}

void ClientConnectionData::ProcessSynchronized(const MsgSynchronized& msg)
{
    if (synchronizationMagic_ != msg.magic_)
    {
        URHO3D_LOGWARNING(
            "Connection {}: Unexpected synchronization ack received", connection_->ToString());
        return;
    }

    synchronized_ = true;
}

void ClientConnectionData::UpdateInputDelay()
{
    const unsigned latestPingTimestamp = connection_->GetLocalTimeOfLatestRoundtrip();
    if (latestProcessedPingTimestamp_ == latestPingTimestamp)
        return;
    latestProcessedPingTimestamp_ = latestPingTimestamp;

    const double inputDelayInFrames = 0.001 * connection_->GetPing() * updateFrequency_;
    inputDelayFilter_.AddValue(CeilToInt(inputDelayInFrames));
    inputDelay_ = inputDelayFilter_.GetStabilizedAverageMaxValue();
}

void ClientConnectionData::UpdateInputBuffer()
{
    inputBufferFilter_.AddValue(inputStats_.GetRecommendedBufferSize());

    const int bufferSizeTweak = GetSetting(NetworkSettings::InputBufferingTweak).GetInt();
    const int newInputBufferSize = bufferSizeTweak + static_cast<int>(inputBufferFilter_.GetStabilizedAverageMaxValue());

    const int minInputBuffer = GetSetting(NetworkSettings::MinInputBuffering).GetUInt();
    const int maxInputBuffer = GetSetting(NetworkSettings::MaxInputBuffering).GetUInt();
    inputBufferSize_ = Clamp(newInputBufferSize, minInputBuffer, maxInputBuffer);
}

unsigned ClientConnectionData::MakeMagic() const
{
    return RandomEngine::GetDefaultEngine().GetUInt();
}

const Variant& ClientConnectionData::GetSetting(const NetworkSetting& setting) const
{
    return GetNetworkSetting(settings_, setting);
}

ServerNetworkManager::ServerNetworkManager(NetworkManagerBase* base, Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , base_(base)
    , scene_(scene)
    , updateFrequency_(network_->GetUpdateFps())
    , physicsSync_(scene_, updateFrequency_, true)
    , sharedState_(MakeShared<SharedReplicationState>(base_))
{
    SetNetworkSetting(settings_.map_, NetworkSettings::UpdateFrequency, updateFrequency_);

    SubscribeToEvent(E_INPUTREADY, [this](StringHash, VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();

        const bool isUpdateNow = network_->IsUpdateNow();
        const float overtime = network_->GetUpdateOvertime();

        if (isUpdateNow)
            physicsSync_.Synchronize(overtime);
        else
            physicsSync_.Update(timeStep);

        if (isUpdateNow)
            BeginNetworkFrame(overtime);
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

void ServerNetworkManager::BeginNetworkFrame(float overtime)
{
    ++currentFrame_;

    const float timeStep = 1.0f / updateFrequency_;
    for (auto& [connection, data] : connections_)
        data.UpdateFrame(timeStep, NetworkTime{currentFrame_}, overtime);

    network_->SendEvent(E_BEGINSERVERNETWORKUPDATE);
}

void ServerNetworkManager::PrepareNetworkFrame()
{
    const float timeStep = 1.0f / updateFrequency_;

    sharedState_->PrepareForNewFrame();
    for (auto& [connection, data] : connections_)
    {
        if (data.IsSynchronized())
            data.ProcessNetworkObjects(*sharedState_, timeStep);
    }
    sharedState_->CookDeltaUpdates(currentFrame_);
}

void ServerNetworkManager::AddConnection(AbstractConnection* connection)
{
    if (connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection {} is already added", connection->ToString());
        assert(0);
    }

    connections_.erase(connection);
    const auto& [iter, insterted] = connections_.emplace(
        connection, ClientConnectionData{connection, settings_.map_});

    URHO3D_ASSERT(insterted);
    ClientConnectionData& data = iter->second;

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
    data.SendCommonUpdates();
    if (data.IsSynchronized())
        data.SendSynchronizedMessages();

    if (!data.IsSynchronized())
        return;

    SendRemoveObjectsMessage(data);
    SendAddObjectsMessage(data);
    SendUpdateObjectsReliableMessage(data);
    SendUpdateObjectsUnreliableMessage(data);
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
            if (isSnapshot)
                continue;

            const auto updateSpan = sharedState_->GetReliableUpdateByIndex(index);
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

void ServerNetworkManager::SendUpdateObjectsUnreliableMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_UNRELIABLE, NetworkMessageFlag::None,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        bool sendMessage = false;

        msg.WriteUInt(currentFrame_);

        for (const auto& [networkObject, isSnapshot] : data.pendingUpdatedComponents_)
        {
            // Skip redundant updates, both if update is empty or if snapshot was already sent
            const unsigned index = GetIndex(networkObject->GetNetworkId());
            if (isSnapshot)
                continue;

            const auto updateSpan = sharedState_->GetUnreliableUpdateByIndex(index);
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

void ServerNetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    ClientConnectionData& data = GetConnection(connection);

    switch (messageId)
    {
    case MSG_SYNCHRONIZED:
    {
        const auto& msg = ReadNetworkMessage<MsgSynchronized>(messageData);
        connection->OnMessageReceived(messageId, msg);

        data.ProcessSynchronized(msg);
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

void ServerNetworkManager::ProcessObjectsFeedbackUnreliable(ClientConnectionData& data, MemoryBuffer& messageData)
{
    if (!data.IsSynchronized())
    {
        URHO3D_LOGWARNING("Connection {}: Received unexpected feedback", data.connection_->ToString());
        return;
    }

    // Input is processed before BeginNetworkFrame, so add 1 to get actual current frame
    const unsigned feedbackFrame = messageData.ReadUInt();
    data.OnFeedbackReceived(feedbackFrame);

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
        networkObject->ReadUnreliableFeedback(feedbackFrame, componentBuffer_);
    }
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
        result += Format("Connection {}: Ping {}ms, Input delay {}+{} frames\n",
            connection->ToString(), connection->GetPing(), data.GetInputDelay(), data.GetInputBufferSize());
    }

    return result;
}

unsigned ServerNetworkManager::GetFeedbackDelay(AbstractConnection* connection) const
{
    const auto iter = connections_.find(connection);
    return iter != connections_.end() ? iter->second.GetInputDelay() + iter->second.GetInputBufferSize() : 0;
}

}
