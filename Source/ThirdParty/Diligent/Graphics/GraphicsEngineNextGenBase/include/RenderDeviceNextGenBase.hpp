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

#include <vector>
#include <mutex>
#include <atomic>

#include "PrivateConstants.h"
#include "EngineFactory.h"
#include "BasicTypes.h"
#include "ReferenceCounters.h"
#include "MemoryAllocator.h"
#include "RefCntAutoPtr.hpp"
#include "PlatformMisc.hpp"
#include "ResourceReleaseQueue.hpp"
#include "EngineMemory.h"
#include "IndexWrapper.hpp"

namespace Diligent
{

struct EngineCreateInfo;

/// Base implementation of the render device for next-generation backends.

template <class TBase, typename CommandQueueType>
class RenderDeviceNextGenBase : public TBase
{
public:
    RenderDeviceNextGenBase(IReferenceCounters*        pRefCounters,
                            IMemoryAllocator&          RawMemAllocator,
                            IEngineFactory*            pEngineFactory,
                            size_t                     CmdQueueCount,
                            CommandQueueType**         Queues,
                            const EngineCreateInfo&    EngineCI,
                            const GraphicsAdapterInfo& AdapterInfo) :
        TBase{pRefCounters, RawMemAllocator, pEngineFactory, EngineCI, AdapterInfo},
        m_CmdQueueCount{CmdQueueCount}
    {
        VERIFY(m_CmdQueueCount < MAX_COMMAND_QUEUES, "The number of command queue is greater than maximum allowed value (", MAX_COMMAND_QUEUES, ")");

        m_CommandQueues = ALLOCATE(this->m_RawMemAllocator, "Raw memory for the device command/release queues", CommandQueue, m_CmdQueueCount);
        for (size_t q = 0; q < m_CmdQueueCount; ++q)
            new (m_CommandQueues + q) CommandQueue{RefCntAutoPtr<CommandQueueType>(Queues[q]), this->m_RawMemAllocator};
    }

    ~RenderDeviceNextGenBase()
    {
        DestroyCommandQueues();
    }

    // The following basic requirement guarantees correctness of resource deallocation:
    //
    //        A resource is never released before the last draw command referencing it is submitted to the command queue
    //

    //
    // CPU
    //                       Last Reference
    //                        of resource X
    //                             |
    //                             |     Submit Cmd       Submit Cmd            Submit Cmd
    //                             |      List N           List N+1              List N+2
    //                             V         |                |                     |
    //    NextFenceValue       |   *  N      |      N+1       |          N+2        |
    //
    //
    //    CompletedFenceValue       |     N-3      |      N-2      |        N-1        |        N       |
    //                              .              .               .                   .                .
    // -----------------------------.--------------.---------------.-------------------.----------------.-------------
    //                              .              .               .                   .                .
    //
    // GPU                          | Cmd List N-2 | Cmd List N-1  |    Cmd List N     |   Cmd List N+1 |
    //                                                                                 |
    //                                                                                 |
    //                                                                          Resource X can
    //                                                                           be released
    template <typename ObjectType, typename = typename std::enable_if<std::is_object<ObjectType>::value>::type>
    void SafeReleaseDeviceObject(ObjectType&& Object, Uint64 QueueMask)
    {
        VERIFY(m_CommandQueues != nullptr, "Command queues have been destroyed. Are you releasing an object from the render device destructor?");

        QueueMask &= GetCommandQueueMask();

        VERIFY(QueueMask != 0, "At least one bit should be set in the command queue mask");
        if (QueueMask == 0)
            return;

        DynamicStaleResourceWrapper::RefCounterType NumReferences = PlatformMisc::CountOneBits(QueueMask);

        auto Wrapper = DynamicStaleResourceWrapper::Create(std::move(Object), NumReferences);

        while (QueueMask != 0)
        {
            auto QueueInd = PlatformMisc::GetLSB(QueueMask);
            VERIFY_EXPR(QueueInd < m_CmdQueueCount);

            auto& Queue = m_CommandQueues[QueueInd];
            // Do not use std::move on wrapper!!!
            Queue.ReleaseQueue.SafeReleaseResource(Wrapper, Queue.NextCmdBufferNumber.load());
            QueueMask &= ~(Uint64{1} << Uint64{QueueInd});
            --NumReferences;
        }
        VERIFY_EXPR(NumReferences == 0);

        Wrapper.GiveUpOwnership();
    }

