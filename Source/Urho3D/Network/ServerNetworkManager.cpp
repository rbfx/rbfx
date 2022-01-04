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
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkObject.h"
#include "../Network/NetworkManager.h"
#include "../Network/ServerNetworkManager.h"
#include "../Physics/PhysicsWorld.h"
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
    , physicsWorld_(scene_->GetComponent<PhysicsWorld>())
{
    SubscribeToEvent(network_, E_NETWORKINPUTPROCESSED, [this](StringHash, VariantMap& eventData)
    {
        using namespace NetworkInputProcessed;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();
        const bool updateNow = eventData[P_UPDATENOW].GetBool();
        const float accumulatedTime = eventData[P_ACCUMULATEDTIME].GetFloat();
        UpdatePhysicsClock(timeStep, updateNow, accumulatedTime);
    });

    SubscribeToEvent(network_, E_NETWORKUPDATE, [this](StringHash, VariantMap&)
    {
        BeginNetworkFrame();
        for (auto& [connection, data] : connections_)
            SendUpdate(data);
    });

    SubscribeToEvent(scene_, E_SCENESUBSYSTEMUPDATE, [this](StringHash, VariantMap&)
    {
        UpdatePhysics();
    });

    if (physicsWorld_)
        physicsWorld_->SetUpdateEnabled(false);
}

ServerNetworkManager::~ServerNetworkManager()
{
    if (physicsWorld_)
        physicsWorld_->SetUpdateEnabled(true);
}

void ServerNetworkManager::UpdatePhysicsClock(float timeStep, bool networkUpdateNow, float accumulatedTime)
{
    if (!physicsWorld_)
        return;

    const unsigned updatePhysicsFrequency = physicsWorld_->GetFps();
    const unsigned maxPhysicalStepsPerNetworkUpdate = ea::max(1u, updatePhysicsFrequency / updateFrequency_);
    physicsUpdateFrequency_ = updateFrequency_ * maxPhysicalStepsPerNetworkUpdate;
    if (physicsUpdateFrequency_ != updatePhysicsFrequency)
    {
        URHO3D_LOGWARNING(
            "Network at {} FPS and PhysicsWorld at {} FPS cannot be synchronized, PhysicsWorld is forced at {} FPS",
            updateFrequency_, updatePhysicsFrequency, physicsUpdateFrequency_);
    }

    if (networkUpdateNow)
    {
        numPhysicsSteps_ = 0;
        physicsExtraTime_ = accumulatedTime;
        numPendingPhysicsSteps_ = 1;
    }
    else
    {
        physicsExtraTime_ += timeStep;
        numPendingPhysicsSteps_ = 0;
    }

    const float fixedTimeStep = 1.0f / physicsUpdateFrequency_;
    while (physicsExtraTime_ >= fixedTimeStep)
    {
        physicsExtraTime_ -= fixedTimeStep;
        ++numPendingPhysicsSteps_;
    }

    numPendingPhysicsSteps_ =
        ea::min(numPhysicsSteps_ + numPendingPhysicsSteps_, maxPhysicalStepsPerNetworkUpdate) - numPhysicsSteps_;
}

void ServerNetworkManager::UpdatePhysics()
{
    if (!physicsWorld_)
        return;

    const float fixedTimeStep = 1.0f / physicsUpdateFrequency_;
    physicsWorld_->CustomUpdate(numPendingPhysicsSteps_, fixedTimeStep, 0.0f);

    numPhysicsSteps_ += numPendingPhysicsSteps_;
    numPendingPhysicsSteps_ = 0;
}

void ServerNetworkManager::BeginNetworkFrame()
{
    const float timeStep = 1.0f / updateFrequency_;
    UpdateClocks(timeStep);
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
        URHO3D_LOGWARNING("Connection is already added");
        assert(0);
    }

    ClientConnectionData& data = connections_[connection];
    data = ClientConnectionData{};

    data.connection_ = connection;
    data.comfirmedPings_.set_capacity(settings_.numInitialPings_);
    data.comfirmedPingsSorted_.set_capacity(settings_.numInitialPings_);
    data.pendingPings_.set_capacity(settings_.maxOngoingPings_);
}

void ServerNetworkManager::RemoveConnection(AbstractConnection* connection)
{
    if (!connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection is not added");
        assert(0);
    }

    connections_.erase(connection);
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

            msg.numStartClockSamples_ = settings_.numStartClockSamples_;
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
                debugInfo->append(NetworkManagerBase::FormatNetworkId(networkId));
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
                debugInfo->append(NetworkManagerBase::FormatNetworkId(networkObject->GetNetworkId()));
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
                debugInfo->append(NetworkManagerBase::FormatNetworkId(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

void ServerNetworkManager::SendUpdateObjectsUnreliableMessage(ClientConnectionData& data)
{
    data.connection_->SendGeneratedMessage(MSG_UPDATE_OBJECTS_UNRELIABLE, NetworkMessageFlag::InOrder | NetworkMessageFlag::Reliable,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(currentFrame_);

        bool sendMessage = false;
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
                debugInfo->append(NetworkManagerBase::FormatNetworkId(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

void ServerNetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_PONG:
    {
        ClientConnectionData& data = GetConnection(connection);
        const auto& msg = ReadNetworkMessage<MsgPingPong>(messageData);
        connection->OnMessageReceived(messageId, msg);

        const auto iter = ea::find_if(data.pendingPings_.begin(), data.pendingPings_.end(),
            [&](const ClientPing& ping) { return ping.magic_ == msg.magic_; });
        if (iter == data.pendingPings_.end())
        {
            URHO3D_LOGWARNING("{}: Unexpected pong received", connection->ToString(), msg.magic_);
            break;
        }

        ClientPing& pendingPing = *iter;
        const auto pingMs = pendingPing.timer_.GetUSec(false) / 1000 / 2;
        data.pendingPings_.erase(iter);

        if (data.comfirmedPings_.full())
            data.comfirmedPings_.pop_front();
        data.comfirmedPings_.push_back(pingMs);

        RecalculateAvergagePing(data);
        URHO3D_LOGTRACE("{}: Ping of {}ms added to buffer, average is {}ms", connection->ToString(), pingMs, data.averagePing_);
        break;
    }

    case MSG_SYNCHRONIZE_ACK:
    {
        ClientConnectionData& data = GetConnection(connection);
        const auto& msg = ReadNetworkMessage<MsgSynchronizeAck>(messageData);
        connection->OnMessageReceived(messageId, msg);

        if (!data.pendingSynchronization_ || data.pendingSynchronization_ != msg.magic_)
        {
            URHO3D_LOGWARNING("{}: Unexpected synchronization ack received", connection->ToString(), msg.magic_);
            break;
        }

        data.pendingSynchronization_ = ea::nullopt;
        data.synchronized_ = true;
    }

    default:
        break;
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

ea::string ServerNetworkManager::ToString() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        currentFrame_);
}

}
