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

#include <atomic>

#include "BasicTypes.h"
#include "ReferenceCounters.h"
#include "RefCntAutoPtr.hpp"
#include "DeviceContextBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "IndexWrapper.hpp"

namespace Diligent
{

/// Base implementation of the device context for next-generation backends.

template <typename EngineImplTraits>
class DeviceContextNextGenBase : public DeviceContextBase<EngineImplTraits>
{
public:
    using TBase             = DeviceContextBase<EngineImplTraits>;
    using DeviceImplType    = typename EngineImplTraits::RenderDeviceImplType;
    using ICommandQueueType = typename EngineImplTraits::CommandQueueInterface;

    DeviceContextNextGenBase(IReferenceCounters*      pRefCounters,
                             DeviceImplType*          pRenderDevice,
                             const DeviceContextDesc& Desc) :
        // clang-format off
        TBase{pRefCounters, pRenderDevice, Desc},
        m_SubmittedBuffersCmdQueueMask{Desc.IsDeferred ? 0 : Uint64{1} << Uint64{Desc.ContextId}}
    // clang-format on
    {
    }

    ~DeviceContextNextGenBase()
    {
    }

    virtual ICommandQueueType* DILIGENT_CALL_TYPE LockCommandQueue() override final
    {
        DEV_CHECK_ERR(!this->IsDeferred(), "Deferred contexts have no associated command queues");
        return this->m_pDevice->LockCommandQueue(this->GetCommandQueueId());
    }

    virtual void DILIGENT_CALL_TYPE UnlockCommandQueue() override final
    {
        DEV_CHECK_ERR(!this->IsDeferred(), "Deferred contexts have no associated command queues");
        this->m_pDevice->UnlockCommandQueue(this->GetCommandQueueId());
    }

    SoftwareQueueIndex GetCommandQueueId() const
    {
        return SoftwareQueueIndex{this->GetExecutionCtxId()};
    }

    HardwareQueueIndex GetHardwareQueueId() const { return HardwareQueueIndex{this->m_Desc.QueueId}; }

    Uint64 GetSubmittedBuffersCmdQueueMask() const { return m_SubmittedBuffersCmdQueueMask.load(); }

protected:
    // Should be called at the end of FinishFrame()
    void EndFrame()
    {
        if (this->IsDeferred())
        {
            // For deferred context, reset submitted cmd queue mask
            m_SubmittedBuffersCmdQueueMask.store(0);
        }
        else
        {
            this->m_pDevice->FlushStaleResources(this->GetCommandQueueId());
        }
        TBase::EndFrame();
    }

    void UpdateSubmittedBuffersCmdQueueMask(Uint32 QueueId)
    {
        m_SubmittedBuffersCmdQueueMask.fetch_or(Uint64{1} << Uint64{QueueId});
    }

private:
    // This mask indicates which command queues command buffers from this context were submitted to.
    // For immediate context, this will always be 1 << GetCommandQueueId().
    // For deferred contexts, this will accumulate bits of the queues to which command buffers
    // were submitted to before FinishFrame() was called. This mask is used to release resources
    // allocated by the context during the frame when FinishFrame() is called.
    std::atomic<Uint64> m_SubmittedBuffersCmdQueueMask{0};
};

} // namespace Diligent
