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

#include "pch.h"
#include <thread>

#include "CommandQueueVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"

namespace Diligent
{

CommandQueueVkImpl::CommandQueueVkImpl(IReferenceCounters*                                   pRefCounters,
                                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice> LogicalDevice,
                                       SoftwareQueueIndex                                    CommandQueueId,
                                       Uint32                                                NumCommandQueues,
                                       Uint32                                                vkQueueIndex,
                                       const ImmediateContextCreateInfo&                     CreateInfo) :
    // clang-format off
    TBase{pRefCounters},
    m_LogicalDevice             {LogicalDevice},
    m_VkQueue                   {LogicalDevice->GetQueue(HardwareQueueIndex{CreateInfo.QueueId}, vkQueueIndex)},
    m_QueueFamilyIndex          {CreateInfo.QueueId},
    m_CommandQueueId            {static_cast<Uint8>(CommandQueueId)},
    m_SupportedTimelineSemaphore{LogicalDevice->GetEnabledExtFeatures().TimelineSemaphore.timelineSemaphore == VK_TRUE},
    m_NumCommandQueues          {static_cast<Uint8>(m_SupportedTimelineSemaphore ? 1u : NumCommandQueues)},
    m_NextFenceValue            {1},
    m_SyncObjectManager         {std::make_shared<VulkanUtilities::VulkanSyncObjectManager>(*LogicalDevice)},
    m_SyncPointAllocator        {GetRawAllocator(), SyncPointVk::SizeOf(m_NumCommandQueues), 16}
// clang-format on
{
    VERIFY(m_CommandQueueId == CommandQueueId, "Not enough bits to store command queue index");
    VERIFY(m_SupportedTimelineSemaphore || m_NumCommandQueues == NumCommandQueues, "Not enough bits to store command queue count");

    if (CreateInfo.Name != nullptr)
        VulkanUtilities::SetQueueName(m_LogicalDevice->GetVkDevice(), m_VkQueue, CreateInfo.Name);

    m_TempSignalSemaphores.reserve(16);
}

CommandQueueVkImpl::~CommandQueueVkImpl()
{
    // Fence have resources that will be added to release queue.
    // But release queue will be destroyed after command queue and it will not release new resources.
    if (m_pFence)
        m_pFence->ImmediatelyReleaseResources();

    m_pFence.Release();
    m_LastSyncPoint.reset();

    // Queues are created along with the logical device during vkCreateDevice.
    // All queues associated with the logical device are destroyed when vkDestroyDevice
    // is called on that device.
}

SyncPointVk::SyncPointVk(SoftwareQueueIndex                        CommandQueueId,
                         Uint32                                    NumContexts,
                         VulkanUtilities::VulkanSyncObjectManager& SyncObjectMngr,
                         VkDevice                                  LogicalDevice,
                         Uint64                                    dbgValue) :
    m_CommandQueueId{CommandQueueId},
    m_NumSemaphores{static_cast<Uint8>(NumContexts)},
    m_Fence{SyncObjectMngr.CreateFence()}
{
    VERIFY(m_CommandQueueId == CommandQueueId, "Not enough bits to store command queue index");
    VERIFY(m_NumSemaphores == NumContexts, "Not enough bits to store command queue count");

    // Call constructors for semaphores
    for (Uint32 s = _countof(m_Semaphores); s < NumContexts; ++s)
        new (&m_Semaphores[s]) VulkanUtilities::VulkanRecycledSemaphore{};

    // Semaphores are used to synchronize between queues; they are not used for synchronization within one queue.
    if (NumContexts > 1)
    {
        SyncObjectMngr.CreateSemaphores(m_Semaphores, NumContexts - 1);

        // Semaphore for the current queue is not used.
        std::swap(m_Semaphores[CommandQueueId], m_Semaphores[NumContexts - 1]);
    }

#ifdef DILIGENT_DEBUG
    String Name = String{"Queue ("} + std::to_string(CommandQueueId) + ") Value (" + std::to_string(dbgValue) + ")";
    VulkanUtilities::SetFenceName(LogicalDevice, m_Fence, Name.c_str());

    for (Uint32 s = 0; s < m_NumSemaphores; ++s)
    {
        if (m_Semaphores[s])
        {
            Name = String{"Queue ("} + std::to_string(CommandQueueId) + ") Value (" + std::to_string(dbgValue) + ") Ctx (" + std::to_string(s) + ")";
            VulkanUtilities::SetSemaphoreName(LogicalDevice, m_Semaphores[s], Name.c_str());
        }
    }
#endif
}

SyncPointVk::~SyncPointVk()
{
    // Call destructors for semaphores
    for (Uint32 s = _countof(m_Semaphores); s < m_NumSemaphores; ++s)
        m_Semaphores[s].~RecycledSyncObject();
}

__forceinline void SyncPointVk::GetSemaphores(std::vector<VkSemaphore>& Semaphores)
{
    for (Uint32 s = 0; s < m_NumSemaphores; ++s)
    {
        if (m_Semaphores[s])
            Semaphores.push_back(m_Semaphores[s]);
    }
}

__forceinline SyncPointVkPtr CommandQueueVkImpl::CreateSyncPoint(Uint64 dbgValue)
{
    auto* pAllocator = &m_SyncPointAllocator;
    void* ptr        = pAllocator->Allocate(SyncPointVk::SizeOf(m_NumCommandQueues), "SyncPointVk", __FILE__, __LINE__);
    auto  Deleter    = [pAllocator](SyncPointVk* ptr) //
    {
        ptr->~SyncPointVk();
        pAllocator->Free(ptr);
    };

    return {new (ptr) SyncPointVk{m_CommandQueueId, m_NumCommandQueues, *m_SyncObjectManager, m_LogicalDevice->GetVkDevice(), dbgValue}, std::move(Deleter)};
}

Uint64 CommandQueueVkImpl::Submit(const VkSubmitInfo& InSubmitInfo)
{
    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};

