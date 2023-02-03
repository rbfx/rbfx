//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Utility/AnimationVelocityExtractor.h"

#include "../Graphics/AnimatedModel.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/Octree.h"
#include "../Math/NumericRange.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>

namespace Urho3D
{

AnimationVelocityExtractor::AnimationVelocityExtractor(Context* context)
    : AssetTransformer(context)
{
}

AnimationVelocityExtractor::~AnimationVelocityExtractor()
{
}

void AnimationVelocityExtractor::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimationVelocityExtractor>(Category_Transformer);

    URHO3D_ATTRIBUTE("Target Bones", StringVector, targetBones_, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Ground Threshold", float, groundThreshold_, DefaultThreshold, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Sample Rate", float, sampleRate_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Model", ResourceRef, skeletonModel_, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
}

bool AnimationVelocityExtractor::IsApplicable(const AssetTransformerInput& input)
{
    return input.inputFileName_.ends_with(".ani", false);
}

bool AnimationVelocityExtractor::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto animation = cache->GetResource<Animation>(input.resourceName_);
    if (!animation)
        return false;

    const ea::string& modelName = GetModelName(animation);
    auto model = cache->GetResource<Model>(modelName);
    if (!model)
    {
        URHO3D_LOGERROR(
            "Model used to evaluate animation is not found. "
            "You should either specify 'Model' attribute in the transformer "
            "or add 'Model' variable to the animation metadata.");
        return false;
    }

    // Don't run this transformer on animations that are too short.
    if (animation->GetLength() < M_LARGE_EPSILON)
        return true;

    const ExtractedTrackSet trackSet = ExtractAnimationTracks(animation, model);

    ea::vector<Vector3> velocities;
    for (const PositionTrack& track : trackSet.tracks_)
    {
        if (const auto velocity = EvaluateVelocity(track, trackSet.sampleRate_))
            velocities.push_back(*velocity);
    }

    if (velocities.empty())
        return true;

    const Vector3 averageVelocity =
        ea::accumulate(velocities.begin(), velocities.end(), Vector3::ZERO) / velocities.size();

    if (averageVelocity.Length() > M_LARGE_EPSILON)
        animation->AddMetadata("Velocity", averageVelocity);
    else
        animation->AddMetadata("Velocity", Vector3::ZERO);

    animation->SaveFile(animation->GetAbsoluteFileName());
    return true;
}

ea::string AnimationVelocityExtractor::GetModelName(Animation* animation) const
{
    const ea::string& modelName = animation->GetMetadata("Model").GetString();
    if (!modelName.empty())
        return modelName;
    return skeletonModel_.name_;
}

AnimationVelocityExtractor::ExtractedTrackSet AnimationVelocityExtractor::ExtractAnimationTracks(
    Animation* animation, Model* model) const
{
    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild();

    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(model);
    animatedModel->ApplyAnimation();

    ea::vector<Node*> targetNodes;
    for (const ea::string& boneName : targetBones_)
    {
        if (Node* boneNode = node->GetChild(boneName, true))
            targetNodes.push_back(boneNode);
    }
    ExtractedTrackSet result;
    result.tracks_.resize(targetNodes.size());

    auto animationController = node->CreateComponent<AnimationController>();
    animationController->Update(0.0f);
    animationController->PlayNew(AnimationParameters{animation}.Looped());

    const float animationLength = animation->GetLength();
    const float animationFrameRate = animation->GetMetadata("FrameRate").GetFloat();

    const float sampleRate = sampleRate_ != 0 ? sampleRate_ : animationFrameRate != 0.0f ? animationFrameRate : 30.0f;
    const float numFramesEstimate = animationLength * sampleRate;
    const unsigned numFrames = CeilToInt(numFramesEstimate - M_LARGE_EPSILON);
    for (unsigned i = 0; i < numFrames; ++i)
    {
        const float frameTime = ea::min(i / sampleRate, animationLength);
        animationController->UpdateAnimationTime(animation, frameTime);
        animationController->Update(0.0f);
        animatedModel->ApplyAnimation();

        for (unsigned trackIndex = 0; trackIndex < targetNodes.size(); ++trackIndex)
        {
            Node* node = targetNodes[trackIndex];
            PositionTrack& track = result.tracks_[trackIndex];
            track.push_back(node->GetWorldPosition());
        }
    }

    result.sampleRate_ = sampleRate;
    return result;
}

ea::optional<Vector3> AnimationVelocityExtractor::EvaluateVelocity(const PositionTrack& track, float sampleRate) const
{
    if (track.size() <= 3)
        return ea::nullopt;

    // Find range of Y values that are considered to be on the ground.
    const auto compareY = [](const Vector3& a, const Vector3& b) { return a.y_ < b.y_; };
    const float minY = ea::min_element(track.begin(), track.end(), compareY)->y_;
    const float maxY = ea::max_element(track.begin(), track.end(), compareY)->y_;
    const FloatRange groundRangeY{minY, ea::min(minY + groundThreshold_, maxY)};

    // Find the frames that are on the ground.
    ea::vector<bool> isGrounded(track.size());
    for (unsigned i = 0; i < track.size(); ++i)
        isGrounded[i] = groundRangeY.ContainsInclusive(track[i].y_);

    // Find groups of consecutive frames that are on the ground.
    ea::vector<UintRange> groups;
    ea::vector<ea::optional<unsigned>> groupIndices(track.size());
    for (unsigned i = 0; i < track.size(); ++i)
    {
        if (!isGrounded[i])
            continue;

        if (i == 0 || !isGrounded[i - 1])
            groups.emplace_back(i, i);

        groupIndices[i] = groups.size() - 1;
        groups.back().second = i;
    }
    if (groups.empty())
        return ea::nullopt;

    // Since animation is considered looped, merge the first and last groups if they are on the ground.
    if (groups.size() > 1 && isGrounded.front() && isGrounded.back())
    {
        const unsigned lastGroupIndex = groups.size() - 1;
        ea::replace(groupIndices.begin(), groupIndices.end(), lastGroupIndex, 0u);

        groups.front().first = groups.back().first;
        groups.pop_back();
    }

    // Find the longest group of frames that are on the ground.
    ea::vector<unsigned> groupLengths(groups.size());
    for (unsigned i = 0; i < track.size(); ++i)
    {
        if (const auto groupIndex = groupIndices[i])
            ++groupLengths[*groupIndex];
    }
    const unsigned longestGroupIndex = ea::max_element(groupLengths.begin(), groupLengths.end()) - groupLengths.begin();
    const unsigned longestGroupLength = groupLengths[longestGroupIndex];
    if (longestGroupLength < 2)
        return ea::nullopt;

    // Find the delta position
    const Vector3& firstGroundPosition = track[groups[longestGroupIndex].first];
    const Vector3& lastGroundPosition = track[groups[longestGroupIndex].second];
    const float groundTime = (longestGroupLength - 1) / sampleRate;

    // Tracks are relative to the moving object, while we need velocity relative to the ground. Invert result.
    return -(lastGroundPosition - firstGroundPosition) / groundTime;
}

}
