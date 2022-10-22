#include "../RenderPipeline/StereoRenderPipeline.h"

#include "../RenderPipeline/BatchRenderer.h"
#include "../Graphics/Drawable.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Input/Input.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/OctreeQuery.h"
#include "../Graphics/Renderer.h"
#include "../Scene/Scene.h"
#include "../RenderPipeline/ShadowMapAllocator.h"
#if URHO3D_SYSTEMUI
    #include "../SystemUI/SystemUI.h"
#endif
#include "../Graphics/Viewport.h"
#include "../Graphics/DebugRenderer.h"

#include "../RenderPipeline/AutoExposurePass.h"
#include "../RenderPipeline/BloomPass.h"
#include "../RenderPipeline/ToneMappingPass.h"

#include "../XR/XR.h"

#include "../DebugNew.h"

namespace Urho3D
{

class StereoFrustumOctreeQuery : public OctreeQuery
{
public:
    StereoFrustumOctreeQuery(ea::vector<Drawable*>& result, ea::array<Frustum, 2> frustums, DrawableFlags drawableFlags, unsigned viewMask) :
        OctreeQuery(result, drawableFlags, viewMask)
    {
        frustums_[0] = frustums[0];
        frustums_[1] = frustums[1];
    }

    virtual Intersection TestOctant(const BoundingBox& box, bool inside) override
    {
        if (inside)
            return INSIDE;

        Intersection inter = frustums_[0].IsInside(box);
        inter = Max(inter, frustums_[1].IsInside(box));
        return inter;
    }
    
    virtual void TestDrawables(Drawable** start, Drawable** end, bool inside)
    {
        while (start != end)
        {
            Drawable* drawable = *start++;
            if ((drawable->GetDrawableFlags() & drawableFlags_) && (drawable->GetViewMask() & viewMask_))
            {
                const auto bb = drawable->GetWorldBoundingBox();
                if (inside || frustums_[0].IsInsideFast(bb) || frustums_[1].IsInsideFast(bb))
                    result_.push_back(drawable);
            }
        }
    }

    Frustum frustums_[2];
};

class StereoOccluderOctreeQuery : public StereoFrustumOctreeQuery
{
public:
    /// Construct with frustum and query parameters.
    StereoOccluderOctreeQuery(ea::vector<Drawable*>& result, ea::array<Frustum,2> frustums, unsigned viewMask = DEFAULT_VIEWMASK) :
        StereoFrustumOctreeQuery(result, frustums, DRAWABLE_GEOMETRY, viewMask)
    {
    }

    /// Intersection test for drawables.
    void TestDrawables(Drawable** start, Drawable** end, bool inside) override
    {
        while (start != end)
        {
            Drawable* drawable = *start++;

            const DrawableFlags flags = drawable->GetDrawableFlags();
            if (flags == DRAWABLE_GEOMETRY && drawable->IsOccluder() && (drawable->GetViewMask() & viewMask_))
            {
                const auto bb = drawable->GetWorldBoundingBox();
                if (inside || frustums_[0].IsInsideFast(bb) || frustums_[1].IsInsideFast(bb))
                    result_.push_back(drawable);
            }
        }
    }
};

class StereoOccludedFrustumOctreeQuery : public StereoFrustumOctreeQuery
{
public:
    /// Construct with frustum, occlusion buffer and query parameters.
    StereoOccludedFrustumOctreeQuery(ea::vector<Drawable*>& result, ea::array<Frustum,2> frustums, ea::array<OcclusionBuffer*, 2> buffers,
        DrawableFlags drawableFlags = DRAWABLE_ANY, unsigned viewMask = DEFAULT_VIEWMASK) :
        StereoFrustumOctreeQuery(result, frustums, drawableFlags, viewMask)
    {
        buffers_[0] = buffers[0];
        buffers_[1] = buffers[1];
    }

