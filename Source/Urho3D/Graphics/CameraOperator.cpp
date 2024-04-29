//
// Copyright (c) 2024-2024 the Urho3D project.
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

#include "../Precompiled.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/CameraOperator.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "../DebugNew.h"

namespace Urho3D
{

CameraOperator::CameraOperator(Context* context)
    : Component(context)
    , boundingBox_(-Vector3::ONE, Vector3::ONE)
{
}

CameraOperator::~CameraOperator() = default;

void CameraOperator::RegisterObject(Context* context)
{
    context->AddFactoryReflection<CameraOperator>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Track Bounding Box", bool, boundingBoxEnabled_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bounding Box Min", Vector3, boundingBox_.min_, -Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bounding Box Max", Vector3, boundingBox_.max_, Vector3::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Nodes To Track", GetNodeIDsAttr, SetNodeIDsAttr, VariantVector,
        Variant::emptyVariantVector, AM_DEFAULT | AM_NODEIDVECTOR);
    URHO3D_ACTION_STATIC_LABEL("Update Camera", MoveCamera, "Move camera to keep tracked nodes and/or bounding box in frustum");
}

void CameraOperator::ApplyAttributes()
{
    if (!nodesDirty_)
        return;

    // Remove all old instance nodes before searching for new
    trackedNodes_.clear();

    const Scene* scene = GetScene();
    if (scene)
    {
        // The first index stores the number of IDs redundantly. This is for editing
        for (unsigned i = 1; i < nodeIDsAttr_.size(); ++i)
        {
            Node* node = scene->GetNode(nodeIDsAttr_[i].GetUInt());
            if (node)
            {
                WeakPtr<Node> instanceWeak(node);
                trackedNodes_.push_back(instanceWeak);
            }
        }
    }

    nodesDirty_ = false;

    OnMarkedDirty(GetNode());
}


void CameraOperator::SetNodeIDsAttr(const VariantVector& value)
{
    // Just remember the node IDs. They need to go through the SceneResolver, and we actually find the nodes during
    // ApplyAttributes()
    if (value.size())
    {
        nodeIDsAttr_.clear();

        unsigned index = 0;
        unsigned numInstances = value[index++].GetUInt();
        // Prevent crash on entering negative value in the editor
        if (numInstances > M_MAX_INT)
            numInstances = 0;

        nodeIDsAttr_.push_back(numInstances);
        while (numInstances--)
        {
            // If vector contains less IDs than should, fill the rest with zeroes
            if (index < value.size())
                nodeIDsAttr_.push_back(value[index++].GetUInt());
            else
                nodeIDsAttr_.push_back(0);
        }
    }
    else
    {
        nodeIDsAttr_.clear();
        nodeIDsAttr_.push_back(0);
    }

    nodesDirty_ = true;
    nodeIDsDirty_ = false;
}

const VariantVector& CameraOperator::GetNodeIDsAttr() const
{
    if (nodeIDsDirty_)
        UpdateNodeIDs();

    return nodeIDsAttr_;
}

void CameraOperator::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
}


void CameraOperator::SetBoundingBoxTrackingEnabled(bool enable)
{
    boundingBoxEnabled_ = enable;
}

void CameraOperator::TrackNode(Node* node)
{
    if (!node)
        return;

    const WeakPtr<Node> instanceWeak(node);
    if (trackedNodes_.contains(instanceWeak))
        return;

    trackedNodes_.push_back(instanceWeak);
}

void CameraOperator::RemoveTrackedNode(Node* node)
{
    if (!node)
        return;

    const WeakPtr<Node> instanceWeak(node);
    auto i = trackedNodes_.find(instanceWeak);
    if (i == trackedNodes_.end())
        return;

    trackedNodes_.erase(i);
}

void CameraOperator::RemoveAllTrackedNodes()
{
    trackedNodes_.clear();
    nodeIDsDirty_ = true;
}

Node* CameraOperator::GetTrackedNode(unsigned index) const
{
    return index < trackedNodes_.size() ? trackedNodes_[index].Get() : nullptr;
}

void CameraOperator::UpdateNodeIDs() const
{
    const unsigned numInstances = trackedNodes_.size();

    nodeIDsAttr_.clear();
    nodeIDsAttr_.push_back(numInstances);

    for (unsigned i = 0; i < numInstances; ++i)
    {
        Node* node = trackedNodes_[i];
        nodeIDsAttr_.push_back(node ? node->GetID() : 0);
    }

    nodeIDsDirty_ = false;
}

void CameraOperator::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        SubscribeToEvent(scene, E_SCENEDRAWABLEUPDATEFINISHED, &CameraOperator::HandleSceneDrawableUpdateFinished);
    }
    else
    {
        UnsubscribeFromEvent(E_SCENEDRAWABLEUPDATEFINISHED);
    }
}

