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

#include "../RenderPipeline/DefaultRenderPipeline.h"

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/OutlineGroup.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Viewport.h"
#include "../Input/Input.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/BatchRenderer.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/InstancingBuffer.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/OutlineScenePass.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../RenderPipeline/ShadowMapAllocator.h"
#include "../RenderPipeline/Passes/OutlineRenderPass.h"
#include "../Scene/Scene.h"
#if URHO3D_SYSTEMUI
    #include "../SystemUI/SystemUI.h"
#endif

#include "../DebugNew.h"

namespace Urho3D
{

DefaultRenderPipelineView::DefaultRenderPipelineView(RenderPipeline* renderPipeline)
    : RenderPipelineView(renderPipeline)
{
    SetSettings(renderPipeline_->GetSettings());
    SetRenderPath(renderPipeline_->GetRenderPath());
    MarkParametersDirty();

    renderPipeline_->OnSettingsChanged.Subscribe(this, &DefaultRenderPipelineView::SetSettings);
    renderPipeline_->OnRenderPathChanged.Subscribe(this, &DefaultRenderPipelineView::SetRenderPath);
    renderPipeline_->OnParametersChanged.Subscribe(this, &DefaultRenderPipelineView::MarkParametersDirty);
}

DefaultRenderPipelineView::~DefaultRenderPipelineView()
{
}

const FrameInfo& DefaultRenderPipelineView::GetFrameInfo() const
{
    static const FrameInfo defaultFrameInfo;
    return sceneProcessor_ ? sceneProcessor_->GetFrameInfo() : defaultFrameInfo;
}

void DefaultRenderPipelineView::SetSettings(const RenderPipelineSettings& settings)
{
    settings_ = settings;
    settings_.Validate();
    settings_.AdjustToSupported(context_);
    settings_.PropagateImpliedSettings();
    settingsDirty_ = true;
    settingsPipelineStateHash_ = settings_.CalculatePipelineStateHash();
}

void DefaultRenderPipelineView::SetRenderPath(RenderPath* renderPath)
{
    originalRenderPath_ = renderPath;
    renderPath_ = nullptr;
}

void DefaultRenderPipelineView::SendViewEvent(StringHash eventType)
{
    Texture* parentTexture = frameInfo_.renderTarget_ ? frameInfo_.renderTarget_->GetParentTexture() : nullptr;

    using namespace BeginViewRender;

    VariantMap& eventData = GetEventDataMap();

    eventData[P_RENDERPIPELINEVIEW] = this;
    eventData[P_SURFACE] = frameInfo_.renderTarget_;
    eventData[P_TEXTURE] = parentTexture;
    eventData[P_SCENE] = sceneProcessor_->GetFrameInfo().scene_;
    eventData[P_CAMERA] = sceneProcessor_->GetFrameInfo().camera_;

    Object* sender = parentTexture ? static_cast<Object*>(parentTexture) : renderer_;
    sender->SendEvent(eventType, eventData);
}

void DefaultRenderPipelineView::ApplySettings()
{
    sceneProcessor_->SetSettings(settings_);
    instancingBuffer_->SetSettings(settings_.instancingBuffer_);
    shadowMapAllocator_->SetSettings(settings_.shadowMapAllocator_);

    if (settings_.sceneProcessor_.depthPrePass_ && !depthPrePass_)
    {
        depthPrePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
            DrawableProcessorPassFlag::DepthOnlyPass, "depth");
    }
    else
    {
        depthPrePass_ = nullptr;
    }

    if (!opaquePass_ || settings_.sceneProcessor_.IsDeferredLighting() != deferred_.has_value())
    {
        if (settings_.sceneProcessor_.IsDeferredLighting())
        {
            opaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
                DrawableProcessorPassFlag::HasAmbientLighting | DrawableProcessorPassFlag::DeferredLightMaskToStencil,
                "deferred", "base", "litbase", "light");

            deferred_ = DeferredLightingData{};
            deferred_->albedoBuffer_ = renderBufferManager_->CreateColorBuffer({TextureFormat::TEX_FORMAT_RGBA8_UNORM});
            deferred_->specularBuffer_ = renderBufferManager_->CreateColorBuffer({TextureFormat::TEX_FORMAT_RGBA8_UNORM});
            deferred_->normalBuffer_ = renderBufferManager_->CreateColorBuffer({TextureFormat::TEX_FORMAT_RGBA8_UNORM});

            state_.renderBuffers_[SharedRenderPassState::AlbedoBufferId] = deferred_->albedoBuffer_;
            state_.renderBuffers_[SharedRenderPassState::SpecularBufferId] = deferred_->specularBuffer_;
            state_.renderBuffers_[SharedRenderPassState::NormalBufferId] = deferred_->normalBuffer_;
        }
        else
        {
            opaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
                DrawableProcessorPassFlag::HasAmbientLighting,
                "", "base", "litbase", "light");

