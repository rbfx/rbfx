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

#pragma once

/// \file
/// Declaration of Diligent::DeviceContextD3D12Impl class

#include <unordered_map>
#include <vector>

#include "EngineD3D12ImplTraits.hpp"
#include "DeviceContextNextGenBase.hpp"

// D3D12 object implementations are required by DeviceContextBase
#include "BufferD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "QueryD3D12Impl.hpp"
#include "FramebufferD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "BottomLevelASD3D12Impl.hpp"
#include "TopLevelASD3D12Impl.hpp"

#include "D3D12DynamicHeap.hpp"

namespace Diligent
{

class CommandContext;
class GraphicsContext;
class ComputeContext;

/// Device context implementation in Direct3D12 backend.
class DeviceContextD3D12Impl final : public DeviceContextNextGenBase<EngineD3D12ImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextNextGenBase<EngineD3D12ImplTraits>;

    DeviceContextD3D12Impl(IReferenceCounters*          pRefCounters,
                           RenderDeviceD3D12Impl*       pDevice,
                           const EngineD3D12CreateInfo& EngineCI,
                           const DeviceContextDesc&     Desc);
    ~DeviceContextD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceContextD3D12, TDeviceContextBase)

    /// Implementation of IDeviceContext::Begin() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Begin(Uint32 ImmediateContextId) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer* const*                ppBuffers,
                                                     const Uint64*                  pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint64                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargetsExt() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BeginRenderPass() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::NextSubpass() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE NextSubpass() override final;

    /// Implementation of IDeviceContext::EndRenderPass() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE EndRenderPass() override final;

    // clang-format off
    /// Implementation of IDeviceContext::Draw() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Draw               (const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed        (const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect       (const DrawIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMesh() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh           (const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect   (const DrawMeshIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::MultiDraw() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MultiDraw          (const MultiDrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::MultiDrawIndexed() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MultiDrawIndexed   (const MultiDrawIndexedAttribs& Attribs) override final;


    /// Implementation of IDeviceContext::DispatchCompute() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute        (const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs) override final;
    // clang-format on

    /// Implementation of IDeviceContext::ClearDepthStencil() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView*                  pView,
                                                      const void*                    RGBA,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint64                         Offset,
                                                 Uint64                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint64                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint64                         DstOffset,
                                               Uint64                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer*  pBuffer,
                                              MAP_TYPE  MapType,
                                              MAP_FLAGS MapFlags,
                                              PVoid&    pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::FinishFrame() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE FinishCommandList(ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandLists() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandLists(Uint32               NumCommandLists,
                                                        ICommandList* const* ppCommandLists) override final;

    /// Implementation of IDeviceContext::EnqueueSignal() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE EnqueueSignal(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::DeviceWaitForFence() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DeviceWaitForFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContext::TransitionTextureState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionTextureState(ITexture* pTexture, D3D12_RESOURCE_STATES State) override final;

    /// Implementation of IDeviceContext::TransitionBufferState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionBufferState(IBuffer* pBuffer, D3D12_RESOURCE_STATES State) override final;

    /// Implementation of IDeviceContext::BuildBLAS() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BuildBLAS(const BuildBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BuildTLAS() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BuildTLAS(const BuildTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyBLAS() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyBLAS(const CopyBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyTLAS() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyTLAS(const CopyTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteBLASCompactedSize() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteTLASCompactedSize() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRays() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TraceRays(const TraceRaysAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRaysIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::UpdateSBT() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs) override final;

    /// Implementation of IDeviceContext::BeginDebugGroup() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BeginDebugGroup(const Char* Name, const float* pColor) override final;

    /// Implementation of IDeviceContext::EndDebugGroup() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE EndDebugGroup() override final;

    /// Implementation of IDeviceContext::InsertDebugLabel() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE InsertDebugLabel(const Char* Label, const float* pColor) override final;

    /// Implementation of IDeviceContextD3D12::ID3D12GraphicsCommandList() in Direct3D12 backend.
    virtual ID3D12GraphicsCommandList* DILIGENT_CALL_TYPE GetD3D12CommandList() override final;

    /// Implementation of IDeviceContext::SetShadingRate() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetShadingRate(SHADING_RATE          BaseRate,
                                                   SHADING_RATE_COMBINER PrimitiveCombiner,
                                                   SHADING_RATE_COMBINER TextureCombiner) override final;

    /// Implementation of IDeviceContext::BindSparseResourceMemory() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs) override final;

    void UpdateBufferRegion(class BufferD3D12Impl*         pBuffD3D12,
                            D3D12DynamicAllocation&        Allocation,
                            Uint64                         DstOffset,
                            Uint64                         NumBytes,
                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode);

    void CopyTextureRegion(class TextureD3D12Impl*        pSrcTexture,
                           Uint32                         SrcSubResIndex,
                           const D3D12_BOX*               pD3D12SrcBox,
                           RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                           class TextureD3D12Impl*        pDstTexture,
                           Uint32                         DstSubResIndex,
                           Uint32                         DstX,
                           Uint32                         DstY,
                           Uint32                         DstZ,
                           RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode);

    void CopyTextureRegion(IBuffer*                       pSrcBuffer,
                           Uint64                         SrcOffset,
                           Uint64                         SrcStride,
                           Uint64                         SrcDepthStride,
                           class TextureD3D12Impl&        TextureD3D12,
                           Uint32                         DstSubResIndex,
                           const Box&                     DstBox,
                           RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode,
                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);
    void CopyTextureRegion(ID3D12Resource*                pd3d12Buffer,
                           Uint64                         SrcOffset,
                           Uint64                         SrcStride,
                           Uint64                         SrcDepthStride,
                           Uint64                         BufferSize,
                           class TextureD3D12Impl&        TextureD3D12,
                           Uint32                         DstSubResIndex,
                           const Box&                     DstBox,
                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);

    void UpdateTextureRegion(const void*                    pSrcData,
                             Uint64                         SrcStride,
                             Uint64                         SrcDepthStride,
                             class TextureD3D12Impl&        TextureD3D12,
                             Uint32                         DstSubResIndex,
                             const Box&                     DstBox,
                             RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);

    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override final;

    D3D12DynamicAllocation AllocateDynamicSpace(Uint64 NumBytes, Uint32 Alignment);

    size_t GetNumCommandsInCtx() const { return m_State.NumCommands; }

    QueryManagerD3D12& GetQueryManager()
    {
        VERIFY(m_QueryMgr != nullptr || IsDeferred(), "Query manager should never be null for immediate contexts. This might be a bug.");
        DEV_CHECK_ERR(m_QueryMgr != nullptr, "Query manager is null, which indicates that this deferred context is not in a recording state");
        return *m_QueryMgr;
    }

