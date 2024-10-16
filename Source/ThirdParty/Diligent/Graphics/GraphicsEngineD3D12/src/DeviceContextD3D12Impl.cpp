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

#include "DeviceContextD3D12Impl.hpp"

#include <sstream>

#include "RenderDeviceD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "FenceD3D12Impl.hpp"
#include "ShaderBindingTableD3D12Impl.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "CommandListD3D12Impl.hpp"
#include "DeviceMemoryD3D12Impl.hpp"
#include "CommandQueueD3D12Impl.hpp"

#include "CommandContext.hpp"
#include "D3D12TypeConversions.hpp"
#include "d3dx12_win.h"
#include "D3D12DynamicHeap.hpp"
#include "QueryManagerD3D12.hpp"
#include "DXGITypeConversions.hpp"

#include "D3D12TileMappingHelper.hpp"

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

DeviceContextD3D12Impl::DeviceContextD3D12Impl(IReferenceCounters*          pRefCounters,
                                               RenderDeviceD3D12Impl*       pDeviceD3D12Impl,
                                               const EngineD3D12CreateInfo& EngineCI,
                                               const DeviceContextDesc&     Desc) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDeviceD3D12Impl,
        Desc
    },
    m_DynamicHeap
    {
        pDeviceD3D12Impl->GetDynamicMemoryManager(),
        GetContextObjectName("Dynamic heap", Desc.IsDeferred, Desc.ContextId),
        EngineCI.DynamicHeapPageSize
    },
    m_DynamicGPUDescriptorAllocator
    {
        {
            GetRawAllocator(),
            pDeviceD3D12Impl->GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
            EngineCI.DynamicDescriptorAllocationChunkSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
            GetContextObjectName("CBV_SRV_UAV dynamic descriptor allocator", Desc.IsDeferred, Desc.ContextId)
        },
        {
            GetRawAllocator(),
            pDeviceD3D12Impl->GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
            EngineCI.DynamicDescriptorAllocationChunkSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER],
            GetContextObjectName("SAMPLER     dynamic descriptor allocator", Desc.IsDeferred, Desc.ContextId)
        }
    },
    m_CmdListAllocator{GetRawAllocator(), sizeof(CommandListD3D12Impl), 64},
    m_NullRTV{pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1)}
// clang-format on
{
    auto* pd3d12Device = pDeviceD3D12Impl->GetD3D12Device();
    if (!IsDeferred())
    {
        RequestCommandContext();
        m_QueryMgr = &pDeviceD3D12Impl->GetQueryMgr(GetCommandQueueId());
    }

    auto* pDrawIndirectSignature = GetDrawIndirectSignature(sizeof(UINT) * 4);
    if (pDrawIndirectSignature == nullptr)
        LOG_ERROR_AND_THROW("Failed to create indirect draw command signature");

    auto* pDrawIndexedIndirectSignature = GetDrawIndexedIndirectSignature(sizeof(UINT) * 5);
    if (pDrawIndexedIndirectSignature == nullptr)
        LOG_ERROR_AND_THROW("Failed to create draw indexed indirect command signature");

    D3D12_COMMAND_SIGNATURE_DESC CmdSignatureDesc = {};
    D3D12_INDIRECT_ARGUMENT_DESC IndirectArg      = {};

    CmdSignatureDesc.NodeMask         = 0;
    CmdSignatureDesc.NumArgumentDescs = 1;
    CmdSignatureDesc.pArgumentDescs   = &IndirectArg;

    CmdSignatureDesc.ByteStride = sizeof(UINT) * 3;
    IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    auto hr                     = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDispatchIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDispatchIndirectSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create dispatch indirect command signature");

#ifdef D3D12_H_HAS_MESH_SHADER
    if (pDeviceD3D12Impl->GetFeatures().MeshShaders == DEVICE_FEATURE_STATE_ENABLED)
    {
        CmdSignatureDesc.ByteStride = sizeof(UINT) * 3;
        IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
        hr                          = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDrawMeshIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDrawMeshIndirectSignature)));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create draw mesh indirect command signature");
        VERIFY_EXPR(CmdSignatureDesc.ByteStride == DrawMeshIndirectCommandStride);
    }
#endif
    if (pDeviceD3D12Impl->GetFeatures().RayTracing == DEVICE_FEATURE_STATE_ENABLED &&
        (pDeviceD3D12Impl->GetAdapterInfo().RayTracing.CapFlags & RAY_TRACING_CAP_FLAG_INDIRECT_RAY_TRACING) != 0)
    {
        CmdSignatureDesc.ByteStride = sizeof(D3D12_DISPATCH_RAYS_DESC);
        IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
        hr                          = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pTraceRaysIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pTraceRaysIndirectSignature)));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create trace rays indirect command signature");
        static_assert(TraceRaysIndirectCommandSBTSize == offsetof(D3D12_DISPATCH_RAYS_DESC, Width), "Invalid SBT offsets size");
        static_assert(TraceRaysIndirectCommandSize == sizeof(D3D12_DISPATCH_RAYS_DESC), "Invalid trace ray indirect command size");
    }

    {
        D3D12_RENDER_TARGET_VIEW_DESC NullRTVDesc{DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RTV_DIMENSION_TEXTURE2D, {}};
        // A null pResource is used to initialize a null descriptor, which guarantees D3D11-like null binding behavior
        // (reading 0s, writes are discarded), but must have a valid pDesc in order to determine the descriptor type.
        // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createrendertargetview
        pd3d12Device->CreateRenderTargetView(nullptr, &NullRTVDesc, m_NullRTV.GetCpuHandle());
        VERIFY(!m_NullRTV.IsNull(), "Failed to create null RTV");
    }
}

DeviceContextD3D12Impl::~DeviceContextD3D12Impl()
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

    if (IsDeferred())
    {
        if (m_CurrCmdCtx)
        {
            // The command context has never been executed, so it can be disposed without going through release queue
            m_pDevice->DisposeCommandContext(std::move(m_CurrCmdCtx));
        }
    }
    else
    {
        Flush(false);
    }

    // For deferred contexts, m_SubmittedBuffersCmdQueueMask is reset to 0 after every call to FinishFrame().
    // In this case there are no resources to release, so there will be no issues.
    FinishFrame();

    // Note: as dynamic pages are returned to the global dynamic memory manager hosted by the render device,
    // the dynamic heap can be destroyed before all pages are actually returned to the global manager.
    DEV_CHECK_ERR(m_DynamicHeap.GetAllocatedPagesCount() == 0, "All dynamic pages must have been released by now.");

    for (size_t i = 0; i < _countof(m_DynamicGPUDescriptorAllocator); ++i)
    {
        // Note: as dynamic descriptor suballocations are returned to the global GPU descriptor heap that
        // is hosted by the render device, the descriptor allocator can be destroyed before all suballocations
        // are actually returned to the global heap.
        DEV_CHECK_ERR(m_DynamicGPUDescriptorAllocator[i].GetSuballocationCount() == 0, "All dynamic suballocations must have been released");
    }
}

ID3D12CommandSignature* DeviceContextD3D12Impl::GetDrawIndirectSignature(Uint32 Stride)
{
    auto& Sig = m_pDrawIndirectSignatureMap[Stride];
    if (Sig == nullptr)
    {
        VERIFY_EXPR(Stride >= sizeof(UINT) * 4);

        D3D12_INDIRECT_ARGUMENT_DESC IndirectArg{};
        IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

        D3D12_COMMAND_SIGNATURE_DESC CmdSignatureDesc{};
        CmdSignatureDesc.NodeMask         = 0;
        CmdSignatureDesc.NumArgumentDescs = 1;
        CmdSignatureDesc.pArgumentDescs   = &IndirectArg;
        CmdSignatureDesc.ByteStride       = Stride;

        auto hr = m_pDevice->GetD3D12Device()->CreateCommandSignature(&CmdSignatureDesc, nullptr, IID_PPV_ARGS(&Sig));
        CHECK_D3D_RESULT(hr, "Failed to create indirect draw command signature");
    }
    return Sig;
}

ID3D12CommandSignature* DeviceContextD3D12Impl::GetDrawIndexedIndirectSignature(Uint32 Stride)
{
    auto& Sig = m_pDrawIndexedIndirectSignatureMap[Stride];
    if (Sig == nullptr)
    {
        VERIFY_EXPR(Stride >= sizeof(UINT) * 5);

        D3D12_INDIRECT_ARGUMENT_DESC IndirectArg{};
        IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC CmdSignatureDesc{};
        CmdSignatureDesc.NodeMask         = 0;
        CmdSignatureDesc.NumArgumentDescs = 1;
        CmdSignatureDesc.pArgumentDescs   = &IndirectArg;
        CmdSignatureDesc.ByteStride       = Stride;

        auto hr = m_pDevice->GetD3D12Device()->CreateCommandSignature(&CmdSignatureDesc, nullptr, IID_PPV_ARGS(&Sig));
        CHECK_D3D_RESULT(hr, "Failed to create draw indexed indirect command signature");
    }
    return Sig;
}

void DeviceContextD3D12Impl::Begin(Uint32 ImmediateContextId)
{
    DEV_CHECK_ERR(ImmediateContextId < m_pDevice->GetCommandQueueCount(), "ImmediateContextId is out of range");
    SoftwareQueueIndex CommandQueueId{ImmediateContextId};
    const auto         d3d12CmdListType = m_pDevice->GetCommandQueueType(CommandQueueId);
    const auto         QueueType        = D3D12CommandListTypeToCmdQueueType(d3d12CmdListType);
    TDeviceContextBase::Begin(DeviceContextIndex{ImmediateContextId}, QueueType);
    RequestCommandContext();
    m_QueryMgr = &m_pDevice->GetQueryMgr(CommandQueueId);
}

void DeviceContextD3D12Impl::SetPipelineState(IPipelineState* pPipelineState)
{
    RefCntAutoPtr<PipelineStateD3D12Impl> pPipelineStateD3D12{pPipelineState, PipelineStateD3D12Impl::IID_InternalImpl};
    VERIFY(pPipelineState == nullptr || pPipelineStateD3D12 != nullptr, "Unknown pipeline state object implementation");
    if (PipelineStateD3D12Impl::IsSameObject(m_pPipelineState, pPipelineStateD3D12))
        return;

    const auto& PSODesc = pPipelineStateD3D12->GetDesc();

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
        // Commit all graphics states when switching from compute pipeline
        // This is necessary because if the command list had been flushed
        // and the first PSO set on the command list was a compute pipeline,
        // the states would otherwise never be committed (since m_pPipelineState != nullptr)
        CommitStates = !OldPSODesc.IsAnyGraphicsPipeline();
        // We also need to update scissor rect if ScissorEnable state has changed
        if (OldPSODesc.IsAnyGraphicsPipeline() && PSODesc.IsAnyGraphicsPipeline())
            CommitScissor = m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable != pPipelineStateD3D12->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable;
    }

    TDeviceContextBase::SetPipelineState(std::move(pPipelineStateD3D12), 0 /*Dummy*/);

    auto& CmdCtx        = GetCmdContext();
    auto& RootInfo      = GetRootTableInfo(PSODesc.PipelineType);
    auto* pd3d12RootSig = m_pPipelineState->GetD3D12RootSignature();

    if (RootInfo.pd3d12RootSig != pd3d12RootSig)
    {
        RootInfo.pd3d12RootSig = pd3d12RootSig;

        Uint32 DvpCompatibleSRBCount = 0;
        PrepareCommittedResources(RootInfo, DvpCompatibleSRBCount);

        // When root signature changes, all resources must be committed anew
        RootInfo.MakeAllStale();
    }

    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the switch below to handle the new pipeline type");
    switch (PSODesc.PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
        {
            auto& GraphicsPipeline = m_pPipelineState->GetGraphicsPipelineDesc();
            auto& GraphicsCtx      = CmdCtx.AsGraphicsContext();
            auto* pd3d12PSO        = m_pPipelineState->GetD3D12PipelineState();
            GraphicsCtx.SetPipelineState(pd3d12PSO);
            GraphicsCtx.SetGraphicsRootSignature(pd3d12RootSig);

            if (PSODesc.PipelineType == PIPELINE_TYPE_GRAPHICS)
            {
                auto D3D12Topology = TopologyToD3D12Topology(GraphicsPipeline.PrimitiveTopology);
                GraphicsCtx.SetPrimitiveTopology(D3D12Topology);
            }

            if (CommitStates)
            {
                GraphicsCtx.SetStencilRef(m_StencilRef);
                GraphicsCtx.SetBlendFactor(m_BlendFactors);
                if (GraphicsPipeline.pRenderPass == nullptr)
                {
                    CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE_VERIFY);
                }
                CommitViewports();
            }

            if (CommitStates || CommitScissor)
            {
                CommitScissorRects(GraphicsCtx, GraphicsPipeline.RasterizerDesc.ScissorEnable);
            }
            break;
        }
        case PIPELINE_TYPE_COMPUTE:
        {
            auto* pd3d12PSO = m_pPipelineState->GetD3D12PipelineState();
            auto& CompCtx   = CmdCtx.AsComputeContext();
            CompCtx.SetPipelineState(pd3d12PSO);
            CompCtx.SetComputeRootSignature(pd3d12RootSig);
            break;
        }
        case PIPELINE_TYPE_RAY_TRACING:
        {
            auto* pd3d12SO = m_pPipelineState->GetD3D12StateObject();
            auto& RTCtx    = CmdCtx.AsGraphicsContext4();
            RTCtx.SetRayTracingPipelineState(pd3d12SO);
            RTCtx.SetComputeRootSignature(pd3d12RootSig);
            break;
        }
        case PIPELINE_TYPE_TILE:
            UNEXPECTED("Unsupported pipeline type");
            break;
        default:
            UNEXPECTED("Unknown pipeline type");
    }
}

template <bool IsCompute>
void DeviceContextD3D12Impl::CommitRootTablesAndViews(RootTableInfo& RootInfo, Uint32 CommitSRBMask, CommandContext& CmdCtx) const
{
    const auto& RootSig = m_pPipelineState->GetRootSignature();

    PipelineResourceSignatureD3D12Impl::CommitCacheResourcesAttribs CommitAttribs //
        {
            m_pDevice->GetD3D12Device(),
            CmdCtx,
            GetContextId(),
            IsCompute //
        };

    VERIFY(CommitSRBMask != 0, "This method should not be called when there is nothing to commit");
    while (CommitSRBMask != 0)
    {
        const auto SignBit = ExtractLSB(CommitSRBMask);
        const auto sign    = PlatformMisc::GetLSB(SignBit);
        VERIFY_EXPR(sign < m_pPipelineState->GetResourceSignatureCount());

        const auto* const pSignature = RootSig.GetResourceSignature(sign);
        VERIFY_EXPR(pSignature != nullptr && pSignature->GetTotalResourceCount() > 0);

        const auto* pResourceCache = RootInfo.ResourceCaches[sign];
        DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at index ", sign, " is null.");

        CommitAttribs.pResourceCache = pResourceCache;
        CommitAttribs.BaseRootIndex  = RootSig.GetBaseRootIndex(sign);
        if ((RootInfo.StaleSRBMask & SignBit) != 0)
        {
            // Commit root tables for stale SRBs only
            pSignature->CommitRootTables(CommitAttribs);
        }

        // Always commit root views. If the root view is up-to-date (e.g. it is not stale and is intact),
        // the bit should not be set in CommitSRBMask.
        if (auto DynamicRootBuffersMask = pResourceCache->GetDynamicRootBuffersMask())
        {
            DEV_CHECK_ERR((RootInfo.DynamicSRBMask & SignBit) != 0,
                          "There are dynamic root buffers in the cache, but the bit in DynamicSRBMask is not set. This may indicate that resources "
                          "in the cache have changed, but the SRB has not been committed before the draw/dispatch command.");
            pSignature->CommitRootViews(CommitAttribs, DynamicRootBuffersMask);
        }
        else
        {
            DEV_CHECK_ERR((RootInfo.DynamicSRBMask & SignBit) == 0,
                          "There are no dynamic root buffers in the cache, but the bit in DynamicSRBMask is set. This may indicate that resources "
                          "in the cache have changed, but the SRB has not been committed before the draw/dispatch command.");
        }
    }

    VERIFY_EXPR((CommitSRBMask & RootInfo.ActiveSRBMask) == 0);
    RootInfo.StaleSRBMask &= ~RootInfo.ActiveSRBMask;
}

void DeviceContextD3D12Impl::TransitionShaderResources(IShaderResourceBinding* pShaderResourceBinding)
{
    DEV_CHECK_ERR(!m_pActiveRenderPass, "State transitions are not allowed inside a render pass.");

    auto& CmdCtx               = GetCmdContext();
    auto* pResBindingD3D12Impl = ClassPtrCast<ShaderResourceBindingD3D12Impl>(pShaderResourceBinding);
    auto& ResourceCache        = pResBindingD3D12Impl->GetResourceCache();

    ResourceCache.TransitionResourceStates(CmdCtx, ShaderResourceCacheD3D12::StateTransitionMode::Transition);
}

void DeviceContextD3D12Impl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0 /*Dummy*/);

    auto* pResBindingD3D12Impl = ClassPtrCast<ShaderResourceBindingD3D12Impl>(pShaderResourceBinding);
    auto& ResourceCache        = pResBindingD3D12Impl->GetResourceCache();
    auto& CmdCtx               = GetCmdContext();
    auto* pSignature           = pResBindingD3D12Impl->GetSignature();

#ifdef DILIGENT_DEBUG
    ResourceCache.DbgValidateDynamicBuffersMask();
#endif

    if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        ResourceCache.TransitionResourceStates(CmdCtx, ShaderResourceCacheD3D12::StateTransitionMode::Transition);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        ResourceCache.TransitionResourceStates(CmdCtx, ShaderResourceCacheD3D12::StateTransitionMode::Verify);
    }
