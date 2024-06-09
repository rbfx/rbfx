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

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Graphics/Drawable.h"
#include "Urho3D/RenderPipeline/RenderPath.h"
#include "Urho3D/RenderPipeline/RenderPipelineDebugger.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"
#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

class RenderPipeline;
class RenderSurface;
class Viewport;

/// Base interface of render pipeline viewport instance.
class URHO3D_API RenderPipelineView
    : public Object
    , public RenderPipelineInterface
{
    URHO3D_OBJECT(RenderPipelineView, Object);

public:
    explicit RenderPipelineView(RenderPipeline* renderPipeline);
    RenderPipeline* GetRenderPipeline() const { return renderPipeline_; }

    /// Called in the beginning of the update to check if pipeline should be executed.
    virtual bool Define(RenderSurface* renderTarget, Viewport* viewport) = 0;
    /// Called for defined pipelines before rendering. Frame info is only partially filled.
    virtual void Update(const FrameInfo& frameInfo) = 0;
    /// Called for updated pipelines in appropriate order.
    virtual void Render() = 0;
    /// Return frame info with all members filled.
    virtual const FrameInfo& GetFrameInfo() const = 0;
    /// Return render pipeline statistics for profiling.
    virtual const RenderPipelineStats& GetStats() const = 0;
    /// Draw debug geometries, if applicable.
    virtual void DrawDebugGeometries(bool depthTest) = 0;
    /// Draw debug lights, if applicable.
    virtual void DrawDebugLights(bool depthTest) = 0;

    /// Implement RenderPipelineInterface
    /// @{
    Context* GetContext() const override { return BaseClassName::GetContext(); }
    RenderPipelineDebugger* GetDebugger() override { return nullptr; }
    bool IsLinearColorSpace() const override { return linearColorSpace_; }
    /// @}

protected:
    RenderPipeline* const renderPipeline_{};
    Graphics* const graphics_{};
    Renderer* const renderer_{};

    bool linearColorSpace_{};
};

/// Scene component that spawns render pipeline instances.
class URHO3D_API RenderPipeline : public Component
{
    URHO3D_OBJECT(RenderPipeline, Component);

public:
    /// Invoked when settings change.
    Signal<void(const RenderPipelineSettings& settings)> OnSettingsChanged;
    /// Invoked when render path changes.
    Signal<void(RenderPath* renderPath)> OnRenderPathChanged;
    /// Invoked when render path parameters change.
    Signal<void()> OnParametersChanged;

    RenderPipeline(Context* context);
    ~RenderPipeline() override;

    static void RegisterObject(Context* context);

    /// Implement Component.
    /// @{
    void ApplyAttributes() override;
    /// @}

    /// Properties.
    /// @{
    const RenderPipelineSettings& GetSettings() const { return settings_; }
    void SetSettings(const RenderPipelineSettings& settings);

    ResourceRef GetRenderPathAttr() const;
    void SetRenderPathAttr(const ResourceRef& value);

    RenderPath* GetRenderPath() const { return renderPath_; }
    void SetRenderPath(RenderPath* renderPath);

    const EnabledRenderPasses& GetRenderPasses() const { return renderPasses_; }
    void SetRenderPasses(const EnabledRenderPasses& renderPasses);

    const VariantVector& GetRenderPassesAttr() const;
    void SetRenderPassesAttr(const VariantVector& value);

    const StringVariantMap& GetRenderPathParameters() const { return renderPathParameters_; }
    void SetRenderPathParameters(const StringVariantMap& params);
    /// @}

    /// Update existing render path parameters.
    void UpdateRenderPathParameters(const VariantMap& params);

    /// Create new instance of render pipeline.
    virtual SharedPtr<RenderPipelineView> Instantiate();

private:
    void MarkSettingsDirty();
    void OnRenderPathReloaded();

    bool loadDefaultRenderPath_{true};
    SharedPtr<RenderPath> renderPath_;

    EnabledRenderPasses renderPasses_;
    StringVariantMap renderPathParameters_;

    RenderPipelineSettings settings_;
};

} // namespace Urho3D
