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

#include "../Core/IniHelpers.h"
#include "../Core/HotkeyManager.h"
#include "../Project/ResourceEditorTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/finally.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_SaveDocument, "Global.SaveDocument", QUAL_CTRL, KEY_S);
URHO3D_EDITOR_HOTKEY(Hotkey_CloseDocument, "Global.CloseDocument", QUAL_CTRL, KEY_W);

}

ResourceEditorTab::ResourceEditorTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : EditorTab(context, title, guid, flags, placement)
{
    // TODO(editor): Consider doing it at first tab open after loading
    auto project = GetProject();
    project->OnInitialized.Subscribe(this, &ResourceEditorTab::OnProjectInitialized);

    HotkeyManager* hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_SaveDocument, &ResourceEditorTab::SaveCurrentResource);
    hotkeyManager->BindHotkey(this, Hotkey_CloseDocument, &ResourceEditorTab::CloseCurrentResource);
}

ResourceEditorTab::~ResourceEditorTab()
{
}

void ResourceEditorTab::SaveCurrentResource()
{
    SaveResource(activeResourceName_);
}

void ResourceEditorTab::CloseCurrentResource()
{
    CloseResourceGracefully(activeResourceName_);
}

void ResourceEditorTab::EnumerateUnsavedItems(ea::vector<ea::string>& items)
{
    for (const auto& [resourceName, data] : resources_)
    {
        if (data.IsUnsaved())
            items.push_back(resourceName);
    }
}

void ResourceEditorTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);

    const StringVector resourceNames = GetResourceNames();
    WriteStringToIni(output, "ResourceNames", ea::string::joined(resourceNames, "|"));
    WriteStringToIni(output, "ActiveResourceName", activeResourceName_);
}

void ResourceEditorTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);

    if (const auto value = ReadStringFromIni(line, "ResourceNames"))
    {
        const StringVector resourceNames = value->split('|');
        for (const ea::string& resourceName : resourceNames)
            OpenResource(resourceName);
    }

    if (const auto value = ReadStringFromIni(line, "ActiveResourceName"))
        SetActiveResource(*value);
}

void ResourceEditorTab::OpenResource(const ea::string& resourceName, bool activate)
{
    if (!resources_.contains(resourceName))
    {
        if (!SupportMultipleResources())
        {
            CloseAllResourcesGracefully(resourceName);
            return;
        }

        resources_.emplace(resourceName, ResourceData{});
        if (loadResources_)
            OnResourceLoaded(resourceName);
    }

    if (activate || activeResourceName_.empty())
        SetActiveResource(resourceName);
}

void ResourceEditorTab::CloseResource(const ea::string& resourceName)
{
    if (resources_.contains(resourceName))
    {
        auto undoManager = GetProject()->GetUndoManager();
        const bool isActive = resourceName == activeResourceName_;

        resources_.erase(resourceName);
        if (loadResources_)
            OnResourceUnloaded(resourceName);

        if (!resources_.contains(activeResourceName_))
        {
            const auto iter = resources_.lower_bound(activeResourceName_);
            if (iter != resources_.end())
                SetActiveResource(iter->first);
            else if (!resources_.empty())
                SetActiveResource(resources_.begin()->first);
            else
                SetActiveResource("");
        }
    }
}

StringVector ResourceEditorTab::GetResourceNames() const
{
    StringVector resourceNames;
    for (const auto& [resourceName, data] : resources_)
        resourceNames.push_back(resourceName);
    return resourceNames;
}

void ResourceEditorTab::OnProjectInitialized()
{
    loadResources_ = true;
    for (const auto& [resourceName, data] : resources_)
        OnResourceLoaded(resourceName);
}

void ResourceEditorTab::SetActiveResource(const ea::string& activeResourceName)
{
    if (activeResourceName_ != activeResourceName)
    {
        if (resources_.contains(activeResourceName))
        {
            OnActiveResourceChanged(activeResourceName);
            activeResourceName_ = activeResourceName;
        }
        else
        {
            activeResourceName_ = "";
        }
    }
}

void ResourceEditorTab::SetCurrentAction(const ea::string& resourceName, ea::optional<EditorActionFrame> frame)
{
    const auto iter = resources_.find(resourceName);
    if (iter != resources_.end())
    {
        iter->second.currentActionFrame_ = frame;
    }
}