#endif

    const auto SRBIndex = pResBindingD3D12Impl->GetBindingIndex();
    auto&      RootInfo = GetRootTableInfo(pSignature->GetPipelineType());

    RootInfo.Set(SRBIndex, pResBindingD3D12Impl);
}

DeviceContextD3D12Impl::RootTableInfo& DeviceContextD3D12Impl::GetRootTableInfo(PIPELINE_TYPE PipelineType)
{
    return PipelineType == PIPELINE_TYPE_GRAPHICS || PipelineType == PIPELINE_TYPE_MESH ?
        m_GraphicsResources :
        m_ComputeResources;
}

#ifdef DILIGENT_DEVELOPMENT
void DeviceContextD3D12Impl::DvpValidateCommittedShaderResources(RootTableInfo& RootInfo) const
{
    if (RootInfo.ResourcesValidated)
        return;

    DvpVerifySRBCompatibility(RootInfo,
                              [this](Uint32 idx) {
                                  // Use signature from the root signature
                                  return m_pPipelineState->GetRootSignature().GetResourceSignature(idx);
                              });

    m_pPipelineState->DvpVerifySRBResources(this, RootInfo.ResourceCaches);
    RootInfo.ResourcesValidated = true;
}
#endif

void DeviceContextD3D12Impl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        GetCmdContext().AsGraphicsContext().SetStencilRef(m_StencilRef);
    }
}

void DeviceContextD3D12Impl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        GetCmdContext().AsGraphicsContext().SetBlendFactor(m_BlendFactors);
    }
}

void DeviceContextD3D12Impl::CommitD3D12IndexBuffer(GraphicsContext& GraphCtx, VALUE_TYPE IndexType)
{
    DEV_CHECK_ERR(m_pIndexBuffer != nullptr, "Index buffer is not set up for indexed draw command");

    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = m_pIndexBuffer->GetGPUAddress(GetContextId(), this) + m_IndexDataStartOffset;
    if (IndexType == VT_UINT32)
        IBView.Format = DXGI_FORMAT_R32_UINT;
    else
    {
        DEV_CHECK_ERR(IndexType == VT_UINT16, "Unsupported index format. Only R16_UINT and R32_UINT are allowed.");
        IBView.Format = DXGI_FORMAT_R16_UINT;
    }
    // Note that for a dynamic buffer, what we use here is the size of the buffer itself, not the upload heap buffer!
    IBView.SizeInBytes = StaticCast<UINT>(m_pIndexBuffer->GetDesc().Size - m_IndexDataStartOffset);

    // Device context keeps strong reference to bound index buffer.
    // When the buffer is unbound, the reference to the D3D12 resource
    // is added to the context. There is no need to add reference here
    //auto &GraphicsCtx = GetCmdContext().AsGraphicsContext();
    //auto *pd3d12Resource = pBuffD3D12->GetD3D12Buffer();
    //GraphicsCtx.AddReferencedObject(pd3d12Resource);

    bool IsDynamic = m_pIndexBuffer->GetDesc().Usage == USAGE_DYNAMIC;
#ifdef DILIGENT_DEVELOPMENT
    if (IsDynamic)
        m_pIndexBuffer->DvpVerifyDynamicAllocation(this);
#endif

    Uint64 BuffDataStartByteOffset;
    auto*  pd3d12Buff = m_pIndexBuffer->GetD3D12Buffer(BuffDataStartByteOffset, this);

    // clang-format off
    if (IsDynamic ||
        m_State.CommittedD3D12IndexBuffer          != pd3d12Buff ||
        m_State.CommittedIBFormat                  != IndexType ||
        m_State.CommittedD3D12IndexDataStartOffset != m_IndexDataStartOffset + BuffDataStartByteOffset)
    // clang-format on
    {
        m_State.CommittedD3D12IndexBuffer          = pd3d12Buff;
        m_State.CommittedIBFormat                  = IndexType;
        m_State.CommittedD3D12IndexDataStartOffset = m_IndexDataStartOffset + BuffDataStartByteOffset;
        GraphCtx.SetIndexBuffer(IBView);
    }

    // GPU virtual address of a dynamic index buffer can change every time
    // a draw command is invoked
    m_State.bCommittedD3D12IBUpToDate = !IsDynamic;
}

void DeviceContextD3D12Impl::CommitD3D12VertexBuffers(GraphicsContext& GraphCtx)
{
    // Do not initialize array with zeroes for performance reasons
    D3D12_VERTEX_BUFFER_VIEW VBViews[MAX_BUFFER_SLOTS]; // = {}
    VERIFY(m_NumVertexStreams <= MAX_BUFFER_SLOTS, "Too many buffers are being set");
    DEV_CHECK_ERR(m_NumVertexStreams >= m_pPipelineState->GetNumBufferSlotsUsed(), "Currently bound pipeline state '", m_pPipelineState->GetDesc().Name, "' expects ", m_pPipelineState->GetNumBufferSlotsUsed(), " input buffer slots, but only ", m_NumVertexStreams, " is bound");
    bool DynamicBufferPresent = false;
    for (UINT Buff = 0; Buff < m_NumVertexStreams; ++Buff)
    {
        auto& CurrStream = m_VertexStreams[Buff];
        auto& VBView     = VBViews[Buff];
        if (auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr())
        {
            if (pBufferD3D12->GetDesc().Usage == USAGE_DYNAMIC)
            {
                DynamicBufferPresent = true;
#ifdef DILIGENT_DEVELOPMENT
                pBufferD3D12->DvpVerifyDynamicAllocation(this);
#endif
            }

            // Device context keeps strong references to all vertex buffers.
            // When a buffer is unbound, a reference to D3D12 resource is added to the context,
            // so there is no need to reference the resource here
            //GraphicsCtx.AddReferencedObject(pd3d12Resource);

            VBView.BufferLocation = pBufferD3D12->GetGPUAddress(GetContextId(), this) + CurrStream.Offset;
            VBView.StrideInBytes  = m_pPipelineState->GetBufferStride(Buff);
            // Note that for a dynamic buffer, what we use here is the size of the buffer itself, not the upload heap buffer!
            VBView.SizeInBytes = StaticCast<UINT>(pBufferD3D12->GetDesc().Size - CurrStream.Offset);
        }
        else
        {
            VBView = D3D12_VERTEX_BUFFER_VIEW{};
        }
    }

    GraphCtx.FlushResourceBarriers();
    GraphCtx.SetVertexBuffers(0, m_NumVertexStreams, VBViews);

    // GPU virtual address of a dynamic vertex buffer can change every time
    // a draw command is invoked
    m_State.bCommittedD3D12VBsUpToDate = !DynamicBufferPresent;
}

void DeviceContextD3D12Impl::PrepareForDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();
#endif

    if (!m_State.bCommittedD3D12VBsUpToDate && m_pPipelineState->GetNumBufferSlotsUsed() > 0)
    {
        CommitD3D12VertexBuffers(GraphCtx);
    }

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        for (Uint32 Buff = 0; Buff < m_NumVertexStreams; ++Buff)
        {
            const auto& CurrStream   = m_VertexStreams[Buff];
            const auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr();
            if (pBufferD3D12 != nullptr)
            {
                DvpVerifyBufferState(*pBufferD3D12, RESOURCE_STATE_VERTEX_BUFFER, "Using vertex buffers (DeviceContextD3D12Impl::Draw())");
            }
        }
    }
#endif

    auto& RootInfo = GetRootTableInfo(PIPELINE_TYPE_GRAPHICS);
#ifdef DILIGENT_DEVELOPMENT
    DvpValidateCommittedShaderResources(RootInfo);
#endif
    if (Uint32 CommitSRBMask = RootInfo.GetCommitMask(Flags & DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT))
    {
        CommitRootTablesAndViews<false>(RootInfo, CommitSRBMask, GraphCtx);
    }

#ifdef NTDDI_WIN10_19H1
    // In Vulkan, shading rate is applied to a PSO created with the shading rate dynamic state.
    // In D3D12, shading rate is applied to all subsequent draw commands, but for compatibility with Vulkan
    // we need to reset shading rate to default state.
    if (m_State.bUsingShadingRate && m_pPipelineState->GetGraphicsPipelineDesc().ShadingRateFlags == PIPELINE_SHADING_RATE_FLAG_NONE)
    {
        m_State.bUsingShadingRate = false;
        GraphCtx.AsGraphicsContext5().SetShadingRate(D3D12_SHADING_RATE_1X1, nullptr);
    }
#endif
}

void DeviceContextD3D12Impl::PrepareForIndexedDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags, VALUE_TYPE IndexType)
{
    PrepareForDraw(GraphCtx, Flags);
    if (m_State.CommittedIBFormat != IndexType)
        m_State.bCommittedD3D12IBUpToDate = false;
    if (!m_State.bCommittedD3D12IBUpToDate)
    {
        CommitD3D12IndexBuffer(GraphCtx, IndexType);
    }
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        DvpVerifyBufferState(*m_pIndexBuffer, RESOURCE_STATE_INDEX_BUFFER, "Indexed draw (DeviceContextD3D12Impl::Draw())");
    }
#endif
}

void DeviceContextD3D12Impl::Draw(const DrawAttribs& Attribs)
{
    TDeviceContextBase::Draw(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);
    if (Attribs.NumVertices > 0 && Attribs.NumInstances > 0)
    {
        GraphCtx.Draw(Attribs.NumVertices, Attribs.NumInstances, Attribs.StartVertexLocation, Attribs.FirstInstanceLocation);
        ++m_State.NumCommands;
    }
}

void DeviceContextD3D12Impl::MultiDraw(const MultiDrawAttribs& Attribs)
{
    TDeviceContextBase::MultiDraw(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);
    if (Attribs.NumInstances > 0)
    {
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumVertices > 0)
            {
                GraphCtx.Draw(Item.NumVertices, Attribs.NumInstances, Item.StartVertexLocation, Attribs.FirstInstanceLocation);
                ++m_State.NumCommands;
            }
        }
    }
}

void DeviceContextD3D12Impl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    TDeviceContextBase::DrawIndexed(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForIndexedDraw(GraphCtx, Attribs.Flags, Attribs.IndexType);
    if (Attribs.NumIndices > 0 && Attribs.NumInstances > 0)
    {
        GraphCtx.DrawIndexed(Attribs.NumIndices, Attribs.NumInstances, Attribs.FirstIndexLocation, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
        ++m_State.NumCommands;
    }
}

void DeviceContextD3D12Impl::MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs)
{
    TDeviceContextBase::MultiDrawIndexed(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForIndexedDraw(GraphCtx, Attribs.Flags, Attribs.IndexType);
    if (Attribs.NumInstances > 0)
    {
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
        {
            const auto& Item = Attribs.pDrawItems[i];
            if (Item.NumIndices > 0)
            {
                GraphCtx.DrawIndexed(Item.NumIndices, Attribs.NumInstances, Item.FirstIndexLocation, Item.BaseVertex, Attribs.FirstInstanceLocation);
                ++m_State.NumCommands;
            }
        }
    }
}

void DeviceContextD3D12Impl::PrepareIndirectAttribsBuffer(CommandContext&                CmdCtx,
                                                          IBuffer*                       pAttribsBuffer,
                                                          RESOURCE_STATE_TRANSITION_MODE BufferStateTransitionMode,
                                                          ID3D12Resource*&               pd3d12ArgsBuff,
                                                          Uint64&                        BuffDataStartByteOffset,
                                                          const char*                    OpName)
{
    DEV_CHECK_ERR(pAttribsBuffer != nullptr, "Indirect draw attribs buffer must not be null");

    auto* pIndirectDrawAttribsD3D12 = ClassPtrCast<BufferD3D12Impl>(pAttribsBuffer);
#ifdef DILIGENT_DEVELOPMENT
    if (pIndirectDrawAttribsD3D12->GetDesc().Usage == USAGE_DYNAMIC)
        pIndirectDrawAttribsD3D12->DvpVerifyDynamicAllocation(this);
#endif

    TransitionOrVerifyBufferState(CmdCtx, *pIndirectDrawAttribsD3D12, BufferStateTransitionMode,
                                  RESOURCE_STATE_INDIRECT_ARGUMENT,
                                  OpName);

    pd3d12ArgsBuff = pIndirectDrawAttribsD3D12->GetD3D12Buffer(BuffDataStartByteOffset, this);
}

void DeviceContextD3D12Impl::DrawIndirect(const DrawIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawIndirect(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    ID3D12Resource* pd3d12ArgsBuff          = nullptr;
    Uint64          BuffDataStartByteOffset = 0;
    PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset,
                                 "Indirect draw (DeviceContextD3D12Impl::DrawIndirect)");

    auto* pDrawIndirectSignature = GetDrawIndirectSignature(Attribs.DrawCount > 1 ? Attribs.DrawArgsStride : sizeof(UINT) * 4);
    VERIFY_EXPR(pDrawIndirectSignature != nullptr);

    ID3D12Resource* pd3d12CountBuff              = nullptr;
    Uint64          CountBuffDataStartByteOffset = 0;
    if (Attribs.pCounterBuffer != nullptr)
    {
        PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, pd3d12CountBuff, CountBuffDataStartByteOffset,
                                     "Counter buffer (DeviceContextD3D12Impl::DrawIndirect)");
    }

    if (Attribs.DrawCount > 0)
    {
        GraphCtx.ExecuteIndirect(pDrawIndirectSignature,
                                 Attribs.DrawCount,
                                 pd3d12ArgsBuff,
                                 Attribs.DrawArgsOffset + BuffDataStartByteOffset,
                                 pd3d12CountBuff,
                                 pd3d12CountBuff != nullptr ? Attribs.CounterOffset + CountBuffDataStartByteOffset : 0);
    }

    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawIndexedIndirect(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForIndexedDraw(GraphCtx, Attribs.Flags, Attribs.IndexType);

    ID3D12Resource* pd3d12ArgsBuff          = nullptr;
    Uint64          BuffDataStartByteOffset = 0;
    PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset,
                                 "indexed Indirect draw (DeviceContextD3D12Impl::DrawIndexedIndirect)");

    auto* pDrawIndexedIndirectSignature = GetDrawIndexedIndirectSignature(Attribs.DrawCount > 1 ? Attribs.DrawArgsStride : sizeof(UINT) * 5);
    VERIFY_EXPR(pDrawIndexedIndirectSignature != nullptr);


    ID3D12Resource* pd3d12CountBuff              = nullptr;
    Uint64          CountBuffDataStartByteOffset = 0;
    if (Attribs.pCounterBuffer != nullptr)
    {
        PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, pd3d12CountBuff, CountBuffDataStartByteOffset,
                                     "Count buffer (DeviceContextD3D12Impl::DrawIndexedIndirect)");
    }

    if (Attribs.DrawCount > 0)
    {
        GraphCtx.ExecuteIndirect(pDrawIndexedIndirectSignature,
                                 Attribs.DrawCount,
                                 pd3d12ArgsBuff,
                                 Attribs.DrawArgsOffset + BuffDataStartByteOffset,
                                 pd3d12CountBuff,
                                 pd3d12CountBuff != nullptr ? Attribs.CounterOffset + CountBuffDataStartByteOffset : 0);
    }

    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    TDeviceContextBase::DrawMesh(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext6();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    if (Attribs.ThreadGroupCountX > 0 && Attribs.ThreadGroupCountY > 0 && Attribs.ThreadGroupCountZ > 0)
    {
        GraphCtx.DrawMesh(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
        ++m_State.NumCommands;
    }
}

void DeviceContextD3D12Impl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs)
{
    TDeviceContextBase::DrawMeshIndirect(Attribs, 0);

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    ID3D12Resource* pd3d12ArgsBuff          = nullptr;
    Uint64          BuffDataStartByteOffset = 0;
    PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset,
                                 "Indirect draw mesh (DeviceContextD3D12Impl::DrawMeshIndirect)");

    ID3D12Resource* pd3d12CountBuff              = nullptr;
    Uint64          CountBuffDataStartByteOffset = 0;
    if (Attribs.pCounterBuffer != nullptr)
    {
        PrepareIndirectAttribsBuffer(GraphCtx, Attribs.pCounterBuffer, Attribs.CounterBufferStateTransitionMode, pd3d12CountBuff, CountBuffDataStartByteOffset,
                                     "Counter buffer (DeviceContextD3D12Impl::DrawMeshIndirect)");
    }

    if (Attribs.CommandCount > 0)
    {
        GraphCtx.ExecuteIndirect(m_pDrawMeshIndirectSignature,
                                 Attribs.CommandCount,
                                 pd3d12ArgsBuff,
                                 Attribs.DrawArgsOffset + BuffDataStartByteOffset,
                                 pd3d12CountBuff,
                                 Attribs.CounterOffset + CountBuffDataStartByteOffset);
    }

    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::PrepareForDispatchCompute(ComputeContext& ComputeCtx)
{
    auto& RootInfo = GetRootTableInfo(PIPELINE_TYPE_COMPUTE);
#ifdef DILIGENT_DEVELOPMENT
    DvpValidateCommittedShaderResources(RootInfo);
#endif
    if (Uint32 CommitSRBMask = RootInfo.GetCommitMask())
    {
        CommitRootTablesAndViews<true>(RootInfo, CommitSRBMask, ComputeCtx);
    }
}

void DeviceContextD3D12Impl::PrepareForDispatchRays(GraphicsContext& GraphCtx)
{
    auto& RootInfo = GetRootTableInfo(PIPELINE_TYPE_RAY_TRACING);
#ifdef DILIGENT_DEVELOPMENT
    DvpValidateCommittedShaderResources(RootInfo);
#endif
    if (Uint32 CommitSRBMask = RootInfo.GetCommitMask())
    {
        CommitRootTablesAndViews<true>(RootInfo, CommitSRBMask, GraphCtx);
    }
}