    /// Intersection test for an octant.
    Intersection TestOctant(const BoundingBox& box, bool inside) override
    {
        if (inside)
            return buffers_[0]->IsVisible(box) || buffers_[1]->IsVisible(box) ? INSIDE : OUTSIDE;
        else
        {
            Intersection result = frustums_[0].IsInside(box);
            result = Max(result, frustums_[1].IsInside(box));
            if (result != OUTSIDE && !buffers_[0]->IsVisible(box) && !buffers_[1]->IsVisible(box))
                result = OUTSIDE;
            return result;
        }
    }

    /// Intersection test for drawables. Note: drawable occlusion is performed later in worker threads.
    void TestDrawables(Drawable** start, Drawable** end, bool inside) override
    {
        while (start != end)
        {
            Drawable* drawable = *start++;

            if ((drawable->GetDrawableFlags() & drawableFlags_) && (drawable->GetViewMask() & viewMask_))
            {
                const auto bb = drawable->GetWorldBoundingBox();
                if (inside || frustums_[0].IsInsideFast(bb) || frustums_[1].IsInsideFast(bb))
                    result_.push_back(drawable);
            }
        }
    }

    /// Occlusion buffer.
    OcclusionBuffer* buffers_[2];
};



class StereoSceneProcessor : public SceneProcessor
{
    URHO3D_OBJECT(StereoSceneProcessor, SceneProcessor)
public:
    StereoSceneProcessor(RenderPipelineInterface* renderPipeInterface, ShadowMapAllocator* shadowAlloc, InstancingBuffer* instBuffer) :
        SceneProcessor(renderPipeInterface, "shadow", shadowAlloc, instBuffer)
    {

    }

    virtual ~StereoSceneProcessor()
    {

    }

