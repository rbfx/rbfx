// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Resource/ImageDDS.h"

namespace Urho3D
{

namespace
{

static constexpr unsigned FOURCC_DXT1 = MakeFourCC('D', 'X', 'T', '1');
static constexpr unsigned FOURCC_DXT2 = MakeFourCC('D', 'X', 'T', '2');
static constexpr unsigned FOURCC_DXT3 = MakeFourCC('D', 'X', 'T', '3');
static constexpr unsigned FOURCC_DXT4 = MakeFourCC('D', 'X', 'T', '4');
static constexpr unsigned FOURCC_DXT5 = MakeFourCC('D', 'X', 'T', '5');

static constexpr unsigned FOURCC_ETC1 = MakeFourCC('E', 'T', 'C', '1');
static constexpr unsigned FOURCC_ETC2 = MakeFourCC('E', 'T', 'C', '2');
static constexpr unsigned FOURCC_ETC2A = MakeFourCC('E', 'T', '2', 'A');

static constexpr unsigned FOURCC_PTC2 = MakeFourCC('P', 'T', 'C', '2');
static constexpr unsigned FOURCC_PTC4 = MakeFourCC('P', 'T', 'C', '4');

static constexpr unsigned D3DFMT_A16B16G16R16 = 36;
static constexpr unsigned D3DFMT_A16B16G16R16F = 113;
static constexpr unsigned D3DFMT_A32B32G32R32F = 116;

static const unsigned DDSCAPS_COMPLEX = 0x00000008U;
static const unsigned DDSCAPS_TEXTURE = 0x00001000U;
static const unsigned DDSCAPS_MIPMAP = 0x00400000U;
static const unsigned DDSCAPS2_VOLUME = 0x00200000U;
static const unsigned DDSCAPS2_CUBEMAP = 0x00000200U;

static const unsigned DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
static const unsigned DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

// DX10 flags
static const unsigned DDS_DIMENSION_TEXTURE1D = 2;
static const unsigned DDS_DIMENSION_TEXTURE2D = 3;
static const unsigned DDS_DIMENSION_TEXTURE3D = 4;

static const unsigned DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;

static const unsigned DDS_DXGI_FORMAT_R32G32B32A32_FLOAT = 2;
static const unsigned DDS_DXGI_FORMAT_R16G16B16A16_FLOAT = 10;
static const unsigned DDS_DXGI_FORMAT_R16G16B16A16_UNORM = 11;
static const unsigned DDS_DXGI_FORMAT_R10G10B10A2_UNORM = 24;
static const unsigned DDS_DXGI_FORMAT_R8G8B8A8_UNORM = 28;
static const unsigned DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 26;
static const unsigned DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67;
static const unsigned DDS_DXGI_FORMAT_BC1_UNORM = 71;
static const unsigned DDS_DXGI_FORMAT_BC1_UNORM_SRGB = 72;
static const unsigned DDS_DXGI_FORMAT_BC2_UNORM = 74;
static const unsigned DDS_DXGI_FORMAT_BC2_UNORM_SRGB = 75;
static const unsigned DDS_DXGI_FORMAT_BC3_UNORM = 77;
static const unsigned DDS_DXGI_FORMAT_BC3_UNORM_SRGB = 78;
static const unsigned DDS_DXGI_FORMAT_B5G6R5_UNORM = 85;
static const unsigned DDS_DXGI_FORMAT_B5G5R5A1_UNORM = 86;

const DDPixelFormat MakePixelFormat(unsigned numBits, unsigned maskR, unsigned maskG, unsigned maskB, unsigned maskA)
{
    DDPixelFormat format{};
    format.dwRGBBitCount_ = numBits;
    format.dwRBitMask_ = maskR;
    format.dwGBitMask_ = maskG;
    format.dwBBitMask_ = maskB;
    format.dwRGBAlphaBitMask_ = maskA;
    return format;
}

bool IsSamePixelFormat(const DDPixelFormat& lhs, const DDPixelFormat& rhs)
{
    return lhs.dwRGBBitCount_ == rhs.dwRGBBitCount_ //
        && lhs.dwRBitMask_ == rhs.dwRBitMask_ //
        && lhs.dwGBitMask_ == rhs.dwGBitMask_ //
        && lhs.dwBBitMask_ == rhs.dwBBitMask_ //
        && lhs.dwRGBAlphaBitMask_ == rhs.dwRGBAlphaBitMask_;
}

} // namespace

TextureFormat PickTextureFormat(const DDPixelFormat& pixelFormat, unsigned dxgiFormat)
{
    static const ea::unordered_map<unsigned, TextureFormat> dxgiToTextureFormat = {
        {DDS_DXGI_FORMAT_R32G32B32A32_FLOAT, TextureFormat::TEX_FORMAT_RGBA32_FLOAT},
        {DDS_DXGI_FORMAT_R16G16B16A16_FLOAT, TextureFormat::TEX_FORMAT_RGBA16_FLOAT},
        {DDS_DXGI_FORMAT_R16G16B16A16_UNORM, TextureFormat::TEX_FORMAT_RGBA16_UNORM},
        {DDS_DXGI_FORMAT_R10G10B10A2_UNORM, TextureFormat::TEX_FORMAT_RGB10A2_UNORM},
        {DDS_DXGI_FORMAT_R8G8B8A8_UNORM, TextureFormat::TEX_FORMAT_RGBA8_UNORM},
        {DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB},
        {DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP, TextureFormat::TEX_FORMAT_RGB9E5_SHAREDEXP},
        {DDS_DXGI_FORMAT_BC1_UNORM, TextureFormat::TEX_FORMAT_BC1_UNORM},
        {DDS_DXGI_FORMAT_BC1_UNORM_SRGB, TextureFormat::TEX_FORMAT_BC1_UNORM_SRGB},
        {DDS_DXGI_FORMAT_BC2_UNORM, TextureFormat::TEX_FORMAT_BC2_UNORM},
        {DDS_DXGI_FORMAT_BC2_UNORM_SRGB, TextureFormat::TEX_FORMAT_BC2_UNORM_SRGB},
        {DDS_DXGI_FORMAT_BC3_UNORM, TextureFormat::TEX_FORMAT_BC3_UNORM},
        {DDS_DXGI_FORMAT_BC3_UNORM_SRGB, TextureFormat::TEX_FORMAT_BC3_UNORM_SRGB},
        {DDS_DXGI_FORMAT_B5G6R5_UNORM, TextureFormat::TEX_FORMAT_B5G6R5_UNORM},
        {DDS_DXGI_FORMAT_B5G5R5A1_UNORM, TextureFormat::TEX_FORMAT_B5G5R5A1_UNORM},
    };

    static const ea::unordered_map<unsigned, TextureFormat> fourCCToTextureFormat = {
        {FOURCC_DXT1, TextureFormat::TEX_FORMAT_BC1_UNORM},
        {FOURCC_DXT3, TextureFormat::TEX_FORMAT_BC2_UNORM},
        {FOURCC_DXT5, TextureFormat::TEX_FORMAT_BC3_UNORM},
        {FOURCC_ETC1, TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM},
        {FOURCC_ETC2, TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM},
        {FOURCC_ETC2A, TextureFormat::TEX_FORMAT_ETC2_RGBA8_UNORM},
        {FOURCC_PTC2, EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_2BPP},
        {FOURCC_PTC4, EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_4BPP},
        {D3DFMT_A32B32G32R32F, TextureFormat::TEX_FORMAT_RGBA32_FLOAT},
        {D3DFMT_A16B16G16R16F, TextureFormat::TEX_FORMAT_RGBA16_FLOAT},
        {D3DFMT_A16B16G16R16, TextureFormat::TEX_FORMAT_RGBA16_UNORM},
    };

    static const ea::vector<TextureFormat> simpleFormats = {
        TextureFormat::TEX_FORMAT_RGBA8_UNORM,
        TextureFormat::TEX_FORMAT_B5G6R5_UNORM,
        TextureFormat::TEX_FORMAT_B5G5R5A1_UNORM,
    };

    if (const auto iter = dxgiToTextureFormat.find(dxgiFormat); iter != dxgiToTextureFormat.end())
        return iter->second;
    else if (const auto iter = fourCCToTextureFormat.find(pixelFormat.dwFourCC_); iter != fourCCToTextureFormat.end())
        return iter->second;

    for (const TextureFormat textureFormat : simpleFormats)
    {
        if (AreTextureComponentsMatching(pixelFormat, textureFormat))
            return textureFormat;
    }

    if (pixelFormat.dwRGBBitCount_ == 32)
        return TextureFormat::TEX_FORMAT_RGBA8_UNORM; // TODO: Support BGRA too?
    else
        return TextureFormat::TEX_FORMAT_UNKNOWN;
}

bool AreTextureComponentsMatching(const DDPixelFormat& pixelFormat, TextureFormat textureFormat)
{
    static const ea::unordered_map<TextureFormat, DDPixelFormat> pixelFormats = {
        {TextureFormat::TEX_FORMAT_RGBA8_UNORM, MakePixelFormat(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)},
        {TextureFormat::TEX_FORMAT_B5G6R5_UNORM, MakePixelFormat(16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000)},
        {TextureFormat::TEX_FORMAT_B5G5R5A1_UNORM, MakePixelFormat(16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000)},
    };

    const auto iter = pixelFormats.find(textureFormat);
    return iter != pixelFormats.end() && IsSamePixelFormat(iter->second, pixelFormat);
}

} // namespace Urho3D
