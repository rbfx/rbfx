//
// Copyright (c) 2017-2019 the rbfx project.
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
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_stdlib.h>
#include "Editor.h"
#include "Plugins/ModulePlugin.h"

namespace Urho3D
{
static const VariantType variantTypes[] = {
    VAR_BOOL,
    VAR_INT,
    VAR_INT64,
    VAR_FLOAT,
    VAR_DOUBLE,
    VAR_COLOR,
    VAR_STRING,
};

static const char* variantNames[] = {
    "Bool",
    "Int",
    "Int64",
    "Float",
    "Double",
    "Color",
    "String",
};

static const char* predefinedNames[] = {
    "Select Option Name",
    "Enter Custom",
    EP_AUTOLOAD_PATHS.c_str(),
    EP_BORDERLESS.c_str(),
    EP_DUMP_SHADERS.c_str(),
    EP_FLUSH_GPU.c_str(),
    EP_FORCE_GL2.c_str(),
    EP_FRAME_LIMITER.c_str(),
    EP_FULL_SCREEN.c_str(),
    EP_HEADLESS.c_str(),
    EP_HIGH_DPI.c_str(),
    EP_LOG_LEVEL.c_str(),
    EP_LOG_NAME.c_str(),
    EP_LOG_QUIET.c_str(),
    EP_LOW_QUALITY_SHADOWS.c_str(),
    EP_MATERIAL_QUALITY.c_str(),
    EP_MONITOR.c_str(),
    EP_MULTI_SAMPLE.c_str(),
    EP_ORGANIZATION_NAME.c_str(),
    EP_APPLICATION_NAME.c_str(),
    EP_ORIENTATIONS.c_str(),
    EP_PACKAGE_CACHE_DIR.c_str(),
    EP_RENDER_PATH.c_str(),
    EP_REFRESH_RATE.c_str(),
    EP_RESOURCE_PACKAGES.c_str(),
    EP_RESOURCE_PATHS.c_str(),
    EP_RESOURCE_PREFIX_PATHS.c_str(),
    EP_SHADER_CACHE_DIR.c_str(),
    EP_SHADOWS.c_str(),
    EP_SOUND.c_str(),
    EP_SOUND_BUFFER.c_str(),
    EP_SOUND_INTERPOLATION.c_str(),
    EP_SOUND_MIX_RATE.c_str(),
    EP_SOUND_STEREO.c_str(),
    EP_TEXTURE_ANISOTROPY.c_str(),
    EP_TEXTURE_FILTER_MODE.c_str(),
    EP_TEXTURE_QUALITY.c_str(),
    EP_TOUCH_EMULATION.c_str(),
    EP_TRIPLE_BUFFER.c_str(),
    EP_VSYNC.c_str(),
    EP_WINDOW_HEIGHT.c_str(),
    EP_WINDOW_ICON.c_str(),
    EP_WINDOW_POSITION_X.c_str(),
    EP_WINDOW_POSITION_Y.c_str(),
    EP_WINDOW_RESIZABLE.c_str(),
    EP_WINDOW_MAXIMIZE.c_str(),
    EP_WINDOW_TITLE.c_str(),
    EP_WINDOW_WIDTH.c_str(),
    EP_WORKER_THREADS.c_str(),
    EP_ENGINE_CLI_PARAMETERS.c_str(),
    EP_ENGINE_AUTO_LOAD_SCRIPTS.c_str(),
};

static VariantType predefinedTypes[] = {
    VAR_NONE,   // Select Option Name
    VAR_NONE,   // Enter Custom
    VAR_STRING, // EP_AUTOLOAD_PATHS
    VAR_BOOL,   // EP_BORDERLESS
    VAR_BOOL,   // EP_DUMP_SHADERS
    VAR_BOOL,   // EP_FLUSH_GPU
    VAR_BOOL,   // EP_FORCE_GL2
    VAR_BOOL,   // EP_FRAME_LIMITER
    VAR_BOOL,   // EP_FULL_SCREEN
    VAR_BOOL,   // EP_HEADLESS
    VAR_BOOL,   // EP_HIGH_DPI
    VAR_INT,    // EP_LOG_LEVEL
    VAR_STRING, // EP_LOG_NAME
    VAR_BOOL,   // EP_LOG_QUIET
    VAR_BOOL,   // EP_LOW_QUALITY_SHADOWS
    VAR_INT,    // EP_MATERIAL_QUALITY
    VAR_INT,    // EP_MONITOR
    VAR_INT,    // EP_MULTI_SAMPLE
    VAR_STRING, // EP_ORGANIZATION_NAME
    VAR_STRING, // EP_APPLICATION_NAME
    VAR_STRING, // EP_ORIENTATIONS
    VAR_STRING, // EP_PACKAGE_CACHE_DIR
    VAR_STRING, // EP_RENDER_PATH
    VAR_INT,    // EP_REFRESH_RATE
    VAR_STRING, // EP_RESOURCE_PACKAGES
    VAR_STRING, // EP_RESOURCE_PATHS
    VAR_STRING, // EP_RESOURCE_PREFIX_PATHS
    VAR_STRING, // EP_SHADER_CACHE_DIR
    VAR_BOOL,   // EP_SHADOWS
    VAR_BOOL,   // EP_SOUND
    VAR_INT,    // EP_SOUND_BUFFER
    VAR_BOOL,   // EP_SOUND_INTERPOLATION
    VAR_INT,    // EP_SOUND_MIX_RATE
    VAR_BOOL,   // EP_SOUND_STEREO
    VAR_INT,    // EP_TEXTURE_ANISOTROPY
    VAR_INT,    // EP_TEXTURE_FILTER_MODE
    VAR_INT,    // EP_TEXTURE_QUALITY
    VAR_BOOL,   // EP_TOUCH_EMULATION
    VAR_BOOL,   // EP_TRIPLE_BUFFER
    VAR_BOOL,   // EP_VSYNC
    VAR_INT,    // EP_WINDOW_HEIGHT
    VAR_STRING, // EP_WINDOW_ICON
    VAR_INT,    // EP_WINDOW_POSITION_X
    VAR_INT,    // EP_WINDOW_POSITION_Y
    VAR_BOOL,   // EP_WINDOW_RESIZABLE
    VAR_BOOL,   // EP_WINDOW_MAXIMIZE
    VAR_STRING, // EP_WINDOW_TITLE
    VAR_INT,    // EP_WINDOW_WIDTH
    VAR_INT,    // EP_WORKER_THREADS
    VAR_BOOL,   // EP_ENGINE_CLI_PARAMETERS
    VAR_BOOL,   // EP_ENGINE_AUTO_LOAD_SCRIPTS
};

static_assert(URHO3D_ARRAYSIZE(predefinedNames) == URHO3D_ARRAYSIZE(predefinedNames), "Sizes must match.");

void Editor::RenderSettingsWindow()
{
    if (!settingsOpen_)
        return;
    if (ui::Begin("Project Settings", &settingsOpen_, ImGuiWindowFlags_NoDocking))
    {
        if (ui::BeginTabBar("Project Categories"))
        {
            if (ui::BeginTabItem("General"))
            {
                // Default scene
                ui::PushID("Default Scene");
                struct DefaultSceneState
                {
                    /// A list of scenes present in resource directories.
                    StringVector scenes_;

                    explicit DefaultSceneState(Editor* editor)
                    {
                        editor->GetFileSystem()->ScanDir(scenes_, editor->project_->GetResourcePath(), "*.xml", SCAN_FILES, true);
                        for (auto it = scenes_.begin(); it != scenes_.end();)
                        {
                            if (GetContentType(editor->context_, *it) == CTYPE_SCENE)
                                ++it;
                            else
                                it = scenes_.erase(it);
                        }
                    }
                };
                auto* state = ui::GetUIState<DefaultSceneState>(this);
                if (ui::BeginCombo("Default Scene", project_->GetDefaultSceneName().c_str()))
                {
                    for (const ea::string& resourceName : state->scenes_)
                    {
                        if (ui::Selectable(resourceName.c_str(), project_->GetDefaultSceneName() == resourceName))
                            project_->SetDefaultSceneName(resourceName);
                    }
                    ui::EndCombo();
                }
                if (state->scenes_.empty())
                    ui::SetHelpTooltip("Create a new scene first.", KEY_UNKNOWN);
                ui::SetHelpTooltip("Select a default scene that will be started on application startup.");

                ui::PopID();    // Default Scene

                // Plugins
#if URHO3D_PLUGINS
                ui::PushID("Plugins");
                ui::Separator();
                ui::Text("Active plugins:");
#if URHO3D_STATIC
                static const char* pluginStates[] = {"Loaded"};
#else
                static const char* pluginStates[] = {"Inactive", "Editor", "Editor and Application"};
#endif
                const StringVector& pluginNames = project_->GetPlugins()->GetPluginNames();
                bool hasPlugins = false;
                PluginManager* plugins = project_->GetPlugins();
#if URHO3D_STATIC
                for (Plugin* plugin : plugins->GetPlugins())
                {
                    const ea::string& baseName = plugin->GetName();
                    int currentState = 0;
#else
                for (const ea::string& baseName : pluginNames)
                {
                    Plugin* plugin = plugins->GetPlugin(baseName);
                    bool loaded = plugin != nullptr && plugin->IsLoaded();
                    bool editorOnly = plugin && plugin->IsPrivate();
                    int currentState = loaded ? (editorOnly ? 1 : 2) : 0;
#endif
                    hasPlugins = true;
                    if (ui::Combo(baseName.c_str(), &currentState, pluginStates, URHO3D_ARRAYSIZE(pluginStates)))
                    {
#if !URHO3D_STATIC
                        if (currentState == 0)
                        {
                            if (loaded)
                                plugin->Unload();
                        }
                        else
                        {
                            if (!loaded)
                                plugin = plugins->Load(ModulePlugin::GetTypeStatic(), baseName);

                            if (plugin != nullptr)
                                plugin->SetPrivate(currentState == 1);
                        }
#endif
                    }
#if URHO3D_STATIC
                    ui::SetHelpTooltip("Plugin state is read-only in static builds.");
#endif
                }
                if (!hasPlugins)
                {
                    ui::TextUnformatted("No available files.");
                    ui::SetHelpTooltip("Plugins are shared libraries that have a class inheriting from PluginApplication and "
                                       "define a plugin entry point. Look at Samples/103_GamePlugin for more information.");
                }
                ui::PopID();        // Plugins
#endif
                ui::EndTabItem();   // General
            }
            if (ui::BeginTabItem("Pipeline"))
            {
                auto* pipeline = project_->GetPipeline();
                const auto& style = ui::GetStyle();

                // Add new flavor
                ea::string& newFlavorName = *ui::GetUIState<ea::string>();
                bool canAdd = newFlavorName != Flavor::DEFAULT && !newFlavorName.empty() && pipeline->GetFlavor(newFlavorName) == nullptr;
                if (!canAdd)
                    ui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
                bool addNew = ui::InputText("Flavor Name", &newFlavorName, ImGuiInputTextFlags_EnterReturnsTrue);
                ui::SameLine();
                addNew |= ui::ToolbarButton(ICON_FA_PLUS " Add New");
                if (addNew && canAdd)
                    pipeline->AddFlavor(newFlavorName);
                if (!canAdd)
                    ui::PopStyleColor();

                // Flavor tabs
                if (ui::BeginTabBar("Flavors", ImGuiTabBarFlags_AutoSelectNewTabs))
                {
                    for (Flavor* flavor : pipeline->GetFlavors())
                    {
                        ui::PushID(flavor);
                        ea::string& editBuffer = *ui::GetUIState<ea::string>(flavor->GetName());
                        bool isOpen = true;
                        if (ui::BeginTabItem(flavor->GetName().c_str(), &isOpen, flavor->IsDefault() ? ImGuiTabItemFlags_NoCloseButton | ImGuiTabItemFlags_NoCloseWithMiddleMouseButton : 0))
                        {
                            bool canRename = editBuffer != Flavor::DEFAULT && !editBuffer.empty() && pipeline->GetFlavor(editBuffer) == nullptr;
                            if (flavor->IsDefault() || !canRename)
                                ui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);

                            bool save = ui::InputText("Flavor Name", &editBuffer, ImGuiInputTextFlags_EnterReturnsTrue);
                            ui::SameLine();
                            save |= ui::ToolbarButton(ICON_FA_CHECK);
                            ui::SetHelpTooltip("Rename flavor", KEY_UNKNOWN);
                            if (save && canRename && !flavor->IsDefault())
                                pipeline->RenameFlavor(flavor->GetName(), editBuffer);

                            if (flavor->IsDefault() || !canRename)
                                ui::PopStyleColor();

                            ui::Separator();
                            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            ui::TextUnformatted("Engine Settings:");
                            ui::PushID("Engine Settings");
                            struct NewEntryState
                            {
                                /// Custom name of the new parameter.
                                ea::string customName_;
                                /// Custom type of the new parameter.
                                int customType_ = 0;
                                /// Index of predefined engine parameter.
                                int predefinedItem_ = 0;
                            };

                            auto* state = ui::GetUIState<NewEntryState>();
                            ea::map<ea::string, Variant>& settings = flavor->GetEngineParameters();
                            for (auto it = settings.begin(); it != settings.end();)
                            {
                                const ea::string& settingName = it->first;
                                ui::IdScope idScope(settingName.c_str());
                                Variant& value = it->second;
                                float startPos = ui::GetCursorPosX();
                                ui::TextUnformatted(settingName.c_str());
                                ui::SameLine();
                                ui::SetCursorPosX(startPos + 180_dp + ui::GetStyle().ItemSpacing.x);
                                UI_ITEMWIDTH(100_dp)
                                    RenderSingleAttribute(value);
                                ui::SameLine();
                                ui::SetCursorPosX(startPos + 280_dp + ui::GetStyle().ItemSpacing.x);
                                if (ui::Button(ICON_FA_TRASH))
                                    it = settings.erase(it);
                                else
                                    ++it;
                            }

                            UI_ITEMWIDTH(280_dp)
                                ui::Combo("###Selector", &state->predefinedItem_, predefinedNames, URHO3D_ARRAYSIZE(predefinedNames));

                            ui::SameLine();

                            const char* cantSubmitHelpText = nullptr;
                            if (state->predefinedItem_ == 0)
                                cantSubmitHelpText = "Parameter is not selected.";
                            else if (state->predefinedItem_ == 1)
                            {
                                if (state->customName_.empty())
                                    cantSubmitHelpText = "Custom name can not be empty.";
                                else if (settings.find(state->customName_.c_str()) != settings.end())
                                    cantSubmitHelpText = "Parameter with same name is already added.";
                            }
                            else if (state->predefinedItem_ > 1 && settings.find(predefinedNames[state->predefinedItem_]) !=
                                                                  settings.end())
                                cantSubmitHelpText = "Parameter with same name is already added.";

                            ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[cantSubmitHelpText == nullptr ? ImGuiCol_Text : ImGuiCol_TextDisabled]);
                            if (ui::Button(ICON_FA_CHECK) && cantSubmitHelpText == nullptr)
                            {
                                if (state->predefinedItem_ == 1)
                                    settings.insert({state->customName_.c_str(), Variant{variantTypes[state->customType_]}});
                                else
                                    settings.insert({predefinedNames[state->predefinedItem_], Variant{predefinedTypes[state->predefinedItem_]}});
                                state->customName_.clear();
                                state->customType_ = 0;
                            }
                            ui::PopStyleColor();
                            if (cantSubmitHelpText)
                                ui::SetHelpTooltip(cantSubmitHelpText, KEY_UNKNOWN);

                            if (state->predefinedItem_ == 1)
                            {
                                UI_ITEMWIDTH(180_dp)
                                    ui::InputText("###Key", &state->customName_);

                                // Custom entry type selector
                                ui::SameLine();
                                UI_ITEMWIDTH(100_dp - style.ItemSpacing.x)
                                    ui::Combo("###Type", &state->customType_, variantNames, SDL_arraysize(variantTypes));
                            }
                            ui::PopID();    // Engine Settings

                            ui::EndTabItem();
                        }
                        if (!isOpen && !flavor->IsDefault())
                            flavorPendingRemoval_ = flavor;
                        ui::PopID();    // flavor.c_str()
                    }

                    ui::EndTabBar();
                }

                ui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ui::End();
}

}
