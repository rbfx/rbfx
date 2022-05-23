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

namespace Urho3D
{

ResourceEditorTab::ResourceEditorTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : EditorTab(context, title, guid, flags, placement)
{
    // TODO(editor): Consider doing it at first tab open after loading
    auto project = GetProject();
    project->OnInitialized.Subscribe(this, &ResourceEditorTab::OnProjectInitialized);
}

ResourceEditorTab::~ResourceEditorTab()
{
}

void ResourceEditorTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);

    const StringVector resourceNamesVector{resourceNames_.begin(), resourceNames_.end()};
    WriteStringToIni(output, "ResourceNames", ea::string::joined(resourceNamesVector, "|"));
    WriteStringToIni(output, "ActiveResourceName", activeResourceName_);
}

void ResourceEditorTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);

    if (const auto value = ReadStringFromIni(line, "ResourceNames"))
    {
        const StringVector resourceNamesVector = value->split('|');
        for (const ea::string& resourceName : resourceNamesVector)
            OpenResource(resourceName);
    }

    if (const auto value = ReadStringFromIni(line, "ActiveResourceName"))
        SetActiveResource(*value);
}

void ResourceEditorTab::OpenResource(const ea::string& resourceName, bool activate)
{
    if (!resourceNames_.contains(resourceName))
    {
        if (!SupportMultipleResources())
            CloseAllResources();

        resourceNames_.insert(resourceName);
        if (loadResources_)
            OnResourceLoaded(resourceName);
    }

    if (activate || activeResourceName_.empty())
        SetActiveResource(resourceName);
}

void ResourceEditorTab::CloseResource(const ea::string& resourceName)
{
    if (resourceNames_.contains(resourceName))
    {
        auto undoManager = GetProject()->GetUndoManager();
        const bool isActive = resourceName == activeResourceName_;
        undoManager->PushAction(MakeShared<CloseResourceAction>(this, resourceName, isActive));

        resourceNames_.erase(resourceName);
        if (loadResources_)
            OnResourceUnloaded(resourceName);

        if (!resourceNames_.contains(activeResourceName_))
        {
            const auto iter = resourceNames_.lower_bound(activeResourceName_);
            if (iter != resourceNames_.end())
                SetActiveResource(*iter);
            else if (!resourceNames_.empty())
                SetActiveResource(*resourceNames_.begin());
            else
                SetActiveResource("");
        }
    }
}

void ResourceEditorTab::OnProjectInitialized()
{
    loadResources_ = true;
    for (const ea::string& resourceName : resourceNames_)
        OnResourceLoaded(resourceName);
}

void ResourceEditorTab::SetActiveResource(const ea::string& activeResourceName)
{
    if (activeResourceName_ != activeResourceName)
    {
        if (resourceNames_.contains(activeResourceName))
        {
            activeResourceName_ = activeResourceName;
            OnActiveResourceChanged(activeResourceName_);
        }
        else
        {
            activeResourceName_ = "";
        }
    }
}

void ResourceEditorTab::CloseAllResources()
{
    auto undoManager = GetProject()->GetUndoManager();
    if (loadResources_)
    {
        for (const ea::string& resourceName : resourceNames_)
        {
            const bool isActive = resourceName == activeResourceName_;
            undoManager->PushAction(MakeShared<CloseResourceAction>(this, resourceName, isActive));
            OnResourceUnloaded(resourceName);
        }
    }
    resourceNames_.clear();
    activeResourceName_ = "";
}

void ResourceEditorTab::SaveResource(const ea::string& resourceName)
{
    if (resourceNames_.contains(resourceName))
    {
        OnResourceSaved(resourceName);
    }
}

void ResourceEditorTab::SaveAllResources()
{
    for (const ea::string& resourceName : resourceNames_)
    {
        OnResourceSaved(resourceName);
    }
}

void ResourceEditorTab::UpdateAndRenderContextMenuItems()
{
    ea::string closeResourcePending;
    bool closeAllResourcesPending = false;

    ResetSeparator();
    if (resourceNames_.empty())
    {
        ui::MenuItem("(No Resources)", nullptr, false, false);
        SetSeparator();
    }
    else
    {
        ui::PushID("ActiveResources");
        for (const ea::string& resourceName : resourceNames_)
        {
            bool selected = resourceName == activeResourceName_;
            if (ui::SmallButton(ICON_FA_XMARK))
                closeResourcePending = resourceName;
            ui::SameLine();
            if (ui::MenuItem(resourceName.c_str(), nullptr, &selected))
                SetActiveResource(resourceName);
        }
        ui::PopID();
        SetSeparator();
    }

    ResetSeparator();
    {
        const ea::string title = Format("Close Current [{}]", GetResourceTitle());
        if (ui::MenuItem(title.c_str(), nullptr, false, !resourceNames_.empty()))
            closeResourcePending = activeResourceName_;
    }
    {
        const ea::string title = Format("Close All [{}]s", GetResourceTitle());
        if (ui::MenuItem(title.c_str(), nullptr, false, !resourceNames_.empty()))
            closeAllResourcesPending = true;
    }

    SetSeparator();

    if (closeAllResourcesPending)
        CloseAllResources();
    else if (!closeResourcePending.empty())
        CloseResource(closeResourcePending);
}

void ResourceEditorTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    BaseClassName::ApplyHotkeys(hotkeyManager);
}

CloseResourceAction::CloseResourceAction(ResourceEditorTab* tab, const ea::string& resourceName, bool activate)
    : tab_(tab)
    , resourceName_(resourceName)
    , activate_(activate)
{
}

bool CloseResourceAction::Undo() const
{
    if (!tab_)
        return false;

    tab_->OpenResource(resourceName_, activate_);
    return true;
}

}
