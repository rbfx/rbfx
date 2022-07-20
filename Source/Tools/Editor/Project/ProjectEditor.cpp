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

#include "../Project/ProjectEditor.h"

#include "../Core/EditorPluginManager.h"
#include "../Core/IniHelpers.h"
#include "../Core/SettingsManager.h"
#include "../Core/UndoManager.h"
#include "../Project/AssetManager.h"
#include "../Project/CreateDefaultScene.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>
#include <Urho3D/Utility/SceneViewerApplication.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <string>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_SaveProject, "Global.SaveProject", QUAL_CTRL | QUAL_SHIFT, KEY_S);

static unsigned numActiveProjects = 0;

static const ea::string selfIniEntry{"Project"};

bool IsEscapedChar(const char ch)
{
    switch (ch)
    {
    case '[':
    case ']':

    case '(':
    case ')':

    case '{':
    case '}':

    case '*':
    case '+':
    case '?':
    case '|':

    case '^':
    case '$':

    case '.':
    case '\\':
        return true;

    default:
        return false;
    }
}

std::regex PatternToRegex(const ea::string& pattern)
{
    std::string r;
    for (const char ch : pattern)
    {
        if (ch == '*')
            r += ".*";
        else if (ch == '?')
            r += '.';
        else if (IsEscapedChar(ch))
        {
            r += '\\';
            r += ch;
        }
        else
            r += ch;
    }
    return std::regex(r, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
}

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
    , tempPath_(projectPath_ + "Temp/")
    , projectJsonPath_(projectPath_ + "Project.json")
    , settingsJsonPath_(projectPath_ + "Settings.json")
    , cacheJsonPath_(projectPath_ + "Cache.json")
    , uiIniPath_(projectPath_ + ".ui.ini")
    , gitIgnorePath_(projectPath_ + ".gitignore")
    , dataPath_(projectPath_ + "Data/")
    , oldCacheState_(context)
    , hotkeyManager_(MakeShared<HotkeyManager>(context_))
    , undoManager_(MakeShared<UndoManager>(context_))
    , settingsManager_(MakeShared<SettingsManager>(context_))
    , pluginManager_(MakeShared<PluginManager>(context_))
    , launchManager_(MakeShared<LaunchManager>(context_))
    , closeDialog_(MakeShared<CloseDialog>(context_))
{
    auto initializationGuard = ea::make_shared<int>(0);
    initializationGuard_ = initializationGuard;

    URHO3D_ASSERT(numActiveProjects == 0);
    context_->RegisterSubsystem(this, ProjectEditor::GetTypeStatic());
    ++numActiveProjects;

    context_->RemoveSubsystem<PluginManager>();
    context_->RegisterSubsystem(pluginManager_);

    ui::GetIO().IniFilename = uiIniPath_.c_str();

    InitializeHotkeys();
    EnsureDirectoryInitialized();
    InitializeResourceCache();

    // Delay asset manager creation until project is ready
    assetManager_ = MakeShared<AssetManager>(context);
    assetManager_->OnInitialized.Subscribe(this, [=](ProjectEditor*) mutable { initializationGuard.reset(); });

    ApplyPlugins();

    assetManager_->LoadFile(cacheJsonPath_);
    settingsManager_->LoadFile(settingsJsonPath_);

    JSONFile projectJsonFile(context_);
    projectJsonFile.LoadFile(projectJsonPath_);
    JSONInputArchive archive{&projectJsonFile};
    SerializeOptionalValue(archive, "Project", *this, AlwaysSerialize{});

    if (firstInitialization_)
        InitializeDefaultProject();
}

void ProjectEditor::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "PluginManager", *pluginManager_, AlwaysSerialize{});
    SerializeOptionalValue(archive, "LaunchManager", *launchManager_, AlwaysSerialize{});
}

ProjectEditor::~ProjectEditor()
{
    ProcessDelayedSaves(true);

    --numActiveProjects;
    URHO3D_ASSERT(numActiveProjects == 0);

    context_->RemoveSubsystem<PluginManager>();
    context_->RegisterSubsystem(pluginManager_);

    ui::GetIO().IniFilename = nullptr;
}

