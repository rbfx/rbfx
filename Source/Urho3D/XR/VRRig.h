// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/LogicComponent.h"

namespace Urho3D
{

class Camera;

class URHO3D_API VRRig : public LogicComponent
{
    URHO3D_OBJECT(VRRig, LogicComponent);

public:
    explicit VRRig(Context* context);
    ~VRRig() override;
    static void RegisterObject(Context* context);

    void Activate();

    /// Return internal nodes.
    /// @{
    Node* GetHead() const { return headNode_; }
    Node* GetLeftEye() const { return leftEyeNode_; }
    Node* GetRightEye() const { return rightEyeNode_; }
    Node* GetLeftHandPose() const { return leftHandPoseNode_; }
    Node* GetRightHandPose() const { return rightHandPoseNode_; }
    Node* GetLeftHandAim() const { return leftHandAimNode_; }
    Node* GetRightHandAim() const { return rightHandAimNode_; }
    Node* GetLeftHandController() const { return leftHandControllerNode_; }
    Node* GetRightHandController() const { return rightHandControllerNode_; }
    /// @}

private:
    /// Implement Component.
    /// @{
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// @}

    WeakPtr<Node> headNode_;
    WeakPtr<Node> leftEyeNode_;
    WeakPtr<Node> rightEyeNode_;
    WeakPtr<Camera> leftEyeCamera_;
    WeakPtr<Camera> rightEyeCamera_;
    WeakPtr<Node> leftHandPoseNode_;
    WeakPtr<Node> rightHandPoseNode_;
    WeakPtr<Node> leftHandAimNode_;
    WeakPtr<Node> rightHandAimNode_;
    WeakPtr<Node> leftHandControllerNode_;
    WeakPtr<Node> rightHandControllerNode_;
};

} // namespace Urho3D
