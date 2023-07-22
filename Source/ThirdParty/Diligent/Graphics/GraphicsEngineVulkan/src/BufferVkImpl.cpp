/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
#include "BufferVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "BufferViewVkImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

BufferVkImpl::BufferVkImpl(IReferenceCounters*        pRefCounters,
                           FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                           RenderDeviceVkImpl*        pRenderDeviceVk,
                           const BufferDesc&          BuffDesc,
                           const BufferData*          pBuffData /*= nullptr*/) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceVk,
        BuffDesc,
        false
    },
    m_DynamicData(STD_ALLOCATOR_RAW_MEM(CtxDynamicData, GetRawAllocator(), "Allocator for vector<VulkanDynamicAllocation>"))
// clang-format on
{
    ValidateBufferInitData(m_Desc, pBuffData);

    const auto& LogicalDevice  = pRenderDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice = pRenderDeviceVk->GetPhysicalDevice();
    const auto& DeviceLimits   = PhysicalDevice.GetProperties().limits;
    m_DynamicOffsetAlignment   = std::max(Uint32{4}, static_cast<Uint32>(DeviceLimits.optimalBufferCopyOffsetAlignment));

    VkBufferCreateInfo VkBuffCI{};
    VkBuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VkBuffCI.pNext = nullptr;
    VkBuffCI.flags = 0; // VK_BUFFER_CREATE_SPARSE_BINDING_BIT, VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT, VK_BUFFER_CREATE_SPARSE_ALIASED_BIT
    VkBuffCI.size  = m_Desc.Size;
    VkBuffCI.usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | // The buffer can be used as the source of a transfer command
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // The buffer can be used as the destination of a transfer command

    static_assert(BIND_FLAG_LAST == 0x800, "Please update this function to handle the new bind flags");

    for (auto BindFlags = m_Desc.BindFlags; BindFlags != 0;)
    {
        auto BindFlag = ExtractLSB(BindFlags);
        switch (BindFlag)
        {
            case BIND_SHADER_RESOURCE:
            {
                if (m_Desc.Mode == BUFFER_MODE_FORMATTED)
                {
                    // Formatted buffers are mapped to uniform texel buffers in Vulkan.
                    VkBuffCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
                    m_DynamicOffsetAlignment = std::max(m_DynamicOffsetAlignment, static_cast<Uint32>(DeviceLimits.minTexelBufferOffsetAlignment));
                }
                else
                {
                    // Structured and ByteAddress buffers are mapped to read-only storage buffers in Vulkan.
                    VkBuffCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    m_DynamicOffsetAlignment = std::max(m_DynamicOffsetAlignment, static_cast<Uint32>(DeviceLimits.minStorageBufferOffsetAlignment));
                }

                break;
            }
            case BIND_UNORDERED_ACCESS:
            {
                if (m_Desc.Mode == BUFFER_MODE_FORMATTED)
                {
                    // RW formatted buffers are mapped to storage texel buffers in Vulkan.
                    VkBuffCI.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
                    m_DynamicOffsetAlignment = std::max(m_DynamicOffsetAlignment, static_cast<Uint32>(DeviceLimits.minTexelBufferOffsetAlignment));
                }
                else
                {
                    // RWStructured and RWByteAddress buffers are mapped to storage buffers in Vulkan.
                    VkBuffCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    // Each element of pDynamicOffsets of vkCmdBindDescriptorSets function which corresponds to a descriptor
                    // binding with type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC must be a multiple of
                    // VkPhysicalDeviceLimits::minStorageBufferOffsetAlignment (13.2.5)
                    m_DynamicOffsetAlignment = std::max(m_DynamicOffsetAlignment, static_cast<Uint32>(DeviceLimits.minStorageBufferOffsetAlignment));
                }

                break;
            }
            case BIND_VERTEX_BUFFER:
            {
                VkBuffCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                break;
            }
            case BIND_INDEX_BUFFER:
            {
                VkBuffCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                break;
            }
            case BIND_INDIRECT_DRAW_ARGS:
            {
                VkBuffCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                break;
            }
            case BIND_UNIFORM_BUFFER:
            {
                VkBuffCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

                // Each element of pDynamicOffsets parameter of vkCmdBindDescriptorSets function which corresponds to a descriptor
                // binding with type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC must be a multiple of
                // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment (13.2.5)
                m_DynamicOffsetAlignment = std::max(m_DynamicOffsetAlignment, static_cast<Uint32>(DeviceLimits.minUniformBufferOffsetAlignment));
                break;
            }
            case BIND_RAY_TRACING:
            {
                VkBuffCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // for scratch buffer
                VkBuffCI.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                VkBuffCI.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR; // acceleration structure build inputs such as vertex, index, transform, aabb, and instance data
                VkBuffCI.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
                break;
            }
            default:
                UNEXPECTED("unsupported buffer binding type");
                break;
        }
    }

    if (m_Desc.Usage == USAGE_DYNAMIC)
    {
        auto CtxCount = pRenderDeviceVk->GetNumImmediateContexts() + pRenderDeviceVk->GetNumDeferredContexts();
        m_DynamicData.resize(CtxCount);
    }

    VkBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE; // Sharing mode of the buffer when it is accessed by multiple queue families.
    VkBuffCI.queueFamilyIndexCount = 0;                         // The number of entries in the pQueueFamilyIndices array.
    VkBuffCI.pQueueFamilyIndices   = nullptr;                   // The list of queue families that will access this buffer
                                                                // (ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT).

    const auto QueueFamilyIndices = PlatformMisc::CountOneBits(m_Desc.ImmediateContextMask) > 1 ?
        GetDevice()->ConvertCmdQueueIdsToQueueFamilies(m_Desc.ImmediateContextMask) :
        std::vector<uint32_t>{};
    if (QueueFamilyIndices.size() > 1)
    {
        // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
        VkBuffCI.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        VkBuffCI.pQueueFamilyIndices   = QueueFamilyIndices.data();
        VkBuffCI.queueFamilyIndexCount = static_cast<uint32_t>(QueueFamilyIndices.size());
    }

    constexpr VkBufferUsageFlags UsageThatRequiresBackingBuffer =
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    const bool RequiresBackingBuffer =
        (VkBuffCI.usage & UsageThatRequiresBackingBuffer) != 0 ||
        // We only need a backing buffer for the storage buffer if there is an unordered access bind flag (aka RW structured buffers).
        // Read-only storage buffers (aka structured buffers) don't need a backing buffer.
        ((VkBuffCI.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0 && (m_Desc.BindFlags & BIND_UNORDERED_ACCESS) != 0);

    if (m_Desc.Usage == USAGE_SPARSE)
    {
        VkBuffCI.flags =
            VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
            VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT |
            (m_Desc.MiscFlags & MISC_BUFFER_FLAG_SPARSE_ALIASING ? VK_BUFFER_CREATE_SPARSE_ALIASED_BIT : 0);

        m_VulkanBuffer = LogicalDevice.CreateBuffer(VkBuffCI, m_Desc.Name);

        SetState(RESOURCE_STATE_UNDEFINED);
    }
    else if (m_Desc.Usage == USAGE_DYNAMIC && !RequiresBackingBuffer)
    {
        VERIFY(VkBuffCI.sharingMode == VK_SHARING_MODE_EXCLUSIVE,
               "Sharing mode is not supported for dynamic buffers, must be handled by ValidateBufferDesc()");

        // Dynamic constant/vertex/index/structured buffers are suballocated in the upload heap when Map() is called.
        // Dynamic formatted buffers or writable buffers need to be allocated in GPU-local memory.
        constexpr RESOURCE_STATE State = static_cast<RESOURCE_STATE>(
            RESOURCE_STATE_VERTEX_BUFFER |
            RESOURCE_STATE_INDEX_BUFFER |
            RESOURCE_STATE_CONSTANT_BUFFER |
            RESOURCE_STATE_SHADER_RESOURCE |
            RESOURCE_STATE_COPY_SOURCE |
            RESOURCE_STATE_INDIRECT_ARGUMENT);
        SetState(State);

#ifdef DILIGENT_DEBUG
        {
            constexpr VkAccessFlags AccessFlags =
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_TRANSFER_READ_BIT;
            VERIFY_EXPR(ResourceStateFlagsToVkAccessFlags(State) == AccessFlags);
        }
#endif
        // Dynamic buffer memory is always host-coherent
        m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
    }
    else
    {
        VERIFY(m_Desc.Usage != USAGE_DYNAMIC || PlatformMisc::CountOneBits(m_Desc.ImmediateContextMask) <= 1,
               "ImmediateContextMask must contain single set bit, this error should've been handled in ValidateBufferDesc()");

        m_VulkanBuffer = LogicalDevice.CreateBuffer(VkBuffCI, m_Desc.Name);

        VkMemoryRequirements MemReqs = LogicalDevice.GetBufferMemoryRequirements(m_VulkanBuffer);

        static constexpr auto InvalidMemoryTypeIndex = VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex;

        uint32_t MemoryTypeIndex = InvalidMemoryTypeIndex;
        {
            VkMemoryPropertyFlags vkMemoryFlags = 0;
            switch (m_Desc.Usage)
            {
                case USAGE_IMMUTABLE:
                case USAGE_DEFAULT:
                case USAGE_DYNAMIC: // Dynamic buffer with SRV or UAV bind flag
                    vkMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                    break;

                case USAGE_UNIFIED:
                case USAGE_STAGING:
                    vkMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

                    if (m_Desc.Usage == USAGE_UNIFIED)
                        vkMemoryFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

                    if ((m_Desc.CPUAccessFlags & CPU_ACCESS_READ) != 0)
                        vkMemoryFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

                    // Try to find coherent memory first
                    vkMemoryFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                    MemoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, vkMemoryFlags);
                    if (MemoryTypeIndex == InvalidMemoryTypeIndex)
                    {
                        // Use non-coherent memory
                        vkMemoryFlags &= ~VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                    }
                    break;

                default:
                    UNEXPECTED("Unexpected usage");
            }
            if (MemoryTypeIndex == InvalidMemoryTypeIndex)
                MemoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, vkMemoryFlags);

            if (vkMemoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                m_MemoryProperties |= MEMORY_PROPERTY_HOST_COHERENT;
        }

        VkMemoryAllocateFlags AllocateFlags = 0;
        if (VkBuffCI.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            AllocateFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        if (MemoryTypeIndex == InvalidMemoryTypeIndex)
            LOG_ERROR_AND_THROW("Failed to find suitable memory type for buffer '", m_Desc.Name, '\'');

        VkDeviceSize RequiredAlignment = MemReqs.alignment;
        if ((m_Desc.BindFlags & BIND_RAY_TRACING) != 0)
        {
            // geometry.triangles.vertexData.deviceAddress must be aligned to the size in bytes of the smallest component of the format in vertexFormat (which is 4 bytes).
            // geometry.triangles.indexData.deviceAddress must be aligned to the size in bytes of the type in indexType (which is 4 bytes).
            // if geometry.triangles.transformData.deviceAddress is not 0, it must be aligned to 16 bytes.
            // geometry.aabbs.data.deviceAddress must be aligned to 8 bytes.
            const VkDeviceSize ReadOnlyRTBufferAlign = 16u;
            const VkDeviceSize ScratchBufferAlign    = PhysicalDevice.GetExtProperties().AccelStruct.minAccelerationStructureScratchOffsetAlignment;
            RequiredAlignment                        = std::max(RequiredAlignment, std::max(ScratchBufferAlign, ReadOnlyRTBufferAlign));
            VERIFY_EXPR(RequiredAlignment % MemReqs.alignment == 0);
        }

        const bool AlignToNonCoherentAtomSize = (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) != 0 && (m_MemoryProperties & MEMORY_PROPERTY_HOST_COHERENT) == 0;
        if (AlignToNonCoherentAtomSize)
        {
            // From specs:
            //  If the device memory was allocated without the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT set,
            //  these guarantees must be made for an extended range: the application must round down the start
            //  of the range to the nearest multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize,
            //  and round the end of the range up to the nearest multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize.
            RequiredAlignment = std::max(RequiredAlignment, DeviceLimits.nonCoherentAtomSize);
            MemReqs.size      = AlignUp(MemReqs.size, DeviceLimits.nonCoherentAtomSize);
        }

        VERIFY(IsPowerOfTwo(RequiredAlignment), "Alignment is not power of 2!");
        m_MemoryAllocation = pRenderDeviceVk->AllocateMemory(MemReqs.size, RequiredAlignment, MemoryTypeIndex, AllocateFlags);
        if (!m_MemoryAllocation)
            LOG_ERROR_AND_THROW("Failed to allocate memory for buffer '", m_Desc.Name, "'.");

        m_BufferMemoryAlignedOffset = AlignUp(VkDeviceSize{m_MemoryAllocation.UnalignedOffset}, RequiredAlignment);
        VERIFY(m_MemoryAllocation.Size >= MemReqs.size + (m_BufferMemoryAlignedOffset - m_MemoryAllocation.UnalignedOffset), "Size of memory allocation is too small");
        auto Memory = m_MemoryAllocation.Page->GetVkMemory();
        auto err    = LogicalDevice.BindBufferMemory(m_VulkanBuffer, Memory, m_BufferMemoryAlignedOffset);
        CHECK_VK_ERROR_AND_THROW(err, "Failed to bind buffer memory");

        VERIFY(!AlignToNonCoherentAtomSize || (m_BufferMemoryAlignedOffset + MemReqs.size) % DeviceLimits.nonCoherentAtomSize == 0, "End offset is not properly aligned");

#ifdef DILIGENT_DEBUG
        if ((m_Desc.BindFlags & BIND_RAY_TRACING) != 0)
        {
            const VkDeviceSize ReadOnlyRTBufferAlign = 16u;
            const VkDeviceSize ScratchBufferAlign    = PhysicalDevice.GetExtProperties().AccelStruct.minAccelerationStructureScratchOffsetAlignment;
            const VkDeviceSize MinRTBufferAlign      = std::max(ScratchBufferAlign, ReadOnlyRTBufferAlign);
            const auto         DeviceAddress         = GetVkDeviceAddress();
            VERIFY(DeviceAddress % MinRTBufferAlign == 0, "Address is not properly aligned for ray tracing usage");
        }
#endif

        const auto InitialDataSize = (pBuffData != nullptr && pBuffData->pData != nullptr) ?
            std::min(pBuffData->DataSize, VkBuffCI.size) :
            0;

        RESOURCE_STATE InitialState = RESOURCE_STATE_UNDEFINED;
        if (InitialDataSize > 0)
        {
            const auto& MemoryProps = PhysicalDevice.GetMemoryProperties();
            VERIFY_EXPR(MemoryTypeIndex < MemoryProps.memoryTypeCount);
            const auto MemoryPropFlags = MemoryProps.memoryTypes[MemoryTypeIndex].propertyFlags;
            if ((MemoryPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
            {
                // Memory is directly accessible by CPU
                auto* pData = reinterpret_cast<uint8_t*>(m_MemoryAllocation.Page->GetCPUMemory());
                VERIFY_EXPR(pData != nullptr);
                memcpy(pData + m_BufferMemoryAlignedOffset, pBuffData->pData, StaticCast<size_t>(InitialDataSize));

                if ((MemoryPropFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                {
                    // Explicit flush is required
                    FlushMappedRange(0, m_Desc.Size);
                }
            }
            else
            {
                VkBufferCreateInfo VkStaginBuffCI = VkBuffCI;
                VkStaginBuffCI.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                std::string StagingBufferName = "Upload buffer for '";
                StagingBufferName += m_Desc.Name;
                StagingBufferName += '\'';
                VulkanUtilities::BufferWrapper StagingBuffer = LogicalDevice.CreateBuffer(VkStaginBuffCI, StagingBufferName.c_str());

                VkMemoryRequirements StagingBufferMemReqs = LogicalDevice.GetBufferMemoryRequirements(StagingBuffer);
                VERIFY(IsPowerOfTwo(StagingBufferMemReqs.alignment), "Alignment is not power of 2!");

                // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
                // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
                // to the host (10.2)
                auto StagingMemoryAllocation = pRenderDeviceVk->AllocateMemory(StagingBufferMemReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (!StagingMemoryAllocation)
                    LOG_ERROR_AND_THROW("Failed to allocate staging memory for buffer '", m_Desc.Name, "'.");

                auto StagingBufferMemory     = StagingMemoryAllocation.Page->GetVkMemory();
                auto AlignedStagingMemOffset = AlignUp(VkDeviceSize{StagingMemoryAllocation.UnalignedOffset}, StagingBufferMemReqs.alignment);
                VERIFY_EXPR(StagingMemoryAllocation.Size >= StagingBufferMemReqs.size + (AlignedStagingMemOffset - StagingMemoryAllocation.UnalignedOffset));

                auto* StagingData = reinterpret_cast<uint8_t*>(StagingMemoryAllocation.Page->GetCPUMemory());
                if (StagingData == nullptr)
                    LOG_ERROR_AND_THROW("Failed to allocate staging data for buffer '", m_Desc.Name, '\'');
                memcpy(StagingData + AlignedStagingMemOffset, pBuffData->pData, StaticCast<size_t>(InitialDataSize));

                err = LogicalDevice.BindBufferMemory(StagingBuffer, StagingBufferMemory, AlignedStagingMemOffset);
                CHECK_VK_ERROR_AND_THROW(err, "Failed to bind staging buffer memory");

                const auto CmdQueueInd = pBuffData->pContext ?
                    ClassPtrCast<DeviceContextVkImpl>(pBuffData->pContext)->GetCommandQueueId() :
                    SoftwareQueueIndex{PlatformMisc::GetLSB(m_Desc.ImmediateContextMask)};

                VulkanUtilities::CommandPoolWrapper  CmdPool;
                VulkanUtilities::VulkanCommandBuffer CmdBuffer;
                pRenderDeviceVk->AllocateTransientCmdPool(CmdQueueInd, CmdPool, CmdBuffer, "Transient command pool to copy staging data to a device buffer");

                CmdBuffer.MemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                InitialState              = RESOURCE_STATE_COPY_DEST;
                VkAccessFlags AccessFlags = ResourceStateFlagsToVkAccessFlags(InitialState);
                VERIFY_EXPR(AccessFlags == VK_ACCESS_TRANSFER_WRITE_BIT);
                CmdBuffer.MemoryBarrier(0, AccessFlags, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                // Copy commands MUST be recorded outside of a render pass instance. This is OK here
                // as copy will be the only command in the cmd buffer
                VkBufferCopy BuffCopy{};
                BuffCopy.srcOffset = 0;
                BuffCopy.dstOffset = 0;
                BuffCopy.size      = VkBuffCI.size;
                CmdBuffer.CopyBuffer(StagingBuffer, m_VulkanBuffer, 1, &BuffCopy);

                pRenderDeviceVk->ExecuteAndDisposeTransientCmdBuff(CmdQueueInd, CmdBuffer.GetVkCmdBuffer(), std::move(CmdPool));


                // After command buffer is submitted, safe-release staging resources. This strategy
                // is little overconservative as the resources will only be released after the
                // first command buffer submitted through the immediate context is complete

                // Next Cmd Buff| Next Fence |               This Thread                      |           Immediate Context
                //              |            |                                                |
                //      N       |     F      |                                                |
                //              |            |                                                |
                //              |            |  ExecuteAndDisposeTransientCmdBuff(vkCmdBuff)  |
                //              |            |  - SubmittedCmdBuffNumber = N                  |
                //              |            |  - SubmittedFenceValue = F                     |
                //     N+1 -  - | -  F+1  -  |                                                |
                //              |            |  Release(StagingBuffer)                        |
                //              |            |  - {N+1, StagingBuffer} -> Stale Objects       |
                //              |            |                                                |
                //              |            |                                                |
                //              |            |                                                | ExecuteCommandBuffer()
                //              |            |                                                | - SubmittedCmdBuffNumber = N+1
                //              |            |                                                | - SubmittedFenceValue = F+1
                //     N+2 -  - | -  F+2  -  |  -   -   -   -   -   -   -   -   -   -   -   - |
                //              |            |                                                | - DiscardStaleVkObjects(N+1, F+1)
                //              |            |                                                |   - {F+1, StagingBuffer} -> Release Queue
                //              |            |                                                |

                pRenderDeviceVk->SafeReleaseDeviceObject(std::move(StagingBuffer), Uint64{1} << Uint64{CmdQueueInd});
                pRenderDeviceVk->SafeReleaseDeviceObject(std::move(StagingMemoryAllocation), Uint64{1} << Uint64{CmdQueueInd});
            }
        }

        SetState(InitialState);
    }

    VERIFY_EXPR(IsInKnownState());
}


BufferVkImpl::BufferVkImpl(IReferenceCounters*        pRefCounters,
                           FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                           RenderDeviceVkImpl*        pRenderDeviceVk,
                           const BufferDesc&          BuffDesc,
                           RESOURCE_STATE             InitialState,
                           VkBuffer                   vkBuffer) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceVk,
        BuffDesc,
        false
    },
    m_DynamicData(STD_ALLOCATOR_RAW_MEM(CtxDynamicData, GetRawAllocator(), "Allocator for vector<VulkanDynamicAllocation>")),
    m_VulkanBuffer{vkBuffer}
// clang-format on
{
    SetState(InitialState);
}

BufferVkImpl::~BufferVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    if (m_VulkanBuffer != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanBuffer), m_Desc.ImmediateContextMask);
    if (m_MemoryAllocation.Page != nullptr)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_MemoryAllocation), m_Desc.ImmediateContextMask);
}

void BufferVkImpl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto& BuffViewAllocator = m_pDevice->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buff view allocator does not match allocator provided at buffer initialization");

        BufferViewDesc ViewDesc = OrigViewDesc;
        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS || ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
        {
            auto View = CreateView(ViewDesc);
            *ppView   = NEW_RC_OBJ(BuffViewAllocator, "BufferViewVkImpl instance", BufferViewVkImpl, bIsDefaultView ? this : nullptr)(GetDevice(), ViewDesc, this, std::move(View), bIsDefaultView);
        }

        if (!bIsDefaultView && *ppView)
            (*ppView)->AddRef();
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetBufferViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", OrigViewDesc.Name ? OrigViewDesc.Name : "", "\" (", ViewTypeName, ") for buffer \"", m_Desc.Name, "\"");
    }
}


