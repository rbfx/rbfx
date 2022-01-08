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
#include "../Network/ClientNetworkManager.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

ClientSynchronizationManager::ClientSynchronizationManager(
    Scene* scene, const MsgSynchronize& msg, const ClientSynchronizationSettings& settings)
    : scene_(scene)
    , thisConnectionId_(msg.connectionId_)
    , updateFrequency_(msg.updateFrequency_)
    , numSamples_(msg.numOngoingClockSamples_)
    , numTrimmedSamples_(msg.numTrimmedClockSamples_)
    , settings_(settings)
    , latestServerTime_(msg.lastFrame_)
    , latestPing_(msg.ping_)
    , serverTime_(ToServerTime(msg.lastFrame_, msg.ping_))
    , clientTime_(ToClientTime(serverTime_))
    , smoothClientTime_(clientTime_)
    , latestUnstableClientTime_(clientTime_)
    , physicsSync_(scene_, updateFrequency_, true)
{
    serverTimeErrors_.reserve(numSamples_);
    // Don't smooth client time as it adds even more latency to synchronization
    clientTimeErrors_.reserve(1);
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

void ClientSynchronizationManager::ProcessClockUpdate(const MsgClock& msg)
{
    pendingClockUpdates_.push_back(msg);
}

float ClientSynchronizationManager::ApplyTimeStep(float timeStep)
{
    serverTime_ += SecondsToFrames(timeStep);
    clientTime_ += SecondsToFrames(timeStep);

    for (const MsgClock& msg : pendingClockUpdates_)
        ProcessPendingClockUpdate(msg);
    pendingClockUpdates_.clear();

    const NetworkTime previousSmoothClientTime = smoothClientTime_;
    const float adjustedTimeStep = UpdateSmoothClientTime(timeStep);

    isNewFrame_ = previousSmoothClientTime.GetFrame() != smoothClientTime_.GetFrame();
    const float overtime = smoothClientTime_.GetSubFrame() / updateFrequency_;
    physicsSync_.UpdateClock(adjustedTimeStep, isNewFrame_ ? ea::make_optional(overtime) : ea::nullopt);

    return adjustedTimeStep;
}

void ClientSynchronizationManager::ProcessPendingClockUpdate(const MsgClock& msg)
{
    // Skip outdated updates
    const bool isLatest = NetworkTime{msg.lastFrame_} - latestServerTime_ > 0.0;
    if (!isLatest)
        return;

    latestServerTime_ = NetworkTime{msg.lastFrame_};
    latestPing_ = msg.ping_;

    const NetworkTime serverTimeFromMessage = ToServerTime(msg.lastFrame_, msg.ping_);
    const double serverTimeError = serverTimeFromMessage - serverTime_;

    if (serverTimeError >= SecondsToFrames(settings_.timeSnapThresholdInSeconds_))
    {
        ResetServerAndClientTime(serverTimeFromMessage);
        return;
    }

    CheckAndAdjustTime(serverTime_, serverTimeErrors_, serverTimeError, numTrimmedSamples_, false);

    const NetworkTime expectedClientTime = ToClientTime(serverTime_);
    const double clientTimeError = expectedClientTime - clientTime_;
    CheckAndAdjustTime(clientTime_, clientTimeErrors_, clientTimeError, 0, true);
}

void ClientSynchronizationManager::ResetServerAndClientTime(const NetworkTime& serverTime)
{
    URHO3D_LOGINFO("Client clock is reset from {} to {}",
        serverTime_.ToString(), serverTime.ToString());

    serverTime_ = serverTime;
    clientTime_ = ToClientTime(serverTime);
    smoothClientTime_ = clientTime_;
    latestUnstableClientTime_ = clientTime_;

    serverTimeErrors_.clear();
    clientTimeErrors_.clear();
}

void ClientSynchronizationManager::CheckAndAdjustTime(
    NetworkTime& time, ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples, bool quiet) const
{
    if (const auto averageError = UpdateAverageError(errors, error, numTrimmedSamples))
    {
        const double averageErrorAbs = std::abs(*averageError);
        if (averageErrorAbs >= settings_.timeRewindThresholdInSeconds_)
        {
            const double adjustment = Sign(*averageError) * (averageErrorAbs - settings_.timeRewindThresholdInSeconds_ / 2);
            const NetworkTime oldTime = time;
            AdjustTime(time, errors, adjustment);

            if (!quiet)
                URHO3D_LOGINFO("Client clock is reset from {} to {}", oldTime.ToString(), time.ToString());
        }
    }
}

ea::optional<double> ClientSynchronizationManager::UpdateAverageError(
    ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples) const
{
    if (errors.full())
        errors.pop_front();
    errors.push_back(error);
    if (!errors.full())
        return ea::nullopt;

    static thread_local ea::vector<double> tempBuffer;
    tempBuffer.assign(errors.begin(), errors.end());
    ea::sort(tempBuffer.begin(), tempBuffer.end());

    // Skip first elements because lowest values correspond to ping spikes
    const auto beginIter = std::next(tempBuffer.begin(), ea::min(tempBuffer.size() - 1, numTrimmedSamples));
    const auto endIter = tempBuffer.end();
    return ea::accumulate(beginIter, endIter, 0.0) / ea::distance(beginIter, endIter);
}

void ClientSynchronizationManager::AdjustTime(
    NetworkTime& time, ea::ring_buffer<double>& errors, double adjustment) const
{
    time += adjustment;
    for (double& error : errors)
        error -= adjustment;
}

float ClientSynchronizationManager::UpdateSmoothClientTime(float timeStep)
{
    const double timeError = (clientTime_ - smoothClientTime_) / updateFrequency_ - timeStep;
    if (std::abs(timeError) < M_LARGE_EPSILON)
        smoothClientTime_ = clientTime_;
    else
    {
        const double minTimeStep = timeStep * settings_.minTimeStepScale_;
        const double maxTimeStep = timeStep * settings_.maxTimeStepScale_;
        timeStep = Clamp(timeStep + timeError, minTimeStep, maxTimeStep);
        latestUnstableClientTime_ = clientTime_;
        smoothClientTime_ += SecondsToFrames(timeStep);
    }

    return timeStep;
}

NetworkTime ClientSynchronizationManager::ToServerTime(unsigned lastServerFrame, unsigned pingMs) const
{
    return NetworkTime{lastServerFrame} + MillisecondsToFrames(pingMs);
}

NetworkTime ClientSynchronizationManager::ToClientTime(const NetworkTime& serverTime) const
{
    const double clientDelay = settings_.clientTimeDelayInSeconds_ + latestPing_ * 0.001;
    return serverTime - SecondsToFrames(clientDelay);
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
        UpdateReplica(timeStep);
    });
}

