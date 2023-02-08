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

#include "../Project/Project.h"

#include "../Core/EditorPluginManager.h"
#include "../Core/IniHelpers.h"
#include "../Core/SettingsManager.h"
#include "../Core/UndoManager.h"
#include "../Project/AssetManager.h"
#include "../Project/CreateDefaultScene.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VirtualFileSystem.h>
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

const auto Hotkey_SaveProject = EditorHotkey{"Global.SaveProject"}.Ctrl().Shift().Press(KEY_S);

ImFont* monoFont = nullptr;

unsigned numActiveProjects = 0;

const ea::string selfIniEntry{"Project"};

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

void CreateAssetPipeline(Context* context, const ea::string& fileName)
{
    JSONFile jsonFile(context);
    JSONValue& root = jsonFile.GetRoot();

    JSONValue modelTransformer;
    modelTransformer["_Class"] = "ModelImporter";
    root["Transformers"].Push(modelTransformer);

    jsonFile.SaveFile(fileName);
}

ea::pair<ea::string, ea::string> ParseCommand(const ea::string& command)
{
    ea::string commandName;
    ea::string commandArgs;
    const unsigned spacePos = command.find(' ');
    if (spacePos != ea::string::npos)
    {
        commandName = command.substr(0, spacePos).trimmed();
        commandArgs = command.substr(spacePos + 1).trimmed();
    }
    else
        commandName = command.trimmed();
    return {commandName, commandArgs};
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

bool AnalyzeFileContext::HasXMLRoot(ea::string_view root) const
{
    return xmlFile_ && Compare(xmlFile_->GetRoot().GetName(), root, false) == 0;
}

bool AnalyzeFileContext::HasXMLRoot(std::initializer_list<ea::string_view> roots) const
{
    return ea::any_of(roots.begin(), roots.end(),
        [this](ea::string_view root) { return HasXMLRoot(root); });
}

void Project::SetMonoFont(ImFont* font)
{
    monoFont = font;
}

ImFont* Project::GetMonoFont()
{
    return monoFont;
}

Project::Project(Context* context, const ea::string& projectPath, const ea::string& settingsJsonPath, bool isReadOnly)
    : Object(context)
    , isHeadless_(context->GetSubsystem<Engine>()->IsHeadless())
    , isReadOnly_(isReadOnly)
    , projectPath_(GetSanitizedPath(projectPath + "/"))
    , coreDataPath_(projectPath_ + "CoreData/")
    , cachePath_(projectPath_ + "Cache/")
    , tempPath_(projectPath_ + "Temp/")
    , projectJsonPath_(projectPath_ + "Project.json")
    , settingsJsonPath_(settingsJsonPath)
    , cacheJsonPath_(projectPath_ + "Cache.json")
    , uiIniPath_(projectPath_ + "ui.ini")
    , gitIgnorePath_(projectPath_ + ".gitignore")
    , previewPngPath_(projectPath_ + "Preview.png")
    , dataPath_(projectPath_ + "Data/")
    , oldCacheState_(context)
    , hotkeyManager_(MakeShared<HotkeyManager>(context_))
    , undoManager_(MakeShared<UndoManager>(context_))
    , settingsManager_(MakeShared<SettingsManager>(context_))
    , pluginManager_(MakeShared<PluginManager>(context_))
    , launchManager_(MakeShared<LaunchManager>(context_))
    , toolManager_(MakeShared<ToolManager>(context_))
    , closeDialog_(MakeShared<CloseDialog>(context_))
{
    auto initializationGuard = ea::make_shared<int>(0);
    initializationGuard_ = initializationGuard;

    URHO3D_ASSERT(numActiveProjects == 0);
    context_->RegisterSubsystem(this, Project::GetTypeStatic());
    ++numActiveProjects;

    context_->RemoveSubsystem<PluginManager>();
    context_->RegisterSubsystem(pluginManager_);

    if (!isHeadless_ && !isReadOnly_)
        ui::GetIO().IniFilename = uiIniPath_.c_str();

    InitializeHotkeys();
    EnsureDirectoryInitialized();
    InitializeResourceCache();

    // Delay asset manager creation until project is ready
    assetManager_ = MakeShared<AssetManager>(context);
    assetManager_->OnInitialized.Subscribe(this, [=](Project*) mutable { initializationGuard.reset(); });

    IgnoreFileNamePattern("*.user.json");

    ApplyPlugins();

    settingsManager_->AddPage(toolManager_);

    settingsManager_->LoadFile(settingsJsonPath_);
    assetManager_->LoadFile(cacheJsonPath_);

    JSONFile projectJsonFile(context_);
    projectJsonFile.LoadFile(projectJsonPath_);
    JSONInputArchive archive{&projectJsonFile};
    SerializeOptionalValue(archive, "Project", *this, AlwaysSerialize{});

    if (firstInitialization_)
        InitializeDefaultProject();
}

void Project::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "PluginManager", *pluginManager_, AlwaysSerialize{});
    SerializeOptionalValue(archive, "LaunchManager", *launchManager_, AlwaysSerialize{});
}

