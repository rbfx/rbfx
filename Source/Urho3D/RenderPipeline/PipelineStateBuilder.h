// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../RenderAPI/PipelineState.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../RenderPipeline/ShaderProgramCompositor.h"

namespace Urho3D
{

class BatchCompositorPass;
class CameraProcessor;
class InstancingBuffer;
class Light;
class PipelineState;
class SceneProcessor;
class ShadowMapAllocator;

/// Utility to build pipeline states for RenderPipeline.
class URHO3D_API PipelineStateBuilder : public Object, public BatchStateCacheCallback
{
    URHO3D_OBJECT(PipelineStateBuilder, Object);

public:
    PipelineStateBuilder(Context* context, const SceneProcessor* sceneProcessor, const CameraProcessor* cameraProcessor,
        const ShadowMapAllocator* shadowMapAllocator, const InstancingBuffer* instancingBuffer);
    void SetSettings(const ShaderProgramCompositorSettings& settings);
    void UpdateFrameSettings(bool linearColorSpace);

    /// Implement BatchStateCacheCallback
    /// @{
    SharedPtr<PipelineState> CreateBatchPipelineState(const BatchStateCreateKey& key,
        const BatchStateCreateContext& ctx, const PipelineStateOutputDesc& outputDesc) override;
    SharedPtr<PipelineState> CreateBatchPipelineStatePlaceholder(
        unsigned vertexStride, const PipelineStateOutputDesc& outputDesc) override;
    /// @}

    /// Helpers for passes that override pipeline state creation.
    /// @{
    void SetupInputLayoutAndPrimitiveType(GraphicsPipelineStateDesc& pipelineStateDesc,
        const ShaderProgramDesc& shaderProgramDesc, const Geometry* geometry, bool isStereoPass) const;
    void SetupShaders(GraphicsPipelineStateDesc& pipelineStateDesc, ShaderProgramDesc& shaderProgramDesc) const;
    /// @}

    ShaderProgramCompositor* GetShaderProgramCompositor() const { return compositor_; }

private:
    /// State builder
    /// @{
    void ClearState();

    void SetupUserPassState(const Drawable* drawable,
        const Material* material, const Pass* pass, bool lightMaskToStencil);
    void SetupLightVolumePassState(const LightProcessor* lightProcessor);
    void SetupShadowPassState(unsigned splitIndex, const LightProcessor* lightProcessor,
        const Material* material, const Pass* pass);
    void SetupLightSamplers(const LightProcessor* lightProcessor);
    void SetupSamplersForUserOrShadowPass(
        const Material* material, bool hasLightmap, bool hasAmbient, bool isRefractionPass);
    void SetupGeometryBufferSamplers();
    /// @}

    /// Objects whose settings contribute to pipeline states.
    /// Pipeline states should be invalidated if any of those changes.
    /// @{
    const SceneProcessor* const sceneProcessor_{};
    const CameraProcessor* cameraProcessor_{};
    const ShadowMapAllocator* const shadowMapAllocator_{};
    const InstancingBuffer* const instancingBuffer_{};
    /// @}

    Graphics* graphics_{};
    RenderDevice* renderDevice_{};
    PipelineStateCache* pipelineStateCache_{};

    SharedPtr<ShaderProgramCompositor> compositor_;

    /// Re-used objects
    /// @{
    GraphicsPipelineStateDesc pipelineStateDesc_;
    ShaderProgramDesc shaderProgramDesc_;
    /// @}
};

}
