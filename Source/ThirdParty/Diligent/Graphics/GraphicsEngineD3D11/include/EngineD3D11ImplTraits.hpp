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
/// Declaration of Diligent::EngineD3D11ImplTraits struct

#include "RenderDeviceD3D11.h"
#include "PipelineStateD3D11.h"
#include "ShaderResourceBindingD3D11.h"
#include "BufferD3D11.h"
#include "BufferViewD3D11.h"
#include "TextureD3D11.h"
#include "TextureViewD3D11.h"
#include "ShaderD3D11.h"
#include "SamplerD3D11.h"
#include "FenceD3D11.h"
#include "QueryD3D11.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandList.h"
#include "PipelineResourceSignature.h"
#include "DeviceMemoryD3D11.h"
#include "DeviceContextD3D11.h"

namespace Diligent
{

class RenderDeviceD3D11Impl;
class DeviceContextD3D11Impl;
class PipelineStateD3D11Impl;
class ShaderResourceBindingD3D11Impl;
class BufferD3D11Impl;
class BufferViewD3D11Impl;
class TextureBaseD3D11;
class TextureViewD3D11Impl;
class ShaderD3D11Impl;
class SamplerD3D11Impl;
class FenceD3D11Impl;
class QueryD3D11Impl;
class RenderPassD3D11Impl;
class FramebufferD3D11Impl;
class CommandListD3D11Impl;
class BottomLevelASD3D11Impl;
class TopLevelASD3D11Impl;
class ShaderBindingTableD3D11Impl;
class PipelineResourceSignatureD3D11Impl;
class DeviceMemoryD3D11Impl;
class PipelineStateCacheD3D11Impl
{};

class FixedBlockMemoryAllocator;

class ShaderResourceCacheD3D11;
class ShaderVariableManagerD3D11;

struct PipelineResourceAttribsD3D11;
struct PipelineResourceImmutableSamplerAttribsD3D11;
struct PipelineResourceSignatureInternalDataD3D11;

struct EngineD3D11ImplTraits
{
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_D3D11;

    using RenderDeviceInterface              = IRenderDeviceD3D11;
    using DeviceContextInterface             = IDeviceContextD3D11;
    using PipelineStateInterface             = IPipelineStateD3D11;
    using ShaderResourceBindingInterface     = IShaderResourceBindingD3D11;
    using BufferInterface                    = IBufferD3D11;
    using BufferViewInterface                = IBufferViewD3D11;
    using TextureInterface                   = ITextureD3D11;
    using TextureViewInterface               = ITextureViewD3D11;
    using ShaderInterface                    = IShaderD3D11;
    using SamplerInterface                   = ISamplerD3D11;
    using FenceInterface                     = IFenceD3D11;
    using QueryInterface                     = IQueryD3D11;
    using RenderPassInterface                = IRenderPass;
    using FramebufferInterface               = IFramebuffer;
    using CommandListInterface               = ICommandList;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;
    using DeviceMemoryInterface              = IDeviceMemoryD3D11;

    using RenderDeviceImplType              = RenderDeviceD3D11Impl;
    using DeviceContextImplType             = DeviceContextD3D11Impl;
    using PipelineStateImplType             = PipelineStateD3D11Impl;
    using ShaderResourceBindingImplType     = ShaderResourceBindingD3D11Impl;
    using BufferImplType                    = BufferD3D11Impl;
    using BufferViewImplType                = BufferViewD3D11Impl;
    using TextureImplType                   = TextureBaseD3D11;
    using TextureViewImplType               = TextureViewD3D11Impl;
    using ShaderImplType                    = ShaderD3D11Impl;
    using SamplerImplType                   = SamplerD3D11Impl;
    using FenceImplType                     = FenceD3D11Impl;
    using QueryImplType                     = QueryD3D11Impl;
    using RenderPassImplType                = RenderPassD3D11Impl;
    using FramebufferImplType               = FramebufferD3D11Impl;
    using CommandListImplType               = CommandListD3D11Impl;
    using BottomLevelASImplType             = BottomLevelASD3D11Impl;
    using TopLevelASImplType                = TopLevelASD3D11Impl;
    using ShaderBindingTableImplType        = ShaderBindingTableD3D11Impl;
    using PipelineResourceSignatureImplType = PipelineResourceSignatureD3D11Impl;
    using DeviceMemoryImplType              = DeviceMemoryD3D11Impl;
    using PipelineStateCacheImplType        = PipelineStateCacheD3D11Impl;


    using BuffViewObjAllocatorType = FixedBlockMemoryAllocator;
    using TexViewObjAllocatorType  = FixedBlockMemoryAllocator;

    using ShaderResourceCacheImplType   = ShaderResourceCacheD3D11;
    using ShaderVariableManagerImplType = ShaderVariableManagerD3D11;

    using PipelineResourceAttribsType = PipelineResourceAttribsD3D11;
};

} // namespace Diligent
