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

#include "../../Foundation/SettingsTab/PluginsPage.h"

#include "../../Core/IniHelpers.h"

#include <Urho3D/Utility/SceneViewerApplication.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Apply = EditorHotkey{"PluginsPage.Apply"}.Ctrl().Press(KEY_RETURN);
const auto Hotkey_Discard = EditorHotkey{"PluginsPage.Discard"}.Press(KEY_ESCAPE);

}

void Foundation_PluginsPage(Context* context, SettingsTab* settingsTab)
{
    auto project = settingsTab->GetProject();
    auto settingsManager = project->GetSettingsManager();
    settingsManager->AddPage(MakeShared<PluginsPage>(context));
}

PluginsPage::PluginsPage(Context* context)
    : SettingsPage(context)
{
    auto project = GetSubsystem<Project>();
    auto hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_Apply, &PluginsPage::Apply);
    hotkeyManager->BindHotkey(this, Hotkey_Discard, &PluginsPage::Discard);
}

void PluginsPage::Apply()
{
    auto pluginManager = GetSubsystem<PluginManager>();
    if (hasChanges_)
        pluginManager->SetPluginsLoaded(loadedPlugins_);
}

void PluginsPage::Discard()
{
    if (hasChanges_)
        revision_ = 0;
}

void PluginsPage::RenderSettings()
{
    auto pluginManager = GetSubsystem<PluginManager>();

    UpdateAvailablePlugins();
    UpdateLoadedPlugins();

    const unsigned oldHash = MakeHash(loadedPlugins_);

    ui::Separator();

    {
        const IdScopeGuard guardLoadedPlugins("##LoadedPlugins");

        if (ui::SmallButton(ICON_FA_ARROWS_ROTATE))
            pluginManager->Reload();
        if (ui::IsItemHovered())
            ui::SetTooltip("Unload and reload all currently loaded plugins");
        ui::SameLine();
        ui::TextUnformatted("Loaded plugins:");
        ui::Separator();

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
            const IdScopeGuard guardItem(plugin.c_str());
            if (ui::SmallButton(ICON_FA_SQUARE_MINUS))
            {
                pluginsToUnload.insert(plugin);
                hasChanges_ = true;
            }
            ui::SameLine();
            ui::Text("%s", plugin.c_str());
        }
        ea::erase_if(loadedPlugins_, [&](const ea::string& plugin) { return pluginsToUnload.contains(plugin); });
    }

    ui::Separator();

    {
        const IdScopeGuard guardUnloadedPlugins("##UnloadedPlugins");

        if (ui::SmallButton(ICON_FA_ARROWS_ROTATE))
            refreshPlugins_ = true;
        if (ui::IsItemHovered())
            ui::SetTooltip("Refresh the list of available plugins");
        ui::SameLine();
        ui::TextUnformatted("Available plugins:");
        ui::Separator();

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

            const IdScopeGuard guardItem(plugin.c_str());
            if (ui::SmallButton(ICON_FA_SQUARE_PLUS))
            {
                loadedPlugins_.push_back(plugin);
                hasChanges_ = true;
            }
            ui::SameLine();
            ui::Text("%s", plugin.c_str());
        }
    }

    ui::Separator();

    ui::BeginDisabled(!hasChanges_);
    if (ui::Button(ICON_FA_SQUARE_CHECK " Apply"))
        Apply();
    ui::SameLine();
    if (ui::Button(ICON_FA_SQUARE_XMARK " Discard"))
        Discard();
    ui::EndDisabled();

    if (hasChanges_)
        ui::Text(ICON_FA_TRIANGLE_EXCLAMATION " Some changes are not applied yet!");
    else
        ui::NewLine();

    const unsigned newHash = MakeHash(loadedPlugins_);
    if (oldHash != newHash)
    {
        auto project = GetSubsystem<Project>();
        project->MarkUnsaved();
    }
}

void PluginsPage::ResetToDefaults()
{
    const auto oldLoadedPlugins = ea::move(loadedPlugins_);
    loadedPlugins_ = {SceneViewerApplication::GetStaticPluginName()};
    hasChanges_ = loadedPlugins_ != oldLoadedPlugins;
}

void PluginsPage::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

void PluginsPage::UpdateAvailablePlugins()
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

void PluginsPage::UpdateLoadedPlugins()
{
    auto pluginManager = GetSubsystem<PluginManager>();

    if (revision_ == pluginManager->GetRevision())
        return;

    revision_ = pluginManager->GetRevision();
    hasChanges_ = false;
    loadedPlugins_ = pluginManager->GetLoadedPlugins();
}

}
