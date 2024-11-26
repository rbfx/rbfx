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

#include "DeviceContextVkImpl.hpp"

#include <sstream>
#include <vector>

#include "RenderDeviceVkImpl.hpp"
#include "PipelineStateVkImpl.hpp"
#include "TextureVkImpl.hpp"
#include "BufferVkImpl.hpp"
#include "RenderPassVkImpl.hpp"
#include "FenceVkImpl.hpp"
#include "DeviceMemoryVkImpl.hpp"

#include "VulkanTypeConversions.hpp"
#include "CommandListVkImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "GenerateMipsVkHelper.hpp"
#include "QueryManagerVk.hpp"
#include "CommandQueueVkImpl.hpp"

namespace Diligent
{

static std::string GetContextObjectName(const char* Object, bool bIsDeferred, Uint32 ContextId)
{
    std::stringstream ss;
    ss << Object;
    if (bIsDeferred)
        ss << " of deferred context #" << ContextId;
    else
        ss << " of immediate context";
    return ss.str();
}

DeviceContextVkImpl::DeviceContextVkImpl(IReferenceCounters*       pRefCounters,
                                         RenderDeviceVkImpl*       pDeviceVkImpl,
                                         const EngineVkCreateInfo& EngineCI,
                                         const DeviceContextDesc&  Desc) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDeviceVkImpl,
        Desc
    },
    m_CmdListAllocator { GetRawAllocator(), sizeof(CommandListVkImpl), 64 },
    // Upload heap must always be thread-safe as Finish() may be called from another thread
    m_QueueFamilyCmdPools
    {
        new std::unique_ptr<VulkanUtilities::VulkanCommandBufferPool>[
            // Note that for immediate contexts we will always use one pool,
            // but we still allocate space for all queue families for consistency.
            pDeviceVkImpl->GetPhysicalDevice().GetQueueProperties().size()
        ]
    },
    m_UploadHeap
    {
        *pDeviceVkImpl,
        GetContextObjectName("Upload heap", Desc.IsDeferred, Desc.ContextId),
        EngineCI.UploadHeapPageSize
    },
    m_DynamicHeap
    {
        pDeviceVkImpl->GetDynamicMemoryManager(),
        GetContextObjectName("Dynamic heap", Desc.IsDeferred, Desc.ContextId),
        EngineCI.DynamicHeapPageSize
    },
    m_DynamicDescrSetAllocator
    {
        pDeviceVkImpl->GetDynamicDescriptorPool(),
        GetContextObjectName("Dynamic descriptor set allocator", Desc.IsDeferred, Desc.ContextId),
    }
// clang-format on
{
    if (!IsDeferred())
    {
        PrepareCommandPool(GetCommandQueueId());
        m_pQueryMgr = &pDeviceVkImpl->GetQueryMgr(GetCommandQueueId());
        EnsureVkCmdBuffer();
        m_State.NumCommands += m_pQueryMgr->ResetStaleQueries(m_pDevice->GetLogicalDevice(), m_CommandBuffer);
    }

    BufferDesc DummyVBDesc;
    DummyVBDesc.Name      = "Dummy vertex buffer";
    DummyVBDesc.BindFlags = BIND_VERTEX_BUFFER;
    DummyVBDesc.Usage     = USAGE_DEFAULT;
    DummyVBDesc.Size      = 32;
    RefCntAutoPtr<IBuffer> pDummyVB;
    m_pDevice->CreateBuffer(DummyVBDesc, nullptr, &pDummyVB);
    m_DummyVB = pDummyVB.RawPtr<BufferVkImpl>();

    m_vkClearValues.reserve(16);

    m_DynamicBufferOffsets.reserve(64);

    CreateASCompactedSizeQueryPool();
}

DeviceContextVkImpl::~DeviceContextVkImpl()
{
    if (m_State.NumCommands != 0)
    {
        if (IsDeferred())
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in deferred context #", GetContextId(),
                              " being destroyed, which indicates that FinishCommandList() has not been called."
                              " This may cause synchronization issues.");
        }
        else
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in the immediate context being destroyed, "
                              "which indicates the context has not been Flush()'ed.",
                              " This may cause synchronization issues.");
        }
    }

    if (!IsDeferred())
    {
        Flush();
    }

    // For deferred contexts, m_SubmittedBuffersCmdQueueMask is reset to 0 after every call to FinishFrame().
    // In this case there are no resources to release, so there will be no issues.
    FinishFrame();

    // There must be no stale resources
    // clang-format off
    DEV_CHECK_ERR(m_UploadHeap.GetStalePagesCount()                  == 0, "All allocated upload heap pages must have been released at this point");
    DEV_CHECK_ERR(m_DynamicHeap.GetAllocatedMasterBlockCount()       == 0, "All allocated dynamic heap master blocks must have been released");
    DEV_CHECK_ERR(m_DynamicDescrSetAllocator.GetAllocatedPoolCount() == 0, "All allocated dynamic descriptor set pools must have been released at this point");
    // clang-format on

    // NB: If there are any command buffers in the release queue, they will always be returned to the pool
    //     before the pool itself is released because the pool will always end up later in the queue,
    //     so we do not need to idle the GPU.
    //     Also note that command buffers are disposed directly into the release queue, but
    //     the command pool goes into the stale objects queue and is moved into the release queue
    //     when the next command buffer is submitted.
    if (m_QueueFamilyCmdPools)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_QueueFamilyCmdPools), ~Uint64{0});

    // NB: Upload heap, dynamic heap and dynamic descriptor manager return their resources to
    //     global managers and do not need to wait for GPU to idle.
}

void DeviceContextVkImpl::PrepareCommandPool(SoftwareQueueIndex CommandQueueId)
{
    DEV_CHECK_ERR(CommandQueueId < m_pDevice->GetCommandQueueCount(), "CommandQueueId is out of range");

    const auto  QueueFamilyIndex = HardwareQueueIndex{m_pDevice->GetCommandQueue(CommandQueueId).GetQueueFamilyIndex()};
    const auto& QueueProps       = m_pDevice->GetPhysicalDevice().GetQueueProperties();
    DEV_CHECK_ERR(QueueFamilyIndex < QueueProps.size(), "QueueFamilyIndex is out of range");

    auto& Pool = m_QueueFamilyCmdPools[QueueFamilyIndex];
    if (!Pool)
    {
        // Command pools must be thread-safe because command buffers are returned into pools by release queues
        // potentially running in another thread
        Pool = std::make_unique<VulkanUtilities::VulkanCommandBufferPool>(
            m_pDevice->GetLogicalDevice().GetSharedPtr(),
            QueueFamilyIndex,
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }
    m_CmdPool = Pool.get();

    // Set queue properties
    const auto& QueueInfo{QueueProps[QueueFamilyIndex]};
    m_Desc.QueueId                   = static_cast<Uint8>(QueueFamilyIndex);
    m_Desc.QueueType                 = VkQueueFlagsToCmdQueueType(QueueInfo.queueFlags);
    m_Desc.TextureCopyGranularity[0] = QueueInfo.minImageTransferGranularity.width;
    m_Desc.TextureCopyGranularity[1] = QueueInfo.minImageTransferGranularity.height;
    m_Desc.TextureCopyGranularity[2] = QueueInfo.minImageTransferGranularity.depth;
}

void DeviceContextVkImpl::Begin(Uint32 ImmediateContextId)
{
    DEV_CHECK_ERR(IsDeferred(), "Begin() should only be called for deferred contexts.");
    DEV_CHECK_ERR(!IsRecordingDeferredCommands(), "This context is already recording commands. Call FinishCommandList() before beginning new recording.");
    const SoftwareQueueIndex CommandQueueId{ImmediateContextId};
    PrepareCommandPool(CommandQueueId);
    m_DstImmediateContextId = static_cast<Uint8>(ImmediateContextId);
    VERIFY_EXPR(m_DstImmediateContextId == ImmediateContextId);
    m_pQueryMgr = &m_pDevice->GetQueryMgr(CommandQueueId);
}

void DeviceContextVkImpl::DisposeVkCmdBuffer(SoftwareQueueIndex CmdQueue, VkCommandBuffer vkCmdBuff, Uint64 FenceValue)
{
    VERIFY_EXPR(vkCmdBuff != VK_NULL_HANDLE);
    VERIFY_EXPR(m_CmdPool != nullptr);
    class CmdBufferRecycler
    {
    public:
        // clang-format off
        CmdBufferRecycler(VkCommandBuffer                           _vkCmdBuff,
                         VulkanUtilities::VulkanCommandBufferPool& _Pool) noexcept :
            vkCmdBuff {_vkCmdBuff},
            Pool      {&_Pool    }
        {
            VERIFY_EXPR(vkCmdBuff != VK_NULL_HANDLE);
        }

        CmdBufferRecycler             (const CmdBufferRecycler&)  = delete;
        CmdBufferRecycler& operator = (const CmdBufferRecycler&)  = delete;
        CmdBufferRecycler& operator = (      CmdBufferRecycler&&) = delete;

        CmdBufferRecycler(CmdBufferRecycler&& rhs) noexcept :
            vkCmdBuff {rhs.vkCmdBuff},
            Pool      {rhs.Pool     }
        {
            rhs.vkCmdBuff = VK_NULL_HANDLE;
            rhs.Pool      = nullptr;
        }
        // clang-format on

        ~CmdBufferRecycler()
        {
            if (Pool != nullptr)
            {
                Pool->RecycleCommandBuffer(std::move(vkCmdBuff));
            }
        }

    private:
        VkCommandBuffer                           vkCmdBuff = VK_NULL_HANDLE;
        VulkanUtilities::VulkanCommandBufferPool* Pool      = nullptr;
    };

    // Discard command buffer directly to the release queue since we know exactly which queue it was submitted to
    // as well as the associated FenceValue.
    auto& ReleaseQueue = m_pDevice->GetReleaseQueue(CmdQueue);
    ReleaseQueue.DiscardResource(CmdBufferRecycler{vkCmdBuff, *m_CmdPool}, FenceValue);
}

inline void DeviceContextVkImpl::DisposeCurrentCmdBuffer(SoftwareQueueIndex CmdQueue, Uint64 FenceValue)
{
    VERIFY(m_CommandBuffer.GetState().RenderPass == VK_NULL_HANDLE, "Disposing command buffer with unfinished render pass");
    auto vkCmdBuff = m_CommandBuffer.GetVkCmdBuffer();
    if (vkCmdBuff != VK_NULL_HANDLE)
    {
        DisposeVkCmdBuffer(CmdQueue, vkCmdBuff, FenceValue);
        m_CommandBuffer.Reset();
    }
}


void DeviceContextVkImpl::SetPipelineState(IPipelineState* pPipelineState)
{
    RefCntAutoPtr<PipelineStateVkImpl> pPipelineStateVk{pPipelineState, PipelineStateVkImpl::IID_InternalImpl};
    VERIFY(pPipelineState == nullptr || pPipelineStateVk != nullptr, "Unknown pipeline state object implementation");
    if (PipelineStateVkImpl::IsSameObject(m_pPipelineState, pPipelineStateVk))
        return;

    const auto& PSODesc = pPipelineStateVk->GetDesc();

    bool CommitStates  = false;
    bool CommitScissor = false;
    if (!m_pPipelineState)
    {
        // If no pipeline state is bound, we are working with the fresh command
        // list. We have to commit the states set in the context that are not
        // committed by the draw command (render targets, viewports, scissor rects, etc.)
        CommitStates = true;
    }
    else
    {
        const auto& OldPSODesc = m_pPipelineState->GetDesc();
        // Commit all graphics states when switching from non-graphics pipeline
        // This is necessary because if the command list had been flushed
        // and the first PSO set on the command list was a compute pipeline,
        // the states would otherwise never be committed (since m_pPipelineState != nullptr)
        CommitStates = !OldPSODesc.IsAnyGraphicsPipeline();
        // We also need to update scissor rect if ScissorEnable state was disabled in previous pipeline
        if (OldPSODesc.IsAnyGraphicsPipeline())
            CommitScissor = !m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable;
    }

    TDeviceContextBase::SetPipelineState(std::move(pPipelineStateVk), 0 /*Dummy*/);
    EnsureVkCmdBuffer();

    auto vkPipeline = m_pPipelineState->GetVkPipeline();

    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the switch below to handle the new pipeline type");
    switch (PSODesc.PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
        {
            auto& GraphicsPipeline = m_pPipelineState->GetGraphicsPipelineDesc();
            m_CommandBuffer.BindGraphicsPipeline(vkPipeline);

            if (CommitStates)
            {
                m_CommandBuffer.SetStencilReference(m_StencilRef);
                m_CommandBuffer.SetBlendConstants(m_BlendFactors);
                CommitViewports();
            }

            if (GraphicsPipeline.RasterizerDesc.ScissorEnable && (CommitStates || CommitScissor))
            {
                CommitScissorRects();
            }
            m_State.vkPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            m_State.NullRenderTargets =
                GraphicsPipeline.pRenderPass == nullptr &&
                GraphicsPipeline.NumRenderTargets == 0 &&
                GraphicsPipeline.DSVFormat == TEX_FORMAT_UNKNOWN;
            break;
        }
        case PIPELINE_TYPE_COMPUTE:
        {
            m_CommandBuffer.BindComputePipeline(vkPipeline);
            m_State.vkPipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            break;
        }
        case PIPELINE_TYPE_RAY_TRACING:
        {
            m_CommandBuffer.BindRayTracingPipeline(vkPipeline);
            m_State.vkPipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            break;
        }
        case PIPELINE_TYPE_TILE:
            UNEXPECTED("Unsupported pipeline type");
            break;
        default:
            UNEXPECTED("unknown pipeline type");
    }

    const auto& Layout    = m_pPipelineState->GetPipelineLayout();
    const auto  SignCount = m_pPipelineState->GetResourceSignatureCount();
    auto&       BindInfo  = GetBindInfo(PSODesc.PipelineType);

    Uint32 DvpCompatibleSRBCount = 0;
    PrepareCommittedResources(BindInfo, DvpCompatibleSRBCount);
#ifdef DILIGENT_DEVELOPMENT
    for (auto sign = DvpCompatibleSRBCount; sign < SignCount; ++sign)
    {
        // Do not clear DescriptorSetBaseInd and DynamicOffsetCount!
        BindInfo.SetInfo[sign].vkSets.fill(VK_NULL_HANDLE);
    }
#endif

    BindInfo.vkPipelineLayout = Layout.GetVkPipelineLayout();

    Uint32 TotalDynamicOffsetCount = 0;
    for (Uint32 i = 0; i < SignCount; ++i)
    {
        auto& SetInfo = BindInfo.SetInfo[i];

        auto* pSignature = m_pPipelineState->GetResourceSignature(i);
        if (pSignature == nullptr || pSignature->GetNumDescriptorSets() == 0)
        {
            SetInfo = {};
            continue;
        }

        VERIFY_EXPR(BindInfo.ActiveSRBMask & (1u << i));

        SetInfo.BaseInd            = Layout.GetFirstDescrSetIndex(pSignature->GetDesc().BindingIndex);
        SetInfo.DynamicOffsetCount = pSignature->GetDynamicOffsetCount();
        TotalDynamicOffsetCount += SetInfo.DynamicOffsetCount;
    }

    // Reserve space to store all dynamic buffer offsets
    m_DynamicBufferOffsets.resize(TotalDynamicOffsetCount);
}

DeviceContextVkImpl::ResourceBindInfo& DeviceContextVkImpl::GetBindInfo(PIPELINE_TYPE Type)
{
    VERIFY_EXPR(Type != PIPELINE_TYPE_INVALID);

    // clang-format off
    static_assert(PIPELINE_TYPE_GRAPHICS    == 0, "PIPELINE_TYPE_GRAPHICS == 0 is expected");
    static_assert(PIPELINE_TYPE_COMPUTE     == 1, "PIPELINE_TYPE_COMPUTE == 1 is expected");
    static_assert(PIPELINE_TYPE_MESH        == 2, "PIPELINE_TYPE_MESH == 2 is expected");
    static_assert(PIPELINE_TYPE_RAY_TRACING == 3, "PIPELINE_TYPE_RAY_TRACING == 3 is expected");
    static_assert(PIPELINE_TYPE_TILE        == 4, "PIPELINE_TYPE_TILE == 4 is expected");
    // clang-format on
    constexpr size_t Indices[] = {
        0, // PIPELINE_TYPE_GRAPHICS
        1, // PIPELINE_TYPE_COMPUTE
        0, // PIPELINE_TYPE_MESH
        2, // PIPELINE_TYPE_RAY_TRACING
        0, // PIPELINE_TYPE_TILE
    };
    static_assert(_countof(Indices) == Uint32{PIPELINE_TYPE_LAST} + 1, "Please add the new pipeline type to the list above");

    return m_BindInfo[Indices[Uint32{Type}]];
}

void DeviceContextVkImpl::CommitDescriptorSets(ResourceBindInfo& BindInfo, Uint32 CommitSRBMask)
{
    VERIFY(CommitSRBMask != 0, "This method should not be called when there is nothing to commit");

    const auto FirstSign = PlatformMisc::GetLSB(CommitSRBMask);
    const auto LastSign  = PlatformMisc::GetMSB(CommitSRBMask);
    VERIFY_EXPR(LastSign < m_pPipelineState->GetResourceSignatureCount());

    // Bind all descriptor sets in a single BindDescriptorSets call
    uint32_t   DynamicOffsetCount = 0;
    uint32_t   TotalSetCount      = 0;
    const auto FirstSetToBind     = BindInfo.SetInfo[FirstSign].BaseInd;
    for (Uint32 sign = FirstSign; sign <= LastSign; ++sign)
    {
        auto& SetInfo = BindInfo.SetInfo[sign];
        VERIFY(SetInfo.vkSets[0] != VK_NULL_HANDLE || (CommitSRBMask & (1u << sign)) == 0,
               "At least one descriptor set in the stale SRB must not be NULL. Empty SRBs should not be marked as stale by CommitShaderResources()");

        VERIFY((BindInfo.ActiveSRBMask & (1u << sign)) != 0 || SetInfo.vkSets[0] == VK_NULL_HANDLE, "Descriptor sets must be null for inactive slots");
        if (SetInfo.vkSets[0] == VK_NULL_HANDLE)
        {
            VERIFY_EXPR(SetInfo.vkSets[1] == VK_NULL_HANDLE);
            continue;
        }

        VERIFY_EXPR(SetInfo.BaseInd >= FirstSetToBind + TotalSetCount);
        while (FirstSetToBind + TotalSetCount < SetInfo.BaseInd)
            m_DescriptorSets[TotalSetCount++] = VK_NULL_HANDLE;

        const auto* pResourceCache = BindInfo.ResourceCaches[sign];
        DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at binding index ", sign, " is null, but corresponding descriptor set is not");

        m_DescriptorSets[TotalSetCount++] = SetInfo.vkSets[0];
        if (SetInfo.vkSets[1] != VK_NULL_HANDLE)
            m_DescriptorSets[TotalSetCount++] = SetInfo.vkSets[1];

        if (SetInfo.DynamicOffsetCount > 0)
        {
            VERIFY(m_DynamicBufferOffsets.size() >= size_t{DynamicOffsetCount} + size_t{SetInfo.DynamicOffsetCount},
                   "m_DynamicBufferOffsets must've been resized by SetPipelineState() to have enough space");

            auto NumOffsetsWritten = pResourceCache->GetDynamicBufferOffsets(GetContextId(), m_DynamicBufferOffsets, DynamicOffsetCount);
            VERIFY_EXPR(NumOffsetsWritten == SetInfo.DynamicOffsetCount);
            DynamicOffsetCount += SetInfo.DynamicOffsetCount;
        }

#ifdef DILIGENT_DEVELOPMENT
        SetInfo.LastBoundBaseInd = SetInfo.BaseInd;
#endif
    }

    // Note that there is one global dynamic buffer from which all dynamic resources are suballocated in Vulkan back-end,
    // and this buffer is not resizable, so the buffer handle can never change.

    // vkCmdBindDescriptorSets causes the sets numbered [firstSet .. firstSet+descriptorSetCount-1] to use the
    // bindings stored in pDescriptorSets[0 .. descriptorSetCount-1] for subsequent rendering commands
    // (either compute or graphics, according to the pipelineBindPoint). Any bindings that were previously
    // applied via these sets are no longer valid.
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkCmdBindDescriptorSets.html
    VERIFY_EXPR(m_State.vkPipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);
    m_CommandBuffer.BindDescriptorSets(m_State.vkPipelineBindPoint, BindInfo.vkPipelineLayout, FirstSetToBind, TotalSetCount,
                                       m_DescriptorSets.data(), DynamicOffsetCount, m_DynamicBufferOffsets.data());

    BindInfo.StaleSRBMask &= ~BindInfo.ActiveSRBMask;
}

#ifdef DILIGENT_DEVELOPMENT
void DeviceContextVkImpl::DvpValidateCommittedShaderResources(ResourceBindInfo& BindInfo)
{
    if (BindInfo.ResourcesValidated)
        return;

    DvpVerifySRBCompatibility(BindInfo);

    const auto SignCount = m_pPipelineState->GetResourceSignatureCount();
    for (Uint32 i = 0; i < SignCount; ++i)
    {
        const auto* pSign = m_pPipelineState->GetResourceSignature(i);
        if (pSign == nullptr || pSign->GetNumDescriptorSets() == 0)
            continue; // Skip null and empty signatures

        DEV_CHECK_ERR((BindInfo.StaleSRBMask & BindInfo.ActiveSRBMask) == 0, "CommitDescriptorSets() must be called before validation.");

        const auto& SetInfo = BindInfo.SetInfo[i];
        const auto  DSCount = pSign->GetNumDescriptorSets();
        for (Uint32 s = 0; s < DSCount; ++s)
        {
            DEV_CHECK_ERR(SetInfo.vkSets[s] != VK_NULL_HANDLE,
                          "descriptor set with index ", s, " is not bound for resource signature '",
                          pSign->GetDesc().Name, "', binding index ", i, ".");
        }

        DEV_CHECK_ERR(SetInfo.LastBoundBaseInd == SetInfo.BaseInd,
                      "Shader resource binding at index ", i, " has descriptor set base offset ", SetInfo.BaseInd,
                      ", but currently bound descriptor sets have base offset ", SetInfo.LastBoundBaseInd,
                      "; one of the resource signatures with lower binding index is not compatible.");
    }

    m_pPipelineState->DvpVerifySRBResources(this, BindInfo.ResourceCaches);

    BindInfo.ResourcesValidated = true;
}
#endif

void DeviceContextVkImpl::TransitionShaderResources(IShaderResourceBinding* pShaderResourceBinding)
{
    DEV_CHECK_ERR(!m_pActiveRenderPass, "State transitions are not allowed inside a render pass.");
    DEV_CHECK_ERR(pShaderResourceBinding != nullptr, "pShaderResourceBinding must not be null");

    auto* pResBindingVkImpl = ClassPtrCast<ShaderResourceBindingVkImpl>(pShaderResourceBinding);
    auto& ResourceCache     = pResBindingVkImpl->GetResourceCache();

    ResourceCache.TransitionResources<false>(this);
}

void DeviceContextVkImpl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0 /*Dummy*/);

    auto* pResBindingVkImpl = ClassPtrCast<ShaderResourceBindingVkImpl>(pShaderResourceBinding);
    auto& ResourceCache     = pResBindingVkImpl->GetResourceCache();
    if (ResourceCache.GetNumDescriptorSets() == 0)
    {
        // Ignore SRBs that contain no resources
        return;
    }

