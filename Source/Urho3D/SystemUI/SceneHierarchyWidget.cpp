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

#include "../Precompiled.h"

#include "../SystemUI/SceneHierarchyWidget.h"

#include "../Scene/Component.h"
#include "../SystemUI/DragDropPayload.h"
#include "../SystemUI/Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

unsigned GetObjectIndexInParent(Node* parentNode, Node* node)
{
    return parentNode->GetChildIndex(node);
}

unsigned GetObjectIndexInParent(Node* parentNode, Component* component)
{
    return parentNode->GetComponentIndex(component);
}

ea::string GetNodeTitle(Node* node)
{
    const bool isScene = node->GetParent() == nullptr;
    ea::string title = isScene ? ICON_FA_CUBES " " : ICON_FA_CUBE " ";
    if (!node->GetName().empty())
        title += node->GetName();
    else if (isScene)
        title += "Scene";
    else
        title += Format("Node {}", node->GetID());
    return title;
}

bool CanBeDroppedTo(Node* parentNode, NodeComponentDragDropPayload& payload)
{
    if (payload.nodes_.empty() || parentNode->GetScene() != payload.scene_)
        return false;

    const bool containsScene = ea::any_of(payload.nodes_.begin(), payload.nodes_.end(),
        [](Node* node) { return node->GetScene() == node; });
    const bool anyLoops = ea::any_of(payload.nodes_.begin(), payload.nodes_.end(),
        [&](Node* node) { return !node || parentNode->IsChildOf(node); });
    return !anyLoops && !containsScene;
}

enum class HierarchyItemFlag
{
    Node = 1 << 0,
    Component = 1 << 1,
    Enabled = 1 << 2,
    Temporary = 1 << 3,
    Subsystem = 1 << 4,
};
URHO3D_FLAGSET(HierarchyItemFlag, HierarchyItemFlags);

ImVec4 GetItemColor(const HierarchyItemFlags& flags)
{
    const bool enabled = flags.Test(HierarchyItemFlag::Enabled);
    const bool temporary = flags.Test(HierarchyItemFlag::Temporary);
    if (flags.Test(HierarchyItemFlag::Component))
    {
        if (temporary)
        {
            return enabled
                ? ImVec4(0.65f, 0.65f, 1.00f, 1.00f)
                : ImVec4(0.25f, 0.25f, 0.50f, 1.00f);
        }
        else
        {
            return enabled
                ? ImVec4(1.00f, 1.00f, 0.35f, 1.00f)
                : ImVec4(0.50f, 0.50f, 0.00f, 1.00f);
        }
    }
    else
    {
        if (temporary)
        {
            return enabled
                ? ImVec4(0.65f, 0.65f, 1.00f, 1.00f)
                : ImVec4(0.25f, 0.25f, 0.50f, 1.00f);
        }
        else
        {
            return enabled
                ? ImVec4(1.00f, 1.00f, 1.00f, 1.00f)
                : ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        }
    }
}

unsigned GetVisibleChildCount(Node* node, bool showTemporary)
{
    return showTemporary ? node->GetNumChildren() : node->GetNumPersistentChildren();
}

unsigned GetVisibleComponentCount(Node* node, bool showTemporary)
{
    return showTemporary ? node->GetNumComponents() : node->GetNumPersistentComponents();
}

unsigned GetVisibleItemsCount(Node* node, bool showTemporary, bool showComponents)
{
    const unsigned numChildren = GetVisibleChildCount(node, showTemporary);
    const unsigned numComponents = showComponents ? GetVisibleComponentCount(node, showTemporary) : 0;
    return numChildren + numComponents;
}

}

SceneHierarchyWidget::SceneHierarchyWidget(Context* context)
    : Object(context)
{
}

void SceneHierarchyWidget::SetSettings(const SceneHierarchySettings& settings)
{
    settings_ = settings;
}

