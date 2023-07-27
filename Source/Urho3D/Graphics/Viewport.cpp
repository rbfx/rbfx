//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Viewport.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"

#include "../DebugNew.h"

namespace Urho3D
{

Viewport::Viewport(Context* context) :
    Object(context),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera) :
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect) :   // NOLINT(modernize-pass-by-value)
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(rect),
    drawDebug_(true)
{
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect, RenderPipeline* renderPipeline)
    : Object(context)
    , scene_(scene)
    , camera_(camera)
    , rect_(rect)
    , drawDebug_(true)
    , autoRenderPipeline_(false)
    , renderPipeline_(renderPipeline)
{
}

Viewport::~Viewport() = default;

void Viewport::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Viewport>();
}

void Viewport::SetScene(Scene* scene)
{
    if (!!scene_ != !!scene)
    {
        renderPipelineView_ = nullptr;
    }

    scene_ = scene;
}

void Viewport::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void Viewport::SetCullCamera(Camera* camera)
{
    cullCamera_ = camera;
}

void Viewport::SetRect(const IntRect& rect)
{
    rect_ = rect;
}

void Viewport::SetDrawDebug(bool enable)
{
    drawDebug_ = enable;
}

Scene* Viewport::GetScene() const
{
    return scene_;
}

Camera* Viewport::GetCamera() const
{
    return camera_;
}

IntRect Viewport::GetEffectiveRect(RenderSurface* renderTarget, bool compensateRenderTargetFlip) const
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    const bool isOpenGL = renderDevice && renderDevice->GetBackend() == RenderBackend::OpenGL;

    const IntVector2 renderTargetSize = RenderSurface::GetSize(GetSubsystem<Graphics>(), renderTarget);

    if (rect_ == IntRect::ZERO)
    {
        // Return render target dimensions if viewport rectangle is not defined
        return { IntVector2::ZERO, renderTargetSize };
    }
    else
    {
        // Validate and return viewport rectangle
        IntRect rect;
        rect.left_ = Clamp(rect_.left_, 0, renderTargetSize.x_ - 1);
        rect.top_ = Clamp(rect_.top_, 0, renderTargetSize.y_ - 1);
        rect.right_ = Clamp(rect_.right_, rect.left_ + 1, renderTargetSize.x_);
        rect.bottom_ = Clamp(rect_.bottom_, rect.top_ + 1, renderTargetSize.y_);

        if (isOpenGL && renderTarget && compensateRenderTargetFlip)
        {
            // On OpenGL the render to texture is flipped vertically.
            // Flip the viewport rectangle to compensate.
            rect.top_ = renderTargetSize.y_ - rect.top_;
            rect.bottom_ = renderTargetSize.y_ - rect.bottom_;
            ea::swap(rect.top_, rect.bottom_);
        }

        return rect;
    }
}

Camera* Viewport::GetCullCamera() const
{
    return cullCamera_;
}

RenderPipelineView* Viewport::GetRenderPipelineView() const
{
    // Render pipeline is null or expired
    if (!renderPipeline_)
        return nullptr;

    // Automatic pipeline is not from the scene
    if (autoRenderPipeline_ && renderPipeline_->GetScene() != scene_)
        return nullptr;

    // View is expired or outdated
    if (!renderPipelineView_ || renderPipelineView_->GetRenderPipeline() != renderPipeline_)
        return nullptr;

    return renderPipelineView_;
}

Ray Viewport::GetScreenRay(int x, int y) const
{
    if (!camera_)
        return Ray();

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        auto* graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics->GetWidth();
        screenY = (float)y / (float)graphics->GetHeight();
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->GetScreenRay(screenX, screenY);
}

IntVector2 Viewport::WorldToScreenPoint(const Vector3& worldPos) const
{
    if (!camera_)
        return IntVector2::ZERO;

    Vector2 screenPoint = camera_->WorldToScreenPoint(worldPos);

    int x;
    int y;
    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        auto* graphics = GetSubsystem<Graphics>();
        x = (int)(screenPoint.x_ * graphics->GetWidth());
        y = (int)(screenPoint.y_ * graphics->GetHeight());
    }
    else
    {
        x = (int)(rect_.left_ + screenPoint.x_ * rect_.Width());
        y = (int)(rect_.top_ + screenPoint.y_ * rect_.Height());
    }

    return IntVector2(x, y);
}

Vector3 Viewport::ScreenToWorldPoint(int x, int y, float depth) const
{
    if (!camera_)
        return Vector3::ZERO;

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        auto* graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics->GetWidth();
        screenY = (float)y / (float)graphics->GetHeight();
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->ScreenToWorldPoint(Vector3(screenX, screenY, depth));
}

void Viewport::AllocateView()
{
    // If automatic render pipeline, expire it on scene mismatch
    if (autoRenderPipeline_ && renderPipeline_ && renderPipeline_->GetScene() != scene_)
        renderPipeline_ = nullptr;

    if (!renderPipeline_ && scene_)
    {
        renderPipeline_ = scene_->GetDerivedComponent<RenderPipeline>();
        if (!renderPipeline_)
            renderPipeline_ = scene_->CreateComponent<RenderPipeline>();
    }

    // Expire view on pipeline mismatch
    if (renderPipeline_ && (!renderPipelineView_ || renderPipelineView_->GetRenderPipeline() != renderPipeline_))
        renderPipelineView_ = renderPipeline_->Instantiate();
}

}
