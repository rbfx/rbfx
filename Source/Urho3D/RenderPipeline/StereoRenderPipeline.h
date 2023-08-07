//
// Copyright (c) 2022 the RBFX project.
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

#pragma once

#include "../RenderPipeline/RenderPipeline.h"

#include "../RenderPipeline/CameraProcessor.h"
#include "../RenderPipeline/OutlinePass.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderBufferManager.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../RenderPipeline/PostProcessPass.h"
#include "../RenderPipeline/ScenePass.h"
#include "../RenderPipeline/SceneProcessor.h"

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

}
