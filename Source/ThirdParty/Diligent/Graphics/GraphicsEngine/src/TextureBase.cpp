/*
 *  Copyright 2019-2023 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "TextureBase.hpp"

#include <algorithm>

#include "DeviceContext.h"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

namespace Diligent
{

#define LOG_TEXTURE_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Texture '", (Desc.Name ? Desc.Name : ""), "': ", ##__VA_ARGS__)
#define VERIFY_TEXTURE(Expr, ...)                     \
    do                                                \
    {                                                 \
        if (!(Expr))                                  \
        {                                             \
            LOG_TEXTURE_ERROR_AND_THROW(__VA_ARGS__); \
        }                                             \
    } while (false)

void ValidateTextureDesc(const TextureDesc& Desc, const IRenderDevice* pDevice) noexcept(false)
{
    const auto& FmtAttribs  = GetTextureFormatAttribs(Desc.Format);
    const auto& AdapterInfo = pDevice->GetAdapterInfo();
    const auto& DeviceInfo  = pDevice->GetDeviceInfo();

    if (Desc.Type == RESOURCE_DIM_UNDEFINED)
    {
        LOG_TEXTURE_ERROR_AND_THROW("Resource dimension is undefined.");
    }

    if (!(Desc.Type >= RESOURCE_DIM_TEX_1D && Desc.Type <= RESOURCE_DIM_TEX_CUBE_ARRAY))
    {
        LOG_TEXTURE_ERROR_AND_THROW("Unexpected resource dimension.");
    }

    if (Desc.Width == 0)
    {
        LOG_TEXTURE_ERROR_AND_THROW("Texture width cannot be zero.");
    }

    // Perform some parameter correctness check
    if (Desc.Type == RESOURCE_DIM_TEX_1D || Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
    {
        if (Desc.Height != FmtAttribs.BlockHeight)
        {
            if (FmtAttribs.BlockHeight == 1)
            {
                LOG_TEXTURE_ERROR_AND_THROW("Height (", Desc.Height, ") of a Texture 1D/Texture 1D Array must be 1.");
            }
            else
            {
                LOG_TEXTURE_ERROR_AND_THROW("For block-compressed formats, the height (", Desc.Height,
                                            ") of a Texture 1D/Texture 1D Array must be equal to the compressed block height (",
                                            Uint32{FmtAttribs.BlockHeight}, ").");
            }
        }
    }
    else
    {
        if (Desc.Height == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Texture height cannot be zero.");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_3D && Desc.Depth == 0)
    {
        LOG_TEXTURE_ERROR_AND_THROW("3D texture depth cannot be zero.");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_1D || Desc.Type == RESOURCE_DIM_TEX_2D)
    {
        if (Desc.ArraySize != 1)
            LOG_TEXTURE_ERROR_AND_THROW("Texture 1D/2D must have one array slice (", Desc.ArraySize, " provided). Use Texture 1D/2D array if you need more than one slice.");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        if (Desc.Width != Desc.Height)
            LOG_TEXTURE_ERROR_AND_THROW("For cube map textures, texture width (", Desc.Width, " provided) must match texture height (", Desc.Height, " provided).");

        if (Desc.ArraySize < 6)
            LOG_TEXTURE_ERROR_AND_THROW("Texture cube/cube array must have at least 6 slices (", Desc.ArraySize, " provided).");
    }

#ifdef DILIGENT_DEVELOPMENT
    {
        Uint32 MaxDim = 0;
        if (Desc.Is1D())
            MaxDim = Desc.Width;
        else if (Desc.Is2D())
            MaxDim = std::max(Desc.Width, Desc.Height);
        else if (Desc.Is3D())
            MaxDim = std::max(std::max(Desc.Width, Desc.Height), Desc.Depth);
        DEV_CHECK_ERR(MaxDim >= (1U << (Desc.MipLevels - 1)), "Texture '", Desc.Name ? Desc.Name : "", "': Incorrect number of Mip levels (", Desc.MipLevels, ").");
    }
#endif

    if (Desc.SampleCount > 1)
    {
        VERIFY_TEXTURE(IsPowerOfTwo(Desc.SampleCount), "SampleCount must be a power-of-two value");

        if (!(Desc.Type == RESOURCE_DIM_TEX_2D || Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY))
            LOG_TEXTURE_ERROR_AND_THROW("Only Texture 2D/Texture 2D Array can be multisampled");

        if (Desc.MipLevels != 1)
            LOG_TEXTURE_ERROR_AND_THROW("Multisampled textures must have one mip level (", Desc.MipLevels, " levels specified).");

        if (Desc.BindFlags & BIND_UNORDERED_ACCESS)
            LOG_TEXTURE_ERROR_AND_THROW("UAVs are not allowed for multisampled resources");
    }

    if ((Desc.BindFlags & BIND_RENDER_TARGET) &&
        (Desc.Format == TEX_FORMAT_R8_SNORM || Desc.Format == TEX_FORMAT_RG8_SNORM || Desc.Format == TEX_FORMAT_RGBA8_SNORM ||
         Desc.Format == TEX_FORMAT_R16_SNORM || Desc.Format == TEX_FORMAT_RG16_SNORM || Desc.Format == TEX_FORMAT_RGBA16_SNORM))
    {
        const auto* FmtName = GetTextureFormatAttribs(Desc.Format).Name;
        LOG_WARNING_MESSAGE(FmtName, " texture is created with BIND_RENDER_TARGET flag set.\n"
                                     "There might be an issue in OpenGL driver on NVidia hardware: when rendering to SNORM textures, all negative values are clamped to zero.\n"
                                     "Use UNORM format instead.");
    }

    if (Desc.MiscFlags & MISC_TEXTURE_FLAG_MEMORYLESS)
    {
        const auto& MemInfo = AdapterInfo.Memory;

        if (MemInfo.MemorylessTextureBindFlags == BIND_NONE)
            LOG_TEXTURE_ERROR_AND_THROW("Memoryless textures are not supported by device");

        if ((Desc.BindFlags & MemInfo.MemorylessTextureBindFlags) != Desc.BindFlags)
            LOG_TEXTURE_ERROR_AND_THROW("BindFlags ", GetBindFlagsString(Desc.BindFlags & ~MemInfo.MemorylessTextureBindFlags), " are not supported for memoryless textures.");

        if (Desc.Usage != USAGE_DEFAULT)
            LOG_TEXTURE_ERROR_AND_THROW("Memoryless attachment requires USAGE_DEFAULT.");

        if (Desc.CPUAccessFlags != 0)
            LOG_TEXTURE_ERROR_AND_THROW("Memoryless attachment requires CPUAccessFlags to be NONE.");

        if (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
            LOG_TEXTURE_ERROR_AND_THROW("Memoryless attachment is not compatible with mipmap generation.");
    }

    if (Desc.Usage == USAGE_STAGING)
    {
        if (Desc.BindFlags != 0)
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures cannot be bound to any GPU pipeline stage.");

        if (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
            LOG_TEXTURE_ERROR_AND_THROW("Mipmaps cannot be autogenerated for staging textures.");

        if (Desc.CPUAccessFlags == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures must specify CPU access flags.");

        if ((Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == (CPU_ACCESS_READ | CPU_ACCESS_WRITE))
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures must use exactly one of ACCESS_READ or ACCESS_WRITE flags.");
    }
    else if (Desc.Usage == USAGE_UNIFIED)
    {
        LOG_TEXTURE_ERROR_AND_THROW("USAGE_UNIFIED textures are currently not supported.");
    }

    if (Desc.Usage == USAGE_DYNAMIC &&
        PlatformMisc::CountOneBits(Desc.ImmediateContextMask) > 1)
    {
        // Dynamic textures always use backing resource that requires implicit state transitions
        // in map/unmap operations, which is not safe in multiple contexts.
        LOG_TEXTURE_ERROR_AND_THROW("USAGE_DYNAMIC textures may only be used in one immediate device context.");
    }

    const auto& SRProps = pDevice->GetAdapterInfo().ShadingRate;
    if (Desc.MiscFlags & MISC_TEXTURE_FLAG_SUBSAMPLED)
    {
        if (!pDevice->GetDeviceInfo().Features.VariableRateShading)
            LOG_TEXTURE_ERROR_AND_THROW("MISC_TEXTURE_FLAG_SUBSAMPLED requires VariableRateShading feature.");

        if (pDevice->GetDeviceInfo().IsMetalDevice())
            LOG_TEXTURE_ERROR_AND_THROW("MISC_TEXTURE_FLAG_SUBSAMPLED is not supported in Metal, use IRasterizationRateMapMtl to implement VRS in Metal");

        if ((SRProps.CapFlags & SHADING_RATE_CAP_FLAG_SUBSAMPLED_RENDER_TARGET) == 0)
            LOG_TEXTURE_ERROR_AND_THROW("MISC_TEXTURE_FLAG_SUBSAMPLED requires SHADING_RATE_CAP_FLAG_SUBSAMPLED_RENDER_TARGET capability.");

        if ((Desc.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Subsampled texture must use one of BIND_RENDER_TARGET or BIND_DEPTH_STENCIL bind flags");

        if (Desc.BindFlags & BIND_SHADING_RATE)
            LOG_TEXTURE_ERROR_AND_THROW("MISC_TEXTURE_FLAG_SUBSAMPLED is not compatible with BIND_SHADING_RATE");
    }

    if (Desc.BindFlags & BIND_SHADING_RATE)
    {
        if (!pDevice->GetDeviceInfo().Features.VariableRateShading)
            LOG_TEXTURE_ERROR_AND_THROW("BIND_SHADING_RATE requires VariableRateShading feature.");

        if (pDevice->GetDeviceInfo().IsMetalDevice())
            LOG_TEXTURE_ERROR_AND_THROW("BIND_SHADING_RATE is not supported in Metal, use IRasterizationRateMapMtl instead.");

        if (DeviceInfo.IsMetalDevice())
            LOG_TEXTURE_ERROR_AND_THROW("BIND_FLAG_SHADING_RATE is not supported in Metal, use IRasterizationRateMapMtl instead.");

        if ((SRProps.CapFlags & SHADING_RATE_CAP_FLAG_TEXTURE_BASED) == 0)
            LOG_TEXTURE_ERROR_AND_THROW("BIND_SHADING_RATE requires SHADING_RATE_CAP_FLAG_TEXTURE_BASED capability.");

        if (Desc.SampleCount != 1)
            LOG_TEXTURE_ERROR_AND_THROW("BIND_SHADING_RATE is not allowed for multisample texture.");

        if (Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY && Desc.ArraySize > 1 && (SRProps.CapFlags & SHADING_RATE_CAP_FLAG_TEXTURE_ARRAY) == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture arrays require SHADING_RATE_CAP_FLAG_TEXTURE_ARRAY capability");

        if (Desc.Usage != USAGE_DEFAULT && Desc.Usage != USAGE_IMMUTABLE)
            LOG_TEXTURE_ERROR_AND_THROW("Shading rate textures only allow USAGE_DEFAULT or USAGE_IMMUTABLE.");

        // For Direct3D12 and Vulkan with VK_EXT_fragment_density_map
        if (Desc.MipLevels != 1)
            LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture must have 1 mip level.");

        if ((Desc.BindFlags & ~SRProps.BindFlags) != 0)
            LOG_TEXTURE_ERROR_AND_THROW("the following bind flags are not allowed for a shading rate texture: ", GetBindFlagsString(Desc.BindFlags & ~SRProps.BindFlags, ", "), '.');

        // TODO: Vulkan allows to create 2D texture array and then use single slice for view even if SHADING_RATE_CAP_FLAG_TEXTURE_ARRAY capability is not supported
        if (Desc.Type != RESOURCE_DIM_TEX_2D && !(Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY && (SRProps.CapFlags & SHADING_RATE_CAP_FLAG_TEXTURE_ARRAY) != 0))
            LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture must be 2D or 2D Array with SHADING_RATE_CAP_FLAG_TEXTURE_ARRAY capability.");

        switch (SRProps.Format)
        {
            case SHADING_RATE_FORMAT_PALETTE:
                if (Desc.Format != TEX_FORMAT_R8_UINT)
                    LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture format must be R8_UINT.");
                break;

            case SHADING_RATE_FORMAT_UNORM8:
                if (Desc.Format != TEX_FORMAT_RG8_UNORM)
                    LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture format must be RG8_UNORM.");
                break;

            case SHADING_RATE_FORMAT_COL_ROW_FP32:
            default:
                LOG_TEXTURE_ERROR_AND_THROW("Shading rate texture is not supported.");
        }
    }

    if (Desc.Usage == USAGE_SPARSE)
    {
        VERIFY_TEXTURE(DeviceInfo.Features.SparseResources, "sparse texture requires SparseResources feature");

        const auto& SparseRes = AdapterInfo.SparseResources;

        if ((Desc.MiscFlags & MISC_TEXTURE_FLAG_SPARSE_ALIASING) != 0)
            VERIFY_TEXTURE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_ALIASED) != 0, "MISC_TEXTURE_FLAG_SPARSE_ALIASING flag requires SPARSE_RESOURCE_CAP_FLAG_ALIASED capability");

        static_assert(RESOURCE_DIM_NUM_DIMENSIONS == 9, "Please update the switch below to handle the new resource dimension type");
        switch (Desc.Type)
        {
            case RESOURCE_DIM_TEX_2D:
                VERIFY_TEXTURE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) != 0,
                               "2D texture requires SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D capability");
                break;

            case RESOURCE_DIM_TEX_2D_ARRAY:
            case RESOURCE_DIM_TEX_CUBE:
            case RESOURCE_DIM_TEX_CUBE_ARRAY:
                VERIFY_TEXTURE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) != 0,
                               "2D array or Cube sparse textures requires SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D capability");

                if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
                {
                    const auto  Props = GetStandardSparseTextureProperties(Desc);
                    const uint2 MipSize{std::max(1u, Desc.Width >> Desc.MipLevels),
                                        std::max(1u, Desc.Height >> Desc.MipLevels)};
                    VERIFY_TEXTURE(MipSize.x >= Props.TileSize[0] && MipSize.y >= Props.TileSize[1],
                                   "2D array or Cube sparse texture with mip level count ", Desc.MipLevels,
                                   ", where the last mip with dimension (", MipSize.x, "x", MipSize.y, ") is less than the tile size (",
                                   Props.TileSize[0], "x", Props.TileSize[1], ") requires SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL capability");
                }
                break;

            case RESOURCE_DIM_TEX_3D:
                VERIFY_TEXTURE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D) != 0,
                               "3D sparse texture requires SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D capability");
                break;

            case RESOURCE_DIM_TEX_1D:
            case RESOURCE_DIM_TEX_1D_ARRAY:
                LOG_TEXTURE_ERROR_AND_THROW("Sparse 1D textures are not supported");
                break;

            default:
                LOG_TEXTURE_ERROR_AND_THROW("unknown texture type");
        }
    }
    else
    {
        VERIFY_TEXTURE((Desc.MiscFlags & MISC_TEXTURE_FLAG_SPARSE_ALIASING) == 0,
                       "MiscFlags must not have MISC_TEXTURE_FLAG_SPARSE_ALIASING if usage is not USAGE_SPARSE");
    }
}


void ValidateTextureRegion(const TextureDesc& TexDesc, Uint32 MipLevel, Uint32 Slice, const Box& Box)
{
#define VERIFY_TEX_PARAMS(Expr, ...)                                                          \
    do                                                                                        \
    {                                                                                         \
        if (!(Expr))                                                                          \
        {                                                                                     \
            LOG_ERROR("Texture '", (TexDesc.Name ? TexDesc.Name : ""), "': ", ##__VA_ARGS__); \
        }                                                                                     \
    } while (false)

#ifdef DILIGENT_DEVELOPMENT
    VERIFY_TEX_PARAMS(MipLevel < TexDesc.MipLevels, "Mip level (", MipLevel, ") is out of allowed range [0, ", TexDesc.MipLevels - 1, "]");
    VERIFY_TEX_PARAMS(Box.MinX < Box.MaxX, "Invalid X range: ", Box.MinX, "..", Box.MaxX);
    VERIFY_TEX_PARAMS(Box.MinY < Box.MaxY, "Invalid Y range: ", Box.MinY, "..", Box.MaxY);
    VERIFY_TEX_PARAMS(Box.MinZ < Box.MaxZ, "Invalid Z range: ", Box.MinZ, "..", Box.MaxZ);

    if (TexDesc.IsArray())
    {
        VERIFY_TEX_PARAMS(Slice < TexDesc.ArraySize, "Array slice (", Slice, ") is out of range [0,", TexDesc.ArraySize - 1, "].");
    }
    else
    {
        VERIFY_TEX_PARAMS(Slice == 0, "Array slice (", Slice, ") must be 0 for non-array textures.");
    }

    const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    Uint32 MipWidth = std::max(TexDesc.Width >> MipLevel, 1U);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        VERIFY_EXPR((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0);
        Uint32 BlockAlignedMipWidth = (MipWidth + (FmtAttribs.BlockWidth - 1)) & ~(FmtAttribs.BlockWidth - 1);
        VERIFY_TEX_PARAMS(Box.MaxX <= BlockAlignedMipWidth, "Region max X coordinate (", Box.MaxX, ") is out of allowed range [0, ", BlockAlignedMipWidth, "].");
        VERIFY_TEX_PARAMS((Box.MinX % FmtAttribs.BlockWidth) == 0, "For compressed formats, the region min X coordinate (", Box.MinX, ") must be a multiple of block width (", Uint32{FmtAttribs.BlockWidth}, ").");
        VERIFY_TEX_PARAMS((Box.MaxX % FmtAttribs.BlockWidth) == 0 || Box.MaxX == MipWidth, "For compressed formats, the region max X coordinate (", Box.MaxX, ") must be a multiple of block width (", Uint32{FmtAttribs.BlockWidth}, ") or equal the mip level width (", MipWidth, ").");
    }
    else
        VERIFY_TEX_PARAMS(Box.MaxX <= MipWidth, "Region max X coordinate (", Box.MaxX, ") is out of allowed range [0, ", MipWidth, "].");

    if (TexDesc.Type != RESOURCE_DIM_TEX_1D &&
        TexDesc.Type != RESOURCE_DIM_TEX_1D_ARRAY)
    {
        Uint32 MipHeight = std::max(TexDesc.Height >> MipLevel, 1U);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            VERIFY_EXPR((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0);
            Uint32 BlockAlignedMipHeight = (MipHeight + (FmtAttribs.BlockHeight - 1)) & ~(FmtAttribs.BlockHeight - 1);
            VERIFY_TEX_PARAMS(Box.MaxY <= BlockAlignedMipHeight, "Region max Y coordinate (", Box.MaxY, ") is out of allowed range [0, ", BlockAlignedMipHeight, "].");
            VERIFY_TEX_PARAMS((Box.MinY % FmtAttribs.BlockHeight) == 0, "For compressed formats, the region min Y coordinate (", Box.MinY, ") must be a multiple of block height (", Uint32{FmtAttribs.BlockHeight}, ").");
            VERIFY_TEX_PARAMS((Box.MaxY % FmtAttribs.BlockHeight) == 0 || Box.MaxY == MipHeight, "For compressed formats, the region max Y coordinate (", Box.MaxY, ") must be a multiple of block height (", Uint32{FmtAttribs.BlockHeight}, ") or equal the mip level height (", MipHeight, ").");
        }
        else
            VERIFY_TEX_PARAMS(Box.MaxY <= MipHeight, "Region max Y coordinate (", Box.MaxY, ") is out of allowed range [0, ", MipHeight, "].");
    }

    if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
    {
        Uint32 MipDepth = std::max(TexDesc.Depth >> MipLevel, 1U);
        VERIFY_TEX_PARAMS(Box.MaxZ <= MipDepth, "Region max Z coordinate (", Box.MaxZ, ") is out of allowed range  [0, ", MipDepth, "].");
    }
    else
    {
        VERIFY_TEX_PARAMS(Box.MinZ == 0, "Region min Z (", Box.MinZ, ") must be 0 for all but 3D textures.");
        VERIFY_TEX_PARAMS(Box.MaxZ == 1, "Region max Z (", Box.MaxZ, ") must be 1 for all but 3D textures.");
    }
#endif
}

void ValidateUpdateTextureParams(const TextureDesc& TexDesc, Uint32 MipLevel, Uint32 Slice, const Box& DstBox, const TextureSubResData& SubresData)
{
    VERIFY((SubresData.pData != nullptr) ^ (SubresData.pSrcBuffer != nullptr), "Either CPU data pointer (pData) or GPU buffer (pSrcBuffer) must not be null, but not both.");
    ValidateTextureRegion(TexDesc, MipLevel, Slice, DstBox);

#ifdef DILIGENT_DEVELOPMENT
    VERIFY_TEX_PARAMS(TexDesc.SampleCount == 1, "Only non-multisampled textures can be updated with UpdateData().");
    VERIFY_TEX_PARAMS((SubresData.Stride & 0x03) == 0, "Texture data stride (", SubresData.Stride, ") must be at least 32-bit aligned.");
    VERIFY_TEX_PARAMS((SubresData.DepthStride & 0x03) == 0, "Texture data depth stride (", SubresData.DepthStride, ") must be at least 32-bit aligned.");

    auto        UpdateRegionWidth  = DstBox.Width();
    auto        UpdateRegionHeight = DstBox.Height();
    auto        UpdateRegionDepth  = DstBox.Depth();
    const auto& FmtAttribs         = GetTextureFormatAttribs(TexDesc.Format);
    Uint32      RowSize            = 0;
    Uint32      RowCount           = 0;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region size by the block size. This is only necessary when updating
        // coarse mip levels. Otherwise UpdateRegionWidth/Height should be multiples of block size
        VERIFY_EXPR((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0);
        VERIFY_EXPR((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0);
        UpdateRegionWidth  = (UpdateRegionWidth + (FmtAttribs.BlockWidth - 1)) & ~(FmtAttribs.BlockWidth - 1);
        UpdateRegionHeight = (UpdateRegionHeight + (FmtAttribs.BlockHeight - 1)) & ~(FmtAttribs.BlockHeight - 1);
        RowSize            = UpdateRegionWidth / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize};
        RowCount           = UpdateRegionHeight / FmtAttribs.BlockHeight;
    }
    else
    {
        RowSize  = UpdateRegionWidth * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        RowCount = UpdateRegionHeight;
    }
    DEV_CHECK_ERR(SubresData.Stride >= RowSize, "Source data stride (", SubresData.Stride, ") is below the image row size (", RowSize, ").");
    const auto PlaneSize = SubresData.Stride * RowCount;
    DEV_CHECK_ERR(UpdateRegionDepth == 1 || SubresData.DepthStride >= PlaneSize, "Source data depth stride (", SubresData.DepthStride, ") is below the image plane size (", PlaneSize, ").");
#endif
}

void ValidateCopyTextureParams(const CopyTextureAttribs& CopyAttribs)
{
    VERIFY_EXPR(CopyAttribs.pSrcTexture != nullptr && CopyAttribs.pDstTexture != nullptr);
    Box         SrcBox;
    const auto& SrcTexDesc = CopyAttribs.pSrcTexture->GetDesc();
    const auto& DstTexDesc = CopyAttribs.pDstTexture->GetDesc();
    auto        pSrcBox    = CopyAttribs.pSrcBox;
    if (pSrcBox == nullptr)
    {
        auto MipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);
        SrcBox.MaxX          = MipLevelAttribs.LogicalWidth;
        SrcBox.MaxY          = MipLevelAttribs.LogicalHeight;
        SrcBox.MaxZ          = MipLevelAttribs.Depth;
        pSrcBox              = &SrcBox;
    }
    ValidateTextureRegion(SrcTexDesc, CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, *pSrcBox);

    Box DstBox;
    DstBox.MinX = CopyAttribs.DstX;
    DstBox.MinY = CopyAttribs.DstY;
    DstBox.MinZ = CopyAttribs.DstZ;
    DstBox.MaxX = DstBox.MinX + pSrcBox->Width();
    DstBox.MaxY = DstBox.MinY + pSrcBox->Height();
    DstBox.MaxZ = DstBox.MinZ + pSrcBox->Depth();
    ValidateTextureRegion(DstTexDesc, CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, DstBox);
}

void ValidateMapTextureParams(const TextureDesc& TexDesc,
                              Uint32             MipLevel,
                              Uint32             ArraySlice,
                              MAP_TYPE           MapType,
                              Uint32             MapFlags,
                              const Box*         pMapRegion)
{
    VERIFY_TEX_PARAMS(MipLevel < TexDesc.MipLevels, "Mip level (", MipLevel, ") is out of allowed range [0, ", TexDesc.MipLevels - 1, "].");
    if (TexDesc.IsArray())
    {
        VERIFY_TEX_PARAMS(ArraySlice < TexDesc.ArraySize, "Array slice (", ArraySlice, ") is out of range [0,", TexDesc.ArraySize - 1, "].");
    }
    else
    {
        VERIFY_TEX_PARAMS(ArraySlice == 0, "Array slice (", ArraySlice, ") must be 0 for non-array textures.");
    }

    if (pMapRegion != nullptr)
    {
        ValidateTextureRegion(TexDesc, MipLevel, ArraySlice, *pMapRegion);
    }
}

void ValidatedAndCorrectTextureViewDesc(const TextureDesc& TexDesc, TextureViewDesc& ViewDesc) noexcept(false)
{
#define TEX_VIEW_VALIDATION_ERROR(...) LOG_ERROR_AND_THROW("\n                 Failed to create texture view '", (ViewDesc.Name ? ViewDesc.Name : ""), "' for texture '", TexDesc.Name, "': ", ##__VA_ARGS__)

    if (!(ViewDesc.ViewType > TEXTURE_VIEW_UNDEFINED && ViewDesc.ViewType < TEXTURE_VIEW_NUM_VIEWS))
        TEX_VIEW_VALIDATION_ERROR("Texture view type is not specified.");

    if (ViewDesc.MostDetailedMip >= TexDesc.MipLevels)
        TEX_VIEW_VALIDATION_ERROR("Most detailed mip (", ViewDesc.MostDetailedMip, ") is out of range. The texture has only ", TexDesc.MipLevels, " mip ", (TexDesc.MipLevels > 1 ? "levels." : "level."));

    if (ViewDesc.NumMipLevels != REMAINING_MIP_LEVELS && ViewDesc.MostDetailedMip + ViewDesc.NumMipLevels > TexDesc.MipLevels)
        TEX_VIEW_VALIDATION_ERROR("Most detailed mip (", ViewDesc.MostDetailedMip, ") and number of mip levels in the view (", ViewDesc.NumMipLevels, ") is out of range. The texture has only ", TexDesc.MipLevels, " mip ", (TexDesc.MipLevels > 1 ? "levels." : "level."));

    if (ViewDesc.Format == TEX_FORMAT_UNKNOWN)
        ViewDesc.Format = GetDefaultTextureViewFormat(TexDesc.Format, ViewDesc.ViewType, TexDesc.BindFlags);

    if (TexDesc.IsArray())
    {
        if (ViewDesc.FirstArraySlice >= TexDesc.ArraySize)
            TEX_VIEW_VALIDATION_ERROR("First array slice (", ViewDesc.FirstArraySlice, ") is out of range. The texture has only (", TexDesc.ArraySize, ") slices.");

        if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && ViewDesc.FirstArraySlice + ViewDesc.NumArraySlices > TexDesc.ArraySize)
            TEX_VIEW_VALIDATION_ERROR("First array slice (", ViewDesc.FirstArraySlice, ") and number of array slices (", ViewDesc.NumArraySlices,
                                      ") is out of range. The texture has only (", TexDesc.ArraySize, ") slices.");
    }
    else if (!TexDesc.Is3D())
    {
        if (ViewDesc.FirstArraySlice != 0)
            TEX_VIEW_VALIDATION_ERROR("For non-array texture FirstArraySlice must be 0");
    }

    if (ViewDesc.TextureDim == RESOURCE_DIM_UNDEFINED)
    {
        if (TexDesc.Type == RESOURCE_DIM_TEX_CUBE || TexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        {
            switch (ViewDesc.ViewType)
            {
                case TEXTURE_VIEW_SHADER_RESOURCE:
                    ViewDesc.TextureDim = TexDesc.Type;
                    break;

                case TEXTURE_VIEW_RENDER_TARGET:
                case TEXTURE_VIEW_DEPTH_STENCIL:
                case TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL:
                case TEXTURE_VIEW_UNORDERED_ACCESS:
                    ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                    break;

                default: UNEXPECTED("Unexpected view type");
            }
        }
        else
        {
            ViewDesc.TextureDim = TexDesc.Type;
        }
    }

    switch (TexDesc.Type)
    {
        case RESOURCE_DIM_TEX_1D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 1D view: only Texture 1D is allowed.");
            }
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect view type for Texture 1D Array: only Texture 1D or Texture 1D Array are allowed.");
            }
            break;

        case RESOURCE_DIM_TEX_2D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 2D view: only Texture 2D or Texture 2D Array are allowed.");
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 2D Array view: only Texture 2D or Texture 2D Array are allowed.");
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_3D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 3D view: only Texture 3D is allowed.");
            }
            break;

        case RESOURCE_DIM_TEX_CUBE:
            if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube SRV: Texture 2D, Texture 2D array or Texture Cube is allowed.");
                }
            }
            else
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube non-shader resource view: Texture 2D or Texture 2D array is allowed.");
                }
            }
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube array SRV: Texture 2D, Texture 2D array, Texture Cube or Texture Cube Array is allowed.");
                }
            }
            else
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube array non-shader resource view: Texture 2D or Texture 2D array is allowed.");
                }
            }
            break;

        default:
            UNEXPECTED("Unexpected texture type");
            break;
    }

    switch (ViewDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_CUBE:
            if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
                TEX_VIEW_VALIDATION_ERROR("Unexpected view type: SRV is expected.");
            if (ViewDesc.NumArraySlices != 6 && ViewDesc.NumArraySlices != 0 && ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES)
                TEX_VIEW_VALIDATION_ERROR("Texture cube SRV is expected to have 6 array slices, while ", ViewDesc.NumArraySlices, " is provided.");
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
                TEX_VIEW_VALIDATION_ERROR("Unexpected view type: SRV is expected.");
            if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && (ViewDesc.NumArraySlices % 6) != 0)
                TEX_VIEW_VALIDATION_ERROR("Number of slices in texture cube array SRV is expected to be multiple of 6. ", ViewDesc.NumArraySlices, " slices is provided.");
            break;

        case RESOURCE_DIM_TEX_1D:
        case RESOURCE_DIM_TEX_2D:
            if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && ViewDesc.NumArraySlices > 1)
                TEX_VIEW_VALIDATION_ERROR("Number of slices in the view (", ViewDesc.NumArraySlices, ") must be 1 (or 0) for non-array texture 1D/2D views.");
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
        case RESOURCE_DIM_TEX_2D_ARRAY:
            break;

        case RESOURCE_DIM_TEX_3D:
        {
            auto MipDepth = std::max(TexDesc.Depth >> ViewDesc.MostDetailedMip, 1u);
            if (ViewDesc.FirstDepthSlice + ViewDesc.NumDepthSlices > MipDepth)
                TEX_VIEW_VALIDATION_ERROR("First slice (", ViewDesc.FirstDepthSlice, ") and number of slices in the view (", ViewDesc.NumDepthSlices, ") specify more slices than target 3D texture mip level has (", MipDepth, ").");
            break;
        }

        default:
            UNEXPECTED("Unexpected texture dimension");
    }

    if (GetTextureFormatAttribs(ViewDesc.Format).IsTypeless)
    {
        TEX_VIEW_VALIDATION_ERROR("Texture view format (", GetTextureFormatAttribs(ViewDesc.Format).Name, ") cannot be typeless.");
    }

    if ((ViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0)
    {
        if ((TexDesc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) == 0)
            TEX_VIEW_VALIDATION_ERROR("TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag can only set if the texture was created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag.");

        if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
            TEX_VIEW_VALIDATION_ERROR("TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag can only be used with TEXTURE_VIEW_SHADER_RESOURCE view type.");
    }

    if (ViewDesc.ViewType == TEXTURE_VIEW_SHADING_RATE)
    {
        if ((TexDesc.BindFlags & BIND_SHADING_RATE) == 0)
            TEX_VIEW_VALIDATION_ERROR("To create TEXTURE_VIEW_SHADING_RATE, the texture must be created with BIND_SHADING_RATE flag.");
    }

    if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE && !IsIdentityComponentMapping(ViewDesc.Swizzle))
        TEX_VIEW_VALIDATION_ERROR("Non-identity texture component swizzle is only supported for shader resource views.");

#undef TEX_VIEW_VALIDATION_ERROR

    if (ViewDesc.NumMipLevels == 0 || ViewDesc.NumMipLevels == REMAINING_MIP_LEVELS)
    {
        if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            ViewDesc.NumMipLevels = TexDesc.MipLevels - ViewDesc.MostDetailedMip;
        else
            ViewDesc.NumMipLevels = 1;
    }

    if (ViewDesc.NumArraySlices == 0 || ViewDesc.NumArraySlices == REMAINING_ARRAY_SLICES)
    {
        if (TexDesc.IsArray())
            ViewDesc.NumArraySlices = TexDesc.ArraySize - ViewDesc.FirstArraySlice;
        else if (TexDesc.Is3D())
        {
            auto MipDepth           = std::max(TexDesc.Depth >> ViewDesc.MostDetailedMip, 1u);
            ViewDesc.NumDepthSlices = MipDepth - ViewDesc.FirstDepthSlice;
        }
        else
            ViewDesc.NumArraySlices = 1;
    }

    if ((ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET) &&
        (ViewDesc.Format == TEX_FORMAT_R8_SNORM || ViewDesc.Format == TEX_FORMAT_RG8_SNORM || ViewDesc.Format == TEX_FORMAT_RGBA8_SNORM ||
         ViewDesc.Format == TEX_FORMAT_R16_SNORM || ViewDesc.Format == TEX_FORMAT_RG16_SNORM || ViewDesc.Format == TEX_FORMAT_RGBA16_SNORM))
    {
        const auto* FmtName = GetTextureFormatAttribs(ViewDesc.Format).Name;
        LOG_WARNING_MESSAGE(FmtName, " render target view is created.\n"
                                     "There might be an issue in OpenGL driver on NVidia hardware: when rendering to SNORM textures, all negative values are clamped to zero.\n"
                                     "Use UNORM format instead.");
    }
}

} // namespace Diligent
