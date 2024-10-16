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
/// Declaration of Diligent::EngineGLImplTraits struct

#include "RenderDeviceGL.h"
#include "PipelineStateGL.h"
#include "ShaderResourceBindingGL.h"
#include "BufferGL.h"
#include "BufferViewGL.h"
#include "TextureGL.h"
#include "TextureViewGL.h"
#include "ShaderGL.h"
#include "SamplerGL.h"
#include "FenceGL.h"
#include "QueryGL.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineResourceSignature.h"
#include "DeviceContextGL.h"
#include "BaseInterfacesGL.h"

namespace Diligent
{

class RenderDeviceGLImpl;
class DeviceContextGLImpl;
class PipelineStateGLImpl;
class ShaderResourceBindingGLImpl;
class BufferGLImpl;
class BufferViewGLImpl;
class TextureBaseGL;
class TextureViewGLImpl;
class ShaderGLImpl;
class SamplerGLImpl;
class FenceGLImpl;
class QueryGLImpl;
class RenderPassGLImpl;
class FramebufferGLImpl;
class BottomLevelASGLImpl;
class TopLevelASGLImpl;
class ShaderBindingTableGLImpl;
class PipelineResourceSignatureGLImpl;
class DeviceMemoryGLImpl;
class PipelineStateCacheGlImpl
{};

class FixedBlockMemoryAllocator;

class ShaderResourceCacheGL;
class ShaderVariableManagerGL;

struct PipelineResourceAttribsGL;
struct ImmutableSamplerAttribsGL;
struct PipelineResourceSignatureInternalDataGL;

struct EngineGLImplTraits
{
#if PLATFORM_WIN32 || PLATFORM_LINUX || PLATFORM_MACOS
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_GL;
#elif PLATFORM_ANDROID || PLATFORM_IOS || PLATFORM_EMSCRIPTEN
    static constexpr auto DeviceType = RENDER_DEVICE_TYPE_GL;
#else
#    error Unsupported platform
#endif

    using RenderDeviceInterface              = IGLDeviceBaseInterface;
    using DeviceContextInterface             = IDeviceContextGL;
    using PipelineStateInterface             = IPipelineStateGL;
    using ShaderResourceBindingInterface     = IShaderResourceBindingGL;
    using BufferInterface                    = IBufferGL;
    using BufferViewInterface                = IBufferViewGL;
    using TextureInterface                   = ITextureGL;
    using TextureViewInterface               = ITextureViewGL;
    using ShaderInterface                    = IShaderGL;
    using SamplerInterface                   = ISamplerGL;
    using FenceInterface                     = IFenceGL;
    using QueryInterface                     = IQueryGL;
    using RenderPassInterface                = IRenderPass;
    using FramebufferInterface               = IFramebuffer;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;

    using RenderDeviceImplType              = RenderDeviceGLImpl;
    using DeviceContextImplType             = DeviceContextGLImpl;
    using PipelineStateImplType             = PipelineStateGLImpl;
    using ShaderResourceBindingImplType     = ShaderResourceBindingGLImpl;
    using BufferImplType                    = BufferGLImpl;
    using BufferViewImplType                = BufferViewGLImpl;
    using TextureImplType                   = TextureBaseGL;
    using TextureViewImplType               = TextureViewGLImpl;
    using ShaderImplType                    = ShaderGLImpl;
    using SamplerImplType                   = SamplerGLImpl;
    using FenceImplType                     = FenceGLImpl;
    using QueryImplType                     = QueryGLImpl;
    using RenderPassImplType                = RenderPassGLImpl;
    using FramebufferImplType               = FramebufferGLImpl;
    using BottomLevelASImplType             = BottomLevelASGLImpl;
    using TopLevelASImplType                = TopLevelASGLImpl;
    using ShaderBindingTableImplType        = ShaderBindingTableGLImpl;
    using PipelineResourceSignatureImplType = PipelineResourceSignatureGLImpl;
    using DeviceMemoryImplType              = DeviceMemoryGLImpl;
    using PipelineStateCacheImplType        = PipelineStateCacheGlImpl;

    using BuffViewObjAllocatorType = FixedBlockMemoryAllocator;
    using TexViewObjAllocatorType  = FixedBlockMemoryAllocator;

    using ShaderResourceCacheImplType   = ShaderResourceCacheGL;
    using ShaderVariableManagerImplType = ShaderVariableManagerGL;

    using PipelineResourceAttribsType               = PipelineResourceAttribsGL;
    using ImmutableSamplerAttribsType               = ImmutableSamplerAttribsGL;
    using PipelineResourceSignatureInternalDataType = PipelineResourceSignatureInternalDataGL;
};

} // namespace Diligent
