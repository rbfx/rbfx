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
#include "../Graphics/GraphicsUtils.h"
#include "../IO/Log.h"
#include "../RenderAPI/PipelineState.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/CameraProcessor.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../RenderPipeline/InstancingBuffer.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/PipelineStateBuilder.h"
#include "../RenderPipeline/SceneProcessor.h"
#include "../RenderPipeline/ShadowMapAllocator.h"

#include <EASTL/span.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

CullMode GetEffectiveCullMode(CullMode mode, bool isCameraReversed)
{
    if (mode == CULL_NONE || !isCameraReversed)
        return mode;

    return mode == CULL_CW ? CULL_CCW : CULL_CW;
}

CullMode GetEffectiveCullMode(CullMode passCullMode, CullMode materialCullMode, bool isCameraReversed)
{
    const CullMode cullMode = passCullMode != MAX_CULLMODES ? passCullMode : materialCullMode;
    return GetEffectiveCullMode(cullMode, isCameraReversed);
}

}

PipelineStateBuilder::PipelineStateBuilder(Context* context,
    const SceneProcessor* sceneProcessor, const CameraProcessor* cameraProcessor,
    const ShadowMapAllocator* shadowMapAllocator, const InstancingBuffer* instancingBuffer)
    : Object(context)
    , sceneProcessor_(sceneProcessor)
    , cameraProcessor_(cameraProcessor)
    , shadowMapAllocator_(shadowMapAllocator)
    , instancingBuffer_(instancingBuffer)
    , graphics_(GetSubsystem<Graphics>())
    , renderDevice_(GetSubsystem<RenderDevice>())
    , pipelineStateCache_(GetSubsystem<PipelineStateCache>())
    , compositor_(MakeShared<ShaderProgramCompositor>(context_))
{
}

void PipelineStateBuilder::SetSettings(const ShaderProgramCompositorSettings& settings)
{
    compositor_->SetSettings(settings);
}

void PipelineStateBuilder::UpdateFrameSettings(bool linearColorSpace)
{
    compositor_->SetFrameSettings(cameraProcessor_, linearColorSpace);
}

