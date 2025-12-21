// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Utility/RetargetAnimationsTransformer.h"

#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/AnimationTrack.h"
#include "Urho3D/Graphics/Octree.h"
#ifdef URHO3D_IK
    #include "Urho3D/IK/IKChainSolver.h"
    #include "Urho3D/IK/IKSolver.h"
#endif
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Utility/AnimationMetadata.h"

namespace Urho3D
{

namespace
{

ea::unordered_map<ea::string, ea::string> InvertMap(const ea::unordered_map<ea::string, ea::string>& original)
{
    ea::unordered_map<ea::string, ea::string> result;
    for (const auto& [key, value] : original)
        result[value] = key;
    return result;
}

ea::optional<ea::string> GetMappedBoneName(
    const ea::string& boneName, const ea::unordered_map<ea::string, ea::string>& boneMapping)
{
    if (boneMapping.empty())
        return boneName;

    const auto iter = boneMapping.find(boneName);
    return iter != boneMapping.end() ? ea::make_optional(iter->second) : ea::nullopt;
}

float GetBaseScale(const Skeleton& skeleton)
{
    const unsigned boneIndex = skeleton.GetBonesOrder().front();
    return skeleton.GetBones()[boneIndex].initialPosition_.Length();
}

#ifdef URHO3D_IK
struct IKChainData
{
    IKChainSolver* solverComponent_{};
    Node* targetNode_{};
    Node* effectorNode_{};
    ea::vector<ea::pair<Node*, AnimationTrack>> joints_;
};
#endif

} // namespace

void RetargetAnimationsTransformer::TaskDescription::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "sourceModel", sourceModel_);
    SerializeOptionalValue(archive, "sourceAnimation", sourceAnimation_);
    SerializeOptionalValue(archive, "targetModel", targetModel_);
    SerializeOptionalValue(archive, "targetAnimation", targetAnimation_);
    SerializeOptionalValue(archive, "boneMapping", boneMapping_);
    SerializeOptionalValue(archive, "ikChains", ikChains_);
}

void RetargetAnimationsTransformer::TransformerParams::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "tasks", tasks_);
}

RetargetAnimationsTransformer::RetargetAnimationsTransformer(Context* context)
    : BaseAssetPostTransformer(context)
{
}

void RetargetAnimationsTransformer::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RetargetAnimationsTransformer>(Category_Transformer);
}

bool RetargetAnimationsTransformer::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();

    const auto parameters = LoadParameters<TransformerParams>(input.inputFileName_);
    const ea::string baseResourceName = GetPath(input.resourceName_);

    ea::vector<RetargetAnimationTask> tasks;
    for (const TaskDescription& taskDescription : parameters.tasks_)
    {
        RetargetAnimationTask retargetTask;
        retargetTask.sourceModel_ = cache->GetTempResource<Model>(baseResourceName + taskDescription.sourceModel_);
        retargetTask.sourceAnimation_ =
            cache->GetTempResource<Animation>(baseResourceName + taskDescription.sourceAnimation_);
        retargetTask.targetModel_ = cache->GetTempResource<Model>(baseResourceName + taskDescription.targetModel_);
        retargetTask.targetAnimationName_ = baseResourceName + taskDescription.targetAnimation_;
        retargetTask.sourceToTargetBones_ = taskDescription.boneMapping_;
        retargetTask.targetToSourceBones_ = InvertMap(taskDescription.boneMapping_);
        retargetTask.ikChains_ = taskDescription.ikChains_;

        if (!retargetTask.sourceModel_)
        {
            URHO3D_LOGERROR("Source model '{}' is not found", taskDescription.sourceModel_);
            continue;
        }
        if (!retargetTask.sourceAnimation_)
        {
            URHO3D_LOGERROR("Source animation '{}' is not found", taskDescription.sourceAnimation_);
            continue;
        }
        if (!retargetTask.targetModel_)
        {
            URHO3D_LOGERROR("Target model '{}' is not found", taskDescription.targetModel_);
            continue;
        }

        tasks.push_back(retargetTask);
    }

    for (const RetargetAnimationTask& task : tasks)
    {
        if (auto targetAnimation = RetargetAnimation(task))
            targetAnimation->SaveFile(FileIdentifier{input.tempPath_ + targetAnimation->GetName()});
    }

    return true;
}

