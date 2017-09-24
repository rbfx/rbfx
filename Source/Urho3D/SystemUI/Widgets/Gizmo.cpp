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

void ToImGuizmo(float* dest, const Matrix4& src)
{
    for (auto row = 0; row < 4; row++)
    {
        for (auto col = 0; col < 4; col++)
            dest[row * 4 + col] = src.Data()[col * 4 + row];
    }
}

void FromImGuizmo(Matrix4& dest, float* src)
{
    for (auto row = 0; row < 4; row++)
    {
        for (auto col = 0; col < 4; col++)
            (&dest.m00_)[col * 4 + row] = src[row * 4 + col];
    }
}

bool Gizmo::Manipulate(const Camera* camera, Node* node)
{
    float view[16];
    float proj[16];
    float tran[16];

    memcpy(view, camera->GetView().ToMatrix4().Data(), sizeof(view));
    memcpy(proj, camera->GetProjection().Data(), sizeof(proj));
    memcpy(tran, node->GetTransform().ToMatrix4().Data(), sizeof(tran));

    FlipMatrix(view);
    FlipMatrix(proj);
    FlipMatrix(tran);

//    ToImGuizmo(view, camera->GetView().ToMatrix4());
//    ToImGuizmo(proj, camera->GetProjection());
//    ToImGuizmo(tran, node->GetTransform().ToMatrix4());

    // Enums are compatible.
    ImGuizmo::OPERATION operation = static_cast<ImGuizmo::OPERATION>(operation_);
    ImGuizmo::MODE mode;
    // Scaling only works in local space.
    if (transformSpace_ == TS_LOCAL || operation_ == GIZMOOP_SCALE)
        mode = ImGuizmo::LOCAL;
    else
        mode = ImGuizmo::WORLD;

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(view, proj, operation, mode, tran, nullptr, nullptr);

    if (IsActive())
    {
        FlipMatrix(tran);
        node->SetTransform(Matrix4(tran));
//        Matrix4 matrix;
//        FromImGuizmo(matrix, tran);
//        node->SetTransform(matrix);
        return true;
    }

    return false;
}

bool Gizmo::IsActive() const
{
    return ImGuizmo::IsUsing() && ImGui::IsMouseDown(0);
}

}
