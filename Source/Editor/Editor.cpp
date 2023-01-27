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

#include "Editor.h"

#include "Assets/ModelImporter.h"
#include "Foundation/AnimationViewTab.h"
#include "Foundation/ConsoleTab.h"
#include "Foundation/ConcurrentAssetProcessing.h"
#include "Foundation/GameViewTab.h"
#include "Foundation/Glue/ProjectGlue.h"
#include "Foundation/Glue/ResourceBrowserGlue.h"
#include "Foundation/Glue/SceneViewGlue.h"
#include "Foundation/HierarchyBrowserTab.h"
#include "Foundation/InspectorTab.h"
#include "Foundation/InspectorTab/AnimationInspector.h"
#include "Foundation/InspectorTab/AssetPipelineInspector.h"
#include "Foundation/InspectorTab/EmptyInspector.h"
#include "Foundation/InspectorTab/MaterialInspector.h"
#include "Foundation/InspectorTab/ModelInspector.h"
#include "Foundation/InspectorTab/NodeComponentInspector.h"
#include "Foundation/InspectorTab/PlaceholderResourceInspector.h"
#include "Foundation/InspectorTab/SoundInspector.h"
#include "Foundation/InspectorTab/Texture2DInspector.h"
#include "Foundation/InspectorTab/TextureCubeInspector.h"
#include "Foundation/ModelViewTab.h"
#include "Foundation/ProfilerTab.h"
#include "Foundation/ResourceBrowserTab.h"
#include "Foundation/ResourceBrowserTab/AssetPipelineFactory.h"
#include "Foundation/ResourceBrowserTab/MaterialFactory.h"
#include "Foundation/ResourceBrowserTab/SceneFactory.h"
#include "Foundation/SceneViewTab.h"
#include "Foundation/SceneViewTab/CreatePrefabFromNode.h"
#include "Foundation/SceneViewTab/EditorCamera.h"
#include "Foundation/SceneViewTab/SceneHierarchy.h"
#include "Foundation/SceneViewTab/SceneSelectionRenderer.h"
#include "Foundation/SceneViewTab/SceneSelector.h"
#include "Foundation/SceneViewTab/TransformManipulator.h"
#include "Foundation/SceneViewTab/SceneDragAndDropMaterial.h"
#include "Foundation/SceneViewTab/SceneDragAndDropPrefab.h"
#include "Foundation/SceneViewTab/SceneDebugInfo.h"
#include "Foundation/SettingsTab.h"
#include "Foundation/SettingsTab/KeyBindingsPage.h"
#include "Foundation/SettingsTab/LaunchPage.h"
#include "Foundation/SettingsTab/PluginsPage.h"
#include "Foundation/StandardFileTypes.h"
#include "Foundation/Texture2DViewTab.h"
#include "Foundation/TextureCubeViewTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/SystemUI/DebugHud.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>
#include <nativefiledialog/nfd.h>

#ifdef WIN32
#include <windows.h>
#endif

