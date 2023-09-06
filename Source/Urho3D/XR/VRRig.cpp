// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/VRRig.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/XR/VRInterface.h"

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
    if (auto virtualReality = GetSubsystem<VRInterface>())
    {
        VRRigDesc desc;
        desc.scene_ = GetScene();
        desc.head_ = headNode_;
        desc.leftEye_ = leftEyeCamera_;
        desc.rightEye_ = rightEyeCamera_;
        desc.leftHand_ = leftHandNode_;
        desc.rightHand_ = rightHandNode_;
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
    static const ea::string leftHandName = "Left_Hand";
    static const ea::string rightHandName = "Right_Hand";

    headNode_ = node_->GetChild(headName);
    if (!headNode_)
        headNode_ = node_->CreateTemporaryChild(headName);

    leftEyeNode_ = headNode_->GetChild(leftEyeName);
    if (!leftEyeNode_)
        leftEyeNode_ = headNode_->CreateTemporaryChild(leftEyeName);

    rightEyeNode_ = headNode_->GetChild(rightEyeName);
    if (!rightEyeNode_)
        rightEyeNode_ = headNode_->CreateTemporaryChild(rightEyeName);

    leftHandNode_ = node_->GetChild(leftHandName);
    if (!leftHandNode_)
        leftHandNode_ = node_->CreateTemporaryChild(leftHandName);

    rightHandNode_ = node_->GetChild(rightHandName);
    if (!rightHandNode_)
        rightHandNode_ = node_->CreateTemporaryChild(rightHandName);

    leftEyeCamera_ = leftEyeNode_->GetOrCreateComponent<Camera>();
    rightEyeCamera_ = rightEyeNode_->GetOrCreateComponent<Camera>();

    if (renderDevice && renderDevice->GetBackend() == RenderBackend::OpenGL)
    {
        leftEyeCamera_->SetFlipVertical(true);
        rightEyeCamera_->SetFlipVertical(true);
    }
}

} // namespace Urho3D
