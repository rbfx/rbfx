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

#include "RenderDeviceVkImpl.hpp"

#include "PipelineStateVkImpl.hpp"
#include "ShaderVkImpl.hpp"
#include "TextureVkImpl.hpp"
#include "SamplerVkImpl.hpp"
#include "BufferVkImpl.hpp"
#include "ShaderResourceBindingVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "FenceVkImpl.hpp"
#include "QueryVkImpl.hpp"
#include "RenderPassVkImpl.hpp"
#include "FramebufferVkImpl.hpp"
#include "BottomLevelASVkImpl.hpp"
#include "TopLevelASVkImpl.hpp"
#include "ShaderBindingTableVkImpl.hpp"
#include "DeviceMemoryVkImpl.hpp"
#include "PipelineStateCacheVkImpl.hpp"
#include "CommandQueueVkImpl.hpp"
#include "PipelineResourceSignatureVkImpl.hpp"

#include "VulkanTypeConversions.hpp"
#include "EngineMemory.h"
#include "QueryManagerVk.hpp"

namespace Diligent
{

RenderDeviceVkImpl::RenderDeviceVkImpl(IReferenceCounters*                                    pRefCounters,
                                       IMemoryAllocator&                                      RawMemAllocator,
                                       IEngineFactory*                                        pEngineFactory,
                                       const EngineVkCreateInfo&                              EngineCI,
                                       const GraphicsAdapterInfo&                             AdapterInfo,
                                       size_t                                                 CommandQueueCount,
                                       ICommandQueueVk**                                      CmdQueues,
                                       std::shared_ptr<VulkanUtilities::VulkanInstance>       Instance,
                                       std::unique_ptr<VulkanUtilities::VulkanPhysicalDevice> PhysicalDevice,
                                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice>  LogicalDevice) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        CommandQueueCount,
        CmdQueues,
        EngineCI,
        AdapterInfo
    },
    m_VulkanInstance         {Instance                 },
    m_PhysicalDevice         {std::move(PhysicalDevice)},
    m_LogicalVkDevice        {std::move(LogicalDevice) },
    m_FramebufferCache       {*this                    },
    m_ImplicitRenderPassCache{*this                    },
    m_DescriptorSetAllocator
    {
        *this,
        "Main descriptor pool",
        std::vector<VkDescriptorPoolSize>
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                    EngineCI.MainDescriptorPoolSize.NumSeparateSamplerDescriptors},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     EngineCI.MainDescriptorPoolSize.NumCombinedSamplerDescriptors},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              EngineCI.MainDescriptorPoolSize.NumSampledImageDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              EngineCI.MainDescriptorPoolSize.NumStorageImageDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,       EngineCI.MainDescriptorPoolSize.NumUniformTexelBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,       EngineCI.MainDescriptorPoolSize.NumStorageTexelBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             EngineCI.MainDescriptorPoolSize.NumUniformBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             EngineCI.MainDescriptorPoolSize.NumStorageBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,     EngineCI.MainDescriptorPoolSize.NumUniformBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,     EngineCI.MainDescriptorPoolSize.NumStorageBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,           EngineCI.MainDescriptorPoolSize.NumInputAttachmentDescriptors},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, EngineCI.MainDescriptorPoolSize.NumAccelStructDescriptors}
        },
        EngineCI.MainDescriptorPoolSize.MaxDescriptorSets,
        true
    },
    m_DynamicDescriptorPool
    {
        *this,
        "Dynamic descriptor pool",
        std::vector<VkDescriptorPoolSize>
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                    EngineCI.DynamicDescriptorPoolSize.NumSeparateSamplerDescriptors},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     EngineCI.DynamicDescriptorPoolSize.NumCombinedSamplerDescriptors},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              EngineCI.DynamicDescriptorPoolSize.NumSampledImageDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              EngineCI.DynamicDescriptorPoolSize.NumStorageImageDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,       EngineCI.DynamicDescriptorPoolSize.NumUniformTexelBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,       EngineCI.DynamicDescriptorPoolSize.NumStorageTexelBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             EngineCI.DynamicDescriptorPoolSize.NumUniformBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             EngineCI.DynamicDescriptorPoolSize.NumStorageBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,     EngineCI.DynamicDescriptorPoolSize.NumUniformBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,     EngineCI.DynamicDescriptorPoolSize.NumStorageBufferDescriptors},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,           EngineCI.MainDescriptorPoolSize.NumInputAttachmentDescriptors},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, EngineCI.MainDescriptorPoolSize.NumAccelStructDescriptors}
        },
        EngineCI.DynamicDescriptorPoolSize.MaxDescriptorSets,
        false // Pools can only be reset
    },
    m_MemoryMgr
    {
        "Global resource memory manager",
        *m_LogicalVkDevice,
        *m_PhysicalDevice,
        GetRawAllocator(),
        EngineCI.DeviceLocalMemoryPageSize,
        EngineCI.HostVisibleMemoryPageSize,
        EngineCI.DeviceLocalMemoryReserveSize,
        EngineCI.HostVisibleMemoryReserveSize
    },
    m_DynamicMemoryManager
    {
        GetRawAllocator(),
        *this,
        EngineCI.DynamicHeapSize,
        ~Uint64{0}
    },
    m_pDxCompiler{CreateDXCompiler(DXCompilerTarget::Vulkan, m_PhysicalDevice->GetVkVersion(), EngineCI.pDxCompilerPath)}
