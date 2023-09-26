// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/CameraProcessor.h"
#include "Urho3D/RenderPipeline/OutlinePass.h"
#include "Urho3D/RenderPipeline/PostProcessPass.h"
#include "Urho3D/RenderPipeline/RenderBuffer.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/RenderPipeline.h"
#include "Urho3D/RenderPipeline/ScenePass.h"
#include "Urho3D/RenderPipeline/SceneProcessor.h"

namespace Urho3D
{

class URHO3D_API StereoRenderPipelineView : public RenderPipelineView
{
    URHO3D_OBJECT(StereoRenderPipelineView, RenderPipelineView)

public:
    explicit StereoRenderPipelineView(RenderPipeline* pipeline);
    ~StereoRenderPipelineView();

    const RenderPipelineSettings& GetSettings() const { return settings_; }
    void SetSettings(const RenderPipelineSettings& settings);

    /// Implement RenderPipelineView
    /// @{
    virtual bool Define(RenderSurface* renderTarget, Viewport* viewport) override;
    virtual void Update(const FrameInfo& frameInfo) override;
    virtual void Render() override;
    virtual const FrameInfo& GetFrameInfo() const override;
    virtual const RenderPipelineStats& GetStats() const override;
    void DrawDebugGeometries(bool depthTest) override;
    void DrawDebugLights(bool depthTest) override;
    /// @}

    /// Implement RenderPipelineInterface
    /// @{
    Context* GetContext() const override { return BaseClassName::GetContext(); }
    RenderPipelineDebugger* GetDebugger() override { return nullptr; }
    /// @}

protected:
    void SendViewEvent(StringHash eventID);
    void ApplySettings();

    RenderPipelineSettings settings_;

    CommonFrameInfo frameInfo_;
    PostProcessPassFlags postProcessFlags_;

    RenderPipelineStats stats_;
    RenderPipelineDebugger debugger_;

    SharedPtr<RenderBufferManager> renderBufferManager_;
    SharedPtr<ShadowMapAllocator> shadowMapAllocator_;
    SharedPtr<InstancingBuffer> instancingBuffer_;
    SharedPtr<SceneProcessor> sceneProcessor_;

    SharedPtr<UnorderedScenePass> depthPrePass_;
    SharedPtr<UnorderedScenePass> opaquePass_;
    SharedPtr<UnorderedScenePass> postOpaquePass_;
    SharedPtr<BackToFrontScenePass> alphaPass_;
    SharedPtr<BackToFrontScenePass> postAlphaPass_;
    SharedPtr<OutlineScenePass> outlineScenePass_;
    SharedPtr<OutlinePass> outlinePostProcessPass_;

    ea::vector<SharedPtr<PostProcessPass>> postProcessPasses_;

    unsigned settingsHash_{};
    unsigned oldPipelineStateHash_{};
    bool settingsDirty_{};
};

class URHO3D_API StereoRenderPipeline : public RenderPipeline
{
    URHO3D_OBJECT(StereoRenderPipeline, RenderPipeline);

public:
    StereoRenderPipeline(Context*);
    virtual ~StereoRenderPipeline();

    static void RegisterObject(Context* context);

    virtual SharedPtr<RenderPipelineView> Instantiate() override;
};

} // namespace Urho3D