void Project::ExecuteCommand(const ea::string& command, bool exitOnCompletion)
{
    if (command.trimmed().empty())
    {
        URHO3D_LOGWARNING("Empty command is ignored");
        return;
    }

    if (initialized_)
        ProcessCommand(command, exitOnCompletion);
    else
        pendingCommands_.emplace_back(command, exitOnCompletion);
}

bool Project::ExecuteRemoteCommand(const ea::string& command, ea::string* output)
{
    auto fileSystem = context_->GetSubsystem<FileSystem>();
    auto engine = context_->GetSubsystem<Engine>();

    const StringVector arguments = {
        "--quiet",
        "--log",
        "ERROR",
        "--headless",
        "--exit",
        "--read-only",
        "--command",
        command,
        "--prefix-paths",
        engine->GetParameter(EP_RESOURCE_PREFIX_PATHS).GetString(),
        projectPath_,
    };

    ea::string tempOutput;
    ea::string& effectiveOutput = output ? *output : tempOutput;

    effectiveOutput.clear();
    const int exitCode = fileSystem->SystemRun(fileSystem->GetProgramFileName(), arguments, effectiveOutput);
    if (exitCode != 0)
    {
        URHO3D_LOGERROR("Failed to execute remote command \"{}\" with exit code {}: {}",
            command, exitCode, effectiveOutput);
        return false;
    }
    return true;
}

void Project::ExecuteRemoteCommandAsync(const ea::string& command, CommandExecutedCallback callback)
{
    PendingRemoteCommand remoteCommand;
    remoteCommand.callback_ = ea::move(callback);
    remoteCommand.result_ = std::async([=]()
    {
        ea::string output;
        const bool success = ExecuteRemoteCommand(command, &output);
        return ea::make_pair(success, output);
    });
    pendingRemoteCommands_.push_back(ea::move(remoteCommand));
}

void Project::Destroy()
{
    // Always save shallow data on close
    SaveShallowOnly();

    ProcessDelayedSaves(true);

    context_->RemoveSubsystem<PluginManager>();
    context_->RegisterSubsystem<PluginManager>();
}

Project::~Project()
{
    auto cache = GetSubsystem<ResourceCache>();
    cache->ReleaseAllResources(true);

    --numActiveProjects;
    URHO3D_ASSERT(numActiveProjects == 0);

    if (!isHeadless_)
        ui::GetIO().IniFilename = nullptr;
}

CloseProjectResult Project::CloseGracefully()
{
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
    const bool hasUnsavedCookedAssets = assetManager_->IsProcessing();

    ea::vector<ea::string> unsavedItems;
    if (hasUnsavedChanges_)
        unsavedItems.push_back("[Project]");
    if (hasUnsavedCookedAssets)
        unsavedItems.push_back("[Cooked Assets]");
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
    closeDialog_->SetSaveEnabled(!hasUnsavedCookedAssets);
    closeDialog_->RequestClose(ea::move(request));
    return CloseProjectResult::Undefined;
}

void Project::CloseResourceGracefully(const CloseResourceRequest& request)
{
    closeDialog_->RequestClose(request);
}

void Project::ProcessRequest(ProjectRequest* request, EditorTab* sender)
{
    pendingRequests_.emplace_back(PendingRequest{SharedPtr<ProjectRequest>{request}, WeakPtr<EditorTab>{sender}});
}

void Project::AddAnalyzeFileCallback(const AnalyzeFileCallback& callback)
{
    analyzeFileCallbacks_.push_back(callback);
}