// clang-format on
{
    static_assert(sizeof(VulkanDescriptorPoolSize) == sizeof(Uint32) * 11, "Please add new descriptors to m_DescriptorSetAllocator and m_DynamicDescriptorPool constructors");

    const auto vkVersion    = m_PhysicalDevice->GetVkVersion();
    m_DeviceInfo.Type       = RENDER_DEVICE_TYPE_VULKAN;
    m_DeviceInfo.APIVersion = Version{VK_API_VERSION_MAJOR(vkVersion), VK_API_VERSION_MINOR(vkVersion)};

    m_DeviceInfo.Features = VkFeaturesToDeviceFeatures(vkVersion,
                                                       m_LogicalVkDevice->GetEnabledFeatures(),
                                                       m_PhysicalDevice->GetProperties(),
                                                       m_LogicalVkDevice->GetEnabledExtFeatures(),
                                                       m_PhysicalDevice->GetExtProperties());

    m_DeviceInfo.MaxShaderVersion.HLSL   = {5, 1};
    m_DeviceInfo.MaxShaderVersion.GLSL   = {4, 6};
    m_DeviceInfo.MaxShaderVersion.GLESSL = {3, 2};

    // Note that Vulkan itself does not invert Y coordinate when transforming
    // normalized device Y to window space. However, we use negative viewport
    // height which achieves the same effect as in D3D, therefore we need to
    // invert y (see comments in DeviceContextVkImpl::CommitViewports() for details)
    m_DeviceInfo.NDC = NDCAttribs{0.0f, 1.0f, -0.5f};

    // Every queue family needs its own command pool.
    // Every queue needs its own query pool.
    m_QueryMgrs.reserve(CommandQueueCount);
    for (Uint32 q = 0; q < CommandQueueCount; ++q)
    {
        auto QueueFamilyIndex = HardwareQueueIndex{GetCommandQueue(SoftwareQueueIndex{q}).GetQueueFamilyIndex()};

        if (m_TransientCmdPoolMgrs.find(QueueFamilyIndex) == m_TransientCmdPoolMgrs.end())
        {
            m_TransientCmdPoolMgrs.emplace(
                QueueFamilyIndex,
                CommandPoolManager::CreateInfo{
                    //
                    GetLogicalDevice(),
                    "Transient command buffer pool manager",
                    QueueFamilyIndex,
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT //
                });
        }

        m_QueryMgrs.emplace_back(std::make_unique<QueryManagerVk>(this, EngineCI.QueryPoolSizes, SoftwareQueueIndex{q}));
    }

    for (Uint32 fmt = 1; fmt < m_TextureFormatsInfo.size(); ++fmt)
        m_TextureFormatsInfo[fmt].Supported = true; // We will test every format on a specific hardware device

    InitShaderCompilationThreadPool(EngineCI.pAsyncShaderCompilationThreadPool, EngineCI.NumAsyncShaderCompilationThreads);
}

