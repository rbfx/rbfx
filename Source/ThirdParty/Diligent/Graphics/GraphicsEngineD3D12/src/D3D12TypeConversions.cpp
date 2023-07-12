/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "D3D12TypeConversions.hpp"

#include <array>

#include "PrivateConstants.h"
#include "DXGITypeConversions.hpp"
#include "D3D12TypeDefinitions.h"
#include "D3DTypeConversionImpl.hpp"
#include "D3DViewDescConversionImpl.hpp"
#include "PlatformMisc.hpp"
#include "Align.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

D3D12_COMPARISON_FUNC ComparisonFuncToD3D12ComparisonFunc(COMPARISON_FUNCTION Func)
{
    return ComparisonFuncToD3DComparisonFunc<D3D12_COMPARISON_FUNC>(Func);
}

D3D12_FILTER FilterTypeToD3D12Filter(FILTER_TYPE MinFilter, FILTER_TYPE MagFilter, FILTER_TYPE MipFilter)
{
    return FilterTypeToD3DFilter<D3D12_FILTER>(MinFilter, MagFilter, MipFilter);
}

D3D12_TEXTURE_ADDRESS_MODE TexAddressModeToD3D12AddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    return TexAddressModeToD3DAddressMode<D3D12_TEXTURE_ADDRESS_MODE>(Mode);
}

void DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(const DepthStencilStateDesc& DepthStencilDesc,
                                                       D3D12_DEPTH_STENCIL_DESC&    d3d12DSSDesc)
{
    DepthStencilStateDesc_To_D3D_DEPTH_STENCIL_DESC<D3D12_DEPTH_STENCIL_DESC, D3D12_DEPTH_STENCILOP_DESC, D3D12_STENCIL_OP, D3D12_COMPARISON_FUNC>(DepthStencilDesc, d3d12DSSDesc);
}

void RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(const RasterizerStateDesc& RasterizerDesc,
                                                  D3D12_RASTERIZER_DESC&     d3d12RSDesc)
{
    RasterizerStateDesc_To_D3D_RASTERIZER_DESC<D3D12_RASTERIZER_DESC, D3D12_FILL_MODE, D3D12_CULL_MODE>(RasterizerDesc, d3d12RSDesc);

    // The sample count that is forced while UAV rendering or rasterizing.
    // Valid values are 0, 1, 2, 4, 8, and optionally 16. 0 indicates that
    // the sample count is not forced.
    d3d12RSDesc.ForcedSampleCount = 0;

    d3d12RSDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}



D3D12_LOGIC_OP LogicOperationToD3D12LogicOp(LOGIC_OPERATION lo)
{
    // Note that this code is safe for multithreaded environments since
    // bIsInit is set to true only AFTER the entire map is initialized.
    static bool           bIsInit                               = false;
    static D3D12_LOGIC_OP D3D12LogicOp[LOGIC_OP_NUM_OPERATIONS] = {};
    if (!bIsInit)
    {
        // clang-format off
        // In a multithreaded environment, several threads can potentially enter
        // this block. This is not a problem since they will just initialize the
        // memory with the same values more than once
        D3D12LogicOp[ D3D12_LOGIC_OP_CLEAR		    ]  = D3D12_LOGIC_OP_CLEAR;
        D3D12LogicOp[ D3D12_LOGIC_OP_SET			]  = D3D12_LOGIC_OP_SET;
        D3D12LogicOp[ D3D12_LOGIC_OP_COPY			]  = D3D12_LOGIC_OP_COPY;
        D3D12LogicOp[ D3D12_LOGIC_OP_COPY_INVERTED  ]  = D3D12_LOGIC_OP_COPY_INVERTED;
        D3D12LogicOp[ D3D12_LOGIC_OP_NOOP			]  = D3D12_LOGIC_OP_NOOP;
        D3D12LogicOp[ D3D12_LOGIC_OP_INVERT		    ]  = D3D12_LOGIC_OP_INVERT;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND			]  = D3D12_LOGIC_OP_AND;
        D3D12LogicOp[ D3D12_LOGIC_OP_NAND			]  = D3D12_LOGIC_OP_NAND;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR			    ]  = D3D12_LOGIC_OP_OR;
        D3D12LogicOp[ D3D12_LOGIC_OP_NOR			]  = D3D12_LOGIC_OP_NOR;
        D3D12LogicOp[ D3D12_LOGIC_OP_XOR			]  = D3D12_LOGIC_OP_XOR;
        D3D12LogicOp[ D3D12_LOGIC_OP_EQUIV		    ]  = D3D12_LOGIC_OP_EQUIV;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND_REVERSE	]  = D3D12_LOGIC_OP_AND_REVERSE;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND_INVERTED	]  = D3D12_LOGIC_OP_AND_INVERTED;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR_REVERSE	    ]  = D3D12_LOGIC_OP_OR_REVERSE;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR_INVERTED	]  = D3D12_LOGIC_OP_OR_INVERTED;
        // clang-format on

        bIsInit = true;
    }
    if (lo >= LOGIC_OP_CLEAR && lo < LOGIC_OP_NUM_OPERATIONS)
    {
        auto d3dlo = D3D12LogicOp[lo];
        return d3dlo;
    }
    else
    {
        UNEXPECTED("Incorrect blend factor (", lo, ")");
        return static_cast<D3D12_LOGIC_OP>(0);
    }
}


