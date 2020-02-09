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

#include <EASTL/sort.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SceneManager.h>
#include <Urho3D/SystemUI/DebugHud.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Toolbox/Scene/DebugCameraController.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include "SceneTab.h"
#include "EditorEvents.h"
#include "Editor.h"
#include "Widgets.h"
#include "EditorSceneSettings.h"
#include "Tabs/HierarchyTab.h"
#include "Tabs/InspectorTab.h"
#include "Tabs/PreviewTab.h"
#include "Inspector/MaterialInspector.h"


namespace Urho3D
{

static const IntVector2 cameraPreviewSize{320, 200};

SceneTab::SceneTab(Context* context)
    : BaseClassName(context)
    , rect_({0, 0, 1024, 768})
    , gizmo_(context)
    , clipboard_(context)
{
    SetID("be1c7280-08e4-4b9f-b6ec-6d32bd9e3293");
    SetTitle("Scene");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    isUtility_ = true;
    noContentPadding_ = true;

    // Main viewport
    viewport_ = new Viewport(context_);
    viewport_->SetRect(rect_);
    texture_ = new Texture2D(context_);
    texture_->SetSize(rect_.Width(), rect_.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    texture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
    texture_->GetRenderSurface()->SetViewport(0, viewport_);

    rootElement_ = new RootUIElement(context_);
    rootElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootElement_->SetEnabled(true);

    offScreenUI_ = new UI(context_);
    offScreenUI_->SetRoot(rootElement_);
    offScreenUI_->SetRenderInSystemUI(true);
    offScreenUI_->SetBlockEvents(true);
    context_->RegisterSubsystem(offScreenUI_);

    // Camera preview objects
    cameraPreviewViewport_ = new Viewport(context_);
    cameraPreviewViewport_->SetRect(IntRect{{0, 0}, cameraPreviewSize});
    cameraPreviewViewport_->SetDrawDebug(false);
    cameraPreviewtexture_ = new Texture2D(context_);
    cameraPreviewtexture_->SetSize(cameraPreviewSize.x_, cameraPreviewSize.y_, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    cameraPreviewtexture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
    cameraPreviewtexture_->GetRenderSurface()->SetViewport(0, cameraPreviewViewport_);

    // Events
    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, [this](StringHash, VariantMap&) { OnNodeSelectionChanged(); });
    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap& args) { OnUpdate(args); });
    SubscribeToEvent(E_POSTRENDERUPDATE, [&](StringHash, VariantMap&) { RenderDebugInfo(); });
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { UpdateCameras(); });
    SubscribeToEvent(E_SCENEACTIVATED, [this](StringHash, VariantMap& args) { OnSceneActivated(args); });
    SubscribeToEvent(E_EDITORPROJECTCLOSING, [this](StringHash, VariantMap&) { OnEditorProjectClosing(); });

    undo_->Connect(&gizmo_);

    UpdateUniqueTitle();
}

SceneTab::~SceneTab()
{
    Close();
    context_->RegisterSubsystem(new UI(context_));
}