RenderDeviceVkImpl::~RenderDeviceVkImpl()
{
    // Explicitly destroy dynamic heap. This will move resources owned by
    // the heap into release queues
    m_DynamicMemoryManager.Destroy();

    // Explicitly destroy render pass cache
    m_ImplicitRenderPassCache.Destroy();

    // Wait for the GPU to complete all its operations
    IdleGPU();

    ReleaseStaleResources(true);

    DEV_CHECK_ERR(m_DescriptorSetAllocator.GetAllocatedDescriptorSetCounter() == 0, "All allocated descriptor sets must have been released now.");
    DEV_CHECK_ERR(m_DynamicDescriptorPool.GetAllocatedPoolCounter() == 0, "All allocated dynamic descriptor pools must have been released now.");
    DEV_CHECK_ERR(m_DynamicMemoryManager.GetMasterBlockCounter() == 0, "All allocated dynamic master blocks must have been returned to the pool.");

    // Immediately destroys all command pools
    for (auto& CmdPool : m_TransientCmdPoolMgrs)
    {
        DEV_CHECK_ERR(CmdPool.second.GetAllocatedPoolCount() == 0, "All allocated transient command pools must have been released now. If there are outstanding references to the pools in release queues, the app will crash when CommandPoolManager::FreeCommandPool() is called.");
        CmdPool.second.DestroyPools();
    }

    // We must destroy command queues explicitly prior to releasing Vulkan device
    DestroyCommandQueues();

    //if(m_PhysicalDevice)
    //{
    //    // If m_PhysicalDevice is empty, the device does not own vulkan logical device and must not
    //    // destroy it
    //    vkDestroyDevice(m_VkDevice, m_VulkanInstance->GetVkAllocator());
    //}
}


void RenderDeviceVkImpl::AllocateTransientCmdPool(SoftwareQueueIndex                    CommandQueueId,
                                                  VulkanUtilities::CommandPoolWrapper&  CmdPool,
                                                  VulkanUtilities::VulkanCommandBuffer& CmdBuffer,
                                                  const Char*                           DebugPoolName)
{
    auto QueueFamilyIndex = HardwareQueueIndex{GetCommandQueue(CommandQueueId).GetQueueFamilyIndex()};
    auto CmdPoolMgrIter   = m_TransientCmdPoolMgrs.find(QueueFamilyIndex);
    VERIFY(CmdPoolMgrIter != m_TransientCmdPoolMgrs.end(),
           "Con not find transient command pool manager for queue family index (", Uint32{QueueFamilyIndex}, ")");

    CmdPool = CmdPoolMgrIter->second.AllocateCommandPool(DebugPoolName);

    // Allocate command buffer from the cmd pool
    VkCommandBufferAllocateInfo BuffAllocInfo{};
    BuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    BuffAllocInfo.pNext              = nullptr;
    BuffAllocInfo.commandPool        = CmdPool;
    BuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    BuffAllocInfo.commandBufferCount = 1;

    auto vkCmdBuff = m_LogicalVkDevice->AllocateVkCommandBuffer(BuffAllocInfo);
    DEV_CHECK_ERR(vkCmdBuff != VK_NULL_HANDLE, "Failed to allocate Vulkan command buffer");


    VkCommandBufferBeginInfo CmdBuffBeginInfo{};
    CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.pNext = nullptr;
    CmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Each recording of the command buffer will only be
                                                                          // submitted once, and the command buffer will be reset
                                                                          // and recorded again between each submission.
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;                          // Ignored for a primary command buffer

    auto err = vkBeginCommandBuffer(vkCmdBuff, &CmdBuffBeginInfo);
    DEV_CHECK_ERR(err == VK_SUCCESS, "vkBeginCommandBuffer() failed");
    (void)err;

    CmdBuffer.SetVkCmdBuffer(vkCmdBuff,
                             m_LogicalVkDevice->GetSupportedStagesMask(QueueFamilyIndex),
                             m_LogicalVkDevice->GetSupportedAccessMask(QueueFamilyIndex));
}


