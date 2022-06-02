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
#include "../Foundation/PluginsTab.h"

#include <EASTL/sort.h>

namespace Urho3D
{

void Foundation_PluginsTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<PluginsTab>(context));
}

PluginsTab::PluginsTab(Context* context)
    : EditorTab(context, "Plugins", "b1c35ca0-e90f-4f32-9311-d7d349c3ac98",
        EditorTabFlag::None, EditorTabPlacement::DockCenter)
{
}

void PluginsTab::RenderContent()
{
    UpdateList();

    ui::PushID("##Plugins");
    auto pluginManager = GetSubsystem<PluginManager>();

    bool pluginsChanged = false;
    StringVector enabledPlugins;
    for (const ea::string& name : availablePlugins_)
    {
        bool isLoaded = pluginManager->IsPluginLoaded(name);
        if (ui::Checkbox(name.c_str(), &isLoaded))
            pluginsChanged = true;
        if (isLoaded)
            enabledPlugins.push_back(name);
    }
    if (pluginsChanged)
        pluginManager->SetPluginsLoaded(enabledPlugins);

    ui::PopID();

    ui::Separator();
    if (ui::Button("Refresh List"))
        refreshPlugins_ = true;
    ui::SameLine();
    if (ui::Button("Reload Plugins"))
        pluginManager->Reload();

    ui::Text("TODO: Plugins can be reordered, but UI doesn't support it");
}

void PluginsTab::UpdateList()
{
    if (refreshTimer_.GetMSec(false) >= refreshInterval_)
    {
        refreshTimer_.Reset();
        refreshPlugins_ = true;
    }

    if (refreshPlugins_)
    {
        availablePlugins_.clear();
        refreshPlugins_ = false;

        auto pluginManager = GetSubsystem<PluginManager>();
        availablePlugins_ = pluginManager->ScanAvailableModules();
        availablePlugins_.append(pluginManager->EnumerateLoadedModules());
        ea::sort(availablePlugins_.begin(), availablePlugins_.end());
        availablePlugins_.erase(ea::unique(availablePlugins_.begin(), availablePlugins_.end()), availablePlugins_.end());
    }
}

}