namespace Urho3D
{

Editor::Editor(Context* context)
    : Application(context)
    , editorPluginManager_(MakeShared<EditorPluginManager>(context_))
{
    editorPluginManager_->AddPlugin("Assets.ModelImporter", &Assets_ModelImporter);

    editorPluginManager_->AddPlugin("Foundation.StandardFileTypes", &Foundation_StandardFileTypes);
    editorPluginManager_->AddPlugin("Foundation.ConcurrentAssetProcessing", &Foundation_ConcurrentAssetProcessing);

    editorPluginManager_->AddPlugin("Foundation.GameView", &Foundation_GameViewTab);
    editorPluginManager_->AddPlugin("Foundation.SceneView", &Foundation_SceneViewTab);
    editorPluginManager_->AddPlugin("Foundation.Texture2DView", &Foundation_Texture2DViewTab);
    editorPluginManager_->AddPlugin("Foundation.TextureCubeView", &Foundation_TextureCubeViewTab);
    editorPluginManager_->AddPlugin("Foundation.ModelView", &Foundation_ModelViewTab);
    editorPluginManager_->AddPlugin("Foundation.AnimationView", &Foundation_AnimationViewTab);
    editorPluginManager_->AddPlugin("Foundation.Console", &Foundation_ConsoleTab);
    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser", &Foundation_ResourceBrowserTab);
    editorPluginManager_->AddPlugin("Foundation.HierarchyBrowser", &Foundation_HierarchyBrowserTab);
    editorPluginManager_->AddPlugin("Foundation.Settings", &Foundation_SettingsTab);
    editorPluginManager_->AddPlugin("Foundation.Inspector", &Foundation_InspectorTab);
    editorPluginManager_->AddPlugin("Foundation.Profiler", &Foundation_ProfilerTab);

    editorPluginManager_->AddPlugin("Foundation.Settings.KeyBindings", &Foundation_KeyBindingsPage);
    editorPluginManager_->AddPlugin("Foundation.Settings.Launch", &Foundation_LaunchPage);
    editorPluginManager_->AddPlugin("Foundation.Settings.Plugins", &Foundation_PluginsPage);

    editorPluginManager_->AddPlugin("Foundation.SceneView.CreatePrefabFromNode", &Foundation_CreatePrefabFromNode);
    editorPluginManager_->AddPlugin("Foundation.SceneView.EditorCamera", &Foundation_EditorCamera);
    editorPluginManager_->AddPlugin("Foundation.SceneView.Selector", &Foundation_SceneSelector);
    editorPluginManager_->AddPlugin("Foundation.SceneView.Hierarchy", &Foundation_SceneHierarchy);
    editorPluginManager_->AddPlugin("Foundation.SceneView.SelectionRenderer", &Foundation_SceneSelectionRenderer);
    editorPluginManager_->AddPlugin("Foundation.SceneView.TransformGizmo", &Foundation_TransformManipulator);
    editorPluginManager_->AddPlugin("Foundation.SceneView.DragAndDropPrefab", &Foundation_SceneDragAndDropPrefab);
    editorPluginManager_->AddPlugin("Foundation.SceneView.DragAndDropMaterial", &Foundation_SceneDragAndDropMaterial);
    editorPluginManager_->AddPlugin("Foundation.SceneView.SceneDebugInfo", &Foundation_SceneDebugInfo);

    editorPluginManager_->AddPlugin("Foundation.Inspector.Empty", &Foundation_EmptyInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.AssetPipeline", &Foundation_AssetPipelineInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Animation", &Foundation_AnimationInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Texture2D", &Foundation_Texture2DInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.TextureCube", &Foundation_TextureCubeInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Model", &Foundation_ModelInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Material", &Foundation_MaterialInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.NodeComponent", &Foundation_NodeComponentInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.PlaceholderResource", &Foundation_PlaceholderResourceInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Sound", &Foundation_SoundInspector);

    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser.AssetPipelineFactory", &Foundation_AssetPipelineFactory);
    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser.MaterialFactory", &Foundation_MaterialFactory);
    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser.SceneFactory", &Foundation_SceneFactory);

    editorPluginManager_->AddPlugin("Foundation.Glue.Project", &Foundation_ProjectGlue);
    editorPluginManager_->AddPlugin("Foundation.Glue.ResourceBrowser", &Foundation_ResourceBrowserGlue);
    editorPluginManager_->AddPlugin("Foundation.Glue.SceneView", &Foundation_SceneViewGlue);
}

void Editor::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "RecentProjects", recentProjects_);

    auto fs = GetSubsystem<FileSystem>();
    if (archive.IsInput())
        ea::erase_if(recentProjects_, [&](const ea::string& path) { return path.empty() || !fs->DirExists(path); });
}

void Editor::Setup()
{
    auto fs = GetSubsystem<FileSystem>();
    auto log = GetSubsystem<Log>();

    context_->RegisterSubsystem(editorPluginManager_, EditorPluginManager::GetTypeStatic());

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

    resourcePrefixPath_ = fs->FindResourcePrefixPath();
    if (resourcePrefixPath_.empty())
    {
        ErrorDialog("Cannot launch Editor", "Prefix path is not found, unable to continue. Prefix path must contain CoreData and EditorData.");
        engine_->Exit();
    }

    log->SetLogFormat("[%H:%M:%S] [%l] [%n] : %v");

    SetRandomSeed(Time::GetTimeSinceEpoch());

    // Define custom command line parameters here
    auto& cmd = GetCommandLineParser();
    cmd.add_flag("--read-only", readOnly_, "Prevents Editor from modifying any project files, unless it is explicitly done via executed command.");
    cmd.add_option("--command", command_, "Command to execute on startup.")->type_name("command");
    cmd.add_flag("--exit", exitAfterCommand_, "Forces Editor to exit after command execution.");
    cmd.add_option("project", pendingOpenProject_, "Project to open or create on startup.")->type_name("dir");

    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_APPLICATION_NAME] = GetWindowTitle();
    engineParameters_[EP_HEADLESS] = false;
    engineParameters_[EP_FULL_SCREEN] = false;
    engineParameters_[EP_BORDERLESS] = false;
    engineParameters_[EP_LOG_LEVEL] = LOG_DEBUG;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
    engineParameters_[EP_AUTOLOAD_PATHS] = "";
    engineParameters_[EP_RESOURCE_PATHS] = "CoreData;EditorData";
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = resourcePrefixPath_;
    engineParameters_[EP_WINDOW_MAXIMIZE] = true;
    engineParameters_[EP_ENGINE_AUTO_LOAD_SCRIPTS] = false;
    engineParameters_[EP_SYSTEMUI_FLAGS] = ImGuiConfigFlags_DpiEnableScaleFonts;
