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
#include "../Network/NetworkObject.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkManager.h"
#include "../Network/NetworkSettingsConsts.h"
#include "../Network/ClientNetworkManager.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

ClientSynchronizationManager::ClientSynchronizationManager(Scene* scene, AbstractConnection* connection,
    const MsgSceneClock& msg, const VariantMap& serverSettings, const ClientSynchronizationSettings& settings)
    : scene_(scene)
    , connection_(connection)
    , serverSettings_(serverSettings)
    , thisConnectionId_(GetSetting(NetworkSettings::ConnectionId).GetUInt())
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , timeSnapThreshold_{GetSetting(NetworkSettings::TimeSnapThreshold).GetFloat()}
    , timeErrorTolerance_{GetSetting(NetworkSettings::TimeErrorTolerance).GetFloat()}
    , minTimeDilation_{GetSetting(NetworkSettings::MinTimeDilation).GetFloat()}
    , maxTimeDilation_{GetSetting(NetworkSettings::MaxTimeDilation).GetFloat()}
    , settings_(settings)
    , inputDelay_(msg.inputDelay_)
    , replicaTime_(updateFrequency_, timeSnapThreshold_, timeErrorTolerance_, minTimeDilation_, maxTimeDilation_)
    , inputTime_(updateFrequency_, timeSnapThreshold_, timeErrorTolerance_, minTimeDilation_, maxTimeDilation_)
    , physicsSync_(scene_, updateFrequency_, false)
{
    UpdateServerTime(msg, false);
    replicaTime_.Reset(ToClientTime(serverTime_));
    inputTime_.Reset(ToInputTime(serverTime_));
    latestScaledInputTime_ = replicaTime_.Get();
}

ClientSynchronizationManager::~ClientSynchronizationManager()
{
}

double ClientSynchronizationManager::MillisecondsToFrames(double valueMs) const
{
    return SecondsToFrames(valueMs * 0.001f);
}

double ClientSynchronizationManager::SecondsToFrames(double valueSec) const
{
    // s * (frames / s) = frames
    return valueSec * updateFrequency_;
}

float ClientSynchronizationManager::ApplyTimeStep(float timeStep, const ea::vector<MsgSceneClock>& pendingClockUpdates)
{
    serverTime_ += SecondsToFrames(timeStep);
    for (const MsgSceneClock& msg : pendingClockUpdates)
        UpdateServerTime(msg, true);

    replicaTime_.Update(timeStep, ToClientTime(serverTime_));

    const NetworkTime previousInputTime = inputTime_.Get();
    const float scaledTimeStep = inputTime_.Update(timeStep, ToInputTime(serverTime_));

    if (timeStep != scaledTimeStep)
        latestScaledInputTime_ = inputTime_.Get();

    if (previousInputTime.GetFrame() != inputTime_.Get().GetFrame())
        synchronizedPhysicsTick_ = physicsSync_.Synchronize(inputTime_.Get().GetSubFrame() / updateFrequency_);
    else
    {
        physicsSync_.Update(scaledTimeStep);
        synchronizedPhysicsTick_ = ea::nullopt;
    }

    return scaledTimeStep;
}

const Variant& ClientSynchronizationManager::GetSetting(const NetworkSetting& setting) const
{
    return GetNetworkSetting(serverSettings_, setting);
}

void ClientSynchronizationManager::UpdateServerTime(const MsgSceneClock& msg, bool skipOutdated)
{
    if (skipOutdated && NetworkTime{msg.latestFrame_} - NetworkTime{latestServerFrame_} < 0.0)
        return;

    const unsigned serverFrameTime = connection_->RemoteToLocalTime(msg.latestFrameTime_);
    const auto offsetMs = static_cast<int>(connection_->GetLocalTime() - serverFrameTime);

    inputDelay_ = msg.inputDelay_;
    latestServerFrame_ = msg.latestFrame_;
    serverTime_ = NetworkTime{msg.latestFrame_};
    serverTime_ += MillisecondsToFrames(offsetMs);
}

NetworkTime ClientSynchronizationManager::ToClientTime(const NetworkTime& serverTime) const
{
    const double clientDelay = settings_.clientTimeDelayInSeconds_ + connection_->GetPing() * 0.001;
    return serverTime - SecondsToFrames(clientDelay);
}

NetworkTime ClientSynchronizationManager::ToInputTime(const NetworkTime& serverTime) const
{
    return serverTime + static_cast<double>(inputDelay_);
}

