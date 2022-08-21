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

#include "../../Foundation/SettingsTab/LaunchPage.h"

#include "../../Core/IniHelpers.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_LaunchPage(Context* context, SettingsTab* settingsTab)
{
    auto project = settingsTab->GetProject();
    auto settingsManager = project->GetSettingsManager();
    settingsManager->AddPage(MakeShared<LaunchPage>(context));
}

LaunchPage::LaunchPage(Context* context)
    : SettingsPage(context)
    , launchManager_(GetSubsystem<Project>()->GetLaunchManager())
{
}

void LaunchPage::RenderSettings()
{
    if (!launchManager_)
        return;

    pendingConfigurationsRemoved_.clear();

    const unsigned oldHash = MakeHash(launchManager_->GetConfigurations());

    unsigned index = 0;
    for (LaunchConfiguration& config : launchManager_->GetMutableConfigurations())
    {
        RenderConfiguration(index, config);
        ui::Separator();
        ++index;
    }

    if (ui::Button(ICON_FA_SQUARE_PLUS " Add new launch configuration"))
    {
        LaunchConfiguration config;
        config.name_ = GetUnusedConfigurationName();
        launchManager_->AddConfiguration(config);
    }

    ea::sort(pendingConfigurationsRemoved_.begin(), pendingConfigurationsRemoved_.end(), ea::greater<unsigned>{});
    for (const unsigned indexToRemove : pendingConfigurationsRemoved_)
        launchManager_->RemoveConfiguration(indexToRemove);

    const unsigned newHash = MakeHash(launchManager_->GetConfigurations());
    if (oldHash != newHash)
    {
        auto project = GetSubsystem<Project>();
        project->MarkUnsaved();
    }
}

void LaunchPage::RenderConfiguration(unsigned index, LaunchConfiguration& config)
{
    const IdScopeGuard guard{index};

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingConfigurationsRemoved_.push_back(index);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove this launch configuration");

    ui::SameLine();

    const bool isBadName = config.name_.empty() || launchManager_->FindConfiguration(config.name_) != &config;
    const ea::string title = Format("{}{}", isBadName ? ICON_FA_TRIANGLE_EXCLAMATION : "", config.name_);
    if (!ui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ui::InputText("Name", &config.name_);
    RenderMainPlugin(config.mainPlugin_);
}

void LaunchPage::RenderMainPlugin(ea::string& mainPlugin)
{
    if (!ui::BeginCombo("Main Plugin", !mainPlugin.empty() ? mainPlugin.c_str() : LaunchConfiguration::UnspecifiedName.c_str()))
        return;

    if (ui::Selectable(LaunchConfiguration::UnspecifiedName.c_str(), mainPlugin.empty()))
        mainPlugin.clear();

    auto pluginManager = GetSubsystem<PluginManager>();
    for (const ea::string& plugin : pluginManager->GetLoadedPlugins())
    {
        const auto pluginApplication = pluginManager->GetPluginApplication(plugin, true);
        if (!pluginApplication || !pluginApplication->IsMain())
            continue;

        if (ui::Selectable(plugin.c_str(), plugin == mainPlugin))
            mainPlugin = plugin;
    }
    ui::EndCombo();
}

ea::string LaunchPage::GetUnusedConfigurationName() const
{
    ea::string name;
    for (unsigned i = 0; i < 1024; ++i)
    {
        name = Format("Configuration {}", i);
        if (!launchManager_->HasConfiguration(name))
            break;
    }
    return name;
}

}