VulkanUtilities::BufferViewWrapper BufferVkImpl::CreateView(struct BufferViewDesc& ViewDesc)
{
    VulkanUtilities::BufferViewWrapper BuffView;
    ValidateAndCorrectBufferViewDesc(m_Desc, ViewDesc, GetDevice()->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment);
    if (m_Desc.Mode == BUFFER_MODE_FORMATTED)
    {
        VkBufferViewCreateInfo ViewCI{};
        ViewCI.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        ViewCI.pNext  = nullptr;
        ViewCI.flags  = 0; // reserved for future use
        ViewCI.buffer = m_VulkanBuffer;
        DEV_CHECK_ERR(ViewDesc.Format.ValueType != VT_UNDEFINED, "Undefined format");
        ViewCI.format = TypeToVkFormat(ViewDesc.Format.ValueType, ViewDesc.Format.NumComponents, ViewDesc.Format.IsNormalized);
        ViewCI.offset = ViewDesc.ByteOffset; // offset in bytes from the base address of the buffer
        ViewCI.range  = ViewDesc.ByteWidth;  // size in bytes of the buffer view

        const auto& LogicalDevice = GetDevice()->GetLogicalDevice();
        BuffView                  = LogicalDevice.CreateBufferView(ViewCI, ViewDesc.Name);
    }
    else if (m_Desc.Mode == BUFFER_MODE_STRUCTURED ||
             m_Desc.Mode == BUFFER_MODE_RAW)
    {
        // Structured and raw buffers are mapped to storage buffers in GLSL
    }

    return BuffView;
}