void ResourceEditorTab::CloseAllResources()
{
    if (loadResources_)
    {
        for (const auto& [resourceName, data] : resources_)
        {
            const bool isActive = resourceName == activeResourceName_;
            OnResourceUnloaded(resourceName);
        }
    }
    resources_.clear();
    activeResourceName_ = "";
}

void ResourceEditorTab::CloseResourceGracefully(const ea::string& resourceName, ea::function<void()> onClosed)
{
    if (!IsResourceUnsaved(resourceName))
    {
        CloseResource(resourceName);
        onClosed();
        return;
    }

    const WeakPtr<ResourceEditorTab> weakSelf(this);

    CloseResourceRequest request;
    request.resourceNames_ = {resourceName};
    request.onSave_ = [=]()
    {
        if (weakSelf)
        {
            weakSelf->SaveResource(resourceName);
            weakSelf->CloseResource(resourceName);
            onClosed();
        }
    };
    request.onDiscard_ = [=]()
    {
        if (weakSelf)
        {
            weakSelf->CloseResource(resourceName);
            onClosed();
        }
    };

    auto project = GetProject();
    project->CloseResourceGracefully(request);
}

void ResourceEditorTab::CloseAllResourcesGracefully(ea::function<void()> onAllClosed)
{
    if (!IsAnyResourceUnsaved())
    {
        CloseAllResources();
        return;
    }

    auto delayedCallback = ea::make_finally([onAllClosed]() { onAllClosed(); });
    auto releaseWhenClosed = ea::make_shared<decltype(delayedCallback)>(ea::move(delayedCallback));

    const StringVector resourceNames = GetResourceNames();
    for (const ea::string& resourceName : resourceNames)
    {
        CloseResourceGracefully(resourceName, [releaseWhenClosed]() mutable
        {
            releaseWhenClosed.reset();
        });
    }
}

void ResourceEditorTab::CloseAllResourcesGracefully(const ea::string& pendingOpenResourceName)
{
    CloseAllResourcesGracefully([=]()
    {
        if (!pendingOpenResourceName.empty())
            OpenResource(pendingOpenResourceName);
    });
}

void ResourceEditorTab::SaveResource(const ea::string& resourceName)
{
    const auto iter = resources_.find(resourceName);
    if (iter != resources_.end())
    {
        DoSaveResource(iter->first, iter->second);
    }
}

void ResourceEditorTab::SaveAllResources()
{
    for (auto& [resourceName, data] : resources_)
    {
        DoSaveResource(resourceName, data);
    }
}

void ResourceEditorTab::DoSaveResource(const ea::string& resourceName, ResourceData& data)
{
    OnResourceSaved(resourceName);
    data.savedActionFrame_ = data.currentActionFrame_;
}

void ResourceEditorTab::PushAction(SharedPtr<EditorAction> action)
{
    const auto iter = resources_.find(activeResourceName_);
    if (iter != resources_.end())
    {
        ResourceData& data = iter->second;
        auto project = GetProject();
        UndoManager* undoManager = project->GetUndoManager();

        const auto oldActionFrame = data.currentActionFrame_;
        const auto wrappedAction = MakeShared<ResourceActionWrapper>(action, this, activeResourceName_, oldActionFrame);
        data.currentActionFrame_ = undoManager->PushAction(wrappedAction);
    }
}

bool ResourceEditorTab::IsResourceUnsaved(const ea::string& resourceName) const
{
    const auto iter = resources_.find(resourceName);
    return iter != resources_.end() && iter->second.IsUnsaved();
}

bool ResourceEditorTab::IsAnyResourceUnsaved() const
{
    for (const auto& [resourceName, data] : resources_)
    {
        if (data.IsUnsaved())
            return true;
    }
    return false;
}

bool ResourceEditorTab::IsMarkedUnsaved()
{
    return IsResourceUnsaved(activeResourceName_);
}

