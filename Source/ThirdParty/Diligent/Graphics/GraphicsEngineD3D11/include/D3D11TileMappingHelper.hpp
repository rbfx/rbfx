/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#pragma once

/// \file
/// D3D11 tile mapping helper

#include <functional>

#include "D3D11TypeDefinitions.h"
#include "DeviceMemoryD3D11.h"
#include "D3DTileMappingHelper.hpp"
#include "RefCntAutoPtr.hpp"
#include "BufferD3D11Impl.hpp"
#include "TextureBaseD3D11.hpp"

namespace Diligent
{

struct D3D11TileMappingHelper : D3DTileMappingHelper<D3D11_TILED_RESOURCE_COORDINATE, D3D11_TILE_REGION_SIZE, UINT, D3D11TileMappingHelper>
{
    using TBase = D3DTileMappingHelper<D3D11_TILED_RESOURCE_COORDINATE, D3D11_TILE_REGION_SIZE, UINT, D3D11TileMappingHelper>;

    RefCntAutoPtr<IDeviceMemoryD3D11> pMemory;

    UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, const TextureDesc& TexDesc) const
    {
        VERIFY(PlaneSlice == 0, "Plane slices are not supported in D3D11");
        return D3D11CalcSubresource(MipSlice, ArraySlice, TexDesc.MipLevels);
    }

    void SetUseBox(D3D11_TILE_REGION_SIZE& RegionSize, BOOL UseBox) const
    {
        RegionSize.bUseBox = UseBox;
    }

    void AddBufferBindRange(const SparseBufferMemoryBindRange& BindRange)
    {
        SetMemory(BindRange.pMemory);
        TBase::AddBufferBindRange(BindRange);
    }

    void AddTextureBindRange(const SparseTextureMemoryBindRange& BindRange,
                             const SparseTextureProperties&      TexSparseProps,
                             const TextureDesc&                  TexDesc,
                             bool                                _UseNVApi)
    {
        SetMemory(BindRange.pMemory);
        TBase::AddTextureBindRange(BindRange, TexSparseProps, TexDesc, _UseNVApi);
    }


    //      WARNING
    //
    // There appears to be a bug on NVidia GPUs: when pd3d11TilePool is null, all tile mappings are invalidated,
    // including those that are not specified in the call to UpdateTileMappings().
    // This is against the spec that states the following:
    //
    //     If no Tile Pool is specified (NULL), or the same one as a previous call to UpdateTileMappings is provided,
    //     the call just adds the new mappings to existing ones (overwriting on overlap).
    //     If the call is only defining NULL mappings, no Tile Pool needs to be specified, since it doesn't matter.
    //     But if one is specified anyway it takes the same behavior as described above when providing a Tile Pool.
    //
    // https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#5.9.3%20Tiled%20Resource%20APIs
    //
    // As a workaround we keep the pointer to the last used memory pool

    void Commit(ID3D11DeviceContext2* pd3d11DeviceContext2, BufferD3D11Impl* pBuffD3D11)
    {
        VERIFY_EXPR(pBuffD3D11 != nullptr);
        if (pMemory)
            pBuffD3D11->SetSparseResourceMemory(pMemory);
        else
            pMemory = pBuffD3D11->GetSparseResourceMemory();
        Commit(pd3d11DeviceContext2, pBuffD3D11->GetD3D11Buffer());
    }

    void Commit(ID3D11DeviceContext2* pd3d11DeviceContext2, TextureBaseD3D11* pTexD3D11)
    {
        VERIFY_EXPR(pTexD3D11 != nullptr);
        if (pMemory)
            pTexD3D11->SetSparseResourceMemory(pMemory);
        else
            pMemory = pTexD3D11->GetSparseResourceMemory();
        Commit(pd3d11DeviceContext2, pTexD3D11->GetD3D11Texture());
    }

private:
    void Commit(ID3D11DeviceContext2* pd3d11DeviceContext2, ID3D11Resource* pResource)
    {
        auto* const pd3d11TilePool = pMemory != nullptr ? pMemory->GetD3D11TilePool() : nullptr;

#ifdef DILIGENT_ENABLE_D3D_NVAPI
        if (UseNVApi)
        {
            // From NVAPI docs:
            //   "If any of API from this set is used, using all of them is highly recommended."
            NvAPI_D3D11_UpdateTileMappings(pd3d11DeviceContext2,
                                           pResource,
                                           static_cast<UINT>(Coordinates.size()),
                                           Coordinates.data(),
                                           RegionSizes.data(),
                                           pd3d11TilePool,
                                           static_cast<UINT>(RangeFlags.size()),
                                           RangeFlags.data(),
                                           RangeStartOffsets.data(),
                                           RangeTileCounts.data(),
                                           D3D11_TILE_MAPPING_NO_OVERWRITE);
        }
        else
#endif
        {
            pd3d11DeviceContext2->UpdateTileMappings(pResource,
                                                     static_cast<UINT>(Coordinates.size()),
                                                     Coordinates.data(),
                                                     RegionSizes.data(),
                                                     pd3d11TilePool,
                                                     static_cast<UINT>(RangeFlags.size()),
                                                     RangeFlags.data(),
                                                     RangeStartOffsets.data(),
                                                     RangeTileCounts.data(),
                                                     D3D11_TILE_MAPPING_NO_OVERWRITE);
        }

        pMemory = nullptr;
        TBase::Reset();
    }

    void SetMemory(IDeviceMemory* pNewMemory)
    {
        auto pNewMemD3D11 = RefCntAutoPtr<IDeviceMemoryD3D11>{pNewMemory, IID_DeviceMemoryD3D11};
        DEV_CHECK_ERR(pNewMemory == nullptr || pNewMemD3D11, "Failed to query IID_DeviceMemoryD3D11 interface");
        if (pMemory != nullptr && pNewMemD3D11 != nullptr && pMemory != pNewMemD3D11)
        {
            LOG_ERROR_MESSAGE("Binding multiple memory objects to a single resource is not allowed in Direct3D11.");
            // all previous mapping will be unmapped
            Reset();
        }

        if (pNewMemD3D11)
        {
            pMemory = std::move(pNewMemD3D11);
        }
    }
};

} // namespace Diligent
