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

#include "../Container/ConstString.h"
#include "../Container/Hash.h"
#include "../Core/Signal.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/Light.h"
#include "../Math/Vector2.h"

namespace Urho3D
{

class Camera;
class Light;
class PipelineState;
class RenderPipelineDebugger;
class RenderSurface;
class Texture2D;
class Viewport;
struct BatchStateCreateKey;
struct BatchStateCreateContext;
struct PipelineStateOutputDesc;
struct UIBatchStateKey;
struct UIBatchStateCreateContext;

/// Macro to define shader constant name. Group name doesn't serve any functional purpose.
#define URHO3D_SHADER_CONST(group, name) URHO3D_GLOBAL_CONSTANT(ConstString group##_##name{#name})

/// Macro to define shader resource name.
#define URHO3D_SHADER_RESOURCE(name) URHO3D_GLOBAL_CONSTANT(ConstString name{#name})

/// Common parameters of rendered frame.
struct CommonFrameInfo
{
    unsigned frameNumber_{};
    float timeStep_{};

    IntVector2 viewportSize_;
    IntRect viewportRect_;

    Viewport* viewport_{};
    RenderSurface* renderTarget_{};

    ea::array<Camera*, 2> cameras_{};
};

/// Traits of scene pass.
enum class DrawableProcessorPassFlag
{
    None = 0,

    HasAmbientLighting = 1 << 0,
    DisableInstancing = 1 << 1,
    DeferredLightMaskToStencil = 1 << 2,
    NeedReadableDepth = 1 << 3,
    RefractionPass = 1 << 4,
    DepthOnlyPass = 1 << 5,
    ReadOnlyDepth = 1 << 6,
    StereoInstancing = 1 << 7,

    BatchCallback = 1 << 8,
    PipelineStateCallback = 1 << 9,
};

URHO3D_FLAGSET(DrawableProcessorPassFlag, DrawableProcessorPassFlags);

/// Sub-passes of user pass.
enum class BatchCompositorSubpass
{
    /// Geometry buffer building pass of deferred rendering.
    Deferred,
    /// Base pass, optionally lit with forward rendering.
    Base,
    /// Additive light pass for forward rendering.
    Light,
    /// Artificial value that indicates to ignore subpass-specific properties.
    Ignored,
};

/// Flags that control how exactly batches are rendered.
enum class BatchRenderFlag
{
    None = 0,
    EnableAmbientLighting = 1 << 0,
    EnableVertexLights = 1 << 1,
    EnablePixelLights = 1 << 2,
    EnableInstancingForStaticGeometry = 1 << 3,
    DisableColorOutput = 1 << 4,
    LightMaskToStencil = 1 << 5,
    LinearColorSpace = 1 << 6,

    EnableAmbientAndVertexLighting = EnableAmbientLighting | EnableVertexLights,
};

URHO3D_FLAGSET(BatchRenderFlag, BatchRenderFlags);

/// Render buffer traits.
enum class RenderBufferFlag
{
    /// Texture content is preserved between frames.
    Persistent = 1 << 0,
    FixedTextureSize = 1 << 1,
    BilinearFiltering = 1 << 2,
    CubeMap = 1 << 3,
    NoMultiSampledAutoResolve = 1 << 4
};

URHO3D_FLAGSET(RenderBufferFlag, RenderBufferFlags);

/// Render buffer parameters. Actual render buffer size is controlled externally.
struct RenderBufferParams
{
    TextureFormat textureFormat_{};
    int multiSampleLevel_{ 1 };
    RenderBufferFlags flags_;

    bool operator==(const RenderBufferParams& rhs) const
    {
        return textureFormat_ == rhs.textureFormat_
            && multiSampleLevel_ == rhs.multiSampleLevel_
            && flags_ == rhs.flags_;
    }

