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

#include "../Graphics/Viewport.h"
#include "../Plugins/PluginApplication.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

/// Simple application to show scene with free-fly camera.
class URHO3D_API SceneViewerApplication : public MainPluginApplication
{
    URHO3D_OBJECT(SceneViewerApplication, MainPluginApplication);
    URHO3D_MANUAL_PLUGIN("Builtin.SceneViewer");

public:
    explicit SceneViewerApplication(Context* context);
    ~SceneViewerApplication() override;

protected:
    /// Implement MainPluginApplication
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
