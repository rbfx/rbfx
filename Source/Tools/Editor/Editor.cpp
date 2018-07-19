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

#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>
#include <Toolbox/ToolboxAPI.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>
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

URHO3D_DEFINE_APPLICATION_MAIN(Editor);


namespace Urho3D
{

Editor::Editor(Context* context)
    : Application(context)
#if URHO3D_PLUGINS_NATIVE
    , pluginsNative_(context)
#endif
#if URHO3D_PLUGINS_CSHARP
    , pluginsManaged_(context)
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
    engineParameters_[EP_AUTOLOAD_PATHS] = "Autoload";
    engineParameters_[EP_RESOURCE_PATHS] = "Data;CoreData;EditorData";
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = coreResourcePrefixPath_;

    SetRandomSeed(Time::GetTimeSinceEpoch());
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

    context_->RegisterSubsystem(new EditorIconCache(context_));
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetMouseVisible(true);
    RegisterToolboxTypes(context_);
    context_->RegisterFactory<Editor>();
    context_->RegisterSubsystem(this);
    SceneSettings::RegisterObject(context_);

    GetSystemUI()->ApplyStyleDefault(true, 1.0f);
    GetSystemUI()->AddFont("Fonts/DejaVuSansMono.ttf");
    GetSystemUI()->AddFont("Fonts/fontawesome-webfont.ttf", {ICON_MIN_FA, ICON_MAX_FA, 0}, 0, true);
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

    // Process arguments
    const auto& arguments = GetArguments();
    {
        unsigned i = 0;
        for (; i < arguments.Size(); ++i)
        {
            if (arguments[i].Length() > 1 && arguments[i][0] == '-')
            {
                auto argument = arguments[i].Substring(1).ToLower();
                const auto& value = i + 1 < arguments.Size() ? arguments[i + 1] : String::EMPTY;

                // TODO: Any editor arguments
            }
            else
                break;
        }

        String loadProject = GetCache()->GetResourceFileName("Etc/DefaultEditorProject.project");
        if (i < arguments.Size())
            loadProject = arguments[i];

        // Load default project on start
        OpenProject(loadProject);
    }

    // Plugin loading
#if URHO3D_PLUGINS_CSHARP
    pluginsManaged_.AutoLoadFrom(GetFileSystem()->GetProgramDir());
#endif
#if URHO3D_PLUGINS_NATIVE
    pluginsNative_.AutoLoadFrom(GetFileSystem()->GetProgramDir());
#endif
}

void Editor::Stop()
{
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
        else
            tabs_.Erase(tabs_.Find(tab));
    }

    if (!activeTab_.Expired())
    {
        activeTab_->OnActiveUpdate();
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
                    if (project_->GetProjectFilePath().Empty())
                    {
                        nfdchar_t* savePath = nullptr;
                        if (NFD_SaveDialog("project", "", &savePath) == NFD_OKAY)
                        {
                            for (auto& tab : tabs_)
                                tab->SaveResource();
                            project_->SaveProject(savePath);
                            NFD_FreePath(savePath);
                        }
                    }
                    else
                    {
                        for (auto& tab : tabs_)
                            tab->SaveResource();
                        project_->SaveProject();
                    }
                }

                if (ui::MenuItem("Save Project As"))
                {
                    nfdchar_t* savePath = nullptr;
                    if (NFD_SaveDialog("project", "", &savePath) == NFD_OKAY)
                    {
                        for (auto& tab : tabs_)
                            tab->SaveResource();
                        project_->SaveProject(savePath);
                        NFD_FreePath(savePath);
                    }
                }
            }

            if (ui::MenuItem("Open Project"))
            {
                nfdchar_t* projectFilePath = nullptr;
                if (NFD_OpenDialog("project", "", &projectFilePath) == NFD_OKAY)
                {
                    if (OpenProject(projectFilePath) == nullptr)
                        URHO3D_LOGERROR("Loading project failed.");
                    NFD_FreePath(projectFilePath);
                }
            }

            if (ui::MenuItem("Create Project"))
            {
                OpenProject();
                LoadDefaultLayout();
            }

            if (project_.NotNull())
            {
                ui::Separator();

                if (ui::MenuItem("New Scene"))
                    CreateTab<SceneTab>();

                if (ui::MenuItem("New UI Layout"))
                    CreateTab<UITab>();
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
            if (ui::BeginMenu("Settings"))
            {
                if (ImGui::CollapsingHeader("Data directories"))
                {
                    // TODO: This is very out of place. To be moved somewhere better fitting.
                    const auto& cacheDirectories = GetCache()->GetResourceDirs();
                    auto programDir = GetFileSystem()->GetProgramDir();
                    for (const auto& dir : cacheDirectories)
                    {
                        String relativeDir;
                        GetRelativePath(programDir, dir, relativeDir);
                        ui::TextUnformatted(relativeDir.CString());
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

}