bool SceneTab::RenderWindowContent()
{
    ImGuiContext& g = *GImGui;
    bool open = true;

    if (GetScene() == nullptr)
        return true;

    // Focus window when appearing
    if (!isRendered_)
        ui::SetWindowFocus();

    if (!ui::BeginChild("Scene view", g.CurrentWindow->ContentRegionRect.GetSize(), false, windowFlags_))
    {
        ui::EndChild();
        return open;
    }

    ImGuiWindow* window = g.CurrentWindow;
    ImGuiViewport* viewport = window->Viewport;

    ImRect rect = ImRound(window->ContentRegionRect);
    IntVector2 textureSize{
        static_cast<int>(IM_ROUND(rect.GetWidth() * viewport->DpiScale)),
        static_cast<int>(IM_ROUND(rect.GetHeight() * viewport->DpiScale))
    };
    if (textureSize.x_ != texture_->GetWidth() || textureSize.y_ != texture_->GetHeight())
    {
        viewport_->SetRect(IntRect(IntVector2::ZERO, textureSize));
        texture_->SetSize(textureSize.x_, textureSize.y_, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        texture_->GetRenderSurface()->SetViewport(0, viewport_);
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
    }

    Input* input = GetSubsystem<Input>();
    bool wasActive = isViewportActive_;
    bool isClickedLeft = false;
    bool isClickedRight = false;

    // Buttons must render above the viewport, but they also should be rendered first in order to take precedence in
    // item activation.
    viewportSplitter_.Split(window->DrawList, 3);
    viewportSplitter_.SetCurrentChannel(window->DrawList, 2);
    RenderToolbarButtons();
    viewportSplitter_.SetCurrentChannel(window->DrawList, 1);
    gizmo_.ManipulateSelection(GetCamera());
    viewportSplitter_.SetCurrentChannel(window->DrawList, 0);
    ui::SetCursorPos({0, 0});
    ui::ImageItem(texture_, rect.GetSize());
    isViewportActive_ = ui::ItemMouseActivation(MOUSEB_RIGHT) && ui::IsMouseDragging(MOUSEB_RIGHT);
    isClickedLeft = ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_LEFT);
    isClickedRight = ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_RIGHT) && input->IsMouseVisible();
    ImRect viewportRect{ui::GetItemRectMin(), ui::GetItemRectMax()};
    viewportSplitter_.Merge(window->DrawList);

    if (wasActive != isViewportActive_)
        GetSubsystem<Input>()->SetMouseVisible(!isViewportActive_);

    // Render camera preview
    if (cameraPreviewViewport_->GetCamera() != nullptr)
    {
        float border = 1;
        ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, border);
        ui::PushStyleColor(ImGuiCol_Border, ui::GetColorU32(ImGuiCol_Border, 255.f));                                   // Make a border opaque, otherwise it is barely visible.
        ui::SetCursorScreenPos(rect.Max - ToImGui(cameraPreviewSize) - ImVec2{10, 10});

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

    if (!gizmo_.IsActive() && (isClickedLeft || isClickedRight) && context_->GetInput()->IsMouseVisible())
    {
        // Handle object selection.
        ImGuiIO& io = ui::GetIO();
        Ray cameraRay = GetCamera()->GetScreenRay(
            (io.MousePos.x - viewportRect.Min.x) / viewportRect.GetWidth(),
            (io.MousePos.y - viewportRect.Min.y) / viewportRect.GetHeight());
        // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
        ea::vector<RayQueryResult> results;

        RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
        GetScene()->GetComponent<Octree>()->RaycastSingle(query);

        if (!results.size())
        {
            // When object geometry was not hit by a ray - query for object bounding box.
            RayOctreeQuery query2(results, cameraRay, RAY_OBB, M_INFINITY, DRAWABLE_GEOMETRY);
            GetScene()->GetComponent<Octree>()->RaycastSingle(query2);
        }

        if (results.size() && results[0].drawable_->GetNode() != nullptr)
        {
            StringHash componentType;
            WeakPtr<Node> clickNode(results[0].drawable_->GetNode());

            if (clickNode->HasTag("DebugIcon"))
                componentType = clickNode->GetVar("ComponentType").GetStringHash();

            while (!clickNode.Expired() && clickNode->HasTag("__EDITOR_OBJECT__"))
                clickNode = clickNode->GetParent();

            if (isClickedLeft)
            {
                if (!context_->GetInput()->GetKeyDown(KEY_CTRL))
                    UnselectAll();

                if (clickNode == GetScene())
                {
                    if (componentType != StringHash::ZERO)
                        ToggleSelection(clickNode->GetComponent(componentType));
                }
                else
                    ToggleSelection(clickNode);
            }
            else if (isClickedRight && ImLengthSqr(ui::GetMouseDragDelta(MOUSEB_RIGHT)) == 0.0f)
            {
                if (clickNode == GetScene())
                {
                    if (componentType != StringHash::ZERO)
                    {
                        Component* component = clickNode->GetComponent(componentType);
                        if (!IsSelected(component))
                        {
                            UnselectAll();
                            ToggleSelection(component);
                        }
                    }
                }
                else if (!IsSelected(clickNode))
                {
                    UnselectAll();
                    ToggleSelection(clickNode);
                }
                ui::OpenPopupEx(ui::GetID("Node context menu"));
            }
        }
        else
            UnselectAll();
    }

    RenderNodeContextMenu();

    if (debugHudVisible_)
    {
        if (auto* hud = GetSubsystem<DebugHud>())
        {
            ImFont* monospaceFont = GetSubsystem<Editor>()->GetMonoSpaceFont();
            ui::PushFont(monospaceFont);
            ImGuiWindow* window = ui::GetCurrentWindow();
            // FIXME: Without this FPS has extra space below.
            ui::GetCurrentWindow()->DC.CurrLineSize.y = monospaceFont->FontSize;
            ui::SetCursorScreenPos(rect.Min + ImVec2{10, 10 + ui::GetIO().Fonts->Fonts[0]->FontSize});
            hud->RenderUI(DEBUGHUD_SHOW_ALL);
            ui::PopFont();
        }
    }

    ui::EndChild(); // Scene view

    BaseClassName::RenderWindowContent();

    return open;
}

bool SceneTab::LoadResource(const ea::string& resourcePath)
{
    UndoTrackGuard noTrack(undo_, false);

    if (resourcePath == GetResourceName())
        // Already loaded.
        return true;

    if (!BaseClassName::LoadResource(resourcePath))
        return false;

    auto cache = GetSubsystem<ResourceCache>();
    auto manager = GetSubsystem<SceneManager>();
    Scene* scene = manager->GetOrCreateScene(GetFileName(resourcePath));

    UndoTrackGuard(undo_, false);
    undo_->Connect(scene);

    manager->SetActiveScene(scene);

    bool loaded = false;
    if (resourcePath.ends_with(".xml", false))
    {
        auto* file = cache->GetResource<XMLFile>(resourcePath);
        loaded = file && scene->LoadXML(file->GetRoot());
        if (file)
            scene->SetFileName(file->GetNativeFileName());
    }
    else if (resourcePath.ends_with(".json", false))
    {
        auto* file = cache->GetResource<JSONFile>(resourcePath);
        loaded = file && scene->LoadJSON(file->GetRoot());
        if (file)
            scene->SetFileName(file->GetNativeFileName());
    }
    else
    {
        URHO3D_LOGERRORF("Unknown scene file format %s", GetExtension(resourcePath).c_str());
        manager->UnloadScene(scene);
        cache->ReleaseResource(XMLFile::GetTypeStatic(), resourcePath, true);
        return false;
    }

    if (!loaded)
    {
        URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).c_str());
        manager->UnloadScene(scene);
        cache->ReleaseResource(XMLFile::GetTypeStatic(), resourcePath, true);
        return false;
    }

    manager->UnloadAllButActiveScene();
    scene->SetUpdateEnabled(false);    // Scene is updated manually.
    scene->GetOrCreateComponent<Octree>();
    scene->GetOrCreateComponent<EditorSceneSettings>(LOCAL);

    lastUndoIndex_ = undo_->Index();

    GetSubsystem<Editor>()->UpdateWindowTitle(resourcePath);

    return true;
}

