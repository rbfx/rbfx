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

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKTargetExtractor.h"

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
    AnimationTrack* track_{};
    Quaternion rotationOffset_;
};

ea::vector<ExtractedTrack> GetTracks(AnimatedModel* animatedModel, Animation* destAnimation, bool includeRotations)
{
    Skeleton& skeleton = animatedModel->GetSkeleton();
    const unsigned numBones = skeleton.GetNumBones();

    ea::vector<ExtractedTrack> tracks;
    for (unsigned i = 0; i < numBones; ++i)
    {
        const Bone& bone = skeleton.GetBones()[i];
        if (!bone.node_)
            continue;

        const ea::string trackName = bone.name_ + "_Target";
        AnimationTrack* track = destAnimation->GetTrack(trackName);
        if (!track)
            track = destAnimation->CreateTrack(trackName);
        else
        {
            // Skip tracks on name collision
            if (skeleton.GetBone(trackName) != nullptr)
                continue;

            track->RemoveAllKeyFrames();
        }

        ExtractedTrack entry;
        entry.node_ = bone.node_;
        entry.rotationOffset_ = bone.node_->GetWorldRotation();
        entry.track_ = track;
        entry.track_->channelMask_ = CHANNEL_POSITION;
        if (includeRotations)
            entry.track_->channelMask_ |= CHANNEL_ROTATION;
        tracks.push_back(entry);
    }
    return tracks;
}

ea::vector<ExtractedTrack> GetBendTracks(
    AnimatedModel* animatedModel, Animation* destAnimation, const StringVariantMap& offsets)
{
    Skeleton& skeleton = animatedModel->GetSkeleton();

    ea::vector<ExtractedTrack> tracks;
    for (const auto& [boneName, offsetVar] : offsets)
    {
        const Bone* bone = skeleton.GetBone(boneName);
        if (!bone || !bone->node_)
        {
            URHO3D_LOGERROR("Bone '{}' is not found for bend track", boneName);
            continue;
        }

        const ea::string trackName = boneName + "_BendTarget";
        AnimationTrack* track = destAnimation->GetTrack(trackName);
        if (!track)
            track = destAnimation->CreateTrack(trackName);
        else
        {
            // Skip tracks on name collision
            if (skeleton.GetBone(trackName) != nullptr)
                continue;

            track->RemoveAllKeyFrames();
        }

        Node* probeNode = bone->node_->CreateChild();
        probeNode->Translate(offsetVar.GetVector3(), TS_WORLD);

        ExtractedTrack entry;
        entry.node_ = probeNode;
        entry.track_ = track;
        entry.track_->channelMask_ = CHANNEL_POSITION;
        tracks.push_back(entry);
    }
    return tracks;
}

} // namespace

const ea::string IKTargetExtractor::DefaultNewFileName = "*_Targets.ani";

IKTargetExtractor::IKTargetExtractor(Context* context)
    : AssetTransformer(context)
{
}

IKTargetExtractor::~IKTargetExtractor()
{
}