void DeviceContextD3D12Impl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    TDeviceContextBase::DispatchCompute(Attribs, 0);

    auto& ComputeCtx = GetCmdContext().AsComputeContext();
    PrepareForDispatchCompute(ComputeCtx);
    if (Attribs.ThreadGroupCountX > 0 && Attribs.ThreadGroupCountY > 0 && Attribs.ThreadGroupCountZ > 0)
    {
        ComputeCtx.Dispatch(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
        ++m_State.NumCommands;
    }
}

void DeviceContextD3D12Impl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs)
{
    TDeviceContextBase::DispatchComputeIndirect(Attribs, 0);

    auto& ComputeCtx = GetCmdContext().AsComputeContext();
    PrepareForDispatchCompute(ComputeCtx);

    ID3D12Resource* pd3d12ArgsBuff;
    Uint64          BuffDataStartByteOffset;
    PrepareIndirectAttribsBuffer(ComputeCtx, Attribs.pAttribsBuffer, Attribs.AttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset,
                                 "Indirect dispatch (DeviceContextD3D12Impl::DispatchComputeIndirect)");

    ComputeCtx.ExecuteIndirect(m_pDispatchIndirectSignature, 1, pd3d12ArgsBuff, Attribs.DispatchArgsByteOffset + BuffDataStartByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::ClearDepthStencil(ITextureView*                  pView,
                                               CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                               float                          fDepth,
                                               Uint8                          Stencil,
                                               RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Direct3D12 does not allow depth-stencil clears inside a render pass");

    TDeviceContextBase::ClearDepthStencil(pView);

    auto* pViewD3D12    = ClassPtrCast<ITextureViewD3D12>(pView);
    auto* pTextureD3D12 = ClassPtrCast<TextureD3D12Impl>(pViewD3D12->GetTexture());
    auto& CmdCtx        = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pTextureD3D12, StateTransitionMode, RESOURCE_STATE_DEPTH_WRITE, "Clearing depth-stencil buffer (DeviceContextD3D12Impl::ClearDepthStencil)");

    D3D12_CLEAR_FLAGS d3d12ClearFlags = (D3D12_CLEAR_FLAGS)0;
    if (ClearFlags & CLEAR_DEPTH_FLAG) d3d12ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    if (ClearFlags & CLEAR_STENCIL_FLAG) d3d12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;

    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied??
    GetCmdContext().AsGraphicsContext().ClearDepthStencil(pViewD3D12->GetCPUDescriptorHandle(), d3d12ClearFlags, fDepth, Stencil);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::ClearRenderTarget(ITextureView* pView, const void* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Direct3D12 does not allow render target clears inside a render pass");

    TDeviceContextBase::ClearRenderTarget(pView);

    auto* pViewD3D12 = ClassPtrCast<ITextureViewD3D12>(pView);

    static constexpr float Zero[4] = {0.f, 0.f, 0.f, 0.f};
    if (RGBA == nullptr)
        RGBA = Zero;

#ifdef DILIGENT_DEVELOPMENT
    {
        const TEXTURE_FORMAT        RTVFormat  = pViewD3D12->GetDesc().Format;
        const TextureFormatAttribs& FmtAttribs = GetTextureFormatAttribs(RTVFormat);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_SINT ||
            FmtAttribs.ComponentType == COMPONENT_TYPE_UINT)
        {
            DEV_CHECK_ERR(memcmp(RGBA, Zero, 4 * sizeof(float)) == 0, "Integer render targets can at the moment only be cleared to zero in Direct3D12");
        }
    }
#endif

    auto* pTextureD3D12 = ClassPtrCast<TextureD3D12Impl>(pViewD3D12->GetTexture());
    auto& CmdCtx        = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pTextureD3D12, StateTransitionMode, RESOURCE_STATE_RENDER_TARGET, "Clearing render target (DeviceContextD3D12Impl::ClearRenderTarget)");

    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied??
    CmdCtx.AsGraphicsContext().ClearRenderTarget(pViewD3D12->GetCPUDescriptorHandle(), static_cast<const float*>(RGBA));
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::RequestCommandContext()
{
    m_CurrCmdCtx = m_pDevice->AllocateCommandContext(GetCommandQueueId(), "Command list");
    m_CurrCmdCtx->SetDynamicGPUDescriptorAllocators(m_DynamicGPUDescriptorAllocator);
}

void DeviceContextD3D12Impl::Flush(bool                 RequestNewCmdCtx,
                                   Uint32               NumCommandLists,
                                   ICommandList* const* ppCommandLists)
{
    VERIFY(!IsDeferred() || NumCommandLists == 0 && ppCommandLists == nullptr, "Only immediate context can execute command lists");

    DEV_CHECK_ERR(m_ActiveQueriesCounter == 0,
                  "Flushing device context that has ", m_ActiveQueriesCounter,
                  " active queries. Direct3D12 requires that queries are begun and ended in the same command list");

    // TODO: use small_vector
    std::vector<RenderDeviceD3D12Impl::PooledCommandContext> Contexts;
    Contexts.reserve(size_t{NumCommandLists} + 1);

    // First, execute current context
    if (m_CurrCmdCtx)
    {
        VERIFY(!IsDeferred(), "Deferred contexts cannot execute command lists directly");
        if (m_State.NumCommands != 0)
            Contexts.emplace_back(std::move(m_CurrCmdCtx));
        else if (!RequestNewCmdCtx) // Reuse existing context instead of disposing and creating new one.
            m_pDevice->DisposeCommandContext(std::move(m_CurrCmdCtx));
    }

    // Next, add extra command lists from deferred contexts
    for (Uint32 i = 0; i < NumCommandLists; ++i)
    {
        auto* const pCmdListD3D12 = ClassPtrCast<CommandListD3D12Impl>(ppCommandLists[i]);

        RefCntAutoPtr<DeviceContextD3D12Impl> pDeferredCtx;
        Contexts.emplace_back(pCmdListD3D12->Close(pDeferredCtx));
        VERIFY(Contexts.back() && pDeferredCtx, "Trying to execute empty command buffer");
        // Set the bit in the deferred context cmd queue mask corresponding to the cmd queue of this context
        pDeferredCtx->UpdateSubmittedBuffersCmdQueueMask(GetCommandQueueId());
    }

    if (!Contexts.empty())
    {
        m_pDevice->CloseAndExecuteCommandContexts(GetCommandQueueId(), static_cast<Uint32>(Contexts.size()), Contexts.data(), true, &m_SignalFences, &m_WaitFences);

#ifdef DILIGENT_DEBUG
        for (const auto& Ctx : Contexts)
            VERIFY(!Ctx, "All contexts must be disposed by CloseAndExecuteCommandContexts");
#endif
    }
    else
    {
        // If there is no command list to submit, but there are pending fences, we need to process them now

        if (!m_WaitFences.empty())
        {
            m_pDevice->WaitFences(GetCommandQueueId(), m_WaitFences);
        }

        if (!m_SignalFences.empty())
        {
            m_pDevice->SignalFences(GetCommandQueueId(), m_SignalFences);
        }
    }

    m_SignalFences.clear();
    m_WaitFences.clear();

    if (!m_CurrCmdCtx && RequestNewCmdCtx)
        RequestCommandContext();

    m_State             = State{};
    m_GraphicsResources = RootTableInfo{};
    m_ComputeResources  = RootTableInfo{};

    // Setting pipeline state to null makes sure that render targets and other
    // states will be restored in the command list next time a PSO is bound.
    m_pPipelineState = nullptr;
}

void DeviceContextD3D12Impl::Flush()
{
    DEV_CHECK_ERR(!IsDeferred(), "Flush() should only be called for immediate contexts");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Flushing device context inside an active render pass.");

    Flush(true);
}

void DeviceContextD3D12Impl::FinishFrame()
{
#ifdef DILIGENT_DEBUG
    for (const auto& MappedBuffIt : m_DbgMappedBuffers)
    {
        const auto& BuffDesc = MappedBuffIt.first->GetDesc();
        if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            LOG_WARNING_MESSAGE("Dynamic buffer '", BuffDesc.Name,
                                "' is still mapped when finishing the frame. The contents of the buffer and "
                                "mapped address will become invalid");
        }
    }
#endif
    if (GetNumCommandsInCtx() != 0)
    {
        if (IsDeferred())
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in deferred device context #", GetContextId(),
                              " when finishing the frame. This is an error and may cause unpredicted behaviour. "
                              "Close all deferred contexts and execute them before finishing the frame");
        }
        else
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in the immediate device context when finishing the frame. "
                              "This is an error and may cause unpredicted behaviour. Call Flush() to submit all commands"
                              " for execution before finishing the frame");
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

    const auto QueueMask = GetSubmittedBuffersCmdQueueMask();
    VERIFY_EXPR(IsDeferred() || QueueMask == (Uint64{1} << GetCommandQueueId()));

    // Released pages are returned to the global dynamic memory manager hosted by render device.
    m_DynamicHeap.ReleaseAllocatedPages(QueueMask);

    // Dynamic GPU descriptor allocations are returned to the global GPU descriptor heap
    // hosted by the render device.
    for (size_t i = 0; i < _countof(m_DynamicGPUDescriptorAllocator); ++i)
        m_DynamicGPUDescriptorAllocator[i].ReleaseAllocations(QueueMask);

    EndFrame();
}

void DeviceContextD3D12Impl::SetVertexBuffers(Uint32                         StartSlot,
                                              Uint32                         NumBuffersSet,
                                              IBuffer* const*                ppBuffers,
                                              const Uint64*                  pOffsets,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                              SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);

    auto& CmdCtx = GetCmdContext();
    for (Uint32 Buff = 0; Buff < m_NumVertexStreams; ++Buff)
    {
        auto& CurrStream = m_VertexStreams[Buff];
        if (auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr())
            TransitionOrVerifyBufferState(CmdCtx, *pBufferD3D12, StateTransitionMode, RESOURCE_STATE_VERTEX_BUFFER, "Setting vertex buffers (DeviceContextD3D12Impl::SetVertexBuffers)");
    }

    m_State.bCommittedD3D12VBsUpToDate = false;
}

void DeviceContextD3D12Impl::InvalidateState()
{
    if (m_State.NumCommands != 0)
        LOG_WARNING_MESSAGE("Invalidating context that has outstanding commands in it. Call Flush() to submit commands for execution");

    TDeviceContextBase::InvalidateState();
    m_State             = {};
    m_GraphicsResources = {};
    m_ComputeResources  = {};
}

void DeviceContextD3D12Impl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint64 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);
    if (m_pIndexBuffer)
    {
        auto& CmdCtx = GetCmdContext();
        TransitionOrVerifyBufferState(CmdCtx, *m_pIndexBuffer, StateTransitionMode, RESOURCE_STATE_INDEX_BUFFER, "Setting index buffer (DeviceContextD3D12Impl::SetIndexBuffer)");
    }
    m_State.bCommittedD3D12IBUpToDate = false;
}

void DeviceContextD3D12Impl::CommitViewports()
{
    static_assert(MAX_VIEWPORTS >= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    D3D12_VIEWPORT d3d12Viewports[MAX_VIEWPORTS]; // Do not waste time initializing array to zero

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        d3d12Viewports[vp].TopLeftX = m_Viewports[vp].TopLeftX;
        d3d12Viewports[vp].TopLeftY = m_Viewports[vp].TopLeftY;
        d3d12Viewports[vp].Width    = m_Viewports[vp].Width;
        d3d12Viewports[vp].Height   = m_Viewports[vp].Height;
        d3d12Viewports[vp].MinDepth = m_Viewports[vp].MinDepth;
        d3d12Viewports[vp].MaxDepth = m_Viewports[vp].MaxDepth;
    }
    // All viewports must be set atomically as one operation.
    // Any viewports not defined by the call are disabled.
    GetCmdContext().AsGraphicsContext().SetViewports(m_NumViewports, d3d12Viewports);
}

void DeviceContextD3D12Impl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    static_assert(MAX_VIEWPORTS >= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);
    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");

    CommitViewports();
}


constexpr LONG   MaxD3D12TexDim       = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
constexpr Uint32 MaxD3D12ScissorRects = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
// clang-format off
static constexpr RECT MaxD3D12TexSizeRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] =
{
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim}
};
// clang-format on

void DeviceContextD3D12Impl::CommitScissorRects(GraphicsContext& GraphCtx, bool ScissorEnable)
{
    if (ScissorEnable)
    {
        // Commit currently set scissor rectangles
        D3D12_RECT d3d12ScissorRects[MaxD3D12ScissorRects]; // Do not waste time initializing array with zeroes
        for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
        {
            d3d12ScissorRects[sr].left   = m_ScissorRects[sr].left;
            d3d12ScissorRects[sr].top    = m_ScissorRects[sr].top;
            d3d12ScissorRects[sr].right  = m_ScissorRects[sr].right;
            d3d12ScissorRects[sr].bottom = m_ScissorRects[sr].bottom;
        }
        GraphCtx.SetScissorRects(m_NumScissorRects, d3d12ScissorRects);
    }
    else
    {
        // Disable scissor rectangles
        static_assert(_countof(MaxD3D12TexSizeRects) == MaxD3D12ScissorRects, "Unexpected array size");
        GraphCtx.SetScissorRects(MaxD3D12ScissorRects, MaxD3D12TexSizeRects);
    }
}

void DeviceContextD3D12Impl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    const Uint32 MaxScissorRects = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    VERIFY(NumRects < MaxScissorRects, "Too many scissor rects are being set");
    NumRects = std::min(NumRects, MaxScissorRects);

    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    // Only commit scissor rects if scissor test is enabled in the rasterizer state.
    // If scissor is currently disabled, or no PSO is bound, scissor rects will be committed by
    // the SetPipelineState() when a PSO with enabled scissor test is set.
    if (m_pPipelineState)
    {
        const auto& PSODesc = m_pPipelineState->GetDesc();
        if (PSODesc.IsAnyGraphicsPipeline() && m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable)
        {
            VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
            auto& Ctx = GetCmdContext().AsGraphicsContext();
            CommitScissorRects(Ctx, true);
        }
    }
}

void DeviceContextD3D12Impl::CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "This method must not be called inside a render pass");

    const Uint32 MaxD3D12RTs      = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    Uint32       NumRenderTargets = m_NumBoundRenderTargets;
    VERIFY(NumRenderTargets <= MaxD3D12RTs, "D3D12 only allows 8 simultaneous render targets");
    NumRenderTargets = std::min(MaxD3D12RTs, NumRenderTargets);

    ITextureViewD3D12* ppRTVs[MaxD3D12RTs]; // Do not initialize with zeroes!
    ITextureViewD3D12* pDSV = nullptr;

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        ppRTVs[rt] = m_pBoundRenderTargets[rt].RawPtr();
    pDSV = m_pBoundDepthStencil.RawPtr();

    auto& CmdCtx = GetCmdContext();

    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[MAX_RENDER_TARGETS]; // Do not waste time initializing array to zero
    D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
    for (UINT i = 0; i < NumRenderTargets; ++i)
    {
        if (auto* pRTV = ppRTVs[i])
        {
            auto* pTexture = ClassPtrCast<TextureD3D12Impl>(pRTV->GetTexture());
            TransitionOrVerifyTextureState(CmdCtx, *pTexture, StateTransitionMode, RESOURCE_STATE_RENDER_TARGET, "Setting render targets (DeviceContextD3D12Impl::CommitRenderTargets)");
            RTVHandles[i] = pRTV->GetCPUDescriptorHandle();
            VERIFY_EXPR(RTVHandles[i].ptr != 0);
        }
        else
        {
            // Binding NULL descriptor handle is invalid. We need to use a non-NULL handle
            // that defines null RTV.
            RTVHandles[i] = m_NullRTV.GetCpuHandle();
        }
    }

    if (pDSV)
    {
        const auto ViewType = m_pBoundDepthStencil->GetDesc().ViewType;
        VERIFY_EXPR(ViewType == TEXTURE_VIEW_DEPTH_STENCIL || ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL);
        auto NewState = ViewType == TEXTURE_VIEW_DEPTH_STENCIL ?
            RESOURCE_STATE_DEPTH_WRITE :
            RESOURCE_STATE_DEPTH_READ;
        if (NewState == RESOURCE_STATE_DEPTH_READ && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        {
            // Read-only depth is likely to be used as shader resource, so set this flag.
            // If this is not intended, the app should manually transition resource states.
            NewState |= RESOURCE_STATE_SHADER_RESOURCE;
        }

        auto* pTexture = ClassPtrCast<TextureD3D12Impl>(pDSV->GetTexture());

        TransitionOrVerifyTextureState(CmdCtx, *pTexture, StateTransitionMode, NewState, "Setting depth-stencil buffer (DeviceContextD3D12Impl::CommitRenderTargets)");
        DSVHandle = pDSV->GetCPUDescriptorHandle();
        VERIFY_EXPR(DSVHandle.ptr != 0);
    }

    if (NumRenderTargets > 0 || DSVHandle.ptr != 0)
    {
        // No need to flush resource barriers as this is a CPU-side command
        CmdCtx.AsGraphicsContext().GetCommandList()->OMSetRenderTargets(NumRenderTargets, RTVHandles, FALSE, DSVHandle.ptr != 0 ? &DSVHandle : nullptr);
    }

#ifdef NTDDI_WIN10_19H1
    if (m_pBoundShadingRateMap != nullptr)
    {
        auto* pTexD3D12 = ClassPtrCast<TextureD3D12Impl>(m_pBoundShadingRateMap->GetTexture());
        TransitionOrVerifyTextureState(CmdCtx, *pTexD3D12, StateTransitionMode, RESOURCE_STATE_SHADING_RATE, "Shading rate texture (DeviceContextD3D12Impl::CommitRenderTargets)");

        m_State.bShadingRateMapBound = true;
        CmdCtx.AsGraphicsContext5().SetShadingRateImage(pTexD3D12->GetD3D12Resource());
    }
    else if (m_State.bShadingRateMapBound)
    {
        m_State.bShadingRateMapBound = false;
        CmdCtx.AsGraphicsContext5().SetShadingRateImage(nullptr);
    }
#endif
}