#ifdef DILIGENT_DEBUG
    ResourceCache.DbgVerifyDynamicBuffersCounter();
#endif

    if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        ResourceCache.TransitionResources<false>(this);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        ResourceCache.TransitionResources<true>(this);
    }
#endif

    const auto  SRBIndex   = pResBindingVkImpl->GetBindingIndex();
    const auto* pSignature = pResBindingVkImpl->GetSignature();
    auto&       BindInfo   = GetBindInfo(pResBindingVkImpl->GetPipelineType());
    auto&       SetInfo    = BindInfo.SetInfo[SRBIndex];

    BindInfo.Set(SRBIndex, pResBindingVkImpl);
    // We must not clear entire ResInfo as DescriptorSetBaseInd and DynamicOffsetCount
    // are set by SetPipelineState().
    SetInfo.vkSets = {};

    Uint32 DSIndex = 0;
    if (pSignature->HasDescriptorSet(PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE))
    {
        VERIFY_EXPR(DSIndex == pSignature->GetDescriptorSetIndex<PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE>());
        const auto& CachedDescrSet = const_cast<const ShaderResourceCacheVk&>(ResourceCache).GetDescriptorSet(DSIndex);
        VERIFY_EXPR(CachedDescrSet.GetVkDescriptorSet() != VK_NULL_HANDLE);
        SetInfo.vkSets[DSIndex] = CachedDescrSet.GetVkDescriptorSet();
        ++DSIndex;
    }

    if (pSignature->HasDescriptorSet(PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC))
    {
        VERIFY_EXPR(DSIndex == pSignature->GetDescriptorSetIndex<PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC>());
        VERIFY_EXPR(const_cast<const ShaderResourceCacheVk&>(ResourceCache).GetDescriptorSet(DSIndex).GetVkDescriptorSet() == VK_NULL_HANDLE);

        const auto vkLayout = pSignature->GetVkDescriptorSetLayout(PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC);

        VkDescriptorSet vkDynamicDescrSet   = VK_NULL_HANDLE;
        const char*     DynamicDescrSetName = "Dynamic Descriptor Set";
#ifdef DILIGENT_DEVELOPMENT
        String _DynamicDescrSetName{DynamicDescrSetName};
        _DynamicDescrSetName.append(" (");
        _DynamicDescrSetName.append(pSignature->GetDesc().Name);
        _DynamicDescrSetName += ')';
        DynamicDescrSetName = _DynamicDescrSetName.c_str();
#endif
        // Allocate vulkan descriptor set for dynamic resources
        vkDynamicDescrSet = AllocateDynamicDescriptorSet(vkLayout, DynamicDescrSetName);

        // Write all dynamic resource descriptors
        pSignature->CommitDynamicResources(ResourceCache, vkDynamicDescrSet);

        SetInfo.vkSets[DSIndex] = vkDynamicDescrSet;
        ++DSIndex;
    }

    VERIFY_EXPR(DSIndex == ResourceCache.GetNumDescriptorSets());
}

void DeviceContextVkImpl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        EnsureVkCmdBuffer();
        m_CommandBuffer.SetStencilReference(m_StencilRef);
    }
}

void DeviceContextVkImpl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        EnsureVkCmdBuffer();
        m_CommandBuffer.SetBlendConstants(m_BlendFactors);
    }
}

void DeviceContextVkImpl::CommitVkVertexBuffers()
{
#ifdef DILIGENT_DEVELOPMENT
    if (m_NumVertexStreams < m_pPipelineState->GetNumBufferSlotsUsed())
        LOG_ERROR("Currently bound pipeline state '", m_pPipelineState->GetDesc().Name, "' expects ", m_pPipelineState->GetNumBufferSlotsUsed(), " input buffer slots, but only ", m_NumVertexStreams, " is bound");
#endif
    // Do not initialize array with zeros for performance reasons
    VkBuffer     vkVertexBuffers[MAX_BUFFER_SLOTS]; // = {}
    VkDeviceSize Offsets[MAX_BUFFER_SLOTS];
    VERIFY(m_NumVertexStreams <= MAX_BUFFER_SLOTS, "Too many buffers are being set");
    bool DynamicBufferPresent = false;
    for (Uint32 slot = 0; slot < m_NumVertexStreams; ++slot)
    {
        auto& CurrStream = m_VertexStreams[slot];
        if (auto* pBufferVk = CurrStream.pBuffer.RawPtr())
        {
            if (pBufferVk->GetDesc().Usage == USAGE_DYNAMIC)
            {
                DynamicBufferPresent = true;
#ifdef DILIGENT_DEVELOPMENT
                pBufferVk->DvpVerifyDynamicAllocation(this);
#endif
            }

            // Device context keeps strong references to all vertex buffers.

            vkVertexBuffers[slot] = pBufferVk->GetVkBuffer();
            Offsets[slot]         = CurrStream.Offset + pBufferVk->GetDynamicOffset(GetContextId(), this);
        }
        else
        {
            // We can't bind null vertex buffer in Vulkan and have to use a dummy one
            vkVertexBuffers[slot] = m_DummyVB->GetVkBuffer();
            Offsets[slot]         = 0;
        }
    }

    //GraphCtx.FlushResourceBarriers();
    if (m_NumVertexStreams > 0)
        m_CommandBuffer.BindVertexBuffers(0, m_NumVertexStreams, vkVertexBuffers, Offsets);

    // GPU offset for a dynamic vertex buffer can change every time a draw command is invoked
    m_State.CommittedVBsUpToDate = !DynamicBufferPresent;
}

void DeviceContextVkImpl::DvpLogRenderPass_PSOMismatch()
{
    const auto& Desc       = m_pPipelineState->GetDesc();
    const auto& GrPipeline = m_pPipelineState->GetGraphicsPipelineDesc();

    std::stringstream ss;
    ss << "Active render pass is incompatible with PSO '" << Desc.Name
       << "'. This indicates the mismatch between the number and/or format of bound render "
          "targets and/or depth stencil buffer and the PSO. Vulkan requires exact match.\n"
          "    Bound render targets ("
       << m_NumBoundRenderTargets << "):";
    Uint32 SampleCount = 0;
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        ss << ' ';
        if (auto* pRTV = m_pBoundRenderTargets[rt].RawPtr())
        {
            VERIFY_EXPR(SampleCount == 0 || SampleCount == pRTV->GetTexture()->GetDesc().SampleCount);
            SampleCount = pRTV->GetTexture()->GetDesc().SampleCount;
            ss << GetTextureFormatAttribs(pRTV->GetDesc().Format).Name;
        }
        else
            ss << "<Not set>";
    }
    ss << "; DSV: ";
    if (m_pBoundDepthStencil)
    {
        VERIFY_EXPR(SampleCount == 0 || SampleCount == m_pBoundDepthStencil->GetTexture()->GetDesc().SampleCount);
        SampleCount = m_pBoundDepthStencil->GetTexture()->GetDesc().SampleCount;
        ss << GetTextureFormatAttribs(m_pBoundDepthStencil->GetDesc().Format).Name;
        if (m_pBoundDepthStencil->GetDesc().ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL)
            ss << " (read-only)";
    }
    else
        ss << "<Not set>";
    ss << "; Sample count: " << SampleCount;

    if (m_pBoundShadingRateMap)
        ss << "; VRS";

    ss << "\n    PSO: render targets (" << Uint32{GrPipeline.NumRenderTargets} << "): ";
    for (Uint32 rt = 0; rt < GrPipeline.NumRenderTargets; ++rt)
        ss << ' ' << GetTextureFormatAttribs(GrPipeline.RTVFormats[rt]).Name;
    ss << "; DSV: " << GetTextureFormatAttribs(GrPipeline.DSVFormat).Name;
    if (GrPipeline.ReadOnlyDSV)
        ss << " (read-only)";
    ss << "; Sample count: " << Uint32{GrPipeline.SmplDesc.Count};

    if (GrPipeline.ShadingRateFlags & PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED)
        ss << "; VRS";

    LOG_ERROR_MESSAGE(ss.str());
}

void DeviceContextVkImpl::PrepareForDraw(DRAW_FLAGS Flags)
{
    if (m_vkFramebuffer == VK_NULL_HANDLE && m_State.NullRenderTargets)
    {
        DEV_CHECK_ERR(m_FramebufferWidth > 0 && m_FramebufferHeight > 0,
                      "Framebuffer width/height is zero. Call SetViewports to set the framebuffer sizes when no render targets are set.");
        ChooseRenderPassAndFramebuffer();
    }

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();

    VERIFY(m_vkRenderPass != VK_NULL_HANDLE, "No render pass is active while executing draw command");
    VERIFY(m_vkFramebuffer != VK_NULL_HANDLE, "No framebuffer is bound while executing draw command");
#endif

    EnsureVkCmdBuffer();

    if (!m_State.CommittedVBsUpToDate && m_pPipelineState->GetNumBufferSlotsUsed() > 0)
    {
        CommitVkVertexBuffers();
    }

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        for (Uint32 slot = 0; slot < m_NumVertexStreams; ++slot)
        {
            if (auto* pBufferVk = m_VertexStreams[slot].pBuffer.RawPtr())
            {
                DvpVerifyBufferState(*pBufferVk, RESOURCE_STATE_VERTEX_BUFFER, "Using vertex buffers (DeviceContextVkImpl::Draw)");
            }
        }
    }
#endif

    auto& BindInfo = GetBindInfo(PIPELINE_TYPE_GRAPHICS);
    // First time we must always bind descriptor sets with dynamic offsets as SRBs are stale.
    // If there are no dynamic buffers bound in the resource cache, for all subsequent
    // calls we do not need to bind the sets again.
    if (Uint32 CommitMask = BindInfo.GetCommitMask(Flags & DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT))
    {
        CommitDescriptorSets(BindInfo, CommitMask);
    }
#ifdef DILIGENT_DEVELOPMENT
    // Must be called after CommitDescriptorSets as it needs SetInfo.BaseInd
    DvpValidateCommittedShaderResources(BindInfo);
#endif

    if (m_pPipelineState->GetGraphicsPipelineDesc().pRenderPass == nullptr)
    {
#ifdef DILIGENT_DEVELOPMENT
        if (m_pPipelineState->GetRenderPass()->GetVkRenderPass() != m_vkRenderPass)
        {
            // Note that different Vulkan render passes may still be compatible,
            // so we should only verify implicit render passes
            DvpLogRenderPass_PSOMismatch();
        }
#endif

        CommitRenderPassAndFramebuffer((Flags & DRAW_FLAG_VERIFY_STATES) != 0);
    }
}

BufferVkImpl* DeviceContextVkImpl::PrepareIndirectAttribsBuffer(IBuffer*                       pAttribsBuffer,
                                                                RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                                const char*                    OpName)
{
    DEV_CHECK_ERR(pAttribsBuffer, "Indirect draw attribs buffer must not be null");
    auto* pIndirectDrawAttribsVk = ClassPtrCast<BufferVkImpl>(pAttribsBuffer);

#ifdef DILIGENT_DEVELOPMENT
    if (pIndirectDrawAttribsVk->GetDesc().Usage == USAGE_DYNAMIC)
        pIndirectDrawAttribsVk->DvpVerifyDynamicAllocation(this);
#endif

    // Buffer memory barriers must be executed outside of render pass
    TransitionOrVerifyBufferState(*pIndirectDrawAttribsVk, TransitionMode, RESOURCE_STATE_INDIRECT_ARGUMENT,
                                  VK_ACCESS_INDIRECT_COMMAND_READ_BIT, OpName);
    return pIndirectDrawAttribsVk;
}

void DeviceContextVkImpl::PrepareForIndexedDraw(DRAW_FLAGS Flags, VALUE_TYPE IndexType)
{
    PrepareForDraw(Flags);

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        DvpVerifyBufferState(*m_pIndexBuffer, RESOURCE_STATE_INDEX_BUFFER, "Indexed draw call (DeviceContextVkImpl::Draw)");
    }
#endif
    DEV_CHECK_ERR(IndexType == VT_UINT16 || IndexType == VT_UINT32, "Unsupported index format. Only R16_UINT and R32_UINT are allowed.");
    VkIndexType vkIndexType = TypeToVkIndexType(IndexType);
    m_CommandBuffer.BindIndexBuffer(m_pIndexBuffer->GetVkBuffer(), m_IndexDataStartOffset + m_pIndexBuffer->GetDynamicOffset(GetContextId(), this), vkIndexType);
}

void DeviceContextVkImpl::Draw(const DrawAttribs& Attribs)
{
    TDeviceContextBase::Draw(Attribs, 0);

    PrepareForDraw(Attribs.Flags);

    if (Attribs.NumVertices > 0 && Attribs.NumInstances > 0)
    {
        m_CommandBuffer.Draw(Attribs.NumVertices, Attribs.NumInstances, Attribs.StartVertexLocation, Attribs.FirstInstanceLocation);
        ++m_State.NumCommands;
    }
}

void DeviceContextVkImpl::MultiDraw(const MultiDrawAttribs& Attribs)
{
    TDeviceContextBase::MultiDraw(Attribs, 0);

    PrepareForDraw(Attribs.Flags);

    if (Attribs.NumInstances == 0)
        return;

    if (m_NativeMultiDrawSupported)
    {
        m_ScratchSpace.resize(sizeof(VkMultiDrawInfoEXT) * Attribs.DrawCount);
        VkMultiDrawInfoEXT* pDrawInfo = reinterpret_cast<VkMultiDrawInfoEXT*>(m_ScratchSpace.data());

        Uint32 DrawCount = 0;
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumVertices > 0)
            {
                pDrawInfo[i] = {Item.StartVertexLocation, Item.NumVertices};
                ++DrawCount;
            }
        }
        if (DrawCount > 0)
        {
            m_CommandBuffer.MultiDraw(DrawCount, pDrawInfo, Attribs.NumInstances, Attribs.FirstInstanceLocation);
        }
    }
    else
    {
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumVertices > 0)
            {
                m_CommandBuffer.Draw(Item.NumVertices, Attribs.NumInstances, Item.StartVertexLocation, Attribs.FirstInstanceLocation);
                ++m_State.NumCommands;
            }
        }
    }
}

void DeviceContextVkImpl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    TDeviceContextBase::DrawIndexed(Attribs, 0);

    PrepareForIndexedDraw(Attribs.Flags, Attribs.IndexType);

    if (Attribs.NumIndices > 0 && Attribs.NumInstances > 0)
    {
        m_CommandBuffer.DrawIndexed(Attribs.NumIndices, Attribs.NumInstances, Attribs.FirstIndexLocation, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
        ++m_State.NumCommands;
    }
}

void DeviceContextVkImpl::MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs)
{
    TDeviceContextBase::MultiDrawIndexed(Attribs, 0);

    PrepareForIndexedDraw(Attribs.Flags, Attribs.IndexType);

    if (Attribs.NumInstances == 0)
        return;

    if (m_NativeMultiDrawSupported)
    {
        m_ScratchSpace.resize(sizeof(VkMultiDrawIndexedInfoEXT) * Attribs.DrawCount);
        VkMultiDrawIndexedInfoEXT* pDrawInfo = reinterpret_cast<VkMultiDrawIndexedInfoEXT*>(m_ScratchSpace.data());

        Uint32 DrawCount = 0;
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumIndices > 0)
            {
                pDrawInfo[i] = {Item.FirstIndexLocation, Item.NumIndices, static_cast<int32_t>(Item.BaseVertex)};
                ++DrawCount;
            }
        }
        if (DrawCount > 0)
        {
            m_CommandBuffer.MultiDrawIndexed(DrawCount, pDrawInfo, Attribs.NumInstances, Attribs.FirstInstanceLocation);
        }
    }
    else
    {
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumIndices > 0)
            {
                m_CommandBuffer.DrawIndexed(Item.NumIndices, Attribs.NumInstances, Item.FirstIndexLocation, Item.BaseVertex, Attribs.FirstInstanceLocation);
                ++m_State.NumCommands;
            }
        }
    }
}

void DeviceContextVkImpl::DrawIndirect(const DrawIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawIndirect(Attribs, 0);

    // We must prepare indirect draw attribs buffer first because state transitions must
    // be performed outside of render pass, and PrepareForDraw commits render pass
    BufferVkImpl* pIndirectDrawAttribsVk = PrepareIndirectAttribsBuffer(Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, "Indirect draw (DeviceContextVkImpl::DrawIndirect)");
    BufferVkImpl* pCountBufferVk         = Attribs.pCounterBuffer != nullptr ?
        PrepareIndirectAttribsBuffer(Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, "Count buffer (DeviceContextVkImpl::DrawIndirect)") :
        nullptr;

    PrepareForDraw(Attribs.Flags);

    if (Attribs.DrawCount > 0)
    {
        if (Attribs.pCounterBuffer == nullptr)
        {
            m_CommandBuffer.DrawIndirect(pIndirectDrawAttribsVk->GetVkBuffer(),
                                         pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                         Attribs.DrawCount, Attribs.DrawCount > 1 ? Attribs.DrawArgsStride : 0);
        }
        else
        {
            m_CommandBuffer.DrawIndirectCount(pIndirectDrawAttribsVk->GetVkBuffer(),
                                              pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                              pCountBufferVk->GetVkBuffer(),
                                              pCountBufferVk->GetDynamicOffset(GetContextId(), this) + Attribs.CounterOffset,
                                              Attribs.DrawCount,
                                              Attribs.DrawArgsStride);
        }
    }

    ++m_State.NumCommands;
}

void DeviceContextVkImpl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawIndexedIndirect(Attribs, 0);

    // We must prepare indirect draw attribs buffer first because state transitions must
    // be performed outside of render pass, and PrepareForDraw commits render pass
    BufferVkImpl* pIndirectDrawAttribsVk = PrepareIndirectAttribsBuffer(Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, "Indirect draw (DeviceContextVkImpl::DrawIndexedIndirect)");
    BufferVkImpl* pCountBufferVk         = Attribs.pCounterBuffer != nullptr ?
        PrepareIndirectAttribsBuffer(Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, "Count buffer (DeviceContextVkImpl::DrawIndexedIndirect)") :
        nullptr;

    PrepareForIndexedDraw(Attribs.Flags, Attribs.IndexType);

    if (Attribs.DrawCount > 0)
    {
        if (Attribs.pCounterBuffer == nullptr)
        {
            m_CommandBuffer.DrawIndexedIndirect(pIndirectDrawAttribsVk->GetVkBuffer(),
                                                pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                                Attribs.DrawCount, Attribs.DrawCount > 1 ? Attribs.DrawArgsStride : 0);
        }
        else
        {
            m_CommandBuffer.DrawIndexedIndirectCount(pIndirectDrawAttribsVk->GetVkBuffer(),
                                                     pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                                     pCountBufferVk->GetVkBuffer(),
                                                     pCountBufferVk->GetDynamicOffset(GetContextId(), this) + Attribs.CounterOffset,
                                                     Attribs.DrawCount,
                                                     Attribs.DrawArgsStride);
        }
    }

    ++m_State.NumCommands;
}

void DeviceContextVkImpl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    TDeviceContextBase::DrawMesh(Attribs, 0);

    PrepareForDraw(Attribs.Flags);

    if (Attribs.ThreadGroupCountX > 0 && Attribs.ThreadGroupCountY > 0 && Attribs.ThreadGroupCountZ > 0)
    {
        m_CommandBuffer.DrawMesh(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
        ++m_State.NumCommands;
    }
}

void DeviceContextVkImpl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawMeshIndirect(Attribs, 0);

    // We must prepare indirect draw attribs buffer first because state transitions must
    // be performed outside of render pass, and PrepareForDraw commits render pass
    BufferVkImpl* pIndirectDrawAttribsVk = PrepareIndirectAttribsBuffer(Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, "Indirect draw (DeviceContextVkImpl::DrawMeshIndirect)");
    BufferVkImpl* pCountBufferVk         = Attribs.pCounterBuffer != nullptr ?
        PrepareIndirectAttribsBuffer(Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, "Counter buffer (DeviceContextVkImpl::DrawMeshIndirect)") :
        nullptr;

    PrepareForDraw(Attribs.Flags);

    if (Attribs.CommandCount > 0)
    {
        if (Attribs.pCounterBuffer == nullptr)
        {
            m_CommandBuffer.DrawMeshIndirect(pIndirectDrawAttribsVk->GetVkBuffer(),
                                             pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                             Attribs.CommandCount,
                                             DrawMeshIndirectCommandStride);
        }
        else
        {
            m_CommandBuffer.DrawMeshIndirectCount(pIndirectDrawAttribsVk->GetVkBuffer(),
                                                  pIndirectDrawAttribsVk->GetDynamicOffset(GetContextId(), this) + Attribs.DrawArgsOffset,
                                                  pCountBufferVk->GetVkBuffer(),
                                                  pCountBufferVk->GetDynamicOffset(GetContextId(), this) + Attribs.CounterOffset,
                                                  Attribs.CommandCount,
                                                  DrawMeshIndirectCommandStride);
        }
    }

    ++m_State.NumCommands;
}

void DeviceContextVkImpl::PrepareForDispatchCompute()
{
    EnsureVkCmdBuffer();

    // Dispatch commands must be executed outside of render pass
    if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
        m_CommandBuffer.EndRenderPass();

    auto& BindInfo = GetBindInfo(PIPELINE_TYPE_COMPUTE);
    if (Uint32 CommitMask = BindInfo.GetCommitMask())
    {
        CommitDescriptorSets(BindInfo, CommitMask);
    }

#ifdef DILIGENT_DEVELOPMENT
    // Must be called after CommitDescriptorSets as it needs SetInfo.BaseInd
    DvpValidateCommittedShaderResources(BindInfo);
#endif
}

void DeviceContextVkImpl::PrepareForRayTracing()
{
    EnsureVkCmdBuffer();

    auto& BindInfo = GetBindInfo(PIPELINE_TYPE_RAY_TRACING);
    if (Uint32 CommitMask = BindInfo.GetCommitMask())
    {
        CommitDescriptorSets(BindInfo, CommitMask);
    }

#ifdef DILIGENT_DEVELOPMENT
    // Must be called after CommitDescriptorSets as it needs SetInfo.BaseInd
    DvpValidateCommittedShaderResources(BindInfo);
#endif
}

void DeviceContextVkImpl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    TDeviceContextBase::DispatchCompute(Attribs, 0);

    PrepareForDispatchCompute();

    if (Attribs.ThreadGroupCountX > 0 && Attribs.ThreadGroupCountY > 0 && Attribs.ThreadGroupCountZ > 0)
    {
        m_CommandBuffer.Dispatch(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
        ++m_State.NumCommands;
    }
}

