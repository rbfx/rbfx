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

#include "../Core/EditorPluginManager.h"
#include "../Project/ProjectEditor.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

static unsigned numActiveProjects = 0;

OpenResourceRequest OpenResourceRequest::FromResourceName(Context* context, const ea::string& resourceName)
{
    auto cache = context->GetSubsystem<ResourceCache>();

    const auto file = cache->GetFile(resourceName);
    if (!file)
        return {};

    OpenResourceRequest request;
    request.fileName_ = file->GetAbsoluteName();
    request.resourceName_ = resourceName;
    request.file_ = file;

    if (request.resourceName_.ends_with(".xml"))
    {
        request.xmlFile_ = MakeShared<XMLFile>(context);
        request.xmlFile_->Load(*file);
        file->Seek(0);
    }

    if (request.resourceName_.ends_with(".json"))
    {
        request.jsonFile_ = MakeShared<JSONFile>(context);
        request.jsonFile_->Load(*file);
    }

    return request;
}

ResourceCacheGuard::ResourceCacheGuard(Context* context)
    : context_(context)
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    oldResourceDirs_ = cache->GetResourceDirs();
    for (const ea::string& resourceDir : oldResourceDirs_)
    {
        if (oldCoreData_.empty() && resourceDir.ends_with("/CoreData/"))
            oldCoreData_ = resourceDir;
        if (oldEditorData_.empty() && resourceDir.ends_with("/EditorData/"))
            oldEditorData_ = resourceDir;
    }
}

ResourceCacheGuard::~ResourceCacheGuard()
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    cache->RemoveAllResourceDirs();
    for (const ea::string& resourceDir : oldResourceDirs_)
        cache->AddResourceDir(resourceDir);
}

ProjectEditor::ProjectEditor(Context* context, const ea::string& projectPath)
    : Object(context)
    , projectPath_(GetSanitizedPath(projectPath + "/"))
    , coreDataPath_(projectPath_ + "CoreData/")
    , cachePath_(projectPath_ + "Cache/")
    , projectJsonPath_(projectPath_ + "Project.json")
    , uiIniPath_(projectPath_ + ".ui.ini")
    , dataPath_(projectPath_ + "Data/")
    , oldCacheState_(context)
{
    URHO3D_ASSERT(numActiveProjects == 0);
    context_->RegisterSubsystem(this, ProjectEditor::GetTypeStatic());
    ++numActiveProjects;

    ui::GetIO().IniFilename = uiIniPath_.c_str();

    EnsureDirectoryInitialized();
    InitializeResourceCache();

    ApplyPlugins();
}

ProjectEditor::~ProjectEditor()
{
    --numActiveProjects;
    URHO3D_ASSERT(numActiveProjects == 0);

    ui::GetIO().IniFilename = nullptr;
}

void ProjectEditor::AddTab(SharedPtr<EditorTab> tab)
{
    tabs_.push_back(tab);
}

void ProjectEditor::OpenResource(const OpenResourceRequest& request)
{
    for (EditorTab* tab : tabs_)
    {
        if (auto resourceTab = dynamic_cast<ResourceEditorTab*>(tab))
        {
            if (resourceTab->CanOpenResource(request))
            {
                resourceTab->OpenResource(request.resourceName_);
                resourceTab->Focus();
                break;
            }
        }
    }
}

void ProjectEditor::EnsureDirectoryInitialized()
{
    auto fs = GetSubsystem<FileSystem>();

    if (!fs->DirExists(cachePath_))
    {
        if (fs->FileExists(cachePath_))
            fs->Delete(cachePath_);
        fs->CreateDirsRecursive(cachePath_);
    }

    if (!fs->DirExists(coreDataPath_))
    {
        if (fs->FileExists(coreDataPath_))
            fs->Delete(coreDataPath_);
        fs->CopyDir(oldCacheState_.GetCoreData(), coreDataPath_);
    }

    if (!fs->FileExists(projectJsonPath_))
    {
        if (fs->DirExists(projectJsonPath_))
            fs->RemoveDir(projectJsonPath_, true);

        JSONFile emptyFile(context_);
        emptyFile.SaveFile(projectJsonPath_);
    }

    // Legacy: to support old projects
    if (fs->DirExists(projectPath_ + "Resources/"))
        dataPath_ = projectPath_ + "Resources/";

    if (!fs->DirExists(dataPath_))
    {
        if (fs->FileExists(dataPath_))
            fs->Delete(dataPath_);
        fs->CreateDirsRecursive(dataPath_);
    }

    if (!fs->FileExists(uiIniPath_))
    {
        if (fs->DirExists(uiIniPath_))
            fs->RemoveDir(uiIniPath_, true);
        pendingResetLayout_ = true;
    }
}

void ProjectEditor::InitializeResourceCache()
{
    auto cache = GetSubsystem<ResourceCache>();
    cache->RemoveAllResourceDirs();
    cache->AddResourceDir(dataPath_);
    cache->AddResourceDir(coreDataPath_);
    cache->AddResourceDir(cachePath_);
    cache->AddResourceDir(oldCacheState_.GetEditorData());
}

void ProjectEditor::ResetLayout()
{
    pendingResetLayout_ = false;

    ImGui::DockBuilderRemoveNode(dockspaceId_);
    ImGui::DockBuilderAddNode(dockspaceId_, 0);
    ImGui::DockBuilderSetNodeSize(dockspaceId_, ui::GetMainViewport()->Size);

    ImGuiID dockCenter = dockspaceId_;
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, 0.20f, nullptr, &dockCenter);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.30f, nullptr, &dockCenter);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.30f, nullptr, &dockCenter);

    for (EditorTab* tab : tabs_)
    {
        switch (tab->GetPlacement())
        {
        case EditorTabPlacement::DockCenter:
            ImGui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockCenter);
            break;
        case EditorTabPlacement::DockLeft:
            ImGui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockLeft);
            break;
        case EditorTabPlacement::DockRight:
            ImGui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockRight);
            break;
        case EditorTabPlacement::DockBottom:
            ImGui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockBottom);
            break;
        }
    }
    ImGui::DockBuilderFinish(dockspaceId_);

    for (EditorTab* tab : tabs_)
    {
        if (tab->GetFlags().Test(EditorTabFlag::OpenByDefault))
            tab->Open();
    }
}

void ProjectEditor::ApplyPlugins()
{
    auto editorPluginManager = GetSubsystem<EditorPluginManager>();
    editorPluginManager->Apply(this);

    for (EditorTab* tab : tabs_)
        tab->ApplyPlugins();
}

void ProjectEditor::UpdateAndRender()
{
    dockspaceId_ = ui::GetID("Root");
    ui::DockSpace(dockspaceId_);

    if (pendingResetLayout_)
        ResetLayout();

    // TODO(editor): Postpone this call until assets are imported
    if (!initialized_)
    {
        initialized_ = true;
        OnInitialized(this);
    }

    for (EditorTab* tab : tabs_)
        tab->UpdateAndRender();
}

void ProjectEditor::Save()
{
    ui::SaveIniSettingsToDisk(uiIniPath_.c_str());
}

void ProjectEditor::ReadIniSettings(const char* entry, const char* line)
{
    for (EditorTab* tab : tabs_)
    {
        if (entry == tab->GetUniqueId())
            tab->ReadIniSettings(line);
    }
}

void ProjectEditor::WriteIniSettings(ImGuiTextBuffer* output)
{
    for (EditorTab* tab : tabs_)
        tab->WriteIniSettings(output);
}

}
