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
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_ToggleLocal = EditorHotkey{"TransformGizmo.ToggleLocal"}.Press(KEY_X);
const auto Hotkey_TogglePivoted = EditorHotkey{"TransformGizmo.TogglePivoted"}.Press(KEY_Z);
const auto Hotkey_Select = EditorHotkey{"TransformGizmo.Select"}.Press(KEY_Q);
const auto Hotkey_Translate = EditorHotkey{"TransformGizmo.Translate"}.Press(KEY_W);
const auto Hotkey_Rotate = EditorHotkey{"TransformGizmo.Rotate"}.Press(KEY_E);
const auto Hotkey_Scale = EditorHotkey{"TransformGizmo.Scale"}.Press(KEY_R);

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
    SerializeOptionalValue(archive, "SnapPosition", snapPosition_, Settings{}.snapPosition_);
    SerializeOptionalValue(archive, "SnapRotation", snapRotation_, Settings{}.snapRotation_);
    SerializeOptionalValue(archive, "SnapScale", snapScale_, Settings{}.snapScale_);
}

void TransformManipulator::Settings::RenderSettings()
{
    if (ui::DragFloat("Snap Position", &snapPosition_.x_, 0.1f, 0.1f, 10.0f, "%.2f"))
    {
        snapPosition_.y_ = snapPosition_.x_;
        snapPosition_.z_ = snapPosition_.x_;
    }
    ui::Indent();
    ui::DragFloat("X", &snapPosition_.x_, 0.1f, 0.1f, 10.0f, "%.2f");
    ui::DragFloat("Y", &snapPosition_.y_, 0.1f, 0.1f, 10.0f, "%.2f");
    ui::DragFloat("Z", &snapPosition_.z_, 0.1f, 0.1f, 10.0f, "%.2f");
    ui::Unindent();

    ui::DragFloat("Snap Rotation", &snapRotation_, 5.0f, 5.0f, 180.0f, "%.1f");
    ui::DragFloat("Snap Scale", &snapScale_, 0.1f, 0.1f, 1.0f, "%.2f");
}

Vector3 TransformManipulator::Settings::GetSnapValue(TransformGizmoOperation op) const
{
    switch (op)
    {
    case TransformGizmoOperation::Translate:
        return snapPosition_;
    case TransformGizmoOperation::Rotate:
        return Vector3::ONE * snapRotation_;
    case TransformGizmoOperation::Scale:
        return Vector3::ONE * snapScale_;
    case TransformGizmoOperation::None:
    default:
        return Vector3::ZERO;
    }
}

TransformManipulator::TransformManipulator(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
    auto hotkeyManager = owner_->GetHotkeyManager();
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
        const Vector3 snapValue = needSnap ? cfg.GetSnapValue(operation_) : Vector3::ZERO;
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

void TransformManipulator::Render(SceneViewPage& scenePage)
{
}

void TransformManipulator::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

bool TransformManipulator::RenderTabContextMenu()
{
    auto hotkeyManager = owner_->GetHotkeyManager();

    if (!ui::BeginMenu("Transform Gizmo"))
        return true;

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

    ui::EndMenu();
    return true;
}

bool TransformManipulator::RenderToolbar()
{
    if (Widgets::ToolbarButton(ICON_FA_ARROW_POINTER, "Select Objects", operation_ == TransformGizmoOperation::None))
        operation_ = TransformGizmoOperation::None;
    if (Widgets::ToolbarButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, "Move Objects", operation_ == TransformGizmoOperation::Translate))
        operation_ = TransformGizmoOperation::Translate;
    if (Widgets::ToolbarButton(ICON_FA_ARROWS_ROTATE, "Rotate Objects", operation_ == TransformGizmoOperation::Rotate))
        operation_ = TransformGizmoOperation::Rotate;
    if (Widgets::ToolbarButton(ICON_FA_ARROWS_LEFT_RIGHT_TO_LINE, "Scale Objects", operation_ == TransformGizmoOperation::Scale))
        operation_ = TransformGizmoOperation::Scale;

    Widgets::ToolbarSeparator();

    const char* localTitle = isLocal_ ? "Transform in local object space" : "Transform in world space";
    if (Widgets::ToolbarButton(ICON_FA_CUBE, localTitle, isLocal_))
        isLocal_ = !isLocal_;

    const char* pivotedTitle = isPivoted_ ? "Transform around individual objects' pivots" : "Transform around the center of selection";\
    if (Widgets::ToolbarButton(ICON_FA_ARROWS_TO_DOT, pivotedTitle, isPivoted_))
        isPivoted_ = !isPivoted_;

    Widgets::ToolbarSeparator();

    return true;
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