void DeviceContextD3D12Impl::SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Calling SetRenderTargets inside active render pass is invalid. End the render pass first");

    if (TDeviceContextBase::SetRenderTargets(Attribs))
    {
        CommitRenderTargets(Attribs.StateTransitionMode);

        // Set the viewport to match the render target size
        SetViewports(1, nullptr, 0, 0);
    }
}

void DeviceContextD3D12Impl::TransitionSubpassAttachments(Uint32 NextSubpass)
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY_EXPR(RPDesc.AttachmentCount == FBDesc.AttachmentCount);
    for (Uint32 att = 0; att < RPDesc.AttachmentCount; ++att)
    {
        const auto& AttDesc  = RPDesc.pAttachments[att];
        auto        OldState = NextSubpass > 0 ? m_pActiveRenderPass->GetAttachmentState(NextSubpass - 1, att) : AttDesc.InitialState;
        auto        NewState = NextSubpass < RPDesc.SubpassCount ? m_pActiveRenderPass->GetAttachmentState(NextSubpass, att) : AttDesc.FinalState;
        if (OldState != NewState)
        {
            auto&      CmdCtx       = GetCmdContext();
            const auto ResStateMask = GetSupportedD3D12ResourceStatesForCommandList(CmdCtx.GetCommandListType());

            auto* pViewD3D12 = ClassPtrCast<TextureViewD3D12Impl>(FBDesc.ppAttachments[att]);
            if (pViewD3D12 == nullptr)
                continue;

            auto* pTexD3D12 = pViewD3D12->GetTexture<TextureD3D12Impl>();

            const auto& ViewDesc = pViewD3D12->GetDesc();
            const auto& TexDesc  = pTexD3D12->GetDesc();

            D3D12_RESOURCE_BARRIER BarrierDesc;
            BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            BarrierDesc.Transition.pResource   = pTexD3D12->GetD3D12Resource();
            BarrierDesc.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(OldState) & ResStateMask;
            BarrierDesc.Transition.StateAfter  = ResourceStateFlagsToD3D12ResourceStates(NewState) & ResStateMask;

            for (Uint32 mip = ViewDesc.MostDetailedMip; mip < ViewDesc.MostDetailedMip + ViewDesc.NumDepthSlices; ++mip)
            {
                for (Uint32 slice = ViewDesc.FirstArraySlice; slice < ViewDesc.FirstArraySlice + ViewDesc.NumArraySlices; ++slice)
                {
                    BarrierDesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, TexDesc.MipLevels, TexDesc.GetArraySize());
                    CmdCtx.ResourceBarrier(BarrierDesc);
                }
            }
        }
    }
}

void DeviceContextD3D12Impl::CommitSubpassRenderTargets()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& Subpass = RPDesc.pSubpasses[m_SubpassIndex];
    VERIFY(Subpass.RenderTargetAttachmentCount == m_NumBoundRenderTargets,
           "The number of currently bound render targets (", m_NumBoundRenderTargets,
           ") is not consistent with the number of render target attachments (", Subpass.RenderTargetAttachmentCount,
           ") in current subpass");

    D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderPassRTs[MAX_RENDER_TARGETS];
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        const auto& RTRef = Subpass.pRenderTargetAttachments[rt];
        if (RTRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            TextureViewD3D12Impl* pRTV = m_pBoundRenderTargets[rt];
            VERIFY(pRTV == FBDesc.ppAttachments[RTRef.AttachmentIndex],
                   "Render target bound in the device context at slot ", rt, " is not consistent with the corresponding framebuffer attachment");
            const auto  FirstLastUse     = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTRef.AttachmentIndex);
            const auto& RTAttachmentDesc = RPDesc.pAttachments[RTRef.AttachmentIndex];

            auto& RPRT = RenderPassRTs[rt];
            RPRT       = D3D12_RENDER_PASS_RENDER_TARGET_DESC{};

            RPRT.cpuDescriptor = pRTV->GetCPUDescriptorHandle();
            if (FirstLastUse.first == m_SubpassIndex)
            {
                // This is the first use of this attachment - use LoadOp
                RPRT.BeginningAccess.Type = AttachmentLoadOpToD3D12BeginningAccessType(RTAttachmentDesc.LoadOp);
            }
            else
            {
                // Preserve the attachment contents
                RPRT.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            }

            if (RPRT.BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
            {
                RPRT.BeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(RTAttachmentDesc.Format);

                const auto ClearColor = m_AttachmentClearValues[RTRef.AttachmentIndex].Color;
                for (Uint32 i = 0; i < 4; ++i)
                    RPRT.BeginningAccess.Clear.ClearValue.Color[i] = ClearColor[i];
            }

            if (FirstLastUse.second == m_SubpassIndex)
            {
                // This is the last use of this attachment - use StoreOp or resolve parameters
                if (Subpass.pResolveAttachments != nullptr && Subpass.pResolveAttachments[rt].AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(Subpass.pResolveAttachments[rt].AttachmentIndex < RPDesc.AttachmentCount);
                    auto* pDstView     = FBDesc.ppAttachments[Subpass.pResolveAttachments[rt].AttachmentIndex];
                    auto* pSrcTexD3D12 = pRTV->GetTexture<TextureD3D12Impl>();
                    auto* pDstTexD3D12 = ClassPtrCast<TextureViewD3D12Impl>(pDstView)->GetTexture<TextureD3D12Impl>();

                    const auto& SrcRTVDesc  = pRTV->GetDesc();
                    const auto& DstViewDesc = pDstView->GetDesc();
                    const auto& SrcTexDesc  = pSrcTexD3D12->GetDesc();
                    const auto& DstTexDesc  = pDstTexD3D12->GetDesc();

                    VERIFY_EXPR(SrcRTVDesc.NumArraySlices == 1);
                    Uint32 SubresourceCount = SrcRTVDesc.NumArraySlices;
                    m_AttachmentResolveInfo.resize(SubresourceCount);
                    const auto MipProps = GetMipLevelProperties(SrcTexDesc, SrcRTVDesc.MostDetailedMip);
                    for (Uint32 slice = 0; slice < SrcRTVDesc.NumArraySlices; ++slice)
                    {
                        auto& ARI = m_AttachmentResolveInfo[slice];

                        ARI.SrcSubresource = D3D12CalcSubresource(SrcRTVDesc.MostDetailedMip, SrcRTVDesc.FirstArraySlice + slice, 0, SrcTexDesc.MipLevels, SrcTexDesc.GetArraySize());
                        ARI.DstSubresource = D3D12CalcSubresource(DstViewDesc.MostDetailedMip, DstViewDesc.FirstArraySlice + slice, 0, DstTexDesc.MipLevels, DstTexDesc.GetArraySize());
                        ARI.DstX           = 0;
                        ARI.DstY           = 0;
                        ARI.SrcRect.left   = 0;
                        ARI.SrcRect.top    = 0;
                        ARI.SrcRect.right  = MipProps.LogicalWidth;
                        ARI.SrcRect.bottom = MipProps.LogicalHeight;
                    }

                    // The resolve source is left in its initial resource state at the time the render pass ends.
                    // A resolve operation submitted by a render pass doesn't implicitly change the state of any resource.
                    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_ending_access_type
                    RPRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;

                    auto& ResolveParams            = RPRT.EndingAccess.Resolve;
                    ResolveParams.pSrcResource     = pSrcTexD3D12->GetD3D12Resource();
                    ResolveParams.pDstResource     = pDstTexD3D12->GetD3D12Resource();
                    ResolveParams.SubresourceCount = SubresourceCount;
                    // This pointer is directly referenced by the command list, and the memory for this array
                    // must remain alive and intact until EndRenderPass is called.
                    ResolveParams.pSubresourceParameters = m_AttachmentResolveInfo.data();
                    ResolveParams.Format                 = TexFormatToDXGI_Format(RTAttachmentDesc.Format);
                    ResolveParams.ResolveMode            = D3D12_RESOLVE_MODE_AVERAGE;
                    ResolveParams.PreserveResolveSource  = RTAttachmentDesc.StoreOp == ATTACHMENT_STORE_OP_STORE;
                }
                else
                {
                    RPRT.EndingAccess.Type = AttachmentStoreOpToD3D12EndingAccessType(RTAttachmentDesc.StoreOp);
                }
            }
            else
            {
                // The attachment will be used in subsequent subpasses - preserve its contents
                RPRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            }
        }
        else
        {
            // Attachment is not used
            RenderPassRTs[rt].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            RenderPassRTs[rt].EndingAccess.Type    = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            continue;
        }
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC RenderPassDS;
    if (m_pBoundDepthStencil)
    {
        RenderPassDS = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{};

        const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
        VERIFY_EXPR(Subpass.pDepthStencilAttachment != nullptr && DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED);
        const auto  FirstLastUse     = m_pActiveRenderPass->GetAttachmentFirstLastUse(DSAttachmentRef.AttachmentIndex);
        const auto& DSAttachmentDesc = RPDesc.pAttachments[DSAttachmentRef.AttachmentIndex];
        VERIFY(m_pBoundDepthStencil ==
                   (DSAttachmentRef.State == RESOURCE_STATE_DEPTH_READ ?
                        m_pBoundFramebuffer->GetReadOnlyDSV(m_SubpassIndex) :
                        FBDesc.ppAttachments[DSAttachmentRef.AttachmentIndex]),
               "Depth-stencil buffer in the device context is inconsistent with the framebuffer");

        RenderPassDS.cpuDescriptor = m_pBoundDepthStencil->GetCPUDescriptorHandle();
        if (FirstLastUse.first == m_SubpassIndex)
        {
            RenderPassDS.DepthBeginningAccess.Type   = AttachmentLoadOpToD3D12BeginningAccessType(DSAttachmentDesc.LoadOp);
            RenderPassDS.StencilBeginningAccess.Type = AttachmentLoadOpToD3D12BeginningAccessType(DSAttachmentDesc.StencilLoadOp);
        }
        else
        {
            RenderPassDS.DepthBeginningAccess.Type   = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            RenderPassDS.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        }

        if (RenderPassDS.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
        {
            RenderPassDS.DepthBeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(DSAttachmentDesc.Format);
            RenderPassDS.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth =
                m_AttachmentClearValues[DSAttachmentRef.AttachmentIndex].DepthStencil.Depth;
        }

        if (RenderPassDS.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
        {
            RenderPassDS.StencilBeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(DSAttachmentDesc.Format);
            RenderPassDS.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil =
                m_AttachmentClearValues[DSAttachmentRef.AttachmentIndex].DepthStencil.Stencil;
        }

        if (FirstLastUse.second == m_SubpassIndex)
        {
            RenderPassDS.DepthEndingAccess.Type   = AttachmentStoreOpToD3D12EndingAccessType(DSAttachmentDesc.StoreOp);
            RenderPassDS.StencilEndingAccess.Type = AttachmentStoreOpToD3D12EndingAccessType(DSAttachmentDesc.StencilStoreOp);
        }
        else
        {
            RenderPassDS.DepthEndingAccess.Type   = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            RenderPassDS.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
    }

    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().BeginRenderPass(
        Subpass.RenderTargetAttachmentCount,
        RenderPassRTs,
        m_pBoundDepthStencil ? &RenderPassDS : nullptr,
        D3D12_RENDER_PASS_FLAG_NONE);

    // Set the viewport to match the framebuffer size
    SetViewports(1, nullptr, 0, 0);

    if (m_pBoundShadingRateMap != nullptr)
    {
        auto* pTexD3D12 = ClassPtrCast<TextureD3D12Impl>(m_pBoundShadingRateMap->GetTexture());
        CmdCtx.AsGraphicsContext5().SetShadingRateImage(pTexD3D12->GetD3D12Resource());
    }
}

void DeviceContextD3D12Impl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);

    m_AttachmentClearValues.resize(Attribs.ClearValueCount);
    for (Uint32 i = 0; i < Attribs.ClearValueCount; ++i)
        m_AttachmentClearValues[i] = Attribs.pClearValues[i];

    TransitionSubpassAttachments(m_SubpassIndex);
    CommitSubpassRenderTargets();
}

void DeviceContextD3D12Impl::NextSubpass()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().EndRenderPass();
    TDeviceContextBase::NextSubpass();
    TransitionSubpassAttachments(m_SubpassIndex);
    CommitSubpassRenderTargets();
}

void DeviceContextD3D12Impl::EndRenderPass()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().EndRenderPass();
    TransitionSubpassAttachments(m_SubpassIndex + 1);
    if (m_pBoundShadingRateMap)
        CmdCtx.AsGraphicsContext5().SetShadingRateImage(nullptr);
    TDeviceContextBase::EndRenderPass();
}

D3D12DynamicAllocation DeviceContextD3D12Impl::AllocateDynamicSpace(Uint64 NumBytes, Uint32 Alignment)
{
    return m_DynamicHeap.Allocate(NumBytes, Alignment, GetFrameNumber());
}

void DeviceContextD3D12Impl::UpdateBufferRegion(BufferD3D12Impl*               pBuffD3D12,
                                                D3D12DynamicAllocation&        Allocation,
                                                Uint64                         DstOffset,
                                                Uint64                         NumBytes,
                                                RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    auto& CmdCtx = GetCmdContext();
    TransitionOrVerifyBufferState(CmdCtx, *pBuffD3D12, StateTransitionMode, RESOURCE_STATE_COPY_DEST, "Updating buffer (DeviceContextD3D12Impl::UpdateBufferRegion)");
    Uint64 DstBuffDataStartByteOffset;
    auto*  pd3d12Buff = pBuffD3D12->GetD3D12Buffer(DstBuffDataStartByteOffset, this);
    VERIFY(DstBuffDataStartByteOffset == 0, "Dst buffer must not be suballocated");
    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyBufferRegion(pd3d12Buff, DstOffset + DstBuffDataStartByteOffset, Allocation.pBuffer, Allocation.Offset, NumBytes);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::UpdateBuffer(IBuffer*                       pBuffer,
                                          Uint64                         Offset,
                                          Uint64                         Size,
                                          const void*                    pData,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    // We must use cmd context from the device context provided, otherwise there will
    // be resource barrier issues in the cmd list in the device context
    auto* pBuffD3D12 = ClassPtrCast<BufferD3D12Impl>(pBuffer);
    VERIFY(pBuffD3D12->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers must be updated via Map()");
    constexpr size_t DefaultAlignment = 16;
    auto             TmpSpace         = m_DynamicHeap.Allocate(Size, DefaultAlignment, GetFrameNumber());
    memcpy(TmpSpace.CPUAddress, pData, StaticCast<size_t>(Size));
    UpdateBufferRegion(pBuffD3D12, TmpSpace, Offset, Size, StateTransitionMode);
}

void DeviceContextD3D12Impl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                        Uint64                         SrcOffset,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                        IBuffer*                       pDstBuffer,
                                        Uint64                         DstOffset,
                                        Uint64                         Size,
                                        RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBuffD3D12 = ClassPtrCast<BufferD3D12Impl>(pSrcBuffer);
    auto* pDstBuffD3D12 = ClassPtrCast<BufferD3D12Impl>(pDstBuffer);

    VERIFY(pDstBuffD3D12->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers cannot be copy destinations");

    auto& CmdCtx = GetCmdContext();
    TransitionOrVerifyBufferState(CmdCtx, *pSrcBuffD3D12, SrcBufferTransitionMode, RESOURCE_STATE_COPY_SOURCE, "Using resource as copy source (DeviceContextD3D12Impl::CopyBuffer)");
    TransitionOrVerifyBufferState(CmdCtx, *pDstBuffD3D12, DstBufferTransitionMode, RESOURCE_STATE_COPY_DEST, "Using resource as copy destination (DeviceContextD3D12Impl::CopyBuffer)");

    Uint64 DstDataStartByteOffset;
    auto*  pd3d12DstBuff = pDstBuffD3D12->GetD3D12Buffer(DstDataStartByteOffset, this);
    VERIFY(DstDataStartByteOffset == 0, "Dst buffer must not be suballocated");

    Uint64 SrcDataStartByteOffset;
    auto*  pd3d12SrcBuff = pSrcBuffD3D12->GetD3D12Buffer(SrcDataStartByteOffset, this);
    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyBufferRegion(pd3d12DstBuff, DstOffset + DstDataStartByteOffset, pd3d12SrcBuff, SrcOffset + SrcDataStartByteOffset, Size);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);
    auto*       pBufferD3D12   = ClassPtrCast<BufferD3D12Impl>(pBuffer);
    const auto& BuffDesc       = pBufferD3D12->GetDesc();
    auto*       pd3d12Resource = pBufferD3D12->m_pd3d12Resource.p;

    if (MapType == MAP_READ)
    {
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING, "Buffer must be created as USAGE_STAGING to be mapped for reading");
        DEV_CHECK_ERR(pd3d12Resource != nullptr, "USAGE_STAGING buffer must initialize D3D12 resource");

        if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
        {
            LOG_WARNING_MESSAGE("D3D12 backend never waits for GPU when mapping staging buffers for reading. "
                                "Applications must use fences or other synchronization methods to explicitly synchronize "
                                "access and use MAP_FLAG_DO_NOT_WAIT flag.");
        }

        D3D12_RANGE MapRange;
        MapRange.Begin = 0;
        MapRange.End   = StaticCast<SIZE_T>(BuffDesc.Size);
        pd3d12Resource->Map(0, &MapRange, &pMappedData);
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING)
        {
            DEV_CHECK_ERR(pd3d12Resource != nullptr, "USAGE_STAGING buffer mapped for writing must initialize D3D12 resource");
            if (MapFlags & MAP_FLAG_DISCARD)
            {
                // Nothing to do
            }
            pd3d12Resource->Map(0, nullptr, &pMappedData);
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0, "D3D12 buffer must be mapped for writing with MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flag");
            auto& DynamicData = pBufferD3D12->m_DynamicData[GetContextId()];
            if ((MapFlags & MAP_FLAG_DISCARD) != 0 || DynamicData.CPUAddress == nullptr)
            {
                Uint32 Alignment = (BuffDesc.BindFlags & BIND_UNIFORM_BUFFER) ? D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT : 16;
                DynamicData      = AllocateDynamicSpace(BuffDesc.Size, Alignment);
            }
            else
            {
                VERIFY_EXPR(MapFlags & MAP_FLAG_NO_OVERWRITE);

                if (pd3d12Resource != nullptr)
                {
                    LOG_ERROR("Formatted buffers require actual Direct3D12 backing resource and cannot be suballocated "
                              "from dynamic heap. In current implementation, the entire contents of the backing buffer is updated when the buffer is unmapped. "
                              "As a consequence, the buffer cannot be mapped with MAP_FLAG_NO_OVERWRITE flag because updating the whole "
                              "buffer will overwrite regions that may still be in use by the GPU.");
                    return;
                }

                // Reuse previously mapped region
            }
            pMappedData = DynamicData.CPUAddress;
        }
        else
        {
            LOG_ERROR("Only USAGE_DYNAMIC and USAGE_STAGING D3D12 buffers can be mapped for writing");
        }
    }
    else if (MapType == MAP_READ_WRITE)
    {
        LOG_ERROR("MAP_READ_WRITE is not supported in D3D12");
    }
    else
    {
        LOG_ERROR("Only MAP_WRITE_DISCARD and MAP_READ are currently implemented in D3D12");
    }
}

