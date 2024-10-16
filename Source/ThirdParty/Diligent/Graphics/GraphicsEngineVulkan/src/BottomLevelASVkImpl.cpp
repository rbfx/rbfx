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
#include "BottomLevelASVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanTypeConversions.hpp"

namespace Diligent
{

BottomLevelASVkImpl::BottomLevelASVkImpl(IReferenceCounters*      pRefCounters,
                                         RenderDeviceVkImpl*      pRenderDeviceVk,
                                         const BottomLevelASDesc& Desc) :
    TBottomLevelASBase{pRefCounters, pRenderDeviceVk, Desc}
{
    const auto& LogicalDevice   = pRenderDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice  = pRenderDeviceVk->GetPhysicalDevice();
    const auto& RTProps         = pRenderDeviceVk->GetAdapterInfo().RayTracing;
    auto        AccelStructSize = m_Desc.CompactedSize;

    if (AccelStructSize == 0)
    {
        VkAccelerationStructureBuildGeometryInfoKHR     vkBuildInfo{};
        std::vector<VkAccelerationStructureGeometryKHR> vkGeometries;
        std::vector<uint32_t>                           MaxPrimitiveCounts;
        VkAccelerationStructureBuildSizesInfoKHR        vkSizeInfo{};
        vkSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGeometries.resize(size_t{Desc.TriangleCount} + size_t{Desc.BoxCount});
        MaxPrimitiveCounts.resize(vkGeometries.size());

        if (m_Desc.pTriangles != nullptr)
        {
            Uint32 MaxPrimitiveCount = 0;
            for (uint32_t i = 0; i < m_Desc.TriangleCount; ++i)
            {
                auto& src = m_Desc.pTriangles[i];
                auto& dst = vkGeometries[i];
                auto& tri = dst.geometry.triangles;

                dst.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                dst.pNext        = nullptr;
                dst.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                dst.flags        = 0;

                tri.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                tri.pNext        = nullptr;
                tri.vertexFormat = TypeToVkFormat(src.VertexValueType, src.VertexComponentCount, src.VertexValueType < VT_FLOAT16);
                // maxVertex is the number of vertices in vertexData minus one.
                VERIFY(src.MaxVertexCount > 0, "MaxVertexCount must be greater than 0");
                tri.maxVertex = src.MaxVertexCount - 1;
                tri.indexType = TypeToVkIndexType(src.IndexType);
                // Non-null address indicates that non-null transform data will be provided to the build command.
                tri.transformData.deviceAddress = src.AllowsTransforms ? 1 : 0;
                MaxPrimitiveCounts[i]           = src.MaxPrimitiveCount;

                MaxPrimitiveCount += src.MaxPrimitiveCount;

#ifdef DILIGENT_DEVELOPMENT
#    if DILIGENT_USE_VOLK
                {
                    VkFormatProperties2 vkProps{};
                    vkProps.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
                    vkGetPhysicalDeviceFormatProperties2KHR(PhysicalDevice.GetVkDeviceHandle(), tri.vertexFormat, &vkProps);

                    DEV_CHECK_ERR((vkProps.formatProperties.bufferFeatures & VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR) != 0,
                                  "combination of pTriangles[", i, "].VertexValueType (", GetValueTypeString(src.VertexValueType),
                                  ") and pTriangles[", i, "].VertexComponentCount (", src.VertexComponentCount, ") is not supported by this device.");
                }
#    else
                UNSUPPORTED("vkGetPhysicalDeviceFormatProperties2KHR is only available through Volk");
#    endif
#endif
            }
            DEV_CHECK_ERR(MaxPrimitiveCount <= RTProps.MaxPrimitivesPerBLAS,
                          "Max primitives count (", MaxPrimitiveCount, ") exceeds device limit (", RTProps.MaxPrimitivesPerBLAS, ")");
            (void)MaxPrimitiveCount;
        }
        else if (m_Desc.pBoxes != nullptr)
        {
            Uint32 MaxBoxCount = 0;
            for (uint32_t i = 0; i < m_Desc.BoxCount; ++i)
            {
                auto& src = m_Desc.pBoxes[i];
                auto& dst = vkGeometries[i];
                auto& box = dst.geometry.aabbs;

                dst.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                dst.pNext        = nullptr;
                dst.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;

                box.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
                box.pNext             = nullptr;
                MaxPrimitiveCounts[i] = src.MaxBoxCount;

                MaxBoxCount += src.MaxBoxCount;
            }
            DEV_CHECK_ERR(MaxBoxCount <= RTProps.MaxPrimitivesPerBLAS,
                          "Max box count (", MaxBoxCount, ") exceeds device limit (", RTProps.MaxPrimitivesPerBLAS, ")");
            (void)MaxBoxCount;
        }
        else
        {
            UNEXPECTED("Either pTriangles or pBoxes must not be null");
        }

        vkBuildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        vkBuildInfo.flags         = BuildASFlagsToVkBuildAccelerationStructureFlags(m_Desc.Flags);
        vkBuildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkBuildInfo.pGeometries   = vkGeometries.data();
        vkBuildInfo.geometryCount = static_cast<uint32_t>(vkGeometries.size());

        DEV_CHECK_ERR(vkBuildInfo.geometryCount <= RTProps.MaxGeometriesPerBLAS, "Geometry count (", vkBuildInfo.geometryCount,
                      ") exceeds device limit (", RTProps.MaxGeometriesPerBLAS, ").");

        LogicalDevice.GetAccelerationStructureBuildSizes(vkBuildInfo, MaxPrimitiveCounts.data(), vkSizeInfo);

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
        LOG_ERROR_AND_THROW("Failed to allocate memory for BLAS '", m_Desc.Name, "'.");

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
    vkAccelStrCI.type        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    m_VulkanBLAS = LogicalDevice.CreateAccelStruct(vkAccelStrCI, m_Desc.Name);

    m_DeviceAddress = LogicalDevice.GetAccelerationStructureDeviceAddress(m_VulkanBLAS);

    SetState(RESOURCE_STATE_BUILD_AS_READ);
}

BottomLevelASVkImpl::BottomLevelASVkImpl(IReferenceCounters*        pRefCounters,
                                         RenderDeviceVkImpl*        pRenderDeviceVk,
                                         const BottomLevelASDesc&   Desc,
                                         RESOURCE_STATE             InitialState,
                                         VkAccelerationStructureKHR vkBLAS) :
    TBottomLevelASBase{pRefCounters, pRenderDeviceVk, Desc},
    m_VulkanBLAS{vkBLAS}
{
    SetState(InitialState);
    m_DeviceAddress = pRenderDeviceVk->GetLogicalDevice().GetAccelerationStructureDeviceAddress(m_VulkanBLAS);
}

BottomLevelASVkImpl::~BottomLevelASVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    if (m_VulkanBLAS != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanBLAS), m_Desc.ImmediateContextMask);
    if (m_VulkanBuffer != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanBuffer), m_Desc.ImmediateContextMask);
    if (m_MemoryAllocation.Page != nullptr)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_MemoryAllocation), m_Desc.ImmediateContextMask);
}

} // namespace Diligent