void RenderDeviceVkImpl::ExecuteAndDisposeTransientCmdBuff(SoftwareQueueIndex                    CommandQueueId,
                                                           VkCommandBuffer                       vkCmdBuff,
                                                           VulkanUtilities::CommandPoolWrapper&& CmdPool)
{
    VERIFY_EXPR(vkCmdBuff != VK_NULL_HANDLE);

    auto err = vkEndCommandBuffer(vkCmdBuff);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to end command buffer");
    (void)err;

    // We MUST NOT discard stale objects when executing transient command buffer,
    // otherwise a resource can be destroyed while still being used by the GPU:
    //
    //
    // Next Cmd Buff| Next Fence |        Immediate Context               |            This thread               |
    //              |            |                                        |                                      |
    //      N       |     F      |                                        |                                      |
    //              |            |  Draw(ResourceX)                       |                                      |
    //      N  -  - | -   -   -  |  Release(ResourceX)                    |                                      |
    //              |            |  - {N, ResourceX} -> Stale Objects     |                                      |
    //              |            |                                        |                                      |
    //              |            |                                        | SubmitCommandBuffer()                |
    //              |            |                                        | - SubmittedCmdBuffNumber = N         |
    //              |            |                                        | - SubmittedFenceValue = F            |
    //     N+1      |    F+1     |                                        | - DiscardStaleVkObjects(N, F)        |
    //              |            |                                        |   - {F, ResourceX} -> Release Queue  |
    //              |            |                                        |                                      |
    //     N+2 -   -|  - F+2  -  |  ExecuteCommandBuffer()                |                                      |
    //              |            |  - SubmitCommandBuffer()               |                                      |
    //              |            |  - ResourceX is already in release     |                                      |
    //              |            |    queue with fence value F, and       |                                      |
    //              |            |    F < SubmittedFenceValue==F+1        |                                      |
    //
    // Since transient command buffers do not count as real command buffers, submit them directly to the queue
    // to avoid interference with the command buffer counter
    Uint64 FenceValue = 0;
    LockCmdQueueAndRun(CommandQueueId,
                       [&](ICommandQueueVk* pCmdQueueVk) //
                       {
                           FenceValue = pCmdQueueVk->SubmitCmdBuffer(vkCmdBuff);
                       } //
    );

    class TransientCmdPoolRecycler
    {
    public:
        TransientCmdPoolRecycler(const VulkanUtilities::VulkanLogicalDevice& _LogicalDevice,
                                 CommandPoolManager&                         _CmdPoolMgr,
                                 VulkanUtilities::CommandPoolWrapper&&       _Pool,
                                 VkCommandBuffer&&                           _vkCmdBuffer) :
            // clang-format off
            LogicalDevice{_LogicalDevice         },
            CmdPoolMgr   {&_CmdPoolMgr           },
            Pool         {std::move(_Pool)       },
            vkCmdBuffer  {std::move(_vkCmdBuffer)}
        // clang-format on
        {
            VERIFY_EXPR(Pool != VK_NULL_HANDLE && vkCmdBuffer != VK_NULL_HANDLE);
            _vkCmdBuffer = VK_NULL_HANDLE;
        }

        // clang-format off
        TransientCmdPoolRecycler             (const TransientCmdPoolRecycler&)  = delete;
        TransientCmdPoolRecycler& operator = (const TransientCmdPoolRecycler&)  = delete;
        TransientCmdPoolRecycler& operator = (      TransientCmdPoolRecycler&&) = delete;

        TransientCmdPoolRecycler(TransientCmdPoolRecycler&& rhs) noexcept :
            LogicalDevice{rhs.LogicalDevice         },
            CmdPoolMgr   {rhs.CmdPoolMgr            },
            Pool         {std::move(rhs.Pool)       },
            vkCmdBuffer  {std::move(rhs.vkCmdBuffer)}
        {
            rhs.CmdPoolMgr  = nullptr;
            rhs.vkCmdBuffer = VK_NULL_HANDLE;
        }
        // clang-format on

        ~TransientCmdPoolRecycler()
        {
            if (CmdPoolMgr != nullptr)
            {
                LogicalDevice.FreeCommandBuffer(Pool, vkCmdBuffer);
                CmdPoolMgr->RecycleCommandPool(std::move(Pool));
            }
        }

    private:
        const VulkanUtilities::VulkanLogicalDevice& LogicalDevice;

        CommandPoolManager*                 CmdPoolMgr = nullptr;
        VulkanUtilities::CommandPoolWrapper Pool;
        VkCommandBuffer                     vkCmdBuffer = VK_NULL_HANDLE;
    };

    auto QueueFamilyIndex = HardwareQueueIndex{GetCommandQueue(CommandQueueId).GetQueueFamilyIndex()};
    auto CmdPoolMgrIter   = m_TransientCmdPoolMgrs.find(QueueFamilyIndex);
    VERIFY(CmdPoolMgrIter != m_TransientCmdPoolMgrs.end(),
           "Unable to find transient command pool manager for queue family index ", Uint32{QueueFamilyIndex}, ".");

    // Discard command pool directly to the release queue since we know exactly which queue it was submitted to
    // as well as the associated FenceValue
    // clang-format off
    GetReleaseQueue(CommandQueueId).DiscardResource(
        TransientCmdPoolRecycler
        {
            GetLogicalDevice(),
            CmdPoolMgrIter->second,
            std::move(CmdPool),
            std::move(vkCmdBuff)
        },
        FenceValue);
    // clang-format on
}

