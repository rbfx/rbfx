//
// Copyright (c) 2018 Rokas Kupstys
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

#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include <Toolbox/Scene/DebugCameraController.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <ImGuizmo/ImGuizmo.h>

#include "SceneTab.h"
#include "EditorEvents.h"
#include "Editor.h"
#include "Widgets.h"
#include "SceneSettings.h"
#include <ImGui/imgui_internal.h>


namespace Urho3D
{

SceneTab::SceneTab(Context* context)
    : BaseClassName(context)
    , view_(context, {0, 0, 1024, 768})
    , gizmo_(context)
    , undo_(context)
    , sceneState_(context)
{
    SetTitle("New Scene");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    settings_ = new SceneSettings(context_);
    effectSettings_ = new SceneEffects(this);

    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, std::bind(&SceneTab::OnNodeSelectionChanged, this));
//    SubscribeToEvent(effectSettings_, E_EDITORSCENEEFFECTSCHANGED, std::bind(&AttributeInspector::CopyEffectsFrom,
//        &inspector_, view_.GetViewport()));
    SubscribeToEvent(E_UPDATE, std::bind(&SceneTab::OnUpdate, this, _2));
    // On plugin code reload all scene state is serialized, plugin library is reloaded and scene state is unserialized.
    // This way scene recreates all plugin-provided components on reload and gets to use new versions of them.
    SubscribeToEvent(E_EDITORUSERCODERELOADSTART, [&](StringHash, VariantMap&) {
        undo_.SetTrackingEnabled(false);
        Pause();
        SceneStateSave();
        GetScene()->RemoveAllChildren();
        GetScene()->RemoveAllComponents();
    });
    SubscribeToEvent(E_EDITORUSERCODERELOADEND, [&](StringHash, VariantMap&) {
        SceneStateRestore(sceneState_);
        undo_.SetTrackingEnabled(true);
    });
    SubscribeToEvent(GetScene(), E_COMPONENTADDED, std::bind(&SceneTab::OnComponentAdded, this, _2));
    SubscribeToEvent(GetScene(), E_COMPONENTREMOVED, std::bind(&SceneTab::OnComponentRemoved, this, _2));

    undo_.Connect(GetScene());
    undo_.Connect(&inspector_);
    undo_.Connect(&gizmo_);

    SubscribeToEvent(GetScene(), E_ASYNCLOADFINISHED, [&](StringHash, VariantMap&) {
        undo_.Clear();
    });

    // Scene is updated manually.
    GetScene()->SetUpdateEnabled(false);

    CreateObjects();
    undo_.Clear();

    UpdateUniqueTitle();
}

