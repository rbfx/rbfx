// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../RenderPipeline/PipelineBatchSortKey.h"

namespace Urho3D
{

class BatchCompositor;
class BatchRenderer;
class CameraProcessor;
class Drawable;
class DrawableProcessor;
class DrawCommandQueue;
class InstancingBuffer;
class PipelineStateBuilder;
class RenderPipelineInterface;
class RenderSurface;
class ScenePass;
class ShadowMapAllocator;
class Viewport;
struct ShaderParameterDesc;
struct ShaderResourceDesc;

/// Scene processor for RenderPipeline
class URHO3D_API SceneProcessor : public Object, public LightProcessorCallback
{
    URHO3D_OBJECT(SceneProcessor, Object);

public:
    SceneProcessor(RenderPipelineInterface* renderPipeline, const ea::string& shadowTechnique,
        ShadowMapAllocator* shadowMapAllocator, InstancingBuffer* instancingBuffer);
    ~SceneProcessor() override;

    /// Setup. Slow, called rarely.
    /// @{
    /// It's okay to pass null passes in vector.
    void SetPasses(ea::vector<SharedPtr<ScenePass>> passes);
    void SetSettings(const ShaderProgramCompositorSettings& settings);
    /// @}

    /// Create scene passes
    /// @{
    template <class T, class ... Args> SharedPtr<T> CreatePass(Args&& ... args)
    {
        return MakeShared<T>(renderPipeline_, drawableProcessor_, batchStateCacheCallback_, ea::forward<Args>(args)...);
    }
    /// @}

    /// Called on every frame.
    /// @{
    bool Define(const CommonFrameInfo& frameInfo);
    void SetRenderCamera(Camera* camera);
    void SetRenderCameras(ea::span<Camera* const> cameras);

    virtual void Update();
    void PrepareInstancingBuffer();
    void PrepareDrawablesBeforeRendering();
    void RenderShadowMaps();
    void RenderSceneBatches(ea::string_view debugName, Camera* camera,
        const PipelineBatchGroup<PipelineBatchByState>& batchGroup,
        ea::span<const ShaderResourceDesc> globalResources = {},
        ea::span<const ShaderParameterDesc> cameraParameters = {},
        ea::span<const ShaderParameterDesc> frameParameters = {}, unsigned instanceMultiplier = 1u);
    void RenderSceneBatches(ea::string_view debugName, Camera* camera,
        const PipelineBatchGroup<PipelineBatchBackToFront>& batchGroup,
        ea::span<const ShaderResourceDesc> globalResources = {},
        ea::span<const ShaderParameterDesc> cameraParameters = {},
        ea::span<const ShaderParameterDesc> frameParameters = {}, unsigned instanceMultipler = 1u);
    void RenderLightVolumeBatches(ea::string_view debugName, Camera* camera,
        ea::span<const ShaderResourceDesc> globalResources, ea::span<const ShaderParameterDesc> cameraParameters,
        ea::span<const ShaderParameterDesc> frameParameters = {}, unsigned instanceMultiplier = 1u);
    /// @}

    /// Getters
    /// @{
    const SceneProcessorSettings& GetSettings() const { return settings_; }
    const FrameInfo& GetFrameInfo() const { return frameInfo_; }
    /// Takes pass object from BatchStateCreateContext.
    /// Returns user-configured pass (inherited from BatchCompositorPass) if possible.
    /// Returns nullptr if internal pass from BatchCompositor itself (e.g. shadow or light volume pass).
    BatchCompositorPass* GetUserPass(Object* pass) const;
    const auto& GetLightVolumeBatches() const { return batchCompositor_->GetLightVolumeBatches(); }
    CameraProcessor* GetCameraProcessor() const { return cameraProcessor_; }
    PipelineStateBuilder* GetPipelineStateBuilder() const { return pipelineStateBuilder_; }
    DrawableProcessor* GetDrawableProcessor() const { return drawableProcessor_; }
    BatchCompositor* GetBatchCompositor() const { return batchCompositor_; }
    BatchRenderer* GetBatchRenderer() const { return batchRenderer_; }
    /// @}

protected:
    virtual void DrawOccluders();

private:
    /// Callbacks from RenderPipeline
    /// @{
    void OnUpdateBegin(const CommonFrameInfo& frameInfo);
    void OnRenderBegin(const CommonFrameInfo& frameInfo);
    void OnRenderEnd(const CommonFrameInfo& frameInfo);
    /// @}

    /// LightProcessorCallback implementation
    /// @{
    bool IsLightShadowed(Light* light) override;
    unsigned GetShadowMapSize(Light* light, unsigned numActiveSplits) const override;
    ShadowMapRegion AllocateTransientShadowMap(const IntVector2& size) override;
    /// @}

    template <class T>
    void RenderBatchesInternal(ea::string_view debugName, Camera* camera, const PipelineBatchGroup<T>& batchGroup,
        ea::span<const ShaderResourceDesc> globalResources, ea::span<const ShaderParameterDesc> cameraParameters,
        ea::span<const ShaderParameterDesc> frameParameters, unsigned instanceMultiplier);

protected:
    Graphics* graphics_{};
    RenderDevice* renderDevice_{};
    RenderContext* renderContext_{};
    RenderPipelineInterface* renderPipeline_{};
    RenderPipelineDebugger* debugger_{};

    /// Shared objects
    /// @{
    SharedPtr<ShadowMapAllocator> shadowMapAllocator_;
    SharedPtr<InstancingBuffer> instancingBuffer_;
    SharedPtr<DrawCommandQueue> drawQueue_;
    /// @}

    /// Owned objects
    /// @{
    SharedPtr<CameraProcessor> cameraProcessor_;
    SharedPtr<PipelineStateBuilder> pipelineStateBuilder_;
    SharedPtr<DrawableProcessor> drawableProcessor_;
    SharedPtr<BatchCompositor> batchCompositor_;
    SharedPtr<BatchRenderer> batchRenderer_;
    SharedPtr<OcclusionBuffer> occlusionBuffer_;
    BatchStateCacheCallback* batchStateCacheCallback_{};
    /// @}

    SceneProcessorSettings settings_;
    ea::vector<SharedPtr<ScenePass>> passes_;

    FrameInfo frameInfo_;
    bool flipCameraForRendering_{};

    OcclusionBuffer* currentOcclusionBuffer_{};
    ea::vector<Drawable*> occluders_;
    ea::vector<Drawable*> drawables_;
};

}
