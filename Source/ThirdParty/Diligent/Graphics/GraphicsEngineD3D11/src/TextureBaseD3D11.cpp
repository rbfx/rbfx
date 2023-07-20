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

#include "pch.h"
#include "TextureBaseD3D11.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"
#include "D3D11TypeConversions.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

TextureBaseD3D11::TextureBaseD3D11(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& TexViewObjAllocator,
                                   RenderDeviceD3D11Impl*     pRenderDeviceD3D11,
                                   const TextureDesc&         TexDesc,
                                   const TextureData*         pInitData /*= nullptr*/) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pRenderDeviceD3D11,
        TexDesc
    }
// clang-format on
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    if (m_Desc.Usage == USAGE_SPARSE)
    {
        constexpr auto AllowedBindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET | BIND_DEPTH_STENCIL;
        if ((m_Desc.BindFlags & ~AllowedBindFlags) != 0)
            LOG_ERROR_AND_THROW("Texture '", m_Desc.Name, "': the following bind flags are not allowed for sparse textures in Direct3D11: ", GetBindFlagsString(m_Desc.BindFlags & ~AllowedBindFlags, ", "), '.');
    }

    SetState(RESOURCE_STATE_UNDEFINED);
}

IMPLEMENT_QUERY_INTERFACE(TextureBaseD3D11, IID_TextureD3D11, TTextureBase)

