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

#include "SceneView.h"
#include "EditorConstants.h"
#include <Toolbox/DebugCameraController.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include <Toolbox/ImGuiDock.h>


namespace Urho3D
{

SceneView::SceneView(Context* context)
    : Object(context)
    , gizmo_(context)
{
    scene_ = SharedPtr<Scene>(new Scene(context));
    scene_->CreateComponent<Octree>();
    camera_ = scene_->CreateChild("DebugCamera");
    camera_->AddTag(InternalEditorElementTag);
    camera_->CreateComponent<Camera>();
    camera_->CreateComponent<DebugCameraController>();
    scene_->GetOrCreateComponent<DebugRenderer>()->SetView(GetCamera());
    view_ = SharedPtr<Texture2D>(new Texture2D(context));
    view_->SetFilterMode(FILTER_ANISOTROPIC);
}

SceneView::~SceneView()
{
    renderer_->Remove();
}

void SceneView::SetScreenRect(const IntRect& rect)
{
    if (rect == screenRect_)
        return;
    screenRect_ = rect;
    view_->SetSize(rect.Width(), rect.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    viewport_ = SharedPtr<Viewport>(new Viewport(context_, scene_, camera_->GetComponent<Camera>(), IntRect(IntVector2::ZERO, rect.Size())));
    view_->GetRenderSurface()->SetViewport(0, viewport_);
    gizmo_.SetScreenRect(rect);
}

bool SceneView::RenderWindow()
{
    bool open = true;
    auto& style = ui::GetStyle();
    auto padding = style.WindowPadding;
    style.WindowPadding = {0, 0};
    if (ui::BeginDock(title_.CString(), &open, windowFlags_))
    {
        isActive_ = ui::IsWindowFocused() && ui::IsWindowHovered();
        camera_->GetComponent<DebugCameraController>()->SetEnabled(isActive_);

        ImGuizmo::SetDrawlist();
        ui::Image(view_, ToImGui(screenRect_.Size()));
        gizmo_.ManipulateSelection(GetCamera());

        // Update scene view rect according to window position
        {
            auto titlebarHeight = ui::GetCurrentContext()->CurrentWindow->TitleBarHeight();
            auto pos = ui::GetWindowPos();
            pos.y += titlebarHeight;
            auto size = ui::GetWindowSize();
            size.y -= titlebarHeight;
            if (size.x > 0 && size.y > 0)
            {
                IntRect newRect(ToIntVector2(pos), ToIntVector2(pos + size));
                SetScreenRect(newRect);
            }
        }

        if (ui::IsItemHovered())
        {
            // Prevent dragging window when scene view is clicked.
            windowFlags_ = ImGuiWindowFlags_NoMove;

            // Handle object selection.
            if (!gizmo_.IsActive() && GetInput()->GetMouseButtonPress(MOUSEB_LEFT))
            {
                IntVector2 pos = GetInput()->GetMousePosition();
                pos -= screenRect_.Min();

                Ray cameraRay = camera_->GetComponent<Camera>()->GetScreenRay((float)pos.x_ / screenRect_.Width(), (float)pos.y_ / screenRect_.Height());
                // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
                PODVector<RayQueryResult> results;
                RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
                scene_->GetComponent<Octree>()->RaycastSingle(query);
                if (results.Size())
                {
                    WeakPtr<Node> clickNode(results[0].drawable_->GetNode());
                    if (!GetInput()->GetKeyDown(KEY_CTRL))
                        gizmo_.UnselectAll();

                    gizmo_.ToggleSelection(clickNode);
                }
                else
                    gizmo_.UnselectAll();
            }
        }
        else
            windowFlags_ = 0;
    }
    ui::EndDock();
    style.WindowPadding = padding;
    return open;
}

}
