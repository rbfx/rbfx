// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Object.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Request to gracefully close resource(s) with prompt.
struct CloseResourceRequest
{
    ea::vector<ea::string> resourceNames_;
    ea::function<void()> onSave_ = []{};
    ea::function<void()> onDiscard_ = []{};
    ea::function<void()> onCancel_ = []{};
};

/// Wrapper for close dialog widget.
class CloseDialog : public Object
{
    URHO3D_OBJECT(CloseDialog, Object);

public:
    explicit CloseDialog(Context* context);
    ~CloseDialog() override;

    /// Set whether "Save & Close" option is enabled.
    void SetSaveEnabled(bool enabled) { saveEnabled_ = enabled; }

    /// Process close request.
    void RequestClose(CloseResourceRequest request);
    /// Return whether the dialog is currently open or will be open on this frame.
    bool IsActive() const;

    /// Update and render contents if necessary.
    void Render();

private:
    void CloseDialogSave();
    void CloseDialogDiscard();
    void CloseDialogCancel();

    bool saveEnabled_{true};
    ea::vector<CloseResourceRequest> requests_;

    bool isOpen_{};
    ea::string popupName_{"Close?"};

    ea::vector<ea::string> items_;
};

}
