// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
