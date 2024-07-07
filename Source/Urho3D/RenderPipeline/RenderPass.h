// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/SharedRenderPassState.h"
#include "Urho3D/Scene/Serializable.h"

namespace Urho3D
{

class RenderBufferManager;
class RenderPipelineInterface;
class RenderPipelineView;
struct RenderPipelineSettings;

/// Render pass traits that are important for render pipeline configuration.
struct RenderPassTraits
{
    /// Whether it's required to read from and write to color buffer at the same time.
    bool needReadWriteColorBuffer_{};
    /// Whether it's required that color sampling is at least bilinear.
    bool needBilinearColorSampler_{};
};

/// Render pass, component of render path.
class URHO3D_API RenderPass
    : public Serializable
{
    URHO3D_OBJECT(RenderPass, Serializable);

public:
    explicit RenderPass(Context* context);
    ~RenderPass() override;
    static void RegisterObject(Context* context);

    /// Return unique pass name.
    virtual const ea::string& GetPassName() const;
    /// Create missing parameters in the global map with default values.
    virtual void CollectParameters(StringVariantMap& params) const {}
    /// Initialize render pass before using it in view.
    virtual void InitializeView(RenderPipelineView* view) {}
    /// Update settings and parameters of the pass.
    /// This function is always called before any rendering updates or getters.
    virtual void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) {}
    /// Perform update that does not invoke any rendering commands.
    virtual void Update(const SharedRenderPassState& sharedState) {}
    /// Execute render commands.
    virtual void Render(const SharedRenderPassState& sharedState) {}

    /// Attributes.
    /// @{
    void SetEnabled(bool enabled) { isEnabledByUser_ = enabled; }
    bool IsEnabledEffectively() const { return isEnabledByUser_ && isEnabledInternally_; }
    const RenderPassTraits& GetTraits() const { return traits_; }

    bool IsEnabledByDefault() const { return attributes_.isEnabledByDefault_; }
    void SetEnabledByDefault(bool enabled) { attributes_.isEnabledByDefault_ = enabled; }
    const ea::string& GetComment() const { return attributes_.comment_; }
    void SetComment(const ea::string& comment) { attributes_.comment_ = comment; }
    /// @}

protected:
    void DeclareParameter(const ea::string& name, const Variant& value, StringVariantMap& params) const;
    const Variant& LoadParameter(const ea::string& name, const StringVariantMap& params) const;
    void ConnectToRenderBuffer(WeakPtr<RenderBuffer>& renderBuffer, StringHash name,
        const SharedRenderPassState& sharedState, bool required = true) const;

    struct Attributes
    {
        ea::string passName_;
        bool isEnabledByDefault_{true};
        ea::string comment_;
    } attributes_;

    bool isEnabledByUser_{};
    bool isEnabledInternally_{true};
    RenderPassTraits traits_;
};

}
