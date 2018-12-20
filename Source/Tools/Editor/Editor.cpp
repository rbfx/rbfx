//
// Copyright (c) 2018 Rokas Kupstys
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
#ifdef WIN32
#include <windows.h>
#endif

#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/LibraryInfo.h>
#include <Urho3D/Core/CommandLine.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_stdlib.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>
#include <Toolbox/ToolboxAPI.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <nativefiledialog/nfd.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "EditorIconCache.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/Scene/EditorSceneSettings.h"
#include "Tabs/UI/UITab.h"
#include "Tabs/InspectorTab.h"
#include "Tabs/HierarchyTab.h"
#include "Tabs/ConsoleTab.h"
#include "Tabs/ResourceTab.h"
#include "Tabs/PreviewTab.h"
#include "Assets/AssetConverter.h"
#include "Assets/CookScene.h"
#include "Assets/ImportAssimp.h"
#include "Assets/Inspector/MaterialInspector.h"

using namespace ui::litterals;

URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::Editor);


namespace Urho3D
{

Editor::Editor(Context* context)
    : Application(context)
{
}

void Editor::Setup()
{
#ifdef _WIN32
    // Required until SDL supports hdpi on windows
    if (HMODULE hLibrary = LoadLibraryA("Shcore.dll"))
    {
        typedef HRESULT(WINAPI*SetProcessDpiAwarenessType)(size_t value);
        if (auto fn = GetProcAddress(hLibrary, "SetProcessDpiAwareness"))
            ((SetProcessDpiAwarenessType)fn)(2);    // PROCESS_PER_MONITOR_DPI_AWARE
        FreeLibrary(hLibrary);
    }
#endif

    // Discover resource prefix path by looking for CoreData and going up.
    for (coreResourcePrefixPath_ = GetFileSystem()->GetProgramDir();;
        coreResourcePrefixPath_ = GetParentPath(coreResourcePrefixPath_))
    {
        if (GetFileSystem()->DirExists(coreResourcePrefixPath_ + "CoreData"))
            break;
        else
        {
#if WIN32
            if (coreResourcePrefixPath_.Length() <= 3)   // Root path of any drive
#else
            if (coreResourcePrefixPath_ == "/")          // Filesystem root
#endif
            {
                URHO3D_LOGERROR("Prefix path not found, unable to continue. Prefix path must contain all of your data "
                                "directories (including CoreData).");
                engine_->Exit();
            }
        }
    }

    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_HEADLESS] = false;
    engineParameters_[EP_FULL_SCREEN] = false;
    engineParameters_[EP_WINDOW_HEIGHT] = 1080;
    engineParameters_[EP_WINDOW_WIDTH] = 1920;
    engineParameters_[EP_LOG_LEVEL] = LOG_DEBUG;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
    engineParameters_[EP_AUTOLOAD_PATHS] = "";
    engineParameters_[EP_RESOURCE_PATHS] = "CoreData;EditorData";
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = coreResourcePrefixPath_;

    SetRandomSeed(Time::GetTimeSinceEpoch());

    // Define custom command line parameters here
    auto& commandLine = GetCommandLineParser();
    CLI::Option* optProject = commandLine.add_option("project", defaultProjectPath_,
        "Project to open or create on startup.");
    optProject->set_custom_option("dir");

    CLI::Option* optConvInput = commandLine.add_option("--converter-input", converterInput_,
        "Input resource resource name.");
    CLI::Option* optConverter = commandLine.add_option("--converter", converterName_,
        "Run a specified converter and exit immediately.");
    optConverter->requires(optProject, optConvInput);
}

