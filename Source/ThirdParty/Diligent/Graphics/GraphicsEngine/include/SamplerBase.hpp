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
/// Implementation of the Diligent::SamplerBase template class

#include "Sampler.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"

namespace Diligent
{

/// Validates sampler description and throws an exception in case of an error.
void ValidateSamplerDesc(const SamplerDesc& Desc, const IRenderDevice* pDevice) noexcept(false);

/// Template class implementing base functionality of the sampler object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class SamplerBase : public DeviceObjectBase<typename EngineImplTraits::SamplerInterface, typename EngineImplTraits::RenderDeviceImplType, SamplerDesc>
{
public:
    // Base interface this class inherits (ISamplerD3D12, ISamplerVk, etc.)
    using BaseInterface = typename EngineImplTraits::SamplerInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, SamplerDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this sampler.
    /// \param pDevice           - Pointer to the device.
    /// \param SamDesc           - Sampler description.
    /// \param bIsDeviceInternal - Flag indicating if the sampler is an internal device object and
    ///							   must not keep a strong reference to the device.
    SamplerBase(IReferenceCounters* pRefCounters, RenderDeviceImplType* pDevice, const SamplerDesc& SamDesc, bool bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, SamDesc, bIsDeviceInternal}
    {
        ValidateSamplerDesc(this->m_Desc, this->GetDevice());
    }

    /// Special constructor that is only used for serialization when there is no device
    SamplerBase(IReferenceCounters* pRefCounters, const SamplerDesc& SamDesc) noexcept :
        TDeviceObjectBase{pRefCounters, nullptr, SamDesc, true /*Pretend device internal to allow null device*/}
    {
    }

    ~SamplerBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Sampler, TDeviceObjectBase)
};

} // namespace Diligent
