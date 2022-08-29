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