void CameraOperator::MoveCamera()
{

    if (!node_)
        return;

    Camera* camera = node_->GetComponent<Camera>();

    if (!camera)
        return;

    points_.clear();
    if (boundingBoxEnabled_)
    {
        points_.push_back(Vector3{boundingBox_.min_.x_, boundingBox_.min_.y_, boundingBox_.min_.z_});
        points_.push_back(Vector3{boundingBox_.max_.x_, boundingBox_.min_.y_, boundingBox_.min_.z_});
        points_.push_back(Vector3{boundingBox_.min_.x_, boundingBox_.max_.y_, boundingBox_.min_.z_});
        points_.push_back(Vector3{boundingBox_.max_.x_, boundingBox_.max_.y_, boundingBox_.min_.z_});
        points_.push_back(Vector3{boundingBox_.min_.x_, boundingBox_.min_.y_, boundingBox_.max_.z_});
        points_.push_back(Vector3{boundingBox_.max_.x_, boundingBox_.min_.y_, boundingBox_.max_.z_});
        points_.push_back(Vector3{boundingBox_.min_.x_, boundingBox_.max_.y_, boundingBox_.max_.z_});
        points_.push_back(Vector3{boundingBox_.max_.x_, boundingBox_.max_.y_, boundingBox_.max_.z_});
    }

    if (nodesDirty_)
    {
        ApplyAttributes();
    }

    for (const auto& node : trackedNodes_)
    {
        if (!node.Expired())
        {
            points_.push_back(node->GetWorldPosition());
        }
    }
    if (!points_.empty())
    {
        FocusOn(points_.begin(), points_.end(), camera);
    }
}


void CameraOperator::HandleSceneDrawableUpdateFinished(StringHash eventType, VariantMap& eventData)
{
    if (!IsEnabledEffective())
        return;

    MoveCamera();
}


void CameraOperator::FocusOn(const Vector3* begin, const Vector3* end, Camera* camera)
{
    if (begin == end)
    {
        URHO3D_LOGERROR("Can't focus on empty array of vertices");
        return;
    }

    if (!camera)
    {
        URHO3D_LOGERROR("No Camera found");
        return;
    }

    if (!node_)
    {
        URHO3D_LOGERROR("No Node to move");
        return;
    }

    const Frustum frustum = camera->GetFrustum();
    ea::array<Plane, 6> planes{frustum.planes_[0], frustum.planes_[1], frustum.planes_[2], frustum.planes_[3],
        frustum.planes_[4], frustum.planes_[5]};

    for (auto& plane : planes)
    {
        plane.d_ = -ea::numeric_limits<float>::max();
        for (auto point = begin; point != end; ++point)
        {
            plane.d_ = ea::max(plane.d_, -plane.normal_.DotProduct(*point));
        }
    }

    if (camera->IsOrthographic())
    {
        // Evaluate new central point.
        const auto left = node_->WorldToLocal(planes[PLANE_LEFT].GetPoint()).x_;
        const auto right = node_->WorldToLocal(planes[PLANE_RIGHT].GetPoint()).x_;
        const auto up = node_->WorldToLocal(planes[PLANE_UP].GetPoint()).y_;
        const auto down = node_->WorldToLocal(planes[PLANE_DOWN].GetPoint()).y_;
        auto offset = Vector3(left + right, up + down, 0.0f) * 0.5f;

        // Move camera back if it is too close to the closest point.
        const auto n = node_->WorldToLocal(planes[PLANE_NEAR].GetPoint()).z_;
        if (n < 0.0f)
            offset.z_ += n;

        // Move camera node.
        node_->SetWorldPosition(node_->LocalToWorld(offset));

        // Adjust orthoSize_ to avoid any extra padding.
        const auto autoAspectRatio = camera->GetAutoAspectRatio();
        const auto zoom = camera->GetZoom();
        const auto aspectRatio = camera->GetAspectRatio();
        const float orthoSizeY = (up - down) * zoom;
        const float orthoSizeX = (right - left) * zoom;
        const auto verticalOrthoSize = Urho3D::Max(orthoSizeX / aspectRatio, orthoSizeY);

        if (autoAspectRatio)
        {
            camera->SetOrthoSize(verticalOrthoSize);
            camera->SetAspectRatio(aspectRatio);
            camera->SetAutoAspectRatio(true);
        }
        else
        {
            camera->SetOrthoSize(Vector2(verticalOrthoSize*aspectRatio, verticalOrthoSize));
        }
    }
    else
    {
        // Evaluate focal point.
        const auto ray0 = planes[PLANE_LEFT].Intersect(planes[PLANE_RIGHT]);
        const auto ray1 = planes[PLANE_UP].Intersect(planes[PLANE_DOWN]);
        const auto distance0 = frustum.planes_[PLANE_NEAR].Distance(ray0.origin_);
        const auto distance1 = frustum.planes_[PLANE_NEAR].Distance(ray1.origin_);
        Vector3 focalPoint = (distance0 < distance1) ? ray0.ClosestPoint(ray1) : ray1.ClosestPoint(ray0);

        const auto n = planes[PLANE_NEAR].Distance(focalPoint) + camera->GetNearClip();
        if (n > 0.0f)
            focalPoint -= planes[PLANE_NEAR].normal_ * n;
        // Move camera node.
        node_->SetWorldPosition(focalPoint);
    }
}

}