void BlendStateDesc_To_D3D12_BLEND_DESC(const BlendStateDesc& BSDesc, D3D12_BLEND_DESC& d3d12BlendDesc)
{
    BlendStateDescToD3DBlendDesc<D3D12_BLEND_DESC, D3D12_BLEND, D3D12_BLEND_OP>(BSDesc, d3d12BlendDesc);

    for (int i = 0; i < 8; ++i)
    {
        const auto& SrcRTDesc = BSDesc.RenderTargets[i];
        auto&       DstRTDesc = d3d12BlendDesc.RenderTarget[i];

        // The following members only present in D3D_RENDER_TARGET_BLEND_DESC
        DstRTDesc.LogicOpEnable = SrcRTDesc.LogicOperationEnable ? TRUE : FALSE;
        DstRTDesc.LogicOp       = LogicOperationToD3D12LogicOp(SrcRTDesc.LogicOp);
    }
}

void LayoutElements_To_D3D12_INPUT_ELEMENT_DESCs(const InputLayoutDesc&                                                               InputLayout,
                                                 std::vector<D3D12_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D12_INPUT_ELEMENT_DESC>>& d3d12InputElements)
{
    LayoutElements_To_D3D_INPUT_ELEMENT_DESCs<D3D12_INPUT_ELEMENT_DESC>(InputLayout, d3d12InputElements);
}

D3D12_PRIMITIVE_TOPOLOGY TopologyToD3D12Topology(PRIMITIVE_TOPOLOGY Topology)
{
    return TopologyToD3DTopology<D3D12_PRIMITIVE_TOPOLOGY>(Topology);
}


UINT TextureComponentSwizzleToD3D12ShaderComponentMapping(TEXTURE_COMPONENT_SWIZZLE Swizzle, UINT IdentityComponent)
{
    static_assert(TEXTURE_COMPONENT_SWIZZLE_COUNT == 7, "Did you add a new swizzle mode? Please handle it here.");
    switch (Swizzle)
    {
        // clang-format off
        case TEXTURE_COMPONENT_SWIZZLE_IDENTITY: return IdentityComponent;
        case TEXTURE_COMPONENT_SWIZZLE_ZERO:     return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
        case TEXTURE_COMPONENT_SWIZZLE_ONE:      return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
        case TEXTURE_COMPONENT_SWIZZLE_R:        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
        case TEXTURE_COMPONENT_SWIZZLE_G:        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
        case TEXTURE_COMPONENT_SWIZZLE_B:        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
        case TEXTURE_COMPONENT_SWIZZLE_A:        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
        // clang-format on
        default:
            UNEXPECTED("Unknown swizzle");
            return IdentityComponent;
    }
}

UINT TextureComponentMappingToD3D12Shader4ComponentMapping(const TextureComponentMapping& Mapping)
{
    return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
        TextureComponentSwizzleToD3D12ShaderComponentMapping(Mapping.R, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0),
        TextureComponentSwizzleToD3D12ShaderComponentMapping(Mapping.G, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1),
        TextureComponentSwizzleToD3D12ShaderComponentMapping(Mapping.B, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2),
        TextureComponentSwizzleToD3D12ShaderComponentMapping(Mapping.A, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3));
}