            deferred_ = ea::nullopt;

            state_.renderBuffers_[SharedRenderPassState::AlbedoBufferId] = nullptr;
            state_.renderBuffers_[SharedRenderPassState::SpecularBufferId] = nullptr;
            state_.renderBuffers_[SharedRenderPassState::NormalBufferId] = nullptr;
        }
    }

    outlineScenePass_ = sceneProcessor_->CreatePass<OutlineScenePass>(StringVector{"deferred", "deferred_decal", "base", "alpha"});

    sceneProcessor_->SetPasses({depthPrePass_, opaquePass_, deferredDecalPass_, postOpaquePass_, alphaPass_, postAlphaPass_, outlineScenePass_});
}

void DefaultRenderPipelineView::UpdateRenderOutputFlags()
{
    renderOutputFlags_ = {};
    if (renderPath_)
    {
        const RenderPassTraits& aggregatedTraits = renderPath_->GetAggregatedPassTraits();
        if (aggregatedTraits.needReadWriteColorBuffer_)
            renderOutputFlags_ |= RenderOutputFlag::NeedColorOutputReadAndWrite;
        if (aggregatedTraits.needBilinearColorSampler_)
            renderOutputFlags_ |= RenderOutputFlag::NeedColorOutputBilinear;
    }

    settings_.AdjustForRenderPath(renderOutputFlags_);
    renderBufferManager_->SetSettings(settings_.renderBufferManager_);
}

bool DefaultRenderPipelineView::Define(RenderSurface* renderTarget, Viewport* viewport)
{
    URHO3D_PROFILE("SetupRenderPipeline");

    if (!viewport->GetScene())
        return false;

    // Ensure that render path is ready
    if (!renderPath_ && originalRenderPath_)
    {
        renderPath_ = originalRenderPath_->Clone();
        renderPath_->InitializeView(this);
        settingsDirty_ = true;
    }

    // Lazy initialize heavy objects
    if (!sceneProcessor_)
    {
        state_.renderPipelineInterface_ = this;
        state_.renderBufferManager_ = MakeShared<RenderBufferManager>(this);

        renderBufferManager_ = state_.renderBufferManager_;
        shadowMapAllocator_ = MakeShared<ShadowMapAllocator>(context_);
        instancingBuffer_ = MakeShared<InstancingBuffer>(context_);
        sceneProcessor_ = MakeShared<SceneProcessor>(this, "shadow", shadowMapAllocator_, instancingBuffer_);

        postOpaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(DrawableProcessorPassFlag::None, "postopaque");
        deferredDecalPass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
            DrawableProcessorPassFlag::HasAmbientLighting | DrawableProcessorPassFlag::NeedReadableDepth,
            "deferred_decal", "", "", "");
        alphaPass_ = sceneProcessor_->CreatePass<BackToFrontScenePass>( //
            DrawableProcessorPassFlag::HasAmbientLighting | DrawableProcessorPassFlag::NeedReadableDepth
                | DrawableProcessorPassFlag::RefractionPass | DrawableProcessorPassFlag::ReadOnlyDepth,
            "", "alpha", "alpha", "litalpha");
        postAlphaPass_ = sceneProcessor_->CreatePass<BackToFrontScenePass>(
            DrawableProcessorPassFlag::ReadOnlyDepth, "postalpha");

        const RenderBufferParams params{TextureFormat::TEX_FORMAT_RGBA8_UNORM, 1, RenderBufferFlag::BilinearFiltering};
        outlineBuffer_ = renderBufferManager_->CreateColorBuffer(params);
        outlineBuffer_->SetEnabled(false);

        state_.renderBuffers_[OutlineRenderPass::ColorBufferId] = outlineBuffer_;
    }

    frameInfo_.viewport_ = viewport;
    frameInfo_.renderTarget_ = renderTarget;
    frameInfo_.viewportRect_ = viewport->GetEffectiveRect(renderTarget);
    frameInfo_.viewportSize_ = frameInfo_.viewportRect_.Size();

    if (!sceneProcessor_->Define(frameInfo_))
        return false;

    sceneProcessor_->SetRenderCamera(viewport->GetCamera());
    state_.renderCamera_ = viewport->GetCamera();

    if (settingsDirty_)
    {
        settingsDirty_ = false;
        parametersDirty_ = true;
        ApplySettings();
        UpdateRenderOutputFlags();
    }

    if (parametersDirty_ && renderPath_)
    {
        parametersDirty_ = false;
        renderPath_->UpdateParameters(
            settings_, renderPipeline_->GetRenderPasses(), renderPipeline_->GetRenderPathParameters());
        UpdateRenderOutputFlags();
    }

    renderBufferManager_->OnViewportDefined(frameInfo_.renderTarget_, frameInfo_.viewportRect_);
    linearColorSpace_ = renderBufferManager_->IsLinearColorSpace();

    const unsigned outputMultiSample = renderBufferManager_->GetOutputMultiSample();
    const TextureFormat outputColorFormat = renderBufferManager_->GetOutputColorFormat();
    const TextureFormat outputDepthFormat = renderBufferManager_->GetOutputDepthStencilFormat();

    const PipelineStateOutputDesc standardOutputDesc{outputDepthFormat, 1, {outputColorFormat}, outputMultiSample};
    const PipelineStateOutputDesc deferredOutputDesc{
        outputDepthFormat, 4, {outputColorFormat, albedoFormat_, specularFormat_, normalFormat_}};

    opaquePass_->SetDeferredOutputDesc(deferredOutputDesc);
    deferredDecalPass_->SetDeferredOutputDesc(deferredOutputDesc);

    opaquePass_->SetForwardOutputDesc(standardOutputDesc);
    if (depthPrePass_)
        depthPrePass_->SetForwardOutputDesc(standardOutputDesc);
    postOpaquePass_->SetForwardOutputDesc(standardOutputDesc);
    alphaPass_->SetForwardOutputDesc(standardOutputDesc);
    postAlphaPass_->SetForwardOutputDesc(standardOutputDesc);

    auto batchCompositor = sceneProcessor_->GetBatchCompositor();
    batchCompositor->SetLightVolumesOutputDesc(standardOutputDesc);
    batchCompositor->SetShadowOutputDesc(shadowMapAllocator_->GetShadowOutputDesc());

    return true;
}