#if URHO3D_SYSTEMUI_VIEWPORTS
    engineParameters_[EP_HIGH_DPI] = true;
    engineParameters_[EP_SYSTEMUI_FLAGS] = engineParameters_[EP_SYSTEMUI_FLAGS].GetUInt() | ImGuiConfigFlags_ViewportsEnable /*| ImGuiConfigFlags_DpiEnableScaleViewports*/;
#else
    engineParameters_[EP_HIGH_DPI] = true;
#endif

    PluginApplication::RegisterStaticPlugins();
}

void Editor::Start()
{
    auto cache = GetSubsystem<ResourceCache>();
    auto input = GetSubsystem<Input>();
    auto fs = GetSubsystem<FileSystem>();

    const bool isHeadless = engine_->IsHeadless();

    tempJsonPath_ = engine_->GetAppPreferencesDir() + "Temp.json";
    settingsJsonPath_ = engine_->GetAppPreferencesDir() + "Settings.json";

    JSONFile tempFile(context_);
    if (tempFile.LoadFile(tempJsonPath_))
        tempFile.LoadObject(*this);

    input->SetMouseMode(MM_ABSOLUTE);
    input->SetMouseVisible(true);
    input->SetEnabled(false);

    cache->SetAutoReloadResources(true);

    engine_->SetAutoExit(false);

    // Creates console but makes sure it's UI is not rendered. Console rendering is done manually in editor.
    if (auto console = engine_->CreateConsole())
        console->SetAutoVisibleOnError(false);
    fs->SetExecuteConsoleCommands(false);

    // Hud will be rendered manually.
    if (auto debugHud = engine_->CreateDebugHud())
        debugHud->SetMode(DEBUGHUD_SHOW_NONE);

    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap& args) { Render(); });
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { UpdateProjectStatus(); });
    SubscribeToEvent(E_EXITREQUESTED, [this](StringHash, VariantMap&) { OnExitRequested(); });
    SubscribeToEvent(E_CONSOLEURICLICK, [this](StringHash, VariantMap& args) { OnConsoleUriClick(args); });

    if (!isHeadless)
    {
        InitializeUI();

        // Avoid creating imgui.ini if project with its own imgui.ini is about to be opened.
        if (!pendingOpenProject_.empty())
            ui::GetIO().IniFilename = nullptr;
    }

    if (!pendingOpenProject_.empty())
        OpenProject(pendingOpenProject_);
    else
        command_.clear(); // Execute commands only if the project is opened too.
}

void Editor::Stop()
{
    auto workQueue = GetSubsystem<WorkQueue>();
    workQueue->Complete(0);

    CloseProject();

    context_->RemoveSubsystem<WorkQueue>(); // Prevents deadlock when unloading plugin AppDomain in managed host.
    context_->RemoveSubsystem<EditorPluginManager>();
}

Texture2D* Editor::GetProjectPreview(const ea::string& projectPath)
{
    if (projectPreviews_.contains(projectPath))
        return projectPreviews_[projectPath];

    SharedPtr<Texture2D>& texture = projectPreviews_[projectPath];

    auto fs = GetSubsystem<FileSystem>();
    const ea::string previewFileName = AddTrailingSlash(projectPath) + "Preview.png";
    if (fs->FileExists(previewFileName))
    {
        Image image(context_);
        if (image.LoadFile(previewFileName))
        {
            texture = MakeShared<Texture2D>(context_);
            texture->SetData(&image);
        }
    }

    return texture;
}

ea::string Editor::GetWindowTitle() const
{
    ea::string result = "Editor";

    if (auto graphics = GetSubsystem<Graphics>())
    {
        result += " | ";
        result += graphics->GetApiName();
    }

    if (project_)
    {
        result += " | ";
        result += project_->GetProjectPath();
    }

    return result;
}