SharedPtr<PipelineState> PipelineStateBuilder::CreateBatchPipelineState(
    const BatchStateCreateKey& key, const BatchStateCreateContext& ctx, const PipelineStateOutputDesc& outputDesc)
{
    const RenderDeviceCaps& caps = renderDevice_->GetCaps();

    Light* light = key.pixelLight_ ? key.pixelLight_->GetLight() : nullptr;
    const bool hasShadow = key.pixelLight_ && key.pixelLight_->HasShadow();

    BatchCompositorPass* batchCompositorPass = sceneProcessor_->GetUserPass(ctx.pass_);
    const bool isShadowPass = batchCompositorPass == nullptr && ctx.subpassIndex_ == BatchCompositor::ShadowSubpass;
    const bool isLightVolumePass = batchCompositorPass == nullptr && ctx.subpassIndex_ == BatchCompositor::LitVolumeSubpass;
    const bool isRefractionPass =
        batchCompositorPass && batchCompositorPass->GetFlags().Test(DrawableProcessorPassFlag::RefractionPass);
    const bool isStereoPass =
        batchCompositorPass && batchCompositorPass->GetFlags().Test(DrawableProcessorPassFlag::StereoInstancing);

    ClearState();

    pipelineStateDesc_.output_ = outputDesc;

    if (isShadowPass)
    {
        compositor_->ProcessShadowBatch(shaderProgramDesc_,
            key.geometry_, key.geometryType_, key.material_, key.pass_, light);
        SetupShadowPassState(ctx.shadowSplitIndex_, key.pixelLight_, key.material_, key.pass_);

        SetupSamplersForUserOrShadowPass(key.material_, false, false, false);
        SetupInputLayoutAndPrimitiveType(pipelineStateDesc_, shaderProgramDesc_, key.geometry_, false);
        SetupShaders(pipelineStateDesc_, shaderProgramDesc_);
    }
    else if (isLightVolumePass)
    {
        compositor_->ProcessLightVolumeBatch(shaderProgramDesc_,
            key.geometry_, key.geometryType_, key.pass_, light, hasShadow);
        SetupLightVolumePassState(key.pixelLight_);

        SetupLightSamplers(key.pixelLight_);
        SetupGeometryBufferSamplers();
        SetupInputLayoutAndPrimitiveType(pipelineStateDesc_, shaderProgramDesc_, key.geometry_, false);
        SetupShaders(pipelineStateDesc_, shaderProgramDesc_);

        pipelineStateDesc_.readOnlyDepth_ = true;
    }
    else if (batchCompositorPass && batchCompositorPass->IsFlagSet(DrawableProcessorPassFlag::PipelineStateCallback))
    {
        batchCompositorPass->CreatePipelineState(pipelineStateDesc_, this, key, ctx);
    }
    else if (batchCompositorPass)
    {
        const DrawableProcessorPassFlags flags = batchCompositorPass->GetFlags();
        const auto subpass = static_cast<BatchCompositorSubpass>(ctx.subpassIndex_);
        const bool lightMaskToStencil = subpass == BatchCompositorSubpass::Deferred
            && flags.Test(DrawableProcessorPassFlag::DeferredLightMaskToStencil);
        const bool hasAmbient = flags.Test(DrawableProcessorPassFlag::HasAmbientLighting);
        const bool hasLightmap = key.drawable_->GetGlobalIlluminationType() == GlobalIlluminationType::UseLightMap;

        compositor_->ProcessUserBatch(shaderProgramDesc_, flags, key.drawable_, key.geometry_, key.geometryType_,
            key.material_, key.pass_, light, hasShadow, subpass);
        SetupUserPassState(key.drawable_, key.material_, key.pass_, lightMaskToStencil);

        // Support negative lights
        if (light && light->IsNegative())
        {
            assert(subpass == BatchCompositorSubpass::Light);
            if (pipelineStateDesc_.blendMode_ == BLEND_ADD)
                pipelineStateDesc_.blendMode_ = BLEND_SUBTRACT;
            else if (pipelineStateDesc_.blendMode_ == BLEND_ADDALPHA)
                pipelineStateDesc_.blendMode_ = BLEND_SUBTRACTALPHA;
        }

        // Mark depth as read-only if requested and supported
        if (caps.readOnlyDepth_ && flags.Test(DrawableProcessorPassFlag::ReadOnlyDepth))
            pipelineStateDesc_.readOnlyDepth_ = true;

        SetupLightSamplers(key.pixelLight_);
        SetupSamplersForUserOrShadowPass(key.material_, hasLightmap, hasAmbient, isRefractionPass);
        SetupInputLayoutAndPrimitiveType(pipelineStateDesc_, shaderProgramDesc_, key.geometry_, isStereoPass);
        SetupShaders(pipelineStateDesc_, shaderProgramDesc_);
    }

    return pipelineStateCache_->GetGraphicsPipelineState(pipelineStateDesc_);
}

SharedPtr<PipelineState> PipelineStateBuilder::CreateBatchPipelineStatePlaceholder(
    unsigned vertexStride, const PipelineStateOutputDesc& outputDesc)
{
    GraphicsPipelineStateDesc desc;

    desc.debugName_ = Format("Pipeline State Placeholder for vertex stride {}", vertexStride);
    desc.colorWriteEnabled_ = true;
    desc.depthWriteEnabled_ = true;
    desc.depthCompareFunction_ = CMP_ALWAYS;

    desc.inputLayout_.size_ = 1;
    desc.inputLayout_.elements_[0].bufferStride_ = vertexStride;
    desc.inputLayout_.elements_[0].elementType_ = TYPE_VECTOR3;
    desc.inputLayout_.elements_[0].elementSemantic_ = SEM_POSITION;

    desc.output_ = outputDesc;

    desc.vertexShader_ = graphics_->GetShader(VS, "v2/X_PlaceholderShader", "");
    desc.pixelShader_ = graphics_->GetShader(PS, "v2/X_PlaceholderShader", "");

    return pipelineStateCache_->GetGraphicsPipelineState(desc);
}

void PipelineStateBuilder::ClearState()
{
    pipelineStateDesc_ = {};
    shaderProgramDesc_.Clear();
}

