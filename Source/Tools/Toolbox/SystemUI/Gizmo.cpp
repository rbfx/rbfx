//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Graphics/Light.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include "Gizmo.h"

namespace Urho3D
{

Gizmo::Gizmo(Context* context) : Object(context)
{
}

Gizmo::~Gizmo()
{
    UnsubscribeFromAllEvents();
}

bool Gizmo::Manipulate(const Camera* camera, Node* node)
{
    ea::vector<WeakPtr<Node>> nodes;
    nodes.push_back(WeakPtr<Node>(node));
    return Manipulate(camera, nodes);
}

bool Gizmo::IsActive() const
{
    return ImGuizmo::IsUsing();
}

bool Gizmo::Manipulate(const Camera* camera, const ea::vector<WeakPtr<Node>>& nodes)
{
    if (nodes.empty())
        return false;

    ImGuizmo::SetOrthographic(camera->IsOrthographic());

    if (!IsActive())
    {
        if (nodes.size() > 1)
        {
            // Find center point of all nodes
            // It is not clear what should be rotation and scale of center point for multiselection, therefore we limit
            // multiselection operations to world space (see above).
            Vector3
            center = Vector3::ZERO;
            auto count = 0;
            for (const auto& node: nodes)
            {
                if (node.Expired() || node->GetType() == Scene::GetTypeStatic())
                    continue;
                center += node->GetWorldPosition();
                count++;
            }

            if (count == 0)
                return false;

            center /= count;
            currentOrigin_.SetTranslation(center);
        }
        else if (!nodes.front().Expired())
            currentOrigin_ = nodes.front()->GetTransform().ToMatrix4();
    }

    // Enums are compatible.
    auto operation = static_cast<ImGuizmo::OPERATION>(operation_);
    ImGuizmo::MODE mode = ImGuizmo::WORLD;
    // Scaling only works in local space. Multiselections only work in world space.
    if (transformSpace_ == TS_LOCAL)
        mode = ImGuizmo::LOCAL;
    else if (transformSpace_ == TS_WORLD)
        mode = ImGuizmo::WORLD;

    // Scaling is always done in local space even for multiselections.
    if (operation_ == GIZMOOP_SCALE)
        mode = ImGuizmo::LOCAL;
        // Any other operations on multiselections are done in world space.
    else if (nodes.size() > 1)
        mode = ImGuizmo::WORLD;

    Matrix4 view = camera->GetView().ToMatrix4().Transpose();
    Matrix4 proj = camera->GetProjection().Transpose();
    Matrix4 tran = currentOrigin_.Transpose();
    Matrix4 delta;

    ImGuiIO& io = ImGui::GetIO();

    auto pos = displayPos_;
    auto size = displaySize_;
    if (size.x == 0 && size.y == 0)
        size = io.DisplaySize;
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
    ImGuizmo::Manipulate(&view.m00_, &proj.m00_, operation, mode, &tran.m00_, &delta.m00_, nullptr);

    if (IsActive())
    {
        if (!wasActive_)
        {
            // Just started modifying nodes.
            for (const auto& node: nodes)
                initialTransforms_[node] = node->GetTransform();
        }

        wasActive_ = true;
        tran = tran.Transpose();
        delta = delta.Transpose();

        currentOrigin_ = Matrix4(tran);

        for (const auto& node: nodes)
        {
            if (node == nullptr)
            {
                URHO3D_LOGERROR("Gizmo received null pointer of node.");
                continue;
            }

            if (operation_ == GIZMOOP_SCALE)
            {
                // A workaround for ImGuizmo bug where delta matrix returns absolute scale value.
                if (!nodeScaleStart_.contains(node))
                    nodeScaleStart_[node] = node->GetScale();
                node->SetScale(nodeScaleStart_[node] * delta.Scale());
            }
            else
            {
                // Delta matrix is always in world-space.
                if (operation_ == GIZMOOP_ROTATE)
                    node->RotateAround(currentOrigin_.Translation(), -delta.Rotation(), TS_WORLD);
                else
                    node->Translate(delta.Translation(), TS_WORLD);
            }
        }

        return true;
    }
    else
    {
        if (wasActive_)
        {
            // Just finished modifying nodes.
            using namespace GizmoNodeModified;
            for (const auto& node: nodes)
            {
                if (node.Expired())
                {
                    URHO3D_LOGWARNINGF("Node expired while manipulating it with gizmo.");
                    continue;
                }

                auto it = initialTransforms_.find(node.Get());
                if (it == initialTransforms_.end())
                {
                    URHO3D_LOGWARNINGF("Gizmo has no record of initial node transform. List of transformed nodes "
                        "changed mid-manipulation?");
                    continue;
                }

                SendEvent(E_GIZMONODEMODIFIED, P_NODE, node.Get(), P_OLDTRANSFORM, it->second,
                    P_NEWTRANSFORM, node->GetTransform());
            }
        }
        wasActive_ = false;
        initialTransforms_.clear();
        if (operation_ == GIZMOOP_SCALE && !nodeScaleStart_.empty())
            nodeScaleStart_.clear();
    }
    return false;
}

bool Gizmo::ManipulateSelection(const Camera* camera)
{
    ImGuizmo::SetDrawlist();

    // Remove expired selections
    for (auto it = nodeSelection_.begin(); it != nodeSelection_.end();)
    {
        if (it->Expired())
            it = nodeSelection_.erase(it);
        else
            ++it;
    }

    return Manipulate(camera, nodeSelection_);
}

void Gizmo::RenderUI()
{
    ui::TextUnformatted("Op:");
    ui::SameLine(60);

    if (ui::RadioButton("Tr", GetOperation() == GIZMOOP_TRANSLATE))
        SetOperation(GIZMOOP_TRANSLATE);
    ui::SameLine();
    if (ui::RadioButton("Rot", GetOperation() == GIZMOOP_ROTATE))
        SetOperation(GIZMOOP_ROTATE);
    ui::SameLine();
    if (ui::RadioButton("Scl", GetOperation() == GIZMOOP_SCALE))
        SetOperation(GIZMOOP_SCALE);

    ui::TextUnformatted("Space:");
    ui::SameLine(60);
    if (ui::RadioButton("World", GetTransformSpace() == TS_WORLD))
        SetTransformSpace(TS_WORLD);
    ui::SameLine();
    if (ui::RadioButton("Local", GetTransformSpace() == TS_LOCAL))
        SetTransformSpace(TS_LOCAL);
}

bool Gizmo::Select(Node* node)
{
    WeakPtr<Node> weakNode(node);
    if (nodeSelection_.contains(weakNode))
        return false;
    nodeSelection_.push_back(weakNode);
    SendEvent(E_GIZMOSELECTIONCHANGED);
    return true;
}

bool Gizmo::Select(ea::vector<Node*> nodes)
{
    bool selectedAny = false;
    for (auto* node : nodes)
    {
        WeakPtr<Node> weakNode(node);
        if (!nodeSelection_.contains(weakNode))
        {
            nodeSelection_.push_back(weakNode);
            selectedAny = true;
        }
    }
    if (selectedAny)
        SendEvent(E_GIZMOSELECTIONCHANGED);

    return selectedAny;
}

bool Gizmo::Unselect(Node* node)
{
    WeakPtr<Node> weakNode(node);
    if (!nodeSelection_.contains(weakNode))
        return false;
    nodeSelection_.erase_first(weakNode);
    SendEvent(E_GIZMOSELECTIONCHANGED);
    return true;
}

void Gizmo::ToggleSelection(Node* node)
{
    if (IsSelected(node))
        Unselect(node);
    else
        Select(node);
}

bool Gizmo::UnselectAll()
{
    if (nodeSelection_.empty())
        return false;
    nodeSelection_.clear();
    SendEvent(E_GIZMOSELECTIONCHANGED);
    return true;
}

bool Gizmo::IsSelected(Node* node) const
{
    WeakPtr<Node> pNode(node);
    return nodeSelection_.contains(pNode);
}

void Gizmo::SetScreenRect(const IntVector2& pos, const IntVector2& size)
{
    displayPos_ = ToImGui(pos);
    displaySize_ = ToImGui(size);
}

void Gizmo::SetScreenRect(const IntRect& rect)
{
    displayPos_ = ToImGui(rect.Min());
    displaySize_.x = rect.Width();
    displaySize_.y = rect.Height();
}

}