SharedPtr<Animation> RetargetAnimationsTransformer::RetargetAnimation(const RetargetAnimationTask& task) const
{
    if (!task.sourceAnimation_ || !task.sourceModel_ || !task.targetModel_)
    {
        URHO3D_LOGERROR("Input resources are not available during retargeting");
        return nullptr;
    }

    auto targetAnimation = MakeShared<Animation>(context_);
    targetAnimation->SetLength(task.sourceAnimation_->GetLength());
    targetAnimation->SetName(task.targetAnimationName_);
    targetAnimation->SetAnimationName(task.sourceAnimation_->GetAnimationName());
    targetAnimation->CopyMetadata(*task.sourceAnimation_);

    // Prepare scene and models
    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();

    Node* sourceNode = scene->CreateChild("Source");
    Node* targetNode = scene->CreateChild("Target");

    AnimatedModel* sourceAnimatedModel = sourceNode->CreateComponent<AnimatedModel>();
    AnimatedModel* targetAnimatedModel = targetNode->CreateComponent<AnimatedModel>();
    sourceAnimatedModel->SetModel(task.sourceModel_);
    targetAnimatedModel->SetModel(task.targetModel_);

    auto sourceController = sourceNode->CreateComponent<AnimationController>();
    sourceController->PlayNew(AnimationParameters{task.sourceAnimation_});
    sourceController->SetSkeletonReset(true);
    sourceController->Update(0.0f);
    sourceAnimatedModel->ApplyAnimation();

    auto targetController = targetNode->CreateComponent<AnimationController>();
    targetController->PlayNew(AnimationParameters{targetAnimation});
    targetController->SetSkeletonReset(true);
    targetController->Update(0.0f);
    targetAnimatedModel->ApplyAnimation();

    // Deduce total scale from root bone
    const float sourceBaseScale = GetBaseScale(sourceAnimatedModel->GetSkeleton());
    const float targetBaseScale = GetBaseScale(targetAnimatedModel->GetSkeleton());
    const float positionScale = sourceBaseScale > M_LARGE_EPSILON && targetBaseScale > M_LARGE_EPSILON
        ? targetBaseScale / sourceBaseScale
        : 0.0f;

    // Retarget bones one by one
    for (const unsigned targetBoneIndex : targetAnimatedModel->GetSkeleton().GetBonesOrder())
    {
        const Bone& targetBone = targetAnimatedModel->GetSkeleton().GetBones()[targetBoneIndex];
        const auto sourceBoneName = GetMappedBoneName(targetBone.name_, task.targetToSourceBones_);
        if (!sourceBoneName)
            continue;

        const Bone* sourceBone = sourceAnimatedModel->GetSkeleton().GetBone(*sourceBoneName);
        if (!sourceBone)
            continue;

        const AnimationTrack* sourceTrack = task.sourceAnimation_->GetTrack(sourceBone->name_);
        if (!sourceTrack)
            continue;

        URHO3D_ASSERT(targetBone.node_);
        URHO3D_ASSERT(sourceBone->node_);

        const bool hasPosition = sourceTrack->channelMask_.IsAnyOf(CHANNEL_POSITION);
        const bool hasRotation = sourceTrack->channelMask_.IsAnyOf(CHANNEL_ROTATION);
        const bool hasScale = sourceTrack->channelMask_.IsAnyOf(CHANNEL_SCALE);
        const bool isRootBone = targetBone.parentIndex_ == targetBoneIndex;

        ea::vector<AnimationKeyFrame> targetKeyFrames;
        for (const AnimationKeyFrame& sourceKeyFrame : sourceTrack->keyFrames_)
        {
            sourceController->UpdateAnimationTime(task.sourceAnimation_, sourceKeyFrame.time_);
            sourceController->Update(0.0f);
            sourceAnimatedModel->ApplyAnimation();

            targetController->UpdateAnimationTime(targetAnimation, sourceKeyFrame.time_);
            targetController->Update(0.0f);
            targetAnimatedModel->ApplyAnimation();

            AnimationKeyFrame& targetKeyFrame = targetKeyFrames.emplace_back();
            targetKeyFrame.time_ = sourceKeyFrame.time_;

            const Matrix3x4 sourceParentWorld = sourceBone->node_->GetParent()->GetWorldTransform();
            const Matrix3x4 targetParentWorld = targetBone.node_->GetParent()->GetWorldTransform();
            const Matrix3x4 parentWorldDelta = sourceParentWorld.Inverse() * targetParentWorld;

            const Matrix3x4 sourceBindLocal{
                sourceBone->initialPosition_, sourceBone->initialRotation_, sourceBone->initialScale_};
            const Matrix3x4 sourceAnimationLocal{hasPosition ? sourceKeyFrame.position_ : sourceBone->initialPosition_,
                hasRotation ? sourceKeyFrame.rotation_ : sourceBone->initialRotation_,
                hasScale ? sourceKeyFrame.scale_ : sourceBone->initialScale_};
            const Matrix3x4 sourceLocalDelta = sourceAnimationLocal * sourceBindLocal.Inverse();

            const Matrix3x4 targetBindLocal{
                targetBone.initialPosition_, targetBone.initialRotation_, targetBone.initialScale_};

            const Matrix3x4 targetAnimationLocal =
                parentWorldDelta.Inverse() * sourceLocalDelta * parentWorldDelta * targetBindLocal;

            const Transform targetAnimationLocalTransform = Transform::FromMatrix3x4(targetAnimationLocal);

            // Scale position for root bones, discard position for other bones
            targetKeyFrame.position_ =
                isRootBone ? sourceKeyFrame.position_ * positionScale : targetBone.initialPosition_;

            // Rotation is properly re-targeted
            targetKeyFrame.rotation_ = targetAnimationLocalTransform.rotation_;

            // Scale is never re-targeted
            targetKeyFrame.scale_ = sourceKeyFrame.scale_;
        }

        // Create target track and populate keyframes
        AnimationTrack* targetTrack = targetAnimation->CreateTrack(targetBone.name_);
        targetTrack->channelMask_ = sourceTrack->channelMask_;
        targetTrack->keyFrames_ = std::move(targetKeyFrames);
    }

    // Resolve IK chains to stabilize animations
    if (!task.ikChains_.empty())
    {
#ifdef URHO3D_IK

        const auto ikSolver = targetNode->CreateComponent<IKSolver>();
        ikSolver->SetSolveFromOriginal(false);

        const float frameRate = targetAnimation->GetMetadata(AnimationMetadata::FrameRate).GetFloat();
        const unsigned numChains = task.ikChains_.size();

        ea::vector<IKChainData> chains;
        chains.resize(numChains);
        for (unsigned i = 0; i < numChains; ++i)
        {
            const StringVector& boneNames = task.ikChains_[i];
            if (boneNames.size() < 3)
            {
                URHO3D_LOGERROR("IK chain should have at least 3 bones");
                return nullptr;
            }

            IKChainData& chainData = chains[i];

            const ea::string chainTargetName = Format("__RetargetAnimation_IK_Target_{}__", i);
            const auto chainComponent = targetNode->CreateComponent<IKChainSolver>();
            const unsigned numBones = boneNames.size();
            const Bone* sourceEffectorBone = sourceAnimatedModel->GetSkeleton().GetBone(boneNames.back());
            if (!sourceEffectorBone)
            {
                URHO3D_LOGERROR("IK effector bone '{}' is not found in source skeleton", boneNames.back());
                return nullptr;
            }

            chainData.solverComponent_ = chainComponent;
            chainData.targetNode_ = targetNode->CreateChild(chainTargetName);
            chainData.effectorNode_ = sourceEffectorBone->node_;
            chainData.joints_.resize(numBones);

            StringVector boneNamesRemapped;
            for (unsigned j = 0; j < numBones; ++j)
            {
                const auto boneName = GetMappedBoneName(boneNames[j], task.sourceToTargetBones_);
                if (!boneName)
                {
                    URHO3D_LOGERROR(
                        "Bone '{}' cannot be used in IK chain because is missing from target skeleton", boneNames[j]);
                    return nullptr;
                }

                const Bone* bone = targetAnimatedModel->GetSkeleton().GetBone(*boneName);
                if (!bone)
                {
                    URHO3D_LOGERROR("Bone '{}' -> '{}' is not found", boneNames[j], *boneName);
                    return nullptr;
                }

                chainData.joints_[j].first = bone->node_;
                boneNamesRemapped.push_back(*boneName);
            }

            chainComponent->SetTargetName(chainTargetName);
            chainComponent->SetBoneNames(boneNamesRemapped);
        }

        // Rebuild solvers before animation
        ikSolver->Solve(0.0f);

        const unsigned numFrames = CeilToInt(targetAnimation->GetLength() * frameRate - M_LARGE_EPSILON);
        for (unsigned i = 0; i < numFrames; ++i)
        {
            const float frameTime = ea::min(i / frameRate, targetAnimation->GetLength());

            sourceController->UpdateAnimationTime(task.sourceAnimation_, frameTime);
            sourceController->Update(0.0f);
            sourceAnimatedModel->ApplyAnimation();

            for (const IKChainData& chainData : chains)
                chainData.targetNode_->SetWorldPosition(chainData.effectorNode_->GetWorldPosition() * positionScale);

            targetController->UpdateAnimationTime(targetAnimation, frameTime);
            targetController->Update(0.0f);
            targetAnimatedModel->ApplyAnimation();
            ikSolver->Solve(0.0f);

            for (IKChainData& chainData : chains)
            {
                for (auto& [node, track] : chainData.joints_)
                {
                    AnimationKeyFrame& keyFrame = track.keyFrames_.emplace_back();
                    keyFrame.time_ = frameTime;
                    keyFrame.rotation_ = node->GetRotation();
                }
            }
        }

        // Replace tracks in target animation with IK-created rotation tracks (KISS)
        for (IKChainData& chainData : chains)
        {
            for (auto& [node, ikTrack] : chainData.joints_)
            {
                const ea::string boneName = node->GetName();
                AnimationTrack* targetTrack = targetAnimation->GetTrack(boneName);
                if (!targetTrack)
                    targetTrack = targetAnimation->CreateTrack(boneName);

                // IK solver only provides rotation keyframes for now.
                targetTrack->channelMask_ = CHANNEL_ROTATION;
                targetTrack->keyFrames_ = std::move(ikTrack.keyFrames_);
            }
        }
#else
        URHO3D_LOGERROR("IK library is disabled, cannot preserve IK chains");
#endif
    }

    return targetAnimation;
}

} // namespace Urho3D
