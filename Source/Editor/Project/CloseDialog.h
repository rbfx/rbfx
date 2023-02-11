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
