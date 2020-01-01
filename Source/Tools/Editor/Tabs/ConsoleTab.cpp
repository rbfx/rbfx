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

#include <Urho3D/SystemUI/Console.h>
#include <Toolbox/SystemUI/Widgets.h>
#include "ConsoleTab.h"
#include "Editor.h"

namespace Urho3D
{

ConsoleTab::ConsoleTab(Context* context)
    : Tab(context)
{
    SetID("2c1b8e59-3e21-4a14-bc20-d35af0ba5031");
    SetTitle("Console");
    isUtility_ = true;

    onTabContextMenu_.Subscribe(this, &ConsoleTab::OnTabContextMenu);
}

bool ConsoleTab::RenderWindowContent()
{
    auto font = GetSubsystem<Editor>()->GetMonoSpaceFont();
    if (font)
        ui::PushFont(font);
    GetSubsystem<Console>()->RenderContent();
    if (font)
        ui::PopFont();
    return true;
}

void ConsoleTab::OnTabContextMenu()
{
    // Inner part of window should have a proper padding, context menu and other controls might depend on it.
    if (ui::BeginPopupContextItem("ConsoleTab context menu"))
    {
        if (ui::BeginMenu("Levels"))
        {
            auto* console = GetSubsystem<Console>();
            for (unsigned i = LOG_TRACE; i < LOG_NONE; i++)
            {
                bool visible = console->GetLevelVisible((LogLevel)i);
                if (ui::MenuItem(logLevelNames[i], nullptr, &visible))
                    console->SetLevelVisible((LogLevel)i, visible);
            }
            ui::EndMenu();
        }

        if (ui::BeginMenu("Loggers"))
        {
            struct State
            {
                StringVector loggers_;

                explicit State(Context* context)
                    : loggers_(context->GetSubsystem<Console>()->GetLoggers())
                {
                }
            };
            auto* state = ui::GetUIState<State>(context_);
            auto* console = GetSubsystem<Console>();
            for (const ea::string& logger : state->loggers_)
            {
                bool visible = console->GetLoggerVisible(logger);
                if (ui::MenuItem(logger.c_str(), nullptr, &visible))
                    console->SetLoggerVisible(logger, visible);
            }
            ui::EndMenu();
        }

        ui::Separator();

        if (ui::MenuItem("Close"))
            open_ = false;

        ui::EndPopup();
    }
}

}