void DefaultRenderPipelineView::Update(const FrameInfo& frameInfo)
{
    URHO3D_PROFILE("UpdateRenderPipeline");

    frameInfo_.frameNumber_ = frameInfo.frameNumber_;
    frameInfo_.timeStep_ = frameInfo.timeStep_;

    // Begin debug snapshot
#if URHO3D_SYSTEMUI
    const bool shiftDown = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);
    const bool ctrlDown = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
    const bool takeSnapshot = shiftDown && ctrlDown && ui::IsKeyPressed(KEY_F12);
#else
    auto input = GetSubsystem<Input>();
    const bool takeSnapshot = input->GetQualifiers().Test(QUAL_CTRL | QUAL_SHIFT) && input->GetKeyPress(KEY_F12);
#endif
    if (takeSnapshot)
        debugger_.BeginSnapshot();

    // Begin update. Should happen before pipeline state hash check.
    shadowMapAllocator_->ResetAllShadowMaps();
    OnUpdateBegin(this, frameInfo_);
    SendViewEvent(E_BEGINVIEWUPDATE);

    // Invalidate pipeline states if necessary
    const unsigned pipelineStateHash = RecalculatePipelineStateHash();
    if (oldPipelineStateHash_ != pipelineStateHash)
    {
        oldPipelineStateHash_ = pipelineStateHash;
        OnPipelineStatesInvalidated(this);
    }

    const FrameInfo& fullFrameInfo = sceneProcessor_->GetFrameInfo();
    const bool drawDebugGeometry = fullFrameInfo.camera_->GetDrawDebugGeometry();
    outlineScenePass_->SetOutlineGroups(fullFrameInfo.scene_, drawDebugGeometry);

    sceneProcessor_->Update();

    outlineBuffer_->SetEnabled(outlineScenePass_->IsEnabled() && outlineScenePass_->HasBatches());

    if (renderPath_)
        renderPath_->Update(state_);

    SendViewEvent(E_ENDVIEWUPDATE);
    OnUpdateEnd(this, frameInfo_);
}

