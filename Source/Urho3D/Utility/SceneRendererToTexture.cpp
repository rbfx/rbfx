// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    if (size.x_ == 0 || size.y_ == 0)
    {
        URHO3D_LOGERROR("Invalid texture size.");
        return;
    }

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
