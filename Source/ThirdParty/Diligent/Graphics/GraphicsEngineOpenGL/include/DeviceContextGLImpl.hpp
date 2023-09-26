/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <vector>

#include "EngineGLImplTraits.hpp"
#include "DeviceContextBase.hpp"

// GL object implementations are required by DeviceContextBase
#include "BufferGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "QueryGLImpl.hpp"
#include "FramebufferGLImpl.hpp"
#include "RenderPassGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"

#include "GLContextState.hpp"
#include "GLObjectWrapper.hpp"

namespace Diligent
{

/// Device context implementation in OpenGL backend.
class DeviceContextGLImpl final : public DeviceContextBase<EngineGLImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextBase<EngineGLImplTraits>;

    DeviceContextGLImpl(IReferenceCounters*      pRefCounters,
                        RenderDeviceGLImpl*      pDeviceGL,
                        const DeviceContextDesc& Desc);

    /// Queries the specific interface, see IObject::QueryInterface() for details.
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceContext::Begin() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Begin(Uint32 ImmediateContextId) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer**                      ppBuffers,
                                                     const Uint64*                  pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint64                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargetsExt() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BeginRenderPass() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::NextSubpass() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE NextSubpass() override final;

    /// Implementation of IDeviceContext::EndRenderPass() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE EndRenderPass() override final;

    // clang-format off

    /// Implementation of IDeviceContext::Draw() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Draw               (const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed        (const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect       (const DrawIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMesh() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh           (const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect   (const DrawMeshIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::DispatchCompute() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute        (const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs) override final;

    // clang-format on

    /// Implementation of IDeviceContext::ClearDepthStencil() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView*                  pView,
                                                      const float*                   RGBA,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint64                         Offset,
                                                 Uint64                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint64                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint64                         DstOffset,
                                               Uint64                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::GenerateMips() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override;

    /// Implementation of IDeviceContext::FinishFrame() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE FinishCommandList(ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandLists() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandLists(Uint32               NumCommandLists,
                                                        ICommandList* const* ppCommandLists) override final;

    /// Implementation of IDeviceContext::EnqueueSignal() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE EnqueueSignal(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::DeviceWaitForFence() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DeviceWaitForFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContext::BuildBLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BuildBLAS(const BuildBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::BuildTLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BuildTLAS(const BuildTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyBLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyBLAS(const CopyBLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::CopyTLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyTLAS(const CopyTLASAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteBLASCompactedSize() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::WriteTLASCompactedSize() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRays() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TraceRays(const TraceRaysAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::TraceRaysIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::UpdateSBT() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs) override final;

    /// Implementation of IDeviceContext::BeginDebugGroup() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BeginDebugGroup(const Char* Name, const float* pColor) override final;

    /// Implementation of IDeviceContext::EndDebugGroup() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE EndDebugGroup() override final;

    /// Implementation of IDeviceContext::InsertDebugLabel() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE InsertDebugLabel(const Char* Label, const float* pColor) override final;

    /// Implementation of IDeviceContext::LockCommandQueue() in OpenGL backend.
    virtual ICommandQueue* DILIGENT_CALL_TYPE LockCommandQueue() override final { return nullptr; }

    /// Implementation of IDeviceContext::UnlockCommandQueue() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UnlockCommandQueue() override final {}

    /// Implementation of IDeviceContext::SetShadingRate() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetShadingRate(SHADING_RATE          BaseRate,
                                                   SHADING_RATE_COMBINER PrimitiveCombiner,
                                                   SHADING_RATE_COMBINER TextureCombiner) override final;

    /// Implementation of IDeviceContext::BindSparseResourceMemory() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs) override final;

    /// Implementation of IDeviceContextGL::UpdateCurrentGLContext().
    virtual bool DILIGENT_CALL_TYPE UpdateCurrentGLContext() override final;

    GLContextState& GetContextState() { return m_ContextState; }

    void CommitRenderTargets();

    virtual void DILIGENT_CALL_TYPE SetSwapChain(ISwapChainGL* pSwapChain) override final;

    virtual void ResetRenderTargets() override final;


protected:
    friend class BufferGLImpl;
    friend class TextureBaseGL;
    friend class ShaderGLImpl;

    GLContextState m_ContextState;

private:
    __forceinline void PrepareForDraw(DRAW_FLAGS Flags, bool IsIndexed, GLenum& GlTopology);
    __forceinline void PrepareForIndexedDraw(VALUE_TYPE IndexType, Uint32 FirstIndexLocation, GLenum& GLIndexType, size_t& FirstIndexByteOffset);
    __forceinline void PrepareForIndirectDraw(IBuffer* pAttribsBuffer);
    __forceinline void PrepareForIndirectDrawCount(IBuffer* pCountBuffer);
    __forceinline void PostDraw();

    using TBindings = PipelineResourceSignatureGLImpl::TBindings;
    void BindProgramResources(Uint32 BindSRBMask);

#ifdef DILIGENT_DEVELOPMENT
    void DvpValidateCommittedShaderResources();
#endif

    void BeginSubpass();
    void EndSubpass();

    struct BindInfo : CommittedShaderResources
    {
#ifdef DILIGENT_DEVELOPMENT
        // Binding offsets that were used in the last BindProgramResources() call.
        std::array<TBindings, MAX_RESOURCE_SIGNATURES> BaseBindings = {};
#endif

        BindInfo()
        {}

        void Invalidate()
        {
            *this = {};
        }
    } m_BindInfo;

    MEMORY_BARRIER m_CommittedResourcesTentativeBarriers = MEMORY_BARRIER_NONE;

    std::vector<class TextureBaseGL*> m_BoundWritableTextures;
    std::vector<class BufferGLImpl*>  m_BoundWritableBuffers;

    RefCntAutoPtr<ISwapChainGL> m_pSwapChain;

    bool m_IsDefaultFBOBound = false;

    GLObjectWrappers::GLFrameBufferObj m_DefaultFBO;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;
};

} // namespace Diligent
