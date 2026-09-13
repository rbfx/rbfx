// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/OctreeQuery.h>

namespace Urho3D
{

void Foundation_SceneSelector(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage scene selection with mouse and render debug geometry.
class SceneSelector : public SceneViewAddon
{
    URHO3D_OBJECT(SceneSelector, SceneViewAddon);

public:
    explicit SceneSelector(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "Selector"; }
    int GetInputPriority() const override { return M_MIN_INT; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    /// @}

private:
    Drawable* QuerySelectedDrawable(Scene* scene, const Ray& cameraRay, RayQueryLevel level) const;
    Node* QuerySelectedNode(Scene* scene, const Ray& cameraRay) const;
    void SelectNode(SceneSelection& selection, Node* node, bool toggle, bool append) const;
};

}
