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

#include <Urho3D/SystemUI/TransformGizmo.h>

#include <EASTL/optional.h>

namespace Urho3D
{

void Foundation_TransformManipulator(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage scene selection with mouse and render debug geometry.
class TransformManipulator : public SceneViewAddon
{
    URHO3D_OBJECT(TransformManipulator, SceneViewAddon);

public:
    TransformManipulator(Context* context, HotkeyManager* hotkeyManager);

    void ToggleSpace() { isLocal_ = !isLocal_; }

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "TransformManipulator"; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void UpdateAndRender(SceneViewPage& scenePage) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void EnsureGizmoInitialized(SceneSelection& selection);

    unsigned selectionRevision_{};
    ea::optional<TransformNodesGizmo> transformGizmo_;

    bool isLocal_{};
};

}
