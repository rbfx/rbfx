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

#pragma once

/// \file
/// Declaration of Diligent::RenderDeviceVkImpl class
#include <memory>
#include <unordered_map>
#include <vector>

#include "EngineVkImplTraits.hpp"

#include "RenderDeviceBase.hpp"
#include "RenderDeviceNextGenBase.hpp"

#include "VulkanUtilities/VulkanInstance.hpp"
#include "VulkanUtilities/VulkanPhysicalDevice.hpp"
#include "VulkanUtilities/VulkanCommandBufferPool.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanMemoryManager.hpp"

#include "DescriptorPoolManager.hpp"
#include "VulkanDynamicHeap.hpp"
#include "VulkanUploadHeap.hpp"
#include "FramebufferCache.hpp"
#include "RenderPassCache.hpp"
#include "CommandPoolManager.hpp"
#include "DXCompiler.hpp"

namespace Diligent
{

class QueryManagerVk;

/// Render device implementation in Vulkan backend.
class RenderDeviceVkImpl final : public RenderDeviceNextGenBase<RenderDeviceBase<EngineVkImplTraits>, ICommandQueueVk>
{
public:
    using TRenderDeviceBase = RenderDeviceNextGenBase<RenderDeviceBase<EngineVkImplTraits>, ICommandQueueVk>;

    RenderDeviceVkImpl(IReferenceCounters*                                    pRefCounters,
                       IMemoryAllocator&                                      RawMemAllocator,
                       IEngineFactory*                                        pEngineFactory,
                       const EngineVkCreateInfo&                              EngineCI,
                       const GraphicsAdapterInfo&                             AdapterInfo,
                       size_t                                                 CommandQueueCount,
                       ICommandQueueVk**                                      pCmdQueues,
                       std::shared_ptr<VulkanUtilities::VulkanInstance>       Instance,
                       std::unique_ptr<VulkanUtilities::VulkanPhysicalDevice> PhysicalDevice,
                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice>  LogicalDevice) noexcept(false);
    ~RenderDeviceVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_RenderDeviceVk, TRenderDeviceBase)