ClientNetworkManager::ClientNetworkManager(NetworkManagerBase* base, Scene* scene, AbstractConnection* connection)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , base_(base)
    , scene_(scene)
    , connection_(connection)
{
    SubscribeToEvent(E_INPUTREADY, [this](StringHash, VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();
        SynchronizeClocks(timeStep);
        UpdateReplica(timeStep);
    });
}

void ClientNetworkManager::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_CONFIGURE:
    {
        const auto msg = ReadNetworkMessage<MsgConfigure>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessConfigure(msg);
        break;
    }

    case MSG_SCENE_CLOCK:
    {
        const auto msg = ReadNetworkMessage<MsgSceneClock>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessSceneClock(msg);
        break;
    }

    case MSG_REMOVE_OBJECTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessRemoveObjects(messageData);
        break;
    }

    case MSG_ADD_OBJECTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessAddObjects(messageData);
        break;
    }

    case MSG_UPDATE_OBJECTS_RELIABLE:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessUpdateObjectsReliable(messageData);
        break;
    }

    case MSG_UPDATE_OBJECTS_UNRELIABLE:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessUpdateObjectsUnreliable(messageData);
        break;
    }

    default:
        break;
    }
}

void ClientNetworkManager::ProcessConfigure(const MsgConfigure& msg)
{
    serverSettings_ = msg.settings_;
    synchronizationMagic_ = msg.magic_;
}

void ClientNetworkManager::ProcessSceneClock(const MsgSceneClock& msg)
{
    if (!sync_)
        pendingClockUpdates_.clear();
    pendingClockUpdates_.push_back(msg);
}

void ClientNetworkManager::ProcessRemoveObjects(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        WeakPtr<NetworkObject> networkObject{ base_->GetNetworkObject(networkId) };
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Cannot find NetworkObject {} to remove", ToString(networkId));
            continue;
        }
        networkObject->PrepareToRemove();
        if (networkObject)
            networkObject->Remove();
    }
}

void ClientNetworkManager::ProcessAddObjects(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();
        const unsigned ownerConnectionId = messageData.ReadVLE();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        const bool isOwned = ownerConnectionId == sync_->GetConnectionId();
        if (NetworkObject* networkObject = CreateNetworkObject(networkId, componentType, isOwned))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadSnapshot(messageFrame, componentBuffer_);
        }
    }
}

void ClientNetworkManager::ProcessUpdateObjectsReliable(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadReliableDelta(messageFrame, componentBuffer_);
        }
    }
}

void ClientNetworkManager::ProcessUpdateObjectsUnreliable(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();

    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadUnreliableDelta(messageFrame, componentBuffer_);
        }
    }
}

NetworkObject* ClientNetworkManager::CreateNetworkObject(NetworkId networkId, StringHash componentType, bool isOwned)
{
    auto networkObject = DynamicCast<NetworkObject>(context_->CreateObject(componentType));
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot create NetworkObject {} of type #{} '{}'",
            ToString(networkId), componentType.Value(), componentType.Reverse());
        return nullptr;
    }
    networkObject->SetNetworkId(networkId);

    if (isOwned)
    {
        networkObject->SetNetworkMode(NetworkObjectMode::ClientOwned);
        ownedObjects_.insert(WeakPtr<NetworkObject>(networkObject));
    }
    else
        networkObject->SetNetworkMode(NetworkObjectMode::ClientReplicated);

    const unsigned networkIndex = NetworkManagerBase::DecomposeNetworkId(networkId).first;
    if (NetworkObject* oldNetworkObject = base_->GetNetworkObjectByIndex(networkIndex))
    {
        URHO3D_LOGWARNING("NetworkObject {} overwrites existing NetworkObject {}",
            ToString(networkId),
            ToString(oldNetworkObject->GetNetworkId()));
        RemoveNetworkObject(WeakPtr<NetworkObject>(oldNetworkObject));
    }

    Node* newNode = scene_->CreateChild(EMPTY_STRING, LOCAL);
    newNode->AddComponent(networkObject, 0, LOCAL);
    return networkObject;
}

NetworkObject* ClientNetworkManager::GetCheckedNetworkObject(NetworkId networkId, StringHash componentType)
{
    NetworkObject* networkObject = base_->GetNetworkObject(networkId);
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot find existing NetworkObject {}", ToString(networkId));
        return nullptr;
    }

    if (networkObject->GetType() != componentType)
    {
        URHO3D_LOGWARNING("NetworkObject {} has unexpected type '{}', message was prepared for {}",
            ToString(networkId), networkObject->GetTypeName(),
            componentType.ToDebugString());
        return nullptr;
    }

    return networkObject;
}