bool SceneTab::SaveResource()
{
    if (GetScene() == nullptr)
        return false;

    if (!BaseClassName::SaveResource())
        return false;

    context_->GetCache()->ReleaseResource(XMLFile::GetTypeStatic(), resourceName_, true);

    auto fullPath = context_->GetCache()->GetResourceFileName(resourceName_);
    if (fullPath.empty())
        return false;

    File file(context_, fullPath, FILE_WRITE);
    bool result = false;

    float elapsed = GetScene()->GetElapsedTime();
    GetScene()->SetElapsedTime(0);
    GetScene()->SetUpdateEnabled(true);
    if (fullPath.ends_with(".xml", false))
        result = GetScene()->SaveXML(file);
    else if (fullPath.ends_with(".json", false))
        result = GetScene()->SaveJSON(file);
    GetScene()->SetUpdateEnabled(false);
    GetScene()->SetElapsedTime(elapsed);

    if (result)
        SendEvent(E_EDITORRESOURCESAVED);
    else
        URHO3D_LOGERRORF("Saving scene to %s failed.", resourceName_.c_str());

    return result;
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

    selectedComponents_.insert(WeakPtr<Component>(component));
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::Select(ea::vector<Node*> nodes)
{
    if (nodes.empty())
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

    if (selectedComponents_.erase(WeakPtr<Component>(component)))
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
    if (selectedComponents_.contains(componentPtr))
        selectedComponents_.erase(componentPtr);
    else
        selectedComponents_.insert(componentPtr);

    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
}

void SceneTab::UnselectAll()
{
    selectionRect_ = {ui::GetMousePos(), ui::GetMousePos()};
    bool hadComponents = !selectedComponents_.empty();
    selectedComponents_.clear();
    if (gizmo_.UnselectAll() || hadComponents)
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENE, GetScene());
    }
}

const ea::vector<WeakPtr<Node>>& SceneTab::GetSelection() const
{
    return gizmo_.GetSelection();
}

void SceneTab::RenderToolbarButtons()
{
    ui::SetCursorPos({4, 4});

    if (ui::EditorToolbarButton(ICON_FA_SAVE, "Save"))
        SaveResource();

    ui::SameLine(0, 3.f);

    ui::BeginButtonGroup();
    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT "###Translate", "Translate", gizmo_.GetOperation() == GIZMOOP_TRANSLATE))
        gizmo_.SetOperation(GIZMOOP_TRANSLATE);
    if (ui::EditorToolbarButton(ICON_FA_SYNC "###Rotate", "Rotate", gizmo_.GetOperation() == GIZMOOP_ROTATE))
        gizmo_.SetOperation(GIZMOOP_ROTATE);
    if (ui::EditorToolbarButton(ICON_FA_EXPAND_ARROWS_ALT "###Scale", "Scale", gizmo_.GetOperation() == GIZMOOP_SCALE))
        gizmo_.SetOperation(GIZMOOP_SCALE);
    ui::EndButtonGroup();

    ui::SameLine(0, 3.f);

    ui::BeginButtonGroup();
    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT "###World", "World", gizmo_.GetTransformSpace() == TS_WORLD))
        gizmo_.SetTransformSpace(TS_WORLD);
    if (ui::EditorToolbarButton(ICON_FA_EXPAND_ARROWS_ALT "###Local", "Local", gizmo_.GetTransformSpace() == TS_LOCAL))
        gizmo_.SetTransformSpace(TS_LOCAL);
    ui::EndButtonGroup();

    ui::SameLine(0, 3.f);

    if (auto* settings = GetScene()->GetComponent<EditorSceneSettings>())
    {
        ui::BeginButtonGroup();
        if (ui::EditorToolbarButton("3D", "3D mode in editor viewport.", !settings->GetCamera2D()))
            settings->SetCamera2D(false);
        if (ui::EditorToolbarButton("2D", "2D mode in editor viewport.", settings->GetCamera2D()))
            settings->SetCamera2D(true);
        ui::EndButtonGroup();
    }
    ui::SameLine(0, 3.f);

    if (auto* hud = GetSubsystem<DebugHud>())
    {
        if (ui::EditorToolbarButton(ICON_FA_BUG, "Display debug hud.", debugHudVisible_))
            debugHudVisible_ ^= true;
    }

    ui::SameLine(0, 3.f);

    SendEvent(E_EDITORTOOLBARBUTTONS);

    ui::SameLine(0, 3.f);
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

    return selectedComponents_.contains(WeakPtr<Component>(component));
}

