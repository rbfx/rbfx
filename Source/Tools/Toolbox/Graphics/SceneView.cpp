//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Toolbox/Scene/DebugCameraController.h>

#include "SceneView.h"

namespace Urho3D
{

SceneView::SceneView(Context* context, const IntRect& rect)
    : rect_(rect)
{
    scene_ = SharedPtr<Scene>(new Scene(context));
    scene_->CreateComponent<Octree>();
    viewport_ = SharedPtr<Viewport>(new Viewport(context, scene_, nullptr));
    viewport_->SetRect(IntRect(IntVector2::ZERO, rect_.Size()));
    CreateObjects();
    texture_ = SharedPtr<Texture2D>(new Texture2D(context));
    // Make sure viewport is not using default renderpath. That would cause issues when renderpath
    // is shared with other viewports (like in resource inspector).
    viewport_->SetRenderPath(viewport_->GetRenderPath()->Clone());
    SetSize(rect);
}

void SceneView::SetSize(const IntRect& rect)
{
    if (rect_ == rect)
        return;

    rect_ = rect;
    viewport_->SetRect(IntRect(IntVector2::ZERO, rect.Size()));
    texture_->SetSize(rect.Width(), rect.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    texture_->GetRenderSurface()->SetViewport(0, viewport_);
    texture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
}

void SceneView::CreateObjects()
{
    Node* parent = scene_->GetChild("EditorObjects");
    if (parent == nullptr)
    {
        parent = scene_->CreateChild("EditorObjects", LOCAL, 0, true);
        parent->AddTag("__EDITOR_OBJECT__");
    }
    Node* camera = parent->GetChild("EditorCamera");
    if (camera == nullptr)
    {
        camera = parent->CreateChild("EditorCamera", LOCAL);
        camera->CreateComponent<Camera>()->SetFarClip(160000);
        camera->AddTag("__EDITOR_OBJECT__");
    }
    auto* debug = scene_->GetOrCreateComponent<DebugRenderer>(LOCAL);
    debug->SetView(GetCamera());
    debug->SetTemporary(true);
    debug->SetLineAntiAlias(true);
    viewport_->SetCamera(GetCamera());
}

Camera* SceneView::GetCamera() const
{
    return scene_->GetChild("EditorObjects")->GetChild("EditorCamera")->GetComponent<Camera>();
}

}
