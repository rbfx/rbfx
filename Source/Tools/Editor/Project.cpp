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
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/Archive.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/SystemUI/SystemUI.h>
#if URHO3D_RMLUI
#   include <Urho3D/RmlUI/RmlUI.h>
#endif

#include <regex>
#include <EASTL/sort.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Toolbox/SystemUI/Widgets.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "Pipeline/Pipeline.h"
#include "Project.h"
#if URHO3D_PLUGINS
#   include "Plugins/ModulePlugin.h"
#   include "Plugins/PluginManager.h"
#   include "Plugins/ScriptBundlePlugin.h"
#endif
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/ResourceTab.h"


namespace Urho3D
{

Project::Project(Context* context)
    : Object(context)
    , pipeline_(new Pipeline(context))
#if URHO3D_PLUGINS
    , plugins_(new PluginManager(context))
#endif
    , undo_(new UndoStack(context))
{
    SubscribeToEvent(E_EDITORRESOURCESAVED, &Project::OnEditorResourceSaved);
    SubscribeToEvent(E_RESOURCERENAMED, &Project::OnResourceRenamed);
    SubscribeToEvent(E_RESOURCEBROWSERDELETE, &Project::OnResourceBrowserDelete);
    SubscribeToEvent(E_ENDFRAME, &Project::OnEndFrame);
    context_->RegisterSubsystem(pipeline_);
#if URHO3D_PLUGINS
    context_->RegisterSubsystem(plugins_);
#endif
    context_->RegisterSubsystem(undo_);

    // Key bindings
    auto* editor = GetSubsystem<Editor>();
    editor->keyBindings_.Bind(ActionType::SaveProject, this, &Project::SaveProject);
    editor->keyBindings_.Bind(ActionType::Undo, this, &Project::OnUndo);
    editor->keyBindings_.Bind(ActionType::Redo, this, &Project::OnRedo);
    editor->settingsTabs_.Subscribe(this, &Project::RenderSettingsUI);
}

Project::~Project()
{
    context_->RemoveSubsystem(undo_->GetType());
    context_->RemoveSubsystem(pipeline_->GetType());
#if URHO3D_PLUGINS
    context_->RemoveSubsystem(plugins_->GetType());
#endif
    if (context_->GetSubsystem<SystemUI>())
        ui::GetIO().IniFilename = nullptr;

    if (auto* cache = context_->GetSubsystem<ResourceCache>())
    {
        cache->RemoveResourceDir(GetCachePath());
        for (const ea::string& resourcePath : resourcePaths_)
            cache->RemoveResourceDir(projectFileDir_ + resourcePath);
        cache->AddResourceDir(coreDataPath_);
        cache->SetAutoReloadResources(false);
    }

    if (auto* editor = GetSubsystem<Editor>())
        editor->UpdateWindowTitle();
}

bool Project::LoadProject(const ea::string& projectPath, bool disableAssetImport)
{
    if (!projectFileDir_.empty())
        // Project is already loaded.
        return false;

    if (projectPath.empty())
        return false;

    auto* fs = GetSubsystem<FileSystem>();
    auto* cache = GetSubsystem<ResourceCache>();
    projectFileDir_ = AddTrailingSlash(projectPath);


    // Cache directory setup. Needs to happen before deserialization of Project.json because flavors depend on cache
    // path availability.
    cachePath_ = projectFileDir_ + "Cache/";
    if (!fs->Exists(GetCachePath()))
        fs->CreateDirsRecursive(GetCachePath());

    // Project.json
    ea::string filePath(projectFileDir_ + "Project.json");
    JSONFile file(context_);
    if (fs->Exists(filePath))
    {
        if (!file.LoadFile(filePath))
            return false;
    }
    // Loading is performed even on empty file. Give a chance for serialization function to do default setup in case of missing data.
    JSONInputArchive archive(&file);
    if (!Serialize(archive))
        return false;

    // Default resources directory for new projects.
    if (resourcePaths_.empty())
    {
        resourcePaths_.push_back("Resources/");
        resourcePaths_.push_back("CoreData/");
    }

    // Default resource path is first resource directory in the list.
    defaultResourcePath_ = projectFileDir_ + resourcePaths_.front();

    if (context_->GetSubsystem<SystemUI>())
    {
        uiConfigPath_ = projectFileDir_ + ".ui.ini";
        defaultUiPlacement_ = !fs->FileExists(uiConfigPath_);
        ui::GetIO().IniFilename = uiConfigPath_.c_str();
    }

#if URHO3D_HASH_DEBUG
    // StringHashNames.json
    {
        ea::string filePath(projectFileDir_ + "StringHashNames.json");
        if (fs->Exists(filePath))
        {
            JSONFile file(context_);
            if (!file.LoadFile(filePath))
                return false;

            for (const auto& value : file.GetRoot().GetArray())
            {
                // Seed global string hash to name map.
                StringHash hash(value.GetString());
                (void) (hash);
            }
        }
    }
#endif

    // Find CoreData path, it will be useful for other subsystems later.
    for (const ea::string& resourcePath : cache->GetResourceDirs())
    {
        if (resourcePath.ends_with("/CoreData/"))
        {
            coreDataPath_ = resourcePath;
            break;
        }
    }
    assert(!coreDataPath_.empty());
    cache->RemoveResourceDir(coreDataPath_);
    if (!fs->DirExists(projectFileDir_ + "CoreData/"))
        fs->CopyDir(coreDataPath_, projectFileDir_ + "CoreData/");

    // Register asset dirs
    cache->AddResourceDir(GetCachePath(), 0);
    for (int i = 0; i < resourcePaths_.size(); i++)
    {
        ea::string absolutePath = projectFileDir_ + resourcePaths_[i];
        if (!fs->DirExists(absolutePath))
            fs->CreateDirsRecursive(absolutePath);
        // Directories further down the list have lower priority. (Highest priority is 0 and lowest is 0xFFFFFFFF).
        cache->AddResourceDir(absolutePath, i + 1);
    }
    cache->SetAutoReloadResources(true);

#if URHO3D_RMLUI
    // TODO: Sucks. Newly added fonts wont work.
    auto* ui = GetSubsystem<RmlUI>();
    ea::vector<ea::string> fonts;
    cache->Scan(fonts, "Fonts/", "*.ttf", SCAN_FILES, true);
    cache->Scan(fonts, "Fonts/", "*.otf", SCAN_FILES, true);
    for (const ea::string& font : fonts)
        ui->LoadFont(Format("Fonts/{}", font));
#endif

#if URHO3D_PLUGINS
    // Clean up old copies of reloadable files.
    if (!context_->GetSubsystem<Engine>()->IsHeadless())
    {
        // Normal execution cleans up old versions of plugins.
        StringVector files;
        fs->ScanDir(files, fs->GetProgramDir(), "", SCAN_FILES, false);
        for (const ea::string& fileName : files)
        {
            if (std::regex_match(fileName.c_str(), std::regex("^.*[0-9]+\\.(dll|dylib|so)$")))
                fs->Delete(fs->GetProgramDir() + fileName);
        }
    }
#if URHO3D_CSHARP
    plugins_->Load(ScriptBundlePlugin::GetTypeStatic(), "Scripts");
#endif
#endif
    if (!disableAssetImport)
    {
        pipeline_->EnableWatcher();
        pipeline_->BuildCache(nullptr, PipelineBuildFlag::SKIP_UP_TO_DATE);
    }
    return true;
}

bool Project::SaveProject()
{
    if (context_->GetSubsystem<Engine>()->IsHeadless())
    {
        URHO3D_LOGERROR("Headless instance is supposed to use project as read-only.");
        return false;
    }

    // Saving project data of tabs may trigger saving resources, which in turn triggers saving editor project. Avoid
    // that loop.
    UnsubscribeFromEvent(E_EDITORRESOURCESAVED);

    if (projectFileDir_.empty())
    {
        URHO3D_LOGERROR("Unable to save project. Project path is empty.");
        return false;
    }

    ui::SaveIniSettingsToDisk(uiConfigPath_.c_str());

#if URHO3D_HASH_DEBUG
    // StringHashNames.json
    {
        auto hashNames = StringHash::GetGlobalStringHashRegister()->GetInternalMap().values();
        ea::quick_sort(hashNames.begin(), hashNames.end());
        JSONFile file(context_);
        JSONArray names;
        for (const auto& string : hashNames)
            names.push_back(string);
        file.GetRoot() = names;

        ea::string filePath(projectFileDir_ + "StringHashNames.json");
        if (!file.SaveFile(filePath))
        {
            projectFileDir_.clear();
            URHO3D_LOGERRORF("Saving StringHash names to '%s' failed", filePath.c_str());
            return false;
        }
    }
#endif

    // Project.json
    JSONFile file(context_);
    JSONOutputArchive archive(&file);
    if (!Serialize(archive))
        return false;

    ea::string filePath(projectFileDir_ + "Project.json");
    if (!file.SaveFile(filePath))
    {
        projectFileDir_.clear();
        URHO3D_LOGERROR("Saving project to '{}' failed", filePath);
        return false;
    }

    return true;
}

bool Project::Serialize(Archive& archive)
{
    const int version = 1;
    if (!archive.IsInput() && context_->GetSubsystem<Engine>()->IsHeadless())
    {
        URHO3D_LOGERROR("Headless instance is supposed to use project as read-only.");
        return false;
    }

    // Saving project data of tabs may trigger saving resources, which in turn triggers saving editor project. Avoid that loop.
    UnsubscribeFromEvent(E_EDITORRESOURCESAVED);

    if (auto projectBlock = archive.OpenUnorderedBlock("project"))
    {
        int archiveVersion = version;
        SerializeValue(archive, "version", archiveVersion);
        SerializeValue(archive, "defaultScene", defaultScene_);
        SerializeValue(archive, "resourcePaths", resourcePaths_);

        if (archive.IsInput())
        {
            for (ea::string& path : resourcePaths_)
                path = AddTrailingSlash(path);
        }

        if (!pipeline_->Serialize(archive))
            return false;
#if URHO3D_PLUGINS
        if (!plugins_->Serialize(archive))
            return false;
#endif
        using namespace EditorProjectSerialize;
        SendEvent(E_EDITORPROJECTSERIALIZE, P_ARCHIVE, (void*)&archive);
    }

    SubscribeToEvent(E_EDITORRESOURCESAVED, [this](StringHash, VariantMap&) { SaveProject(); });

    return true;
}

void Project::RenderSettingsUI()
{
    if (ui::BeginTabItem("General"))
    {
        struct ProjectSettingsState
        {
            /// A list of scenes present in resource directories.
            StringVector scenes_;
            ///
            ea::string newResourceDir_;

            explicit ProjectSettingsState(Project* project)
            {
                project->context_->GetSubsystem<FileSystem>()->ScanDir(scenes_, project->GetResourcePath(), "*.xml", SCAN_FILES, true);
                for (auto it = scenes_.begin(); it != scenes_.end();)
                {
                    if (GetContentType(project->context_, *it) == CTYPE_SCENE)
                        ++it;
                    else
                        it = scenes_.erase(it);
                }
            }
        };
        auto* state = ui::GetUIState<ProjectSettingsState>(this);

        // Default scene
        ui::PushID("Default Scene");
        if (ui::BeginCombo("Default Scene", GetDefaultSceneName().c_str()))
        {
            for (const ea::string& resourceName : state->scenes_)
            {
                if (ui::Selectable(resourceName.c_str(), GetDefaultSceneName() == resourceName))
                    SetDefaultSceneName(resourceName);
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
        const StringVector& pluginNames = GetPlugins()->GetPluginNames();
        bool hasPlugins = false;
        PluginManager* plugins = GetPlugins();
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
        ui::PushID("Resource Paths");
        ui::Separator();

        ui::Text("Resource Dirs:");
        auto* cache = GetSubsystem<ResourceCache>();
        auto* fs = GetSubsystem<FileSystem>();
        if (ui::InputText("Add resource directory", &state->newResourceDir_, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            ea::string absolutePath = projectFileDir_ + AddTrailingSlash(state->newResourceDir_);

            if (absolutePath == GetCachePath())
                URHO3D_LOGERROR("Can not add a cache path as resource directory.");
            else
            {
                bool exists = false;
                for (const ea::string& path : cache->GetResourceDirs())
                    exists |= path == absolutePath;

                if (exists)
                    URHO3D_LOGERROR("This resource path is already added.");
                else
                {
                    pipeline_->watcher_.StopWatching();
                    resourcePaths_.push_back(AddTrailingSlash(state->newResourceDir_));
                    fs->CreateDirsRecursive(absolutePath);
                    cache->AddResourceDir(absolutePath, resourcePaths_.size());
                    pipeline_->EnableWatcher();
                    state->newResourceDir_.clear();
                }
            }
        }
        for (int i = 0; i < resourcePaths_.size();)
        {
            ui::PushID(i);
            int swapNext = i;
            if (ui::Button(ICON_FA_ANGLE_UP))
                swapNext = Max(i - 1, 0);
            if (ImGui::IsItemHovered())
                ui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ui::SameLine();
            if (ui::Button(ICON_FA_ANGLE_DOWN))
                swapNext = Min(i + 1, resourcePaths_.size() - 1);
            if (ImGui::IsItemHovered())
                ui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ui::SameLine();

            if (swapNext != i)
            {
                // Remove and re-add same paths with changed priority.
                cache->SetAutoReloadResources(false);
                cache->RemoveResourceDir(projectFileDir_ + resourcePaths_[i]);
                cache->RemoveResourceDir(projectFileDir_ + resourcePaths_[swapNext]);

                ea::swap(resourcePaths_[i], resourcePaths_[swapNext]);

                cache->AddResourceDir(projectFileDir_ + resourcePaths_[i], 1 + i);
                cache->AddResourceDir(projectFileDir_ + resourcePaths_[swapNext], 1 + swapNext);
                cache->SetAutoReloadResources(true);

                if (i == 0 || swapNext == 0)
                    defaultResourcePath_ = projectFileDir_ + resourcePaths_[0];
            }

            if (i == 0)
            {
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                ui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            }
            bool deleted = ui::Button(ICON_FA_TRASH_ALT);
            if (i == 0)
            {
                ui::PopItemFlag();
                ui::PopStyleColor();        // ImGuiCol_TextDisabled
            }
            ui::SameLine();

            ui::TextUnformatted(resourcePaths_[i].c_str());

            if (deleted)
            {
                pipeline_->watcher_.StopWatching();
                cache->SetAutoReloadResources(false);
                cache->RemoveResourceDir(projectFileDir_ + resourcePaths_[i]);
                resourcePaths_.erase_at(i);
                pipeline_->EnableWatcher();
            }
            else
                ++i;

            ui::SetHelpTooltip("Remove resource directory. This does not delete any files.");
            ui::PopID();    // i
        }
        ui::PopID();        // Resource Paths

        ui::EndTabItem();   // General
    }
}

void Project::OnEditorResourceSaved(StringHash, VariantMap&)
{
    SaveProject();
}

void Project::OnResourceRenamed(StringHash, VariantMap& args)
{
    using namespace ResourceRenamed;
    if (args[P_FROM].GetString() == defaultScene_)
        defaultScene_ = args[P_TO].GetString();
}

void Project::OnResourceBrowserDelete(StringHash, VariantMap& args)
{
    using namespace ResourceBrowserDelete;
    if (args[P_NAME].GetString() == defaultScene_)
        defaultScene_ = EMPTY_STRING;
}

void Project::OnEndFrame(StringHash, VariantMap&)
{
    // Save project every minute.
    // TODO: Make save time configurable.
    if (saveProjectTimer_.GetMSec(false) >= 60000)
    {
        SaveProject();
        saveProjectTimer_.Reset();
    }
}

void Project::OnUndo()
{
    if (ui::IsAnyItemActive() || !undo_->IsTrackingEnabled())
        return;

    undo_->Undo();
}

void Project::OnRedo()
{
    if (ui::IsAnyItemActive() || !undo_->IsTrackingEnabled())
        return;

    undo_->Redo();
}

}
