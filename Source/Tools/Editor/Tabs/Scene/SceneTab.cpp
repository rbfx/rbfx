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
#include <Toolbox/SystemUI/Widgets.h>


namespace Urho3D
{

SceneTab::SceneTab(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position)
    : Tab(context, id, afterDockName, position)
    , view_(context, {0, 0, 1024, 768})
    , gizmo_(context)
    , undo_(context)
{
    SetTitle(title_);

    settings_ = new SceneSettings(context);
    effectSettings_ = new SceneEffects(this);

    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, std::bind(&SceneTab::OnNodeSelectionChanged, this));
    SubscribeToEvent(effectSettings_, E_EDITORSCENEEFFECTSCHANGED, std::bind(&AttributeInspector::CopyEffectsFrom,
        &inspector_, view_.GetViewport()));

    undo_.Connect(view_.GetScene());
    undo_.Connect(&inspector_);
    undo_.Connect(&gizmo_);

    SubscribeToEvent(view_.GetScene(), E_ASYNCLOADFINISHED, [&](StringHash, VariantMap&) {
        undo_.Clear();
    });
}

SceneTab::~SceneTab() = default;

void SceneTab::SetSize(const IntRect& rect)
{
    Tab::SetSize(rect);
    view_.SetSize(rect);
    gizmo_.SetScreenRect(rect);
}