void DeviceContextVkImpl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs)
{
    TDeviceContextBase::DispatchComputeIndirect(Attribs, 0);

    PrepareForDispatchCompute();

    auto* pBufferVk = ClassPtrCast<BufferVkImpl>(Attribs.pAttribsBuffer);

#ifdef DILIGENT_DEVELOPMENT
    if (pBufferVk->GetDesc().Usage == USAGE_DYNAMIC)
        pBufferVk->DvpVerifyDynamicAllocation(this);
#endif

    // Buffer memory barriers must be executed outside of render pass
    TransitionOrVerifyBufferState(*pBufferVk, Attribs.AttribsBufferStateTransitionMode, RESOURCE_STATE_INDIRECT_ARGUMENT,
                                  VK_ACCESS_INDIRECT_COMMAND_READ_BIT, "Indirect dispatch (DeviceContextVkImpl::DispatchCompute)");

    m_CommandBuffer.DispatchIndirect(pBufferVk->GetVkBuffer(), pBufferVk->GetDynamicOffset(GetContextId(), this) + Attribs.DispatchArgsByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::GetTileSize(Uint32& TileSizeX, Uint32& TileSizeY)
{
    TileSizeX = 0;
    TileSizeY = 0;

    if (m_vkRenderPass != VK_NULL_HANDLE)
    {
        const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
        VkExtent2D  Granularity   = {};
        vkGetRenderAreaGranularity(LogicalDevice.GetVkDevice(), m_vkRenderPass, &Granularity);

        TileSizeX = Granularity.width;
        TileSizeY = Granularity.height;
    }
}


void DeviceContextVkImpl::ClearDepthStencil(ITextureView*                  pView,
                                            CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                            float                          fDepth,
                                            Uint8                          Stencil,
                                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::ClearDepthStencil(pView);

    auto* pVkDSV = ClassPtrCast<ITextureViewVk>(pView);

    EnsureVkCmdBuffer();

    const auto& ViewDesc = pVkDSV->GetDesc();
    VERIFY(ViewDesc.TextureDim != RESOURCE_DIM_TEX_3D, "Depth-stencil view of a 3D texture should've been created as 2D texture array view");

    bool ClearAsAttachment = pVkDSV == m_pBoundDepthStencil;
    VERIFY(m_pActiveRenderPass == nullptr || ClearAsAttachment,
           "DSV was not found in the framebuffer. This is unexpected because TDeviceContextBase::ClearDepthStencil "
           "checks if the DSV is bound as a framebuffer attachment and triggers an assert otherwise (in development mode).");
    if (ClearAsAttachment)
    {
        VERIFY_EXPR(m_vkRenderPass != VK_NULL_HANDLE && m_vkFramebuffer != VK_NULL_HANDLE);
        if (m_pActiveRenderPass == nullptr)
        {
            // Render pass may not be currently committed

            TransitionRenderTargets(StateTransitionMode);
            // No need to verify states again
            CommitRenderPassAndFramebuffer(false);
        }

        VkClearAttachment ClearAttachment = {};
        ClearAttachment.aspectMask        = 0;
        if (ClearFlags & CLEAR_DEPTH_FLAG) ClearAttachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if (ClearFlags & CLEAR_STENCIL_FLAG) ClearAttachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT is set in aspectMask
        ClearAttachment.colorAttachment                 = VK_ATTACHMENT_UNUSED;
        ClearAttachment.clearValue.depthStencil.depth   = fDepth;
        ClearAttachment.clearValue.depthStencil.stencil = Stencil;
        VkClearRect ClearRect;
        // m_FramebufferWidth, m_FramebufferHeight are scaled to the proper mip level
        ClearRect.rect = {{0, 0}, {m_FramebufferWidth, m_FramebufferHeight}};
        // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of
        // the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice
        ClearRect.baseArrayLayer = 0;
        ClearRect.layerCount     = ViewDesc.NumArraySlices;
        // No memory barriers are needed between vkCmdClearAttachments and preceding or
        // subsequent draw or attachment clear commands in the same subpass (17.2)
        m_CommandBuffer.ClearAttachment(ClearAttachment, ClearRect);
    }
    else
    {
        // End render pass to clear the buffer with vkCmdClearDepthStencilImage
        if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
            m_CommandBuffer.EndRenderPass();

        auto* pTexture   = pVkDSV->GetTexture();
        auto* pTextureVk = ClassPtrCast<TextureVkImpl>(pTexture);

        // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
        TransitionOrVerifyTextureState(*pTextureVk, StateTransitionMode, RESOURCE_STATE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       "Clearing depth-stencil buffer outside of render pass (DeviceContextVkImpl::ClearDepthStencil)");

        VkClearDepthStencilValue ClearValue;
        ClearValue.depth   = fDepth;
        ClearValue.stencil = Stencil;
        VkImageSubresourceRange Subresource;
        Subresource.aspectMask = 0;
        if (ClearFlags & CLEAR_DEPTH_FLAG) Subresource.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if (ClearFlags & CLEAR_STENCIL_FLAG) Subresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        // We are clearing the image, not image view with vkCmdClearDepthStencilImage
        Subresource.baseArrayLayer = ViewDesc.FirstArraySlice;
        Subresource.layerCount     = ViewDesc.NumArraySlices;
        Subresource.baseMipLevel   = ViewDesc.MostDetailedMip;
        Subresource.levelCount     = ViewDesc.NumMipLevels;

        m_CommandBuffer.ClearDepthStencilImage(pTextureVk->GetVkImage(), ClearValue, Subresource);
    }

    ++m_State.NumCommands;
}

VkClearColorValue ClearValueToVkClearValue(const void* RGBA, TEXTURE_FORMAT TexFmt)
{
    VkClearColorValue ClearValue;
    const auto&       FmtAttribs = GetTextureFormatAttribs(TexFmt);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_SINT)
    {
        for (int i = 0; i < 4; ++i)
            ClearValue.int32[i] = static_cast<const int32_t*>(RGBA)[i];
    }
    else if (FmtAttribs.ComponentType == COMPONENT_TYPE_UINT)
    {
        for (int i = 0; i < 4; ++i)
            ClearValue.uint32[i] = static_cast<const uint32_t*>(RGBA)[i];
    }
    else
    {
        for (int i = 0; i < 4; ++i)
            ClearValue.float32[i] = static_cast<const float*>(RGBA)[i];
    }

    return ClearValue;
}

void DeviceContextVkImpl::ClearRenderTarget(ITextureView* pView, const void* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::ClearRenderTarget(pView);

    auto* pVkRTV = ClassPtrCast<ITextureViewVk>(pView);

    static constexpr float Zero[4] = {0.f, 0.f, 0.f, 0.f};
    if (RGBA == nullptr)
        RGBA = Zero;

    EnsureVkCmdBuffer();

    const auto& ViewDesc = pVkRTV->GetDesc();
    VERIFY(ViewDesc.TextureDim != RESOURCE_DIM_TEX_3D, "Render target view of a 3D texture should've been created as 2D texture array view");

    // Check if the texture is one of the currently bound render targets
    static constexpr const Uint32 InvalidAttachmentIndex = ~Uint32{0};

    Uint32 attachmentIndex = InvalidAttachmentIndex;
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (m_pBoundRenderTargets[rt] == pVkRTV)
        {
            attachmentIndex = rt;
            break;
        }
    }

    VERIFY(m_pActiveRenderPass == nullptr || attachmentIndex != InvalidAttachmentIndex,
           "Render target was not found in the framebuffer. This is unexpected because TDeviceContextBase::ClearRenderTarget "
           "checks if the RTV is bound as a framebuffer attachment and triggers an assert otherwise (in development mode).");

    if (attachmentIndex != InvalidAttachmentIndex)
    {
        VERIFY_EXPR(m_vkRenderPass != VK_NULL_HANDLE && m_vkFramebuffer != VK_NULL_HANDLE);
        if (m_pActiveRenderPass == nullptr)
        {
            // Render pass may not be currently committed

            TransitionRenderTargets(StateTransitionMode);
            // No need to verify states again
            CommitRenderPassAndFramebuffer(false);
        }

        VkClearAttachment ClearAttachment = {};
        ClearAttachment.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT is set in aspectMask,
        // in which case it is an index to the pColorAttachments array in the VkSubpassDescription
        // structure of the current subpass which selects the color attachment to clear (17.2)
        // It is NOT the render pass attachment index
        ClearAttachment.colorAttachment  = attachmentIndex;
        ClearAttachment.clearValue.color = ClearValueToVkClearValue(RGBA, ViewDesc.Format);
        VkClearRect ClearRect;
        // m_FramebufferWidth, m_FramebufferHeight are scaled to the proper mip level
        ClearRect.rect = {{0, 0}, {m_FramebufferWidth, m_FramebufferHeight}};
        // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of
        // the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice
        ClearRect.baseArrayLayer = 0;
        ClearRect.layerCount     = ViewDesc.NumArraySlices;
        // No memory barriers are needed between vkCmdClearAttachments and preceding or
        // subsequent draw or attachment clear commands in the same subpass (17.2)
        m_CommandBuffer.ClearAttachment(ClearAttachment, ClearRect);
    }
    else
    {
        VERIFY(m_pActiveRenderPass == nullptr, "This branch should never execute inside a render pass.");

        // End current render pass and clear the image with vkCmdClearColorImage
        if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
            m_CommandBuffer.EndRenderPass();

        auto* pTexture   = pVkRTV->GetTexture();
        auto* pTextureVk = ClassPtrCast<TextureVkImpl>(pTexture);

        // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
        TransitionOrVerifyTextureState(*pTextureVk, StateTransitionMode, RESOURCE_STATE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       "Clearing render target outside of render pass (DeviceContextVkImpl::ClearRenderTarget)");

        auto ClearValue = ClearValueToVkClearValue(RGBA, ViewDesc.Format);

        VkImageSubresourceRange Subresource;
        Subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // We are clearing the image, not image view with vkCmdClearColorImage
        Subresource.baseArrayLayer = ViewDesc.FirstArraySlice;
        Subresource.layerCount     = ViewDesc.NumArraySlices;
        Subresource.baseMipLevel   = ViewDesc.MostDetailedMip;
        Subresource.levelCount     = ViewDesc.NumMipLevels;
        VERIFY(ViewDesc.NumMipLevels, "RTV must contain single mip level");

        m_CommandBuffer.ClearColorImage(pTextureVk->GetVkImage(), ClearValue, Subresource);
    }

    ++m_State.NumCommands;
}

void DeviceContextVkImpl::FinishFrame()
{
#ifdef DILIGENT_DEBUG
    for (const auto& MappedBuffIt : m_DbgMappedBuffers)
    {
        const auto& BuffDesc = MappedBuffIt.first->GetDesc();
        if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            LOG_WARNING_MESSAGE("Dynamic buffer '", BuffDesc.Name, "' is still mapped when finishing the frame. The contents of the buffer and mapped address will become invalid");
        }
    }
#endif

    if (GetNumCommandsInCtx() != 0)
    {
        if (IsDeferred())
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in deferred device context #", GetContextId(),
                              " when finishing the frame. This is an error and may cause unpredicted behaviour."
                              " Close all deferred contexts and execute them before finishing the frame.");
        }
        else
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in the immediate device context when finishing the frame."
                              " This is an error and may cause unpredicted behaviour. Call Flush() to submit all commands"
                              " for execution before finishing the frame.");
        }
    }

    if (m_ActiveQueriesCounter > 0)
    {
        LOG_ERROR_MESSAGE("There are ", m_ActiveQueriesCounter,
                          " active queries in the device context when finishing the frame. "
                          "All queries must be ended before the frame is finished.");
    }

    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Finishing frame inside an active render pass.");
    }

    if (!m_MappedTextures.empty())
        LOG_ERROR_MESSAGE("There are mapped textures in the device context when finishing the frame. All dynamic resources must be used in the same frame in which they are mapped.");

    const Uint64 QueueMask = GetSubmittedBuffersCmdQueueMask();
    VERIFY_EXPR(IsDeferred() || QueueMask == (Uint64{1} << GetCommandQueueId()));

    // Release resources used by the context during this frame.

    // Upload heap returns all allocated pages to the global memory manager.
    // Note: as global memory manager is hosted by the render device, the upload heap can be destroyed
    // before the pages are actually returned to the manager.
    m_UploadHeap.ReleaseAllocatedPages(QueueMask);

    // Dynamic heap returns all allocated master blocks to the global dynamic memory manager.
    // Note: as global dynamic memory manager is hosted by the render device, the dynamic heap can
    // be destroyed before the blocks are actually returned to the global dynamic memory manager.
    m_DynamicHeap.ReleaseMasterBlocks(*m_pDevice, QueueMask);

    // Dynamic descriptor set allocator returns all allocated pools to the global dynamic descriptor pool manager.
    // Note: as global pool manager is hosted by the render device, the allocator can
    // be destroyed before the pools are actually returned to the global pool manager.
    m_DynamicDescrSetAllocator.ReleasePools(QueueMask);

    EndFrame();
}

void DeviceContextVkImpl::Flush()
{
    Flush(0, nullptr);
}

void DeviceContextVkImpl::Flush(Uint32               NumCommandLists,
                                ICommandList* const* ppCommandLists)
{
    DEV_CHECK_ERR(!IsDeferred(), "Flush() should only be called for immediate contexts.");

    DEV_CHECK_ERR(m_ActiveQueriesCounter == 0,
                  "Flushing device context that has ", m_ActiveQueriesCounter,
                  " active queries. Vulkan requires that queries are begun and ended in the same command buffer.");

    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr,
                  "Flushing device context inside an active render pass.");

    // TODO: replace with small_vector
    std::vector<VkCommandBuffer>               vkCmdBuffs;
    std::vector<RefCntAutoPtr<IDeviceContext>> DeferredCtxs;
    vkCmdBuffs.reserve(size_t{NumCommandLists} + 1);
    DeferredCtxs.reserve(size_t{NumCommandLists} + 1);

    auto vkCmdBuff = m_CommandBuffer.GetVkCmdBuffer();
    if (vkCmdBuff != VK_NULL_HANDLE)
    {
        if (m_pQueryMgr != nullptr)
        {
            VERIFY_EXPR(!IsDeferred());
            // Note that vkCmdResetQueryPool must be called outside of a render pass,
            // so it is better to reset all queries at once at the end of the command buffer.
            m_State.NumCommands += m_pQueryMgr->ResetStaleQueries(m_pDevice->GetLogicalDevice(), m_CommandBuffer);
        }

        if (m_State.NumCommands != 0)
        {
            if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
            {
                m_CommandBuffer.EndRenderPass();
            }

#ifdef DILIGENT_DEVELOPMENT
            DEV_CHECK_ERR(m_DvpDebugGroupCount == 0, "Not all debug groups have been ended");
            m_DvpDebugGroupCount = 0;
#endif

            m_CommandBuffer.FlushBarriers();
            m_CommandBuffer.EndCommandBuffer();

            vkCmdBuffs.push_back(vkCmdBuff);
        }
    }

    // Add command buffers from deferred contexts
    for (Uint32 i = 0; i < NumCommandLists; ++i)
    {
        auto* pCmdListVk = ClassPtrCast<CommandListVkImpl>(ppCommandLists[i]);
        DEV_CHECK_ERR(pCmdListVk != nullptr, "Command list must not be null");
        DEV_CHECK_ERR(pCmdListVk->GetQueueId() == GetDesc().QueueId, "Command list recorded for QueueId ", pCmdListVk->GetQueueId(), ", but executed on QueueId ", GetDesc().QueueId, ".");
        DeferredCtxs.emplace_back();
        vkCmdBuffs.emplace_back();
        pCmdListVk->Close(DeferredCtxs.back(), vkCmdBuffs.back());
        VERIFY(vkCmdBuffs.back() != VK_NULL_HANDLE, "Trying to execute empty command buffer");
        VERIFY_EXPR(DeferredCtxs.back() != nullptr);
    }

    VERIFY_EXPR(m_VkWaitSemaphores.size() == m_WaitManagedSemaphores.size() + m_WaitRecycledSemaphores.size());
    VERIFY_EXPR(m_VkSignalSemaphores.size() == m_SignalManagedSemaphores.size());

    bool UsedTimelineSemaphore = false;
    for (auto& val_fence : m_SignalFences)
    {
        auto* pFenceVk = val_fence.second.RawPtr<FenceVkImpl>();
        if (!pFenceVk->IsTimelineSemaphore())
            continue;
        UsedTimelineSemaphore = true;
        pFenceVk->DvpSignal(val_fence.first);
        m_VkSignalSemaphores.push_back(pFenceVk->GetVkSemaphore());
        m_SignalSemaphoreValues.push_back(val_fence.first);
    }

    for (auto& val_fence : m_WaitFences)
    {
        auto* pFenceVk = val_fence.second.RawPtr<FenceVkImpl>();
        pFenceVk->DvpDeviceWait(val_fence.first);

        if (pFenceVk->IsTimelineSemaphore())
        {
            UsedTimelineSemaphore = true;
            VkSemaphore WaitSem   = pFenceVk->GetVkSemaphore();
#ifdef DILIGENT_DEVELOPMENT
            for (size_t i = 0; i < m_VkWaitSemaphores.size(); ++i)
            {
                if (m_VkWaitSemaphores[i] == WaitSem)
                {
                    LOG_ERROR_MESSAGE("Fence '", pFenceVk->GetDesc().Name, "' with value (", val_fence.first,
                                      ") is already added to wait operation with value (", m_WaitSemaphoreValues[i], ")");
                }
            }
#endif
            m_VkWaitSemaphores.push_back(WaitSem);
            m_WaitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            m_WaitSemaphoreValues.push_back(val_fence.first);
        }
        else
        {
            auto WaitSem = pFenceVk->ExtractSignalSemaphore(GetCommandQueueId(), val_fence.first);
            if (WaitSem)
            {
                // Here we have unique binary semaphore that must be released/recycled using release queue
                m_VkWaitSemaphores.push_back(WaitSem);
                m_WaitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                m_WaitRecycledSemaphores.push_back(std::move(WaitSem));
                m_WaitSemaphoreValues.push_back(0); // Ignored for binary semaphore
            }
        }
    }

    VERIFY_EXPR(m_VkWaitSemaphores.size() == m_WaitDstStageMasks.size());
    VERIFY_EXPR(m_VkWaitSemaphores.size() == m_WaitSemaphoreValues.size());
    VERIFY_EXPR(m_VkSignalSemaphores.size() == m_SignalSemaphoreValues.size());

    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext                = nullptr;
    SubmitInfo.commandBufferCount   = static_cast<uint32_t>(vkCmdBuffs.size());
    SubmitInfo.pCommandBuffers      = vkCmdBuffs.data();
    SubmitInfo.waitSemaphoreCount   = static_cast<uint32_t>(m_VkWaitSemaphores.size());
    SubmitInfo.pWaitSemaphores      = SubmitInfo.waitSemaphoreCount != 0 ? m_VkWaitSemaphores.data() : nullptr;
    SubmitInfo.pWaitDstStageMask    = SubmitInfo.waitSemaphoreCount != 0 ? m_WaitDstStageMasks.data() : nullptr;
    SubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(m_VkSignalSemaphores.size());
    SubmitInfo.pSignalSemaphores    = SubmitInfo.signalSemaphoreCount != 0 ? m_VkSignalSemaphores.data() : nullptr;

    VkTimelineSemaphoreSubmitInfo TimelineSemaphoreSubmitInfo{};
    if (UsedTimelineSemaphore)
    {
        SubmitInfo.pNext = &TimelineSemaphoreSubmitInfo;

        TimelineSemaphoreSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSemaphoreSubmitInfo.pNext                     = nullptr;
        TimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = SubmitInfo.waitSemaphoreCount;
        TimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = SubmitInfo.waitSemaphoreCount ? m_WaitSemaphoreValues.data() : nullptr;
        TimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = SubmitInfo.signalSemaphoreCount;
        TimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = SubmitInfo.signalSemaphoreCount ? m_SignalSemaphoreValues.data() : nullptr;
    }

    // Submit command buffer even if there are no commands to release stale resources.
    auto SubmittedFenceValue = m_pDevice->ExecuteCommandBuffer(GetCommandQueueId(), SubmitInfo, &m_SignalFences);

    // Recycle semaphores
    {
        auto& ReleaseQueue = m_pDevice->GetReleaseQueue(GetCommandQueueId());
        for (auto& Sem : m_WaitRecycledSemaphores)
        {
            Sem.SetUnsignaled();
            ReleaseQueue.DiscardResource(std::move(Sem), SubmittedFenceValue);
        }
        m_WaitRecycledSemaphores.clear();
    }

    m_WaitManagedSemaphores.clear();
    m_WaitDstStageMasks.clear();
    m_SignalManagedSemaphores.clear();
    m_VkWaitSemaphores.clear();
    m_VkSignalSemaphores.clear();
    m_SignalFences.clear();
    m_WaitFences.clear();
    m_WaitSemaphoreValues.clear();
    m_SignalSemaphoreValues.clear();

    size_t buff_idx = 0;
    if (vkCmdBuff != VK_NULL_HANDLE)
    {
        VERIFY_EXPR(vkCmdBuffs[buff_idx] == vkCmdBuff);
        DisposeCurrentCmdBuffer(GetCommandQueueId(), SubmittedFenceValue);
        ++buff_idx;
    }

    for (Uint32 i = 0; i < NumCommandLists; ++i, ++buff_idx)
    {
        auto pDeferredCtxVkImpl = DeferredCtxs[i].RawPtr<DeviceContextVkImpl>();
        // Set the bit in the deferred context cmd queue mask corresponding to cmd queue of this context
        pDeferredCtxVkImpl->UpdateSubmittedBuffersCmdQueueMask(GetCommandQueueId());
        // It is OK to dispose command buffer from another thread. We are not going to
        // record any commands and only need to add the buffer to the queue
        pDeferredCtxVkImpl->DisposeVkCmdBuffer(GetCommandQueueId(), std::move(vkCmdBuffs[buff_idx]), SubmittedFenceValue);
    }
    VERIFY_EXPR(buff_idx == vkCmdBuffs.size());

    m_State    = {};
    m_BindInfo = {};
    m_CommandBuffer.Reset();
    m_pPipelineState    = nullptr;
    m_pActiveRenderPass = nullptr;
    m_pBoundFramebuffer = nullptr;
}

void DeviceContextVkImpl::SetVertexBuffers(Uint32                         StartSlot,
                                           Uint32                         NumBuffersSet,
                                           IBuffer* const*                ppBuffers,
                                           const Uint64*                  pOffsets,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                           SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);
    for (Uint32 Buff = 0; Buff < m_NumVertexStreams; ++Buff)
    {
        auto& CurrStream = m_VertexStreams[Buff];
        if (auto* pBufferVk = CurrStream.pBuffer.RawPtr())
        {
            TransitionOrVerifyBufferState(*pBufferVk, StateTransitionMode, RESOURCE_STATE_VERTEX_BUFFER, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                                          "Setting vertex buffers (DeviceContextVkImpl::SetVertexBuffers)");
        }
    }
    m_State.CommittedVBsUpToDate = false;
}

void DeviceContextVkImpl::InvalidateState()
{
    if (m_State.NumCommands != 0)
        LOG_WARNING_MESSAGE("Invalidating context that has outstanding commands in it. Call Flush() to submit commands for execution");

    TDeviceContextBase::InvalidateState();
    m_State         = {};
    m_BindInfo      = {};
    m_vkRenderPass  = VK_NULL_HANDLE;
    m_vkFramebuffer = VK_NULL_HANDLE;

    VERIFY(m_CommandBuffer.GetState().RenderPass == VK_NULL_HANDLE, "Invalidating context with unfinished render pass");
    m_CommandBuffer.Reset();
}

void DeviceContextVkImpl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint64 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);
    if (m_pIndexBuffer)
    {
        TransitionOrVerifyBufferState(*m_pIndexBuffer, StateTransitionMode, RESOURCE_STATE_INDEX_BUFFER, VK_ACCESS_INDEX_READ_BIT, "Binding buffer as index buffer  (DeviceContextVkImpl::SetIndexBuffer)");
    }
    m_State.CommittedIBUpToDate = false;
}