void PipelineStateBuilder::SetupShadowPassState(unsigned splitIndex, const LightProcessor* lightProcessor,
        const Material* material, const Pass* pass)
{
    const ShadowMapAllocatorSettings& settings = shadowMapAllocator_->GetSettings();
    const CookedLightParams& lightParams = lightProcessor->GetParams();
    const float biasMultiplier = lightParams.shadowDepthBiasMultiplier_[splitIndex] * settings.depthBiasScale_;
    const BiasParameters& biasParameters = lightProcessor->GetLight()->GetShadowBias();

    pipelineStateDesc_.debugName_ = Format("Shadow Pass for material '{}'", material->GetName());

    if (settings.enableVarianceShadowMaps_)
    {
        pipelineStateDesc_.colorWriteEnabled_ = true;
        pipelineStateDesc_.constantDepthBias_ = 0.0f;
        pipelineStateDesc_.slopeScaledDepthBias_ = 0.0f;
    }
    else
    {
        pipelineStateDesc_.colorWriteEnabled_ = false;
        pipelineStateDesc_.constantDepthBias_ =
            biasMultiplier * biasParameters.constantBias_ + settings.depthBiasOffset_;
        pipelineStateDesc_.slopeScaledDepthBias_ = biasMultiplier * biasParameters.slopeScaledBias_;
    }

    pipelineStateDesc_.depthWriteEnabled_ = pass->GetDepthWrite();
    pipelineStateDesc_.depthCompareFunction_ = pass->GetDepthTestMode();

    pipelineStateDesc_.cullMode_ = GetEffectiveCullMode(pass->GetCullMode(), material->GetShadowCullMode(), false);
}

void PipelineStateBuilder::SetupLightVolumePassState(const LightProcessor* lightProcessor)
{
    const Light* light = lightProcessor->GetLight();
    pipelineStateDesc_.debugName_ = "Light Volume Pass";
    pipelineStateDesc_.colorWriteEnabled_ = true;
    pipelineStateDesc_.blendMode_ = light->IsNegative() ? BLEND_SUBTRACT : BLEND_ADD;

    if (light->GetLightType() != LIGHT_DIRECTIONAL)
    {
        if (lightProcessor->DoesOverlapCamera())
        {
            pipelineStateDesc_.cullMode_ = GetEffectiveCullMode(CULL_CW, cameraProcessor_->IsCameraReversed());
            pipelineStateDesc_.depthCompareFunction_ = CMP_GREATER;
        }
        else
        {
            pipelineStateDesc_.cullMode_ = GetEffectiveCullMode(CULL_CCW, cameraProcessor_->IsCameraReversed());
            pipelineStateDesc_.depthCompareFunction_ = CMP_LESSEQUAL;
        }
    }
    else
    {
        pipelineStateDesc_.cullMode_ = CULL_NONE;
        pipelineStateDesc_.depthCompareFunction_ = CMP_ALWAYS;
    }

    pipelineStateDesc_.stencilTestEnabled_ = true;
    pipelineStateDesc_.stencilCompareFunction_ = CMP_NOTEQUAL;
    pipelineStateDesc_.stencilCompareMask_ = light->GetLightMaskEffective() & PORTABLE_LIGHTMASK;
}

void PipelineStateBuilder::SetupUserPassState(const Drawable* drawable,
    const Material* material, const Pass* pass, bool lightMaskToStencil)
{
    pipelineStateDesc_.debugName_ = Format("User Pass '{}' for material '{}'", pass->GetName(), material->GetName());

    pipelineStateDesc_.depthWriteEnabled_ = pass->GetDepthWrite();
    pipelineStateDesc_.depthCompareFunction_ = pass->GetDepthTestMode();

    pipelineStateDesc_.colorWriteEnabled_ = pass->GetColorWrite();
    pipelineStateDesc_.blendMode_ = pass->GetBlendMode();
    pipelineStateDesc_.alphaToCoverageEnabled_ = pass->GetAlphaToCoverage() || material->GetAlphaToCoverage();
    pipelineStateDesc_.constantDepthBias_ = material->GetDepthBias().constantBias_;
    pipelineStateDesc_.slopeScaledDepthBias_ = material->GetDepthBias().slopeScaledBias_;

    pipelineStateDesc_.fillMode_ = ea::max(cameraProcessor_->GetCameraFillMode(), material->GetFillMode());
    pipelineStateDesc_.cullMode_ = GetEffectiveCullMode(pass->GetCullMode(),
        material->GetCullMode(), cameraProcessor_->IsCameraReversed());

    if (lightMaskToStencil)
    {
        pipelineStateDesc_.stencilTestEnabled_ = true;
        pipelineStateDesc_.stencilOperationOnPassed_ = OP_REF;
        pipelineStateDesc_.stencilWriteMask_ = PORTABLE_LIGHTMASK;
    }
}