void TextureViewDesc_to_D3D12_SRV_DESC(const TextureViewDesc&           SRVDesc,
                                       D3D12_SHADER_RESOURCE_VIEW_DESC& D3D12SRVDesc,
                                       Uint32                           SampleCount)
{
    TextureViewDesc_to_D3D_SRV_DESC(SRVDesc, D3D12SRVDesc, SampleCount);
    D3D12SRVDesc.Shader4ComponentMapping = TextureComponentMappingToD3D12Shader4ComponentMapping(SRVDesc.Swizzle);
    switch (SRVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            D3D12SRVDesc.Texture1D.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            D3D12SRVDesc.Texture1DArray.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12SRVDesc.Texture2D.PlaneSlice          = 0;
                D3D12SRVDesc.Texture2D.ResourceMinLODClamp = 0;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12SRVDesc.Texture2DArray.PlaneSlice          = 0;
                D3D12SRVDesc.Texture2DArray.ResourceMinLODClamp = 0;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            D3D12SRVDesc.Texture3D.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_CUBE:
            D3D12SRVDesc.TextureCube.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            D3D12SRVDesc.TextureCubeArray.ResourceMinLODClamp = 0;
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

void TextureViewDesc_to_D3D12_RTV_DESC(const TextureViewDesc&         RTVDesc,
                                       D3D12_RENDER_TARGET_VIEW_DESC& D3D12RTVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_RTV_DESC(RTVDesc, D3D12RTVDesc, SampleCount);
    switch (RTVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            break;

        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12RTVDesc.Texture2D.PlaneSlice = 0;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12RTVDesc.Texture2DArray.PlaneSlice = 0;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

void TextureViewDesc_to_D3D12_DSV_DESC(const TextureViewDesc&         DSVDesc,
                                       D3D12_DEPTH_STENCIL_VIEW_DESC& D3D12DSVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_DSV_DESC(DSVDesc, D3D12DSVDesc, SampleCount);
}

void TextureViewDesc_to_D3D12_UAV_DESC(const TextureViewDesc&            UAVDesc,
                                       D3D12_UNORDERED_ACCESS_VIEW_DESC& D3D12UAVDesc)
{
    TextureViewDesc_to_D3D_UAV_DESC(UAVDesc, D3D12UAVDesc);
    switch (UAVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            break;

        case RESOURCE_DIM_TEX_2D:
            D3D12UAVDesc.Texture2D.PlaneSlice = 0;
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            D3D12UAVDesc.Texture2DArray.PlaneSlice = 0;
            break;

        case RESOURCE_DIM_TEX_3D:
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}


void BufferViewDesc_to_D3D12_SRV_DESC(const BufferDesc&                BuffDesc,
                                      const BufferViewDesc&            SRVDesc,
                                      D3D12_SHADER_RESOURCE_VIEW_DESC& D3D12SRVDesc)
{
    BufferViewDesc_to_D3D_SRV_DESC(BuffDesc, SRVDesc, D3D12SRVDesc);
    D3D12SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (BuffDesc.Mode == BUFFER_MODE_RAW && SRVDesc.Format.ValueType == VT_UNDEFINED)
    {
        D3D12SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        D3D12SRVDesc.Format       = DXGI_FORMAT_R32_TYPELESS;
    }
    else
        D3D12SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    VERIFY_EXPR(BuffDesc.BindFlags & BIND_SHADER_RESOURCE);
    if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED)
        D3D12SRVDesc.Buffer.StructureByteStride = BuffDesc.ElementByteStride;
}

void BufferViewDesc_to_D3D12_UAV_DESC(const BufferDesc&                 BuffDesc,
                                      const BufferViewDesc&             UAVDesc,
                                      D3D12_UNORDERED_ACCESS_VIEW_DESC& D3D12UAVDesc)
{
    BufferViewDesc_to_D3D_UAV_DESC(BuffDesc, UAVDesc, D3D12UAVDesc);
    VERIFY_EXPR(BuffDesc.BindFlags & BIND_UNORDERED_ACCESS);
    if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED)
        D3D12UAVDesc.Buffer.StructureByteStride = BuffDesc.ElementByteStride;
}

D3D12_STATIC_BORDER_COLOR BorderColorToD3D12StaticBorderColor(const Float32 BorderColor[])
{
    D3D12_STATIC_BORDER_COLOR StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    else if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    else if (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    else
    {
        LOG_ERROR_MESSAGE("D3D12 static samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors.");
    }
    return StaticBorderColor;
}


static D3D12_RESOURCE_STATES ResourceStateFlagToD3D12ResourceState(RESOURCE_STATE StateFlag)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    VERIFY(IsPowerOfTwo(StateFlag), "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return D3D12_RESOURCE_STATE_COMMON;
        case RESOURCE_STATE_VERTEX_BUFFER:     return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case RESOURCE_STATE_CONSTANT_BUFFER:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case RESOURCE_STATE_INDEX_BUFFER:      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case RESOURCE_STATE_RENDER_TARGET:     return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case RESOURCE_STATE_DEPTH_WRITE:       return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case RESOURCE_STATE_DEPTH_READ:        return D3D12_RESOURCE_STATE_DEPTH_READ;
        case RESOURCE_STATE_SHADER_RESOURCE:   return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_STREAM_OUT:        return D3D12_RESOURCE_STATE_STREAM_OUT;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case RESOURCE_STATE_COPY_DEST:         return D3D12_RESOURCE_STATE_COPY_DEST;
        case RESOURCE_STATE_COPY_SOURCE:       return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RESOURCE_STATE_RESOLVE_DEST:      return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_PRESENT:           return D3D12_RESOURCE_STATE_PRESENT;
        case RESOURCE_STATE_BUILD_AS_READ:     return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_BUILD_AS_WRITE:    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case RESOURCE_STATE_RAY_TRACING:       return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_COMMON:            return D3D12_RESOURCE_STATE_COMMON;
        case RESOURCE_STATE_SHADING_RATE:      return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        // clang-format on
        default:
            UNEXPECTED("Unexpected resource state flag");
            return static_cast<D3D12_RESOURCE_STATES>(0);
    }
}

class StateFlagBitPosToD3D12ResourceState
{
public:
    StateFlagBitPosToD3D12ResourceState()
    {
        static_assert((1 << MaxFlagBitPos) == RESOURCE_STATE_MAX_BIT, "This function must be updated to handle new resource state flag");
        for (Uint32 bit = 0; bit < FlagBitPosToResStateMap.size(); ++bit)
        {
            FlagBitPosToResStateMap[bit] = ResourceStateFlagToD3D12ResourceState(static_cast<RESOURCE_STATE>(1 << bit));
        }
    }

    D3D12_RESOURCE_STATES operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", MaxFlagBitPos, ")");
        return FlagBitPosToResStateMap[BitPos];
    }

private:
    static constexpr Uint32                              MaxFlagBitPos = 21;
    std::array<D3D12_RESOURCE_STATES, MaxFlagBitPos + 1> FlagBitPosToResStateMap;
};

D3D12_RESOURCE_STATES ResourceStateFlagsToD3D12ResourceStates(RESOURCE_STATE StateFlags)
{
    VERIFY(StateFlags < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");
    static const StateFlagBitPosToD3D12ResourceState BitPosToD3D12ResState;
    D3D12_RESOURCE_STATES                            D3D12ResourceStates = static_cast<D3D12_RESOURCE_STATES>(0);
    Uint32                                           Bits                = StateFlags;
    while (Bits != 0)
    {
        auto lsb = PlatformMisc::GetLSB(Bits);
        D3D12ResourceStates |= BitPosToD3D12ResState(lsb);
        Bits &= ~(1 << lsb);
    }
    return D3D12ResourceStates;
}

D3D12_RESOURCE_STATES GetSupportedD3D12ResourceStatesForCommandList(D3D12_COMMAND_LIST_TYPE CmdListType)
{
    constexpr D3D12_RESOURCE_STATES TransferResStates =
        D3D12_RESOURCE_STATE_COMMON |
        D3D12_RESOURCE_STATE_COPY_DEST |
        D3D12_RESOURCE_STATE_COPY_SOURCE;

    constexpr D3D12_RESOURCE_STATES ComputeResStates =
        TransferResStates |
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    constexpr D3D12_RESOURCE_STATES GraphicsResStates =
        ComputeResStates |
        D3D12_RESOURCE_STATE_INDEX_BUFFER |
        D3D12_RESOURCE_STATE_RENDER_TARGET |
        D3D12_RESOURCE_STATE_DEPTH_WRITE |
        D3D12_RESOURCE_STATE_DEPTH_READ |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_STREAM_OUT |
        D3D12_RESOURCE_STATE_RESOLVE_DEST |
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
        D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

    switch (CmdListType)
    {
        // clang-format off
        case D3D12_COMMAND_LIST_TYPE_DIRECT:  return GraphicsResStates;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return ComputeResStates;
        case D3D12_COMMAND_LIST_TYPE_COPY:    return TransferResStates;
        // clang-format on
        default:
            UNEXPECTED("Unexpected command list type");
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

static RESOURCE_STATE D3D12ResourceStateToResourceStateFlags(D3D12_RESOURCE_STATES state)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    VERIFY(IsPowerOfTwo(state), "Only single state must be set");
    switch (state)
    {
        // clang-format off
        //case D3D12_RESOURCE_STATE_COMMON:
        case D3D12_RESOURCE_STATE_PRESENT:                    return RESOURCE_STATE_PRESENT;
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: return static_cast<RESOURCE_STATE>(RESOURCE_STATE_VERTEX_BUFFER | RESOURCE_STATE_CONSTANT_BUFFER);
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:               return RESOURCE_STATE_INDEX_BUFFER;
        case D3D12_RESOURCE_STATE_RENDER_TARGET:              return RESOURCE_STATE_RENDER_TARGET;
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:           return RESOURCE_STATE_UNORDERED_ACCESS;
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:                return RESOURCE_STATE_DEPTH_WRITE;
        case D3D12_RESOURCE_STATE_DEPTH_READ:                 return RESOURCE_STATE_DEPTH_READ;
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:  return RESOURCE_STATE_SHADER_RESOURCE;
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:      return RESOURCE_STATE_SHADER_RESOURCE;
        case D3D12_RESOURCE_STATE_STREAM_OUT:                 return RESOURCE_STATE_STREAM_OUT;
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:          return RESOURCE_STATE_INDIRECT_ARGUMENT;
        case D3D12_RESOURCE_STATE_COPY_DEST:                  return RESOURCE_STATE_COPY_DEST;
        case D3D12_RESOURCE_STATE_COPY_SOURCE:                return RESOURCE_STATE_COPY_SOURCE;
        case D3D12_RESOURCE_STATE_RESOLVE_DEST:               return RESOURCE_STATE_RESOLVE_DEST;
        case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:             return RESOURCE_STATE_RESOLVE_SOURCE;
#ifdef NTDDI_WIN10_19H1
        // First defined in Win SDK 10.0.18362.0
        case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:        return RESOURCE_STATE_SHADING_RATE;
#endif
        // clang-format on
        default:
            UNEXPECTED("Unexpected D3D12 resource state");
            return RESOURCE_STATE_UNKNOWN;
    }
}


class D3D12StateFlagBitPosToResourceState
{
public:
    D3D12StateFlagBitPosToResourceState()
    {
        for (Uint32 bit = 0; bit < FlagBitPosToResStateMap.size(); ++bit)
        {
            FlagBitPosToResStateMap[bit] = D3D12ResourceStateToResourceStateFlags(static_cast<D3D12_RESOURCE_STATES>(1 << bit));
        }
    }

    RESOURCE_STATE operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", MaxFlagBitPos, ")");
        return FlagBitPosToResStateMap[BitPos];
    }

private:
    static constexpr Uint32                       MaxFlagBitPos = 13;
    std::array<RESOURCE_STATE, MaxFlagBitPos + 1> FlagBitPosToResStateMap;
};

RESOURCE_STATE D3D12ResourceStatesToResourceStateFlags(D3D12_RESOURCE_STATES StateFlags)
{
    if (StateFlags == D3D12_RESOURCE_STATE_PRESENT)
        return RESOURCE_STATE_PRESENT;

    static const D3D12StateFlagBitPosToResourceState BitPosToResState;

    Uint32 ResourceStates = 0;
    Uint32 Bits           = StateFlags;
    while (Bits != 0)
    {
        auto lsb = PlatformMisc::GetLSB(Bits);
        ResourceStates |= BitPosToResState(lsb);
        Bits &= ~(1 << lsb);
    }
    return static_cast<RESOURCE_STATE>(ResourceStates);
}

D3D12_QUERY_TYPE QueryTypeToD3D12QueryType(QUERY_TYPE QueryType)
{
    // clang-format off
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:           return D3D12_QUERY_TYPE_OCCLUSION;
        case QUERY_TYPE_BINARY_OCCLUSION:    return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
        case QUERY_TYPE_TIMESTAMP:           return D3D12_QUERY_TYPE_TIMESTAMP;
        case QUERY_TYPE_PIPELINE_STATISTICS: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
        case QUERY_TYPE_DURATION:            return D3D12_QUERY_TYPE_TIMESTAMP;

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
        default:
            UNEXPECTED("Unexpected query type");
            return static_cast<D3D12_QUERY_TYPE>(-1);
    }
    // clang-format on
}