void IKTargetExtractor::RegisterObject(Context* context)
{
    context->RegisterFactory<IKTargetExtractor>(Category_Transformer);

    URHO3D_ATTRIBUTE("Extract Rotations", bool, extractRotations_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Sample Rate", float, sampleRate_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Extract to Existing File", bool, extractToExistingFile_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Extract to New File", bool, extractToNewFile_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("New File Name", ea::string, newFileName_, DefaultNewFileName, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Model", ResourceRef, skeletonModel_, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Targets", StringVariantMap, bendTargets_, Variant::emptyStringVariantMap, AM_DEFAULT);
}

bool IKTargetExtractor::IsApplicable(const AssetTransformerInput& input)
{
    return input.inputFileName_.ends_with(".ani", false);
}

bool IKTargetExtractor::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();
    SharedPtr<Animation> sourceAnimation{cache->GetResource<Animation>(input.resourceName_)};
    if (!sourceAnimation)
        return false;

    // Copy to avoid modifying currently used animation
    sourceAnimation = sourceAnimation->Clone(sourceAnimation->GetName());

    const ea::string& modelName = GetModelName(sourceAnimation);
    auto model = cache->GetResource<Model>(modelName);
    if (!model)
    {
        URHO3D_LOGERROR(
            "Model used to evaluate animation is not found. "
            "You should either specify 'Model' attribute in the transformer "
            "or add 'Model' variable to the animation metadata.");
        return false;
    }

    if (extractToNewFile_)
    {
        if (newFileName_.empty() || newFileName_ == "*")
        {
            URHO3D_LOGERROR("New file name should not be empty or identical to existing file name");
            return false;
        }

        if (ea::count(newFileName_.begin(), newFileName_.end(), '*') > 1)
        {
            URHO3D_LOGERROR("New file name must contain at most one '*' character");
            return false;
        }

        auto targetAnimation = sourceAnimation->Clone(GetNewFileName(sourceAnimation->GetName()));
        targetAnimation->SetAbsoluteFileName(GetNewFileName(input.tempPath_ + sourceAnimation->GetName()));
        targetAnimation->RemoveAllTracks();

        ExtractAnimation(sourceAnimation, targetAnimation, model);

        if (!targetAnimation->SaveFile(targetAnimation->GetAbsoluteFileName()))
            return false;
    }

    if (extractToExistingFile_)
    {
        ExtractAnimation(sourceAnimation, sourceAnimation, model);
        if (!sourceAnimation->SaveFile(sourceAnimation->GetAbsoluteFileName()))
            return false;
        output.sourceModified_ = true;
    }

    return true;
}

ea::string IKTargetExtractor::GetNewFileName(const ea::string& fileName) const
{
    ea::string path, file, extension;
    SplitPath(fileName, path, file, extension);

    const ea::string newFile = newFileName_.replaced("*", file);
    return path + newFile;
}

ea::string IKTargetExtractor::GetModelName(Animation* sourceAnimation) const
{
    const ea::string& modelName = sourceAnimation->GetMetadata(AnimationMetadata::Model).GetString();
    if (!modelName.empty())
        return modelName;
    return skeletonModel_.name_;
}

void IKTargetExtractor::ExtractAnimation(Animation* sourceAnimation, Animation* destAnimation, Model* model) const
{
    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild();

    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(model);
    animatedModel->ApplyAnimation();

    ea::vector<ExtractedTrack> tracks = GetTracks(animatedModel, destAnimation, extractRotations_);
    tracks.append(GetBendTracks(animatedModel, destAnimation, bendTargets_));

    const Variant& whitelistTracksVar = sourceAnimation->GetMetadata(AnimationMetadata::IKTargetTracks);
    if (!whitelistTracksVar.IsEmpty())
    {
        const StringVector& whitelistTracks = whitelistTracksVar.GetStringVector();
        ea::erase_if(
            tracks, [&](const ExtractedTrack& track) { return !whitelistTracks.contains(track.track_->name_); });
    }

    auto animationController = node->CreateComponent<AnimationController>();
    animationController->Update(0.0f);
    animationController->PlayNew(AnimationParameters{sourceAnimation}.Looped());

    const float animationLength = sourceAnimation->GetLength();
    const float animationFrameRate = sourceAnimation->GetMetadata("FrameRate").GetFloat();

    const float sampleRate = sampleRate_ != 0 ? sampleRate_ : animationFrameRate != 0.0f ? animationFrameRate : 30.0f;
    const float numFramesEstimate = animationLength * sampleRate;
    const unsigned numFrames = CeilToInt(numFramesEstimate - M_LARGE_EPSILON);
    for (unsigned i = 0; i < numFrames; ++i)
    {
        const float frameTime = ea::min(i / sampleRate, animationLength);
        animationController->UpdateAnimationTime(sourceAnimation, frameTime);
        animationController->Update(0.0f);
        animatedModel->ApplyAnimation();

        for (ExtractedTrack& track : tracks)
        {
            AnimationKeyFrame frame{frameTime, track.node_->GetWorldPosition()};
            if (extractRotations_)
                frame.rotation_ = track.node_->GetWorldRotation() * track.rotationOffset_.Inverse();
            track.track_->AddKeyFrame(frame);
        }
    }
}

} // namespace Urho3D
