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
/// Declaration of Diligent::CommandQueueVkImpl class

#include <mutex>
#include <deque>
#include <atomic>

#include "EngineVkImplTraits.hpp"
#include "ObjectBase.hpp"
#include "FenceVkImpl.hpp"

#include "VulkanUtilities/VulkanHeaders.h"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanSyncObjectManager.hpp"


namespace Diligent
{

class SyncPointVk final : public std::enable_shared_from_this<SyncPointVk>
{
private:
    friend class CommandQueueVkImpl;
    SyncPointVk(SoftwareQueueIndex                        CommandQueueId,
                Uint32                                    NumContexts,
                VulkanUtilities::VulkanSyncObjectManager& SyncObjectMngr,
                VkDevice                                  LogicalDevice,
                Uint64                                    dbgValue);

    void GetSemaphores(std::vector<VkSemaphore>& Semaphores);

    static constexpr size_t SizeOf(Uint32 NumContexts)
    {
        return sizeof(SyncPointVk) + sizeof(VulkanUtilities::VulkanRecycledSemaphore) * (NumContexts - _countof(SyncPointVk::m_Semaphores));
    }

public:
    ~SyncPointVk();

    // Returns semaphore which is in signaled state.
    // Access to semaphore in CommandQueueId index must be thread safe.
    VulkanUtilities::VulkanRecycledSemaphore ExtractSemaphore(Uint32 CommandQueueId)
    {
        return std::move(m_Semaphores[CommandQueueId]);
    }

    // vkGetFenceStatus and vkWaitForFences with same fence can be used in multiple threads.
    // Other functions requires external synchronization.
    VkFence GetFence() const
    {
        return m_Fence;
    }

    SoftwareQueueIndex GetCommandQueueId() const
    {
        return m_CommandQueueId;
    }

private:
    const SoftwareQueueIndex                 m_CommandQueueId;
    const Uint8                              m_NumSemaphores; // same as NumContexts
    VulkanUtilities::VulkanRecycledFence     m_Fence;
    VulkanUtilities::VulkanRecycledSemaphore m_Semaphores[1]; // [m_NumSemaphores]
};


/// Implementation of the Diligent::ICommandQueueVk interface
class CommandQueueVkImpl final : public ObjectBase<ICommandQueueVk>
{
public:
    using TBase = ObjectBase<ICommandQueueVk>;

    CommandQueueVkImpl(IReferenceCounters*                                   pRefCounters,
                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice> LogicalDevice,
                       SoftwareQueueIndex                                    CommandQueueId,
                       Uint32                                                NumCommandQueues,
                       Uint32                                                vkQueueIndex,
                       const ImmediateContextCreateInfo&                     CreateInfo);
    ~CommandQueueVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_CommandQueueVk, TBase)

    /// Implementation of ICommandQueue::GetNextFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetNextFenceValue() const override final { return m_NextFenceValue.load(); }

    /// Implementation of ICommandQueue::GetCompletedFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedFenceValue() override final;

    /// Implementation of ICommandQueue::WaitForIdle().
    virtual Uint64 DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of ICommandQueueVk::SubmitCmdBuffer().
    virtual Uint64 DILIGENT_CALL_TYPE SubmitCmdBuffer(VkCommandBuffer cmdBuffer) override final;

    /// Implementation of ICommandQueueVk::Submit().
    virtual Uint64 DILIGENT_CALL_TYPE Submit(const VkSubmitInfo& SubmitInfo) override final;

    /// Implementation of ICommandQueueVk::Present().
    virtual VkResult DILIGENT_CALL_TYPE Present(const VkPresentInfoKHR& PresentInfo) override final;

    /// Implementation of ICommandQueueVk::BindSparse().
    virtual Uint64 DILIGENT_CALL_TYPE BindSparse(const VkBindSparseInfo& InBindInfo) override final;

    /// Implementation of ICommandQueueVk::GetVkQueue().
    virtual VkQueue DILIGENT_CALL_TYPE GetVkQueue() override final { return m_VkQueue; }

    /// Implementation of ICommandQueueVk::GetQueueFamilyIndex().
    virtual uint32_t DILIGENT_CALL_TYPE GetQueueFamilyIndex() const override final { return m_QueueFamilyIndex; }

    /// Implementation of ICommandQueueVk::EnqueueSignalFence().
    virtual void DILIGENT_CALL_TYPE EnqueueSignalFence(VkFence vkFence) override final;

    /// Implementation of ICommandQueueVk::EnqueueSignal().
    virtual void DILIGENT_CALL_TYPE EnqueueSignal(VkSemaphore vkTimelineSemaphore, Uint64 Value) override final;

    void SetFence(RefCntAutoPtr<FenceVkImpl> pFence)
    {
        VERIFY_EXPR(pFence->GetDesc().Type == FENCE_TYPE_CPU_WAIT_ONLY);
        VERIFY_EXPR(!pFence->IsTimelineSemaphore());
        m_pFence = std::move(pFence);
    }

    SyncPointVkPtr GetLastSyncPoint()
    {
        Threading::SpinLockGuard Guard{m_LastSyncPointLock};
        return m_LastSyncPoint;
    }

private:
    SyncPointVkPtr CreateSyncPoint(Uint64 dbgValue);

    void InternalSignalSemaphore(VkSemaphore vkTimelineSemaphore, Uint64 Value);

    std::shared_ptr<VulkanUtilities::VulkanLogicalDevice> m_LogicalDevice;

    const VkQueue            m_VkQueue;
    const HardwareQueueIndex m_QueueFamilyIndex;
    const SoftwareQueueIndex m_CommandQueueId;
    const bool               m_SupportedTimelineSemaphore;
    const Uint8              m_NumCommandQueues;

    // Fence is signaled right after a command buffer has been
    // submitted to the command queue for execution.
    // All command buffers with fence value less than or equal to the signaled value
    // are guaranteed to be finished by the GPU
    RefCntAutoPtr<FenceVkImpl> m_pFence;

    // A value that will be signaled by the command queue next
    std::atomic<Uint64> m_NextFenceValue{1};

    // Protects access to the m_VkQueue internal data.
    std::mutex m_QueueMutex;

    // Array used to merge semaphores from SubmitInfo and from SyncPointVk
    std::vector<VkSemaphore> m_TempSignalSemaphores;

    // Protects access to the m_LastSyncPoint
    Threading::SpinLock m_LastSyncPointLock;

    // Fence and semaphores which were signaled when the last submitted commands have been completed.
    SyncPointVkPtr m_LastSyncPoint;

    std::shared_ptr<VulkanUtilities::VulkanSyncObjectManager> m_SyncObjectManager;
    FixedBlockMemoryAllocator                                 m_SyncPointAllocator;
};

} // namespace Diligent
