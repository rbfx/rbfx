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

#include "../Core/CommonEditorActions.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneResource.h>
#include <Urho3D/Utility/SceneRendererToTexture.h>
#include <Urho3D/Utility/SceneSelection.h>
#include <Urho3D/Utility/PackedSceneData.h>

#include <EASTL/any.h>
#include <EASTL/vector_multiset.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, Project* project);

class SceneViewAddon;
class SceneViewTab;
class SimulateSceneAction;

/// Declare Editor-only type to avoid interference with user code.
class SceneResourceForEditor : public SceneResource
{
    URHO3D_OBJECT(SceneResourceForEditor, SceneResource);

public:
    using SceneResource::SceneResource;
};

/// Single page of SceneViewTab.
class SceneViewPage : public Object
{
    URHO3D_OBJECT(SceneViewPage, Object);

public:
    explicit SceneViewPage(SceneResource* resource);
    ~SceneViewPage() override;

    ea::any& GetAddonData(const SceneViewAddon& addon);

public:
    const SharedPtr<SceneResource> resource_;
    const SharedPtr<Scene> scene_;
    const SharedPtr<SceneRendererToTexture> renderer_;
    const ea::string cfgFileName_;

    ea::unordered_map<ea::string, ea::pair<WeakPtr<const SceneViewAddon>, ea::any>> addonData_;

    SceneSelection selection_;
    PackedSceneSelection oldSelection_;
    PackedSceneSelection newSelection_;

    bool ignoreNextReload_{};
    ea::optional<PackedSceneSelection> loadingSelection_;

    SharedPtr<SimulateSceneAction> currentSimulationAction_;

    Ray cameraRay_;

    PackedSceneData archivedScene_;
    PackedSceneSelection archivedSelection_;

    /// UI state
    /// @{
    Rect contentArea_;
    /// @}

    bool IsSimulationActive() const;
    void StartSimulation(SceneViewTab* owner);
    void BeginSelection();
    void EndSelection(SceneViewTab* owner);
};

/// Interface of SceneViewTab addon.
class SceneViewAddon : public Object
{
    URHO3D_OBJECT(SceneViewAddon, Object);

public:
    explicit SceneViewAddon(SceneViewTab* owner);
    ~SceneViewAddon() override;

    /// Return unique name of the addon for serialization.
    virtual ea::string GetUniqueName() const = 0;
    /// Return input priority.
    virtual int GetInputPriority() const { return 0; }
    /// Return priority in the toolbar.
    virtual int GetToolbarPriority() const { return 0; }

    /// Initialize addon for the given page.
    virtual void Initialize(SceneViewPage& page) {}
    /// Process input.
    virtual void ProcessInput(SceneViewPage& page, bool& mouseConsumed) {}
    /// Update and render addon.
    virtual void Render(SceneViewPage& page) {}
    /// Apply hotkeys for given addon.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager);
    /// Render context menu of the tab.
    virtual bool RenderTabContextMenu() { return false; }
    /// Render main toolbar.
    virtual bool RenderToolbar() { return false; }

    /// Serialize per-scene page state of the addon.
    virtual void SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const;

    /// Check if this type of drag&drop payload is accepted.
    virtual bool IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const { return false; }
    /// Begin drag&drop operation, render preview.
    virtual void BeginDragDrop(SceneViewPage& page, DragDropPayload* payload) {}
    /// Update drag&drop state, called continuously while dragging.
    virtual void UpdateDragDrop(DragDropPayload* payload) {}
    /// End drag&drop operation and commit result.
    virtual void CompleteDragDrop(DragDropPayload* payload) {}
    /// End drag&drop operation and discard result.
    virtual void CancelDragDrop() {}

    /// Write INI settings to file. Use as few lines as possible.
    virtual void WriteIniSettings(ImGuiTextBuffer& output) {}
    /// Read INI settings from file. Use as few lines as possible.
    virtual void ReadIniSettings(const char* line) {}

protected:
    WeakPtr<SceneViewTab> owner_;
};