void RenderDeviceVkImpl::SubmitCommandBuffer(SoftwareQueueIndex                                          CommandQueueId,
                                             const VkSubmitInfo&                                         SubmitInfo,
                                             Uint64&                                                     SubmittedCmdBuffNumber, // Number of the submitted command buffer
                                             Uint64&                                                     SubmittedFenceValue,    // Fence value associated with the submitted command buffer
                                             std::vector<std::pair<Uint64, RefCntAutoPtr<FenceVkImpl>>>* pSignalFences           // List of fences to signal
)
{
    // Submit the command list to the queue
    auto CmbBuffInfo       = TRenderDeviceBase::SubmitCommandBuffer(CommandQueueId, true, SubmitInfo);
    SubmittedFenceValue    = CmbBuffInfo.FenceValue;
    SubmittedCmdBuffNumber = CmbBuffInfo.CmdBufferNumber;

    if (pSignalFences != nullptr && !pSignalFences->empty())
    {
        auto* pQueue     = m_CommandQueues[CommandQueueId].CmdQueue.RawPtr<CommandQueueVkImpl>();
        auto  pSyncPoint = pQueue->GetLastSyncPoint();

        for (auto& val_fence : *pSignalFences)
        {
            FenceVkImpl* pFenceVkImpl = val_fence.second;
            if (!pFenceVkImpl->IsTimelineSemaphore())
                pFenceVkImpl->AddPendingSyncPoint(CommandQueueId, val_fence.first, pSyncPoint);
        }
    }
}

Uint64 RenderDeviceVkImpl::ExecuteCommandBuffer(SoftwareQueueIndex CommandQueueId, const VkSubmitInfo& SubmitInfo, std::vector<std::pair<Uint64, RefCntAutoPtr<FenceVkImpl>>>* pSignalFences)
{
    Uint64 SubmittedFenceValue    = 0;
    Uint64 SubmittedCmdBuffNumber = 0;
    SubmitCommandBuffer(CommandQueueId, SubmitInfo, SubmittedCmdBuffNumber, SubmittedFenceValue, pSignalFences);

    m_MemoryMgr.ShrinkMemory();
    PurgeReleaseQueue(CommandQueueId);

    return SubmittedFenceValue;
}


void RenderDeviceVkImpl::IdleGPU()
{
    IdleAllCommandQueues(true);
    m_LogicalVkDevice->WaitIdle();
    ReleaseStaleResources();
}

void RenderDeviceVkImpl::FlushStaleResources(SoftwareQueueIndex CmdQueueIndex)
{
    // Submit empty command buffer to the queue. This will effectively signal the fence and
    // discard all resources
    VkSubmitInfo DummySubmitInfo{};
    DummySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    TRenderDeviceBase::SubmitCommandBuffer(CmdQueueIndex, true, DummySubmitInfo);
}

void RenderDeviceVkImpl::ReleaseStaleResources(bool ForceRelease)
{
    m_MemoryMgr.ShrinkMemory();
    PurgeReleaseQueues(ForceRelease);
}


