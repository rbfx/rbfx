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

#include "Foundation/ConsoleTab.h"
#include "Foundation/GameViewTab.h"
#include "Foundation/Glue/ProjectEditorGlue.h"
#include "Foundation/Glue/ResourceBrowserGlue.h"
#include "Foundation/Glue/SceneViewGlue.h"
#include "Foundation/HierarchyBrowserTab.h"
#include "Foundation/InspectorTab.h"
#include "Foundation/InspectorTab/EmptyInspector.h"
#include "Foundation/InspectorTab/MaterialInspector.h"
#include "Foundation/InspectorTab/NodeComponentInspector.h"
#include "Foundation/InspectorTab/PlaceholderResourceInspector.h"
#include "Foundation/InspectorTab/SoundInspector.h"
#include "Foundation/ModelImporter.h"
#include "Foundation/ProfilerTab.h"
#include "Foundation/ResourceBrowserTab.h"
#include "Foundation/ResourceBrowserTab/MaterialFactory.h"
#include "Foundation/ResourceBrowserTab/SceneFactory.h"
#include "Foundation/SceneViewTab.h"
#include "Foundation/SceneViewTab/EditorCamera.h"
#include "Foundation/SceneViewTab/SceneHierarchy.h"
#include "Foundation/SceneViewTab/SceneSelectionRenderer.h"
#include "Foundation/SceneViewTab/SceneSelector.h"
#include "Foundation/SceneViewTab/TransformManipulator.h"
#include "Foundation/SettingsTab.h"
#include "Foundation/SettingsTab/KeyBindingsPage.h"
#include "Foundation/SettingsTab/LaunchPage.h"
#include "Foundation/SettingsTab/PluginsPage.h"
#include "Foundation/StandardFileTypes.h"

#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/LibraryInfo.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/SystemUI/DebugHud.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <Toolbox/ToolboxAPI.h>
#include <Toolbox/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>
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
    editorPluginManager_->AddPlugin("Foundation.StandardFileTypes", &Foundation_StandardFileTypes);

    editorPluginManager_->AddPlugin("Foundation.GameView", &Foundation_GameViewTab);
    editorPluginManager_->AddPlugin("Foundation.SceneView", &Foundation_SceneViewTab);
    editorPluginManager_->AddPlugin("Foundation.Console", &Foundation_ConsoleTab);
    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser", &Foundation_ResourceBrowserTab);
    editorPluginManager_->AddPlugin("Foundation.HierarchyBrowser", &Foundation_HierarchyBrowserTab);
    editorPluginManager_->AddPlugin("Foundation.Settings", &Foundation_SettingsTab);
    editorPluginManager_->AddPlugin("Foundation.Inspector", &Foundation_InspectorTab);
    editorPluginManager_->AddPlugin("Foundation.Profiler", &Foundation_ProfilerTab);

    editorPluginManager_->AddPlugin("Foundation.Settings.KeyBindings", &Foundation_KeyBindingsPage);
    editorPluginManager_->AddPlugin("Foundation.Settings.Launch", &Foundation_LaunchPage);
    editorPluginManager_->AddPlugin("Foundation.Settings.Plugins", &Foundation_PluginsPage);

    editorPluginManager_->AddPlugin("Foundation.Asset.ModelImporter", &Foundation_ModelImporter);

    editorPluginManager_->AddPlugin("Foundation.SceneView.EditorCamera", &Foundation_EditorCamera);
    editorPluginManager_->AddPlugin("Foundation.SceneView.Selector", &Foundation_SceneSelector);
    editorPluginManager_->AddPlugin("Foundation.SceneView.Hierarchy", &Foundation_SceneHierarchy);
    editorPluginManager_->AddPlugin("Foundation.SceneView.SelectionRenderer", &Foundation_SceneSelectionRenderer);
    editorPluginManager_->AddPlugin("Foundation.SceneView.TransformGizmo", &Foundation_TransformManipulator);

    editorPluginManager_->AddPlugin("Foundation.Inspector.Empty", &Foundation_EmptyInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Material", &Foundation_MaterialInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.NodeComponent", &Foundation_NodeComponentInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.PlaceholderResource", &Foundation_PlaceholderResourceInspector);
    editorPluginManager_->AddPlugin("Foundation.Inspector.Sound", &Foundation_SoundInspector);

    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser.MaterialFactory", &Foundation_MaterialFactory);
    editorPluginManager_->AddPlugin("Foundation.ResourceBrowser.SceneFactory", &Foundation_SceneFactory);

    editorPluginManager_->AddPlugin("Foundation.Glue.ProjectEditor", &Foundation_ProjectEditorGlue);
    editorPluginManager_->AddPlugin("Foundation.Glue.ResourceBrowser", &Foundation_ResourceBrowserGlue);
    editorPluginManager_->AddPlugin("Foundation.Glue.SceneView", &Foundation_SceneViewGlue);
}

