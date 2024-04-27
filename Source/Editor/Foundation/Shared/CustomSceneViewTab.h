//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../../Core/CommonEditorActions.h"
#include "../../Project/Project.h"
#include "../../Project/ResourceEditorTab.h"
#include "../../Foundation/Shared/CameraController.h"

#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/Graphics/Animation.h>

namespace Urho3D
{

/// Tab that renders custom Scene.
class CustomSceneViewTab : public ResourceEditorTab
{
    URHO3D_OBJECT(CustomSceneViewTab, ResourceEditorTab)

public:
    CustomSceneViewTab(Context* context, const ea::string& title, const ea::string& guid, EditorTabFlags flags,
        EditorTabPlacement placement);
    ~CustomSceneViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;
    /// @}

    virtual void ResetCamera();

    Scene* GetScene() const { return preview_ ? preview_->GetScene() : nullptr; }
    Camera* GetCamera() const { return preview_ ? preview_->GetCamera() : nullptr; }

protected:
    virtual void RenderTitle();

    const SharedPtr<SceneWidget> preview_;
    SharedPtr<CameraController> cameraController_;
    CameraController::PageState state_;
};

} // namespace Urho3D
