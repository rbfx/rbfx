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
/// Implementation of the Diligent::FenceBase template class

#include <atomic>

#include "DeviceObjectBase.hpp"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Template class implementing base functionality of the fence object

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class FenceBase : public DeviceObjectBase<typename EngineImplTraits::FenceInterface, typename EngineImplTraits::RenderDeviceImplType, FenceDesc>
{
public:
    // Base interface that this class inherits (IFenceD3D12, IFenceVk, etc.).
    using BaseInterface = typename EngineImplTraits::FenceInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    typedef DeviceObjectBase<BaseInterface, RenderDeviceImplType, FenceDesc> TDeviceObjectBase;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this command list.
    /// \param Desc              - Fence description
    /// \param pDevice           - Pointer to the device.
    /// \param bIsDeviceInternal - Flag indicating if the Fence is an internal device object and
    ///							   must not keep a strong reference to the device.
    FenceBase(IReferenceCounters* pRefCounters, RenderDeviceImplType* pDevice, const FenceDesc& Desc, bool bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {}

    ~FenceBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Fence, TDeviceObjectBase)


    // Validate IFence::Signal() and IDeviceContext::EnqueueSignal()
    void DvpSignal(Uint64 NewValue)
    {
#ifdef DILIGENT_DEVELOPMENT
        auto EnqueuedValue = m_EnqueuedFenceValue.load();
        DEV_CHECK_ERR(NewValue >= EnqueuedValue,
                      "Fence '", this->m_Desc.Name, "' is being signaled or enqueued for signal with value ", NewValue,
                      ", but the previous value (", EnqueuedValue,
                      ") is greater than the new value. Signal operation will have no effect.");

        while (!m_EnqueuedFenceValue.compare_exchange_weak(EnqueuedValue, std::max(EnqueuedValue, NewValue)))
        {
            // If exchange fails, EnqueuedValue will hold the actual value of m_EnqueuedFenceValue.
        }
#endif
    }

    // Validate IDeviceContext::DeviceWaitForFence()
    void DvpDeviceWait(Uint64 Value)
    {
#ifdef DILIGENT_DEVELOPMENT
        if (!this->GetDevice()->GetFeatures().NativeFence)
        {
            auto EnqueuedValue = m_EnqueuedFenceValue.load();
            DEV_CHECK_ERR(Value <= EnqueuedValue,
                          "Can not wait for value ", Value, " that is greater than the last enqueued for signal value (", EnqueuedValue,
                          "). This is not supported when NativeFence feature is disabled.");
        }
#endif
    }

protected:
    void UpdateLastCompletedFenceValue(Uint64 NewValue)
    {
        auto LastCompletedValue = m_LastCompletedFenceValue.load();
        while (!m_LastCompletedFenceValue.compare_exchange_weak(LastCompletedValue, std::max(LastCompletedValue, NewValue)))
        {
            // If exchange fails, LastCompletedValue will hold the actual value of m_LastCompletedFenceValue.
        }
    }

    std::atomic<Uint64> m_LastCompletedFenceValue{0};

#ifdef DILIGENT_DEVELOPMENT
    std::atomic<Uint64> m_EnqueuedFenceValue{0};
#endif
};

} // namespace Diligent