void Editor::Start()
{
    // Register factories
    context_->RegisterFactory<EditorIconCache>();
    context_->RegisterFactory<SceneTab>();
    context_->RegisterFactory<UITab>();
    context_->RegisterFactory<ConsoleTab>();
    context_->RegisterFactory<HierarchyTab>();
    context_->RegisterFactory<InspectorTab>();
    context_->RegisterFactory<ResourceTab>();
    context_->RegisterFactory<PreviewTab>();
    // Register converter factories
    context_->RegisterFactory<ImportAssimp>();
    context_->RegisterFactory<CookScene>();
    // Register other types
    RegisterToolboxTypes(context_);
    EditorSceneSettings::RegisterObject(context_);
    // Register subsystems
    context_->RegisterSubsystem(this);

    Inspectable::Material::RegisterObject(context_);

    GetLog()->SetTimeStampFormat("%H:%M:%S");

    if (!converterName_.Empty())
        InitializeConverter();
    else
        InitializeEditor();
}

void Editor::InitializeEditor()
{
    context_->RegisterSubsystem(new SceneManager(context_));
    context_->RegisterSubsystem(new EditorIconCache(context_));
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetMouseVisible(true);

    GetCache()->SetAutoReloadResources(true);
    engine_->SetAutoExit(false);

    SubscribeToEvent(E_UPDATE, std::bind(&Editor::OnUpdate, this, _2));

    // Creates console but makes sure it's UI is not rendered. Console rendering is done manually in editor.
    auto* console = engine_->CreateConsole();
    console->SetAutoVisibleOnError(false);
    GetFileSystem()->SetExecuteConsoleCommands(false);
    SubscribeToEvent(E_CONSOLECOMMAND, std::bind(&Editor::OnConsoleCommand, this, _2));
    console->RefreshInterpreters();

    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
        // Opening a new project must be done at the point when SystemUI is not in use. End of the frame is a good
        // candidate. This subsystem will be recreated.
        if (!pendingOpenProject_.Empty())
        {
            CloseProject();
            // Reset SystemUI so that imgui loads it's config proper.
            context_->RemoveSubsystem<SystemUI>();
            context_->RegisterSubsystem(new SystemUI(context_));
            SetupSystemUI();

            project_ = new Project(context_);
            context_->RegisterSubsystem(project_);
            bool loaded = project_->LoadProject(pendingOpenProject_);
            // SystemUI has to be started after loading project, because project sets custom settings file path. Starting
            // subsystem reads this file and loads settings.
            GetSystemUI()->Start();
            if (loaded)
                loadDefaultLayout_ = project_->IsNewProject();
            else
                CloseProject();
            pendingOpenProject_.Clear();
        }
    });
    SubscribeToEvent(E_EXITREQUESTED, [this](StringHash, VariantMap&) {
        auto* preview = GetTab<PreviewTab>();
        if (preview->GetSceneSimulationStatus() == SCENE_SIMULATION_STOPPED)
        {
            exiting_ = true;
            if (project_.NotNull())
                project_->SaveProject();
        }
        else
            preview->Stop();
    });
    if (!defaultProjectPath_.Empty())
    {
        ui::GetIO().IniFilename = nullptr;  // Avoid creating imgui.ini in some cases
        pendingOpenProject_ = defaultProjectPath_.CString();
    }
    else
        SetupSystemUI();
}

void Editor::InitializeConverter()
{
    project_ = new Project(context_);
    context_->RegisterSubsystem(project_);
    if (!project_->LoadProject(defaultProjectPath_))
    {
        URHO3D_LOGERRORF("Loading project '%s' failed.", pendingOpenProject_.CString());
        exitCode_ = EXIT_FAILURE;
        engine_->Exit();
        return;
    }

    SharedPtr<ImportAsset> converter = DynamicCast<ImportAsset>(context_->CreateObject(converterName_));
    if (converter.Null())
    {
        URHO3D_LOGERRORF("Converter '%s' does not exist.", converterName_.CString());
        exitCode_ = EXIT_FAILURE;
        engine_->Exit();
        return;
    }

    if (!converter->RunConverter(converterInput_))
        exitCode_ = EXIT_FAILURE;

    engine_->Exit();
}

void Editor::Stop()
{
    if (auto* manager = GetSubsystem<SceneManager>())
        manager->UnloadAll();
    CloseProject();
    context_->RemoveSubsystem<WorkQueue>(); // Prevents deadlock when unloading plugin AppDomain in managed host.
}

