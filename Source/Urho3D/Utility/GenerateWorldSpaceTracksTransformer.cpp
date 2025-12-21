// Copyright (c) 2022-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/GenerateWorldSpaceTracksTransformer.h"

#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Utility/AnimationMetadata.h"

namespace Urho3D
{

namespace
{

struct ExtractedTrack
{
    WeakPtr<Node> node_;
    AnimationTrack track_;
    Quaternion rotationOffset_;
};

ea::vector<ExtractedTrack> GetTracks(AnimatedModel* animatedModel, const GenerateWorldSpaceTracksParams& params)
{
    Skeleton& skeleton = animatedModel->GetSkeleton();
    const unsigned numBones = skeleton.GetNumBones();

    ea::vector<ExtractedTrack> tracks;
    for (unsigned i = 0; i < numBones; ++i)
    {
        const Bone& bone = skeleton.GetBones()[i];
        if (!bone.node_ || (!params.bones_.empty() && !params.bones_.contains(bone.name_)))
            continue;

        ExtractedTrack entry;
        entry.node_ = bone.node_;
        entry.rotationOffset_ = params.deltaRotation_ ? bone.node_->GetWorldRotation() : Quaternion::IDENTITY;
        entry.track_.name_ = Format(params.targetTrackNameFormat_, bone.name_);
        entry.track_.channelMask_ = CHANNEL_POSITION;
        if (params.fillRotations_)
            entry.track_.channelMask_ |= CHANNEL_ROTATION;
        tracks.push_back(entry);
    }
    return tracks;
}

ea::vector<ExtractedTrack> GetBendTracks(AnimatedModel* animatedModel, const GenerateWorldSpaceTracksParams& params)
{
    Skeleton& skeleton = animatedModel->GetSkeleton();

    ea::vector<ExtractedTrack> tracks;
    for (const auto& [boneName, offset] : params.bendTargetOffsets_)
    {
        const Bone* bone = skeleton.GetBone(boneName);
        if (!bone || !bone->node_)
        {
            URHO3D_LOGERROR("Bone '{}' is not found for bend track", boneName);
            continue;
        }

        Node* probeNode = bone->node_->CreateChild();
        probeNode->Translate(offset, TS_WORLD);

        ExtractedTrack entry;
        entry.node_ = probeNode;
        entry.track_.name_ = Format(params.bendTargetTrackNameFormat_, bone->name_);
        entry.track_.channelMask_ = CHANNEL_POSITION;
        tracks.push_back(entry);
    }
    return tracks;
}

} // namespace

void GenerateWorldSpaceTracksParams::SerializeInBlock(Archive& archive)
{
    static const GenerateWorldSpaceTracksParams defaults{};

    SerializeOptionalValue(archive, "fillRotations", fillRotations_, defaults.fillRotations_);
    SerializeOptionalValue(archive, "deltaRotation", deltaRotation_, defaults.deltaRotation_);
    SerializeOptionalValue(archive, "sampleRate", sampleRate_, defaults.sampleRate_);

    SerializeOptionalValue(archive, "targetTrackNameFormat", targetTrackNameFormat_, defaults.targetTrackNameFormat_);
    SerializeOptionalValue(
        archive, "bendTargetTrackNameFormat", bendTargetTrackNameFormat_, defaults.bendTargetTrackNameFormat_);

    SerializeOptionalValue(archive, "bones", bones_);
    SerializeOptionalValue(archive, "bendTargetOffsets", bendTargetOffsets_);
}

void GenerateWorldSpaceTracksTransformer::TaskDescription::SerializeInBlock(Archive& archive)
{
    GenerateWorldSpaceTracksParams::SerializeInBlock(archive);

    SerializeOptionalValue(archive, "model", model_);
    SerializeOptionalValue(archive, "sourceAnimation", sourceAnimation_);
    SerializeOptionalValue(archive, "targetAnimation", targetAnimation_);
}

void GenerateWorldSpaceTracksTransformer::TransformerParams::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "tasks", tasks_);
    SerializeOptionalValue(archive, "taskTemplates", taskTemplates_);
}

GenerateWorldSpaceTracksTransformer::GenerateWorldSpaceTracksTransformer(Context* context)
    : BaseAssetPostTransformer(context)
{
}

