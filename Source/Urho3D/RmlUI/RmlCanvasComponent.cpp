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

#include "../Core/Context.h"
#include "../Graphics/BillboardSet.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/Texture2D.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../RmlUI/RmlCanvasComponent.h"
#include "../RmlUI/RmlUI.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"

#include <RmlUi/Core/Context.h>

#include <assert.h>

namespace Urho3D
{

static const int UICOMPONENT_DEFAULT_TEXTURE_SIZE = 512;
static const int UICOMPONENT_MIN_TEXTURE_SIZE = 64;
static const int UICOMPONENT_MAX_TEXTURE_SIZE = 4096;

RmlCanvasComponent::RmlCanvasComponent(Context* context)
    : LogicComponent(context)
{
    offScreenUI_ = new RmlUI(context_, Format("RmlTextureComponent_{:p}", (void*)this).c_str());
    offScreenUI_->mouseMoveEvent_.Subscribe(this, &RmlCanvasComponent::RemapMousePos);
    texture_ = MakeShared<Texture2D>(context_);
    SetUpdateEventMask(USE_UPDATE);
}

RmlCanvasComponent::~RmlCanvasComponent()
{
    // Unload document first so other components can receive events about document invalidation and null their pointers. This process
    // depends on RmlUI instance being alive.
    offScreenUI_->GetRmlContext()->UnloadAllDocuments();
}

void RmlCanvasComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RmlCanvasComponent>(Category_RmlUI);

    URHO3D_ACCESSOR_ATTRIBUTE("Texture", GetTextureRef, SetTextureRef, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Remap Mouse Position", bool, remapMousePos_, true, AM_DEFAULT);
}

void RmlCanvasComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_ == nullptr)
        ClearTexture();
}

void RmlCanvasComponent::OnSetEnabled()
{
    if (!enabled_)
        ClearTexture();
    offScreenUI_->SetRendering(enabled_);
    offScreenUI_->SetBlockEvents(!enabled_);
}

void RmlCanvasComponent::SetUISize(IntVector2 size)
{
    assert(texture_ != nullptr);
    if (size.x_ < UICOMPONENT_MIN_TEXTURE_SIZE || size.x_ > UICOMPONENT_MAX_TEXTURE_SIZE ||
        size.y_ < UICOMPONENT_MIN_TEXTURE_SIZE || size.y_ > UICOMPONENT_MAX_TEXTURE_SIZE)
    {
        URHO3D_LOGERROR("RmlCanvasComponent: Invalid texture size {}x{}", size.x_, size.y_);
        return;
    }

    if (texture_->SetSize(size.x_, size.y_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget))
    {
        RenderSurface* surface = texture_->GetRenderSurface();
        surface->SetUpdateMode(SURFACE_MANUALUPDATE);
        offScreenUI_->SetRenderTarget(surface, Color::BLACK);
    }
    else
    {
        offScreenUI_->SetRenderTarget(nullptr);
        SetEnabled(false);
        URHO3D_LOGERROR("RmlCanvasComponent: Resizing of UI render-target texture failed.");
    }
    ClearTexture();
}

void RmlCanvasComponent::SetTextureRef(const ResourceRef& texture)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (Resource* textureRes = cache->GetResource(texture.type_, texture.name_))
    {
        if (Texture2D* texture2D = textureRes->Cast<Texture2D>())
        {
            SetTexture(texture2D);
            SetUISize({UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE});    // TODO: To attribute
        }
        else
            URHO3D_LOGERROR("Resource with name {} exists, but is not a Texture2D.", texture.name_);
    }
    else
        URHO3D_LOGERROR("Resource with name {} is already registered.", texture.name_);
}

ResourceRef RmlCanvasComponent::GetTextureRef() const
{
    if (texture_ == nullptr)
        return ResourceRef(Texture2D::GetTypeStatic());
    return ResourceRef(Texture2D::GetTypeStatic(), texture_->GetName());
}