    size_t GetCommandQueueCount() const
    {
        return m_CmdQueueCount;
    }

    Uint64 GetCommandQueueMask() const
    {
        return (m_CmdQueueCount < MAX_COMMAND_QUEUES) ? ((Uint64{1} << Uint64{m_CmdQueueCount}) - 1) : ~Uint64{0};
    }

    void PurgeReleaseQueues(bool ForceRelease = false)
    {
        for (Uint32 q = 0; q < m_CmdQueueCount; ++q)
            PurgeReleaseQueue(SoftwareQueueIndex{q}, ForceRelease);
    }

    void PurgeReleaseQueue(SoftwareQueueIndex QueueInd, bool ForceRelease = false)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto& Queue               = m_CommandQueues[QueueInd];
        auto  CompletedFenceValue = ForceRelease ? std::numeric_limits<Uint64>::max() : Queue.CmdQueue->GetCompletedFenceValue();
        Queue.ReleaseQueue.Purge(CompletedFenceValue);
    }

    void IdleCommandQueue(SoftwareQueueIndex QueueInd, bool ReleaseResources)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueInd];

        Uint64 CmdBufferNumber = 0;
        Uint64 FenceValue      = 0;
        {
            std::lock_guard<std::mutex> Lock{Queue.Mtx};

            if (ReleaseResources)
            {
                // Increment the command buffer number before idling the queue.
                // This will make sure that any resource released while this function
                // is running will be associated with the next command buffer submission.
                CmdBufferNumber = Queue.NextCmdBufferNumber.fetch_add(1);
                // fetch_add returns the original value immediately preceding the addition.
            }

            FenceValue = Queue.CmdQueue->WaitForIdle();
        }

        if (ReleaseResources)
        {
            Queue.ReleaseQueue.DiscardStaleResources(CmdBufferNumber, FenceValue);
            Queue.ReleaseQueue.Purge(Queue.CmdQueue->GetCompletedFenceValue());
        }
    }

    void IdleAllCommandQueues(bool ReleaseResources)
    {
        for (Uint32 q = 0; q < m_CmdQueueCount; ++q)
            IdleCommandQueue(SoftwareQueueIndex{q}, ReleaseResources);
    }

    struct SubmittedCommandBufferInfo
    {
        Uint64 CmdBufferNumber = 0;
        Uint64 FenceValue      = 0;
    };
    template <typename... SubmitDataType>
    SubmittedCommandBufferInfo SubmitCommandBuffer(SoftwareQueueIndex QueueInd, bool DiscardStaleResources, const SubmitDataType&... SubmitData)
    {
        SubmittedCommandBufferInfo CmdBuffInfo;
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueInd];

        {
            std::lock_guard<std::mutex> Lock{Queue.Mtx};

            // Increment the command buffer number before submitting the cmd buffer.
            // This will make sure that any resource released while this function
            // is running will be associated with the next command buffer.
            CmdBuffInfo.CmdBufferNumber = Queue.NextCmdBufferNumber.fetch_add(1);
            // fetch_add returns the original value immediately preceding the addition.

            CmdBuffInfo.FenceValue = Queue.CmdQueue->Submit(SubmitData...);
        }

        if (DiscardStaleResources)
        {
            // The following basic requirement guarantees correctness of resource deallocation:
            //
            //        A resource is never released before the last draw command referencing it is submitted for execution
            //

            // Move stale objects into the release queue.
            // Note that objects are moved from stale list to release queue based on the cmd buffer number,
            // not fence value. This makes sure that basic requirement is met even when the fence value is
            // not incremented while executing the command buffer (as is the case with Unity command queue).

            // As long as resources used by deferred contexts are not released before the command list
            // is executed through immediate context, this strategy always works.
            Queue.ReleaseQueue.DiscardStaleResources(CmdBuffInfo.CmdBufferNumber, CmdBuffInfo.FenceValue);
        }

        return CmdBuffInfo;
    }

    ResourceReleaseQueue<DynamicStaleResourceWrapper>& GetReleaseQueue(SoftwareQueueIndex QueueInd)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        return m_CommandQueues[QueueInd].ReleaseQueue;
    }

    const CommandQueueType& GetCommandQueue(SoftwareQueueIndex CommandQueueInd) const
    {
        VERIFY_EXPR(CommandQueueInd < m_CmdQueueCount);
        return *m_CommandQueues[CommandQueueInd].CmdQueue;
    }

    Uint64 GetCompletedFenceValue(SoftwareQueueIndex CommandQueueInd)
    {
        return m_CommandQueues[CommandQueueInd].CmdQueue->GetCompletedFenceValue();
    }

    Uint64 GetNextFenceValue(SoftwareQueueIndex CommandQueueInd)
    {
        return m_CommandQueues[CommandQueueInd].CmdQueue->GetNextFenceValue();
    }

    template <typename TAction>
    void LockCmdQueueAndRun(SoftwareQueueIndex QueueInd, TAction Action)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto&                       Queue = m_CommandQueues[QueueInd];
        std::lock_guard<std::mutex> Lock{Queue.Mtx};
        Action(Queue.CmdQueue);
    }

    CommandQueueType* LockCommandQueue(SoftwareQueueIndex QueueInd)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueInd];
        Queue.Mtx.lock();
        return Queue.CmdQueue;
    }

    void UnlockCommandQueue(SoftwareQueueIndex QueueInd)
    {
        VERIFY_EXPR(QueueInd < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueInd];
        Queue.Mtx.unlock();
    }