void ClientNetworkManager::RemoveNetworkObject(WeakPtr<NetworkObject> networkObject)
{
    if (networkObject->GetNetworkMode() == NetworkObjectMode::ClientOwned)
        ownedObjects_.erase(networkObject);

    networkObject->PrepareToRemove();
    if (networkObject)
        networkObject->Remove();
}

double ClientNetworkManager::GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const
{
    if (!sync_)
        return M_LARGE_VALUE;

    return sync_->GetServerTime() - NetworkTime{referenceFrame};
}

unsigned ClientNetworkManager::GetTraceCapacity() const
{
    if (!sync_)
        return 0;

    return CeilToInt(settings2_.traceDurationInSeconds_ * sync_->GetUpdateFrequency());
}

unsigned ClientNetworkManager::GetPositionExtrapolationFrames() const
{
    if (!sync_)
        return 0;

    return RoundToInt(settings2_.positionExtrapolationTimeInSeconds_ * sync_->GetUpdateFrequency());
}

ea::string ClientNetworkManager::GetDebugInfo() const
{
    static const ea::string unnamedScene = "Unnamed";
    const ea::string& sceneName = !scene_->GetName().empty() ? scene_->GetName() : unnamedScene;
    if (!sync_)
        return Format("Scene '{}': Ping {}ms, Pending synchronization...\n", sceneName, connection_->GetPing());

    const double inputDelayMs = (GetInputTime() - GetServerTime()) / sync_->GetUpdateFrequency() * 1000.0f;
    const double replicaDelayMs = (GetServerTime() - GetClientTime()) / sync_->GetUpdateFrequency() * 1000.0f;
    return Format("Scene '{}': Ping {}ms, Time {}ms+#{}-{}ms, Sync since #{}\n",
        sceneName,
        connection_->GetPing(),
        ea::max(0, CeilToInt(inputDelayMs)),
        GetServerTime().GetFrame(),
        ea::max(0, CeilToInt(replicaDelayMs)),
        GetLatestScaledInputFrame());
}

void ClientNetworkManager::SynchronizeClocks(float timeStep)
{
    if (sync_)
    {
        sync_->ApplyTimeStep(timeStep, pendingClockUpdates_);
        pendingClockUpdates_.clear();
        return;
    }

    if (!serverSettings_ || !connection_->IsClockSynchronized() || pendingClockUpdates_.empty())
        return;

    sync_.emplace(scene_, connection_, pendingClockUpdates_[0], *serverSettings_, settings2_);
    connection_->SendSerializedMessage(
        MSG_SYNCHRONIZED, MsgSynchronized{*synchronizationMagic_}, NetworkMessageFlag::Reliable);
}

void ClientNetworkManager::UpdateReplica(float timeStep)
{
    if (!sync_)
        return;

    const auto isNewInputFrame = sync_->GetSynchronizedPhysicsTick();

    const auto& networkObjects = base_->GetUnorderedNetworkObjects();
    for (NetworkObject* networkObject : networkObjects)
    {
        if (networkObject)
            networkObject->InterpolateState(sync_->GetReplicaTime(), sync_->GetInputTime(), isNewInputFrame);
    }

    if (isNewInputFrame)
    {
        network_->SendEvent(E_NETWORKCLIENTUPDATE);
        SendObjectsFeedbackUnreliable(sync_->GetInputTime().GetFrame());
    }
}

void ClientNetworkManager::SendObjectsFeedbackUnreliable(unsigned feedbackFrame)
{
    connection_->SendGeneratedMessage(MSG_OBJECTS_FEEDBACK_UNRELIABLE, NetworkMessageFlag::None,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(feedbackFrame);

        bool sendMessage = false;
        for (NetworkObject* networkObject : ownedObjects_)
        {
            if (!networkObject)
                continue;

            componentBuffer_.Clear();
            if (const auto mask = networkObject->GetUnreliableFeedbackMask(feedbackFrame))
            {
                networkObject->WriteUnreliableFeedback(feedbackFrame, mask, componentBuffer_);
                sendMessage = true;
                msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
                msg.WriteBuffer(componentBuffer_.GetBuffer());
            }

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

}