D3D12_QUERY_HEAP_TYPE QueryTypeToD3D12QueryHeapType(QUERY_TYPE QueryType, HardwareQueueIndex QueueId)
{
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
            VERIFY(QueueId == D3D12HWQueueIndex_Graphics, "Occlusion queries are only supported in graphics queue");
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;

        case QUERY_TYPE_BINARY_OCCLUSION:
            VERIFY(QueueId == D3D12HWQueueIndex_Graphics, "Occlusion queries are only supported in graphics queue");
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;

        case QUERY_TYPE_PIPELINE_STATISTICS:
            VERIFY(QueueId == D3D12HWQueueIndex_Graphics, "Pipeline statistics queries are only supported in graphics queue");
            return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;

        case QUERY_TYPE_DURATION:
        case QUERY_TYPE_TIMESTAMP:
            return QueueId == D3D12HWQueueIndex_Copy ? D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP : D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

            static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
        default:
            UNEXPECTED("Unexpected query type");
            return static_cast<D3D12_QUERY_HEAP_TYPE>(-1);
    }
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE AttachmentLoadOpToD3D12BeginningAccessType(ATTACHMENT_LOAD_OP LoadOp)
{
    // clang-format off
    switch (LoadOp)
    {
        case ATTACHMENT_LOAD_OP_LOAD:    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        case ATTACHMENT_LOAD_OP_CLEAR:   return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        case ATTACHMENT_LOAD_OP_DISCARD: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;

        default:
            UNEXPECTED("Unexpected attachment load op");
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    }
    // clang-format on
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE AttachmentStoreOpToD3D12EndingAccessType(ATTACHMENT_STORE_OP StoreOp)
{
    // clang-format off
    switch (StoreOp)
    {
        case ATTACHMENT_STORE_OP_STORE:    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        case ATTACHMENT_STORE_OP_DISCARD:  return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;

        default:
            UNEXPECTED("Unexpected attachment store op");
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
    // clang-format on
}

D3D12_SHADER_VISIBILITY ShaderTypeToD3D12ShaderVisibility(SHADER_TYPE ShaderType)
{
    VERIFY(IsPowerOfTwo(ShaderType), "Only single shader stage should be provided");

    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:            return D3D12_SHADER_VISIBILITY_VERTEX;
        case SHADER_TYPE_PIXEL:             return D3D12_SHADER_VISIBILITY_PIXEL;
        case SHADER_TYPE_GEOMETRY:          return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case SHADER_TYPE_HULL:              return D3D12_SHADER_VISIBILITY_HULL;
        case SHADER_TYPE_DOMAIN:            return D3D12_SHADER_VISIBILITY_DOMAIN;
        case SHADER_TYPE_COMPUTE:           return D3D12_SHADER_VISIBILITY_ALL;
#   ifdef D3D12_H_HAS_MESH_SHADER
        case SHADER_TYPE_AMPLIFICATION:     return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
        case SHADER_TYPE_MESH:              return D3D12_SHADER_VISIBILITY_MESH;
#   endif
        case SHADER_TYPE_RAY_GEN:
        case SHADER_TYPE_RAY_MISS:
        case SHADER_TYPE_RAY_CLOSEST_HIT:
        case SHADER_TYPE_RAY_ANY_HIT:
        case SHADER_TYPE_RAY_INTERSECTION:
        case SHADER_TYPE_CALLABLE:          return D3D12_SHADER_VISIBILITY_ALL;
        // clang-format on
        case SHADER_TYPE_TILE:
            UNSUPPORTED("Unsupported shader type (", GetShaderTypeLiteralName(ShaderType), ")");
            return D3D12_SHADER_VISIBILITY_ALL;
        default:
            UNSUPPORTED("Unknown shader type (", ShaderType, ")");
            return D3D12_SHADER_VISIBILITY_ALL;
    }
}

SHADER_TYPE D3D12ShaderVisibilityToShaderType(D3D12_SHADER_VISIBILITY ShaderVisibility)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    switch (ShaderVisibility)
    {
        // clang-format off
        case D3D12_SHADER_VISIBILITY_ALL:           return SHADER_TYPE_UNKNOWN;
        case D3D12_SHADER_VISIBILITY_VERTEX:        return SHADER_TYPE_VERTEX;
        case D3D12_SHADER_VISIBILITY_PIXEL:         return SHADER_TYPE_PIXEL;
        case D3D12_SHADER_VISIBILITY_GEOMETRY:      return SHADER_TYPE_GEOMETRY;
        case D3D12_SHADER_VISIBILITY_HULL:          return SHADER_TYPE_HULL;
        case D3D12_SHADER_VISIBILITY_DOMAIN:        return SHADER_TYPE_DOMAIN;
#   ifdef D3D12_H_HAS_MESH_SHADER
        case D3D12_SHADER_VISIBILITY_AMPLIFICATION: return SHADER_TYPE_AMPLIFICATION;
        case D3D12_SHADER_VISIBILITY_MESH:          return SHADER_TYPE_MESH;
#   endif
        // clang-format on
        default:
            LOG_ERROR("Unknown shader visibility (", ShaderVisibility, ")");
            return SHADER_TYPE_UNKNOWN;
    }
}

