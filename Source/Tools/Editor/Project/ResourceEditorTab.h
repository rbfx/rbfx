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

#include "../Project/EditorTab.h"
#include "../Project/ProjectEditor.h"

#include <EASTL/set.h>

namespace Urho3D
{

/// Base class for editor tab that represents engine resource.
class ResourceEditorTab : public EditorTab
{
    URHO3D_OBJECT(ResourceEditorTab, EditorTab);

public:
    ResourceEditorTab(Context* context, const ea::string& title, const ea::string& guid,
        EditorTabFlags flags, EditorTabPlacement placement);
    ~ResourceEditorTab() override;

    /// EditorTab implementation
    /// @{
    void WriteIniSettings(ImGuiTextBuffer* output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

    /// Return whether the specified request can be handled by this tab.
    virtual bool CanOpenResource(const OpenResourceRequest& request) = 0;
    /// Return whether the several resources can be handled simultaneously.
    virtual bool SupportMultipleResources() = 0;
    /// Open resource.
    void OpenResource(const ea::string& resourceName);
    /// Close resource.
    void CloseResource(const ea::string& resourceName);
    /// Close all opened resources.
    void CloseAllResources();

    /// Return properties of the tab.
    /// @{
    const ea::set<ea::string>& GetResourceNames() const { return resourceNames_; }
    const ea::string& GetActiveResourceName() const { return activeResourceName_; }
    /// @}

protected:
    /// EditorTab implementation
    /// @{
    void UpdateAndRenderContextMenuItems() override;
    /// @}

    /// Called when resource should be loaded.
    virtual void OnResourceLoaded(const ea::string& resourceName) = 0;
    /// Called when resource should be unloaded.
    virtual void OnResourceUnloaded(const ea::string& resourceName) = 0;
    /// Called when active resource changed.
    virtual void OnActiveResourceChanged(const ea::string& resourceName) = 0;

private:
    void OnProjectInitialized();
    void SetActiveResource(const ea::string& activeResourceName);

    bool loadResources_{};
    ea::set<ea::string> resourceNames_;
    ea::string activeResourceName_;
};

}