void SceneTab::OnNodeSelectionChanged()
{
    using namespace EditorSelectionChanged;
    auto* editor = GetSubsystem<Editor>();

    editor->ClearInspector();
    int inspectedNodes = 0;
    int inspectedComponents = 0;
    Node* lastInspectedNode = nullptr;

    // Inspect all selected nodes.
    for (Node* node : gizmo_.GetSelection())
    {
        if (node)
        {
            editor->Inspect(node);
            inspectedNodes++;
            lastInspectedNode = node;
        }
    }
    // And all selected components.
    for (Component* component : selectedComponents_)
    {
        if (component)
        {
            editor->Inspect(component);
            inspectedComponents++;
        }
    }
    // However if we selected a single node and no components - inspect all components of that node for convenience.
    if (inspectedNodes == 1 && inspectedComponents == 0)
    {
        for (Component* component : lastInspectedNode->GetComponents())
            editor->Inspect(component);
    }

    editor->GetTab<HierarchyTab>()->SetProvider(this);
}

void SceneTab::RenderHierarchy()
{
    if (performRangeSelectionFrame_ != ui::GetFrameCount())
        selectionRect_ = {ui::GetMousePos(), ui::GetMousePos()};

    if (auto* scene = GetScene())
    {
        ui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10);
        RenderNodeTree(scene);
        ui::PopStyleVar();
    }

    // Perform range selection on next frame. Only then we can have a rectangle covering selection ends
    // which we need to do range selection.
    if (ui::IsMouseClicked(MOUSEB_LEFT) && ui::IsKeyDown(SCANCODE_SHIFT) && !GetSelection().empty())
        performRangeSelectionFrame_ = ui::GetFrameCount() + 1;
}

