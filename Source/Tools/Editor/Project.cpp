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

#include <EASTL/sort.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Tabs/ConsoleTab.h>
#include <Tabs/ResourceTab.h>
#include <Tabs/HierarchyTab.h>
#include <Tabs/InspectorTab.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <ThirdParty/tracy/server/IconsFontAwesome5.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "Project.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/UI/UITab.h"


namespace Urho3D
{

Project::Project(Context* context)
    : Object(context)
    , pipeline_(context)
#if URHO3D_PLUGINS
    , plugins_(context)
#endif
{
    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this));
    SubscribeToEvent(E_RESOURCERENAMED, [this](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;
        if (args[P_FROM].GetString() == defaultScene_)
            defaultScene_ = args[P_TO].GetString();
    });
    SubscribeToEvent(E_RESOURCEBROWSERDELETE, [this](StringHash, VariantMap& args) {
        using namespace ResourceBrowserDelete;
        if (args[P_NAME].GetString() == defaultScene_)
            defaultScene_ = EMPTY_STRING;
    });

    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
        // Save project every minute.
        // TODO: Make save time configurable.
        if (saveProjectTimer_.GetMSec(false) >= 60000)
        {
            SaveProject();
            saveProjectTimer_.Reset();
        }
    });
}

Project::~Project()
{
    if (GetSystemUI())
        ui::GetIO().IniFilename = nullptr;

    if (auto* cache = GetCache())
    {
        cache->RemoveResourceDir(GetCachePath());
        cache->RemoveResourceDir(GetResourcePath());

        for (const auto& path : cachedEngineResourcePaths_)
            GetCache()->AddResourceDir(path);
    }

    if (auto* editor = GetSubsystem<Editor>())
        editor->UpdateWindowTitle();
}

