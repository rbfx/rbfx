//
// Copyright (c) 2017-2020 the rbfx project.
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

bool Gizmo::IsActive() const
{
    return ImGuizmo::IsUsing();
}

bool Gizmo::ManipulateNode(const Camera* camera, Node* node)
{
    return Manipulate(camera, &node, &node + 1);
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

int Gizmo::GetSelectionCenter(Vector3& outCenter, Node** begin, Node** end)
{
    outCenter = Vector3::ZERO;
    auto count = 0;
    for (auto it = begin; it != end; it++)
    {
        Node* node = *it;
        if (!node || node->GetType() == Scene::GetTypeStatic())
            continue;
        outCenter += node->GetWorldPosition();
        count++;
    }

    if (count != 0)
        outCenter /= count;
    return count;
}

bool Gizmo::Manipulate(const Camera* camera, Node** begin, Node** end)
{
    if (begin == end)
        return false;

    ImGuizmo::SetOrthographic(camera->IsOrthographic());

    Matrix4 currentOrigin;
    if (begin + 1 != end)   // nodes.size() > 1
    {
        // Find center point of all nodes
        // It is not clear what should be rotation and scale of center point for multiselection, therefore we limit
        // multiselection operations to world space (see above).
        Vector3 center;
        int count = GetSelectionCenter(center, begin, end);
        if (count == 0)
            return false;
        currentOrigin.SetTranslation(center);
    }
    else if (*begin)
        currentOrigin = (*begin)->GetWorldTransform().ToMatrix4();

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
    if (begin + 1 != end)   // nodes.size() > 1
        mode = ImGuizmo::WORLD;

    Matrix4 view = camera->GetView().ToMatrix4().Transpose();
    Matrix4 proj = camera->GetProjection().Transpose();
    Matrix4 tran = currentOrigin.Transpose();
    Matrix4 delta;

    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(window->Pos.x, window->Pos.y, window->Size.x, window->Size.y);
    ImGuizmo::Manipulate(&view.m00_, &proj.m00_, operation, mode, &tran.m00_, &delta.m00_, nullptr);

    if (IsActive())
    {
        if (!wasActive_)
        {
            // Just started modifying nodes.
            for (auto it = begin; it != end; it++)
                initialTransforms_[*it] = (*it)->GetTransform();
        }

        wasActive_ = true;
        tran = tran.Transpose();
        delta = delta.Transpose();

        currentOrigin = Matrix4(tran);

        for (auto it = begin; it != end; it++)
        {
            Node* node = *it;
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
            // Delta matrix is always in world-space.
            else if (operation_ == GIZMOOP_ROTATE)
                node->RotateAround(currentOrigin.Translation(), -delta.Rotation(), TS_WORLD);
            else
                node->Translate(delta.Translation(), TS_WORLD);
        }

        return true;
    }
    else
    {
        if (wasActive_)
        {
            // Just finished modifying nodes.
            using namespace GizmoNodeModified;
            for (auto it = begin; it != end; it++)
            {
                Node* node = *it;
                if (!node)
                {
                    URHO3D_LOGWARNINGF("Node expired while manipulating it with gizmo.");
                    continue;
                }

                auto jt = initialTransforms_.find(node);
                if (jt == initialTransforms_.end())
                {
                    URHO3D_LOGWARNINGF("Gizmo has no record of initial node transform. List of transformed nodes "
                                       "changed mid-manipulation?");
                    continue;
                }

                SendEvent(E_GIZMONODEMODIFIED, P_NODE, &*node, P_OLDTRANSFORM, jt->second,
                    P_NEWTRANSFORM, node->GetTransform());
            }
        }
        wasActive_ = false;
        initialTransforms_.clear();
        nodeScaleStart_.clear();
    }
    return false;
}

}
