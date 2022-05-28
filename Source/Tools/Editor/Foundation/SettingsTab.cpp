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

namespace Urho3D
{

void Foundation_SettingsTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<SettingsTab>(context));
}

SettingsTab::SettingsTab(Context* context)
    : EditorTab(context, "Settings", "5123082a-1ded-4de7-bab0-b48a3d56a073",
        EditorTabFlag::None, EditorTabPlacement::DockCenter)
{
}

void SettingsTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);

    WriteStringToIni(output, "SelectedPage", selectedPage_);
}

void SettingsTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);

    if (const auto value = ReadStringFromIni(line, "SelectedPage"))
        selectedPage_ = *value;
}

void SettingsTab::RenderContent()
{
    if (selectedPage_.empty())
        selectNextValidPage_ = true;

    if (ui::BeginTable("##ResourceBrowserTab", 2, ImGuiTableFlags_Resizable))
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
            RenderCurrentSettingsPage();
        ui::EndChild();

        ui::EndTable();
    }
}

void SettingsTab::RenderSettingsTree()
{
    auto project = GetProject();
    SettingsManager* settingsManager = project->GetSettingsManager();

    const auto& rootNode = settingsManager->GetPageTree();
    for (const auto& [shortName, childNode] : rootNode.children_)
        RenderSettingsSubtree(childNode, shortName);
}

void SettingsTab::RenderSettingsSubtree(const SettingTreeNode& treeNode, const ea::string& shortName)
{
    if (selectNextValidPage_ && treeNode.page_)
    {
        selectNextValidPage_ = false;
        selectedPage_ = treeNode.page_->GetUniqueName();
    }

    ui::PushID(shortName.c_str());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
    if (treeNode.children_.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (treeNode.page_ && treeNode.page_->GetUniqueName() == selectedPage_)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (selectNextValidPage_)
        ui::SetNextItemOpen(true);
    const bool isOpen = ui::TreeNodeEx(shortName.c_str(), flags);

    // Process clicking
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        if (treeNode.page_)
            selectedPage_ = treeNode.page_->GetUniqueName();
        else
            selectNextValidPage_ = true;
    }

    // Render children
    if (isOpen)
    {
        for (const auto& [shortName, childNode] : treeNode.children_)
            RenderSettingsSubtree(childNode, shortName);
        ui::TreePop();
    }

    ui::PopID();
}

void SettingsTab::RenderCurrentSettingsPage()
{
    auto project = GetProject();
    SettingsManager* settingsManager = project->GetSettingsManager();
    if (SettingsPage* page = settingsManager->FindPage(selectedPage_))
    {
        page->RenderSettings();
        ui::Separator();
        if (ui::Button("Reset to Defaults"))
            page->ResetToDefaults();
    }
}

}
