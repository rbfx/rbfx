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

#include "../SystemUI/SceneHierarchyWidget.h"

#include "../Scene/Component.h"
#include "../SystemUI/Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

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

    // Suppress future updates if selection was changed by the widget
    lastActiveObject_ = selection.GetActiveObject();
}

void SceneHierarchyWidget::RenderNode(SceneSelection& selection, Node* node)
{
    if (!settings_.showTemporary_ && node->IsTemporary())
        return;

    ProcessItemIfActive(selection, node);

    const bool isEmpty = node->GetChildren().empty() && (!settings_.showComponents_ || node->GetComponents().empty());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_AllowItemOverlap;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (selection.IsSelected(node))
        flags |= ImGuiTreeNodeFlags_Selected;
    if (isEmpty)
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (pathToActiveObject_.contains(node))
        ui::SetNextItemOpen(true);

    const IdScopeGuard guard(static_cast<void*>(node));
    const bool opened = ui::TreeNodeEx(GetNodeTitle(node).c_str(), flags);
    ProcessRangeSelection(node, opened);

    if ((ui::IsItemClicked(MOUSEB_LEFT) || ui::IsItemClicked(MOUSEB_RIGHT)) && !ui::IsItemToggledOpen())
    {
        const bool toggleSelect = ui::IsKeyDown(KEY_CTRL);
        const bool rangeSelect = ui::IsKeyDown(KEY_SHIFT);
        ProcessObjectSelected(selection, node, toggleSelect, rangeSelect);
    }

    if (opened)
    {
        if (settings_.showComponents_)
        {
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

    const IdScopeGuard guard(static_cast<void*>(component));
    const bool opened = ui::TreeNodeEx(component->GetTypeName().c_str(), flags);
    ProcessRangeSelection(component, opened);

    if (ui::IsItemClicked(MOUSEB_LEFT) || ui::IsItemClicked(MOUSEB_RIGHT))
    {
        const bool toggleSelect = ui::IsKeyDown(KEY_CTRL);
        const bool rangeSelect = ui::IsKeyDown(KEY_SHIFT);
        ProcessObjectSelected(selection, component, toggleSelect, rangeSelect);
    }

    if (opened)
        ui::TreePop();
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

}