/// Tab that renders Scene and enables Scene manipulation.
class SceneViewTab : public ResourceEditorTab
{
    URHO3D_OBJECT(SceneViewTab, ResourceEditorTab);

public:
    Signal<void(SceneViewPage& page, const Vector3& position)> OnLookAt;
    Signal<void(SceneViewPage& page, Scene* scene, SceneSelection& selection)> OnSelectionEditMenu;

    struct ByInputPriority
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    struct ByToolbarPriority
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    struct ByName
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    using AddonSetByInputPriority = ea::vector_multiset<SharedPtr<SceneViewAddon>, ByInputPriority>;
    using AddonSetByToolbarPriority = ea::vector_multiset<SharedPtr<SceneViewAddon>, ByToolbarPriority>;
    using AddonSetByName = ea::vector_multiset<SharedPtr<SceneViewAddon>, ByName>;

    explicit SceneViewTab(Context* context);
    ~SceneViewTab() override;

    /// Register new scene addon.
    void RegisterAddon(const SharedPtr<SceneViewAddon>& addon);
    template <class T, class ... Args> SceneViewAddon* RegisterAddon(const Args&... args);
    template <class T> T* GetAddon();

    /// Setup context for plugin application execution.
    void SetupPluginContext();
    /// Draw Edit menu for selection in the scene.
    void RenderEditMenu(Scene* scene, SceneSelection& selection);
    /// Draw Create menu for selection in the scene.
    void RenderCreateMenu(Scene* scene, SceneSelection& selection);
    /// Set whether component selection is supported.
    void SetComponentSelection(bool enabled) { componentSelection_ = enabled; }

    /// Commands
    /// @{
    void ResumeSimulation();
    void PauseSimulation();
    void ToggleSimulationPaused();
    void RewindSimulation();

    void CutSelection(SceneSelection& selection);
    void CopySelection(SceneSelection& selection);
    void PasteNextToSelection(Scene* scene, SceneSelection& selection);
    void PasteIntoSelection(Scene* scene, SceneSelection& selection);
    void DeleteSelection(SceneSelection& selection);
    void DuplicateSelection(SceneSelection& selection);
    void CreateNodeNextToSelection(Scene* scene, SceneSelection& selection);
    void CreateNodeInSelection(Scene* scene, SceneSelection& selection);
    void CreateComponentInSelection(Scene* scene, SceneSelection& selection, StringHash componentType);
    void FocusSelection(SceneSelection& selection);
    void MoveSelectionToLatest(SceneSelection& selection);
    void MoveSelectionPositionToLatest(SceneSelection& selection);
    void MoveSelectionRotationToLatest(SceneSelection& selection);
    void MoveSelectionScaleToLatest(SceneSelection& selection);
    void MakePersistent(SceneSelection& selection);

    void CutSelection();
    void CopySelection();
    void PasteNextToSelection();
    void PasteIntoSelection();
    void DeleteSelection();
    void DuplicateSelection();
    void CreateNodeNextToSelection();
    void CreateNodeInSelection();
    void FocusSelection();
    void MoveSelectionToLatest();
    void MoveSelectionPositionToLatest();
    void MoveSelectionRotationToLatest();
    void MoveSelectionScaleToLatest();
    void MakePersistent();
    /// @}

    /// ResourceEditorTab implementation
    /// @{
    void PreRenderUpdate() override;
    void PostRenderUpdate() override;

    void RenderMenu() override;
    void RenderToolbar() override;
    bool IsUndoSupported() { return true; }
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;

    void RenderContextMenuItems() override;
    void RenderContent() override;

    ea::string GetResourceTitle() { return "Scene"; }
    bool SupportMultipleResources() { return true; }
    bool CanOpenResource(const ResourceFileDescriptor& desc) override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;

    ea::optional<EditorActionFrame> PushAction(SharedPtr<EditorAction> action) override;
    using ResourceEditorTab::PushAction;
    /// @}