void DeviceContextVkImpl::CommitViewports()
{
    if (m_NumViewports == 0)
        return;

    VkViewport VkViewports[MAX_VIEWPORTS]; // Do not waste time initializing array to zero
    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        VkViewports[vp].x        = m_Viewports[vp].TopLeftX;
        VkViewports[vp].y        = m_Viewports[vp].TopLeftY;
        VkViewports[vp].width    = m_Viewports[vp].Width;
        VkViewports[vp].height   = m_Viewports[vp].Height;
        VkViewports[vp].minDepth = m_Viewports[vp].MinDepth;
        VkViewports[vp].maxDepth = m_Viewports[vp].MaxDepth;

        // Turn the viewport upside down to be consistent with Direct3D. Note that in both APIs,
        // the viewport covers the same texture rows. The difference is that Direct3D inverts
        // normalized device Y coordinate when transforming NDC to window coordinates. In Vulkan
        // we achieve the same effect by using negative viewport height. Therefore we need to
        // invert normalized device Y coordinate when transforming to texture V
        //
        //
        //       Image                Direct3D                                       Image               Vulkan
        //        row                                                                 row
        //         0 _   (0,0)_______________________(1,0)                  Tex Height _   (0,1)_______________________(1,1)
        //         1 _       |                       |      |             VP Top + Hght _ _ _ _|   __________          |      A
        //         2 _       |                       |      |                          .       |  |   .--> +x|         |      |
        //           .       |                       |      |                          .       |  |   |      |         |      |
        //           .       |                       |      | V Coord                          |  |   V +y   |         |      | V Coord
        //     VP Top _ _ _ _|   __________          |      |                    VP Top _ _ _ _|  |__________|         |      |
        //           .       |  |    A +y  |         |      |                          .       |                       |      |
        //           .       |  |    |     |         |      |                          .       |                       |      |
        //           .       |  |    '-->+x|         |      |                        2 _       |                       |      |
        //           .       |  |__________|         |      |                        1 _       |                       |      |
        //Tex Height _       |_______________________|      V                        0 _       |_______________________|      |
        //               (0,1)                       (1,1)                                 (0,0)                       (1,0)
        //
        //

        VkViewports[vp].y      = VkViewports[vp].y + VkViewports[vp].height;
        VkViewports[vp].height = -VkViewports[vp].height;
    }
    EnsureVkCmdBuffer();
    // TODO: reinterpret_cast m_Viewports to VkViewports?
    m_CommandBuffer.SetViewports(0, m_NumViewports, VkViewports);
}

void DeviceContextVkImpl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);
    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");

    if (m_State.NullRenderTargets)
    {
        DEV_CHECK_ERR(m_NumViewports == 1, "Only a single viewport is supported when rendering without render targets");

        const auto VPWidth  = static_cast<Uint32>(m_Viewports[0].Width);
        const auto VPHeight = static_cast<Uint32>(m_Viewports[0].Height);
        if (m_FramebufferWidth != VPWidth || m_FramebufferHeight != VPHeight)
        {
            // We need to bind another framebuffer since the size has changed
            m_vkFramebuffer = VK_NULL_HANDLE;
        }
        m_FramebufferWidth   = VPWidth;
        m_FramebufferHeight  = VPHeight;
        m_FramebufferSlices  = 1;
        m_FramebufferSamples = 1;
    }

    // If no graphics PSO is currently bound, viewports will be committed by
    // the SetPipelineState() when a graphics PSO is set.
    if (m_pPipelineState && m_pPipelineState->GetDesc().IsAnyGraphicsPipeline())
    {
        CommitViewports();
    }
}

void DeviceContextVkImpl::CommitScissorRects()
{
    VERIFY(m_pPipelineState && m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable, "Scissor test must be enabled in the graphics pipeline");

    if (m_NumScissorRects == 0)
        return; // Scissors have not been set in the context yet

    VkRect2D VkScissorRects[MAX_VIEWPORTS]; // Do not waste time initializing array with zeroes
    for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
    {
        const auto& SrcRect       = m_ScissorRects[sr];
        VkScissorRects[sr].offset = {SrcRect.left, SrcRect.top};
        VkScissorRects[sr].extent = {static_cast<uint32_t>(SrcRect.right - SrcRect.left), static_cast<uint32_t>(SrcRect.bottom - SrcRect.top)};
    }

    EnsureVkCmdBuffer();
    // TODO: reinterpret_cast m_Viewports to m_Viewports?
    m_CommandBuffer.SetScissorRects(0, m_NumScissorRects, VkScissorRects);
}


void DeviceContextVkImpl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    // Only commit scissor rects if scissor test is enabled in the rasterizer state.
    // If scissor is currently disabled, or no PSO is bound, scissor rects will be committed by
    // the SetPipelineState() when a PSO with enabled scissor test is set.
    if (m_pPipelineState && m_pPipelineState->GetDesc().IsAnyGraphicsPipeline() && m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable)
    {
        VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
        CommitScissorRects();
    }
}


void DeviceContextVkImpl::TransitionRenderTargets(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    VERIFY(StateTransitionMode != RESOURCE_STATE_TRANSITION_MODE_TRANSITION || m_pActiveRenderPass == nullptr,
           "State transitions are not allowed inside a render pass.");

    if (m_pBoundDepthStencil)
    {
        const auto ViewType = m_pBoundDepthStencil->GetDesc().ViewType;
        VERIFY_EXPR(ViewType == TEXTURE_VIEW_DEPTH_STENCIL || ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL);
        const bool bReadOnly = ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL;

        const RESOURCE_STATE NewState = bReadOnly ?
            RESOURCE_STATE_DEPTH_READ :
            RESOURCE_STATE_DEPTH_WRITE;

        const VkImageLayout ExpectedLayout = bReadOnly ?
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        auto* pDepthBufferVk = m_pBoundDepthStencil->GetTexture<TextureVkImpl>();
        TransitionOrVerifyTextureState(*pDepthBufferVk, StateTransitionMode, NewState, ExpectedLayout,
                                       "Binding depth-stencil buffer (DeviceContextVkImpl::TransitionRenderTargets)");
    }

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (auto& pRTVVk = m_pBoundRenderTargets[rt])
        {
            auto* pRenderTargetVk = pRTVVk->GetTexture<TextureVkImpl>();
            TransitionOrVerifyTextureState(*pRenderTargetVk, StateTransitionMode, RESOURCE_STATE_RENDER_TARGET, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           "Binding render targets (DeviceContextVkImpl::TransitionRenderTargets)");
        }
    }

    if (m_pBoundShadingRateMap)
    {
        const auto& ExtFeatures       = m_pDevice->GetLogicalDevice().GetEnabledExtFeatures();
        auto*       pShadingRateMapVk = ClassPtrCast<TextureVkImpl>(m_pBoundShadingRateMap->GetTexture());
        VERIFY_EXPR((ExtFeatures.ShadingRate.attachmentFragmentShadingRate != VK_FALSE) ^ (ExtFeatures.FragmentDensityMap.fragmentDensityMap != VK_FALSE));
        const auto vkRequiredLayout = ExtFeatures.ShadingRate.attachmentFragmentShadingRate ?
            VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR :
            VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        TransitionOrVerifyTextureState(*pShadingRateMapVk, StateTransitionMode, RESOURCE_STATE_SHADING_RATE, vkRequiredLayout,
                                       "Binding shading rate map (DeviceContextVkImpl::TransitionRenderTargets)");
    }
}

void DeviceContextVkImpl::CommitRenderPassAndFramebuffer(bool VerifyStates)
{
    VERIFY(m_pActiveRenderPass == nullptr, "This method must not be called inside an active render pass.");

    const auto& CmdBufferState = m_CommandBuffer.GetState();
    if (CmdBufferState.Framebuffer != m_vkFramebuffer)
    {
        if (CmdBufferState.RenderPass != VK_NULL_HANDLE)
            m_CommandBuffer.EndRenderPass();

        if (m_vkFramebuffer != VK_NULL_HANDLE)
        {
            VERIFY_EXPR(m_vkRenderPass != VK_NULL_HANDLE);
#ifdef DILIGENT_DEVELOPMENT
            if (VerifyStates)
            {
                TransitionRenderTargets(RESOURCE_STATE_TRANSITION_MODE_VERIFY);
            }
#endif
            m_CommandBuffer.BeginRenderPass(m_vkRenderPass, m_vkFramebuffer, m_FramebufferWidth, m_FramebufferHeight);
        }
    }
}

void DeviceContextVkImpl::ChooseRenderPassAndFramebuffer()
{
    FramebufferCache::FramebufferCacheKey FBKey;
    RenderPassCache::RenderPassCacheKey   RenderPassKey;
    if (m_pBoundDepthStencil)
    {
        auto* pDepthBuffer        = m_pBoundDepthStencil->GetTexture();
        FBKey.DSV                 = m_pBoundDepthStencil->GetVulkanImageView();
        RenderPassKey.DSVFormat   = m_pBoundDepthStencil->GetDesc().Format;
        RenderPassKey.ReadOnlyDSV = m_pBoundDepthStencil->GetDesc().ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL;
        RenderPassKey.SampleCount = static_cast<Uint8>(pDepthBuffer->GetDesc().SampleCount);
    }
    else
    {
        FBKey.DSV               = VK_NULL_HANDLE;
        RenderPassKey.DSVFormat = TEX_FORMAT_UNKNOWN;
    }

    FBKey.NumRenderTargets         = m_NumBoundRenderTargets;
    RenderPassKey.NumRenderTargets = static_cast<Uint8>(m_NumBoundRenderTargets);

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (auto* pRTVVk = m_pBoundRenderTargets[rt].RawPtr())
        {
            auto* pRenderTarget          = pRTVVk->GetTexture();
            FBKey.RTVs[rt]               = pRTVVk->GetVulkanImageView();
            RenderPassKey.RTVFormats[rt] = pRenderTarget->GetDesc().Format;
            if (RenderPassKey.SampleCount == 0)
                RenderPassKey.SampleCount = static_cast<Uint8>(pRenderTarget->GetDesc().SampleCount);
            else
                VERIFY(RenderPassKey.SampleCount == pRenderTarget->GetDesc().SampleCount, "Inconsistent sample count");
        }
        else
        {
            FBKey.RTVs[rt]               = VK_NULL_HANDLE;
            RenderPassKey.RTVFormats[rt] = TEX_FORMAT_UNKNOWN;
        }
    }

    if (RenderPassKey.SampleCount == 0)
        RenderPassKey.SampleCount = static_cast<Uint8>(m_FramebufferSamples);

    if (m_pBoundShadingRateMap)
    {
        FBKey.ShadingRate       = m_pBoundShadingRateMap.RawPtr<TextureViewVkImpl>()->GetVulkanImageView();
        RenderPassKey.EnableVRS = true;
    }
    else
    {
        FBKey.ShadingRate       = VK_NULL_HANDLE;
        RenderPassKey.EnableVRS = false;
    }

    auto& FBCache = m_pDevice->GetFramebufferCache();
    auto& RPCache = m_pDevice->GetImplicitRenderPassCache();

    if (auto* pRenderPass = RPCache.GetRenderPass(RenderPassKey))
    {
        m_vkRenderPass         = pRenderPass->GetVkRenderPass();
        FBKey.Pass             = m_vkRenderPass;
        FBKey.CommandQueueMask = ~Uint64{0};
        m_vkFramebuffer        = FBCache.GetFramebuffer(FBKey, m_FramebufferWidth, m_FramebufferHeight, m_FramebufferSlices);
    }
    else
    {
        UNEXPECTED("Unable to get render pass for the currently bound render targets");
        m_vkRenderPass  = VK_NULL_HANDLE;
        m_vkFramebuffer = VK_NULL_HANDLE;
    }
}

void DeviceContextVkImpl::SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Calling SetRenderTargets inside active render pass is invalid. End the render pass first");

    if (TDeviceContextBase::SetRenderTargets(Attribs))
    {
        ChooseRenderPassAndFramebuffer();

        // Set the viewport to match the render target size
        SetViewports(1, nullptr, 0, 0);
    }

    // Layout transitions can only be performed outside of render pass, so defer
    // CommitRenderPassAndFramebuffer() until draw call, otherwise we may have to
    // to end render pass and begin it again if we need to transition any resource
    // (for instance when CommitShaderResources() is called after SetRenderTargets())
    TransitionRenderTargets(Attribs.StateTransitionMode);
}

void DeviceContextVkImpl::ResetRenderTargets()
{
    TDeviceContextBase::ResetRenderTargets();
    m_vkRenderPass  = VK_NULL_HANDLE;
    m_vkFramebuffer = VK_NULL_HANDLE;
    if (m_CommandBuffer.GetVkCmdBuffer() != VK_NULL_HANDLE && m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
        m_CommandBuffer.EndRenderPass();
    m_State.ShadingRateIsSet = false;
}

void DeviceContextVkImpl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);

    VERIFY_EXPR(m_pActiveRenderPass != nullptr);
    VERIFY_EXPR(m_pBoundFramebuffer != nullptr);
    VERIFY_EXPR(m_vkRenderPass == VK_NULL_HANDLE);
    VERIFY_EXPR(m_vkFramebuffer == VK_NULL_HANDLE);

    m_vkRenderPass  = m_pActiveRenderPass->GetVkRenderPass();
    m_vkFramebuffer = m_pBoundFramebuffer->GetVkFramebuffer();

    VkClearValue* pVkClearValues = nullptr;
    if (Attribs.ClearValueCount > 0)
    {
        m_vkClearValues.resize(Attribs.ClearValueCount);
        const auto& RPDesc = m_pActiveRenderPass->GetDesc();
        for (Uint32 i = 0; i < std::min(RPDesc.AttachmentCount, Attribs.ClearValueCount); ++i)
        {
            const auto& ClearVal   = Attribs.pClearValues[i];
            auto&       vkClearVal = m_vkClearValues[i];

            const auto& FmtAttribs = GetTextureFormatAttribs(RPDesc.pAttachments[i].Format);
            if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ||
                FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            {
                vkClearVal.depthStencil.depth   = ClearVal.DepthStencil.Depth;
                vkClearVal.depthStencil.stencil = ClearVal.DepthStencil.Stencil;
            }
            else
            {
                vkClearVal.color.float32[0] = ClearVal.Color[0];
                vkClearVal.color.float32[1] = ClearVal.Color[1];
                vkClearVal.color.float32[2] = ClearVal.Color[2];
                vkClearVal.color.float32[3] = ClearVal.Color[3];
            }
        }
        pVkClearValues = m_vkClearValues.data();
    }

    EnsureVkCmdBuffer();
    m_CommandBuffer.BeginRenderPass(m_vkRenderPass, m_vkFramebuffer, m_FramebufferWidth, m_FramebufferHeight, Attribs.ClearValueCount, pVkClearValues);

    // Set the viewport to match the framebuffer size
    SetViewports(1, nullptr, 0, 0);

    m_State.ShadingRateIsSet = false;
}

void DeviceContextVkImpl::NextSubpass()
{
    TDeviceContextBase::NextSubpass();
    VERIFY_EXPR(m_CommandBuffer.GetVkCmdBuffer() != VK_NULL_HANDLE && m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE);
    m_CommandBuffer.NextSubpass();
}

void DeviceContextVkImpl::EndRenderPass()
{
    TDeviceContextBase::EndRenderPass();
    // TDeviceContextBase::EndRenderPass calls ResetRenderTargets() that in turn
    // calls m_CommandBuffer.EndRenderPass()
}

void DeviceContextVkImpl::UpdateBufferRegion(BufferVkImpl*                  pBuffVk,
                                             Uint64                         DstOffset,
                                             Uint64                         NumBytes,
                                             VkBuffer                       vkSrcBuffer,
                                             Uint64                         SrcOffset,
                                             RESOURCE_STATE_TRANSITION_MODE TransitionMode)
{
    DEV_CHECK_ERR(DstOffset + NumBytes <= pBuffVk->GetDesc().Size,
                  "Update region is out of buffer bounds which will result in an undefined behavior");

    EnsureVkCmdBuffer();
    TransitionOrVerifyBufferState(*pBuffVk, TransitionMode, RESOURCE_STATE_COPY_DEST, VK_ACCESS_TRANSFER_WRITE_BIT, "Updating buffer (DeviceContextVkImpl::UpdateBufferRegion)");

    VkBufferCopy CopyRegion;
    CopyRegion.srcOffset = SrcOffset;
    CopyRegion.dstOffset = DstOffset;
    CopyRegion.size      = NumBytes;
    VERIFY(pBuffVk->m_VulkanBuffer != VK_NULL_HANDLE, "Copy destination buffer must not be suballocated");
    m_CommandBuffer.CopyBuffer(vkSrcBuffer, pBuffVk->GetVkBuffer(), 1, &CopyRegion);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::UpdateBuffer(IBuffer*                       pBuffer,
                                       Uint64                         Offset,
                                       Uint64                         Size,
                                       const void*                    pData,
                                       RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    // We must use cmd context from the device context provided, otherwise there will
    // be resource barrier issues in the cmd list in the device context
    auto* pBuffVk = ClassPtrCast<BufferVkImpl>(pBuffer);

    DEV_CHECK_ERR(pBuffVk->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers must be updated via Map()");

    constexpr size_t Alignment = 4;
    // Source buffer offset must be multiple of 4 (18.4)
    auto TmpSpace = m_UploadHeap.Allocate(Size, Alignment);
    memcpy(TmpSpace.CPUAddress, pData, StaticCast<size_t>(Size));
    UpdateBufferRegion(pBuffVk, Offset, Size, TmpSpace.vkBuffer, TmpSpace.AlignedOffset, StateTransitionMode);
    // The allocation will stay in the upload heap until the end of the frame at which point all upload
    // pages will be discarded
}

void DeviceContextVkImpl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                     Uint64                         SrcOffset,
                                     RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                     IBuffer*                       pDstBuffer,
                                     Uint64                         DstOffset,
                                     Uint64                         Size,
                                     RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBuffVk = ClassPtrCast<BufferVkImpl>(pSrcBuffer);
    auto* pDstBuffVk = ClassPtrCast<BufferVkImpl>(pDstBuffer);

    DEV_CHECK_ERR(pDstBuffVk->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers cannot be copy destinations");

    EnsureVkCmdBuffer();
    TransitionOrVerifyBufferState(*pSrcBuffVk, SrcBufferTransitionMode, RESOURCE_STATE_COPY_SOURCE, VK_ACCESS_TRANSFER_READ_BIT, "Using buffer as copy source (DeviceContextVkImpl::CopyBuffer)");
    TransitionOrVerifyBufferState(*pDstBuffVk, DstBufferTransitionMode, RESOURCE_STATE_COPY_DEST, VK_ACCESS_TRANSFER_WRITE_BIT, "Using buffer as copy destination (DeviceContextVkImpl::CopyBuffer)");

    VkBufferCopy CopyRegion;
    CopyRegion.srcOffset = SrcOffset + pSrcBuffVk->GetDynamicOffset(GetContextId(), this);
    CopyRegion.dstOffset = DstOffset;
    CopyRegion.size      = Size;
    VERIFY(pDstBuffVk->m_VulkanBuffer != VK_NULL_HANDLE, "Copy destination buffer must not be suballocated");
    VERIFY_EXPR(pDstBuffVk->GetDynamicOffset(GetContextId(), this) == 0);
    m_CommandBuffer.CopyBuffer(pSrcBuffVk->GetVkBuffer(), pDstBuffVk->GetVkBuffer(), 1, &CopyRegion);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);
    auto* const pBufferVk = ClassPtrCast<BufferVkImpl>(pBuffer);
    const auto& BuffDesc  = pBufferVk->GetDesc();

    if (MapType == MAP_READ)
    {
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                      "Buffer must be created as USAGE_STAGING or USAGE_UNIFIED to be mapped for reading");

        if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
        {
            LOG_WARNING_MESSAGE("Vulkan backend never waits for GPU when mapping staging buffers for reading. "
                                "Applications must use fences or other synchronization methods to explicitly synchronize "
                                "access and use MAP_FLAG_DO_NOT_WAIT flag.");
        }

        pMappedData = pBufferVk->GetCPUAddress();
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED)
        {
            pMappedData = pBufferVk->GetCPUAddress();
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0, "Failed to map buffer '",
                          BuffDesc.Name, "': Vulkan buffer must be mapped for writing with MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flag. Context Id: ", GetContextId());

            auto& DynAllocation = pBufferVk->m_DynamicData[GetContextId()];
            if ((MapFlags & MAP_FLAG_DISCARD) != 0 || !DynAllocation)
            {
                DynAllocation = AllocateDynamicSpace(BuffDesc.Size, pBufferVk->m_DynamicOffsetAlignment);
            }
            else
            {
                VERIFY_EXPR(MapFlags & MAP_FLAG_NO_OVERWRITE);

                if (pBufferVk->m_VulkanBuffer != VK_NULL_HANDLE)
                {
                    LOG_ERROR("Formatted or structured buffers require actual Vulkan backing resource and cannot be suballocated "
                              "from dynamic heap. In current implementation, the entire contents of the backing buffer is updated when the buffer is unmapped. "
                              "As a consequence, the buffer cannot be mapped with MAP_FLAG_NO_OVERWRITE flag because updating the whole "
                              "buffer will overwrite regions that may still be in use by the GPU.");
                    return;
                }

                // Reuse the same allocation
            }

            if (DynAllocation)
            {
                auto* CPUAddress = DynAllocation.pDynamicMemMgr->GetCPUAddress();
                pMappedData      = CPUAddress + DynAllocation.AlignedOffset;
            }
            else
            {
                pMappedData = nullptr;
            }
        }
        else
        {
            LOG_ERROR("Only USAGE_DYNAMIC, USAGE_STAGING and USAGE_UNIFIED Vulkan buffers can be mapped for writing");
        }
    }
    else if (MapType == MAP_READ_WRITE)
    {
        LOG_ERROR("MAP_READ_WRITE is not supported in Vulkan backend");
    }
    else
    {
        UNEXPECTED("Unknown map type");
    }
}

void DeviceContextVkImpl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto* const pBufferVk = ClassPtrCast<BufferVkImpl>(pBuffer);
    const auto& BuffDesc  = pBufferVk->GetDesc();

    if (MapType == MAP_READ)
    {
        if (BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED)
        {
            if ((pBufferVk->GetMemoryProperties() & MEMORY_PROPERTY_HOST_COHERENT) == 0)
            {
                pBufferVk->InvalidateMappedRange(0, BuffDesc.Size);
            }
        }
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED)
        {
            if ((pBufferVk->GetMemoryProperties() & MEMORY_PROPERTY_HOST_COHERENT) == 0)
            {
                pBufferVk->FlushMappedRange(0, BuffDesc.Size);
            }
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            if (pBufferVk->m_VulkanBuffer != VK_NULL_HANDLE)
            {
                auto& DynAlloc  = pBufferVk->m_DynamicData[GetContextId()];
                auto  vkSrcBuff = DynAlloc.pDynamicMemMgr->GetVkBuffer();
                UpdateBufferRegion(pBufferVk, 0, BuffDesc.Size, vkSrcBuff, DynAlloc.AlignedOffset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
        }
    }
}

