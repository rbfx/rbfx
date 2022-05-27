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

#include "../../Core/CommonEditorActions.h"
#include "../../Core/IniHelpers.h"
#include "../../Foundation/SceneViewTab/TransformManipulator.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <ImGuizmo/ImGuizmo.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_ToggleLocal, "TransformGizmo.ToggleLocal", QUAL_NONE, KEY_X);
URHO3D_EDITOR_HOTKEY(Hotkey_TogglePivoted, "TransformGizmo.TogglePivoted", QUAL_NONE, KEY_Z);
URHO3D_EDITOR_HOTKEY(Hotkey_Select, "TransformGizmo.Select", QUAL_NONE, KEY_Q);
URHO3D_EDITOR_HOTKEY(Hotkey_Translate, "TransformGizmo.Translate", QUAL_NONE, KEY_W);
URHO3D_EDITOR_HOTKEY(Hotkey_Rotate, "TransformGizmo.Rotate", QUAL_NONE, KEY_E);
URHO3D_EDITOR_HOTKEY(Hotkey_Scale, "TransformGizmo.Scale", QUAL_NONE, KEY_R);

}

void Foundation_TransformManipulator(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    auto settingsManager = project->GetSettingsManager();

    auto settingsPage = MakeShared<TransformManipulator::SettingsPage>(context);
    settingsManager->AddPage(settingsPage);

    sceneViewTab->RegisterAddon<TransformManipulator>(settingsPage);
}

void TransformManipulator::Settings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "SnapPosition", snapPosition_);
    SerializeOptionalValue(archive, "SnapRotation", snapRotation_);
    SerializeOptionalValue(archive, "SnapScale", snapScale_);
}

void TransformManipulator::Settings::RenderSettings()
{
    ui::DragFloat("Snap Position", &snapPosition_, 0.001f, 0.001f, 10.0f, "%.3f");
    ui::DragFloat("Snap Rotation", &snapRotation_, 0.001f, 0.001f, 360.0f, "%.3f");
    ui::DragFloat("Snap Scale", &snapScale_, 0.001f, 0.001f, 1.0f, "%.3f");
}

float TransformManipulator::Settings::GetSnapValue(TransformGizmoOperation op) const
{
    switch (op)
    {
    case TransformGizmoOperation::Translate:
        return snapPosition_;
    case TransformGizmoOperation::Rotate:
        return snapRotation_;
    case TransformGizmoOperation::Scale:
        return snapScale_;
    case TransformGizmoOperation::None:
    default:
        return 0.0f;
    }
}

TransformManipulator::TransformManipulator(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
    auto project = owner_->GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_ToggleLocal, &TransformManipulator::ToggleSpace);
    hotkeyManager->BindHotkey(this, Hotkey_TogglePivoted, &TransformManipulator::TogglePivoted);
    hotkeyManager->BindHotkey(this, Hotkey_Select, &TransformManipulator::SetSelect);
    hotkeyManager->BindHotkey(this, Hotkey_Translate, &TransformManipulator::SetTranslate);
    hotkeyManager->BindHotkey(this, Hotkey_Rotate, &TransformManipulator::SetRotate);
    hotkeyManager->BindHotkey(this, Hotkey_Scale, &TransformManipulator::SetScale);
}

void TransformManipulator::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    if (!settings_)
        return;
    const Settings& cfg = settings_->GetValues();

    const auto& nodes = scenePage.selection_.GetEffectiveNodes();
    if (nodes.empty())
        return;

    EnsureGizmoInitialized(scenePage);

    if (!mouseConsumed)
    {
        Camera* camera = scenePage.renderer_->GetCamera();
        const TransformGizmo gizmo{camera, scenePage.contentArea_};

        const bool needSnap = ui::IsKeyDown(KEY_CTRL);
        const float snapValue = needSnap ? cfg.GetSnapValue(operation_) : 0.0f;
        if (transformNodesGizmo_->Manipulate(gizmo, operation_, isLocal_, isPivoted_, snapValue))
            mouseConsumed = true;
    }
}

void TransformManipulator::EnsureGizmoInitialized(SceneViewPage& scenePage)
{
    if (scenePage.selection_.GetRevision() != selectionRevision_ || scenePage.scene_ != selectionScene_)
    {
        selectionRevision_ = scenePage.selection_.GetRevision();
        selectionScene_ = scenePage.scene_;
        transformNodesGizmo_ = ea::nullopt;
    }

    if (!transformNodesGizmo_)
    {
        const auto& nodes = scenePage.selection_.GetEffectiveNodes();
        const Node* activeNode = scenePage.selection_.GetActiveNode();
        transformNodesGizmo_ = TransformNodesGizmo{activeNode, nodes.begin(), nodes.end()};
        transformNodesGizmo_->OnNodeTransformChanged.Subscribe(this, &TransformManipulator::OnNodeTransformChanged);
    }
}

void TransformManipulator::OnNodeTransformChanged(Node* node, const Transform& oldTransform)
{
    owner_->PushAction<ChangeNodeTransformAction>(node, oldTransform);
}

void TransformManipulator::UpdateAndRender(SceneViewPage& scenePage)
{
}

void TransformManipulator::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

void TransformManipulator::RenderTabContextMenu()
{
    auto project = owner_->GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    if (ui::MenuItem("In Local Space", hotkeyManager->GetHotkeyLabel(Hotkey_ToggleLocal).c_str(), isLocal_))
        isLocal_ = !isLocal_;

    if (ui::MenuItem("Is Pivoted", hotkeyManager->GetHotkeyLabel(Hotkey_TogglePivoted).c_str(), isPivoted_))
        isPivoted_ = !isPivoted_;

    ui::Separator();

    if (ui::MenuItem("Select", hotkeyManager->GetHotkeyLabel(Hotkey_Select).c_str(), operation_ == TransformGizmoOperation::None))
        operation_ = TransformGizmoOperation::None;
    if (ui::MenuItem("Translate", hotkeyManager->GetHotkeyLabel(Hotkey_Translate).c_str(), operation_ == TransformGizmoOperation::Translate))
        operation_ = TransformGizmoOperation::Translate;
    if (ui::MenuItem("Rotate", hotkeyManager->GetHotkeyLabel(Hotkey_Rotate).c_str(), operation_ == TransformGizmoOperation::Rotate))
        operation_ = TransformGizmoOperation::Rotate;
    if (ui::MenuItem("Scale", hotkeyManager->GetHotkeyLabel(Hotkey_Scale).c_str(), operation_ == TransformGizmoOperation::Scale))
        operation_ = TransformGizmoOperation::Scale;
}

void TransformManipulator::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "TransformGizmo.IsLocal", isLocal_);
    WriteIntToIni(output, "TransformGizmo.IsPivoted", isPivoted_);
    WriteIntToIni(output, "TransformGizmo.Operation", static_cast<int>(operation_));
}

void TransformManipulator::ReadIniSettings(const char* line)
{
    if (const auto value = ReadIntFromIni(line, "TransformGizmo.IsLocal"))
        isLocal_ = *value != 0;
    if (const auto value = ReadIntFromIni(line, "TransformGizmo.IsPivoted"))
        isPivoted_ = *value != 0;
    if (const auto value = ReadIntFromIni(line, "TransformGizmo.Operation"))
    {
        operation_ = Clamp(static_cast<TransformGizmoOperation>(*value),
            TransformGizmoOperation::None, TransformGizmoOperation::Scale);
    }
}

}
