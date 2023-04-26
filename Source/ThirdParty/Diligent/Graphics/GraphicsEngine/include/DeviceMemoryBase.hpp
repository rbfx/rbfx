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
/// Implementation of the Diligent::DeviceMemoryBase template class

#include "DeviceMemory.h"
#include "GraphicsTypes.h"
#include "DeviceObjectBase.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

/// Validates device memory description and throws an exception in case of an error.
void ValidateDeviceMemoryDesc(const DeviceMemoryDesc& Desc, const IRenderDevice* pDevice) noexcept(false);


/// Template class implementing base functionality of the device memory object

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class DeviceMemoryBase : public DeviceObjectBase<typename EngineImplTraits::DeviceMemoryInterface, typename EngineImplTraits::RenderDeviceImplType, DeviceMemoryDesc>
{
public:
    // Base interface that this class inherits (IDeviceMemoryD3D12, IDeviceMemoryVk, etc.).
    using BaseInterface = typename EngineImplTraits::DeviceMemoryInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, DeviceMemoryDesc>;

    /// \param pRefCounters - Reference counters object that controls the lifetime of this device memory object.
    /// \param pDevice      - Pointer to the device.
    /// \param MemCI        - Device memory create info.
    DeviceMemoryBase(IReferenceCounters*           pRefCounters,
                     RenderDeviceImplType*         pDevice,
                     const DeviceMemoryCreateInfo& MemCI) :
        TDeviceObjectBase{pRefCounters, pDevice, MemCI.Desc, false}
    {
        if (MemCI.InitialSize == 0)
            LOG_ERROR_AND_THROW("DeviceMemoryCreateInfo::InitialSize must not be zero");

        ValidateDeviceMemoryDesc(this->m_Desc, this->GetDevice());

        Uint64 DeviceQueuesMask = this->GetDevice()->GetCommandQueueMask();
        DEV_CHECK_ERR((this->m_Desc.ImmediateContextMask & DeviceQueuesMask) != 0,
                      "No bits in the immediate context mask (0x", std::hex, this->m_Desc.ImmediateContextMask,
                      ") correspond to one of ", this->GetDevice()->GetCommandQueueCount(), " available software command queues");
        this->m_Desc.ImmediateContextMask &= DeviceQueuesMask;
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceMemory, TDeviceObjectBase)

    void DvpVerifyResize(Uint64 NewSize) const
    {
        DEV_CHECK_ERR((NewSize % this->m_Desc.PageSize) == 0,
                      "NewSize (", NewSize, ") must be  a multiple of the page size (", this->m_Desc.PageSize, ")");
    }
};

} // namespace Diligent
