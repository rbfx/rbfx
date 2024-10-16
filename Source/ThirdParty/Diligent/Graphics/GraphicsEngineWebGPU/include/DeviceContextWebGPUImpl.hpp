/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::DeviceContextWebGPUImpl class

#include "EngineWebGPUImplTraits.hpp"
#include "DeviceContextBase.hpp"
#include "TextureWebGPUImpl.hpp"
#include "BufferWebGPUImpl.hpp"
#include "DynamicMemoryManagerWebGPU.hpp"
#include "PipelineStateWebGPUImpl.hpp"
#include "ShaderWebGPUImpl.hpp"
#include "FramebufferWebGPUImpl.hpp"
#include "RenderPassWebGPUImpl.hpp"
#include "UploadMemoryManagerWebGPU.hpp"

namespace Diligent
{

class QueryManagerWebGPU;

/// Device context implementation in WebGPU backend.
class DeviceContextWebGPUImpl final : public DeviceContextBase<EngineWebGPUImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextBase;

    DeviceContextWebGPUImpl(IReferenceCounters*           pRefCounters,
                            RenderDeviceWebGPUImpl*       pDevice,
                            const EngineWebGPUCreateInfo& EngineCI,
                            const DeviceContextDesc&      Desc);

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceContextWebGPU, TDeviceContextBase)

    /// Implementation of IDeviceContext::Begin() in WebGPU backend.
    void DILIGENT_CALL_TYPE Begin(Uint32 ImmediateContextId) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in WebGPU backend.
    void DILIGENT_CALL_TYPE TransitionShaderResources(IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in WebGPU backend.
    void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                  RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                             Uint32                         NumBuffersSet,
                                             IBuffer* const*                ppBuffers,
                                             const Uint64*                  pOffsets,
                                             RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                             SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in WebGPU backend.
    void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                           Uint64                         ByteOffset,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                         const Viewport* pViewports,
                                         Uint32          RTWidth,
                                         Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                            const Rect* pRects,
                                            Uint32      RTWidth,
                                            Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargetsExt() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BeginRenderPass() in WebGPU backend.
    void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::NextSubpass() in WebGPU backend.
    void DILIGENT_CALL_TYPE NextSubpass() override final;

    /// Implementation of IDeviceContext::EndRenderPass() in WebGPU backend.
    void DILIGENT_CALL_TYPE EndRenderPass() override final;

    /// Implementation of IDeviceContext::Draw() in WebGPU backend.
    void DILIGENT_CALL_TYPE Draw(const DrawAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DrawIndexed() in WebGPU backend.
    void DILIGENT_CALL_TYPE DrawIndexed(const DrawIndexedAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DrawIndirect() in WebGPU backend.
    void DILIGENT_CALL_TYPE DrawIndirect(const DrawIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DrawIndexedIndirect() in WebGPU backend.
    void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DrawMesh() in WebGPU backend.
    void DILIGENT_CALL_TYPE DrawMesh(const DrawMeshAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DrawMeshIndirect() in WebGPU backend.
    void DILIGENT_CALL_TYPE DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::MultiDraw() in WebGPU backend.
    void DILIGENT_CALL_TYPE MultiDraw(const MultiDrawAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::MultiDrawIndexed() in WebGPU backend.
    void DILIGENT_CALL_TYPE MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DispatchCompute() in WebGPU backend.
    void DILIGENT_CALL_TYPE DispatchCompute(const DispatchComputeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DispatchComputeIndirect() in WebGPU backend.
    void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::ClearDepthStencil() in WebGPU backend.
    void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                              CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                              float                          fDepth,
                                              Uint8                          Stencil,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in WebGPU backend.
    void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView*                  pView,
                                              const void*                    RGBA,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in WebGPU backend.
    void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                         Uint64                         Offset,
                                         Uint64                         Size,
                                         const void*                    pData,
                                         RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in WebGPU backend.
    void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                       Uint64                         SrcOffset,
                                       RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                       IBuffer*                       pDstBuffer,
                                       Uint64                         DstOffset,
                                       Uint64                         Size,
                                       RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in WebGPU backend.
    void DILIGENT_CALL_TYPE MapBuffer(IBuffer*  pBuffer,
                                      MAP_TYPE  MapType,
                                      MAP_FLAGS MapFlags,
                                      PVoid&    pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in WebGPU backend.
    void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in WebGPU backend.
    void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                          Uint32                         MipLevel,
                                          Uint32                         Slice,
                                          const Box&                     DstBox,
                                          const TextureSubResData&       SubresData,
                                          RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                          RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in WebGPU backend.
    void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in WebGPU backend.
    void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                  Uint32                    MipLevel,
                                                  Uint32                    ArraySlice,
                                                  MAP_TYPE                  MapType,
                                                  MAP_FLAGS                 MapFlags,
                                                  const Box*                pMapRegion,
                                                  MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in WebGPU backend.
    void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in WebGPU backend.
    void DILIGENT_CALL_TYPE FinishCommandList(ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandLists() in WebGPU backend.
    void DILIGENT_CALL_TYPE ExecuteCommandLists(Uint32               NumCommandLists,
                                                ICommandList* const* ppCommandLists) override final;

    /// Implementation of IDeviceContext::EnqueueSignal() in WebGPU backend.
    void DILIGENT_CALL_TYPE EnqueueSignal(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::DeviceWaitForFence() in WebGPU backend.
    void DILIGENT_CALL_TYPE DeviceWaitForFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in WebGPU backend.
    void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in WebGPU backend.
    void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in WebGPU backend.
    void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in WebGPU backend.
    void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContext::BuildBLAS() in WebGPU backend.
    void DILIGENT_CALL_TYPE BuildBLAS(const BuildBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BuildTLAS() in WebGPU backend.
    void DILIGENT_CALL_TYPE BuildTLAS(const BuildTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyBLAS() in WebGPU backend.
    void DILIGENT_CALL_TYPE CopyBLAS(const CopyBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyTLAS() in WebGPU backend.
    void DILIGENT_CALL_TYPE CopyTLAS(const CopyTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteBLASCompactedSize() in WebGPU backend.
    void DILIGENT_CALL_TYPE WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteTLASCompactedSize() in WebGPU backend.
    void DILIGENT_CALL_TYPE WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRays() in WebGPU backend.
    void DILIGENT_CALL_TYPE TraceRays(const TraceRaysAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRaysIndirect() in WebGPU backend.
    void DILIGENT_CALL_TYPE TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::UpdateSBT() in WebGPU backend.
    void DILIGENT_CALL_TYPE UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs) override final;

    /// Implementation of IDeviceContext::BeginDebugGroup() in WebGPU backend.
    void DILIGENT_CALL_TYPE BeginDebugGroup(const Char* Name, const float* pColor) override final;

    /// Implementation of IDeviceContext::EndDebugGroup() in WebGPU backend.
    void DILIGENT_CALL_TYPE EndDebugGroup() override final;

    /// Implementation of IDeviceContext::InsertDebugLabel() in WebGPU backend.
    void DILIGENT_CALL_TYPE InsertDebugLabel(const Char* Label, const float* pColor) override final;

    /// Implementation of IDeviceContext::SetShadingRate() in WebGPU backend.
    void DILIGENT_CALL_TYPE SetShadingRate(SHADING_RATE          BaseRate,
                                           SHADING_RATE_COMBINER PrimitiveCombiner,
                                           SHADING_RATE_COMBINER TextureCombiner) override final;

    /// Implementation of IDeviceContext::BindSparseResourceMemory() in WebGPU backend.
    void DILIGENT_CALL_TYPE BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::GenerateMips() in WebGPU backend.
    void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override final;

    /// Implementation of IDeviceContext::FinishFrame() in WebGPU backend.
    void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in WebGPU backend.
    void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::LockCommandQueue() in WebGPU backend.
    ICommandQueue* DILIGENT_CALL_TYPE LockCommandQueue() override final;

    /// Implementation of IDeviceContext::UnlockCommandQueue() in WebGPU backend.
    void DILIGENT_CALL_TYPE UnlockCommandQueue() override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in WebGPU backend.
    void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                      ITexture*                               pDstTexture,
                                                      const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContextWebGPU::GetWebGPUQueue() in WebGPU backend.
    WGPUQueue DILIGENT_CALL_TYPE GetWebGPUQueue() override final;

    QueryManagerWebGPU& GetQueryManager();

    Uint64 GetNextFenceValue();

    Uint64 GetCompletedFenceValue();

private:
    enum COMMAND_ENCODER_FLAGS : Uint32
    {
        COMMAND_ENCODER_FLAG_NONE    = 0u,
        COMMAND_ENCODER_FLAG_RENDER  = 1u << 0,
        COMMAND_ENCODER_FLAG_COMPUTE = 1u << 1,

        COMMAND_ENCODER_FLAG_ALL =
            COMMAND_ENCODER_FLAG_RENDER |
            COMMAND_ENCODER_FLAG_COMPUTE
    };

    enum DEBUG_GROUP_TYPE : Uint32
    {
        // Debug group was started within render pass encoder
        DEBUG_GROUP_TYPE_RENDER,

        // Debug group was started within compute pass encoder
        DEBUG_GROUP_TYPE_COMPUTE,

        // Debug group was started outside of any encoder
        DEBUG_GROUP_TYPE_OUTER,

        // Debug group has been ended when the encoder was ended
        DEBUG_GROUP_TYPE_NULL
    };

    enum OCCLUSION_QUERY_TYPE : Uint32
    {
        // Occlusion query was started within render pass encoder
        OCCLUSION_QUERY_TYPE_INNER,

        // Occlusion query was started was started outside of render pass encoder
        OCCLUSION_QUERY_TYPE_OUTER
    };

    WGPUCommandEncoder     GetCommandEncoder();
    WGPURenderPassEncoder  GetRenderPassCommandEncoder();
    WGPUComputePassEncoder GetComputePassCommandEncoder();

    void EndCommandEncoders(Uint32 EncoderFlags = COMMAND_ENCODER_FLAG_ALL);

    void CommitRenderTargets();
    void CommitSubpassRenderTargets();
    void ClearEncoderState();

    void ClearAttachment(Int32                     RTIndex,
                         COLOR_MASK                ColorMask,
                         CLEAR_DEPTH_STENCIL_FLAGS DSFlags,
                         const float               ClearData[],
                         Uint8                     Stencil);

    WGPURenderPassEncoder  PrepareForDraw(DRAW_FLAGS Flags);
    WGPURenderPassEncoder  PrepareForIndexedDraw(DRAW_FLAGS Flags, VALUE_TYPE IndexType);
    WGPUComputePassEncoder PrepareForDispatchCompute();
    WGPUBuffer             PrepareForIndirectCommand(IBuffer* pAttribsBuffer, Uint64& IdirectBufferOffset);

    void CommitGraphicsPSO(WGPURenderPassEncoder CmdEncoder);
    void CommitComputePSO(WGPUComputePassEncoder CmdEncoder);
    void CommitVertexBuffers(WGPURenderPassEncoder CmdEncoder);
    void CommitIndexBuffer(WGPURenderPassEncoder CmdEncoder, VALUE_TYPE IndexType);
    void CommitViewports(WGPURenderPassEncoder CmdEncoder);
    void CommitScissorRects(WGPURenderPassEncoder CmdEncoder);

    template <typename CmdEncoderType>
    void CommitBindGroups(CmdEncoderType CmdEncoder, Uint32 CommitSRBMask);

    UploadMemoryManagerWebGPU::Allocation  AllocateUploadMemory(size_t Size, size_t Alignment = 16);
    DynamicMemoryManagerWebGPU::Allocation AllocateDynamicMemory(size_t Size, size_t Alignment = 16);

#ifdef DILIGENT_DEVELOPMENT
    void DvpValidateCommittedShaderResources();
#endif

private:
    friend class QueryManagerWebGPU;
    friend class GenerateMipsHelperWebGPU;

    struct WebGPUEncoderState
    {
        enum CMD_ENCODER_STATE_FLAGS : Uint32
        {
            CMD_ENCODER_STATE_NONE           = 0,
            CMD_ENCODER_STATE_PIPELINE_STATE = 1 << 0,
            CMD_ENCODER_STATE_INDEX_BUFFER   = 1 << 1,
            CMD_ENCODER_STATE_VERTEX_BUFFERS = 1 << 2,
            CMD_ENCODER_STATE_VIEWPORTS      = 1 << 3,
            CMD_ENCODER_STATE_SCISSOR_RECTS  = 1 << 4,
            CMD_ENCODER_STATE_BLEND_FACTORS  = 1 << 5,
            CMD_ENCODER_STATE_STENCIL_REF    = 1 << 6,
            CMD_ENCODER_STATE_LAST           = CMD_ENCODER_STATE_STENCIL_REF,
            CMD_ENCODER_STATE_ALL            = 2 * CMD_ENCODER_STATE_LAST - 1
        };

        bool IsUpToDate(CMD_ENCODER_STATE_FLAGS StateFlag) const
        {
            return (CmdEncoderUpToDateStates & StateFlag) != 0;
        }

        void SetUpToDate(CMD_ENCODER_STATE_FLAGS StateFlag)
        {
            CmdEncoderUpToDateStates |= StateFlag;
        }

        void Invalidate(CMD_ENCODER_STATE_FLAGS StateFlag)
        {
            CmdEncoderUpToDateStates &= ~StateFlag;
        }

        void Clear()
        {
            *this = WebGPUEncoderState{};
        }

        Uint32 CmdEncoderUpToDateStates = CMD_ENCODER_STATE_NONE;
        bool   HasDynamicVertexBuffers  = false;

        std::array<Uint64, MAX_BUFFER_SLOTS> VertexBufferOffsets = []() {
            std::array<Uint64, MAX_BUFFER_SLOTS> InvalidOffsets;
            InvalidOffsets.fill(UINT64_MAX);
            return InvalidOffsets;
        }();

        std::array<Viewport, MAX_VIEWPORTS> Viewports    = {};
        std::array<Rect, MAX_VIEWPORTS>     ScissorRects = {};

    } m_EncoderState;

    struct WebGPUResourceBindInfo : CommittedShaderResources
    {
        struct BindGroupInfo
        {
            WGPUBindGroup wgpuBindGroup = nullptr;

            // Bind index to use with wgpuEncoderSetBindGroup
            Uint32 BindIndex = ~0u;

            // Memory to store dynamic buffer offsets for wgpuEncoderSetBindGroup.
            // The total number of resources with dynamic offsets is given by pSignature->GetDynamicOffsetCount().
            // Note that this is not the actual number of dynamic buffers in the resource cache.
            std::vector<Uint32> DynamicBufferOffsets;

            bool IsActive() const
            {
                return BindIndex != BindGroupInfo{}.BindIndex;
            }
            void MakeInactive()
            {
                BindIndex = BindGroupInfo{}.BindIndex;
            }
        };
        // Bind groups for each resource signature.
        std::array<std::array<BindGroupInfo, PipelineResourceSignatureWebGPUImpl::MAX_BIND_GROUPS>, MAX_RESOURCE_SIGNATURES> BindGroups = {};

        void Reset()
        {
            *this = WebGPUResourceBindInfo{};
        }
    } m_BindInfo;

    struct PendingClears
    {
        using RenderTargetClearColors = std::array<float[4], MAX_RENDER_TARGETS>;

        void SetColor(Uint32 RTIndex, const float Color[])
        {
            for (int i = 0; i < 4; ++i)
                Colors[RTIndex][i] = Color[i];
            Flags |= RT0Flag << RTIndex;
        }
        void SetDepth(float _Depth)
        {
            Depth = _Depth;
            Flags |= DepthFlag;
        }
        void SetStencil(Uint8 _Stencil)
        {
            Stencil = _Stencil;
            Flags |= StencilFlag;
        }

        bool ColorPending(Uint32 RTIndex) const
        {
            return Flags & (RT0Flag << RTIndex);
        }
        bool DepthPending() const
        {
            return Flags & DepthFlag;
        }
        bool StencilPending() const
        {
            return Flags & StencilFlag;
        }
        bool AnyPending() const
        {
            return Flags != 0;
        }

        void ResetFlags()
        {
            Flags = 0;
        }

        void Clear()
        {
            *this = PendingClears{};
        }

        RenderTargetClearColors Colors  = {};
        float                   Depth   = 0;
        Uint8                   Stencil = 0;

    private:
        static constexpr Uint32 RT0Flag     = 1U;
        static constexpr Uint32 DepthFlag   = 1U << MAX_RENDER_TARGETS;
        static constexpr Uint32 StencilFlag = 1U << (MAX_RENDER_TARGETS + 1);

        Uint32 Flags = 0;
    } m_PendingClears;

    struct PendingQuery
    {
        QueryWebGPUImpl* const pQuery;
        const bool             IsBegin;
    };

    struct MappedTextureKey
    {
        const UniqueIdentifier TextureId;
        const Uint32           MipLevel;
        const Uint32           ArraySlice;

        bool operator==(const MappedTextureKey& RHS) const
        {
            // clang-format off
            return TextureId  == RHS.TextureId &&
                   MipLevel   == RHS.MipLevel  &&
                   ArraySlice == RHS.ArraySlice;
            // clang-format on
        }
        struct Hasher
        {
            size_t operator()(const MappedTextureKey& Key) const
            {
                return ComputeHash(Key.TextureId, Key.MipLevel, Key.ArraySlice);
            }
        };
    };

    struct MappedTexture
    {
        BufferToTextureCopyInfo               CopyInfo;
        UploadMemoryManagerWebGPU::Allocation Allocation;
    };

    using PendingFenceList        = std::vector<std::pair<Uint64, RefCntAutoPtr<FenceWebGPUImpl>>>;
    using PendingQueryList        = std::vector<PendingQuery>;
    using AttachmentClearList     = std::vector<OptimizedClearValue>;
    using UploadMemoryPageList    = std::vector<UploadMemoryManagerWebGPU::Page>;
    using DynamicMemoryPageList   = std::vector<DynamicMemoryManagerWebGPU::Page>;
    using MappedTextureCache      = std::unordered_map<MappedTextureKey, MappedTexture, MappedTextureKey::Hasher>;
    using DebugGroupStack         = std::vector<DEBUG_GROUP_TYPE>;
    using OcclusionQueryStack     = std::vector<std::pair<OCCLUSION_QUERY_TYPE, Uint32>>;
    using PendingStagingResources = std::unordered_map<WebGPUResourceBase::StagingBufferInfo*, RefCntAutoPtr<IObject>>;

    WebGPUQueueWrapper              m_wgpuQueue;
    WebGPUCommandEncoderWrapper     m_wgpuCommandEncoder;
    WebGPURenderPassEncoderWrapper  m_wgpuRenderPassEncoder;
    WebGPUComputePassEncoderWrapper m_wgpuComputePassEncoder;

    PendingFenceList        m_SignaledFences;
    AttachmentClearList     m_AttachmentClearValues;
    PendingQueryList        m_PendingTimeQueries;
    UploadMemoryPageList    m_UploadMemPages;
    DynamicMemoryPageList   m_DynamicMemPages;
    MappedTextureCache      m_MappedTextures;
    DebugGroupStack         m_DebugGroupsStack;
    DebugGroupStack         m_PendingDebugGroups;
    OcclusionQueryStack     m_OcclusionQueriesStack;
    PendingStagingResources m_PendingStagingReads;
    PendingStagingResources m_PendingStagingWrites;

    RefCntAutoPtr<IFence> m_pFence;
    Uint64                m_FenceValue = 0;
};

} // namespace Diligent
