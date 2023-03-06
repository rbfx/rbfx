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
/// Declaration of Diligent::EngineVkImplTraits struct

#include "RenderDeviceVk.h"
#include "PipelineStateVk.h"
#include "ShaderResourceBindingVk.h"
#include "BufferVk.h"
#include "BufferViewVk.h"
#include "TextureVk.h"
#include "TextureViewVk.h"
#include "ShaderVk.h"
#include "SamplerVk.h"
#include "FenceVk.h"
#include "QueryVk.h"
#include "RenderPassVk.h"
#include "FramebufferVk.h"
#include "CommandList.h"
#include "BottomLevelASVk.h"
#include "TopLevelASVk.h"
#include "ShaderBindingTableVk.h"
#include "PipelineResourceSignature.h"
#include "DeviceMemoryVk.h"
#include "PipelineStateCacheVk.h"
#include "CommandQueueVk.h"
#include "DeviceContextVk.h"

namespace Diligent
{

class RenderDeviceVkImpl;
class DeviceContextVkImpl;
class PipelineStateVkImpl;
class ShaderResourceBindingVkImpl;
class BufferVkImpl;
class BufferViewVkImpl;
class TextureVkImpl;
class TextureViewVkImpl;
class ShaderVkImpl;
class SamplerVkImpl;
class FenceVkImpl;
class QueryVkImpl;
class RenderPassVkImpl;
class FramebufferVkImpl;
class CommandListVkImpl;
class BottomLevelASVkImpl;
class TopLevelASVkImpl;
class ShaderBindingTableVkImpl;
class PipelineResourceSignatureVkImpl;
class DeviceMemoryVkImpl;
class PipelineStateCacheVkImpl;

class FixedBlockMemoryAllocator;

class ShaderResourceCacheVk;
class ShaderVariableManagerVk;

struct PipelineResourceAttribsVk;
struct PipelineResourceImmutableSamplerAttribsVk;
struct PipelineResourceSignatureInternalDataVk;

struct EngineVkImplTraits
{
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_VULKAN;

    using RenderDeviceInterface              = IRenderDeviceVk;
    using DeviceContextInterface             = IDeviceContextVk;
    using PipelineStateInterface             = IPipelineStateVk;
    using ShaderResourceBindingInterface     = IShaderResourceBindingVk;
    using BufferInterface                    = IBufferVk;
    using BufferViewInterface                = IBufferViewVk;
    using TextureInterface                   = ITextureVk;
    using TextureViewInterface               = ITextureViewVk;
    using ShaderInterface                    = IShaderVk;
    using SamplerInterface                   = ISamplerVk;
    using FenceInterface                     = IFenceVk;
    using QueryInterface                     = IQueryVk;
    using RenderPassInterface                = IRenderPassVk;
    using FramebufferInterface               = IFramebufferVk;
    using CommandListInterface               = ICommandList;
    using BottomLevelASInterface             = IBottomLevelASVk;
    using TopLevelASInterface                = ITopLevelASVk;
    using ShaderBindingTableInterface        = IShaderBindingTableVk;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;
    using CommandQueueInterface              = ICommandQueueVk;
    using DeviceMemoryInterface              = IDeviceMemoryVk;
    using PipelineStateCacheInterface        = IPipelineStateCacheVk;

    using RenderDeviceImplType              = RenderDeviceVkImpl;
    using DeviceContextImplType             = DeviceContextVkImpl;
    using PipelineStateImplType             = PipelineStateVkImpl;
    using ShaderResourceBindingImplType     = ShaderResourceBindingVkImpl;
    using BufferImplType                    = BufferVkImpl;
    using BufferViewImplType                = BufferViewVkImpl;
    using TextureImplType                   = TextureVkImpl;
    using TextureViewImplType               = TextureViewVkImpl;
    using ShaderImplType                    = ShaderVkImpl;
    using SamplerImplType                   = SamplerVkImpl;
    using FenceImplType                     = FenceVkImpl;
    using QueryImplType                     = QueryVkImpl;
    using RenderPassImplType                = RenderPassVkImpl;
    using FramebufferImplType               = FramebufferVkImpl;
    using CommandListImplType               = CommandListVkImpl;
    using BottomLevelASImplType             = BottomLevelASVkImpl;
    using TopLevelASImplType                = TopLevelASVkImpl;
    using ShaderBindingTableImplType        = ShaderBindingTableVkImpl;
    using PipelineResourceSignatureImplType = PipelineResourceSignatureVkImpl;
    using DeviceMemoryImplType              = DeviceMemoryVkImpl;
    using PipelineStateCacheImplType        = PipelineStateCacheVkImpl;

    using BuffViewObjAllocatorType = FixedBlockMemoryAllocator;
    using TexViewObjAllocatorType  = FixedBlockMemoryAllocator;

    using ShaderResourceCacheImplType   = ShaderResourceCacheVk;
    using ShaderVariableManagerImplType = ShaderVariableManagerVk;

    using PipelineResourceAttribsType = PipelineResourceAttribsVk;
};

} // namespace Diligent
