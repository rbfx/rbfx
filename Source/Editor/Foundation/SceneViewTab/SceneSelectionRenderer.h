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

    void DrawNodeSelection(Scene* scene, Node* node, bool recursive);
    void DrawComponentSelection(Scene* scene, Component* component);

    const WeakPtr<SettingsPage> settings_;
    bool drawDebugGeometry_{true};
};

}