void RmlCanvasComponent::ClearTexture()
{
    if (texture_ == nullptr)
        return;

    Image clear(context_);
    int w = texture_->GetWidth(), h = texture_->GetHeight();
    if (w > 0 && h > 0)
    {
        clear.SetSize(w, h, 4);
        clear.Clear(Color::TRANSPARENT_BLACK);
        texture_->SetData(0, 0, 0, w, h, clear.GetData());
    }
}

void RmlCanvasComponent::RemapMousePos(IntVector2& screenPos)
{
    if (!remapMousePos_ || node_ == nullptr)
        return;

    if (auto* ui = GetSubsystem<RmlUI>())
    {
        Rml::Context* context = ui->GetRmlContext();
        if (!ui->GetBlockEvents() && context->GetHoverElement() != context->GetRootElement())
        {
            // Cursor hovers UI rendered into backbuffer. Do not process any input here.
            screenPos = {-1, -1};
            return;
        }
    }

    Scene* scene = node_->GetScene();
    auto* model = node_->GetComponent<StaticModel>();
    auto* renderer = GetSubsystem<Renderer>();
    auto* octree = scene ? scene->GetComponent<Octree>() : nullptr;
    if (scene == nullptr || model == nullptr || renderer == nullptr || octree == nullptr)
        return;

    Viewport* viewport = nullptr;
    for (int i = 0; i < renderer->GetNumViewports(); i++)
    {
        if (Viewport* vp = renderer->GetViewport(i))
        {
            IntRect rect = vp->GetRect();
            if (vp->GetScene() == scene)
            {
                if (rect == IntRect::ZERO)
                {
                    // Save full-screen viewport only if we do not have a better smaller candidate.
                    if (viewport == nullptr)
                        viewport = vp;
                }
                else if (rect.Contains(screenPos))
                    // Small viewports override full-screen one (picture-in-picture situation).
                    viewport = vp;
                break;
            }
        }
    }

    if (viewport == nullptr)
        return;

    Camera* camera = viewport->GetCamera();
    if (camera == nullptr)
        return;

    IntRect rect = viewport->GetRect();
    if (rect == IntRect::ZERO)
    {
        auto* graphics = GetSubsystem<Graphics>();
        rect.right_ = graphics->GetWidth();
        rect.bottom_ = graphics->GetHeight();
    }

    Ray ray(camera->GetScreenRay((float)screenPos.x_ / rect.Width(), (float)screenPos.y_ / rect.Height()));
    ea::vector<RayQueryResult> queryResultVector;
    RayOctreeQuery query(queryResultVector, ray, RAY_TRIANGLE_UV, M_INFINITY, DRAWABLE_GEOMETRY, DEFAULT_VIEWMASK);
    octree->Raycast(query);
    if (queryResultVector.empty())
        return;

    for (RayQueryResult& queryResult : queryResultVector)
    {
        if (queryResult.drawable_ != model)
        {
            // ignore billboard sets by default
            if (queryResult.drawable_->IsInstanceOf<BillboardSet>())
                continue;
            return;
        }

        Vector2& uv = queryResult.textureUV_;
        IntVector2 uiSize = ToIntVector2(offScreenUI_->GetRmlContext()->GetDimensions());
        screenPos = IntVector2(static_cast<int>(uv.x_ * uiSize.x_), static_cast<int>(uv.y_ * uiSize.y_));
    }
}

void RmlCanvasComponent::SetTexture(Texture2D* texture)
{
    if (texture)
    {
        texture_->SetFilterMode(FILTER_BILINEAR);
        texture_->SetAddressMode(TextureCoordinate::U, ADDRESS_CLAMP);
        texture_->SetAddressMode(TextureCoordinate::V, ADDRESS_CLAMP);
        texture_->SetNumLevels(1);  // No mipmaps
    }
    texture_ = texture;
}

}
