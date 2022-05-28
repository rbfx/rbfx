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

#include "../../Foundation/SceneViewTab/SceneHierarchy.h"

#include <Urho3D/Scene/Component.h>

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

void Foundation_SceneHierarchy(Context* context, SceneViewTab* sceneViewTab)
{
}

SceneHierarchy::SceneHierarchy(SceneViewTab* sceneViewTab)
    : HierarchyBrowserSource(sceneViewTab->GetContext())
    , owner_(sceneViewTab)
{
}

void SceneHierarchy::RenderContent()
{
    SceneViewPage* activePage = owner_->GetActivePage();
    if (!activePage)
        return;

    BeginRangeSelection();

    const ImGuiStyle& style = ui::GetStyle();
    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0));
    RenderNode(*activePage, activePage->scene_);
    ui::PopStyleVar();

    EndRangeSelection(*activePage);
}

void SceneHierarchy::RenderContextMenuItems()
{

}

void SceneHierarchy::RenderMenu()
{
    if (owner_)
        owner_->RenderMenu();
}

void SceneHierarchy::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    if (owner_)
        owner_->ApplyHotkeys(hotkeyManager);
}

void SceneHierarchy::RenderNode(SceneViewPage& page, Node* node)
{
    if (node->IsTemporary() && !showTemporary_)
        return;

    UpdateActiveObjectVisibility(page, node);

    const bool isEmpty = node->GetChildren().empty() && (!showComponents_ || node->GetComponents().empty());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_AllowItemOverlap;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (page.selection_.IsSelected(node))
        flags |= ImGuiTreeNodeFlags_Selected;
    if (isEmpty)
        flags |= ImGuiTreeNodeFlags_Leaf;

    ui::PushID(static_cast<void*>(node));
    const bool opened = ui::TreeNodeEx(GetNodeTitle(node).c_str(), flags);
    ProcessRangeSelection(node, opened);

    if (ui::IsItemClicked(MOUSEB_LEFT) || ui::IsItemClicked(MOUSEB_RIGHT))
    {
        const bool toggleSelect = ui::IsKeyDown(KEY_CTRL);
        const bool rangeSelect = ui::IsKeyDown(KEY_SHIFT);
        ProcessNodeSelected(page, node, toggleSelect, rangeSelect);
    }

    if (opened)
    {
        for (Node* child : node->GetChildren())
            RenderNode(page, child);

        ui::TreePop();
    }
    ui::PopID();
}

void SceneHierarchy::ProcessNodeSelected(SceneViewPage& page, Node* node, bool toggle, bool range)
{
    SceneSelection& selection = page.selection_;
    Object* activeObject = selection.GetActiveObject();

    if (toggle)
        selection.SetSelected(node, !selection.IsSelected(node));
    else if (range && activeObject && wasActiveObjectVisible_ && activeObject != node)
        rangeSelection_.pendingRequest_ = RangeSelectionRequest{WeakPtr<Object>(activeObject), WeakPtr<Object>(node)};
    else
    {
        selection.Clear();
        selection.SetSelected(node, true);
    }
}

void SceneHierarchy::UpdateActiveObjectVisibility(SceneViewPage& page, Object* currentItem)
{
    if (page.selection_.GetActiveObject() == currentItem)
        isActiveObjectVisible_ = true;
}

void SceneHierarchy::BeginRangeSelection()
{
    wasActiveObjectVisible_ = isActiveObjectVisible_;
    isActiveObjectVisible_ = false;
    rangeSelection_.result_.clear();
    rangeSelection_.isActive_ = false;
    rangeSelection_.currentRequest_ = rangeSelection_.pendingRequest_;
    rangeSelection_.pendingRequest_ = ea::nullopt;
}

void SceneHierarchy::ProcessRangeSelection(Object* currentObject, bool open)
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

void SceneHierarchy::EndRangeSelection(SceneViewPage& page)
{
    rangeSelection_.currentRequest_ = ea::nullopt;

    if (!rangeSelection_.isActive_)
    {
        for (Object* object : rangeSelection_.result_)
            page.selection_.SetSelected(object, true);
    }
}

}