void DeviceContextD3D12Impl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto*       pBufferD3D12   = ClassPtrCast<BufferD3D12Impl>(pBuffer);
    const auto& BuffDesc       = pBufferD3D12->GetDesc();
    auto*       pd3d12Resource = pBufferD3D12->m_pd3d12Resource.p;
    if (MapType == MAP_READ)
    {
        D3D12_RANGE MapRange;
        // It is valid to specify the CPU didn't write any data by passing a range where End is less than or equal to Begin.
        MapRange.Begin = 1;
        MapRange.End   = 0;
        pd3d12Resource->Unmap(0, &MapRange);
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING)
        {
            VERIFY(pd3d12Resource != nullptr, "USAGE_STAGING buffer mapped for writing must initialize D3D12 resource");
            pd3d12Resource->Unmap(0, nullptr);
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            // Copy data into the resource
            if (pd3d12Resource)
            {
                UpdateBufferRegion(pBufferD3D12, pBufferD3D12->m_DynamicData[GetContextId()], 0, BuffDesc.Size, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
        }
    }
}

void DeviceContextD3D12Impl::UpdateTexture(ITexture*                      pTexture,
                                           Uint32                         MipLevel,
                                           Uint32                         Slice,
                                           const Box&                     DstBox,
                                           const TextureSubResData&       SubresData,
                                           RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferTransitionMode, TextureTransitionMode);

    auto*       pTexD3D12 = ClassPtrCast<TextureD3D12Impl>(pTexture);
    const auto& Desc      = pTexD3D12->GetDesc();
    // OpenGL backend uses UpdateData() to initialize textures, so we can't check the usage in ValidateUpdateTextureParams()
    DEV_CHECK_ERR(Desc.Usage == USAGE_DEFAULT || Desc.Usage == USAGE_SPARSE,
                  "Only USAGE_DEFAULT or USAGE_SPARSE textures should be updated with UpdateData()");

    Box         BlockAlignedBox;
    const auto& FmtAttribs = GetTextureFormatAttribs(Desc.Format);
    const Box*  pBox       = nullptr;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region by the compressed block size

        VERIFY((DstBox.MinX % FmtAttribs.BlockWidth) == 0, "Update region min X coordinate (", DstBox.MinX, ") must be multiple of a compressed block width (", Uint32{FmtAttribs.BlockWidth}, ")");
        BlockAlignedBox.MinX = DstBox.MinX;
        VERIFY((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0, "Compressed block width (", Uint32{FmtAttribs.BlockWidth}, ") is expected to be power of 2");
        BlockAlignedBox.MaxX = (DstBox.MaxX + FmtAttribs.BlockWidth - 1) & ~(FmtAttribs.BlockWidth - 1);

        VERIFY((DstBox.MinY % FmtAttribs.BlockHeight) == 0, "Update region min Y coordinate (", DstBox.MinY, ") must be multiple of a compressed block height (", Uint32{FmtAttribs.BlockHeight}, ")");
        BlockAlignedBox.MinY = DstBox.MinY;
        VERIFY((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0, "Compressed block height (", Uint32{FmtAttribs.BlockHeight}, ") is expected to be power of 2");
        BlockAlignedBox.MaxY = (DstBox.MaxY + FmtAttribs.BlockHeight - 1) & ~(FmtAttribs.BlockHeight - 1);

        BlockAlignedBox.MinZ = DstBox.MinZ;
        BlockAlignedBox.MaxZ = DstBox.MaxZ;

        pBox = &BlockAlignedBox;
    }
    else
    {
        pBox = &DstBox;
    }
    auto DstSubResIndex = D3D12CalcSubresource(MipLevel, Slice, 0, Desc.MipLevels, Desc.GetArraySize());
    if (SubresData.pSrcBuffer == nullptr)
    {
        UpdateTextureRegion(SubresData.pData, SubresData.Stride, SubresData.DepthStride,
                            *pTexD3D12, DstSubResIndex, *pBox, TextureTransitionMode);
    }
    else
    {
        CopyTextureRegion(SubresData.pSrcBuffer, 0, SubresData.Stride, SubresData.DepthStride,
                          *pTexD3D12, DstSubResIndex, *pBox,
                          SrcBufferTransitionMode, TextureTransitionMode);
    }
}

void DeviceContextD3D12Impl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);

    auto* pSrcTexD3D12 = ClassPtrCast<TextureD3D12Impl>(CopyAttribs.pSrcTexture);
    auto* pDstTexD3D12 = ClassPtrCast<TextureD3D12Impl>(CopyAttribs.pDstTexture);

    const auto& SrcTexDesc = pSrcTexD3D12->GetDesc();
    const auto& DstTexDesc = pDstTexD3D12->GetDesc();

    D3D12_BOX D3D12SrcBox, *pD3D12SrcBox = nullptr;
    if (const auto* pSrcBox = CopyAttribs.pSrcBox)
    {
        D3D12SrcBox.left   = pSrcBox->MinX;
        D3D12SrcBox.right  = pSrcBox->MaxX;
        D3D12SrcBox.top    = pSrcBox->MinY;
        D3D12SrcBox.bottom = pSrcBox->MaxY;
        D3D12SrcBox.front  = pSrcBox->MinZ;
        D3D12SrcBox.back   = pSrcBox->MaxZ;
        pD3D12SrcBox       = &D3D12SrcBox;
    }

    auto DstSubResIndex = D3D12CalcSubresource(CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, 0, DstTexDesc.MipLevels, DstTexDesc.GetArraySize());
    auto SrcSubResIndex = D3D12CalcSubresource(CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, 0, SrcTexDesc.MipLevels, SrcTexDesc.GetArraySize());
    CopyTextureRegion(pSrcTexD3D12, SrcSubResIndex, pD3D12SrcBox, CopyAttribs.SrcTextureTransitionMode,
                      pDstTexD3D12, DstSubResIndex, CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ,
                      CopyAttribs.DstTextureTransitionMode);
}