bool SceneTab::RenderWindowContent()
{
    auto& style = ui::GetStyle();
    if (GetInput()->IsMouseVisible())
        lastMousePosition_ = GetInput()->GetMousePosition();
    bool open = true;

    // Focus window when appearing
    if (!isRendered_)
    {
        ui::SetWindowFocus();
        effectSettings_->Prepare(true);
    }

    ImGuizmo::SetDrawlist();
    ui::SetCursorPos(ui::GetCursorPos() - style.WindowPadding);
    ui::Image(view_.GetTexture(), ToImGui(tabRect_.Size()));

            if (!ui::IsWindowFocused() && ui::IsItemHovered() && GetInput()->GetMouseButtonDown(MOUSEB_RIGHT))
    view_.GetCamera()->GetNode()->GetComponent<DebugCameraController>()->SetEnabled(isActive_);

    gizmo_.ManipulateSelection(view_.GetCamera());

    // Update scene view rect according to window position
        // if (!input->GetMouseButtonDown(MOUSEB_LEFT))
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
            pos -= tabRect_.Min();

            Ray cameraRay = view_.GetCamera()->GetScreenRay((float)pos.x_ / tabRect_.Width(), (float)pos.y_ / tabRect_.Height());
            // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
            PODVector<RayQueryResult> results;

            RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
            view_.GetScene()->GetComponent<Octree>()->RaycastSingle(query);

            if (!results.Size())
            {
                // When object geometry was not hit by a ray - query for object bounding box.
                RayOctreeQuery query2(results, cameraRay, RAY_OBB, M_INFINITY, DRAWABLE_GEOMETRY);
                view_.GetScene()->GetComponent<Octree>()->RaycastSingle(query2);
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

    return open;
}

void SceneTab::LoadScene(const String& filePath)
{
    if (filePath.Empty())
        return;

    if (filePath.EndsWith(".xml", false))
    {
        if (view_.GetScene()->LoadXML(GetCache()->GetResource<XMLFile>(filePath)->GetRoot()))
        {
            path_ = filePath;
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(filePath).CString());
    }
    else if (filePath.EndsWith(".json", false))
    {
        if (view_.GetScene()->LoadJSON(GetCache()->GetResource<JSONFile>(filePath)->GetRoot()))
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
        elapsed = view_.GetScene()->GetElapsedTime();
        view_.GetScene()->SetElapsedTime(0);
    }

    if (fullPath.EndsWith(".xml", false))
        result = view_.GetScene()->SaveXML(file);
    else if (fullPath.EndsWith(".json", false))
        result = view_.GetScene()->SaveJSON(file);

    if (!settings_->saveElapsedTime_)
        view_.GetScene()->SetElapsedTime(elapsed);

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
    view_.CreateObjects();
    view_.GetCamera()->GetNode()->CreateComponent<DebugCameraController>();
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

void SceneTab::RenderToolbarButtons()
{
    const auto& style = ui::GetStyle();

    auto drawGizmoOperationButton = [&](GizmoOperation operation, const char* icon, const char* tooltip)
    {
        if (gizmo_.GetOperation() == operation)
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        else
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        if (ui::ToolbarButton(icon))
            gizmo_.SetOperation(operation);
        ui::PopStyleColor();
        ui::SameLine(0, 3.f);
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip);
    };

    auto drawGizmoTransformButton = [&](TransformSpace transformSpace, const char* icon, const char* tooltip)
    {
        if (gizmo_.GetTransformSpace() == transformSpace)
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        else
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        if (ui::ToolbarButton(icon))
            gizmo_.SetTransformSpace(transformSpace);
        ui::PopStyleColor();
        ui::SameLine(0, 3.f);
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip);
    };

    drawGizmoOperationButton(GIZMOOP_TRANSLATE, ICON_FA_ARROWS, "Translate");
    drawGizmoOperationButton(GIZMOOP_ROTATE, ICON_FA_REPEAT, "Rotate");
    drawGizmoOperationButton(GIZMOOP_SCALE, ICON_FA_ARROWS_ALT, "Scale");
    ui::TextUnformatted("|");
    ui::SameLine(0, 3.f);
    drawGizmoTransformButton(TS_WORLD, ICON_FA_ARROWS, "World");
    drawGizmoTransformButton(TS_LOCAL, ICON_FA_ARROWS_ALT, "Local");
    ui::TextUnformatted("|");
    ui::SameLine(0, 3.f);


    if (Light* light = view_.GetCamera()->GetNode()->GetComponent<Light>())
    {
        if (light->IsEnabled())
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        else
            ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        if (ui::ToolbarButton(ICON_FA_LIGHTBULB_O))
            light->SetEnabled(!light->IsEnabled());
        ui::PopStyleColor();
        ui::SameLine(0, 3.f);
        if (ui::IsItemHovered())
            ui::SetTooltip("Camera Headlight");
    }
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
        if (node == view_.GetScene())
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
        node = view_.GetScene();
    }

    if (node->IsTemporary())
        return;

    String name = ToString("%s (%d)", (node->GetName().Empty() ? node->GetTypeName() : node->GetName()).CString(), node->GetID());
    bool isSelected = IsSelected(node);

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    auto opened = ui::TreeNodeEx(name.CString(), flags);
    if (!opened)
    {
        // If TreeNode above is opened, it pushes it's label as an ID to the stack. However if it is not open then no
        // ID is pushed. This creates a situation where context menu is not properly attached to said tree node due to
        // missing ID on the stack. To correct this we ensure that ID is always pushed. This allows us to show context
        // menus even for closed tree nodes.
        ui::PushID(name.CString());
    }

    if (ui::IsItemClicked(0))
    {
        if (!GetInput()->GetKeyDown(KEY_CTRL))
            UnselectAll();
        ToggleSelection(node);
    }
    else if (ui::IsItemClicked(2))
    {
        UnselectAll();
        ToggleSelection(node);
        ui::OpenPopup("Node context menu");
    }

    if (ui::BeginPopup("Node context menu"))
    {
        Input* input = GetSubsystem<Input>();
        bool alternative = input->GetKeyDown(KEY_SHIFT);

        if (ui::MenuItem(alternative ? "Create Child (Local)" : "Create Child"))
        {
            UnselectAll();
            Select(node->CreateChild(String::EMPTY, alternative ? LOCAL : REPLICATED));
        }

        if (ui::BeginMenu(alternative ? "Add Component (local)" : "Add Component"))
        {
            auto categories = context_->GetObjectCategories();

            for (auto it : categories)
            {
                if (it.first_ == "UI")
                    continue;

                if (ui::BeginMenu(it.first_.CString()))
                {
                    auto& factories = context_->GetObjectFactories();
                    Vector<Pair<StringHash, ObjectFactory*>> components;
                    for (const StringHash& componentHash : it.second_)
                    {
                        auto jt = factories.Find(componentHash);
                        if (jt != factories.End())
                            components.Push({componentHash, jt->second_.Get()});

                    }

                    Sort(components.Begin(), components.End(), [](Pair<StringHash, ObjectFactory*>& a, Pair<StringHash, ObjectFactory*>& b) {
                        return a.second_->GetTypeName().Compare(b.second_->GetTypeName()) < 0;
                    });

                    for (const auto& pair : components)
                    {
                        if (ui::MenuItem(pair.second_->GetTypeName().CString()))
                            selectedComponent_ = node->CreateComponent(pair.first_, alternative ? LOCAL : REPLICATED);
                    }

                    ui::EndMenu();
                }
            }

            ui::EndMenu();
        }

        ui::Separator();

        if (ui::MenuItem("Remove"))
        {
            Unselect(node);
            node->Remove();
        }
        ui::EndPopup();
    }

    if (opened)
    {
        for (auto& component: node->GetComponents())
        {
            if (component->IsTemporary())
                continue;

            ui::PushID(component);

            bool selected = selectedComponent_ == component;
            selected = ui::Selectable(component->GetTypeName().CString(), selected);

            if (ui::IsItemClicked(2))
            {
                selected = true;
                ui::OpenPopup("Component context menu");
            }

            if (selected)
            {
                UnselectAll();
                ToggleSelection(node);
                selectedComponent_ = component;
            }

            if (ui::BeginPopup("Component context menu"))
            {
                if (ui::MenuItem("Remove"))
                    component->Remove();
                ui::EndPopup();
            }

            ui::PopID();
        }

        for (auto& child: node->GetChildren())
            RenderSceneNodeTree(child);
        ui::TreePop();
    }
    else
        ui::PopID();
}

