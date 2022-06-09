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

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_Apply, "PluginsTab.Apply", QUAL_CTRL, KEY_RETURN);
URHO3D_EDITOR_HOTKEY(Hotkey_Discard, "PluginsTab.Discard", QUAL_NONE, KEY_ESCAPE);

}

void Foundation_PluginsTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<PluginsTab>(context));
}

PluginsTab::PluginsTab(Context* context)
    : EditorTab(context, "Plugins", "b1c35ca0-e90f-4f32-9311-d7d349c3ac98",
        EditorTabFlag::None, EditorTabPlacement::DockRight)
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    hotkeyManager->BindHotkey(this, Hotkey_Apply, &PluginsTab::Apply);
    hotkeyManager->BindHotkey(this, Hotkey_Discard, &PluginsTab::Discard);
}

void PluginsTab::Apply()
{
    auto pluginManager = GetSubsystem<PluginManager>();
    if (hasChanges_)
        pluginManager->SetPluginsLoaded(loadedPlugins_);
}

void PluginsTab::Discard()
{
    if (hasChanges_)
        revision_ = 0;
}

void PluginsTab::RenderContent()
{
    auto pluginManager = GetSubsystem<PluginManager>();

    UpdateAvailablePlugins();
    UpdateLoadedPlugins();

    ui::PushID("##LoadedPlugins");

    if (ui::SmallButton(ICON_FA_SQUARE_MINUS))
    {
        loadedPlugins_.clear();
        hasChanges_ = true;
    }
    ui::SameLine();
    ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", "[Unload All]");

    ea::unordered_set<ea::string> pluginsToUnload;
    for (const ea::string& plugin : loadedPlugins_)
    {
        ui::PushID(plugin.c_str());
        if (ui::SmallButton(ICON_FA_SQUARE_MINUS))
        {
            pluginsToUnload.insert(plugin);
            hasChanges_ = true;
        }
        ui::SameLine();
        ui::Text("%s", plugin.c_str());
        ui::PopID();
    }
    ea::erase_if(loadedPlugins_, [&](const ea::string& plugin) { return pluginsToUnload.contains(plugin); });

    ui::PopID();

    ui::Separator();

    ui::PushID("##UnloadedPlugins");
    if (ui::SmallButton(ICON_FA_SQUARE_PLUS))
    {
        auto pluginsToLoad = availablePlugins_;
        for (const ea::string& alreadyLoadedPlugin : loadedPlugins_)
            pluginsToLoad.erase(alreadyLoadedPlugin);
        loadedPlugins_.insert(loadedPlugins_.end(), pluginsToLoad.begin(), pluginsToLoad.end());
        hasChanges_ = true;
    }
    ui::SameLine();
    ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", "[Load All]");

    for (const ea::string& plugin : availablePlugins_)
    {
        // TODO: Optimize?
        if (loadedPlugins_.contains(plugin))
            continue;

        ui::PushID(plugin.c_str());
        if (ui::SmallButton(ICON_FA_SQUARE_PLUS))
        {
            loadedPlugins_.push_back(plugin);
            hasChanges_ = true;
        }
        ui::SameLine();
        ui::Text("%s", plugin.c_str());
        ui::PopID();
    }

    ui::PopID();

    ui::Separator();

    ui::BeginDisabled(!hasChanges_);
    if (ui::Button("Apply"))
        Apply();
    ui::SameLine();
    if (ui::Button("Discard"))
        Discard();
    ui::EndDisabled();

    ui::SameLine();
    if (ui::Button("Refresh List"))
        refreshPlugins_ = true;
    ui::SameLine();
    if (ui::Button("Reload Plugins"))
        pluginManager->Reload();
}

void PluginsTab::UpdateAvailablePlugins()
{
    auto pluginManager = GetSubsystem<PluginManager>();

    if (refreshTimer_.GetMSec(false) >= refreshInterval_)
    {
        refreshTimer_.Reset();
        refreshPlugins_ = true;
    }

    if (refreshPlugins_)
    {
        availablePlugins_.clear();
        refreshPlugins_ = false;

        const auto availableModules = pluginManager->ScanAvailableModules();
        const auto loadedModules = pluginManager->EnumerateLoadedModules();
        availablePlugins_.insert(availableModules.begin(), availableModules.end());
        availablePlugins_.insert(loadedModules.begin(), loadedModules.end());
    }
}

void PluginsTab::UpdateLoadedPlugins()
{
    auto pluginManager = GetSubsystem<PluginManager>();

    if (revision_ == pluginManager->GetRevision())
        return;

    revision_ = pluginManager->GetRevision();
    hasChanges_ = false;
    loadedPlugins_ = pluginManager->GetLoadedPlugins();
}

}
