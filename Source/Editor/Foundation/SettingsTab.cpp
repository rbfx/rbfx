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

#include "../Core/IniHelpers.h"
#include "../Foundation/SettingsTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_SettingsTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<SettingsTab>(context));
}

SettingsTab::SettingsTab(Context* context)
    : EditorTab(context, "Settings", "5123082a-1ded-4de7-bab0-b48a3d56a073",
        EditorTabFlag::None, EditorTabPlacement::DockRight)
{
}

void SettingsTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    if (!selectedGroup_)
        return;

    for (const auto& [_, page] : selectedGroup_->pages_)
        page->ApplyHotkeys(hotkeyManager);
}

void SettingsTab::RenderContent()
{
    if (!selectedGroup_ || selectedGroup_->pages_.empty())
        selectNextValidGroup_ = true;

    const float verticalThresholdIn = 700.0f;
    const float verticalThresholdOut = 750.0f;
    const float totalWidth = ui::GetContentRegionAvail().x;
    const float totalHeight = ui::GetContentRegionAvail().y;

    if (!useVerticalLayout_.has_value() || totalWidth > verticalThresholdOut)
        useVerticalLayout_ = false;
    else if (totalWidth < verticalThresholdIn)
        useVerticalLayout_ = true;

    if (*useVerticalLayout_)
    {
        if (ui::BeginChild("##SettingsTree", {totalWidth, totalHeight * 0.3f}))
            RenderSettingsTree();
        ui::EndChild();

        ui::Separator();

        if (ui::BeginChild("##SettingsPage", ui::GetContentRegionAvail()))
            RenderCurrentGroup();
        ui::EndChild();
    }
    else if (ui::BeginTable("##ResourceBrowserTab", 2, ImGuiTableFlags_Resizable))
    {
        ui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.65f);

        ui::TableNextRow();

        ui::TableSetColumnIndex(0);
        if (ui::BeginChild("##SettingsTree", ui::GetContentRegionAvail()))
            RenderSettingsTree();
        ui::EndChild();

        ui::TableSetColumnIndex(1);
        if (ui::BeginChild("##SettingsPage", ui::GetContentRegionAvail()))
            RenderCurrentGroup();
        ui::EndChild();

        ui::EndTable();
    }
}

bool SettingsTab::IsMarkedUnsaved()
{
    return GetProject()->HasUnsavedChanges();
}

void SettingsTab::RenderSettingsTree()
{
    auto project = GetProject();
    SettingsManager* settingsManager = project->GetSettingsManager();

    const auto& rootGroup = settingsManager->GetPageTree();
    for (const auto& [shortName, childGroup] : rootGroup.children_)
        RenderSettingsSubtree(*childGroup, shortName);
}

void SettingsTab::RenderSettingsSubtree(const SettingsPageGroup& group, const ea::string& shortName)
{
    const IdScopeGuard guard(shortName.c_str());

    if (selectNextValidGroup_ && !group.pages_.empty())
    {
        selectNextValidGroup_ = false;
        selectedGroup_ = &group;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth
        | ImGuiTreeNodeFlags_DefaultOpen;
    if (group.children_.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (&group == selectedGroup_)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (selectNextValidGroup_)
        ui::SetNextItemOpen(true);
    const bool isOpen = ui::TreeNodeEx(shortName.c_str(), flags);

    // Process clicking
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        selectedGroup_ = &group;
        if (group.pages_.empty())
            selectNextValidGroup_ = true;
    }

    // Render children
    if (isOpen)
    {
        for (const auto& [shortName, childGroup] : group.children_)
            RenderSettingsSubtree(*childGroup, shortName);
        ui::TreePop();
    }
}

void SettingsTab::RenderCurrentGroup()
{
    if (selectedGroup_ && !selectedGroup_->pages_.empty())
    {
        for (const auto& [section, page] : selectedGroup_->pages_)
            RenderPage(section, page);
    }
}

void SettingsTab::RenderPage(const ea::string& section, SettingsPage* page)
{
    const IdScopeGuard guard(page);

    if (page->CanResetToDefault())
    {
        if (ui::Button(ICON_FA_CLOCK_ROTATE_LEFT "##Revert"))
            page->ResetToDefaults();
        if (ui::IsItemHovered())
            ui::SetTooltip("Revert settings to default values");
    }

    bool showPage = true;
    if (!section.empty())
    {
        ui::SameLine();
        showPage = ui::CollapsingHeader(section.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    }

    if (showPage)
        page->RenderSettings();
}

}
