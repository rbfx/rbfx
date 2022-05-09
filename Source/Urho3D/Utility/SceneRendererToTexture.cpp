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

#ifdef URHO3D_D3D11
#include <dxgi1_2.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

unsigned GetViewportTextureFormat()
{
#ifdef URHO3D_D3D11
    // DX11 doesn't have RGB texture format and we don't want ImGUI to use alpha
    return DXGI_FORMAT_B8G8R8X8_UNORM;
#else
    return Graphics::GetRGBFormat();
#endif
}

}

SceneRendererToTexture::SceneRendererToTexture(Scene* scene)
    : Object(scene->GetContext())
    , scene_(scene)
    , cameraNode_(MakeShared<Node>(context_))
    , camera_(cameraNode_->CreateComponent<Camera>())
    , texture_(MakeShared<Texture2D>(context_))
    , viewport_(MakeShared<Viewport>(context_, scene_, camera_))
{
}

SceneRendererToTexture::~SceneRendererToTexture()
{
}

void SceneRendererToTexture::SetTextureSize(const IntVector2& size)
{
    if (textureSize_ != size)
    {
        textureSize_ = size;
        textureDirty_ = true;
    }
}

void SceneRendererToTexture::SetActive(bool active)
{
    isActive_ = active;
    if (auto renderSurface = texture_->GetRenderSurface())
        renderSurface->SetUpdateMode(active ? SURFACE_UPDATEALWAYS : SURFACE_MANUALUPDATE);
}

void SceneRendererToTexture::Update()
{
    if (textureDirty_)
    {
        textureDirty_ = false;
        texture_->SetSize(textureSize_.x_, textureSize_.y_, GetViewportTextureFormat(), TEXTURE_RENDERTARGET);
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
        texture_->GetRenderSurface()->SetViewport(0, viewport_);
    }
}

}
