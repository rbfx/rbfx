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
/// Declaration of Diligent::DeviceContextD3D11Impl class

#include <vector>

#include "EngineD3D11ImplTraits.hpp"
#include "DeviceContextBase.hpp"
#include "BufferD3D11Impl.hpp"
#include "TextureBaseD3D11.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "QueryD3D11Impl.hpp"
#include "FramebufferD3D11Impl.hpp"
#include "RenderPassD3D11Impl.hpp"
#include "DisjointQueryPool.hpp"
#include "BottomLevelASBase.hpp"
#include "TopLevelASBase.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"

namespace Diligent
{

/// Device context implementation in Direct3D11 backend.
class DeviceContextD3D11Impl final : public DeviceContextBase<EngineD3D11ImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextBase<EngineD3D11ImplTraits>;

    DeviceContextD3D11Impl(IReferenceCounters*          pRefCounters,
                           IMemoryAllocator&            Allocator,
                           RenderDeviceD3D11Impl*       pDevice,
                           ID3D11DeviceContext1*        pd3d11DeviceContext,
                           const EngineD3D11CreateInfo& EngineCI,
                           const DeviceContextDesc&     Desc);
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceContext::Begin() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Begin(Uint32 ImmediateContextId) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer* const*                ppBuffers,
                                                     const Uint64*                  pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint64                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargetsExt() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BeginRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::NextSubpass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE NextSubpass() override final;

    /// Implementation of IDeviceContext::EndRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EndRenderPass() override final;

    /// Implementation of IDeviceContext::Draw() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Draw(const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed(const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect(const DrawIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMesh() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh(const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::MultiDraw() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MultiDraw(const MultiDrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::MultiDrawIndexed() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DispatchCompute() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute(const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::ClearDepthStencil() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView* pView, const void* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint64                         Offset,
                                                 Uint64                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint64                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint64                         DstOffset,
                                               Uint64                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::GenerateMips() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTextureView) override final;

    /// Implementation of IDeviceContext::FinishFrame() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in Direct3D11 backend.
    void DILIGENT_CALL_TYPE FinishCommandList(ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandLists() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandLists(Uint32               NumCommandLists,
                                                        ICommandList* const* ppCommandLists) override final;

    /// Implementation of IDeviceContext::EnqueueSignal() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EnqueueSignal(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::DeviceWaitForFence() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DeviceWaitForFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContext::BuildBLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BuildBLAS(const BuildBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BuildTLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BuildTLAS(const BuildTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyBLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyBLAS(const CopyBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyTLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyTLAS(const CopyTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteBLASCompactedSize() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteTLASCompactedSize() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRays() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TraceRays(const TraceRaysAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRaysIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::UpdateSBT() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs) override final;

    /// Implementation of IDeviceContext::BeginDebugGroup() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BeginDebugGroup(const Char* Name, const float* pColor) override final;

    /// Implementation of IDeviceContext::EndDebugGroup() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EndDebugGroup() override final;

    /// Implementation of IDeviceContext::InsertDebugLabel() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE InsertDebugLabel(const Char* Label, const float* pColor) override final;

    /// Implementation of IDeviceContext::LockCommandQueue() in Direct3D11 backend.
    virtual ICommandQueue* DILIGENT_CALL_TYPE LockCommandQueue() override final { return nullptr; }

    /// Implementation of IDeviceContext::UnlockCommandQueue() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UnlockCommandQueue() override final {}

    /// Implementation of IDeviceContext::SetShadingRate() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetShadingRate(SHADING_RATE          BaseRate,
                                                   SHADING_RATE_COMBINER PrimitiveCombiner,
                                                   SHADING_RATE_COMBINER TextureCombiner) override final;

    /// Implementation of IDeviceContext::BindSparseResourceMemory() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs) override final;

    /// Implementation of IDeviceContextD3D11::GetD3D11DeviceContext().
    virtual ID3D11DeviceContext* DILIGENT_CALL_TYPE GetD3D11DeviceContext() override final { return m_pd3d11DeviceContext; }

    void CommitRenderTargets();

    /// Clears committed shader resource cache. This function
    /// is called once per frame (before present) to release all
    /// outstanding objects that are only kept alive by references
    /// in the cache. The function does not release cached vertex and
    /// index buffers, input layout, depth-stencil, rasterizer, and blend
    /// states.
    void ReleaseCommittedShaderResources();

    /// Unbinds all render targets. Used when resizing the swap chain.
    virtual void ResetRenderTargets() override final;

    void TransitionResource(TextureBaseD3D11& Texture, RESOURCE_STATE NewState, RESOURCE_STATE OldState = RESOURCE_STATE_UNKNOWN, bool UpdateResourceState = true);
    void TransitionResource(BufferD3D11Impl& Buffer, RESOURCE_STATE NewState, RESOURCE_STATE OldState = RESOURCE_STATE_UNKNOWN, bool UpdateResourceState = true);

    bool ResizeTilePool(ID3D11Buffer* pBuffer, UINT NewSize);