protected:
    void DestroyCommandQueues()
    {
        if (m_CommandQueues != nullptr)
        {
            for (size_t q = 0; q < m_CmdQueueCount; ++q)
            {
                auto& Queue = m_CommandQueues[q];
                DEV_CHECK_ERR(Queue.ReleaseQueue.GetStaleResourceCount() == 0, "All stale resources must be released before destroying a command queue");
                DEV_CHECK_ERR(Queue.ReleaseQueue.GetPendingReleaseResourceCount() == 0, "All resources must be released before destroying a command queue");
                Queue.~CommandQueue();
            }
            this->m_RawMemAllocator.Free(m_CommandQueues);
            m_CommandQueues = nullptr;
        }
    }

    struct CommandQueue
    {
        CommandQueue(RefCntAutoPtr<CommandQueueType> _CmdQueue, IMemoryAllocator& Allocator) noexcept :
            CmdQueue{std::move(_CmdQueue)},
            ReleaseQueue{Allocator}
        {
            NextCmdBufferNumber.store(0);
        }

        // clang-format off
        CommandQueue             (const CommandQueue&)  = delete;
        CommandQueue             (      CommandQueue&&) = delete;
        CommandQueue& operator = (const CommandQueue&)  = delete;
        CommandQueue& operator = (      CommandQueue&&) = delete;
        // clang-format on

        std::mutex                                        Mtx; // Protects access to the CmdQueue.
        std::atomic<Uint64>                               NextCmdBufferNumber{0};
        RefCntAutoPtr<CommandQueueType>                   CmdQueue;
        ResourceReleaseQueue<DynamicStaleResourceWrapper> ReleaseQueue;
    };
    const size_t  m_CmdQueueCount = 0;
    CommandQueue* m_CommandQueues = nullptr;
};

} // namespace Diligent
