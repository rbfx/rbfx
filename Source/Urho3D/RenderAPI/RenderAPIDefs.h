// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Container/Hash.h"
#include "Urho3D/Core/Variant.h"
#include "Urho3D/Math/Color.h"

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Diligent/Graphics/GraphicsEngine/interface/InputLayout.h>

#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>
#include <EASTL/tuple.h>

namespace Urho3D
{

class RawBuffer;

/// Maximum number of bound render targets supported by the engine. Hardware limit could be lower.
static constexpr unsigned MaxRenderTargets = 8;
/// Maximum number of bound vertex buffers supported by the engine.
static constexpr unsigned MaxVertexStreams = 4;
/// Some vertex elements in layout may be unused and the hard GPU limit is only applied to the used ones.
static constexpr unsigned MaxNumVertexElements = 2 * Diligent::MAX_LAYOUT_ELEMENTS;
/// Max number of immutable samplers on CPU side. Can be extended freely if needed.
static constexpr unsigned MaxNumImmutableSamplers = 16;

/// Index of the frame, counted by the presents of the primary swap chain.
enum class FrameIndex : long long
{
    None,
    First
};

/// GAPI used for rendering.
enum class RenderBackend
{
    /// Direct3D 11.1 or later.
    D3D11,
    /// Direct3D 12.0 for SDK 10.0.17763.0 or later.
    D3D12,
    /// OpenGL 4.1 (for Desktop) or OpenGL ES 3.0 (for mobiles) or later.
    OpenGL,
    /// Vulkan 1.0 or later.
    Vulkan,

    Count
};

/// Window mode.
enum class WindowMode
{
    /// Windowed.
    Windowed,
    /// Borderless "full-screen" window.
    Borderless,
    /// Native full-screen.
    Fullscreen,
};

/// Shader translation policy.
enum class ShaderTranslationPolicy
{
    /// Do not translate shaders, use universal GLSL shaders directly.
    /// This mode is only supported for OpenGL and OpenGL ES backends (GLSL-based backends).
    Verbatim,
    /// Preprocess and translate shader to the target language through SPIR-V without any optimization.
    /// This results in slower shader compilation, especially in Debug builds.
    /// This mode may help to work around OpenGL driver bugs if used for GLSL-based backends.
    Translate,
    /// Fully process and optimize shader via SPIR-V Tools.
    /// This results in even slower shader compilation, especially in Debug builds.
    /// This mode may improve realtime performance of the shaders, especially on mobile platforms.
    Optimize
};

/// Description of fullscreen mode (resolution and refresh rate).
struct URHO3D_API FullscreenMode
{
    IntVector2 size_{};
    int refreshRate_{};

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const { return ea::tie(size_.x_, size_.y_, refreshRate_); }
#endif
    bool operator==(const FullscreenMode& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const FullscreenMode& rhs) const { return Tie() != rhs.Tie(); }
    bool operator<(const FullscreenMode& rhs) const { return Tie() < rhs.Tie(); }
    /// @}
};
using FullscreenModeVector = ea::vector<FullscreenMode>;

/// Description of the window and GAPI. Some settings may be changed in real time.
struct URHO3D_API WindowSettings
{
    /// Type of window (windowed, borderless fullscreen, native fullscreen).
    WindowMode mode_{};
    /// Windowed: size of the window in units. May be different from the size in pixels due to DPI scale.
    /// Fullscreen: display resolution in pixels.
    /// Borderless: ignored.
    /// Set to 0 to pick automatically.
    IntVector2 size_;
    /// Window title.
    ea::string title_;

    /// Windowed only: whether the window can be resized.
    bool resizable_{};
    /// Fullscreen and Borderless only: index of the monitor.
    int monitor_{};

    /// Whether to enable vertical synchronization.
    bool vSync_{};
    /// Refresh rate. 0 to pick automatically.
    int refreshRate_{};
    /// Multi-sampling level.
    int multiSample_{1};
    /// Whether to use sRGB framebuffer.
    bool sRGB_{};

