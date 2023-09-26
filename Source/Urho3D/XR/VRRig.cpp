// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/VRRig.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/XR/VirtualReality.h"

namespace Urho3D
{

VRRig::VRRig(Context* context)
    : LogicComponent(context)
{
}

VRRig::~VRRig()
{
}

void VRRig::RegisterObject(Context* context)
{
    context->AddFactoryReflection<VRRig>(Category_Logic);

    URHO3D_ACTION_STATIC_LABEL("Activate", Activate, "Use this rig for the headset display");
}

void VRRig::Activate()
{
    if (auto virtualReality = GetSubsystem<VirtualReality>())
    {
        VRRigDesc desc;
        desc.scene_ = GetScene();
        desc.head_ = headNode_;
        desc.leftEye_ = leftEyeCamera_;
        desc.rightEye_ = rightEyeCamera_;
        desc.leftHandPose_ = leftHandPoseNode_;
        desc.rightHandPose_ = rightHandPoseNode_;
        desc.leftHandAim_ = leftHandAimNode_;
        desc.rightHandAim_ = rightHandAimNode_;
        desc.leftController_ = leftHandControllerNode_;
        desc.rightController_ = rightHandControllerNode_;
        desc.nearDistance_ = 0.01f;
        desc.farDistance_ = 150.0f;
        virtualReality->ConnectToRig(desc);
    }
}

void VRRig::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (!currentNode)
        return;

    auto renderDevice = GetSubsystem<RenderDevice>();

    const auto getOrCreateTemporaryChild = [&](Node* parent, const char* name)
    {
        WeakPtr<Node> child{parent->GetChild(name)};
        if (!child)
            child = parent->CreateTemporaryChild(name);
        return child;
    };

    headNode_ = getOrCreateTemporaryChild(node_, "Head");
    leftEyeNode_ = getOrCreateTemporaryChild(headNode_, "Left_Eye");
    rightEyeNode_ = getOrCreateTemporaryChild(headNode_, "Right_Eye");
    leftHandPoseNode_ = getOrCreateTemporaryChild(node_, "Left_Hand");
    rightHandPoseNode_ = getOrCreateTemporaryChild(node_, "Right_Hand");
    leftHandAimNode_ = getOrCreateTemporaryChild(node_, "Left_Aim");
    rightHandAimNode_ = getOrCreateTemporaryChild(node_, "Right_Aim");
    leftHandControllerNode_ = getOrCreateTemporaryChild(leftHandPoseNode_, "Left_Controller");
    rightHandControllerNode_ = getOrCreateTemporaryChild(rightHandPoseNode_, "Right_Controller");

    leftEyeCamera_ = leftEyeNode_->GetOrCreateComponent<Camera>();
    rightEyeCamera_ = rightEyeNode_->GetOrCreateComponent<Camera>();

    if (renderDevice && renderDevice->GetBackend() == RenderBackend::OpenGL)
    {
        leftEyeCamera_->SetFlipVertical(true);
        rightEyeCamera_->SetFlipVertical(true);
    }
}

} // namespace Urho3D
