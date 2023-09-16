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
/// D3D12 tile mapping helper

#include "D3D12TypeDefinitions.h"
#include "D3DTileMappingHelper.hpp"

namespace Diligent
{

struct D3D12TileMappingHelper : D3DTileMappingHelper<D3D12_TILED_RESOURCE_COORDINATE, D3D12_TILE_REGION_SIZE, D3D12_TILE_RANGE_FLAGS, D3D12TileMappingHelper>
{
    UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, const TextureDesc& TexDesc) const
    {
        return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, TexDesc.MipLevels, TexDesc.GetArraySize());
    }

    void SetUseBox(D3D12_TILE_REGION_SIZE& RegionSize, BOOL UseBox) const
    {
        RegionSize.UseBox = UseBox;
    }

    ResourceTileMappingsD3D12 GetMappings(ID3D12Resource* pResource, ID3D12Heap* pHeap) const
    {
        ResourceTileMappingsD3D12 Mapping{};
        Mapping.pResource                       = pResource;
        Mapping.NumResourceRegions              = static_cast<UINT>(Coordinates.size());
        Mapping.pResourceRegionStartCoordinates = Coordinates.data();
        Mapping.pResourceRegionSizes            = RegionSizes.data();
        Mapping.pHeap                           = pHeap;
        Mapping.NumRanges                       = static_cast<UINT>(RangeFlags.size());
        Mapping.pRangeFlags                     = RangeFlags.data();
        Mapping.pHeapRangeStartOffsets          = RangeStartOffsets.data();
        Mapping.pRangeTileCounts                = RangeTileCounts.data();
        Mapping.Flags                           = D3D12_TILE_MAPPING_FLAG_NONE;
        Mapping.UseNVApi                        = UseNVApi;

        return Mapping;
    }
};

} // namespace Diligent
