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
#include <Toolbox/Toolbox.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <nfd.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "EditorIconCache.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/Scene/SceneSettings.h"
#include "Tabs/UI/UITab.h"
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
        idPool_.Clear();
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
        project_ = new Project(context_);
        if (!project_->LoadProject(loadProject))
            project_.Reset();
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

    if (project_.Null())
        return;

    ui::RootDock({0, 20}, ui::GetIO().DisplaySize - ImVec2(0, 20));

    ui::SetNextDockPos(nullptr, ui::Slot_Left, ImGuiCond_FirstUseEver);
    if (ui::BeginDock("Hierarchy"))
    {
        if (!activeTab_.Expired())
            activeTab_->RenderNodeTree();
    }
    ui::EndDock();

    bool renderedWasActive = false;
    for (auto it = tabs_.Begin(); it != tabs_.End();)
    {
        auto& tab = *it;
        if (tab->RenderWindow())
        {
            if (tab->IsRendered())
            {
                // Only active window may override another active window
                if (renderedWasActive && tab->IsActive())
                    activeTab_ = tab;
                else if (!renderedWasActive)
                {
                    renderedWasActive = tab->IsActive();
                    activeTab_ = tab;
                }
            }

            ++it;
        }
        else
            it = tabs_.Erase(it);
    }


    if (!activeTab_.Expired())
    {
        activeTab_->OnActiveUpdate();
        ui::SetNextDockPos(activeTab_->GetUniqueTitle().CString(), ui::Slot_Right, ImGuiCond_FirstUseEver);
    }
    if (ui::BeginDock("Inspector"))
    {
        if (!activeTab_.Expired())
            activeTab_->RenderInspector();
    }
    ui::EndDock();

    if (ui::BeginDock("Console"))
        GetSubsystem<Console>()->RenderContent();
    ui::EndDock();

    String selected;
    if (tabs_.Size())
        ui::SetNextDockPos(tabs_.Back()->GetUniqueTitle().CString(), ui::Slot_Bottom, ImGuiCond_FirstUseEver);
    if (ResourceBrowserWindow(selected))
    {
        auto type = GetContentType(selected);
        if (type == CTYPE_SCENE)
            CreateNewTab<SceneTab>()->LoadResource(selected);
        else if (type == CTYPE_UILAYOUT)
            CreateNewTab<UITab>()->LoadResource(selected);
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
                            project_->SaveProject(savePath);
                            NFD_FreePath(savePath);
                        }
                    }
                    else
                        project_->SaveProject();
                }

                if (ui::MenuItem("Save Project As"))
                {
                    nfdchar_t* savePath = nullptr;
                    if (NFD_SaveDialog("project", "", &savePath) == NFD_OKAY)
                    {
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
                    project_ = new Project(context_);
                    if (!project_->LoadProject(projectFilePath))
                    {
                        URHO3D_LOGERROR("Loading project failed.");
                        project_.Reset();
                    }
                    NFD_FreePath(projectFilePath);
                }
            }

            if (ui::MenuItem("Create Project"))
            {
                project_ = new Project(context_);
            }

            if (project_.NotNull())
            {
                ui::Separator();

                if (ui::MenuItem("New Scene"))
                    CreateNewTab<SceneTab>();

                if (ui::MenuItem("New UI Layout"))
                    CreateNewTab<UITab>();
            }

            ui::Separator();

            if (ui::MenuItem("Close Project"))
            {
                context_->RemoveSubsystem<Project>();
                project_.Reset();
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

template<typename T>
T* Editor::CreateNewTab(const JSONValue& project)
{
    SharedPtr<T> tab;
    StringHash id;

    if (project.IsNull())
        id = idPool_.NewID();           // Make new ID only if scene is not being loaded from a project.

    if (tabs_.Empty())
        tab = new T(context_, id, "Hierarchy", ui::Slot_Right);
    else
        tab = new T(context_, id, tabs_.Back()->GetUniqueTitle(), ui::Slot_Tab);

    if (project.IsObject())
    {
        tab->LoadProject(project);
        if (!idPool_.TakeID(tab->GetID()))
        {
            URHO3D_LOGERRORF("Scene loading failed because unique id %s is already taken",
                tab->GetID().ToString().CString());
            return nullptr;
        }
    }

    // In order to render scene to a texture we must add a dummy node to scene rendered to a screen, which has material
    // pointing to scene texture. This object must also be visible to main camera.
    tabs_.Push(DynamicCast<Tab>(tab));
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

}
