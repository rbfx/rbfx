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
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include "SceneTab.h"
#include "EditorEvents.h"
#include "Editor.h"
#include "Widgets.h"
#include "SceneSettings.h"
#include "Tabs/InspectorTab.h"
#include "Tabs/PreviewTab.h"
#include "Assets/Inspector/MaterialInspector.h"


namespace Urho3D
{

using namespace ui::litterals;

static const IntVector2 cameraPreviewSize{320, 200};

SceneTab::SceneTab(Context* context)
    : BaseClassName(context)
    , view_(context, {0, 0, 1024, 768})
    , gizmo_(context)
    , undo_(context)
    , clipboard_(context)
{
    SetTitle("New Scene");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    // Camera preview objects
    cameraPreviewViewport_ = new Viewport(context_);
    cameraPreviewViewport_->SetScene(view_.GetScene());
    cameraPreviewViewport_->SetRect(IntRect{{0, 0}, cameraPreviewSize});
    cameraPreviewViewport_->SetDrawDebug(false);
    cameraPreviewtexture_ = new Texture2D(context_);
    cameraPreviewtexture_->SetSize(cameraPreviewSize.x_, cameraPreviewSize.y_, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    cameraPreviewtexture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
    cameraPreviewtexture_->GetRenderSurface()->SetViewport(0, cameraPreviewViewport_);

    // Events
    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, std::bind(&SceneTab::OnNodeSelectionChanged, this));
    SubscribeToEvent(E_UPDATE, std::bind(&SceneTab::OnUpdate, this, _2));
    SubscribeToEvent(GetScene(), E_COMPONENTADDED, std::bind(&SceneTab::OnComponentAdded, this, _2));
    SubscribeToEvent(GetScene(), E_COMPONENTREMOVED, std::bind(&SceneTab::OnComponentRemoved, this, _2));

    SubscribeToEvent(E_SCENESETTINGMODIFIED, [this](StringHash, VariantMap& args) {
        using namespace SceneSettingModified;
        if (GetScene()->GetComponent<SceneSettings>() != GetEventSender())
            return;
        // TODO: Stinks.
        if (args[P_NAME].GetString() == "Editor Viewport RenderPath")
        {
            const ResourceRef& renderPathResource = args[P_VALUE].GetResourceRef();
            if (renderPathResource.type_ == XMLFile::GetTypeStatic())
            {
                if (XMLFile* renderPathFile = GetCache()->GetResource<XMLFile>(renderPathResource.name_))
                {
                    auto setRenderPathToViewport = [this, renderPathFile](Viewport* viewport)
                    {
                        if (!viewport->SetRenderPath(renderPathFile))
                            return;

                        RenderPath* path = viewport->GetRenderPath();
                        for (auto& command: path->commands_)
                        {
                            if (command.pixelShaderName_.StartsWith("PBR"))
                            {
                                XMLFile* gammaCorrection = GetCache()->GetResource<XMLFile>(
                                    "PostProcess/GammaCorrection.xml");
                                path->Append(gammaCorrection);
                                return;
                            }
                        }
                    };
                    setRenderPathToViewport(GetSceneView()->GetViewport());
                    setRenderPathToViewport(cameraPreviewViewport_);
                }
            }
        }
    });
    SubscribeToEvent(&inspector_, E_INSPECTORRENDERSTART, [this](StringHash, VariantMap& args) {
        Serializable* serializable = static_cast<Serializable*>(args[InspectorRenderStart::P_SERIALIZABLE].GetPtr());
        if (serializable->GetType() == Node::GetTypeStatic())
        {
            UI_UPIDSCOPE(1)
                ui::Columns(2);
            Node* node = static_cast<Node*>(serializable);
            ui::TextUnformatted("ID");
            ui::NextColumn();
            ui::Text("%u", node->GetID());
            if (node->IsReplicated())
            {
                ui::SameLine();
                ui::TextUnformatted(ICON_FA_WIFI);
                ui::SetHelpTooltip("Replicated over the network.", KEY_UNKNOWN);
            }
            ui::NextColumn();
        }
    });

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
    if (GetInput()->IsMouseVisible())
        lastMousePosition_ = GetInput()->GetMousePosition();
    bool open = true;

    // Focus window when appearing
    if (!isRendered_)
        ui::SetWindowFocus();

    RenderToolbarButtons();

    IntRect tabRect = UpdateViewRect();

    ui::SetCursorScreenPos(ToImGui(tabRect.Min()));
    ui::Image(view_.GetTexture(), ToImGui(tabRect.Size()));
    gizmo_.ManipulateSelection(view_.GetCamera());

    if (GetInput()->IsMouseVisible())
        mouseHoversViewport_ = ui::IsItemHovered();

    bool isClickedLeft = GetInput()->GetMouseButtonClick(MOUSEB_LEFT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    bool isClickedRight = GetInput()->GetMouseButtonClick(MOUSEB_RIGHT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    // Render camera preview
    if (cameraPreviewViewport_->GetCamera() != nullptr)
    {
        float border = 1;
        ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, border);
        ui::PushStyleColor(ImGuiCol_Border, ui::GetColorU32(ImGuiCol_Border, 255.f));                                   // Make a border opaque, otherwise it is barely visible.
        ui::SetCursorScreenPos(ToImGui(tabRect.Max() - cameraPreviewSize - IntVector2{10, 10}));

        ui::Image(cameraPreviewtexture_.Get(), ToImGui(cameraPreviewSize));
        ui::RenderFrameBorder(ui::GetItemRectMin() - ImVec2{border, border}, ui::GetItemRectMax() + ImVec2{border, border});

        ui::PopStyleColor();
        ui::PopStyleVar();
    }

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
                if (isClickedLeft)
                {
                    if (!GetInput()->GetKeyDown(KEY_CTRL))
                    {
                        UnselectAll();
                    }
                    ToggleSelection(clickNode);
                }
                else if (isClickedRight)
                {
                    if (!IsSelected(clickNode))
                    {
                        UnselectAll();
                        ToggleSelection(clickNode);
                    }
                    if (undo_.IsTrackingEnabled())
                        ui::OpenPopupEx(ui::GetID("Node context menu"));
                }
            }
        }
        else
            UnselectAll();
    }

    RenderNodeContextMenu();

    return open;
}