void RenderDeviceVkImpl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto vkPhysicalDevice = m_PhysicalDevice->GetVkDeviceHandle();

    auto CheckFormatProperties =
        [vkPhysicalDevice](VkFormat vkFmt, VkImageType vkImgType, VkImageUsageFlags vkUsage, VkImageFormatProperties& ImgFmtProps) //
    {
        auto err = vkGetPhysicalDeviceImageFormatProperties(vkPhysicalDevice, vkFmt, vkImgType, VK_IMAGE_TILING_OPTIMAL,
                                                            vkUsage, 0, &ImgFmtProps);
        return err == VK_SUCCESS;
    };


    TexFormatInfo.BindFlags  = BIND_NONE;
    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;

    {
        auto SRVFormat = GetDefaultTextureViewFormat(TexFormat, TEXTURE_VIEW_SHADER_RESOURCE, BIND_SHADER_RESOURCE);
        if (SRVFormat != TEX_FORMAT_UNKNOWN)
        {
            VkFormat           vkSrvFormat   = TexFormatToVkFormat(SRVFormat);
            VkFormatProperties vkSrvFmtProps = {};
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkSrvFormat, &vkSrvFmtProps);

            if (vkSrvFmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            {
                TexFormatInfo.Filterable = (vkSrvFmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
                TexFormatInfo.BindFlags |= BIND_SHADER_RESOURCE;

                VkImageFormatProperties ImgFmtProps = {};
                if (CheckFormatProperties(vkSrvFormat, VK_IMAGE_TYPE_1D, VK_IMAGE_USAGE_SAMPLED_BIT, ImgFmtProps))
                    TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D | RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;

                if (CheckFormatProperties(vkSrvFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_SAMPLED_BIT, ImgFmtProps))
                    TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D | RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;

                if (CheckFormatProperties(vkSrvFormat, VK_IMAGE_TYPE_3D, VK_IMAGE_USAGE_SAMPLED_BIT, ImgFmtProps))
                    TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;

                {
                    auto err = vkGetPhysicalDeviceImageFormatProperties(vkPhysicalDevice, vkSrvFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                                        VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, &ImgFmtProps);
                    if (err == VK_SUCCESS)
                        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE | RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;
                }
            }
        }
    }

    {
        auto RTVFormat = GetDefaultTextureViewFormat(TexFormat, TEXTURE_VIEW_RENDER_TARGET, BIND_RENDER_TARGET);
        if (RTVFormat != TEX_FORMAT_UNKNOWN)
        {
            VkFormat           vkRtvFormat   = TexFormatToVkFormat(RTVFormat);
            VkFormatProperties vkRtvFmtProps = {};
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkRtvFormat, &vkRtvFmtProps);

            if (vkRtvFmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
            {
                VkImageFormatProperties ImgFmtProps = {};
                if (CheckFormatProperties(vkRtvFormat, VK_IMAGE_TYPE_2D, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, ImgFmtProps))
                {
                    TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
                    TexFormatInfo.SampleCounts = VkSampleCountFlagsToSampleCount(ImgFmtProps.sampleCounts);
                }
            }
        }
    }

    {
        auto DSVFormat = GetDefaultTextureViewFormat(TexFormat, TEXTURE_VIEW_DEPTH_STENCIL, BIND_DEPTH_STENCIL);
        if (DSVFormat != TEX_FORMAT_UNKNOWN)
        {
            VkFormat           vkDsvFormat   = TexFormatToVkFormat(DSVFormat);
            VkFormatProperties vkDsvFmtProps = {};
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkDsvFormat, &vkDsvFmtProps);
            if (vkDsvFmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                VkImageFormatProperties ImgFmtProps = {};
                if (CheckFormatProperties(vkDsvFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, ImgFmtProps))
                {
                    // MoltenVK reports VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT for
                    // VK_FORMAT_D24_UNORM_S8_UINT even though the format is not supported.
                    TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
                    TexFormatInfo.SampleCounts = VkSampleCountFlagsToSampleCount(ImgFmtProps.sampleCounts);
                }
            }
        }
    }

    {
        auto UAVFormat = GetDefaultTextureViewFormat(TexFormat, TEXTURE_VIEW_UNORDERED_ACCESS, BIND_DEPTH_STENCIL);
        if (UAVFormat != TEX_FORMAT_UNKNOWN)
        {
            VkFormat           vkUavFormat   = TexFormatToVkFormat(UAVFormat);
            VkFormatProperties vkUavFmtProps = {};
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkUavFormat, &vkUavFmtProps);
            if (vkUavFmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
            {
                VkImageFormatProperties ImgFmtProps = {};
                if (CheckFormatProperties(vkUavFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_STORAGE_BIT, ImgFmtProps))
                {
                    TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;
                }
            }
        }
    }
}

void RenderDeviceVkImpl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceVkImpl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceVkImpl::CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceVkImpl::CreateBufferFromVulkanResource(VkBuffer vkBuffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, InitialState, vkBuffer);
}

void RenderDeviceVkImpl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, pBuffData);
}


void RenderDeviceVkImpl::CreateShader(const ShaderCreateInfo& ShaderCI,
                                      IShader**               ppShader,
                                      IDataBlob**             ppCompilerOutput)
{
    const ShaderVkImpl::CreateInfo VkShaderCI{
        GetDxCompiler(),
        GetDeviceInfo(),
        GetAdapterInfo(),
        GetVkVersion(),
        GetLogicalDevice().GetEnabledExtFeatures().Spirv14,
        ppCompilerOutput,
        m_pShaderCompilationThreadPool,
    };
    CreateShaderImpl(ppShader, ShaderCI, VkShaderCI);
}


void RenderDeviceVkImpl::CreateTextureFromVulkanImage(VkImage vkImage, const TextureDesc& TexDesc, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    CreateTextureImpl(ppTexture, TexDesc, InitialState, vkImage);
}