    /// Mobiles: orientation hints.
    /// Could be any combination of "LandscapeLeft", "LandscapeRight", "Portrait" and "PortraitUpsideDown".
    StringVector orientations_{"LandscapeLeft", "LandscapeRight"};
};

/// Extra tweaks for Vulkan backend.
struct URHO3D_API RenderDeviceSettingsVulkan
{
    StringVector instanceExtensions_;
    StringVector deviceExtensions_;

    ea::optional<Diligent::VulkanDescriptorPoolSize> mainDescriptorPoolSize_;
    ea::optional<Diligent::VulkanDescriptorPoolSize> dynamicDescriptorPoolSize_;

    ea::optional<unsigned> deviceLocalMemoryPageSize_;
    ea::optional<unsigned> hostVisibleMemoryPageSize_;
    ea::optional<unsigned> deviceLocalMemoryReserveSize_;
    ea::optional<unsigned> hostVisibleMemoryReserveSize_;

    ea::optional<unsigned> uploadHeapPageSize_;
    ea::optional<unsigned> dynamicHeapSize_;
    ea::optional<unsigned> dynamicHeapPageSize_;

    ea::optional<unsigned> queryPoolSizes_[Diligent::QUERY_TYPE_NUM_TYPES];
};

/// Extra tweaks for D3D12 backend.
struct URHO3D_API RenderDeviceSettingsD3D12
{
    ea::optional<unsigned> cpuDescriptorHeapAllocationSize_[4];
    ea::optional<unsigned> gpuDescriptorHeapSize_[2];
    ea::optional<unsigned> gpuDescriptorHeapDynamicSize_[2];
    ea::optional<unsigned> dynamicDescriptorAllocationChunkSize_[2];

    ea::optional<unsigned> dynamicHeapPageSize_;
    ea::optional<unsigned> numDynamicHeapPagesToReserve_;

    ea::optional<unsigned> queryPoolSizes_[Diligent::QUERY_TYPE_NUM_TYPES];
};

/// Immutable settings of the render device.
struct URHO3D_API RenderDeviceSettings
{
    /// Render backend to use.
    RenderBackend backend_{};
    /// Pointer to external window native handle.
    void* externalWindowHandle_{};
    /// Whether to enable debug mode on GPU if possible.
    bool gpuDebug_{};
    /// Adapter ID.
    ea::optional<unsigned> adapterId_;

    /// Extra tweaks for Vulkan backend.
    RenderDeviceSettingsVulkan vulkan_;
    /// Extra tweaks for D3D12 backend.
    RenderDeviceSettingsD3D12 d3d12_;
};

/// Capabilities of the render device.
struct URHO3D_API RenderDeviceCaps
{
    bool computeShaders_{};
    bool drawBaseVertex_{};
    bool drawBaseInstance_{};
    bool clipDistance_{};
    bool readOnlyDepth_{};

    bool srgbOutput_{};
    bool hdrOutput_{};

    unsigned constantBufferOffsetAlignment_{};

    unsigned maxTextureSize_{};
    unsigned maxRenderTargetSize_{};
};

/// Statistics of the render device and/or context.
struct URHO3D_API RenderDeviceStats
{
    /// Number of primitives drawn (triangles, lines, patches, etc.)
    unsigned numPrimitives_{};
    /// Number of draw operations.
    unsigned numDraws_{};
    /// Number of compute dispatches.
    unsigned numDispatches_{};
};

/// GPU buffer types.
enum class BufferType
{
    /// Vertex buffer for per-vertex or per-instance data.
    Vertex,
    /// Index buffer.
    Index,
    /// Uniform aka constant buffer.
    Uniform,