ResourceFileDescriptor Project::GetResourceDescriptor(const ea::string& resourceName, const ea::string& fileName) const
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

void Project::SaveFileDelayed(const ea::string& fileName, const ea::string& resourceName, const SharedByteVector& bytes,
    const FileSavedCallback& onSaved)
{
    delayedFileSaves_[resourceName] = PendingFileSave{fileName, bytes, onSaved};
}

void Project::SaveFileDelayed(Resource* resource, const FileSavedCallback& onSaved)
{
    delayedFileSaves_[resource->GetName()] =
        PendingFileSave{resource->GetAbsoluteFileName(), nullptr, onSaved, SharedPtr<Resource>(resource)};
}

void Project::IgnoreFileNamePattern(const ea::string& pattern)
{
    const bool inserted = ignoredFileNames_.insert(pattern).second;
    if (inserted)
        ignoredFileNameRegexes_.push_back(PatternToRegex(pattern));
}

bool Project::IsFileNameIgnored(const ea::string& fileName) const
{
    for (const std::regex& r : ignoredFileNameRegexes_)
    {
        if (std::regex_match(fileName.begin(), fileName.end(), r))
            return true;
    }
    return false;
}

void Project::AddTab(SharedPtr<EditorTab> tab)
{
    if (isHeadless_)
        return;

    tabs_.push_back(tab);
    sortedTabs_[tab->GetTitle()] = tab;
}

ea::string Project::GetRandomTemporaryPath() const
{
    return Format("{}{}/", tempPath_, GenerateUUID());
}

TemporaryDir Project::CreateTemporaryDir()
{
    return TemporaryDir{context_, GetRandomTemporaryPath()};
}

void Project::InitializeHotkeys()
{
    hotkeyManager_->BindHotkey(this, Hotkey_SaveProject, &Project::Save);
}

void Project::EnsureDirectoryInitialized()
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
    if (fs->FileExists(dataPath_ + "AssetPipeline.json"))
        fs->Rename(dataPath_ + "AssetPipeline.json", dataPath_ + "Default.assetpipeline");

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

void Project::InitializeDefaultProject()
{
    pluginManager_->SetPluginsLoaded({SceneViewerApplication::GetStaticPluginName()});

    const ea::string configName = "View Current Scene";
    launchManager_->AddConfiguration(LaunchConfiguration{configName, SceneViewerApplication::GetStaticPluginName()});
    currentLaunchConfiguration_ = configName;

    const ea::string defaultSceneName = "Scenes/Default.scene";
    DefaultSceneParameters params;
    params.highQuality_ = true;
    params.createObjects_ = true;
    CreateDefaultScene(context_, dataPath_ + defaultSceneName, params);

    const auto request = MakeShared<OpenResourceRequest>(context_, defaultSceneName);
    ProcessRequest(request, nullptr);

    const ea::string defaultAssetPipeline = "Default.assetpipeline";
    CreateAssetPipeline(context_, dataPath_ + defaultAssetPipeline);

    Save();
}

void Project::InitializeResourceCache()
{
    const auto engine = GetSubsystem<Engine>();
    const auto cache = GetSubsystem<ResourceCache>();
    cache->ReleaseAllResources(true);
    cache->RemoveAllResourceDirs();
    cache->AddResourceDir(dataPath_);
    cache->AddResourceDir(coreDataPath_);
    cache->AddResourceDir(cachePath_);
    cache->AddResourceDir(oldCacheState_.GetEditorData());

    const auto vfs = GetSubsystem<VirtualFileSystem>();
    vfs->UnmountAll();
    vfs->MountDir(oldCacheState_.GetEditorData());
    vfs->MountDir(coreDataPath_);
    vfs->MountDir(dataPath_);
    vfs->MountDir(cachePath_);
    vfs->MountDir("conf" , engine->GetAppPreferencesDir());
}

