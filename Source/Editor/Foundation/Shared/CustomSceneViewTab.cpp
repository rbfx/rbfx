//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../../Foundation/Shared/CustomSceneViewTab.h"

#include "../../Core/IniHelpers.h"

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

CustomSceneViewTab::CustomSceneViewTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : ResourceEditorTab(context, title, guid, flags, placement)
    , preview_(MakeShared<SceneWidget>(context))
    , cameraController_(MakeShared<CameraController>(context, GetHotkeyManager()))
{
    preview_->CreateDefaultScene();
}

CustomSceneViewTab::~CustomSceneViewTab()
{
}

void CustomSceneViewTab::RenderTitle()
{
    ui::Text("%s", GetActiveResourceName().c_str());
    ui::SameLine(GetContentSize().x_ - 100);
    if (ui::Button("Reset camera", ImVec2(100,0)))
    {
        ResetCamera();
    }
}

void CustomSceneViewTab::RenderContent()
{
    const ImVec2 basePosition = ui::GetCursorPos();

    RenderTitle();

    if (preview_)
    {
        const ImVec2 contentPosition = ui::GetCursorPos();

        const auto contentSize = ToImGui(GetContentSize()) - ImVec2(0, contentPosition.y - basePosition.y);
        if (ui::BeginChild("scene_preview", contentSize))
        {
            preview_->RenderContent();
        }
        ui::EndChild();

        cameraController_->ProcessInput(preview_->GetCamera(), state_);
    }
}

void CustomSceneViewTab::ResetCamera()
{
    if (preview_)
    {
        preview_->LookAt(BoundingBox(-0.5f, 0.5f));
    }
}

} // namespace Urho3D