void Editor::Render()
{
    auto engine = GetSubsystem<Engine>();
    const bool isHeadless = engine->IsHeadless();
    if (isHeadless)
    {
        // Exit immediately if requested.
        if (exiting_)
        {
            context_->GetSubsystem<WorkQueue>()->Complete(0);
            engine_->Exit();
        }

        // In headless mode only run Project::Render which acts as main loop.
        if (project_)
            project_->Render();

        return;
    }

    ImGuiContext& g = *GImGui;

    const bool hasToolbar = project_ != nullptr;
    const float toolbarButtonHeight = Widgets::GetSmallButtonSize();
    const float toolbarWindowPadding = ea::max(3.0f, (g.Style.WindowMinSize.y - toolbarButtonHeight) / 2);
    const float toolbarHeight = hasToolbar
        ? Widgets::GetSmallButtonSize() + 2 * (toolbarWindowPadding + 0)//g.Style.FramePadding.y)
        : 0.0f;
    const float toolbarEffectiveHeight = toolbarHeight + 1;

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ui::GetMainViewport();
    ui::SetNextWindowPos(viewport->Pos + ImVec2(0, toolbarEffectiveHeight));
    ui::SetNextWindowSize(viewport->Size - ImVec2(0, toolbarEffectiveHeight));
    ui::SetNextWindowViewport(viewport->ID);
    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ui::Begin("DockSpace", nullptr, flags);
    ui::PopStyleVar();

    RenderMenuBar();
    RenderAboutDialog();

    if (project_)
    {
        project_->Render();
    }
    else
    {
        // Render start page
        auto& style = ui::GetStyle();
        auto* lists = ui::GetWindowDrawList();
        ImRect rect{ui::GetWindowContentRegionMin(), ui::GetWindowContentRegionMax()};

        ImVec2 tileSize{200, 200};
        ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10, 10});

        ui::SetCursorPos(rect.GetCenter() - ImVec2{tileSize.x * 1.5f + 10, tileSize.y * 1.5f + 10});

        ui::BeginGroup();

        int index = 0;
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++, index++)
            {
                // Last tile never shows a project.
                if (recentProjects_.size() <= index || (row == 2 && col == 2))
                {
                    if (ui::Button("Open/Create Project", tileSize))
                        OpenOrCreateProject();
                }
                else
                {
                    const ea::string& projectPath = recentProjects_[index];
                    if (Texture2D* previewTexture = GetProjectPreview(projectPath))
                    {
                        if (Widgets::ImageButton(previewTexture, tileSize - style.ItemInnerSpacing * 2))
                            OpenProject(projectPath);
                    }
                    else
                    {
                        if (ui::Button(recentProjects_[index].c_str(), tileSize))
                            OpenProject(projectPath);
                    }
                    if (ui::IsItemHovered())
                        ui::SetTooltip("%s", projectPath.c_str());
                }
                ui::SameLine();
            }
            ui::NewLine();
        }

        ui::EndGroup();
        ui::PopStyleVar();
    }

    const float menuBarHeight = ui::GetCurrentWindow()->MenuBarHeight();

    ui::End();
    ui::PopStyleVar();

    // TODO(editor): Refactor this function
    if (hasToolbar)
    {
        ui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
        ui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));
        ui::SetNextWindowViewport(viewport->ID);

        const ImGuiWindowFlags toolbarWindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings;
        ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(toolbarWindowPadding, toolbarWindowPadding));
        ui::Begin("Toolbar", nullptr, toolbarWindowFlags);

        if (project_)
            project_->RenderToolbar();

        ui::End();
        ui::PopStyleVar(2);
    }

    // Dialog for a warning when application is being closed with unsaved resources.
    if (exiting_)
    {
        if (!context_->GetSubsystem<WorkQueue>()->IsCompleted(0))
        {
            ui::OpenPopup("Completing Tasks");

            if (ui::BeginPopupModal("Completing Tasks", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize |
                                                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_Popup))
            {
                ui::TextUnformatted("Some tasks are in progress and are being completed. Please wait.");
                static float totalIncomplete = context_->GetSubsystem<WorkQueue>()->GetNumIncomplete(0);
                ui::ProgressBar(100.f / totalIncomplete * Min(totalIncomplete - (float)context_->GetSubsystem<WorkQueue>()->GetNumIncomplete(0), totalIncomplete));
                ui::EndPopup();
            }
        }
        else if (project_)
        {
            const auto result = project_->CloseGracefully();
            if (result == CloseProjectResult::Closed)
                engine_->Exit();
            else if (result == CloseProjectResult::Canceled)
                exiting_ = false;
        }
        else
        {
            context_->GetSubsystem<WorkQueue>()->Complete(0);
            engine_->Exit();
        }
    }

    const ea::string title = GetWindowTitle();
    if (windowTitle_ != title)
    {
        auto graphics = context_->GetSubsystem<Graphics>();
        graphics->SetWindowTitle(title.c_str());
        windowTitle_ = title;
    }
}

