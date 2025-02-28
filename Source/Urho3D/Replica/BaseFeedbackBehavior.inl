// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Core/Context.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Replica/NetworkSettingsConsts.h"

namespace Urho3D
{

template <class T>
BaseFeedbackBehavior<T>::BaseFeedbackBehavior(Context* context, NetworkCallbackFlags callbackMask)
    : NetworkBehavior(context, callbackMask)
{
}

template <class T> BaseFeedbackBehavior<T>::~BaseFeedbackBehavior()
{
}

template <class T> void BaseFeedbackBehavior<T>::InitializeStandalone()
{
    InitializeCommon();
}

template <class T> void BaseFeedbackBehavior<T>::InitializeOnServer()
{
    InitializeCommon();

    server_.input_.Resize(maxInputFrames_);

    SubscribeToEvent(E_BEGINSERVERNETWORKFRAME, [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto frame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameBegin(frame);
    });
}

template <class T>
void BaseFeedbackBehavior<T>::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    InitializeCommon();

    if (isOwned)
        client_.input_.set_capacity(maxInputFrames_);
}

template <class T>
void BaseFeedbackBehavior<T>::InterpolateState(
    float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    NetworkObject* networkObject = GetNetworkObject();
    if (networkObject->IsOwnedByThisClient())
        client_.desiredRedundancy_ = ea::max(1, FloorToInt(inputTime - replicaTime));
}

template <class T> void BaseFeedbackBehavior<T>::InitializeCommon()
{
    UnsubscribeFromEvent(E_BEGINSERVERNETWORKFRAME);

    NetworkObject* networkObject = GetNetworkObject();
    const auto replicationManager = networkObject->GetReplicationManager();

    maxInputFrames_ = replicationManager->GetSetting(NetworkSettings::MaxInputFrames).GetUInt();
    maxRedundancy_ = replicationManager->GetSetting(NetworkSettings::MaxInputRedundancy).GetUInt();
}

template <class T> bool BaseFeedbackBehavior<T>::PrepareUnreliableFeedback(NetworkFrame frame)
{
    return true;
}

template <class T> void BaseFeedbackBehavior<T>::WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest)
{
    if (client_.input_.empty() || client_.input_.back().frame_ != frame)
    {
        URHO3D_ASSERTLOG(0, "NetworkObject {}: Unexpected call to WriteUnreliableFeedback",
            ToString(GetNetworkObject()->GetNetworkId()));
        return;
    }

    const unsigned maxSize = client_.input_.size();
    const unsigned inputBufferSize = ea::min({client_.desiredRedundancy_, maxRedundancy_, maxSize});

    dest.WriteVLE(inputBufferSize);
    ea::for_each_n(client_.input_.rbegin(), inputBufferSize,
        [&](const InputFrameData& frameData) { WritePayload(frameData.payload_, dest); });
}

template <class T> void BaseFeedbackBehavior<T>::ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src)
{
    const unsigned numInputFrames = ea::min(src.ReadVLE(), maxRedundancy_);
    for (unsigned i = 0; i < numInputFrames; ++i)
    {
        InputFrameData frameData;
        frameData.frame_ = feedbackFrame - i;

        ReadPayload(frameData.payload_, src);

        if (!server_.input_.Has(frameData.frame_))
            server_.input_.Set(frameData.frame_, frameData);
    }
}

template <class T> void BaseFeedbackBehavior<T>::OnServerFrameBegin(NetworkFrame serverFrame)
{
    if (const auto frameDataAndIndex = server_.input_.GetRawOrPrior(serverFrame))
    {
        const auto& [currentInput, currentInputFrame] = *frameDataAndIndex;
        ApplyPayload(currentInput.payload_);

        if (currentInputFrame != serverFrame)
            ++server_.lostFrames_;
    }
    else
    {
        ++server_.lostFrames_;
    }
    ++server_.totalFrames_;

    static const unsigned batchSize = 100;
    if (server_.totalFrames_ >= batchSize)
    {
        const float loss = static_cast<float>(server_.lostFrames_) / server_.totalFrames_;
        server_.lostFrames_ = 0;
        server_.totalFrames_ = 0;

        const auto replicationManager = GetNetworkObject()->GetReplicationManager();
        ServerReplicator* serverReplicator = replicationManager->GetServerReplicator();
        if (AbstractConnection* ownerConnection = GetNetworkObject()->GetOwnerConnection())
            serverReplicator->ReportInputLoss(ownerConnection, loss);
    }
}

template <class T> void BaseFeedbackBehavior<T>::CreateFrameOnClient(NetworkFrame frame, const T& payload)
{
    if (!client_.input_.empty())
    {
        const NetworkFrame previousInputFrame = client_.input_.back().frame_;
        const auto numSkippedFrames = static_cast<int>(frame - previousInputFrame) - 1;
        if (numSkippedFrames > 0)
        {
            if (numSkippedFrames >= static_cast<int>(client_.input_.capacity()))
                client_.input_.clear();
            else
            {
                InputFrameData lastFrame = client_.input_.back();
                lastFrame.isLost_ = true;
                for (int i = 1; i <= numSkippedFrames; ++i)
                {
                    lastFrame.frame_ = previousInputFrame;
                    client_.input_.push_back(lastFrame);
                }
            }

            URHO3D_LOGTRACE("NetworkObject {}: skipped {} input frames on client starting from #{}",
                ToString(GetNetworkObject()->GetNetworkId()), numSkippedFrames, previousInputFrame);
        }
    }

    InputFrameData frameData;
    frameData.frame_ = frame;
    frameData.payload_ = payload;
    client_.input_.push_back(frameData);
}

template <class T> const T* BaseFeedbackBehavior<T>::FindFrameOnClient(NetworkFrame frame, bool ignoreLost) const
{
    const auto iter = ea::find_if(client_.input_.begin(), client_.input_.end(), [&](const InputFrameData& frameData)
    {//
        return (!frameData.isLost_ || !ignoreLost) && frameData.frame_ == frame;
    });
    return iter != client_.input_.end() ? &iter->payload_ : nullptr;
}

} // namespace Urho3D
