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

#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <nativefiledialog/nfd.h>
#include <Toolbox/SystemUI/Widgets.h>

#include "Plugins/ModulePlugin.h"
#include "Pipeline/Flavor.h"
#include "Pipeline/Pipeline.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/PreviewTab.h"
#include "Editor.h"
#include "EditorEvents.h"

namespace Urho3D
{

void Editor::RenderMenuBar()
{
    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("File"))
        {
            if (project_)
            {
                if (ui::MenuItem("Save Project", keyBindings_.GetKeyCombination(ActionType::SaveProject)))
                {
                    for (auto& tab : tabs_)
                        tab->SaveResource();
                    project_->SaveProject();
                }
            }

            if (ui::MenuItem("Open/Create Project", keyBindings_.GetKeyCombination(ActionType::OpenProject)))
                OpenOrCreateProject();

            StringVector & recents = recentProjects_;
            // Does not show very first item, which is current project
            if (recents.size() == (project_.NotNull() ? 1 : 0))
            {
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                ui::MenuItem("Recent Projects");
                ui::PopStyleColor();
            }
            else if (ui::BeginMenu("Recent Projects"))
            {
                for (int i = project_.NotNull() ? 1 : 0; i < recents.size(); i++)
                {
                    const ea::string& projectPath = recents[i];

                    if (ui::MenuItem(GetFileNameAndExtension(RemoveTrailingSlash(projectPath)).c_str()))
                        OpenProject(projectPath);

                    if (ui::IsItemHovered())
                        ui::SetTooltip("%s", projectPath.c_str());
                }
                ui::Separator();
                if (ui::MenuItem("Clear All"))
                    recents.clear();
                ui::EndMenu();
            }

            ui::Separator();

            if (project_)
            {
                if (ui::MenuItem("Reset UI"))
                {
                    ea::string projectPath = project_->GetProjectPath();
                    CloseProject();
                    context_->GetSubsystem<FileSystem>()->Delete(projectPath + ".ui.ini");
                    OpenProject(projectPath);
                }

                if (ui::MenuItem("Close Project"))
                {
                    CloseProject();
                }
            }

            if (ui::MenuItem("Exit", keyBindings_.GetKeyCombination(ActionType::Exit)))
                engine_->Exit();

            ui::EndMenu();
        }
        if (project_)
        {
            if (ui::BeginMenu("View"))
            {
                for (auto& tab : tabs_)
                {
                    if (tab->IsUtility())
                    {
                        // Tabs that can not be closed permanently
                        auto open = tab->IsOpen();
                        if (ui::MenuItem(tab->GetUniqueTitle().c_str(), nullptr, &open))
                            tab->SetOpen(open);
                    }
                }
                ui::EndMenu();
            }

            if (ui::BeginMenu("Project"))
            {
                RenderProjectMenu();
                ui::EndMenu();
            }

#if URHO3D_PROFILING
            if (ui::BeginMenu("Tools"))
            {
                if (ui::MenuItem("Profiler"))
                {
                    context_->GetSubsystem<FileSystem>()->SystemSpawn(context_->GetSubsystem<FileSystem>()->GetProgramDir() + "Profiler"
#if _WIN32
                        ".exe"
#endif
                        , {});
                }

                ui::MenuItem("Metrics", NULL, &showMetricsWindow_);

                ui::EndMenu();
            }
#endif
        }

        SendEvent(E_EDITORAPPLICATIONMENU);

        // Scene simulation buttons.
        if (project_)
        {
            // Copied from ToolbarButton()
            auto& g = *ui::GetCurrentContext();
            float dimension = g.FontBaseSize + g.Style.FramePadding.y * 2.0f;
            ui::SetCursorScreenPos(ImVec2{ui::GetMainViewport()->Pos.x + g.IO.DisplaySize.x / 2 - dimension * 4 / 2, ui::GetCursorScreenPos().y});
            if (auto* previewTab = GetTab<PreviewTab>())
                previewTab->RenderButtons();
        }

        ui::EndMainMenuBar();
    }
}

void Editor::RenderProjectMenu()
{
    settingsOpen_ |= ui::MenuItem("Settings");

    if (ui::BeginMenu(ICON_FA_BOXES " Repackage files"))
    {
        auto pipeline = GetSubsystem<Pipeline>();

        if (ui::MenuItem("All Flavors"))
        {
            for (Flavor* flavor : pipeline->GetFlavors())
                pipeline->CreatePaksAsync(flavor);
        }

        for (Flavor* flavor : pipeline->GetFlavors())
        {
            if (ui::MenuItem(flavor->GetName().c_str()))
                pipeline->CreatePaksAsync(flavor);
        }

        ui::EndMenu();
    }
    ui::SetHelpTooltip("(Re)Packages all resources from scratch. Existing packages will be removed!");
}

}
