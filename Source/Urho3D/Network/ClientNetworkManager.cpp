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

void AddDeltaToFrame(unsigned& frame, float& blendFactor, double delta)
{
    const int deltaInt = static_cast<int>(delta);
    const float deltaFract = delta - deltaInt;
    frame += deltaInt;

    blendFactor += deltaFract;
    if (blendFactor < 0.0)
        --frame;
    if (blendFactor >= 1.0)
        ++frame;
}

void ResetClock(ClientClock& clock, unsigned currentFrame)
{
    clock.currentFrame_ = currentFrame;
    clock.subFrameTime_ = 0.0;

    clock.lastSynchronizationFrame_ = clock.currentFrame_;
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
    const int integerError = RoundToInt(deltaFrames);
    const float subFrameError = static_cast<float>(deltaFrames - integerError) * clock.frameDuration_;
    clock.currentFrame_ += integerError;
    clock.subFrameTime_ += subFrameError;
    if (clock.subFrameTime_ < 0.0f)
    {
        clock.subFrameTime_ += clock.frameDuration_;
        --clock.currentFrame_;
    }
    else if (clock.subFrameTime_ >= clock.frameDuration_)
    {
        clock.subFrameTime_ -= clock.frameDuration_;
        ++clock.currentFrame_;
    }

    for (double& error : clock.synchronizationErrors_)
        error -= deltaFrames;
    clock.averageError_ -= deltaFrames;

    clock.lastSynchronizationFrame_ = clock.currentFrame_;
}

void AdvanceClockTime(ClientClock& clock, double timeStep)
{
    clock.subFrameTime_ += timeStep;
    while (clock.subFrameTime_ >= clock.frameDuration_)
    {
        clock.subFrameTime_ -= clock.frameDuration_;
        ++clock.currentFrame_;
    }
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

        const bool isReliable = !!(msg.magic_ & 1u);
        connection_->SendSerializedMessage(MSG_PONG, MsgPingPong{ msg.magic_ }, isReliable ? NetworkMessageFlag::Reliable : NetworkMessageFlag::None);
        break;
    }

    case MSG_SYNCHRONIZE:
    {
        const auto msg = ReadNetworkMessage<MsgSynchronize>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        clock_.emplace(ClientClock{ msg.updateFrequency_, msg.numStartClockSamples_, msg.numTrimmedClockSamples_, msg.numOngoingClockSamples_ });

        clock_->latestServerFrame_ = msg.lastFrame_;
        clock_->ping_ = msg.ping_;
        clock_->smoothPing_ = clock_->ping_;

        clock_->currentFrame_ = clock_->latestServerFrame_ + RoundToInt(GetPingInFrames(*clock_));
        clock_->frameDuration_ = 1.0f / clock_->updateFrequency_;
        clock_->subFrameTime_ = 0.0f;
        clock_->lastSynchronizationFrame_ = clock_->currentFrame_;

        connection_->SendSerializedMessage(MSG_SYNCHRONIZE_ACK, MsgSynchronizeAck{ msg.magic_ }, NetworkMessageFlag::Reliable);
        URHO3D_LOGINFO("Client clock is started from frame #{}", clock_->currentFrame_);
        break;
    }

    case MSG_CLOCK:
    {
        const auto msg = ReadNetworkMessage<MsgClock>(messageData);
        connection_->OnMessageReceived(messageId, msg);
        if (!clock_)
        {
            URHO3D_LOGWARNING("Client clock update received before synchronization");
            break;
        }

        const auto delta = static_cast<int>(msg.lastFrame_ - clock_->latestServerFrame_);
        clock_->ping_ = msg.ping_;
        clock_->latestServerFrame_ += ea::max(0, delta);
        ProcessTimeCorrection();
        break;
    }

    case MSG_REMOVE_COMPONENTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        const unsigned timestamp = messageData.ReadUInt();
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
        break;
    }
    case MSG_UPDATE_COMPONENTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        const unsigned timestamp = messageData.ReadUInt();
        while (!messageData.IsEof())
        {
            const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
            const StringHash componentType = messageData.ReadStringHash();
            messageData.ReadBuffer(componentBuffer_.GetBuffer());
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);

            if (NetworkObject* networkObject = GetOrCreateNetworkObject(networkId, componentType))
            {
                const bool isSnapshot = componentType != StringHash{};
                if (isSnapshot)
                    networkObject->ReadSnapshot(timestamp, componentBuffer_);
                else
                    networkObject->ReadReliableDelta(timestamp, componentBuffer_);
            }
        }
        break;
    }
    case MSG_UNRELIABLE_UPDATE_COMPONENTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        const unsigned timestamp = messageData.ReadUInt();
        while (!messageData.IsEof())
        {
            const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
            messageData.ReadBuffer(componentBuffer_.GetBuffer());
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);

            if (NetworkObject* networkObject = base_->GetNetworkObject(networkId))
                networkObject->ReadUnreliableDelta(timestamp, componentBuffer_);
        }
        break;
    }

    default:
        break;
    }
}