void DefaultRenderPipelineView::Render()
{
    URHO3D_PROFILE("ExecuteRenderPipeline");

    const RenderDeviceCaps& caps = GetSubsystem<RenderDevice>()->GetCaps();
    const bool canReadDepth = settings_.renderBufferManager_.readableDepth_ && caps.readOnlyDepth_;

    const FrameInfo& fullFrameInfo = sceneProcessor_->GetFrameInfo();

    const bool hasRefraction = alphaPass_->HasRefractionBatches();
    RenderBufferManagerFrameSettings frameSettings;
    frameSettings.supportColorReadWrite_ = renderOutputFlags_.Test(RenderOutputFlag::NeedColorOutputReadAndWrite);
    if (hasRefraction)
        frameSettings.supportColorReadWrite_ = true;
    renderBufferManager_->SetFrameSettings(frameSettings);

    OnRenderBegin(this, frameInfo_);
    SendViewEvent(E_BEGINVIEWRENDER);
    SendViewEvent(E_VIEWBUFFERSREADY);

    sceneProcessor_->PrepareDrawablesBeforeRendering();
    sceneProcessor_->PrepareInstancingBuffer();
    sceneProcessor_->RenderShadowMaps();

    Camera* camera = fullFrameInfo.camera_;
    const Color fogColorInGammaSpace = camera->GetEffectiveFogColor();
    const Color effectiveFogColor = linearColorSpace_ ? fogColorInGammaSpace.GammaToLinear() : fogColorInGammaSpace;

    if (settings_.sceneProcessor_.IsDeferredLighting())
    {
        // Draw deferred GBuffer
        renderBufferManager_->ClearColor(deferred_->albedoBuffer_, Color::TRANSPARENT_BLACK);
        renderBufferManager_->ClearColor(deferred_->specularBuffer_, Color::TRANSPARENT_BLACK);
        renderBufferManager_->ClearColor(deferred_->normalBuffer_, Color::TRANSPARENT_BLACK);
        renderBufferManager_->ClearOutput(effectiveFogColor, 1.0f, 0);

        if (depthPrePass_)
            sceneProcessor_->RenderSceneBatches("DepthPrePass", camera, depthPrePass_->GetBaseBatches());

        RenderBuffer* const gBuffer[] = {
            renderBufferManager_->GetColorOutput(),
            deferred_->albedoBuffer_,
            deferred_->specularBuffer_,
            deferred_->normalBuffer_
        };

        renderBufferManager_->SetRenderTargets(renderBufferManager_->GetDepthStencilOutput(), gBuffer);
        sceneProcessor_->RenderSceneBatches("GeometryBuffer", camera, opaquePass_->GetDeferredBatches());
        if (canReadDepth && !deferredDecalPass_->GetDeferredBatches().batches_.empty())
        {
            renderBufferManager_->SetRenderTargets(renderBufferManager_->GetDepthStencilOutput(), gBuffer, true);

            ShaderResourceDesc depthAndColorTextures[] = {
                {ShaderResources::DepthBuffer, renderBufferManager_->GetDepthStencilTexture()},
            };

            sceneProcessor_->RenderSceneBatches("DeferredDecals", camera, deferredDecalPass_->GetDeferredBatches(), depthAndColorTextures);
        }

        // Draw deferred lights
        const ShaderResourceDesc geometryBuffer[] = {
            { ShaderResources::Albedo, deferred_->albedoBuffer_->GetTexture() },
            { ShaderResources::Properties, deferred_->specularBuffer_->GetTexture() },
            { ShaderResources::Normal, deferred_->normalBuffer_->GetTexture() },
            { ShaderResources::DepthBuffer, renderBufferManager_->GetDepthStencilTexture() }
        };
        const ShaderParameterDesc cameraParameters[] = {
            {ShaderConsts::Camera_GBufferOffsets, renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale()},
            {ShaderConsts::Camera_GBufferInvSize, renderBufferManager_->GetInvOutputSize()},
        };

        renderBufferManager_->SetOutputRenderTargets(true);
        sceneProcessor_->RenderLightVolumeBatches("LightVolumes", camera, geometryBuffer, cameraParameters);
        renderBufferManager_->SetOutputRenderTargets();
    }
    else
    {
        renderBufferManager_->ClearOutput(effectiveFogColor, 1.0f, 0);
        renderBufferManager_->SetOutputRenderTargets();

        if (depthPrePass_)
            sceneProcessor_->RenderSceneBatches("DepthPrePass", camera, depthPrePass_->GetBaseBatches());
    }

    const ShaderParameterDesc cameraParameters[] = {
        {VSP_GBUFFEROFFSETS, renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale()},
        {PSP_GBUFFERINVSIZE, renderBufferManager_->GetInvOutputSize()},
    };

    sceneProcessor_->RenderSceneBatches("OpaqueBase", camera, opaquePass_->GetBaseBatches(), {}, cameraParameters);
    sceneProcessor_->RenderSceneBatches("OpaqueLight", camera, opaquePass_->GetLightBatches(), {}, cameraParameters);
    sceneProcessor_->RenderSceneBatches("PostOpaque", camera, postOpaquePass_->GetBaseBatches(), {}, cameraParameters);

    if (hasRefraction)
        renderBufferManager_->SwapColorBuffers(true);

    ShaderResourceDesc depthAndColorTextures[] = {
        {ShaderResources::DepthBuffer, canReadDepth ? renderBufferManager_->GetDepthStencilTexture() : nullptr},
        {ShaderResources::Emission, renderBufferManager_->GetSecondaryColorTexture()},
    };

    if (canReadDepth)
        renderBufferManager_->SetOutputRenderTargets(true);

    sceneProcessor_->RenderSceneBatches("Alpha", camera, alphaPass_->GetBatches(),
        depthAndColorTextures, cameraParameters);
    sceneProcessor_->RenderSceneBatches("PostAlpha", camera, postAlphaPass_->GetBatches());

    if (outlineBuffer_->IsEnabled())
    {
        // TODO: Do we want it dynamic?
        const unsigned outlinePadding = 2;

        RenderBuffer* const renderTargets[] = {outlineBuffer_};
        auto batches = outlineScenePass_->GetBatches();

        batches.scissorRect_ = renderTargets[0]->GetViewportRect();
        if (batches.scissorRect_.Width() > outlinePadding * 2 && batches.scissorRect_.Height() > outlinePadding * 2)
        {
            batches.scissorRect_.left_ += outlinePadding;
            batches.scissorRect_.top_ += outlinePadding;
            batches.scissorRect_.right_ -= outlinePadding;
            batches.scissorRect_.bottom_ -= outlinePadding;
        }

        renderBufferManager_->SetRenderTargets(nullptr, renderTargets);
        renderBufferManager_->ClearColor(renderTargets[0], Color::TRANSPARENT_BLACK);
        sceneProcessor_->RenderSceneBatches("Outline", camera, batches, {}, cameraParameters);
    }

    if (renderPath_)
        renderPath_->Render(state_);

    const bool drawDebugGeometry = settings_.drawDebugGeometry_ && camera->GetDrawDebugGeometry();
    auto debug = fullFrameInfo.scene_->GetComponent<DebugRenderer>();
    if (drawDebugGeometry && debug && debug->IsEnabledEffective() && debug->HasContent())
    {
        renderBufferManager_->SetOutputRenderTargets();
        debug->SetView(camera);
        debug->Render();
    }

    OnRenderEnd(this, frameInfo_);
    SendViewEvent(E_ENDVIEWRENDER);

    // Update statistics
    stats_ = {};
    OnCollectStatistics(this, stats_);

    // End debug snapshot
    if (debugger_.IsSnapshotInProgress())
    {
        debugger_.EndSnapshot();

        const DebugFrameSnapshot& snapshot = debugger_.GetSnapshot();
        URHO3D_LOGINFO("RenderPipeline snapshot:\n\n{}\n", snapshot.ToString());
    }
}

void DefaultRenderPipelineView::DrawDebugGeometries(bool depthTest)
{
    const FrameInfo& fullFrameInfo = sceneProcessor_->GetFrameInfo();
    auto debug = fullFrameInfo.scene_->GetComponent<DebugRenderer>();

    const auto& geometries = sceneProcessor_->GetDrawableProcessor()->GetGeometries();
    for (Drawable* geometry : geometries)
        geometry->DrawDebugGeometry(debug, depthTest);
}

void DefaultRenderPipelineView::DrawDebugLights(bool depthTest)
{
    const FrameInfo& fullFrameInfo = sceneProcessor_->GetFrameInfo();
    auto debug = fullFrameInfo.scene_->GetComponent<DebugRenderer>();

    const auto& lights = sceneProcessor_->GetDrawableProcessor()->GetLights();
    for (Drawable* light : lights)
        light->DrawDebugGeometry(debug, depthTest);
}

unsigned DefaultRenderPipelineView::RecalculatePipelineStateHash() const
{
    unsigned hash = settingsPipelineStateHash_;
    CombineHash(hash, sceneProcessor_->GetCameraProcessor()->GetPipelineStateHash());
    return hash;
}

}
