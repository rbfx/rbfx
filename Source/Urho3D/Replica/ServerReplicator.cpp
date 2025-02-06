// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/ServerReplicator.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Exception.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/RandomEngine.h"
#include "Urho3D/Network/Connection.h"
#include "Urho3D/Network/MessageUtils.h"
#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Replica/NetworkObject.h"
#include "Urho3D/Replica/NetworkSettingsConsts.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

unsigned GetIndex(NetworkId networkId)
{
    return DeconstructComponentReference(networkId).first;
}

} // namespace

SharedReplicationState::SharedReplicationState(NetworkObjectRegistry* objectRegistry)
    : objectRegistry_(objectRegistry)
{
    URHO3D_ASSERT(objectRegistry_);

    objectRegistry_->OnNetworkObjectAdded.Subscribe(this, &SharedReplicationState::OnNetworkObjectAdded);
    objectRegistry_->OnNetworkObjectRemoved.Subscribe(this, &SharedReplicationState::OnNetworkObjectRemoved);

    for (NetworkObject* networkObject : objectRegistry_->GetNetworkObjects())
        OnNetworkObjectAdded(networkObject);
}

void SharedReplicationState::OnNetworkObjectAdded(NetworkObject* networkObject)
{
    recentlyAddedObjects_.insert(networkObject->GetNetworkId());
}

void SharedReplicationState::OnNetworkObjectRemoved(NetworkObject* networkObject)
{
    if (recentlyAddedObjects_.erase(networkObject->GetNetworkId()) == 0)
        recentlyRemovedObjects_.insert(networkObject->GetNetworkId());

    if (AbstractConnection* ownerConnection = networkObject->GetOwnerConnection())
    {
        auto& ownedObjects = ownedObjectsByConnection_[ownerConnection];
        ownedObjects.erase(networkObject);
        if (ownedObjects.empty())
            ownedObjectsByConnection_.erase(ownerConnection);
    }
}

void SharedReplicationState::PrepareForUpdate()
{
    ResetFrameBuffers();
    InitializeNewObjects();

    objectRegistry_->UpdateNetworkObjects();
    objectRegistry_->GetSortedNetworkObjects(sortedNetworkObjects_);
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
    for (NetworkId networkId : recentlyAddedObjects_)
    {
        NetworkObject* networkObject = objectRegistry_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_ASSERTLOG(0, "Cannot find recently added NetworkObject");
            continue;
        }

        networkObject->InitializeOnServer();
        networkObject->SetNetworkMode(NetworkObjectMode::Server);

        if (AbstractConnection* ownerConnection = networkObject->GetOwnerConnection())
            ownedObjectsByConnection_[ownerConnection].insert(networkObject);
    }
    recentlyAddedObjects_.clear();
}

void SharedReplicationState::QueueDeltaUpdate(NetworkObject* networkObject)
{
    const unsigned index = GetIndex(networkObject->GetNetworkId());
    isDeltaUpdateQueued_[index] = true;
}

