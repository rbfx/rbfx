// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Project/CloseDialog.h"

#include <Urho3D/SystemUI/SystemUI.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/sort.h>

namespace Urho3D
{

CloseDialog::CloseDialog(Context* context)
    : Object(context)
{
}

CloseDialog::~CloseDialog()
{
}

void CloseDialog::RequestClose(CloseResourceRequest request)
{
    requests_.push_back(ea::move(request));
}

bool CloseDialog::IsActive() const
{
    return isOpen_ || !requests_.empty();
}

void CloseDialog::Render()
{
    if (!isOpen_ && !requests_.empty())
    {
        isOpen_ = true;

        items_.clear();
        for (const CloseResourceRequest& request : requests_)
            items_.append(request.resourceNames_);
        ea::sort(items_.begin(), items_.end());

        ui::OpenPopup(popupName_.c_str());
    }

    if (ui::BeginPopupModal(popupName_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ui::Text("The following items are unsaved:");

        if (ui::BeginChild("Items", ImVec2{0.0f, 100.0f}))
        {
            for (const ea::string& item : items_)
                ui::MenuItem(item.c_str());
        }
        ui::EndChild();

        ui::BeginDisabled(!saveEnabled_);
        if (ui::Button(ICON_FA_FLOPPY_DISK " Save & Close"))
        {
            URHO3D_ASSERT(saveEnabled_);

            CloseDialogSave();
            ui::CloseCurrentPopup();
        }
        ui::EndDisabled();

        ui::SameLine();

        if (ui::Button(ICON_FA_TRIANGLE_EXCLAMATION " Discard & Close"))
        {
            CloseDialogDiscard();
            ui::CloseCurrentPopup();
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
        {
            CloseDialogCancel();
            ui::CloseCurrentPopup();
        }

        ui::EndPopup();
    }
    else if (isOpen_)
    {
        CloseDialogDiscard();
    }
}

void CloseDialog::CloseDialogSave()
{
    for (const CloseResourceRequest& request : requests_)
        request.onSave_();
    requests_.clear();

    isOpen_ = false;
}

void CloseDialog::CloseDialogDiscard()
{
    for (const CloseResourceRequest& request : requests_)
        request.onDiscard_();
    requests_.clear();

    isOpen_ = false;
}

void CloseDialog::CloseDialogCancel()
{
    for (const CloseResourceRequest& request : requests_)
        request.onCancel_();
    requests_.clear();

    isOpen_ = false;
}

}
