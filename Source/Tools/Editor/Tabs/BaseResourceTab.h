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
#pragma once

#include <Toolbox/Common/UndoManager.h>
#include "Tabs/Tab.h"

namespace Urho3D
{

class BaseResourceTab
    : public Tab
{
    URHO3D_OBJECT(BaseResourceTab, Tab);
public:
    /// Construct.
    explicit BaseResourceTab(Context* context);
    /// Returns type of resource that this tab can handle.
    virtual StringHash GetResourceType() = 0;
    /// Load resource from cache.
    bool LoadResource(const ea::string& resourcePath) override;
    /// Save resource o disk.
    bool SaveResource() override;
    /// Save ui settings.
    void OnSaveUISettings(ImGuiTextBuffer* buf) override;
    /// Load ui settings.
    void OnLoadUISettings(const char* name, const char* line) override;
    /// Returns name of opened resource.
    ea::string GetResourceName() const { return resourceName_; }
    /// Returns true when loaded resource was modified.
    bool IsModified() const override;
    /// Closes current tab and unloads it's contents from memory.
    void Close() override;
    ///
    void OnBeforeEnd() override;

protected:
    /// Set resource name.
    void SetResourceName(const ea::string& resourceName);

    /// Name of loaded resource.
    ea::string resourceName_;
    /// Comparing undo stack size allows determining if open resource was modified during last frame.
    int lastUndoIndex_ = 0;
    /// State change tracker.
    Undo::Manager undo_;
    /// Resource that user would like to open on top of current loaded resource. Used for displaying warning.
    ea::string pendingLoadResource_;
};

}