void RenderDeviceVkImpl::CreateTexture(const TextureDesc& TexDesc, VkImage vkImgHandle, RESOURCE_STATE InitialState, class TextureVkImpl** ppTexture)
{
    CreateDeviceObject(
        "texture", TexDesc, ppTexture,
        [&]() //
        {
            TextureVkImpl* pTextureVk = NEW_RC_OBJ(m_TexObjAllocator, "TextureVkImpl instance", TextureVkImpl)(m_TexViewObjAllocator, this, TexDesc, InitialState, std::move(vkImgHandle));
            pTextureVk->QueryInterface(IID_TextureVk, reinterpret_cast<IObject**>(ppTexture));
        } //
    );
}


void RenderDeviceVkImpl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateTextureImpl(ppTexture, TexDesc, pData);
}

void RenderDeviceVkImpl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateSamplerImpl(ppSampler, SamplerDesc);
}

void RenderDeviceVkImpl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateFenceImpl(ppFence, Desc);
}

void RenderDeviceVkImpl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateQueryImpl(ppQuery, Desc);
}

void RenderDeviceVkImpl::CreateRenderPass(const RenderPassDesc& Desc,
                                          IRenderPass**         ppRenderPass,
                                          bool                  IsDeviceInternal)
{
    CreateRenderPassImpl(ppRenderPass, Desc, IsDeviceInternal);
}

void RenderDeviceVkImpl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateRenderPass(Desc, ppRenderPass, /*IsDeviceInternal = */ false);
}

void RenderDeviceVkImpl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateFramebufferImpl(ppFramebuffer, Desc);
}

void RenderDeviceVkImpl::CreateBLASFromVulkanResource(VkAccelerationStructureKHR vkBLAS,
                                                      const BottomLevelASDesc&   Desc,
                                                      RESOURCE_STATE             InitialState,
                                                      IBottomLevelAS**           ppBLAS)
{
    CreateBLASImpl(ppBLAS, Desc, InitialState, vkBLAS);
}

void RenderDeviceVkImpl::CreateBLAS(const BottomLevelASDesc& Desc,
                                    IBottomLevelAS**         ppBLAS)
{
    CreateBLASImpl(ppBLAS, Desc);
}

void RenderDeviceVkImpl::CreateTLASFromVulkanResource(VkAccelerationStructureKHR vkTLAS,
                                                      const TopLevelASDesc&      Desc,
                                                      RESOURCE_STATE             InitialState,
                                                      ITopLevelAS**              ppTLAS)
{
    CreateTLASImpl(ppTLAS, Desc, InitialState, vkTLAS);
}

void RenderDeviceVkImpl::CreateFenceFromVulkanResource(VkSemaphore      vkTimelineSemaphore,
                                                       const FenceDesc& Desc,
                                                       IFence**         ppFence)
{
    CreateFenceImpl(ppFence, Desc, vkTimelineSemaphore);
}

void RenderDeviceVkImpl::CreateTLAS(const TopLevelASDesc& Desc,
                                    ITopLevelAS**         ppTLAS)
{
    CreateTLASImpl(ppTLAS, Desc);
}

void RenderDeviceVkImpl::CreateSBT(const ShaderBindingTableDesc& Desc,
                                   IShaderBindingTable**         ppSBT)
{
    CreateSBTImpl(ppSBT, Desc);
}

void RenderDeviceVkImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                         IPipelineResourceSignature**         ppSignature)
{
    CreatePipelineResourceSignature(Desc, ppSignature, SHADER_TYPE_UNKNOWN, false);
}

void RenderDeviceVkImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                         IPipelineResourceSignature**         ppSignature,
                                                         SHADER_TYPE                          ShaderStages,
                                                         bool                                 IsDeviceInternal)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, ShaderStages, IsDeviceInternal);
}

void RenderDeviceVkImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&           Desc,
                                                         const PipelineResourceSignatureInternalDataVk& InternalData,
                                                         IPipelineResourceSignature**                   ppSignature)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, InternalData);
}

void RenderDeviceVkImpl::CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo, IDeviceMemory** ppMemory)
{
    CreateDeviceMemoryImpl(ppMemory, CreateInfo);
}

void RenderDeviceVkImpl::CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo, IPipelineStateCache** ppPipelineStateCache)
{
    CreatePipelineStateCacheImpl(ppPipelineStateCache, CreateInfo);
}

