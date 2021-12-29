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
#include "../Network/NetworkObject.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkManager.h"
#include "../Network/ClientNetworkManager.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

void ResetClock(ClientClock& clock, const NetworkTime& serverTime)
{
    clock.serverTime_ = serverTime;
    clock.lastSynchronizationFrame_ = clock.serverTime_.GetFrame();
    clock.synchronizationErrors_.clear();
    clock.synchronizationErrorsSorted_.clear();
}

bool IsClockInitializing(const ClientClock& clock)
{
    return clock.synchronizationErrors_.size() == clock.numStartSamples_;
}

bool DoesClockNeedRewind(const ClientClock& clock, double rewindThreshold)
{
    return clock.synchronizationErrors_.full() && std::abs(clock.averageError_) >= rewindThreshold;
}

double GetPingInFrames(const ClientClock& clock)
{
    // ms * (frames / s) * (s / ms)
    return clock.ping_ * clock.updateFrequency_ * 0.001;
}

double GetSmoothPingInSeconds(const ClientClock& clock)
{
    return clock.smoothPing_ * 0.001;
}

void AddClockError(ClientClock& clock, double clockError)
{
    auto& buffer = clock.synchronizationErrors_;
    auto& bufferSorted = clock.synchronizationErrorsSorted_;

    if (buffer.full())
        buffer.pop_front();
    buffer.push_back(clockError);

    bufferSorted.assign(buffer.begin(), buffer.end());
    ea::sort(bufferSorted.begin(), bufferSorted.end());

    // Skip first elements because highest pings cause lowest errors
    const auto beginIter = std::next(bufferSorted.begin(), ea::min(buffer.size() - 1, clock.numTrimmedSamples_));
    const auto endIter = bufferSorted.end();
    clock.averageError_ = ea::accumulate(beginIter, endIter, 0.0) / ea::distance(beginIter, endIter);
}

void RewindClock(ClientClock& clock, double deltaFrames)
{
    clock.serverTime_ += deltaFrames;

    for (double& error : clock.synchronizationErrors_)
        error -= deltaFrames;
    clock.averageError_ -= deltaFrames;

    clock.lastSynchronizationFrame_ = clock.serverTime_.GetFrame();
}

void UpdateSmoothPing(ClientClock& clock, float blendFactor)
{
    clock.smoothPing_ = Lerp(clock.smoothPing_, static_cast<double>(clock.ping_), blendFactor);
}

}

ClientClock::ClientClock(unsigned updateFrequency, unsigned numStartSamples, unsigned numTrimmedSamples, unsigned numOngoingSamples)
    : updateFrequency_(updateFrequency)
    , numStartSamples_(numStartSamples)
    , numTrimmedSamples_(numTrimmedSamples)
    , numOngoingSamples_(ea::max(numOngoingSamples, numStartSamples + 1))
{
    synchronizationErrors_.set_capacity(numOngoingSamples_);
    synchronizationErrorsSorted_.set_capacity(numOngoingSamples_);
}

