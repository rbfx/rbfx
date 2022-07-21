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

#include "Editor.h"

#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <IconFontCppHeaders/IconsFontAwesome6.h>
#include <nativefiledialog/nfd.h>
#include <Toolbox/SystemUI/Widgets.h>

namespace Urho3D
{

void Editor::RenderMenuBar()
{
    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("Project"))
        {
            if (projectEditor_)
            {
                projectEditor_->RenderProjectMenu();
                ui::Separator();
            }

            if (ui::MenuItem("Open or Create Project"))
                OpenOrCreateProject();

            StringVector & recents = recentProjects_;
            // Does not show very first item, which is current project
            if (recents.size() == (projectEditor_.NotNull() ? 1 : 0))
            {
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                ui::MenuItem("Recent Projects");
                ui::PopStyleColor();
            }
            else if (ui::BeginMenu("Recent Projects"))
            {
                for (int i = projectEditor_.NotNull() ? 1 : 0; i < recents.size(); i++)
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

            if (projectEditor_)
            {
                if (ui::MenuItem("Close Project"))
                    pendingCloseProject_ = true;
            }

            ui::Separator();

            if (ui::MenuItem("Exit"))
                SendEvent(E_EXITREQUESTED);

            ui::EndMenu();
        }
        if (projectEditor_)
            projectEditor_->RenderMainMenu();

        ui::EndMainMenuBar();
    }
}

}