    /// Return current state.
    /// @{
    const AddonSetByName& GetAddonsByName() const { return addonsByName_; }
    SceneViewPage* GetPage(const ea::string& resourceName);
    SceneViewPage* GetPage(Scene* scene);
    SceneViewPage* GetActivePage();
    /// @}

protected:
    /// ResourceEditorTab implementation
    /// @{
    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;
    void OnResourceShallowSaved(const ea::string& resourceName) override;
    /// @}

private:
    /// Manage pages
    /// @{
    SharedPtr<SceneViewPage> CreatePage(SceneResource* sceneResource, bool isActive);
    void SavePageScene(SceneViewPage& page) const;

    void SavePageConfig(const SceneViewPage& page) const;
    void LoadPageConfig(SceneViewPage& page) const;

    void SavePagePreview(SceneViewPage& page) const;
    /// @}

    void UpdateAddons(SceneViewPage& page);
    void UpdateCameraRay();
    bool UpdateDropToScene();
    void InspectSelection(SceneViewPage& page);

    void BeginPluginReload();
    void EndPluginReload();

    ea::vector<SharedPtr<SceneViewAddon>> addons_;
    AddonSetByInputPriority addonsByInputPriority_;
    AddonSetByToolbarPriority addonsByToolbarPriority_;
    AddonSetByName addonsByName_;

    SharedPtr<SceneViewAddon> dragAndDropAddon_;

    ea::unordered_map<ea::string, SharedPtr<SceneViewPage>> scenes_;
    PackedNodeComponentData clipboard_;

    bool componentSelection_{true};
};

/// Action for scene simulation interval.
class SimulateSceneAction : public EditorAction
{
public:
    explicit SimulateSceneAction(SceneViewPage* page);

    /// Implement EditorAction.
    /// @{
    bool IsComplete() const override { return isComplete_; }
    void Complete(bool force) override;
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    /// @}

private:
    void SetState(const PackedSceneData& data, const PackedSceneSelection& selection) const;

    WeakPtr<SceneViewPage> page_;
    bool isComplete_{};

    PackedSceneData oldData_;
    PackedSceneSelection oldSelection_;

    PackedSceneData newData_;
    PackedSceneSelection newSelection_;
};

/// Action for scene selection.
class ChangeSceneSelectionAction : public EditorAction
{
public:
    ChangeSceneSelectionAction(SceneViewPage* page,
        const PackedSceneSelection& oldSelection, const PackedSceneSelection& newSelection);

    /// Implement EditorAction
    /// @{
    bool IsTransparent() const override { return true; }
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void SetSelection(const PackedSceneSelection& selection) const;

    const WeakPtr<SceneViewPage> page_;
    const PackedSceneSelection oldSelection_;
    PackedSceneSelection newSelection_;
};

/// Wrapper to preserve scene selection on undo/redo.
class PreserveSceneSelectionWrapper : public BaseEditorActionWrapper
{
public:
    PreserveSceneSelectionWrapper(SharedPtr<EditorAction> action, SceneViewPage* page);

    /// Implement BaseEditorActionWrapper.
    /// @{
    bool CanRedo() const override;
    void Redo() const override;
    bool CanUndo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    const WeakPtr<SceneViewPage> page_;
    const PackedSceneSelection selection_;
};

/// Helper function to query geometries from a scene.
ea::vector<RayQueryResult> QueryGeometriesFromScene(Scene* scene, const Ray& ray,
    RayQueryLevel level = RAY_TRIANGLE, float maxDistance = M_INFINITY, unsigned viewMask = DEFAULT_VIEWMASK);

template <class T, class ... Args>
SceneViewAddon* SceneViewTab::RegisterAddon(const Args&... args)
{
    const auto addon = MakeShared<T>(this, args...);
    RegisterAddon(addon);
    return addon;
}

template <class T>
T* SceneViewTab::GetAddon()
{
    for (SceneViewAddon* addon : addons_)
    {
        if (auto casted = dynamic_cast<T*>(addon))
            return casted;
    }
    return nullptr;
}

}