DXGI_FORMAT ValueTypeToIndexType(VALUE_TYPE IndexType)
{
    switch (IndexType)
    {
        // clang-format off
        case VT_UNDEFINED: return DXGI_FORMAT_UNKNOWN; // only for ray tracing
        case VT_UINT16:    return DXGI_FORMAT_R16_UINT;
        case VT_UINT32:    return DXGI_FORMAT_R32_UINT;
        // clang-format on
        default:
            UNEXPECTED("Unexpected index type");
            return DXGI_FORMAT_R32_UINT;
    }
}

D3D12_RAYTRACING_GEOMETRY_FLAGS GeometryFlagsToD3D12RTGeometryFlags(RAYTRACING_GEOMETRY_FLAGS Flags)
{
    static_assert(RAYTRACING_GEOMETRY_FLAG_LAST == RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION,
                  "Please update the switch below to handle the new ray tracing geometry flag");

    D3D12_RAYTRACING_GEOMETRY_FLAGS Result = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    while (Flags != RAYTRACING_GEOMETRY_FLAG_NONE)
    {
        auto FlagBit = static_cast<RAYTRACING_GEOMETRY_FLAGS>(1 << PlatformMisc::GetLSB(Uint32{Flags}));
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_GEOMETRY_FLAG_OPAQUE:                          Result |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;                         break;
            case RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION: Result |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION; break;
            // clang-format on
            default: UNEXPECTED("unknown geometry flag");
        }
        Flags &= ~FlagBit;
    }
    return Result;
}

