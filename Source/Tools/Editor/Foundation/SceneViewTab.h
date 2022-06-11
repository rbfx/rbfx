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

#include <EASTL/any.h>
#include <EASTL/vector_multiset.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor);

class SceneViewAddon;
class SceneViewTab;

/// Single page of SceneViewTab.
/// TODO(editor): Encapsulate
class SceneViewPage : public Object
{
    URHO3D_OBJECT(SceneViewPage, Object);

public:
    explicit SceneViewPage(Scene* scene);
    ~SceneViewPage() override;

    ea::any& GetAddonData(const SceneViewAddon& addon);

public:
    const SharedPtr<Scene> scene_;
    const SharedPtr<SceneRendererToTexture> renderer_;
    const ea::string cfgFileName_;

    ea::unordered_map<ea::string, ea::pair<WeakPtr<const SceneViewAddon>, ea::any>> addonData_;

    SceneSelection selection_;

    ea::optional<PackedSceneData> simulationBase_;

    /// UI state
    /// @{
    Rect contentArea_;
    /// @}

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
    /// Serialize per-scene page state of the addon.
    virtual void SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const;

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
    struct ByPriority
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    struct ByName
    {
        bool operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const;
    };

    using AddonSetByPriority = ea::vector_multiset<SharedPtr<SceneViewAddon>, ByPriority>;
    using AddonSetByName = ea::vector_multiset<SharedPtr<SceneViewAddon>, ByName>;

    explicit SceneViewTab(Context* context);
    ~SceneViewTab() override;

    /// Register new scene addon.
    void RegisterAddon(const SharedPtr<SceneViewAddon>& addon);
    template <class T, class ... Args> SceneViewAddon* RegisterAddon(const Args&... args);

    /// Setup context for plugin application execution.
    void SetupPluginContext();

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
    void RenderToolbar() override;
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
    const AddonSetByName& GetAddonsByName() const { return addonsByName_; }
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
    /// Manage pages
    /// @{
    SharedPtr<SceneViewPage> CreatePage(Scene* scene, bool isActive) const;
    void SavePageScene(SceneViewPage& page) const;

    void SavePageConfig(const SceneViewPage& page) const;
    void LoadPageConfig(SceneViewPage& page) const;
    /// @}

    void UpdateAddons(SceneViewPage& page);

    ea::vector<SharedPtr<SceneViewAddon>> addons_;
    AddonSetByPriority addonsByInputPriority_;
    AddonSetByName addonsByName_;

    ea::unordered_map<ea::string, SharedPtr<SceneViewPage>> scenes_;
    PackedSceneData clipboard_;
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

}
