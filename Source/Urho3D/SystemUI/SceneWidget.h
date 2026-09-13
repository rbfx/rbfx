// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Signal.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/SystemUI/BaseWidget.h"
#include "Urho3D/SystemUI/Widgets.h"
#include "Urho3D/Utility/SceneRendererToTexture.h"

namespace Urho3D
{

/// SystemUI widget to preview scene.
class URHO3D_API SceneWidget : public BaseWidget
{
    URHO3D_OBJECT(SceneWidget, BaseWidget)

public:
    SceneWidget(Context* context);
    ~SceneWidget() override;

    void RenderContent() override;

    Scene* GetScene() const { return scene_; }
    SceneRendererToTexture* GetRenderer();
    Camera* GetCamera() { return GetRenderer() ? GetRenderer()->GetCamera() : nullptr; }
    Scene* CreateDefaultScene();
    void SetSkyboxTexture(Texture* texture);
    void LookAt(const BoundingBox& box);

private:
    SharedPtr<Scene> scene_;
    SharedPtr<SceneRendererToTexture> renderer_;
    Node* lightPivotNode_;
    Node* lightNode_;
};

} // namespace Urho3D
