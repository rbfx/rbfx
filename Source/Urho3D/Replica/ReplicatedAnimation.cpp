//
// Copyright (c) 2017-2020 the rbfx project.
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
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Network/NetworkEvents.h"
#include "../Replica/ReplicatedAnimation.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D
{

ReplicatedAnimation::ReplicatedAnimation(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

ReplicatedAnimation::~ReplicatedAnimation()
{
}

void ReplicatedAnimation::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReplicatedAnimation>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);

    URHO3D_ATTRIBUTE("Num Upload Attempts", unsigned, numUploadAttempts_, DefaultNumUploadAttempts, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Replicate Owner", bool, replicateOwner_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Smoothing Time", float, smoothingTime_, DefaultSmoothingTime, AM_DEFAULT);
}

void ReplicatedAnimation::InitializeStandalone()
{
    InitializeCommon();
}

void ReplicatedAnimation::InitializeOnServer()
{
    InitializeCommon();
    if (!animationController_)
        return;

    server_.latestRevision_ = animationController_->GetRevision();

    UpdateLookupsOnServer();
    server_.newAnimationLookups_.clear();

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME,
        [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto serverFrame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameEnd(serverFrame);
    });
}

void ReplicatedAnimation::WriteSnapshot(NetworkFrame frame, Serializer& dest)
{
    dest.WriteVLE(animationLookup_.size());
    for (const auto& [nameHash, name] : animationLookup_)
        dest.WriteString(name);

    WriteSnapshot(dest);
}

void ReplicatedAnimation::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    InitializeCommon();
    if (!animationController_)
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    client_.networkStepTime_ = 1.0f / replicationManager->GetUpdateFrequency();
    client_.animationTrace_.Resize(traceDuration);
    client_.latestAppliedFrame_ = ea::nullopt;

    ReadLookupsOnClient(src);

    // Read initial animations
    const AnimationSnapshot snapshot = ReadSnapshot(src);
    client_.animationTrace_.Set(frame, snapshot);
}

void ReplicatedAnimation::InitializeCommon()
{
    animationController_ = GetComponent<AnimationController>();
    if (!animationController_)
        return;

    // Update controller manually
    animationController_->SetEnabled(false);
}

void ReplicatedAnimation::OnServerFrameEnd(NetworkFrame frame)
{
    if (!animationController_)
        return;

    if (server_.pendingUploadAttempts_ > 0)
        --server_.pendingUploadAttempts_;

    const unsigned revision = animationController_->GetRevision();
    if (server_.latestRevision_ != revision)
    {
        server_.latestRevision_ = revision;
        server_.pendingUploadAttempts_ = numUploadAttempts_;
        UpdateLookupsOnServer();
    }
    else
    {
        server_.newAnimationLookups_.clear();
    }
}

void ReplicatedAnimation::UpdateLookupsOnServer()
{
    server_.newAnimationLookups_.clear();
    for (unsigned i = 0; i < animationController_->GetNumAnimations(); ++i)
    {
        const AnimationParameters& params = animationController_->GetAnimationParameters(i);
        if (animationLookup_.find(params.GetAnimationName()) != animationLookup_.end())
            continue;

        const ea::string& name = params.GetAnimation()->GetName();
        animationLookup_[StringHash{name}] = name;
        server_.newAnimationLookups_.push_back(name);
    }
}

void ReplicatedAnimation::ReadLookupsOnClient(Deserializer& src)
{
    const unsigned numLookups = src.ReadVLE();
    for (unsigned i = 0; i < numLookups; ++i)
    {
        const ea::string name = src.ReadString();
        animationLookup_[StringHash{name}] = name;
    }
}

Animation* ReplicatedAnimation::GetAnimationByHash(StringHash nameHash) const
{
    const auto iter = animationLookup_.find(nameHash);
    if (iter == animationLookup_.end())
        return nullptr;

    return GetSubsystem<ResourceCache>()->GetResource<Animation>(iter->second);
}

