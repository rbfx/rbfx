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
/// Declaration of Diligent::EngineWebGPUImplTraits struct

#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandList.h"
#include "PipelineResourceSignature.h"

#include "BufferWebGPU.h"
#include "BufferViewWebGPU.h"
#include "RenderDeviceWebGPU.h"
#include "DeviceContextWebGPU.h"
#include "TextureWebGPU.h"
#include "TextureViewWebGPU.h"
#include "SamplerWebGPU.h"
#include "PipelineStateWebGPU.h"
#include "ShaderWebGPU.h"
#include "ShaderResourceBindingWebGPU.h"
#include "FenceWebGPU.h"
#include "QueryWebGPU.h"

namespace Diligent
{

class RenderDeviceWebGPUImpl;
class DeviceContextWebGPUImpl;
class PipelineStateWebGPUImpl;
class ShaderResourceBindingWebGPUImpl;
class BufferWebGPUImpl;
class BufferViewWebGPUImpl;
class TextureWebGPUImpl;
class TextureViewWebGPUImpl;
class ShaderWebGPUImpl;
class SamplerWebGPUImpl;
class FenceWebGPUImpl;
class QueryWebGPUImpl;
class RenderPassWebGPUImpl;
class FramebufferWebGPUImpl;
class CommandListWebGPUImpl;
class BottomLevelASWebGPUImpl;
class TopLevelASWebGPUImpl;
class ShaderBindingTableWebGPUImpl;
class PipelineResourceSignatureWebGPUImpl;
class DeviceMemoryWebGPUImpl;
class PipelineStateCacheWebGPUImpl
{};

class FixedBlockMemoryAllocator;

class SyncPointWebGPUImpl;
class ShaderResourceCacheWebGPU;
class ShaderVariableManagerWebGPU;

struct PipelineResourceAttribsWebGPU;
struct ImmutableSamplerAttribsWebGPU;
struct PipelineResourceSignatureInternalDataWebGPU;

struct EngineWebGPUImplTraits
{
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_WEBGPU;

    using RenderDeviceInterface              = IRenderDeviceWebGPU;
    using DeviceContextInterface             = IDeviceContextWebGPU;
    using PipelineStateInterface             = IPipelineStateWebGPU;
    using ShaderResourceBindingInterface     = IShaderResourceBindingWebGPU;
    using BufferInterface                    = IBufferWebGPU;
    using BufferViewInterface                = IBufferViewWebGPU;
    using TextureInterface                   = ITextureWebGPU;
    using TextureViewInterface               = ITextureViewWebGPU;
    using ShaderInterface                    = IShaderWebGPU;
    using SamplerInterface                   = ISamplerWebGPU;
    using FenceInterface                     = IFenceWebGPU;
    using QueryInterface                     = IQueryWebGPU;
    using RenderPassInterface                = IRenderPass;
    using FramebufferInterface               = IFramebuffer;
    using CommandListInterface               = ICommandList;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;
    using DeviceMemoryInterface              = IDeviceMemory;

    using RenderDeviceImplType              = RenderDeviceWebGPUImpl;
    using DeviceContextImplType             = DeviceContextWebGPUImpl;
    using PipelineStateImplType             = PipelineStateWebGPUImpl;
    using ShaderResourceBindingImplType     = ShaderResourceBindingWebGPUImpl;
    using BufferImplType                    = BufferWebGPUImpl;
    using BufferViewImplType                = BufferViewWebGPUImpl;
    using TextureImplType                   = TextureWebGPUImpl;
    using TextureViewImplType               = TextureViewWebGPUImpl;
    using ShaderImplType                    = ShaderWebGPUImpl;
    using SamplerImplType                   = SamplerWebGPUImpl;
    using FenceImplType                     = FenceWebGPUImpl;
    using QueryImplType                     = QueryWebGPUImpl;
    using RenderPassImplType                = RenderPassWebGPUImpl;
    using FramebufferImplType               = FramebufferWebGPUImpl;
    using CommandListImplType               = CommandListWebGPUImpl;
    using BottomLevelASImplType             = BottomLevelASWebGPUImpl;
    using TopLevelASImplType                = TopLevelASWebGPUImpl;
    using ShaderBindingTableImplType        = ShaderBindingTableWebGPUImpl;
    using PipelineResourceSignatureImplType = PipelineResourceSignatureWebGPUImpl;
    using DeviceMemoryImplType              = DeviceMemoryWebGPUImpl;
    using PipelineStateCacheImplType        = PipelineStateCacheWebGPUImpl;

    using BuffViewObjAllocatorType = FixedBlockMemoryAllocator;
    using TexViewObjAllocatorType  = FixedBlockMemoryAllocator;

    using ShaderResourceCacheImplType   = ShaderResourceCacheWebGPU;
    using ShaderVariableManagerImplType = ShaderVariableManagerWebGPU;

    using PipelineResourceAttribsType               = PipelineResourceAttribsWebGPU;
    using ImmutableSamplerAttribsType               = ImmutableSamplerAttribsWebGPU;
    using PipelineResourceSignatureInternalDataType = PipelineResourceSignatureInternalDataWebGPU;
};

} // namespace Diligent