CloseProjectResult ProjectEditor::CloseGracefully()
{
    // Always save shallow data on close
    SaveShallowOnly();

    // If result is ready, return it now and reset state
    if (closeProjectResult_ != CloseProjectResult::Undefined)
    {
        const auto result = closeProjectResult_;
        closeProjectResult_ = CloseProjectResult::Undefined;
        return result;
    }

    // Wait if dialog is already open
    if (closeDialog_->IsActive())
        return CloseProjectResult::Undefined;

    // Collect unsaved items
    ea::vector<ea::string> unsavedItems;
    if (hasUnsavedChanges_)
        unsavedItems.push_back("[Project]");
    for (EditorTab* tab : tabs_)
        tab->EnumerateUnsavedItems(unsavedItems);

    // If nothing to save, close immediately
    if (unsavedItems.empty())
        return CloseProjectResult::Closed;

    // Open popup otherwise
    CloseResourceRequest request;
    request.resourceNames_ = ea::move(unsavedItems);
    request.onSave_ = [this]()
    {
        Save();
        closeProjectResult_ = CloseProjectResult::Closed;
    };
    request.onDiscard_ = [this]()
    {
        closeProjectResult_ = CloseProjectResult::Closed;
    };
    request.onCancel_ = [this]()
    {
        closeProjectResult_ = CloseProjectResult::Canceled;
    };
    closeDialog_->RequestClose(ea::move(request));
    return CloseProjectResult::Undefined;
}

void ProjectEditor::CloseResourceGracefully(const CloseResourceRequest& request)
{
    closeDialog_->RequestClose(request);
}

void ProjectEditor::ProcessRequest(ProjectRequest* request, EditorTab* sender)
{
    pendingRequests_.emplace_back(PendingRequest{SharedPtr<ProjectRequest>{request}, WeakPtr<EditorTab>{sender}});
}

void ProjectEditor::AddAnalyzeFileCallback(const AnalyzeFileCallback& callback)
{
    analyzeFileCallbacks_.push_back(callback);
}

ResourceFileDescriptor ProjectEditor::GetResourceDescriptor(const ea::string& resourceName, const ea::string& fileName) const
{
    auto cache = context_->GetSubsystem<ResourceCache>();

    AnalyzeFileContext ctx;
    ctx.binaryFile_ = cache->GetFile(resourceName, false);

    if (ctx.binaryFile_ && resourceName.ends_with(".xml", false))
    {
        ctx.xmlFile_ = MakeShared<XMLFile>(context_);
        ctx.xmlFile_->Load(*ctx.binaryFile_);
        ctx.binaryFile_->Seek(0);
    }

    if (ctx.binaryFile_ && resourceName.ends_with(".json", false))
    {
        ctx.jsonFile_ = MakeShared<JSONFile>(context_);
        ctx.jsonFile_->Load(*ctx.binaryFile_);
        ctx.binaryFile_->Seek(0);
    }

    ResourceFileDescriptor result;
    result.localName_ = GetFileNameAndExtension(resourceName);
    result.resourceName_ = resourceName;
    result.fileName_ = fileName;

    if (ctx.binaryFile_ && result.fileName_.empty())
        result.fileName_ = ctx.binaryFile_->GetAbsoluteName();
    if (result.fileName_.empty())
        result.fileName_ = dataPath_ + resourceName;

    result.isDirectory_ = ctx.binaryFile_ == nullptr;
    result.isAutomatic_ = result.fileName_.starts_with(cachePath_);

    for (const auto& callback : analyzeFileCallbacks_)
        callback(result, ctx);

    return result;
}

void ProjectEditor::SaveFileDelayed(const ea::string& fileName, const ea::string& resourceName, const SharedByteVector& bytes)
{
    delayedFileSaves_[resourceName] = PendingFileSave{fileName, bytes};
}

void ProjectEditor::SaveFileDelayed(Resource* resource)
{
    delayedFileSaves_[resource->GetName()] = PendingFileSave{resource->GetAbsoluteFileName(), nullptr, SharedPtr<Resource>(resource)};
}

void ProjectEditor::IgnoreFileNamePattern(const ea::string& pattern)
{
    const bool inserted = ignoredFileNames_.insert(pattern).second;
    if (inserted)
        ignoredFileNameRegexes_.push_back(PatternToRegex(pattern));
}

bool ProjectEditor::IsFileNameIgnored(const ea::string& fileName) const
{
    for (const std::regex& r : ignoredFileNameRegexes_)
    {
        if (std::regex_match(fileName.begin(), fileName.end(), r))
            return true;
    }
    return false;
}

void ProjectEditor::AddTab(SharedPtr<EditorTab> tab)
{
    tabs_.push_back(tab);
    sortedTabs_[tab->GetTitle()] = tab;
}

ea::string ProjectEditor::GetRandomTemporaryPath() const
{
    return Format("{}{}/", tempPath_, GenerateUUID());
}

TemporaryDir ProjectEditor::CreateTemporaryDir()
{
    return TemporaryDir{context_, GetRandomTemporaryPath()};
}