void DeviceContextVkImpl::UpdateTexture(ITexture*                      pTexture,
                                        Uint32                         MipLevel,
                                        Uint32                         Slice,
                                        const Box&                     DstBox,
                                        const TextureSubResData&       SubresData,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                        RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferStateTransitionMode, TextureStateTransitionMode);

    auto* pTexVk = ClassPtrCast<TextureVkImpl>(pTexture);
    // OpenGL backend uses UpdateData() to initialize textures, so we can't check the usage in ValidateUpdateTextureParams()
    DEV_CHECK_ERR(pTexVk->GetDesc().Usage == USAGE_DEFAULT || pTexVk->GetDesc().Usage == USAGE_SPARSE,
                  "Only USAGE_DEFAULT or USAGE_SPARSE textures should be updated with UpdateData()");

    if (SubresData.pSrcBuffer != nullptr)
    {
        UNSUPPORTED("Copying buffer to texture is not implemented");
    }
    else
    {
        UpdateTextureRegion(SubresData.pData, SubresData.Stride, SubresData.DepthStride, *pTexVk,
                            MipLevel, Slice, DstBox, TextureStateTransitionMode);
    }
}

void DeviceContextVkImpl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);

    auto* pSrcTexVk = ClassPtrCast<TextureVkImpl>(CopyAttribs.pSrcTexture);
    auto* pDstTexVk = ClassPtrCast<TextureVkImpl>(CopyAttribs.pDstTexture);

    // We must unbind the textures from framebuffer because
    // we will transition their states. If we later try to commit
    // them as render targets (e.g. from SetPipelineState()), a
    // state mismatch error will occur.
    UnbindTextureFromFramebuffer(pSrcTexVk, true);
    UnbindTextureFromFramebuffer(pDstTexVk, true);

    const auto& SrcTexDesc = pSrcTexVk->GetDesc();
    const auto& DstTexDesc = pDstTexVk->GetDesc();
    auto*       pSrcBox    = CopyAttribs.pSrcBox;
    Box         FullMipBox;
    if (pSrcBox == nullptr)
    {
        auto MipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);
        FullMipBox.MaxX      = MipLevelAttribs.LogicalWidth;
        FullMipBox.MaxY      = MipLevelAttribs.LogicalHeight;
        FullMipBox.MaxZ      = MipLevelAttribs.Depth;
        pSrcBox              = &FullMipBox;
    }

    if (SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING)
    {
        VkImageCopy CopyRegion = {};

        CopyRegion.srcOffset.x   = pSrcBox->MinX;
        CopyRegion.srcOffset.y   = pSrcBox->MinY;
        CopyRegion.srcOffset.z   = pSrcBox->MinZ;
        CopyRegion.extent.width  = pSrcBox->Width();
        CopyRegion.extent.height = std::max(pSrcBox->Height(), 1u);
        CopyRegion.extent.depth  = std::max(pSrcBox->Depth(), 1u);

        auto GetAspectMak = [](TEXTURE_FORMAT Format) -> VkImageAspectFlags {
            const auto& FmtAttribs = GetTextureFormatAttribs(Format);
            switch (FmtAttribs.ComponentType)
            {
                // clang-format off
                case COMPONENT_TYPE_DEPTH:         return VK_IMAGE_ASPECT_DEPTH_BIT;
                case COMPONENT_TYPE_DEPTH_STENCIL: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                // clang-format on
                default: return VK_IMAGE_ASPECT_COLOR_BIT;
            }
        };
        VkImageAspectFlags aspectMask = GetAspectMak(SrcTexDesc.Format);
        DEV_CHECK_ERR(aspectMask == GetAspectMak(DstTexDesc.Format), "Vulkan spec requires that dst and src aspect masks must match");

        CopyRegion.srcSubresource.baseArrayLayer = CopyAttribs.SrcSlice;
        CopyRegion.srcSubresource.layerCount     = 1;
        CopyRegion.srcSubresource.mipLevel       = CopyAttribs.SrcMipLevel;
        CopyRegion.srcSubresource.aspectMask     = aspectMask;

        CopyRegion.dstSubresource.baseArrayLayer = CopyAttribs.DstSlice;
        CopyRegion.dstSubresource.layerCount     = 1;
        CopyRegion.dstSubresource.mipLevel       = CopyAttribs.DstMipLevel;
        CopyRegion.dstSubresource.aspectMask     = aspectMask;

        CopyRegion.dstOffset.x = CopyAttribs.DstX;
        CopyRegion.dstOffset.y = CopyAttribs.DstY;
        CopyRegion.dstOffset.z = CopyAttribs.DstZ;

        CopyTextureRegion(pSrcTexVk, CopyAttribs.SrcTextureTransitionMode, pDstTexVk, CopyAttribs.DstTextureTransitionMode, CopyRegion);
    }
    else if (SrcTexDesc.Usage == USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING)
    {
        DEV_CHECK_ERR((SrcTexDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Attempting to copy from staging texture that was not created with CPU_ACCESS_WRITE flag");
        DEV_CHECK_ERR(pSrcTexVk->GetState() == RESOURCE_STATE_COPY_SOURCE, "Source staging texture must permanently be in RESOURCE_STATE_COPY_SOURCE state");

        // address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)

        // bufferOffset must be a multiple of 4 (18.4)
        // If the calling command's VkImage parameter is a compressed image, bufferOffset
        // must be a multiple of the compressed texel block size in bytes (18.4). This
        // is automatically guaranteed as MipWidth and MipHeight are rounded to block size.

        const auto SrcBufferOffset =
            GetStagingTextureLocationOffset(SrcTexDesc, CopyAttribs.SrcSlice, CopyAttribs.SrcMipLevel,
                                            TextureVkImpl::StagingBufferOffsetAlignment,
                                            pSrcBox->MinX, pSrcBox->MinY, pSrcBox->MinZ);
        const auto SrcMipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);

        Box DstBox;
        DstBox.MinX = CopyAttribs.DstX;
        DstBox.MinY = CopyAttribs.DstY;
        DstBox.MinZ = CopyAttribs.DstZ;
        DstBox.MaxX = DstBox.MinX + pSrcBox->Width();
        DstBox.MaxY = DstBox.MinY + pSrcBox->Height();
        DstBox.MaxZ = DstBox.MinZ + pSrcBox->Depth();

        CopyBufferToTexture(
            pSrcTexVk->GetVkStagingBuffer(),
            SrcBufferOffset,
            SrcMipLevelAttribs.StorageWidth, // GetStagingTextureLocationOffset assumes texels are tightly packed
            *pDstTexVk,
            DstBox,
            CopyAttribs.DstMipLevel,
            CopyAttribs.DstSlice,
            CopyAttribs.DstTextureTransitionMode);
    }
    else if (SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage == USAGE_STAGING)
    {
        DEV_CHECK_ERR((DstTexDesc.CPUAccessFlags & CPU_ACCESS_READ), "Attempting to copy to staging texture that was not created with CPU_ACCESS_READ flag");
        DEV_CHECK_ERR(pDstTexVk->GetState() == RESOURCE_STATE_COPY_DEST, "Destination staging texture must permanently be in RESOURCE_STATE_COPY_DEST state");

        // address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
        const auto DstBufferOffset =
            GetStagingTextureLocationOffset(DstTexDesc, CopyAttribs.DstSlice, CopyAttribs.DstMipLevel,
                                            TextureVkImpl::StagingBufferOffsetAlignment,
                                            CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ);
        const auto DstMipLevelAttribs = GetMipLevelProperties(DstTexDesc, CopyAttribs.DstMipLevel);

        CopyTextureToBuffer(
            *pSrcTexVk,
            *pSrcBox,
            CopyAttribs.SrcMipLevel,
            CopyAttribs.SrcSlice,
            CopyAttribs.SrcTextureTransitionMode,
            pDstTexVk->GetVkStagingBuffer(),
            DstBufferOffset,
            DstMipLevelAttribs.StorageWidth // GetStagingTextureLocationOffset assumes texels are tightly packed
        );
    }
    else
    {
        UNSUPPORTED("Copying data between staging textures is not supported and is likely not want you really want to do");
    }
}

void DeviceContextVkImpl::CopyTextureRegion(TextureVkImpl*                 pSrcTexture,
                                            RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                                            TextureVkImpl*                 pDstTexture,
                                            RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode,
                                            const VkImageCopy&             CopyRegion)
{
    EnsureVkCmdBuffer();
    TransitionOrVerifyTextureState(*pSrcTexture, SrcTextureTransitionMode, RESOURCE_STATE_COPY_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   "Using texture as transfer source (DeviceContextVkImpl::CopyTextureRegion)");
    TransitionOrVerifyTextureState(*pDstTexture, DstTextureTransitionMode, RESOURCE_STATE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   "Using texture as transfer destination (DeviceContextVkImpl::CopyTextureRegion)");

    // srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
    // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.3)
    m_CommandBuffer.CopyImage(pSrcTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDstTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::UpdateTextureRegion(const void*                    pSrcData,
                                              Uint64                         SrcStride,
                                              Uint64                         SrcDepthStride,
                                              TextureVkImpl&                 TextureVk,
                                              Uint32                         MipLevel,
                                              Uint32                         Slice,
                                              const Box&                     DstBox,
                                              RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    const auto& TexDesc = TextureVk.GetDesc();
    VERIFY(TexDesc.SampleCount == 1, "Only single-sample textures can be updated with vkCmdCopyBufferToImage()");

    const auto& DeviceLimits      = m_pDevice->GetPhysicalDevice().GetProperties().limits;
    const auto  CopyInfo          = GetBufferToTextureCopyInfo(TexDesc.Format, DstBox, static_cast<Uint32>(DeviceLimits.optimalBufferCopyRowPitchAlignment));
    const auto  UpdateRegionDepth = CopyInfo.Region.Depth();

    // For UpdateTextureRegion(), use UploadHeap, not dynamic heap
    // Source buffer offset must be multiple of 4 (18.4)
    auto BufferOffsetAlignment = std::max(DeviceLimits.optimalBufferCopyOffsetAlignment, VkDeviceSize{4});
    // If the calling command's VkImage parameter is a compressed image, bufferOffset must be a multiple of
    // the compressed texel block size in bytes (18.4)
    const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        BufferOffsetAlignment = std::max(BufferOffsetAlignment, VkDeviceSize{FmtAttribs.ComponentSize});
    }
    auto Allocation = m_UploadHeap.Allocate(CopyInfo.MemorySize, BufferOffsetAlignment);
    // The allocation will stay in the upload heap until the end of the frame at which point all upload
    // pages will be discarded
    VERIFY((Allocation.AlignedOffset % BufferOffsetAlignment) == 0, "Allocation offset must be at least 32-bit aligned");

#ifdef DILIGENT_DEBUG
    {
        VERIFY(SrcStride >= CopyInfo.RowSize, "Source data stride (", SrcStride, ") is below the image row size (", CopyInfo.RowSize, ")");
        const auto PlaneSize = SrcStride * CopyInfo.RowCount;
        VERIFY(UpdateRegionDepth == 1 || SrcDepthStride >= PlaneSize, "Source data depth stride (", SrcDepthStride, ") is below the image plane size (", PlaneSize, ")");
    }
#endif
    for (Uint32 DepthSlice = 0; DepthSlice < UpdateRegionDepth; ++DepthSlice)
    {
        for (Uint32 row = 0; row < CopyInfo.RowCount; ++row)
        {
            // clang-format off
            const auto* pSrcPtr =
                reinterpret_cast<const Uint8*>(pSrcData)
                + row        * SrcStride
                + DepthSlice * SrcDepthStride;
            auto* pDstPtr =
                reinterpret_cast<Uint8*>(Allocation.CPUAddress)
                + row        * CopyInfo.RowStride
                + DepthSlice * CopyInfo.DepthStride;
            // clang-format on

            memcpy(pDstPtr, pSrcPtr, StaticCast<size_t>(CopyInfo.RowSize));
        }
    }
    CopyBufferToTexture(Allocation.vkBuffer,
                        Allocation.AlignedOffset,
                        CopyInfo.RowStrideInTexels,
                        TextureVk,
                        CopyInfo.Region,
                        MipLevel,
                        Slice,
                        TextureTransitionMode);
}

void DeviceContextVkImpl::GenerateMips(ITextureView* pTexView)
{
    TDeviceContextBase::GenerateMips(pTexView);
    GenerateMipsVkHelper::GenerateMips(*ClassPtrCast<TextureViewVkImpl>(pTexView), *this);
}

static VkBufferImageCopy GetBufferImageCopyInfo(Uint64             BufferOffset,
                                                Uint32             BufferRowStrideInTexels,
                                                const TextureDesc& TexDesc,
                                                const Box&         Region,
                                                Uint32             MipLevel,
                                                Uint32             ArraySlice)
{
    VkBufferImageCopy CopyRegion = {};
    VERIFY((BufferOffset % 4) == 0, "Source buffer offset must be multiple of 4 (18.4)");
    CopyRegion.bufferOffset = BufferOffset; // must be a multiple of 4 (18.4)

    // bufferRowLength and bufferImageHeight specify the data in buffer memory as a subregion of a larger two- or
    // three-dimensional image, and control the addressing calculations of data in buffer memory. If either of these
    // values is zero, that aspect of the buffer memory is considered to be tightly packed according to the imageExtent (18.4).
    CopyRegion.bufferRowLength   = BufferRowStrideInTexels;
    CopyRegion.bufferImageHeight = 0;

    const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
    // The aspectMask member of imageSubresource must only have a single bit set (18.4)
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
    {
        UNSUPPORTED("Updating depth-stencil texture is not currently supported");
        // When copying to or from a depth or stencil aspect, the data in buffer memory uses a layout
        // that is a (mostly) tightly packed representation of the depth or stencil data.
        // To copy both the depth and stencil aspects of a depth/stencil format, two entries in
        // pRegions can be used, where one specifies the depth aspect in imageSubresource, and the
        // other specifies the stencil aspect (18.4)
    }
    else
        CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    CopyRegion.imageSubresource.baseArrayLayer = ArraySlice;
    CopyRegion.imageSubresource.layerCount     = 1;
    CopyRegion.imageSubresource.mipLevel       = MipLevel;
    // - imageOffset.x and (imageExtent.width + imageOffset.x) must both be greater than or equal to 0 and
    //   less than or equal to the image subresource width (18.4)
    // - imageOffset.y and (imageExtent.height + imageOffset.y) must both be greater than or equal to 0 and
    //   less than or equal to the image subresource height (18.4)
    CopyRegion.imageOffset =
        VkOffset3D //
        {
            static_cast<int32_t>(Region.MinX),
            static_cast<int32_t>(Region.MinY),
            static_cast<int32_t>(Region.MinZ) //
        };
    VERIFY(Region.IsValid(),
           "[", Region.MinX, " .. ", Region.MaxX, ") x [", Region.MinY, " .. ", Region.MaxY, ") x [", Region.MinZ, " .. ", Region.MaxZ, ") is not a valid region");
    CopyRegion.imageExtent =
        VkExtent3D //
        {
            static_cast<uint32_t>(Region.Width()),
            static_cast<uint32_t>(Region.Height()),
            static_cast<uint32_t>(Region.Depth()) //
        };

    return CopyRegion;
}

void DeviceContextVkImpl::CopyBufferToTexture(VkBuffer                       vkSrcBuffer,
                                              Uint64                         SrcBufferOffset,
                                              Uint32                         SrcBufferRowStrideInTexels,
                                              TextureVkImpl&                 DstTextureVk,
                                              const Box&                     DstRegion,
                                              Uint32                         DstMipLevel,
                                              Uint32                         DstArraySlice,
                                              RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode)
{
    EnsureVkCmdBuffer();
    TransitionOrVerifyTextureState(DstTextureVk, DstTextureTransitionMode, RESOURCE_STATE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   "Using texture as copy destination (DeviceContextVkImpl::CopyBufferToTexture)");

    const auto&       TexDesc     = DstTextureVk.GetDesc();
    VkBufferImageCopy BuffImgCopy = GetBufferImageCopyInfo(SrcBufferOffset, SrcBufferRowStrideInTexels, TexDesc, DstRegion, DstMipLevel, DstArraySlice);

    m_CommandBuffer.CopyBufferToImage(
        vkSrcBuffer,
        DstTextureVk.GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
        1,
        &BuffImgCopy);
}

void DeviceContextVkImpl::CopyTextureToBuffer(TextureVkImpl&                 SrcTextureVk,
                                              const Box&                     SrcRegion,
                                              Uint32                         SrcMipLevel,
                                              Uint32                         SrcArraySlice,
                                              RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                                              VkBuffer                       vkDstBuffer,
                                              Uint64                         DstBufferOffset,
                                              Uint32                         DstBufferRowStrideInTexels)
{
    EnsureVkCmdBuffer();
    TransitionOrVerifyTextureState(SrcTextureVk, SrcTextureTransitionMode, RESOURCE_STATE_COPY_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   "Using texture as source destination (DeviceContextVkImpl::CopyTextureToBuffer)");

    const auto&       TexDesc     = SrcTextureVk.GetDesc();
    VkBufferImageCopy BuffImgCopy = GetBufferImageCopyInfo(DstBufferOffset, DstBufferRowStrideInTexels, TexDesc, SrcRegion, SrcMipLevel, SrcArraySlice);

    m_CommandBuffer.CopyImageToBuffer(
        SrcTextureVk.GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
        vkDstBuffer,
        1,
        &BuffImgCopy);
}


void DeviceContextVkImpl::MapTextureSubresource(ITexture*                 pTexture,
                                                Uint32                    MipLevel,
                                                Uint32                    ArraySlice,
                                                MAP_TYPE                  MapType,
                                                MAP_FLAGS                 MapFlags,
                                                const Box*                pMapRegion,
                                                MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);

    TextureVkImpl& TextureVk  = *ClassPtrCast<TextureVkImpl>(pTexture);
    const auto&    TexDesc    = TextureVk.GetDesc();
    const auto&    FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    Box FullExtentBox;
    if (pMapRegion == nullptr)
    {
        auto MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        FullExtentBox.MaxX   = MipLevelAttribs.LogicalWidth;
        FullExtentBox.MaxY   = MipLevelAttribs.LogicalHeight;
        FullExtentBox.MaxZ   = MipLevelAttribs.Depth;
        pMapRegion           = &FullExtentBox;
    }

    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        if (MapType != MAP_WRITE)
        {
            LOG_ERROR("Dynamic textures can be mapped for writing only in Vulkan backend");
            MappedData = MappedTextureSubresource{};
            return;
        }

        if ((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0)
            LOG_INFO_MESSAGE_ONCE("Mapping textures with flags MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE has no effect in Vulkan backend");

        const auto& DeviceLimits = m_pDevice->GetPhysicalDevice().GetProperties().limits;
        const auto  CopyInfo     = GetBufferToTextureCopyInfo(TexDesc.Format, *pMapRegion, static_cast<Uint32>(DeviceLimits.optimalBufferCopyRowPitchAlignment));
        // Source buffer offset must be multiple of 4 (18.4)
        auto Alignment = std::max(DeviceLimits.optimalBufferCopyOffsetAlignment, VkDeviceSize{4});
        // If the calling command's VkImage parameter is a compressed image, bufferOffset must be a multiple of
        // the compressed texel block size in bytes (18.4)
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            Alignment = std::max(Alignment, VkDeviceSize{FmtAttribs.ComponentSize});
        }
        if (auto Allocation = AllocateDynamicSpace(CopyInfo.MemorySize, static_cast<Uint32>(Alignment)))
        {
            MappedData.pData       = reinterpret_cast<Uint8*>(Allocation.pDynamicMemMgr->GetCPUAddress()) + Allocation.AlignedOffset;
            MappedData.Stride      = CopyInfo.RowStride;
            MappedData.DepthStride = CopyInfo.DepthStride;

            auto it = m_MappedTextures.emplace(MappedTextureKey{&TextureVk, MipLevel, ArraySlice}, MappedTexture{CopyInfo, std::move(Allocation)});
            if (!it.second)
                LOG_ERROR_MESSAGE("Mip level ", MipLevel, ", slice ", ArraySlice, " of texture '", TexDesc.Name, "' has already been mapped");
        }
    }
    else if (TexDesc.Usage == USAGE_STAGING)
    {
        auto SubresourceOffset =
            GetStagingTextureSubresourceOffset(TexDesc, ArraySlice, MipLevel, TextureVkImpl::StagingBufferOffsetAlignment);
        const auto MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        // address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
        auto MapStartOffset = SubresourceOffset +
            // For compressed-block formats, RowSize is the size of one compressed row.
            // For non-compressed formats, BlockHeight is 1.
            (pMapRegion->MinZ * MipLevelAttribs.StorageHeight + pMapRegion->MinY) / FmtAttribs.BlockHeight * MipLevelAttribs.RowSize +
            // For non-compressed formats, BlockWidth is 1.
            pMapRegion->MinX / FmtAttribs.BlockWidth * Uint64{FmtAttribs.GetElementSize()};

        MappedData.pData       = TextureVk.GetStagingDataCPUAddress() + MapStartOffset;
        MappedData.Stride      = MipLevelAttribs.RowSize;
        MappedData.DepthStride = MipLevelAttribs.DepthSliceSize;

        if (MapType == MAP_READ)
        {
            if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
            {
                LOG_WARNING_MESSAGE("Vulkan backend never waits for GPU when mapping staging textures for reading. "
                                    "Applications must use fences or other synchronization methods to explicitly synchronize "
                                    "access and use MAP_FLAG_DO_NOT_WAIT flag.");
            }

            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_READ), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_READ flag and can't be mapped for reading");
            // Readback memory is not created with HOST_COHERENT flag, so we have to explicitly invalidate the mapped range
            // to make device writes visible to CPU reads
            VERIFY_EXPR(pMapRegion->MaxZ >= 1 && pMapRegion->MaxY >= 1);
            auto BlockAlignedMaxX = AlignUp(pMapRegion->MaxX, Uint32{FmtAttribs.BlockWidth});
            auto BlockAlignedMaxY = AlignUp(pMapRegion->MaxY, Uint32{FmtAttribs.BlockHeight});
            auto MapEndOffset     = SubresourceOffset +
                ((pMapRegion->MaxZ - 1) * MipLevelAttribs.StorageHeight + (BlockAlignedMaxY - FmtAttribs.BlockHeight)) / FmtAttribs.BlockHeight * MipLevelAttribs.RowSize +
                (BlockAlignedMaxX / FmtAttribs.BlockWidth) * Uint64{FmtAttribs.GetElementSize()};
            TextureVk.InvalidateStagingRange(MapStartOffset, MapEndOffset - MapStartOffset);
        }
        else if (MapType == MAP_WRITE)
        {
            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_WRITE flag and can't be mapped for writing");
            // Nothing needs to be done when mapping texture for writing
        }
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in Vulkan back-end");
    }
}

void DeviceContextVkImpl::UnmapTextureSubresource(ITexture* pTexture,
                                                  Uint32    MipLevel,
                                                  Uint32    ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);

    TextureVkImpl& TextureVk = *ClassPtrCast<TextureVkImpl>(pTexture);
    const auto&    TexDesc   = TextureVk.GetDesc();

    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        auto UploadSpaceIt = m_MappedTextures.find(MappedTextureKey{&TextureVk, MipLevel, ArraySlice});
        if (UploadSpaceIt != m_MappedTextures.end())
        {
            auto& MappedTex = UploadSpaceIt->second;
            CopyBufferToTexture(MappedTex.Allocation.pDynamicMemMgr->GetVkBuffer(),
                                MappedTex.Allocation.AlignedOffset,
                                MappedTex.CopyInfo.RowStrideInTexels,
                                TextureVk,
                                MappedTex.CopyInfo.Region,
                                MipLevel,
                                ArraySlice,
                                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            m_MappedTextures.erase(UploadSpaceIt);
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to unmap mip level ", MipLevel, ", slice ", ArraySlice, " of texture '", TexDesc.Name, "'. The texture has either been unmapped already or has not been mapped");
        }
    }
    else if (TexDesc.Usage == USAGE_STAGING)
    {
        if (TexDesc.CPUAccessFlags & CPU_ACCESS_READ)
        {
            // Nothing needs to be done
        }
        else if (TexDesc.CPUAccessFlags & CPU_ACCESS_WRITE)
        {
            // Upload textures are currently created with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, so
            // there is no need to explicitly flush the mapped range.
        }
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in Vulkan back-end");
    }
}