D3D12_RAYTRACING_INSTANCE_FLAGS InstanceFlagsToD3D12RTInstanceFlags(RAYTRACING_INSTANCE_FLAGS Flags)
{
    static_assert(RAYTRACING_INSTANCE_FLAG_LAST == RAYTRACING_INSTANCE_FORCE_NO_OPAQUE,
                  "Please update the switch below to handle the new ray tracing instance flag");

    D3D12_RAYTRACING_INSTANCE_FLAGS Result = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    while (Flags != RAYTRACING_INSTANCE_NONE)
    {
        auto FlagBit = static_cast<RAYTRACING_INSTANCE_FLAGS>(1 << PlatformMisc::GetLSB(Uint32{Flags}));
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_INSTANCE_TRIANGLE_FACING_CULL_DISABLE:    Result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;           break;
            case RAYTRACING_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE: Result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; break;
            case RAYTRACING_INSTANCE_FORCE_OPAQUE:                    Result |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;                    break;
            case RAYTRACING_INSTANCE_FORCE_NO_OPAQUE:                 Result |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;                break;
            // clang-format on
            default: UNEXPECTED("unknown instance flag");
        }
        Flags &= ~FlagBit;
    }
    return Result;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS BuildASFlagsToD3D12ASBuildFlags(RAYTRACING_BUILD_AS_FLAGS Flags)
{
    static_assert(RAYTRACING_BUILD_AS_FLAG_LAST == RAYTRACING_BUILD_AS_LOW_MEMORY,
                  "Please update the switch below to handle the new acceleration structure build flag");

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Result = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    while (Flags != RAYTRACING_BUILD_AS_NONE)
    {
        auto FlagBit = static_cast<RAYTRACING_BUILD_AS_FLAGS>(1 << PlatformMisc::GetLSB(Uint32{Flags}));
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_BUILD_AS_ALLOW_UPDATE:      Result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;      break;
            case RAYTRACING_BUILD_AS_ALLOW_COMPACTION:  Result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;  break;
            case RAYTRACING_BUILD_AS_PREFER_FAST_TRACE: Result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; break;
            case RAYTRACING_BUILD_AS_PREFER_FAST_BUILD: Result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD; break;
            case RAYTRACING_BUILD_AS_LOW_MEMORY:        Result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;   break;
            // clang-format on
            default: UNEXPECTED("unknown build AS flag");
        }
        Flags &= ~FlagBit;
    }
    return Result;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE CopyASModeToD3D12ASCopyMode(COPY_AS_MODE Mode)
{
    static_assert(COPY_AS_MODE_LAST == COPY_AS_MODE_COMPACT,
                  "Please update the switch below to handle the new copy AS mode");

    switch (Mode)
    {
        // clang-format off
        case COPY_AS_MODE_CLONE:   return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
        case COPY_AS_MODE_COMPACT: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT;
        // clang-format on
        default:
            UNEXPECTED("unknown AS copy mode");
            return static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE>(~0u);
    }
}

