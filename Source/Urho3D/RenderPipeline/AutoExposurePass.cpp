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

#include "../Graphics/Graphics.h"
#include "../Graphics/Texture2D.h"
#include "../RenderPipeline/RenderBufferManager.h"
#include "../RenderPipeline/AutoExposurePass.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include "../DebugNew.h"

namespace Urho3D
{

AutoExposurePass::AutoExposurePass(
    RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager)
    : PostProcessPass(renderPipeline, renderBufferManager)
{
    InitializeTextures();

    auto renderDevice = GetSubsystem<RenderDevice>();
    renderDevice->OnDeviceRestored.Subscribe(this, [&] { isAdaptedLuminanceInitialized_ = false; });
}

void AutoExposurePass::SetSettings(const AutoExposurePassSettings& settings)
{
    if (settings_ != settings)
    {
        const bool resetCachedTexturesAndStates = settings_.autoExposure_ != settings.autoExposure_;
        settings_ = settings;
        if (resetCachedTexturesAndStates)
        {
            pipelineStates_ = ea::nullopt;
            InitializeTextures();
        }
    }
}

void AutoExposurePass::InitializeTextures()
{
    if (!settings_.autoExposure_)
    {
        textures_ = {};
        return;
    }

    const RenderBufferFlags flagFixedBilinear = RenderBufferFlag::BilinearFiltering | RenderBufferFlag::FixedTextureSize;
    const RenderBufferFlags flagFixedNearest = RenderBufferFlag::FixedTextureSize;
    const RenderBufferFlags flagFixedNearestPersistent = RenderBufferFlag::FixedTextureSize | RenderBufferFlag::Persistent;
    const auto rgbaFormat = TextureFormat::TEX_FORMAT_RGBA16_FLOAT;
    const auto rgFormat = TextureFormat::TEX_FORMAT_RG16_FLOAT;

    textures_.color128_ = renderBufferManager_->CreateColorBuffer({ rgbaFormat, 1, flagFixedBilinear }, { 128, 128 });
    textures_.lum64_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedBilinear }, { 64, 64 });
    textures_.lum16_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedBilinear }, { 16, 16 });
    textures_.lum4_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedBilinear }, { 4, 4 });
    textures_.lum1_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedNearest }, { 1, 1 });
    textures_.adaptedLum_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedNearestPersistent }, { 1, 1 });
    textures_.prevAdaptedLum_ = renderBufferManager_->CreateColorBuffer({ rgFormat, 1, flagFixedNearest }, { 1, 1 });
}

void AutoExposurePass::InitializeStates()
{
    static const NamedSamplerStateDesc lumSamplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};
    static const NamedSamplerStateDesc adaptedLumSamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
    };
    static const NamedSamplerStateDesc applySamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
    };

    pipelineStates_ = CachedStates{};
    pipelineStates_->lum64_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE64", lumSamplers);
    pipelineStates_->lum16_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE16", lumSamplers);
    pipelineStates_->lum4_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE4", lumSamplers);
    pipelineStates_->lum1_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE1", lumSamplers);
    pipelineStates_->adaptedLum_ = renderBufferManager_->CreateQuadPipelineState(
        BLEND_REPLACE, "v2/P_AutoExposure", "ADAPTLUMINANCE", adaptedLumSamplers);

    ea::string exposureDefines = "EXPOSURE ";
    if (settings_.autoExposure_)
        exposureDefines += "AUTOEXPOSURE ";

    pipelineStates_->autoExposure_  = renderBufferManager_->CreateQuadPipelineState(
        BLEND_REPLACE, "v2/P_AutoExposure", exposureDefines, applySamplers);
}

void AutoExposurePass::EvaluateDownsampledColorBuffer()
{
    RawTexture* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();

    renderBufferManager_->SetRenderTargets(nullptr, { textures_.color128_ });
    renderBufferManager_->DrawTexture("Downsample color buffer", viewportTexture);
}

void AutoExposurePass::EvaluateLuminance()
{
    ShaderResourceDesc shaderResources[1];
    shaderResources[0].name_ = ShaderResources::Albedo;
    ShaderParameterDesc shaderParameters[1];
    shaderParameters[0].name_ = "InputInvSize";

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    shaderResources[0].texture_ = textures_.color128_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 128.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum64_;
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.lum64_ });
    renderBufferManager_->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_.lum64_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 64.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum16_;
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.lum16_ });
    renderBufferManager_->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_.lum16_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 16.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum4_;
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.lum4_ });
    renderBufferManager_->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_.lum4_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 4.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum1_;
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.lum1_ });
    renderBufferManager_->DrawQuad("Downsample luminosity buffer", drawParams);
}

void AutoExposurePass::EvaluateAdaptedLuminance()
{
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.prevAdaptedLum_ });
    RenderBuffer* sourceBuffer = isAdaptedLuminanceInitialized_ ? textures_.adaptedLum_ : textures_.lum1_;
    renderBufferManager_->DrawTexture("Store previous luminance", sourceBuffer->GetTexture());

    const ShaderResourceDesc shaderResources[] = {
        { ShaderResources::Albedo, textures_.prevAdaptedLum_->GetTexture() },
        { ShaderResources::Normal, textures_.lum1_->GetTexture() }
    };
    const ShaderParameterDesc shaderParameters[] = {
        { "AdaptRate", settings_.adaptRate_ },
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.pipelineStateId_ = pipelineStates_->adaptedLum_;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();
    renderBufferManager_->SetRenderTargets(nullptr, { textures_.adaptedLum_ });
    renderBufferManager_->DrawQuad("Adapt luminosity", drawParams);

    isAdaptedLuminanceInitialized_ = true;
}

void AutoExposurePass::Execute(Camera* camera)
{
    if (!pipelineStates_)
        InitializeStates();

    renderBufferManager_->SwapColorBuffers(false);

    if (settings_.autoExposure_)
    {
        EvaluateDownsampledColorBuffer();
        EvaluateLuminance();
        EvaluateAdaptedLuminance();
    }

    const ShaderResourceDesc shaderResources[] = {
        { ShaderResources::Normal, settings_.autoExposure_ ? textures_.adaptedLum_->GetTexture() : nullptr }
    };
    const ShaderParameterDesc shaderParameters[] = {
        { "MinMaxExposure", Vector2(settings_.minExposure_, settings_.maxExposure_) },
        { "AutoExposureMiddleGrey", 0.6f },
    };
    renderBufferManager_->SetOutputRenderTargets();
    renderBufferManager_->DrawFeedbackViewportQuad("Apply exposure",
        pipelineStates_->autoExposure_, shaderResources, shaderParameters);
}

}
