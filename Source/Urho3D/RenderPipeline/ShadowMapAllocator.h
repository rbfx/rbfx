// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../Math/AreaAllocator.h"
#include "../Math/Rect.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Light.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class RenderContext;
class RenderDevice;
class Renderer;

/// Utility to allocate shadow maps in texture atlas.
class URHO3D_API ShadowMapAllocator : public Object
{
    URHO3D_OBJECT(ShadowMapAllocator, Object);

public:
    explicit ShadowMapAllocator(Context* context);
    void SetSettings(const ShadowMapAllocatorSettings& settings);

    /// Reset allocated shadow maps.
    void ResetAllShadowMaps();
    /// Allocate shadow map of given size. It is better to allocate from bigger to smaller sizes.
    ShadowMapRegion AllocateShadowMap(const IntVector2& size);
    /// Begin shadow map rendering. Clears shadow map if necessary.
    bool BeginShadowMapRendering(const ShadowMapRegion& shadowMap);

    const ShadowMapAllocatorSettings& GetSettings() const { return settings_; }
    const SamplerStateDesc& GetSamplerStateDesc() const { return samplerStateDesc_; }
    const PipelineStateOutputDesc& GetShadowOutputDesc() const { return shadowOutputDesc_; }

private:
    struct AtlasPage
    {
        unsigned index_{};
        SharedPtr<Texture2D> texture_;
        AreaAllocator areaAllocator_;
        bool clearBeforeRendering_{};

        /// Allocate shadow map.
        ShadowMapRegion AllocateRegion(const IntVector2& size);
    };

    void CacheSettings();
    void AllocatePage();

    /// External dependencies
    /// @{
    RenderDevice* renderDevice_{};
    RenderContext* renderContext_{};
    /// @}

    /// Settings
    /// @{
    ShadowMapAllocatorSettings settings_;
    SamplerStateDesc samplerStateDesc_;
    PipelineStateOutputDesc shadowOutputDesc_;
    TextureFormat shadowMapFormat_{};
    IntVector2 shadowAtlasPageSize_;
    /// @}

    ea::vector<AtlasPage> pages_;
    SharedPtr<Texture2D> vsmDepthTexture_;
};

}