void Project::ResetLayout()
{
    pendingResetLayout_ = false;

    ui::DockBuilderRemoveNode(dockspaceId_);
    ui::DockBuilderAddNode(dockspaceId_, 0);
    ui::DockBuilderSetNodeSize(dockspaceId_, ui::GetMainViewport()->Size);

    ImGuiID dockCenter = dockspaceId_;
    ImGuiID dockLeft = ui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, 0.20f, nullptr, &dockCenter);
    ImGuiID dockRight = ui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.30f, nullptr, &dockCenter);
    ImGuiID dockBottom = ui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.30f, nullptr, &dockCenter);

    for (EditorTab* tab : tabs_)
    {
        switch (tab->GetPlacement())
        {
        case EditorTabPlacement::DockCenter:
            ui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockCenter);
            break;
        case EditorTabPlacement::DockLeft:
            ui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockLeft);
            break;
        case EditorTabPlacement::DockRight:
            ui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockRight);
            break;
        case EditorTabPlacement::DockBottom:
            ui::DockBuilderDockWindow(tab->GetUniqueId().c_str(), dockBottom);
            break;
        }
    }
    ui::DockBuilderFinish(dockspaceId_);

    for (EditorTab* tab : tabs_)
    {
        if (tab->GetFlags().Test(EditorTabFlag::OpenByDefault))
            tab->Open();
    }
}

void Project::ApplyPlugins()
{
    auto editorPluginManager = GetSubsystem<EditorPluginManager>();
    editorPluginManager->Apply(this);

    for (EditorTab* tab : tabs_)
        tab->ApplyPlugins();
}

void Project::SaveGitIgnore()
{
    ea::string content;

    content += "# Ignore asset cache\n";
    content += "/Cache/\n";
    content += "/Cache.json\n";
    content += "\n";

    content += "# Ignore temporary files\n";
    content += "/Temp/\n";
    content += "\n";

    content += "# Ignore UI settings\n";
    content += "/ui.ini\n";
    content += "\n";

    content += "# Ignore preview screenshot\n";
    content += "/Preview.png\n";
    content += "\n";

    content += "# Ignore internal files\n";
    for (const ea::string& pattern : ignoredFileNames_)
        content += Format("{}\n", pattern);
    content += "\n";

    File file(context_, gitIgnorePath_, FILE_WRITE);
    if (file.IsOpen())
        file.Write(content.c_str(), content.size());
}

void Project::Render()
{
    const float tint = 0.15f;
    const ColorScopeGuard guardHighlightColors{{
        {ImGuiCol_Tab,                ImVec4(0.26f, 0.26f + tint, 0.26f, 0.40f)},
        {ImGuiCol_TabHovered,         ImVec4(0.31f, 0.31f + tint, 0.31f, 1.00f)},
        {ImGuiCol_TabActive,          ImVec4(0.28f, 0.28f + tint, 0.28f, 1.00f)},
        {ImGuiCol_TabUnfocused,       ImVec4(0.17f, 0.17f + tint, 0.17f, 1.00f)},
        {ImGuiCol_TabUnfocusedActive, ImVec4(0.26f, 0.26f + tint, 0.26f, 1.00f)},
    }, isHighlightEnabled_};

    if (!isHeadless_)
    {
        hotkeyManager_->Update();
        hotkeyManager_->InvokeFor(hotkeyManager_);
        if (areGlobalHotkeysEnabled_)
            hotkeyManager_->InvokeFor(this);

        dockspaceId_ = ui::GetID("Root");
        ui::DockSpace(dockspaceId_);

        if (pendingResetLayout_)
            ResetLayout();
    }

    if (!assetManagerInitialized_ && !pluginManager_->IsReloadPending())
    {
        assetManagerInitialized_ = true;
        assetManager_->Initialize(isReadOnly_);
    }

    assetManager_->Update();

    bool initialFocusPending = false;
    if (!initialized_ && initializationGuard_.expired())
    {
        initialized_ = true;
        initialFocusPending = true;

        OnInitialized(this);

        for (const auto& [command, exitOnCompletion] : pendingCommands_)
            ProcessCommand(command, exitOnCompletion);
        pendingCommands_.clear();
    }

    if (!isHeadless_)
    {
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
    }

    ProcessDelayedSaves();
    ProcessPendingRequests();
    ProcessPendingRemoteCommands();
}

void Project::ProcessPendingRequests()
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

void Project::ProcessDelayedSaves(bool forceSave)
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

        bool needReload = !fileExists;
        if (delayedSave.onSaved_)
            delayedSave.onSaved_(delayedSave.fileName_, resourceName, needReload);

        if (!needReload)
            cache->IgnoreResourceReload(resourceName);

        delayedSave.Clear();
    }

    ea::erase_if(delayedFileSaves_, [](const auto& pair) { return pair.second.IsEmpty(); });
}

