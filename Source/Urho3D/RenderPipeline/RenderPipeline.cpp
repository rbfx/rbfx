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
#include "../Core/IteratorRange.h"
#include "../Graphics/Camera.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/ShaderProgramLayout.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/DrawCommandQueue.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Viewport.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../RenderPipeline/InstancingBuffer.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/BatchRenderer.h"
#include "../RenderPipeline/ShadowMapAllocator.h"
#include "../Scene/Scene.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/sort.h>

#include "../Graphics/Light.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Batch.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Zone.h"
#include "../Graphics/PipelineState.h"
#include "../Core/WorkQueue.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

static const ea::vector<ea::string> ambientModeNames =
{
    "Constant",
    "Flat",
    "Directional",
};

static const ea::vector<ea::string> postProcessAntialiasingNames =
{
    "None",
    "FXAA2",
    "FXAA3"
};

static const ea::vector<ea::string> postProcessTonemappingNames =
{
    "None",
    "ReinhardEq3",
    "ReinhardEq4",
    "Uncharted2",
};

}

RenderPipeline::RenderPipeline(Context* context)
    : RenderPipelineInterface(context)
    , graphics_(context_->GetSubsystem<Graphics>())
    , renderer_(context_->GetSubsystem<Renderer>())
    , workQueue_(context_->GetSubsystem<WorkQueue>())
{
}

RenderPipeline::~RenderPipeline()
{
}