    /// Implementation of IRenderDevice::CreateGraphicsPipelineState() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateComputePipelineState() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateRayTracingPipelineState() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateBuffer() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateBuffer(const BufferDesc& BuffDesc,
                                                 const BufferData* pBuffData,
                                                 IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDevice::CreateShader() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCreateInfo,
                                                 IShader**               ppShader,
                                                 IDataBlob**             ppCompilerOutput) override final;

    /// Implementation of IRenderDevice::CreateTexture() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture(const TextureDesc& TexDesc,
                                                  const TextureData* pData,
                                                  ITexture**         ppTexture) override final;

    void CreateTexture(const TextureDesc& TexDesc, VkImage vkImgHandle, RESOURCE_STATE InitialState, class TextureVkImpl** ppTexture);

    /// Implementation of IRenderDevice::CreateSampler() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler) override final;

    /// Implementation of IRenderDevice::CreateFence() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateFence(const FenceDesc& Desc, IFence** ppFence) override final;

    /// Implementation of IRenderDevice::CreateQuery() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateQuery(const QueryDesc& Desc, IQuery** ppQuery) override final;

    /// Implementation of IRenderDevice::CreateRenderPass() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateRenderPass(const RenderPassDesc& Desc,
                                                     IRenderPass**         ppRenderPass) override final;

    void CreateRenderPass(const RenderPassDesc& Desc,
                          IRenderPass**         ppRenderPass,
                          bool                  IsDeviceInternal);


    /// Implementation of IRenderDevice::CreateFramebuffer() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateFramebuffer(const FramebufferDesc& Desc,
                                                      IFramebuffer**         ppFramebuffer) override final;

    /// Implementation of IRenderDevice::CreateBLAS() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateBLAS(const BottomLevelASDesc& Desc,
                                               IBottomLevelAS**         ppBLAS) override final;

    /// Implementation of IRenderDevice::CreateTLAS() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateTLAS(const TopLevelASDesc& Desc,
                                               ITopLevelAS**         ppTLAS) override final;

    /// Implementation of IRenderDevice::CreateSBT() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateSBT(const ShaderBindingTableDesc& Desc,
                                              IShaderBindingTable**         ppSBT) override final;

    /// Implementation of IRenderDevice::CreatePipelineResourceSignature() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                                    IPipelineResourceSignature**         ppSignature) override final;

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                         IPipelineResourceSignature**         ppSignature,
                                         SHADER_TYPE                          ShaderStages,
                                         bool                                 IsDeviceInternal);

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&           Desc,
                                         const PipelineResourceSignatureInternalDataVk& InternalData,
                                         IPipelineResourceSignature**                   ppSignature);

    /// Implementation of IRenderDevice::CreateDeviceMemory() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo,
                                                       IDeviceMemory**               ppMemory) override final;

    /// Implementation of IRenderDevice::CreatePipelineStateCache() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                             IPipelineStateCache**               ppPipelineStateCache) override final;

    /// Implementation of IRenderDeviceVk::GetVkDevice().
    virtual VkDevice DILIGENT_CALL_TYPE GetVkDevice() override final { return m_LogicalVkDevice->GetVkDevice(); }

    /// Implementation of IRenderDeviceVk::GetVkPhysicalDevice().
    virtual VkPhysicalDevice DILIGENT_CALL_TYPE GetVkPhysicalDevice() override final { return m_PhysicalDevice->GetVkDeviceHandle(); }

    /// Implementation of IRenderDeviceVk::GetVkInstance().
    virtual VkInstance DILIGENT_CALL_TYPE GetVkInstance() override final { return m_VulkanInstance->GetVkInstance(); }

    /// Implementation of IRenderDeviceVk::GetVkVersion().
    virtual Uint32 DILIGENT_CALL_TYPE GetVkVersion() override final { return m_PhysicalDevice->GetVkVersion(); }

    /// Implementation of IRenderDeviceVk::CreateTextureFromVulkanImage().
    virtual void DILIGENT_CALL_TYPE CreateTextureFromVulkanImage(VkImage            vkImage,
                                                                 const TextureDesc& TexDesc,
                                                                 RESOURCE_STATE     InitialState,
                                                                 ITexture**         ppTexture) override final;

    /// Implementation of IRenderDeviceVk::CreateBufferFromVulkanResource().
    virtual void DILIGENT_CALL_TYPE CreateBufferFromVulkanResource(VkBuffer          vkBuffer,
                                                                   const BufferDesc& BuffDesc,
                                                                   RESOURCE_STATE    InitialState,
                                                                   IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDeviceVk::CreateBLASFromVulkanResource().
    virtual void DILIGENT_CALL_TYPE CreateBLASFromVulkanResource(VkAccelerationStructureKHR vkBLAS,
                                                                 const BottomLevelASDesc&   Desc,
                                                                 RESOURCE_STATE             InitialState,
                                                                 IBottomLevelAS**           ppBLAS) override final;

    /// Implementation of IRenderDeviceVk::CreateTLASFromVulkanResource().
    virtual void DILIGENT_CALL_TYPE CreateTLASFromVulkanResource(VkAccelerationStructureKHR vkTLAS,
                                                                 const TopLevelASDesc&      Desc,
                                                                 RESOURCE_STATE             InitialState,
                                                                 ITopLevelAS**              ppTLAS) override final;

    /// Implementation of IRenderDeviceVk::CreateFenceFromVulkanResource().
    virtual void DILIGENT_CALL_TYPE CreateFenceFromVulkanResource(VkSemaphore      vkTimelineSemaphore,
                                                                  const FenceDesc& Desc,
                                                                  IFence**         ppFence) override final;

    /// Implementation of IRenderDevice::IdleGPU() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE IdleGPU() override final;

    // pImmediateCtx parameter is only used to make sure the command buffer is submitted from the immediate context
    // The method returns fence value associated with the submitted command buffer
    Uint64 ExecuteCommandBuffer(SoftwareQueueIndex CommandQueueId, const VkSubmitInfo& SubmitInfo, std::vector<std::pair<Uint64, RefCntAutoPtr<FenceVkImpl>>>* pSignalFences);

    void AllocateTransientCmdPool(SoftwareQueueIndex                    CommandQueueId,
                                  VulkanUtilities::CommandPoolWrapper&  CmdPool,
                                  VulkanUtilities::VulkanCommandBuffer& CmdBuffer,
                                  const Char*                           DebugPoolName = nullptr);
    void ExecuteAndDisposeTransientCmdBuff(SoftwareQueueIndex CommandQueueId, VkCommandBuffer vkCmdBuff, VulkanUtilities::CommandPoolWrapper&& CmdPool);

    /// Implementation of IRenderDevice::ReleaseStaleResources() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE ReleaseStaleResources(bool ForceRelease = false) override final;

    /// Implementation of IRenderDevice::GetSparseTextureFormatInfo() in Vulkan backend.
    virtual SparseTextureFormatInfo DILIGENT_CALL_TYPE GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                                  RESOURCE_DIMENSION Dimension,
                                                                                  Uint32             SampleCount) const override final;

    DescriptorSetAllocation AllocateDescriptorSet(Uint64 CommandQueueMask, VkDescriptorSetLayout SetLayout, const char* DebugName = "")
    {
        return m_DescriptorSetAllocator.Allocate(CommandQueueMask, SetLayout, DebugName);
    }
    DescriptorPoolManager& GetDynamicDescriptorPool() { return m_DynamicDescriptorPool; }

    std::shared_ptr<const VulkanUtilities::VulkanInstance> GetVulkanInstance() const { return m_VulkanInstance; }

    const VulkanUtilities::VulkanPhysicalDevice& GetPhysicalDevice() const { return *m_PhysicalDevice; }
    const VulkanUtilities::VulkanLogicalDevice&  GetLogicalDevice() const { return *m_LogicalVkDevice; }

    FramebufferCache& GetFramebufferCache() { return m_FramebufferCache; }
    RenderPassCache&  GetImplicitRenderPassCache() { return m_ImplicitRenderPassCache; }

    VulkanUtilities::VulkanMemoryAllocation AllocateMemory(const VkMemoryRequirements& MemReqs, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags = 0)
    {
        return m_MemoryMgr.Allocate(MemReqs, MemoryProperties, AllocateFlags);
    }
    VulkanUtilities::VulkanMemoryAllocation AllocateMemory(VkDeviceSize Size, VkDeviceSize Alignment, uint32_t MemoryTypeIndex, VkMemoryAllocateFlags AllocateFlags = 0)
    {
        const auto& MemoryProps = m_PhysicalDevice->GetMemoryProperties();
        VERIFY_EXPR(MemoryTypeIndex < MemoryProps.memoryTypeCount);
        const auto MemoryFlags = MemoryProps.memoryTypes[MemoryTypeIndex].propertyFlags;
        return m_MemoryMgr.Allocate(Size, Alignment, MemoryTypeIndex, (MemoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0, AllocateFlags);
    }
    VulkanUtilities::VulkanMemoryManager& GetGlobalMemoryManager() { return m_MemoryMgr; }

    VulkanDynamicMemoryManager& GetDynamicMemoryManager() { return m_DynamicMemoryManager; }

    void FlushStaleResources(SoftwareQueueIndex CmdQueueIndex);

    IDXCompiler* GetDxCompiler() const { return m_pDxCompiler.get(); }

    struct Properties
    {
        const Uint32 ShaderGroupHandleSize;
        const Uint32 MaxShaderRecordStride;
        const Uint32 ShaderGroupBaseAlignment;
        const Uint32 MaxDrawMeshTasksCount;
        const Uint32 MaxRayTracingRecursionDepth;
        const Uint32 MaxRayGenThreads;
    };

    // TODO: use small_vector
    std::vector<uint32_t> ConvertCmdQueueIdsToQueueFamilies(Uint64 CommandQueueMask) const;

    HardwareQueueIndex GetQueueFamilyIndex(SoftwareQueueIndex CmdQueueInd) const;

    QueryManagerVk& GetQueryMgr(SoftwareQueueIndex CmdQueueInd)
    {
        return *m_QueryMgrs[CmdQueueInd];
    }

private:
    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) override final;

    // Submits command buffer(s) for execution to the command queue and
    // returns the submitted command buffer(s) number and the fence value.
    // If SubmitInfo contains multiple command buffers, they all are treated
    // like one and submitted atomically.
    // Parameters:
    //      * SubmittedCmdBuffNumber - submitted command buffer number
    //      * SubmittedFenceValue    - fence value associated with the submitted command buffer
    void SubmitCommandBuffer(SoftwareQueueIndex                                          CommandQueueId,
                             const VkSubmitInfo&                                         SubmitInfo,
                             Uint64&                                                     SubmittedCmdBuffNumber,
                             Uint64&                                                     SubmittedFenceValue,
                             std::vector<std::pair<Uint64, RefCntAutoPtr<FenceVkImpl>>>* pFences);

    std::shared_ptr<VulkanUtilities::VulkanInstance>       m_VulkanInstance;
    std::unique_ptr<VulkanUtilities::VulkanPhysicalDevice> m_PhysicalDevice;
    std::shared_ptr<VulkanUtilities::VulkanLogicalDevice>  m_LogicalVkDevice;

    FramebufferCache       m_FramebufferCache;
    RenderPassCache        m_ImplicitRenderPassCache;
    DescriptorSetAllocator m_DescriptorSetAllocator;
    DescriptorPoolManager  m_DynamicDescriptorPool;

    // These one-time command pools are used by buffer and texture constructors to
    // issue copy commands. Vulkan requires that every command pool is used by one thread
    // at a time, so every constructor must allocate command buffer from its own pool.
    std::unordered_map<HardwareQueueIndex, CommandPoolManager, HardwareQueueIndex::Hasher> m_TransientCmdPoolMgrs;

    // Each command queue needs its own query manager to avoid race conditions.
    std::vector<std::unique_ptr<QueryManagerVk>> m_QueryMgrs;

    VulkanUtilities::VulkanMemoryManager m_MemoryMgr;

    VulkanDynamicMemoryManager m_DynamicMemoryManager;

    std::unique_ptr<IDXCompiler> m_pDxCompiler;
};

} // namespace Diligent
