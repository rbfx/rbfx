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

#include <CLI11/CLI11.hpp>

#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>
#include <Toolbox/ToolboxAPI.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <nativefiledialog/nfd.h>
#include <Toolbox/SystemUI/Widgets.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "EditorIconCache.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/Scene/SceneSettings.h"
#include "Tabs/UI/UITab.h"
#include "Tabs/InspectorTab.h"
#include "Tabs/HierarchyTab.h"
#include "Tabs/ConsoleTab.h"
#include "Tabs/ResourceTab.h"
#include "Tabs/PreviewTab.h"
#include "Assets/AssetConverter.h"
#include "Assets/Inspector/MaterialInspector.h"



URHO3D_DEFINE_APPLICATION_MAIN(Editor);


namespace Urho3D
{

static std::string defaultProjectPath;

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
    commandLine.add_option("project", defaultProjectPath, "Project to open or create on startup.")
            ->set_custom_option("dir");
}

void Editor::Start()
{
    context_->RegisterFactory<EditorIconCache>();
    context_->RegisterFactory<SceneTab>();
    context_->RegisterFactory<UITab>();
    context_->RegisterFactory<ConsoleTab>();
    context_->RegisterFactory<HierarchyTab>();
    context_->RegisterFactory<InspectorTab>();
    context_->RegisterFactory<ResourceTab>();
    context_->RegisterFactory<PreviewTab>();

    Inspectable::Material::RegisterObject(context_);

    context_->RegisterSubsystem(new EditorIconCache(context_));
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetMouseVisible(true);
    RegisterToolboxTypes(context_);
    context_->RegisterSubsystem(this);
    SceneSettings::RegisterObject(context_);

    SetupSystemUI();

    GetCache()->SetAutoReloadResources(true);

    SubscribeToEvent(E_UPDATE, std::bind(&Editor::OnUpdate, this, _2));

    // Creates console but makes sure it's UI is not rendered. Console rendering is done manually in editor.
    auto* console = engine_->CreateConsole();
    console->SetAutoVisibleOnError(false);
    GetFileSystem()->SetExecuteConsoleCommands(false);
    SubscribeToEvent(E_CONSOLECOMMAND, std::bind(&Editor::OnConsoleCommand, this, _2));
    console->RefreshInterpreters();

    // Prepare editor for loading new project.
    SubscribeToEvent(E_EDITORPROJECTLOADINGSTART, [&](StringHash, VariantMap&) {
        tabs_.Clear();
    });

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

    if (!defaultProjectPath.empty())
        pendingOpenProject_ = defaultProjectPath.c_str();
}

