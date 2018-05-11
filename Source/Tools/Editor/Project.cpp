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

    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (!xml->LoadFile(filePath))
        return false;

    auto root = xml->GetRoot();
    if (root.NotNull())
    {
        context_->RemoveSubsystem<Project>();
        projectFilePath_ = filePath;
        projectFileDir_ = GetParentPath(filePath);
        SendEvent(E_EDITORPROJECTLOADING);

        auto window = root.GetChild("window");
        if (window.NotNull())
        {
            GetGraphics()->SetMode(ToInt(window.GetAttribute("width")), ToInt(window.GetAttribute("height")));
            GetGraphics()->SetWindowPosition(ToInt(window.GetAttribute("x")), ToInt(window.GetAttribute("y")));
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

        auto tabs = root.GetChild("tabs");
        if (tabs.NotNull())
        {
            auto tab = tabs.GetChild("tab");
            while (tab.NotNull())
            {
                if (tab.GetAttribute("type") == "scene")
                    GetSubsystem<Editor>()->CreateNewTab<SceneTab>(tab);
                else if (tab.GetAttribute("type") == "ui")
                    GetSubsystem<Editor>()->CreateNewTab<UITab>(tab);
                tab = tab.GetNext();
            }
        }

        ui::LoadDock(root.GetChild("docks"));

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

    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement root = xml->CreateRoot("project");
    root.SetAttribute("version", "0");

    auto window = root.CreateChild("window");
    window.SetAttribute("width", ToString("%d", GetGraphics()->GetWidth()));
    window.SetAttribute("height", ToString("%d", GetGraphics()->GetHeight()));
    window.SetAttribute("x", ToString("%d", GetGraphics()->GetWindowPosition().x_));
    window.SetAttribute("y", ToString("%d", GetGraphics()->GetWindowPosition().y_));

    auto scenes = root.CreateChild("tabs");
    for (auto& tab: GetSubsystem<Editor>()->GetContentTabs())
    {
        XMLElement tabXml = scenes.CreateChild("tab");
        tab->SaveProject(tabXml);
    }

    ui::SaveDock(root.CreateChild("docks"));

    if (!xml->SaveFile(projectFilePath))
    {
        projectFilePath_.Clear();
        URHO3D_LOGERRORF("Saving project to %s failed", projectFilePath.CString());
    }

    SubscribeToEvent(E_EDITORRESOURCESAVED, std::bind(&Project::SaveProject, this, ""));
}

String Project::GetCachePath() const
{
    if (projectFileDir_.Empty())
        return String::EMPTY;
    return projectFileDir_ + "bin/Cache/";
}

}