DXGI_FORMAT TypeToRayTracingVertexFormat(VALUE_TYPE ValueType, Uint32 ComponentCount)
{
    // Vertex format must be one of the following (https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_geometry_triangles_desc):
    //  * DXGI_FORMAT_R32G32_FLOAT       - third component is assumed 0
    //  * DXGI_FORMAT_R32G32B32_FLOAT
    //  * DXGI_FORMAT_R16G16_FLOAT       - third component is assumed 0
    //  * DXGI_FORMAT_R16G16B16A16_FLOAT - A16 component is ignored, other data can be packed there, such as setting vertex stride to 6 bytes.
    //  * DXGI_FORMAT_R16G16_SNORM       - third component is assumed 0
    //  * DXGI_FORMAT_R16G16B16A16_SNORM - A16 component is ignored, other data can be packed there, such as setting vertex stride to 6 bytes.
    // Note that DXGI_FORMAT_R16G16B16A16_FLOAT and DXGI_FORMAT_R16G16B16A16_SNORM are merely workarounds for missing 16-bit 3-component DXGI formats
    switch (ValueType)
    {
        case VT_FLOAT16:
            switch (ComponentCount)
            {
                case 2: return DXGI_FORMAT_R16G16_FLOAT;
                case 3: return DXGI_FORMAT_R16G16B16A16_FLOAT;

                default:
                    UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                    return DXGI_FORMAT_UNKNOWN;
            }
            break;

        case VT_FLOAT32:
            switch (ComponentCount)
            {
                case 2: return DXGI_FORMAT_R32G32_FLOAT;
                case 3: return DXGI_FORMAT_R32G32B32_FLOAT;

                default:
                    UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                    return DXGI_FORMAT_UNKNOWN;
            }
            break;

        case VT_INT16:
            switch (ComponentCount)
            {
                case 2: return DXGI_FORMAT_R16G16_SNORM;
                case 3: return DXGI_FORMAT_R16G16B16A16_SNORM;

                default:
                    UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                    return DXGI_FORMAT_UNKNOWN;
            }
            break;

        default:
            UNEXPECTED(GetValueTypeString(ValueType), " is not a valid vertex component type");
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE ResourceTypeToD3D12DescriptorRangeType(SHADER_RESOURCE_TYPE ResType)
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new resource type");

    switch (ResType)
    {
        // clang-format off
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:     return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case SHADER_RESOURCE_TYPE_BUFFER_SRV:      return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:     return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        case SHADER_RESOURCE_TYPE_BUFFER_UAV:      return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        case SHADER_RESOURCE_TYPE_SAMPLER:         return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        // clang-format on
        default:
            UNEXPECTED("Unknown resource type");
            return static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(-1);
    }
}

D3D12_DESCRIPTOR_HEAP_TYPE D3D12DescriptorRangeTypeToD3D12HeapType(D3D12_DESCRIPTOR_RANGE_TYPE RangeType)
{
    VERIFY_EXPR(RangeType >= D3D12_DESCRIPTOR_RANGE_TYPE_SRV && RangeType <= D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
    switch (RangeType)
    {
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        default:
            UNEXPECTED("Unexpected descriptor range type");
            return static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(-1);
    }
}

D3D12_SHADER_VISIBILITY ShaderStagesToD3D12ShaderVisibility(SHADER_TYPE Stages)
{
    return IsPowerOfTwo(Stages) ?
        ShaderTypeToD3D12ShaderVisibility(Stages) :
        D3D12_SHADER_VISIBILITY_ALL;
}

HardwareQueueIndex D3D12CommandListTypeToQueueId(D3D12_COMMAND_LIST_TYPE Type)
{
    switch (Type)
    {
        // clang-format off
        case D3D12_COMMAND_LIST_TYPE_DIRECT:  return D3D12HWQueueIndex_Graphics;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return D3D12HWQueueIndex_Compute;
        case D3D12_COMMAND_LIST_TYPE_COPY:    return D3D12HWQueueIndex_Copy;
        // clang-format on
        default:
            UNEXPECTED("Unexpected command list type");
            return HardwareQueueIndex{MAX_COMMAND_QUEUES};
    }
}

D3D12_COMMAND_LIST_TYPE QueueIdToD3D12CommandListType(HardwareQueueIndex QueueId)
{
    switch (QueueId)
    {
        // clang-format off
        case D3D12HWQueueIndex_Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case D3D12HWQueueIndex_Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case D3D12HWQueueIndex_Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
        // clang-format on
        default:
            UNEXPECTED("Unexpected queue id");
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

COMMAND_QUEUE_TYPE D3D12CommandListTypeToCmdQueueType(D3D12_COMMAND_LIST_TYPE ListType)
{
    static_assert(COMMAND_QUEUE_TYPE_MAX_BIT == 0x7, "Please update the switch below to handle the new context type");
    switch (ListType)
    {
        // clang-format off
        case D3D12_COMMAND_LIST_TYPE_DIRECT:  return COMMAND_QUEUE_TYPE_GRAPHICS;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return COMMAND_QUEUE_TYPE_COMPUTE;
        case D3D12_COMMAND_LIST_TYPE_COPY:    return COMMAND_QUEUE_TYPE_TRANSFER;
        // clang-format on
        default:
            UNEXPECTED("Unexpected command list type");
            return COMMAND_QUEUE_TYPE_UNKNOWN;
    }
}

D3D12_COMMAND_QUEUE_PRIORITY QueuePriorityToD3D12QueuePriority(QUEUE_PRIORITY Priority)
{
    static_assert(QUEUE_PRIORITY_LAST == 4, "Please update the switch below to handle the new queue priority");
    switch (Priority)
    {
        // clang-format off
        case QUEUE_PRIORITY_LOW:      return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        case QUEUE_PRIORITY_MEDIUM:   return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        case QUEUE_PRIORITY_HIGH:     return D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
        case QUEUE_PRIORITY_REALTIME: return D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME;
        // clang-format on
        default:
            UNEXPECTED("Unexpected queue priority");
            return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    }
}

D3D12_SHADING_RATE ShadingRateToD3D12ShadingRate(SHADING_RATE Rate)
{
#ifdef NTDDI_WIN10_19H1
    static constexpr D3D12_SHADING_RATE d3d12Rates[] =
        {
            D3D12_SHADING_RATE_1X1,
            D3D12_SHADING_RATE_1X2,
            D3D12_SHADING_RATE_2X4, // replacement for 1x4
            D3D12_SHADING_RATE_1X1, // unused
            D3D12_SHADING_RATE_2X1,
            D3D12_SHADING_RATE_2X2,
            D3D12_SHADING_RATE_2X4,
            D3D12_SHADING_RATE_1X1, // unused
            D3D12_SHADING_RATE_4X2, // replacement for 4x1
            D3D12_SHADING_RATE_4X2,
            D3D12_SHADING_RATE_4X4 //
        };
    static_assert(d3d12Rates[SHADING_RATE_1X1] == D3D12_SHADING_RATE_1X1, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_1X2] == D3D12_SHADING_RATE_1X2, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_1X4] == D3D12_SHADING_RATE_2X4, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_2X1] == D3D12_SHADING_RATE_2X1, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_2X2] == D3D12_SHADING_RATE_2X2, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_2X4] == D3D12_SHADING_RATE_2X4, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_4X1] == D3D12_SHADING_RATE_4X2, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_4X2] == D3D12_SHADING_RATE_4X2, "Incorrect mapping to D3D12 shading rate");
    static_assert(d3d12Rates[SHADING_RATE_4X4] == D3D12_SHADING_RATE_4X4, "Incorrect mapping to D3D12 shading rate");
    static_assert(_countof(d3d12Rates) == SHADING_RATE_MAX + 1, "Invalid number of elements in d3d12Rates");

    VERIFY_EXPR(Rate <= SHADING_RATE_MAX);
    return d3d12Rates[Rate];
