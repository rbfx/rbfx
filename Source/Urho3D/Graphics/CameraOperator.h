//
// Copyright (c) 2024-2024 the rbfx project.
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

/// \file

#pragma once

#include <Urho3D/Graphics/Camera.h>

namespace Urho3D
{
/// Helper component that tracks points in world space and updates camera position.
class URHO3D_API CameraOperator : public Component
{
    URHO3D_OBJECT(CameraOperator, Component);

    /// Construct.
    explicit CameraOperator(Context* context);
    /// Destruct.
    ~CameraOperator() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;
    
    /// Set node IDs attribute.
    void SetNodeIDsAttr(const VariantVector& value);
    /// Return node IDs attribute.
    const VariantVector& GetNodeIDsAttr() const;

    /// Get padding in world space units.
    const Rect& GetPadding() const { return padding_; }
    /// Set padding in world space units.
    void SetPadding(const Rect& padding);
    /// Set uniform padding in every direction in world space units.
    void SetUniformPadding(float padding);

    /// Get tracked bounding box.
    const BoundingBox& GetBoundingBox() const { return boundingBox_; }
    /// Set bounding box.
    void SetBoundingBox(const BoundingBox& box);

    /// Return whether bounding box tracking is enabled.
    bool IsBoundingBoxTrackingEnabled() const { return boundingBoxEnabled_; }
    /// Enable or disable bounding box tracking. Disabled by default.
    void SetBoundingBoxTrackingEnabled(bool enable);

    /// Add scene node to track.
    void TrackNode(Node* node);
    /// Remove scene node from trackable nodes.
    void RemoveTrackedNode(Node* node);
    /// Remove all instance scene nodes.
    void RemoveAllTrackedNodes();

    /// Return number of tracked nodes.
    unsigned GetNumTrackedNodes() const { return trackedNodes_.size(); }

    /// Return tracked node by index.
    Node* GetTrackedNode(unsigned index) const;

    /// Move camera.
    void MoveCamera();

protected:
    /// Update node IDs attribute from the actual nodes.
    void UpdateNodeIDs() const;
    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;

private:
    void HandleSceneDrawableUpdateFinished(StringHash eventType, VariantMap& eventData);
    void OnNodeSet(Node* previousNode, Node* currentNode);
    void FocusOn(const Vector3* begin, const Vector3* end, Camera* camera);

    /// Instance nodes.
    ea::vector<WeakPtr<Node>> trackedNodes_;
    /// IDs of instance nodes for serialization.
    mutable VariantVector nodeIDsAttr_;
    /// Whether node IDs have been set and nodes should be searched for during ApplyAttributes.
    mutable bool nodesDirty_{};
    /// Whether nodes have been manipulated by the API and node ID attribute should be refreshed.
    mutable bool nodeIDsDirty_{};
    /// Is bounding box tracking enabled.
    bool boundingBoxEnabled_{};
    /// Bounding box to track.
    BoundingBox boundingBox_{};
    /// Padding in units.
    Rect padding_{Rect::ZERO};


    /// Point cloud cache
    ea::vector<Vector3> points_;
};
}