void SceneTab::OnBeforeBegin()
{
    // Allow viewport texture to cover entire window
    windowPadding_ = ui::GetStyle().WindowPadding;
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
}

void SceneTab::OnAfterBegin()
{
    // Inner part of window should have a proper padding, context menu and other controls might depend on it.
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding_);
    if (ui::BeginPopupContextItem("SceneTab context menu"))
    {
        if (ui::MenuItem("Save"))
            SaveResource();

        ui::Separator();

        if (ui::MenuItem("Close"))
            open_ = false;

        ui::EndPopup();
    }
}

void SceneTab::OnBeforeEnd()
{
    ui::PopStyleVar();  // ImGuiStyleVar_WindowPadding
}

void SceneTab::OnAfterEnd()
{
    ui::PopStyleVar();  // ImGuiStyleVar_WindowPadding
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
        {
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
            return false;
        }
    }
    else if (resourcePath.EndsWith(".json", false))
    {
        auto* file = GetCache()->GetResource<JSONFile>(resourcePath);
        if (file && GetScene()->LoadJSON(file->GetRoot()))
        {
            CreateObjects();
        }
        else
        {
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
            return false;
        }
    }
    else
    {
        URHO3D_LOGERRORF("Unknown scene file format %s", GetExtension(resourcePath).CString());
        return false;
    }

    SetTitle(GetFileName(resourcePath));

    // Components for custom scene settings
    GetScene()->GetOrCreateComponent<SceneSettings>(LOCAL, FIRST_INTERNAL_ID);

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

    float elapsed = GetScene()->GetElapsedTime();
    GetScene()->SetElapsedTime(0);
    GetScene()->SetUpdateEnabled(true);
    if (fullPath.EndsWith(".xml", false))
        result = GetScene()->SaveXML(file);
    else if (fullPath.EndsWith(".json", false))
        result = GetScene()->SaveJSON(file);
    GetScene()->SetUpdateEnabled(false);
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
    if (node == nullptr)
        return;

    if (gizmo_.Select(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::Select(Component* component)
{
    if (component == nullptr)
        return;

    selectedComponents_.Insert(WeakPtr<Component>(component));
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::Select(PODVector<Node*> nodes)
{
    if (nodes.Empty())
        return;

    if (gizmo_.Select(nodes))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::Unselect(Node* node)
{
    if (node == nullptr)
        return;

    if (gizmo_.Unselect(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::Unselect(Component* component)
{
    if (component == nullptr)
        return;

    if (selectedComponents_.Erase(WeakPtr<Component>(component)))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

void SceneTab::ToggleSelection(Node* node)
{
    if (node == nullptr)
        return;

    gizmo_.ToggleSelection(node);
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::ToggleSelection(Component* component)
{
    if (component == nullptr)
        return;

    WeakPtr<Component> componentPtr(component);
    if (selectedComponents_.Contains(componentPtr))
        selectedComponents_.Erase(componentPtr);
    else
        selectedComponents_.Insert(componentPtr);

    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::UnselectAll()
{
    bool hadComponents = !selectedComponents_.Empty();
    selectedComponents_.Clear();
    if (gizmo_.UnselectAll() || hadComponents)
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
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

    ui::SetCursorPos({4_dpx, 4_dpy});

    if (ui::EditorToolbarButton(ICON_FA_SAVE, "Save"))
        SaveResource();

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

    SendEvent(E_EDITORTOOLBARBUTTONS);

    ui::NewLine();
    ui::PopStyleVar();
}

bool SceneTab::IsSelected(Node* node) const
{
    if (node == nullptr)
        return false;

    return gizmo_.IsSelected(node);
}

bool SceneTab::IsSelected(Component* component) const
{
    if (component == nullptr)
        return false;

    return selectedComponents_.Contains(WeakPtr<Component>(component));
}

void SceneTab::OnNodeSelectionChanged()
{
    using namespace EditorSelectionChanged;
    UpdateCameraPreview();
}

void SceneTab::RenderInspector(const char* filter)
{
    for (auto& node : GetSelection())
    {
        if (node.Expired())
            continue;
        RenderAttributes(node.Get(), filter, &inspector_);
    }

    for (auto& component : selectedComponents_)
    {
        if (component.Expired())
            continue;
        RenderAttributes(component.Get(), filter, &inspector_);
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

    if (node == scrollTo_.Get())
        ui::SetScrollHereY();

    String name = node->GetName().Empty() ? ToString("%s %d", node->GetTypeName().CString(), node->GetID()) : node->GetName();
    bool isSelected = IsSelected(node);

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
        else if (ui::IsMouseClicked(MOUSEB_RIGHT) && undo_.IsTrackingEnabled())
        {
            if (!IsSelected(node))
            {
                UnselectAll();
                ToggleSelection(node);
            }
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

                bool selected = selectedComponents_.Contains(component);
                ui::Selectable(component->GetTypeName().CString(), selected);

                if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                {
                    if (ui::IsMouseClicked(MOUSEB_LEFT))
                    {
                        if (!GetInput()->GetKeyDown(KEY_CTRL))
                            UnselectAll();
                        Select(component);
                    }
                    else if (ui::IsMouseClicked(MOUSEB_RIGHT))
                    {
                        if (!IsSelected(component))
                        {
                            UnselectAll();
                            Select(component);
                        }
                        ui::OpenPopupEx(ui::GetID("Node context menu"));
                    }
                }


                RenderNodeContextMenu();

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
    }

    undo_.SetTrackingEnabled(isTracking);
}

void SceneTab::OnSaveProject(JSONValue& tab)
{
    BaseClassName::OnSaveProject(tab);

    auto& camera = tab["camera"];
    Node* cameraNode = view_.GetCamera()->GetNode();
    camera["position"].SetVariant(cameraNode->GetPosition());
    camera["rotation"].SetVariant(cameraNode->GetRotation());
}

void SceneTab::OnActiveUpdate()
{
    if (!ui::IsAnyItemActive() && undo_.IsTrackingEnabled())
    {
        // Global view hotkeys
        if (GetInput()->GetKeyPress(KEY_DELETE))
            RemoveSelection();
        else if (GetInput()->GetKeyDown(KEY_CTRL))
        {
            if (GetInput()->GetKeyPress(KEY_C))
                CopySelection();
            else if (GetInput()->GetKeyPress(KEY_V))
                PasteToSelection();
        }
        else if (GetInput()->GetKeyPress(KEY_ESCAPE))
            UnselectAll();
    }
}

void SceneTab::RemoveSelection()
{
    for (auto& component : selectedComponents_)
    {
        if (!component.Expired())
            component->Remove();
    }

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

    if (auto component = view_.GetCamera()->GetComponent<DebugCameraController>())
    {
        if (mouseHoversViewport_)
            component->Update(timeStep);
    }

    // Render editor camera rotation guide
    if (auto* debug = GetScene()->GetComponent<DebugRenderer>())
    {
        Vector3 guideRoot = GetSceneView()->GetCamera()->ScreenToWorldPoint({0.95, 0.1, 1});
        debug->AddLine(guideRoot, guideRoot + Vector3::RIGHT * 0.05f, Color::RED, false);
        debug->AddLine(guideRoot, guideRoot + Vector3::UP * 0.05f, Color::GREEN, false);
        debug->AddLine(guideRoot, guideRoot + Vector3::FORWARD * 0.05f, Color::BLUE, false);
    }
}

void SceneTab::SceneStateSave(VectorBuffer& destination)
{
    Undo::SetTrackingScoped tracking(undo_, false);

    // Preserve current selection
    savedNodeSelection_.Clear();
    savedComponentSelection_.Clear();
    for (auto& node : GetSelection())
    {
        if (node)
            savedNodeSelection_.Push(node->GetID());
    }
    for (auto& component : selectedComponents_)
    {
        if (component)
            savedComponentSelection_.Push(component->GetID());
    }

    // Ensure that editor objects are saved.
    PODVector<Node*> nodes;
    GetScene()->GetNodesWithTag(nodes, "__EDITOR_OBJECT__");
    for (auto* node : nodes)
        node->SetTemporary(false);

    destination.Clear();
    GetScene()->Save(destination);

    // Now that editor objects are saved make sure UI does not expose them
    for (auto* node : nodes)
        node->SetTemporary(true);
}

void SceneTab::SceneStateRestore(VectorBuffer& source)
{
    Undo::SetTrackingScoped tracking(undo_, false);

    source.Seek(0);
    GetScene()->Load(source);

    CreateObjects();

    // Ensure that editor objects are not saved in user scene.
    PODVector<Node*> nodes;
    GetScene()->GetNodesWithTag(nodes, "__EDITOR_OBJECT__");
    for (auto* node : nodes)
        node->SetTemporary(true);

    source.Clear();

    // Restore previous selection
    UnselectAll();
    for (unsigned id : savedNodeSelection_)
        Select(GetScene()->GetNode(id));

    for (unsigned id : savedComponentSelection_)
        Select(GetScene()->GetComponent(id));
    savedNodeSelection_.Clear();
    savedComponentSelection_.Clear();

    UpdateCameraPreview();
}

void SceneTab::RenderNodeContextMenu()
{
    if (undo_.IsTrackingEnabled() && (!GetSelection().Empty() || !selectedComponents_.Empty()) && ui::BeginPopup("Node context menu"))
    {
        Input* input = GetSubsystem<Input>();
        if (input->GetKeyPress(KEY_ESCAPE) || !input->IsMouseVisible())
        {
            // Close when interacting with scene camera.
            ui::CloseCurrentPopup();
            ui::EndPopup();
            return;
        }

        if (!GetSelection().Empty())
        {
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
                        openHierarchyNodes_.Push(newNodes.Back());
                        scrollTo_ = newNodes.Back();
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
                    auto components = editor->GetObjectsByCategory(category);
                    if (components.Empty())
                        continue;

                    if (ui::BeginMenu(category.CString()))
                    {
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
        }

        if (ui::MenuItem(ICON_FA_COPY " Copy", "Ctrl+C"))
            CopySelection();

        if (ui::MenuItem(ICON_FA_PASTE " Paste", "Ctrl+V"))
            PasteToSelection();

        if (ui::MenuItem(ICON_FA_TRASH " Delete", "Del"))
            RemoveSelection();

        ui::EndPopup();
    }
}

void SceneTab::OnComponentAdded(VariantMap& args)
{
    using namespace ComponentAdded;
    auto* component = static_cast<Component*>(args[P_COMPONENT].GetPtr());
    auto* node = static_cast<Node*>(args[P_NODE].GetPtr());

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
            billboard->SetViewMask(EDITOR_VIEW_LAYER);
            if (auto* bb = billboard->GetBillboard(0))
            {
                bb->size_ = Vector2::ONE * 0.2f;
                bb->enabled_ = true;
                bb->position_ = {0, count * 0.4f, 0};
            }
            billboard->Commit();
        }
    }

    UpdateCameraPreview();
}

void SceneTab::OnComponentRemoved(VariantMap& args)
{
    using namespace ComponentRemoved;
    auto* component = static_cast<Component*>(args[P_COMPONENT].GetPtr());
    auto* node = static_cast<Node*>(args[P_NODE].GetPtr());

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

    UpdateCameraPreview();
}

void SceneTab::OnFocused()
{
    if (InspectorTab* inspector = GetSubsystem<Editor>()->GetTab<InspectorTab>())
    {
        if (auto* inspectorProvider = dynamic_cast<MaterialInspector*>(inspector->GetInspector(IC_RESOURCE)))
            inspectorProvider->SetEffectSource(GetSceneView()->GetViewport()->GetRenderPath());
    }
}

void SceneTab::UpdateCameraPreview()
{
    cameraPreviewViewport_->SetCamera(nullptr);

    if (GetSelection().Size())
    {
        if (Node* node = GetSelection().At(0))
        {
            if (Camera* camera = node->GetComponent<Camera>())
            {
                camera->SetViewMask(camera->GetViewMask() & ~EDITOR_VIEW_LAYER);
                cameraPreviewViewport_->SetCamera(camera);
                cameraPreviewViewport_->SetRenderPath(GetSceneView()->GetViewport()->GetRenderPath());
            }
        }
    }
}

void SceneTab::CopySelection()
{
    clipboard_.Clear();
    clipboard_.Copy(GetSelection());
    clipboard_.Copy(selectedComponents_);
}

void SceneTab::PasteToSelection()
{
    const auto& selection = GetSelection();
    PasteResult result;
    if (!selection.Empty())
        result = clipboard_.Paste(selection);
    else
        result = clipboard_.Paste(GetScene());

    UnselectAll();

    for (Node* node : result.nodes_)
        Select(node);

    for (Component* component : result.components_)
        selectedComponents_.Insert(WeakPtr<Component>(component));
}

}
