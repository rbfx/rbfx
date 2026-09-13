// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/Shared/HierarchyBrowserSource.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/SystemUI/SceneHierarchyWidget.h>

namespace Urho3D
{

void Foundation_SceneHierarchy(Context* context, SceneViewTab* sceneViewTab);

/// Scene hierarchy provider for hierarchy browser tab.
class SceneHierarchy : public SceneViewAddon, public HierarchyBrowserSource
{
    URHO3D_OBJECT(SceneHierarchy, SceneViewAddon)

public:
    explicit SceneHierarchy(SceneViewTab* sceneViewTab);

    /// Implement SceneViewAddon
    /// @{
    ea::string GetUniqueName() const override { return "SceneHierarchy"; }
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

    /// Implement HierarchyBrowserSource
    /// @{
    EditorTab* GetOwnerTab() override { return owner_; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void RenderToolbar(SceneViewPage& page);
    void RenderSelectionContextMenu(Scene* scene, SceneSelection& selection);

    void ReorderNode(Node* node, unsigned oldIndex, unsigned newIndex);
    void ReorderComponent(Component* component, unsigned oldIndex, unsigned newIndex);
    void ReparentNode(Node* parentNode, Node* childNode);

    bool reentrant_{};
    SharedPtr<SceneHierarchyWidget> widget_;
};

}