GenerateWorldSpaceTracksTransformer::~GenerateWorldSpaceTracksTransformer()
{
}

void GenerateWorldSpaceTracksTransformer::RegisterObject(Context* context)
{
    context->RegisterFactory<GenerateWorldSpaceTracksTransformer>(Category_Transformer);
}

bool GenerateWorldSpaceTracksTransformer::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();

    const auto parameters = LoadParameters<TransformerParams>(input.inputFileName_);
    const ea::string baseResourceName = GetPath(input.resourceName_);

    auto taskDescriptions = parameters.tasks_;
    for (const TaskDescription& taskTemplate : parameters.taskTemplates_)
    {
        const auto matches = GetResourcesByPattern(baseResourceName, taskTemplate.sourceAnimation_);
        for (const PatternMatch& match : matches)
        {
            TaskDescription& task = taskDescriptions.emplace_back(taskTemplate);
            task.sourceAnimation_ = match.fileName_;
            task.targetAnimation_ = GetMatchFileName(taskTemplate.targetAnimation_, match);
        }
    }

    ea::vector<GenerateWorldSpaceTracksTask> tasks;
    for (const TaskDescription& taskDescription : taskDescriptions)
    {
        GenerateWorldSpaceTracksTask generateTask;
        generateTask.model_ = cache->GetTempResource<Model>(baseResourceName + taskDescription.model_);
        generateTask.sourceAnimation_ =
            cache->GetTempResource<Animation>(baseResourceName + taskDescription.sourceAnimation_);
        generateTask.params_ = taskDescription;

        if (taskDescription.targetAnimation_ == taskDescription.sourceAnimation_)
            generateTask.targetAnimation_ = generateTask.sourceAnimation_;
        else
        {
            const ea::string targetResourceName = baseResourceName + taskDescription.targetAnimation_;
            auto targetAnimation = generateTask.sourceAnimation_->Clone(targetResourceName);
            targetAnimation->RemoveAllTracks();
            generateTask.targetAnimation_ = targetAnimation;
        }

        if (!generateTask.model_)
        {
            URHO3D_LOGERROR("Base model '{}' is not found", taskDescription.model_);
            continue;
        }
        if (!generateTask.sourceAnimation_)
        {
            URHO3D_LOGERROR("Source animation '{}' is not found", taskDescription.sourceAnimation_);
            continue;
        }

        tasks.push_back(generateTask);
    }

    for (const GenerateWorldSpaceTracksTask& task : tasks)
    {
        GenerateTracks(task);
        task.targetAnimation_->SaveFile(FileIdentifier{input.tempPath_ + task.targetAnimation_->GetName()});
    }

    return true;
}

void GenerateWorldSpaceTracksTransformer::GenerateTracks(const GenerateWorldSpaceTracksTask& task) const
{
    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild();

    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(task.model_);
    animatedModel->ApplyAnimation();

    ea::vector<ExtractedTrack> tracks = GetTracks(animatedModel, task.params_);
    tracks.append(GetBendTracks(animatedModel, task.params_));

    auto animationController = node->CreateComponent<AnimationController>();
    animationController->Update(0.0f);
    animationController->PlayNew(AnimationParameters{task.sourceAnimation_}.Looped());

    const float animationLength = task.sourceAnimation_->GetLength();
    const float animationFrameRate = task.sourceAnimation_->GetMetadata("FrameRate").GetFloat();

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
        animationController->UpdateAnimationTime(task.sourceAnimation_, frameTime);
        animationController->Update(0.0f);
        animatedModel->ApplyAnimation();

        for (ExtractedTrack& track : tracks)
        {
            AnimationKeyFrame frame{frameTime, track.node_->GetWorldPosition()};
            if (track.track_.channelMask_.IsAnyOf(CHANNEL_ROTATION))
                frame.rotation_ = track.node_->GetWorldRotation() * track.rotationOffset_.Inverse();
            track.track_.AddKeyFrame(frame);
        }
    }

    for (const ExtractedTrack& track : tracks)
    {
        task.targetAnimation_->RemoveTrack(track.track_.name_);
        AnimationTrack* destTrack = task.targetAnimation_->CreateTrack(track.track_.name_);

        destTrack->channelMask_ = track.track_.channelMask_;
        destTrack->keyFrames_ = track.track_.keyFrames_;
    }
}

} // namespace Urho3D