void SharedReplicationState::CookDeltaUpdates(NetworkFrame currentFrame)
{
    recentlyRemovedObjects_.clear();

    for (unsigned i = 0; i < isDeltaUpdateQueued_.size(); ++i)
    {
        if (!isDeltaUpdateQueued_[i])
            continue;

        NetworkObject* networkObject = objectRegistry_->GetNetworkObjectByIndex(i);
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
    return objectRegistry_->GetNetworkIndexUpperBound();
}

const ea::unordered_set<NetworkObject*>& SharedReplicationState::GetOwnedObjectsByConnection(
    AbstractConnection* connection) const
{
    static const ea::unordered_set<NetworkObject*> emptyCollection;
    const auto iter = ownedObjectsByConnection_.find(connection);
    return iter != ownedObjectsByConnection_.end() ? iter->second : emptyCollection;
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
    NetworkObjectRegistry* objectRegistry, AbstractConnection* connection, const VariantMap& settings)
    : objectRegistry_(objectRegistry)
    , connection_(connection)
    , settings_(settings)
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , inputDelayFilter_(GetSetting(NetworkSettings::InputDelayFilterBufferSize).GetUInt())
    , inputStats_(GetSetting(NetworkSettings::InputBufferingWindowSize).GetUInt(), InputStatsSafetyLimit)
    , inputBufferFilter_(GetSetting(NetworkSettings::InputBufferingFilterBufferSize).GetUInt())
{
    SetNetworkSetting(settings_, NetworkSettings::ConnectionId, connection_->GetObjectID());
}

void ClientSynchronizationState::BeginNetworkFrame(NetworkFrame currentFrame, float overtime)
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
        WriteSerializedMessage(
            *connection_, MSG_CONFIGURE, MsgConfigure{magic, settings_}, PacketType::ReliableUnordered);
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
        WriteSerializedMessage(*connection_, MSG_SCENE_CLOCK, msg, PacketType::UnreliableUnordered);
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

    const float tweakA = GetSetting(NetworkSettings::InputBufferingTweakA).GetFloat();
    const float tweakB = GetSetting(NetworkSettings::InputBufferingTweakB).GetFloat();
    const int newInputBufferSize = RoundToInt(tweakA * inputBufferFilter_.GetStabilizedAverageMaxValue() + tweakB);

    const int minInputBuffer = GetSetting(NetworkSettings::InputBufferingMin).GetUInt();
    const int maxInputBuffer = GetSetting(NetworkSettings::InputBufferingMax).GetUInt();
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
        const auto msg = ReadSerializedMessage<MsgSynchronized>(messageData);
        connection_->LogMessagePayload(messageId, msg);

        ProcessSynchronized(msg);
        return true;
    }
    default: return false;
    }
}

void ClientSynchronizationState::OnInputReceived(NetworkFrame inputFrame)
{
    inputStats_.OnInputReceived(inputFrame);
}

unsigned ClientSynchronizationState::MakeMagic() const
{
    return RandomEngine::GetDefaultEngine().GetUInt();
}

ClientReplicationState::ClientReplicationState(
    NetworkObjectRegistry* objectRegistry, AbstractConnection* connection, const VariantMap& settings)
    : ClientSynchronizationState(objectRegistry, connection, settings)
{
}

void ClientReplicationState::SendMessages(NetworkFrame currentFrame, const SharedReplicationState& sharedState)
{
    ClientSynchronizationState::SendMessages();

    if (IsSynchronized())
    {
        SendRemoveObjects();
        SendAddObjects();
        SendUpdateObjectsReliable(sharedState);
        SendUpdateObjectsUnreliable(currentFrame, sharedState);
    }
}

bool ClientReplicationState::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (ClientSynchronizationState::ProcessMessage(messageId, messageData))
        return true;

    switch (messageId)
    {
    case MSG_OBJECTS_FEEDBACK_UNRELIABLE:
        ProcessObjectsFeedbackUnreliable(messageData);
        return true;

    default: return false;
    }
}

void ClientReplicationState::ProcessObjectsFeedbackUnreliable(MemoryBuffer& messageData)
{
    if (!IsSynchronized())
    {
        URHO3D_LOGWARNING("Connection {}: Received unexpected feedback", connection_->ToString());
        return;
    }

    const auto feedbackFrame = static_cast<NetworkFrame>(messageData.ReadInt64());
    OnInputReceived(feedbackFrame);

    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        NetworkObject* networkObject = objectRegistry_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Connection {}: Received feedback for unknown NetworkObject {}", connection_->ToString(),
                ToString(networkId));
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
    MultiMessageWriter writer{*connection_, MSG_REMOVE_OBJECTS, PacketType::ReliableOrdered};

    VectorBuffer& msg = writer.GetBuffer();
    ea::string* debugInfo = writer.GetDebugInfo();

    msg.WriteInt64(static_cast<long long>(GetCurrentFrame()));
    writer.CompleteHeader();

    for (NetworkId networkId : pendingRemovedObjects_)
    {
        msg.WriteUInt(static_cast<unsigned>(networkId));

        if (debugInfo)
        {
            if (!debugInfo->empty())
                debugInfo->append(", ");
            debugInfo->append(ToString(networkId));
        }

        writer.CompletePayload();
    }
}

