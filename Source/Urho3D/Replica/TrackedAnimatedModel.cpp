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
    animatedModel_ = GetComponent<AnimatedModel>();
    if (!animatedModel_)
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();
    transformTrace_.Resize(traceDuration);
    boundingBoxTrace_.Resize(traceDuration);

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME,
        [this](StringHash, VariantMap& eventData)
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

    if (bonePositionsTrace_.Size() != numBones)
    {
        const auto replicationManager = GetNetworkObject()->GetReplicationManager();
        const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

        bonePositionsTrace_.Resize(numBones, traceDuration);
        boneRotationsTrace_.Resize(numBones, traceDuration);
    }

    transformTrace_.Set(frame, node_->GetWorldTransform());
    boundingBoxTrace_.Set(frame, animatedModel_->GetWorldBoundingBox());

    const auto positions = bonePositionsTrace_.SetUninitialized(frame);
    const auto rotations = boneRotationsTrace_.SetUninitialized(frame);

    for (unsigned i = 0; i < numBones; ++i)
    {
        if (Node* node = bones[i].node_)
        {
            positions[i] = node->GetWorldPosition();
            rotations[i] = node->GetWorldRotation();
        }
        else
        {
            positions[i] = Vector3::ZERO;
            rotations[i] = Quaternion::IDENTITY;
        }
    }
}

Vector3 TrackedAnimatedModel::SampleTemporalBonePosition(const NetworkTime& time, unsigned index) const
{
    const auto result = bonePositionsTrace_.SampleValid(time);
    return index < result.Size() ? result[index] : Vector3::ZERO;
}

Quaternion TrackedAnimatedModel::SampleTemporalBoneRotation(const NetworkTime& time, unsigned index) const
{
    const auto result = boneRotationsTrace_.SampleValid(time);
    return index < result.Size() ? result[index] : Quaternion::IDENTITY;
}

void TrackedAnimatedModel::ProcessTemporalRayQuery(const NetworkTime& time, const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) const
{
    if (!animatedModel_)
        return;

    const auto& bones = animatedModel_->GetSkeleton().GetBones();
    const unsigned numBones = bones.size();
    if (numBones != bonePositionsTrace_.Size() || numBones != boneRotationsTrace_.Size())
        return;

    const BoundingBox worldBoundingBox = boundingBoxTrace_.GetClosestRaw(time.Frame());
    const Matrix3x4 worldTransform = transformTrace_.SampleValid(time);
    const auto bonePositions = bonePositionsTrace_.SampleValid(time);
    const auto boneRotations = boneRotationsTrace_.SampleValid(time);

    thread_local ea::vector<Matrix3x4> boneTransformsStorage;
    auto& boneTransforms = boneTransformsStorage;
    auto boneTransforms11 = boneTransformsStorage;
    boneTransforms.resize(numBones);
    for (unsigned i = 0; i < numBones; ++i)
    {
        const Vector3 position = bonePositions[i];
        const Quaternion rotation = boneRotations[i];
        const Vector3 scale = bones[i].node_ ? bones[i].node_->GetWorldScale() : Vector3::ONE;
        boneTransforms[i] = Matrix3x4{position, rotation, scale};
    }

    animatedModel_->ProcessCustomRayQuery(query, worldBoundingBox, worldTransform, boneTransforms, results);
}

}