void Editor::Stop()
{
    CloseProject();
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
    for (auto& tab : tabsCopy)
    {
        if (tab->RenderWindow())
        {
            if (tab->IsRendered())
            {
                if (activeTab_ != tab)
                {
                    // Only active window may override another active window
                    if (tab->IsActive())
                    {
                        activeTab_ = tab;
                        if (SceneTab* sceneTab = tab->Cast<SceneTab>())
                            lastActiveScene_ = sceneTab;
                        tab->OnFocused();
                    }
                }
            }
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
                if (ui::BeginMenu("Plugins"))
                {
                    RenderProjectPluginsMenu();
                    ui::EndMenu();
                }
                ui::EndMenu();
            }

            if (ui::BeginMenu("Tools"))
            {
#if URHO3D_PROFILING
                if (ui::MenuItem("Profiler"))
                {
                    GetFileSystem()->SystemSpawn(GetFileSystem()->GetProgramDir() + "Profiler"
#if _WIN32
                        ".exe"
#endif
                        , {});
                }
#endif
                ui::EndMenu();
            }
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
    auto tab = DynamicCast<Tab>(context_->CreateObject(type));
    tabs_.Push(tab);
    return tab.Get();
}

Tab* Editor::GetOrCreateTab(StringHash type, const String& resourceName)
{
    Tab* tab = GetTab(type, resourceName);
    if (tab == nullptr)
    {
        tab = CreateTab(type);
        tab->LoadResource(resourceName);
    }
    return tab;
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

String Editor::GetResourceAbsolutePath(const String& resourceName, const String& defaultResult, const char* patterns,
    const String& dialogTitle)
{
    String resourcePath = resourceName.Empty() ? defaultResult : resourceName;
    String fullPath;
    if (!resourcePath.Empty())
        fullPath = GetCache()->GetResourceFileName(resourcePath);

    if (fullPath.Empty())
    {
        nfdchar_t* savePath = nullptr;
        if (NFD_SaveDialog(patterns, "", &savePath) == NFD_OKAY)
        {
            fullPath = savePath;
            NFD_FreePath(savePath);
        }
    }

    return fullPath;
}

void Editor::OnConsoleCommand(VariantMap& args)
{
    using namespace ConsoleCommand;
    if (args[P_COMMAND].GetString() == "revision")
        URHO3D_LOGINFOF("Engine revision: %s", GetRevision());
}

void Editor::LoadDefaultLayout()
{
    tabs_.Clear();

    auto* inspector = new InspectorTab(context_);
    auto* hierarchy = new HierarchyTab(context_);
    auto* resources = new ResourceTab(context_);
    auto* console = new ConsoleTab(context_);
    auto* preview = new PreviewTab(context_);

    tabs_.EmplaceBack(inspector);
    tabs_.EmplaceBack(hierarchy);
    tabs_.EmplaceBack(resources);
    tabs_.EmplaceBack(console);
    tabs_.EmplaceBack(preview);

    ImGui::DockBuilderRemoveNode(dockspaceId_);
    ImGui::DockBuilderAddNode(dockspaceId_, ui::GetMainViewport()->Size);

    ImGuiID dock_main_id = dockspaceId_;
    ImGuiID dockHierarchy = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
    ImGuiID dockResources = ImGui::DockBuilderSplitNode(dockHierarchy, ImGuiDir_Down, 0.40f, NULL, &dockHierarchy);
    ImGuiID dockInspector = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, NULL, &dock_main_id);
    ImGuiID dockLog = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, NULL, &dock_main_id);

    ImGui::DockBuilderDockWindow(hierarchy->GetUniqueTitle().CString(), dockHierarchy);
    ImGui::DockBuilderDockWindow(resources->GetUniqueTitle().CString(), dockResources);
    ImGui::DockBuilderDockWindow(console->GetUniqueTitle().CString(), dockLog);
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
    project_.Reset();
    tabs_.Clear();
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

void Editor::RenderProjectPluginsMenu()
{
    unsigned possiblePluginCount = 0;
    StringVector files;
    GetFileSystem()->ScanDir(files, GetFileSystem()->GetProgramDir(), "*.*", SCAN_FILES, false);
    for (auto it = files.Begin(); it != files.End(); ++it)
    {
        String baseName = PluginManager::PathToName(*it);
        if (baseName.Empty())
            // Definitely not plugin file.
            continue;

        if (IsDigit(static_cast<unsigned int>(baseName.Back())))
            // Native plugins will rename main file and append version after base name.
            continue;

        if (baseName.EndsWith("CSharp"))
            // Libraries for C# interop
            continue;

        if (baseName == "Urho3D" || baseName == "Toolbox")
            // Internal engine libraries
            continue;

        ++possiblePluginCount;

        PluginManager* plugins = project_->GetPlugins();
        Plugin* plugin = plugins->GetPlugin(baseName);
        bool loaded = plugin != nullptr;
        if (ui::Checkbox(baseName.CString(), &loaded))
        {
            if (loaded)
                plugins->Load(baseName);
            else
                plugins->Unload(plugin);
        }
    }

    if (possiblePluginCount == 0)
    {
        ui::TextUnformatted("No available files.");
        ui::SetHelpTooltip("Plugins are shared libraries that have a class inheriting from PluginApplication and "
                           "define a plugin entry point. Look at Samples/103_GamePlugin for more information.");
    }
}

Tab* Editor::GetTab(StringHash type, const String& resourceName)
{
    for (auto& tab : tabs_)
    {
        if (tab->GetType() == type)
        {
            if (resourceName.Empty())
                return tab.Get();
            else
            {
                auto resourceTab = DynamicCast<BaseResourceTab>(tab);
                if (resourceTab.NotNull())
                {
                    if (resourceTab->GetResourceName() == resourceName)
                        return tab.Get();
                }
            }
        }
    }
    return nullptr;
}

void Editor::SetupSystemUI()
{
    GetSystemUI()->ApplyStyleDefault(true, 1.0f);
    GetSystemUI()->AddFont("Fonts/NotoSans-Regular.ttf", {}, 16.f);
    GetSystemUI()->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, {ICON_MIN_FA, ICON_MAX_FA, 0}, 0, true);
    ui::GetStyle().WindowRounding = 3;
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
    ui::GetIO().IniFilename = nullptr;
    ui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    ui::GetIO().BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    ui::GetIO().ConfigResizeWindowsFromEdges = true;
}

}