    Count
};

/// Buffer usage flags.
enum class BufferFlag
{
    /// Buffer maintains up-to-date CPU-readable copy of the data. Some buffer types cannot be shadowed.
    Shadowed = 1 << 0,
    /// Buffer is dynamic and will be updated frequently.
    Dynamic = 1 << 1,
    /// Buffer data is discarded when frame ends.
    Discard = 1 << 2,
    /// Buffer can be accessed via unordered access view.
    BindUnorderedAccess = 1 << 3,
    /// Buffer contains instance data. This hint is used only on OpenGL ES platforms to emulate base instance.
    PerInstanceData = 1 << 4,
    /// Buffer data cannot change after creation. Data updates lead to buffer recreation.
    Immutable = 1 << 5,
};
URHO3D_FLAGSET(BufferFlag, BufferFlags);

/// GPU texture types.
enum class TextureType
{
    /// Singular 2D texture.
    Texture2D,
    /// Singular cube texture.
    TextureCube,
    /// Singular 3D texture. Support is not guaranteed.
    Texture3D,
    /// Array of 2D textures. Support is not guaranteed.
    Texture2DArray,

    Count
};

/// Texture usage flags.
enum class TextureFlag
{
    None = 0,
    /// Texture can be used as possibly sampled shader resource.
    /// TODO: Every texture is a shader resource implicitly, consider changing this.
    // BindShaderResource = 1 << 0,

    /// Texture can be used as render target.
    BindRenderTarget = 1 << 1,
    /// Texture can be used as depth-stencil target.
    BindDepthStencil = 1 << 2,
    /// Texture can be used via unordered access view.
    BindUnorderedAccess = 1 << 3,
    /// Whether NOT to resolve multisampled texture after rendering.
    /// If set, multisampled texture is used as is. Keep in mind that you cannot easily sample such texture in shader.
    /// By default, shader resource view will point to the resolved texture.
    /// Automatically resolved textures cannot be accessed via unordered access view.
    NoMultiSampledAutoResolve = 1 << 4,
};
URHO3D_FLAGSET(TextureFlag, TextureFlags);

/// Shader types.
enum ShaderType
{
    VS = 0,
    PS,
    GS,
    HS,
    DS,
    CS,
    MAX_SHADER_TYPES
};

/// Texture format, equivalent to Diligent::TEXTURE_FORMAT.
using TextureFormat = Diligent::TEXTURE_FORMAT;

/// Vertex declaration element semantics.
enum VertexElementSemantic : unsigned char
{
    SEM_POSITION = 0,
    SEM_NORMAL,
    SEM_BINORMAL,
    SEM_TANGENT,
    SEM_TEXCOORD,
    SEM_COLOR,
    SEM_BLENDWEIGHTS,
    SEM_BLENDINDICES,
    SEM_OBJECTINDEX,
    MAX_VERTEX_ELEMENT_SEMANTICS
};

/// Arbitrary vertex declaration element datatypes.
enum VertexElementType : unsigned char
{
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_VECTOR2,
    TYPE_VECTOR3,
    TYPE_VECTOR4,
    TYPE_UBYTE4,
    TYPE_UBYTE4_NORM,
    MAX_VERTEX_ELEMENT_TYPES
};

/// Description of the single input required by the vertex shader.
struct URHO3D_API VertexShaderAttribute
{
    VertexElementSemantic semantic_{};
    unsigned semanticIndex_{};
    unsigned inputIndex_{};
};

/// Description of vertex shader attributes.
using VertexShaderAttributeVector = ea::fixed_vector<VertexShaderAttribute, Diligent::MAX_LAYOUT_ELEMENTS>;

/// Texture filtering mode.
enum TextureFilterMode : unsigned char
{
    FILTER_NEAREST = 0,
    FILTER_BILINEAR,
    FILTER_TRILINEAR,
    FILTER_ANISOTROPIC,
    FILTER_NEAREST_ANISOTROPIC,
    FILTER_DEFAULT,
    MAX_FILTERMODES
};

/// Texture addressing mode.
enum TextureAddressMode : unsigned char
{
    ADDRESS_WRAP = 0,
    ADDRESS_MIRROR,
    ADDRESS_CLAMP,
    MAX_ADDRESSMODES
};

/// Texture coordinates.
enum class TextureCoordinate
{
    U = 0,
    V,
    W,