void ProjectEditor::InitializeHotkeys()
{
    hotkeyManager_->BindHotkey(this, Hotkey_SaveProject, &ProjectEditor::Save);
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

    if (!fs->DirExists(tempPath_))
    {
        if (fs->FileExists(tempPath_))
            fs->Delete(tempPath_);
        fs->CreateDirsRecursive(tempPath_);
    }

    if (!fs->DirExists(coreDataPath_))
    {
        if (fs->FileExists(coreDataPath_))
            fs->Delete(coreDataPath_);
        fs->CopyDir(oldCacheState_.GetCoreData(), coreDataPath_);
    }

    if (!fs->FileExists(settingsJsonPath_))
    {
        if (fs->DirExists(settingsJsonPath_))
            fs->RemoveDir(settingsJsonPath_, true);

        JSONFile emptyFile(context_);
        emptyFile.SaveFile(settingsJsonPath_);
    }

    if (!fs->FileExists(projectJsonPath_))
    {
        if (fs->DirExists(projectJsonPath_))
            fs->RemoveDir(projectJsonPath_, true);

        JSONFile emptyFile(context_);
        emptyFile.SaveFile(projectJsonPath_);

        firstInitialization_ = true;
    }

    if (!fs->FileExists(cacheJsonPath_))
    {
        if (fs->DirExists(cacheJsonPath_))
            fs->RemoveDir(cacheJsonPath_, true);

        JSONFile emptyFile(context_);
        emptyFile.SaveFile(cacheJsonPath_);
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

void ProjectEditor::InitializeDefaultProject()
{
    pluginManager_->SetPluginsLoaded({SceneViewerApplication::GetStaticPluginName()});

    const ea::string configName = "View Current Scene";
    launchManager_->AddConfiguration(LaunchConfiguration{configName, SceneViewerApplication::GetStaticPluginName()});
    currentLaunchConfiguration_ = configName;

    const ea::string defaultSceneName = "Scenes/DefaultScene.xml";
    DefaultSceneParameters params;
    params.highQuality_ = true;
    params.createObjects_ = true;
    CreateDefaultScene(context_, dataPath_ + defaultSceneName, params);

    const auto request = MakeShared<OpenResourceRequest>(context_, defaultSceneName);
    ProcessRequest(request, nullptr);
    Save();
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

void ProjectEditor::SaveGitIgnore()
{
    ea::string content;

    content += "# Ignore asset cache\n";
    content += "/Cache/\n";
    content += "/Cache.json\n";
    content += "\n";

    content += "# Ignore UI settings\n";
    content += "/.ui.ini\n";
    content += "\n";

    content += "# Ignore internal files\n";
    for (const ea::string& pattern : ignoredFileNames_)
        content += Format("{}\n", pattern);
    content += "\n";

    File file(context_, gitIgnorePath_, FILE_WRITE);
    if (file.IsOpen())
        file.Write(content.c_str(), content.size());
}

void ProjectEditor::Render()
{
    const float tint = 0.15f;
    const ColorScopeGuard guardHighlightColors{{
        {ImGuiCol_Tab,                ImVec4(0.26f, 0.26f + tint, 0.26f, 0.40f)},
        {ImGuiCol_TabHovered,         ImVec4(0.31f, 0.31f + tint, 0.31f, 1.00f)},
        {ImGuiCol_TabActive,          ImVec4(0.28f, 0.28f + tint, 0.28f, 1.00f)},
        {ImGuiCol_TabUnfocused,       ImVec4(0.17f, 0.17f + tint, 0.17f, 1.00f)},
        {ImGuiCol_TabUnfocusedActive, ImVec4(0.26f, 0.26f + tint, 0.26f, 1.00f)},
    }, isHighlightEnabled_};

    hotkeyManager_->Update();
    hotkeyManager_->InvokeFor(hotkeyManager_);
    if (areGlobalHotkeysEnabled_)
        hotkeyManager_->InvokeFor(this);

    assetManager_->Update();

    dockspaceId_ = ui::GetID("Root");
    ui::DockSpace(dockspaceId_);

    if (pendingResetLayout_)
        ResetLayout();

    bool initialFocusPending = false;
    if (!initialized_ && initializationGuard_.expired())
    {
        initialized_ = true;
        initialFocusPending = true;

        OnInitialized(this);
    }

    for (EditorTab* tab : tabs_)
        tab->PreRenderUpdate();
    for (EditorTab* tab : tabs_)
        tab->Render();
    if (focusedTab_)
        focusedTab_->ApplyHotkeys(hotkeyManager_);
    for (EditorTab* tab : tabs_)
        tab->PostRenderUpdate();

    closeDialog_->Render();

    if (initialFocusPending)
    {
        for (EditorTab* tab : tabs_)
        {
            if (tab->IsOpen() && tab->GetFlags().Test(EditorTabFlag::FocusOnStart))
                tab->Focus(true);
        }
    }

    ProcessDelayedSaves();
    ProcessPendingRequests();
}

void ProjectEditor::ProcessPendingRequests()
{
    const auto requests = ea::move(pendingRequests_);
    pendingRequests_.clear();

    for (const PendingRequest& pending : requests)
    {
        EditorTab* sender = pending.sender_;
        OnRequest(sender, pending.request_);
        pending.request_->InvokeProcessCallback();
    }
}

void ProjectEditor::ProcessDelayedSaves(bool forceSave)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto fs = GetSubsystem<FileSystem>();
    for (auto& [resourceName, delayedSave] : delayedFileSaves_)
    {
        if (!forceSave && delayedSave.timer_.GetMSec(false) < saveDelayMs_)
            continue;

        const bool fileExists = fs->FileExists(delayedSave.fileName_);

        if (delayedSave.bytes_)
        {
            auto file = MakeShared<File>(context_, delayedSave.fileName_, FILE_WRITE);
            if (file->IsOpen())
                file->Write(delayedSave.bytes_->data(), delayedSave.bytes_->size());
        }
        else
        {
            delayedSave.resource_->SaveFile(delayedSave.fileName_);
        }

        if (fileExists)
            cache->IgnoreResourceReload(resourceName);

        delayedSave.Clear();
    }

    ea::erase_if(delayedFileSaves_, [](const auto& pair) { return pair.second.IsEmpty(); });
}

void ProjectEditor::RenderToolbar()
{
    if (Widgets::ToolbarButton(ICON_FA_FLOPPY_DISK, "Save Project"))
        Save();
    OnRenderProjectToolbar(this);

    Widgets::ToolbarSeparator();

    if (focusedRootTab_)
        focusedRootTab_->RenderToolbar();
}

void ProjectEditor::RenderProjectMenu()
{
    if (ui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project", hotkeyManager_->GetHotkeyLabel(Hotkey_SaveProject).c_str()))
        Save();
    OnRenderProjectMenu(this);
}

void ProjectEditor::RenderMainMenu()
{
    if (focusedRootTab_)
        focusedRootTab_->RenderMenu();

    if (focusedRootTab_ && ui::BeginMenu("Tab"))
    {
        focusedRootTab_->RenderContextMenuItems();
        ui::EndMenu();
    }

    if (ui::BeginMenu("Window"))
    {
        for (const auto& [title, tab] : sortedTabs_)
        {
            bool open = tab->IsOpen();
            if (ui::MenuItem(title.c_str(), "", &open))
            {
                if (!open)
                    tab->Close();
                else
                    tab->Focus();
            }
        }
        ui::EndMenu();
    }
}

void ProjectEditor::SaveShallowOnly()
{
    ui::SaveIniSettingsToDisk(uiIniPath_.c_str());
    settingsManager_->SaveFile(settingsJsonPath_);

    for (EditorTab* tab : tabs_)
    {
        if (auto resourceTab = dynamic_cast<ResourceEditorTab*>(tab))
            resourceTab->SaveShallow();
    }
}

void ProjectEditor::SaveProjectOnly()
{
    JSONFile projectJsonFile(context_);
    JSONOutputArchive archive{&projectJsonFile};
    SerializeOptionalValue(archive, "Project", *this, AlwaysSerialize{});
    projectJsonFile.SaveFile(projectJsonPath_);

    SaveGitIgnore();
    assetManager_->SaveFile(cacheJsonPath_);

    hasUnsavedChanges_ = false;
}

void ProjectEditor::SaveResourcesOnly()
{
    for (EditorTab* tab : tabs_)
    {
        if (auto resourceTab = dynamic_cast<ResourceEditorTab*>(tab))
            resourceTab->SaveAllResources();
    }
}

void ProjectEditor::Save()
{
    SaveProjectOnly();
    SaveShallowOnly();
    SaveResourcesOnly();
}

void ProjectEditor::ReadIniSettings(const char* entry, const char* line)
{
    if (entry == selfIniEntry)
    {
        if (const auto value = ReadStringFromIni(line, "LaunchConfiguration"))
            currentLaunchConfiguration_ = *value;
    }

    for (EditorTab* tab : tabs_)
    {
        if (entry == tab->GetIniEntry())
            tab->ReadIniSettings(line);
    }
}

void ProjectEditor::WriteIniSettings(ImGuiTextBuffer& output)
{
    output.appendf("\n[Project][%s]\n", selfIniEntry.c_str());
    WriteStringToIni(output, "LaunchConfiguration", currentLaunchConfiguration_);

    for (EditorTab* tab : tabs_)
    {
        output.appendf("\n[Project][%s]\n", tab->GetIniEntry().c_str());
        tab->WriteIniSettings(output);
    }
}

void ProjectEditor::SetFocusedTab(EditorTab* tab)
{
    if (focusedTab_ != tab)
    {
        focusedTab_ = tab;
        if (tab)
            tab->OnFocused(tab);
    }

    EditorTab* ownerTab = tab && tab->GetOwnerTab() ? tab->GetOwnerTab() : nullptr;
    if (focusedRootTab_ != ownerTab)
    {
        focusedRootTab_ = ownerTab;
    }
}

}
