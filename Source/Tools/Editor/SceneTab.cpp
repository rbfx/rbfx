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

#include "SceneTab.h"
#include "EditorEvents.h"
#include "SceneSettings.h"
#include <Toolbox/Scene/DebugCameraController.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>


namespace Urho3D
{

SceneTab::SceneTab(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position)
    : SceneView(context, {0, 0, 1024, 768})
    , gizmo_(context)
    , inspector_(context)
    , placeAfter_(afterDockName)
    , placePosition_(position)
    , id_(id)
{
    SetTitle(title_);

    settings_ = new SceneSettings(context);
    effectSettings_ = new SceneEffects(this);

    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, std::bind(&SceneTab::OnNodeSelectionChanged, this));
    SubscribeToEvent(effectSettings_, E_EDITORSCENEEFFECTSCHANGED, std::bind(&AttributeInspector::CopyEffectsFrom,
                                                                             &inspector_, viewport_));
}

SceneTab::~SceneTab() = default;

void SceneTab::SetSize(const IntRect& rect)
{
    SceneView::SetSize(rect);
    gizmo_.SetScreenRect(rect);
}

bool SceneTab::RenderWindow()
{
    bool open = true;
    auto& style = ui::GetStyle();

    if (GetInput()->IsMouseVisible())
        lastMousePosition_ = GetInput()->GetMousePosition();

    ui::SetNextDockPos(placeAfter_.CString(), placePosition_, ImGuiCond_FirstUseEver);
    if (ui::BeginDock(uniqueTitle_.CString(), &open, windowFlags_))
    {
        // Focus window when appearing
        if (!isRendered_)
        {
            ui::SetWindowFocus();
            isRendered_ = true;
            effectSettings_->Prepare(true);
        }

        ImGuizmo::SetDrawlist();
        ui::SetCursorPos(ui::GetCursorPos() - style.WindowPadding);
        ui::Image(texture_, ToImGui(rect_.Size()));

        if (rect_.IsInside(lastMousePosition_) == INSIDE)
        {
            if (!ui::IsWindowFocused() && ui::IsItemHovered() && GetInput()->GetMouseButtonDown(MOUSEB_RIGHT))
                ui::SetWindowFocus();

            if (ui::IsDockActive())
                isActive_ = ui::IsWindowFocused();
            else
                isActive_ = false;
        }
        else
            isActive_ = false;

        camera_->GetComponent<DebugCameraController>()->SetEnabled(isActive_);

        gizmo_.ManipulateSelection(GetCamera());

        // Update scene view rect according to window position
        // if (!GetInput()->GetMouseButtonDown(MOUSEB_LEFT))
        {
            auto titlebarHeight = ui::GetCurrentContext()->CurrentWindow->TitleBarHeight();
            auto pos = ui::GetWindowPos();
            pos.y += titlebarHeight;
            auto size = ui::GetWindowSize();
            size.y -= titlebarHeight;
            if (size.x > 0 && size.y > 0)
            {
                IntRect newRect(ToIntVector2(pos), ToIntVector2(pos + size));
                SetSize(newRect);
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
                pos -= rect_.Min();

                Ray cameraRay = GetCamera()->GetScreenRay((float)pos.x_ / rect_.Width(), (float)pos.y_ / rect_.Height());
                // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
                PODVector<RayQueryResult> results;

                RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
                scene_->GetComponent<Octree>()->RaycastSingle(query);

                if (!results.Size())
                {
                    // When object geometry was not hit by a ray - query for object bounding box.
                    RayOctreeQuery query2(results, cameraRay, RAY_OBB, M_INFINITY, DRAWABLE_GEOMETRY);
                    scene_->GetComponent<Octree>()->RaycastSingle(query2);
                }

                if (results.Size())
                {
                    WeakPtr<Node> clickNode(results[0].drawable_->GetNode());
                    if (!GetInput()->GetKeyDown(KEY_CTRL))
                        UnselectAll();

                    ToggleSelection(clickNode);
                }
                else
                    UnselectAll();
            }
        }
        else
            windowFlags_ = 0;

        const auto tabContextMenuTitle = "SceneTab context menu";
        if (ui::IsDockTabHovered() && GetInput()->GetMouseButtonPress(MOUSEB_RIGHT))
            ui::OpenPopup(tabContextMenuTitle);
        if (ui::BeginPopup(tabContextMenuTitle))
        {
            if (ui::MenuItem("Save"))
                SaveScene();

            ui::Separator();

            if (ui::MenuItem("Close"))
                open = false;

            ui::EndPopup();
        }
    }
    else
    {
        isActive_ = false;
        isRendered_ = false;
    }
    ui::EndDock();

    return open;
}

