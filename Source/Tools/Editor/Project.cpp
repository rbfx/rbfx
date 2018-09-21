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

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Toolbox/SystemUI/ImGuiDock.h>
#include <Tabs/ConsoleTab.h>
#include <Tabs/ResourceTab.h>
#include <Tabs/HierarchyTab.h>
#include <Tabs/InspectorTab.h>

#include "Editor.h"
#include "EditorEvents.h"
#include "Project.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/UI/UITab.h"


namespace Urho3D
{

Project::Project(Context* context)
    : Object(context)
    , assetConverter_(context)
{
    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this));
}

Project::~Project()
{
    ui::GetIO().IniFilename = nullptr;

    if (auto* cache = GetCache())
    {
        cache->RemoveResourceDir(GetCachePath());
        cache->RemoveResourceDir(GetResourcePath());

        for (const auto& path : cachedEngineResourcePaths_)
            GetCache()->AddResourceDir(path);
    }

    // Clear dock state
    ui::LoadDock(JSONValue::EMPTY);
}

bool Project::LoadProject(const String& projectPath)
{
    if (!projectFilePath_.Empty())
        // Project is already loaded.
        return false;

    if (projectPath.Empty())
        return false;

    projectFileDir_ = AddTrailingSlash(projectPath);
    projectFilePath_ = projectFileDir_ + ".user.json";

    if (!GetFileSystem()->Exists(GetCachePath()))
        GetFileSystem()->CreateDirsRecursive(GetCachePath());

    if (!GetFileSystem()->Exists(GetResourcePath()))
    {
        // Initialize new project
        GetFileSystem()->CreateDirsRecursive(GetResourcePath());

        for (const auto& path : GetCache()->GetResourceDirs())
        {
            if (path.EndsWith("/EditorData/") || path.Contains("/Autoload/"))
                continue;

            StringVector names;

            URHO3D_LOGINFOF("Importing resources from '%s'", path.CString());

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

    // Register asset dirs
    GetCache()->AddResourceDir(GetCachePath(), 0);
    GetCache()->AddResourceDir(GetResourcePath(), 1);
    assetConverter_.SetCachePath(GetCachePath());
    assetConverter_.AddAssetDirectory(GetResourcePath());
    assetConverter_.VerifyCacheAsync();

    // Unregister engine dirs
    auto enginePrefixPath = GetSubsystem<Editor>()->GetCoreResourcePrefixPath();
    auto pathsCopy = GetCache()->GetResourceDirs();
    cachedEngineResourcePaths_.Clear();
    for (const auto& path : pathsCopy)
    {
        if (path.StartsWith(enginePrefixPath) && !path.EndsWith("/EditorData/"))
        {
            cachedEngineResourcePaths_.EmplaceBack(path);
            GetCache()->RemoveResourceDir(path);
        }
    }

    uiConfigPath_ = projectPath + "/.ui.ini";
    ui::GetIO().IniFilename = uiConfigPath_.CString();

    // Load default layout if no user config exists
    if (!GetFileSystem()->Exists(projectFilePath_))
    {
        GetSubsystem<Editor>()->LoadDefaultLayout();
        return true;
    }

    // Load user config
    JSONFile file(context_);
    if (!file.LoadFile(projectFilePath_))
        return false;

    const auto& root = file.GetRoot();
    if (root.IsObject())
    {
        SendEvent(E_EDITORPROJECTLOADINGSTART);

        const auto& window = root["window"];
        if (window.IsObject())
        {
            auto size = window["size"].GetVariant().GetIntVector2();
            GetGraphics()->SetMode(size.x_, size.y_);
            GetGraphics()->SetWindowPosition(window["position"].GetVariant().GetIntVector2());
        }

        const auto& tabs = root["tabs"];
        if (tabs.IsArray())
        {
            auto* editor = GetSubsystem<Editor>();
            for (auto i = 0; i < tabs.Size(); i++)
            {
                const auto& tab = tabs[i];

                auto tabType = tab["type"].GetString();
                auto* runtimeTab = editor->CreateTab(tabType);

                if (runtimeTab == nullptr)
                    URHO3D_LOGERRORF("Unknown tab type '%s'", tabType.CString());
                else
                    runtimeTab->OnLoadProject(tab);
            }
        }

        ui::LoadDock(root["docks"]);

        // Plugins may load state by subscribing to this event
        using namespace EditorProjectLoading;
        SendEvent(E_EDITORPROJECTLOADING, P_ROOT, (void*)&root);

        return true;
    }

    return false;
}

bool Project::SaveProject()
{
    // Saving project data of tabs may trigger saving resources, which in turn triggers saving editor project. Avoid
    // that loop.
    UnsubscribeFromEvent(E_EDITORRESOURCESAVED);

    if (projectFilePath_.Empty())
    {
        URHO3D_LOGERROR("Unable to save project. Project path is empty.");
        return false;
    }

    JSONFile file(context_);
    JSONValue& root = file.GetRoot();
    root["version"] = 0;

    JSONValue& window = root["window"];
    {
        window["size"].SetVariant(GetGraphics()->GetSize());
        window["position"].SetVariant(GetGraphics()->GetWindowPosition());
    }

    // Plugins may save state by subscribing to this event
    using namespace EditorProjectSaving;
    SendEvent(E_EDITORPROJECTSAVING, P_ROOT, (void*)&root);

    ui::SaveDock(root["docks"]);

    auto success = file.SaveFile(projectFilePath_);
    if (!success)
    {
        projectFilePath_.Clear();
        URHO3D_LOGERRORF("Saving project to '%s' failed", projectFilePath_.CString());
    }

    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this));

    return success;
}

String Project::GetCachePath() const
{
    if (projectFileDir_.Empty())
        return String::EMPTY;
    return projectFileDir_ + "Cache/";
}

String Project::GetResourcePath() const
{
    if (projectFileDir_.Empty())
        return String::EMPTY;
    return projectFileDir_ + "Resources/";
}

}
