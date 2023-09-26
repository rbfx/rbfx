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
/// Implementation of D3D tile mapping helper template

/// This file must be included after D3D11TypeDefinitions.h or D3D12TypeDefinitions.h

#include "GraphicsAccessories.hpp"

namespace Diligent
{

template <typename D3D_TILED_RESOURCE_COORDINATE_TYPE,
          typename D3D_TILE_REGION_SIZE_TYPE,
          typename D3D_TILE_RANGE_FLAGS_TYPE,
          typename ThisImplType>
struct D3DTileMappingHelper
{
    std::vector<D3D_TILED_RESOURCE_COORDINATE_TYPE> Coordinates;
    std::vector<D3D_TILE_REGION_SIZE_TYPE>          RegionSizes;
    std::vector<D3D_TILE_RANGE_FLAGS_TYPE>          RangeFlags;
    std::vector<UINT>                               RangeStartOffsets;
    std::vector<UINT>                               RangeTileCounts;

    bool UseNVApi = false;

    void AddBufferBindRange(const SparseBufferMemoryBindRange& BindRange,
                            Uint64                             MemOffsetInBytes)
    {
        D3D_TILED_RESOURCE_COORDINATE_TYPE d3dCoord{};
        d3dCoord.X           = StaticCast<UINT>(BindRange.BufferOffset / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
        d3dCoord.Y           = 0;
        d3dCoord.Z           = 0;
        d3dCoord.Subresource = 0;

        D3D_TILE_REGION_SIZE_TYPE d3dRegionSize{};
        d3dRegionSize.NumTiles = StaticCast<UINT>(BindRange.MemorySize / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
        d3dRegionSize.Width    = 0;
        d3dRegionSize.Height   = 0;
        d3dRegionSize.Depth    = 0;

        VERIFY(d3dRegionSize.NumTiles > 0, "NumTiles must not be zero");

        AddBindRange(d3dCoord, d3dRegionSize, BindRange.pMemory, MemOffsetInBytes);
    }

    void AddBufferBindRange(const SparseBufferMemoryBindRange& BindRange)
    {
        AddBufferBindRange(BindRange, BindRange.MemoryOffset);
    }

    void AddTextureBindRange(const SparseTextureMemoryBindRange& BindRange,
                             const SparseTextureProperties&      TexSparseProps,
                             const TextureDesc&                  TexDesc,
                             bool                                _UseNVApi,
                             Uint64                              MemOffsetInBytes)
    {
        VERIFY(Coordinates.empty() || UseNVApi == _UseNVApi, "Inconsistent use of NV API among different bind ranges");
        UseNVApi = _UseNVApi;

        const auto& This = *static_cast<const ThisImplType*>(this);

        D3D_TILED_RESOURCE_COORDINATE_TYPE d3dCoord{};
        d3dCoord.Subresource = This.CalcSubresource(BindRange.MipLevel, BindRange.ArraySlice, 0, TexDesc);

        D3D_TILE_REGION_SIZE_TYPE d3dRegionSize{};
        if (BindRange.MipLevel < TexSparseProps.FirstMipInTail)
        {
            This.SetUseBox(d3dRegionSize, TRUE);

            d3dCoord.X = BindRange.Region.MinX / TexSparseProps.TileSize[0];
            d3dCoord.Y = BindRange.Region.MinY / TexSparseProps.TileSize[1];
            d3dCoord.Z = BindRange.Region.MinZ / TexSparseProps.TileSize[2];

            const auto NumTiles    = GetNumSparseTilesInBox(BindRange.Region, TexSparseProps.TileSize);
            d3dRegionSize.NumTiles = NumTiles.x * NumTiles.y * NumTiles.z;
            d3dRegionSize.Width    = NumTiles.x;
            d3dRegionSize.Height   = StaticCast<UINT16>(NumTiles.y);
            d3dRegionSize.Depth    = StaticCast<UINT16>(NumTiles.z);

            VERIFY((BindRange.MemorySize == 0 ||
                    d3dRegionSize.NumTiles == StaticCast<UINT>(BindRange.MemorySize / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES)),
                   "MemorySize must be zero or equal to NumTiles * BlockSize");
        }
        else
        {
            // The X coordinate is used to indicate a tile within the packed mip region, rather than a logical region of a single subresource.
            // The Y and Z coordinates must be zero.
            d3dCoord.X = StaticCast<UINT>(BindRange.OffsetInMipTail / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
            d3dCoord.Y = 0;
            d3dCoord.Z = 0;

            d3dRegionSize.NumTiles = StaticCast<UINT>(BindRange.MemorySize / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
            d3dRegionSize.Width    = 0;
            d3dRegionSize.Height   = 0;
            d3dRegionSize.Depth    = 0;
        }

        VERIFY(d3dRegionSize.NumTiles > 0, "NumTiles must not be zero");

        AddBindRange(d3dCoord, d3dRegionSize, BindRange.pMemory, MemOffsetInBytes);
    }

    void AddTextureBindRange(const SparseTextureMemoryBindRange& BindRange,
                             const SparseTextureProperties&      TexSparseProps,
                             const TextureDesc&                  TexDesc,
                             bool                                _UseNVApi)
    {
        AddTextureBindRange(BindRange, TexSparseProps, TexDesc, _UseNVApi, BindRange.MemoryOffset);
    }

    void Reset()
    {
        *this = {};
    }

private:
    void AddBindRange(const D3D_TILED_RESOURCE_COORDINATE_TYPE& d3dCoords,
                      const D3D_TILE_REGION_SIZE_TYPE&          d3dRegionSize,
                      const IDeviceMemory*                      pMemory,
                      Uint64                                    MemOffsetInBytes)
    {
        Coordinates.emplace_back(d3dCoords);
        RegionSizes.emplace_back(d3dRegionSize);

        // If pRangeFlags[i] is D3D12_TILE_RANGE_FLAG_NONE, that range defines sequential tiles in the heap,
        // with the number of tiles being pRangeTileCounts[i] and the starting location pHeapRangeStartOffsets[i]
        const auto d3dRangeFlags  = pMemory != nullptr ? D3D_TILE_RANGE_FLAG_NONE : D3D_TILE_RANGE_FLAG_NULL;
        const auto StartTile      = StaticCast<UINT>(MemOffsetInBytes / D3D_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
        const auto RangeTileCount = d3dRegionSize.NumTiles;

        VERIFY(RangeTileCount > 0, "Tile count must not be zero");

        RangeFlags.emplace_back(d3dRangeFlags);
        RangeStartOffsets.emplace_back(StartTile);
        RangeTileCounts.emplace_back(RangeTileCount);
    }
};

} // namespace Diligent