    // Increment the value before submitting the buffer to be overly safe
    const uint64_t FenceValue = m_NextFenceValue.fetch_add(1);

    auto NewSyncPoint = CreateSyncPoint(FenceValue);

    m_TempSignalSemaphores.clear();
    NewSyncPoint->GetSemaphores(m_TempSignalSemaphores);

#ifdef DILIGENT_DEBUG
    const VkBaseInStructure* pStruct = static_cast<const VkBaseInStructure*>(InSubmitInfo.pNext);
    for (; pStruct != nullptr;)
    {
        if (pStruct->sType == VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO)
        {
            VERIFY(m_TempSignalSemaphores.empty(), "Can not append semaphores when timeline semaphores are used");
            break;
        }
        pStruct = pStruct->pNext;
    }
#endif

    for (uint32_t s = 0; s < InSubmitInfo.signalSemaphoreCount; ++s)
        m_TempSignalSemaphores.push_back(InSubmitInfo.pSignalSemaphores[s]);

    VkSubmitInfo SubmitInfo         = InSubmitInfo;
    SubmitInfo.signalSemaphoreCount = static_cast<Uint32>(m_TempSignalSemaphores.size());
    SubmitInfo.pSignalSemaphores    = m_TempSignalSemaphores.data();

    const uint32_t SubmitCount =
        (SubmitInfo.waitSemaphoreCount != 0 ||
         SubmitInfo.commandBufferCount != 0 ||
         SubmitInfo.signalSemaphoreCount != 0) ?
        1 :
        0;

    auto err = vkQueueSubmit(m_VkQueue, SubmitCount, &SubmitInfo, NewSyncPoint->GetFence());
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to submit command buffer to the command queue");
    (void)err;

    VERIFY(m_pFence != nullptr, "Command queue fence has not been initialized");
    m_pFence->AddPendingSyncPoint(m_CommandQueueId, FenceValue, NewSyncPoint);

    // Update the last sync point
    {
        Threading::SpinLockGuard SyncPointGuard{m_LastSyncPointLock};
        m_LastSyncPoint = std::move(NewSyncPoint);
    }

    return FenceValue;
}

Uint64 CommandQueueVkImpl::SubmitCmdBuffer(VkCommandBuffer cmdBuffer)
{
    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = cmdBuffer != VK_NULL_HANDLE ? 1 : 0;
    SubmitInfo.pCommandBuffers    = &cmdBuffer;
    SubmitInfo.waitSemaphoreCount = 0;       // the number of semaphores upon which to wait before executing the command buffers
    SubmitInfo.pWaitSemaphores    = nullptr; // a pointer to an array of semaphores upon which to wait before the command
                                             // buffers begin execution
    SubmitInfo.pWaitDstStageMask = nullptr;  // a pointer to an array of pipeline stages at which each corresponding
                                             // semaphore wait will occur
    SubmitInfo.signalSemaphoreCount = 0;     // the number of semaphores to be signaled once the commands specified in
                                             // pCommandBuffers have completed execution
    SubmitInfo.pSignalSemaphores = nullptr;  // a pointer to an array of semaphores which will be signaled when the
                                             // command buffers for this batch have completed execution

    return Submit(SubmitInfo);
}

