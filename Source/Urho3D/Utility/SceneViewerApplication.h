// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Viewport.h"
#include "Urho3D/Plugins/Plugin.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

/// Simple application to show scene with free-fly camera.
class URHO3D_API SceneViewerApplication : public ExecutablePlugin
{
    URHO3D_OBJECT(SceneViewerApplication, ExecutablePlugin);
    URHO3D_MANUAL_PLUGIN("Builtin.SceneViewer");

public:
    explicit SceneViewerApplication(Context* context);
    ~SceneViewerApplication() override;

    /// Implement ExecutablePlugin
    /// @{
    bool IsSuspendSupported() const override { return true; }
    /// @}

protected:
    /// Implement ExecutablePlugin
    /// @{
    void Load() override;
    void Unload() override;
    void Start(bool isMain) override;
    void Stop() override;
    void Suspend(Archive& output) override;
    void Resume(Archive* input, bool differentVersion) override;
    /// @}

private:
    SharedPtr<Viewport> viewport_;
    SharedPtr<Scene> scene_;

    SharedPtr<Node> cameraNode_;
};

}
