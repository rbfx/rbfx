// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "AssetViewer.h"

namespace Urho3D
{

using namespace AssetViewerDetail;

void AssetViewer::UpdateCameraInput()
{
    Input* input = GetSubsystem<Input>();

    // SystemUI passes the mouse events to the engine Input (see Start). The viewer decides here who owns a mouse
    // button. A button press over the viewport item gives the button to the camera until the button is released.
    const auto updateOwner = [&](MouseButton button, bool& owned)
    {
        if (!input->GetMouseButtonDown(button))
            owned = false;
        else if (!owned && input->GetMouseButtonPress(button) && sceneHovered_)
            owned = true;
    };
    updateOwner(MOUSEB_RIGHT, rightButtonOwned_);
    updateOwner(MOUSEB_MIDDLE, middleButtonOwned_);

    const IntVector2 delta = input->GetMouseMove();
    const int wheel = sceneHovered_ ? input->GetMouseMoveWheel() : 0;
    // Free look: the right button gives the mouse look and the WASD/QE flight to the controller.
    const bool freeLook = freeLookEnabled_ && !is2DPreview_ && freeFlyController_;
    if (freeFlyController_)
        freeFlyController_->SetEnabled(freeLook && rightButtonOwned_);
    if (freeLook)
    {
        // The controller moves the camera node. Keep the orbit state in sync with the node.
        const Vector3 angles = cameraNode_->GetRotation().EulerAngles();
        pitch_ = Clamp(angles.x_, -89.0f, 89.0f);
        yaw_ = angles.y_;
        cameraTarget_ = cameraNode_->GetPosition() + cameraNode_->GetDirection() * distance_;
        if (rightButtonOwned_)
            particleAutoFramePending_ = false;
    }

    bool changed = false;
    if (!freeLook && !is2DPreview_ && rightButtonOwned_)
    {
        particleAutoFramePending_ = false;
        yaw_ += delta.x_ * 0.25f;
        pitch_ = Clamp(pitch_ + delta.y_ * 0.25f, -89.0f, 89.0f);
        changed = true;
    }
    if (middleButtonOwned_)
    {
        particleAutoFramePending_ = false;
        if (is2DPreview_)
        {
            const float scale = camera_->GetOrthoSize() / Max(1, GetSubsystem<Graphics>()->GetHeight());
            cameraTarget_.x_ -= delta.x_ * scale;
            cameraTarget_.y_ += delta.y_ * scale;
        }
        else
        {
            const Quaternion rotation(pitch_, yaw_, 0.0f);
            const float scale = distance_ * 0.0015f;
            cameraTarget_ -= rotation * Vector3::RIGHT * delta.x_ * scale;
            cameraTarget_ += rotation * Vector3::UP * delta.y_ * scale;
        }
        changed = true;
    }
    if (wheel != 0)
    {
        particleAutoFramePending_ = false;
        if (is2DPreview_)
            camera_->SetOrthoSize(Max(0.001f, camera_->GetOrthoSize() * Pow(0.82f, static_cast<float>(wheel))));
        else
            distance_ = Max(0.001f, distance_ * Pow(0.82f, static_cast<float>(wheel)));
        changed = true;
    }

    if (changed || !freeLook)
        UpdateCameraTransform();
}

void AssetViewer::UpdateCameraTransform()
{
    if (is2DPreview_)
    {
        cameraNode_->SetRotation(Quaternion::IDENTITY);
        cameraNode_->SetPosition(cameraTarget_ - Vector3::FORWARD * distance_);
        return;
    }
    const Quaternion rotation(pitch_, yaw_, 0.0f);
    cameraNode_->SetPosition(cameraTarget_ - rotation * Vector3::FORWARD * distance_);
    cameraNode_->LookAt(cameraTarget_, Vector3::UP);
}

BoundingBox AssetViewer::GetContentBoundingBox(bool ignoreLargeOutliers) const
{
    BoundingBox result;
    BoundingBox degenerateResult;
    ea::vector<BoundingBox> drawableBoxes;
    if (!contentRoot_)
        return result;

    ea::vector<Drawable*> drawables;
    contentRoot_->FindComponents<Drawable>(drawables, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (Drawable* drawable : drawables)
    {
        if (!drawable || !drawable->IsEnabledEffective() || drawable->GetType() == Skybox::GetTypeStatic()
            || environment_.IsEnvironmentNode(drawable->GetNode()))
            continue;
        const DrawableFlags flags = drawable->GetDrawableFlags();
        if (!flags.Test(DRAWABLE_GEOMETRY) && !flags.Test(DRAWABLE_GEOMETRY2D))
            continue;
        const BoundingBox& drawableBox = drawable->GetWorldBoundingBox();
        if (IsFiniteBoundingBox(drawableBox))
        {
            if (drawableBox.Size().LengthSquared() > M_EPSILON * M_EPSILON)
            {
                result.Merge(drawableBox);
                drawableBoxes.push_back(drawableBox);
            }
            else
                degenerateResult.Merge(drawableBox);
        }
    }
    if (!result.Defined())
        result = degenerateResult;
    // Ignore a tiny leading group of proxy/background meshes only when it is clearly larger than everything else.
    if (ignoreLargeOutliers && drawableBoxes.size() >= 100)
    {
        ea::sort(drawableBoxes.begin(), drawableBoxes.end(), [](const BoundingBox& lhs, const BoundingBox& rhs)
        { return lhs.Size().LengthSquared() > rhs.Size().LengthSquared(); });
        const unsigned maxIgnored = ea::min<unsigned>(16, drawableBoxes.size() / 100);
        for (unsigned i = 0; i < maxIgnored && i + 1 < drawableBoxes.size(); ++i)
        {
            const float currentSize = drawableBoxes[i].Size().Length();
            const float nextSize = drawableBoxes[i + 1].Size().Length();
            if (currentSize <= nextSize * 8.0f)
                continue;
            result.Clear();
            for (unsigned j = i + 1; j < drawableBoxes.size(); ++j)
                result.Merge(drawableBoxes[j]);
            // Vast aggregate scenes are useful to inspect at a representative upper-percentile draw scale.
            const float representativeSize = drawableBoxes[drawableBoxes.size() / 50].Size().Length() * 2.0f;
            const float resultSize = result.Size().Length();
            if (representativeSize < resultSize)
            {
                const Vector3 center = result.Center();
                const Vector3 halfSize = result.HalfSize() * (representativeSize / resultSize);
                result = BoundingBox(center - halfSize, center + halfSize);
            }
            break;
        }
    }
    for (const WeakPtr<Node>& node : animationDebugNodes_)
    {
        if (node)
            result.Merge(node->GetWorldPosition());
    }
    if (!animationDebugNodes_.empty() && result.Defined() && result.Size().Length() < 0.1f)
        result.Merge(BoundingBox(result.Center() - Vector3::ONE * 0.5f, result.Center() + Vector3::ONE * 0.5f));
    return result;
}

Animation* AssetViewer::GetSelectedAnimation() const
{
    return selectedAnimation_ >= 0 && selectedAnimation_ < animations_.size() ? animations_[selectedAnimation_]
                                                                              : nullptr;
}

ea::vector<Animation*> AssetViewer::GetSelectedAnimationGroup() const
{
    Animation* selectedAnimation = GetSelectedAnimation();
    if (!selectedAnimation)
        return {};

    const Variant& sourceAnimationIndex = selectedAnimation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
    if (sourceAnimationIndex.GetType() != VAR_INT)
        return {selectedAnimation};

    ea::vector<Animation*> result;
    for (const SharedPtr<Resource>& resource : assetResources_)
    {
        Animation* animation = dynamic_cast<Animation*>(resource.Get());
        if (!animation)
            continue;
        const Variant& candidateIndex = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
        if (candidateIndex.GetType() == VAR_INT && candidateIndex.GetInt() == sourceAnimationIndex.GetInt())
            result.push_back(animation);
    }
    return result;
}

ea::string AssetViewer::GetAnimationLabel(Animation* animation) const
{
    if (!animation->GetAnimationName().empty())
        return animation->GetAnimationName();
    if (!animation->GetName().empty())
        return GetFileName(animation->GetName());
    return "Unnamed animation";
}

bool AssetViewer::IsAnimationCompatible(Animation* animation, AnimatedModel* animatedModel) const
{
    if (!animation || !animatedModel || !animatedModel->GetModel())
        return false;

    Model* model = animatedModel->GetModel();
    const ea::string expectedModel = animation->GetMetadata(AnimationMetadata::Model).GetString();
    if (!expectedModel.empty())
    {
        const ea::string expectedFile = GetFileNameAndExtension(expectedModel).to_lower();
        const ea::string actualFile = GetFileNameAndExtension(model->GetName()).to_lower();
        if (expectedFile == actualFile
            || GetFileName(expectedModel).to_lower() == GetFileName(model->GetName()).to_lower())
            return true;
    }

    const Skeleton& skeleton = model->GetSkeleton();
    unsigned matchedTracks = 0;
    for (const auto& [nameHash, _] : animation->GetTracks())
    {
        if (skeleton.GetBoneIndex(nameHash) != M_MAX_UNSIGNED)
            ++matchedTracks;
    }
    const unsigned numTracks = animation->GetNumTracks();
    if (matchedTracks > 0 && (numTracks <= 2 || matchedTracks * 2 >= numTracks))
        return true;

    if (animatedModel->GetNumMorphs() > 0)
    {
        for (const auto& [_, track] : animation->GetVariantTracks())
        {
            if (track.name_.contains("@AnimatedModel") && track.name_.contains("/Morphs/"))
                return true;
        }
    }
    return false;
}

bool AssetViewer::CanControllerPlayAnimation(AnimationController* controller, Animation* animation) const
{
    if (!controller || !animation)
        return false;

    const bool isImportedSceneAnimation = animation->GetNumTracks() == 0 && animation->GetNumVariantTracks() > 0
        && animation->GetMetadata(AnimationMetadata::SourceAnimationIndex).GetType() == VAR_INT;
    if (isImportedSceneAnimation)
        return controller->GetNode() == contentRoot_;

    ea::vector<AnimatedModel*> models;
    controller->GetNode()->GetComponents<AnimatedModel>(models);
    if (!models.empty())
    {
        return ea::any_of(models.begin(), models.end(),
            [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); });
    }

    if (controller->GetNode() == contentRoot_
        && animation->GetMetadata(AnimationMetadata::SourceAnimationIndex).GetType() == VAR_INT)
        return false;

    controller->GetNode()->FindComponents<AnimatedModel>(
        models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    if (models.empty())
        return true;
    return ea::any_of(models.begin(), models.end(),
        [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); });
}

void AssetViewer::UpdateAnimationTimeFromController()
{
    const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
    if (animations.empty())
        return;
    for (const WeakPtr<AnimationController>& controller : animationControllers_)
    {
        if (controller)
        {
            for (Animation* animation : animations)
            {
                if (const AnimationParameters* parameters = controller->GetLastAnimationParameters(animation))
                {
                    animationTime_ = parameters->GetTime();
                    return;
                }
            }
        }
    }
}

void AssetViewer::ApplySelectedAnimation(bool playing)
{
    const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
    bool hasCompatibleController = false;
    if (animations.empty())
    {
        animationsPlaying_ = false;
        return;
    }

    for (const WeakPtr<AnimationController>& controller : animationControllers_)
    {
        if (controller)
        {
            controller->SetEnabled(true);
            controller->StopAll();
            Animation* compatibleAnimation = nullptr;
            for (Animation* animation : animations)
            {
                if (CanControllerPlayAnimation(controller, animation))
                {
                    compatibleAnimation = animation;
                    break;
                }
            }
            if (!compatibleAnimation)
            {
                controller->SetEnabled(false);
                continue;
            }
            hasCompatibleController = true;
            controller->PlayNewExclusive(AnimationParameters{compatibleAnimation}
                    .Looped(animationLooped_)
                    .Speed(animationSpeed_)
                    .Time(animationTime_));
            controller->UpdatePose();
            controller->SetEnabled(playing);
        }
    }
    animationsPlaying_ = playing && hasCompatibleController;
}

void AssetViewer::SetSelectedAnimation(int index, bool playing)
{
    if (index < 0 || index >= animations_.size())
        return;
    selectedAnimation_ = index;
    animationTime_ = 0.0f;
    ApplySelectedAnimation(playing);
}

void AssetViewer::SetAnimationsPlaying(bool playing)
{
    if (playing && !GetSelectedAnimation())
        return;
    UpdateAnimationTimeFromController();
    const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
    bool hasCompatibleController = false;
    for (const WeakPtr<AnimationController>& controller : animationControllers_)
    {
        if (controller)
        {
            const bool compatible =
                ea::any_of(animations.begin(), animations.end(), [this, controller](Animation* animation)
            { return CanControllerPlayAnimation(controller, animation); });
            hasCompatibleController |= compatible;
            controller->SetEnabled(playing && compatible);
        }
    }
    animationsPlaying_ = playing && hasCompatibleController;
}

void AssetViewer::SetAnimationTime(float time)
{
    Animation* animation = GetSelectedAnimation();
    if (!animation)
        return;
    animationTime_ = Clamp(time, 0.0f, animation->GetLength());
    const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
    for (const WeakPtr<AnimationController>& controller : animationControllers_)
    {
        bool updated = false;
        for (Animation* groupedAnimation : animations)
            updated |= controller && controller->UpdateAnimationTime(groupedAnimation, animationTime_);
        if (controller && updated)
            controller->UpdatePose();
    }
}

void AssetViewer::SetAnimationSpeed(float speed)
{
    Animation* animation = GetSelectedAnimation();
    animationSpeed_ = speed;
    if (!animation)
        return;
    const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
    for (const WeakPtr<AnimationController>& controller : animationControllers_)
    {
        if (controller)
        {
            for (Animation* groupedAnimation : animations)
                controller->UpdateAnimationSpeed(groupedAnimation, animationSpeed_);
        }
    }
}

void AssetViewer::FrameContent()
{
    const BoundingBox box = GetContentBoundingBox(true);
    if (!box.Defined())
        return;
    FrameBounds(box);
}

void AssetViewer::FrameBounds(const BoundingBox& box)
{
    if (!IsFiniteBoundingBox(box))
        return;
    BoundingBox sceneBounds = box;
    const float padding = Max(1.0f, box.Size().Length() * 0.01f);
    sceneBounds.min_ -= Vector3::ONE * padding;
    sceneBounds.max_ += Vector3::ONE * padding;
    if (Octree* octree = scene_->GetComponent<Octree>();
        octree && octree->GetRootOctant()->GetWorldBoundingBox().IsInside(sceneBounds) != INSIDE)
    {
        BoundingBox octreeBounds = octree->GetRootOctant()->GetWorldBoundingBox();
        octreeBounds.Merge(sceneBounds);
        octree->SetSize(octreeBounds, octree->GetNumLevels());
    }
    environment_.ExpandZone(sceneBounds);

    cameraTarget_ = box.Center();
    const float radius = Max(0.05f, box.Size().Length() * 0.5f);
    if (is2DPreview_)
    {
        const Vector3 size = box.Size();
        const float verticalSize = Max(size.y_, size.x_ / Max(camera_->GetAspectRatio(), M_EPSILON));
        camera_->SetOrthoSize(Max(0.01f, verticalSize * 1.25f));
        distance_ = radius * 2.0f;
    }
    else
    {
        distance_ = radius / Tan(camera_->GetFov() * 0.5f) * 1.25f;
        pitch_ = 12.0f;
        yaw_ = 180.0f;
    }
    camera_->SetNearClip(Max(0.001f, radius * 0.001f));
    camera_->SetFarClip(Max(100000.0f, distance_ + radius * 2.0f));
    if (freeFlyController_)
    {
        const float speed = Max(0.1f, radius * 0.5f);
        freeFlyController_->SetSpeed(speed);
        freeFlyController_->SetAcceleratedSpeed(speed * 5.0f);
    }
    UpdateCameraTransform();
}

void AssetViewer::UpdatePreviewProjection()
{
    bool has3DGeometry = false;
    bool has2DGeometry = !particleEmitters2D_.empty();
    if (contentRoot_)
    {
        ea::vector<Drawable*> drawables;
        contentRoot_->FindComponents<Drawable>(drawables, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        for (Drawable* drawable : drawables)
        {
            if (!drawable || !drawable->IsEnabledEffective() || drawable->GetType() == Skybox::GetTypeStatic())
                continue;
            const DrawableFlags flags = drawable->GetDrawableFlags();
            has3DGeometry |= flags.Test(DRAWABLE_GEOMETRY);
            has2DGeometry |= flags.Test(DRAWABLE_GEOMETRY2D);
        }
    }

    is2DPreview_ = has2DGeometry && !has3DGeometry;
    camera_->SetOrthographic(is2DPreview_);
    camera_->SetZoom(1.0f);
    UpdateCameraTransform();
}

void AssetViewer::BeginParticleAutoFrame()
{
    float observationTime = 0.0f;
    for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
    {
        if (emitter && emitter->GetEffect())
            observationTime = Max(observationTime, emitter->GetEffect()->GetMaxTimeToLive());
    }
    for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
    {
        if (!emitter || !emitter->GetEffect())
            continue;
        ParticleGraphEffect* effect = emitter->GetEffect();
        for (unsigned i = 0; i < effect->GetNumLayers(); ++i)
            observationTime = Max(observationTime, effect->GetLayer(i)->GetDuration());
    }
    for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
    {
        if (emitter && emitter->GetEffect())
        {
            ParticleEffect2D* effect = emitter->GetEffect();
            observationTime =
                Max(observationTime, effect->GetParticleLifeSpan() + Abs(effect->GetParticleLifespanVariance()));
        }
    }

    particleAutoFrameTimeRemaining_ = Clamp(observationTime * 0.75f, 0.25f, 2.0f);
    particleAutoFrameBounds_.Clear();
    particleAutoFramePending_ = true;
}

void AssetViewer::UpdateParticleAutoFrame(float timeStep)
{
    if (!particleAutoFramePending_)
        return;

    const BoundingBox currentBounds = GetContentBoundingBox();
    if (currentBounds.Defined())
        particleAutoFrameBounds_.Merge(currentBounds);

    particleAutoFrameTimeRemaining_ -= timeStep;
    if (particleAutoFrameBounds_.Defined() && particleAutoFrameTimeRemaining_ <= 0.0f)
    {
        FrameBounds(particleAutoFrameBounds_);
        particleAutoFramePending_ = false;
    }
}

} // namespace Urho3D