void DeviceContextVkImpl::FinishCommandList(ICommandList** ppCommandList)
{
    DEV_CHECK_ERR(IsDeferred(), "Only deferred context can record command list");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Finishing command list inside an active render pass.");

    if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
    {
        m_CommandBuffer.EndRenderPass();
    }

    auto vkCmdBuff = m_CommandBuffer.GetVkCmdBuffer();
    auto err       = vkEndCommandBuffer(vkCmdBuff);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to end command buffer");
    (void)err;

    CommandListVkImpl* pCmdListVk{NEW_RC_OBJ(m_CmdListAllocator, "CommandListVkImpl instance", CommandListVkImpl)(m_pDevice, this, vkCmdBuff)};
    pCmdListVk->QueryInterface(IID_CommandList, reinterpret_cast<IObject**>(ppCommandList));

    m_CommandBuffer.Reset();
    m_State          = ContextState{};
    m_pPipelineState = nullptr;
    m_pQueryMgr      = nullptr;

    InvalidateState();

    TDeviceContextBase::FinishCommandList();
}

void DeviceContextVkImpl::ExecuteCommandLists(Uint32               NumCommandLists,
                                              ICommandList* const* ppCommandLists)
{
    DEV_CHECK_ERR(!IsDeferred(), "Only immediate context can execute command list");

    if (NumCommandLists == 0)
        return;
    DEV_CHECK_ERR(ppCommandLists != nullptr, "ppCommandLists must not be null when NumCommandLists is not zero");

    Flush(NumCommandLists, ppCommandLists);

    InvalidateState();
}

void DeviceContextVkImpl::EnqueueSignal(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::EnqueueSignal(pFence, Value, 0);
    m_SignalFences.emplace_back(std::make_pair(Value, ClassPtrCast<FenceVkImpl>(pFence)));
}

void DeviceContextVkImpl::DeviceWaitForFence(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::DeviceWaitForFence(pFence, Value, 0);
    m_WaitFences.emplace_back(std::make_pair(Value, ClassPtrCast<FenceVkImpl>(pFence)));
}

void DeviceContextVkImpl::WaitForIdle()
{
    DEV_CHECK_ERR(!IsDeferred(), "Only immediate contexts can be idled");
    Flush();
    m_pDevice->IdleCommandQueue(GetCommandQueueId(), true);
}

void DeviceContextVkImpl::BeginQuery(IQuery* pQuery)
{
    TDeviceContextBase::BeginQuery(pQuery, 0);

    VERIFY(m_pQueryMgr != nullptr || IsDeferred(), "Query manager should never be null for immediate contexts. This might be a bug.");
    DEV_CHECK_ERR(m_pQueryMgr != nullptr, "Query manager is null, which indicates that this deferred context is not in a recording state");

    auto*      pQueryVkImpl = ClassPtrCast<QueryVkImpl>(pQuery);
    const auto QueryType    = pQueryVkImpl->GetDesc().Type;
    auto       vkQueryPool  = m_pQueryMgr->GetQueryPool(QueryType);
    auto       Idx          = pQueryVkImpl->GetQueryPoolIndex(0);

    VERIFY(vkQueryPool != VK_NULL_HANDLE, "Query pool is not initialized for query type");

    EnsureVkCmdBuffer();
    if (QueryType == QUERY_TYPE_TIMESTAMP)
    {
        LOG_ERROR_MESSAGE("BeginQuery() is disabled for timestamp queries");
    }
    else if (QueryType == QUERY_TYPE_DURATION)
    {
        m_CommandBuffer.WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkQueryPool, Idx);
    }
    else
    {
        const auto& CmdBuffState = m_CommandBuffer.GetState();
        if ((CmdBuffState.InsidePassQueries | CmdBuffState.OutsidePassQueries) & (1u << QueryType))
        {
            LOG_ERROR_MESSAGE("Another query of type ", GetQueryTypeString(QueryType),
                              " is currently active. Overlapping queries do not work in Vulkan. "
                              "End the first query before beginning another one.");
            return;
        }

        // A query must either begin and end inside the same subpass of a render pass instance, or must
        // both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)

        ++m_ActiveQueriesCounter;
        m_CommandBuffer.BeginQuery(vkQueryPool,
                                   Idx,
                                   // If flags does not contain VK_QUERY_CONTROL_PRECISE_BIT an implementation
                                   // may generate any non-zero result value for the query if the count of
                                   // passing samples is non-zero (17.3).
                                   QueryType == QUERY_TYPE_OCCLUSION ? VK_QUERY_CONTROL_PRECISE_BIT : 0,
                                   1u << QueryType);
    }
}

void DeviceContextVkImpl::EndQuery(IQuery* pQuery)
{
    TDeviceContextBase::EndQuery(pQuery, 0);

    VERIFY(m_pQueryMgr != nullptr || IsDeferred(), "Query manager should never be null for immediate contexts. This might be a bug.");
    DEV_CHECK_ERR(m_pQueryMgr != nullptr, "Query manager is null, which indicates that this deferred context is not in a recording state");

    auto*      pQueryVkImpl = ClassPtrCast<QueryVkImpl>(pQuery);
    const auto QueryType    = pQueryVkImpl->GetDesc().Type;
    auto       vkQueryPool  = m_pQueryMgr->GetQueryPool(QueryType);
    auto       Idx          = pQueryVkImpl->GetQueryPoolIndex(QueryType == QUERY_TYPE_DURATION ? 1 : 0);

    VERIFY(vkQueryPool != VK_NULL_HANDLE, "Query pool is not initialized for query type");

    EnsureVkCmdBuffer();
    if (QueryType == QUERY_TYPE_TIMESTAMP || QueryType == QUERY_TYPE_DURATION)
    {
        m_CommandBuffer.WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkQueryPool, Idx);
    }
    else
    {
        VERIFY(m_ActiveQueriesCounter > 0, "Active query counter is 0 which means there was a mismatch between BeginQuery() / EndQuery() calls");

        // A query must either begin and end inside the same subpass of a render pass instance, or must
        // both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)
        const auto& CmdBuffState = m_CommandBuffer.GetState();
        VERIFY((CmdBuffState.InsidePassQueries | CmdBuffState.OutsidePassQueries) & (1u << QueryType),
               "No query flag is set which indicates there was no matching BeginQuery call or there was an error while beginning the query.");
        if (CmdBuffState.OutsidePassQueries & (1 << QueryType))
        {
            if (m_CommandBuffer.GetState().RenderPass)
                m_CommandBuffer.EndRenderPass();
        }
        else
        {
            if (!m_CommandBuffer.GetState().RenderPass)
                LOG_ERROR_MESSAGE("The query was started inside render pass, but is being ended outside of render pass. "
                                  "Vulkan requires that a query must either begin and end inside the same "
                                  "subpass of a render pass instance, or must both begin and end outside of a render pass "
                                  "instance (i.e. contain entire render pass instances). (17.2)");
        }

        --m_ActiveQueriesCounter;
        m_CommandBuffer.EndQuery(vkQueryPool, Idx, 1u << QueryType);
    }
}


void DeviceContextVkImpl::TransitionImageLayout(ITexture* pTexture, VkImageLayout NewLayout)
{
    VERIFY_EXPR(pTexture != nullptr);
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
    auto pTextureVk = ClassPtrCast<TextureVkImpl>(pTexture);
    if (!pTextureVk->IsInKnownState())
    {
        LOG_ERROR_MESSAGE("Failed to transition layout for texture '", pTextureVk->GetDesc().Name, "' because the texture state is unknown");
        return;
    }
    auto NewState = VkImageLayoutToResourceState(NewLayout);
    if (!pTextureVk->CheckState(NewState))
    {
        TransitionTextureState(*pTextureVk, RESOURCE_STATE_UNKNOWN, NewState, STATE_TRANSITION_FLAG_UPDATE_STATE);
    }
}

namespace
{
NODISCARD inline bool ResourceStateHasWriteAccess(RESOURCE_STATE State)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    constexpr RESOURCE_STATE WriteAccessStates =
        RESOURCE_STATE_RENDER_TARGET |
        RESOURCE_STATE_UNORDERED_ACCESS |
        RESOURCE_STATE_COPY_DEST |
        RESOURCE_STATE_RESOLVE_DEST |
        RESOURCE_STATE_BUILD_AS_WRITE;

    return State & WriteAccessStates;
}
} // namespace

void DeviceContextVkImpl::TransitionTextureState(TextureVkImpl&           TextureVk,
                                                 RESOURCE_STATE           OldState,
                                                 RESOURCE_STATE           NewState,
                                                 STATE_TRANSITION_FLAGS   Flags,
                                                 VkImageSubresourceRange* pSubresRange /* = nullptr*/)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
    if (OldState == RESOURCE_STATE_UNKNOWN)
    {
        if (TextureVk.IsInKnownState())
        {
            OldState = TextureVk.GetState();
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to transition the state of texture '", TextureVk.GetDesc().Name, "' because the state is unknown and is not explicitly specified.");
            return;
        }
    }
    else
    {
        if (TextureVk.IsInKnownState() && TextureVk.GetState() != OldState)
        {
            LOG_ERROR_MESSAGE("The state ", GetResourceStateString(TextureVk.GetState()), " of texture '",
                              TextureVk.GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                              " specified by the barrier");
        }
    }

    EnsureVkCmdBuffer();

    auto vkImg = TextureVk.GetVkImage();

    VkImageSubresourceRange FullSubresRange;
    if (pSubresRange == nullptr)
    {
        pSubresRange = &FullSubresRange;

        FullSubresRange.aspectMask     = 0;
        FullSubresRange.baseArrayLayer = 0;
        FullSubresRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        FullSubresRange.baseMipLevel   = 0;
        FullSubresRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    }

    if (pSubresRange->aspectMask == 0)
    {
        const auto& TexDesc    = TextureVk.GetDesc();
        const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            pSubresRange->aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            // If image has a depth / stencil format with both depth and stencil components, then the
            // aspectMask member of subresourceRange must include both VK_IMAGE_ASPECT_DEPTH_BIT and
            // VK_IMAGE_ASPECT_STENCIL_BIT (6.7.3)
            pSubresRange->aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            pSubresRange->aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Always add barrier after writes.
    const bool AfterWrite = ResourceStateHasWriteAccess(OldState);

    const auto& ExtFeatures    = m_pDevice->GetLogicalDevice().GetEnabledExtFeatures();
    const auto  FragDensityMap = ExtFeatures.FragmentDensityMap.fragmentDensityMap != VK_FALSE;
    const auto  OldLayout      = (Flags & STATE_TRANSITION_FLAG_DISCARD_CONTENT) != 0 ? VK_IMAGE_LAYOUT_UNDEFINED : ResourceStateToVkImageLayout(OldState, /*IsInsideRenderPass = */ false, FragDensityMap);
    const auto  NewLayout      = ResourceStateToVkImageLayout(NewState, /*IsInsideRenderPass = */ false, FragDensityMap);
    const auto  OldStages      = ResourceStateFlagsToVkPipelineStageFlags(OldState);
    const auto  NewStages      = ResourceStateFlagsToVkPipelineStageFlags(NewState);

    if (((OldState & NewState) != NewState) || OldLayout != NewLayout || AfterWrite)
    {
        m_CommandBuffer.TransitionImageLayout(vkImg, OldLayout, NewLayout, *pSubresRange, OldStages, NewStages);
        if ((Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) != 0)
        {
            TextureVk.SetState(NewState);
            VERIFY_EXPR(TextureVk.GetLayout() == NewLayout);
        }
    }
}

void DeviceContextVkImpl::TransitionOrVerifyTextureState(TextureVkImpl&                 Texture,
                                                         RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                         RESOURCE_STATE                 RequiredState,
                                                         VkImageLayout                  ExpectedLayout,
                                                         const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
        if (Texture.IsInKnownState())
        {
            TransitionTextureState(Texture, RESOURCE_STATE_UNKNOWN, RequiredState, STATE_TRANSITION_FLAG_UPDATE_STATE);
            VERIFY_EXPR(Texture.GetLayout() == ExpectedLayout);
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTextureState(Texture, RequiredState, OperationName);
    }
#endif
}

void DeviceContextVkImpl::BufferMemoryBarrier(IBuffer* pBuffer, VkAccessFlags NewAccessFlags)
{
    VERIFY_EXPR(pBuffer != nullptr);
    auto pBuffVk = ClassPtrCast<BufferVkImpl>(pBuffer);
    if (!pBuffVk->IsInKnownState())
    {
        LOG_ERROR_MESSAGE("Failed to execute buffer memory barrier for buffer '", pBuffVk->GetDesc().Name, "' because the buffer state is unknown");
        return;
    }
    auto NewState = VkAccessFlagsToResourceStates(NewAccessFlags);
    if ((pBuffVk->GetState() & NewState) != NewState)
    {
        TransitionBufferState(*pBuffVk, RESOURCE_STATE_UNKNOWN, NewState, true);
    }
}

VkCommandBuffer DeviceContextVkImpl::GetVkCommandBuffer()
{
    EnsureVkCmdBuffer();
    m_CommandBuffer.FlushBarriers();
    return m_CommandBuffer.GetVkCmdBuffer();
}

void DeviceContextVkImpl::TransitionBufferState(BufferVkImpl& BufferVk, RESOURCE_STATE OldState, RESOURCE_STATE NewState, bool UpdateBufferState)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
    if (OldState == RESOURCE_STATE_UNKNOWN)
    {
        if (BufferVk.IsInKnownState())
        {
            OldState = BufferVk.GetState();
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to transition the state of buffer '", BufferVk.GetDesc().Name, "' because the buffer state is unknown and is not explicitly specified");
            return;
        }
    }
    else
    {
        if (BufferVk.IsInKnownState() && BufferVk.GetState() != OldState)
        {
            LOG_ERROR_MESSAGE("The state ", GetResourceStateString(BufferVk.GetState()), " of buffer '",
                              BufferVk.GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                              " specified by the barrier");
        }
    }

    // Always add barrier after writes.
    const bool AfterWrite = ResourceStateHasWriteAccess(OldState);

    if (((OldState & NewState) != NewState) || AfterWrite)
    {
        DEV_CHECK_ERR(BufferVk.m_VulkanBuffer != VK_NULL_HANDLE, "Cannot transition suballocated buffer");
        VERIFY_EXPR(BufferVk.GetDynamicOffset(GetContextId(), this) == 0);

        EnsureVkCmdBuffer();
        auto OldAccessFlags = ResourceStateFlagsToVkAccessFlags(OldState);
        auto NewAccessFlags = ResourceStateFlagsToVkAccessFlags(NewState);
        auto OldStages      = ResourceStateFlagsToVkPipelineStageFlags(OldState);
        auto NewStages      = ResourceStateFlagsToVkPipelineStageFlags(NewState);
        m_CommandBuffer.MemoryBarrier(OldAccessFlags, NewAccessFlags, OldStages, NewStages);
        if (UpdateBufferState)
        {
            BufferVk.SetState(NewState);
        }
    }
}

void DeviceContextVkImpl::TransitionOrVerifyBufferState(BufferVkImpl&                  Buffer,
                                                        RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                        RESOURCE_STATE                 RequiredState,
                                                        VkAccessFlagBits               ExpectedAccessFlags,
                                                        const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
        if (Buffer.IsInKnownState())
        {
            TransitionBufferState(Buffer, RESOURCE_STATE_UNKNOWN, RequiredState, true);
            VERIFY_EXPR(Buffer.CheckAccessFlags(ExpectedAccessFlags));
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyBufferState(Buffer, RequiredState, OperationName);
    }
#endif
}

void DeviceContextVkImpl::TransitionBLASState(BottomLevelASVkImpl& BLAS,
                                              RESOURCE_STATE       OldState,
                                              RESOURCE_STATE       NewState,
                                              bool                 UpdateInternalState)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
    if (OldState == RESOURCE_STATE_UNKNOWN)
    {
        if (BLAS.IsInKnownState())
        {
            OldState = BLAS.GetState();
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to transition the state of BLAS '", BLAS.GetDesc().Name, "' because the BLAS state is unknown and is not explicitly specified");
            return;
        }
    }
    else
    {
        if (BLAS.IsInKnownState() && BLAS.GetState() != OldState)
        {
            LOG_ERROR_MESSAGE("The state ", GetResourceStateString(BLAS.GetState()), " of BLAS '",
                              BLAS.GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                              " specified by the barrier");
        }
    }

    // Always add barrier after writes.
    const bool AfterWrite = ResourceStateHasWriteAccess(OldState);

    if ((OldState & NewState) != NewState || AfterWrite)
    {
        EnsureVkCmdBuffer();
        auto OldAccessFlags = AccelStructStateFlagsToVkAccessFlags(OldState);
        auto NewAccessFlags = AccelStructStateFlagsToVkAccessFlags(NewState);
        auto OldStages      = ResourceStateFlagsToVkPipelineStageFlags(OldState);
        auto NewStages      = ResourceStateFlagsToVkPipelineStageFlags(NewState);
        m_CommandBuffer.MemoryBarrier(OldAccessFlags, NewAccessFlags, OldStages, NewStages);
        if (UpdateInternalState)
        {
            BLAS.SetState(NewState);
        }
    }
}

void DeviceContextVkImpl::TransitionTLASState(TopLevelASVkImpl& TLAS,
                                              RESOURCE_STATE    OldState,
                                              RESOURCE_STATE    NewState,
                                              bool              UpdateInternalState)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
    if (OldState == RESOURCE_STATE_UNKNOWN)
    {
        if (TLAS.IsInKnownState())
        {
            OldState = TLAS.GetState();
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to transition the state of TLAS '", TLAS.GetDesc().Name, "' because the TLAS state is unknown and is not explicitly specified");
            return;
        }
    }
    else
    {
        if (TLAS.IsInKnownState() && TLAS.GetState() != OldState)
        {
            LOG_ERROR_MESSAGE("The state ", GetResourceStateString(TLAS.GetState()), " of TLAS '",
                              TLAS.GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                              " specified by the barrier");
        }
    }

    // Always add barrier after writes.
    const bool AfterWrite = ResourceStateHasWriteAccess(OldState);

    if ((OldState & NewState) != NewState || AfterWrite)
    {
        EnsureVkCmdBuffer();
        auto OldAccessFlags = AccelStructStateFlagsToVkAccessFlags(OldState);
        auto NewAccessFlags = AccelStructStateFlagsToVkAccessFlags(NewState);
        auto OldStages      = ResourceStateFlagsToVkPipelineStageFlags(OldState);
        auto NewStages      = ResourceStateFlagsToVkPipelineStageFlags(NewState);
        m_CommandBuffer.MemoryBarrier(OldAccessFlags, NewAccessFlags, OldStages, NewStages);
        if (UpdateInternalState)
        {
            TLAS.SetState(NewState);
        }
    }
}

void DeviceContextVkImpl::TransitionOrVerifyBLASState(BottomLevelASVkImpl&           BLAS,
                                                      RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                      RESOURCE_STATE                 RequiredState,
                                                      const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
        if (BLAS.IsInKnownState())
        {
            TransitionBLASState(BLAS, RESOURCE_STATE_UNKNOWN, RequiredState, true);
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyBLASState(BLAS, RequiredState, OperationName);
    }
#endif
}

void DeviceContextVkImpl::TransitionOrVerifyTLASState(TopLevelASVkImpl&              TLAS,
                                                      RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                      RESOURCE_STATE                 RequiredState,
                                                      const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
        if (TLAS.IsInKnownState())
        {
            TransitionTLASState(TLAS, RESOURCE_STATE_UNKNOWN, RequiredState, true);
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTLASState(TLAS, RequiredState, OperationName);
    }

    if (RequiredState & (RESOURCE_STATE_RAY_TRACING | RESOURCE_STATE_BUILD_AS_READ))
    {
        TLAS.ValidateContent();
    }
#endif
}

VulkanDynamicAllocation DeviceContextVkImpl::AllocateDynamicSpace(Uint64 SizeInBytes, Uint32 Alignment)
{
    DEV_CHECK_ERR(SizeInBytes < std::numeric_limits<Uint32>::max(),
                  "Dynamic allocation size must be less than 2^32");

    auto DynAlloc = m_DynamicHeap.Allocate(static_cast<Uint32>(SizeInBytes), Alignment);
#ifdef DILIGENT_DEVELOPMENT
    DynAlloc.dvpFrameNumber = GetFrameNumber();
#endif
    return DynAlloc;
}

void DeviceContextVkImpl::TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");

    if (BarrierCount == 0)
        return;

    EnsureVkCmdBuffer();

    for (Uint32 i = 0; i < BarrierCount; ++i)
    {
        const auto& Barrier = pResourceBarriers[i];
#ifdef DILIGENT_DEVELOPMENT
        DvpVerifyStateTransitionDesc(Barrier);
#endif
        if (Barrier.TransitionType == STATE_TRANSITION_TYPE_BEGIN)
        {
            // Skip begin-split barriers
            VERIFY((Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) == 0, "Resource state can't be updated in begin-split barrier");
            continue;
        }
        if (Barrier.Flags & STATE_TRANSITION_FLAG_ALIASING)
        {
            AliasingBarrier(Barrier.pResourceBefore, Barrier.pResource);
        }
        else
        {
            VERIFY(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE || Barrier.TransitionType == STATE_TRANSITION_TYPE_END, "Unexpected barrier type");

            if (RefCntAutoPtr<TextureVkImpl> pTexture{Barrier.pResource, IID_TextureVk})
            {
                VkImageSubresourceRange SubResRange;
                SubResRange.aspectMask     = 0;
                SubResRange.baseMipLevel   = Barrier.FirstMipLevel;
                SubResRange.levelCount     = (Barrier.MipLevelsCount == REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : Barrier.MipLevelsCount;
                SubResRange.baseArrayLayer = Barrier.FirstArraySlice;
                SubResRange.layerCount     = (Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES) ? VK_REMAINING_ARRAY_LAYERS : Barrier.ArraySliceCount;
                TransitionTextureState(*pTexture, Barrier.OldState, Barrier.NewState, Barrier.Flags, &SubResRange);
            }
            else if (RefCntAutoPtr<BufferVkImpl> pBuffer{Barrier.pResource, IID_BufferVk})
            {
                TransitionBufferState(*pBuffer, Barrier.OldState, Barrier.NewState, (Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) != 0);
            }
            else if (RefCntAutoPtr<BottomLevelASVkImpl> pBottomLevelAS{Barrier.pResource, IID_BottomLevelAS})
            {
                TransitionBLASState(*pBottomLevelAS, Barrier.OldState, Barrier.NewState, (Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) != 0);
            }
            else if (RefCntAutoPtr<TopLevelASVkImpl> pTopLevelAS{Barrier.pResource, IID_TopLevelAS})
            {
                TransitionTLASState(*pTopLevelAS, Barrier.OldState, Barrier.NewState, (Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) != 0);
            }
            else
            {
                UNEXPECTED("unsupported resource type");
            }
        }
    }
}

