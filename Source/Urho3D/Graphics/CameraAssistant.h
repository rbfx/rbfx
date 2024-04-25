// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Camera.h"

namespace Urho3D
{

/// %CameraAssistant component.
/// This component does job similar to "first assistant camera" in film ьaking.
/// It tracks objects in scene and adjusts camera settings to keep them in view.
class URHO3D_API CameraAssistant : public Component
{
    URHO3D_OBJECT(CameraAssistant, Component);

public:
    /// Construct.
    explicit CameraAssistant(Context* context);
    /// Destruct.
    ~CameraAssistant() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;

    /// Add an boundary scene node. It does not need any drawable components of its own.
    void AddBoundaryNode(Node* node);
    /// Remove an boundary scene node.
    void RemoveBoundaryNode(Node* node);
    /// Remove all boundary scene nodes.
    void RemoveAllBoundaryNodes();

    /// Return number of boundary nodes.
    /// @property
    unsigned GetNumBoundaryNodes() const { return boundaryNodes_.size(); }

    /// Return boundary node by index.
    /// @property{get_boundaryNodes}
    Node* GetBoundaryNode(unsigned index) const;

    /// Set node IDs attribute.
    void SetNodeIDsAttr(const VariantVector& value);

    /// Return node IDs attribute.
    const VariantVector& GetNodeIDsAttr() const;

    /// Set easing factor.
    /// @property
    void SetEasingFactor(float factor);
    /// Set minimal vertical field of view in degrees.
    /// @property
    void SetMinFov(float fov);
    /// Set minimal orthographic mode view uniform size.
    /// @property
    void SetMinOrthoSize(float orthoSize);
    /// Set padding in world space units.
    /// @property
    void SetWorldSpacePadding(float padding);

    
    /// Return easing factor.
    /// @property
    float GetEasingFactor() const { return easingFactor_; }
    /// Return minimal vertical field of view in degrees.
    /// @property
    float GetMinFov() const { return minFov_; }
    /// Return minimal orthographic mode size.
    /// @property
    float GetMinOrthoSize() const { return minOrthoSize_; }
    /// Return padding in world space units.
    /// @property
    float GetWorldSpacePadding() const { return padding_; }

protected:
    /// Update scene subscriptions.
    void UpdateSubscriptions();
    /// Handle enabled/disabled state change. Changes update event subscription.
    void OnSetEnabled() override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene);
    /// Handle node being assigned.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Update camera parameters in scene postupdate.
    void UpdateCameraParameters(VariantMap& args);

private:
    /// Ensure proper size of world transforms when nodes are added/removed. Also mark node IDs dirty.
    void UpdateNumTransforms();
    /// Update node IDs attribute from the actual nodes.
    void UpdateNodeIDs() const;

    /// Minimal field of view.
    float minFov_{0};
    /// Minimal orthographic view size.
    float minOrthoSize_{0};

    /// Boundary nodes.
    ea::vector<WeakPtr<Node>> boundaryNodes_;
    /// IDs of boundary nodes for serialization.
    mutable VariantVector nodeIDsAttr_;
    /// Whether node IDs have been set and nodes should be searched for during ApplyAttributes.
    mutable bool nodesDirty_{};
    /// Whether nodes have been manipulated by the API and node ID attribute should be refreshed.
    mutable bool nodeIDsDirty_{};
    /// Lerp easing factor.
    float easingFactor_{1.0f};
    /// Padding in world space units.
    float padding_{0.0f};
    /// Is subscriptions enabled.
    bool subscribed_{};
};

}