void SceneHierarchyWidget::RenderContent(Scene* scene, SceneSelection& selection)
{
    const bool sceneChanged = search_.lastScene_ != scene;
    const bool queryChanged = search_.currentQuery_ != settings_.filterByName_;
    search_.currentQuery_ = settings_.filterByName_;
    if (queryChanged || sceneChanged)
        UpdateSearchResults(scene);

    ProcessActiveObject(selection.GetActiveObject());

    BeginRangeSelection();

    const ImGuiStyle& style = ui::GetStyle();
    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0));
    if (search_.lastQuery_.empty())
    {
        RenderNode(selection, scene);
    }
    else
    {
        for (Node* node : search_.lastResults_)
        {
            if (node && (settings_.showTemporary_ || !node->IsTemporaryEffective()))
                RenderNode(selection, node);
        }
    }
    ui::PopStyleVar();

    EndRangeSelection(selection);

    RenderContextMenu(scene, selection);

    ApplyPendingUpdates(scene);

    // Suppress future updates if selection was changed by the widget
    lastActiveObject_ = selection.GetActiveObject();

    // Reset flag just in case
    if (ignoreNextMouseRelease_ && ui::IsMouseReleased(MOUSEB_LEFT))
        ignoreNextMouseRelease_ = false;
}

void SceneHierarchyWidget::RenderNode(SceneSelection& selection, Node* node)
{
    if (!settings_.showTemporary_ && node->IsTemporary())
        return;

    ProcessItemIfActive(selection, node);

    const unsigned numItems = GetVisibleItemsCount(node, settings_.showTemporary_, settings_.showComponents_);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_AllowItemOverlap;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (selection.IsSelected(node))
        flags |= ImGuiTreeNodeFlags_Selected;
    if (numItems == 0)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (numItems <= 2)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    HierarchyItemFlags itemFlags = HierarchyItemFlag::Node;
    if (node->IsTemporaryEffective())
        itemFlags |= HierarchyItemFlag::Temporary;
    if (node->IsEnabled())
        itemFlags |= HierarchyItemFlag::Enabled;

    if (pathToActiveObject_.contains(node))
        ui::SetNextItemOpen(true);

    const IdScopeGuard guard(node->GetID());
    ui::PushStyleColor(ImGuiCol_Text, GetItemColor(itemFlags));
    const bool opened = ui::TreeNodeEx(GetNodeTitle(node).c_str(), flags);
    ui::PopStyleColor();
    const bool toggleSelect = ui::IsKeyDown(KEY_CTRL);
    const bool rangeSelect = ui::IsKeyDown(KEY_SHIFT);

    ProcessRangeSelection(node, opened);

    if (ui::IsItemClicked(MOUSEB_LEFT) && ui::IsItemToggledOpen())
        ignoreNextMouseRelease_ = true;

    if (ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_LEFT) && !ui::IsMouseDragPastThreshold(MOUSEB_LEFT))
    {
        if (ignoreNextMouseRelease_)
            ignoreNextMouseRelease_ = false;
        else
            ProcessObjectSelected(selection, node, toggleSelect, rangeSelect);
    }
    else if (ui::IsItemClicked(MOUSEB_RIGHT))
    {
        if (selection.IsSelected(node))
            OpenSelectionContextMenu();
        else
        {
            ProcessObjectSelected(selection, node, toggleSelect, false);
            OpenSelectionContextMenu();
        }
    }

    if (ui::BeginDragDropSource())
    {
        if (!selection.IsSelected(node))
            ProcessObjectSelected(selection, node, toggleSelect, false);

        BeginSelectionDrag(node->GetScene(), selection);
        ui::EndDragDropSource();
    }

    if (ui::BeginDragDropTarget())
    {
        DropPayloadToNode(node);
        ui::EndDragDropTarget();
    }

    if (auto parent = node->GetParent())
    {
        if (const auto reorder = RenderObjectReorder(nodeReorder_, node, parent, "Move node up or down in the parent node"))
            pendingNodeReorder_ = reorder;
    }

    if (opened)
    {
        if (settings_.showComponents_)
        {
            const IdScopeGuard guard("Components");
            for (Component* component : node->GetComponents())
                RenderComponent(selection, component);
        }

        for (Node* child : node->GetChildren())
            RenderNode(selection, child);

        ui::TreePop();
    }
}