    bool operator!=(const RenderBufferParams& rhs) const { return !(*this == rhs); }
};

/// Color space of primary color outputs of render pipeline.
/// Color buffer is guaranteed to have Red, Green and Blue channels regardless of this choice.
enum class RenderPipelineColorSpace
{
    /// Low dynamic range lighting in Gamma space, trimmed to [0, 1].
    GammaLDR,
    /// Low dynamic range lighting in Linear space, trimmed to [0, 1].
    LinearLDR,
    /// High dynamic range lighting in Linear space. Should be tone mapped before frame end.
    LinearHDR,
    /// Use the color space that matches output render texture.
    Optimized,
};

/// Rarely-changing settings of render buffer manager.
struct RenderBufferManagerSettings
{
    /// Whether to inherit multisample level from output render texture.
    bool inheritMultiSampleLevel_{ true };
    /// Multisample level of both output color buffers and depth buffer.
    int multiSampleLevel_{ 1 };
    /// Preferred color space of both output color buffers.
    RenderPipelineColorSpace colorSpace_{};
    /// Whether output color buffers are required to have at least bilinear filtering.
    bool filteredColor_{};
    /// Whether the depth-stencil buffer is required to have stencil.
    bool stencilBuffer_{};
    /// Whether the depth buffer should be readable.
    bool readableDepth_{};
    /// Whether the both of output color buffers should be usable with other render targets.
    /// OpenGL backbuffer color cannot do that.
    bool colorUsableWithMultipleRenderTargets_{};

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, readableDepth_);
        return hash;
    }

    void Validate()
    {
        multiSampleLevel_ = Clamp(ClosestPowerOfTwo(multiSampleLevel_), 1u, 16u);
    }

    bool operator==(const RenderBufferManagerSettings& rhs) const
    {
        return inheritMultiSampleLevel_ == rhs.inheritMultiSampleLevel_
            && multiSampleLevel_ == rhs.multiSampleLevel_
            && colorSpace_ == rhs.colorSpace_
            && filteredColor_ == rhs.filteredColor_
            && stencilBuffer_ == rhs.stencilBuffer_
            && readableDepth_ == rhs.readableDepth_
            && colorUsableWithMultipleRenderTargets_ == rhs.colorUsableWithMultipleRenderTargets_;
    }

    bool operator!=(const RenderBufferManagerSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

/// Frequently-changing settings of render buffer manager.
struct RenderBufferManagerFrameSettings
{
    /// Whether the both of output color buffers should be readable.
    bool readableColor_{};
    /// Whether it's should be supported to read from and write to output color buffer simultaneously.
    bool supportColorReadWrite_{};
};

/// Traits of post-processing pass
enum class RenderOutputFlag
{
    None = 0,
    NeedColorOutputReadAndWrite = 1 << 0,
    NeedColorOutputBilinear = 1 << 1,
};

URHO3D_FLAGSET(RenderOutputFlag, RenderOutputFlags);

/// Pipeline state cache callback used to create actual pipeline state.
class BatchStateCacheCallback
{
public:
    /// Destruct.
    virtual ~BatchStateCacheCallback();
    /// Create pipeline state for given context and key.
    /// Only attributes that contribute to pipeline state hashes are safe to use.
    virtual SharedPtr<PipelineState> CreateBatchPipelineState(const BatchStateCreateKey& key,
        const BatchStateCreateContext& ctx, const PipelineStateOutputDesc& outputDesc) = 0;
    /// Create placeholder pipeline state when actual pipeline state creation fails.
    virtual SharedPtr<PipelineState> CreateBatchPipelineStatePlaceholder(
        unsigned vertexStride, const PipelineStateOutputDesc& outputDesc) = 0;
};

/// Pipeline state cache callback used to create actual pipeline state for UI batches.
class UIBatchStateCacheCallback
{
public:
    /// Destruct.
    virtual ~UIBatchStateCacheCallback();
    /// Create pipeline state for given key.
    /// Only attributes that constribute to pipeline state hashes are safe to use.
    virtual SharedPtr<PipelineState> CreateUIBatchPipelineState(const UIBatchStateKey& key, const UIBatchStateCreateContext& ctx) = 0;
};

/// Render pipeline frame statistics.
struct RenderPipelineStats
{
    /// Total number of lights processed.
    unsigned numLights_{};
    /// Total number of lights with shadows processed.
    unsigned numShadowedLights_{};
    /// Total number of geometries in the frame (excluding shadow casters).
    unsigned numGeometries_{};
    /// Number of occluders rendered.
    unsigned numOccluders_{};
};

/// Base interface of render pipeline required by Render Pipeline classes.
class URHO3D_API RenderPipelineInterface
{
public:
    virtual ~RenderPipelineInterface();
    virtual Context* GetContext() const = 0;
    virtual RenderPipelineDebugger* GetDebugger() = 0;
    virtual bool IsLinearColorSpace() const = 0;

    /// Callbacks
    /// @{
    Signal<void(const CommonFrameInfo& frameInfo)> OnUpdateBegin;
    Signal<void(const CommonFrameInfo& frameInfo)> OnUpdateEnd;
    Signal<void(const CommonFrameInfo& frameInfo)> OnRenderBegin;
    Signal<void(const CommonFrameInfo& frameInfo)> OnRenderEnd;
    Signal<void()> OnPipelineStatesInvalidated;
    Signal<void(RenderPipelineStats& stats)> OnCollectStatistics;
    /// @}
};

/// Region of shadow map that contains one or more shadow split.
struct ShadowMapRegion
{
    unsigned pageIndex_{};
    Texture2D* texture_;
    IntRect rect_;

    /// Return whether the shadow map region is not empty.
    operator bool() const { return !!texture_; }
    /// Return sub-region for split.
    /// Splits are indexed as elements in rectangle grid, from left to right, top to bottom, row-major.
    ShadowMapRegion GetSplit(unsigned split, const IntVector2& numSplits) const;
};

/// Light processor callback.
class LightProcessorCallback
{
public:
    /// Destruct.
    virtual ~LightProcessorCallback();
    /// Return whether light needs shadow.
    virtual bool IsLightShadowed(Light* light) = 0;
    /// Return best shadow map size for given light. Should be safe to call from multiple threads.
    virtual unsigned GetShadowMapSize(Light* light, unsigned numActiveSplits) const = 0;
    /// Allocate shadow map for one frame.
    virtual ShadowMapRegion AllocateTransientShadowMap(const IntVector2& size) = 0;
};

struct LightProcessorCacheSettings
{
    /// How many lights there could be before cache is cleaned up aggressively.
    unsigned budget_{ 64 };
    /// Number of seconds to keep cached LightProcessor if cache size is within the budget.
    unsigned normalTimeToLive_{ 60 };
    /// Number of seconds to keep cached LightProcessor if cache size is out of the budget.
    unsigned agressiveTimeToLive_{ 2 };

    /// Utility operators
    /// @{
    bool operator==(const LightProcessorCacheSettings& rhs) const
    {
        return budget_ == rhs.budget_
            && normalTimeToLive_ == rhs.normalTimeToLive_
            && agressiveTimeToLive_ == rhs.agressiveTimeToLive_;
    }

    bool operator!=(const LightProcessorCacheSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

struct DrawableProcessorSettings
{
    MaterialQuality materialQuality_{ QUALITY_HIGH };
    unsigned maxVertexLights_{ 4 };
    unsigned maxPixelLights_{ 4 };
    unsigned pcfKernelSize_{ 1 };
    float normalOffsetScale_{1.0f};
    LightProcessorCacheSettings lightProcessorCache_;

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, maxVertexLights_);
        CombineHash(hash, pcfKernelSize_);
        CombineHash(hash, MakeHash(normalOffsetScale_));
        return hash;
    }

    void Validate()
    {
        maxVertexLights_ = Clamp(maxVertexLights_, 0u, 4u);
        maxPixelLights_ = Clamp(maxPixelLights_, 0u, 256u);
        pcfKernelSize_ = Clamp(pcfKernelSize_, 1u, 5u);
        normalOffsetScale_ = ea::max(0.0f, normalOffsetScale_);

        // Kernel size of 4 is not supported
        if (pcfKernelSize_ == 4)
            pcfKernelSize_ = 3;
    }

    bool operator==(const DrawableProcessorSettings& rhs) const
    {
        return materialQuality_ == rhs.materialQuality_
            && maxVertexLights_ == rhs.maxVertexLights_
            && maxPixelLights_ == rhs.maxPixelLights_
            && pcfKernelSize_ == rhs.pcfKernelSize_
            && lightProcessorCache_ == rhs.lightProcessorCache_
            && normalOffsetScale_ == rhs.normalOffsetScale_;
    }

    bool operator!=(const DrawableProcessorSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

struct InstancingBufferSettings
{
    bool enableInstancing_{};
    unsigned firstInstancingTexCoord_{};
    unsigned numInstancingTexCoords_{};
    unsigned stepRate_{ 1 };

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, enableInstancing_);
        CombineHash(hash, firstInstancingTexCoord_);
        CombineHash(hash, numInstancingTexCoords_);
        CombineHash(hash, stepRate_);
        return hash;
    }

    void Validate()
    {
    }

    bool operator==(const InstancingBufferSettings& rhs) const
    {
        return enableInstancing_ == rhs.enableInstancing_
            && firstInstancingTexCoord_ == rhs.firstInstancingTexCoord_
            && numInstancingTexCoords_ == rhs.numInstancingTexCoords_
            && stepRate_ == rhs.stepRate_;
    }

    bool operator!=(const InstancingBufferSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

enum class DrawableAmbientMode
{
    Constant,
    Flat,
    Directional,
};

struct BatchRendererSettings
{
    bool cubemapBoxProjection_{};
    DrawableAmbientMode ambientMode_{ DrawableAmbientMode::Directional };
    Vector2 varianceShadowMapParams_{ 0.0000001f, 0.9f };

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, cubemapBoxProjection_);
        CombineHash(hash, MakeHash(ambientMode_));
        return hash;
    }

    void Validate()
    {
    }

    bool operator==(const BatchRendererSettings& rhs) const
    {
        return cubemapBoxProjection_ == rhs.cubemapBoxProjection_
            && ambientMode_ == rhs.ambientMode_
            && varianceShadowMapParams_ == rhs.varianceShadowMapParams_;
    }

    bool operator!=(const BatchRendererSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

struct ShadowMapAllocatorSettings
{
    bool enableVarianceShadowMaps_{};
    int varianceShadowMapMultiSample_{ 1 };
    bool use16bitShadowMaps_{};
    unsigned shadowAtlasPageSize_{ 2048 };

    float depthBiasScale_{1.0f};
    float depthBiasOffset_{0.0f};

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, enableVarianceShadowMaps_);
        CombineHash(hash, use16bitShadowMaps_);
        CombineHash(hash, MakeHash(depthBiasScale_));
        CombineHash(hash, MakeHash(depthBiasOffset_));
        return hash;
    }

    void Validate()
    {
        varianceShadowMapMultiSample_ = Clamp(ClosestPowerOfTwo(varianceShadowMapMultiSample_), 1u, 16u);
        shadowAtlasPageSize_ = Clamp(ClosestPowerOfTwo(shadowAtlasPageSize_), 128u, 16 * 1024u);
        depthBiasScale_ = ea::max(0.0f, depthBiasScale_);
    }

    bool operator==(const ShadowMapAllocatorSettings& rhs) const
    {
        return enableVarianceShadowMaps_ == rhs.enableVarianceShadowMaps_
            && varianceShadowMapMultiSample_ == rhs.varianceShadowMapMultiSample_
            && use16bitShadowMaps_ == rhs.use16bitShadowMaps_
            && shadowAtlasPageSize_ == rhs.shadowAtlasPageSize_
            && depthBiasScale_ == rhs.depthBiasScale_
            && depthBiasOffset_ == rhs.depthBiasOffset_;
    }

    bool operator!=(const ShadowMapAllocatorSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

struct OcclusionBufferSettings
{
    bool threadedOcclusion_{};
    unsigned maxOccluderTriangles_{ 5000 };
    unsigned occlusionBufferSize_{ 256 };
    float occluderSizeThreshold_{ 0.025f };

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        return 0;
    }

    void Validate()
    {
        occlusionBufferSize_ = Clamp(occlusionBufferSize_, 1u, 16 * 1024u);
    }

    bool operator==(const OcclusionBufferSettings& rhs) const
    {
        return threadedOcclusion_ == rhs.threadedOcclusion_
            && maxOccluderTriangles_ == rhs.maxOccluderTriangles_
            && occlusionBufferSize_ == rhs.occlusionBufferSize_
            && occluderSizeThreshold_ == rhs.occluderSizeThreshold_;
    }

    bool operator!=(const OcclusionBufferSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

enum class DirectLightingMode
{
    Forward,
    DeferredBlinnPhong,
    DeferredPBR
};

enum class SpecularQuality
{
    Disabled,
    Simple,
    Antialiased
};

enum class ReflectionQuality
{
    Vertex,
    Pixel
};

struct SceneProcessorSettings
    : public DrawableProcessorSettings
    , public OcclusionBufferSettings
    , public BatchRendererSettings
{
    SpecularQuality specularQuality_{ SpecularQuality::Simple };
    ReflectionQuality reflectionQuality_{ ReflectionQuality::Pixel };
    bool linearReflectionMaps_{};
    bool depthPrePass_{ false };
    bool enableShadows_{ true };
    DirectLightingMode lightingMode_{};
    unsigned directionalShadowSize_{ 1024 };
    unsigned spotShadowSize_{ 1024 };
    unsigned pointShadowSize_{ 256 };

    /// Utility operators
    /// @{
    bool IsDeferredLighting() const
    {
        switch (lightingMode_)
        {
        case DirectLightingMode::DeferredBlinnPhong:
        case DirectLightingMode::DeferredPBR:
            return true;
        default:
            return false;
        }
    }

    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, DrawableProcessorSettings::CalculatePipelineStateHash());
        CombineHash(hash, OcclusionBufferSettings::CalculatePipelineStateHash());
        CombineHash(hash, BatchRendererSettings::CalculatePipelineStateHash());
        CombineHash(hash, MakeHash(specularQuality_));
        CombineHash(hash, MakeHash(reflectionQuality_));
        CombineHash(hash, linearReflectionMaps_);
        CombineHash(hash, enableShadows_);
        CombineHash(hash, MakeHash(lightingMode_));
        return hash;
    }

    void Validate()
    {
        DrawableProcessorSettings::Validate();
        OcclusionBufferSettings::Validate();
        BatchRendererSettings::Validate();
        directionalShadowSize_ = ClosestPowerOfTwo(directionalShadowSize_);
        spotShadowSize_ = ClosestPowerOfTwo(spotShadowSize_);
        pointShadowSize_ = ClosestPowerOfTwo(pointShadowSize_);
    }

    bool operator==(const SceneProcessorSettings& rhs) const
    {
        return DrawableProcessorSettings::operator==(rhs)
            && OcclusionBufferSettings::operator==(rhs)
            && BatchRendererSettings::operator==(rhs)
            && specularQuality_ == rhs.specularQuality_
            && reflectionQuality_ == rhs.reflectionQuality_
            && linearReflectionMaps_ == rhs.linearReflectionMaps_
            && depthPrePass_ == rhs.depthPrePass_
            && enableShadows_ == rhs.enableShadows_
            && lightingMode_ == rhs.lightingMode_
            && directionalShadowSize_ == rhs.directionalShadowSize_
            && spotShadowSize_ == rhs.spotShadowSize_
            && pointShadowSize_ == rhs.pointShadowSize_;
    }

    bool operator!=(const SceneProcessorSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

/// Settings that contribute to shader defines.
struct ShaderProgramCompositorSettings
{
    RenderBufferManagerSettings renderBufferManager_;
    SceneProcessorSettings sceneProcessor_;
    ShadowMapAllocatorSettings shadowMapAllocator_;
    InstancingBufferSettings instancingBuffer_;

    /// Utility operators
    /// @{
    unsigned CalculatePipelineStateHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, renderBufferManager_.CalculatePipelineStateHash());
        CombineHash(hash, sceneProcessor_.CalculatePipelineStateHash());
        CombineHash(hash, shadowMapAllocator_.CalculatePipelineStateHash());
        CombineHash(hash, instancingBuffer_.CalculatePipelineStateHash());
        return hash;
    }

    void Validate()
    {
        renderBufferManager_.Validate();
        sceneProcessor_.Validate();
        shadowMapAllocator_.Validate();
        instancingBuffer_.Validate();
    }

    bool operator==(const ShaderProgramCompositorSettings& rhs) const
    {
        return renderBufferManager_ == rhs.renderBufferManager_
            && sceneProcessor_ == rhs.sceneProcessor_
            && shadowMapAllocator_ == rhs.shadowMapAllocator_
            && instancingBuffer_ == rhs.instancingBuffer_;
    }

    bool operator!=(const ShaderProgramCompositorSettings& rhs) const { return !(*this == rhs); }
    /// @}
};

/// Settings of default render pipeline.
struct RenderPipelineSettings : public ShaderProgramCompositorSettings
{
    /// Global pipeline settings
    /// @{
    bool drawDebugGeometry_{true};
    /// @}

    /// Utility operators
    /// @{
    bool operator==(const RenderPipelineSettings& rhs) const
    {
        return ShaderProgramCompositorSettings::operator==(rhs)
            && drawDebugGeometry_ == rhs.drawDebugGeometry_;
    }

    bool operator!=(const RenderPipelineSettings& rhs) const { return !(*this == rhs); }
    /// @}

    /// Adjust to closest settings suported by the platform.
    void AdjustToSupported(Context* context);

    /// Don't modify settings inplace after these calls! Always restore settings from external source.
    /// @{
    void PropagateImpliedSettings();
    void AdjustForRenderPath(RenderOutputFlags flags);
    /// @}
};

/// ID of static pipeline state.
enum class StaticPipelineStateId : unsigned
{
    Invalid,
};

/// Helper for pipeline state creation.
/// Keep entire string because it should be used only on startup.
using NamedSamplerStateDesc = ea::pair<ea::string, SamplerStateDesc>;

/// Reference to input shader resource. Only textures are supported now.
struct ShaderResourceDesc
{
    StringHash name_{};
    RawTexture* texture_{};
};

/// Generic description of shader parameter.
/// Beware of Variant allocations for types larger than Vector4!
struct ShaderParameterDesc
{
    StringHash name_;
    Variant value_;
};

}
