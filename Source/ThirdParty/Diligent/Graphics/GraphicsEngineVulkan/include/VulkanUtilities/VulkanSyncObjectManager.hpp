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

#include <vector>
#include <mutex>

#include "VulkanLogicalDevice.hpp"
#include "DebugUtilities.hpp"

namespace VulkanUtilities
{

// In 32-bit version, all vulkan handles are the same type
struct VkSemaphoreType
{
    using Type = VkSemaphore;
    Type Value = VK_NULL_HANDLE;
};

struct VkFenceType
{
    using Type    = VkFence;
    VkFence Value = VK_NULL_HANDLE;
};


class VulkanSyncObjectManager : public std::enable_shared_from_this<VulkanSyncObjectManager>
{
public:
    template <typename T>
    class RecycledSyncObject;

public:
    explicit VulkanSyncObjectManager(VulkanLogicalDevice& LogicalDevice);
    ~VulkanSyncObjectManager();

    void CreateSemaphores(RecycledSyncObject<VkSemaphoreType>* pSemaphores, uint32_t Count);

    RecycledSyncObject<VkFenceType> CreateFence();

    void Recycle(VkSemaphoreType Semaphore, bool IsUnsignaled);
    void Recycle(VkFenceType Fence, bool IsUnsignaled);

private:
    VulkanLogicalDevice& m_LogicalDevice;

    std::mutex               m_SemaphorePoolGuard;
    std::vector<VkSemaphore> m_SemaphorePool;

    std::mutex           m_FencePoolGuard;
    std::vector<VkFence> m_FencePool;
};

using VulkanRecycledSemaphore = VulkanSyncObjectManager::RecycledSyncObject<VkSemaphoreType>;
using VulkanRecycledFence     = VulkanSyncObjectManager::RecycledSyncObject<VkFenceType>;


template <typename VkSyncObjType>
class VulkanSyncObjectManager::RecycledSyncObject
{
public:
    using NativeType = typename VkSyncObjType::Type;

    RecycledSyncObject()
    {}

    RecycledSyncObject(std::shared_ptr<VulkanSyncObjectManager> pManager, NativeType SyncObj) :
        m_pManager{pManager},
        m_VkSyncObject{SyncObj}
    {}

    RecycledSyncObject(const RecycledSyncObject&) = delete;
    RecycledSyncObject& operator=(const RecycledSyncObject&) = delete;

    RecycledSyncObject(RecycledSyncObject&& rhs) noexcept :
        m_pManager{std::move(rhs.m_pManager)},
        m_VkSyncObject{rhs.m_VkSyncObject},
        m_IsUnsignaled{rhs.m_IsUnsignaled}
    {
        rhs.m_VkSyncObject = VK_NULL_HANDLE;
    }

    RecycledSyncObject& operator=(RecycledSyncObject&& rhs) noexcept
    {
        Release();
        m_pManager         = std::move(rhs.m_pManager);
        m_VkSyncObject     = rhs.m_VkSyncObject;
        m_IsUnsignaled     = rhs.m_IsUnsignaled;
        rhs.m_VkSyncObject = VK_NULL_HANDLE;
        return *this;
    }

    explicit operator bool() const
    {
        return m_VkSyncObject != VK_NULL_HANDLE;
    }

    operator NativeType() const
    {
        return m_VkSyncObject;
    }

    void Release()
    {
        if (m_pManager && m_VkSyncObject != VK_NULL_HANDLE)
        {
            m_pManager->Recycle(VkSyncObjType{m_VkSyncObject}, m_IsUnsignaled);
            m_VkSyncObject = VK_NULL_HANDLE;
            m_pManager.reset();
        }
    }

    // Semaphore is used for wait operation and will be unsignaled
    void SetUnsignaled()
    {
        VERIFY_EXPR(!m_IsUnsignaled);
        m_IsUnsignaled = true;
    }

    ~RecycledSyncObject()
    {
        Release();
    }

private:
    std::shared_ptr<VulkanSyncObjectManager> m_pManager;
    NativeType                               m_VkSyncObject = VK_NULL_HANDLE;
    bool                                     m_IsUnsignaled = false;
};

} // namespace VulkanUtilities
