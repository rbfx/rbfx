// Copyright (c) 2022-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Utility/AnimationVelocityExtractor.h"

#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Math/NumericRange.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

using PositionTrack = ea::vector<Vector3>;

struct ExtractedTrackSet
{
    ea::vector<PositionTrack> tracks_;
    float sampleRate_{};
};

ExtractedTrackSet ExtractAnimationTracks(const CalculateAnimationVelocityTask& task)
{
    auto scene = MakeShared<Scene>(task.model_->GetContext());
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild();

    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(task.model_);
    animatedModel->ApplyAnimation();

    ea::vector<Node*> targetNodes;
    for (const ea::string& boneName : task.params_.targetBones_)
    {
        if (Node* boneNode = node->GetChild(boneName, true))
            targetNodes.push_back(boneNode);
    }
    ExtractedTrackSet result;
    result.tracks_.resize(targetNodes.size());

    auto animationController = node->CreateComponent<AnimationController>();
    animationController->Update(0.0f);
    animationController->PlayNew(AnimationParameters{task.animation_}.Looped());

    const float animationLength = task.animation_->GetLength();
    const float animationFrameRate = task.animation_->GetMetadata("FrameRate").GetFloat();

    const float sampleRate = task.params_.sampleRate_ != 0 //
        ? task.params_.sampleRate_
        : animationFrameRate != 0.0f //
            ? animationFrameRate
            : 30.0f;
    const float numFramesEstimate = animationLength * sampleRate;
    const unsigned numFrames = CeilToInt(numFramesEstimate - M_LARGE_EPSILON);
    for (unsigned i = 0; i < numFrames; ++i)
    {
        const float frameTime = ea::min(i / sampleRate, animationLength);
        animationController->UpdateAnimationTime(task.animation_, frameTime);
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

ea::optional<Vector3> EvaluateVelocity(
    const PositionTrack& track, float sampleRate, const CalculateAnimationVelocityParams& params)
{
    if (track.size() <= 3)
        return ea::nullopt;

    // Find range of Y values that are considered to be on the ground.
    const auto compareY = [](const Vector3& a, const Vector3& b) { return a.y_ < b.y_; };
    const float minY = ea::min_element(track.begin(), track.end(), compareY)->y_;
    const float maxY = ea::max_element(track.begin(), track.end(), compareY)->y_;
    const FloatRange groundRangeY{minY, ea::min(minY + params.groundThreshold_, maxY)};

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
    const auto longestGroupIndex = static_cast<ea::vector<unsigned>::size_type>(
        ea::max_element(groupLengths.begin(), groupLengths.end()) - groupLengths.begin());
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

} // namespace

void CalculateAnimationVelocityParams::SerializeInBlock(Archive& archive)
{
    static const CalculateAnimationVelocityParams defaults{};

    SerializeOptionalValue(archive, "targetBones", targetBones_, defaults.targetBones_);
    SerializeOptionalValue(archive, "groundThreshold", groundThreshold_, defaults.groundThreshold_);
    SerializeOptionalValue(archive, "velocityThreshold", velocityThreshold_, defaults.velocityThreshold_);
    SerializeOptionalValue(archive, "sampleRate", sampleRate_, defaults.sampleRate_);
}

void AnimationVelocityExtractor::TaskDescription::SerializeInBlock(Archive& archive)
{
    CalculateAnimationVelocityParams::SerializeInBlock(archive);

    SerializeOptionalValue(archive, "model", model_);
    SerializeOptionalValue(archive, "animation", animation_);
}

void AnimationVelocityExtractor::TransformerParams::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "tasks", tasks_);
    SerializeOptionalValue(archive, "taskTemplates", taskTemplates_);
}

AnimationVelocityExtractor::AnimationVelocityExtractor(Context* context)
    : BaseAssetPostTransformer(context)
{
}

AnimationVelocityExtractor::~AnimationVelocityExtractor()
{
}

void AnimationVelocityExtractor::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimationVelocityExtractor>(Category_Transformer);
}

bool AnimationVelocityExtractor::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();

    const auto parameters = LoadParameters<TransformerParams>(input.inputFileName_);
    const ea::string baseResourceName = GetPath(input.resourceName_);

    auto taskDescriptions = parameters.tasks_;
    for (const TaskDescription& taskTemplate : parameters.taskTemplates_)
    {
        const auto matches = GetResourcesByPattern(baseResourceName, taskTemplate.animation_);
        for (const PatternMatch& match : matches)
        {
            TaskDescription& task = taskDescriptions.emplace_back(taskTemplate);
            task.animation_ = match.fileName_;
        }
    }

    ea::vector<CalculateAnimationVelocityTask> tasks;
    for (const TaskDescription& taskDescription : taskDescriptions)
    {
        CalculateAnimationVelocityTask generateTask;
        generateTask.model_ = cache->GetTempResource<Model>(baseResourceName + taskDescription.model_);
        generateTask.animation_ = cache->GetTempResource<Animation>(baseResourceName + taskDescription.animation_);
        generateTask.params_ = taskDescription;

        if (!generateTask.model_)
        {
            URHO3D_LOGERROR("Base model '{}' is not found", taskDescription.model_);
            continue;
        }
        if (!generateTask.animation_)
        {
            URHO3D_LOGERROR("Animation '{}' is not found", taskDescription.animation_);
            continue;
        }

        tasks.push_back(generateTask);
    }

    for (const CalculateAnimationVelocityTask& task : tasks)
    {
        if (const auto velocity = CalculateVelocity(task))
        {
            task.animation_->AddMetadata("Velocity", *velocity);
            task.animation_->SaveFile(FileIdentifier{task.animation_->GetAbsoluteFileName()});
        }
        else
        {
            URHO3D_LOGERROR("Cannot calculate velocity for animation '{}'", task.animation_->GetName());
        }
    }

    return true;
}

ea::optional<Vector3> AnimationVelocityExtractor::CalculateVelocity(const CalculateAnimationVelocityTask& task) const
{
    // Don't run this transformer on animations that are too short.
    if (task.animation_->GetLength() < M_LARGE_EPSILON)
        return ea::nullopt;

    const ExtractedTrackSet trackSet = ExtractAnimationTracks(task);

    ea::vector<Vector3> velocities;
    for (const PositionTrack& track : trackSet.tracks_)
    {
        if (const auto velocity = EvaluateVelocity(track, trackSet.sampleRate_, task.params_))
            velocities.push_back(*velocity);
    }

    if (velocities.empty())
        return ea::nullopt;

    const Vector3 averageVelocity =
        ea::accumulate(velocities.begin(), velocities.end(), Vector3::ZERO) / velocities.size();

    if (averageVelocity.Length() > task.params_.velocityThreshold_)
        return averageVelocity;
    else
        return Vector3::ZERO;
}

} // namespace Urho3D
