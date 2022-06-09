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
#include "../Core/ResourceDragDropPayload.h"
#include "../Project/ProjectEditor.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Utility/SceneRendererToTexture.h>
#include <Urho3D/Utility/SceneSelection.h>
#include <Urho3D/Utility/PackedSceneData.h>

#include <EASTL/vector_multiset.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor);

class SceneViewTab;

/// Interface of Camera controller used by Scene.
class SceneCameraController : public Object
{
    URHO3D_OBJECT(SceneCameraController, Object);

public:
    explicit SceneCameraController(Scene* scene, Camera* camera);
    ~SceneCameraController() override;

    /// Return name in UI.
    virtual ea::string GetTitle() const = 0;
    /// Return whether the camera manipulation is active.
    virtual bool IsActive(bool wasActive) { return false; }
    /// Update controller for given camera object.
    virtual void Update(bool isActive) = 0;

protected:
    Vector2 GetMouseMove() const;
    Vector3 GetMoveDirection() const;
    bool GetMoveAccelerated() const;

    const WeakPtr<Scene> scene_;
    const WeakPtr<Camera> camera_;
};

using SceneCameraControllerPtr = SharedPtr<SceneCameraController>;

/// Description of camera controller for SceneViewTab.
struct SceneCameraControllerDesc
{
    ea::string name_;
    ea::function<SceneCameraControllerPtr(Scene* scene, Camera* camera)> factory_;
};

/// Single page of SceneViewTab.
struct SceneViewPage : public RefCounted
{
    SharedPtr<Scene> scene_;
    SharedPtr<SceneRendererToTexture> renderer_;
    ea::vector<SceneCameraControllerPtr> cameraControllers_;
    ea::string cfgFileName_;

    SceneSelection selection_;

    ea::optional<PackedSceneData> simulationBase_;

    /// UI state
    /// @{
    Rect contentArea_;
    unsigned currentCameraController_{};
    /// @}

    SceneCameraController* GetCurrentCameraController() const;
    void RewindSimulation();
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
    /// Process input.
    virtual void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) {}
    /// Update and render addon.
    virtual void Render(SceneViewPage& scenePage) {}
    /// Apply hotkeys for given addon.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager);
    /// Render context menu of the tab.
    virtual bool RenderTabContextMenu() { return false; }

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
    explicit SceneViewTab(Context* context);
    ~SceneViewTab() override;

    /// Register new scene addon.
    void RegisterAddon(const SharedPtr<SceneViewAddon>& addon);
    template <class T, class ... Args> SceneViewAddon* RegisterAddon(const Args&... args);
    /// Register new type of camera controller. Should be called before any scenes are loaded.
    void RegisterCameraController(const SceneCameraControllerDesc& desc);
    template <class T, class ... Args> void RegisterCameraController(const Args&... args);

    /// Commands
    /// @{
    void ResumeSimulation();
    void PauseSimulation();
    void ToggleSimulationPaused();
    void RewindSimulation();

    void CutSelection();
    void CopySelection();
    void PasteNextToSelection();
    void DeleteSelection();
    /// @}

    /// ResourceEditorTab implementation
    /// @{
    void RenderMenu() override;
    bool IsUndoSupported() { return true; }
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;

    ea::string GetResourceTitle() { return "Scene"; }
    bool SupportMultipleResources() { return true; }
    bool CanOpenResource(const OpenResourceRequest& request) override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;

    void PushAction(SharedPtr<EditorAction> action) override;
    using ResourceEditorTab::PushAction;
    /// @}

    /// Return current state.
    /// @{
    SceneViewPage* GetPage(const ea::string& resourceName);
    SceneViewPage* GetActivePage();
    /// @}

protected:
    /// ResourceEditorTab implementation
    /// @{
    void RenderContextMenuItems() override;

    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& resourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;

    void RenderContent() override;
    void UpdateFocused() override;
    /// @}

private:
    struct ByPriority
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    struct ByName
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    /// Manage pages
    /// @{
    SharedPtr<SceneViewPage> CreatePage(Scene* scene, bool isActive) const;
    void SavePageScene(SceneViewPage& page) const;

    void SavePageConfig(const SceneViewPage& page) const;
    void LoadPageConfig(SceneViewPage& page) const;
    /// @}

    void UpdateCameraController(SceneViewPage& page);
    void UpdateAddons(SceneViewPage& page);

    ea::vector<SharedPtr<SceneViewAddon>> addons_;
    ea::vector_multiset<SharedPtr<SceneViewAddon>, ByPriority> addonsByInputPriority_;
    ea::vector_multiset<SharedPtr<SceneViewAddon>, ByName> addonsByName_;

    ea::vector<SceneCameraControllerDesc> cameraControllers_;
    ea::unordered_map<ea::string, SharedPtr<SceneViewPage>> scenes_;
    PackedSceneData clipboard_;

    /// UI state
    /// @{
    unsigned isCameraControllerActive_{};
    /// @}

};

/// Action wrapper that rewinds scene simulation.
class RewindSceneActionWrapper : public BaseEditorActionWrapper
{
public:
    RewindSceneActionWrapper(SharedPtr<EditorAction> action, SceneViewPage* page);

    /// Implement EditorAction.
    /// @{
    bool IsAlive() const override;
    void Redo() const override;
    void Undo() const override;
    /// @}

private:
    WeakPtr<SceneViewPage> page_;
};

template <class T, class ... Args>
SceneViewAddon* SceneViewTab::RegisterAddon(const Args&... args)
{
    const auto addon = MakeShared<T>(this, args...);
    RegisterAddon(addon);
    return addon;
}

template <class T, class ... Args>
void SceneViewTab::RegisterCameraController(const Args&... args)
{
    SceneCameraControllerDesc desc;
    desc.name_ = T::GetTypeNameStatic();
    desc.factory_ = [=](Scene* scene, Camera* camera) { return MakeShared<T>(scene, camera, args...); };
    RegisterCameraController(desc);
}

}