void RenderPipeline::RegisterObject(Context* context)
{
    context->RegisterFactory<RenderPipeline>();

    URHO3D_ATTRIBUTE_EX("Max Vertex Lights", int, settings_.maxVertexLights_, MarkSettingsDirty, 4, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Max Pixel Lights", int, settings_.maxPixelLights_, MarkSettingsDirty, 4, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Ambient Mode", settings_.ambientMode_, MarkSettingsDirty, ambientModeNames, DrawableAmbientMode::Directional, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Enable Instancing", bool, settings_.enableInstancing_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Enable Shadow", bool, settings_.enableShadows_, MarkSettingsDirty, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Deferred Lighting", bool, settings_.deferredLighting_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("PCF Kernel Size", unsigned, settings_.pcfKernelSize_, MarkSettingsDirty, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Use Variance Shadow Maps", bool, settings_.enableVarianceShadowMaps_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("VSM Shadow Settings", Vector2, settings_.varianceShadowMapParams_, MarkSettingsDirty, BatchRendererSettings{}.varianceShadowMapParams_, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("VSM Multi Sample", int, settings_.varianceShadowMapMultiSample_, MarkSettingsDirty, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Gamma Correction", bool, settings_.gammaCorrection_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Post Process Antialiasing", settings_.postProcess_.antialiasing_, MarkSettingsDirty, postProcessAntialiasingNames, PostProcessAntialiasing::None, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Post Process Tonemapping", settings_.postProcess_.tonemapping_, MarkSettingsDirty, postProcessTonemappingNames, PostProcessTonemapping::None, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Post Process Grey Scale", bool, settings_.postProcess_.greyScale_, MarkSettingsDirty, false, AM_DEFAULT);
}

void RenderPipeline::ApplyAttributes()
{
}

void RenderPipeline::ValidateSettings()
{
    settings_.shadowAtlasPageSize_ = ea::min(settings_.shadowAtlasPageSize_, Graphics::GetCaps().maxRenderTargetSize_);

    if (settings_.deferredLighting_ && !graphics_->GetDeferredSupport()
        && !Graphics::GetReadableDepthStencilFormat())
        settings_.deferredLighting_ = false;

    if (!settings_.use16bitShadowMaps_ && !graphics_->GetHiresShadowMapFormat())
        settings_.use16bitShadowMaps_ = true;

    if (settings_.enableInstancing_ && !graphics_->GetInstancingSupport())
        settings_.enableInstancing_ = false;

    if (settings_.enableInstancing_)
    {
        settings_.firstInstancingTexCoord_ = 4;
        settings_.numInstancingTexCoords_ = 3 +
            (settings_.ambientMode_ == DrawableAmbientMode::Constant
            ? 0
            : settings_.ambientMode_ == DrawableAmbientMode::Flat
            ? 1
            : 7);
    }
}

void RenderPipeline::ApplySettings()
{
    sceneProcessor_->SetSettings(settings_);
    instancingBuffer_->SetSettings(settings_);
    shadowMapAllocator_->SetSettings(settings_);

    if (!opaquePass_ || settings_.deferredLighting_ != deferred_.has_value())
    {
        if (settings_.deferredLighting_)
        {
            opaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
                DrawableProcessorPassFlag::HasAmbientLighting | DrawableProcessorPassFlag::DeferredLightMaskToStencil,
                "deferred", "base", "litbase", "light");

            deferred_ = DeferredLightingData{};
            deferred_->albedoBuffer_ = renderBufferManager_->CreateColorBuffer({ Graphics::GetRGBAFormat() });
            deferred_->specularBuffer_ = renderBufferManager_->CreateColorBuffer({ Graphics::GetRGBAFormat() });
            deferred_->normalBuffer_ = renderBufferManager_->CreateColorBuffer({ Graphics::GetRGBAFormat() });
        }
        else
        {
            opaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
                DrawableProcessorPassFlag::HasAmbientLighting,
                "", "base", "litbase", "light");

            deferred_ = ea::nullopt;
        }
    }

    sceneProcessor_->SetPasses({ opaquePass_, alphaPass_, postOpaquePass_ });

    postProcessPasses_.clear();

    switch (settings_.postProcess_.antialiasing_)
    {
    case PostProcessAntialiasing::FXAA2:
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite | PostProcessPassFlag::NeedColorOutputBilinear,
            BLEND_REPLACE, "v2/PP_FXAA2", "");
        pass->AddShaderParameter("FXAAParams", Vector3(0.4f, 0.5f, 0.75f));
        postProcessPasses_.push_back(pass);
        break;
    }
    case PostProcessAntialiasing::FXAA3:
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite | PostProcessPassFlag::NeedColorOutputBilinear,
            BLEND_REPLACE, "v2/PP_FXAA3", "FXAA_QUALITY_PRESET=12");
        postProcessPasses_.push_back(pass);
        break;
    }
    default:
        break;
    }

    switch (settings_.postProcess_.tonemapping_)
    {
    case PostProcessTonemapping::ReinhardEq3:
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite,
            BLEND_REPLACE, "v2/PP_Tonemap", "REINHARDEQ3");
        pass->AddShaderParameter("TonemapExposureBias", 1.0f);
        postProcessPasses_.push_back(pass);
        break;
    }
    case PostProcessTonemapping::ReinhardEq4:
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite,
            BLEND_REPLACE, "v2/PP_Tonemap", "REINHARDEQ4");
        pass->AddShaderParameter("TonemapExposureBias", 1.0f);
        pass->AddShaderParameter("TonemapMaxWhite", 8.0f);
        postProcessPasses_.push_back(pass);
        break;
    }
    case PostProcessTonemapping::Uncharted2:
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite,
            BLEND_REPLACE, "v2/PP_Tonemap", "UNCHARTED2");
        pass->AddShaderParameter("TonemapExposureBias", 1.0f);
        pass->AddShaderParameter("TonemapMaxWhite", 4.0f);
        postProcessPasses_.push_back(pass);
        break;
    }
    default:
        break;
    }

    if (settings_.postProcess_.greyScale_)
    {
        auto pass = MakeShared<SimplePostProcessPass>(this, renderBufferManager_,
            PostProcessPassFlag::NeedColorOutputReadAndWrite,
            BLEND_REPLACE, "v2/PP_GreyScale", "");
        postProcessPasses_.push_back(pass);
    }
}

