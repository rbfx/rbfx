// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderTargetView.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/SwapChain.h>

namespace Urho3D
{

RenderTargetView::RenderTargetView(RenderDevice* renderDevice, RawTexture* texture, Type type, unsigned slice)
    : renderDevice_(renderDevice)
    , texture_(texture)
    , type_(type)
    , slice_(slice)
{
}
RenderTargetView RenderTargetView::SwapChainColor(RenderDevice* renderDevice)
{
    URHO3D_ASSERT(renderDevice);
    return {renderDevice, nullptr, Type::SwapChainColor, 0};
}

RenderTargetView RenderTargetView::SwapChainDepthStencil(RenderDevice* renderDevice)
{
    URHO3D_ASSERT(renderDevice);
    return {renderDevice, nullptr, Type::SwapChainDepthStencil, 0};
}

RenderTargetView RenderTargetView::Texture(RawTexture* texture)
{
    URHO3D_ASSERT(texture);
    return {texture->GetRenderDevice(), texture, Type::Resource, 0};
}

RenderTargetView RenderTargetView::TextureSlice(RawTexture* texture, unsigned slice)
{
    URHO3D_ASSERT(texture);
    return {texture->GetRenderDevice(), texture, Type::ResourceSlice, slice};
}

void RenderTargetView::MarkDirty() const
{
    if (texture_)
        texture_->MarkDirty();
}

Diligent::ITextureView* RenderTargetView::GetView() const
{
    if (type_ == Type::SwapChainColor)
        return renderDevice_->GetSwapChain()->GetCurrentBackBufferRTV();
    if (type_ == Type::SwapChainDepthStencil)
        return renderDevice_->GetSwapChain()->GetDepthBufferDSV();

    if (!texture_)
    {
        URHO3D_ASSERTLOG(false, "RenderTargetView::GetView called for null resource");
        return nullptr;
    }

    const auto& handles = texture_->GetHandles();
    if (type_ == Type::Resource)
    {
        if (!handles.rtv_ && !handles.dsv_)
        {
            URHO3D_ASSERTLOG(false, "RenderTargetView::GetView called for resource without RTV");
            return nullptr;
        }

        return handles.rtv_ ? handles.rtv_ : handles.dsv_;
    }
    else
    {
        if (slice_ >= handles.renderSurfaces_.size())
        {
            URHO3D_ASSERTLOG(false, "Invalid slice index: {} of {}", slice_, handles.renderSurfaces_.size());
            return nullptr;
        }
        return handles.renderSurfaces_[slice_];
    }
}

TextureFormat RenderTargetView::GetFormat() const
{
    switch (type_)
    {
    case Type::SwapChainColor: return renderDevice_->GetSwapChain()->GetDesc().ColorBufferFormat;

    case Type::SwapChainDepthStencil: return renderDevice_->GetSwapChain()->GetDesc().DepthBufferFormat;

    case Type::Resource:
    case Type::ResourceSlice: return texture_->GetParams().format_;

    default: URHO3D_ASSERT(false); return TextureFormat::TEX_FORMAT_UNKNOWN;
    }
}

int RenderTargetView::GetMultiSample() const
{
    switch (type_)
    {
    case Type::SwapChainColor:
    case Type::SwapChainDepthStencil: return 1; // TODO(diligent): Support multi-sampling

    case Type::Resource:
    case Type::ResourceSlice: return texture_->GetParams().multiSample_;

    default: URHO3D_ASSERT(false); return 0;
    }
}

} // namespace Urho3D