void SceneHierarchyWidget::RenderComponent(SceneSelection& selection, Component* component)
{
    if (component->IsTemporary() && !settings_.showTemporary_)
        return;

    ProcessItemIfActive(selection, component);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_Leaf;
    if (selection.IsSelected(component))
        flags |= ImGuiTreeNodeFlags_Selected;
    HierarchyItemFlags itemFlags = HierarchyItemFlag::Component;
    if (component->IsTemporary() || component->GetNode()->IsTemporaryEffective())
        itemFlags |= HierarchyItemFlag::Temporary;
    if (component->IsEnabledEffective())
        itemFlags |= HierarchyItemFlag::Enabled;

    const IdScopeGuard guard(component->GetID());
    ui::PushStyleColor(ImGuiCol_Text, GetItemColor(itemFlags));
    const bool opened = ui::TreeNodeEx(component->GetTypeName().c_str(), flags);
    ui::PopStyleColor();
    const bool toggleSelect = ui::IsKeyDown(KEY_CTRL);
    const bool rangeSelect = ui::IsKeyDown(KEY_SHIFT);

    ProcessRangeSelection(component, opened);

    if (ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_LEFT) && !ui::IsMouseDragPastThreshold(MOUSEB_LEFT))
    {
        ProcessObjectSelected(selection, component, toggleSelect, rangeSelect);
    }
    else if (ui::IsItemClicked(MOUSEB_RIGHT))
    {
        if (selection.IsSelected(component))
            OpenSelectionContextMenu();
        else
        {
            ProcessObjectSelected(selection, component, toggleSelect, false);
            OpenSelectionContextMenu();
        }
    }

    if (ui::BeginDragDropSource())
    {
        if (!selection.IsSelected(component))
            ProcessObjectSelected(selection, component, toggleSelect, false);

        BeginSelectionDrag(component->GetScene(), selection);
        ui::EndDragDropSource();
    }

    if (const auto reorder = RenderObjectReorder(componentReorder_, component, component->GetNode(), "Move component up or down in the node"))
        pendingComponentReorder_ = reorder;

    if (opened)
        ui::TreePop();
}

template <class T>
SceneHierarchyWidget::OptionalReorderInfo SceneHierarchyWidget::RenderObjectReorder(
    OptionalReorderInfo& info, T* object, Node* parentNode, const char* hint)
{
    const bool reorderingThisObject = ui::IsItemHovered() || (info && info->id_ == object->GetID());
    if (!reorderingThisObject)
        return ea::nullopt;

    ImGuiStyle& style = ui::GetStyle();
    const float reorderButtonWidth = ui::CalcTextSize(ICON_FA_UP_DOWN).x;

    ui::SameLine();
    ui::SetCursorPosX(ui::GetContentRegionMax().x - reorderButtonWidth * 2.0f);
    ui::SmallButton(ICON_FA_UP_DOWN);

    if (ui::IsItemActive())
    {
        const unsigned oldIndex = GetObjectIndexInParent(parentNode, object);

        if (ui::IsItemActivated())
            info = ReorderInfo{object->GetID(), oldIndex};

        const ImVec2 mousePos = ui::GetMousePos();
        unsigned newIndex = oldIndex;

        if (mousePos.y < ui::GetItemRectMin().y && newIndex > 0)
            --newIndex;
        else if (mousePos.y > ui::GetItemRectMax().y)
            ++newIndex; // It's okay to overflow

        if (newIndex != oldIndex)
            return ReorderInfo{object->GetID(), oldIndex, newIndex};
    }
    else if (info && info->id_ == object->GetID())
    {
        info = ea::nullopt;
    }
    else if (ui::IsItemHovered())
    {
        ui::SetTooltip("%s", hint);
    }
    return ea::nullopt;
}