bool RenderPipeline::Define(RenderSurface* renderTarget, Viewport* viewport)
{
    // Lazy initialize heavy objects
    if (!sceneProcessor_)
    {
        renderBufferManager_ = MakeShared<RenderBufferManager>(this);
        shadowMapAllocator_ = MakeShared<ShadowMapAllocator>(context_);
        instancingBuffer_ = MakeShared<InstancingBuffer>(context_);
        sceneProcessor_ = MakeShared<SceneProcessor>(this, "shadow", shadowMapAllocator_, instancingBuffer_);

        alphaPass_ = sceneProcessor_->CreatePass<BackToFrontScenePass>(
            DrawableProcessorPassFlag::HasAmbientLighting, "", "alpha", "alpha", "litalpha");
        postOpaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(DrawableProcessorPassFlag::None, "postopaque");
    }

    frameInfo_.viewport_ = viewport;
    frameInfo_.renderTarget_ = renderTarget;
    frameInfo_.viewportRect_ = viewport->GetEffectiveRect(renderTarget);
    frameInfo_.viewportSize_ = frameInfo_.viewportRect_.Size();

    if (!sceneProcessor_->Define(frameInfo_))
        return false;

    sceneProcessor_->SetRenderCamera(viewport->GetCamera());

    //settings_.deferredLighting_ = true;
    //settings_.enableInstancing_ = GetSubsystem<Renderer>()->GetDynamicInstancing();
    //settings_.ambientMode_ = DrawableAmbientMode::Directional;

    if (settingsDirty_)
    {
        settingsDirty_ = false;
        ValidateSettings();
        ApplySettings();
    }

    return true;
}

void RenderPipeline::Update(const FrameInfo& frameInfo)
{
    frameInfo_.frameNumber_ = frameInfo.frameNumber_;
    frameInfo_.timeStep_ = frameInfo.timeStep_;

    // Begin update. Should happen before pipeline state hash check.
    shadowMapAllocator_->ResetAllShadowMaps();
    OnUpdateBegin(this, frameInfo_);

    // Invalidate pipeline states if necessary
    MarkPipelineStateHashDirty();
    const unsigned pipelineStateHash = GetPipelineStateHash();
    if (oldPipelineStateHash_ != pipelineStateHash)
    {
        oldPipelineStateHash_ = pipelineStateHash;
        OnPipelineStatesInvalidated(this);
    }

    sceneProcessor_->Update();

    OnUpdateEnd(this, frameInfo_);
}

