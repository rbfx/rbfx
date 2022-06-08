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

#include "../Input/FreeFlyController.h"
#include "../Input/Input.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Utility/SceneViewerApplication.h"

namespace Urho3D
{

SceneViewerApplication::SceneViewerApplication(Context* context)
    : PluginApplication(context)
{
}

SceneViewerApplication::~SceneViewerApplication()
{
}

void SceneViewerApplication::Load()
{
}

void SceneViewerApplication::Unload()
{
}

void SceneViewerApplication::Start()
{
    auto cache = GetSubsystem<ResourceCache>();
    auto renderer = GetSubsystem<Renderer>();

    scene_ = MakeShared<Scene>(context_);
    // TODO(editor): Fix me
    scene_->LoadFile("Scenes/RenderingShowcase_2_BakedDirectIndirect.xml");

    cameraNode_ = scene_->CreateChild("Viewer Camera");
    auto camera = cameraNode_->CreateComponent<Camera>();
    auto controller = cameraNode_->CreateComponent<FreeFlyController>();

    // TODO(editor): Fix me
    cameraNode_->SetPosition({0.0f, 5.0f, -10.0f});
    cameraNode_->LookAt({0.0f, 0.0f, 0.0f});

    viewport_ = MakeShared<Viewport>(context_, scene_, camera);
    renderer->SetNumViewports(1);
    renderer->SetViewport(0, viewport_);
}

void SceneViewerApplication::Stop()
{
    auto renderer = GetSubsystem<Renderer>();
    renderer->SetNumViewports(0);

    viewport_ = nullptr;
    cameraNode_ = nullptr;
    scene_ = nullptr;
}

void SceneViewerApplication::Suspend(Archive& output)
{
}

void SceneViewerApplication::Resume(Archive* input, bool differentVersion)
{
}

}
