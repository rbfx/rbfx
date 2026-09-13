// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/Project.h"

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
    void RenderContextMenuItems() override;

    void EnumerateUnsavedItems(ea::vector<ea::string>& items) override;
    ea::optional<EditorActionFrame> PushAction(SharedPtr<EditorAction> action) override;
    using EditorTab::PushAction;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

    /// Return user-readable name of resource.
    virtual ea::string GetResourceTitle() { return "Resource"; }
    /// Return whether the specified request can be handled by this tab.
    virtual bool CanOpenResource(const ResourceFileDescriptor& desc) = 0;
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
    bool CloseAllResourcesGracefully(ea::function<void()> onAllClosed = []{});
    /// Close all opened resources gracefully. Open other resource if requested.
    bool CloseAllResourcesGracefully(const ea::string& pendingOpenResourceName);
    /// Save specific opened resource.
    void SaveResource(const ea::string& resourceName, bool forced = false);
    /// Save all resources.
    void SaveAllResources(bool forced = false);
    /// Save all shallow data.
    void SaveShallow();
    /// Set currently active resource.
    void SetActiveResource(const ea::string& activeResourceName);
    /// Set current action for resource.
    void SetCurrentAction(const ea::string& resourceName, ea::optional<EditorActionFrame> frame);

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
    /// @}

    /// Called when resource should be loaded.
    virtual void OnResourceLoaded(const ea::string& resourceName) = 0;
    /// Called when resource should be unloaded.
    virtual void OnResourceUnloaded(const ea::string& resourceName) = 0;
    /// Called when active resource changed.
    virtual void OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName) = 0;
    /// Called when resource should be saved.
    virtual void OnResourceSaved(const ea::string& resourceName) = 0;
    /// Called when shallow data for resource is saved.
    virtual void OnResourceShallowSaved(const ea::string& resourceName) = 0;

private:
    struct ResourceData
    {
        ea::optional<EditorActionFrame> currentActionFrame_;
        ea::optional<EditorActionFrame> savedActionFrame_;

        bool IsUnsaved() const { return currentActionFrame_ != savedActionFrame_; }
    };

    StringVector GetResourceNames() const;
    void OnProjectInitialized();
    void OnProjectRequest(ProjectRequest* request);
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
    void OnPushed(EditorActionFrame frame) override;
    bool CanRedo() const override;
    void Redo() const override;
    bool CanUndo() const override;
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

}