void DeviceContextVkImpl::AliasingBarrier(IDeviceObject* pResourceBefore, IDeviceObject* pResourceAfter)
{
    auto GetResourceBindFlags = [](IDeviceObject* pResource) //
    {
        if (RefCntAutoPtr<ITextureVk> pTexture{pResource, IID_TextureVk})
        {
            return pTexture.RawPtr<TextureVkImpl>()->GetDesc().BindFlags;
        }
        else if (RefCntAutoPtr<IBufferVk> pBuffer{pResource, IID_BufferVk})
        {
            return pBuffer.RawPtr<BufferVkImpl>()->GetDesc().BindFlags;
        }
        else
        {
            constexpr auto BindAll = static_cast<BIND_FLAGS>((Uint32{BIND_FLAG_LAST} << 1) - 1);
            return BindAll;
        }
    };

    VkPipelineStageFlags vkSrcStages     = 0;
    VkAccessFlags        vkSrcAccessMask = 0;
    GetAllowedStagesAndAccessMask(GetResourceBindFlags(pResourceBefore), vkSrcStages, vkSrcAccessMask);

    VkPipelineStageFlags vkDstStages     = 0;
    VkAccessFlags        vkDstAccessMask = 0;
    GetAllowedStagesAndAccessMask(GetResourceBindFlags(pResourceAfter), vkDstStages, vkDstAccessMask);

    EnsureVkCmdBuffer();
    m_CommandBuffer.MemoryBarrier(vkSrcAccessMask, vkDstAccessMask, vkSrcStages, vkDstStages);
}

void DeviceContextVkImpl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                    ITexture*                               pDstTexture,
                                                    const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);

    auto*       pSrcTexVk  = ClassPtrCast<TextureVkImpl>(pSrcTexture);
    auto*       pDstTexVk  = ClassPtrCast<TextureVkImpl>(pDstTexture);
    const auto& SrcTexDesc = pSrcTexVk->GetDesc();
    const auto& DstTexDesc = pDstTexVk->GetDesc();

    DEV_CHECK_ERR(SrcTexDesc.Format == DstTexDesc.Format, "Vulkan requires that source and destination textures of a resolve operation "
                                                          "have the same format (18.6)");
    (void)DstTexDesc;

    EnsureVkCmdBuffer();
    // srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.6)
    TransitionOrVerifyTextureState(*pSrcTexVk, ResolveAttribs.SrcTextureTransitionMode, RESOURCE_STATE_RESOLVE_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   "Resolving multi-sampled texture (DeviceContextVkImpl::ResolveTextureSubresource)");

    // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.6)
    TransitionOrVerifyTextureState(*pDstTexVk, ResolveAttribs.DstTextureTransitionMode, RESOURCE_STATE_RESOLVE_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   "Resolving multi-sampled texture (DeviceContextVkImpl::ResolveTextureSubresource)");

    const auto& ResolveFmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
    DEV_CHECK_ERR(ResolveFmtAttribs.ComponentType != COMPONENT_TYPE_DEPTH && ResolveFmtAttribs.ComponentType != COMPONENT_TYPE_DEPTH_STENCIL,
                  "Vulkan only allows resolve operation for color formats");
    (void)ResolveFmtAttribs;
    // The aspectMask member of srcSubresource and dstSubresource must only contain VK_IMAGE_ASPECT_COLOR_BIT (18.6)
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageResolve ResolveRegion;
    ResolveRegion.srcSubresource.baseArrayLayer = ResolveAttribs.SrcSlice;
    ResolveRegion.srcSubresource.layerCount     = 1;
    ResolveRegion.srcSubresource.mipLevel       = ResolveAttribs.SrcMipLevel;
    ResolveRegion.srcSubresource.aspectMask     = aspectMask;

    ResolveRegion.dstSubresource.baseArrayLayer = ResolveAttribs.DstSlice;
    ResolveRegion.dstSubresource.layerCount     = 1;
    ResolveRegion.dstSubresource.mipLevel       = ResolveAttribs.DstMipLevel;
    ResolveRegion.dstSubresource.aspectMask     = aspectMask;

    ResolveRegion.srcOffset = VkOffset3D{};
    ResolveRegion.dstOffset = VkOffset3D{};
    const auto& MipAttribs  = GetMipLevelProperties(SrcTexDesc, ResolveAttribs.SrcMipLevel);
    ResolveRegion.extent    = VkExtent3D{
        static_cast<uint32_t>(MipAttribs.LogicalWidth),
        static_cast<uint32_t>(MipAttribs.LogicalHeight),
        static_cast<uint32_t>(MipAttribs.Depth)};

    m_CommandBuffer.ResolveImage(pSrcTexVk->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 pDstTexVk->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 1, &ResolveRegion);
}

void DeviceContextVkImpl::BuildBLAS(const BuildBLASAttribs& Attribs)
{
    TDeviceContextBase::BuildBLAS(Attribs, 0);

    auto* pBLASVk    = ClassPtrCast<BottomLevelASVkImpl>(Attribs.pBLAS);
    auto* pScratchVk = ClassPtrCast<BufferVkImpl>(Attribs.pScratchBuffer);
    auto& BLASDesc   = pBLASVk->GetDesc();

    EnsureVkCmdBuffer();

    const char* OpName = "Build BottomLevelAS (DeviceContextVkImpl::BuildBLAS)";
    TransitionOrVerifyBLASState(*pBLASVk, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);
    TransitionOrVerifyBufferState(*pScratchVk, Attribs.ScratchBufferTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, OpName);

    VkAccelerationStructureBuildGeometryInfoKHR           vkASBuildInfo = {};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> vkRanges;
    std::vector<VkAccelerationStructureGeometryKHR>       vkGeometries;

    if (Attribs.pTriangleData != nullptr)
    {
        vkGeometries.resize(Attribs.TriangleDataCount);
        vkRanges.resize(Attribs.TriangleDataCount);
        pBLASVk->SetActualGeometryCount(Attribs.TriangleDataCount);

        for (Uint32 i = 0; i < Attribs.TriangleDataCount; ++i)
        {
            const auto& SrcTris = Attribs.pTriangleData[i];
            Uint32      Idx     = i;
            Uint32      GeoIdx  = pBLASVk->UpdateGeometryIndex(SrcTris.GeometryName, Idx, Attribs.Update);

            if (GeoIdx == INVALID_INDEX || Idx == INVALID_INDEX)
            {
                UNEXPECTED("Failed to find geometry by name");
                continue;
            }

            auto&       vkGeo   = vkGeometries[Idx];
            auto&       vkTris  = vkGeo.geometry.triangles;
            auto&       off     = vkRanges[Idx];
            const auto& TriDesc = BLASDesc.pTriangles[GeoIdx];

            vkGeo.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            vkGeo.pNext        = nullptr;
            vkGeo.flags        = GeometryFlagsToVkGeometryFlags(SrcTris.Flags);
            vkGeo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            vkTris.sType       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            vkTris.pNext       = nullptr;

            auto* const pVB = ClassPtrCast<BufferVkImpl>(SrcTris.pVertexBuffer);

            // vertex format in SrcTris may be undefined, so use vertex format from description
            vkTris.vertexFormat = TypeToVkFormat(TriDesc.VertexValueType, TriDesc.VertexComponentCount, TriDesc.VertexValueType < VT_FLOAT16);
            vkTris.vertexStride = SrcTris.VertexStride;
            // maxVertex is the number of vertices in vertexData minus one.
            VERIFY(SrcTris.VertexCount > 0, "Vertex count must be greater than 0");
            vkTris.maxVertex                = SrcTris.VertexCount - 1;
            vkTris.vertexData.deviceAddress = pVB->GetVkDeviceAddress() + SrcTris.VertexOffset;

            // geometry.triangles.vertexData.deviceAddress must be aligned to the size in bytes of the smallest component of the format in vertexFormat
            VERIFY(vkTris.vertexData.deviceAddress % GetValueSize(TriDesc.VertexValueType) == 0, "Vertex buffer start address is not properly aligned");

            TransitionOrVerifyBufferState(*pVB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, VK_ACCESS_SHADER_READ_BIT, OpName);

            if (SrcTris.pIndexBuffer)
            {
                auto* const pIB = ClassPtrCast<BufferVkImpl>(SrcTris.pIndexBuffer);

                // index type in SrcTris may be undefined, so use index type from description
                vkTris.indexType               = TypeToVkIndexType(TriDesc.IndexType);
                vkTris.indexData.deviceAddress = pIB->GetVkDeviceAddress() + SrcTris.IndexOffset;

                // geometry.triangles.indexData.deviceAddress must be aligned to the size in bytes of the type in indexType
                VERIFY(vkTris.indexData.deviceAddress % GetValueSize(TriDesc.IndexType) == 0, "Index buffer start address is not properly aligned");

                TransitionOrVerifyBufferState(*pIB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, VK_ACCESS_SHADER_READ_BIT, OpName);
            }
            else
            {
                vkTris.indexType               = VK_INDEX_TYPE_NONE_KHR;
                vkTris.indexData.deviceAddress = 0;
            }

            if (SrcTris.pTransformBuffer)
            {
                auto* const pTB                    = ClassPtrCast<BufferVkImpl>(SrcTris.pTransformBuffer);
                vkTris.transformData.deviceAddress = pTB->GetVkDeviceAddress() + SrcTris.TransformBufferOffset;

                // If geometry.triangles.transformData.deviceAddress is not 0, it must be aligned to 16 bytes
                VERIFY(vkTris.indexData.deviceAddress % 16 == 0, "Transform buffer start address is not properly aligned");

                TransitionOrVerifyBufferState(*pTB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, VK_ACCESS_SHADER_READ_BIT, OpName);
            }
            else
            {
                vkTris.transformData.deviceAddress = 0;
            }

            off.primitiveCount  = SrcTris.PrimitiveCount;
            off.firstVertex     = 0;
            off.primitiveOffset = 0;
            off.transformOffset = 0;
        }
    }
    else if (Attribs.pBoxData != nullptr)
    {
        vkGeometries.resize(Attribs.BoxDataCount);
        vkRanges.resize(Attribs.BoxDataCount);
        pBLASVk->SetActualGeometryCount(Attribs.BoxDataCount);

        for (Uint32 i = 0; i < Attribs.BoxDataCount; ++i)
        {
            const auto& SrcBoxes = Attribs.pBoxData[i];
            Uint32      Idx      = i;
            Uint32      GeoIdx   = pBLASVk->UpdateGeometryIndex(SrcBoxes.GeometryName, Idx, Attribs.Update);

            if (GeoIdx == INVALID_INDEX || Idx == INVALID_INDEX)
            {
                UNEXPECTED("Failed to find geometry by name");
                continue;
            }

            auto& vkGeo   = vkGeometries[Idx];
            auto& vkAABBs = vkGeo.geometry.aabbs;
            auto& off     = vkRanges[Idx];

            vkGeo.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            vkGeo.pNext        = nullptr;
            vkGeo.flags        = GeometryFlagsToVkGeometryFlags(SrcBoxes.Flags);
            vkGeo.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;

            auto* const pBB            = ClassPtrCast<BufferVkImpl>(SrcBoxes.pBoxBuffer);
            vkAABBs.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
            vkAABBs.pNext              = nullptr;
            vkAABBs.stride             = SrcBoxes.BoxStride;
            vkAABBs.data.deviceAddress = pBB->GetVkDeviceAddress() + SrcBoxes.BoxOffset;

            // geometry.aabbs.data.deviceAddress must be aligned to 8 bytes
            VERIFY(vkAABBs.data.deviceAddress % 8 == 0, "AABB start address is not properly aligned");

            TransitionOrVerifyBufferState(*pBB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, VK_ACCESS_SHADER_READ_BIT, OpName);

            off.firstVertex     = 0;
            off.transformOffset = 0;
            off.primitiveOffset = 0;
            off.primitiveCount  = SrcBoxes.BoxCount;
        }
    }

    VkAccelerationStructureBuildRangeInfoKHR const* VkRangePtr = vkRanges.data();

    vkASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    vkASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;                 // type must be compatible with create info
    vkASBuildInfo.flags                     = BuildASFlagsToVkBuildAccelerationStructureFlags(BLASDesc.Flags); // flags must be compatible with create info
    vkASBuildInfo.mode                      = Attribs.Update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    vkASBuildInfo.srcAccelerationStructure  = Attribs.Update ? pBLASVk->GetVkBLAS() : VK_NULL_HANDLE;
    vkASBuildInfo.dstAccelerationStructure  = pBLASVk->GetVkBLAS();
    vkASBuildInfo.geometryCount             = static_cast<uint32_t>(vkGeometries.size());
    vkASBuildInfo.pGeometries               = vkGeometries.data();
    vkASBuildInfo.ppGeometries              = nullptr;
    vkASBuildInfo.scratchData.deviceAddress = pScratchVk->GetVkDeviceAddress() + Attribs.ScratchBufferOffset;

    const auto& ASLimits = m_pDevice->GetPhysicalDevice().GetExtProperties().AccelStruct;
    VERIFY(vkASBuildInfo.scratchData.deviceAddress % ASLimits.minAccelerationStructureScratchOffsetAlignment == 0, "Scratch buffer start address is not properly aligned");

    EnsureVkCmdBuffer();
    m_CommandBuffer.BuildAccelerationStructure(1, &vkASBuildInfo, &VkRangePtr);
    ++m_State.NumCommands;

#ifdef DILIGENT_DEVELOPMENT
    pBLASVk->DvpUpdateVersion();
#endif
}