void Editor::RenderMenuBar()
{
    auto fs = GetSubsystem<FileSystem>();

    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("Project"))
        {
            if (project_)
            {
                project_->RenderProjectMenu();
                ui::Separator();
            }

            if (ui::MenuItem("Open or Create Project"))
                OpenOrCreateProject();

            StringVector & recents = recentProjects_;
            // Does not show very first item, which is current project
            if (recents.size() == (project_ != nullptr ? 1 : 0))
            {
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                ui::MenuItem("Recent Projects");
                ui::PopStyleColor();
            }
            else if (ui::BeginMenu("Recent Projects"))
            {
                for (int i = project_ != nullptr ? 1 : 0; i < recents.size(); i++)
                {
                    const ea::string& projectPath = recents[i];

                    if (ui::MenuItem(GetFileNameAndExtension(RemoveTrailingSlash(projectPath)).c_str()))
                        OpenProject(projectPath);

                    if (ui::IsItemHovered())
                        ui::SetTooltip("%s", projectPath.c_str());
                }
                ui::Separator();
                if (ui::MenuItem("Clear All"))
                    recents.clear();
                ui::EndMenu();
            }

            if (project_)
            {
                if (ui::MenuItem("Close Project"))
                    pendingCloseProject_ = true;
            }

            ui::Separator();

            if (ui::MenuItem("Exit"))
                SendEvent(E_EXITREQUESTED);

            ui::EndMenu();
        }

        if (project_)
            project_->RenderMainMenu();

        if (ui::BeginMenu("Help"))
        {
            if (ui::MenuItem("Open Application Preferences Folder"))
                fs->Reveal(engine_->GetAppPreferencesDir());
            ui::Separator();
            if (ui::MenuItem("About"))
                showAbout_ = true;
            ui::EndMenu();
        }

        ui::EndMainMenuBar();
    }
}