void ClientNetworkManager::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_PING:
    {
        const auto msg = ReadNetworkMessage<MsgPingPong>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessPing(msg);
        break;
    }

    case MSG_SYNCHRONIZE:
    {
        const auto msg = ReadNetworkMessage<MsgSynchronize>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessSynchronize(msg);
        break;
    }

    case MSG_CLOCK:
    {
        const auto msg = ReadNetworkMessage<MsgClock>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessClock(msg);
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

void ClientNetworkManager::ProcessPing(const MsgPingPong& msg)
{
    const bool isReliable = !!(msg.magic_ & 1u);
    connection_->SendSerializedMessage(
        MSG_PONG, MsgPingPong{msg.magic_}, isReliable ? NetworkMessageFlag::Reliable : NetworkMessageFlag::None);
}

void ClientNetworkManager::ProcessSynchronize(const MsgSynchronize& msg)
{
    sync_.emplace(scene_, msg, settings_);

    connection_->SendSerializedMessage(
        MSG_SYNCHRONIZE_ACK, MsgSynchronizeAck{msg.magic_}, NetworkMessageFlag::Reliable);

    URHO3D_LOGINFO("Client clock is started from {}", sync_->GetServerTime().ToString());
}

void ClientNetworkManager::ProcessClock(const MsgClock& msg)
{
    if (!sync_)
    {
        URHO3D_LOGWARNING("Client clock update received before synchronization");
        return;
    }

    sync_->ProcessClockUpdate(msg);
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
    const unsigned feedbackDelay = messageData.ReadVLE();

    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadUnreliableDelta(messageFrame, messageFrame - feedbackDelay, componentBuffer_);
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

    return CeilToInt(settings_.traceDurationInSeconds_ * sync_->GetUpdateFrequency());
}

unsigned ClientNetworkManager::GetPositionExtrapolationFrames() const
{
    if (!sync_)
        return 0;

    return RoundToInt(settings_.positionExtrapolationTimeInSeconds_ * sync_->GetUpdateFrequency());
}

ea::string ClientNetworkManager::GetDebugInfo() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Estimated frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        GetCurrentFrame());
}

void ClientNetworkManager::UpdateReplica(float timeStep)
{
    if (!sync_)
        return;

    sync_->ApplyTimeStep(timeStep);
    const bool isNewFrame = sync_->IsNewFrame();

    const auto& networkObjects = base_->GetUnorderedNetworkObjects();
    for (NetworkObject* networkObject : networkObjects)
    {
        if (networkObject)
            networkObject->InterpolateState(sync_->GetSmoothClientTime(), isNewFrame);
    }

    if (isNewFrame)
    {
        network_->SendEvent(E_NETWORKCLIENTUPDATE);
        SendObjectsFeedbackUnreliable(sync_->GetSmoothClientTime().GetFrame());
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
            if (networkObject->WriteUnreliableFeedback(feedbackFrame, componentBuffer_))
            {
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