VkBuffer BufferVkImpl::GetVkBuffer() const
{
    if (m_VulkanBuffer != VK_NULL_HANDLE)
        return m_VulkanBuffer;
    else
    {
        VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer expected");
        return m_pDevice->GetDynamicMemoryManager().GetVkBuffer();
    }
}

void BufferVkImpl::SetAccessFlags(VkAccessFlags AccessFlags)
{
    SetState(VkAccessFlagsToResourceStates(AccessFlags));
}

VkAccessFlags BufferVkImpl::GetAccessFlags() const
{
    return ResourceStateFlagsToVkAccessFlags(GetState());
}

VkDeviceAddress BufferVkImpl::GetVkDeviceAddress() const
{
    constexpr auto DeviceAddressFlags = BIND_RAY_TRACING;

    if (m_VulkanBuffer != VK_NULL_HANDLE && (m_Desc.BindFlags & DeviceAddressFlags) != 0)
    {
#if DILIGENT_USE_VOLK
        VkBufferDeviceAddressInfoKHR BufferInfo = {};

        BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
        BufferInfo.buffer      = m_VulkanBuffer;
        VkDeviceAddress Result = vkGetBufferDeviceAddressKHR(m_pDevice->GetLogicalDevice().GetVkDevice(), &BufferInfo);
        VERIFY_EXPR(Result > 0);
        return Result;
#else
        UNSUPPORTED("vkGetBufferDeviceAddressKHR is only available through Volk");
        return VkDeviceAddress{};
#endif
    }
    else
    {
        UNEXPECTED("Can't get device address for buffer");
        return 0;
    }
}