void TextureBaseD3D11::CreateViewInternal(const TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto UpdatedViewDesc = ViewDesc;
        ValidatedAndCorrectTextureViewDesc(m_Desc, UpdatedViewDesc);

        if (m_Desc.IsArray() && (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D || ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D))
        {
            if (ViewDesc.FirstArraySlice != 0)
                LOG_ERROR_AND_THROW("FirstArraySlice must be 0; slice offset is not supported for non-array views in Direct3D11");
        }

        RefCntAutoPtr<ID3D11View> pD3D11View;
        switch (ViewDesc.ViewType)
        {
            case TEXTURE_VIEW_SHADER_RESOURCE:
            {
                if (!IsIdentityComponentMapping(ViewDesc.Swizzle))
                    LOG_ERROR_AND_THROW("Direct3D11 does not support texture component swizzle");

                VERIFY(m_Desc.BindFlags & BIND_SHADER_RESOURCE, "BIND_SHADER_RESOURCE flag is not set");
                ID3D11ShaderResourceView* pSRV = nullptr;
                CreateSRV(UpdatedViewDesc, &pSRV);
                pD3D11View.Attach(pSRV);
            }
            break;

            case TEXTURE_VIEW_RENDER_TARGET:
            {
                VERIFY(m_Desc.BindFlags & BIND_RENDER_TARGET, "BIND_RENDER_TARGET flag is not set");
                ID3D11RenderTargetView* pRTV = nullptr;
                CreateRTV(UpdatedViewDesc, &pRTV);
                pD3D11View.Attach(pRTV);
            }
            break;

            case TEXTURE_VIEW_DEPTH_STENCIL:
            case TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL:
            {
                VERIFY(m_Desc.BindFlags & BIND_DEPTH_STENCIL, "BIND_DEPTH_STENCIL flag is not set");
                ID3D11DepthStencilView* pDSV = nullptr;
                CreateDSV(UpdatedViewDesc, &pDSV);
                pD3D11View.Attach(pDSV);
            }
            break;

            case TEXTURE_VIEW_UNORDERED_ACCESS:
            {
                VERIFY(m_Desc.BindFlags & BIND_UNORDERED_ACCESS, "BIND_UNORDERED_ACCESS flag is not set");
                ID3D11UnorderedAccessView* pUAV = nullptr;
                CreateUAV(UpdatedViewDesc, &pUAV);
                pD3D11View.Attach(pUAV);
            }
            break;

            default: UNEXPECTED("Unknown view type"); break;
        }

        auto* pDeviceD3D11Impl = GetDevice();
        auto& TexViewAllocator = pDeviceD3D11Impl->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        auto pViewD3D11 = NEW_RC_OBJ(TexViewAllocator, "TextureViewD3D11Impl instance", TextureViewD3D11Impl, bIsDefaultView ? this : nullptr)(pDeviceD3D11Impl, UpdatedViewDesc, this, pD3D11View, bIsDefaultView);
        VERIFY(pViewD3D11->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewD3D11;
        else
            pViewD3D11->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

void TextureBaseD3D11::PrepareD3D11InitData(const TextureData*                                                               pInitData,
                                            Uint32                                                                           NumSubresources,
                                            std::vector<D3D11_SUBRESOURCE_DATA, STDAllocatorRawMem<D3D11_SUBRESOURCE_DATA>>& D3D11InitData)
{
    if (pInitData != nullptr && pInitData->pSubResources != nullptr)
    {
        if (NumSubresources == pInitData->NumSubresources)
        {
            D3D11InitData.resize(NumSubresources);
            for (UINT Subres = 0; Subres < NumSubresources; ++Subres)
            {
                auto& CurrSubres                       = pInitData->pSubResources[Subres];
                D3D11InitData[Subres].pSysMem          = CurrSubres.pData;
                D3D11InitData[Subres].SysMemPitch      = StaticCast<UINT>(CurrSubres.Stride);
                D3D11InitData[Subres].SysMemSlicePitch = StaticCast<UINT>(CurrSubres.DepthStride);
            }
        }
        else
        {
            UNEXPECTED("Incorrect number of subresources");
        }
    }
}

TextureBaseD3D11::~TextureBaseD3D11()
{
}

void TextureBaseD3D11::InitSparseProperties()
{
    VERIFY_EXPR(m_Desc.Usage == USAGE_SPARSE);
    VERIFY_EXPR(m_pSparseProps == nullptr);

    m_pSparseProps = std::make_unique<SparseTextureProperties>();

    if (IsUsingNVApi())
    {
        *m_pSparseProps = GetStandardSparseTextureProperties(m_Desc);
    }
    else
    {
        auto* pd3d11Device2 = m_pDevice->GetD3D11Device2();

        UINT                  NumTilesForEntireResource = 0;
        D3D11_PACKED_MIP_DESC PackedMipDesc{};
        D3D11_TILE_SHAPE      StandardTileShapeForNonPackedMips{};
        UINT                  NumSubresourceTilings = 0;
        pd3d11Device2->GetResourceTiling(m_pd3d11Texture,
                                         &NumTilesForEntireResource,
                                         &PackedMipDesc,
                                         &StandardTileShapeForNonPackedMips,
                                         &NumSubresourceTilings,
                                         0,
                                         nullptr);

        auto& Props{*m_pSparseProps};
        Props.AddressSpaceSize = Uint64{NumTilesForEntireResource} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.BlockSize        = D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.MipTailOffset    = Uint64{PackedMipDesc.StartTileIndexInOverallResource} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.MipTailSize      = Uint64{PackedMipDesc.NumTilesForPackedMips} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.FirstMipInTail   = PackedMipDesc.NumStandardMips;
        Props.TileSize[0]      = StandardTileShapeForNonPackedMips.WidthInTexels;
        Props.TileSize[1]      = StandardTileShapeForNonPackedMips.HeightInTexels;
        Props.TileSize[2]      = StandardTileShapeForNonPackedMips.DepthInTexels;
        Props.Flags            = SPARSE_TEXTURE_FLAG_NONE;

        // The number of overall tiles, packed or not, for a given array slice is simply the total number of tiles for the resource divided by the resource's array size
        Props.MipTailStride = m_Desc.IsArray() ? (NumTilesForEntireResource / m_Desc.ArraySize) * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES : 0;
        VERIFY_EXPR(NumTilesForEntireResource % m_Desc.GetArraySize() == 0);
    }
}

} // namespace Diligent
