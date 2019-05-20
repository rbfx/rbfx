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

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui.h>
#include <ImGui/imgui_stdlib.h>
#include <nativefiledialog/nfd.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/PreviewTab.h"
#include "Editor.h"
#include "EditorEvents.h"


using namespace ui::litterals;


namespace Urho3D
{

void Editor::RenderMenuBar()
{
    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("File"))
        {
            if (project_)
            {
                if (ui::MenuItem("Save Project"))
                {
                    for (auto& tab : tabs_)
                        tab->SaveResource();
                    project_->SaveProject();
                }
            }

            if (ui::MenuItem("Open/Create Project"))
            {
                nfdchar_t* projectDir = nullptr;
                if (NFD_PickFolder("", &projectDir) == NFD_OKAY)
                {
                    OpenProject(projectDir);
                    NFD_FreePath(projectDir);
                }
            }

            ui::Separator();

            if (project_)
            {
                if (ui::MenuItem("Close Project"))
                {
                    CloseProject();
                }
            }

            if (ui::MenuItem("Exit"))
                engine_->Exit();

            ui::EndMenu();
        }
        if (project_)
        {
            if (ui::BeginMenu("View"))
            {
                for (auto& tab : tabs_)
                {
                    if (tab->IsUtility())
                    {
                        // Tabs that can not be closed permanently
                        auto open = tab->IsOpen();
                        if (ui::MenuItem(tab->GetUniqueTitle().c_str(), nullptr, &open))
                            tab->SetOpen(open);
                    }
                }
                ui::EndMenu();
            }

            if (ui::BeginMenu("Project"))
            {
                RenderProjectMenu();
                ui::EndMenu();
            }

#if URHO3D_PROFILING
            if (ui::BeginMenu("Tools"))
            {
                if (ui::MenuItem("Profiler"))
                {
                    GetFileSystem()->SystemSpawn(GetFileSystem()->GetProgramDir() + "Profiler"
#if _WIN32
                        ".exe"
#endif
                        , {});
                }
                ui::EndMenu();
            }
#endif
        }

        SendEvent(E_EDITORAPPLICATIONMENU);

        // Scene simulation buttons.
        if (project_)
        {
            // Copied from ToolbarButton()
            auto& g = *ui::GetCurrentContext();
            float dimension = g.FontBaseSize + g.Style.FramePadding.y * 2.0f;
            ui::SetCursorScreenPos({ui::GetIO().DisplaySize.x / 2 - dimension * 4 / 2, ui::GetCursorScreenPos().y});
            if (auto* previewTab = GetTab<PreviewTab>())
                previewTab->RenderButtons();
        }

        ui::EndMainMenuBar();
    }
}

