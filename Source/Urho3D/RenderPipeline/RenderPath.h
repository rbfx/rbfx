// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/RenderPipeline/RenderPass.h>
#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{

/// Array of enabled render passes.
using EnabledRenderPasses = ea::vector<ea::pair<ea::string, bool>>;

/// Base class for serializable resource that uses Archive serialization.
class URHO3D_API RenderPath : public SimpleResource
{
    URHO3D_OBJECT(RenderPath, SimpleResource)

public:
    explicit RenderPath(Context* context);
    ~RenderPath() override;
    static void RegisterObject(Context* context);

    /// Fill missing parameters with defaults.
    void CollectParameters(StringVariantMap& params) const;
    /// Initialize render path before using it in view.
    void InitializeView(RenderPipelineView* view);
    /// Update settings for all passes.
    void UpdateParameters(const RenderPipelineSettings& settings, const EnabledRenderPasses& enabledPasses,
        const StringVariantMap& params);
    /// Perform update that does not invoke any rendering commands.
    void Update(const SharedRenderPassState& sharedState);
    /// Execute render commands.
    void Render(const SharedRenderPassState& sharedState);

    /// Make deep copy of RenderPath.
    SharedPtr<RenderPath> Clone() const;

    /// Repair the array of enabled render passes.
    EnabledRenderPasses RepairEnabledRenderPasses(const EnabledRenderPasses& sourcePasses) const;

    /// Manage passes.
    /// @{
    const ea::vector<SharedPtr<RenderPass>>& GetPasses() const { return passes_; }
    const RenderPassTraits& GetAggregatedPassTraits() const { return traits_; }

    void AddPass(RenderPass* pass);
    void RemovePass(RenderPass* pass);
    void ReorderPass(RenderPass* pass, unsigned index);
    /// @}

    /// Implement SimpleResource.
    /// @{
    void SerializeInBlock(Archive& archive) override;
    /// @}

private:
    ea::vector<SharedPtr<RenderPass>> passes_;
    RenderPassTraits traits_;
};
}