    virtual void Update() override
    {
        // Collect occluders
        currentOcclusionBuffer_ = nullptr;
        currentOcclusionBuffers_ = { nullptr, nullptr };

        assert(frameInfo_.additionalCameras_[0] != nullptr);

        // get our frustums
        ea::array<Frustum, 2> frustums = { frameInfo_.camera_->GetFrustum(), frameInfo_.additionalCameras_[1]->GetFrustum() };
        Camera* cameras[2] = { frameInfo_.camera_, frameInfo_.additionalCameras_[1] };

        if (settings_.maxOccluderTriangles_ > 0)
        {
            URHO3D_PROFILE("ProcessOccluders");

            StereoOccluderOctreeQuery occluderQuery(occluders_, frustums, frameInfo_.camera_->GetViewMask());
            frameInfo_.octree_->GetDrawables(occluderQuery);
            drawableProcessor_->ProcessOccluders(occluders_, settings_.occluderSizeThreshold_);

            if (drawableProcessor_->HasOccluders())
            {
                if (!occlusionBuffer_)
                {
                    occlusionBuffer_ = MakeShared<OcclusionBuffer>(context_);
                    secondaryOcclusion_ = MakeShared<OcclusionBuffer>(context_);
                    eyeOcclusion_ = { occlusionBuffer_, secondaryOcclusion_ };
                }

                static auto CalculateOcclusionBufferSize = [](unsigned size, Camera * cullCamera) -> IntVector2
                {
                    const auto width = static_cast<int>(size);
                    const int height = RoundToInt(size / cullCamera->GetAspectRatio());
                    return { width, height };
                };

                for (unsigned i = 0; i < 2; ++i)
                {
                    auto buff = eyeOcclusion_[i];
                    const IntVector2 bufferSize = CalculateOcclusionBufferSize(settings_.occlusionBufferSize_, cameras[i]);
                    buff->SetSize(bufferSize.x_, bufferSize.y_, settings_.threadedOcclusion_);
                    buff->SetView(frameInfo_.camera_);
                }

                DrawOccluders();
                if (eyeOcclusion_[0]->GetNumTriangles() > 0 || eyeOcclusion_[1]->GetNumTriangles() > 0)
                    currentOcclusionBuffers_ = { eyeOcclusion_[0], eyeOcclusion_[1] };
            }
        }

        // Collect visible drawables
        if (currentOcclusionBuffers_[0])
        {
            URHO3D_PROFILE("QueryVisibleDrawables");
            StereoOccludedFrustumOctreeQuery query(drawables_, frustums,
                currentOcclusionBuffers_, DRAWABLE_GEOMETRY | DRAWABLE_LIGHT, frameInfo_.camera_->GetViewMask());
            frameInfo_.octree_->GetDrawables(query);
        }
        else
        {
            URHO3D_PROFILE("QueryVisibleDrawables");
            StereoFrustumOctreeQuery drawableQuery(drawables_, frustums,
                DRAWABLE_GEOMETRY | DRAWABLE_LIGHT, frameInfo_.camera_->GetViewMask());
            frameInfo_.octree_->GetDrawables(drawableQuery);
        }

        // Process drawables
        drawableProcessor_->ProcessVisibleDrawables(drawables_, currentOcclusionBuffer_ ? ea::span(&currentOcclusionBuffer_, 1u) : ea::span<OcclusionBuffer*>());
        drawableProcessor_->ProcessLights(this);
        drawableProcessor_->ProcessForwardLighting();

        batchCompositor_->ComposeSceneBatches();
        if (settings_.enableShadows_)
            batchCompositor_->ComposeShadowBatches();
    }

protected:
    virtual void DrawOccluders() override
    {
        const auto& activeOccluders = drawableProcessor_->GetOccluders();

        for (auto& occlusionBuffer : eyeOcclusion_)
        {
            if (!occlusionBuffer)
                continue;

            occlusionBuffer->SetMaxTriangles(settings_.maxOccluderTriangles_);
            occlusionBuffer->Clear();

            if (!occlusionBuffer->IsThreaded())
            {
                // If not threaded, draw occluders one by one and test the next occluder against already rasterized depth
                for (unsigned i = 0; i < activeOccluders.size(); ++i)
                {
                    Drawable* occluder = activeOccluders[i].drawable_;
                    if (i > 0)
                    {
                        // For subsequent occluders, do a test against the pixel-level occlusion buffer to see if rendering is necessary
                        if (!occlusionBuffer->IsVisible(occluder->GetWorldBoundingBox()))
                            continue;
                    }

                    // Check for running out of triangles
                    bool success = occluder->DrawOcclusion(occlusionBuffer);
                    // Draw triangles submitted by this occluder
                    occlusionBuffer->DrawTriangles();
                    if (!success)
                        break;
                }
            }
            else
            {
                // In threaded mode submit all triangles first, then render (cannot test in this case)
                for (unsigned i = 0; i < activeOccluders.size(); ++i)
                {
                    // Check for running out of triangles
                    if (!activeOccluders[i].drawable_->DrawOcclusion(occlusionBuffer))
                        break;
                }

                occlusionBuffer->DrawTriangles();
            }

            // Finally build the depth mip levels
            occlusionBuffer->BuildDepthHierarchy();
        }
    }

