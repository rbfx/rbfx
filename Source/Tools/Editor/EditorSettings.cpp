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
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Toolbox/SystemUI/Widgets.h>

#include "Editor.h"
#include "Pipeline/Pipeline.h"

namespace Urho3D
{

void Editor::RenderSettingsWindow()
{
    if (!settingsOpen_)
        return;

    if (ui::Begin("Project Settings", &settingsOpen_, ImGuiWindowFlags_NoDocking))
    {
        if (ui::BeginTabBar("Project Categories", ImGuiTabBarFlags_Reorderable))
        {
            settingsTabs_(this);
            ImGui::EndTabBar();
        }
    }
    ui::End();

    if (!settingsOpen_)
    {
        // Settings window is closing.
        auto pipeline = GetSubsystem<Pipeline>();
        if (!pipeline->CookSettings())
            URHO3D_LOGERROR("Cooking settings failed");
    }
}

bool Editor::Serialize(Archive& archive)
{
    if (auto editorBlock = archive.OpenUnorderedBlock("editor"))
    {
        if (auto windowBlock = archive.OpenUnorderedBlock("window"))
        {
            SerializeValue(archive, "pos", windowPos_);
            SerializeValue(archive, "size", windowSize_);
        }
        SerializeValue(archive, "recentProjects", recentProjects_);
        if (!keyBindings_.Serialize(archive))
            return false;
    }
    return true;
}

}
