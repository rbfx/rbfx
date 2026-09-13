// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "OutlineScenePass.h"
#include "../RenderPipeline/SceneProcessor.h"
#include "../RenderPipeline/CameraProcessor.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderBufferManager.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../RenderPipeline/SharedRenderPassState.h"
#include "../RenderPipeline/ScenePass.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class ShadowMapAllocator;

/// Default implementation of render pipeline instance.
class URHO3D_API DefaultRenderPipelineView
    : public RenderPipelineView
{
    URHO3D_OBJECT(DefaultRenderPipelineView, RenderPipelineView);

public:
    explicit DefaultRenderPipelineView(RenderPipeline* renderPipeline);
    ~DefaultRenderPipelineView() override;

    const RenderPipelineSettings& GetSettings() const { return settings_; }
    void SetSettings(const RenderPipelineSettings& settings);
    void SetRenderPath(RenderPath* renderPath);
    void MarkParametersDirty() { parametersDirty_ = true; }

    /// Implement RenderPipelineInterface
    /// @{
    RenderPipelineDebugger* GetDebugger() override { return &debugger_; }
    /// @}

    /// Implement RenderPipelineView
    /// @{
    bool Define(RenderSurface* renderTarget, Viewport* viewport) override;
    void Update(const FrameInfo& frameInfo) override;
    void Render() override;
    const FrameInfo& GetFrameInfo() const override;
    const RenderPipelineStats& GetStats() const override { return stats_; }
    void DrawDebugGeometries(bool depthTest) override;
    void DrawDebugLights(bool depthTest) override;
    /// @}

protected:
    unsigned RecalculatePipelineStateHash() const;
    void SendViewEvent(StringHash eventType);
    void ApplySettings();
    void UpdateRenderOutputFlags();

private:
    SharedPtr<RenderPath> originalRenderPath_;
    SharedPtr<RenderPath> renderPath_;
    bool parametersDirty_{};

    RenderPipelineSettings settings_;
    unsigned settingsPipelineStateHash_{};
    bool settingsDirty_{};

    TextureFormat albedoFormat_{TextureFormat::TEX_FORMAT_RGBA8_UNORM};
    TextureFormat normalFormat_{TextureFormat::TEX_FORMAT_RGBA8_UNORM};
    TextureFormat specularFormat_{TextureFormat::TEX_FORMAT_RGBA8_UNORM};

    /// Previous pipeline state hash.
    unsigned oldPipelineStateHash_{};

    CommonFrameInfo frameInfo_;
    RenderOutputFlags renderOutputFlags_;

    RenderPipelineStats stats_;
    RenderPipelineDebugger debugger_;
    SharedRenderPassState state_;

    SharedPtr<RenderBufferManager> renderBufferManager_;
    SharedPtr<ShadowMapAllocator> shadowMapAllocator_;
    SharedPtr<InstancingBuffer> instancingBuffer_;
    SharedPtr<SceneProcessor> sceneProcessor_;

    SharedPtr<UnorderedScenePass> depthPrePass_;
    SharedPtr<UnorderedScenePass> opaquePass_;
    SharedPtr<UnorderedScenePass> postOpaquePass_;
    SharedPtr<UnorderedScenePass> deferredDecalPass_;
    SharedPtr<BackToFrontScenePass> alphaPass_;
    SharedPtr<BackToFrontScenePass> postAlphaPass_;

    SharedPtr<RenderBuffer> outlineBuffer_;
    SharedPtr<OutlineScenePass> outlineScenePass_;

    struct DeferredLightingData
    {
        SharedPtr<RenderBuffer> albedoBuffer_;
        SharedPtr<RenderBuffer> specularBuffer_;
        SharedPtr<RenderBuffer> normalBuffer_;
    };
    ea::optional<DeferredLightingData> deferred_;
};

}
