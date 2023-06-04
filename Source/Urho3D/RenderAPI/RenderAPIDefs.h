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

namespace Urho3D
{

/// Maximum number of bound render targets supported by the engine. Hardware limit could be lower.
static constexpr unsigned MaxRenderTargets = 8;
/// Maximum number of bound vertex buffers supported by the engine.
static constexpr unsigned MaxVertexStreams = 4;

/// Index of the frame, counted by the presents of the primary swap chain.
enum class FrameIndex : unsigned long long
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

/// Description of fullscreen mode (resolution and refresh rate).
struct URHO3D_API FullscreenMode
{
    IntVector2 size_{};
    int refreshRate_{};

    /// Operators.
    /// @{
    const auto Tie() const { return ea::tie(size_.x_, size_.y_, refreshRate_); }
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

/// Immutable settings of the render device.
struct RenderDeviceSettings
{
    /// Render backend to use.
    RenderBackend backend_{};
    /// Pointer to external window native handle.
    void* externalWindowHandle_{};
    /// Whether to enable debug mode on GPU if possible.
    bool gpuDebug_{};
    /// Adapter ID.
    ea::optional<unsigned> adapterId_;
};

/// GPU buffer types.
enum class BufferType
{
    /// Vertex buffer for per-vertex or per-instance data.
    Vertex,
    /// Index buffer.
    Index,

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
    /// Buffer can be used as unordered access view.
    BindUnorderedAccess = 1 << 3,
};
URHO3D_FLAGSET(BufferFlag, BufferFlags);

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
/// TODO(diligent): Use stricter typing?
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
    ADDRESS_BORDER,
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
static constexpr auto MaxTextureCoordinates = static_cast<unsigned>(TextureCoordinate::Count);

/// Description of immutable texture sampler bound to the pipeline.
struct URHO3D_API SamplerStateDesc
{
    // TODO(diligent): Remove border color
    Color borderColor_{0.0f, 0.0f, 0.0f, 0.0f};
    TextureFilterMode filterMode_{FILTER_DEFAULT};
    unsigned char anisotropy_{};
    bool shadowCompare_{};
    EnumArray<TextureAddressMode, TextureCoordinate> addressMode_{ADDRESS_WRAP};

    /// Constructors.
    /// @{
    static SamplerStateDesc Bilinear(TextureAddressMode addressMode = ADDRESS_CLAMP)
    {
        SamplerStateDesc desc;
        desc.filterMode_ = FILTER_BILINEAR;
        desc.addressMode_.fill(addressMode);
        return desc;
    }
    /// @}

    /// Operators.
    /// @{
    bool operator==(const SamplerStateDesc& rhs) const
    {
        return borderColor_ == rhs.borderColor_ //
            && filterMode_ == rhs.filterMode_ //
            && anisotropy_ == rhs.anisotropy_ //
            && shadowCompare_ == rhs.shadowCompare_ //
            && addressMode_ == rhs.addressMode_;
    }

    bool operator!=(const SamplerStateDesc& rhs) const { return !(*this == rhs); }

    unsigned ToHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, MakeHash(borderColor_));
        CombineHash(hash, filterMode_);
        CombineHash(hash, anisotropy_);
        CombineHash(hash, shadowCompare_);
        CombineHash(hash, addressMode_[TextureCoordinate::U]);
        CombineHash(hash, addressMode_[TextureCoordinate::V]);
        CombineHash(hash, addressMode_[TextureCoordinate::W]);
        return hash;
    }
    /// @}
};

/// Description of pipeline state output (depth-stencil and render targets).
struct URHO3D_API PipelineStateOutputDesc
{
    TextureFormat depthStencilFormat_{};
    unsigned numRenderTargets_{};
    ea::array<TextureFormat, MaxRenderTargets> renderTargetFormats_{};

    /// Operators.
    /// @{
    bool operator ==(const PipelineStateOutputDesc& rhs) const
    {
        return depthStencilFormat_ == rhs.depthStencilFormat_ //
            && numRenderTargets_ == rhs.numRenderTargets_ //
            && renderTargetFormats_ == rhs.renderTargetFormats_;
    }

    bool operator !=(const PipelineStateOutputDesc& rhs) const { return !(*this == rhs); }

    unsigned ToHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, depthStencilFormat_);
        for (unsigned i = 0; i < numRenderTargets_; ++i)
            CombineHash(hash, renderTargetFormats_[i]);
        return hash;
    }
    /// @}
};

} // namespace Urho3D