    Count
};
static constexpr unsigned MaxTextureCoordinates = static_cast<unsigned>(TextureCoordinate::Count);

/// Blending mode.
enum BlendMode
{
    BLEND_REPLACE = 0,
    BLEND_ADD,
    BLEND_MULTIPLY,
    BLEND_ALPHA,
    BLEND_ADDALPHA,
    BLEND_PREMULALPHA,
    BLEND_INVDESTALPHA,
    BLEND_SUBTRACT,
    BLEND_SUBTRACTALPHA,
    BLEND_DEFERRED_DECAL,
    MAX_BLENDMODES
};

/// Depth or stencil compare mode.
enum CompareMode
{
    CMP_ALWAYS = 0,
    CMP_EQUAL,
    CMP_NOTEQUAL,
    CMP_LESS,
    CMP_LESSEQUAL,
    CMP_GREATER,
    CMP_GREATEREQUAL,
    MAX_COMPAREMODES
};

/// Culling mode.
enum CullMode
{
    CULL_NONE = 0,
    CULL_CCW,
    CULL_CW,
    MAX_CULLMODES
};

/// Fill mode.
enum FillMode
{
    FILL_SOLID = 0,
    FILL_WIREFRAME,
    FILL_POINT,
    MAX_FILLMODES
};

/// Stencil operation.
enum StencilOp
{
    OP_KEEP = 0,
    OP_ZERO,
    OP_REF,
    OP_INCR,
    OP_DECR
};

/// Primitive type.
enum PrimitiveType
{
    TRIANGLE_LIST = 0,
    LINE_LIST,
    POINT_LIST,
    TRIANGLE_STRIP,
    LINE_STRIP,
    TRIANGLE_FAN
};

enum ClearTarget : unsigned
{
    CLEAR_NONE = 0x0,
    CLEAR_COLOR = 0x1,
    CLEAR_DEPTH = 0x2,
    CLEAR_STENCIL = 0x4,
};
URHO3D_FLAGSET(ClearTarget, ClearTargetFlags);

/// Description of immutable texture sampler bound to the pipeline.
struct URHO3D_API SamplerStateDesc
{
    TextureFilterMode filterMode_{FILTER_DEFAULT};
    unsigned char anisotropy_{};
    bool shadowCompare_{};
    EnumArray<TextureAddressMode, TextureCoordinate> addressMode_{ADDRESS_WRAP};

    /// Constructors.
    /// @{
    static SamplerStateDesc Default(TextureAddressMode addressMode = ADDRESS_CLAMP)
    {
        SamplerStateDesc desc;
        desc.addressMode_.fill(addressMode);
        return desc;
    }
    static SamplerStateDesc Nearest(TextureAddressMode addressMode = ADDRESS_CLAMP)
    {
        SamplerStateDesc desc;
        desc.filterMode_ = FILTER_NEAREST;
        desc.addressMode_.fill(addressMode);
        return desc;
    }
    static SamplerStateDesc Bilinear(TextureAddressMode addressMode = ADDRESS_CLAMP)
    {
        SamplerStateDesc desc;
        desc.filterMode_ = FILTER_BILINEAR;
        desc.addressMode_.fill(addressMode);
        return desc;
    }
    static SamplerStateDesc Trilinear(TextureAddressMode addressMode = ADDRESS_CLAMP)
    {
        SamplerStateDesc desc;
        desc.filterMode_ = FILTER_TRILINEAR;
        desc.addressMode_.fill(addressMode);
        return desc;
    }
    /// @}

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const
    {
        return ea::tie( //
            filterMode_, //
            anisotropy_, //
            shadowCompare_, //
            addressMode_[TextureCoordinate::U], //
            addressMode_[TextureCoordinate::V], //
            addressMode_[TextureCoordinate::W]);
    }
#endif