#else
    UNEXPECTED("Requires WinSDK 10.0.18362.0");
    return static_cast<D3D12_SHADING_RATE>(0);
#endif
}

D3D12_SHADING_RATE_COMBINER ShadingRateCombinerToD3D12ShadingRateCombiner(SHADING_RATE_COMBINER Combiner)
{
#ifdef NTDDI_WIN10_19H1
    static_assert(SHADING_RATE_COMBINER_LAST == (1u << 5), "Please update the switch below to handle the new shading rate combiner");
    VERIFY(IsPowerOfTwo(Combiner), "Only a single combiner should be provided");
    switch (Combiner)
    {
        // clang-format off
        case SHADING_RATE_COMBINER_PASSTHROUGH: return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
        case SHADING_RATE_COMBINER_OVERRIDE:    return D3D12_SHADING_RATE_COMBINER_OVERRIDE;
        case SHADING_RATE_COMBINER_MIN:         return D3D12_SHADING_RATE_COMBINER_MIN;
        case SHADING_RATE_COMBINER_MAX:         return D3D12_SHADING_RATE_COMBINER_MAX;
        case SHADING_RATE_COMBINER_SUM:         return D3D12_SHADING_RATE_COMBINER_SUM;
        // clang-format on
        default:
            UNEXPECTED("Unexpected shading rate combiner");
            return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
    }
#else
    UNEXPECTED("Requires WinSDK 10.0.18362.0");
    return static_cast<D3D12_SHADING_RATE_COMBINER>(0);
#endif
}

} // namespace Diligent