bool Project::LoadProject(const ea::string& projectPath)
{
    if (!projectFileDir_.empty())
        // Project is already loaded.
        return false;

    if (projectPath.empty())
        return false;

    projectFileDir_ = AddTrailingSlash(projectPath);

    if (!GetFileSystem()->Exists(GetCachePath()))
        GetFileSystem()->CreateDirsRecursive(GetCachePath());

    if (!GetFileSystem()->Exists(GetResourcePath()))
    {
        // Initialize new project
        GetFileSystem()->CreateDirsRecursive(GetResourcePath());

        for (const auto& path : GetCache()->GetResourceDirs())
        {
            if (path.ends_with("/EditorData/") || path.contains("/Autoload/"))
                continue;

            StringVector names;

            URHO3D_LOGINFOF("Importing resources from '%s'", path.c_str());

            // Copy default resources to the project.
            GetFileSystem()->ScanDir(names, path, "*", SCAN_FILES, false);
            for (const auto& name : names)
                GetFileSystem()->Copy(path + name, GetResourcePath() + name);

            GetFileSystem()->ScanDir(names, path, "*", SCAN_DIRS, false);
            for (const auto& name : names)
            {
                if (name == "." || name == "..")
                    continue;
                GetFileSystem()->CopyDir(path + name, GetResourcePath() + name);
            }
        }
    }

    // Unregister engine dirs
    auto enginePrefixPath = GetSubsystem<Editor>()->GetCoreResourcePrefixPath();
    auto pathsCopy = GetCache()->GetResourceDirs();
    cachedEngineResourcePaths_.clear();
    for (const auto& path : pathsCopy)
    {
        if (path.starts_with(enginePrefixPath) && !path.ends_with("/EditorData/"))
        {
            cachedEngineResourcePaths_.emplace_back(path);
            GetCache()->RemoveResourceDir(path);
        }
    }

    if (GetSystemUI())
    {
        uiConfigPath_ = projectFileDir_ + ".ui.ini";
        ui::GetIO().IniFilename = uiConfigPath_.c_str();

        ImGuiSettingsHandler handler;
        handler.TypeName = "Project";
        handler.TypeHash = ImHashStr(handler.TypeName, 0, 0);
        handler.ReadOpenFn = [](ImGuiContext* context, ImGuiSettingsHandler* handler, const char* name) -> void*
        {
            if (strcmp(name, "Window") == 0)
                return (void*) 1;
            return (void*) name;
        };
        handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
        {
            SystemUI* systemUI = ui::GetSystemUI();
            auto* editor = systemUI->GetSubsystem<Editor>();

            if (entry == (void*) 1)
            {
                auto* project = systemUI->GetSubsystem<Project>();
                project->isNewProject_ = false;
                editor->CreateDefaultTabs();

                int x, y, w, h;
                if (sscanf(line, "Rect=%d,%d,%d,%d", &x, &y, &w, &h) == 4)
                {
                    w = Max(w, 100);            // Foot-shooting prevention
                    h = Max(h, 100);
                    systemUI->GetGraphics()->SetWindowPosition(x, y);
                    systemUI->GetGraphics()->SetMode(w, h);
                }
                else
                    return;
            }
            else
            {
                const char* name = static_cast<const char*>(entry);

                Tab* tab = editor->GetTabByName(name);
                if (tab == nullptr)
                {
                    StringVector parts = ea::string(name).split('#');
                    tab = editor->CreateTab(parts.front());
                }
                tab->OnLoadUISettings(name, line);
            }
        };
        handler.WriteAllFn = [](ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
        {
            buf->appendf("[Project][Window]\n");
            auto* systemUI = ui::GetSystemUI();
            auto* editor = systemUI->GetSubsystem<Editor>();
            IntVector2
            wSize = systemUI->GetGraphics()->GetSize();
            IntVector2
            wPos = systemUI->GetGraphics()->GetWindowPosition();
            buf->appendf("Rect=%d,%d,%d,%d\n", wPos.x_, wPos.y_, wSize.x_, wSize.y_);

            // Save tabs
            for (auto& tab : editor->GetContentTabs())
                tab->OnSaveUISettings(buf);
        };
        ui::GetCurrentContext()->SettingsHandlers.push_back(handler);
    }

#if URHO3D_HASH_DEBUG
    // StringHashNames.json
    {
        ea::string filePath(projectFileDir_ + "StringHashNames.json");
        if (GetFileSystem()->Exists(filePath))
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

    // Settings.json
    {
        ea::string filePath(projectFileDir_ + "Settings.json");
        if (GetFileSystem()->Exists(filePath))
        {
            JSONFile file(context_);
            if (!file.LoadFile(filePath))
                return false;

            for (auto& pair : file.GetRoot().GetObject())
                engineParameters_[pair.first] = pair.second.GetVariant();
        }
    }

    // Pipeline.json
    // Register asset dirs
    GetCache()->AddResourceDir(GetCachePath(), 0);
    GetCache()->AddResourceDir(GetResourcePath(), 1);

    ea::string filePath(projectFileDir_ + "Pipeline.json");
    if (GetFileSystem()->Exists(filePath))
    {
        JSONFile file(context_);
        if (!file.LoadFile(filePath))
        {
            URHO3D_LOGERROR("Loading 'Pipeline.json' failed likely due to syntax error or insufficient access.");
            return false;
        }

        if (!pipeline_.LoadJSON(file.GetRoot()))
        {
            URHO3D_LOGERROR("Deserialization of 'Pipeline.json' failed.");
            return false;
        }
    }

    if (!GetEngine()->IsHeadless())
    {
        pipeline_.BuildCache(CONVERTER_ALWAYS);
        pipeline_.EnableWatcher();
        GetSubsystem<Editor>()->UpdateWindowTitle();
    }

    // Project.json
    {
        ea::string filePath(projectFileDir_ + "Project.json");
        if (GetFileSystem()->Exists(filePath))
        {
            JSONFile file(context_);
            if (!file.LoadFile(filePath))
                return false;

            const auto& root = file.GetRoot().GetObject();
#if URHO3D_PLUGINS
            auto pluginsIt = root.find("plugins");
            if (pluginsIt != root.end())
            {
                const auto& plugins = pluginsIt->second.GetArray();
                for (const auto& pluginInfoValue : plugins)
                {
                    const JSONObject& pluginInfo = pluginInfoValue.GetObject();
                    auto nameIt = pluginInfo.find("name");
                    if (nameIt == pluginInfo.end())
                    {
                        URHO3D_LOGERRORF("Loading plugin failed: 'name' is missing.");
                        continue;
                    }

                    const ea::string& pluginName = nameIt->second.GetString();
                    if (Plugin* plugin = plugins_.Load(pluginName))
                    {
                        auto privateIt = pluginInfo.find("private");
                        if (privateIt != pluginInfo.end() && privateIt->second.GetBool())
                            plugin->SetFlags(plugin->GetFlags() | PLUGIN_PRIVATE);
                    }
                    else
                        URHO3D_LOGERRORF("Loading plugin '%s' failed.", pluginName.c_str());
                }
                // Tick plugins once to ensure plugins are loaded before loading any possibly open scenes. This makes
                // plugins register themselves with the engine so that loaded scenes can properly load components
                // provided by plugins. Not doing this would cause scenes to load these components as UnknownComponent.
                plugins_.OnEndFrame();
            }
#endif
            auto defaultSceneIt = root.find("default-scene");
            if (defaultSceneIt != root.end())
                defaultScene_ = defaultSceneIt->second.GetString();

            using namespace EditorProjectLoading;
            SendEvent(E_EDITORPROJECTLOADING, P_ROOT, (void*)&root);
        }
    }

    return true;
}

bool Project::SaveProject()
{
    if (GetEngine()->IsHeadless())
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

    // Project.json
    {
        JSONFile file(context_);
        JSONValue& root = file.GetRoot();

        root["version"] = 0;

        // Plugins
#if URHO3D_PLUGINS
        {
            JSONArray plugins{};
            for (const auto& plugin : plugins_.GetPlugins())
            {
                plugins.push_back(JSONObject{{"name",    plugin->GetName()},
                                             {"private", plugin->GetFlags() & PLUGIN_PRIVATE ? true : false}});
            }
            ea::quick_sort(plugins.begin(), plugins.end(), [](const JSONValue& a, const JSONValue& b) {
                auto aNameIt = a.GetObject().find("name");
                auto bNameIt = b.GetObject().find("name");
                const ea::string& nameA = aNameIt != a.GetObject().end() ? aNameIt->second.GetString() : EMPTY_STRING;
                const ea::string& nameB = bNameIt != b.GetObject().end() ? bNameIt->second.GetString() : EMPTY_STRING;
                return nameA.compare(nameB);
            });
            root["plugins"] = plugins;
        }
#endif
        root["default-scene"] = defaultScene_;

        ea::string filePath(projectFileDir_ + "Project.json");
        if (!file.SaveFile(filePath))
        {
            projectFileDir_.clear();
            URHO3D_LOGERRORF("Saving project to '%s' failed", filePath.c_str());
            return false;
        }

        using namespace EditorProjectSaving;
        SendEvent(E_EDITORPROJECTSAVING, P_ROOT, (void*)&root);
    }

    // Settings.json
    {
        JSONFile file(context_);
        JSONValue& root = file.GetRoot();

        for (const auto& pair : engineParameters_)
            root[pair.first].SetVariant(pair.second, context_);

        ea::string filePath(projectFileDir_ + "Settings.json");
        if (!file.SaveFile(filePath))
        {
            projectFileDir_.clear();
            URHO3D_LOGERRORF("Saving project to '%s' failed", filePath.c_str());
            return false;
        }
    }

#if URHO3D_HASH_DEBUG
    // StringHashNames.json
    {
        auto hashNames = StringHash::GetGlobalStringHashRegister()->GetInternalMap().values();
        ea::quick_sort(hashNames.begin(), hashNames.end());
        JSONFile file(context_);
        JSONArray names;
        for (const auto& string : hashNames)
            names.push_back(string.ToString());
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

    ui::SaveIniSettingsToDisk(uiConfigPath_.c_str());

    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this));

    return true;
}

ea::string Project::GetCachePath() const
{
    if (projectFileDir_.empty())
        return EMPTY_STRING;
    return projectFileDir_ + "Cache/";
}

ea::string Project::GetResourcePath() const
{
    if (projectFileDir_.empty())
        return EMPTY_STRING;
    return projectFileDir_ + "Resources/";
}

}