private:
    /// Commits d3d11 index buffer to d3d11 device context.
    void CommitD3D11IndexBuffer(VALUE_TYPE IndexType);

    /// Commits d3d11 vertex buffers to d3d11 device context.
    void CommitD3D11VertexBuffers(class PipelineStateD3D11Impl* pPipelineStateD3D11);

    /// Helper template function used to facilitate resource unbinding
    template <typename TD3D11ResourceViewType,
              typename TSetD3D11View,
              size_t NumSlots>
    void UnbindResourceView(TD3D11ResourceViewType CommittedD3D11ViewsArr[][NumSlots],
                            ID3D11Resource*        CommittedD3D11ResourcesArr[][NumSlots],
                            Uint8                  NumCommittedResourcesArr[],
                            ID3D11Resource*        pd3d11ResToUndind,
                            TSetD3D11View          SetD3D11ViewMethods[]);

    /// Unbinds a texture from shader resource view slots.
    /// \note The function only unbinds the texture from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindTextureFromInput(TextureBaseD3D11& Texture, ID3D11Resource* pd3d11Resource);

    /// Unbinds a buffer from input (shader resource views slots, index
    /// and vertex buffer slots).
    /// \note The function only unbinds the buffer from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindBufferFromInput(BufferD3D11Impl& Buffer, RESOURCE_STATE OldState, ID3D11Resource* pd3d11Buffer);

    /// Unbinds a resource from UAV slots.
    /// \note The function only unbinds the resource from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindResourceFromUAV(ID3D11Resource* pd3d11Resource);

    /// Unbinds a texture from render target slots.
    void UnbindTextureFromRenderTarget(TextureBaseD3D11& Resource);

    /// Unbinds a texture from depth-stencil.
    void UnbindTextureFromDepthStencil(TextureBaseD3D11& TexD3D11);

    /// Prepares for a draw command
    __forceinline void PrepareForDraw(DRAW_FLAGS Flags);

    /// Prepares for an indexed draw command
    __forceinline void PrepareForIndexedDraw(DRAW_FLAGS Flags, VALUE_TYPE IndexType);

    /// Performs operations required to begin current subpass (e.g. bind render targets)
    void BeginSubpass();
    /// Ends current subpass
    void EndSubpass();

    void ClearStateCache();

    void BindShaderResources(Uint32 BindSRBMask);

    static constexpr int NumShaderTypes = D3D11ResourceBindPoints::NumShaderTypes;
    struct TCommittedResources
    {
        // clang-format off

        /// An array of D3D11 constant buffers committed to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11Buffer*              d3d11CBs     [NumShaderTypes][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};

        /// An array of D3D11 shader resource views committed to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11ShaderResourceView*  d3d11SRVs    [NumShaderTypes][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

        /// An array of D3D11 samplers committed to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11SamplerState*        d3d11Samplers[NumShaderTypes][D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};

        /// An array of D3D11 UAVs committed to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11UnorderedAccessView* d3d11UAVs    [NumShaderTypes][D3D11_PS_CS_UAV_REGISTER_COUNT] = {};

        /// An array of D3D11 resources committed as SRV to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11Resource*  d3d11SRVResources      [NumShaderTypes][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

        /// An array of D3D11 resources committed as UAV to D3D11 device context,
        /// for each shader type. The context addref's all bound resources, so we do
        /// not need to keep strong references.
        ID3D11Resource*  d3d11UAVResources      [NumShaderTypes][D3D11_PS_CS_UAV_REGISTER_COUNT] = {};


        /// An array of the first D3D11 constant buffer constants committed to D3D11 device context,
        /// for each shader type.
        UINT             CBFirstConstants       [NumShaderTypes][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};

        /// An array of the number of D3D11 constant buffer constants committed to D3D11 device context,
        /// for each shader type.
        UINT             CBNumConstants         [NumShaderTypes][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};


        Uint8 NumCBs     [NumShaderTypes] = {};
        Uint8 NumSRVs    [NumShaderTypes] = {};
        Uint8 NumSamplers[NumShaderTypes] = {};
        Uint8 NumUAVs    [NumShaderTypes] = {};

        // clang-format on

        void Clear()
        {
            *this = {};
        }
    };

    enum class PixelShaderUAVBindMode
    {
        Clear = 0,
        Keep,
        Bind
    };

    // Binds all shader resource cache resources
    void BindCacheResources(const ShaderResourceCacheD3D11&    ResourceCache,
                            const D3D11ShaderResourceCounters& BaseBindings,
                            PixelShaderUAVBindMode&            PsUavBindMode);

    // Binds constant buffers with dynamic offsets only
    void BindDynamicCBs(const ShaderResourceCacheD3D11&    ResourceCache,
                        const D3D11ShaderResourceCounters& BaseBindings);