void Editor::OnUpdate(VariantMap& args)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, flags);
    ImGui::PopStyleVar();

    RenderMenuBar();

    dockspaceId_ = ui::GetID("Root");
    ui::DockSpace(dockspaceId_);

    auto tabsCopy = tabs_;
    bool hasModified = false;
    for (auto& tab : tabsCopy)
    {
        if (tab->RenderWindow())
        {
            // Only active window may override another active window
            if (activeTab_ != tab && tab->IsActive())
            {
                activeTab_ = tab;
                tab->OnFocused();
            }
            hasModified |= tab->IsModified();
        }
        else if (!tab->IsUtility())
            // Content tabs get closed permanently
            tabs_.Erase(tabs_.Find(tab));
    }

    if (!activeTab_.Expired())
    {
        activeTab_->OnActiveUpdate();
    }

    if (loadDefaultLayout_ && project_.NotNull())
    {
        loadDefaultLayout_ = false;
        LoadDefaultLayout();
    }

    HandleHotkeys();

    ui::End();
    ImGui::PopStyleVar();

    // Dialog for a warning when application is being closed with unsaved resources.
    if (exiting_)
    {
        if (!hasModified)
            engine_->Exit();
        else if (!ui::IsPopupOpen("Save All?"))
            ui::OpenPopup("Save All?");
    }

    if (ui::BeginPopupModal("Save All?", &exiting_, ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_Popup))
    {
        ui::TextUnformatted("You have unsaved resources. Save them before exiting?");

        if (ui::Button(ICON_FA_SAVE " Save & Close"))
        {
            for (auto& tab : tabs_)
            {
                if (tab->IsModified())
                    tab->SaveResource();
            }
            engine_->Exit();
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_EXCLAMATION_TRIANGLE " Close without saving"))
            engine_->Exit();
        ui::SetHelpTooltip(ICON_FA_EXCLAMATION_TRIANGLE " All unsaved changes will be lost!", KEY_UNKNOWN);

        ui::SameLine();

        if (ui::Button(ICON_FA_TIMES " Cancel"))
        {
            exiting_ = false;
            ui::CloseCurrentPopup();
        }

        ui::EndPopup();
    }
}