void ReplicatedAnimation::WriteSnapshot(Serializer& dest)
{
    server_.snapshotBuffer_.Clear();

    const unsigned numAnimations = animationController_->GetNumAnimations();
    for (unsigned i = 0; i < numAnimations; ++i)
    {
        const AnimationParameters& params = animationController_->GetAnimationParameters(i);
        server_.snapshotBuffer_.WriteStringHash(params.GetAnimationName());
        params.Serialize(server_.snapshotBuffer_);
    }

    dest.WriteBuffer(server_.snapshotBuffer_.GetBuffer());
}

ReplicatedAnimation::AnimationSnapshot ReplicatedAnimation::ReadSnapshot(Deserializer& src) const
{
    const unsigned size = src.ReadVLE();

    AnimationSnapshot result(size);
    src.Read(result.data(), size);
    return result;
}

void ReplicatedAnimation::DecodeSnapshot(const AnimationSnapshot& snapshot, ea::vector<AnimationParameters>& result) const
{
    result.clear();
    MemoryBuffer src{snapshot.data(), snapshot.size()};
    while (!src.IsEof())
    {
        Animation* animation = GetAnimationByHash(src.ReadStringHash());
        const auto params = AnimationParameters::Deserialize(animation, src);
        if (animation)
            result.push_back(params);
    }
}

bool ReplicatedAnimation::PrepareReliableDelta(NetworkFrame frame)
{
    return !server_.newAnimationLookups_.empty();
}

void ReplicatedAnimation::WriteReliableDelta(NetworkFrame frame, Serializer& dest)
{
    dest.WriteVLE(server_.newAnimationLookups_.size());
    for (const ea::string& name : server_.newAnimationLookups_)
        dest.WriteString(name);
}

void ReplicatedAnimation::ReadReliableDelta(NetworkFrame frame, Deserializer& src)
{
    ReadLookupsOnClient(src);
    // Reset latest frame to reapply animations just in case.
    client_.latestAppliedFrame_ = ea::nullopt;
}

bool ReplicatedAnimation::PrepareUnreliableDelta(NetworkFrame frame)
{
    return animationController_ && (server_.pendingUploadAttempts_ > 0 || numUploadAttempts_ == 0);
}

void ReplicatedAnimation::WriteUnreliableDelta(NetworkFrame frame, Serializer& dest)
{
    WriteSnapshot(dest);
}

void ReplicatedAnimation::ReadUnreliableDelta(NetworkFrame frame, Deserializer& src)
{
    const auto snapshot = ReadSnapshot(src);
    client_.animationTrace_.Set(frame, snapshot);
}

void ReplicatedAnimation::InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!animationController_ || !GetNetworkObject()->IsReplicatedClient())
        return;

    // Subtract time step because it is will be applied during Update
    const NetworkTime adjustedReplicaTime = replicaTime - replicaTimeStep / client_.networkStepTime_;
    const auto closestPriorFrame = client_.animationTrace_.FindClosestAllocatedFrame(adjustedReplicaTime.Frame(), true, false);
    if (!closestPriorFrame || *closestPriorFrame == client_.latestAppliedFrame_)
        return;

    const bool firstUpdate = !client_.latestAppliedFrame_.has_value();
    client_.latestAppliedFrame_ = *closestPriorFrame;
    const AnimationSnapshot& snapshot = client_.animationTrace_.GetRawUnchecked(*closestPriorFrame);
    DecodeSnapshot(snapshot, client_.snapshotAnimations_);

    const float delay = (adjustedReplicaTime - NetworkTime{*closestPriorFrame}) * client_.networkStepTime_;
    animationController_->ReplaceAnimations(client_.snapshotAnimations_, delay, firstUpdate ? 0.0f : smoothingTime_);
}

void ReplicatedAnimation::Update(float replicaTimeStep, float inputTimeStep)
{
    if (!animationController_)
        return;

    NetworkObject* networkObject = GetNetworkObject();
    if (networkObject->IsOwnedByThisClient() && !replicateOwner_)
        animationController_->Update(inputTimeStep);
    else
        animationController_->Update(replicaTimeStep);
}

}
