// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/VirtualReality.h"

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Skybox.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderPipeline/StereoRenderPipeline.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/XR/VRRig.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

bool VRRigDesc::IsValid() const
{
    return scene_ && head_ && leftEye_ && rightEye_ && leftHandPose_ && rightHandPose_ && leftHandAim_ && rightHandAim_;
}

XRBinding::XRBinding(Context* context, const ea::string& name, const ea::string& localizedName, VRHand hand,
    VariantType dataType, bool isPose, bool isAimPose)
    : BaseClassName(context)
    , name_(name)
    , localizedName_(localizedName)
    , hand_(hand)
    , dataType_(dataType)
    , haptic_(dataType_ == VAR_NONE)
    , isPose_(isPose)
    , isAimPose_(isAimPose)
{
}

XRActionGroup::XRActionGroup(Context* context, const ea::string& name, const ea::string& localizedName)
    : BaseClassName(context)
    , name_(name)
    , localizedName_(localizedName)
{
}

XRBinding* XRActionGroup::FindBinding(const ea::string& name, VRHand hand) const
{
    for (XRBinding* binding : bindings_)
    {
        if (binding->GetName().comparei(name) == 0 && (hand == VRHand::None || hand == binding->GetHand()))
            return binding;
    }
    return nullptr;
}

VirtualReality::VirtualReality(Context* ctx)
    : BaseClassName(ctx)
{
}

VirtualReality::~VirtualReality()
{
}

void VirtualReality::ConnectToRig(const VRRigDesc& rig)
{
    if (!rig.IsValid())
    {
        URHO3D_LOGERROR("Invalid VR rig description");
        return;
    }

    rig_ = rig;

    if (!rig_.pipeline_)
    {
        rig_.pipeline_ = MakeShared<StereoRenderPipeline>(context_);

        if (auto sourcePipeline = rig_.scene_->GetComponent<RenderPipeline>())
            rig_.pipeline_->SetSettings(sourcePipeline->GetSettings());
    }
    if (!rig_.viewport_)
        rig_.viewport_ = new Viewport(context_, rig_.scene_, rig_.leftEye_, nullptr, rig_.pipeline_);

    rig_.viewport_->SetEye(rig_.leftEye_, 0);
    rig_.viewport_->SetEye(rig_.rightEye_, 1);
}

void VirtualReality::CreateDefaultRig()
{
    auto cache = GetSubsystem<ResourceCache>();

    defaultScene_ = MakeShared<Scene>(context_);
    defaultScene_->CreateComponent<Octree>();

    Node* skyboxNode = defaultScene_->CreateChild("Skybox");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));

    Node* rigNode = defaultScene_->CreateChild("VRRig");
    defaultRig_ = rigNode->CreateComponent<VRRig>();
}

void VirtualReality::ValidateCurrentRig()
{
    if (!rig_.IsValid())
        defaultRig_->Activate();
}

void VirtualReality::UpdateCurrentRig()
{
    URHO3D_ASSERT(GetRuntime() == VRRuntime::OpenXR, "Only OpenXR is supported at this time");

    // Skip update if we are not ready
    RenderSurface* currentSurface = currentBackBufferColor_ ? currentBackBufferColor_->GetRenderSurface() : nullptr;
    if (!rig_.IsValid() || !currentSurface)
        return;

    // Update transforms and cameras
    Node* head = rig_.head_;
    head->SetVar("PreviousTransformLocal", head->GetTransformMatrix());
    head->SetVar("PreviousTransformWorld", head->GetWorldTransform());
    head->SetTransformMatrix(GetHeadTransform());

    Node* leftEyeNode = rig_.leftEye_->GetNode();
    Node* rightEyeNode = rig_.rightEye_->GetNode();

    Camera* leftEyeCamera = rig_.leftEye_;
    Camera* rightEyeCamera = rig_.rightEye_;

    for (Camera* camera : {leftEyeCamera, rightEyeCamera})
    {
        camera->SetFov(100.0f); // junk mostly, the eye matrices will be overriden
        camera->SetNearClip(rig_.nearDistance_);
        camera->SetFarClip(rig_.farDistance_);
    }

    leftEyeCamera->SetProjection(GetProjection(VREye::Left, rig_.nearDistance_, rig_.farDistance_));
    rightEyeCamera->SetProjection(GetProjection(VREye::Right, rig_.nearDistance_, rig_.farDistance_));

    leftEyeNode->SetTransformMatrix(GetEyeLocalTransform(VREye::Left));
    rightEyeNode->SetTransformMatrix(GetEyeLocalTransform(VREye::Right));

    const float ipdAdjust = ipdCorrection_ * 0.5f * 0.001f;
    leftEyeNode->Translate({ipdAdjust, 0, 0}, TS_LOCAL);
    rightEyeNode->Translate({-ipdAdjust, 0, 0}, TS_LOCAL);

    // Connect to the current surface in the swap chain
    if (currentSurface->GetViewport(0) != rig_.viewport_)
        currentSurface->SetViewport(0, rig_.viewport_);

    rig_.viewport_->SetRect({0, 0, currentBackBufferColor_->GetWidth(), currentBackBufferColor_->GetHeight()});
    currentSurface->QueueUpdate();
}

XRBinding* VirtualReality::GetInputBinding(const ea::string& path) const
{
    if (activeActionSet_)
    {
        if (XRBinding* binding = activeActionSet_->FindBinding(path, VRHand::None))
            return binding;
    }
    return nullptr;
}

XRBinding* VirtualReality::GetInputBinding(const ea::string& path, VRHand hand) const
{
    if (activeActionSet_)
    {
        if (XRBinding* binding = activeActionSet_->FindBinding(path, hand))
            return binding;
    }
    return nullptr;
}

void VirtualReality::SetCurrentActionSet(const ea::string& setName)
{
    auto found = actionSets_.find(setName);
    if (found != actionSets_.end())
        SetCurrentActionSet(found->second);
}

void RegisterVRLibrary(Context* context)
{
    VRRig::RegisterObject(context);
}

} // namespace Urho3D