SceneTab::~SceneTab() = default;

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

    RenderToolbarButtons();
    IntRect tabRect = UpdateViewRect();

    ui::SetCursorScreenPos(ToImGui(tabRect.Min()));
    ui::Image(view_.GetTexture(), ToImGui(tabRect.Size()));
    if (GetInput()->IsMouseVisible())
        mouseHoversViewport_ = ui::IsItemHovered();

    bool isClickedLeft = GetInput()->GetMouseButtonClick(MOUSEB_LEFT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    bool isClickedRight = GetInput()->GetMouseButtonClick(MOUSEB_RIGHT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    gizmo_.ManipulateSelection(view_.GetCamera());

    // Prevent dragging window when scene view is clicked.
    if (ui::IsWindowHovered())
        windowFlags_ |= ImGuiWindowFlags_NoMove;
    else
        windowFlags_ &= ~ImGuiWindowFlags_NoMove;

    if (!gizmo_.IsActive() && (isClickedLeft || isClickedRight) && GetInput()->IsMouseVisible())
    {
        // Handle object selection.
        IntVector2 pos = GetInput()->GetMousePosition();
        pos -= tabRect.Min();

        Ray cameraRay = view_.GetCamera()->GetScreenRay((float)pos.x_ / tabRect.Width(), (float)pos.y_ / tabRect.Height());
        // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
        PODVector<RayQueryResult> results;

        RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
        GetScene()->GetComponent<Octree>()->RaycastSingle(query);

        if (!results.Size())
        {
            // When object geometry was not hit by a ray - query for object bounding box.
            RayOctreeQuery query2(results, cameraRay, RAY_OBB, M_INFINITY, DRAWABLE_GEOMETRY);
            GetScene()->GetComponent<Octree>()->RaycastSingle(query2);
        }

        if (results.Size())
        {
            WeakPtr<Node> clickNode(results[0].drawable_->GetNode());
            // Temporary nodes can not be selected.
            while (!clickNode.Expired() && clickNode->HasTag("__EDITOR_OBJECT__"))
                clickNode = clickNode->GetParent();

            if (!clickNode.Expired())
            {
                bool appendSelection = GetInput()->GetKeyDown(KEY_CTRL);
                if (!appendSelection)
                    UnselectAll();
                ToggleSelection(clickNode);

                if (isClickedRight)
                    ui::OpenPopupEx(ui::GetID("Node context menu"));
            }
        }
        else
            UnselectAll();
    }

    RenderNodeContextMenu();

    const auto tabContextMenuTitle = "SceneTab context menu";
    if (ui::IsDockTabHovered() && GetInput()->GetMouseButtonPress(MOUSEB_RIGHT))
        ui::OpenPopup(tabContextMenuTitle);
    if (ui::BeginPopup(tabContextMenuTitle))
    {
        if (ui::MenuItem("Save"))
            SaveResource();

        ui::Separator();

        if (ui::MenuItem("Close"))
            open = false;

        ui::EndPopup();
    }

    return open;
}

bool SceneTab::LoadResource(const String& resourcePath)
{
    if (!BaseClassName::LoadResource(resourcePath))
        return false;

    if (resourcePath.EndsWith(".xml", false))
    {
        auto* file = GetCache()->GetResource<XMLFile>(resourcePath);
        if (file && GetScene()->LoadXML(file->GetRoot()))
        {
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
    }
    else if (resourcePath.EndsWith(".json", false))
    {
        auto* file = GetCache()->GetResource<JSONFile>(resourcePath);
        if (file && GetScene()->LoadJSON(file->GetRoot()))
        {
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
    }
    else if (resourcePath.EndsWith(".scene", false))
    {
        auto* file = GetCache()->GetResource<YAMLFile>(resourcePath);
        if (file && GetScene()->LoadJSON(file->GetRoot()))
        {
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
    }
    else
        URHO3D_LOGERRORF("Unknown scene file format %s", GetExtension(resourcePath).CString());

    SetTitle(GetFileName(resourcePath));
    return true;
}

bool SceneTab::SaveResource()
{
    if (!BaseClassName::SaveResource())
        return false;

    GetCache()->IgnoreResourceReload(resourceName_);

    auto fullPath = GetCache()->GetResourceFileName(resourceName_);
    if (fullPath.Empty())
        return false;

    File file(context_, fullPath, FILE_WRITE);
    bool result = false;

    float elapsed = 0;
    if (!settings_->saveElapsedTime_)
    {
        elapsed = GetScene()->GetElapsedTime();
        GetScene()->SetElapsedTime(0);
    }

    GetScene()->SetUpdateEnabled(true);
    if (fullPath.EndsWith(".xml", false))
        result = GetScene()->SaveXML(file);
    else if (fullPath.EndsWith(".json", false))
        result = GetScene()->SaveJSON(file);
    else if (fullPath.EndsWith(".scene", false))
        result = GetScene()->SaveYAML(file);
    GetScene()->SetUpdateEnabled(false);

    if (!settings_->saveElapsedTime_)
        GetScene()->SetElapsedTime(elapsed);

    if (result)
        SendEvent(E_EDITORRESOURCESAVED);
    else
        URHO3D_LOGERRORF("Saving scene to %s failed.", resourceName_.CString());

    return result;
}

void SceneTab::CreateObjects()
{
    auto isTracking = undo_.IsTrackingEnabled();
    undo_.SetTrackingEnabled(false);
    view_.CreateObjects();
    view_.GetCamera()->GetNode()->GetOrCreateComponent<DebugCameraController>();
    undo_.SetTrackingEnabled(isTracking);
}

void SceneTab::Select(Node* node)
{
    if (gizmo_.Select(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::Select(PODVector<Node*> nodes)
{
    if (gizmo_.Select(nodes))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::Unselect(Node* node)
{
    if (gizmo_.Unselect(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::ToggleSelection(Node* node)
{
    gizmo_.ToggleSelection(node);
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::UnselectAll()
{
    if (gizmo_.UnselectAll())
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

const Vector<WeakPtr<Node>>& SceneTab::GetSelection() const
{
    return gizmo_.GetSelection();
}

void SceneTab::RenderToolbarButtons()
{
    auto& style = ui::GetStyle();
    auto oldRounding = style.FrameRounding;
    style.FrameRounding = 0;

    if (ui::EditorToolbarButton(ICON_FA_SAVE, "Save"))
        SaveResource();

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_UNDO, "Undo"))
        undo_.Undo();
    if (ui::EditorToolbarButton(ICON_FA_REDO, "Redo"))
        undo_.Redo();

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT, "Translate", gizmo_.GetOperation() == GIZMOOP_TRANSLATE))
        gizmo_.SetOperation(GIZMOOP_TRANSLATE);
    if (ui::EditorToolbarButton(ICON_FA_SYNC, "Rotate", gizmo_.GetOperation() == GIZMOOP_ROTATE))
        gizmo_.SetOperation(GIZMOOP_ROTATE);
    if (ui::EditorToolbarButton(ICON_FA_EXPAND_ARROWS_ALT, "Scale", gizmo_.GetOperation() == GIZMOOP_SCALE))
        gizmo_.SetOperation(GIZMOOP_SCALE);

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT, "World", gizmo_.GetTransformSpace() == TS_WORLD))
        gizmo_.SetTransformSpace(TS_WORLD);
    if (ui::EditorToolbarButton(ICON_FA_EXPAND_ARROWS_ALT, "Local", gizmo_.GetTransformSpace() == TS_LOCAL))
        gizmo_.SetTransformSpace(TS_LOCAL);

    ui::SameLine(0, 3.f);

    if (auto* light = view_.GetCamera()->GetNode()->GetComponent<Light>())
    {
        if (ui::EditorToolbarButton(ICON_FA_LIGHTBULB, "Camera Headlight", light->IsEnabled()))
            light->SetEnabled(!light->IsEnabled());
    }

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(scenePlaying_ ? ICON_FA_PAUSE : ICON_FA_PLAY, scenePlaying_ ? "Pause" : "Play"))
    {
        scenePlaying_ ^= true;
        if (scenePlaying_)
            Pause();
        else
            Play();
    }

    SendEvent(E_EDITORTOOLBARBUTTONS);

    ui::NewLine();
    style.FrameRounding = oldRounding;
}

bool SceneTab::IsSelected(Node* node) const
{
    return gizmo_.IsSelected(node);
}

void SceneTab::OnNodeSelectionChanged()
{
    using namespace EditorSelectionChanged;
    selectedComponent_ = nullptr;
}

void SceneTab::RenderInspector()
{
    // TODO: inspector for multi-selection.
    if (GetSelection().Size() == 1)
    {
        auto node = GetSelection().Front();
        if (node.Expired())
            return;

        PODVector<Serializable*> items;
        items.Push(node.Get());
        if (node == GetScene())
        {
            effectSettings_->Prepare();
            items.Push(settings_.Get());
            items.Push(effectSettings_.Get());
        }
        for (Component* component : node->GetComponents())
            items.Push(component);
        inspector_.RenderAttributes(items);
    }
}

void SceneTab::RenderHierarchy()
{
    auto oldSpacing = ui::GetStyle().IndentSpacing;
    ui::GetStyle().IndentSpacing = 10;
    RenderNodeTree(GetScene());
    ui::GetStyle().IndentSpacing = oldSpacing;
}

void SceneTab::RenderNodeTree(Node* node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (node->IsTemporary())
        return;

    String name = node->GetName().Empty() ? ToString("%s %d", node->GetTypeName().CString(), node->GetID()) : node->GetName();
    bool isSelected = IsSelected(node) && selectedComponent_.Expired();

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    ui::Image("Node");
    ui::SameLine();
    ui::PushID((void*)node);
    auto opened = ui::TreeNodeEx(name.CString(), flags);
    auto it = openHierarchyNodes_.Find(node);
    if (it != openHierarchyNodes_.End())
    {
        if (!opened)
        {
            ui::OpenTreeNode(ui::GetCurrentWindow()->GetID(name.CString()));
            opened = true;
        }
        openHierarchyNodes_.Erase(it);
    }

    if (ui::BeginDragDropSource())
    {
        ui::SetDragDropVariant("ptr", node);
        ui::Text("%s", name.CString());
        ui::EndDragDropSource();
    }

    if (ui::BeginDragDropTarget())
    {
        const Variant& payload = ui::AcceptDragDropVariant("ptr");
        if (!payload.IsEmpty())
        {
            SharedPtr<Node> child(dynamic_cast<Node*>(payload.GetPtr()));
            if (child.NotNull() && child != node)
            {
                node->AddChild(child);
                if (!opened)
                    openHierarchyNodes_.Push(node);
            }
        }
        ui::EndDragDropTarget();
    }

    if (!opened)
    {
        // If TreeNode above is opened, it pushes it's label as an ID to the stack. However if it is not open then no
        // ID is pushed. This creates a situation where context menu is not properly attached to said tree node due to
        // missing ID on the stack. To correct this we ensure that ID is always pushed. This allows us to show context
        // menus even for closed tree nodes.
        ui::PushID(name.CString());
    }

    // Popup may delete node. Weak reference will convey that information.
    WeakPtr<Node> nodeRef(node);

    if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        if (ui::IsMouseClicked(MOUSEB_LEFT))
        {
            if (!GetInput()->GetKeyDown(KEY_CTRL))
                UnselectAll();
            ToggleSelection(node);
        }
        else if (ui::IsMouseClicked(MOUSEB_RIGHT) && !scenePlaying_)
        {
            UnselectAll();
            ToggleSelection(node);
            ui::OpenPopupEx(ui::GetID("Node context menu"));
        }
    }

    RenderNodeContextMenu();

    if (opened)
    {
        if (!nodeRef.Expired())
        {
            Vector<SharedPtr<Component>> components = node->GetComponents();
            for (const auto& component: components)
            {
                if (component->IsTemporary())
                    continue;

                ui::PushID(component);

                ui::Image(component->GetTypeName());
                ui::SameLine();

                bool selected = selectedComponent_ == component;
                selected = ui::Selectable(component->GetTypeName().CString(), selected);

                if (ui::IsMouseClicked(MOUSEB_RIGHT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                {
                    selected = true;
                    ui::OpenPopupEx(ui::GetID("Component context menu"));
                }

                if (selected)
                {
                    UnselectAll();
                    ToggleSelection(node);
                    selectedComponent_ = component;
                }

                if (ui::BeginPopup("Component context menu"))
                {
                    if (ui::MenuItem("Delete", "Del"))
                        component->Remove();
                    ui::EndPopup();
                }

                ui::PopID();
            }

            // Do not use element->GetChildren() because child may be deleted during this loop.
            PODVector<Node*> children;
            node->GetChildren(children);
            for (Node* child: children)
                RenderNodeTree(child);
        }
        ui::TreePop();
    }
    else
        ui::PopID();
    ui::PopID();
}

void SceneTab::OnLoadProject(const JSONValue& tab)
{
    undo_.Clear();
    auto isTracking = undo_.IsTrackingEnabled();
    undo_.SetTrackingEnabled(false);

    BaseClassName::OnLoadProject(tab);

    const auto& camera = tab["camera"];
    if (camera.IsObject())
    {
        Node* cameraNode = view_.GetCamera()->GetNode();
        cameraNode->SetPosition(camera["position"].GetVariant().GetVector3());
        cameraNode->SetRotation(camera["rotation"].GetVariant().GetQuaternion());
        if (auto* lightComponent = cameraNode->GetComponent<Light>())
            lightComponent->SetEnabled(camera["light"].GetBool());
    }

    settings_->LoadProject(tab["settings"]);
    effectSettings_->LoadProject(tab["effects"]);

    undo_.SetTrackingEnabled(isTracking);
}

void SceneTab::OnSaveProject(JSONValue& tab)
{
    BaseClassName::OnSaveProject(tab);

    auto& camera = tab["camera"];
    Node* cameraNode = view_.GetCamera()->GetNode();
    camera["position"].SetVariant(cameraNode->GetPosition());
    camera["rotation"].SetVariant(cameraNode->GetRotation());
    camera["light"] = cameraNode->GetComponent<Light>()->IsEnabled();

    settings_->SaveProject(tab["settings"]);
    effectSettings_->SaveProject(tab["effects"]);
}

void SceneTab::OnActiveUpdate()
{
    // Scene viewport hotkeys
    if (!ui::IsAnyItemActive() && !scenePlaying_)
    {
        Input* input = GetSubsystem<Input>();

        if (input->GetKeyDown(KEY_CTRL))
        {
            if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
                undo_.Redo();
            else if (input->GetKeyPress(KEY_Z))
                undo_.Undo();
        }
    }
}

void SceneTab::RemoveSelection()
{
    if (!selectedComponent_.Expired())
        selectedComponent_->Remove();

    for (auto& selected : GetSelection())
    {
        if (!selected.Expired())
            selected->Remove();
    }
    UnselectAll();
}

IntRect SceneTab::UpdateViewRect()
{
    IntRect tabRect = BaseClassName::UpdateViewRect();
    view_.SetSize(tabRect);
    gizmo_.SetScreenRect(tabRect);
    return tabRect;
}

void SceneTab::OnUpdate(VariantMap& args)
{
    float timeStep = args[Update::P_TIMESTEP].GetFloat();

    if (scenePlaying_)
        GetScene()->Update(timeStep);

    if (auto component = view_.GetCamera()->GetComponent<DebugCameraController>())
    {
        if (mouseHoversViewport_)
            component->Update(timeStep);
    }

    if (!ui::IsAnyItemActive() && !scenePlaying_)
    {
        // Global view hotkeys
        if (GetInput()->GetKeyDown(KEY_DELETE))
            RemoveSelection();
    }
}

void SceneTab::SceneStateSave()
{
    Undo::SetTrackingScoped tracking(undo_, false);

    for (auto& node : GetSelection())
    {
        if (node)
            node->AddTag("__EDITOR_SELECTED__");
    }

    // Ensure that editor objects are saved.
    PODVector<Node*> nodes;
    GetScene()->GetNodesWithTag(nodes, "__EDITOR_OBJECT__");
    for (auto* node : nodes)
        node->SetTemporary(false);

    sceneState_.GetRoot().Remove();
    XMLElement root = sceneState_.CreateRoot("scene");
    GetScene()->SaveXML(root);

    // Now that editor objects are saved make sure UI does not expose them
    for (auto* node : nodes)
        node->SetTemporary(true);
}

void SceneTab::SceneStateRestore(XMLFile& source)
{
    Undo::SetTrackingScoped tracking(undo_, false);

    GetScene()->LoadXML(source.GetRoot());

    CreateObjects();

    // Ensure that editor objects are not saved in user scene.
    PODVector<Node*> nodes;
    GetScene()->GetNodesWithTag(nodes, "__EDITOR_OBJECT__");
    for (auto* node : nodes)
        node->SetTemporary(true);

    source.GetRoot().Remove();

    gizmo_.UnselectAll();
    for (auto node : GetScene()->GetChildrenWithTag("__EDITOR_SELECTED__", true))
    {
        gizmo_.Select(node);
        node->RemoveTag("__EDITOR_SELECTED__");
    }
}

void SceneTab::RenderNodeContextMenu()
{
    if (ui::BeginPopup("Node context menu") && !scenePlaying_)
    {
        Input* input = GetSubsystem<Input>();
        if (input->GetKeyPress(KEY_ESCAPE) || !input->IsMouseVisible())
        {
            // Close when interacting with scene camera.
            ui::CloseCurrentPopup();
            ui::EndPopup();
            return;
        }

        bool alternative = input->GetKeyDown(KEY_SHIFT);

        if (ui::MenuItem(alternative ? "Create Child (Local)" : "Create Child"))
        {
            PODVector<Node*> newNodes;
            for (auto& selectedNode : GetSelection())
            {
                if (!selectedNode.Expired())
                {
                    newNodes.Push(selectedNode->CreateChild(String::EMPTY, alternative ? LOCAL : REPLICATED));
                    openHierarchyNodes_.Push(selectedNode);
                }
            }

            UnselectAll();
            Select(newNodes);
        }

        if (ui::BeginMenu(alternative ? "Create Component (Local)" : "Create Component"))
        {
            auto* editor = GetSubsystem<Editor>();
            auto categories = context_->GetObjectCategories().Keys();
            categories.Remove("UI");

            for (const String& category : categories)
            {
                if (ui::BeginMenu(category.CString()))
                {
                    auto components = editor->GetObjectsByCategory(category);
                    Sort(components.Begin(), components.End());

                    for (const String& component : components)
                    {
                        ui::Image(component);
                        ui::SameLine();
                        if (ui::MenuItem(component.CString()))
                        {
                            for (auto& selectedNode : GetSelection())
                            {
                                if (!selectedNode.Expired())
                                {
                                    if (selectedNode->CreateComponent(StringHash(component),
                                        alternative ? LOCAL : REPLICATED))
                                        openHierarchyNodes_.Push(selectedNode);
                                }
                            }
                        }
                    }
                    ui::EndMenu();
                }
            }
            ui::EndMenu();
        }

        ui::Separator();

        if (ui::MenuItem("Remove"))
            RemoveSelection();

        ui::EndPopup();
    }
}

void SceneTab::Play()
{
    if (!scenePlaying_)
    {
        SceneStateRestore(sceneState_);
        undo_.SetTrackingEnabled(true);
    }
}

void SceneTab::Pause()
{
    if (scenePlaying_)
    {
        undo_.SetTrackingEnabled(false);
        SceneStateSave();
        gizmo_.UnselectAll();
    }
}

void SceneTab::OnComponentAdded(VariantMap& args)
{
    using namespace ComponentAdded;
    auto* component = dynamic_cast<Component*>(args[P_COMPONENT].GetPtr());
    auto* node = dynamic_cast<Node*>(args[P_NODE].GetPtr());

    if (node->IsTemporary() || node->HasTag("__EDITOR_OBJECT__"))
        return;

    auto* material = GetCache()->GetResource<Material>("Materials/Editor/DebugIcon" + component->GetTypeName() + ".xml", false);
    if (material != nullptr)
    {
        if (node->GetChildrenWithTag("DebugIcon" + component->GetTypeName()).Size() > 0)
            return;

        auto iconTag = "DebugIcon" + component->GetTypeName();
        if (node->GetChildrenWithTag(iconTag).Empty())
        {
            Undo::SetTrackingScoped tracking(undo_, false);
            int count = node->GetChildrenWithTag("DebugIcon").Size();
            node = node->CreateChild();
            node->AddTag("DebugIcon");
            node->AddTag("DebugIcon" + component->GetTypeName());
            node->AddTag("__EDITOR_OBJECT__");
            node->SetTemporary(true);

            auto* billboard = node->CreateComponent<BillboardSet>();
            billboard->SetFaceCameraMode(FaceCameraMode::FC_LOOKAT_XYZ);
            billboard->SetNumBillboards(1);
            billboard->SetMaterial(material);
            if (auto* bb = billboard->GetBillboard(0))
            {
                bb->size_ = Vector2::ONE * 0.2f;
                bb->enabled_ = true;
                bb->position_ = {0, count * 0.4f, 0};
            }
            billboard->Commit();
        }
    }
}

void SceneTab::OnComponentRemoved(VariantMap& args)
{
    using namespace ComponentRemoved;
    auto* component = dynamic_cast<Component*>(args[P_COMPONENT].GetPtr());
    auto* node = dynamic_cast<Node*>(args[P_NODE].GetPtr());

    if (!node->IsTemporary())
    {
        Undo::SetTrackingScoped tracking(undo_, false);

        for (auto* icon : node->GetChildrenWithTag("DebugIcon" + component->GetTypeName()))
            icon->Remove();

        int index = 0;
        for (auto* icon : node->GetChildrenWithTag("DebugIcon"))
        {
            if (auto* billboard = icon->GetComponent<BillboardSet>())
            {
                billboard->GetBillboard(0)->position_ = {0, index * 0.4f, 0};
                billboard->Commit();
                index++;
            }
        }
    }
}

void SceneTab::OnFocused()
{
    SendEvent(E_EDITORRENDERINSPECTOR, EditorRenderInspector::P_INSPECTABLE, this);
    SendEvent(E_EDITORRENDERHIERARCHY, EditorRenderHierarchy::P_INSPECTABLE, this);
}

}