void SceneHierarchyWidget::ApplyPendingUpdates(Scene* scene)
{
    if (pendingNodeReorder_)
    {
        if (Node* node = scene->GetNode(pendingNodeReorder_->id_))
            OnNodeReordered(this, node, pendingNodeReorder_->oldIndex_, pendingNodeReorder_->newIndex_);
        pendingNodeReorder_ = ea::nullopt;
    }

    if (pendingComponentReorder_)
    {
        if (Component* component = scene->GetComponent(pendingComponentReorder_->id_))
            OnComponentReordered(this, component, pendingComponentReorder_->oldIndex_, pendingComponentReorder_->newIndex_);
        pendingComponentReorder_ = ea::nullopt;
    }

    if (!pendingNodeReparents_.empty())
    {
        for (const ReparentInfo& info : pendingNodeReparents_)
        {
            Node* childNode = scene->GetNode(info.childId_);
            Node* parentNode = scene->GetNode(info.parentId_);
            if (childNode && parentNode)
                OnNodeReparented(this, parentNode, childNode);
        }
        pendingNodeReparents_.clear();
    }
}

void SceneHierarchyWidget::ProcessObjectSelected(SceneSelection& selection, Object* object, bool toggle, bool range)
{
    Object* activeObject = selection.GetActiveObject();

    if (toggle)
        selection.SetSelected(object, !selection.IsSelected(object));
    else if (range && activeObject && wasActiveObjectVisible_ && activeObject != object)
        rangeSelection_.pendingRequest_ = RangeSelectionRequest{WeakPtr<Object>(activeObject), WeakPtr<Object>(object)};
    else
    {
        selection.Clear();
        selection.SetSelected(object, true);
    }
}

void SceneHierarchyWidget::ProcessItemIfActive(SceneSelection& selection, Object* currentItem)
{
    if (selection.GetActiveObject() == currentItem)
    {
        isActiveObjectVisible_ = true;
        if (scrollToActiveObject_)
        {
            ui::SetScrollHereY();
            scrollToActiveObject_ = false;
        }
    }
}

void SceneHierarchyWidget::ProcessActiveObject(Object* activeObject)
{
    pathToActiveObject_.clear();
    if (lastActiveObject_ != activeObject)
    {
        lastActiveObject_ = activeObject;
        scrollToActiveObject_ = true;

        const auto activeComponent = dynamic_cast<Component*>(activeObject);
        const auto activeNode = dynamic_cast<Node*>(activeObject);
        Node* currentPathNode = activeComponent ? activeComponent->GetNode() : activeNode ? activeNode->GetParent() : nullptr;
        while (currentPathNode != nullptr)
        {
            pathToActiveObject_.push_back(currentPathNode);
            currentPathNode = currentPathNode->GetParent();
        }
    }
}

void SceneHierarchyWidget::OpenSelectionContextMenu()
{
    openContextMenu_ = true;
}

void SceneHierarchyWidget::RenderContextMenu(Scene* scene, SceneSelection& selection)
{
    static const ea::string contextMenuPopup = "##ContextMenu";

    if (openContextMenu_)
    {
        if (OnContextMenu.HasSubscriptions())
            ui::OpenPopup(contextMenuPopup.c_str());
        openContextMenu_ = false;
    }

    if (ui::BeginPopup(contextMenuPopup.c_str()))
    {
        ui::BeginDisabled();
        ui::Text("Selected: %s", selection.GetSummary(scene).c_str());
        ui::EndDisabled();

        OnContextMenu(this, scene, selection);
        ui::EndPopup();
    }
}

