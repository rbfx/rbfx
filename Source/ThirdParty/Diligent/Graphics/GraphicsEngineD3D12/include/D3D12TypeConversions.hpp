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

#pragma once

/// \file
/// Type conversion routines

#include "GraphicsTypes.h"
#include "RenderPass.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "BlendState.h"
#include "InputLayout.h"
#include "Texture.h"
#include "TextureView.h"
#include "Buffer.h"
#include "BufferView.h"
#include "Shader.h"
#include "DeviceContext.h"
#include "IndexWrapper.hpp"

namespace Diligent
{

D3D12_COMPARISON_FUNC      ComparisonFuncToD3D12ComparisonFunc(COMPARISON_FUNCTION Func);
D3D12_FILTER               FilterTypeToD3D12Filter(FILTER_TYPE MinFilter, FILTER_TYPE MagFilter, FILTER_TYPE MipFilter);
D3D12_TEXTURE_ADDRESS_MODE TexAddressModeToD3D12AddressMode(TEXTURE_ADDRESS_MODE Mode);
D3D12_PRIMITIVE_TOPOLOGY   TopologyToD3D12Topology(PRIMITIVE_TOPOLOGY Topology);

void DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(const DepthStencilStateDesc& DepthStencilDesc,
                                                       D3D12_DEPTH_STENCIL_DESC&    d3d12DSSDesc);
void RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(const RasterizerStateDesc& RasterizerDesc,
                                                  D3D12_RASTERIZER_DESC&     d3d11RSDesc);
void BlendStateDesc_To_D3D12_BLEND_DESC(const BlendStateDesc& BSDesc,
                                        D3D12_BLEND_DESC&     d3d12BlendDesc);

void LayoutElements_To_D3D12_INPUT_ELEMENT_DESCs(const InputLayoutDesc&                                                               InputLayout,
                                                 std::vector<D3D12_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D12_INPUT_ELEMENT_DESC>>& d3d12InputElements);

// clang-format off
void TextureViewDesc_to_D3D12_SRV_DESC(const TextureViewDesc& SRVDesc, D3D12_SHADER_RESOURCE_VIEW_DESC  &D3D12SRVDesc, Uint32 SampleCount);
void TextureViewDesc_to_D3D12_RTV_DESC(const TextureViewDesc& RTVDesc, D3D12_RENDER_TARGET_VIEW_DESC    &D3D12RTVDesc, Uint32 SampleCount);
void TextureViewDesc_to_D3D12_DSV_DESC(const TextureViewDesc& DSVDesc, D3D12_DEPTH_STENCIL_VIEW_DESC    &D3D12DSVDesc, Uint32 SampleCount);
void TextureViewDesc_to_D3D12_UAV_DESC(const TextureViewDesc& UAVDesc, D3D12_UNORDERED_ACCESS_VIEW_DESC &D3D12UAVDesc);
// clang-format on

void BufferViewDesc_to_D3D12_SRV_DESC(const BufferDesc&                BuffDesc,
                                      const BufferViewDesc&            SRVDesc,
                                      D3D12_SHADER_RESOURCE_VIEW_DESC& D3D12SRVDesc);
void BufferViewDesc_to_D3D12_UAV_DESC(const BufferDesc&                 BuffDesc,
                                      const BufferViewDesc&             UAVDesc,
                                      D3D12_UNORDERED_ACCESS_VIEW_DESC& D3D12UAVDesc);

D3D12_STATIC_BORDER_COLOR BorderColorToD3D12StaticBorderColor(const Float32 BorderColor[]);
D3D12_RESOURCE_STATES     ResourceStateFlagsToD3D12ResourceStates(RESOURCE_STATE StateFlags);
RESOURCE_STATE            D3D12ResourceStatesToResourceStateFlags(D3D12_RESOURCE_STATES StateFlags);
D3D12_RESOURCE_STATES     GetSupportedD3D12ResourceStatesForCommandList(D3D12_COMMAND_LIST_TYPE CmdListType);

D3D12_QUERY_HEAP_TYPE QueryTypeToD3D12QueryHeapType(QUERY_TYPE QueryType, HardwareQueueIndex QueueId);
D3D12_QUERY_TYPE      QueryTypeToD3D12QueryType(QUERY_TYPE QueryType);

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE AttachmentLoadOpToD3D12BeginningAccessType(ATTACHMENT_LOAD_OP LoadOp);
D3D12_RENDER_PASS_ENDING_ACCESS_TYPE    AttachmentStoreOpToD3D12EndingAccessType(ATTACHMENT_STORE_OP StoreOp);

D3D12_SHADER_VISIBILITY ShaderTypeToD3D12ShaderVisibility(SHADER_TYPE ShaderType);
SHADER_TYPE             D3D12ShaderVisibilityToShaderType(D3D12_SHADER_VISIBILITY ShaderVisibility);

DXGI_FORMAT ValueTypeToIndexType(VALUE_TYPE Type);

D3D12_RAYTRACING_GEOMETRY_FLAGS GeometryFlagsToD3D12RTGeometryFlags(RAYTRACING_GEOMETRY_FLAGS Flags);
D3D12_RAYTRACING_INSTANCE_FLAGS InstanceFlagsToD3D12RTInstanceFlags(RAYTRACING_INSTANCE_FLAGS Flags);

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS BuildASFlagsToD3D12ASBuildFlags(RAYTRACING_BUILD_AS_FLAGS Flags);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE   CopyASModeToD3D12ASCopyMode(COPY_AS_MODE Mode);

DXGI_FORMAT TypeToRayTracingVertexFormat(VALUE_TYPE ValueType, Uint32 ComponentCount);

D3D12_DESCRIPTOR_RANGE_TYPE ResourceTypeToD3D12DescriptorRangeType(SHADER_RESOURCE_TYPE ResType);

D3D12_DESCRIPTOR_HEAP_TYPE D3D12DescriptorRangeTypeToD3D12HeapType(D3D12_DESCRIPTOR_RANGE_TYPE RangeType);

D3D12_SHADER_VISIBILITY ShaderStagesToD3D12ShaderVisibility(SHADER_TYPE Stages);

D3D12_SHADING_RATE          ShadingRateToD3D12ShadingRate(SHADING_RATE Rate);
D3D12_SHADING_RATE_COMBINER ShadingRateCombinerToD3D12ShadingRateCombiner(SHADING_RATE_COMBINER Combiner);

HardwareQueueIndex           D3D12CommandListTypeToQueueId(D3D12_COMMAND_LIST_TYPE Type);
D3D12_COMMAND_LIST_TYPE      QueueIdToD3D12CommandListType(HardwareQueueIndex QueueId);
COMMAND_QUEUE_TYPE           D3D12CommandListTypeToCmdQueueType(D3D12_COMMAND_LIST_TYPE ListType);
D3D12_COMMAND_QUEUE_PRIORITY QueuePriorityToD3D12QueuePriority(QUEUE_PRIORITY Priority);

static constexpr HardwareQueueIndex D3D12HWQueueIndex_Graphics{Uint8{0}};
static constexpr HardwareQueueIndex D3D12HWQueueIndex_Compute{Uint8{1}};
static constexpr HardwareQueueIndex D3D12HWQueueIndex_Copy{Uint8{2}};

} // namespace Diligent