void Editor::Setup()
{
    context_->RegisterSubsystem(this, Editor::GetTypeStatic());
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

    // Discover resource prefix path by looking for CoreData and going up.
    for (coreResourcePrefixPath_ = context_->GetSubsystem<FileSystem>()->GetProgramDir();;
        coreResourcePrefixPath_ = GetParentPath(coreResourcePrefixPath_))
    {
        if (context_->GetSubsystem<FileSystem>()->DirExists(coreResourcePrefixPath_ + "CoreData"))
            break;
        else
        {
#if WIN32
            if (coreResourcePrefixPath_.length() <= 3)   // Root path of any drive
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
    engineParameters_[EP_LOG_LEVEL] = LOG_DEBUG;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
    engineParameters_[EP_AUTOLOAD_PATHS] = "";
    engineParameters_[EP_RESOURCE_PATHS] = "CoreData;EditorData";
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = coreResourcePrefixPath_;
    engineParameters_[EP_WINDOW_MAXIMIZE] = true;
    engineParameters_[EP_ENGINE_AUTO_LOAD_SCRIPTS] = false;
    engineParameters_[EP_SYSTEMUI_FLAGS] = ImGuiConfigFlags_DpiEnableScaleFonts;
#if URHO3D_SYSTEMUI_VIEWPORTS
    engineParameters_[EP_HIGH_DPI] = true;
    engineParameters_[EP_SYSTEMUI_FLAGS] = engineParameters_[EP_SYSTEMUI_FLAGS].GetUInt() | ImGuiConfigFlags_ViewportsEnable /*| ImGuiConfigFlags_DpiEnableScaleViewports*/;
#else
    engineParameters_[EP_HIGH_DPI] = true;
#endif

    // TODO(editor): Move Editor settings to global place
#if 0
    // Load editor settings
    {
        auto* fs = context_->GetSubsystem<FileSystem>();
        ea::string editorSettingsDir = fs->GetAppPreferencesDir("rbfx", "Editor");
        if (!fs->DirExists(editorSettingsDir))
            fs->CreateDir(editorSettingsDir);

        ea::string editorSettingsFile = editorSettingsDir + "Editor.json";
        if (fs->FileExists(editorSettingsFile))
        {
            JSONFile file(context_);
            if (file.LoadFile(editorSettingsFile))
            {
                JSONInputArchive archive(&file);
                ConsumeArchiveException([&]{ SerializeValue(archive, "editor", *this); });

                engineParameters_[EP_WINDOW_WIDTH] = windowSize_.x_;
                engineParameters_[EP_WINDOW_HEIGHT] = windowSize_.y_;
                engineParameters_[EP_WINDOW_POSITION_X] = windowPos_.x_;
                engineParameters_[EP_WINDOW_POSITION_Y] = windowPos_.y_;
            }
        }
    }
#endif

    context_->GetSubsystem<Log>()->SetLogFormat("[%H:%M:%S] [%l] [%n] : %v");

    SetRandomSeed(Time::GetTimeSinceEpoch());

    // Define custom command line parameters here
    auto& cmd = GetCommandLineParser();
    cmd.add_option("project", defaultProjectPath_, "Project to open or create on startup.")->set_custom_option("dir");

    PluginApplication::RegisterStaticPlugins();
}

void Editor::Start()
{
    Input* input = context_->GetSubsystem<Input>();
    input->SetMouseMode(MM_ABSOLUTE);
    input->SetMouseVisible(true);
    input->SetEnabled(false);

    context_->GetSubsystem<ResourceCache>()->SetAutoReloadResources(true);
    engine_->SetAutoExit(false);

    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap& args) { OnUpdate(args); });

    // Creates console but makes sure it's UI is not rendered. Console rendering is done manually in editor.
    auto* console = engine_->CreateConsole();
    console->SetAutoVisibleOnError(false);
    context_->GetSubsystem<FileSystem>()->SetExecuteConsoleCommands(false);
    SubscribeToEvent(E_CONSOLECOMMAND, [this](StringHash, VariantMap& args) { OnConsoleCommand(args); });
    console->RefreshInterpreters();

    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { OnEndFrame(); });
    SubscribeToEvent(E_EXITREQUESTED, [this](StringHash, VariantMap&) { OnExitRequested(); });
    SubscribeToEvent(E_CONSOLEURICLICK, [this](StringHash, VariantMap& args) { OnConsoleUriClick(args); });
    SetupSystemUI();
    if (!defaultProjectPath_.empty())
    {
        ui::GetIO().IniFilename = nullptr;  // Avoid creating imgui.ini in some cases
        OpenProject(defaultProjectPath_);
    }

    // Hud will be rendered manually.
    context_->GetSubsystem<Engine>()->CreateDebugHud()->SetMode(DEBUGHUD_SHOW_NONE);
}