#ifdef DILIGENT_DEVELOPMENT
    void DvpValidateCommittedShaderResources();
#endif

    std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> BeginDisjointQuery();

    CComPtr<ID3D11DeviceContext1> m_pd3d11DeviceContext; ///< D3D11 device context

    struct BindInfo : CommittedShaderResources
    {
        // Shader stages that are active in current PSO.
        SHADER_TYPE ActiveStages = SHADER_TYPE_UNKNOWN;

#ifdef DILIGENT_DEVELOPMENT
        // Base bindings that were used in the last BindShaderResources() call.
        std::array<D3D11ShaderResourceCounters, MAX_RESOURCE_SIGNATURES> BaseBindings = {};
#endif
        void Invalidate()
        {
            *this = {};
        }
    } m_BindInfo;

    TCommittedResources m_CommittedRes;

    /// An array of D3D11 vertex buffers committed to D3D device context.
    /// There is no need to keep strong references because D3D11 device context
    /// already does. Buffers cannot be destroyed while bound to the context.
    /// We only mirror all bindings.
    ID3D11Buffer* m_CommittedD3D11VertexBuffers[MAX_BUFFER_SLOTS] = {};
    /// An array of strides of committed vertex buffers
    UINT m_CommittedD3D11VBStrides[MAX_BUFFER_SLOTS] = {};
    /// An array of offsets of committed vertex buffers
    UINT m_CommittedD3D11VBOffsets[MAX_BUFFER_SLOTS] = {};
    /// Number committed vertex buffers
    UINT m_NumCommittedD3D11VBs = 0;
    /// Flag indicating if currently committed D3D11 vertex buffers are up to date
    bool m_bCommittedD3D11VBsUpToDate = false;

    /// D3D11 input layout committed to device context.
    /// The context keeps the layout alive, so there is no need
    /// to keep strong reference.
    ID3D11InputLayout* m_CommittedD3D11InputLayout = nullptr;

    /// Strong reference to D3D11 buffer committed as index buffer
    /// to D3D device context.
    CComPtr<ID3D11Buffer> m_CommittedD3D11IndexBuffer;
    /// Format of the committed D3D11 index buffer
    VALUE_TYPE m_CommittedIBFormat = VT_UNDEFINED;
    /// Offset of the committed D3D11 index buffer
    Uint32 m_CommittedD3D11IndexDataStartOffset = 0;
    /// Flag indicating if currently committed D3D11 index buffer is up to date
    bool m_bCommittedD3D11IBUpToDate = false;

    D3D11_PRIMITIVE_TOPOLOGY m_CommittedD3D11PrimTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    PRIMITIVE_TOPOLOGY       m_CommittedPrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;

    /// Strong references to committed D3D11 shaders
    CComPtr<ID3D11DeviceChild> m_CommittedD3DShaders[NumShaderTypes];