void ClientNetworkManager::ProcessTimeCorrection()
{
    // Accumulate between client predicted time and server time
    const unsigned currentServerTime = clock_->latestServerFrame_ + RoundToInt(GetPingInFrames(*clock_));
    const double clockError = GetCurrentFrameDeltaRelativeTo(currentServerTime);
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
        const unsigned oldCurrentFrame = clock_->currentFrame_;
        const float oldSubFrameTime = clock_->subFrameTime_;

        RewindClock(*clock_, clock_->averageError_);

        URHO3D_LOGINFO("Client clock is rewound from frame #{}:{} to frame #{}:{}",
            oldCurrentFrame, oldSubFrameTime,
            clock_->currentFrame_, clock_->subFrameTime_);
    }
}

NetworkObject* ClientNetworkManager::GetOrCreateNetworkObject(NetworkId networkId, StringHash componentType)
{
    if (componentType != StringHash{})
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
                NetworkManagerBase::FormatNetworkId(networkId), NetworkManagerBase::FormatNetworkId(oldNetworkObject->GetNetworkId()));
            RemoveNetworkObject(WeakPtr<NetworkObject>(oldNetworkObject));
        }

        Node* newNode = scene_->CreateChild(EMPTY_STRING, LOCAL);
        newNode->AddComponent(networkObject, 0, LOCAL);
        return networkObject;
    }
    else
    {
        NetworkObject* networkObject = base_->GetNetworkObject(networkId);
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Cannot find existing NetworkObject {}", NetworkManagerBase::FormatNetworkId(networkId));
            return nullptr;
        }
        return networkObject;
    }
}

void ClientNetworkManager::RemoveNetworkObject(WeakPtr<NetworkObject> networkObject)
{
    networkObject->PrepareToRemove();
    if (networkObject)
        networkObject->Remove();
}

double ClientNetworkManager::GetCurrentFrameDeltaRelativeTo(double referenceFrame) const
{
    if (!clock_)
        return M_LARGE_VALUE;

    // Cast to integers to support time wrap
    const auto referenceFrameUint = static_cast<unsigned>(std::floor(std::fmod(referenceFrame, 4294967296.0)));
    const auto frameDeltaInt = static_cast<int>(referenceFrameUint - clock_->currentFrame_);
    const double referenceFrameFract = referenceFrame - referenceFrameUint;
    const double currentFrameFract = clock_->subFrameTime_ * clock_->updateFrequency_;
    return frameDeltaInt + referenceFrameFract - currentFrameFract;
}

ea::pair<unsigned, float> ClientNetworkManager::GetCurrentFrameWithOffset(double delta) const
{
    if (!clock_)
        return {};

    unsigned currentFrame = GetCurrentFrame();
    float blendFactor = GetCurrentBlendFactor();
    AddDeltaToFrame(currentFrame, blendFactor, delta);
    return {currentFrame, blendFactor};
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
    AdvanceClockTime(*clock_, timeStep);
    UpdateSmoothPing(*clock_, settings_.pingSmoothConstant_);

    const double interpolationDelay = settings_.sampleDelayInSeconds_ + GetSmoothPingInSeconds(*clock_);
    const auto [currentFrame, blendFactor] = GetCurrentFrameWithOffset(-interpolationDelay * clock_->updateFrequency_);

    const auto& networkObjects = base_->GetUnorderedNetworkObjects();
    for (NetworkObject* networkObject : networkObjects)
    {
        if (networkObject)
            networkObject->InterpolateState(currentFrame, blendFactor);
    }
}

}