void BufferVkImpl::FlushMappedRange(Uint64 StartOffset, Uint64 Size)
{
    DvpVerifyFlushMappedRangeArguments(StartOffset, Size);

    const auto& LogicalDevice = GetDevice()->GetLogicalDevice();
    const auto& DeviceLimits  = GetDevice()->GetPhysicalDevice().GetProperties().limits;

    VkMappedMemoryRange MappedRange{};
    MappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    MappedRange.memory = m_MemoryAllocation.Page->GetVkMemory();
    MappedRange.offset = AlignDown(m_BufferMemoryAlignedOffset + StartOffset, DeviceLimits.nonCoherentAtomSize);
    MappedRange.size   = AlignUp(m_BufferMemoryAlignedOffset + StartOffset + Size, DeviceLimits.nonCoherentAtomSize) - MappedRange.offset;
    LogicalDevice.FlushMappedMemoryRanges(1, &MappedRange);
}

void BufferVkImpl::InvalidateMappedRange(Uint64 StartOffset, Uint64 Size)
{
    DvpVerifyInvalidateMappedRangeArguments(StartOffset, Size);

    const auto& LogicalDevice = GetDevice()->GetLogicalDevice();
    const auto& DeviceLimits  = GetDevice()->GetPhysicalDevice().GetProperties().limits;

    VkMappedMemoryRange MappedRange{};
    MappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    MappedRange.memory = m_MemoryAllocation.Page->GetVkMemory();
    MappedRange.offset = AlignDown(m_BufferMemoryAlignedOffset + StartOffset, DeviceLimits.nonCoherentAtomSize);
    MappedRange.size   = AlignUp(m_BufferMemoryAlignedOffset + StartOffset + Size, DeviceLimits.nonCoherentAtomSize) - MappedRange.offset;
    LogicalDevice.InvalidateMappedMemoryRanges(1, &MappedRange);
}