void RenderPipeline::Render()
{
    PostProcessPassFlags postProcessFlags;
    for (PostProcessPass* postProcessPass : postProcessPasses_)
        postProcessFlags |= postProcessPass->GetExecutionFlags();
    const bool needColorOutputBilinear = postProcessFlags.Test(PostProcessPassFlag::NeedColorOutputBilinear);
    const bool needColorOutputReadAndWrite = postProcessFlags.Test(PostProcessPassFlag::NeedColorOutputReadAndWrite);

    RenderBufferParams viewportParams{ Graphics::GetRGBFormat() };

    ViewportRenderBufferFlags viewportFlags;
    viewportFlags.Assign(ViewportRenderBufferFlag::SupportOutputColorReadWrite, needColorOutputReadAndWrite);
    viewportFlags.Assign(ViewportRenderBufferFlag::InheritBilinearFiltering, !needColorOutputBilinear);
    viewportParams.flags_.Assign(RenderBufferFlag::sRGB, settings_.gammaCorrection_);
    viewportParams.flags_.Assign(RenderBufferFlag::BilinearFiltering, needColorOutputBilinear);

    if (settings_.deferredLighting_)
    {
        viewportFlags |= ViewportRenderBufferFlag::UsableWithMultipleRenderTargets;
        viewportFlags |= ViewportRenderBufferFlag::IsReadableDepth;
        viewportFlags |= ViewportRenderBufferFlag::HasStencil;
    }
    else
    {
        viewportFlags |= ViewportRenderBufferFlag::InheritMultiSampleLevel;
    }
    renderBufferManager_->RequestViewport(viewportFlags, viewportParams);

    OnRenderBegin(this, frameInfo_);

    // TODO(renderer): Do something about this hack
    graphics_->SetVertexBuffer(nullptr);

    sceneProcessor_->RenderShadowMaps();

    auto sceneBatchRenderer_ = sceneProcessor_->GetBatchRenderer();
    const Color fogColor = sceneProcessor_->GetFrameInfo().camera_->GetEffectiveFogColor();
    auto drawQueue = renderer_->GetDefaultDrawQueue();

    // TODO(renderer): Remove this guard
#ifdef DESKTOP_GRAPHICS
    if (settings_.deferredLighting_)
    {
        // Draw deferred GBuffer
        renderBufferManager_->ClearColor(deferred_->albedoBuffer_, Color::TRANSPARENT_BLACK);
        renderBufferManager_->ClearColor(deferred_->specularBuffer_, Color::TRANSPARENT_BLACK);
        renderBufferManager_->ClearOutput(fogColor, 1.0f, 0);

        RenderBuffer* const gBuffer[] = {
            renderBufferManager_->GetColorOutput(),
            deferred_->albedoBuffer_,
            deferred_->specularBuffer_,
            deferred_->normalBuffer_
        };
        renderBufferManager_->SetRenderTargets(renderBufferManager_->GetDepthStencilOutput(), gBuffer);

        drawQueue->Reset();

        instancingBuffer_->Begin();
        sceneBatchRenderer_->RenderBatches({ *drawQueue, *sceneProcessor_->GetFrameInfo().camera_ },
            opaquePass_->GetDeferredRenderFlags(), opaquePass_->GetSortedDeferredBatches());
        instancingBuffer_->End();

        drawQueue->Execute();

        // Draw deferred lights
        const ShaderResourceDesc geometryBuffer[] = {
            { TU_DIFFUSE, deferred_->albedoBuffer_->GetTexture() },
            { TU_SPECULAR, deferred_->specularBuffer_->GetTexture() },
            { TU_NORMAL, deferred_->normalBuffer_->GetTexture() },
            { TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture() }
        };

        const ShaderParameterDesc cameraParameters[] = {
            { VSP_GBUFFEROFFSETS, renderBufferManager_->GetOutputClipToUVSpaceOffsetAndScale() },
            { PSP_GBUFFERINVSIZE, renderBufferManager_->GetInvOutputSize() },
        };

        BatchRenderingContext ctx{ *drawQueue, *sceneProcessor_->GetFrameInfo().camera_ };
        ctx.globalResources_ = geometryBuffer;
        ctx.cameraParameters_ = cameraParameters;

        renderBufferManager_->SetOutputRenderTargers();

        drawQueue->Reset();
        sceneBatchRenderer_->RenderLightVolumeBatches(ctx, sceneProcessor_->GetLightVolumeBatches());
        drawQueue->Execute();
    }
    else
#endif
    {
        renderBufferManager_->ClearOutput(fogColor, 1.0f, 0);
        renderBufferManager_->SetOutputRenderTargers();
    }

    drawQueue->Reset();
    instancingBuffer_->Begin();
    BatchRenderingContext ctx{ *drawQueue, *sceneProcessor_->GetFrameInfo().camera_ };
    sceneBatchRenderer_->RenderBatches(ctx, opaquePass_->GetBaseRenderFlags(), opaquePass_->GetSortedBaseBatches());
    sceneBatchRenderer_->RenderBatches(ctx, opaquePass_->GetLightRenderFlags(), opaquePass_->GetSortedLightBatches());
    sceneBatchRenderer_->RenderBatches(ctx, alphaPass_->GetRenderFlags(), alphaPass_->GetSortedBatches());
    sceneBatchRenderer_->RenderBatches(ctx, postOpaquePass_->GetBaseRenderFlags(), postOpaquePass_->GetSortedBaseBatches());
    instancingBuffer_->End();
    drawQueue->Execute();

    for (PostProcessPass* postProcessPass : postProcessPasses_)
        postProcessPass->Execute();

    auto debug = sceneProcessor_->GetFrameInfo().octree_->GetComponent<DebugRenderer>();
    if (debug && debug->IsEnabledEffective() && debug->HasContent())
    {
        renderBufferManager_->SetOutputRenderTargers();
        debug->SetView(sceneProcessor_->GetFrameInfo().camera_);
        debug->Render();
    }

    OnRenderEnd(this, frameInfo_);
    graphics_->SetColorWrite(true);
}

unsigned RenderPipeline::RecalculatePipelineStateHash() const
{
    unsigned hash = 0;
    CombineHash(hash, sceneProcessor_->GetCameraProcessor()->GetPipelineStateHash());
    CombineHash(hash, settings_.CalculatePipelineStateHash());
    return hash;
}

}