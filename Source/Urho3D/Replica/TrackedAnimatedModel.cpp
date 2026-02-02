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
#include "../Graphics/AnimatedModel.h"
#include "../Network/NetworkEvents.h"
#include "../Replica/TrackedAnimatedModel.h"
#include "../Replica/NetworkSettingsConsts.h"

namespace Urho3D
{

TrackedAnimatedModel::TrackedAnimatedModel(Context* context)
    : NetworkBehavior(context, NetworkCallbackMask::None)
{
}

TrackedAnimatedModel::~TrackedAnimatedModel()
{
}

void TrackedAnimatedModel::RegisterObject(Context* context)
{
    context->AddFactoryReflection<TrackedAnimatedModel>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);

    URHO3D_ATTRIBUTE("Track On Client", bool, trackOnClient_, false, AM_DEFAULT);
}

void TrackedAnimatedModel::InitializeOnServer()
{
    animatedModel_ = node_->GetDerivedComponent<AnimatedModel>();
    if (!animatedModel_)
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();
    nodeTransformTrace_.Resize(traceDuration);
    boundingBoxTrace_.Resize(traceDuration);

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME,
        [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto serverFrame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameEnd(serverFrame);
    });
}

void TrackedAnimatedModel::OnServerFrameEnd(NetworkFrame frame)
{
    if (!animatedModel_)
        return;

    animatedModel_->ApplyAnimation();

    const auto& bones = animatedModel_->GetSkeleton().GetBones();
    const unsigned numBones = bones.size();

    if (boneTransformsTrace_.Size() != numBones)
    {
        const auto replicationManager = GetNetworkObject()->GetReplicationManager();
        const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

        boneTransformsTrace_.Resize(numBones, traceDuration);
    }

    Scene* scene = GetScene();

    const auto nodeTransform = Transform::FromMatrix3x4(node_->GetWorldTransform());

    BoundingBox boundingBox = animatedModel_->GetWorldBoundingBox();
    boundingBox.min_ -= nodeTransform.position_;
    boundingBox.max_ -= nodeTransform.position_;

    nodeTransformTrace_.Set(frame, scene->ToAbsoluteWorldTransform(nodeTransform));
    boundingBoxTrace_.Set(frame, boundingBox);

    const auto transforms = boneTransformsTrace_.SetUninitialized(frame);
    for (unsigned i = 0; i < numBones; ++i)
    {
        Transform boneTransform;
        if (Node* node = bones[i].node_)
        {
            boneTransform.position_ = node->GetWorldPosition();
            boneTransform.rotation_ = node->GetWorldRotation();
            boneTransform.scale_ = node->GetWorldScale();
        }
        transforms[i] = scene->ToAbsoluteWorldTransform(boneTransform);
    }
}

DoubleVector3 TrackedAnimatedModel::SampleTemporalBonePosition(const NetworkTime& time, unsigned index) const
{
    const auto result = boneTransformsTrace_.SampleValid(time);
    return index < result.Size() ? result[index].position_ : DoubleVector3::ZERO;
}

Quaternion TrackedAnimatedModel::SampleTemporalBoneRotation(const NetworkTime& time, unsigned index) const
{
    const auto result = boneTransformsTrace_.SampleValid(time);
    return index < result.Size() ? result[index].rotation_ : Quaternion::IDENTITY;
}

void TrackedAnimatedModel::ProcessTemporalRayQuery(const NetworkTime& time, const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) const
{
    if (!animatedModel_)
        return;

    const auto& bones = animatedModel_->GetSkeleton().GetBones();
    const unsigned numBones = bones.size();
    if (numBones != boneTransformsTrace_.Size())
        return;

    Scene* scene = GetScene();
    const Transform nodeTransform = scene->ToRelativeWorldTransform(nodeTransformTrace_.SampleValid(time));
    const Matrix3x4 worldTransform = nodeTransform.ToMatrix3x4();

    BoundingBox worldBoundingBox = boundingBoxTrace_.GetClosestRaw(time.Frame());
    worldBoundingBox.min_ += nodeTransform.position_;
    worldBoundingBox.max_ += nodeTransform.position_;

    const auto globalBoneTransforms = boneTransformsTrace_.SampleValid(time);

    thread_local ea::vector<Matrix3x4> boneTransformsStorage;
    auto& boneTransforms = boneTransformsStorage;
    boneTransforms.resize(numBones);
    for (unsigned i = 0; i < numBones; ++i)
        boneTransforms[i] = scene->ToRelativeWorldTransform(globalBoneTransforms[i]).ToMatrix3x4();

    animatedModel_->ProcessCustomRayQuery(query, worldBoundingBox, worldTransform, boneTransforms, results);
}

}
