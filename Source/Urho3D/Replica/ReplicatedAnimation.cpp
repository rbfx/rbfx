// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/ReplicatedAnimation.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Resource/ResourceCache.h"
#ifdef URHO3D_IK
    #include "Urho3D/IK/IKSolver.h"
#endif

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

    // clang-format off
    URHO3D_ATTRIBUTE("Num Upload Attempts", unsigned, numUploadAttempts_, DefaultNumUploadAttempts, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Replicate Owner", bool, replicateOwner_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Smoothing Time", float, smoothingTime_, DefaultSmoothingTime, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Layers", GetLayersAttr, SetLayersAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
    // clang-format on
}

void ReplicatedAnimation::SetLayersAttr(const VariantVector& layers)
{
    layers_.clear();
    const auto toUint = [](const Variant& value) { return value.GetUInt(); };
    ea::transform(layers.begin(), layers.end(), ea::back_inserter(layers_), toUint);
}

const VariantVector& ReplicatedAnimation::GetLayersAttr() const
{
    static thread_local VariantVector result;
    result.clear();
    ea::copy(layers_.begin(), layers_.end(), ea::back_inserter(result));
    return result;
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

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME, [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto serverFrame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameEnd(serverFrame);
    });
}

void ReplicatedAnimation::WriteSnapshot(NetworkFrame frame, Serializer& dest)
{
    if (!animationController_)
        return;

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
    animationController_ = node_->GetDerivedComponent<AnimationController>();
    if (!animationController_)
        return;

    // Update controller manually
    animationController_->SetEnabled(false);

    animatedModel_ = node_->GetDerivedComponent<AnimatedModel>();
#ifdef URHO3D_IK
    ikSolver_ = node_->GetDerivedComponent<IKSolver>();
#endif
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

bool ReplicatedAnimation::IsAnimationReplicated() const
{
    NetworkObject* networkObject = GetNetworkObject();
    return networkObject->IsReplicatedClient() || (replicateOwner_ && networkObject->IsOwnedByThisClient());
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
        if (!layers_.empty() && !layers_.contains(params.layer_))
            continue;

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

void ReplicatedAnimation::DecodeSnapshot(
    const AnimationSnapshot& snapshot, ea::vector<AnimationParameters>& result) const
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

void ReplicatedAnimation::InterpolateState(
    float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!animationController_ || !IsAnimationReplicated())
        return;

    // Subtract time step because it is will be applied during Update
    const NetworkTime adjustedReplicaTime = replicaTime - replicaTimeStep / client_.networkStepTime_;
    const auto closestPriorFrame =
        client_.animationTrace_.FindClosestAllocatedFrame(adjustedReplicaTime.Frame(), true, false);
    if (!closestPriorFrame || *closestPriorFrame == client_.latestAppliedFrame_)
        return;

    const bool firstUpdate = !client_.latestAppliedFrame_.has_value();
    client_.latestAppliedFrame_ = *closestPriorFrame;
    const AnimationSnapshot& snapshot = client_.animationTrace_.GetRawUnchecked(*closestPriorFrame);
    DecodeSnapshot(snapshot, client_.snapshotAnimations_);

    const float delay = (adjustedReplicaTime - NetworkTime{*closestPriorFrame}) * client_.networkStepTime_;
    animationController_->ReplaceAnimations(
        client_.snapshotAnimations_, delay, firstUpdate ? 0.0f : smoothingTime_, layers_);
}

void ReplicatedAnimation::PostUpdate(float replicaTimeStep, float inputTimeStep)
{
    if (!animationController_)
        return;

    const float timeStep = IsAnimationReplicated() ? replicaTimeStep : inputTimeStep;
    animationController_->Update(timeStep);

    // On server, force updates now because there may be no Viewport.
    NetworkObject* networkObject = GetNetworkObject();
    if (networkObject->IsServer())
    {
        if (animatedModel_)
            animatedModel_->ApplyAnimation();

#ifdef URHO3D_IK
        if (ikSolver_)
            ikSolver_->Solve(timeStep);
#endif
    }
}

} // namespace Urho3D
