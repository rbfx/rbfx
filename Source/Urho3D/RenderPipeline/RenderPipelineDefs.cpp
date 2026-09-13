// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Graphics/Graphics.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/RenderPipelineDefs.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

TextureFormat GetTextureFormat(RenderPipelineColorSpace colorSpace)
{
    switch (colorSpace)
    {
    case RenderPipelineColorSpace::GammaLDR:
        return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
    case RenderPipelineColorSpace::LinearLDR:
        return TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB;
    case RenderPipelineColorSpace::LinearHDR:
        return TextureFormat::TEX_FORMAT_RGBA16_FLOAT;
    default:
        return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
    }
}

}

BatchStateCacheCallback::~BatchStateCacheCallback()
{
}

UIBatchStateCacheCallback::~UIBatchStateCacheCallback()
{
}

RenderPipelineInterface::~RenderPipelineInterface()
{
}

LightProcessorCallback::~LightProcessorCallback()
{
}

void RenderPipelineSettings::AdjustToSupported(Context* context)
{
    auto renderDevice = context->GetSubsystem<RenderDevice>();
    if (!renderDevice)
        return;

    const RenderDeviceCaps& caps = renderDevice->GetCaps();

    // RenderBufferManagerSettings
    if (renderBufferManager_.colorSpace_ == RenderPipelineColorSpace::LinearHDR && !caps.hdrOutput_)
    {
        URHO3D_LOGWARNING("HDR rendering is not supported, falling back to LDR");
        renderBufferManager_.colorSpace_ = RenderPipelineColorSpace::LinearLDR;
    }

    if (renderBufferManager_.colorSpace_ == RenderPipelineColorSpace::LinearLDR && !caps.srgbOutput_)
    {
        URHO3D_LOGWARNING("sRGB render targets are not supported, falling back to gamma color space");
        renderBufferManager_.colorSpace_ = RenderPipelineColorSpace::GammaLDR;
    }

    const int multiSample = renderDevice->GetSupportedMultiSample(
        GetTextureFormat(renderBufferManager_.colorSpace_), renderBufferManager_.multiSampleLevel_);
    if (multiSample != renderBufferManager_.multiSampleLevel_)
    {
        URHO3D_LOGWARNING("MSAA level {} is not supported, falling back to {}",
            renderBufferManager_.multiSampleLevel_, multiSample);
        renderBufferManager_.multiSampleLevel_ = multiSample;
    }

    // OcclusionBufferSettings

    // BatchRendererSettings

    // SceneProcessorSettings
    const bool deferredSupported = caps.readOnlyDepth_;
    if (sceneProcessor_.IsDeferredLighting() && !deferredSupported)
        sceneProcessor_.lightingMode_ = DirectLightingMode::Forward;

    // ShadowMapAllocatorSettings
    if (!renderDevice->IsRenderTargetFormatSupported(TextureFormat::TEX_FORMAT_RG32_FLOAT))
        shadowMapAllocator_.enableVarianceShadowMaps_ = false;

    shadowMapAllocator_.varianceShadowMapMultiSample_ = renderDevice->GetSupportedMultiSample(
        TextureFormat::TEX_FORMAT_RG32_FLOAT, shadowMapAllocator_.varianceShadowMapMultiSample_);

    shadowMapAllocator_.shadowAtlasPageSize_ = ea::min(
        shadowMapAllocator_.shadowAtlasPageSize_, caps.maxRenderTargetSize_);

    // TODO: Check if instancing is actually supported, i.e. if there's enough vertex attributes
}

void RenderPipelineSettings::PropagateImpliedSettings()
{
    // Deferred rendering expects certain properties from render textures
    if (sceneProcessor_.IsDeferredLighting())
    {
        renderBufferManager_.colorUsableWithMultipleRenderTargets_ = true;
        renderBufferManager_.stencilBuffer_ = true;
        renderBufferManager_.readableDepth_ = true;
        renderBufferManager_.inheritMultiSampleLevel_ = false;
    }

    // Setup instancing buffer format
    if (instancingBuffer_.enableInstancing_)
    {
        instancingBuffer_.firstInstancingTexCoord_ = 4;
        switch (sceneProcessor_.ambientMode_)
        {
        case DrawableAmbientMode::Constant:
            instancingBuffer_.numInstancingTexCoords_ = 3;
            break;
        case DrawableAmbientMode::Flat:
            instancingBuffer_.numInstancingTexCoords_ = 3 + 1;
            break;
        case DrawableAmbientMode::Directional:
            instancingBuffer_.numInstancingTexCoords_ = 3 + 7;
            break;
        }
    }
}

void RenderPipelineSettings::AdjustForRenderPath(RenderOutputFlags flags)
{
    renderBufferManager_.filteredColor_ = flags.Test(RenderOutputFlag::NeedColorOutputBilinear);
}

}
