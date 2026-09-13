// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Plugins/PluginApplication.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

class GamePlugin : public MainPluginApplication
{
    URHO3D_OBJECT(GamePlugin, MainPluginApplication);

public:
    /// Construct.
    explicit GamePlugin(Context* context);

protected:
    /// Implement MainPluginApplication
    /// @{
    void Load() override;
    void Start(bool isMain) override;
    void Stop() override;
    void Unload() override;
    /// @}

private:
    SharedPtr<Viewport> viewport_;
    SharedPtr<Scene> scene_;

    SharedPtr<Node> cameraNode_;
};


}
