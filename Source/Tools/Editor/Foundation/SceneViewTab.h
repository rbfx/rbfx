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

#include "../Core/ResourceDragDropPayload.h"
#include "../Project/ProjectEditor.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Utility/SceneRendererToTexture.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor);

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
struct SceneViewPage
{
    SharedPtr<Scene> scene_;
    SharedPtr<SceneRendererToTexture> renderer_;
    ea::vector<SceneCameraControllerPtr> cameraControllers_;
    ea::string cfgFileName_;

    struct Selection
    {
        ea::unordered_set<WeakPtr<Node>> nodes_;
        ea::unordered_set<WeakPtr<Component>> components_;
        WeakPtr<Node> anchor_{};

        bool IsEmpty() const { return nodes_.empty() && components_.empty(); }
        bool IsSelected(Node* node) const;

        void Clear();
        void SetSelected(Node* node, bool selected, bool anchored = false);
    } selection_;

    /// UI state
    /// @{
    unsigned currentCameraController_{};
    /// @}

    SceneCameraController* GetCurrentCameraController() const;
};

/// Interface of SceneViewTab addon.
class SceneViewAddon : public Object
{
    URHO3D_OBJECT(SceneViewAddon, Object);

public:
    explicit SceneViewAddon(Context* context) : Object(context) {}

    /// Return unique human-readable name of the addon.
    virtual ea::string GetUniqueName() const = 0;
    /// Update and render addon.
    virtual void UpdateAndRender(SceneViewPage& scenePage, bool& mouseConsumed) = 0;
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
    template <class T, class ... Args> void RegisterAddon(const Args&... args);
    /// Register new type of camera controller. Should be called before any scenes are loaded.
    void RegisterCameraController(const SceneCameraControllerDesc& desc);
    template <class T, class ... Args> void RegisterCameraController(const Args&... args);

    /// ResourceEditorTab implementation
    /// @{
    ea::string GetResourceTitle() { return "Scene"; }
    bool SupportMultipleResources() { return true; }
    bool CanOpenResource(const OpenResourceRequest& request) override;
    /// @}

protected:
    /// ResourceEditorTab implementation
    /// @{
    void UpdateAndRenderContextMenuItems() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;

    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& resourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;

    void UpdateAndRenderContent() override;
    void UpdateFocused() override;
    /// @}

private:
    IntVector2 GetContentSize() const;

    /// Manage pages
    /// @{
    SceneViewPage* GetPage(const ea::string& resourceName);
    SceneViewPage* GetActivePage();
    SceneViewPage CreatePage(Scene* scene) const;

    void SavePageConfig(const SceneViewPage& page) const;
    void LoadPageConfig(SceneViewPage& page) const;
    /// @}

    void UpdateCameraController(SceneViewPage& page);
    void UpdateAddons(SceneViewPage& page);

    ea::vector<SharedPtr<SceneViewAddon>> addons_;
    ea::vector<SceneCameraControllerDesc> cameraControllers_;
    ea::unordered_map<ea::string, SceneViewPage> scenes_;

    /// UI state
    /// @{
    unsigned isCameraControllerActive_{};
    /// @}

};

template <class T, class ... Args>
void SceneViewTab::RegisterAddon(const Args&... args)
{
    RegisterAddon(MakeShared<T>(context_, args...));
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