private:
    void CommitD3D12IndexBuffer(GraphicsContext& GraphCtx, VALUE_TYPE IndexType);
    void CommitD3D12VertexBuffers(GraphicsContext& GraphCtx);
    void CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode);
    void CommitViewports();
    void CommitScissorRects(GraphicsContext& GraphCtx, bool ScissorEnable);
    void TransitionSubpassAttachments(Uint32 NextSubpass);
    void CommitSubpassRenderTargets();
    void Flush(bool                 RequestNewCmdCtx,
               Uint32               NumCommandLists = 0,
               ICommandList* const* ppCommandLists  = nullptr);

    __forceinline void RequestCommandContext();

    __forceinline void TransitionOrVerifyBufferState(CommandContext&                CmdCtx,
                                                     BufferD3D12Impl&               Buffer,
                                                     RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                     RESOURCE_STATE                 RequiredState,
                                                     const char*                    OperationName);
    __forceinline void TransitionOrVerifyTextureState(CommandContext&                CmdCtx,
                                                      TextureD3D12Impl&              Texture,
                                                      RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                      RESOURCE_STATE                 RequiredState,
                                                      const char*                    OperationName);
    __forceinline void TransitionOrVerifyBLASState(CommandContext&                CmdCtx,
                                                   BottomLevelASD3D12Impl&        BLAS,
                                                   RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                   RESOURCE_STATE                 RequiredState,
                                                   const char*                    OperationName);
    __forceinline void TransitionOrVerifyTLASState(CommandContext&                CmdCtx,
                                                   TopLevelASD3D12Impl&           TLAS,
                                                   RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                   RESOURCE_STATE                 RequiredState,
                                                   const char*                    OperationName);

    __forceinline void PrepareForDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags);

    __forceinline void PrepareForIndexedDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags, VALUE_TYPE IndexType);

    __forceinline void PrepareForDispatchCompute(ComputeContext& GraphCtx);
    __forceinline void PrepareForDispatchRays(GraphicsContext& GraphCtx);

    __forceinline void PrepareIndirectAttribsBuffer(CommandContext&                CmdCtx,
                                                    IBuffer*                       pAttribsBuffer,
                                                    RESOURCE_STATE_TRANSITION_MODE BufferStateTransitionMode,
                                                    ID3D12Resource*&               pd3d12ArgsBuff,
                                                    Uint64&                        BuffDataStartByteOffset,
                                                    const char*                    OpName);

    struct RootTableInfo : CommittedShaderResources
    {
        ID3D12RootSignature* pd3d12RootSig = nullptr;
    };
    __forceinline RootTableInfo& GetRootTableInfo(PIPELINE_TYPE PipelineType);

    template <bool IsCompute>
    __forceinline void CommitRootTablesAndViews(RootTableInfo& RootInfo, Uint32 CommitSRBMask, CommandContext& CmdCtx) const;