void Editor::RenderProjectMenu()
{
#if URHO3D_PLUGINS
    if (ui::BeginMenu("Plugins"))
    {
        ui::PushID("Plugins");
        const StringVector& pluginNames = project_->GetPlugins()->GetPluginNames();
        if (pluginNames.size() == 0)
        {
            ui::TextUnformatted("No available files.");
            ui::SetHelpTooltip("Plugins are shared libraries that have a class inheriting from PluginApplication and "
                               "define a plugin entry point. Look at Samples/103_GamePlugin for more information.");
        }
        else
        {
            for (const ea::string& baseName : pluginNames)
            {
                PluginManager* plugins = project_->GetPlugins();
                Plugin* plugin = plugins->GetPlugin(baseName);
                bool loaded = plugin != nullptr;
                bool editorOnly = plugin && plugin->GetFlags() & PLUGIN_PRIVATE;

                ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
                if (ui::EditorToolbarButton(ICON_FA_BATTERY_EMPTY, "Inactive", !loaded) && loaded)
                    plugins->Unload(plugin);

                if (ui::EditorToolbarButton(ICON_FA_BATTERY_HALF, "Editor-only", loaded && editorOnly))
                {
                    if (!loaded)
                    {
                        plugins->Load(baseName);
                        plugin = plugins->GetPlugin(baseName);
                    }
                    plugin->SetFlags(plugin->GetFlags() | PLUGIN_PRIVATE);
                }
                if (ui::EditorToolbarButton(ICON_FA_BATTERY_FULL, "Editor and Game", loaded && !editorOnly))
                {
                    if (!loaded)
                    {
                        plugins->Load(baseName);
                        plugin = plugins->GetPlugin(baseName);
                    }
                    plugin->SetFlags(plugin->GetFlags() & ~PLUGIN_PRIVATE);
                }
                ui::PopStyleVar();
                ui::SameLine();
                ui::TextUnformatted(baseName.c_str());
            }
        }
        ui::PopID();    // Plugins
        ui::EndMenu();
    }
#endif
    if (ui::BeginMenu("Main Scene"))
    {
        ui::PushID("Main Scene");
        auto* sceneNames = ui::GetUIState<StringVector>();
        if (sceneNames->empty())
        {
            GetFileSystem()->ScanDir(*sceneNames, project_->GetResourcePath(), "*.xml", SCAN_FILES, true);
            for (auto it = sceneNames->begin(); it != sceneNames->end();)
            {
                if (GetContentType(*it) == CTYPE_SCENE)
                    ++it;
                else
                    it = sceneNames->erase(it);
            }
        }

        for (const ea::string& resourceName : *sceneNames)
        {
            bool isDefaultScene = resourceName == project_->GetDefaultSceneName();
            if (ui::Checkbox(resourceName.c_str(), &isDefaultScene))
            {
                if (isDefaultScene)
                    project_->SetDefaultSceneName(resourceName);
            }
        }

        if (sceneNames->empty())
            ui::TextUnformatted("Create a new scene first.");

        ui::PopID();    // Main Scene
        ui::EndMenu();
    }

    if (ui::BeginMenu("Settings"))
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
            EP_WINDOW_TITLE.c_str(),
            EP_WINDOW_WIDTH.c_str(),
            EP_WORKER_THREADS.c_str(),
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
            VAR_STRING, // EP_WINDOW_TITLE
            VAR_INT,    // EP_WINDOW_WIDTH
            VAR_INT,    // EP_WORKER_THREADS
        };

        static_assert(URHO3D_ARRAYSIZE(predefinedNames) == URHO3D_ARRAYSIZE(predefinedNames), "Sizes must match.");

        struct NewEntryState
        {
            ea::string customName;
            int customType = 0;
            int predefinedItem = 0;
        };

        auto* state = ui::GetUIState<NewEntryState>();
        auto& settings = project_->GetDefaultEngineSettings();
        for (auto it = settings.begin(); it != settings.end();)
        {
            const ea::string& settingName = it->first;
            ui::IdScope idScope(settingName.c_str());
            Variant& value = it->second;
            float startPos = ui::GetCursorPosX();
            ui::TextUnformatted(settingName.c_str());
            ui::SameLine();
            ui::SetCursorPosX(startPos = startPos + 180_dpx + ui::GetStyle().ItemSpacing.x);
            UI_ITEMWIDTH(100_dpx)
                RenderSingleAttribute(value);
            ui::SameLine();
            ui::SetCursorPosX(startPos + 100_dpx + ui::GetStyle().ItemSpacing.x);
            if (ui::Button(ICON_FA_TRASH))
                it = settings.erase(it);
            else
                ++it;
        }

        UI_ITEMWIDTH(280_dpx)
            ui::Combo("###Selector", &state->predefinedItem, predefinedNames, URHO3D_ARRAYSIZE(predefinedNames));

        ui::SameLine();

        const char* cantSubmitHelpText = nullptr;
        if (state->predefinedItem == 0)
            cantSubmitHelpText = "Parameter is not selected.";
        else if (state->predefinedItem == 1)
        {
            if (state->customName.empty())
                cantSubmitHelpText = "Custom name can not be empty.";
            else if (settings.find(state->customName.c_str()) != settings.end())
                cantSubmitHelpText = "Parameter with same name is already added.";
        }
        else if (state->predefinedItem > 1 && settings.find(predefinedNames[state->predefinedItem]) !=
                                                  settings.end())
            cantSubmitHelpText = "Parameter with same name is already added.";

        ui::PushStyleColor(ImGuiCol_Button, ui::GetStyle().Colors[cantSubmitHelpText == nullptr ? ImGuiCol_Button : ImGuiCol_TextDisabled]);
        if (ui::Button(ICON_FA_CHECK) && cantSubmitHelpText == nullptr)
        {
            if (state->predefinedItem == 1)
                settings.insert({state->customName.c_str(), Variant{variantTypes[state->customType]}});
            else
                settings.insert(
                    {predefinedNames[state->predefinedItem], Variant{predefinedTypes[state->predefinedItem]}});
            state->customName.clear();
            state->customType = 0;
        }
        ui::PopStyleColor();
        if (cantSubmitHelpText)
            ui::SetHelpTooltip(cantSubmitHelpText, KEY_UNKNOWN);

        if (state->predefinedItem == 1)
        {
            UI_ITEMWIDTH(180_dpx)
                ui::InputText("###Key", &state->customName);

            // Custom entry type selector
            ui::SameLine();
            UI_ITEMWIDTH(100_dpx)
                ui::Combo("###Type", &state->customType, variantNames, SDL_arraysize(variantTypes));
        }
        ui::EndMenu();
    }

    ui::Separator();

    if (ui::MenuItem(ICON_FA_BOXES " Package files"))
    {
        GetSubsystem<Project>()->GetPipeline().CreatePaksAsync();
    }
}

}