void DeviceContextVkImpl::BuildTLAS(const BuildTLASAttribs& Attribs)
{
    TDeviceContextBase::BuildTLAS(Attribs, 0);

    static_assert(TLAS_INSTANCE_DATA_SIZE == sizeof(VkAccelerationStructureInstanceKHR), "Value in TLAS_INSTANCE_DATA_SIZE doesn't match the actual instance description size");

    auto* pTLASVk      = ClassPtrCast<TopLevelASVkImpl>(Attribs.pTLAS);
    auto* pScratchVk   = ClassPtrCast<BufferVkImpl>(Attribs.pScratchBuffer);
    auto* pInstancesVk = ClassPtrCast<BufferVkImpl>(Attribs.pInstanceBuffer);
    auto& TLASDesc     = pTLASVk->GetDesc();

    EnsureVkCmdBuffer();

    const char* OpName = "Build TopLevelAS (DeviceContextVkImpl::BuildTLAS)";
    TransitionOrVerifyTLASState(*pTLASVk, Attribs.TLASTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);
    TransitionOrVerifyBufferState(*pScratchVk, Attribs.ScratchBufferTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, OpName);

    if (Attribs.Update)
    {
        if (!pTLASVk->UpdateInstances(Attribs.pInstances, Attribs.InstanceCount, Attribs.BaseContributionToHitGroupIndex, Attribs.HitGroupStride, Attribs.BindingMode))
            return;
    }
    else
    {
        if (!pTLASVk->SetInstanceData(Attribs.pInstances, Attribs.InstanceCount, Attribs.BaseContributionToHitGroupIndex, Attribs.HitGroupStride, Attribs.BindingMode))
            return;
    }

    // copy instance data into instance buffer
    {
        size_t Size     = Attribs.InstanceCount * sizeof(VkAccelerationStructureInstanceKHR);
        auto   TmpSpace = m_UploadHeap.Allocate(Size, 16);

        for (Uint32 i = 0; i < Attribs.InstanceCount; ++i)
        {
            const auto& Inst     = Attribs.pInstances[i];
            const auto  InstDesc = pTLASVk->GetInstanceDesc(Inst.InstanceName);

            if (InstDesc.InstanceIndex >= Attribs.InstanceCount)
            {
                UNEXPECTED("Failed to find instance by name");
                return;
            }

            auto& vkASInst = static_cast<VkAccelerationStructureInstanceKHR*>(TmpSpace.CPUAddress)[InstDesc.InstanceIndex];
            auto* pBLASVk  = ClassPtrCast<BottomLevelASVkImpl>(Inst.pBLAS);

            static_assert(sizeof(vkASInst.transform) == sizeof(Inst.Transform), "size mismatch");
            std::memcpy(&vkASInst.transform, Inst.Transform.data, sizeof(vkASInst.transform));

            vkASInst.instanceCustomIndex                    = Inst.CustomId;
            vkASInst.instanceShaderBindingTableRecordOffset = InstDesc.ContributionToHitGroupIndex;
            vkASInst.mask                                   = Inst.Mask;
            vkASInst.flags                                  = InstanceFlagsToVkGeometryInstanceFlags(Inst.Flags);
            vkASInst.accelerationStructureReference         = pBLASVk->GetVkDeviceAddress();

            TransitionOrVerifyBLASState(*pBLASVk, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
        }

        UpdateBufferRegion(pInstancesVk, Attribs.InstanceBufferOffset, Size, TmpSpace.vkBuffer, TmpSpace.AlignedOffset, Attribs.InstanceBufferTransitionMode);
    }
    TransitionOrVerifyBufferState(*pInstancesVk, Attribs.InstanceBufferTransitionMode, RESOURCE_STATE_BUILD_AS_READ, VK_ACCESS_SHADER_READ_BIT, OpName);

    VkAccelerationStructureBuildGeometryInfoKHR     vkASBuildInfo = {};
    VkAccelerationStructureBuildRangeInfoKHR        vkRange       = {};
    VkAccelerationStructureBuildRangeInfoKHR const* vkRangePtr    = &vkRange;
    VkAccelerationStructureGeometryKHR              vkASGeometry  = {};

    vkRange.primitiveCount = Attribs.InstanceCount;

    vkASGeometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    vkASGeometry.pNext        = nullptr;
    vkASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    vkASGeometry.flags        = 0;

    auto& vkASInst              = vkASGeometry.geometry.instances;
    vkASInst.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    vkASInst.pNext              = nullptr;
    vkASInst.arrayOfPointers    = VK_FALSE;
    vkASInst.data.deviceAddress = pInstancesVk->GetVkDeviceAddress() + Attribs.InstanceBufferOffset;

    // if geometry.arrayOfPointers is VK_FALSE, geometry.instances.data.deviceAddress must be aligned to 16 bytes
    VERIFY(vkASInst.data.deviceAddress % 16 == 0, "Instance data address is not properly aligned");

    vkASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    vkASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;                    // type must be compatible with create info
    vkASBuildInfo.flags                     = BuildASFlagsToVkBuildAccelerationStructureFlags(TLASDesc.Flags); // flags must be compatible with create info
    vkASBuildInfo.mode                      = Attribs.Update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    vkASBuildInfo.srcAccelerationStructure  = Attribs.Update ? pTLASVk->GetVkTLAS() : VK_NULL_HANDLE;
    vkASBuildInfo.dstAccelerationStructure  = pTLASVk->GetVkTLAS();
    vkASBuildInfo.geometryCount             = 1;
    vkASBuildInfo.pGeometries               = &vkASGeometry;
    vkASBuildInfo.ppGeometries              = nullptr;
    vkASBuildInfo.scratchData.deviceAddress = pScratchVk->GetVkDeviceAddress() + Attribs.ScratchBufferOffset;

    const auto& ASLimits = m_pDevice->GetPhysicalDevice().GetExtProperties().AccelStruct;
    VERIFY(vkASBuildInfo.scratchData.deviceAddress % ASLimits.minAccelerationStructureScratchOffsetAlignment == 0, "Scratch buffer start address is not properly aligned");

    m_CommandBuffer.BuildAccelerationStructure(1, &vkASBuildInfo, &vkRangePtr);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::CopyBLAS(const CopyBLASAttribs& Attribs)
{
    TDeviceContextBase::CopyBLAS(Attribs, 0);

    auto* pSrcVk = ClassPtrCast<BottomLevelASVkImpl>(Attribs.pSrc);
    auto* pDstVk = ClassPtrCast<BottomLevelASVkImpl>(Attribs.pDst);

    // Dst BLAS description has specified CompactedSize, but doesn't have specified pTriangles and pBoxes.
    // We should copy geometries because it required for SBT to map geometry name to hit group.
    pDstVk->CopyGeometryDescription(*pSrcVk);
    pDstVk->SetActualGeometryCount(pSrcVk->GetActualGeometryCount());

    VkCopyAccelerationStructureInfoKHR Info = {};

    Info.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    Info.src   = pSrcVk->GetVkBLAS();
    Info.dst   = pDstVk->GetVkBLAS();
    Info.mode  = CopyASModeToVkCopyAccelerationStructureMode(Attribs.Mode);

    EnsureVkCmdBuffer();

    const char* OpName = "Copy BottomLevelAS (DeviceContextVkImpl::CopyBLAS)";
    TransitionOrVerifyBLASState(*pSrcVk, Attribs.SrcTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBLASState(*pDstVk, Attribs.DstTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    m_CommandBuffer.CopyAccelerationStructure(Info);
    ++m_State.NumCommands;

#ifdef DILIGENT_DEVELOPMENT
    pDstVk->DvpUpdateVersion();
#endif
}

void DeviceContextVkImpl::CopyTLAS(const CopyTLASAttribs& Attribs)
{
    TDeviceContextBase::CopyTLAS(Attribs, 0);

    auto* pSrcVk = ClassPtrCast<TopLevelASVkImpl>(Attribs.pSrc);
    auto* pDstVk = ClassPtrCast<TopLevelASVkImpl>(Attribs.pDst);

    // Instances specified in BuildTLAS command.
    // We should copy instances because it required for SBT to map instance name to hit group.
    pDstVk->CopyInstanceData(*pSrcVk);

    VkCopyAccelerationStructureInfoKHR Info = {};

    Info.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    Info.src   = pSrcVk->GetVkTLAS();
    Info.dst   = pDstVk->GetVkTLAS();
    Info.mode  = CopyASModeToVkCopyAccelerationStructureMode(Attribs.Mode);

    EnsureVkCmdBuffer();

    const char* OpName = "Copy TopLevelAS (DeviceContextVkImpl::CopyTLAS)";
    TransitionOrVerifyTLASState(*pSrcVk, Attribs.SrcTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyTLASState(*pDstVk, Attribs.DstTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    m_CommandBuffer.CopyAccelerationStructure(Info);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs)
{
    TDeviceContextBase::WriteBLASCompactedSize(Attribs, 0);

    const Uint32 QueryIndex  = 0;
    auto*        pBLASVk     = ClassPtrCast<BottomLevelASVkImpl>(Attribs.pBLAS);
    auto*        pDestBuffVk = ClassPtrCast<BufferVkImpl>(Attribs.pDestBuffer);

    EnsureVkCmdBuffer();

    const char* OpName = "Write AS compacted size (DeviceContextVkImpl::WriteBLASCompactedSize)";
    TransitionOrVerifyBLASState(*pBLASVk, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBufferState(*pDestBuffVk, Attribs.BufferTransitionMode, RESOURCE_STATE_COPY_DEST, VK_ACCESS_TRANSFER_WRITE_BIT, OpName);

    m_CommandBuffer.ResetQueryPool(m_ASQueryPool, QueryIndex, 1);
    m_CommandBuffer.WriteAccelerationStructuresProperties(pBLASVk->GetVkBLAS(), VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_ASQueryPool, QueryIndex);
    m_CommandBuffer.CopyQueryPoolResults(m_ASQueryPool, QueryIndex, 1, pDestBuffVk->GetVkBuffer(), Attribs.DestBufferOffset, sizeof(Uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs)
{
    TDeviceContextBase::WriteTLASCompactedSize(Attribs, 0);

    const Uint32 QueryIndex  = 0;
    auto*        pTLASVk     = ClassPtrCast<TopLevelASVkImpl>(Attribs.pTLAS);
    auto*        pDestBuffVk = ClassPtrCast<BufferVkImpl>(Attribs.pDestBuffer);

    EnsureVkCmdBuffer();

    const char* OpName = "Write AS compacted size (DeviceContextVkImpl::WriteTLASCompactedSize)";
    TransitionOrVerifyTLASState(*pTLASVk, Attribs.TLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBufferState(*pDestBuffVk, Attribs.BufferTransitionMode, RESOURCE_STATE_COPY_DEST, VK_ACCESS_TRANSFER_WRITE_BIT, OpName);

    m_CommandBuffer.ResetQueryPool(m_ASQueryPool, QueryIndex, 1);
    m_CommandBuffer.WriteAccelerationStructuresProperties(pTLASVk->GetVkTLAS(), VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_ASQueryPool, QueryIndex);
    m_CommandBuffer.CopyQueryPoolResults(m_ASQueryPool, QueryIndex, 1, pDestBuffVk->GetVkBuffer(), Attribs.DestBufferOffset, sizeof(Uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::CreateASCompactedSizeQueryPool()
{
    if (m_pDevice->GetFeatures().RayTracing == DEVICE_FEATURE_STATE_ENABLED)
    {
        const auto&           LogicalDevice = m_pDevice->GetLogicalDevice();
        VkQueryPoolCreateInfo Info          = {};

        Info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        Info.queryCount = 1;
        Info.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;

        m_ASQueryPool = LogicalDevice.CreateQueryPool(Info, "AS Compacted Size Query");
    }
}

void DeviceContextVkImpl::TraceRays(const TraceRaysAttribs& Attribs)
{
    TDeviceContextBase::TraceRays(Attribs, 0);

    const auto* pSBTVk       = ClassPtrCast<const ShaderBindingTableVkImpl>(Attribs.pSBT);
    const auto& BindingTable = pSBTVk->GetVkBindingTable();

    PrepareForRayTracing();
    m_CommandBuffer.TraceRays(BindingTable.RaygenShader, BindingTable.MissShader, BindingTable.HitShader, BindingTable.CallableShader,
                              Attribs.DimensionX, Attribs.DimensionY, Attribs.DimensionZ);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs)
{
    TDeviceContextBase::TraceRaysIndirect(Attribs, 0);

    const auto* pSBTVk       = ClassPtrCast<const ShaderBindingTableVkImpl>(Attribs.pSBT);
    const auto& BindingTable = pSBTVk->GetVkBindingTable();

    auto* const pIndirectAttribsVk = PrepareIndirectAttribsBuffer(Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, "Trace rays indirect (DeviceContextVkImpl::TraceRaysIndirect)");
    const auto  IndirectBuffOffset = Attribs.ArgsByteOffset + TraceRaysIndirectCommandSBTSize;

    PrepareForRayTracing();
    m_CommandBuffer.TraceRaysIndirect(BindingTable.RaygenShader, BindingTable.MissShader, BindingTable.HitShader, BindingTable.CallableShader,
                                      pIndirectAttribsVk->GetVkDeviceAddress() + IndirectBuffOffset);
    ++m_State.NumCommands;
}

void DeviceContextVkImpl::UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs)
{
    TDeviceContextBase::UpdateSBT(pSBT, pUpdateIndirectBufferAttribs, 0);

    auto*         pSBTVk       = ClassPtrCast<ShaderBindingTableVkImpl>(pSBT);
    BufferVkImpl* pSBTBufferVk = nullptr;

    ShaderBindingTableVkImpl::BindingTable RayGenShaderRecord  = {};
    ShaderBindingTableVkImpl::BindingTable MissShaderTable     = {};
    ShaderBindingTableVkImpl::BindingTable HitGroupTable       = {};
    ShaderBindingTableVkImpl::BindingTable CallableShaderTable = {};

    pSBTVk->GetData(pSBTBufferVk, RayGenShaderRecord, MissShaderTable, HitGroupTable, CallableShaderTable);

    const char* OpName = "Update shader binding table (DeviceContextVkImpl::UpdateSBT)";

    if (RayGenShaderRecord.pData || MissShaderTable.pData || HitGroupTable.pData || CallableShaderTable.pData)
    {
        TransitionOrVerifyBufferState(*pSBTBufferVk, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, RESOURCE_STATE_COPY_DEST, VK_ACCESS_TRANSFER_WRITE_BIT, OpName);

        // Buffer ranges do not intersect, so we don't need to add barriers between them
        if (RayGenShaderRecord.pData)
            UpdateBuffer(pSBTBufferVk, RayGenShaderRecord.Offset, RayGenShaderRecord.Size, RayGenShaderRecord.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (MissShaderTable.pData)
            UpdateBuffer(pSBTBufferVk, MissShaderTable.Offset, MissShaderTable.Size, MissShaderTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (HitGroupTable.pData)
            UpdateBuffer(pSBTBufferVk, HitGroupTable.Offset, HitGroupTable.Size, HitGroupTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (CallableShaderTable.pData)
            UpdateBuffer(pSBTBufferVk, CallableShaderTable.Offset, CallableShaderTable.Size, CallableShaderTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        TransitionOrVerifyBufferState(*pSBTBufferVk, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, RESOURCE_STATE_RAY_TRACING, VK_ACCESS_SHADER_READ_BIT, OpName);
    }
    else
    {
        // Ray tracing command can be used in parallel with the same SBT, so internal buffer state must be RESOURCE_STATE_RAY_TRACING to allow it.
        VERIFY(pSBTBufferVk->CheckState(RESOURCE_STATE_RAY_TRACING), "SBT buffer must always be in RESOURCE_STATE_RAY_TRACING state");
    }
}

void DeviceContextVkImpl::BeginDebugGroup(const Char* Name, const float* pColor)
{
    TDeviceContextBase::BeginDebugGroup(Name, pColor, 0);

    VkDebugUtilsLabelEXT Info{};
    Info.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Info.pLabelName = Name;

    if (pColor != nullptr)
        memcpy(Info.color, pColor, sizeof(Info.color));

    EnsureVkCmdBuffer();
    m_CommandBuffer.BeginDebugUtilsLabel(Info);
}

void DeviceContextVkImpl::EndDebugGroup()
{
    TDeviceContextBase::EndDebugGroup(0);

    EnsureVkCmdBuffer();
    m_CommandBuffer.EndDebugUtilsLabel();
}

void DeviceContextVkImpl::InsertDebugLabel(const Char* Label, const float* pColor)
{
    TDeviceContextBase::InsertDebugLabel(Label, pColor, 0);

    VkDebugUtilsLabelEXT Info{};
    Info.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Info.pLabelName = Label;

    if (pColor != nullptr)
        memcpy(Info.color, pColor, sizeof(Info.color));

    EnsureVkCmdBuffer();
    m_CommandBuffer.InsertDebugUtilsLabel(Info);
}

void DeviceContextVkImpl::SetShadingRate(SHADING_RATE BaseRate, SHADING_RATE_COMBINER PrimitiveCombiner, SHADING_RATE_COMBINER TextureCombiner)
{
    TDeviceContextBase::SetShadingRate(BaseRate, PrimitiveCombiner, TextureCombiner, 0);

    const auto& ExtFeatures = m_pDevice->GetLogicalDevice().GetEnabledExtFeatures();
    if (ExtFeatures.ShadingRate.attachmentFragmentShadingRate != VK_FALSE)
    {
        const VkFragmentShadingRateCombinerOpKHR CombinerOps[2] =
            {
                ShadingRateCombinerToVkFragmentShadingRateCombinerOp(PrimitiveCombiner),
                ShadingRateCombinerToVkFragmentShadingRateCombinerOp(TextureCombiner) //
            };

        EnsureVkCmdBuffer();
        m_CommandBuffer.SetFragmentShadingRate(ShadingRateToVkFragmentSize(BaseRate), CombinerOps);
        m_State.ShadingRateIsSet = true;
    }
    else if (ExtFeatures.FragmentDensityMap.fragmentDensityMap != VK_FALSE)
    {
        // Ignored
        DEV_CHECK_ERR(BaseRate == SHADING_RATE_1X1, "IDeviceContext::SetShadingRate: BaseRate must be SHADING_RATE_1X1");
        DEV_CHECK_ERR(PrimitiveCombiner == SHADING_RATE_COMBINER_PASSTHROUGH, "IDeviceContext::SetShadingRate: PrimitiveCombiner must be SHADING_RATE_COMBINER_PASSTHROUGH");
        DEV_CHECK_ERR(TextureCombiner == SHADING_RATE_COMBINER_OVERRIDE, "IDeviceContext::SetShadingRate: TextureCombiner must be SHADING_RATE_COMBINER_OVERRIDE");
    }
    else
        UNEXPECTED("VariableRateShading device feature is not enabled");
}

void DeviceContextVkImpl::BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs)
{
    TDeviceContextBase::BindSparseResourceMemory(Attribs, 0);

    VERIFY_EXPR(Attribs.NumBufferBinds != 0 || Attribs.NumTextureBinds != 0);

    Flush();

    // Calculate the required array sizes
    Uint32 ImageBindCount       = 0;
    Uint32 ImageOpqBindCount    = 0;
    Uint32 MemoryBindCount      = 0;
    Uint32 ImageMemoryBindCount = 0;

    for (Uint32 i = 0; i < Attribs.NumBufferBinds; ++i)
        MemoryBindCount += Attribs.pBufferBinds[i].NumRanges;

    for (Uint32 i = 0; i < Attribs.NumTextureBinds; ++i)
    {
        const auto& Bind           = Attribs.pTextureBinds[i];
        const auto* pTexVk         = ClassPtrCast<const TextureVkImpl>(Bind.pTexture);
        const auto& TexSparseProps = pTexVk->GetSparseProperties();

        Uint32 NumImageBindsInRange = 0;
        for (Uint32 j = 0; j < Bind.NumRanges; ++j)
        {
            if (Bind.pRanges[j].MipLevel >= TexSparseProps.FirstMipInTail)
            {
                ++MemoryBindCount;
                ++ImageOpqBindCount;
            }
            else
            {
                ++NumImageBindsInRange;
                ++ImageMemoryBindCount;
            }
        }
        if (NumImageBindsInRange > 0)
            ++ImageBindCount;
    }

    std::vector<VkSparseBufferMemoryBindInfo>      vkBufferBinds{Attribs.NumBufferBinds};
    std::vector<VkSparseImageOpaqueMemoryBindInfo> vkImageOpaqueBinds{ImageOpqBindCount};
    std::vector<VkSparseImageMemoryBindInfo>       vkImageBinds{ImageBindCount};
    std::vector<VkSparseMemoryBind>                vkMemoryBinds{MemoryBindCount};
    std::vector<VkSparseImageMemoryBind>           vkImageMemoryBinds{ImageMemoryBindCount};

    MemoryBindCount      = 0;
    ImageMemoryBindCount = 0;
    ImageBindCount       = 0;
    ImageOpqBindCount    = 0;

    for (Uint32 i = 0; i < Attribs.NumBufferBinds; ++i)
    {
        const auto& BuffBind = Attribs.pBufferBinds[i];
        const auto* pBuffVk  = ClassPtrCast<const BufferVkImpl>(BuffBind.pBuffer);
#ifdef DILIGENT_DEVELOPMENT
        const auto& BuffSparseProps = pBuffVk->GetSparseProperties();
#endif

        auto& vkBuffBind{vkBufferBinds[i]};
        vkBuffBind.buffer    = pBuffVk->GetVkBuffer();
        vkBuffBind.bindCount = BuffBind.NumRanges;
        vkBuffBind.pBinds    = &vkMemoryBinds[MemoryBindCount];

        for (Uint32 r = 0; r < BuffBind.NumRanges; ++r)
        {
            const auto& SrcRange = BuffBind.pRanges[r];
            const auto  pMemVk   = RefCntAutoPtr<IDeviceMemoryVk>{SrcRange.pMemory, IID_DeviceMemoryVk};
            DEV_CHECK_ERR((SrcRange.pMemory != nullptr) == (pMemVk != nullptr),
                          "Failed to query IDeviceMemoryVk interface from non-null memory object");

            const auto MemRangeVk = pMemVk ? pMemVk->GetRange(SrcRange.MemoryOffset, SrcRange.MemorySize) : DeviceMemoryRangeVk{};
            DEV_CHECK_ERR(MemRangeVk.Offset % BuffSparseProps.BlockSize == 0,
                          "MemoryOffset must be multiple of the SparseBufferProperties::BlockSize");

            auto& vkMemBind{vkMemoryBinds[MemoryBindCount++]};
            vkMemBind.resourceOffset = SrcRange.BufferOffset;
            vkMemBind.size           = SrcRange.MemorySize; // MemRangeVk.Size may be zero when range is unbound
            vkMemBind.memory         = MemRangeVk.Handle;
            vkMemBind.memoryOffset   = MemRangeVk.Offset;
            vkMemBind.flags          = 0;

            VERIFY(vkMemBind.size > 0, "Buffer memory size must not be zero");
        }
    }

    for (Uint32 i = 0; i < Attribs.NumTextureBinds; ++i)
    {
        const auto& TexBind        = Attribs.pTextureBinds[i];
        const auto* pTexVk         = ClassPtrCast<const TextureVkImpl>(TexBind.pTexture);
        const auto& TexDesc        = pTexVk->GetDesc();
        const auto& TexSparseProps = pTexVk->GetSparseProperties();
        const auto& FmtAttribs     = GetTextureFormatAttribs(TexDesc.Format);

        VkImageAspectFlags aspectMask = 0;
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        Uint32 NumImageBindsInRange = 0;
        for (Uint32 r = 0; r < TexBind.NumRanges; ++r)
        {
            const auto& SrcRange = TexBind.pRanges[r];
            const auto  pMemVk   = RefCntAutoPtr<IDeviceMemoryVk>{SrcRange.pMemory, IID_DeviceMemoryVk};
            DEV_CHECK_ERR((SrcRange.pMemory != nullptr) == (pMemVk != nullptr),
                          "Failed to query IDeviceMemoryVk interface from non-null memory object");

            const auto MemRangeVk = pMemVk ? pMemVk->GetRange(SrcRange.MemoryOffset, SrcRange.MemorySize) : DeviceMemoryRangeVk{};
            DEV_CHECK_ERR((MemRangeVk.Offset % TexSparseProps.BlockSize) == 0,
                          "MemoryOffset must be a multiple of the SparseTextureProperties::BlockSize");

            if (SrcRange.MipLevel < TexSparseProps.FirstMipInTail)
            {
                const auto TexWidth  = std::max(1u, TexDesc.Width >> SrcRange.MipLevel);
                const auto TexHeight = std::max(1u, TexDesc.Height >> SrcRange.MipLevel);
                const auto TexDepth  = std::max(1u, TexDesc.GetDepth() >> SrcRange.MipLevel);

                auto& vkImgMemBind{vkImageMemoryBinds[size_t{ImageMemoryBindCount} + size_t{NumImageBindsInRange}]};
                vkImgMemBind.subresource.arrayLayer = SrcRange.ArraySlice;
                vkImgMemBind.subresource.aspectMask = aspectMask;
                vkImgMemBind.subresource.mipLevel   = SrcRange.MipLevel;
                vkImgMemBind.offset.x               = static_cast<int32_t>(SrcRange.Region.MinX);
                vkImgMemBind.offset.y               = static_cast<int32_t>(SrcRange.Region.MinY);
                vkImgMemBind.offset.z               = static_cast<int32_t>(SrcRange.Region.MinZ);
                vkImgMemBind.extent.width           = static_cast<int32_t>(std::min(SrcRange.Region.Width(), TexWidth - SrcRange.Region.MinX));
                vkImgMemBind.extent.height          = static_cast<int32_t>(std::min(SrcRange.Region.Height(), TexHeight - SrcRange.Region.MinY));
                vkImgMemBind.extent.depth           = static_cast<int32_t>(std::min(SrcRange.Region.Depth(), TexDepth - SrcRange.Region.MinZ));
                vkImgMemBind.memory                 = MemRangeVk.Handle;
                vkImgMemBind.memoryOffset           = MemRangeVk.Offset;
                vkImgMemBind.flags                  = 0;

                ++NumImageBindsInRange;
            }
            else
            {
                // Bind mip tail memory
                auto& vkImgOpqBind{vkImageOpaqueBinds[ImageOpqBindCount++]};
                vkImgOpqBind.image     = pTexVk->GetVkImage();
                vkImgOpqBind.bindCount = 1;
                vkImgOpqBind.pBinds    = &vkMemoryBinds[MemoryBindCount];

                auto& vkMemBind{vkMemoryBinds[MemoryBindCount++]};
                vkMemBind.resourceOffset = TexSparseProps.MipTailOffset + TexSparseProps.MipTailStride * SrcRange.ArraySlice + SrcRange.OffsetInMipTail;
                vkMemBind.size           = SrcRange.MemorySize; // MemRangeVk.Size may be zero if tail is unbound
                vkMemBind.memory         = MemRangeVk.Handle;
                vkMemBind.memoryOffset   = MemRangeVk.Offset;
                vkMemBind.flags          = 0;

                VERIFY(vkMemBind.size > 0, "Texture mip tail memory size must not be zero");
                VERIFY(!(TexDesc.IsArray() && (TexSparseProps.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL) == 0) || TexSparseProps.MipTailStride != 0,
                       "For texture arrays, if SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL flag is not present, MipTailStride must not be zero");
            }
        }

        if (NumImageBindsInRange > 0)
        {
            auto& vkImgBind{vkImageBinds[ImageBindCount++]};
            vkImgBind.image     = pTexVk->GetVkImage();
            vkImgBind.bindCount = NumImageBindsInRange;
            vkImgBind.pBinds    = &vkImageMemoryBinds[ImageMemoryBindCount];

            ImageMemoryBindCount += NumImageBindsInRange;
        }
    }

    VERIFY_EXPR(MemoryBindCount == vkMemoryBinds.size());
    VERIFY_EXPR(ImageMemoryBindCount == vkImageMemoryBinds.size());
    VERIFY_EXPR(ImageBindCount == vkImageBinds.size());
    VERIFY_EXPR(ImageOpqBindCount == vkImageOpaqueBinds.size());

    VkBindSparseInfo BindSparse{};
    BindSparse.sType                = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    BindSparse.bufferBindCount      = StaticCast<uint32_t>(vkBufferBinds.size());
    BindSparse.pBufferBinds         = !vkBufferBinds.empty() ? vkBufferBinds.data() : nullptr;
    BindSparse.imageOpaqueBindCount = StaticCast<uint32_t>(vkImageOpaqueBinds.size());
    BindSparse.pImageOpaqueBinds    = !vkImageOpaqueBinds.empty() ? vkImageOpaqueBinds.data() : nullptr;
    BindSparse.imageBindCount       = StaticCast<uint32_t>(vkImageBinds.size());
    BindSparse.pImageBinds          = !vkImageBinds.empty() ? vkImageBinds.data() : nullptr;

    VERIFY_EXPR(m_VkSignalSemaphores.empty() && m_SignalSemaphoreValues.empty());
    VERIFY_EXPR(m_VkWaitSemaphores.empty() && m_WaitSemaphoreValues.empty());

    bool UsedTimelineSemaphore = false;
    for (Uint32 i = 0; i < Attribs.NumSignalFences; ++i)
    {
        auto* pFenceVk    = ClassPtrCast<FenceVkImpl>(Attribs.ppSignalFences[i]);
        auto  SignalValue = Attribs.pSignalFenceValues[i];
        if (!pFenceVk->IsTimelineSemaphore())
            continue;
        UsedTimelineSemaphore = true;
        pFenceVk->DvpSignal(SignalValue);
        m_VkSignalSemaphores.push_back(pFenceVk->GetVkSemaphore());
        m_SignalSemaphoreValues.push_back(SignalValue);
    }

    for (Uint32 i = 0; i < Attribs.NumWaitFences; ++i)
    {
        auto* pFenceVk  = ClassPtrCast<FenceVkImpl>(Attribs.ppWaitFences[i]);
        auto  WaitValue = Attribs.pWaitFenceValues[i];
        pFenceVk->DvpDeviceWait(WaitValue);

        if (pFenceVk->IsTimelineSemaphore())
        {
            UsedTimelineSemaphore = true;
            VkSemaphore WaitSem   = pFenceVk->GetVkSemaphore();
#ifdef DILIGENT_DEVELOPMENT
            for (size_t j = 0; j < m_VkWaitSemaphores.size(); ++j)
            {
                if (m_VkWaitSemaphores[j] == WaitSem)
                {
                    LOG_ERROR_MESSAGE("Fence '", pFenceVk->GetDesc().Name, "' with value (", WaitValue,
                                      ") is already added to the wait operation with value (", m_WaitSemaphoreValues[j], ")");
                }
            }
#endif
            m_VkWaitSemaphores.push_back(WaitSem);
            m_WaitSemaphoreValues.push_back(WaitValue);
        }
        else
        {
            if (auto WaitSem = pFenceVk->ExtractSignalSemaphore(GetCommandQueueId(), WaitValue))
            {
                // Here we have unique binary semaphore that must be released/recycled using release queue
                m_VkWaitSemaphores.push_back(WaitSem);
                m_WaitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                m_WaitRecycledSemaphores.push_back(std::move(WaitSem));
                m_WaitSemaphoreValues.push_back(0); // Ignored for binary semaphore
            }
        }
    }
    BindSparse.waitSemaphoreCount   = StaticCast<uint32_t>(m_VkWaitSemaphores.size());
    BindSparse.pWaitSemaphores      = BindSparse.waitSemaphoreCount != 0 ? m_VkWaitSemaphores.data() : nullptr;
    BindSparse.signalSemaphoreCount = StaticCast<uint32_t>(m_VkSignalSemaphores.size());
    BindSparse.pSignalSemaphores    = BindSparse.signalSemaphoreCount != 0 ? m_VkSignalSemaphores.data() : nullptr;

    VkTimelineSemaphoreSubmitInfo TimelineSemaphoreSubmitInfo{};
    if (UsedTimelineSemaphore)
    {
        BindSparse.pNext = &TimelineSemaphoreSubmitInfo;

        TimelineSemaphoreSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSemaphoreSubmitInfo.pNext                     = nullptr;
        TimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = BindSparse.waitSemaphoreCount;
        TimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = BindSparse.waitSemaphoreCount != 0 ? m_WaitSemaphoreValues.data() : nullptr;
        TimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = BindSparse.signalSemaphoreCount;
        TimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = BindSparse.signalSemaphoreCount != 0 ? m_SignalSemaphoreValues.data() : nullptr;
    }

    SyncPointVkPtr pSyncPoint;
    {
        auto* pQueueVk = ClassPtrCast<CommandQueueVkImpl>(LockCommandQueue());

        pQueueVk->BindSparse(BindSparse);
        pSyncPoint = pQueueVk->GetLastSyncPoint();

        UnlockCommandQueue();
    }

    if (!UsedTimelineSemaphore)
    {
        for (Uint32 i = 0; i < Attribs.NumSignalFences; ++i)
        {
            auto* pFenceVk = ClassPtrCast<FenceVkImpl>(Attribs.ppSignalFences[i]);
            if (!pFenceVk->IsTimelineSemaphore())
                pFenceVk->AddPendingSyncPoint(GetCommandQueueId(), Attribs.pSignalFenceValues[i], pSyncPoint);
        }
    }

    m_VkSignalSemaphores.clear();
    m_SignalSemaphoreValues.clear();
    m_VkWaitSemaphores.clear();
    m_WaitSemaphoreValues.clear();
}

} // namespace Diligent
