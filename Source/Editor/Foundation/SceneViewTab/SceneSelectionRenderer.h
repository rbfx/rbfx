// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/OutlineGroup.h>

namespace Urho3D
{

void Foundation_SceneSelectionRenderer(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage scene selection with mouse and render debug geometry.
class SceneSelectionRenderer : public SceneViewAddon
{
    URHO3D_OBJECT(SceneSelectionRenderer, SceneViewAddon);

public:
    static const unsigned DirectSelectionRenderOrder = 255;
    static const unsigned IndirectSelectionRenderOrder = 254;

    struct Settings
    {
        ea::string GetUniqueName() { return "Editor.Scene:Selection"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();

        Color directSelectionColor_{0xfd5602_rgb};
        Color indirectSelectionColor_{0x009dff_rgb};
        bool debugGeometryDepthTest_{};
    };
    using SettingsPage = SimpleSettingsPage<Settings>;

    struct PageState
    {
        WeakPtr<OutlineGroup> directSelection_;
        WeakPtr<OutlineGroup> indirectSelection_;
        unsigned currentRevision_{};
    };

    explicit SceneSelectionRenderer(SceneViewTab* owner, SettingsPage* settings);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "SelectionRenderer"; }
    void Render(SceneViewPage& scenePage) override;
    bool RenderTabContextMenu() override;
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    PageState& GetOrInitializeState(SceneViewPage& scenePage) const;
    bool PrepareInternalComponents(SceneViewPage& scenePage, PageState& state) const;
    void UpdateInternalComponents(SceneViewPage& scenePage, PageState& state) const;
    void AddNodeDrawablesToGroup(const Node* node, OutlineGroup* group, OutlineGroup* excludeGroup = nullptr) const;
    void AddNodeChildrenDrawablesToGroup(const Node* node, OutlineGroup* group, OutlineGroup* excludeGroup = nullptr) const;

    bool NeedDepthTest(Component* component) const;
    void DrawNodeSelection(Scene* scene, Node* node, bool recursive);
    void DrawComponentSelection(Scene* scene, Component* component);

    const WeakPtr<SettingsPage> settings_;
    bool drawDebugGeometry_{true};
};

}
