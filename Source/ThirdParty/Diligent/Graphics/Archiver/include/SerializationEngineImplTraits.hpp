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
/// Declaration of Diligent::SerializationEngineImplTraits struct

#include "CommonDefinitions.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SerializationDevice.h"

namespace Diligent
{

class SerializationDeviceImpl;
class SerializedShaderImpl;
class SerializedRenderPassImpl;
class SerializedResourceSignatureImpl;
class SerializedPipelineStateImpl;
class SerializedObjectStub
{};

struct SerializationEngineImplTraits
{
    using RenderDeviceInterface              = ISerializationDevice;
    using DeviceContextInterface             = IDeviceContext;
    using PipelineStateInterface             = IPipelineState;
    using ShaderResourceBindingInterface     = IShaderResourceBinding;
    using BufferInterface                    = IBuffer;
    using BufferViewInterface                = IBufferView;
    using TextureInterface                   = ITexture;
    using TextureViewInterface               = ITextureView;
    using ShaderInterface                    = IShader;
    using SamplerInterface                   = ISampler;
    using FenceInterface                     = IFence;
    using QueryInterface                     = IQuery;
    using RenderPassInterface                = IRenderPass;
    using FramebufferInterface               = IFramebuffer;
    using CommandListInterface               = ICommandList;
    using BottomLevelASInterface             = IBottomLevelAS;
    using TopLevelASInterface                = ITopLevelAS;
    using ShaderBindingTableInterface        = IShaderBindingTable;
    using PipelineResourceSignatureInterface = IPipelineResourceSignature;
    using CommandQueueInterface              = ICommandQueue;
    using DeviceMemoryInterface              = IDeviceMemory;
    using PipelineStateCacheInterface        = IPipelineStateCache;

    using RenderDeviceImplType              = SerializationDeviceImpl;
    using DeviceContextImplType             = IDeviceContext;
    using PipelineStateImplType             = SerializedPipelineStateImpl;
    using ShaderResourceBindingImplType     = SerializedObjectStub;
    using BufferImplType                    = SerializedObjectStub;
    using BufferViewImplType                = SerializedObjectStub;
    using TextureImplType                   = SerializedObjectStub;
    using TextureViewImplType               = SerializedObjectStub;
    using ShaderImplType                    = SerializedShaderImpl;
    using SamplerImplType                   = SerializedObjectStub;
    using FenceImplType                     = SerializedObjectStub;
    using QueryImplType                     = SerializedObjectStub;
    using RenderPassImplType                = SerializedRenderPassImpl;
    using FramebufferImplType               = SerializedObjectStub;
    using CommandListImplType               = SerializedObjectStub;
    using BottomLevelASImplType             = SerializedObjectStub;
    using TopLevelASImplType                = SerializedObjectStub;
    using ShaderBindingTableImplType        = SerializedObjectStub;
    using PipelineResourceSignatureImplType = SerializedResourceSignatureImpl;
    using DeviceMemoryImplType              = SerializedObjectStub;
    using PipelineStateCacheImplType        = SerializedObjectStub;
};

template <typename ReturnType>
struct _DummyReturn
{
    const ReturnType& operator()()
    {
        static const typename std::remove_reference<ReturnType>::type NullRet = {};
        return NullRet;
    }
};

template <>
struct _DummyReturn<void>
{
    void operator()()
    {
    }
};

#define UNSUPPORTED_METHOD(RetType, MethodName, ...)                          \
    virtual RetType DILIGENT_CALL_TYPE MethodName(__VA_ARGS__) override final \
    {                                                                         \
        UNSUPPORTED(#MethodName " is not supported in serialization engine"); \
        return _DummyReturn<RetType>{}();                                     \
    }

#define UNSUPPORTED_CONST_METHOD(RetType, MethodName, ...)                          \
    virtual RetType DILIGENT_CALL_TYPE MethodName(__VA_ARGS__) const override final \
    {                                                                               \
        UNSUPPORTED(#MethodName " is not supported in serialization engine");       \
        return _DummyReturn<RetType>{}();                                           \
    }
} // namespace Diligent
