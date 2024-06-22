// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/ConsoleTab.h"

#include "../Core/IniHelpers.h"

#include <Urho3D/SystemUI/Console.h>

namespace Urho3D
{

namespace
{

const LogLevel logLevels[] = {LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR};

}

void Foundation_ConsoleTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<ConsoleTab>(context));
}

ConsoleTab::ConsoleTab(Context* context)
    : EditorTab(context, "Console", "2c1b8e59-3e21-4a14-bc20-d35af0ba5031", EditorTabFlag::OpenByDefault,
          EditorTabPlacement::DockBottom)
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

    if (ui::MenuItem("Clear"))
    {
        console->Clear();
    }

    if (ui::BeginMenu("Levels"))
    {
        for (const LogLevel level : logLevels)
        {
            const bool visible = console->GetLevelVisible(level);
            if (ui::MenuItem(logLevelNames[level], nullptr, visible))
                console->SetLevelVisible(level, !visible);
        }
        ui::EndMenu();
    }

    contextMenuSeparator_.Add();
}

void ConsoleTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);

    if (auto console = GetSubsystem<Console>())
    {
        for (const LogLevel level : logLevels)
            WriteIntToIni(output, Format("Show_{}", logLevelNames[level]), console->GetLevelVisible(level));
    }
}

void ConsoleTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);

    if (auto console = GetSubsystem<Console>())
    {
        for (const LogLevel level : logLevels)
        {
            if (const auto value = ReadIntFromIni(line, Format("Show_{}", logLevelNames[level])))
                console->SetLevelVisible(level, !!*value);
        }
    }
}

} // namespace Urho3D