    SharedPtr<OcclusionBuffer> secondaryOcclusion_;
    ea::array<OcclusionBuffer*, 2> eyeOcclusion_{};
    ea::array<OcclusionBuffer*, 2> currentOcclusionBuffers_{};
};

StereoRenderPipelineView::StereoRenderPipelineView(RenderPipeline* pipeline) :
    RenderPipelineView(pipeline)
{
    auto settings = renderPipeline_->GetSettings();
    settings.sceneProcessor_.lightingMode_ = DirectLightingMode::Forward;

    SetSettings(settings);
    renderPipeline_->OnSettingsChanged.Subscribe(this, &StereoRenderPipelineView::SetSettings);
}

StereoRenderPipelineView::~StereoRenderPipelineView()
{
}

void StereoRenderPipelineView::SetSettings(const RenderPipelineSettings& settings)
{
    settings_ = settings;
    settings_.Validate();
    settings_.AdjustToSupported(context_);
    settings_.PropagateImpliedSettings();
    
    settingsHash_ = settings_.CalculatePipelineStateHash();
    settingsDirty_ = true;
}

void StereoRenderPipelineView::ApplySettings()
{
    sceneProcessor_->SetSettings(settings_);
    instancingBuffer_->SetSettings(settings_.instancingBuffer_);
    shadowMapAllocator_->SetSettings(settings_.shadowMapAllocator_);

    if (settings_.sceneProcessor_.depthPrePass_ && !depthPrePass_)
    {
        depthPrePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(
            DrawableProcessorPassFlag::DepthOnlyPass | DrawableProcessorPassFlag::StereoInstancing, "depth");
    }
    else
    {
        depthPrePass_ = nullptr;
    }

    outlineScenePass_ = sceneProcessor_->CreatePass<OutlineScenePass>(StringVector{ "base", "alpha" }, DrawableProcessorPassFlag::StereoInstancing);

    sceneProcessor_->SetPasses({ depthPrePass_, opaquePass_, postOpaquePass_, alphaPass_, postAlphaPass_, outlineScenePass_ });

    postProcessPasses_.clear();

    if (settings_.renderBufferManager_.colorSpace_ == RenderPipelineColorSpace::LinearHDR)
    {
        auto pass = MakeShared<AutoExposurePass>(this, renderBufferManager_);
        pass->SetSettings(settings_.autoExposure_);
        postProcessPasses_.push_back(pass);
    }

    if (settings_.bloom_.enabled_)
    {
        auto pass = MakeShared<BloomPass>(this, renderBufferManager_);
        pass->SetSettings(settings_.bloom_);
        postProcessPasses_.push_back(pass);
    }

    {
        outlinePostProcessPass_ = MakeShared<OutlinePass>(this, renderBufferManager_);
        postProcessPasses_.push_back(outlinePostProcessPass_);
    }

    if (settings_.renderBufferManager_.colorSpace_ == RenderPipelineColorSpace::LinearHDR)
    {
        auto pass = MakeShared<ToneMappingPass>(this, renderBufferManager_);
        pass->SetMode(settings_.toneMapping_);
        postProcessPasses_.push_back(pass);
    }

    postProcessFlags_ = {};
    for (PostProcessPass* postProcessPass : postProcessPasses_)
        postProcessFlags_ |= postProcessPass->GetExecutionFlags();

    settings_.AdjustForPostProcessing(postProcessFlags_);
    renderBufferManager_->SetSettings(settings_.renderBufferManager_);
}

bool StereoRenderPipelineView::Define(RenderSurface* renderTarget, Viewport* viewport)
{
    URHO3D_PROFILE("SetupRenderPipeline");

    if (!viewport->GetScene())
        return false;

    if (!sceneProcessor_)
    {
        renderBufferManager_ = MakeShared<RenderBufferManager>(this);
        shadowMapAllocator_ = MakeShared<ShadowMapAllocator>(context_);
        instancingBuffer_ = MakeShared<InstancingBuffer>(context_);
        sceneProcessor_ = MakeShared<StereoSceneProcessor>(this, shadowMapAllocator_, instancingBuffer_);

        opaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(DrawableProcessorPassFlag::StereoInstancing | DrawableProcessorPassFlag::HasAmbientLighting, "", "base", "litbase", "light");

        postOpaquePass_ = sceneProcessor_->CreatePass<UnorderedScenePass>(DrawableProcessorPassFlag::StereoInstancing, "postopaque");
        alphaPass_ = sceneProcessor_->CreatePass<BackToFrontScenePass>(
            DrawableProcessorPassFlag::NeedReadableDepth
            | DrawableProcessorPassFlag::RefractionPass | DrawableProcessorPassFlag::StereoInstancing, "", "alpha", "alpha", "litalpha");
        postAlphaPass_ = sceneProcessor_->CreatePass<BackToFrontScenePass>(DrawableProcessorPassFlag::StereoInstancing, "postalpha");
    }

    frameInfo_.viewport_ = viewport;
    frameInfo_.renderTarget_ = renderTarget;
    frameInfo_.viewportRect_ = viewport->GetEffectiveRect(renderTarget);
    frameInfo_.viewportSize_ = frameInfo_.viewportRect_.Size();

    ea::array<Camera*, 2> cameras = { viewport->GetEye(0), viewport->GetEye(1) };
    frameInfo_.cameras_ = cameras;

    if (!sceneProcessor_->Define(frameInfo_))
        return false;

    sceneProcessor_->SetRenderCameras(cameras);

    if (settingsDirty_)
    {
        settingsDirty_ = false;
        ApplySettings();
    }

    return true;
}

void StereoRenderPipelineView::Update(const FrameInfo& frameInfo)
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
    const unsigned pipelineStateHash = settings_.CalculatePipelineStateHash();
    if (settingsHash_ != pipelineStateHash)
    {
        oldPipelineStateHash_ = pipelineStateHash;
        OnPipelineStatesInvalidated(this);
    }