void PipelineStateBuilder::SetupInputLayoutAndPrimitiveType(GraphicsPipelineStateDesc& pipelineStateDesc,
    const ShaderProgramDesc& shaderProgramDesc, const Geometry* geometry, bool isStereoPass) const
{
    if (shaderProgramDesc.isInstancingUsed_)
    {
        InitializeInputLayoutAndPrimitiveType(pipelineStateDesc, geometry, instancingBuffer_->GetVertexBuffer());

        if (isStereoPass)
        {
            // Patch step rates for stereo rendering.
            // TODO: Do we want to have something nicer?
            for (unsigned i = 0; i < pipelineStateDesc.inputLayout_.size_; ++i)
            {
                InputLayoutElementDesc& elementDesc = pipelineStateDesc.inputLayout_.elements_[i];
                if (elementDesc.instanceStepRate_ != 0)
                    elementDesc.instanceStepRate_ = 2;
            }
        }
    }
    else
    {
        InitializeInputLayoutAndPrimitiveType(pipelineStateDesc, geometry);
    }
}

void PipelineStateBuilder::SetupShaders(GraphicsPipelineStateDesc& pipelineStateDesc, ShaderProgramDesc& shaderProgramDesc) const
{
    for (ea::string& shaderDefines : shaderProgramDesc.shaderDefines_)
        shaderDefines += shaderProgramDesc.commonShaderDefines_;

    pipelineStateDesc.vertexShader_ = graphics_->GetShader(
        VS, shaderProgramDesc.shaderName_[VS], shaderProgramDesc.shaderDefines_[VS]);
    pipelineStateDesc.pixelShader_ = graphics_->GetShader(
        PS, shaderProgramDesc.shaderName_[PS], shaderProgramDesc.shaderDefines_[PS]);
}

void PipelineStateBuilder::SetupLightSamplers(const LightProcessor* lightProcessor)
{
    const Light* light = lightProcessor ? lightProcessor->GetLight() : nullptr;
    if (light)
    {
        if (Texture* rampTexture = light->GetRampTexture())
            pipelineStateDesc_.samplers_.Add(ShaderResources::LightRamp, rampTexture->GetSamplerStateDesc());
        if (Texture* shapeTexture = light->GetShapeTexture())
            pipelineStateDesc_.samplers_.Add(ShaderResources::LightShape, shapeTexture->GetSamplerStateDesc());
    }
    if (lightProcessor && lightProcessor->HasShadow())
        pipelineStateDesc_.samplers_.Add(ShaderResources::ShadowMap, shadowMapAllocator_->GetSamplerStateDesc());
}

void PipelineStateBuilder::SetupSamplersForUserOrShadowPass(
    const Material* material, bool hasLightmap, bool hasAmbient, bool isRefractionPass)
{
    // TODO: Make configurable
    static const auto lightMapSampler = SamplerStateDesc::Default();
    static const auto reflectionMapSampler = SamplerStateDesc::Trilinear();
    static const auto refractionMapSampler = SamplerStateDesc::Trilinear();

    bool materialHasEnvironmentMap = false;
    for (const auto& [nameHash, texture] : material->GetTextures())
    {
        if (texture.value_)
        {
            if (nameHash == ShaderResources::Emission && hasLightmap)
                continue;
            if (nameHash == ShaderResources::Reflection0)
                materialHasEnvironmentMap = true;
            pipelineStateDesc_.samplers_.Add(nameHash, texture.value_->GetSamplerStateDesc());
        }
    }

    if (hasLightmap)
        pipelineStateDesc_.samplers_.Add(ShaderResources::Emission, lightMapSampler);

    if (hasAmbient)
    {
        if (!materialHasEnvironmentMap)
            pipelineStateDesc_.samplers_.Add(ShaderResources::Reflection0, reflectionMapSampler);
        pipelineStateDesc_.samplers_.Add(ShaderResources::Reflection1, reflectionMapSampler);
    }

    if (isRefractionPass)
        pipelineStateDesc_.samplers_.Add(ShaderResources::Emission, refractionMapSampler);

    pipelineStateDesc_.samplers_.Add(ShaderResources::DepthBuffer, SamplerStateDesc::Nearest());
}

void PipelineStateBuilder::SetupGeometryBufferSamplers()
{
    pipelineStateDesc_.samplers_.Add(ShaderResources::Albedo, SamplerStateDesc::Nearest());
    pipelineStateDesc_.samplers_.Add(ShaderResources::Properties, SamplerStateDesc::Nearest());
    pipelineStateDesc_.samplers_.Add(ShaderResources::Normal, SamplerStateDesc::Nearest());
    pipelineStateDesc_.samplers_.Add(ShaderResources::DepthBuffer, SamplerStateDesc::Nearest());
}

}
