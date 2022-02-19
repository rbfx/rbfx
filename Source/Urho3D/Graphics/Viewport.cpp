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
#include "../Graphics/RenderPath.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/View.h"
#include "../Graphics/Viewport.h"
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
    SetRenderPath((RenderPath*)nullptr);
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, RenderPath* renderPath) :
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath) :   // NOLINT(modernize-pass-by-value)
    Object(context),
    scene_(scene),
    camera_(camera),
    rect_(rect),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::~Viewport() = default;

void Viewport::RegisterObject(Context* context)
{
    context->RegisterFactory<Viewport>();
}

void Viewport::SetScene(Scene* scene)
{
#ifndef URHO3D_LEGACY_RENDERER
    if (!!scene_ != !!scene)
    {
        renderPipeline_ = nullptr;
        view_ = nullptr;
    }
#endif

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

void Viewport::SetRenderPath(RenderPath* renderPath)
{
    if (renderPath)
        renderPath_ = renderPath;
    else
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
            renderPath_ = renderer->GetDefaultRenderPath();
    }
}

bool Viewport::SetRenderPath(XMLFile* file)
{
    SharedPtr<RenderPath> newRenderPath(new RenderPath());
    if (newRenderPath->Load(file))
    {
        renderPath_ = newRenderPath;
        return true;
    }
    return false;
}

Scene* Viewport::GetScene() const
{
    return scene_;
}

Camera* Viewport::GetCamera() const
{
    return camera_;
}

IntRect Viewport::GetEffectiveRect(RenderSurface* renderTarget) const
{
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
        return rect;
    }
}

Camera* Viewport::GetCullCamera() const
{
    return cullCamera_;
}

View* Viewport::GetView() const
{
    return view_;
}

RenderPipelineView* Viewport::GetRenderPipelineView() const
{
    if (renderPipelineComponent_ && renderPipelineComponent_->GetScene() == scene_)
        return renderPipeline_;
    return nullptr;
}

RenderPath* Viewport::GetRenderPath() const
{
    return renderPath_;
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
#ifndef URHO3D_LEGACY_RENDERER
    if (!GetRenderPipelineView())
    {
        renderPipelineComponent_ = nullptr;
        renderPipeline_ = nullptr;
    }

    if (!renderPipelineComponent_ && scene_)
    {
        renderPipelineComponent_ = scene_->GetDerivedComponent<RenderPipeline>();
        if (!renderPipelineComponent_)
            renderPipelineComponent_ = scene_->CreateComponent<RenderPipeline>();
    }

    if (!renderPipeline_ && renderPipelineComponent_)
        renderPipeline_ = renderPipelineComponent_->Instantiate();
#endif

    if (!renderPipeline_)
        view_ = MakeShared<View>(context_);
}

}