#ifdef DILIGENT_DEVELOPMENT
void BufferVkImpl::DvpVerifyDynamicAllocation(const DeviceContextVkImpl* pCtx) const
{
    if (m_VulkanBuffer == VK_NULL_HANDLE)
    {
        VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");

        const auto  ContextId    = pCtx->GetContextId();
        const auto& DynAlloc     = m_DynamicData[ContextId];
        const auto  CurrentFrame = pCtx->GetFrameNumber();
        DEV_CHECK_ERR(DynAlloc.pDynamicMemMgr != nullptr, "Dynamic buffer '", m_Desc.Name, "' has not been mapped before its first use. Context Id: ", ContextId, ". Note: memory for dynamic buffers is allocated when a buffer is mapped.");
        DEV_CHECK_ERR(DynAlloc.dvpFrameNumber == CurrentFrame, "Dynamic allocation of dynamic buffer '", m_Desc.Name, "' in frame ", CurrentFrame, " is out-of-date. Note: contents of all dynamic resources is discarded at the end of every frame. A buffer must be mapped before its first use in any frame.");
    }
}
#endif

SparseBufferProperties BufferVkImpl::GetSparseProperties() const
{
    DEV_CHECK_ERR(m_Desc.Usage == USAGE_SPARSE,
                  "IBuffer::GetSparseProperties() must be used for sparse buffer");

    auto MemReq = m_pDevice->GetLogicalDevice().GetBufferMemoryRequirements(GetVkBuffer());

    SparseBufferProperties Props{};
    Props.AddressSpaceSize = MemReq.size;
    Props.BlockSize        = StaticCast<Uint32>(MemReq.alignment);
    return Props;
}

} // namespace Diligent
