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

#include "../Precompiled.h"

#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Viewport.h"
#include "../Scene/Scene.h"
#include "../Utility/SceneRendererToTexture.h"

#include "../DebugNew.h"

namespace Urho3D
{

CustomBackbufferTexture::CustomBackbufferTexture(Context* context)
    : Object(context)
    , texture_(MakeShared<Texture2D>(context_))
{
}

CustomBackbufferTexture::~CustomBackbufferTexture()
{
}

void CustomBackbufferTexture::SetTextureSize(const IntVector2& size)
{
    if (textureSize_ != size)
    {
        textureSize_ = size;
        textureDirty_ = true;
    }
}

void CustomBackbufferTexture::SetActive(bool active)
{
    isActive_ = active;
    if (auto renderSurface = texture_->GetRenderSurface())
        renderSurface->SetUpdateMode(isActive_ ? SURFACE_UPDATEALWAYS : SURFACE_MANUALUPDATE);
}

void CustomBackbufferTexture::Update()
{
    if (textureDirty_)
    {
        textureDirty_ = false;
        texture_->SetSize(
            textureSize_.x_, textureSize_.y_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget);
        RenderSurface* renderSurface = texture_->GetRenderSurface();
        if (renderSurface)
        {
            OnRenderSurfaceCreated(this, renderSurface);
            renderSurface->SetUpdateMode(isActive_ ? SURFACE_UPDATEALWAYS : SURFACE_MANUALUPDATE);
        }
    }
}

SceneRendererToTexture::SceneRendererToTexture(Scene* scene)
    : CustomBackbufferTexture(scene->GetContext())
    , scene_(scene)
    , cameraNode_(MakeShared<Node>(context_))
    , camera_(cameraNode_->CreateComponent<Camera>())
    , viewport_(MakeShared<Viewport>(context_, scene_, camera_))
{
    OnRenderSurfaceCreated.Subscribe(this, &SceneRendererToTexture::SetupViewport);
}

SceneRendererToTexture::~SceneRendererToTexture()
{
}

void SceneRendererToTexture::SetupViewport(RenderSurface* renderSurface)
{
    renderSurface->SetViewport(0, viewport_);
}

Vector3 SceneRendererToTexture::GetCameraPosition() const
{
    return cameraNode_->GetWorldPosition();
}

Quaternion SceneRendererToTexture::GetCameraRotation() const
{
    return cameraNode_->GetWorldRotation();
}

}