void SceneTab::LoadScene(const String& filePath)
{
    if (filePath.Empty())
        return;

    if (filePath.EndsWith(".xml", false))
    {
        if (scene_->LoadXML(GetCache()->GetResource<XMLFile>(filePath)->GetRoot()))
        {
            path_ = filePath;
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(filePath).CString());
    }
    else if (filePath.EndsWith(".json", false))
    {
        if (scene_->LoadJSON(GetCache()->GetResource<JSONFile>(filePath)->GetRoot()))
        {
            path_ = filePath;
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(filePath).CString());
    }
    else
        URHO3D_LOGERRORF("Unknown scene file format %s", GetExtension(filePath).CString());
}

bool SceneTab::SaveScene(const String& filePath)
{
    auto resourcePath = filePath.Empty() ? path_ : filePath;
    auto fullPath = GetCache()->GetResourceFileName(resourcePath);
    File file(context_, fullPath, FILE_WRITE);
    bool result = false;

    float elapsed = 0;
    if (!settings_->saveElapsedTime_)
    {
        elapsed = scene_->GetElapsedTime();
        scene_->SetElapsedTime(0);
    }

    if (fullPath.EndsWith(".xml", false))
        result = scene_->SaveXML(file);
    else if (fullPath.EndsWith(".json", false))
        result = scene_->SaveJSON(file);

    if (!settings_->saveElapsedTime_)
        scene_->SetElapsedTime(elapsed);

    if (result)
    {
        if (!filePath.Empty())
            path_ = filePath;
    }
    else
        URHO3D_LOGERRORF("Saving scene to %s failed.", resourcePath.CString());

    return result;
}

void SceneTab::CreateObjects()
{
    SceneView::CreateObjects();
    camera_->CreateComponent<DebugCameraController>();
}