void Editor::RenderMenuBar()
{
    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("File"))
        {
            if (project_.NotNull())
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

            if (project_.NotNull())
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
        if (project_.NotNull())
        {
            if (ui::BeginMenu("View"))
            {
                for (auto& tab : tabs_)
                {
                    if (tab->IsUtility())
                    {
                        // Tabs that can not be closed permanently
                        auto open = tab->IsOpen();
                        if (ui::MenuItem(tab->GetUniqueTitle().CString(), nullptr, &open))
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
        if (project_.NotNull())
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

Tab* Editor::CreateTab(StringHash type)
{
    SharedPtr<Tab> tab(DynamicCast<Tab>(context_->CreateObject(type)));
    tabs_.Push(tab);
    return tab.Get();
}

StringVector Editor::GetObjectsByCategory(const String& category)
{
    StringVector result;
    const auto& factories = context_->GetObjectFactories();
    auto it = context_->GetObjectCategories().Find(category);
    if (it != context_->GetObjectCategories().End())
    {
        for (const StringHash& type : it->second_)
        {
            auto jt = factories.Find(type);
            if (jt != factories.End())
                result.Push(jt->second_->GetTypeName());
        }
    }
    return result;
}

void Editor::OnConsoleCommand(VariantMap& args)
{
    using namespace ConsoleCommand;
    if (args[P_COMMAND].GetString() == "revision")
        URHO3D_LOGINFOF("Engine revision: %s", GetRevision());
}

void Editor::CreateDefaultTabs()
{
    tabs_.Clear();
    tabs_.EmplaceBack(new InspectorTab(context_));
    tabs_.EmplaceBack(new HierarchyTab(context_));
    tabs_.EmplaceBack(new ResourceTab(context_));
    tabs_.EmplaceBack(new ConsoleTab(context_));
    tabs_.EmplaceBack(new PreviewTab(context_));
    tabs_.EmplaceBack(new SceneTab(context_));
}

void Editor::LoadDefaultLayout()
{
    CreateDefaultTabs();

    auto* inspector = GetTab<InspectorTab>();
    auto* hierarchy = GetTab<HierarchyTab>();
    auto* resources = GetTab<ResourceTab>();
    auto* console = GetTab<ConsoleTab>();
    auto* preview = GetTab<PreviewTab>();
    auto* scene = GetTab<SceneTab>();

    ImGui::DockBuilderRemoveNode(dockspaceId_);
    ImGui::DockBuilderAddNode(dockspaceId_, ui::GetMainViewport()->Size);

    ImGuiID dock_main_id = dockspaceId_;
    ImGuiID dockHierarchy = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
    ImGuiID dockResources = ImGui::DockBuilderSplitNode(dockHierarchy, ImGuiDir_Down, 0.40f, nullptr, &dockHierarchy);
    ImGuiID dockInspector = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
    ImGuiID dockLog = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

    ImGui::DockBuilderDockWindow(hierarchy->GetUniqueTitle().CString(), dockHierarchy);
    ImGui::DockBuilderDockWindow(resources->GetUniqueTitle().CString(), dockResources);
    ImGui::DockBuilderDockWindow(console->GetUniqueTitle().CString(), dockLog);
    ImGui::DockBuilderDockWindow(scene->GetUniqueTitle().CString(), dock_main_id);
    ImGui::DockBuilderDockWindow(preview->GetUniqueTitle().CString(), dock_main_id);
    ImGui::DockBuilderDockWindow(inspector->GetUniqueTitle().CString(), dockInspector);
    ImGui::DockBuilderFinish(dockspaceId_);
}

void Editor::OpenProject(const String& projectPath)
{
    pendingOpenProject_ = projectPath;
}

void Editor::CloseProject()
{
    context_->RemoveSubsystem<Project>();
    tabs_.Clear();
    project_.Reset();
}

void Editor::HandleHotkeys()
{
    if (ui::IsAnyItemActive())
        return;

    auto* input = GetInput();
    if (input->GetKeyDown(KEY_CTRL))
    {
        if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
        {
            VariantMap args;
            args[Undo::P_TIME] = M_MAX_UNSIGNED;
            SendEvent(E_REDO, args);
            Variant manager;
            if (args.TryGetValue(Undo::P_MANAGER, manager))
                ((Undo::Manager*)manager.GetPtr())->Redo();
        }
        else if (input->GetKeyPress(KEY_Z))
        {
            VariantMap args;
            args[Undo::P_TIME] = 0;
            SendEvent(E_UNDO, args);
            Variant manager;
            if (args.TryGetValue(Undo::P_MANAGER, manager))
                ((Undo::Manager*)manager.GetPtr())->Undo();
        }
    }
}

void Editor::RenderProjectMenu()
{
#if URHO3D_PLUGINS
    if (ui::BeginMenu("Plugins"))
    {
        ui::PushID("Plugins");
        const StringVector& pluginNames = project_->GetPlugins()->GetPluginNames();
        if (pluginNames.Size() == 0)
        {
            ui::TextUnformatted("No available files.");
            ui::SetHelpTooltip("Plugins are shared libraries that have a class inheriting from PluginApplication and "
                               "define a plugin entry point. Look at Samples/103_GamePlugin for more information.");
        }
        else
        {
            for (const String& baseName : pluginNames)
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
                ui::TextUnformatted(baseName.CString());
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
        if (sceneNames->Empty())
        {
            GetFileSystem()->ScanDir(*sceneNames, project_->GetResourcePath(), "*.xml", SCAN_FILES, true);
            for (auto it = sceneNames->Begin(); it != sceneNames->End();)
            {
                if (GetContentType(*it) == CTYPE_SCENE)
                    ++it;
                else
                    it = sceneNames->Erase(it);
            }
        }

        for (const String& resourceName : *sceneNames)
        {
            bool isDefaultScene = resourceName == project_->GetDefaultSceneName();
            if (ui::Checkbox(resourceName.CString(), &isDefaultScene))
            {
                if (isDefaultScene)
                    project_->SetDefaultSceneName(resourceName);
            }
        }

        if (sceneNames->Empty())
            ui::TextUnformatted("Create a new scene first.");

        ui::PopID();    // Main Scene
        ui::EndMenu();
    }

    ui::Separator();

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

        struct NewEntryState
        {
            std::string fieldName;
            int variantTypeIndex = 0;
            bool insertingNew = false;
        };

        auto* state = ui::GetUIState<NewEntryState>();
        auto& settings = project_->GetDefaultEngineSettings();
        for (auto it = settings.Begin(); it != settings.End();)
        {
            const String& settingName = it->first_;
            ui::IdScope idScope(settingName.CString());
            Variant& value = it->second_;
            float startPos = ui::GetCursorPosX();
            ui::TextUnformatted(settingName.CString());
            ui::SameLine();
            ui::SetCursorPosX(startPos = startPos + 180_dpx + ui::GetStyle().ItemSpacing.x);
            UI_ITEMWIDTH(100_dpx)
                RenderSingleAttribute(value);
            ui::SameLine();
            ui::SetCursorPosX(startPos + 100_dpx + ui::GetStyle().ItemSpacing.x);
            if (ui::Button(ICON_FA_TRASH))
                it = settings.Erase(it);
            else
                ++it;
        }

        UI_ITEMWIDTH(180_dpx)
            ui::InputText("###Key", &state->fieldName);
        ui::SameLine();
        UI_ITEMWIDTH(100_dpx)
            ui::Combo("###Type", &state->variantTypeIndex, variantNames, SDL_arraysize(variantTypes));
        ui::SameLine();
        if (ui::Button(ICON_FA_CHECK))
        {
            if (settings.Find(state->fieldName.c_str()) == settings.End())   // TODO: Show warning about duplicate name
            {
                settings.Insert({state->fieldName.c_str(), Variant{variantTypes[state->variantTypeIndex]}});
                state->fieldName.clear();
                state->variantTypeIndex = 0;
                state->insertingNew = false;
            }
        }

        ui::EndMenu();
    }
}

Tab* Editor::GetTabByName(const String& uniqueName)
{
    for (auto& tab : tabs_)
    {
        if (tab->GetUniqueName() == uniqueName)
            return tab.Get();
    }
    return nullptr;
}

Tab* Editor::GetTabByResource(const String& resourceName)
{
    for (auto& tab : tabs_)
    {
        auto resource = DynamicCast<BaseResourceTab>(tab);
        if (resource.NotNull() && resource->GetResourceName() == resourceName)
            return resource.Get();
    }
    return nullptr;
}

Tab* Editor::GetTab(StringHash type)
{
    for (auto& tab : tabs_)
    {
        if (tab->GetType() == type)
            return tab.Get();
    }
    return nullptr;
}

void Editor::SetupSystemUI()
{
    static ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    static ImWchar notoSansRanges[] = {0x20, 0x52f, 0x1ab0, 0x2189, 0x2c60, 0x2e44, 0xa640, 0xab65, 0};
    static ImWchar notoMonoRanges[] = {0x20, 0x513, 0x1e00, 0x1f4d, 0};
    GetSystemUI()->ApplyStyleDefault(true, 1.0f);
    GetSystemUI()->AddFont("Fonts/NotoSans-Regular.ttf", notoSansRanges, 16.f);
    GetSystemUI()->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 0, true);
    monoFont_ = GetSystemUI()->AddFont("Fonts/NotoMono-Regular.ttf", notoMonoRanges, 14.f);
    GetSystemUI()->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 0, true);
    ui::GetStyle().WindowRounding = 3;
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
    auto& io = ui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.ConfigResizeWindowsFromEdges = true;

    // TODO: Make configurable.
    auto& style = ImGui::GetStyle();
    style.FrameBorderSize = 0;
    style.WindowBorderSize = 0;
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.40f, 0.40f, 0.40f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.78f, 0.88f, 1.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.44f, 0.44f, 0.44f, 0.35f);


}

}
