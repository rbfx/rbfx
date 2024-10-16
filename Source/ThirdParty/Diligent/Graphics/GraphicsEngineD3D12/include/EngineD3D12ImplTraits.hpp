/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::EngineD3D12ImplTraits struct

#include "RenderDeviceD3D12.h"
#include "PipelineStateD3D12.h"
#include "ShaderResourceBindingD3D12.h"
#include "BufferD3D12.h"
#include "BufferViewD3D12.h"
#include "TextureD3D12.h"
#include "TextureViewD3D12.h"
#include "ShaderD3D12.h"
#include "SamplerD3D12.h"
#include "FenceD3D12.h"
#include "QueryD3D12.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandList.h"
#include "BottomLevelASD3D12.h"
#include "TopLevelASD3D12.h"
#include "ShaderBindingTableD3D12.h"
#include "PipelineResourceSignature.h"
#include "DeviceMemoryD3D12.h"
#include "PipelineStateCacheD3D12.h"
#include "CommandQueueD3D12.h"
#include "DeviceContextD3D12.h"

namespace Diligent
{

class RenderDeviceD3D12Impl;
class DeviceContextD3D12Impl;
class PipelineStateD3D12Impl;
class ShaderResourceBindingD3D12Impl;
class BufferD3D12Impl;
class BufferViewD3D12Impl;
class TextureD3D12Impl;
class TextureViewD3D12Impl;
class ShaderD3D12Impl;
class SamplerD3D12Impl;
class FenceD3D12Impl;
class QueryD3D12Impl;
class RenderPassD3D12Impl;
class FramebufferD3D12Impl;
class CommandListD3D12Impl;
class BottomLevelASD3D12Impl;
class TopLevelASD3D12Impl;
class ShaderBindingTableD3D12Impl;
class PipelineResourceSignatureD3D12Impl;
class DeviceMemoryD3D12Impl;
class PipelineStateCacheD3D12Impl;

class FixedBlockMemoryAllocator;

class ShaderResourceCacheD3D12;
class ShaderVariableManagerD3D12;

struct PipelineResourceAttribsD3D12;
struct ImmutableSamplerAttribsD3D12;
struct PipelineResourceSignatureInternalDataD3D12;

struct EngineD3D12ImplTraits
{
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_D3D12;

    using RenderDeviceInterface              = IRenderDeviceD3D12;
    using DeviceContextInterface             = IDeviceContextD3D12;
    using PipelineStateInterface             = IPipelineStateD3D12;
    using ShaderResourceBindingInterface     = IShaderResourceBindingD3D12;
    using BufferInterface                    = IBufferD3D12;
    using BufferViewInterface                = IBufferViewD3D12;
    using TextureInterface                   = ITextureD3D12;
    using TextureViewInterface               = ITextureViewD3D12;
    using ShaderInterface                    = IShaderD3D12;
    using SamplerInterface                   = ISamplerD3D12;
    using FenceInterface                     = IFenceD3D12;
    using QueryInterface                     = IQueryD3D12;
    using RenderPassInterface                = IRenderPass;
    using FramebufferInterface               = IFramebuffer;
    using CommandListInterface               = ICommandList;
    using BottomLevelASInterface             = IBottomLevelASD3D12;
    using TopLevelASInterface                = ITopLevelASD3D12;
    using ShaderBindingTableInterface        = IShaderBindingTableD3D12;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;
    using CommandQueueInterface              = ICommandQueueD3D12;
    using DeviceMemoryInterface              = IDeviceMemoryD3D12;
    using PipelineStateCacheInterface        = IPipelineStateCacheD3D12;

    using RenderDeviceImplType              = RenderDeviceD3D12Impl;
    using DeviceContextImplType             = DeviceContextD3D12Impl;
    using PipelineStateImplType             = PipelineStateD3D12Impl;
    using ShaderResourceBindingImplType     = ShaderResourceBindingD3D12Impl;
    using BufferImplType                    = BufferD3D12Impl;
    using BufferViewImplType                = BufferViewD3D12Impl;
    using TextureImplType                   = TextureD3D12Impl;
    using TextureViewImplType               = TextureViewD3D12Impl;
    using ShaderImplType                    = ShaderD3D12Impl;
    using SamplerImplType                   = SamplerD3D12Impl;
    using FenceImplType                     = FenceD3D12Impl;
    using QueryImplType                     = QueryD3D12Impl;
    using RenderPassImplType                = RenderPassD3D12Impl;
    using FramebufferImplType               = FramebufferD3D12Impl;
    using CommandListImplType               = CommandListD3D12Impl;
    using BottomLevelASImplType             = BottomLevelASD3D12Impl;
    using TopLevelASImplType                = TopLevelASD3D12Impl;
    using ShaderBindingTableImplType        = ShaderBindingTableD3D12Impl;
    using PipelineResourceSignatureImplType = PipelineResourceSignatureD3D12Impl;
    using DeviceMemoryImplType              = DeviceMemoryD3D12Impl;
    using PipelineStateCacheImplType        = PipelineStateCacheD3D12Impl;

    using BuffViewObjAllocatorType = FixedBlockMemoryAllocator;
    using TexViewObjAllocatorType  = FixedBlockMemoryAllocator;

    using ShaderResourceCacheImplType   = ShaderResourceCacheD3D12;
    using ShaderVariableManagerImplType = ShaderVariableManagerD3D12;

    using PipelineResourceAttribsType               = PipelineResourceAttribsD3D12;
    using ImmutableSamplerAttribsType               = ImmutableSamplerAttribsD3D12;
    using PipelineResourceSignatureInternalDataType = PipelineResourceSignatureInternalDataD3D12;
};

} // namespace Diligent