void SceneTab::RenderNodeTree(Node* node)
{
    if (node == nullptr)
        return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (node->IsTemporary() || node->HasTag("__EDITOR_OBJECT__"))
        return;

    if (node == scrollTo_.Get())
        ui::SetScrollHereY();

    ea::string name = node->GetName().empty() ? ToString("%s %d", node->GetTypeName().c_str(), node->GetID()) : node->GetName();
    if (IsSelected(node))
        flags |= ImGuiTreeNodeFlags_Selected;

    ui::Image("Node");
    ui::SameLine();
    ui::PushID((void*)node);
    auto opened = ui::TreeNodeEx(name.c_str(), flags);
    ImRect treeNodeRect = {ui::GetItemRectMin(), ui::GetItemRectMax()};
    auto it = openHierarchyNodes_.find(node);
    if (it != openHierarchyNodes_.end())
    {
        if (!opened)
        {
            ui::OpenTreeNode(ui::GetCurrentWindow()->GetID(name.c_str()));
            opened = true;
        }
        openHierarchyNodes_.erase(it);
    }

    if (ui::BeginDragDropSource())
    {
        ui::SetDragDropVariant("ptr", node);
        ui::Text("%s", name.c_str());
        ui::EndDragDropSource();
    }

    if (ui::BeginDragDropTarget())
    {
        const Variant& payload = ui::AcceptDragDropVariant("ptr");
        if (!payload.IsEmpty())
        {
            SharedPtr<Node> child(dynamic_cast<Node*>(payload.GetPtr()));
            if (child && child != node)
            {
                node->AddChild(child);
                if (!opened)
                    openHierarchyNodes_.push_back(node);
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
        ui::PushID(name.c_str());
    }

    // Popup may delete node. Weak reference will convey that information.
    WeakPtr<Node> nodeRef(node);

    if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        if (ui::IsMouseClicked(MOUSEB_LEFT))
        {
            // Select single node.
            if (!ui::IsKeyDown(SCANCODE_SHIFT) && !ui::IsKeyDown(SCANCODE_CTRL))
                UnselectAll();

            if (ui::IsKeyDown(SCANCODE_SHIFT))
                Select(node);
            else
                ToggleSelection(node);
        }
        else if (ui::IsMouseReleased(MOUSEB_RIGHT) && ImLengthSqr(ui::GetMouseDragDelta(MOUSEB_RIGHT)) == 0.0f)
        {
            if (!IsSelected(node))
            {
                UnselectAll();
                ToggleSelection(node);
            }
            ui::OpenPopupEx(ui::GetID("Node context menu"));
        }
    }

    if (IsSelected(node))
        selectionRect_.Add(treeNodeRect);

    // Range-select by shift-clicking an item.
    if (performRangeSelectionFrame_ == ui::GetFrameCount())
    {
        if (selectionRect_.Contains(treeNodeRect.Min))
        {
            Select(node);
            selectionRect_.Add(treeNodeRect.Min);
        }
    }

    RenderNodeContextMenu();

    if (opened)
    {
        if (!nodeRef.Expired())
        {
            ea::vector<SharedPtr<Component>> components = node->GetComponents();
            for (const auto& component: components)
            {
                if (component->IsTemporary())
                    continue;

                ui::PushID(component);

                ui::Image(component->GetTypeName());
                ui::SameLine();

                bool selected = selectedComponents_.contains(component);
                ui::Selectable(component->GetTypeName().c_str(), selected);

                if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                {
                    if (ui::IsMouseClicked(MOUSEB_LEFT))
                    {
                        if (!ui::IsKeyDown(SCANCODE_CTRL))
                            UnselectAll();
                        Select(component);
                    }
                    else if (ui::IsMouseReleased(MOUSEB_RIGHT) && ImLengthSqr(ui::GetMouseDragDelta(MOUSEB_RIGHT)) == 0.0f)
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
            ea::vector<Node*> children;
            node->GetChildren(children);
            for (Node* child : children)
            {
                // ensure the tree is expanded to the currently selected node if there is one node selected.
                if (GetSelection().size() == 1)
                {
                    for (auto& selectedNode : GetSelection())
                    {
                        if (selectedNode.Expired())
                            continue;

                        if(selectedNode->IsChildOf(child))
                            ui::SetNextItemOpen(true);
                    }
                }

                //recursive call.
                RenderNodeTree(child);
            }
        }
        ui::TreePop();
    }
    else
        ui::PopID();
    ui::PopID();
}

void SceneTab::OnActiveUpdate()
{
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

Scene* SceneTab::GetScene()
{
    return GetSubsystem<SceneManager>()->GetActiveScene();
}

void SceneTab::OnUpdate(VariantMap& args)
{
    Scene* scene = GetScene();
    if (scene == nullptr)
        return;

    float timeStep = args[Update::P_TIMESTEP].GetFloat();
    bool isMode3D_ = !scene->GetComponent<EditorSceneSettings>()->GetCamera2D();
    StringHash cameraControllerType = isMode3D_ ? DebugCameraController3D::GetTypeStatic() : DebugCameraController2D::GetTypeStatic();
    if (Camera* camera = GetCamera())
    {
        auto* component = static_cast<DebugCameraController*>(camera->GetComponent(cameraControllerType));
        if (component != nullptr)
        {
            if (isViewportActive_)
            {
                ui::SetMouseCursor(ImGuiMouseCursor_None);
                if (isMode3D_)
                {
                    auto* controller3D = static_cast<DebugCameraController3D*>(component);
                    Vector3 center;
                    int selectionCount = gizmo_.GetSelectionCenter(center);
                    if (selectionCount > 0)
                    {
                        controller3D->SetRotationCenter(center);
                    }
                    else
                    {
                        controller3D->ClearRotationCenter();
                    }
                }
                component->RunFrame(timeStep);
            }
        }
    }

    if (Tab* tab = GetSubsystem<Editor>()->GetActiveTab())
    {
        StringHash activeTabType = tab->GetType();
        if (activeTabType == GetType() || activeTabType == HierarchyTab::GetTypeStatic())
        {
            if (!ui::IsAnyItemActive())
            {
                // Global view hotkeys
                if (context_->GetInput()->GetKeyPress(KEY_DELETE))
                    RemoveSelection();
                else if (context_->GetInput()->GetKeyDown(KEY_CTRL))
                {
                    if (context_->GetInput()->GetKeyPress(KEY_C))
                        CopySelection();
                    else if (context_->GetInput()->GetKeyPress(KEY_V))
                    {
                        if (context_->GetInput()->GetKeyDown(KEY_SHIFT))
                            PasteIntoSelection();
                        else
                            PasteIntuitive();
                    }
                }
                else if (context_->GetInput()->GetKeyPress(KEY_ESCAPE))
                    UnselectAll();
            }
        }

        if (tab == this)
        {
            Input* input = context_->GetInput();
            if (input->IsMouseVisible())
            {
                if (input->GetKeyPress(KEY_W))
                {
                    gizmo_.SetOperation(GIZMOOP_TRANSLATE);
                }
                else if (input->GetKeyPress(KEY_E))
                {
                    gizmo_.SetOperation(GIZMOOP_ROTATE);
                }
                else if (input->GetKeyPress(KEY_R))
                {
                    gizmo_.SetOperation(GIZMOOP_SCALE);
                }
            }
        }
    }

    // Render editor camera rotation guide
    if (isMode3D_)
    {
        if (auto* debug = GetScene()->GetComponent<DebugRenderer>())
        {
            if (Camera* camera = GetCamera())
            {
                Vector3 guideRoot = GetCamera()->ScreenToWorldPoint({0.95, 0.1, 1});
                debug->AddLine(guideRoot, guideRoot + Vector3::RIGHT * 0.05f, Color::RED, false);
                debug->AddLine(guideRoot, guideRoot + Vector3::UP * 0.05f, Color::GREEN, false);
                debug->AddLine(guideRoot, guideRoot + Vector3::FORWARD * 0.05f, Color::BLUE, false);
            }
        }
    }
}

void SceneTab::SaveState(SceneState& destination)
{
    UndoTrackGuard tracking(undo_, false);

    // Preserve current selection
    savedNodeSelection_.clear();
    savedComponentSelection_.clear();
    for (auto& node : GetSelection())
    {
        if (node)
            savedNodeSelection_.push_back(node->GetID());
    }
    for (auto& component : selectedComponents_)
    {
        if (component)
            savedComponentSelection_.push_back(component->GetID());
    }

    destination.Save(GetScene(), rootElement_);
}

void SceneTab::RestoreState(SceneState& source)
{
    UndoTrackGuard tracking(undo_, false);

    VectorBuffer editorObjectsState;
    if (Node* editorObjects = GetScene()->GetChild("EditorObjects"))
    {
        // Preserve state of editor camera.
        editorObjects->SetTemporary(false);
        editorObjects->Save(editorObjectsState);
    }

    source.Load(GetScene(), rootElement_);
    GetSubsystem<SceneManager>()->UnloadAllButActiveScene();

    if (editorObjectsState.GetSize() > 0)
    {
        if (Node* objects = GetScene()->GetChild("EditorObjects"))
            objects->Remove();

        // Make sure camera is not reset upon stopping scene simulation.
        editorObjectsState.Seek(0);
        auto nodeID = editorObjectsState.ReadUInt();
        Node* editorObjects = GetScene()->CreateChild("EditorObjects", LOCAL, nodeID, true);
        editorObjectsState.Seek(0);
        editorObjects->Load(editorObjectsState);
    }

    // Restore previous selection
    UnselectAll();
    for (unsigned id : savedNodeSelection_)
        Select(GetScene()->GetNode(id));

    for (unsigned id : savedComponentSelection_)
        Select(GetScene()->GetComponent(id));
    savedNodeSelection_.clear();
    savedComponentSelection_.clear();
}

void SceneTab::RenderNodeContextMenu()
{
    if ((!GetSelection().empty() || !selectedComponents_.empty()) && ui::BeginPopup("Node context menu"))
    {
        Input* input = GetSubsystem<Input>();
        if (ui::IsKeyPressed(KEY_ESCAPE) || !input->IsMouseVisible())
        {
            // Close when interacting with scene camera.
            ui::CloseCurrentPopup();
            ui::EndPopup();
            return;
        }

        if (!GetSelection().empty())
        {
            bool alternative = ui::IsKeyDown(KEY_SHIFT);

            if (ui::MenuItem(alternative ? "Create Child (Local)" : "Create Child"))
            {
                ea::vector<Node*> newNodes;
                for (auto& selectedNode : GetSelection())
                {
                    if (!selectedNode.Expired())
                    {
                        newNodes.push_back(selectedNode->CreateChild(EMPTY_STRING, alternative ? LOCAL : REPLICATED));
                        openHierarchyNodes_.push_back(selectedNode);
                        openHierarchyNodes_.push_back(newNodes.back());
                        scrollTo_ = newNodes.back();
                    }
                }

                UnselectAll();
                Select(newNodes);
            }

            if (ui::BeginMenu(alternative ? "Create Component (Local)" : "Create Component"))
            {
                auto* editor = GetSubsystem<Editor>();
                auto categories = context_->GetObjectCategories().keys();
                categories.erase_first("UI");
                ea::quick_sort(categories.begin(), categories.end());

                for (const ea::string& category : categories)
                {
                    auto components = editor->GetObjectsByCategory(category);
                    if (components.empty())
                        continue;

                    if (ui::BeginMenu(category.c_str()))
                    {
                        ea::quick_sort(components.begin(), components.end());

                        for (const ea::string& component : components)
                        {
                            ui::Image(component);
                            ui::SameLine();
                            if (ui::MenuItem(component.c_str()))
                            {
                                for (auto& selectedNode : GetSelection())
                                {
                                    if (!selectedNode.Expired())
                                    {
                                        if (selectedNode->CreateComponent(StringHash(component), alternative ? LOCAL : REPLICATED))
                                            openHierarchyNodes_.push_back(selectedNode);
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
            PasteIntuitive();

        if (ui::MenuItem(ICON_FA_PASTE " Paste Into", "Ctrl+Shift+V"))
            PasteIntoSelection();

        if (ui::MenuItem(ICON_FA_TRASH " Delete", "Del"))
            RemoveSelection();

        ui::Separator();

        if (ui::BeginMenu("Debug Info"))
        {
            enum class DebugInfoMode
            {
                AUTOMATIC,
                ALWAYS,
                NEVER,
                MISMATCH,
                NONE,
            } debugMode = DebugInfoMode::NONE;

            for (auto& node : GetSelection())
            {
                if (!node)
                    continue;

                DebugInfoMode nodeMode;
                if (node->HasTag("DebugInfoNever"))
                    nodeMode = DebugInfoMode::NEVER;
                else if (node->HasTag("DebugInfoAlways"))
                    nodeMode = DebugInfoMode::ALWAYS;
                else
                    nodeMode = DebugInfoMode::AUTOMATIC;

                if (debugMode == DebugInfoMode::NONE)
                    debugMode = nodeMode;
                else if (debugMode != nodeMode)
                {
                    debugMode = DebugInfoMode::MISMATCH;
                    break;
                }
            }

            bool setDebugMode = false;
            if (ui::MenuItem("Automatic", nullptr, debugMode == DebugInfoMode::AUTOMATIC))
            {
                debugMode = DebugInfoMode::AUTOMATIC;
                setDebugMode = true;
            }
            if (ui::MenuItem("Always", nullptr, debugMode == DebugInfoMode::ALWAYS))
            {
                debugMode = DebugInfoMode::ALWAYS;
                setDebugMode = true;
            }
            if (ui::MenuItem("Never", nullptr, debugMode == DebugInfoMode::NEVER))
            {
                debugMode = DebugInfoMode::NEVER;
                setDebugMode = true;
            }

            if (setDebugMode)
            {
                for (auto& node : GetSelection())
                {
                    if (!node)
                        continue;

                    if (debugMode == DebugInfoMode::AUTOMATIC)
                    {
                        node->RemoveTag("DebugInfoAlways");
                        node->RemoveTag("DebugInfoNever");
                    }
                    if (debugMode == DebugInfoMode::ALWAYS)
                    {
                        node->AddTag("DebugInfoAlways");
                        node->RemoveTag("DebugInfoNever");
                    }
                    if (debugMode == DebugInfoMode::NEVER)
                    {
                        node->RemoveTag("DebugInfoAlways");
                        node->AddTag("DebugInfoNever");
                    }
                }
            }
            ui::EndMenu();
        }

        ui::EndPopup();
    }
}

void SceneTab::OnComponentAdded(VariantMap& args)
{
    using namespace ComponentAdded;
    auto* component = static_cast<Component*>(args[P_COMPONENT].GetPtr());
    AddComponentIcon(component);
}

void SceneTab::OnComponentRemoved(VariantMap& args)
{
    using namespace ComponentRemoved;
    auto* component = static_cast<Component*>(args[P_COMPONENT].GetPtr());
    RemoveComponentIcon(component);
}

void SceneTab::OnTemporaryChanged(VariantMap& args)
{
    using namespace TemporaryChanged;
    if (Component* component = dynamic_cast<Component*>(args[P_SERIALIZABLE].GetPtr()))
    {
        if (component->IsTemporary())
            RemoveComponentIcon(component);
        else
            AddComponentIcon(component);
    }
}

void SceneTab::OnSceneActivated(VariantMap& args)
{
    using namespace SceneActivated;
    if (auto* scene = static_cast<Scene*>(args[P_NEWSCENE].GetPtr()))
    {
        SubscribeToEvent(scene, E_COMPONENTADDED, [this](StringHash, VariantMap& args) { OnComponentAdded(args); });
        SubscribeToEvent(scene, E_COMPONENTREMOVED, [this](StringHash, VariantMap& args) { OnComponentRemoved(args); });
        SubscribeToEvent(scene, E_TEMPORARYCHANGED, [this](StringHash, VariantMap& args) { OnTemporaryChanged(args); });

        undo_->Connect(scene);

        // If application logic switches to another scene it will have updates enabled. Ensure they are disabled so we do not get double
        // updates per frame.
        scene->SetUpdateEnabled(false);
        cameraPreviewViewport_->SetScene(scene);
        viewport_->SetScene(scene);
        selectedComponents_.clear();
        gizmo_.UnselectAll();
    }
}

void SceneTab::OnEditorProjectClosing()
{
    ea::string snapshotPath = GetSubsystem<Project>()->GetProjectPath() + ".snapshot.png";

    if (texture_.Null() || GetScene() == nullptr)
    {
        GetSubsystem<FileSystem>()->Delete(snapshotPath);
        return;
    }

    // Also crop image to a square.
    SharedPtr<Image> snapshot(texture_->GetImage());
    int w = snapshot->GetWidth();
    int h = snapshot->GetHeight();
    int side = Min(w, h);
    IntRect rect{0, 0, w, h};
    if (w > h)
    {
        rect.left_ = ((w - side) / 2);
        rect.right_ = rect.left_ + side;
    }
    else if (h > w)
    {
        rect.top_ = ((w - side) / 2);
        rect.bottom_ = rect.top_ + side;
    }
    SharedPtr<Image> cropped = snapshot->GetSubimage(rect);
    cropped->SavePNG(snapshotPath);
}

void SceneTab::AddComponentIcon(Component* component)
{
    if (component == nullptr || component->IsTemporary())
        return;

    Node* node = component->GetNode();

    if (node == nullptr || node->IsTemporary() || node->HasTag("__EDITOR_OBJECT__") ||
        (node->GetName().starts_with("__") && node->GetName().ends_with("__")))
        return;

    auto* material = context_->GetCache()->GetResource<Material>(Format("Materials/Editor/DebugIcon{}.xml", component->GetTypeName()), false);
    if (material != nullptr)
    {
        auto iconTag = "DebugIcon" + component->GetTypeName();
        if (!node->GetChildrenWithTag(iconTag).empty())
            return;                                                 // Icon for component of this type is already added

        UndoTrackGuard tracking(undo_, false);
        int count = node->GetChildrenWithTag("DebugIcon").size();
        node = node->CreateChild();                                 // !! from now on `node` is a container for debug icon
        node->AddTag("DebugIcon");
        node->AddTag(iconTag);
        node->AddTag("__EDITOR_OBJECT__");
        node->SetVar("ComponentType", component->GetType());
        node->SetTemporary(true);

        auto* billboard = node->CreateComponent<BillboardSet>();
        billboard->SetFaceCameraMode(FaceCameraMode::FC_LOOKAT_XYZ);
        if (auto* settings = GetScene()->GetComponent<EditorSceneSettings>())
        {
            bool is2D = settings->GetCamera2D();
            if (is2D)
            {
                node->LookAt(node->GetWorldPosition() + Vector3::FORWARD);
                billboard->SetFaceCameraMode(FaceCameraMode::FC_NONE);
            }
        }

        billboard->SetNumBillboards(1);
        billboard->SetMaterial(material);
        billboard->SetViewMask(EDITOR_VIEW_LAYER);

        if (auto* bb = billboard->GetBillboard(0))
        {
            auto* graphics = context_->GetGraphics();
            float scale = graphics->GetDisplayDPI(graphics->GetCurrentMonitor()).z_ / 96.f;
            bb->size_ = Vector2::ONE * 0.2f * scale;
            bb->enabled_ = true;
            bb->position_ = {0, (float)count * 0.4f * scale, 0};
        }
    }
}

void SceneTab::RemoveComponentIcon(Component* component)
{
    if (component == nullptr || component->IsTemporary())
        return;

    Node* node = component->GetNode();
    if (node->IsTemporary())
        return;

    UndoTrackGuard tracking(undo_, false);

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

void SceneTab::OnFocused()
{
    auto* editor = GetSubsystem<Editor>();
    editor->GetTab<HierarchyTab>()->SetProvider(this);
}

void SceneTab::UpdateCameras()
{
    cameraPreviewViewport_->SetCamera(nullptr);

    if (GetSelection().size())
    {
        if (Node* node = GetSelection().at(0))
        {
            if (Camera* camera = node->GetComponent<Camera>())
            {
                camera->SetViewMask(camera->GetViewMask() & ~EDITOR_VIEW_LAYER);
                cameraPreviewViewport_->SetCamera(camera);
                cameraPreviewViewport_->SetRenderPath(GetViewport()->GetRenderPath());
            }
        }
    }

    if (Camera* camera = GetCamera())
    {
        viewport_->SetCamera(camera);
        if (auto* debug = GetScene()->GetComponent<DebugRenderer>())
            debug->SetView(camera);
    }
}

void SceneTab::CopySelection()
{
    const auto& selection = GetSelection();
    if (!selection.empty())
    {
        clipboard_.Clear();
        clipboard_.Copy(GetSelection());
    }
    else if (!selectedComponents_.empty())
    {
        clipboard_.Clear();
        clipboard_.Copy(selectedComponents_);
    }
}

void SceneTab::PasteNextToSelection()
{
    const auto& selection = GetSelection();
    PasteResult result;
    Node* target = nullptr;
    if (!selection.empty())
    {
        target = nullptr;
        unsigned i = 0;
        while (target == nullptr && i < selection.size())   // Selected node may be null
            target = selection[i++].Get();
        if (target != nullptr)
            target = target->GetParent();
    }

    if (target == nullptr)
        target = GetScene();

    result = clipboard_.Paste(target);

    UnselectAll();

    for (Node* node : result.nodes_)
        Select(node);

    for (Component* component : result.components_)
        Select(component);
}

void SceneTab::PasteIntoSelection()
{
    const auto& selection = GetSelection();
    PasteResult result;
    if (selection.empty())
        result = clipboard_.Paste(GetScene());
    else
        result = clipboard_.Paste(selection);

    UnselectAll();

    for (Node* node : result.nodes_)
        Select(node);

    for (Component* component : result.components_)
        Select(WeakPtr<Component>(component));
}

void SceneTab::PasteIntuitive()
{
    if (clipboard_.HasNodes())
        PasteNextToSelection();
    else if (clipboard_.HasComponents())
        PasteIntoSelection();
}

Camera* SceneTab::GetCamera()
{
    if (Scene* scene = GetScene())
    {
        if (Node* node = scene->GetChild("__EditorCamera__", true))
            return node->GetComponent<Camera>();
    }
    return nullptr;
}

void SceneTab::RenderDebugInfo()
{
    Scene* scene = GetScene();
    if (scene == nullptr)
        return;

    DebugRenderer* debug = scene->GetComponent<DebugRenderer>();
    if (debug == nullptr)
        return;

    auto renderDebugInfo = [debug](Component* component) {
        if (auto* light = component->Cast<Light>())
            light->DrawDebugGeometry(debug, true);
        else if (auto* drawable = component->Cast<Drawable>())
            debug->AddBoundingBox(drawable->GetWorldBoundingBox(), Color::WHITE);
        else if (!component->IsInstanceOf<Terrain>())
            component->DrawDebugGeometry(debug, true);
    };

    const auto& selection = GetSelection();
    for (auto& node : selection)
    {
        if (node && !node->HasTag("DebugInfoNever"))
        {
            for (auto& component: node->GetComponents())
                renderDebugInfo(component);
        }
    }

    ea::vector<Node*> debugNodes;
    scene->GetNodesWithTag(debugNodes, "DebugInfoAlways");
    for (Node* node : debugNodes)
    {
        if (selection.contains(WeakPtr<Node>(node)))
            continue;

        for (auto& component: node->GetComponents())
            renderDebugInfo(component);
    }

    for (auto& component : selectedComponents_)
    {
        if (component)
            renderDebugInfo(component);
    }
}

void SceneTab::Close()
{
    UndoTrackGuard noTrack(undo_, false);

    SceneManager* manager = GetSubsystem<SceneManager>();
    manager->UnloadScene(GetScene());

    BaseClassName::Close();

    GetSubsystem<Editor>()->UpdateWindowTitle();
}
}
