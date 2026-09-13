// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    void UpdateFrameParameters();

protected:
    RenderPipeline* const renderPipeline_{};
    Graphics* const graphics_{};
    Renderer* const renderer_{};

    bool linearColorSpace_{};
    ea::vector<ShaderParameterDesc> frameParameters_;
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

    const StringVariantMap& GetFrameShaderParameters() const { return frameShaderParameters_; }
    void SetFrameShaderParameters(const StringVariantMap& params);
    /// @}

    /// Update existing render path parameters.
    void UpdateRenderPathParameters(const VariantMap& params);
    /// Update existing render path parameter.
    void UpdateRenderPathParameter(const StringHash& nameHash, const Variant& value);

    /// Update render pass enabled state.
    void SetRenderPassEnabled(const ea::string& passName, bool enabled);

    /// Update existing global parameters.
    void UpdateFrameShaderParameters(const VariantMap& params);
    /// Update existing render path parameter.
    void UpdateFrameShaderParameter(const StringHash& nameHash, const Variant& value);
    /// Add or update render path parameter.
    void SetFrameShaderParameter(const ea::string& name, const Variant& value);

    /// Create new instance of render pipeline.
    virtual SharedPtr<RenderPipelineView> Instantiate();

private:
    void MarkSettingsDirty();
    void OnRenderPathReloaded();

    SharedPtr<RenderPath> renderPath_;

    EnabledRenderPasses renderPasses_;
    StringVariantMap renderPathParameters_;
    StringVariantMap frameShaderParameters_;

    RenderPipelineSettings settings_;
};

} // namespace Urho3D