#ifdef DILIGENT_DEVELOPMENT
    void DvpValidateCommittedShaderResources(RootTableInfo& RootInfo) const;
#endif

    ID3D12CommandSignature* GetDrawIndirectSignature(Uint32 Stride);
    ID3D12CommandSignature* GetDrawIndexedIndirectSignature(Uint32 Stride);

    struct TextureUploadSpace
    {
        D3D12DynamicAllocation Allocation;
        Uint32                 AlignedOffset = 0;
        Uint64                 Stride        = 0;
        Uint64                 DepthStride   = 0;
        Uint64                 RowSize       = 0;
        Uint32                 RowCount      = 0;
        Box                    Region;
    };
    TextureUploadSpace AllocateTextureUploadSpace(TEXTURE_FORMAT TexFmt,
                                                  const Box&     Region);


    friend class SwapChainD3D12Impl;
    inline CommandContext& GetCmdContext()
    {
        // Make sure that the number of commands in the context is at least one,
        // so that the context cannot be disposed by Flush()
        m_State.NumCommands = m_State.NumCommands != 0 ? m_State.NumCommands : 1;
        return *m_CurrCmdCtx;
    }
    std::unique_ptr<CommandContext, STDDeleterRawMem<CommandContext>> m_CurrCmdCtx;

    struct State
    {
        size_t NumCommands = 0;

        CComPtr<ID3D12Resource> CommittedD3D12IndexBuffer;
        VALUE_TYPE              CommittedIBFormat                  = VT_UNDEFINED;
        Uint64                  CommittedD3D12IndexDataStartOffset = 0;

        // Indicates if currently committed D3D12 vertex buffers are up to date
        bool bCommittedD3D12VBsUpToDate = false;

        // Indicates if currently committed D3D12 index buffer is up to date
        bool bCommittedD3D12IBUpToDate = false;

        // Indicates if custom shading rate is set in the command list
        bool bUsingShadingRate = false;

        // Indicates if shading rate map was bound in previous CommitRenderTargets() call
        bool bShadingRateMapBound = false;
    } m_State;

    RootTableInfo m_GraphicsResources;
    RootTableInfo m_ComputeResources;

    std::unordered_map<Uint32, CComPtr<ID3D12CommandSignature>> m_pDrawIndirectSignatureMap;
    std::unordered_map<Uint32, CComPtr<ID3D12CommandSignature>> m_pDrawIndexedIndirectSignatureMap;
    CComPtr<ID3D12CommandSignature>                             m_pDispatchIndirectSignature;
    CComPtr<ID3D12CommandSignature>                             m_pDrawMeshIndirectSignature;
    CComPtr<ID3D12CommandSignature>                             m_pTraceRaysIndirectSignature;

    D3D12DynamicHeap m_DynamicHeap;

    // Every context must use its own allocator that maintains individual list of retired descriptor heaps to
    // avoid interference with other command contexts
    // The allocations in heaps are discarded at the end of the frame.
    DynamicSuballocationsManager m_DynamicGPUDescriptorAllocator[2];

    FixedBlockMemoryAllocator m_CmdListAllocator;

    std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>> m_SignalFences;
    std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>> m_WaitFences;

    struct MappedTextureKey
    {
        TextureD3D12Impl* const Texture;
        UINT const              Subresource;

        constexpr bool operator==(const MappedTextureKey& rhs) const
        {
            return Texture == rhs.Texture && Subresource == rhs.Subresource;
        }
        struct Hasher
        {
            size_t operator()(const MappedTextureKey& Key) const noexcept
            {
                return ComputeHash(Key.Texture, Key.Subresource);
            }
        };
    };
    std::unordered_map<MappedTextureKey, TextureUploadSpace, MappedTextureKey::Hasher> m_MappedTextures;

    Int32 m_ActiveQueriesCounter = 0;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;

    std::vector<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> m_AttachmentResolveInfo;

    QueryManagerD3D12* m_QueryMgr = nullptr;

    // Null render targets require a null RTV. NULL descriptor causes an error.
    DescriptorHeapAllocation m_NullRTV;
};

} // namespace Diligent
