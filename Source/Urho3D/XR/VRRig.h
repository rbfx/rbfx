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
    Node* GetLeftHand() const { return leftHandNode_; }
    Node* GetRightHand() const { return rightHandNode_; }
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
    WeakPtr<Node> leftHandNode_;
    WeakPtr<Node> rightHandNode_;
};

} // namespace Urho3D