Uint64 CommandQueueVkImpl::WaitForIdle()
{
    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};

    // Update last completed fence value to unlock all waiting events.
    const auto FenceValue = m_NextFenceValue.fetch_add(1);

    vkQueueWaitIdle(m_VkQueue);
    // For some reason after idling the queue not all fences are signaled
    m_pFence->Wait(UINT64_MAX);
    m_pFence->Reset(FenceValue);

    return FenceValue;
}

Uint64 CommandQueueVkImpl::GetCompletedFenceValue()
{
    return m_pFence->GetCompletedValue();
}

void CommandQueueVkImpl::EnqueueSignalFence(VkFence vkFence)
{
    DEV_CHECK_ERR(vkFence != VK_NULL_HANDLE, "vkFence must not be null");

    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};

    auto err = vkQueueSubmit(m_VkQueue, 0, nullptr, vkFence);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to submit fence signal command to the command queue");
    (void)err;
}

void CommandQueueVkImpl::EnqueueSignal(VkSemaphore vkTimelineSemaphore, Uint64 Value)
{
    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};
    InternalSignalSemaphore(vkTimelineSemaphore, Value);
}

void CommandQueueVkImpl::InternalSignalSemaphore(VkSemaphore vkTimelineSemaphore, Uint64 Value)
{
    DEV_CHECK_ERR(vkTimelineSemaphore != VK_NULL_HANDLE, "vkTimelineSemaphore must not be null");
    DEV_CHECK_ERR(m_SupportedTimelineSemaphore, "Timeline semaphore is not supported");

    VkTimelineSemaphoreSubmitInfo TimelineSemaphoreSubmitInfo{};
    TimelineSemaphoreSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    TimelineSemaphoreSubmitInfo.pNext                     = nullptr;
    TimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = 0;
    TimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = nullptr;
    TimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = 1;
    TimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = &Value;

    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext                = &TimelineSemaphoreSubmitInfo;
    SubmitInfo.waitSemaphoreCount   = 0;
    SubmitInfo.pWaitSemaphores      = nullptr;
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores    = &vkTimelineSemaphore;

    auto err = vkQueueSubmit(m_VkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to submit timeline semaphore signal command to the command queue");
    (void)err;
}

VkResult CommandQueueVkImpl::Present(const VkPresentInfoKHR& PresentInfo)
{
    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};
    return vkQueuePresentKHR(m_VkQueue, &PresentInfo);
}

Uint64 CommandQueueVkImpl::BindSparse(const VkBindSparseInfo& InBindInfo)
{
    std::lock_guard<std::mutex> QueueGuard{m_QueueMutex};

    // Increment the value before submitting the buffer to be overly safe
    const uint64_t FenceValue = m_NextFenceValue.fetch_add(1);

    auto NewSyncPoint = CreateSyncPoint(FenceValue);

    m_TempSignalSemaphores.clear();
    NewSyncPoint->GetSemaphores(m_TempSignalSemaphores);

#ifdef DILIGENT_DEBUG
    const VkBaseInStructure* pStruct = static_cast<const VkBaseInStructure*>(InBindInfo.pNext);
    for (; pStruct != nullptr;)
    {
        if (pStruct->sType == VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO)
        {
            VERIFY(m_TempSignalSemaphores.empty(), "Can not append semaphores when timeline semaphores are used");
            break;
        }
        pStruct = pStruct->pNext;
    }
#endif

    for (uint32_t s = 0; s < InBindInfo.signalSemaphoreCount; ++s)
        m_TempSignalSemaphores.push_back(InBindInfo.pSignalSemaphores[s]);

    VkBindSparseInfo BindInfo     = InBindInfo;
    BindInfo.signalSemaphoreCount = static_cast<Uint32>(m_TempSignalSemaphores.size());
    BindInfo.pSignalSemaphores    = m_TempSignalSemaphores.data();

    auto err = vkQueueBindSparse(m_VkQueue, 1, &BindInfo, NewSyncPoint->GetFence());
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to submit sparse bind commands to the command queue");
    (void)err;

    VERIFY(m_pFence != nullptr, "Command queue fence has not been initialized");
    m_pFence->AddPendingSyncPoint(m_CommandQueueId, FenceValue, NewSyncPoint);

    // Update the last sync point
    {
        Threading::SpinLockGuard SyncPointGuard{m_LastSyncPointLock};
        m_LastSyncPoint = std::move(NewSyncPoint);
    }

    return FenceValue;
}

} // namespace Diligent