void Editor::RenderAboutDialog()
{
    if (!showAbout_)
        return;

    ui::Begin("Urho3D Rebel Fork aka rbfx", &showAbout_, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ui::Indent();
    Widgets::TextURL("GitHub page", "https://github.com/rbfx/rbfx");
    Widgets::TextURL("Discord server", "https://discord.gg/XKs73yf");
    ui::Unindent();

    ui::Separator();

    ui::BeginDisabled();
    ui::Text("Copyright © 2008-2022 the Urho3D project.");
    ui::Text("Copyright © 2017-2022 the rbfx project.");
    ui::EndDisabled();

    ui::End();
}

void Editor::UpdateProjectStatus()
{
    auto engine = GetSubsystem<Engine>();
    const bool isHeadless = engine->IsHeadless();

    if (pendingCloseProject_)
    {
        if (project_)
        {
            const auto result = project_->CloseGracefully();
            if (result == CloseProjectResult::Canceled)
            {
                pendingCloseProject_ = false;
                pendingOpenProject_.clear();
            }

            if (result != CloseProjectResult::Closed)
                return;

            CloseProject();
        }
        pendingCloseProject_ = false;
    }

    // Opening a new project must be done at the point when SystemUI is not in use. End of the frame is a good
    // candidate. This subsystem will be recreated.
    if (!pendingOpenProject_.empty())
    {
        if (project_)
        {
            pendingCloseProject_ = true;
            return;
        }

        CloseProject();

        // Reset SystemUI so that imgui loads it's config proper.
        if (!isHeadless)
            InitializeUI();

        project_ = MakeShared<Project>(context_, pendingOpenProject_, settingsJsonPath_, readOnly_);
        project_->OnShallowSaved.Subscribe(this, &Editor::SaveTempJson);

        recentProjects_.erase_first(pendingOpenProject_);
        recentProjects_.push_front(pendingOpenProject_);

        pendingOpenProject_.clear();

        if (!command_.empty())
        {
            project_->ExecuteCommand(command_, exitAfterCommand_);
            command_.clear();
        }
    }
}

void Editor::SaveTempJson()
{
    JSONFile tempFile(context_);
    tempFile.SaveObject(*this);
    tempFile.SaveFile(tempJsonPath_);
}

void Editor::OnExitRequested()
{
    exiting_ = true;
}

void Editor::OpenProject(const ea::string& projectPath)
{
    pendingOpenProject_ = AddTrailingSlash(projectPath);
}

void Editor::CloseProject()
{
    if (project_)
    {
        project_->Destroy();
        project_ = nullptr;
        context_->RemoveSubsystem<Project>();
    }
}

void Editor::InitializeUI()
{
    if (uiAlreadyInitialized_)
        RecreateSystemUI();

    InitializeSystemUI();
    InitializeImGuiConfig();
    InitializeImGuiStyle();
    InitializeImGuiHandlers();

    uiAlreadyInitialized_ = true;
}

void Editor::RecreateSystemUI()
{
    Project::SetMonoFont(nullptr);
    context_->RemoveSubsystem<SystemUI>();
    const unsigned flags = engineParameters_[EP_SYSTEMUI_FLAGS].GetUInt();
    context_->RegisterSubsystem(new SystemUI(context_, flags));
}

void Editor::InitializeSystemUI()
{
    static const ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    static const ImWchar notoSansRanges[] = {0x20, 0x52f, 0x1ab0, 0x2189, 0x2c60, 0x2e44, 0xa640, 0xab65, 0};
    static const ImWchar notoMonoRanges[] = {0x20, 0x513, 0x1e00, 0x1f4d, 0};

    auto systemUI = GetSubsystem<SystemUI>();
    systemUI->ApplyStyleDefault(true, 1.0f);
    systemUI->AddFont("Fonts/NotoSans-Regular.ttf", notoSansRanges, 16.f);
    systemUI->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 14.f, true);
    systemUI->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 12.f, true);

    ImFont* monoFont = systemUI->AddFont("Fonts/NotoMono-Regular.ttf", notoMonoRanges, 14.f);
    Project::SetMonoFont(monoFont);
}

void Editor::InitializeImGuiConfig()
{
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
    auto& io = ui::GetIO();
#if URHO3D_SYSTEMUI_VIEWPORTS
    io.ConfigViewportsNoAutoMerge = true;
#endif
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.ConfigWindowsResizeFromEdges = true;
}

void Editor::InitializeImGuiStyle()
{
    auto& style = ui::GetStyleTemplate();

    // TODO: Make configurable.
    style.WindowRounding = 3;
    style.FrameBorderSize = 0;
    style.WindowBorderSize = 1;
    style.ItemSpacing = {4, 4};
    ImVec4* colors = style.Colors;
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
    colors[ImGuiCol_Tab]                    = ImVec4(0.26f, 0.26f, 0.26f, 0.40f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
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

void Editor::InitializeImGuiHandlers()
{
    ImGuiSettingsHandler handler;
    handler.TypeName = "Project";
    handler.TypeHash = ImHashStr(handler.TypeName, 0, 0);

    handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name)
    {
        return const_cast<void*>(reinterpret_cast<const void*>(name));
    };

    handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
    {
        const char* name = static_cast<const char*>(entry);

        auto context = Context::GetInstance();
        if (auto project = context->GetSubsystem<Project>())
            project->ReadIniSettings(name, line);
    };

    handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf)
    {
        buf->appendf("[Project][Window]\n");

        auto context = Context::GetInstance();
        if (auto project = context->GetSubsystem<Project>())
            project->WriteIniSettings(*buf);
    };

    ui::GetCurrentContext()->SettingsHandlers.push_back(handler);
}

void Editor::OpenOrCreateProject()
{
    nfdchar_t* projectDir = nullptr;
    if (NFD_PickFolder("", &projectDir) == NFD_OKAY)
    {
        OpenProject(projectDir);
        NFD_FreePath(projectDir);
    }
}

void Editor::OnConsoleUriClick(VariantMap& args)
{
    using namespace ConsoleUriClick;
    if (ui::IsMouseClicked(MOUSEB_LEFT))
    {
        const ea::string& protocol = args[P_PROTOCOL].GetString();
        const ea::string& address = args[P_ADDRESS].GetString();
        if (protocol == "res")
            context_->GetSubsystem<FileSystem>()->SystemOpen(context_->GetSubsystem<ResourceCache>()->GetResourceFileName(address));
    }
}

}