void SceneTab::LoadProject(XMLElement scene)
{
    id_ = StringHash(ToUInt(scene.GetAttribute("id"), 16));
    SetTitle(scene.GetAttribute("title"));
    LoadScene(scene.GetAttribute("path"));

    auto camera = scene.GetChild("camera");
    if (camera.NotNull())
    {
        Node* cameraNode = view_.GetCamera()->GetNode();
        if (auto position = camera.GetChild("position"))
            cameraNode->SetPosition(position.GetVariant().GetVector3());
        if (auto rotation = camera.GetChild("rotation"))
            cameraNode->SetRotation(rotation.GetVariant().GetQuaternion());
        if (auto light = camera.GetChild("light"))
            cameraNode->GetComponent<Light>()->SetEnabled(light.GetVariant().GetBool());
    }

    settings_->LoadProject(scene);
    effectSettings_->LoadProject(scene);

    undo_.Clear();
}

void SceneTab::SaveProject(XMLElement scene)
{
    scene.SetAttribute("id", id_.ToString().CString());
    scene.SetAttribute("title", title_);
    scene.SetAttribute("path", path_);

    auto camera = scene.CreateChild("camera");
    Node* cameraNode = view_.GetCamera()->GetNode();
    camera.CreateChild("position").SetVariant(cameraNode->GetPosition());
    camera.CreateChild("rotation").SetVariant(cameraNode->GetRotation());
    camera.CreateChild("light").SetVariant(cameraNode->GetComponent<Light>()->IsEnabled());

    settings_->SaveProject(scene);
    effectSettings_->SaveProject(scene);
    SaveScene();
}

void SceneTab::ClearCachedPaths()
{
    path_.Clear();
}

void SceneTab::OnActiveUpdate()
{
    if (GetSubsystem<SystemUI>()->IsAnyItemActive())
        return;

    Input* input = GetSubsystem<Input>();

    if (input->GetKeyDown(KEY_CTRL))
    {
        if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
            undo_.Redo();
        else if (input->GetKeyPress(KEY_Z))
            undo_.Undo();
    }

    if (input->GetKeyPress(KEY_DELETE))
        RemoveSelection();
}

void SceneTab::RemoveSelection()
{
    for (auto& selected : GetSelection())
        selected->Remove();
    UnselectAll();
}

}
