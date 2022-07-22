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

#include "../Foundation/ConsoleTab.h"

#include <Urho3D/SystemUI/Console.h>

namespace Urho3D
{

void Foundation_ConsoleTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<ConsoleTab>(context));
}

ConsoleTab::ConsoleTab(Context* context)
    : EditorTab(context, "Console", "2c1b8e59-3e21-4a14-bc20-d35af0ba5031",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockBottom)
{
}

void ConsoleTab::RenderContent()
{
    auto console = GetSubsystem<Console>();

    ImFont* font = Project::GetMonoFont();
    if (font)
        ui::PushFont(font);
    console->RenderContent();
    if (font)
        ui::PopFont();
}

void ConsoleTab::RenderContextMenuItems()
{
    auto console = GetSubsystem<Console>();

    contextMenuSeparator_.Reset();
    if (ui::BeginMenu("Levels"))
    {
        for (const LogLevel level : {LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR})
        {
            const bool visible = console->GetLevelVisible(level);
            if (ui::MenuItem(logLevelNames[level], nullptr, visible))
                console->SetLevelVisible(level, !visible);
        }
        ui::EndMenu();
    }

    contextMenuSeparator_.Add();
}

}
