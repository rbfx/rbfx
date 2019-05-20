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
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Toolbox/SystemUI/Widgets.h>
#include "BaseResourceTab.h"

namespace Urho3D
{

BaseResourceTab::BaseResourceTab(Context* context)
    : Tab(context)
    , undo_(context)
{
    SubscribeToEvent(E_RESOURCERENAMED, [&](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;

        auto nameFrom = args[P_FROM].GetString();
        if (resourceName_ == nameFrom)
            SetResourceName(args[P_TO].GetString());
    });
}

bool BaseResourceTab::LoadResource(const ea::string& resourcePath)
{
    if (!Tab::LoadResource(resourcePath))
        return false;

    if (resourcePath.empty())
        return false;

    if (IsModified())
    {
        pendingLoadResource_ = resourcePath;
        return false;
    }

    SetResourceName(resourcePath);

    return true;
}

bool Urho3D::BaseResourceTab::SaveResource()
{
    if (!Tab::SaveResource())
        return false;

    if (resourceName_.empty())
        return false;

    lastUndoIndex_ = undo_.Index();

    return true;
}

void BaseResourceTab::OnSaveUISettings(ImGuiTextBuffer* buf)
{
    Tab::OnSaveUISettings(buf);
    if (!resourceName_.empty())
        buf->appendf("Path=%s\n", resourceName_.c_str());
}

void BaseResourceTab::OnLoadUISettings(const char* name, const char* line)
{
    Tab::OnLoadUISettings(name, line);
    if (strstr(line, "Path=") == line) // starts with
        LoadResource(line + 5);
}

void BaseResourceTab::SetResourceName(const ea::string& resourceName)
{
    resourceName_ = resourceName;
    if (!isUtility_)
        SetTitle(GetFileName(resourceName_));
}

bool BaseResourceTab::IsModified() const
{
    return lastUndoIndex_ != undo_.Index();
}

void BaseResourceTab::Close()
{
    undo_.Clear();
    lastUndoIndex_ = 0;
    GetCache()->ReleaseResource(GetResourceType(), GetResourceName(), true);
    resourceName_.clear();
}

void BaseResourceTab::OnBeforeEnd()
{
    Tab::OnBeforeEnd();

    if (wasOpen_ && !ui::IsPopupOpen("Save?"))
    {
        if ((!open_ && IsModified()) || !pendingLoadResource_.empty())
        {
            ui::OpenPopup("Save?");
            open_ = true;
        }
    }

    bool noCancel = true;
    if (ui::BeginPopupModal("Save?", &noCancel, ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_Popup))
    {
        // Warn when closing a tab that was modified.
        if (!pendingLoadResource_.empty())
        {
            ui::Text(
                "Resource '%s' was modified. Would you like to save it before opening '%s'?",
                GetFileNameAndExtension(resourceName_).c_str(),
                GetFileNameAndExtension(pendingLoadResource_).c_str());

            if (ui::Button(ICON_FA_SAVE " Save & Open"))
            {
                SaveResource();
                LoadResource(pendingLoadResource_);
                pendingLoadResource_.clear();
                ui::CloseCurrentPopup();
            }
        }
        else
        {
            ui::Text("Resource '%s' was modified. Would you like to save it before closing?",
                GetFileNameAndExtension(resourceName_).c_str());

            bool save = ui::Button(ICON_FA_SAVE " Save & Close");
            ui::SameLine();
            bool close = ui::Button(ICON_FA_EXCLAMATION_TRIANGLE " Close without saving");
            ui::SetHelpTooltip("Can not be undone!", KEY_UNKNOWN);

            if (save)
                SaveResource();

            if (save || close)
            {
                open_ = false;
                ui::CloseCurrentPopup();
            }
        }
        ui::SameLine();
        if (ui::Button(ICON_FA_TIMES " Cancel"))
        {
            pendingLoadResource_.clear();
            ui::CloseCurrentPopup();
        }
        ui::EndPopup();
    }
    else if (!pendingLoadResource_.empty())
    {
        // Click outside of popup.
        pendingLoadResource_.clear();
    }

    if (wasOpen_ && !open_)
        Close();
}

}
