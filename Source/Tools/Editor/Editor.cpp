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
#include "Assets/AssetConverter.h"
#include "Assets/Inspector/MaterialInspector.h"



URHO3D_DEFINE_APPLICATION_MAIN(Editor);


namespace Urho3D
{

static std::string defaultProjectPath;

Editor::Editor(Context* context)
    : Application(context)
#if URHO3D_PLUGINS_NATIVE
    , pluginsNative_(context)
#endif
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

    Inspectable::Material::RegisterObject(context_);

    context_->RegisterSubsystem(new EditorIconCache(context_));
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetMouseVisible(true);
    RegisterToolboxTypes(context_);
    context_->RegisterFactory<Editor>();
    context_->RegisterSubsystem(this);
    SceneSettings::RegisterObject(context_);

    GetSystemUI()->ApplyStyleDefault(true, 1.0f);
    GetSystemUI()->AddFont("Fonts/DejaVuSansMono.ttf");
    GetSystemUI()->AddFont("Fonts/" FONT_ICON_FILE_NAME_FAS, {ICON_MIN_FA, ICON_MAX_FA, 0}, 0, true);
    ui::GetStyle().WindowRounding = 3;
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
    ui::GetIO().IniFilename = nullptr;

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

    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&){
        if (!defaultProjectPath.empty())
        {
            OpenProject(defaultProjectPath.c_str());
            defaultProjectPath.clear();
        }
        UnsubscribeFromEvent(E_ENDFRAME);
    });

    // Plugin loading
#if URHO3D_PLUGINS_NATIVE
    pluginsNative_.AutoLoadFrom(GetFileSystem()->GetProgramDir());
#endif
}

void Editor::Stop()
{
    CloseProject();
    ui::ShutdownDock();
}

void Editor::OnUpdate(VariantMap& args)
{
    RenderMenuBar();

    ui::RootDock({0, 20}, ui::GetIO().DisplaySize - ImVec2(0, 20));

    bool renderedWasActive = false;
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
                    if (renderedWasActive && tab->IsActive())
                    {
                        activeTab_ = tab;
                        tab->OnFocused();
                    }
                    else if (!renderedWasActive)
                    {
                        renderedWasActive = tab->IsActive();
                        activeTab_ = tab;
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

    HandleHotkeys();
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
                    if (OpenProject(projectDir) == nullptr)
                        URHO3D_LOGERROR("Loading project failed.");
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
        }

        SendEvent(E_EDITORAPPLICATIONMENU);

        ui::EndMainMenuBar();
    }
}

Tab* Editor::CreateTab(StringHash type)
{
    auto tab = DynamicCast<Tab>(context_->CreateObject(type));
    tabs_.Push(tab);
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

    ui::LoadDock(JSONValue::EMPTY);

    // These dock sizes are bullshit. Actually visible sizes do not match these numbers. Instead of spending
    // time implementing this functionality in ImGuiDock proper values were written down and then tweaked until
    // they looked right. Insertion order is also important here when specifying dock placement location.
    auto screenSize = GetGraphics()->GetSize();
    auto* inspector = new InspectorTab(context_);
    inspector->Initialize("Inspector", {screenSize.x_ * 0.6f, (float)screenSize.y_}, ui::Slot_Right);
    auto* hierarchy = new HierarchyTab(context_);
    hierarchy->Initialize("Hierarchy", {screenSize.x_ * 0.05f, screenSize.y_ * 0.5f}, ui::Slot_Left);
    auto* resources = new ResourceTab(context_);
    resources->Initialize("Resources", {screenSize.x_ * 0.05f, screenSize.y_ * 0.15f}, ui::Slot_Bottom, hierarchy->GetUniqueTitle());
    auto* console = new ConsoleTab(context_);
    console->Initialize("Console", {screenSize.x_ * 0.6f, screenSize.y_ * 0.4f}, ui::Slot_Left, inspector->GetUniqueTitle());

    tabs_.EmplaceBack(inspector);
    tabs_.EmplaceBack(hierarchy);
    tabs_.EmplaceBack(resources);
    tabs_.EmplaceBack(console);
}

Project* Editor::OpenProject(const String& projectPath)
{
    CloseProject();
    project_ = new Project(context_);
    context_->RegisterSubsystem(project_);
    if (!project_->LoadProject(projectPath))
        CloseProject();
    return project_.Get();
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

}