std::vector<uint32_t> RenderDeviceVkImpl::ConvertCmdQueueIdsToQueueFamilies(Uint64 CommandQueueMask) const
{
    std::bitset<MAX_COMMAND_QUEUES> QueueFamilyBits{};

    std::vector<uint32_t> QueueFamilyIndices;
    while (CommandQueueMask != 0)
    {
        auto CmdQueueInd = PlatformMisc::GetLSB(CommandQueueMask);
        CommandQueueMask &= ~(Uint64{1} << Uint64{CmdQueueInd});

        auto& CmdQueue    = GetCommandQueue(SoftwareQueueIndex{CmdQueueInd});
        auto  FamilyIndex = CmdQueue.GetQueueFamilyIndex();
        if (!QueueFamilyBits[FamilyIndex])
        {
            QueueFamilyBits[FamilyIndex] = true;
            QueueFamilyIndices.push_back(FamilyIndex);
        }
    }
    return QueueFamilyIndices;
}

HardwareQueueIndex RenderDeviceVkImpl::GetQueueFamilyIndex(SoftwareQueueIndex CmdQueueInd) const
{
    const auto& CmdQueue = GetCommandQueue(SoftwareQueueIndex{CmdQueueInd});
    return HardwareQueueIndex{CmdQueue.GetQueueFamilyIndex()};
}

SparseTextureFormatInfo RenderDeviceVkImpl::GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                       RESOURCE_DIMENSION Dimension,
                                                                       Uint32             SampleCount) const
{
    const auto ComponentType = CheckSparseTextureFormatSupport(TexFormat, Dimension, SampleCount, m_AdapterInfo.SparseResources);
    if (ComponentType == COMPONENT_TYPE_UNDEFINED)
        return {};

    const auto vkDevice       = m_PhysicalDevice->GetVkDeviceHandle();
    const auto vkType         = Dimension == RESOURCE_DIM_TEX_3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    const auto vkFormat       = TexFormatToVkFormat(TexFormat);
    const auto vkDefaultUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const auto vkSampleCount  = static_cast<VkSampleCountFlagBits>(SampleCount);

    // Texture with depth-stencil format may be implemented with two memory blocks per tile.
    VkSparseImageFormatProperties FmtProps[2]   = {};
    Uint32                        FmtPropsCount = 0;

    vkGetPhysicalDeviceSparseImageFormatProperties(vkDevice, vkFormat, vkType, vkSampleCount, vkDefaultUsage, VK_IMAGE_TILING_OPTIMAL, &FmtPropsCount, nullptr);
    if (FmtPropsCount != 1)
        return {}; // Only single block per region is supported

    vkGetPhysicalDeviceSparseImageFormatProperties(vkDevice, vkFormat, vkType, vkSampleCount, vkDefaultUsage, VK_IMAGE_TILING_OPTIMAL, &FmtPropsCount, FmtProps);

    SparseTextureFormatInfo Info;
    Info.BindFlags   = BIND_NONE;
    Info.TileSize[0] = FmtProps[0].imageGranularity.width;
    Info.TileSize[1] = FmtProps[0].imageGranularity.height;
    Info.TileSize[2] = FmtProps[0].imageGranularity.depth;
    Info.Flags       = VkSparseImageFormatFlagsToSparseTextureFlags(FmtProps[0].flags);

    const auto CheckUsage = [&](VkImageUsageFlags vkUsage) {
        Uint32 Count = 0;
        vkGetPhysicalDeviceSparseImageFormatProperties(vkDevice, vkFormat, vkType, vkSampleCount, vkDefaultUsage | vkUsage, VK_IMAGE_TILING_OPTIMAL, &Count, nullptr);
        return (Count != 0);
    };

    if ((ComponentType == COMPONENT_TYPE_DEPTH || ComponentType == COMPONENT_TYPE_DEPTH_STENCIL) && CheckUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        Info.BindFlags |= BIND_DEPTH_STENCIL;
    else if (ComponentType != COMPONENT_TYPE_COMPRESSED && Dimension != RESOURCE_DIM_TEX_3D && CheckUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        Info.BindFlags |= BIND_RENDER_TARGET;

    if ((Info.BindFlags & (BIND_DEPTH_STENCIL | BIND_RENDER_TARGET)) != 0 && CheckUsage(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
        Info.BindFlags |= BIND_INPUT_ATTACHMENT;
    if (CheckUsage(VK_IMAGE_USAGE_SAMPLED_BIT))
        Info.BindFlags |= BIND_SHADER_RESOURCE;
    if (CheckUsage(VK_IMAGE_USAGE_STORAGE_BIT))
        Info.BindFlags |= BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

    return Info;
}

} // namespace Diligent