void ClientReplicationState::SendAddObjects()
{
    LargeMessageWriter writer{*connection_, MSG_ADD_OBJECTS_INCOMPLETE, MSG_ADD_OBJECTS};

    VectorBuffer& msg = writer.GetBuffer();
    ea::string* debugInfo = writer.GetDebugInfo();

    msg.WriteInt64(static_cast<long long>(GetCurrentFrame()));

    bool sendMessage = false;
    for (const auto& [networkObject, isSnapshot] : pendingUpdatedObjects_)
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

    if (!sendMessage)
        writer.Discard();
}

void ClientReplicationState::SendUpdateObjectsReliable(const SharedReplicationState& sharedState)
{
    LargeMessageWriter writer{*connection_, MSG_UPDATE_OBJECTS_RELIABLE_INCOMPLETE, MSG_UPDATE_OBJECTS_RELIABLE};

    VectorBuffer& msg = writer.GetBuffer();
    ea::string* debugInfo = writer.GetDebugInfo();

    msg.WriteInt64(static_cast<long long>(GetCurrentFrame()));

    bool sendMessage = false;
    for (const auto& [networkObject, isSnapshot] : pendingUpdatedObjects_)
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

    if (!sendMessage)
        writer.Discard();
}

void ClientReplicationState::SendUpdateObjectsUnreliable(
    NetworkFrame currentFrame, const SharedReplicationState& sharedState)
{
    MultiMessageWriter writer{*connection_, MSG_UPDATE_OBJECTS_UNRELIABLE, PacketType::UnreliableUnordered};

    VectorBuffer& msg = writer.GetBuffer();
    ea::string* debugInfo = writer.GetDebugInfo();

    msg.WriteInt64(static_cast<long long>(GetCurrentFrame()));
    writer.CompleteHeader();

    for (const auto& [networkObject, isSnapshot] : pendingUpdatedObjects_)
    {
        // Skip redundant updates, both if update is empty or if snapshot was already sent
        const unsigned index = GetIndex(networkObject->GetNetworkId());
        if (isSnapshot)
            continue;

        const auto updateSpan = sharedState.GetUnreliableUpdateByIndex(index);
        if (!updateSpan)
            continue;

        const NetworkObjectRelevance relevance = objectsRelevance_[index];
        URHO3D_ASSERT(relevance != NetworkObjectRelevance::Irrelevant);
        if (relevance == NetworkObjectRelevance::NoUpdates)
            continue;

        if (static_cast<long long>(currentFrame) % static_cast<unsigned>(relevance) != 0)
            continue;

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

        writer.CompletePayload();
    }
}