void Editor::Stop()
{
#if 0
    // Save editor settings
    if (!engine_->IsHeadless())
    {
        // Save window geometry
        auto* graphics = GetSubsystem<Graphics>();
        windowPos_ = graphics->GetWindowPosition();
        windowSize_ = graphics->GetSize();

        auto* fs = context_->GetSubsystem<FileSystem>();
        ea::string editorSettingsDir = fs->GetAppPreferencesDir("rbfx", "Editor");
        if (!fs->DirExists(editorSettingsDir))
            fs->CreateDir(editorSettingsDir);

        JSONFile json(context_);
        JSONOutputArchive archive(&json);
        SerializeValue(archive, "editor", *this);
        if (!json.SaveFile(editorSettingsDir + "Editor.json"))
            URHO3D_LOGERROR("Saving of editor settings failed.");
    }
#endif

    context_->GetSubsystem<WorkQueue>()->Complete(0);
    CloseProject();
    context_->RemoveSubsystem<WorkQueue>(); // Prevents deadlock when unloading plugin AppDomain in managed host.
    context_->RemoveSubsystem<Editor>();
    context_->RemoveSubsystem<EditorPluginManager>();
}

void Editor::OnUpdate(VariantMap& args)
{
    ImGuiContext& g = *GImGui;

    const bool hasToolbar = projectEditor_ != nullptr;
    const float toolbarButtonHeight = Widgets::GetSmallButtonSize();
    const float toolbarWindowPadding = ea::max(3.0f, (g.Style.WindowMinSize.y - toolbarButtonHeight) / 2);
    const float toolbarHeight = hasToolbar
        ? Widgets::GetSmallButtonSize() + 2 * (toolbarWindowPadding + 0)//g.Style.FramePadding.y)
        : 0.0f;
    const float toolbarEffectiveHeight = toolbarHeight + 1;

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos + ImVec2(0, toolbarEffectiveHeight));
    ImGui::SetNextWindowSize(viewport->Size - ImVec2(0, toolbarEffectiveHeight));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, flags);
    ImGui::PopStyleVar();

    RenderMenuBar();

    if (projectEditor_)
    {
        projectEditor_->Render();
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

        struct State
        {
            explicit State(Editor* editor)
            {
                FileSystem *fs = editor->GetContext()->GetSubsystem<FileSystem>();
                StringVector& recents = editor->recentProjects_;
                snapshots_.resize(recents.size());
                for (int i = 0; i < recents.size();)
                {
                    const ea::string& projectPath = recents[i];
                    ea::string snapshotFile = AddTrailingSlash(projectPath) + "Preview.png";
                    if (fs->FileExists(snapshotFile))
                    {
                        Image img(editor->context_);
                        if (img.LoadFile(snapshotFile))
                        {
                            SharedPtr<Texture2D> texture(editor->context_->CreateObject<Texture2D>());
                            texture->SetData(&img);
                            snapshots_[i] = texture;
                        }
                    }
                    ++i;
                }
            }

            ea::vector<SharedPtr<Texture2D>> snapshots_;
        };

        auto* state = ui::GetUIState<State>(this);
        const StringVector& recents = recentProjects_;

        int index = 0;
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++, index++)
            {
                SharedPtr<Texture2D> snapshot;
                if (state->snapshots_.size() > index)
                    snapshot = state->snapshots_[index];

                if (recents.size() <= index || (row == 2 && col == 2))  // Last tile never shows a project.
                {
                    if (ui::Button("Open/Create Project", tileSize))
                        OpenOrCreateProject();
                }
                else
                {
                    const ea::string& projectPath = recents[index];
                    if (snapshot.NotNull())
                    {
                        if (ui::ImageButton(snapshot.Get(), tileSize - style.ItemInnerSpacing * 2))
                            OpenProject(projectPath);
                    }
                    else
                    {
                        if (ui::Button(recents[index].c_str(), tileSize))
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
    ImGui::PopStyleVar();

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

        if (projectEditor_)
            projectEditor_->RenderToolbar();

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
        else if (projectEditor_)
        {
            const auto result = projectEditor_->CloseGracefully();
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
}

void Editor::OnConsoleCommand(VariantMap& args)
{
    using namespace ConsoleCommand;
    if (args[P_COMMAND].GetString() == "revision")
        URHO3D_LOGINFOF("Engine revision: %s", GetRevision());
}

void Editor::OnEndFrame()
{
    if (pendingCloseProject_)
    {
        if (projectEditor_)
        {
            const auto result = projectEditor_->CloseGracefully();
            if (result == CloseProjectResult::Canceled)
            {
                pendingCloseProject_ = false;
                pendingOpenProject_.clear();
            }

            if (result != CloseProjectResult::Closed)
                return;

            projectEditor_ = nullptr;
        }
        pendingCloseProject_ = false;
    }

    // Opening a new project must be done at the point when SystemUI is not in use. End of the frame is a good
    // candidate. This subsystem will be recreated.
    if (!pendingOpenProject_.empty())
    {
        if (projectEditor_)
        {
            pendingCloseProject_ = true;
            return;
        }

        CloseProject();
        // Reset SystemUI so that imgui loads it's config proper.
        context_->RemoveSubsystem<SystemUI>();
        unsigned flags = engineParameters_[EP_SYSTEMUI_FLAGS].GetUInt();
        context_->RegisterSubsystem(new SystemUI(context_, flags));
        SetupSystemUI();

        projectEditor_ = MakeShared<ProjectEditor>(context_, pendingOpenProject_);
        pendingOpenProject_.clear();
    }
}

void Editor::OnExitRequested()
{
    exiting_ = true;
}

void Editor::OnExitHotkeyPressed()
{
    if (!exiting_)
        OnExitRequested();
}

void Editor::OpenProject(const ea::string& projectPath)
{
    pendingOpenProject_ = AddTrailingSlash(projectPath);
}

void Editor::CloseProject()
{
    projectEditor_ = nullptr;
    context_->RemoveSubsystem<ProjectEditor>();
}

void Editor::SetupSystemUI()
{
    auto& io = ui::GetIO();
    auto& style = ImGui::GetStyleTemplate();
    static ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    static ImWchar notoSansRanges[] = {0x20, 0x52f, 0x1ab0, 0x2189, 0x2c60, 0x2e44, 0xa640, 0xab65, 0};
    static ImWchar notoMonoRanges[] = {0x20, 0x513, 0x1e00, 0x1f4d, 0};
    SystemUI* systemUI = GetSubsystem<SystemUI>();

    systemUI->ApplyStyleDefault(true, 1.0f);
    systemUI->AddFont("Fonts/NotoSans-Regular.ttf", notoSansRanges, 16.f);
    systemUI->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 14.f, true);
    monoFont_ = systemUI->AddFont("Fonts/NotoMono-Regular.ttf", notoMonoRanges, 14.f);
    systemUI->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, fontAwesomeIconRanges, 12.f, true);
    style.WindowRounding = 3;
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
#if URHO3D_SYSTEMUI_VIEWPORTS
    io.ConfigViewportsNoAutoMerge = true;
#endif
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.ConfigWindowsResizeFromEdges = true;

    // TODO: Make configurable.
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

    ImGuiSettingsHandler handler;
    handler.TypeName = "Project";
    handler.TypeHash = ImHashStr(handler.TypeName, 0, 0);
    handler.ReadOpenFn = [](ImGuiContext* context, ImGuiSettingsHandler* handler, const char* name) -> void*
    {
        return (void*) name;
    };
    handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
    {
        const char* name = static_cast<const char*>(entry);

        auto context = Context::GetInstance();
        if (auto projectEditor = context->GetSubsystem<ProjectEditor>())
            projectEditor->ReadIniSettings(name, line);
    };
    handler.WriteAllFn = [](ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
    {
        buf->appendf("[Project][Window]\n");

        auto context = Context::GetInstance();
        if (auto projectEditor = context->GetSubsystem<ProjectEditor>())
            projectEditor->WriteIniSettings(*buf);
    };
    ui::GetCurrentContext()->SettingsHandlers.push_back(handler);
}

void Editor::UpdateWindowTitle(const ea::string& resourcePath)
{
    if (context_->GetSubsystem<Engine>()->IsHeadless())
        return;

    // TODO(editor): Implement me
#if 0
    auto* project = GetSubsystem<Project>();
    ea::string title;
    if (project == nullptr)
        title = "Editor";
    else
    {
        ea::string projectName = GetFileName(RemoveTrailingSlash(project->GetProjectPath()));
        title = ToString("Editor | %s", projectName.c_str());
        if (!resourcePath.empty())
            title += ToString(" | %s", GetFileName(resourcePath).c_str());
    }

    context_->GetSubsystem<Graphics>()->SetWindowTitle(title);
#endif
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
