/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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
#include "BlendState.h"
#include "RenderPass.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "InputLayout.h"

namespace Diligent
{

WGPUTextureFormat TextureFormatToWGPUFormat(TEXTURE_FORMAT TexFmt);

TEXTURE_FORMAT WGPUFormatToTextureFormat(WGPUTextureFormat TexFmt);

WGPUTextureFormat BufferFormatToWGPUTextureFormat(const struct BufferFormat& BuffFmt);

WGPUVertexFormat VertexFormatAttribsToWGPUVertexFormat(VALUE_TYPE ValueType, Uint32 NumComponents, bool IsNormalized);

WGPUIndexFormat IndexTypeToWGPUIndexFormat(VALUE_TYPE ValueType);

WGPUTextureViewDimension ResourceDimensionToWGPUTextureViewDimension(RESOURCE_DIMENSION ResourceDim);

WGPUAddressMode TexAddressModeToWGPUAddressMode(TEXTURE_ADDRESS_MODE Mode);

WGPUFilterMode FilterTypeToWGPUFilterMode(FILTER_TYPE FilterType);

WGPUMipmapFilterMode FilterTypeToWGPUMipMapMode(FILTER_TYPE FilterType);

WGPUCompareFunction ComparisonFuncToWGPUCompareFunction(COMPARISON_FUNCTION CmpFunc);

WGPUStencilOperation StencilOpToWGPUStencilOperation(STENCIL_OP StencilOp);

WGPUQueryType QueryTypeToWGPUQueryType(QUERY_TYPE QueryType);

WGPUColorWriteMaskFlags ColorMaskToWGPUColorWriteMask(COLOR_MASK ColorMask);

WGPULoadOp AttachmentLoadOpToWGPULoadOp(ATTACHMENT_LOAD_OP Operation);

WGPUStoreOp AttachmentStoreOpToWGPUStoreOp(ATTACHMENT_STORE_OP Operation);

WGPUBlendFactor BlendFactorToWGPUBlendFactor(BLEND_FACTOR BlendFactor);

WGPUBlendOperation BlendOpToWGPUBlendOperation(BLEND_OPERATION BlendOp);

WGPUPrimitiveTopology PrimitiveTopologyWGPUPrimitiveType(PRIMITIVE_TOPOLOGY PrimitiveType);

WGPUCullMode CullModeToWGPUCullMode(CULL_MODE CullMode);

WGPUShaderStageFlags ShaderStagesToWGPUShaderStageFlags(SHADER_TYPE Stages);

WGPUVertexStepMode InputElementFrequencyToWGPUVertexStepMode(INPUT_ELEMENT_FREQUENCY StepRate);

} // namespace Diligent
