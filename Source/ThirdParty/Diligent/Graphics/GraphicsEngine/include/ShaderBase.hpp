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
/// Implementation of the Diligent::ShaderBase template class

#include <vector>

#include "Shader.h"
#include "DeviceObjectBase.hpp"
#include "STDAllocator.hpp"
#include "PlatformMisc.hpp"
#include "EngineMemory.h"
#include "Align.hpp"

namespace Diligent
{

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

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this shader.
    /// \param pDevice           - Pointer to the device.
    /// \param ShdrDesc          - Shader description.
    /// \param bIsDeviceInternal - Flag indicating if the shader is an internal device object and
    ///							   must not keep a strong reference to the device.
    ShaderBase(IReferenceCounters*        pRefCounters,
               RenderDeviceImplType*      pDevice,
               const ShaderDesc&          ShdrDesc,
               const RenderDeviceInfo&    DeviceInfo,
               const GraphicsAdapterInfo& AdapterInfo,
               bool                       bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, ShdrDesc, bIsDeviceInternal}
    {
        const auto& deviceFeatures = DeviceInfo.Features;
        if (ShdrDesc.ShaderType == SHADER_TYPE_GEOMETRY && !deviceFeatures.GeometryShaders)
            LOG_ERROR_AND_THROW("Geometry shaders are not supported by this device.");

        if ((ShdrDesc.ShaderType == SHADER_TYPE_DOMAIN || ShdrDesc.ShaderType == SHADER_TYPE_HULL) && !deviceFeatures.Tessellation)
            LOG_ERROR_AND_THROW("Tessellation shaders are not supported by this device.");

        if (ShdrDesc.ShaderType == SHADER_TYPE_COMPUTE && !deviceFeatures.ComputeShaders)
            LOG_ERROR_AND_THROW("Compute shaders are not supported by this device.");

        if ((ShdrDesc.ShaderType == SHADER_TYPE_AMPLIFICATION || ShdrDesc.ShaderType == SHADER_TYPE_MESH) && !deviceFeatures.MeshShaders)
            LOG_ERROR_AND_THROW("Mesh shaders are not supported by this device.");

        if ((ShdrDesc.ShaderType & SHADER_TYPE_ALL_RAY_TRACING) != 0)
        {
            const auto RTCaps = AdapterInfo.RayTracing.CapFlags;
            if (!deviceFeatures.RayTracing || (RTCaps & RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS) == 0)
                LOG_ERROR_AND_THROW("Standalone ray tracing shaders are not supported by this device.");
        }

        if (ShdrDesc.ShaderType == SHADER_TYPE_TILE && !deviceFeatures.TileShaders)
            LOG_ERROR_AND_THROW("Tile shaders are not supported by this device.");
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Shader, TDeviceObjectBase)
};

} // namespace Diligent