void ResourceEditorTab::RenderContextMenuItems()
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    ea::string closeResourcePending;
    bool closeAllResourcesPending = false;
    ea::string saveResourcePending;
    bool saveAllResourcesPending = false;

    contextMenuSeparator_.Reset();
    if (resources_.empty())
    {
        ui::MenuItem("(No Resources)", nullptr, false, false);
        contextMenuSeparator_.Add();
    }
    else
    {
        ui::PushID("ActiveResources");
        for (const auto& [resourceName, data] : resources_)
        {
            ui::PushID(resourceName.c_str());

            bool selected = resourceName == activeResourceName_;
            if (ui::SmallButton(ICON_FA_XMARK))
                closeResourcePending = resourceName;
            ui::SameLine();

            ea::string title;
            if (data.IsUnsaved())
                title += "* ";
            title += resourceName;

            if (ui::MenuItem(title.c_str(), nullptr, &selected))
                SetActiveResource(resourceName);
            ui::PopID();
        }
        ui::PopID();
        contextMenuSeparator_.Add();
    }

    contextMenuSeparator_.Reset();
    {
        const ea::string title = Format("Save Current [{}]", GetResourceTitle());
        const ea::string hotkey = hotkeyManager->GetHotkeyLabel(Hotkey_SaveDocument);
        if (ui::MenuItem(title.c_str(), hotkey.c_str(), false, !resources_.empty()))
            saveResourcePending = activeResourceName_;
    }
    {
        const ea::string title = Format("Save All [{}]s", GetResourceTitle());
        if (ui::MenuItem(title.c_str(), nullptr, false, !resources_.empty()))
            saveAllResourcesPending = true;
    }

    contextMenuSeparator_.Add();

    contextMenuSeparator_.Reset();
    {
        const ea::string title = Format("Close Current [{}]", GetResourceTitle());
        const ea::string hotkey = hotkeyManager->GetHotkeyLabel(Hotkey_CloseDocument);
        if (ui::MenuItem(title.c_str(), hotkey.c_str(), false, !resources_.empty()))
            closeResourcePending = activeResourceName_;
    }
    {
        const ea::string title = Format("Close All [{}]s", GetResourceTitle());
        if (ui::MenuItem(title.c_str(), nullptr, false, !resources_.empty()))
            closeAllResourcesPending = true;
    }

    contextMenuSeparator_.Add();

    // Apply delayed actions
    if (closeAllResourcesPending)
        CloseAllResourcesGracefully();
    else if (!closeResourcePending.empty())
        CloseResourceGracefully(closeResourcePending);
    else if (saveAllResourcesPending)
        SaveAllResources();
    else if (!saveResourcePending.empty())
        SaveResource(saveResourcePending);
}

ResourceActionWrapper::ResourceActionWrapper(SharedPtr<EditorAction> action,
    ResourceEditorTab* tab, const ea::string& resourceName, ea::optional<EditorActionFrame> oldFrame)
    : BaseEditorActionWrapper(action)
    , tab_(tab)
    , resourceName_(resourceName)
    , oldFrame_(oldFrame)
{
    URHO3D_ASSERT(action);
}

bool ResourceActionWrapper::IsAlive() const
{
    return tab_ && tab_->IsResourceOpen(resourceName_) && BaseEditorActionWrapper::IsAlive();
}

void ResourceActionWrapper::OnPushed(EditorActionFrame frame)
{
    newFrame_ = frame;
    BaseEditorActionWrapper::OnPushed(frame);
}

void ResourceActionWrapper::Redo() const
{
    BaseEditorActionWrapper::Redo();
    FocusMe();
    UpdateCurrentAction(newFrame_);
}

void ResourceActionWrapper::Undo() const
{
    BaseEditorActionWrapper::Undo();
    FocusMe();
    UpdateCurrentAction(oldFrame_);
}

bool ResourceActionWrapper::MergeWith(const EditorAction& other)
{
    const auto otherWrapper = dynamic_cast<const ResourceActionWrapper*>(&other);
    if (!otherWrapper)
        return false;

    if (tab_ != otherWrapper->tab_ || resourceName_ != otherWrapper->resourceName_)
        return false;

    if (action_->MergeWith(*otherWrapper->action_))
    {
        newFrame_ = otherWrapper->newFrame_;
        return true;
    }
    return false;
}

void ResourceActionWrapper::FocusMe() const
{
    if (tab_)
    {
        tab_->Focus();
        tab_->SetActiveResource(resourceName_);
    }
}

void ResourceActionWrapper::UpdateCurrentAction(ea::optional<EditorActionFrame> frame) const
{
    if (tab_)
    {
        tab_->Focus();
        tab_->SetCurrentAction(resourceName_, frame);
    }
}

}