void ClientReplicationState::UpdateNetworkObjects(SharedReplicationState& sharedState)
{
    if (!IsSynchronized())
        return;

    const float timeStep = 1.0f / updateFrequency_;
    const float relevanceTimeout = GetSetting(NetworkSettings::RelevanceTimeout).GetFloat();

    const unsigned indexUpperBound = sharedState.GetIndexUpperBound();
    objectsRelevance_.resize(indexUpperBound, NetworkObjectRelevance::Irrelevant);
    objectsRelevanceTimeouts_.resize(indexUpperBound);

    pendingRemovedObjects_.clear();
    pendingUpdatedObjects_.clear();

    // Process removed components first
    for (NetworkId networkId : sharedState.GetRecentlyRemovedObjects())
    {
        const unsigned index = GetIndex(networkId);
        if (objectsRelevance_[index] != NetworkObjectRelevance::Irrelevant)
        {
            objectsRelevance_[index] = NetworkObjectRelevance::Irrelevant;
            pendingRemovedObjects_.push_back(networkId);
        }
    }

    // Process active components
    for (NetworkObject* networkObject : sharedState.GetSortedObjects())
    {
        const NetworkId networkId = networkObject->GetNetworkId();
        const NetworkId parentNetworkId = networkObject->GetParentNetworkId();
        const unsigned index = GetIndex(networkId);

        const bool wasRelevant = objectsRelevance_[index] != NetworkObjectRelevance::Irrelevant;
        const bool isParentRelevant = parentNetworkId == NetworkId::None
            || objectsRelevance_[GetIndex(parentNetworkId)] != NetworkObjectRelevance::Irrelevant;

        if (!wasRelevant && isParentRelevant)
        {
            // Begin replication of the object if both the object and its parent are relevant
            objectsRelevance_[index] =
                networkObject->GetRelevanceForClient(connection_).value_or(NetworkObjectRelevance::NormalUpdates);
            if (objectsRelevance_[index] != NetworkObjectRelevance::Irrelevant)
            {
                objectsRelevanceTimeouts_[index] = relevanceTimeout;
                pendingUpdatedObjects_.push_back({networkObject, true});
            }
        }
        else if (wasRelevant)
        {
            // If replicating, check periodically (abort replication immediately if parent is removed)
            objectsRelevanceTimeouts_[index] -= timeStep;
            if (objectsRelevanceTimeouts_[index] < 0.0f || !isParentRelevant)
            {
                objectsRelevance_[index] = isParentRelevant
                    ? networkObject->GetRelevanceForClient(connection_).value_or(NetworkObjectRelevance::NormalUpdates)
                    : NetworkObjectRelevance::Irrelevant;

                if (objectsRelevance_[index] == NetworkObjectRelevance::Irrelevant)
                {
                    // Remove irrelevant component
                    pendingRemovedObjects_.push_back(networkId);
                    continue;
                }

                objectsRelevanceTimeouts_[index] = relevanceTimeout;
            }

            // Queue non-snapshot update
            sharedState.QueueDeltaUpdate(networkObject);
            pendingUpdatedObjects_.push_back({networkObject, false});
        }
    }
}