void DeviceContextD3D12Impl::CopyTextureRegion(TextureD3D12Impl*              pSrcTexture,
                                               Uint32                         SrcSubResIndex,
                                               const D3D12_BOX*               pD3D12SrcBox,
                                               RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                                               TextureD3D12Impl*              pDstTexture,
                                               Uint32                         DstSubResIndex,
                                               Uint32                         DstX,
                                               Uint32                         DstY,
                                               Uint32                         DstZ,
                                               RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode)
{
    // We must unbind the textures from framebuffer because
    // we will transition their states. If we later try to commit
    // them as render targets (e.g. from SetPipelineState()), a
    // state mismatch error will occur.
    UnbindTextureFromFramebuffer(pSrcTexture, true);
    UnbindTextureFromFramebuffer(pDstTexture, true);

    auto& CmdCtx = GetCmdContext();
    if (pSrcTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DEV_CHECK_ERR((pSrcTexture->GetDesc().CPUAccessFlags & CPU_ACCESS_WRITE) != 0, "Source staging texture must be created with CPU_ACCESS_WRITE flag");
        DEV_CHECK_ERR(pSrcTexture->GetState() == RESOURCE_STATE_GENERIC_READ || !pSrcTexture->IsInKnownState(), "Staging texture must always be in RESOURCE_STATE_GENERIC_READ state");
    }
    TransitionOrVerifyTextureState(CmdCtx, *pSrcTexture, SrcTextureTransitionMode, RESOURCE_STATE_COPY_SOURCE, "Using resource as copy source (DeviceContextD3D12Impl::CopyTextureRegion)");

    if (pDstTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DEV_CHECK_ERR((pDstTexture->GetDesc().CPUAccessFlags & CPU_ACCESS_READ) != 0, "Destination staging texture must be created with CPU_ACCESS_READ flag");
        DEV_CHECK_ERR(pDstTexture->GetState() == RESOURCE_STATE_COPY_DEST || !pDstTexture->IsInKnownState(), "Staging texture must always be in RESOURCE_STATE_COPY_DEST state");
    }
    TransitionOrVerifyTextureState(CmdCtx, *pDstTexture, DstTextureTransitionMode, RESOURCE_STATE_COPY_DEST, "Using resource as copy destination (DeviceContextD3D12Impl::CopyTextureRegion)");

    D3D12_TEXTURE_COPY_LOCATION DstLocation = {}, SrcLocation = {};

    SrcLocation.pResource = pSrcTexture->GetD3D12Resource();
    if (pSrcTexture->GetDesc().Usage == USAGE_STAGING)
    {
        SrcLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SrcLocation.PlacedFootprint = pSrcTexture->GetStagingFootprint(SrcSubResIndex);
    }
    else
    {
        SrcLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        SrcLocation.SubresourceIndex = SrcSubResIndex;
    }

    DstLocation.pResource = pDstTexture->GetD3D12Resource();
    if (pDstTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DstLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        DstLocation.PlacedFootprint = pDstTexture->GetStagingFootprint(DstSubResIndex);
    }
    else
    {
        DstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DstLocation.SubresourceIndex = DstSubResIndex;
    }

    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyTextureRegion(&DstLocation, DstX, DstY, DstZ, &SrcLocation, pD3D12SrcBox);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::CopyTextureRegion(ID3D12Resource*                pd3d12Buffer,
                                               Uint64                         SrcOffset,
                                               Uint64                         SrcStride,
                                               Uint64                         SrcDepthStride,
                                               Uint64                         BufferSize,
                                               TextureD3D12Impl&              TextureD3D12,
                                               Uint32                         DstSubResIndex,
                                               const Box&                     DstBox,
                                               RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    const auto& TexDesc = TextureD3D12.GetDesc();
    auto&       CmdCtx  = GetCmdContext();

    bool StateTransitionRequired = false;
    if (TextureTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        StateTransitionRequired = TextureD3D12.IsInKnownState() && !TextureD3D12.CheckState(RESOURCE_STATE_COPY_DEST);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TextureTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTextureState(TextureD3D12, RESOURCE_STATE_COPY_DEST, "Using texture as copy destination (DeviceContextD3D12Impl::CopyTextureRegion)");
    }
#endif
    D3D12_RESOURCE_BARRIER BarrierDesc;
    if (StateTransitionRequired)
    {
        const auto ResStateMask = GetSupportedD3D12ResourceStatesForCommandList(CmdCtx.GetCommandListType());

        BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource   = TextureD3D12.GetD3D12Resource();
        BarrierDesc.Transition.Subresource = DstSubResIndex;
        BarrierDesc.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(TextureD3D12.GetState()) & ResStateMask;
        BarrierDesc.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        BarrierDesc.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        CmdCtx.ResourceBarrier(BarrierDesc);
    }

    D3D12_TEXTURE_COPY_LOCATION DstLocation;
    DstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DstLocation.pResource        = TextureD3D12.GetD3D12Resource();
    DstLocation.SubresourceIndex = StaticCast<UINT>(DstSubResIndex);

    D3D12_TEXTURE_COPY_LOCATION SrcLocation;
    SrcLocation.Type                              = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SrcLocation.pResource                         = pd3d12Buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Footprint = SrcLocation.PlacedFootprint;
    Footprint.Offset                              = SrcOffset;
    Footprint.Footprint.Width                     = StaticCast<UINT>(DstBox.Width());
    Footprint.Footprint.Height                    = StaticCast<UINT>(DstBox.Height());
    Footprint.Footprint.Depth                     = StaticCast<UINT>(DstBox.Depth()); // Depth cannot be 0
    Footprint.Footprint.Format                    = TexFormatToDXGI_Format(TexDesc.Format);

    Footprint.Footprint.RowPitch = StaticCast<UINT>(SrcStride);

#ifdef DILIGENT_DEBUG
    {
        const auto&  FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
        const Uint32 RowCount   = std::max((Footprint.Footprint.Height / FmtAttribs.BlockHeight), 1u);
        VERIFY(BufferSize >= Uint64{Footprint.Footprint.RowPitch} * RowCount * Footprint.Footprint.Depth, "Buffer is not large enough");
        VERIFY(Footprint.Footprint.Depth == 1 || StaticCast<UINT>(SrcDepthStride) == Footprint.Footprint.RowPitch * RowCount, "Depth stride must be equal to the size of 2D plane");
    }
#endif

    D3D12_BOX D3D12SrcBox;
    D3D12SrcBox.left   = 0;
    D3D12SrcBox.right  = Footprint.Footprint.Width;
    D3D12SrcBox.top    = 0;
    D3D12SrcBox.bottom = Footprint.Footprint.Height;
    D3D12SrcBox.front  = 0;
    D3D12SrcBox.back   = Footprint.Footprint.Depth;
    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyTextureRegion(&DstLocation,
                                               StaticCast<UINT>(DstBox.MinX),
                                               StaticCast<UINT>(DstBox.MinY),
                                               StaticCast<UINT>(DstBox.MinZ),
                                               &SrcLocation, &D3D12SrcBox);

    ++m_State.NumCommands;

    if (StateTransitionRequired)
    {
        std::swap(BarrierDesc.Transition.StateBefore, BarrierDesc.Transition.StateAfter);
        CmdCtx.ResourceBarrier(BarrierDesc);
    }
}

void DeviceContextD3D12Impl::CopyTextureRegion(IBuffer*                       pSrcBuffer,
                                               Uint64                         SrcOffset,
                                               Uint64                         SrcStride,
                                               Uint64                         SrcDepthStride,
                                               class TextureD3D12Impl&        TextureD3D12,
                                               Uint32                         DstSubResIndex,
                                               const Box&                     DstBox,
                                               RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode,
                                               RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    auto* pBufferD3D12 = ClassPtrCast<BufferD3D12Impl>(pSrcBuffer);
    if (pBufferD3D12->GetDesc().Usage == USAGE_DYNAMIC)
        DEV_CHECK_ERR(pBufferD3D12->GetState() == RESOURCE_STATE_GENERIC_READ, "Dynamic buffer is expected to always be in RESOURCE_STATE_GENERIC_READ state");
    else
    {
        if (BufferTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        {
            if (pBufferD3D12->IsInKnownState() && pBufferD3D12->GetState() != RESOURCE_STATE_GENERIC_READ)
                GetCmdContext().TransitionResource(*pBufferD3D12, RESOURCE_STATE_GENERIC_READ);
        }
#ifdef DILIGENT_DEVELOPMENT
        else if (BufferTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
        {
            DvpVerifyBufferState(*pBufferD3D12, RESOURCE_STATE_COPY_SOURCE, "Using buffer as copy source (DeviceContextD3D12Impl::CopyTextureRegion)");
        }
#endif
    }
    GetCmdContext().FlushResourceBarriers();
    Uint64 DataStartByteOffset = 0;
    auto*  pd3d12Buffer        = pBufferD3D12->GetD3D12Buffer(DataStartByteOffset, this);
    CopyTextureRegion(pd3d12Buffer, StaticCast<Uint32>(DataStartByteOffset) + SrcOffset, SrcStride, SrcDepthStride,
                      pBufferD3D12->GetDesc().Size, TextureD3D12, DstSubResIndex, DstBox, TextureTransitionMode);
}

DeviceContextD3D12Impl::TextureUploadSpace DeviceContextD3D12Impl::AllocateTextureUploadSpace(TEXTURE_FORMAT TexFmt,
                                                                                              const Box&     Region)
{
    TextureUploadSpace UploadSpace;
    VERIFY_EXPR(Region.IsValid());
    auto UpdateRegionWidth  = Region.Width();
    auto UpdateRegionHeight = Region.Height();
    auto UpdateRegionDepth  = Region.Depth();

    const auto& FmtAttribs = GetTextureFormatAttribs(TexFmt);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Box must be aligned by the calling function
        VERIFY_EXPR((UpdateRegionWidth % FmtAttribs.BlockWidth) == 0);
        VERIFY_EXPR((UpdateRegionHeight % FmtAttribs.BlockHeight) == 0);
        UploadSpace.RowSize  = Uint64{UpdateRegionWidth} / Uint64{FmtAttribs.BlockWidth} * Uint64{FmtAttribs.ComponentSize};
        UploadSpace.RowCount = UpdateRegionHeight / FmtAttribs.BlockHeight;
    }
    else
    {
        UploadSpace.RowSize  = Uint64{UpdateRegionWidth} * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        UploadSpace.RowCount = UpdateRegionHeight;
    }
    // RowPitch must be a multiple of 256 (aka. D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
    UploadSpace.Stride        = (UploadSpace.RowSize + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UploadSpace.DepthStride   = UploadSpace.RowCount * UploadSpace.Stride;
    const auto MemorySize     = UpdateRegionDepth * UploadSpace.DepthStride;
    UploadSpace.Allocation    = AllocateDynamicSpace(MemorySize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    UploadSpace.AlignedOffset = (UploadSpace.Allocation.Offset + (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
    UploadSpace.Region        = Region;

    return UploadSpace;
}

void DeviceContextD3D12Impl::UpdateTextureRegion(const void*                    pSrcData,
                                                 Uint64                         SrcStride,
                                                 Uint64                         SrcDepthStride,
                                                 TextureD3D12Impl&              TextureD3D12,
                                                 Uint32                         DstSubResIndex,
                                                 const Box&                     DstBox,
                                                 RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    const auto& TexDesc           = TextureD3D12.GetDesc();
    auto        UploadSpace       = AllocateTextureUploadSpace(TexDesc.Format, DstBox);
    auto        UpdateRegionDepth = DstBox.Depth();
#ifdef DILIGENT_DEBUG
    {
        VERIFY(SrcStride >= UploadSpace.RowSize, "Source data stride (", SrcStride, ") is below the image row size (", UploadSpace.RowSize, ")");
        const auto PlaneSize = SrcStride * UploadSpace.RowCount;
        VERIFY(UpdateRegionDepth == 1 || SrcDepthStride >= PlaneSize, "Source data depth stride (", SrcDepthStride, ") is below the image plane size (", PlaneSize, ")");
    }
#endif
    const auto AlignedOffset = UploadSpace.AlignedOffset;

    for (Uint32 DepthSlice = 0; DepthSlice < UpdateRegionDepth; ++DepthSlice)
    {
        for (Uint32 row = 0; row < UploadSpace.RowCount; ++row)
        {
            const auto* pSrcPtr =
                reinterpret_cast<const Uint8*>(pSrcData) + row * SrcStride + DepthSlice * SrcDepthStride;
            auto* pDstPtr =
                reinterpret_cast<Uint8*>(UploadSpace.Allocation.CPUAddress) + (AlignedOffset - UploadSpace.Allocation.Offset) + row * UploadSpace.Stride + DepthSlice * UploadSpace.DepthStride;

            memcpy(pDstPtr, pSrcPtr, StaticCast<size_t>(UploadSpace.RowSize));
        }
    }
    CopyTextureRegion(UploadSpace.Allocation.pBuffer,
                      StaticCast<Uint32>(AlignedOffset),
                      UploadSpace.Stride,
                      UploadSpace.DepthStride,
                      StaticCast<Uint32>(UploadSpace.Allocation.Size - (AlignedOffset - UploadSpace.Allocation.Offset)),
                      TextureD3D12,
                      DstSubResIndex,
                      DstBox,
                      TextureTransitionMode);
}

void DeviceContextD3D12Impl::MapTextureSubresource(ITexture*                 pTexture,
                                                   Uint32                    MipLevel,
                                                   Uint32                    ArraySlice,
                                                   MAP_TYPE                  MapType,
                                                   MAP_FLAGS                 MapFlags,
                                                   const Box*                pMapRegion,
                                                   MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);

    auto&       TextureD3D12 = *ClassPtrCast<TextureD3D12Impl>(pTexture);
    const auto& TexDesc      = TextureD3D12.GetDesc();
    auto        Subres       = D3D12CalcSubresource(MipLevel, ArraySlice, 0, TexDesc.MipLevels, TexDesc.GetArraySize());
    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        if (MapType != MAP_WRITE)
        {
            LOG_ERROR("USAGE_DYNAMIC textures can only be mapped for writing");
            MappedData = MappedTextureSubresource{};
            return;
        }

        if ((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0)
            LOG_INFO_MESSAGE_ONCE("Mapping textures with flags MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE has no effect in D3D12 backend");

        Box FullExtentBox;
        if (pMapRegion == nullptr)
        {
            FullExtentBox.MaxX = std::max(TexDesc.Width >> MipLevel, 1u);
            FullExtentBox.MaxY = std::max(TexDesc.Height >> MipLevel, 1u);
            FullExtentBox.MaxZ = std::max(TexDesc.GetDepth() >> MipLevel, 1u);
            pMapRegion         = &FullExtentBox;
        }

        auto UploadSpace       = AllocateTextureUploadSpace(TexDesc.Format, *pMapRegion);
        MappedData.pData       = reinterpret_cast<Uint8*>(UploadSpace.Allocation.CPUAddress) + (UploadSpace.AlignedOffset - UploadSpace.Allocation.Offset);
        MappedData.Stride      = UploadSpace.Stride;
        MappedData.DepthStride = UploadSpace.DepthStride;

        auto it = m_MappedTextures.emplace(MappedTextureKey{&TextureD3D12, Subres}, std::move(UploadSpace));
        if (!it.second)
            LOG_ERROR_MESSAGE("Mip level ", MipLevel, ", slice ", ArraySlice, " of texture '", TexDesc.Name, "' has already been mapped");
    }
    else if (TexDesc.Usage == USAGE_STAGING)
    {
        const auto& Footprint = TextureD3D12.GetStagingFootprint(Subres);

        // It is valid to specify the CPU won't read any data by passing a range where
        // End is less than or equal to Begin.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        D3D12_RANGE InvalidateRange = {1, 0};
        if (MapType == MAP_READ)
        {
            if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
            {
                LOG_WARNING_MESSAGE("D3D12 backend never waits for GPU when mapping staging textures for reading. "
                                    "Applications must use fences or other synchronization methods to explicitly synchronize "
                                    "access and use MAP_FLAG_DO_NOT_WAIT flag.");
            }

            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_READ), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_READ flag and can't be mapped for reading");
            // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
            InvalidateRange.Begin     = StaticCast<SIZE_T>(Footprint.Offset);
            const auto& NextFootprint = TextureD3D12.GetStagingFootprint(Subres + 1);
            InvalidateRange.End       = StaticCast<SIZE_T>(NextFootprint.Offset);
        }
        else if (MapType == MAP_WRITE)
        {
            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_WRITE flag and can't be mapped for writing");
        }

        // Nested Map() calls are supported and are ref-counted. The first call to Map() allocates
        // a CPU virtual address range for the resource. The last call to Unmap deallocates the CPU
        // virtual address range.

        // Map() invalidates the CPU cache, when necessary, so that CPU reads to this address
        // reflect any modifications made by the GPU.
        void* pMappedDataPtr = nullptr;
        TextureD3D12.GetD3D12Resource()->Map(0, &InvalidateRange, &pMappedDataPtr);
        MappedData.pData       = reinterpret_cast<Uint8*>(pMappedDataPtr) + Footprint.Offset;
        MappedData.Stride      = StaticCast<Uint32>(Footprint.Footprint.RowPitch);
        const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
        MappedData.DepthStride = StaticCast<Uint32>(Footprint.Footprint.Height / FmtAttribs.BlockHeight * Footprint.Footprint.RowPitch);
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in D3D12 back-end");
    }
}

void DeviceContextD3D12Impl::UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);

    TextureD3D12Impl& TextureD3D12 = *ClassPtrCast<TextureD3D12Impl>(pTexture);
    const auto&       TexDesc      = TextureD3D12.GetDesc();
    auto              Subres       = D3D12CalcSubresource(MipLevel, ArraySlice, 0, TexDesc.MipLevels, TexDesc.GetArraySize());
    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        auto UploadSpaceIt = m_MappedTextures.find(MappedTextureKey{&TextureD3D12, Subres});
        if (UploadSpaceIt != m_MappedTextures.end())
        {
            auto& UploadSpace = UploadSpaceIt->second;
            CopyTextureRegion(UploadSpace.Allocation.pBuffer,
                              UploadSpace.AlignedOffset,
                              UploadSpace.Stride,
                              UploadSpace.DepthStride,
                              StaticCast<Uint32>(UploadSpace.Allocation.Size - (UploadSpace.AlignedOffset - UploadSpace.Allocation.Offset)),
                              TextureD3D12,
                              Subres,
                              UploadSpace.Region,
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
        // It is valid to specify the CPU didn't write any data by passing a range where
        // End is less than or equal to Begin.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-unmap
        D3D12_RANGE FlushRange = {1, 0};

        if (TexDesc.CPUAccessFlags == CPU_ACCESS_WRITE)
        {
            const auto& Footprint     = TextureD3D12.GetStagingFootprint(Subres);
            const auto& NextFootprint = TextureD3D12.GetStagingFootprint(Subres + 1);
            FlushRange.Begin          = StaticCast<SIZE_T>(Footprint.Offset);
            FlushRange.End            = StaticCast<SIZE_T>(NextFootprint.Offset);
        }

        // Map and Unmap can be called by multiple threads safely. Nested Map calls are supported
        // and are ref-counted. The first call to Map allocates a CPU virtual address range for the
        // resource. The last call to Unmap deallocates the CPU virtual address range.

        // Unmap() flushes the CPU cache, when necessary, so that GPU reads to this address reflect
        // any modifications made by the CPU.
        TextureD3D12.GetD3D12Resource()->Unmap(0, &FlushRange);
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in D3D12 back-end");
    }
}


void DeviceContextD3D12Impl::GenerateMips(ITextureView* pTexView)
{
    TDeviceContextBase::GenerateMips(pTexView);

    auto& Ctx = GetCmdContext();

    const auto& MipsGenerator = m_pDevice->GetMipsGenerator();
    MipsGenerator.GenerateMips(m_pDevice->GetD3D12Device(), ClassPtrCast<TextureViewD3D12Impl>(pTexView), Ctx);
    ++m_State.NumCommands;

    // Invalidate compute resources as they were set by the mips generator
    m_ComputeResources.MakeAllStale();

    if (m_pPipelineState)
    {
        // Restore previous PSO and root signature
        const auto& PSODesc = m_pPipelineState->GetDesc();
        static_assert(PIPELINE_TYPE_LAST == 4, "Please update the switch below to handle the new pipeline type");
        switch (PSODesc.PipelineType)
        {
            case PIPELINE_TYPE_GRAPHICS:
            {
                Ctx.SetPipelineState(m_pPipelineState->GetD3D12PipelineState());
                // No need to restore graphics signature as it is not changed
            }
            break;

            case PIPELINE_TYPE_COMPUTE:
            {
                auto& CompCtx = Ctx.AsComputeContext();
                CompCtx.SetPipelineState(m_pPipelineState->GetD3D12PipelineState());
                CompCtx.SetComputeRootSignature(m_ComputeResources.pd3d12RootSig);
            }
            break;

            case PIPELINE_TYPE_RAY_TRACING:
            {
                auto& RTCtx = Ctx.AsGraphicsContext4();
                RTCtx.SetRayTracingPipelineState(m_pPipelineState->GetD3D12StateObject());
                RTCtx.SetComputeRootSignature(m_ComputeResources.pd3d12RootSig);
            }
            break;

            case PIPELINE_TYPE_TILE:
                UNEXPECTED("Unsupported pipeline type");
                break;
            default:
                UNEXPECTED("Unknown pipeline type");
        }
    }
}

void DeviceContextD3D12Impl::FinishCommandList(ICommandList** ppCommandList)
{
    DEV_CHECK_ERR(IsDeferred(), "Only deferred context can record command list");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Finishing command list inside an active render pass.");

    CommandListD3D12Impl* pCmdListD3D12(NEW_RC_OBJ(m_CmdListAllocator, "CommandListD3D12Impl instance", CommandListD3D12Impl)(m_pDevice, this, std::move(m_CurrCmdCtx)));
    pCmdListD3D12->QueryInterface(IID_CommandList, reinterpret_cast<IObject**>(ppCommandList));

    // We can't request new cmd context because we don't know the command queue type
    constexpr auto RequestNewCmdCtx = false;
    Flush(RequestNewCmdCtx);

    m_QueryMgr = nullptr;
    InvalidateState();

    TDeviceContextBase::FinishCommandList();
}

void DeviceContextD3D12Impl::ExecuteCommandLists(Uint32               NumCommandLists,
                                                 ICommandList* const* ppCommandLists)
{
    DEV_CHECK_ERR(!IsDeferred(), "Only immediate context can execute command list");

    if (NumCommandLists == 0)
        return;
    DEV_CHECK_ERR(ppCommandLists != nullptr, "ppCommandLists must not be null when NumCommandLists is not zero");

    Flush(true, NumCommandLists, ppCommandLists);

    InvalidateState();
}

void DeviceContextD3D12Impl::EnqueueSignal(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::EnqueueSignal(pFence, Value, 0);
    m_SignalFences.emplace_back(Value, pFence);
}

void DeviceContextD3D12Impl::DeviceWaitForFence(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::DeviceWaitForFence(pFence, Value, 0);
    m_WaitFences.emplace_back(Value, pFence);
}

void DeviceContextD3D12Impl::WaitForIdle()
{
    DEV_CHECK_ERR(!IsDeferred(), "Only immediate contexts can be idled");
    Flush();
    m_pDevice->IdleCommandQueue(GetCommandQueueId(), true);
}

void DeviceContextD3D12Impl::BeginQuery(IQuery* pQuery)
{
    TDeviceContextBase::BeginQuery(pQuery, 0);

    auto*      pQueryD3D12Impl = ClassPtrCast<QueryD3D12Impl>(pQuery);
    const auto QueryType       = pQueryD3D12Impl->GetDesc().Type;
    if (QueryType != QUERY_TYPE_TIMESTAMP)
        ++m_ActiveQueriesCounter;

    auto& QueryMgr = GetQueryManager();
    auto& Ctx      = GetCmdContext();
    auto  Idx      = pQueryD3D12Impl->GetQueryHeapIndex(0);
    if (QueryType != QUERY_TYPE_DURATION)
        QueryMgr.BeginQuery(Ctx, QueryType, Idx);
    else
        QueryMgr.EndQuery(Ctx, QueryType, Idx);
}

void DeviceContextD3D12Impl::EndQuery(IQuery* pQuery)
{
    TDeviceContextBase::EndQuery(pQuery, 0);

    auto*      pQueryD3D12Impl = ClassPtrCast<QueryD3D12Impl>(pQuery);
    const auto QueryType       = pQueryD3D12Impl->GetDesc().Type;
    if (QueryType != QUERY_TYPE_TIMESTAMP)
    {
        VERIFY(m_ActiveQueriesCounter > 0, "Active query counter is 0 which means there was a mismatch between BeginQuery() / EndQuery() calls");
        --m_ActiveQueriesCounter;
    }

    auto& QueryMgr = GetQueryManager();
    auto& Ctx      = GetCmdContext();
    auto  Idx      = pQueryD3D12Impl->GetQueryHeapIndex(QueryType == QUERY_TYPE_DURATION ? 1 : 0);
    QueryMgr.EndQuery(Ctx, QueryType, Idx);
}

static void AliasingBarrier(CommandContext& CmdCtx, IDeviceObject* pResourceBefore, IDeviceObject* pResourceAfter)
{
    bool UseNVApi         = false;
    auto GetD3D12Resource = [&UseNVApi](IDeviceObject* pResource) -> ID3D12Resource* //
    {
        if (RefCntAutoPtr<ITextureD3D12> pTexture{pResource, IID_TextureD3D12})
        {
            const auto* pTexD3D12 = pTexture.ConstPtr<TextureD3D12Impl>();
            if (pTexD3D12->IsUsingNVApi())
                UseNVApi = true;
            return pTexD3D12->GetD3D12Texture();
        }
        else if (RefCntAutoPtr<IBufferD3D12> pBuffer{pResource, IID_BufferD3D12})
        {
            return pBuffer.RawPtr<BufferD3D12Impl>()->GetD3D12Resource();
        }
        else
        {
            return nullptr;
        }
    };

    D3D12_RESOURCE_BARRIER Barrier{};
    Barrier.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    Barrier.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Aliasing.pResourceBefore = GetD3D12Resource(pResourceBefore);
    Barrier.Aliasing.pResourceAfter  = GetD3D12Resource(pResourceAfter);

#ifdef DILIGENT_ENABLE_D3D_NVAPI
    if (UseNVApi)
    {
        NvAPI_D3D12_ResourceAliasingBarrier(CmdCtx.GetCommandList(), 1, &Barrier);
    }
    else
#endif
    {
        VERIFY_EXPR(!UseNVApi);
        CmdCtx.ResourceBarrier(Barrier);
    }
}

void DeviceContextD3D12Impl::TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");

    auto& CmdCtx = GetCmdContext();
    for (Uint32 i = 0; i < BarrierCount; ++i)
    {
#ifdef DILIGENT_DEVELOPMENT
        DvpVerifyStateTransitionDesc(pResourceBarriers[i]);
#endif
        const auto& Barrier = pResourceBarriers[i];
        if (Barrier.Flags & STATE_TRANSITION_FLAG_ALIASING)
        {
            AliasingBarrier(CmdCtx, Barrier.pResourceBefore, Barrier.pResource);
        }
        else
        {
            if (RefCntAutoPtr<TextureD3D12Impl> pTextureD3D12Impl{Barrier.pResource, IID_TextureD3D12})
                CmdCtx.TransitionResource(*pTextureD3D12Impl, Barrier);
            else if (RefCntAutoPtr<BufferD3D12Impl> pBufferD3D12Impl{Barrier.pResource, IID_BufferD3D12})
                CmdCtx.TransitionResource(*pBufferD3D12Impl, Barrier);
            else if (RefCntAutoPtr<BottomLevelASD3D12Impl> pBLASD3D12Impl{Barrier.pResource, IID_BottomLevelASD3D12})
                CmdCtx.TransitionResource(*pBLASD3D12Impl, Barrier);
            else if (RefCntAutoPtr<TopLevelASD3D12Impl> pTLASD3D12Impl{Barrier.pResource, IID_TopLevelASD3D12})
                CmdCtx.TransitionResource(*pTLASD3D12Impl, Barrier);
            else
                UNEXPECTED("Unknown resource type");
        }
    }
}

void DeviceContextD3D12Impl::TransitionOrVerifyBufferState(CommandContext&                CmdCtx,
                                                           BufferD3D12Impl&               Buffer,
                                                           RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                           RESOURCE_STATE                 RequiredState,
                                                           const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (Buffer.IsInKnownState())
            CmdCtx.TransitionResource(Buffer, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyBufferState(Buffer, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionOrVerifyTextureState(CommandContext&                CmdCtx,
                                                            TextureD3D12Impl&              Texture,
                                                            RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                            RESOURCE_STATE                 RequiredState,
                                                            const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (Texture.IsInKnownState())
            CmdCtx.TransitionResource(Texture, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTextureState(Texture, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionOrVerifyBLASState(CommandContext&                CmdCtx,
                                                         BottomLevelASD3D12Impl&        BLAS,
                                                         RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                         RESOURCE_STATE                 RequiredState,
                                                         const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (BLAS.IsInKnownState())
            CmdCtx.TransitionResource(BLAS, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyBLASState(BLAS, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionOrVerifyTLASState(CommandContext&                CmdCtx,
                                                         TopLevelASD3D12Impl&           TLAS,
                                                         RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                         RESOURCE_STATE                 RequiredState,
                                                         const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (TLAS.IsInKnownState())
            CmdCtx.TransitionResource(TLAS, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTLASState(TLAS, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionTextureState(ITexture* pTexture, D3D12_RESOURCE_STATES State)
{
    DEV_CHECK_ERR(pTexture != nullptr, "pTexture must not be null");
    DEV_CHECK_ERR(pTexture->GetState() != RESOURCE_STATE_UNKNOWN, "Texture state is unknown");
    auto& CmdCtx = GetCmdContext();
    CmdCtx.TransitionResource(*ClassPtrCast<TextureD3D12Impl>(pTexture), D3D12ResourceStatesToResourceStateFlags(State));
}

void DeviceContextD3D12Impl::TransitionBufferState(IBuffer* pBuffer, D3D12_RESOURCE_STATES State)
{
    DEV_CHECK_ERR(pBuffer != nullptr, "pBuffer must not be null");
    DEV_CHECK_ERR(pBuffer->GetState() != RESOURCE_STATE_UNKNOWN, "Buffer state is unknown");
    auto& CmdCtx = GetCmdContext();
    CmdCtx.TransitionResource(*ClassPtrCast<BufferD3D12Impl>(pBuffer), D3D12ResourceStatesToResourceStateFlags(State));
}

ID3D12GraphicsCommandList* DeviceContextD3D12Impl::GetD3D12CommandList()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.FlushResourceBarriers();
    return CmdCtx.GetCommandList();
}

void DeviceContextD3D12Impl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                       ITexture*                               pDstTexture,
                                                       const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);

    auto*       pSrcTexD3D12 = ClassPtrCast<TextureD3D12Impl>(pSrcTexture);
    auto*       pDstTexD3D12 = ClassPtrCast<TextureD3D12Impl>(pDstTexture);
    const auto& SrcTexDesc   = pSrcTexD3D12->GetDesc();
    const auto& DstTexDesc   = pDstTexD3D12->GetDesc();

    auto& CmdCtx = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pSrcTexD3D12, ResolveAttribs.SrcTextureTransitionMode, RESOURCE_STATE_RESOLVE_SOURCE, "Resolving multi-sampled texture (DeviceContextD3D12Impl::ResolveTextureSubresource)");
    TransitionOrVerifyTextureState(CmdCtx, *pDstTexD3D12, ResolveAttribs.DstTextureTransitionMode, RESOURCE_STATE_RESOLVE_DEST, "Resolving multi-sampled texture (DeviceContextD3D12Impl::ResolveTextureSubresource)");

    auto Format = ResolveAttribs.Format;
    if (Format == TEX_FORMAT_UNKNOWN)
    {
        const auto& SrcFmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
        if (!SrcFmtAttribs.IsTypeless)
        {
            Format = SrcTexDesc.Format;
        }
        else
        {
            const auto& DstFmtAttribs = GetTextureFormatAttribs(DstTexDesc.Format);
            DEV_CHECK_ERR(!DstFmtAttribs.IsTypeless, "Resolve operation format can't be typeless when both source and destination textures are typeless");
            Format = DstFmtAttribs.Format;
        }
    }

    auto DXGIFmt        = TexFormatToDXGI_Format(Format);
    auto SrcSubresIndex = D3D12CalcSubresource(ResolveAttribs.SrcMipLevel, ResolveAttribs.SrcSlice, 0, SrcTexDesc.MipLevels, SrcTexDesc.GetArraySize());
    auto DstSubresIndex = D3D12CalcSubresource(ResolveAttribs.DstMipLevel, ResolveAttribs.DstSlice, 0, DstTexDesc.MipLevels, DstTexDesc.GetArraySize());

    CmdCtx.ResolveSubresource(pDstTexD3D12->GetD3D12Resource(), DstSubresIndex, pSrcTexD3D12->GetD3D12Resource(), SrcSubresIndex, DXGIFmt);
}

void DeviceContextD3D12Impl::BuildBLAS(const BuildBLASAttribs& Attribs)
{
    TDeviceContextBase::BuildBLAS(Attribs, 0);

    auto* const pBLASD3D12    = ClassPtrCast<BottomLevelASD3D12Impl>(Attribs.pBLAS);
    auto* const pScratchD3D12 = ClassPtrCast<BufferD3D12Impl>(Attribs.pScratchBuffer);
    const auto& BLASDesc      = pBLASD3D12->GetDesc();

    auto&       CmdCtx = GetCmdContext();
    const char* OpName = "Build BottomLevelAS (DeviceContextD3D12Impl::BuildBLAS)";
    TransitionOrVerifyBLASState(CmdCtx, *pBLASD3D12, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);
    TransitionOrVerifyBufferState(CmdCtx, *pScratchD3D12, Attribs.ScratchBufferTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    d3d12BuildASDesc   = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& d3d12BuildASInputs = d3d12BuildASDesc.Inputs;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>           Geometries;

    if (Attribs.pTriangleData != nullptr)
    {
        Geometries.resize(Attribs.TriangleDataCount);
        pBLASD3D12->SetActualGeometryCount(Attribs.TriangleDataCount);

        for (Uint32 i = 0; i < Attribs.TriangleDataCount; ++i)
        {
            const auto& SrcTris = Attribs.pTriangleData[i];
            Uint32      Idx     = i;
            Uint32      GeoIdx  = pBLASD3D12->UpdateGeometryIndex(SrcTris.GeometryName, Idx, Attribs.Update);

            if (GeoIdx == INVALID_INDEX || Idx == INVALID_INDEX)
            {
                UNEXPECTED("Failed to find geometry '", SrcTris.GeometryName, '\'');
                continue;
            }

            auto&       d3d12Geo  = Geometries[Idx];
            auto&       d3d12Tris = d3d12Geo.Triangles;
            const auto& TriDesc   = BLASDesc.pTriangles[GeoIdx];

            d3d12Geo.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            d3d12Geo.Flags = GeometryFlagsToD3D12RTGeometryFlags(SrcTris.Flags);

            auto* const pVB = ClassPtrCast<BufferD3D12Impl>(SrcTris.pVertexBuffer);

            // vertex format in SrcTris may be undefined, so use vertex format from description
            d3d12Tris.VertexBuffer.StartAddress  = pVB->GetGPUAddress() + SrcTris.VertexOffset;
            d3d12Tris.VertexBuffer.StrideInBytes = SrcTris.VertexStride;
            d3d12Tris.VertexCount                = SrcTris.VertexCount;
            d3d12Tris.VertexFormat               = TypeToRayTracingVertexFormat(TriDesc.VertexValueType, TriDesc.VertexComponentCount);
            VERIFY(d3d12Tris.VertexFormat != DXGI_FORMAT_UNKNOWN, "Unsupported combination of vertex value type and component count");

            VERIFY(d3d12Tris.VertexBuffer.StartAddress % GetValueSize(TriDesc.VertexValueType) == 0, "Vertex start address is not properly aligned");
            VERIFY(d3d12Tris.VertexBuffer.StrideInBytes % GetValueSize(TriDesc.VertexValueType) == 0, "Vertex stride is not properly aligned");

            TransitionOrVerifyBufferState(CmdCtx, *pVB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);

            if (SrcTris.pIndexBuffer)
            {
                auto* const pIB = ClassPtrCast<BufferD3D12Impl>(SrcTris.pIndexBuffer);

                // index type in SrcTris may be undefined, so use index type from description
                d3d12Tris.IndexFormat = ValueTypeToIndexType(TriDesc.IndexType);
                d3d12Tris.IndexBuffer = pIB->GetGPUAddress() + SrcTris.IndexOffset;
                d3d12Tris.IndexCount  = SrcTris.PrimitiveCount * 3;

                VERIFY(d3d12Tris.IndexBuffer % GetValueSize(TriDesc.IndexType) == 0, "Index start address is not properly aligned");

                TransitionOrVerifyBufferState(CmdCtx, *pIB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
            }
            else
            {
                d3d12Tris.IndexFormat = DXGI_FORMAT_UNKNOWN;
                d3d12Tris.IndexBuffer = 0;
            }

            if (SrcTris.pTransformBuffer)
            {
                auto* const pTB        = ClassPtrCast<BufferD3D12Impl>(SrcTris.pTransformBuffer);
                d3d12Tris.Transform3x4 = pTB->GetGPUAddress() + SrcTris.TransformBufferOffset;

                VERIFY(d3d12Tris.Transform3x4 % D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT == 0, "Transform start address is not properly aligned");

                TransitionOrVerifyBufferState(CmdCtx, *pTB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
            }
            else
            {
                d3d12Tris.Transform3x4 = 0;
            }
        }
    }
    else if (Attribs.pBoxData != nullptr)
    {
        Geometries.resize(Attribs.BoxDataCount);
        pBLASD3D12->SetActualGeometryCount(Attribs.BoxDataCount);

        for (Uint32 i = 0; i < Attribs.BoxDataCount; ++i)
        {
            const auto& SrcBoxes = Attribs.pBoxData[i];
            Uint32      Idx      = i;
            Uint32      GeoIdx   = pBLASD3D12->UpdateGeometryIndex(SrcBoxes.GeometryName, Idx, Attribs.Update);

            if (GeoIdx == INVALID_INDEX || Idx == INVALID_INDEX)
            {
                UNEXPECTED("Failed to find geometry '", SrcBoxes.GeometryName, '\'');
                continue;
            }

            auto& d3d12Geo  = Geometries[Idx];
            auto& d3d12AABs = d3d12Geo.AABBs;

            d3d12Geo.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            d3d12Geo.Flags = GeometryFlagsToD3D12RTGeometryFlags(SrcBoxes.Flags);

            auto* pBB                     = ClassPtrCast<BufferD3D12Impl>(SrcBoxes.pBoxBuffer);
            d3d12AABs.AABBCount           = SrcBoxes.BoxCount;
            d3d12AABs.AABBs.StartAddress  = pBB->GetGPUAddress() + SrcBoxes.BoxOffset;
            d3d12AABs.AABBs.StrideInBytes = SrcBoxes.BoxStride;

            DEV_CHECK_ERR(d3d12AABs.AABBs.StartAddress % D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT == 0, "AABB start address is not properly aligned");
            DEV_CHECK_ERR(d3d12AABs.AABBs.StrideInBytes % D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT == 0, "AABB stride is not properly aligned");

            TransitionOrVerifyBufferState(CmdCtx, *pBB, Attribs.GeometryTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
        }
    }

    d3d12BuildASInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    d3d12BuildASInputs.Flags          = BuildASFlagsToD3D12ASBuildFlags(BLASDesc.Flags);
    d3d12BuildASInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    d3d12BuildASInputs.NumDescs       = static_cast<UINT>(Geometries.size());
    d3d12BuildASInputs.pGeometryDescs = Geometries.data();

    d3d12BuildASDesc.DestAccelerationStructureData    = pBLASD3D12->GetGPUAddress();
    d3d12BuildASDesc.ScratchAccelerationStructureData = pScratchD3D12->GetGPUAddress() + Attribs.ScratchBufferOffset;
    d3d12BuildASDesc.SourceAccelerationStructureData  = 0;

    if (Attribs.Update)
    {
        d3d12BuildASInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        d3d12BuildASDesc.SourceAccelerationStructureData = d3d12BuildASDesc.DestAccelerationStructureData;
    }

    DEV_CHECK_ERR(d3d12BuildASDesc.ScratchAccelerationStructureData % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0,
                  "Scratch data address is not properly aligned");

    CmdCtx.AsGraphicsContext4().BuildRaytracingAccelerationStructure(d3d12BuildASDesc, 0, nullptr);
    ++m_State.NumCommands;

#ifdef DILIGENT_DEVELOPMENT
    pBLASD3D12->DvpUpdateVersion();
#endif
}

void DeviceContextD3D12Impl::BuildTLAS(const BuildTLASAttribs& Attribs)
{
    TDeviceContextBase::BuildTLAS(Attribs, 0);

    static_assert(TLAS_INSTANCE_DATA_SIZE == sizeof(D3D12_RAYTRACING_INSTANCE_DESC), "Value in TLAS_INSTANCE_DATA_SIZE doesn't match the actual instance description size");

    auto* pTLASD3D12      = ClassPtrCast<TopLevelASD3D12Impl>(Attribs.pTLAS);
    auto* pScratchD3D12   = ClassPtrCast<BufferD3D12Impl>(Attribs.pScratchBuffer);
    auto* pInstancesD3D12 = ClassPtrCast<BufferD3D12Impl>(Attribs.pInstanceBuffer);

    auto&       CmdCtx = GetCmdContext();
    const char* OpName = "Build TopLevelAS (DeviceContextD3D12Impl::BuildTLAS)";
    TransitionOrVerifyTLASState(CmdCtx, *pTLASD3D12, Attribs.TLASTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);
    TransitionOrVerifyBufferState(CmdCtx, *pScratchD3D12, Attribs.ScratchBufferTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    if (Attribs.Update)
    {
        if (!pTLASD3D12->UpdateInstances(Attribs.pInstances, Attribs.InstanceCount, Attribs.BaseContributionToHitGroupIndex, Attribs.HitGroupStride, Attribs.BindingMode))
            return;
    }
    else
    {
        if (!pTLASD3D12->SetInstanceData(Attribs.pInstances, Attribs.InstanceCount, Attribs.BaseContributionToHitGroupIndex, Attribs.HitGroupStride, Attribs.BindingMode))
            return;
    }

    // copy instance data into instance buffer
    {
        size_t Size     = Attribs.InstanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        auto   TmpSpace = m_DynamicHeap.Allocate(Size, 16, m_FrameNumber);

        for (Uint32 i = 0; i < Attribs.InstanceCount; ++i)
        {
            const auto& Inst     = Attribs.pInstances[i];
            const auto  InstDesc = pTLASD3D12->GetInstanceDesc(Inst.InstanceName);

            if (InstDesc.InstanceIndex >= Attribs.InstanceCount)
            {
                UNEXPECTED("Failed to find instance by name");
                return;
            }

            auto& d3d12Inst  = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(TmpSpace.CPUAddress)[InstDesc.InstanceIndex];
            auto* pBLASD3D12 = ClassPtrCast<BottomLevelASD3D12Impl>(Inst.pBLAS);

            static_assert(sizeof(d3d12Inst.Transform) == sizeof(Inst.Transform), "size mismatch");
            std::memcpy(&d3d12Inst.Transform, Inst.Transform.data, sizeof(d3d12Inst.Transform));

            d3d12Inst.InstanceID                          = Inst.CustomId;
            d3d12Inst.InstanceContributionToHitGroupIndex = InstDesc.ContributionToHitGroupIndex;
            d3d12Inst.InstanceMask                        = Inst.Mask;
            d3d12Inst.Flags                               = InstanceFlagsToD3D12RTInstanceFlags(Inst.Flags);
            d3d12Inst.AccelerationStructure               = pBLASD3D12->GetGPUAddress();

            TransitionOrVerifyBLASState(CmdCtx, *pBLASD3D12, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
        }
        UpdateBufferRegion(pInstancesD3D12, TmpSpace, Attribs.InstanceBufferOffset, Size, Attribs.InstanceBufferTransitionMode);
    }
    TransitionOrVerifyBufferState(CmdCtx, *pInstancesD3D12, Attribs.InstanceBufferTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    d3d12BuildASDesc   = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& d3d12BuildASInputs = d3d12BuildASDesc.Inputs;

    d3d12BuildASInputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    d3d12BuildASInputs.Flags         = BuildASFlagsToD3D12ASBuildFlags(pTLASD3D12->GetDesc().Flags);
    d3d12BuildASInputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
    d3d12BuildASInputs.NumDescs      = Attribs.InstanceCount;
    d3d12BuildASInputs.InstanceDescs = pInstancesD3D12->GetGPUAddress() + Attribs.InstanceBufferOffset;

    d3d12BuildASDesc.DestAccelerationStructureData    = pTLASD3D12->GetGPUAddress();
    d3d12BuildASDesc.ScratchAccelerationStructureData = pScratchD3D12->GetGPUAddress() + Attribs.ScratchBufferOffset;
    d3d12BuildASDesc.SourceAccelerationStructureData  = 0;

    if (Attribs.Update)
    {
        d3d12BuildASInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        d3d12BuildASDesc.SourceAccelerationStructureData = d3d12BuildASDesc.DestAccelerationStructureData;
    }

    DEV_CHECK_ERR(d3d12BuildASInputs.InstanceDescs % D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT == 0,
                  "Instance data address is not properly aligned");
    DEV_CHECK_ERR(d3d12BuildASDesc.ScratchAccelerationStructureData % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0,
                  "Scratch data address is not properly aligned");

    CmdCtx.AsGraphicsContext4().BuildRaytracingAccelerationStructure(d3d12BuildASDesc, 0, nullptr);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::CopyBLAS(const CopyBLASAttribs& Attribs)
{
    TDeviceContextBase::CopyBLAS(Attribs, 0);

    auto* pSrcD3D12 = ClassPtrCast<BottomLevelASD3D12Impl>(Attribs.pSrc);
    auto* pDstD3D12 = ClassPtrCast<BottomLevelASD3D12Impl>(Attribs.pDst);
    auto& CmdCtx    = GetCmdContext();
    auto  Mode      = CopyASModeToD3D12ASCopyMode(Attribs.Mode);

    // Dst BLAS description has specified CompactedSize, but doesn't have specified pTriangles and pBoxes.
    // We should copy geometries because it required for SBT to map geometry name to hit group.
    pDstD3D12->CopyGeometryDescription(*pSrcD3D12);
    pDstD3D12->SetActualGeometryCount(pSrcD3D12->GetActualGeometryCount());

    const char* OpName = "Copy BottomLevelAS (DeviceContextD3D12Impl::CopyBLAS)";
    TransitionOrVerifyBLASState(CmdCtx, *pSrcD3D12, Attribs.SrcTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBLASState(CmdCtx, *pDstD3D12, Attribs.DstTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    CmdCtx.AsGraphicsContext4().CopyRaytracingAccelerationStructure(pDstD3D12->GetGPUAddress(), pSrcD3D12->GetGPUAddress(), Mode);
    ++m_State.NumCommands;

#ifdef DILIGENT_DEVELOPMENT
    pDstD3D12->DvpUpdateVersion();
#endif
}

void DeviceContextD3D12Impl::CopyTLAS(const CopyTLASAttribs& Attribs)
{
    TDeviceContextBase::CopyTLAS(Attribs, 0);

    auto* pSrcD3D12 = ClassPtrCast<TopLevelASD3D12Impl>(Attribs.pSrc);
    auto* pDstD3D12 = ClassPtrCast<TopLevelASD3D12Impl>(Attribs.pDst);
    auto& CmdCtx    = GetCmdContext();
    auto  Mode      = CopyASModeToD3D12ASCopyMode(Attribs.Mode);

    // Instances specified in BuildTLAS command.
    // We should copy instances because it required for SBT to map instance name to hit group.
    pDstD3D12->CopyInstanceData(*pSrcD3D12);

    const char* OpName = "Copy BottomLevelAS (DeviceContextD3D12Impl::CopyTLAS)";
    TransitionOrVerifyTLASState(CmdCtx, *pSrcD3D12, Attribs.SrcTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyTLASState(CmdCtx, *pDstD3D12, Attribs.DstTransitionMode, RESOURCE_STATE_BUILD_AS_WRITE, OpName);

    CmdCtx.AsGraphicsContext4().CopyRaytracingAccelerationStructure(pDstD3D12->GetGPUAddress(), pSrcD3D12->GetGPUAddress(), Mode);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs)
{
    TDeviceContextBase::WriteBLASCompactedSize(Attribs, 0);

    static_assert(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC) == sizeof(Uint64),
                  "Engine api specifies that compacted size is 64 bits");

    auto* pBLASD3D12     = ClassPtrCast<BottomLevelASD3D12Impl>(Attribs.pBLAS);
    auto* pDestBuffD3D12 = ClassPtrCast<BufferD3D12Impl>(Attribs.pDestBuffer);
    auto& CmdCtx         = GetCmdContext();

    const char* OpName = "Write AS compacted size (DeviceContextD3D12Impl::WriteBLASCompactedSize)";
    TransitionOrVerifyBLASState(CmdCtx, *pBLASD3D12, Attribs.BLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBufferState(CmdCtx, *pDestBuffD3D12, Attribs.BufferTransitionMode, RESOURCE_STATE_UNORDERED_ACCESS, OpName);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC d3d12Desc = {};

    d3d12Desc.DestBuffer = pDestBuffD3D12->GetGPUAddress() + Attribs.DestBufferOffset;
    d3d12Desc.InfoType   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

    CmdCtx.AsGraphicsContext4().EmitRaytracingAccelerationStructurePostbuildInfo(d3d12Desc, pBLASD3D12->GetGPUAddress());
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs)
{
    TDeviceContextBase::WriteTLASCompactedSize(Attribs, 0);

    static_assert(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC) == sizeof(Uint64),
                  "Engine api specifies that compacted size is 64 bits");

    auto* pTLASD3D12     = ClassPtrCast<TopLevelASD3D12Impl>(Attribs.pTLAS);
    auto* pDestBuffD3D12 = ClassPtrCast<BufferD3D12Impl>(Attribs.pDestBuffer);
    auto& CmdCtx         = GetCmdContext();

    const char* OpName = "Write AS compacted size (DeviceContextD3D12Impl::WriteTLASCompactedSize)";
    TransitionOrVerifyTLASState(CmdCtx, *pTLASD3D12, Attribs.TLASTransitionMode, RESOURCE_STATE_BUILD_AS_READ, OpName);
    TransitionOrVerifyBufferState(CmdCtx, *pDestBuffD3D12, Attribs.BufferTransitionMode, RESOURCE_STATE_UNORDERED_ACCESS, OpName);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC d3d12Desc = {};

    d3d12Desc.DestBuffer = pDestBuffD3D12->GetGPUAddress() + Attribs.DestBufferOffset;
    d3d12Desc.InfoType   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

    CmdCtx.AsGraphicsContext4().EmitRaytracingAccelerationStructurePostbuildInfo(d3d12Desc, pTLASD3D12->GetGPUAddress());
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::TraceRays(const TraceRaysAttribs& Attribs)
{
    TDeviceContextBase::TraceRays(Attribs, 0);

    auto&       CmdCtx    = GetCmdContext().AsGraphicsContext4();
    const auto* pSBTD3D12 = ClassPtrCast<const ShaderBindingTableD3D12Impl>(Attribs.pSBT);

    D3D12_DISPATCH_RAYS_DESC d3d12DispatchDesc = pSBTD3D12->GetD3D12BindingTable();

    d3d12DispatchDesc.Width  = Attribs.DimensionX;
    d3d12DispatchDesc.Height = Attribs.DimensionY;
    d3d12DispatchDesc.Depth  = Attribs.DimensionZ;

    PrepareForDispatchRays(CmdCtx);

    CmdCtx.DispatchRays(d3d12DispatchDesc);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs)
{
    TDeviceContextBase::TraceRaysIndirect(Attribs, 0);

    auto&       CmdCtx              = GetCmdContext().AsGraphicsContext4();
    auto*       pAttribsBufferD3D12 = ClassPtrCast<BufferD3D12Impl>(Attribs.pAttribsBuffer);
    const char* OpName              = "Trace rays indirect (DeviceContextD3D12Impl::TraceRaysIndirect)";
    TransitionOrVerifyBufferState(CmdCtx, *pAttribsBufferD3D12, Attribs.AttribsBufferStateTransitionMode, RESOURCE_STATE_INDIRECT_ARGUMENT, OpName);

    PrepareForDispatchRays(CmdCtx);

    CmdCtx.ExecuteIndirect(m_pTraceRaysIndirectSignature, 1, pAttribsBufferD3D12->GetD3D12Resource(), Attribs.ArgsByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs)
{
    TDeviceContextBase::UpdateSBT(pSBT, pUpdateIndirectBufferAttribs, 0);

    auto&            CmdCtx          = GetCmdContext().AsGraphicsContext();
    const char*      OpName          = "Update shader binding table (DeviceContextD3D12Impl::UpdateSBT)";
    auto*            pSBTD3D12       = ClassPtrCast<ShaderBindingTableD3D12Impl>(pSBT);
    BufferD3D12Impl* pSBTBufferD3D12 = nullptr;

    ShaderBindingTableD3D12Impl::BindingTable RayGenShaderRecord  = {};
    ShaderBindingTableD3D12Impl::BindingTable MissShaderTable     = {};
    ShaderBindingTableD3D12Impl::BindingTable HitGroupTable       = {};
    ShaderBindingTableD3D12Impl::BindingTable CallableShaderTable = {};

    pSBTD3D12->GetData(pSBTBufferD3D12, RayGenShaderRecord, MissShaderTable, HitGroupTable, CallableShaderTable);

    if (RayGenShaderRecord.pData || MissShaderTable.pData || HitGroupTable.pData || CallableShaderTable.pData)
    {
        TransitionOrVerifyBufferState(CmdCtx, *pSBTBufferD3D12, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, RESOURCE_STATE_COPY_DEST, OpName);

        // Buffer ranges do not intersect, so we don't need to add barriers between them
        if (RayGenShaderRecord.pData)
            UpdateBuffer(pSBTBufferD3D12, RayGenShaderRecord.Offset, RayGenShaderRecord.Size, RayGenShaderRecord.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (MissShaderTable.pData)
            UpdateBuffer(pSBTBufferD3D12, MissShaderTable.Offset, MissShaderTable.Size, MissShaderTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (HitGroupTable.pData)
            UpdateBuffer(pSBTBufferD3D12, HitGroupTable.Offset, HitGroupTable.Size, HitGroupTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        if (CallableShaderTable.pData)
            UpdateBuffer(pSBTBufferD3D12, CallableShaderTable.Offset, CallableShaderTable.Size, CallableShaderTable.pData, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        TransitionOrVerifyBufferState(CmdCtx, *pSBTBufferD3D12, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, RESOURCE_STATE_RAY_TRACING, OpName);
    }
    else
    {
        // Ray tracing command can be used in parallel with the same SBT, so internal buffer state must be RESOURCE_STATE_RAY_TRACING to allow it.
        VERIFY(pSBTBufferD3D12->CheckState(RESOURCE_STATE_RAY_TRACING), "SBT buffer must always be in RESOURCE_STATE_RAY_TRACING state");
    }

    if (pUpdateIndirectBufferAttribs != nullptr)
    {
        const auto& d3d12DispatchDesc = pSBTD3D12->GetD3D12BindingTable();
        UpdateBuffer(pUpdateIndirectBufferAttribs->pAttribsBuffer,
                     pUpdateIndirectBufferAttribs->AttribsBufferOffset,
                     TraceRaysIndirectCommandSBTSize,
                     &d3d12DispatchDesc,
                     pUpdateIndirectBufferAttribs->TransitionMode);
    }
}

void DeviceContextD3D12Impl::BeginDebugGroup(const Char* Name, const float* pColor)
{
    TDeviceContextBase::BeginDebugGroup(Name, pColor, 0);

    GetCmdContext().PixBeginEvent(Name, pColor);
}

void DeviceContextD3D12Impl::EndDebugGroup()
{
    TDeviceContextBase::EndDebugGroup(0);

    GetCmdContext().PixEndEvent();
}

void DeviceContextD3D12Impl::InsertDebugLabel(const Char* Label, const float* pColor)
{
    TDeviceContextBase::InsertDebugLabel(Label, pColor, 0);

    GetCmdContext().PixSetMarker(Label, pColor);
}

void DeviceContextD3D12Impl::SetShadingRate(SHADING_RATE BaseRate, SHADING_RATE_COMBINER PrimitiveCombiner, SHADING_RATE_COMBINER TextureCombiner)
{
    TDeviceContextBase::SetShadingRate(BaseRate, PrimitiveCombiner, TextureCombiner, 0);

    const D3D12_SHADING_RATE_COMBINER Combiners[] =
        {
            ShadingRateCombinerToD3D12ShadingRateCombiner(PrimitiveCombiner),
            ShadingRateCombinerToD3D12ShadingRateCombiner(TextureCombiner) //
        };
    GetCmdContext().AsGraphicsContext5().SetShadingRate(ShadingRateToD3D12ShadingRate(BaseRate), Combiners);

    m_State.bUsingShadingRate = !(BaseRate == SHADING_RATE_1X1 && PrimitiveCombiner == SHADING_RATE_COMBINER_PASSTHROUGH && TextureCombiner == SHADING_RATE_COMBINER_PASSTHROUGH);
}

namespace
{

struct TileMappingKey
{
    ID3D12Resource* const pResource;
    ID3D12Heap* const     pHeap;

    bool operator==(const TileMappingKey& Rhs) const noexcept
    {
        return pResource == Rhs.pResource && pHeap == Rhs.pHeap;
    }

    struct Hasher
    {
        size_t operator()(const TileMappingKey& Key) const noexcept
        {
            return ComputeHash(Key.pResource, Key.pHeap);
        }
    };
};

} // namespace

void DeviceContextD3D12Impl::BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs)
{
    TDeviceContextBase::BindSparseResourceMemory(Attribs, 0);

    VERIFY_EXPR(Attribs.NumBufferBinds != 0 || Attribs.NumTextureBinds != 0);

    Flush();

    std::unordered_map<TileMappingKey, D3D12TileMappingHelper, TileMappingKey::Hasher> TileMappingMap;

    for (Uint32 i = 0; i < Attribs.NumBufferBinds; ++i)
    {
        const auto& BuffBind   = Attribs.pBufferBinds[i];
        auto*       pd3d12Buff = ClassPtrCast<const BufferD3D12Impl>(BuffBind.pBuffer)->GetD3D12Resource();

        for (Uint32 r = 0; r < BuffBind.NumRanges; ++r)
        {
            const auto& BindRange = BuffBind.pRanges[r];
            const auto  pMemD3D12 = RefCntAutoPtr<IDeviceMemoryD3D12>{BindRange.pMemory, IID_DeviceMemoryD3D12};
            DEV_CHECK_ERR((BindRange.pMemory != nullptr) == (pMemD3D12 != nullptr),
                          "Failed to query IDeviceMemoryD3D12 interface from non-null memory object");

            const auto MemRange = pMemD3D12 ? pMemD3D12->GetRange(BindRange.MemoryOffset, BindRange.MemorySize) : DeviceMemoryRangeD3D12{};
            DEV_CHECK_ERR((MemRange.Offset % D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES) == 0,
                          "MemoryOffset must be a multiple of sparse block size");

            auto& DstMapping = TileMappingMap[TileMappingKey{pd3d12Buff, MemRange.pHandle}];
            DstMapping.AddBufferBindRange(BindRange, MemRange.Offset);
        }
    }

    for (Uint32 i = 0; i < Attribs.NumTextureBinds; ++i)
    {
        const auto& TexBind        = Attribs.pTextureBinds[i];
        const auto* pTexD3D12      = ClassPtrCast<const TextureD3D12Impl>(TexBind.pTexture);
        const auto& TexSparseProps = pTexD3D12->GetSparseProperties();
        const auto& TexDesc        = pTexD3D12->GetDesc();
        const auto  UseNVApi       = pTexD3D12->IsUsingNVApi();

        for (Uint32 r = 0; r < TexBind.NumRanges; ++r)
        {
            const auto& BindRange = TexBind.pRanges[r];
            const auto  pMemD3D12 = RefCntAutoPtr<IDeviceMemoryD3D12>{BindRange.pMemory, IID_DeviceMemoryD3D12};
            DEV_CHECK_ERR((BindRange.pMemory != nullptr) == (pMemD3D12 != nullptr),
                          "Failed to query IDeviceMemoryD3D12 interface from non-null memory object");

            const auto MemRange = pMemD3D12 ? pMemD3D12->GetRange(BindRange.MemoryOffset, BindRange.MemorySize) : DeviceMemoryRangeD3D12{};
            DEV_CHECK_ERR((MemRange.Offset % D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES) == 0,
                          "MemoryOffset must be a multiple of sparse block size");
            VERIFY_EXPR(pMemD3D12 == nullptr || pMemD3D12->IsUsingNVApi() == UseNVApi);

            auto& DstMapping = TileMappingMap[TileMappingKey{pTexD3D12->GetD3D12Resource(), MemRange.pHandle}];
            DstMapping.AddTextureBindRange(BindRange, TexSparseProps, TexDesc, UseNVApi, MemRange.Offset);
        }
    }

    auto* pQueueD3D12 = LockCommandQueue();

    for (Uint32 i = 0; i < Attribs.NumWaitFences; ++i)
    {
        auto* pFenceD3D12 = ClassPtrCast<FenceD3D12Impl>(Attribs.ppWaitFences[i]);
        DEV_CHECK_ERR(pFenceD3D12 != nullptr, "Wait fence must not be null");
        auto Value = Attribs.pWaitFenceValues[i];
        pQueueD3D12->WaitFence(pFenceD3D12->GetD3D12Fence(), Value);
        pFenceD3D12->DvpDeviceWait(Value);
    }

    std::vector<ResourceTileMappingsD3D12> TileMappings;
    TileMappings.reserve(TileMappingMap.size());
    for (const auto& it : TileMappingMap)
    {
        TileMappings.emplace_back(it.second.GetMappings(it.first.pResource, it.first.pHeap));
    }
    pQueueD3D12->UpdateTileMappings(TileMappings.data(), static_cast<Uint32>(TileMappings.size()));

    for (Uint32 i = 0; i < Attribs.NumSignalFences; ++i)
    {
        auto* pFenceD3D12 = ClassPtrCast<FenceD3D12Impl>(Attribs.ppSignalFences[i]);
        DEV_CHECK_ERR(pFenceD3D12 != nullptr, "Signal fence must not be null");
        auto Value = Attribs.pSignalFenceValues[i];
        pQueueD3D12->EnqueueSignal(pFenceD3D12->GetD3D12Fence(), Value);
        pFenceD3D12->DvpSignal(Value);
    }

    UnlockCommandQueue();
}

} // namespace Diligent
