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
/// Declaration of Diligent::WebGPUObjectWrapper class

namespace Diligent
{

template <typename WebGPUObjectType, typename WebGPUObjectDeleter>
class WebGPUObjectWrapper
{
public:
    WebGPUObjectWrapper()
    {}

    ~WebGPUObjectWrapper()
    {
        if (m_Handle)
            m_Deleter(m_Handle);
    }

    explicit WebGPUObjectWrapper(WebGPUObjectType ObjectHandle, WebGPUObjectDeleter ObjectDeleter = WebGPUObjectDeleter{}) :
        m_Handle{ObjectHandle},
        m_Deleter{ObjectDeleter}
    {}

    WebGPUObjectWrapper(const WebGPUObjectWrapper&) = delete;

    WebGPUObjectWrapper& operator=(const WebGPUObjectWrapper&) = delete;

    WebGPUObjectWrapper(WebGPUObjectWrapper&& RHS) noexcept :
        m_Handle{RHS.Detach()},
        m_Deleter{std::move(RHS.m_Deleter)}
    {
    }

    WebGPUObjectWrapper& operator=(WebGPUObjectWrapper&& RHS) noexcept
    {
        if (this == &RHS)
            return *this;

        Reset(RHS.Detach());
        m_Deleter = std::move(RHS.m_Deleter);
        return *this;
    }

    const WebGPUObjectType& Get() const
    {
        return m_Handle;
    }

    WebGPUObjectType& Get()
    {
        return m_Handle;
    }

    operator WebGPUObjectType() const
    {
        return m_Handle;
    }

    void Reset(WebGPUObjectType Handle = nullptr)
    {
        if (m_Handle != Handle)
        {
            if (m_Handle)
                m_Deleter(m_Handle);
            m_Handle = Handle;
        }
    }

    WebGPUObjectType Detach()
    {
        WebGPUObjectType Handle = m_Handle;
        m_Handle                = nullptr;
        return Handle;
    }

    explicit operator bool() const
    {
        return m_Handle != nullptr;
    }

private:
    WebGPUObjectType    m_Handle{};
    WebGPUObjectDeleter m_Deleter{};
};


#define DECLARE_WEBGPU_WRAPPER(HandleName, TypeName, FunctionName) \
    struct HandleName##Deleter                                     \
    {                                                              \
        void operator()(TypeName Handle) const                     \
        {                                                          \
            if (!IsShared)                                         \
                FunctionName##Release(Handle);                     \
        }                                                          \
                                                                   \
        bool IsShared = false;                                     \
    };                                                             \
    using HandleName##Wrapper = WebGPUObjectWrapper<TypeName, HandleName##Deleter>;

DECLARE_WEBGPU_WRAPPER(WebGPUInstance, WGPUInstance, wgpuInstance)
DECLARE_WEBGPU_WRAPPER(WebGPUAdapter, WGPUAdapter, wgpuAdapter)
DECLARE_WEBGPU_WRAPPER(WebGPUDevice, WGPUDevice, wgpuDevice)
DECLARE_WEBGPU_WRAPPER(WebGPUQueue, WGPUQueue, wgpuQueue)
DECLARE_WEBGPU_WRAPPER(WebGPUSurface, WGPUSurface, wgpuSurface)
DECLARE_WEBGPU_WRAPPER(WebGPUTexture, WGPUTexture, wgpuTexture)
DECLARE_WEBGPU_WRAPPER(WebGPUTextureView, WGPUTextureView, wgpuTextureView)
DECLARE_WEBGPU_WRAPPER(WebGPUBuffer, WGPUBuffer, wgpuBuffer)
DECLARE_WEBGPU_WRAPPER(WebGPUSampler, WGPUSampler, wgpuSampler)
DECLARE_WEBGPU_WRAPPER(WebGPUShaderModule, WGPUShaderModule, wgpuShaderModule)
DECLARE_WEBGPU_WRAPPER(WebGPUBindGroupLayout, WGPUBindGroupLayout, wgpuBindGroupLayout)
DECLARE_WEBGPU_WRAPPER(WebGPUPipelineLayout, WGPUPipelineLayout, wgpuPipelineLayout)
DECLARE_WEBGPU_WRAPPER(WebGPURenderPipeline, WGPURenderPipeline, wgpuRenderPipeline)
DECLARE_WEBGPU_WRAPPER(WebGPUComputePipeline, WGPUComputePipeline, wgpuComputePipeline)
DECLARE_WEBGPU_WRAPPER(WebGPUCommandBuffer, WGPUCommandBuffer, wgpuCommandBuffer)
DECLARE_WEBGPU_WRAPPER(WebGPUCommandEncoder, WGPUCommandEncoder, wgpuCommandEncoder)
DECLARE_WEBGPU_WRAPPER(WebGPURenderPassEncoder, WGPURenderPassEncoder, wgpuRenderPassEncoder)
DECLARE_WEBGPU_WRAPPER(WebGPUComputePassEncoder, WGPUComputePassEncoder, wgpuComputePassEncoder)
DECLARE_WEBGPU_WRAPPER(WebGPUBindGroup, WGPUBindGroup, wgpuBindGroup)
DECLARE_WEBGPU_WRAPPER(WebGPUQuerySet, WGPUQuerySet, wgpuQuerySet)

} // namespace Diligent