ServerReplicator::ServerReplicator(Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
    , replicationManager_(scene->GetSystemComponent<ReplicationManager>())
    , updateFrequency_(network_->GetUpdateFps())
    , sharedState_(MakeShared<SharedReplicationState>(replicationManager_))
{
    if (replicationManager_->IsFixedUpdateServer())
    {
        SceneUpdateSynchronizer::Params params;
        params.isServer_ = true;
        params.networkFrequency_ = updateFrequency_;
        params.allowZeroUpdatesOnServer_ = replicationManager_->IsAllowZeroUpdatesOnServer();
        updateSync_ = MakeShared<SceneUpdateSynchronizer>(scene_, params);
    }

    SetDefaultNetworkSetting(settings_, NetworkSettings::InternalProtocolVersion);
    SetNetworkSetting(settings_, NetworkSettings::UpdateFrequency, updateFrequency_);

    SubscribeToEvent(E_INPUTREADY,
        [this](VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();

        const bool isUpdateNow = network_->IsUpdateNow();
        const float overtime = network_->GetUpdateOvertime();
        OnInputReady(timeStep, isUpdateNow, overtime);
    });

    SubscribeToEvent(network_, E_NETWORKUPDATE,
        [this](VariantMap& eventData)
    {
        using namespace NetworkUpdate;
        const bool isServer = eventData[P_ISSERVER].GetBool();
        if (isServer)
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
        if (updateSync_)
            updateSync_->Synchronize(currentFrame_, overtime);

        for (auto& [connection, clientState] : connections_)
            clientState->BeginNetworkFrame(currentFrame_, overtime);

        using namespace BeginServerNetworkFrame;
        auto& eventData = network_->GetEventDataMap();
        eventData[P_FRAME] = static_cast<long long>(currentFrame_);
        network_->SendEvent(E_BEGINSERVERNETWORKFRAME, eventData);
    }
    else
    {
        if (updateSync_)
            updateSync_->Update(timeStep);
    }
}

void ServerReplicator::OnNetworkUpdate()
{
    using namespace EndServerNetworkFrame;
    auto& eventData = network_->GetEventDataMap();
    eventData[P_FRAME] = static_cast<long long>(currentFrame_);
    network_->SendEvent(E_ENDSERVERNETWORKFRAME, eventData);

    sharedState_->PrepareForUpdate();
    for (auto& [connection, clientState] : connections_)
        clientState->UpdateNetworkObjects(*sharedState_);
    sharedState_->CookDeltaUpdates(currentFrame_);

    for (auto& [connection, clientState] : connections_)
        clientState->SendMessages(currentFrame_, *sharedState_);
}

void ServerReplicator::AddConnection(AbstractConnection* connection)
{
    if (connections_.contains(connection))
    {
        URHO3D_ASSERTLOG(0, "Connection {} is already added", connection->ToString());
        return;
    }

    connections_.erase(connection);
    const auto& [iter, insterted] =
        connections_.emplace(connection, MakeShared<ClientReplicationState>(replicationManager_, connection, settings_));

    URHO3D_LOGINFO("Connection {} is added", connection->ToString());
}

void ServerReplicator::RemoveConnection(AbstractConnection* connection)
{
    if (!connections_.contains(connection))
    {
        URHO3D_ASSERTLOG(0, "Connection {} is not added", connection->ToString());
        return;
    }

    connections_.erase(connection);
    URHO3D_LOGINFO("Connection {} is removed", connection->ToString());
}

bool ServerReplicator::ProcessMessage(
    AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (ClientReplicationState* clientState = GetClientState(connection))
        return clientState->ProcessMessage(messageId, messageData);
    return false;
}

void ServerReplicator::ProcessSceneUpdate()
{
    if (network_->IsUpdateNow())
    {
        VariantMap& eventData = scene_->GetEventDataMap();
        const float fixedTimeStep = 1.0f / GetUpdateFrequency();

        using namespace SceneNetworkUpdate;
        eventData[P_SCENE] = scene_;
        eventData[P_TIMESTEP_REPLICA] = fixedTimeStep;
        eventData[P_TIMESTEP_INPUT] = fixedTimeStep;
        scene_->SendEvent(E_SCENENETWORKUPDATE, eventData);
    }
}

void ServerReplicator::ReportInputLoss(AbstractConnection* connection, float percentLoss)
{
    if (ClientReplicationState* clientState = GetClientState(connection))
        clientState->SetReportedInputLoss(percentLoss);
}

void ServerReplicator::SetCurrentFrame(NetworkFrame frame)
{
    currentFrame_ = frame;
}

ClientReplicationState* ServerReplicator::GetClientState(AbstractConnection* connection) const
{
    auto iter = connections_.find(connection);
    return iter != connections_.end() ? iter->second : nullptr;
}

ea::string ServerReplicator::GetDebugInfo() const
{
    ea::string result;

    result +=
        Format("Scene '{}': Time #{}\n", !scene_->GetName().empty() ? scene_->GetName() : "Unnamed", currentFrame_);

    for (const auto& [connection, clientState] : connections_)
    {
        result += Format("Connection {}: Ping {}ms, InDelay {}+{} frames, InLoss {}%\n", connection->ToString(),
            connection->GetPing(), clientState->GetInputDelay(), clientState->GetInputBufferSize(),
            CeilToInt(clientState->GetReportedInputLoss() * 100.0f));
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

const ea::unordered_set<NetworkObject*>& ServerReplicator::GetNetworkObjectsOwnedByConnection(
    AbstractConnection* connection) const
{
    return sharedState_->GetOwnedObjectsByConnection(connection);
}

NetworkObject* ServerReplicator::GetNetworkObjectOwnedByConnection(AbstractConnection* connection) const
{
    const auto& ownedObjects = GetNetworkObjectsOwnedByConnection(connection);
    return ownedObjects.size() == 1 ? *ownedObjects.begin() : nullptr;
}

} // namespace Urho3D
