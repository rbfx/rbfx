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
#include "../Graphics/Graphics.h"
#include "../Graphics/Texture2D.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../RmlUI/RmlTextureComponent.h"
#include "../RmlUI/RmlUI.h"

#include <RmlUi/Core/Context.h>

#include <assert.h>

namespace Urho3D
{

static int const UICOMPONENT_DEFAULT_TEXTURE_SIZE = 512;
static int const UICOMPONENT_MIN_TEXTURE_SIZE = 64;
static int const UICOMPONENT_MAX_TEXTURE_SIZE = 4096;

extern const char* RML_UI_CATEGORY;

RmlTextureComponent::RmlTextureComponent(Context* context)
    : Component(context)
{
    offScreenUI_ = new RmlUI(context_, Format("RmlTextureComponent_{:p}", (void*)this).c_str());
    offScreenUI_->mouseMoveEvent_.Subscribe(this, &RmlTextureComponent::TranslateMousePos);

    texture_ = context_->CreateObject<Texture2D>();
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);  // No mipmaps

    SetTextureSize({UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE});
}

void RmlTextureComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<RmlTextureComponent>(RML_UI_CATEGORY);
    URHO3D_COPY_BASE_ATTRIBUTES(BaseClassName);
    URHO3D_ATTRIBUTE_EX("Virtual Resource Name", ea::string, virtualResourceName_, OnVirtualResourceNameSet, "", AM_DEFAULT);
}

void RmlTextureComponent::OnNodeSet(Node* node)
{
    Resource* resource = GetVirtualResource();
    assert(resource != nullptr);
    if (node)
    {
        if (!virtualResourceName_.empty())
            AddVirtualResource(resource);
    }
    else
    {
        if (!virtualResourceName_.empty())
            RemoveVirtualResource(resource);
    }
}

void RmlTextureComponent::OnSetEnabled()
{
    if (!enabled_)
        ClearTexture();
    offScreenUI_->SetEnabled(enabled_);
}

void RmlTextureComponent::SetTextureSize(IntVector2 size)
{
    if (size.x_ < UICOMPONENT_MIN_TEXTURE_SIZE || size.x_ > UICOMPONENT_MAX_TEXTURE_SIZE ||
        size.y_ < UICOMPONENT_MIN_TEXTURE_SIZE || size.y_ > UICOMPONENT_MAX_TEXTURE_SIZE || size.x_ != size.y_)
    {
        URHO3D_LOGERROR("RmlTextureComponent: Invalid texture size {}x{}", size.x_, size.y_);
        return;
    }

    if (texture_->SetSize(size.x_, size.y_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET))
    {
        RenderSurface* surface = texture_->GetRenderSurface();
        surface->SetUpdateMode(SURFACE_MANUALUPDATE);
        offScreenUI_->SetRenderTarget(surface, Color::BLACK);
    }
    else
    {
        offScreenUI_->SetRenderTarget(nullptr);
        SetEnabled(false);
        URHO3D_LOGERROR("RmlTextureComponent: Resizing of UI rendertarget texture failed.");
    }
    ClearTexture();
}

IntVector2 RmlTextureComponent::GetTextureSize(IntVector2) const
{
    if (texture_.Null())
        return IntVector2::ZERO;
    else
        return IntVector2(texture_->GetWidth(), texture_->GetHeight());
}

void RmlTextureComponent::SetVirtualResourceName(const ea::string& name)
{
    virtualResourceName_ = name;
    OnVirtualResourceNameSet();
}

void RmlTextureComponent::AddVirtualResource(Resource* resource)
{
    assert(resource != nullptr);
    assert(!resource->GetName().empty());
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddManualResource(resource);
}

void RmlTextureComponent::RemoveVirtualResource(Resource* resource)
{
    assert(resource != nullptr);
    assert(!resource->GetName().empty());
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->ReleaseResource(resource->GetType(), resource->GetName());
}

void RmlTextureComponent::ClearTexture()
{
    Image clear(context_);
    int w = texture_->GetWidth(), h = texture_->GetHeight();
    if (w > 0 && h > 0)
    {
        clear.SetSize(w, h, 4);
        clear.Clear(Color::TRANSPARENT_BLACK);
        texture_->SetData(&clear);
    }
}

void RmlTextureComponent::OnVirtualResourceNameSet()
{
    bool attachedToNode = GetNode() != nullptr;
    Resource* resource = GetVirtualResource();
    assert(resource != nullptr);

    if (attachedToNode && !resource->GetName().empty())
        RemoveVirtualResource(resource);

    resource->SetName(virtualResourceName_);

    if (attachedToNode && !resource->GetName().empty())
        AddVirtualResource(resource);
}

}
