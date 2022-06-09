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

#include "../Core/UndoManager.h"
#include "../Project/EditorTab.h"
#include "../Project/ProjectEditor.h"

#include <EASTL/optional.h>
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

    /// Commands
    /// @{
    void SaveCurrentResource();
    void CloseCurrentResource();
    /// @}

    /// EditorTab implementation
    /// @{
    void EnumerateUnsavedItems(ea::vector<ea::string>& items) override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

    /// Return user-readable name of resource.
    virtual ea::string GetResourceTitle() { return "Resource"; }
    /// Return whether the specified request can be handled by this tab.
    virtual bool CanOpenResource(const OpenResourceRequest& request) = 0;
    /// Return whether the several resources can be handled simultaneously.
    virtual bool SupportMultipleResources() = 0;
    /// Open resource.
    void OpenResource(const ea::string& resourceName, bool activate = true);
    /// Close resource.
    void CloseResource(const ea::string& resourceName);
    /// Close all opened resources.
    void CloseAllResources();
    /// Close resource gracefully.
    void CloseResourceGracefully(const ea::string& resourceName, ea::function<void()> onClosed = []{});
    /// Close all opened resources gracefully.
    void CloseAllResourcesGracefully(ea::function<void()> onAllClosed = []{});
    /// Close all opened resources gracefully. Open other resource if requested.
    void CloseAllResourcesGracefully(const ea::string& pendingOpenResourceName);
    /// Save specific opened resource.
    void SaveResource(const ea::string& resourceName);
    /// Save all resources.
    void SaveAllResources();
    /// Set currently active resource.
    void SetActiveResource(const ea::string& activeResourceName);
    /// Set current action for resource.
    void SetCurrentAction(const ea::string& resourceName, ea::optional<EditorActionFrame> frame);

    /// Push undo action from currently active resource.
    virtual void PushAction(SharedPtr<EditorAction> action);
    template <class T, class ... Args> void PushAction(const Args& ... args);

    /// Return properties of the tab.
    /// @{
    bool IsResourceOpen(const ea::string& resourceName) const { return resources_.contains(resourceName); }
    bool IsResourceUnsaved(const ea::string& resourceName) const;
    bool IsAnyResourceUnsaved() const;
    const ea::string& GetActiveResourceName() const { return activeResourceName_; }
    /// @}

protected:
    /// EditorTab implementation
    /// @{
    bool IsMarkedUnsaved() override;
    void RenderContextMenuItems() override;
    /// @}

    /// Called when resource should be loaded.
    virtual void OnResourceLoaded(const ea::string& resourceName) = 0;
    /// Called when resource should be unloaded.
    virtual void OnResourceUnloaded(const ea::string& resourceName) = 0;
    /// Called when active resource changed.
    virtual void OnActiveResourceChanged(const ea::string& resourceName) = 0;
    /// Called when resource should be saved.
    virtual void OnResourceSaved(const ea::string& resourceName) = 0;

private:
    struct ResourceData
    {
        ea::optional<EditorActionFrame> currentActionFrame_;
        ea::optional<EditorActionFrame> savedActionFrame_;

        bool IsUnsaved() const { return currentActionFrame_ != savedActionFrame_; }
    };

    StringVector GetResourceNames() const;
    void OnProjectInitialized();
    void DoSaveResource(const ea::string& resourceName, ResourceData& data);

    bool loadResources_{};
    ea::map<ea::string, ResourceData> resources_;
    ea::string activeResourceName_;
};

/// Action wrapper that focuses resource on undo/redo.
class ResourceActionWrapper : public BaseEditorActionWrapper
{
public:
    ResourceActionWrapper(SharedPtr<EditorAction> action,
        ResourceEditorTab* tab, const ea::string& resourceName, ea::optional<EditorActionFrame> oldFrame);

    /// Implement EditorAction.
    /// @{
    bool IsAlive() const;
    void OnPushed(EditorActionFrame frame) override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void FocusMe() const;
    void UpdateCurrentAction(ea::optional<EditorActionFrame> frame) const;

    WeakPtr<ResourceEditorTab> tab_;
    ea::string resourceName_;

    ea::optional<EditorActionFrame> oldFrame_;
    EditorActionFrame newFrame_{};
};

template <class T, class ... Args>
void ResourceEditorTab::PushAction(const Args& ... args)
{
    PushAction(MakeShared<T>(args...));
}

}
