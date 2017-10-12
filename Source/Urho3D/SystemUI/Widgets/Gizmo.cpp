//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../../Scene/Scene.h"
#include "../../IO/Log.h"
#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include "Gizmo.h"

namespace Urho3D
{

/// Flips matrix between row-wise and column-wise.
static void FlipMatrix(float* matrix)
{
    for (auto i = 1; i < 4; i++)
        Swap(matrix[4 * 0 + i], matrix[4 * i + 0]);

    for (auto i = 2; i < 4; i++)
        Swap(matrix[4 * 1 + i], matrix[4 * i + 1]);

    Swap(matrix[4 * 2 + 3], matrix[4 * 3 + 2]);
}

Gizmo::Gizmo(Context* context) : Object(context)
{

}

bool Gizmo::Manipulate(const Camera* camera, Node* node)
{
    PODVector<Node*> nodes;
    nodes.Push(node);
    return Manipulate(camera, nodes);
}

bool Gizmo::IsActive() const
{
    return ImGuizmo::IsUsing();
}

bool Gizmo::Manipulate(const Camera* camera, const PODVector<Node*>& nodes)
{
    if (nodes.Empty())
        return false;

    float view[16];
    float proj[16];
    float tran[16];
    float delta[16];

    // Enums are compatible.
    ImGuizmo::OPERATION operation = static_cast<ImGuizmo::OPERATION>(operation_);
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
    else if (nodes.Size() > 1)
        mode = ImGuizmo::WORLD;

    if (!IsActive())
    {
        // Find center point of all nodes
        if (nodes.Size() == 1)
            currentOrigin_ = nodes.Front()->GetTransform().ToMatrix4();     // Makes gizmo work in local space too.
        else
        {
            // It is not clear what should be rotation and scale of center point for multiselection, therefore we limit
            // multiselection operations to world space (see above).
            Vector3 center = Vector3::ZERO;
            for (const auto& node: nodes)
                center += node->GetWorldPosition();
            center /= nodes.Size();
            currentOrigin_.SetTranslation(center);
        }
    }

    memcpy(view, camera->GetView().ToMatrix4().Data(), sizeof(view));
    memcpy(proj, camera->GetProjection().Data(), sizeof(proj));
    memcpy(tran, currentOrigin_.Data(), sizeof(tran));

    FlipMatrix(view);
    FlipMatrix(proj);
    FlipMatrix(tran);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(view, proj, operation, mode, tran, delta, nullptr);

    if (IsActive())
    {
        FlipMatrix(tran);
        FlipMatrix(delta);
        Matrix4 dm(delta);

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
                if (!nodeScaleStart_.Contains(node))
                    nodeScaleStart_[node] = node->GetScale();
                node->SetScale(nodeScaleStart_[node] * dm.Scale());
            }
            else
            {
                // Delta matrix is always in world-space.
                if (operation_ == GIZMOOP_ROTATE)
                    node->RotateAround(currentOrigin_.Translation(), -dm.Rotation(), TS_WORLD);
                else
                    node->Translate(dm.Translation(), TS_WORLD);
            }
        }

        return true;
    }
    else if (operation_ == GIZMOOP_SCALE && !nodeScaleStart_.Empty())
        nodeScaleStart_.Clear();
    return false;
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

}
