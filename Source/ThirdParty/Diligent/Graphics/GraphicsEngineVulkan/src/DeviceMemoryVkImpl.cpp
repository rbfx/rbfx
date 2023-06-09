/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
#include "DeviceMemoryVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "TextureVkImpl.hpp"
#include "BufferVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"

namespace Diligent
{

DeviceMemoryVkImpl::DeviceMemoryVkImpl(IReferenceCounters*           pRefCounters,
                                       RenderDeviceVkImpl*           pDeviceVk,
                                       const DeviceMemoryCreateInfo& MemCI) :
    TDeviceMemoryBase{pRefCounters, pDeviceVk, MemCI}
{
#define DEVMEM_CHECK_CREATE_INFO(...) \
    LOG_ERROR_AND_THROW("Device memory create info is not valid: ", __VA_ARGS__);

    const auto& PhysicalDevice = m_pDevice->GetPhysicalDevice();
    const auto& LogicalDevice  = m_pDevice->GetLogicalDevice();

    if (MemCI.NumResources == 0)
        DEVMEM_CHECK_CREATE_INFO("Vulkan requires at least one resource to choose memory type");

    if (MemCI.ppCompatibleResources == nullptr)
        DEVMEM_CHECK_CREATE_INFO("ppCompatibleResources must not be null");

    uint32_t MemoryTypeBits = ~0u;
    for (Uint32 i = 0; i < MemCI.NumResources; ++i)
    {
        auto* pResource = MemCI.ppCompatibleResources[i];

        if (RefCntAutoPtr<ITextureVk> pTexture{pResource, IID_TextureVk})
        {
            const auto* pTexVk = pTexture.ConstPtr<TextureVkImpl>();
            if (pTexVk->GetDesc().Usage != USAGE_SPARSE)
                DEVMEM_CHECK_CREATE_INFO("ppCompatibleResources[", i, "] must be created with USAGE_SPARSE");

            MemoryTypeBits &= LogicalDevice.GetImageMemoryRequirements(pTexVk->GetVkImage()).memoryTypeBits;
        }
        else if (RefCntAutoPtr<IBufferVk> pBuffer{pResource, IID_BufferVk})
        {
            const auto* pBuffVk = pBuffer.ConstPtr<BufferVkImpl>();
            if (pBuffVk->GetDesc().Usage != USAGE_SPARSE)
                DEVMEM_CHECK_CREATE_INFO("ppCompatibleResources[", i, "] must be created with USAGE_SPARSE");

            MemoryTypeBits &= LogicalDevice.GetBufferMemoryRequirements(pBuffVk->GetVkBuffer()).memoryTypeBits;
        }
        else
        {
            UNEXPECTED("unsupported resource type");
        }
    }

    if (MemoryTypeBits == 0)
        DEVMEM_CHECK_CREATE_INFO("ppCompatibleResources contains incompatible resources");

    static constexpr auto InvalidMemoryTypeIndex = VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex;

    uint32_t MemoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (MemoryTypeIndex == InvalidMemoryTypeIndex)
        DEVMEM_CHECK_CREATE_INFO("Failed to find memory type for resources in ppCompatibleResources");

    m_MemoryTypeIndex = MemoryTypeIndex;

    VkMemoryAllocateInfo MemAlloc{};
    MemAlloc.pNext           = nullptr;
    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = m_Desc.PageSize;
    MemAlloc.memoryTypeIndex = m_MemoryTypeIndex;

    const auto PageCount = StaticCast<size_t>(MemCI.InitialSize / m_Desc.PageSize);
    m_Pages.reserve(PageCount);

    for (size_t i = 0; i < PageCount; ++i)
        m_Pages.emplace_back(LogicalDevice.AllocateDeviceMemory(MemAlloc, m_Desc.Name)); // throw on error
}

DeviceMemoryVkImpl::~DeviceMemoryVkImpl()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_Pages), m_Desc.ImmediateContextMask);
}

IMPLEMENT_QUERY_INTERFACE(DeviceMemoryVkImpl, IID_DeviceMemoryVk, TDeviceMemoryBase)

Bool DeviceMemoryVkImpl::Resize(Uint64 NewSize)
{
    DvpVerifyResize(NewSize);

    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
    const auto  NewPageCount  = StaticCast<size_t>(NewSize / m_Desc.PageSize);

    VkMemoryAllocateInfo MemAlloc{};
    MemAlloc.pNext           = nullptr;
    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = m_Desc.PageSize;
    MemAlloc.memoryTypeIndex = m_MemoryTypeIndex;

    m_Pages.reserve(NewPageCount);

    while (m_Pages.size() < NewPageCount)
    {
        try
        {
            m_Pages.emplace_back(LogicalDevice.AllocateDeviceMemory(MemAlloc, m_Desc.Name)); // throw on error
        }
        catch (...)
        {
            return false;
        }
    }

    while (m_Pages.size() > NewPageCount)
    {
        m_pDevice->SafeReleaseDeviceObject(std::move(m_Pages.back()), m_Desc.ImmediateContextMask);
        m_Pages.pop_back();
    }

    return true;
}

Uint64 DeviceMemoryVkImpl::GetCapacity() const
{
    return m_Desc.PageSize * m_Pages.size();
}

Bool DeviceMemoryVkImpl::IsCompatible(IDeviceObject* pResource) const
{
    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();

    uint32_t memoryTypeBits = 0;
    if (RefCntAutoPtr<ITextureVk> pTexture{pResource, IID_TextureVk})
    {
        const auto* pTexVk = pTexture.ConstPtr<TextureVkImpl>();
        memoryTypeBits     = LogicalDevice.GetImageMemoryRequirements(pTexVk->GetVkImage()).memoryTypeBits;
    }
    else if (RefCntAutoPtr<IBufferVk> pBuffer{pResource, IID_BufferVk})
    {
        const auto* pBuffVk = pBuffer.ConstPtr<BufferVkImpl>();
        memoryTypeBits      = LogicalDevice.GetBufferMemoryRequirements(pBuffVk->GetVkBuffer()).memoryTypeBits;
    }
    else
    {
        UNEXPECTED("unsupported resource type");
    }
    return (memoryTypeBits & (1u << m_MemoryTypeIndex)) != 0;
}

DeviceMemoryRangeVk DeviceMemoryVkImpl::GetRange(Uint64 Offset, Uint64 Size) const
{
    const auto PageIdx = static_cast<size_t>(Offset / m_Desc.PageSize);

    DeviceMemoryRangeVk Range{};
    if (PageIdx >= m_Pages.size())
    {
        DEV_ERROR("DeviceMemoryVkImpl::GetRange(): Offset is out of allocated space bounds");
        return Range;
    }

    const auto OffsetInPage = Offset % m_Desc.PageSize;
    if (OffsetInPage + Size > m_Desc.PageSize)
    {
        DEV_ERROR("DeviceMemoryVkImpl::GetRange(): Offset and Size must be inside a single page");
        return Range;
    }

    Range.Offset = OffsetInPage;
    Range.Handle = m_Pages[PageIdx];
    Range.Size   = std::min(m_Desc.PageSize - OffsetInPage, Size);

    return Range;
}

} // namespace Diligent
