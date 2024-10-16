/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
#include "TopLevelASVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanTypeConversions.hpp"

namespace Diligent
{

TopLevelASVkImpl::TopLevelASVkImpl(IReferenceCounters*   pRefCounters,
                                   RenderDeviceVkImpl*   pRenderDeviceVk,
                                   const TopLevelASDesc& Desc) :
    TTopLevelASBase{pRefCounters, pRenderDeviceVk, Desc}
{
    const auto& LogicalDevice   = pRenderDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice  = pRenderDeviceVk->GetPhysicalDevice();
    const auto& RTProps         = pRenderDeviceVk->GetAdapterInfo().RayTracing;
    auto        AccelStructSize = m_Desc.CompactedSize;

    if (AccelStructSize == 0)
    {
        VkAccelerationStructureGeometryKHR vkGeometry{};
        vkGeometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        vkGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;

        VkAccelerationStructureGeometryInstancesDataKHR& vkInstances{vkGeometry.geometry.instances};
        vkInstances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        vkInstances.arrayOfPointers = VK_FALSE;

        VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo{};
        vkBuildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        vkBuildInfo.flags         = BuildASFlagsToVkBuildAccelerationStructureFlags(m_Desc.Flags);
        vkBuildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkBuildInfo.pGeometries   = &vkGeometry;
        vkBuildInfo.geometryCount = 1;

        DEV_CHECK_ERR(m_Desc.MaxInstanceCount <= RTProps.MaxInstancesPerTLAS,
                      "Max instance count (", m_Desc.MaxInstanceCount, ") exceeds device limit (", RTProps.MaxInstancesPerTLAS, ").");

        VkAccelerationStructureBuildSizesInfoKHR vkSizeInfo{};
        vkSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        const uint32_t MaxPrimitiveCount = m_Desc.MaxInstanceCount;
        LogicalDevice.GetAccelerationStructureBuildSizes(vkBuildInfo, &MaxPrimitiveCount, vkSizeInfo);

        AccelStructSize      = vkSizeInfo.accelerationStructureSize;
        m_ScratchSize.Build  = vkSizeInfo.buildScratchSize;
        m_ScratchSize.Update = vkSizeInfo.updateScratchSize;
    }

    VkBufferCreateInfo vkBuffCI{};
    vkBuffCI.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBuffCI.flags                 = 0;
    vkBuffCI.size                  = AccelStructSize;
    vkBuffCI.usage                 = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    vkBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    vkBuffCI.queueFamilyIndexCount = 0;
    vkBuffCI.pQueueFamilyIndices   = nullptr;

    m_VulkanBuffer = LogicalDevice.CreateBuffer(vkBuffCI, m_Desc.Name);

    VkMemoryRequirements MemReqs         = LogicalDevice.GetBufferMemoryRequirements(m_VulkanBuffer);
    uint32_t             MemoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VERIFY(IsPowerOfTwo(MemReqs.alignment), "Alignment is not power of 2!");
    m_MemoryAllocation = pRenderDeviceVk->AllocateMemory(MemReqs.size, MemReqs.alignment, MemoryTypeIndex, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    if (!m_MemoryAllocation)
        LOG_ERROR_AND_THROW("Failed to allocate memory for TLAS '", m_Desc.Name, "'.");

    m_MemoryAlignedOffset = AlignUp(VkDeviceSize{m_MemoryAllocation.UnalignedOffset}, MemReqs.alignment);
    VERIFY(m_MemoryAllocation.Size >= MemReqs.size + (m_MemoryAlignedOffset - m_MemoryAllocation.UnalignedOffset), "Size of memory allocation is too small");
    auto Memory = m_MemoryAllocation.Page->GetVkMemory();
    auto err    = LogicalDevice.BindBufferMemory(m_VulkanBuffer, Memory, m_MemoryAlignedOffset);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to bind buffer memory");

    VkAccelerationStructureCreateInfoKHR vkAccelStrCI{};
    vkAccelStrCI.sType       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    vkAccelStrCI.createFlags = 0;
    vkAccelStrCI.buffer      = m_VulkanBuffer;
    vkAccelStrCI.offset      = 0;
    vkAccelStrCI.size        = AccelStructSize;
    vkAccelStrCI.type        = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    m_VulkanTLAS = LogicalDevice.CreateAccelStruct(vkAccelStrCI, m_Desc.Name);

    m_DeviceAddress = LogicalDevice.GetAccelerationStructureDeviceAddress(m_VulkanTLAS);

    SetState(RESOURCE_STATE_BUILD_AS_READ);
}

TopLevelASVkImpl::TopLevelASVkImpl(IReferenceCounters*        pRefCounters,
                                   RenderDeviceVkImpl*        pRenderDeviceVk,
                                   const TopLevelASDesc&      Desc,
                                   RESOURCE_STATE             InitialState,
                                   VkAccelerationStructureKHR vkTLAS) :
    TTopLevelASBase{pRefCounters, pRenderDeviceVk, Desc},
    m_VulkanTLAS{vkTLAS}
{
    SetState(InitialState);
    m_DeviceAddress = pRenderDeviceVk->GetLogicalDevice().GetAccelerationStructureDeviceAddress(m_VulkanTLAS);
}

TopLevelASVkImpl::~TopLevelASVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    if (m_VulkanTLAS != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanTLAS), m_Desc.ImmediateContextMask);
    if (m_VulkanBuffer != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanBuffer), m_Desc.ImmediateContextMask);
    if (m_MemoryAllocation.Page != nullptr)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_MemoryAllocation), m_Desc.ImmediateContextMask);
}

} // namespace Diligent