#ifdef DILIGENT_DEVELOPMENT
    const D3D11_VALIDATION_FLAGS m_D3D11ValidationFlags;
#endif

    FixedBlockMemoryAllocator m_CmdListAllocator;

    DisjointQueryPool                                        m_DisjointQueryPool;
    std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> m_ActiveDisjointQuery;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;

#ifdef DILIGENT_DEVELOPMENT

    /// Helper template function used to facilitate context verification
    template <UINT MaxResources, typename TD3D11ResourceType, typename TGetD3D11ResourcesType>
    void DvpVerifyCommittedResources(TD3D11ResourceType     CommittedD3D11ResourcesArr[][MaxResources],
                                     Uint8                  NumCommittedResourcesArr[],
                                     TGetD3D11ResourcesType GetD3D11ResMethods[],
                                     const Char*            ResourceName,
                                     SHADER_TYPE            ShaderStages);

    /// Helper template function used to facilitate validation of SRV and UAV consistency with D3D11 resources
    template <UINT MaxResources, typename TD3D11ViewType>
    void DvpVerifyViewConsistency(TD3D11ViewType  CommittedD3D11ViewArr[][MaxResources],
                                  ID3D11Resource* CommittedD3D11ResourcesArr[][MaxResources],
                                  Uint8           NumCommittedResourcesArr[],
                                  const Char*     ResourceName,
                                  SHADER_TYPE     ShaderStages);

    /// Debug function that verifies that SRVs cached in m_CommittedD3D11SRVs
    /// array comply with resources actually committed to D3D11 device context
    void DvpVerifyCommittedSRVs(SHADER_TYPE ShaderStages);

    /// Debug function that verifies that UAVs cached in m_CommittedD3D11UAVs
    /// array comply with resources actually committed to D3D11 device context
    void DvpVerifyCommittedUAVs(SHADER_TYPE ShaderStages);

    /// Debug function that verifies that samplers cached in m_CommittedD3D11Samplers
    /// array comply with resources actually committed to D3D11 device context
    void DvpVerifyCommittedSamplers(SHADER_TYPE ShaderStages);

    /// Debug function that verifies that constant buffers cached in m_CommittedD3D11CBs
    /// array comply with buffers actually committed to D3D11 device context
    void DvpVerifyCommittedCBs(SHADER_TYPE ShaderStages);

    /// Debug function that verifies that index buffer cached in
    /// m_CommittedD3D11IndexBuffer is the buffer actually committed to D3D11
    /// device context
    void DvpVerifyCommittedIndexBuffer();

    /// Debug function that verifies that vertex buffers cached in
    /// m_CommittedD3D11VertexBuffers are the buffers actually committed to D3D11
    /// device context
    void DvpVerifyCommittedVertexBuffers();

    /// Debug function that verifies that shaders cached in
    /// m_CommittedD3DShaders are the shaders actually committed to D3D11
    /// device context
    void DvpVerifyCommittedShaders();

#endif // DILIGENT_DEVELOPMENT
};

} // namespace Diligent
