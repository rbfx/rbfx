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

#include "../../Core/IniHelpers.h"
#include "../../Foundation/SceneViewTab/SceneHierarchy.h"

#include <Urho3D/Scene/Component.h>
#include <Urho3D/SystemUI/Widgets.h>

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
    sceneViewTab->RegisterAddon<SceneHierarchy>();
}

SceneHierarchy::SceneHierarchy(SceneViewTab* sceneViewTab)
    : SceneViewAddon(sceneViewTab)
    , widget_(MakeShared<SceneHierarchyWidget>(context_))
{
    widget_->OnContextMenu.Subscribe(this, &SceneHierarchy::RenderSelectionContextMenu);
    widget_->OnNodeReordered.Subscribe(this, &SceneHierarchy::ReorderNode);
    widget_->OnComponentReordered.Subscribe(this, &SceneHierarchy::ReorderComponent);
    widget_->OnNodeReparented.Subscribe(this, &SceneHierarchy::ReparentNode);
}

void SceneHierarchy::WriteIniSettings(ImGuiTextBuffer& output)
{
    const SceneHierarchySettings& settings = widget_->GetSettings();
    WriteIntToIni(output, "SceneHierarchy.ShowComponents", settings.showComponents_);
    WriteIntToIni(output, "SceneHierarchy.ShowTemporary", settings.showTemporary_);
    WriteStringToIni(output, "SceneHierarchy.FilterByName", settings.filterByName_);
}

void SceneHierarchy::ReadIniSettings(const char* line)
{
    SceneHierarchySettings settings = widget_->GetSettings();
    if (const auto value = ReadIntFromIni(line, "SceneHierarchy.ShowComponents"))
        settings.showComponents_ = *value != 0;
    if (const auto value = ReadIntFromIni(line, "SceneHierarchy.ShowTemporary"))
        settings.showTemporary_ = *value != 0;
    if (const auto value = ReadStringFromIni(line, "SceneHierarchy.FilterByName"))
        settings.filterByName_ = *value;
    widget_->SetSettings(settings);
}

void SceneHierarchy::RenderContent()
{
    SceneViewPage* activePage = owner_->GetActivePage();
    if (!activePage)
        return;

    RenderToolbar(*activePage);
    if (ui::BeginChild("##SceneHierarchy"))
        widget_->RenderContent(activePage->scene_, activePage->selection_);
    ui::EndChild();
}

void SceneHierarchy::RenderContextMenuItems()
{

}

void SceneHierarchy::RenderMenu()
{
    if (owner_ && !reentrant_)
    {
        reentrant_ = true;
        owner_->RenderMenu();
        reentrant_ = false;
    }
}

void SceneHierarchy::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    if (owner_ && !reentrant_)
    {
        reentrant_ = true;
        owner_->ApplyHotkeys(hotkeyManager);
        reentrant_ = false;
    }
}

void SceneHierarchy::RenderToolbar(SceneViewPage& page)
{
    SceneHierarchySettings settings = widget_->GetSettings();

    if (Widgets::ToolbarButton(ICON_FA_CLOCK, "Show Temporary Nodes & Components", settings.showTemporary_))
        settings.showTemporary_ = !settings.showTemporary_;
    if (Widgets::ToolbarButton(ICON_FA_DIAGRAM_PROJECT, "Show Components", settings.showComponents_))
        settings.showComponents_ = !settings.showComponents_;

    ui::BeginDisabled();
    Widgets::ToolbarButton(ICON_FA_MAGNIFYING_GLASS);
    ui::EndDisabled();

    ui::InputText("##Rename", &settings.filterByName_);

    widget_->SetSettings(settings);
    owner_->SetComponentSelection(settings.showComponents_);
}

void SceneHierarchy::RenderSelectionContextMenu(Scene* scene, SceneSelection& selection)
{
    ui::Separator();
    owner_->RenderEditMenu(scene, selection);
    ui::Separator();
    owner_->RenderCreateMenu(scene, selection);
}

void SceneHierarchy::ReorderNode(Node* node, unsigned oldIndex, unsigned newIndex)
{
    Node* parentNode = node->GetParent();
    parentNode->ReorderChild(node, newIndex);
    owner_->PushAction<ReorderNodeAction>(node, oldIndex, newIndex);
}

void SceneHierarchy::ReorderComponent(Component* component, unsigned oldIndex, unsigned newIndex)
{
    Node* node = component->GetNode();
    node->ReorderComponent(component, newIndex);
    owner_->PushAction<ReorderComponentAction>(component, oldIndex, newIndex);
}

void SceneHierarchy::ReparentNode(Node* parentNode, Node* childNode)
{
    owner_->PushAction<ReparentNodeAction>(childNode, parentNode);
    childNode->SetParent(parentNode);
}

}