    bool operator==(const SamplerStateDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const SamplerStateDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

/// Description of pipeline state output (depth-stencil and render targets).
struct URHO3D_API PipelineStateOutputDesc
{
    TextureFormat depthStencilFormat_{};
    unsigned numRenderTargets_{};
    ea::array<TextureFormat, MaxRenderTargets> renderTargetFormats_{};
    unsigned multiSample_{1};

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const
    {
        return ea::make_tuple( //
            depthStencilFormat_, //
            ea::span<const TextureFormat>(renderTargetFormats_.data(), numRenderTargets_), //
            multiSample_);
    }
#endif

    bool operator==(const PipelineStateOutputDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const PipelineStateOutputDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

/// Internal event sent to DeviceObject by RenderDevice.
enum class DeviceObjectEvent
{
    Invalidate,
    Restore,
    Destroy
};

/// Hard-coded uniform buffer slots.
/// TODO: Make it more flexible using name hashes instead of fixed slots.
enum ShaderParameterGroup
{
    SP_FRAME = 0,
    SP_CAMERA,
    SP_ZONE,
    SP_LIGHT,
    SP_MATERIAL,
    SP_OBJECT,
    SP_CUSTOM,
    MAX_SHADER_PARAMETER_GROUPS
};

/// Description of input layout element.
struct URHO3D_API InputLayoutElementDesc
{
    unsigned bufferIndex_{};
    unsigned bufferStride_{};
    unsigned elementOffset_{};
    unsigned instanceStepRate_{};

    VertexElementType elementType_{};
    VertexElementSemantic elementSemantic_{};
    unsigned char elementSemanticIndex_{};

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const
    {
        return ea::tie( //
            bufferIndex_, //
            bufferStride_, //
            elementOffset_, //
            instanceStepRate_, //
            elementType_, //
            elementSemantic_, //
            elementSemanticIndex_);
    }
#endif

    bool operator==(const InputLayoutElementDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const InputLayoutElementDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

/// Description of input layout of graphics pipeline state.
struct URHO3D_API InputLayoutDesc
{
    unsigned size_{};
    ea::array<InputLayoutElementDesc, MaxNumVertexElements> elements_;

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const { return ea::make_tuple(ea::span<const InputLayoutElementDesc>(elements_.data(), size_)); }
#endif

    bool operator==(const InputLayoutDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const InputLayoutDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

/// Description of immutable texture samplers used by the pipeline.
struct URHO3D_API ImmutableSamplersDesc
{
    unsigned size_{};
    ea::array<StringHash, MaxNumImmutableSamplers> names_;
    ea::array<SamplerStateDesc, MaxNumImmutableSamplers> desc_;

    /// Clear the collection.
    void Clear() { size_ = 0; }
    /// Add sampler to collection.
    void Add(StringHash name, const SamplerStateDesc& desc)
    {
        if (size_ >= MaxNumImmutableSamplers)
        {
            URHO3D_ASSERTLOG(false, "Too many immutable samplers, increase MaxNumImmutableSamplers");
            return;
        }

        names_[size_] = name;
        desc_[size_] = desc;
        ++size_;
    }

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const
    {
        return ea::make_tuple( //
            ea::span<const StringHash>(names_.data(), size_), //
            ea::span<const SamplerStateDesc>(desc_.data(), size_));
    }
#endif

    bool operator==(const ImmutableSamplersDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const ImmutableSamplersDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

/// Pipeline state type.
enum class PipelineStateType
{
    Graphics,
    Compute,

    Count
};

using RawVertexBufferArray = ea::array<RawBuffer*, MaxVertexStreams>;

} // namespace Urho3D
