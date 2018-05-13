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
    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this, String::EMPTY));
}

Project::~Project()
{
    for (auto& directory : dataDirectories_)
    {
        assetConverter_.RemoveAssetDirectory(directory);
        GetCache()->RemoveResourceDir(directory);
    }
}

bool Project::CreateProject(const String& projectPath)
{
    return false;
}

bool Project::LoadProject(const String& filePath)
{
    if (!projectFilePath_.Empty())
        // Project is already loaded.
        return false;

    if (filePath.Empty())
        return false;

    YAMLFile file(context_);
    if (!file.LoadFile(filePath))
        return false;

    const auto& root = file.GetRoot();
    if (root.IsObject())
    {
        context_->RemoveSubsystem<Project>();
        projectFilePath_ = filePath;
        projectFileDir_ = GetParentPath(filePath);

        SendEvent(E_EDITORPROJECTLOADINGSTART);

        const auto& window = root["window"];
        if (window.IsObject())
        {
            auto size = window["size"].GetVariant().GetIntVector2();
            GetGraphics()->SetMode(size.x_, size.y_);
            GetGraphics()->SetWindowPosition(window["position"].GetVariant().GetIntVector2());
        }

        // Autodetect resource dirs
        StringVector directories;
        GetFileSystem()->ScanDir(directories, projectFileDir_ + "bin/", "", SCAN_DIRS, false);
        assetConverter_.SetCachePath(GetCachePath());
        for (auto& dir : directories)
        {
            if (dir == "." || dir == ".." || !dir.ToLower().EndsWith("data"))
                continue;
            // Convert to absolute path
            dir = ToString("%sbin/%s/", projectFileDir_.CString(), dir.CString());
            GetCache()->AddResourceDir(dir);
            assetConverter_.AddAssetDirectory(dir);
        }
        assetConverter_.VerifyCacheAsync();

        const auto& tabs = root["tabs"];
        if (tabs.IsArray())
        {
            for (auto i = 0; i < tabs.Size(); i++)
            {
                const auto& tab = tabs[i];
                if (tab["type"] == "scene")
                    GetSubsystem<Editor>()->CreateNewTab<SceneTab>(tab);
                else if (tab["type"] == "ui")
                    GetSubsystem<Editor>()->CreateNewTab<UITab>(tab);
            }
        }

        ui::LoadDock(root["docks"]);

        // Plugins may load state by subscribing to this event
        using namespace EditorProjectLoading;
        SendEvent(E_EDITORPROJECTLOADING, P_VALUE, (void*)&root);

        context_->RegisterSubsystem(this);

        return true;
    }

    return false;
}

bool Project::SaveProject(const String& filePath)
{
    // Saving project data of tabs may trigger saving resources, which in turn triggers saving editor project. Avoid
    // that loop.
    UnsubscribeFromEvent(E_EDITORRESOURCESAVED);

    auto&& projectFilePath = filePath.Empty() ? projectFilePath_ : filePath;
    if (projectFilePath.Empty())
    {
        URHO3D_LOGERROR("Unable to save project. Project path is empty.");
        return false;
    }

    YAMLFile file(context_);
    JSONValue& root = file.GetRoot();
    root["version"] = 0;

    JSONValue& window = root["window"];
    {
        window["size"].SetVariant(GetGraphics()->GetSize());
        window["position"].SetVariant(GetGraphics()->GetWindowPosition());
    }

    auto&& openTabs = GetSubsystem<Editor>()->GetContentTabs();
    auto& fileTabs = root["tabs"];
    fileTabs.Resize(openTabs.Size());
    for (auto i = 0; i < openTabs.Size(); i++)
        openTabs[i]->SaveProject(fileTabs[i]);

    ui::SaveDock(root["docks"]);

    // Plugins may save state by subscribing to this event
    using namespace EditorProjectSaving;
    SendEvent(E_EDITORPROJECTSAVING, P_VALUE, (void*)&root);

    auto success = file.SaveFile(projectFilePath);
    if (success)
        projectFilePath_ = projectFilePath;
    else
    {
        projectFilePath_.Clear();
        URHO3D_LOGERRORF("Saving project to %s failed", projectFilePath.CString());
    }

    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this, ""));

    return success;
}

String Project::GetCachePath() const
{
    if (projectFileDir_.Empty())
        return String::EMPTY;
    return projectFileDir_ + "bin/Cache/";
}

}
