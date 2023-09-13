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

    static const ea::string headName = "Head";
    static const ea::string leftEyeName = "Left_Eye";
    static const ea::string rightEyeName = "Right_Eye";
    static const ea::string leftHandPoseName = "Left_Hand";
    static const ea::string rightHandPoseName = "Right_Hand";
    static const ea::string leftHandAimName = "Left_Aim";
    static const ea::string rightHandAimName = "Right_Aim";

    headNode_ = node_->GetChild(headName);
    if (!headNode_)
        headNode_ = node_->CreateTemporaryChild(headName);

    leftEyeNode_ = headNode_->GetChild(leftEyeName);
    if (!leftEyeNode_)
        leftEyeNode_ = headNode_->CreateTemporaryChild(leftEyeName);

    rightEyeNode_ = headNode_->GetChild(rightEyeName);
    if (!rightEyeNode_)
        rightEyeNode_ = headNode_->CreateTemporaryChild(rightEyeName);

    leftHandPoseNode_ = node_->GetChild(leftHandPoseName);
    if (!leftHandPoseNode_)
        leftHandPoseNode_ = node_->CreateTemporaryChild(leftHandPoseName);

    rightHandPoseNode_ = node_->GetChild(rightHandPoseName);
    if (!rightHandPoseNode_)
        rightHandPoseNode_ = node_->CreateTemporaryChild(rightHandPoseName);

    leftHandAimNode_ = node_->GetChild(leftHandAimName);
    if (!leftHandAimNode_)
        leftHandAimNode_ = node_->CreateTemporaryChild(leftHandAimName);

    rightHandAimNode_ = node_->GetChild(rightHandAimName);
    if (!rightHandAimNode_)
        rightHandAimNode_ = node_->CreateTemporaryChild(rightHandAimName);

    leftEyeCamera_ = leftEyeNode_->GetOrCreateComponent<Camera>();
    rightEyeCamera_ = rightEyeNode_->GetOrCreateComponent<Camera>();

    if (renderDevice && renderDevice->GetBackend() == RenderBackend::OpenGL)
    {
        leftEyeCamera_->SetFlipVertical(true);
        rightEyeCamera_->SetFlipVertical(true);
    }
}

} // namespace Urho3D