void SceneTab::Select(Node* node)
{
    if (gizmo_.Select(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

void SceneTab::Unselect(Node* node)
{
    if (gizmo_.Unselect(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

void SceneTab::ToggleSelection(Node* node)
{
    gizmo_.ToggleSelection(node);
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
}

void SceneTab::UnselectAll()
{
    if (gizmo_.UnselectAll())
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

const Vector<WeakPtr<Node>>& SceneTab::GetSelection() const
{
    return gizmo_.GetSelection();
}

void SceneTab::RenderGizmoButtons()
{
    const auto& style = ui::GetStyle();

    auto drawGizmoOperationButton = [&](GizmoOperation operation, const char* icon, const char* tooltip)
    {
        if (gizmo_.GetOperation() == operation)
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        else
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        if (ui::ButtonEx(icon, {20, 20}, ImGuiButtonFlags_PressedOnClick))
            gizmo_.SetOperation(operation);
        ui::PopStyleColor();
        ui::SameLine();
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip);
    };

    auto drawGizmoTransformButton = [&](TransformSpace transformSpace, const char* icon, const char* tooltip)
    {
        if (gizmo_.GetTransformSpace() == transformSpace)
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        else
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        if (ui::ButtonEx(icon, {20, 20}, ImGuiButtonFlags_PressedOnClick))
            gizmo_.SetTransformSpace(transformSpace);
        ui::PopStyleColor();
        ui::SameLine();
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip);
    };

    drawGizmoOperationButton(GIZMOOP_TRANSLATE, ICON_FA_ARROWS, "Translate");
    drawGizmoOperationButton(GIZMOOP_ROTATE, ICON_FA_REPEAT, "Rotate");
    drawGizmoOperationButton(GIZMOOP_SCALE, ICON_FA_ARROWS_ALT, "Scale");
    ui::TextUnformatted("|");
    ui::SameLine();
    drawGizmoTransformButton(TS_WORLD, ICON_FA_ARROWS, "World");
    drawGizmoTransformButton(TS_LOCAL, ICON_FA_ARROWS_ALT, "Local");
    ui::TextUnformatted("|");
    ui::SameLine();


    auto light = camera_->GetComponent<Light>();
    if (light->IsEnabled())
        ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    else
        ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
    if (ui::Button(ICON_FA_LIGHTBULB_O, {20, 20}))
        light->SetEnabled(!light->IsEnabled());
    ui::PopStyleColor();
    ui::SameLine();
    if (ui::IsItemHovered())
        ui::SetTooltip("Camera Headlight");
}

bool SceneTab::IsSelected(Node* node) const
{
    return gizmo_.IsSelected(node);
}

void SceneTab::OnNodeSelectionChanged()
{
    using namespace EditorSelectionChanged;
    const auto& selection = GetSelection();
    if (selection.Size() == 1)
    {
        const auto& node = selection.Front();
        const auto& components = node->GetComponents();
        if (!components.Empty())
            selectedComponent_ = components.Front();
        else
            selectedComponent_ = nullptr;
    }
    else
        selectedComponent_ = nullptr;
}

void SceneTab::RenderInspector()
{
    // TODO: inspector for multi-selection.
    if (GetSelection().Size() == 1)
    {
        auto node = GetSelection().Front();
        PODVector<Serializable*> items;
        items.Push(dynamic_cast<Serializable*>(node.Get()));
        if (node == scene_)
        {
            effectSettings_->Prepare();
            items.Push(settings_.Get());
            items.Push(effectSettings_.Get());
        }
        if (!selectedComponent_.Expired())
            items.Push(dynamic_cast<Serializable*>(selectedComponent_.Get()));
        inspector_.RenderAttributes(items);
    }
}

void SceneTab::RenderSceneNodeTree(Node* node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (node == nullptr)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
        node = scene_;
    }

    if (node->IsTemporary())
        return;

    String name = ToString("%s (%d)", (node->GetName().Empty() ? node->GetTypeName() : node->GetName()).CString(), node->GetID());
    bool isSelected = IsSelected(node);

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    auto opened = ui::TreeNodeEx(name.CString(), flags);

    if (ui::IsItemClicked(0))
    {
        if (!GetInput()->GetKeyDown(KEY_CTRL))
            UnselectAll();
        ToggleSelection(node);
    }

    if (opened)
    {
        for (auto& component: node->GetComponents())
        {
            if (component->IsTemporary())
                continue;

            bool selected = selectedComponent_ == component;
            if (ui::Selectable(component->GetTypeName().CString(), selected))
            {
                UnselectAll();
                ToggleSelection(node);
                selectedComponent_ = component;
            }
        }

        for (auto& child: node->GetChildren())
            RenderSceneNodeTree(child);
        ui::TreePop();
    }
}

void SceneTab::LoadProject(XMLElement scene)
{
    id_ = StringHash(ToUInt(scene.GetAttribute("id"), 16));
    SetTitle(scene.GetAttribute("title"));
    LoadScene(scene.GetAttribute("path"));

    auto camera = scene.GetChild("camera");
    if (camera.NotNull())
    {
        if (auto position = camera.GetChild("position"))
            camera_->SetPosition(position.GetVariant().GetVector3());
        if (auto rotation = camera.GetChild("rotation"))
            camera_->SetRotation(rotation.GetVariant().GetQuaternion());
        if (auto light = camera.GetChild("light"))
            camera_->GetComponent<Light>()->SetEnabled(light.GetVariant().GetBool());
    }

    settings_->LoadProject(scene);
    effectSettings_->LoadProject(scene);
}

void SceneTab::SaveProject(XMLElement scene) const
{
    scene.SetAttribute("id", id_.ToString().CString());
    scene.SetAttribute("title", title_);
    scene.SetAttribute("path", path_);

    auto camera = scene.CreateChild("camera");
    camera.CreateChild("position").SetVariant(camera_->GetPosition());
    camera.CreateChild("rotation").SetVariant(camera_->GetRotation());
    camera.CreateChild("light").SetVariant(camera_->GetComponent<Light>()->IsEnabled());

    settings_->SaveProject(scene);
    effectSettings_->SaveProject(scene);
}

void SceneTab::SetTitle(const String& title)
{
    title_ = title;
    uniqueTitle_ = ToString("%s###%s", title.CString(), id_.ToString().CString());
}

void SceneTab::ClearCachedPaths()
{
    path_.Clear();
}

}
