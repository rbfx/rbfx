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
#include "../Network/NetworkObject.h"
#include "../Network/NetworkManager.h"
#include "../Network/NetworkSettingsConsts.h"
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
    CollectObjectsToUpdate(timeStep);
    PrepareDeltaUpdates();
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
        if (!data.IsSynchronized())
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
    if (const auto mask = networkObject->GetReliableDeltaMask(currentFrame_))
    {
        const unsigned beginOffset = deltaUpdateBuffer_.Tell();
        networkObject->WriteReliableDelta(currentFrame_, mask, deltaUpdateBuffer_);
        const unsigned endOffset = deltaUpdateBuffer_.Tell();
        reliableDeltaUpdates_[index] = { beginOffset, endOffset };
    }
    else
    {
        deltaUpdateMask_.ResetReliableDelta(index);
    }
}

void ServerNetworkManager::PrepareUnreliableDeltaForObject(unsigned index, NetworkObject* networkObject)
{
    if (const auto mask = networkObject->GetUnreliableDeltaMask(currentFrame_))
    {
        const unsigned beginOffset = deltaUpdateBuffer_.Tell();
        networkObject->WriteUnreliableDelta(currentFrame_, mask, deltaUpdateBuffer_);
        const unsigned endOffset = deltaUpdateBuffer_.Tell();
        unreliableDeltaUpdates_[index] = { beginOffset, endOffset };
    }
    else
    {
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