ClientNetworkManager::ClientNetworkManager(NetworkManagerBase* base, Scene* scene, AbstractConnection* connection)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , base_(base)
    , scene_(scene)
    , connection_(connection)
{
    SubscribeToEvent(network_, E_NETWORKINPUTPROCESSED, [this](StringHash, VariantMap&)
    {
        OnInputProcessed();
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
    clock_.emplace(ClientClock{
        msg.updateFrequency_, msg.numStartClockSamples_, msg.numTrimmedClockSamples_, msg.numOngoingClockSamples_});

    clock_->latestServerFrame_ = msg.lastFrame_;
    clock_->ping_ = msg.ping_;
    clock_->smoothPing_ = clock_->ping_;

    clock_->serverTime_ = NetworkTime{clock_->latestServerFrame_ + RoundToInt(GetPingInFrames(*clock_))};
    clock_->frameDuration_ = 1.0f / clock_->updateFrequency_;
    clock_->lastSynchronizationFrame_ = clock_->serverTime_.GetFrame();

    connection_->SendSerializedMessage(
        MSG_SYNCHRONIZE_ACK, MsgSynchronizeAck{msg.magic_}, NetworkMessageFlag::Reliable);

    URHO3D_LOGINFO("Client clock is started from frame {}", clock_->serverTime_.ToString());
}

void ClientNetworkManager::ProcessClock(const MsgClock& msg)
{
    if (!clock_)
    {
        URHO3D_LOGWARNING("Client clock update received before synchronization");
        return;
    }

    const auto delta = static_cast<int>(msg.lastFrame_ - clock_->latestServerFrame_);
    clock_->ping_ = msg.ping_;
    clock_->latestServerFrame_ += ea::max(0, delta);
    ProcessTimeCorrection();
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
            URHO3D_LOGWARNING("Cannot find NetworkObject {} to remove", NetworkManagerBase::FormatNetworkId(networkId));
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

        messageData.ReadBuffer(componentBuffer_.GetBuffer());
        componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
        componentBuffer_.Seek(0);

        if (NetworkObject* networkObject = CreateNetworkObject(networkId, componentType))
            networkObject->ReadSnapshot(messageFrame, componentBuffer_);
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
        componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
        componentBuffer_.Seek(0);

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
            networkObject->ReadReliableDelta(messageFrame, componentBuffer_);
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
        componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
        componentBuffer_.Seek(0);

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
            networkObject->ReadUnreliableDelta(messageFrame, componentBuffer_);
    }
}

void ClientNetworkManager::ProcessTimeCorrection()
{
    // Accumulate between client predicted time and server time
    const NetworkTime currentServerTime = NetworkTime{clock_->latestServerFrame_} + GetPingInFrames(*clock_);
    const double clockError = currentServerTime - clock_->serverTime_;
    AddClockError(*clock_, clockError);

    // Snap to new time if error is too big
    const bool needForwardSnap = std::abs(clockError) >= settings_.clockSnapThresholdSec_ * clock_->updateFrequency_;
    const bool needRewind = DoesClockNeedRewind(*clock_, settings_.clockRewindThresholdFrames_);
    if (needForwardSnap || needRewind)
    {
        ResetClock(*clock_, currentServerTime);
        return;
    }

    // Adjust time if (re)initializing
    if (IsClockInitializing(*clock_))
    {
        const NetworkTime oldServerTime = clock_->serverTime_;
        RewindClock(*clock_, clock_->averageError_);

        URHO3D_LOGINFO("Client clock is rewound from frame {} to frame {}",
            oldServerTime.ToString(), clock_->serverTime_.ToString());
    }
}

NetworkObject* ClientNetworkManager::CreateNetworkObject(NetworkId networkId, StringHash componentType)
{
    auto networkObject = DynamicCast<NetworkObject>(context_->CreateObject(componentType));
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot create NetworkObject {} of type #{} '{}'",
            NetworkManagerBase::FormatNetworkId(networkId), componentType.Value(), componentType.Reverse());
        return nullptr;
    }
    networkObject->SetNetworkId(networkId);

    const unsigned networkIndex = NetworkManagerBase::DecomposeNetworkId(networkId).first;
    if (NetworkObject* oldNetworkObject = base_->GetNetworkObjectByIndex(networkIndex))
    {
        URHO3D_LOGWARNING("NetworkObject {} overwrites existing NetworkObject {}",
            NetworkManagerBase::FormatNetworkId(networkId),
            NetworkManagerBase::FormatNetworkId(oldNetworkObject->GetNetworkId()));
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
        URHO3D_LOGWARNING("Cannot find existing NetworkObject {}", NetworkManagerBase::FormatNetworkId(networkId));
        return nullptr;
    }

    if (networkObject->GetType() != componentType)
    {
        URHO3D_LOGWARNING("NetworkObject {} has unexpected type '{}', message was prepared for {}",
            NetworkManagerBase::FormatNetworkId(networkId), networkObject->GetTypeName(),
            componentType.ToDebugString());
        return nullptr;
    }

    return networkObject;
}

void ClientNetworkManager::RemoveNetworkObject(WeakPtr<NetworkObject> networkObject)
{
    networkObject->PrepareToRemove();
    if (networkObject)
        networkObject->Remove();
}

double ClientNetworkManager::GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const
{
    if (!clock_)
        return M_LARGE_VALUE;

    return clock_->serverTime_ - NetworkTime{referenceFrame};
}

unsigned ClientNetworkManager::GetTraceCapacity() const
{
    if (!clock_)
        return 0;

    return CeilToInt(settings_.traceDurationInSeconds_ * clock_->updateFrequency_);
}

ea::string ClientNetworkManager::ToString() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Estimated frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        GetCurrentFrame());
}

void ClientNetworkManager::OnInputProcessed()
{
    if (!clock_)
        return;

    auto time = GetSubsystem<Time>();
    const float timeStep = time->GetTimeStep();

    clock_->serverTime_ += timeStep * clock_->updateFrequency_;
    UpdateSmoothPing(*clock_, settings_.pingSmoothConstant_);

    const double interpolationDelay = settings_.sampleDelayInSeconds_ + GetSmoothPingInSeconds(*clock_);
    clock_->clientTime_ = clock_->serverTime_ - interpolationDelay * clock_->updateFrequency_;

    const auto& networkObjects = base_->GetUnorderedNetworkObjects();
    for (NetworkObject* networkObject : networkObjects)
    {
        if (networkObject)
            networkObject->InterpolateState(clock_->clientTime_);
    }
}

}
