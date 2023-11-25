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
/// Declaration of Diligent::FenceVkImpl class

#include <deque>
#include <atomic>

#include "EngineVkImplTraits.hpp"
#include "FenceBase.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanSyncObjectManager.hpp"

namespace Diligent
{
using SyncPointVkPtr = std::shared_ptr<class SyncPointVk>;


/// Fence implementation in Vulkan backend.
class FenceVkImpl final : public FenceBase<EngineVkImplTraits>
{
public:
    using TFenceBase = FenceBase<EngineVkImplTraits>;

    FenceVkImpl(IReferenceCounters* pRefCounters,
                RenderDeviceVkImpl* pRenderDeviceVkImpl,
                const FenceDesc&    Desc,
                bool                IsDeviceInternal = false);
    FenceVkImpl(IReferenceCounters* pRefCounters,
                RenderDeviceVkImpl* pRenderDeviceVkImpl,
                const FenceDesc&    Desc,
                VkSemaphore         vkTimelineSemaphore);
    ~FenceVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_FenceVk, TFenceBase)

    /// Implementation of IFence::GetCompletedValue() in Vulkan backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedValue() override final;

    /// Implementation of IFence::Signal() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE Signal(Uint64 Value) override final;

    /// Implementation of IFence::Wait() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE Wait(Uint64 Value) override final;

    /// Implementation of IFenceVk::GetVkSemaphore().
    virtual VkSemaphore DILIGENT_CALL_TYPE GetVkSemaphore() override final { return m_TimelineSemaphore; }

    VulkanUtilities::VulkanRecycledSemaphore ExtractSignalSemaphore(SoftwareQueueIndex CommandQueueId, Uint64 Value);

    void Reset(Uint64 Value);

    void AddPendingSyncPoint(SoftwareQueueIndex CommandQueueId, Uint64 Value, SyncPointVkPtr SyncPoint);

    bool IsTimelineSemaphore() const { return m_TimelineSemaphore; }

    void ImmediatelyReleaseResources();

private:
    Uint64 InternalGetCompletedValue();

    VulkanUtilities::SemaphoreWrapper m_TimelineSemaphore;

    struct SyncPointData
    {
        const Uint64   Value;
        SyncPointVkPtr SyncPoint;
    };

    static constexpr Uint32   RequiredArraySize = 8;
    std::mutex                m_SyncPointsGuard; // Protects access to the m_SyncPoints
    std::deque<SyncPointData> m_SyncPoints;

#ifdef DILIGENT_DEVELOPMENT
    size_t m_MaxSyncPoints = 0;
#endif
};

} // namespace Diligent