void Project::ProcessCommand(const ea::string& command, bool exitOnCompletion)
{
    const auto [name, args] = ParseCommand(command);

    if (name != "Idle")
    {
        bool processed = false;
        OnCommand(this, name, args, processed);

        if (!processed)
            URHO3D_LOGWARNING("Cannot process command: {}", command);
    }

    if (exitOnCompletion)
    {
        closeProjectResult_ = CloseProjectResult::Closed;
        SendEvent(E_EXITREQUESTED);
    }
}

void Project::ProcessPendingRemoteCommands()
{
    for (PendingRemoteCommand& command : pendingRemoteCommands_)
    {
        if (command.result_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            const auto [success, output] = command.result_.get();
            command.callback_(success, output);
            command.callback_ = nullptr;
        }
    }

    const auto isDone = [](const PendingRemoteCommand& command) { return command.callback_ == nullptr; };
    ea::erase_if(pendingRemoteCommands_, isDone);
}

void Project::RenderToolbar()
{
    if (Widgets::ToolbarButton(ICON_FA_FLOPPY_DISK, "Save Project"))
        Save();
    OnRenderProjectToolbar(this);

    Widgets::ToolbarSeparator();

    if (focusedRootTab_)
        focusedRootTab_->RenderToolbar();

    RenderAssetsToolbar();
}

void Project::RenderAssetsToolbar()
{
    const auto [numAssetsCooked, numAssetsTotal] = assetManager_->GetProgress();

    if (numAssetsTotal == 0)
        return;

    Widgets::ToolbarSeparator();
    const float ratio = static_cast<float>(numAssetsCooked) / numAssetsTotal;
    const ea::string text = Format("Assets cooked {}/{}", numAssetsCooked, numAssetsTotal);

    // Show some small progress from the start for better visibility
    const float progress = Lerp(0.05f, 1.0f, ratio);
    ui::ProgressBar(progress, ImVec2{200.0f, 0.0f}, text.c_str());
}

void Project::RenderProjectMenu()
{
    if (ui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project", hotkeyManager_->GetHotkeyLabel(Hotkey_SaveProject).c_str()))
        Save();
    OnRenderProjectMenu(this);
}

void Project::RenderMainMenu()
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

void Project::SaveShallowOnly()
{
    if (isReadOnly_)
        return;

    ui::SaveIniSettingsToDisk(uiIniPath_.c_str());
    settingsManager_->SaveFile(settingsJsonPath_);
    assetManager_->SaveFile(cacheJsonPath_);

    for (EditorTab* tab : tabs_)
    {
        if (auto resourceTab = dynamic_cast<ResourceEditorTab*>(tab))
            resourceTab->SaveShallow();
    }

    OnShallowSaved(this);
}

void Project::SaveProjectOnly()
{
    JSONFile projectJsonFile(context_);
    JSONOutputArchive archive{&projectJsonFile};
    SerializeOptionalValue(archive, "Project", *this, AlwaysSerialize{});
    projectJsonFile.SaveFile(projectJsonPath_);

    // Save .gitignore file once so user can edit it
    auto fs = GetSubsystem<FileSystem>();
    if (!fs->FileExists(gitIgnorePath_))
        SaveGitIgnore();

    hasUnsavedChanges_ = false;
}

void Project::SaveResourcesOnly()
{
    for (EditorTab* tab : tabs_)
    {
        if (auto resourceTab = dynamic_cast<ResourceEditorTab*>(tab))
            resourceTab->SaveAllResources(true);
    }
    ProcessDelayedSaves(true);
}

void Project::Save()
{
    SaveProjectOnly();
    SaveShallowOnly();
    SaveResourcesOnly();
}

void Project::ReadIniSettings(const char* entry, const char* line)
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

void Project::WriteIniSettings(ImGuiTextBuffer& output)
{
    output.appendf("\n[Project][%s]\n", selfIniEntry.c_str());
    WriteStringToIni(output, "LaunchConfiguration", currentLaunchConfiguration_);

    for (EditorTab* tab : tabs_)
    {
        output.appendf("\n[Project][%s]\n", tab->GetIniEntry().c_str());
        tab->WriteIniSettings(output);
    }
}

void Project::SetFocusedTab(EditorTab* tab)
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
