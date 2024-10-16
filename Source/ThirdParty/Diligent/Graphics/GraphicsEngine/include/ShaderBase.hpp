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
/// Implementation of the Diligent::ShaderBase template class

#include <vector>
#include <memory>
#include <atomic>

#include "Shader.h"
#include "DeviceObjectBase.hpp"
#include "STDAllocator.hpp"
#include "PlatformMisc.hpp"
#include "EngineMemory.h"
#include "Align.hpp"
#include "RefCntAutoPtr.hpp"
#include "AsyncInitializer.hpp"

namespace Diligent
{

class ShaderCreateInfoWrapper
{
public:
    ShaderCreateInfoWrapper() = default;

    ShaderCreateInfoWrapper(const ShaderCreateInfoWrapper&) = delete;
    ShaderCreateInfoWrapper& operator=(const ShaderCreateInfoWrapper&) = delete;

    ShaderCreateInfoWrapper(ShaderCreateInfoWrapper&& rhs) noexcept :
        m_CreateInfo{rhs.m_CreateInfo},
        m_SourceFactory{std::move(rhs.m_SourceFactory)},
        m_pRawMemory{std::move(rhs.m_pRawMemory)}
    {
        rhs.m_CreateInfo = {};
    }

    ShaderCreateInfoWrapper& operator=(ShaderCreateInfoWrapper&& rhs) noexcept
    {
        m_CreateInfo    = rhs.m_CreateInfo;
        m_SourceFactory = std::move(rhs.m_SourceFactory);
        m_pRawMemory    = std::move(rhs.m_pRawMemory);

        rhs.m_CreateInfo = {};

        return *this;
    }

    ShaderCreateInfoWrapper(const ShaderCreateInfo& CI, IMemoryAllocator& RawAllocator) noexcept(false);

    const ShaderCreateInfo& Get() const
    {
        return m_CreateInfo;
    }

    operator const ShaderCreateInfo&() const
    {
        return m_CreateInfo;
    }

private:
    ShaderCreateInfo                               m_CreateInfo;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> m_SourceFactory;
    std::unique_ptr<void, STDDeleterRawMem<void>>  m_pRawMemory;
};

/// Template class implementing base functionality of the shader object

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class ShaderBase : public DeviceObjectBase<typename EngineImplTraits::ShaderInterface, typename EngineImplTraits::RenderDeviceImplType, ShaderDesc>
{
public:
    // Base interface this class inherits (IShaderD3D12, IShaderVk, etc.)
    using BaseInterface = typename EngineImplTraits::ShaderInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, ShaderDesc>;

    /// \param pRefCounters - Reference counters object that controls the lifetime of this shader.
    /// \param pDevice      - Pointer to the device.
    /// \param Desc         - Shader description.
    /// \param DeviceInfo   - Render device info, see Diligent::RenderDeviceInfo.
    /// \param AdapterInfo  - Graphic adapter info, see Diligent::GraphicsAdapterInfo.
    /// \param bIsDeviceInternal - Flag indicating if the shader is an internal device object and
    ///							   must not keep a strong reference to the device.
    ShaderBase(IReferenceCounters*        pRefCounters,
               RenderDeviceImplType*      pDevice,
               const ShaderDesc&          Desc,
               const RenderDeviceInfo&    DeviceInfo,
               const GraphicsAdapterInfo& AdapterInfo,
               bool                       bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal},
        m_CombinedSamplerSuffix{Desc.CombinedSamplerSuffix != nullptr ? Desc.CombinedSamplerSuffix : ShaderDesc{}.CombinedSamplerSuffix}
    {
        this->m_Desc.CombinedSamplerSuffix = m_CombinedSamplerSuffix.c_str();

        const auto& deviceFeatures = DeviceInfo.Features;
        if (Desc.ShaderType == SHADER_TYPE_GEOMETRY && !deviceFeatures.GeometryShaders)
            LOG_ERROR_AND_THROW("Geometry shaders are not supported by this device.");

        if ((Desc.ShaderType == SHADER_TYPE_DOMAIN || Desc.ShaderType == SHADER_TYPE_HULL) && !deviceFeatures.Tessellation)
            LOG_ERROR_AND_THROW("Tessellation shaders are not supported by this device.");

        if (Desc.ShaderType == SHADER_TYPE_COMPUTE && !deviceFeatures.ComputeShaders)
            LOG_ERROR_AND_THROW("Compute shaders are not supported by this device.");

        if ((Desc.ShaderType == SHADER_TYPE_AMPLIFICATION || Desc.ShaderType == SHADER_TYPE_MESH) && !deviceFeatures.MeshShaders)
            LOG_ERROR_AND_THROW("Mesh shaders are not supported by this device.");

        if ((Desc.ShaderType & SHADER_TYPE_ALL_RAY_TRACING) != 0)
        {
            const auto RTCaps = AdapterInfo.RayTracing.CapFlags;
            if (!deviceFeatures.RayTracing || (RTCaps & RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS) == 0)
                LOG_ERROR_AND_THROW("Standalone ray tracing shaders are not supported by this device.");
        }

        if (Desc.ShaderType == SHADER_TYPE_TILE && !deviceFeatures.TileShaders)
            LOG_ERROR_AND_THROW("Tile shaders are not supported by this device.");
    }

    ~ShaderBase()
    {
        VERIFY(!GetCompileTask(), "Compile task is still running. This may result in a crash if the task accesses resources owned by the shader object.");
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Shader, TDeviceObjectBase)

    virtual SHADER_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override
    {
        VERIFY_EXPR(m_Status.load() != SHADER_STATUS_UNINITIALIZED);
        ASYNC_TASK_STATUS InitTaskStatus = AsyncInitializer::Update(m_AsyncInitializer, WaitForCompletion);
        if (InitTaskStatus == ASYNC_TASK_STATUS_COMPLETE)
        {
            VERIFY(m_Status.load() > SHADER_STATUS_COMPILING, "Shader status must be atomically set by the compiling task before it finishes");
        }
        return m_Status.load();
    }

    bool IsCompiling() const
    {
        return m_Status.load() <= SHADER_STATUS_COMPILING;
    }

    RefCntAutoPtr<IAsyncTask> GetCompileTask() const
    {
        return AsyncInitializer::GetAsyncTask(m_AsyncInitializer);
    }

protected:
    std::unique_ptr<AsyncInitializer> m_AsyncInitializer;

    const std::string m_CombinedSamplerSuffix;

    std::atomic<SHADER_STATUS> m_Status{SHADER_STATUS_UNINITIALIZED};
};

} // namespace Diligent