void SceneHierarchyWidget::BeginRangeSelection()
{
    wasActiveObjectVisible_ = isActiveObjectVisible_;
    isActiveObjectVisible_ = false;
    rangeSelection_.result_.clear();
    rangeSelection_.isActive_ = false;
    rangeSelection_.currentRequest_ = rangeSelection_.pendingRequest_;
    rangeSelection_.pendingRequest_ = ea::nullopt;
}

void SceneHierarchyWidget::ProcessRangeSelection(Object* currentObject, bool open)
{
    if (!rangeSelection_.currentRequest_)
        return;

    const WeakPtr<Object> weakObject{currentObject};
    const bool isBorder = rangeSelection_.currentRequest_->IsBorder(currentObject);

    if (!rangeSelection_.isActive_ && isBorder)
    {
        rangeSelection_.isActive_ = true;
        rangeSelection_.result_.push_back(weakObject);
    }
    else if (rangeSelection_.isActive_ && isBorder)
    {
        rangeSelection_.result_.push_back(weakObject);
        rangeSelection_.isActive_ = false;
        rangeSelection_.currentRequest_ = ea::nullopt;
    }
    else if (rangeSelection_.isActive_)
    {
        rangeSelection_.result_.push_back(weakObject);
    }
}

void SceneHierarchyWidget::EndRangeSelection(SceneSelection& selection)
{
    rangeSelection_.currentRequest_ = ea::nullopt;

    if (!rangeSelection_.isActive_)
    {
        for (Object* object : rangeSelection_.result_)
            selection.SetSelected(object, true);
    }
}

void SceneHierarchyWidget::UpdateSearchResults(Scene* scene)
{
    const bool sceneChanged = search_.lastScene_ != scene;
    search_.lastScene_ = scene;

    // Early return if search was canceled
    if (search_.currentQuery_.empty())
    {
        search_.lastResults_.clear();
        search_.lastQuery_.clear();
        return;
    }

    const bool resultsExpired = sceneChanged
        || search_.lastResults_.empty()
        || !search_.lastQuery_.contains(search_.currentQuery_);
    search_.lastQuery_ = search_.currentQuery_;

    if (resultsExpired)
    {
        ea::vector<Node*> children;
        scene->GetChildren(children, true);

        search_.lastResults_.clear();
        for (Node* child : children)
        {
            if (child->GetName().contains(search_.currentQuery_, false))
                search_.lastResults_.emplace_back(child);
        }
    }
    else
    {
        ea::erase_if(search_.lastResults_, [&](const WeakPtr<Node>& node)
        {
            return !node || !node->GetName().contains(search_.currentQuery_, false);
        });
    }
}

void SceneHierarchyWidget::BeginSelectionDrag(Scene* scene, SceneSelection& selection)
{
    DragDropPayload::UpdateSource([&]()
    {
        auto payload = MakeShared<NodeComponentDragDropPayload>();
        payload->scene_ = scene;
        payload->displayString_ = selection.GetSummary(scene);

        const auto& nodes = selection.GetNodesAndScenes();
        ea::copy(nodes.begin(), nodes.end(), ea::back_inserter(payload->nodes_));

        const auto& components = selection.GetComponents();
        ea::copy(components.begin(), components.end(), ea::back_inserter(payload->components_));

        return payload;
    });
}

void SceneHierarchyWidget::DropPayloadToNode(Node* parentNode)
{
    auto payload = dynamic_cast<NodeComponentDragDropPayload*>(DragDropPayload::Get());
    if (payload && CanBeDroppedTo(parentNode, *payload))
    {
        if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str()))
        {
            Scene* scene = parentNode->GetScene();
            for (Node* childNode : payload->nodes_)
            {
                if (childNode)
                    pendingNodeReparents_.push_back(ReparentInfo{parentNode->GetID(), childNode->GetID()});
            }
        }
    }
}

}