    outlineScenePass_->SetOutlineGroups(sceneProcessor_->GetFrameInfo().scene_);

    sceneProcessor_->Update();

    outlinePostProcessPass_->SetEnabled(outlineScenePass_->IsEnabled() && outlineScenePass_->HasBatches());

    SendViewEvent(E_ENDVIEWUPDATE);
    OnUpdateEnd(this, frameInfo_);
}

void StereoRenderPipelineView::Render()
{
    URHO3D_PROFILE("ExecuteRenderPipeline");

    const bool hasRefraction = alphaPass_->HasRefractionBatches();
    RenderBufferManagerFrameSettings frameSettings;
    frameSettings.supportColorReadWrite_ = postProcessFlags_.Test(PostProcessPassFlag::NeedColorOutputReadAndWrite);
    if (hasRefraction)
        frameSettings.supportColorReadWrite_ = true;

    renderBufferManager_->SetFrameSettings(frameSettings);

    OnRenderBegin(this, frameInfo_);
    SendViewEvent(E_BEGINVIEWRENDER);
    SendViewEvent(E_VIEWBUFFERSREADY);

    // HACK: Graphics may keep expired vertex buffers for some reason, reset it just in case
    graphics_->SetVertexBuffer(nullptr);

    sceneProcessor_->PrepareDrawablesBeforeRendering();
    sceneProcessor_->PrepareInstancingBuffer();

    // shadowmaps, make sure we're single step instancing
    instancingBuffer_->GetVertexBuffer()->ChangeElementStepRate(1u);
    sceneProcessor_->RenderShadowMaps();

    // make sure we clear out vtx buffers because of step-rate change to make sure nothing is sticky
    graphics_->SetVertexBuffer(nullptr);

    // going into pass drawing, make sure we're two step instancing
    instancingBuffer_->GetVertexBuffer()->ChangeElementStepRate(2u);

    Camera* camera = sceneProcessor_->GetFrameInfo().camera_;
    const Color fogColorInGammaSpace = sceneProcessor_->GetFrameInfo().camera_->GetEffectiveFogColor();
    const Color effectiveFogColor = settings_.sceneProcessor_.linearSpaceLighting_
        ? fogColorInGammaSpace.GammaToLinear()
        : fogColorInGammaSpace;
    
    renderBufferManager_->ClearOutput(effectiveFogColor, 1.0f, 0);

    // JS: this currently doesn't work for some reason. DrawEyeMask() causes state to get bungled in some way.
    //auto xr = GetSubsystem<OpenXR>();
    //xr->DrawEyeMask();

    const ShaderParameterDesc cameraParameters[] = {
        {VSP_GBUFFEROFFSETS, renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale()},
        {PSP_GBUFFERINVSIZE, renderBufferManager_->GetInvOutputSize()},
    };

    if (depthPrePass_)
        sceneProcessor_->RenderSceneBatches("DepthPrePass", camera, depthPrePass_->GetBaseBatches(), {}, cameraParameters, 2);

    sceneProcessor_->RenderSceneBatches("OpaqueBase", camera, opaquePass_->GetBaseBatches(), {}, cameraParameters, 2);
    sceneProcessor_->RenderSceneBatches("OpaqueLight", camera, opaquePass_->GetLightBatches(), {}, cameraParameters, 2);
    sceneProcessor_->RenderSceneBatches("PostOpaque", camera, postOpaquePass_->GetBaseBatches(), {}, cameraParameters, 2);

    if (hasRefraction)
        renderBufferManager_->SwapColorBuffers(true);

#ifdef DESKTOP_GRAPHICS
    ShaderResourceDesc depthAndColorTextures[] = {
        { TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture() },
        { TU_EMISSIVE, renderBufferManager_->GetSecondaryColorTexture() },
    };
#else
    ShaderResourceDesc depthAndColorTextures[] = {
        { TU_EMISSIVE, renderBufferManager_->GetSecondaryColorTexture() },
    };
#endif

    sceneProcessor_->RenderSceneBatches("Alpha", camera, alphaPass_->GetBatches(),
        depthAndColorTextures, cameraParameters, 2);
    sceneProcessor_->RenderSceneBatches("PostAlpha", camera, postAlphaPass_->GetBatches(), {}, {}, 2);

    if (outlinePostProcessPass_->IsEnabled())
    {
        // TODO: Do we want it dynamic?
        const unsigned outlinePadding = 2;

        RenderBuffer* const renderTargets[] = { outlinePostProcessPass_->GetColorOutput() };
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

    // going into post-process, switch back to single step instancing
    instancingBuffer_->GetVertexBuffer()->ChangeElementStepRate(1u);
    // make sure we clear out vtx buffers because of step-rate change to make sure nothing is sticky
    graphics_->SetVertexBuffer(nullptr);

    // Not going to work, something will need to be done
    //for (PostProcessPass* postProcessPass : postProcessPasses_)
    //    postProcessPass->Execute();

    // draw debug geometry into each half
    auto debug = sceneProcessor_->GetFrameInfo().scene_->GetComponent<DebugRenderer>();
    if (settings_.drawDebugGeometry_ && debug && debug->IsEnabledEffective() && debug->HasContent())
    {
        renderBufferManager_->SetOutputRenderTargers();
        const auto outSize = renderBufferManager_->GetOutputSize();
        for (int i = 0; i < 2; ++i)
        {
            graphics_->SetViewport(i == 0 ? IntRect(0, 0, outSize.x_ / 2, outSize.y_) : IntRect(outSize.x_ / 2, 0, outSize.x_ / 2, outSize.y_));
            debug->SetView(frameInfo_.viewport_->GetEye(i));
            debug->Render();
        }
    }

    SendViewEvent(E_ENDVIEWRENDER);
    OnRenderEnd(this, frameInfo_);
    graphics_->SetColorWrite(true);

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

    graphics_->ResetRenderTargets();
}

const FrameInfo& StereoRenderPipelineView::GetFrameInfo() const
{
    static const FrameInfo defaultFrameInfo;
    return sceneProcessor_ ? sceneProcessor_->GetFrameInfo() : defaultFrameInfo;
}

const RenderPipelineStats& StereoRenderPipelineView::GetStats() const
{
    return stats_;
}

void StereoRenderPipelineView::SendViewEvent(StringHash eventType)
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

StereoRenderPipeline::StereoRenderPipeline(Context* context) :
    BaseClassName(context)
{

}

StereoRenderPipeline::~StereoRenderPipeline()
{

}

void StereoRenderPipeline::RegisterObject(Context* context)
{
    context->RegisterFactory<StereoRenderPipeline>();
    URHO3D_COPY_BASE_ATTRIBUTES(RenderPipeline);
}


SharedPtr<RenderPipelineView> StereoRenderPipeline::Instantiate()
{
    return MakeShared<StereoRenderPipelineView>(this);
}

}
