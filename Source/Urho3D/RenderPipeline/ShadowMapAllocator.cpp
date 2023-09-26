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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/ShadowMapAllocator.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

ShadowMapRegion ShadowMapRegion::GetSplit(unsigned split, const IntVector2& numSplits) const
{
    const IntVector2 splitSize = rect_.Size() / numSplits;
    assert(rect_.Size() == splitSize * numSplits);

    const IntVector2 index{ static_cast<int>(split % numSplits.x_), static_cast<int>(split / numSplits.x_) };
    const IntVector2 splitBegin = rect_.Min() + splitSize * index;
    const IntVector2 splitEnd = splitBegin + splitSize;

    ShadowMapRegion splitShadowMap = *this;
    splitShadowMap.rect_ = { splitBegin, splitEnd };
    return splitShadowMap;
}

ShadowMapAllocator::ShadowMapAllocator(Context* context)
    : Object(context)
    , renderDevice_(context_->GetSubsystem<RenderDevice>())
    , renderContext_(renderDevice_->GetRenderContext())
{
    CacheSettings();
}

void ShadowMapAllocator::SetSettings(const ShadowMapAllocatorSettings& settings)
{
    if (settings_ != settings)
    {
        settings_ = settings;
        CacheSettings();

        pages_.clear();
    }
}

void ShadowMapAllocator::CacheSettings()
{
    auto renderDevice = GetSubsystem<RenderDevice>();

    shadowOutputDesc_ = {};
    if (settings_.enableVarianceShadowMaps_)
    {
        shadowMapFormat_ = TextureFormat::TEX_FORMAT_RG32_FLOAT;

        shadowOutputDesc_.depthStencilFormat_ = renderDevice->GetDefaultDepthFormat();
        shadowOutputDesc_.numRenderTargets_ = 1;
        shadowOutputDesc_.renderTargetFormats_[0] = shadowMapFormat_;
        shadowOutputDesc_.multiSample_ = settings_.varianceShadowMapMultiSample_;
    }
    else
    {
        shadowMapFormat_ = settings_.use16bitShadowMaps_ //
            ? TextureFormat::TEX_FORMAT_D16_UNORM //
            : renderDevice->GetDefaultDepthFormat();

        shadowOutputDesc_.depthStencilFormat_ = shadowMapFormat_;
        shadowOutputDesc_.numRenderTargets_ = 0;
        shadowOutputDesc_.multiSample_ = 1;
    }

    shadowAtlasPageSize_ = static_cast<int>(settings_.shadowAtlasPageSize_) * IntVector2::ONE;

    const bool isDepthTexture = !settings_.enableVarianceShadowMaps_;
    samplerStateDesc_ = {};
    samplerStateDesc_.shadowCompare_ = isDepthTexture;
    samplerStateDesc_.filterMode_ = FILTER_BILINEAR;
}

void ShadowMapAllocator::ResetAllShadowMaps()
{
    for (AtlasPage& element : pages_)
    {
        element.areaAllocator_.Reset(shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_, shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_);
        element.clearBeforeRendering_ = false;
    }
}

ShadowMapRegion ShadowMapAllocator::AllocateShadowMap(const IntVector2& size)
{
    if (!settings_.shadowAtlasPageSize_ || !shadowMapFormat_)
        return {};

    const IntVector2 clampedSize = VectorMin(size, shadowAtlasPageSize_);

    for (AtlasPage& element : pages_)
    {
        const ShadowMapRegion shadowMap = element.AllocateRegion(clampedSize);
        if (shadowMap)
            return shadowMap;
    }

    AllocatePage();
    return pages_.back().AllocateRegion(clampedSize);
}

bool ShadowMapAllocator::BeginShadowMapRendering(const ShadowMapRegion& shadowMap)
{
    if (!shadowMap || shadowMap.pageIndex_ >= pages_.size())
        return false;

    Texture2D* shadowMapTexture = shadowMap.texture_;
    AtlasPage& poolElement = pages_[shadowMap.pageIndex_];

    if (shadowMapTexture->IsDepthStencil())
    {
        // The shadow map is a depth stencil texture
        renderContext_->SetRenderTargets(RenderTargetView::Texture(shadowMapTexture), {});
    }
    else
    {
        const RenderTargetView renderTargets[] = {RenderTargetView::Texture(shadowMapTexture)};
        renderContext_->SetRenderTargets(RenderTargetView::Texture(vsmDepthTexture_), renderTargets);
    }

    // Clear whole texture if needed
    if (poolElement.clearBeforeRendering_)
    {
        poolElement.clearBeforeRendering_ = false;

        renderContext_->ClearDepthStencil(CLEAR_DEPTH);
        if (settings_.enableVarianceShadowMaps_)
            renderContext_->ClearRenderTarget(0, Color::WHITE);
    }

    renderContext_->SetViewport(shadowMap.rect_);

    return true;
}

ShadowMapRegion ShadowMapAllocator::AtlasPage::AllocateRegion(const IntVector2& size)
{
    int x{}, y{};
    if (areaAllocator_.Allocate(size.x_, size.y_, x, y))
    {
        const IntVector2 offset{ x, y };
        ShadowMapRegion shadowMap;
        shadowMap.pageIndex_ = index_;
        shadowMap.texture_ = texture_;
        shadowMap.rect_ = IntRect(offset, offset + size);

        // Mark shadow map as used
        clearBeforeRendering_ = true;
        return shadowMap;
    }
    return {};
}

void ShadowMapAllocator::AllocatePage()
{
    const bool isDepthTexture = !settings_.enableVarianceShadowMaps_;
    const TextureFlags textureFlags = isDepthTexture ? TextureFlag::BindDepthStencil : TextureFlag::BindRenderTarget;
    const int multiSample = isDepthTexture ? 1 : settings_.varianceShadowMapMultiSample_;

    auto newShadowMap = MakeShared<Texture2D>(context_);

    newShadowMap->SetName(Format("Dynamic ShadowMap #{}", pages_.size()));

    // Disable mipmaps from the shadow map
    newShadowMap->SetNumLevels(1);
    newShadowMap->SetFilterMode(FILTER_BILINEAR);
    newShadowMap->SetShadowCompare(isDepthTexture);
    newShadowMap->SetSize(shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_, shadowMapFormat_, textureFlags, multiSample);

    // Store allocate shadow map
    AtlasPage& element = pages_.emplace_back();
    element.index_ = pages_.size() - 1;
    element.texture_ = newShadowMap;
    element.areaAllocator_.Reset(shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_, shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_);

    if (!settings_.enableVarianceShadowMaps_)
        vsmDepthTexture_ = nullptr;
    else if (!vsmDepthTexture_ || vsmDepthTexture_->GetSize() != shadowAtlasPageSize_)
    {
        vsmDepthTexture_ = MakeShared<Texture2D>(context_);
        vsmDepthTexture_->SetSize(shadowAtlasPageSize_.x_, shadowAtlasPageSize_.y_,
            shadowOutputDesc_.depthStencilFormat_, TextureFlag::BindDepthStencil, multiSample);
    }
}

}
