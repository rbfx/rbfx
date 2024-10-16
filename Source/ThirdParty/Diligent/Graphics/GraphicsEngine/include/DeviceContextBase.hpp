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
/// Implementation of the Diligent::DeviceContextBase template class and related structures

#include <unordered_map>
#include <array>
#include <functional>
#include <vector>

#include "PrivateConstants.h"
#include "DeviceContext.h"
#include "DeviceObjectBase.hpp"
#include "ResourceMapping.h"
#include "Sampler.h"
#include "ObjectBase.hpp"
#include "DebugUtilities.hpp"
#include "Cast.hpp"
#include "GraphicsAccessories.hpp"
#include "TextureBase.hpp"
#include "IndexWrapper.hpp"
#include "BasicMath.hpp"
#include "PlatformMisc.hpp"
#include "Align.hpp"

namespace Diligent
{

// clang-format off
bool VerifyDrawAttribs               (const DrawAttribs&                Attribs);
bool VerifyDrawIndexedAttribs        (const DrawIndexedAttribs&         Attribs);
bool VerifyDrawIndirectAttribs       (const DrawIndirectAttribs&        Attribs);
bool VerifyDrawIndexedIndirectAttribs(const DrawIndexedIndirectAttribs& Attribs);
bool VerifyMultiDrawAttribs          (const MultiDrawAttribs&           Attribs);
bool VerifyMultiDrawIndexedAttribs   (const MultiDrawIndexedAttribs&    Attribs);

bool VerifyDispatchComputeAttribs        (const DispatchComputeAttribs&         Attribs);
bool VerifyDispatchComputeIndirectAttribs(const DispatchComputeIndirectAttribs& Attribs);
// clang-format on

bool VerifyDrawMeshAttribs(const MeshShaderProperties& MeshShaderProps, const DrawMeshAttribs& Attribs);
bool VerifyDrawMeshIndirectAttribs(const DrawMeshIndirectAttribs& Attribs, Uint32 IndirectCmdStride);

bool VerifyResolveTextureSubresourceAttribs(const ResolveTextureSubresourceAttribs& ResolveAttribs,
                                            const TextureDesc&                      SrcTexDesc,
                                            const TextureDesc&                      DstTexDesc);

bool VerifyBeginRenderPassAttribs(const BeginRenderPassAttribs& Attribs);

// Verifies state transition (resource barrier) description.
// ExecutionCtxId - index of the immediate context where the barrier will be executed.
// CtxDesc        - description of the context recording the command (deferred or immediate).
bool VerifyStateTransitionDesc(const IRenderDevice*       pDevice,
                               const StateTransitionDesc& Barrier,
                               DeviceContextIndex         ExecutionCtxId,
                               const DeviceContextDesc&   CtxDesc);

bool VerifyBuildBLASAttribs(const BuildBLASAttribs& Attribs, const IRenderDevice* pDevice);
bool VerifyBuildTLASAttribs(const BuildTLASAttribs& Attribs, const RayTracingProperties& RTProps);
bool VerifyCopyBLASAttribs(const IRenderDevice* pDevice, const CopyBLASAttribs& Attribs);
bool VerifyCopyTLASAttribs(const CopyTLASAttribs& Attribs);
bool VerifyWriteBLASCompactedSizeAttribs(const IRenderDevice* pDevice, const WriteBLASCompactedSizeAttribs& Attribs);
bool VerifyWriteTLASCompactedSizeAttribs(const IRenderDevice* pDevice, const WriteTLASCompactedSizeAttribs& Attribs);
bool VerifyTraceRaysAttribs(const TraceRaysAttribs& Attribs);
bool VerifyTraceRaysIndirectAttribs(const IRenderDevice*            pDevice,
                                    const TraceRaysIndirectAttribs& Attribs,
                                    Uint32                          SBTSize);

bool VerifyBindSparseResourceMemoryAttribs(const IRenderDevice* pDevice, const BindSparseResourceMemoryAttribs& Attribs);


/// Describes input vertex stream
template <typename BufferImplType>
struct VertexStreamInfo
{
    VertexStreamInfo() {}

    /// Strong reference to the buffer object
    RefCntAutoPtr<BufferImplType> pBuffer;

    /// Offset in bytes
    Uint64 Offset = 0;
};

/// Base implementation of the device context.

/// \tparam EngineImplTraits     - Engine implementation traits that define specific implementation details
///                                 (texture implementation type, buffer implementation type, etc.)
/// \remarks Device context keeps strong references to all objects currently bound to
///          the pipeline: buffers, textures, states, SRBs, etc.
///          The context also keeps strong references to the device and
///          the swap chain.
template <typename EngineImplTraits>
class DeviceContextBase : public ObjectBase<typename EngineImplTraits::DeviceContextInterface>
{
public:
    using BaseInterface                     = typename EngineImplTraits::DeviceContextInterface;
    using TObjectBase                       = ObjectBase<BaseInterface>;
    using DeviceImplType                    = typename EngineImplTraits::RenderDeviceImplType;
    using BufferImplType                    = typename EngineImplTraits::BufferImplType;
    using TextureImplType                   = typename EngineImplTraits::TextureImplType;
    using PipelineStateImplType             = typename EngineImplTraits::PipelineStateImplType;
    using ShaderResourceBindingImplType     = typename EngineImplTraits::ShaderResourceBindingImplType;
    using TextureViewImplType               = typename EngineImplTraits::TextureViewImplType;
    using QueryImplType                     = typename EngineImplTraits::QueryImplType;
    using FramebufferImplType               = typename EngineImplTraits::FramebufferImplType;
    using RenderPassImplType                = typename EngineImplTraits::RenderPassImplType;
    using BottomLevelASType                 = typename EngineImplTraits::BottomLevelASImplType;
    using TopLevelASType                    = typename EngineImplTraits::TopLevelASImplType;
    using ShaderBindingTableImplType        = typename EngineImplTraits::ShaderBindingTableImplType;
    using ShaderResourceCacheImplType       = typename EngineImplTraits::ShaderResourceCacheImplType;
    using PipelineResourceSignatureImplType = typename EngineImplTraits::PipelineResourceSignatureImplType;
    using DeviceContextImplType             = typename EngineImplTraits::DeviceContextImplType;

    /// \param pRefCounters  - Reference counters object that controls the lifetime of this device context.
    /// \param pRenderDevice - Render device.
    /// \param Desc          - Context description, see Diligent::DeviceContextDesc.
    DeviceContextBase(IReferenceCounters*      pRefCounters,
                      DeviceImplType*          pRenderDevice,
                      const DeviceContextDesc& Desc) :
        // clang-format off
        TObjectBase{pRefCounters },
        m_pDevice  {pRenderDevice},
        m_Name
        {
            Desc.Name != nullptr && *Desc.Name != '\0' ?
                String{Desc.Name} :
                String{"Context #"} + std::to_string(Uint32{Desc.ContextId}) + (Desc.IsDeferred ? " (deferred)" : " (immediate)")
        },
        m_Desc
        {
            m_Name.c_str(),
            Desc.IsDeferred ? COMMAND_QUEUE_TYPE_UNKNOWN : Desc.QueueType,
            Desc.IsDeferred,
            Desc.ContextId,
            Desc.QueueId
        },
        m_NativeMultiDrawSupported{pRenderDevice->GetDeviceInfo().Features.NativeMultiDraw != DEVICE_FEATURE_STATE_DISABLED}
    // clang-format on
    {
        VERIFY_EXPR(m_pDevice != nullptr);
    }

    ~DeviceContextBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceContext, TObjectBase)

    /// Implementation of IDeviceContext::GetDesc().
    virtual const DeviceContextDesc& DILIGENT_CALL_TYPE GetDesc() const override final { return m_Desc; }

    /// Implementation of IDeviceContext::SetRenderTargets().
    virtual void DILIGENT_CALL_TYPE SetRenderTargets(Uint32                         NumRenderTargets,
                                                     ITextureView*                  ppRenderTargets[],
                                                     ITextureView*                  pDepthStencil,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final
    {
        return this->SetRenderTargetsExt({NumRenderTargets, ppRenderTargets, pDepthStencil, StateTransitionMode});
    }

    /// Base implementation of IDeviceContext::SetVertexBuffers(); validates parameters and
    /// caches references to the buffers.
    inline virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                            Uint32                         NumBuffersSet,
                                                            IBuffer* const*                ppBuffers,
                                                            const Uint64*                  pOffsets,
                                                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                            SET_VERTEX_BUFFERS_FLAGS       Flags) override = 0;

    inline virtual void DILIGENT_CALL_TYPE InvalidateState() override = 0;

    /// Base implementation of IDeviceContext::CommitShaderResources(); validates parameters.
    inline void CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                      int);

    /// Base implementation of IDeviceContext::SetIndexBuffer(); caches the strong reference to the index buffer
    inline virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                          Uint64                         ByteOffset,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override = 0;

    /// Caches the viewports
    inline void SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32& RTWidth, Uint32& RTHeight);

    /// Caches the scissor rects
    inline void SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32& RTWidth, Uint32& RTHeight);

    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override = 0;

    virtual void DILIGENT_CALL_TYPE NextSubpass() override = 0;

    virtual void DILIGENT_CALL_TYPE EndRenderPass() override = 0;

    /// Base implementation of IDeviceContext::UpdateBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint64                         Offset,
                                                 Uint64                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override = 0;

    /// Base implementation of IDeviceContext::CopyBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint64                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint64                         DstOffset,
                                               Uint64                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override = 0;

    /// Base implementation of IDeviceContext::MapBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer*  pBuffer,
                                              MAP_TYPE  MapType,
                                              MAP_FLAGS MapFlags,
                                              PVoid&    pMappedData) override = 0;

    /// Base implementation of IDeviceContext::UnmapBuffer()
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override = 0;

    /// Base implementation of IDeviceContext::UpdateData(); validates input parameters
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode) override = 0;

    /// Base implementation of IDeviceContext::CopyTexture(); validates input parameters
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override = 0;

    /// Base implementation of IDeviceContext::MapTextureSubresource()
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override = 0;

    /// Base implementation of IDeviceContext::UnmapTextureSubresource()
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture,
                                                            Uint32    MipLevel,
                                                            Uint32    ArraySlice) override = 0;

    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override = 0;

    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override = 0;

    virtual Uint64 DILIGENT_CALL_TYPE GetFrameNumber() const override final
    {
        return m_FrameNumber;
    }

    /// Implementation of IDeviceContext::SetUserData.
    virtual void DILIGENT_CALL_TYPE SetUserData(IObject* pUserData) override final
    {
        m_pUserData = pUserData;
    }

    /// Implementation of IDeviceContext::GetUserData.
    virtual IObject* DILIGENT_CALL_TYPE GetUserData() const override final
    {
        return m_pUserData;
    }

    /// Base implementation of IDeviceContext::DispatchTile.
    virtual void DILIGENT_CALL_TYPE DispatchTile(const DispatchTileAttribs& Attribs) override
    {
        UNSUPPORTED("Tile pipeline is not supported by this device. Please check DeviceFeatures.TileShaders feature.");
    }

    /// Base implementation of IDeviceContext::GetTileSize.
    virtual void DILIGENT_CALL_TYPE GetTileSize(Uint32& TileSizeX, Uint32& TileSizeY) override
    {
        UNSUPPORTED("Tile pipeline is not supported by this device. Please check DeviceFeatures.TileShaders feature.");
    }

    virtual void DILIGENT_CALL_TYPE ClearStats() override final
    {
        m_Stats = {};
    }

    virtual const DeviceContextStats& DILIGENT_CALL_TYPE GetStats() const override final
    {
        return m_Stats;
    }

    /// Returns currently bound pipeline state and blend factors
    inline void GetPipelineState(IPipelineState** ppPSO, float* BlendFactors, Uint32& StencilRef);

    /// Returns currently bound render targets
    inline void GetRenderTargets(Uint32& NumRenderTargets, ITextureView** ppRTVs, ITextureView** ppDSV);

    /// Returns currently set viewports
    inline void GetViewports(Uint32& NumViewports, Viewport* pViewports);

    /// Returns the render device
    IRenderDevice* GetDevice() { return m_pDevice; }

    virtual void ResetRenderTargets();

    bool IsDeferred() const { return m_Desc.IsDeferred; }

    /// Checks if a texture is bound as a render target or depth-stencil buffer and
    /// resets render targets if it is.
    bool UnbindTextureFromFramebuffer(TextureImplType* pTexture, bool bShowMessage);

    bool HasActiveRenderPass() const { return m_pActiveRenderPass != nullptr; }

    DeviceContextIndex GetContextId() const { return DeviceContextIndex{m_Desc.ContextId}; }

    // Returns the index of the immediate context where commands from this context will be executed.
    // For immediate contexts this is the same as ContextId. For deferred contexts, this is the index of
    // the context that was given to Begin() method.
    DeviceContextIndex GetExecutionCtxId() const
    {
        VERIFY(!IsDeferred() || IsRecordingDeferredCommands(),
               "For deferred contexts, the execution context id may only be requested while the context is in recording state");
        return IsDeferred() ? m_DstImmediateContextId : GetContextId();
    }

protected:
    /// Committed shader resources for each resource signature
    struct CommittedShaderResources
    {
        // Pointers to shader resource caches for each signature
        std::array<ShaderResourceCacheImplType*, MAX_RESOURCE_SIGNATURES> ResourceCaches = {};

#ifdef DILIGENT_DEVELOPMENT
        // SRB array for each resource signature, corresponding to ResourceCaches
        std::array<RefCntWeakPtr<ShaderResourceBindingImplType>, MAX_RESOURCE_SIGNATURES> SRBs;

        // Shader resource cache version for every SRB at the time when the SRB was set
        std::array<Uint32, MAX_RESOURCE_SIGNATURES> CacheRevisions = {};

        // Indicates if the resources have been validated since they were committed
        bool ResourcesValidated = false;
#endif

        using SRBMaskType = Uint8;
        static_assert(sizeof(SRBMaskType) * 8 >= MAX_RESOURCE_SIGNATURES, "Not enough space to store MAX_RESOURCE_SIGNATURES bits");

        // Indicates which SRBs are active in current PSO
        SRBMaskType ActiveSRBMask = 0;

        // Indicates stale SRBs that have not been committed yet
        SRBMaskType StaleSRBMask = 0;

        // Indicates which SRBs have dynamic resources that need to be
        // processed every frame (e.g. USAGE_DYNAMIC buffers in Direct3D12 and Vulkan,
        // buffers with dynamic offsets in all backends).
        SRBMaskType DynamicSRBMask = 0;

        void Set(Uint32 Index, ShaderResourceBindingImplType* pSRB)
        {
            VERIFY_EXPR(Index < MAX_RESOURCE_SIGNATURES);
            auto* pResourceCache  = pSRB != nullptr ? &pSRB->GetResourceCache() : nullptr;
            ResourceCaches[Index] = pResourceCache;

            const auto SRBBit = static_cast<SRBMaskType>(1u << Index);
            if (pResourceCache != nullptr)
                StaleSRBMask |= SRBBit;
            else
                StaleSRBMask &= ~SRBBit;

            if (pResourceCache != nullptr && pResourceCache->HasDynamicResources())
                DynamicSRBMask |= SRBBit;
            else
                DynamicSRBMask &= ~SRBBit;

#ifdef DILIGENT_DEVELOPMENT
            SRBs[Index] = pSRB;
            if (pSRB != nullptr)
                ResourcesValidated = false;
            CacheRevisions[Index] = pResourceCache != nullptr ? pResourceCache->DvpGetRevision() : 0;
#endif
        }

        void MakeAllStale()
        {
            StaleSRBMask = 0xFFu;
        }

        // Returns the mask of SRBs whose resources need to be committed
        SRBMaskType GetCommitMask(bool DynamicResourcesIntact = false) const
        {
#ifdef DILIGENT_DEVELOPMENT
            DvpVerifyCacheRevisions();
#endif

            // Stale SRBs always have to be committed
            auto CommitMask = StaleSRBMask;
            // If dynamic resources are not intact, SRBs with dynamic resources
            // have to be handled
            if (!DynamicResourcesIntact)
                CommitMask |= DynamicSRBMask;
            // Only process SRBs that are used by current PSO
            CommitMask &= ActiveSRBMask;
            return CommitMask;
        }

#ifdef DILIGENT_DEVELOPMENT
        void DvpVerifyCacheRevisions() const
        {
            for (Uint32 ActiveSRBs = ActiveSRBMask; ActiveSRBs != 0;)
            {
                const auto  SRBBit = ExtractLSB(ActiveSRBs);
                const auto  Idx    = PlatformMisc::GetLSB(SRBBit);
                const auto* pCache = ResourceCaches[Idx];
                if (pCache != nullptr)
                {
                    DEV_CHECK_ERR(CacheRevisions[Idx] == pCache->DvpGetRevision(),
                                  "Revision of the shader resource cache at index ", Idx,
                                  " does not match the revision recorded when the SRB was committed. "
                                  "This indicates that resources have been changed since that time, but "
                                  "the SRB has not been committed with CommitShaderResources(). This usage is invalid.");
                }
                else
                {
                    // This error will be handled by DvpValidateCommittedShaderResources.
                }
            }
        }
#endif
    };

    /// Caches the render target and depth stencil views. Returns true if any view is different
    /// from the cached value and false otherwise.
    inline bool SetRenderTargets(const SetRenderTargetsAttribs& Attribs);

    /// Initializes render targets for the current subpass
    inline bool SetSubpassRenderTargets();

    inline bool SetBlendFactors(const float* BlendFactors, int Dummy);

    inline bool SetStencilRef(Uint32 StencilRef, int Dummy);

    inline void SetPipelineState(RefCntAutoPtr<PipelineStateImplType> pPipelineState, int /*Dummy*/);

    /// Clears all cached resources
    inline void ClearStateCache();

    /// Checks if the texture is currently bound as a render target.
    bool CheckIfBoundAsRenderTarget(TextureImplType* pTexture);

    /// Checks if the texture is currently bound as depth-stencil buffer.
    bool CheckIfBoundAsDepthStencil(TextureImplType* pTexture);

    /// Updates the states of render pass attachments to match states within the given subpass
    void UpdateAttachmentStates(Uint32 SubpassIndex);

    void ClearDepthStencil(ITextureView* pView);

    void ClearRenderTarget(ITextureView* pView);

    void BeginQuery(IQuery* pQuery, int);

    void EndQuery(IQuery* pQuery, int);

    void EnqueueSignal(IFence* pFence, Uint64 Value, int);
    void DeviceWaitForFence(IFence* pFence, Uint64 Value, int);

    void EndFrame()
    {
        ++m_FrameNumber;
    }

    void PrepareCommittedResources(CommittedShaderResources& Resources, Uint32& DvpCompatibleSRBCount);

    bool IsRecordingDeferredCommands() const
    {
        DEV_CHECK_ERR(IsDeferred(), "Only deferred contexts may record deferred commands.");
        return m_DstImmediateContextId != INVALID_CONTEXT_ID;
    }

    void Begin(DeviceContextIndex ImmediateContextId, COMMAND_QUEUE_TYPE QueueType)
    {
        DEV_CHECK_ERR(IsDeferred(), "Begin() is only allowed for deferred contexts.");
        DEV_CHECK_ERR(!IsRecordingDeferredCommands(), "This context is already recording commands. Call FinishCommandList() before beginning new recording.");
        m_DstImmediateContextId = static_cast<Uint8>(ImmediateContextId);
        VERIFY_EXPR(m_DstImmediateContextId == ImmediateContextId);

        // Set command queue type while commands are being recorded
        m_Desc.QueueType = QueueType;
        for (size_t i = 0; i < _countof(m_Desc.TextureCopyGranularity); ++i)
            m_Desc.TextureCopyGranularity[i] = 1;
    }

    void FinishCommandList()
    {
        DEV_CHECK_ERR(IsDeferred(), "FinishCommandList() is only allowed for deferred contexts.");
        DEV_CHECK_ERR(IsRecordingDeferredCommands(), "This context is not recording commands. Call Begin() before finishing the recording.");
        m_DstImmediateContextId = INVALID_CONTEXT_ID;
        m_Desc.QueueType        = COMMAND_QUEUE_TYPE_UNKNOWN;
        for (size_t i = 0; i < _countof(m_Desc.TextureCopyGranularity); ++i)
            m_Desc.TextureCopyGranularity[i] = 0;
    }

#ifdef DILIGENT_DEVELOPMENT
    // clang-format off
    void DvpVerifyDispatchTileArguments(const DispatchTileAttribs& Attribs) const;

    void DvpVerifyRenderTargets() const;
    void DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier) const;
    void DvpVerifyTextureState(const TextureImplType&   Texture, RESOURCE_STATE RequiredState, const char* OperationName) const;
    void DvpVerifyBufferState (const BufferImplType&    Buffer,  RESOURCE_STATE RequiredState, const char* OperationName) const;
    void DvpVerifyBLASState   (const BottomLevelASType& BLAS,    RESOURCE_STATE RequiredState, const char* OperationName) const;
    void DvpVerifyTLASState   (const TopLevelASType&    TLAS,    RESOURCE_STATE RequiredState, const char* OperationName) const;
    // clang-format on

    // Verifies compatibility between current PSO and SRBs
    void DvpVerifySRBCompatibility(
        CommittedShaderResources&                                 Resources,
        std::function<PipelineResourceSignatureImplType*(Uint32)> CustomGetSignature = nullptr) const;
#else
    // clang-format off
    void DvpVerifyDispatchTileArguments(const DispatchTileAttribs& Attribs) const {}

    void DvpVerifyRenderTargets()const {}
    void DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier)const {}
    void DvpVerifyTextureState(const TextureImplType&   Texture, RESOURCE_STATE RequiredState, const char* OperationName) const {}
    void DvpVerifyBufferState (const BufferImplType&    Buffer,  RESOURCE_STATE RequiredState, const char* OperationName) const {}
    void DvpVerifyBLASState   (const BottomLevelASType& BLAS,    RESOURCE_STATE RequiredState, const char* OperationName) const {}
    void DvpVerifyTLASState   (const TopLevelASType&    TLAS,    RESOURCE_STATE RequiredState, const char* OperationName) const {}
    // clang-format on
#endif

    void Draw(const DrawAttribs& Attribs, int);
    void DrawIndexed(const DrawIndexedAttribs& Attribs, int);
    void DrawIndirect(const DrawIndirectAttribs& Attribs, int);
    void DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, int);
    void DrawMesh(const DrawMeshAttribs& Attribs, int);
    void DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, int);
    void MultiDraw(const MultiDrawAttribs& Attribs, int);
    void MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs, int);
    void DispatchCompute(const DispatchComputeAttribs& Attribs, int);
    void DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, int);

    void BuildBLAS(const BuildBLASAttribs& Attribs, int);
    void BuildTLAS(const BuildTLASAttribs& Attribs, int);
    void CopyBLAS(const CopyBLASAttribs& Attribs, int);
    void CopyTLAS(const CopyTLASAttribs& Attribs, int);
    void WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs, int);
    void WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs, int);
    void TraceRays(const TraceRaysAttribs& Attribs, int);
    void TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs, int);
    void UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs, int);

    void BeginDebugGroup(const Char* Name, const float* pColor, int);
    void EndDebugGroup(int);
    void InsertDebugLabel(const Char* Label, const float* pColor, int) const;

    void SetShadingRate(SHADING_RATE BaseRate, SHADING_RATE_COMBINER PrimitiveCombiner, SHADING_RATE_COMBINER TextureCombiner, int) const;

    void BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs, int);

protected:
    static constexpr Uint32 DrawMeshIndirectCommandStride = sizeof(Uint32) * 3; // D3D12: 12 bytes (x, y, z dimension)
                                                                                // Vulkan: 8 bytes (task count, first task)
    static constexpr Uint32 TraceRaysIndirectCommandSBTSize = 88;               // D3D12: 88 bytes, size of SBT offsets
                                                                                // Vulkan: 0 bytes, SBT offsets placed directly into function call
    static constexpr Uint32 TraceRaysIndirectCommandSize = 104;                 // SBT (88 bytes) + Dimension (3*4 bytes) aligned to 8 bytes

    /// Strong reference to the device.
    RefCntAutoPtr<DeviceImplType> m_pDevice;

    /// Vertex streams. Every stream holds strong reference to the buffer
    VertexStreamInfo<BufferImplType> m_VertexStreams[MAX_BUFFER_SLOTS];

    /// Number of bound vertex streams
    Uint32 m_NumVertexStreams = 0;

    /// Strong reference to the bound pipeline state object.
    /// Use final PSO implementation type to avoid virtual calls to AddRef()/Release().
    /// We need to keep strong reference as we examine previous pipeline state in
    /// SetPipelineState()
    RefCntAutoPtr<PipelineStateImplType> m_pPipelineState;

    /// Strong reference to the bound index buffer.
    /// Use final buffer implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<BufferImplType> m_pIndexBuffer;

    /// Offset from the beginning of the index buffer to the start of the index data, in bytes.
    Uint64 m_IndexDataStartOffset = 0;

    /// Current stencil reference value
    Uint32 m_StencilRef = 0;

    /// Current blend factors
    Float32 m_BlendFactors[4] = {-1, -1, -1, -1};

    /// Current viewports
    Viewport m_Viewports[MAX_VIEWPORTS];
    /// Number of current viewports
    Uint32 m_NumViewports = 0;

    /// Current scissor rects
    Rect m_ScissorRects[MAX_VIEWPORTS];
    /// Number of current scissor rects
    Uint32 m_NumScissorRects = 0;

    /// Vector of strong references to the bound render targets.
    /// Use final texture view implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<TextureViewImplType> m_pBoundRenderTargets[MAX_RENDER_TARGETS];
    /// Number of bound render targets
    Uint32 m_NumBoundRenderTargets = 0;
    /// Width of the currently bound framebuffer
    Uint32 m_FramebufferWidth = 0;
    /// Height of the currently bound framebuffer
    Uint32 m_FramebufferHeight = 0;
    /// Number of array slices in the currently bound framebuffer
    Uint32 m_FramebufferSlices = 0;
    /// Number of samples in the currently bound framebuffer
    Uint32 m_FramebufferSamples = 0;

    /// Strong references to the bound depth stencil view.
    /// Use final texture view implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<TextureViewImplType> m_pBoundDepthStencil;

    /// Strong reference to the bound framebuffer.
    RefCntAutoPtr<FramebufferImplType> m_pBoundFramebuffer;

    /// Strong reference to the render pass.
    RefCntAutoPtr<RenderPassImplType> m_pActiveRenderPass;

    /// Strong reference to the variable rate shading view.
    /// Implementation may be TextureViewImplType or IRasterizationRateMapMtl.
    RefCntAutoPtr<ITextureView> m_pBoundShadingRateMap;

    /// Current subpass index.
    Uint32 m_SubpassIndex = 0;

    /// Render pass attachments transition mode.
    RESOURCE_STATE_TRANSITION_MODE m_RenderPassAttachmentsTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;

    Uint64 m_FrameNumber = 0;

    RefCntAutoPtr<IObject> m_pUserData;

    // Must go before m_Desc!
    const String m_Name;

    DeviceContextDesc m_Desc;

    const bool m_NativeMultiDrawSupported;

    // For deferred contexts in recording state only, the index
    // of the destination immediate context where the command list
    // will be submitted.
    DeviceContextIndex m_DstImmediateContextId{INVALID_CONTEXT_ID};

    DeviceContextStats m_Stats;

    std::vector<Uint8> m_ScratchSpace;

#ifdef DILIGENT_DEBUG
    // std::unordered_map is unbelievably slow. Keeping track of mapped buffers
    // in release builds is not feasible
    struct DbgMappedBufferInfo
    {
        MAP_TYPE MapType;
    };
    std::unordered_map<IBuffer*, DbgMappedBufferInfo> m_DbgMappedBuffers;
#endif
#ifdef DILIGENT_DEVELOPMENT
    int m_DvpDebugGroupCount = 0;
#endif
};

#define DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(SupportedQueueType, ...)                                                            \
    do                                                                                                                         \
    {                                                                                                                          \
        if (m_Desc.QueueType == COMMAND_QUEUE_TYPE_UNKNOWN)                                                                    \
        {                                                                                                                      \
            VERIFY(IsDeferred(), "Queue type may never be unknown for immediate contexts. This looks like a bug.");            \
            DEV_ERROR("Queue type is UNKNOWN. This indicates that Begin() has never been called for a deferred context.");     \
        }                                                                                                                      \
        DEV_CHECK_ERR((m_Desc.QueueType & (SupportedQueueType)) == (SupportedQueueType), __VA_ARGS__, " is not supported in ", \
                      GetCommandQueueTypeString(m_Desc.QueueType), " queue.");                                                 \
    } while (false)

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::SetVertexBuffers(
    Uint32                         StartSlot,
    Uint32                         NumBuffersSet,
    IBuffer* const*                ppBuffers,
    const Uint64*                  pOffsets,
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
    SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetVertexBuffers");

    DEV_CHECK_ERR(StartSlot < MAX_BUFFER_SLOTS, "Start vertex buffer slot ", StartSlot, " is out of allowed range [0, ", MAX_BUFFER_SLOTS - 1, "].");

    DEV_CHECK_ERR(StartSlot + NumBuffersSet <= MAX_BUFFER_SLOTS,
                  "The range of vertex buffer slots being set [", StartSlot, ", ", StartSlot + NumBuffersSet - 1,
                  "] is out of allowed range  [0, ", MAX_BUFFER_SLOTS - 1, "].");

    DEV_CHECK_ERR(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
                  "Resource state transitions are not allowed inside a render pass and may result in an undefined behavior. "
                  "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

    if (Flags & SET_VERTEX_BUFFERS_FLAG_RESET)
    {
        // Reset only these buffer slots that are not being set.
        // It is very important to not reset buffers that stay unchanged
        // as AddRef()/Release() are not free
        for (Uint32 s = 0; s < StartSlot; ++s)
            m_VertexStreams[s] = VertexStreamInfo<BufferImplType>{};
        for (Uint32 s = StartSlot + NumBuffersSet; s < m_NumVertexStreams; ++s)
            m_VertexStreams[s] = VertexStreamInfo<BufferImplType>{};
        m_NumVertexStreams = 0;
    }
    m_NumVertexStreams = std::max(m_NumVertexStreams, StartSlot + NumBuffersSet);

    for (Uint32 Buff = 0; Buff < NumBuffersSet; ++Buff)
    {
        auto& CurrStream   = m_VertexStreams[StartSlot + Buff];
        CurrStream.pBuffer = ppBuffers ? ClassPtrCast<BufferImplType>(ppBuffers[Buff]) : nullptr;
        CurrStream.Offset  = pOffsets ? pOffsets[Buff] : 0;
#ifdef DILIGENT_DEVELOPMENT
        if (CurrStream.pBuffer)
        {
            const auto& BuffDesc = CurrStream.pBuffer->GetDesc();
            DEV_CHECK_ERR((BuffDesc.BindFlags & BIND_VERTEX_BUFFER) != 0,
                          "Buffer '", BuffDesc.Name ? BuffDesc.Name : "", "' being bound as vertex buffer to slot ", Buff,
                          " was not created with BIND_VERTEX_BUFFER flag");
        }
#endif
    }
    // Remove null buffers from the end of the array
    while (m_NumVertexStreams > 0 && !m_VertexStreams[m_NumVertexStreams - 1].pBuffer)
        m_VertexStreams[m_NumVertexStreams--] = VertexStreamInfo<BufferImplType>{};

    ++m_Stats.CommandCounters.SetVertexBuffers;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::SetPipelineState(
    RefCntAutoPtr<PipelineStateImplType> pPipelineState,
    int /*Dummy*/)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "SetPipelineState");
    DEV_CHECK_ERR((pPipelineState->GetDesc().ImmediateContextMask & (Uint64{1} << GetExecutionCtxId())) != 0,
                  "PSO '", pPipelineState->GetDesc().Name, "' can't be used in device context '", m_Desc.Name, "'.");
    DEV_CHECK_ERR(pPipelineState->GetStatus() == PIPELINE_STATE_STATUS_READY, "PSO '", pPipelineState->GetDesc().Name, "' is not ready. Use GetStatus() to check the pipeline status.");

    m_pPipelineState = std::move(pPipelineState);
    ++m_Stats.CommandCounters.SetPipelineState;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::CommitShaderResources(
    IShaderResourceBinding*        pShaderResourceBinding,
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
    int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "CommitShaderResources");
    DEV_CHECK_ERR(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
                  "Resource state transitions are not allowed inside a render pass and may result in an undefined behavior. "
                  "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

    DEV_CHECK_ERR(pShaderResourceBinding != nullptr, "pShaderResourceBinding must not be null");

    ++m_Stats.CommandCounters.CommitShaderResources;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::InvalidateState()
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Invalidating context inside an active render pass. Call EndRenderPass() to finish the pass.");

    DeviceContextBase<ImplementationTraits>::ClearStateCache();
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::SetIndexBuffer(
    IBuffer*                       pIndexBuffer,
    Uint64                         ByteOffset,
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    m_pIndexBuffer         = ClassPtrCast<BufferImplType>(pIndexBuffer);
    m_IndexDataStartOffset = ByteOffset;

#ifdef DILIGENT_DEVELOPMENT
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetIndexBuffer");

    DEV_CHECK_ERR(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
                  "Resource state transitions are not allowed inside a render pass and may result in an undefined behavior. "
                  "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

    if (m_pIndexBuffer)
    {
        const auto& BuffDesc = m_pIndexBuffer->GetDesc();
        DEV_CHECK_ERR((BuffDesc.BindFlags & BIND_INDEX_BUFFER) != 0,
                      "Buffer '", BuffDesc.Name ? BuffDesc.Name : "", "' being bound as index buffer was not created with BIND_INDEX_BUFFER flag");
    }
#endif

    ++m_Stats.CommandCounters.SetIndexBuffer;
}


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::GetPipelineState(IPipelineState** ppPSO, float* BlendFactors, Uint32& StencilRef)
{
    DEV_CHECK_ERR(ppPSO != nullptr, "Null pointer provided null");
    DEV_CHECK_ERR(*ppPSO == nullptr, "Memory address contains a pointer to a non-null blend state");
    if (m_pPipelineState)
    {
        m_pPipelineState->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPSO));
    }
    else
    {
        *ppPSO = nullptr;
    }

    for (Uint32 f = 0; f < 4; ++f)
        BlendFactors[f] = m_BlendFactors[f];
    StencilRef = m_StencilRef;
};

template <typename ImplementationTraits>
inline bool DeviceContextBase<ImplementationTraits>::SetBlendFactors(const float* BlendFactors, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetBlendFactors");

    bool FactorsDiffer = false;
    for (Uint32 f = 0; f < 4; ++f)
    {
        if (m_BlendFactors[f] != BlendFactors[f])
            FactorsDiffer = true;
        m_BlendFactors[f] = BlendFactors[f];
    }
    if (FactorsDiffer)
        ++m_Stats.CommandCounters.SetBlendFactors;

    return FactorsDiffer;
}

template <typename ImplementationTraits>
inline bool DeviceContextBase<ImplementationTraits>::SetStencilRef(Uint32 StencilRef, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetStencilRef");

    if (m_StencilRef != StencilRef)
    {
        m_StencilRef = StencilRef;
        ++m_Stats.CommandCounters.SetStencilRef;
        return true;
    }
    return false;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::SetViewports(
    Uint32          NumViewports,
    const Viewport* pViewports,
    Uint32&         RTWidth,
    Uint32&         RTHeight)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetViewports");

    if (NumViewports > 1)
    {
        DEV_CHECK_ERR(m_pDevice->GetFeatures().MultiViewport,
                      "IDeviceContext::SetViewports: multi viewport is not supported by this device");
    }
    if (RTWidth == 0 || RTHeight == 0)
    {
        RTWidth  = m_FramebufferWidth;
        RTHeight = m_FramebufferHeight;
    }

    DEV_CHECK_ERR(NumViewports < MAX_VIEWPORTS, "Number of viewports (", NumViewports, ") exceeds the limit (", MAX_VIEWPORTS, ")");
    m_NumViewports = std::min(MAX_VIEWPORTS, NumViewports);

    Viewport DefaultVP{RTWidth, RTHeight};
    // If no viewports are specified, use default viewport
    if (m_NumViewports == 1 && pViewports == nullptr)
    {
        pViewports = &DefaultVP;
    }
    DEV_CHECK_ERR(pViewports != nullptr, "pViewports must not be null");

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        m_Viewports[vp] = pViewports[vp];
        DEV_CHECK_ERR(m_Viewports[vp].Width >= 0, "Incorrect viewport width (", m_Viewports[vp].Width, ")");
        DEV_CHECK_ERR(m_Viewports[vp].Height >= 0, "Incorrect viewport height (", m_Viewports[vp].Height, ")");
        DEV_CHECK_ERR(m_Viewports[vp].MaxDepth >= m_Viewports[vp].MinDepth, "Incorrect viewport depth range [", m_Viewports[vp].MinDepth, ", ", m_Viewports[vp].MaxDepth, "]");
    }

    ++m_Stats.CommandCounters.SetViewports;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::GetViewports(Uint32& NumViewports, Viewport* pViewports)
{
    NumViewports = m_NumViewports;
    if (pViewports)
    {
        for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
            pViewports[vp] = m_Viewports[vp];
    }
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::SetScissorRects(
    Uint32      NumRects,
    const Rect* pRects,
    Uint32&     RTWidth,
    Uint32&     RTHeight)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetScissorRects");

    if (NumRects > 1)
    {
        DEV_CHECK_ERR(m_pDevice->GetFeatures().MultiViewport,
                      "IDeviceContext::SetScissorRects: multi viewport is not supported by this device");
    }
    if (RTWidth == 0 || RTHeight == 0)
    {
        RTWidth  = m_FramebufferWidth;
        RTHeight = m_FramebufferHeight;
    }

    DEV_CHECK_ERR(NumRects < MAX_VIEWPORTS, "Number of scissor rects (", NumRects, ") exceeds the limit (", MAX_VIEWPORTS, ")");
    m_NumScissorRects = std::min(MAX_VIEWPORTS, NumRects);

    for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
    {
        m_ScissorRects[sr] = pRects[sr];
        DEV_CHECK_ERR(m_ScissorRects[sr].left <= m_ScissorRects[sr].right, "Incorrect horizontal bounds for a scissor rect [", m_ScissorRects[sr].left, ", ", m_ScissorRects[sr].right, ")");
        DEV_CHECK_ERR(m_ScissorRects[sr].top <= m_ScissorRects[sr].bottom, "Incorrect vertical bounds for a scissor rect [", m_ScissorRects[sr].top, ", ", m_ScissorRects[sr].bottom, ")");
    }

    ++m_Stats.CommandCounters.SetScissorRects;
}

template <typename ImplementationTraits>
inline bool DeviceContextBase<ImplementationTraits>::SetRenderTargets(const SetRenderTargetsAttribs& Attribs)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetRenderTargets");

    if (Attribs.NumRenderTargets == 0 && Attribs.pDepthStencil == nullptr)
    {
        ResetRenderTargets();
        return false;
    }

    m_pBoundShadingRateMap.Release();

    bool bBindRenderTargets = false;
    m_FramebufferWidth      = 0;
    m_FramebufferHeight     = 0;
    m_FramebufferSlices     = 0;
    m_FramebufferSamples    = 0;

    if (Attribs.NumRenderTargets != m_NumBoundRenderTargets)
    {
        bBindRenderTargets = true;
        for (Uint32 rt = Attribs.NumRenderTargets; rt < m_NumBoundRenderTargets; ++rt)
            m_pBoundRenderTargets[rt].Release();

        m_NumBoundRenderTargets = Attribs.NumRenderTargets;
    }

    for (Uint32 rt = 0; rt < Attribs.NumRenderTargets; ++rt)
    {
        auto* pRTView = Attribs.ppRenderTargets[rt];
        if (pRTView)
        {
            const auto& RTVDesc = pRTView->GetDesc();
            const auto& TexDesc = pRTView->GetTexture()->GetDesc();
            DEV_CHECK_ERR(RTVDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET,
                          "Texture view object named '", RTVDesc.Name ? RTVDesc.Name : "", "' has incorrect view type (", GetTexViewTypeLiteralName(RTVDesc.ViewType), "). Render target view is expected");
            DEV_CHECK_ERR(m_pBoundFramebuffer || (TexDesc.MiscFlags & MISC_TEXTURE_FLAG_MEMORYLESS) == 0,
                          "Memoryless render target '", TexDesc.Name, "' must be used within a framebuffer");

            // Use this RTV to set the render target size
            if (m_FramebufferWidth == 0)
            {
                m_FramebufferWidth   = std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U);
                m_FramebufferHeight  = std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U);
                m_FramebufferSlices  = RTVDesc.NumArraySlices;
                m_FramebufferSamples = TexDesc.SampleCount;
            }
            else
            {
#ifdef DILIGENT_DEVELOPMENT
                DEV_CHECK_ERR(m_FramebufferWidth == std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U),
                              "Render target width (", std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U), ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the width of previously bound render targets (", m_FramebufferWidth, ")");
                DEV_CHECK_ERR(m_FramebufferHeight == std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U),
                              "Render target height (", std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U), ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the height of previously bound render targets (", m_FramebufferHeight, ")");
                DEV_CHECK_ERR(m_FramebufferSlices == RTVDesc.NumArraySlices,
                              "The number of slices (", RTVDesc.NumArraySlices, ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the number of slices in previously bound render targets (", m_FramebufferSlices, ")");
                DEV_CHECK_ERR(m_FramebufferSamples == TexDesc.SampleCount,
                              "Sample count (", TexDesc.SampleCount, ") of RTV '", RTVDesc.Name, "' is inconsistent with the sample count of previously bound render targets (", m_FramebufferSamples, ")");
#endif
            }
        }

        // Here both views are certainly live objects, since we store
        // strong references to all bound render targets. So we
        // can safely compare pointers.
        if (m_pBoundRenderTargets[rt] != pRTView)
        {
            m_pBoundRenderTargets[rt] = ClassPtrCast<TextureViewImplType>(pRTView);
            bBindRenderTargets        = true;
        }
    }

    if (Attribs.pDepthStencil != nullptr)
    {
        const auto& DSVDesc = Attribs.pDepthStencil->GetDesc();
        const auto& TexDesc = Attribs.pDepthStencil->GetTexture()->GetDesc();
        DEV_CHECK_ERR(DSVDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL || DSVDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL,
                      "Texture view object named '", DSVDesc.Name ? DSVDesc.Name : "", "' has incorrect view type (", GetTexViewTypeLiteralName(DSVDesc.ViewType),
                      "). Depth-stencil or read-only depth-stencil view is expected");
        DEV_CHECK_ERR(m_pBoundFramebuffer || (TexDesc.MiscFlags & MISC_TEXTURE_FLAG_MEMORYLESS) == 0,
                      "Memoryless depth buffer '", TexDesc.Name, "' must be used within a framebuffer");

        // Use depth stencil size to set render target size
        if (m_FramebufferWidth == 0)
        {
            m_FramebufferWidth   = std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U);
            m_FramebufferHeight  = std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U);
            m_FramebufferSlices  = DSVDesc.NumArraySlices;
            m_FramebufferSamples = TexDesc.SampleCount;
        }
        else
        {
#ifdef DILIGENT_DEVELOPMENT
            DEV_CHECK_ERR(m_FramebufferWidth == std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U),
                          "Depth-stencil target width (", std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U), ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the width of previously bound render targets (", m_FramebufferWidth, ")");
            DEV_CHECK_ERR(m_FramebufferHeight == std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U),
                          "Depth-stencil target height (", std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U), ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the height of previously bound render targets (", m_FramebufferHeight, ")");
            DEV_CHECK_ERR(m_FramebufferSlices == DSVDesc.NumArraySlices,
                          "The number of slices (", DSVDesc.NumArraySlices, ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the number of slices in previously bound render targets (", m_FramebufferSlices, ")");
            DEV_CHECK_ERR(m_FramebufferSamples == TexDesc.SampleCount,
                          "Sample count (", TexDesc.SampleCount, ") of DSV '", DSVDesc.Name, "' is inconsistent with the sample count of previously bound render targets (", m_FramebufferSamples, ")");
#endif
        }
    }

    if (m_pBoundDepthStencil != Attribs.pDepthStencil)
    {
        m_pBoundDepthStencil = ClassPtrCast<TextureViewImplType>(Attribs.pDepthStencil);
        bBindRenderTargets   = true;
    }

    VERIFY_EXPR(m_FramebufferWidth > 0 && m_FramebufferHeight > 0 && m_FramebufferSlices > 0 && m_FramebufferSamples > 0);

    if (Attribs.pShadingRateMap)
    {
#ifdef DILIGENT_DEVELOPMENT
        DEV_CHECK_ERR(m_pDevice->GetDeviceInfo().Features.VariableRateShading, "IDeviceContext::SetRenderTargets: VariableRateShading feature must be enabled when used pShadingRateMap");

        const auto& SRProps  = m_pDevice->GetAdapterInfo().ShadingRate;
        const auto& ViewDesc = Attribs.pShadingRateMap->GetDesc();
        DEV_CHECK_ERR(ViewDesc.ViewType == TEXTURE_VIEW_SHADING_RATE, "IDeviceContext::SetRenderTargets: pShadingRateMap must be created with TEXTURE_VIEW_SHADING_RATE type");
        DEV_CHECK_ERR(SRProps.CapFlags & SHADING_RATE_CAP_FLAG_TEXTURE_BASED, "IDeviceContext::SetRenderTargets: SHADING_RATE_CAP_FLAG_TEXTURE_BASED capability must be supported");

        if (!m_pDevice->GetDeviceInfo().IsMetalDevice())
        {
            const auto& TexDesc = Attribs.pShadingRateMap->GetTexture()->GetDesc();
            DEV_CHECK_ERR(TexDesc.BindFlags & BIND_SHADING_RATE, "IDeviceContext::SetRenderTargets: pShadingRateMap must be created with BIND_SHADING_RATE flag");

            switch (SRProps.Format)
            {
                case SHADING_RATE_FORMAT_PALETTE:
                    DEV_CHECK_ERR(ViewDesc.Format == TEX_FORMAT_R8_UINT,
                                  "IDeviceContext::SetRenderTargets: pShadingRateMap format must be R8_UINT. "
                                  "Check supported shading rate format in adapter info.");
                    break;
                case SHADING_RATE_FORMAT_UNORM8:
                    DEV_CHECK_ERR(ViewDesc.Format == TEX_FORMAT_RG8_UNORM,
                                  "IDeviceContext::SetRenderTargets: pShadingRateMap format must be RG8_UNORM. "
                                  "Check supported shading rate format in adapter info.");
                    break;
                default:
                    DEV_ERROR("IDeviceContext::SetRenderTargets: unexpected shading rate format");
            }

            const auto Width     = std::max(TexDesc.Width >> ViewDesc.MostDetailedMip, 1u);
            const auto Height    = std::max(TexDesc.Height >> ViewDesc.MostDetailedMip, 1u);
            const auto MinWidth  = (m_FramebufferWidth + SRProps.MaxTileSize[0] - 1) / SRProps.MaxTileSize[0];
            const auto MinHeight = (m_FramebufferHeight + SRProps.MaxTileSize[1] - 1) / SRProps.MaxTileSize[1];
            DEV_CHECK_ERR(Width >= MinWidth,
                          "IDeviceContext::SetRenderTargets: shading rate texture width (", Width, ") must be at least ",
                          MinWidth, "). Note: minimum width is defined by (framebuffer width) / ShadingRate::MaxTileSize[0].");
            DEV_CHECK_ERR(Height >= MinHeight,
                          "IDeviceContext::SetRenderTargets: shading rate texture height (", Height, ") must be at least",
                          MinHeight, "). Note: minimum height is defined by (framebuffer height) / ShadingRate::MaxTileSize[1].");
        }
#endif
    }

    if (m_pBoundShadingRateMap != Attribs.pShadingRateMap)
    {
        m_pBoundShadingRateMap = Attribs.pShadingRateMap;
        bBindRenderTargets     = true;
    }

#ifdef DILIGENT_DEVELOPMENT
    const auto& SRProps = m_pDevice->GetAdapterInfo().ShadingRate;
    if (m_pBoundShadingRateMap &&
        (SRProps.CapFlags & SHADING_RATE_CAP_FLAG_NON_SUBSAMPLED_RENDER_TARGET) == 0 &&
        !m_pDevice->GetDeviceInfo().IsMetalDevice())
    {
        VERIFY((SRProps.CapFlags & SHADING_RATE_CAP_FLAG_SUBSAMPLED_RENDER_TARGET) != 0,
               "One of NON_SUBSAMPLED_RENDER_TARGET or SUBSAMPLED_RENDER_TARGET caps must be presented if texture-based VRS is supported");

        for (Uint32 i = 0; i < m_NumBoundRenderTargets; ++i)
        {
            if (auto& pRTV = m_pBoundRenderTargets[i])
            {
                DEV_CHECK_ERR((pRTV->GetTexture()->GetDesc().MiscFlags & MISC_TEXTURE_FLAG_SUBSAMPLED) != 0,
                              "Render target used with shading rate map must be created with MISC_TEXTURE_FLAG_SUBSAMPLED flag when "
                              "SHADING_RATE_CAP_FLAG_NON_SUBSAMPLED_RENDER_TARGET capability is not present.");
            }
        }

        if (m_pBoundDepthStencil)
        {
            DEV_CHECK_ERR((m_pBoundDepthStencil->GetTexture()->GetDesc().MiscFlags & MISC_TEXTURE_FLAG_SUBSAMPLED) != 0,
                          "Depth-stencil target used with shading rate map must be created with MISC_TEXTURE_FLAG_SUBSAMPLED flag when "
                          "SHADING_RATE_CAP_FLAG_NON_SUBSAMPLED_RENDER_TARGET capability is not present.");
        }
    }
#endif

    if (bBindRenderTargets)
        ++m_Stats.CommandCounters.SetRenderTargets;

    return bBindRenderTargets;
}

template <typename ImplementationTraits>
inline bool DeviceContextBase<ImplementationTraits>::SetSubpassRenderTargets()
{
    VERIFY_EXPR(m_pBoundFramebuffer);
    VERIFY_EXPR(m_pActiveRenderPass);

    const auto& RPDesc  = m_pActiveRenderPass->GetDesc();
    const auto& FBDesc  = m_pBoundFramebuffer->GetDesc();
    const auto& Subpass = m_pActiveRenderPass->GetSubpass(m_SubpassIndex);

    m_FramebufferSamples = 0;

    ITextureView* ppRTVs[MAX_RENDER_TARGETS] = {};
    ITextureView* pDSV                       = nullptr;
    ITextureView* pSRM                       = nullptr;
    for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
    {
        const auto& RTAttachmentRef = Subpass.pRenderTargetAttachments[rt];
        if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            VERIFY_EXPR(RTAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
            ppRTVs[rt] = FBDesc.ppAttachments[RTAttachmentRef.AttachmentIndex];
            if (ppRTVs[rt] != nullptr)
            {
                if (m_FramebufferSamples == 0)
                    m_FramebufferSamples = ppRTVs[rt]->GetTexture()->GetDesc().SampleCount;
                else
                    DEV_CHECK_ERR(m_FramebufferSamples == ppRTVs[rt]->GetTexture()->GetDesc().SampleCount, "Inconsistent sample count");
            }
        }
    }

    if (Subpass.pDepthStencilAttachment != nullptr)
    {
        const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
        if (DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            VERIFY_EXPR(DSAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
            pDSV = DSAttachmentRef.State == RESOURCE_STATE_DEPTH_READ ?
                m_pBoundFramebuffer->GetReadOnlyDSV(m_SubpassIndex) :
                FBDesc.ppAttachments[DSAttachmentRef.AttachmentIndex];
            if (pDSV != nullptr)
            {
                if (m_FramebufferSamples == 0)
                    m_FramebufferSamples = pDSV->GetTexture()->GetDesc().SampleCount;
                else
                    DEV_CHECK_ERR(m_FramebufferSamples == pDSV->GetTexture()->GetDesc().SampleCount, "Inconsistent sample count");
            }
        }
    }

    if (Subpass.pShadingRateAttachment != nullptr)
    {
        const auto& SRAttachmentRef = *Subpass.pShadingRateAttachment;
        if (SRAttachmentRef.Attachment.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            VERIFY_EXPR(SRAttachmentRef.Attachment.AttachmentIndex < RPDesc.AttachmentCount);
            pSRM = FBDesc.ppAttachments[SRAttachmentRef.Attachment.AttachmentIndex];
        }
    }

    bool BindRenderTargets = SetRenderTargets({Subpass.RenderTargetAttachmentCount, ppRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_NONE, pSRM});

    // Use framebuffer dimensions (override what was set by SetRenderTargets)
    m_FramebufferWidth  = FBDesc.Width;
    m_FramebufferHeight = FBDesc.Height;
    m_FramebufferSlices = FBDesc.NumArraySlices;
    VERIFY_EXPR((m_FramebufferSamples > 0) || (Subpass.RenderTargetAttachmentCount == 0 && Subpass.pDepthStencilAttachment == nullptr));

    return BindRenderTargets;
}


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::GetRenderTargets(
    Uint32&        NumRenderTargets,
    ITextureView** ppRTVs,
    ITextureView** ppDSV)
{
    NumRenderTargets = m_NumBoundRenderTargets;

    if (ppRTVs)
    {
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            DEV_CHECK_ERR(ppRTVs[rt] == nullptr, "Non-null pointer found in RTV array element #", rt);
            auto pBoundRTV = m_pBoundRenderTargets[rt];
            if (pBoundRTV)
                pBoundRTV->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppRTVs + rt));
            else
                ppRTVs[rt] = nullptr;
        }
        for (Uint32 rt = NumRenderTargets; rt < MAX_RENDER_TARGETS; ++rt)
        {
            DEV_CHECK_ERR(ppRTVs[rt] == nullptr, "Non-null pointer found in RTV array element #", rt);
            ppRTVs[rt] = nullptr;
        }
    }

    if (ppDSV)
    {
        DEV_CHECK_ERR(*ppDSV == nullptr, "Non-null DSV pointer found");
        if (m_pBoundDepthStencil)
            m_pBoundDepthStencil->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppDSV));
        else
            *ppDSV = nullptr;
    }
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::ClearStateCache()
{
    for (Uint32 stream = 0; stream < m_NumVertexStreams; ++stream)
        m_VertexStreams[stream] = VertexStreamInfo<BufferImplType>{};
#ifdef DILIGENT_DEBUG
    for (Uint32 stream = m_NumVertexStreams; stream < _countof(m_VertexStreams); ++stream)
    {
        VERIFY(m_VertexStreams[stream].pBuffer == nullptr, "Unexpected non-null buffer");
        VERIFY(m_VertexStreams[stream].Offset == 0, "Unexpected non-zero offset");
    }
#endif
    m_NumVertexStreams = 0;

    m_pPipelineState.Release();

    m_pIndexBuffer.Release();
    m_IndexDataStartOffset = 0;

    m_StencilRef = 0;

    for (int i = 0; i < 4; ++i)
        m_BlendFactors[i] = -1;

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
        m_Viewports[vp] = Viewport();
    m_NumViewports = 0;

    for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
        m_ScissorRects[sr] = Rect();
    m_NumScissorRects = 0;

    ResetRenderTargets();

    VERIFY(!m_pActiveRenderPass, "Clearing state cache inside an active render pass");
    m_pActiveRenderPass = nullptr;
    m_pBoundFramebuffer = nullptr;
}

template <typename ImplementationTraits>
bool DeviceContextBase<ImplementationTraits>::CheckIfBoundAsRenderTarget(TextureImplType* pTexture)
{
    if (pTexture == nullptr)
        return false;

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (m_pBoundRenderTargets[rt] && m_pBoundRenderTargets[rt]->GetTexture() == pTexture)
        {
            return true;
        }
    }

    return false;
}

template <typename ImplementationTraits>
bool DeviceContextBase<ImplementationTraits>::CheckIfBoundAsDepthStencil(TextureImplType* pTexture)
{
    if (pTexture == nullptr)
        return false;

    return m_pBoundDepthStencil && m_pBoundDepthStencil->GetTexture() == pTexture;
}

template <typename ImplementationTraits>
bool DeviceContextBase<ImplementationTraits>::UnbindTextureFromFramebuffer(TextureImplType* pTexture, bool bShowMessage)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass.");

    if (pTexture == nullptr)
        return false;

    const auto& TexDesc = pTexture->GetDesc();

    bool bResetRenderTargets = false;
    if (TexDesc.BindFlags & BIND_RENDER_TARGET)
    {
        if (CheckIfBoundAsRenderTarget(pTexture))
        {
            if (bShowMessage)
            {
                LOG_INFO_MESSAGE("Texture '", TexDesc.Name,
                                 "' is currently bound as render target and will be unset along with all "
                                 "other render targets and depth-stencil buffer. "
                                 "Call SetRenderTargets() to reset the render targets.\n"
                                 "To silence this message, explicitly unbind the texture with "
                                 "SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE)");
            }

            bResetRenderTargets = true;
        }
    }

    if (TexDesc.BindFlags & BIND_DEPTH_STENCIL)
    {
        if (CheckIfBoundAsDepthStencil(pTexture))
        {
            if (bShowMessage)
            {
                LOG_INFO_MESSAGE("Texture '", TexDesc.Name,
                                 "' is currently bound as depth buffer and will be unset along with "
                                 "all render targets. Call SetRenderTargets() to reset the render targets.\n"
                                 "To silence this message, explicitly unbind the texture with "
                                 "SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE)");
            }

            bResetRenderTargets = true;
        }
    }

    if (bResetRenderTargets)
    {
        ResetRenderTargets();
    }

    return bResetRenderTargets;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::ResetRenderTargets()
{
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
        m_pBoundRenderTargets[rt].Release();
#ifdef DILIGENT_DEBUG
    for (Uint32 rt = m_NumBoundRenderTargets; rt < _countof(m_pBoundRenderTargets); ++rt)
    {
        VERIFY(m_pBoundRenderTargets[rt] == nullptr, "Non-null render target found");
    }
#endif
    m_NumBoundRenderTargets = 0;
    m_FramebufferWidth      = 0;
    m_FramebufferHeight     = 0;
    m_FramebufferSlices     = 0;
    m_FramebufferSamples    = 0;

    m_pBoundDepthStencil.Release();
    m_pBoundShadingRateMap.Release();

    // Do not reset framebuffer here as there may potentially
    // be a subpass without any render target attachments.
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "BeginRenderPass");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Attempting to begin render pass while another render pass ('", m_pActiveRenderPass->GetDesc().Name, "') is active.");
    DEV_CHECK_ERR(m_pBoundFramebuffer == nullptr, "Attempting to begin render pass while another framebuffer ('", m_pBoundFramebuffer->GetDesc().Name, "') is bound.");

    VerifyBeginRenderPassAttribs(Attribs);

    // Reset current render targets (in Vulkan backend, this may end current render pass).
    ResetRenderTargets();

    auto* pNewRenderPass  = ClassPtrCast<RenderPassImplType>(Attribs.pRenderPass);
    auto* pNewFramebuffer = ClassPtrCast<FramebufferImplType>(Attribs.pFramebuffer);
    if (Attribs.StateTransitionMode != RESOURCE_STATE_TRANSITION_MODE_NONE)
    {
        const auto& RPDesc = pNewRenderPass->GetDesc();
        const auto& FBDesc = pNewFramebuffer->GetDesc();
        DEV_CHECK_ERR(RPDesc.AttachmentCount <= FBDesc.AttachmentCount,
                      "The number of attachments (", FBDesc.AttachmentCount,
                      ") in currently bound framebuffer is smaller than the number of attachments in the render pass (", RPDesc.AttachmentCount, ")");
        const bool IsMetal = m_pDevice->GetDeviceInfo().IsMetalDevice();
        for (Uint32 i = 0; i < FBDesc.AttachmentCount; ++i)
        {
            auto* pView = FBDesc.ppAttachments[i];
            if (pView == nullptr)
                continue;

            if (IsMetal && pView->GetDesc().ViewType == TEXTURE_VIEW_SHADING_RATE)
                continue;

            auto* pTex          = ClassPtrCast<TextureImplType>(pView->GetTexture());
            auto  RequiredState = RPDesc.pAttachments[i].InitialState;
            if (Attribs.StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
            {
                if (pTex->IsInKnownState() && !pTex->CheckState(RequiredState))
                {
                    StateTransitionDesc Barrier{pTex, RESOURCE_STATE_UNKNOWN, RequiredState, STATE_TRANSITION_FLAG_UPDATE_STATE};
                    this->TransitionResourceStates(1, &Barrier);
                }
            }
            else if (Attribs.StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
            {
                DvpVerifyTextureState(*pTex, RequiredState, "BeginRenderPass");
            }
        }
    }

    m_pActiveRenderPass                   = pNewRenderPass;
    m_pBoundFramebuffer                   = pNewFramebuffer;
    m_SubpassIndex                        = 0;
    m_RenderPassAttachmentsTransitionMode = Attribs.StateTransitionMode;

    UpdateAttachmentStates(m_SubpassIndex);
    SetSubpassRenderTargets();
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::NextSubpass()
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "NextSubpass");
    DEV_CHECK_ERR(m_pActiveRenderPass != nullptr, "There is no active render pass");
    VERIFY(m_SubpassIndex + 1 < m_pActiveRenderPass->GetDesc().SubpassCount, "The render pass has reached the final subpass already");
    ++m_SubpassIndex;
    UpdateAttachmentStates(m_SubpassIndex);
    SetSubpassRenderTargets();
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::UpdateAttachmentStates(Uint32 SubpassIndex)
{
    if (m_RenderPassAttachmentsTransitionMode != RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        return;

    DEV_CHECK_ERR(m_pActiveRenderPass != nullptr, "There is no active render pass");
    DEV_CHECK_ERR(m_pBoundFramebuffer != nullptr, "There is no active framebuffer");

    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY(FBDesc.AttachmentCount == RPDesc.AttachmentCount,
           "Framebuffer attachment count (", FBDesc.AttachmentCount, ") is not consistent with the render pass attachment count (", RPDesc.AttachmentCount, ")");
    VERIFY_EXPR(SubpassIndex <= RPDesc.SubpassCount);
    const bool IsMetal = m_pDevice->GetDeviceInfo().IsMetalDevice();
    for (Uint32 i = 0; i < RPDesc.AttachmentCount; ++i)
    {
        if (auto* pView = FBDesc.ppAttachments[i])
        {
            if (IsMetal && pView->GetDesc().ViewType == TEXTURE_VIEW_SHADING_RATE)
                continue;

            auto* pTex = ClassPtrCast<TextureImplType>(pView->GetTexture());
            if (pTex->IsInKnownState())
            {
                auto CurrState = SubpassIndex < RPDesc.SubpassCount ?
                    m_pActiveRenderPass->GetAttachmentState(SubpassIndex, i) :
                    RPDesc.pAttachments[i].FinalState;
                pTex->SetState(CurrState);
            }
        }
    }
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::EndRenderPass()
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "EndRenderPass");
    DEV_CHECK_ERR(m_pActiveRenderPass != nullptr, "There is no active render pass");
    DEV_CHECK_ERR(m_pBoundFramebuffer != nullptr, "There is no active framebuffer");
    VERIFY(m_pActiveRenderPass->GetDesc().SubpassCount == m_SubpassIndex + 1,
           "Ending render pass at subpass ", m_SubpassIndex, " before reaching the final subpass");

    UpdateAttachmentStates(m_SubpassIndex + 1);

    m_pActiveRenderPass.Release();
    m_pBoundFramebuffer.Release();
    m_SubpassIndex                        = 0;
    m_RenderPassAttachmentsTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;
    ResetRenderTargets();
}


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::ClearDepthStencil(ITextureView* pView)
{
    DEV_CHECK_ERR(pView != nullptr, "Depth-stencil view to clear must not be null");

    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "ClearDepthStencil");

#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pView->GetDesc();
        DEV_CHECK_ERR(ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL,
                      "The type (", GetTexViewTypeLiteralName(ViewDesc.ViewType), ") of the texture view '", ViewDesc.Name,
                      "' is invalid: ClearDepthStencil command expects depth-stencil view (TEXTURE_VIEW_DEPTH_STENCIL).");

        if (pView != m_pBoundDepthStencil)
        {
            DEV_CHECK_ERR(m_pActiveRenderPass == nullptr,
                          "Depth-stencil view '", ViewDesc.Name,
                          "' is not bound as framebuffer attachment. ClearDepthStencil command inside a render pass "
                          "requires depth-stencil view to be bound as a framebuffer attachment.");

            if (m_pDevice->GetDeviceInfo().IsGLDevice())
            {
                LOG_ERROR_MESSAGE("Depth-stencil view '", ViewDesc.Name,
                                  "' is not bound to the device context. ClearDepthStencil command requires "
                                  "depth-stencil view be bound to the device context in OpenGL backend");
            }
            else
            {
                LOG_WARNING_MESSAGE("Depth-stencil view '", ViewDesc.Name,
                                    "' is not bound to the device context. "
                                    "ClearDepthStencil command is more efficient when depth-stencil "
                                    "view is bound to the context. In OpenGL backend this is a requirement.");
            }
        }
    }
#endif

    ++m_Stats.CommandCounters.ClearDepthStencil;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::ClearRenderTarget(ITextureView* pView)
{
    DEV_CHECK_ERR(pView != nullptr, "Render target view to clear must not be null");
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "ClearRenderTarget");

#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pView->GetDesc();
        DEV_CHECK_ERR(ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET,
                      "The type (", GetTexViewTypeLiteralName(ViewDesc.ViewType), ") of texture view '", pView->GetDesc().Name,
                      "' is invalid: ClearRenderTarget command expects render target view (TEXTURE_VIEW_RENDER_TARGET).");

        bool RTFound = false;
        for (Uint32 i = 0; i < m_NumBoundRenderTargets && !RTFound; ++i)
        {
            RTFound = m_pBoundRenderTargets[i] == pView;
        }

        if (!RTFound)
        {
            DEV_CHECK_ERR(m_pActiveRenderPass == nullptr,
                          "Render target view '", ViewDesc.Name,
                          "' is not bound as framebuffer attachment. ClearRenderTarget command inside a render pass "
                          "requires render target view to be bound as a framebuffer attachment.");

            if (m_pDevice->GetDeviceInfo().IsGLDevice())
            {
                LOG_ERROR_MESSAGE("Render target view '", ViewDesc.Name,
                                  "' is not bound to the device context. ClearRenderTarget command "
                                  "requires render target view to be bound to the device context in OpenGL backend");
            }
            else
            {
                LOG_WARNING_MESSAGE("Render target view '", ViewDesc.Name,
                                    "' is not bound to the device context. ClearRenderTarget command is more efficient "
                                    "if render target view is bound to the device context. In OpenGL backend this is a requirement.");
            }
        }
    }
#endif

    ++m_Stats.CommandCounters.ClearRenderTarget;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::BeginQuery(IQuery* pQuery, int)
{
    DEV_CHECK_ERR(pQuery != nullptr, "IDeviceContext::BeginQuery: pQuery must not be null");

    const auto QueryType = pQuery->GetDesc().Type;
    DEV_CHECK_ERR(QueryType != QUERY_TYPE_TIMESTAMP,
                  "BeginQuery() is disabled for timestamp queries. Call EndQuery() to set the timestamp.");

    const auto QueueType = QueryType == QUERY_TYPE_DURATION ? COMMAND_QUEUE_TYPE_TRANSFER : COMMAND_QUEUE_TYPE_GRAPHICS;
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(QueueType, "BeginQuery for query type ", GetQueryTypeString(QueryType));

    ClassPtrCast<QueryImplType>(pQuery)->OnBeginQuery(static_cast<DeviceContextImplType*>(this));

    ++m_Stats.CommandCounters.BeginQuery;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::EndQuery(IQuery* pQuery, int)
{
    DEV_CHECK_ERR(pQuery != nullptr, "IDeviceContext::EndQuery: pQuery must not be null");

    const auto QueryType = pQuery->GetDesc().Type;
    const auto QueueType = QueryType == QUERY_TYPE_DURATION || QueryType == QUERY_TYPE_TIMESTAMP ? COMMAND_QUEUE_TYPE_TRANSFER : COMMAND_QUEUE_TYPE_GRAPHICS;
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(QueueType, "EndQuery for query type ", GetQueryTypeString(QueryType));

    ClassPtrCast<QueryImplType>(pQuery)->OnEndQuery(static_cast<DeviceContextImplType*>(this));
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::EnqueueSignal(IFence* pFence, Uint64 Value, int)
{
    DEV_CHECK_ERR(!IsDeferred(), "Fence signal can only be enqueued from immediate context");
    DEV_CHECK_ERR(pFence != nullptr, "Fence must not be null");
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DeviceWaitForFence(IFence* pFence, Uint64 Value, int)
{
    DEV_CHECK_ERR(!IsDeferred(), "Fence can only be waited from immediate context");
    DEV_CHECK_ERR(pFence, "Fence must not be null");
    DEV_CHECK_ERR(pFence->GetDesc().Type == FENCE_TYPE_GENERAL, "Fence must be created with FENCE_TYPE_GENERAL");
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::UpdateBuffer(
    IBuffer*                       pBuffer,
    Uint64                         Offset,
    Uint64                         Size,
    const void*                    pData,
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_TRANSFER, "UpdateBuffer");
    DEV_CHECK_ERR(pBuffer != nullptr, "Buffer must not be null");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "UpdateBuffer command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& BuffDesc = ClassPtrCast<BufferImplType>(pBuffer)->GetDesc();
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DEFAULT || BuffDesc.Usage == USAGE_SPARSE, "Unable to update buffer '", BuffDesc.Name, "': only USAGE_DEFAULT or USAGE_SPARSE buffers can be updated with UpdateData()");
        DEV_CHECK_ERR(Offset < BuffDesc.Size, "Unable to update buffer '", BuffDesc.Name, "': offset (", Offset, ") exceeds the buffer size (", BuffDesc.Size, ")");
        DEV_CHECK_ERR(Size + Offset <= BuffDesc.Size, "Unable to update buffer '", BuffDesc.Name, "': Update region [", Offset, ",", Size + Offset, ") is out of buffer bounds [0,", BuffDesc.Size, ")");
    }
#endif

    ++m_Stats.CommandCounters.UpdateBuffer;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::CopyBuffer(
    IBuffer*                       pSrcBuffer,
    Uint64                         SrcOffset,
    RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
    IBuffer*                       pDstBuffer,
    Uint64                         DstOffset,
    Uint64                         Size,
    RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_TRANSFER, "CopyBuffer");
    DEV_CHECK_ERR(pSrcBuffer != nullptr, "Source buffer must not be null");
    DEV_CHECK_ERR(pDstBuffer != nullptr, "Destination buffer must not be null");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "CopyBuffer command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& SrcBufferDesc = ClassPtrCast<BufferImplType>(pSrcBuffer)->GetDesc();
        const auto& DstBufferDesc = ClassPtrCast<BufferImplType>(pDstBuffer)->GetDesc();
        DEV_CHECK_ERR(DstOffset + Size <= DstBufferDesc.Size, "Failed to copy buffer '", SrcBufferDesc.Name, "' to '", DstBufferDesc.Name, "': Destination range [", DstOffset, ",", DstOffset + Size, ") is out of buffer bounds [0,", DstBufferDesc.Size, ")");
        DEV_CHECK_ERR(SrcOffset + Size <= SrcBufferDesc.Size, "Failed to copy buffer '", SrcBufferDesc.Name, "' to '", DstBufferDesc.Name, "': Source range [", SrcOffset, ",", SrcOffset + Size, ") is out of buffer bounds [0,", SrcBufferDesc.Size, ")");
    }
#endif

    ++m_Stats.CommandCounters.CopyBuffer;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::MapBuffer(
    IBuffer*  pBuffer,
    MAP_TYPE  MapType,
    MAP_FLAGS MapFlags,
    PVoid&    pMappedData)
{
    DEV_CHECK_ERR(pBuffer, "pBuffer must not be null");

    const auto& BuffDesc = pBuffer->GetDesc();

#ifdef DILIGENT_DEBUG
    {
        VERIFY(m_DbgMappedBuffers.find(pBuffer) == m_DbgMappedBuffers.end(), "Buffer '", BuffDesc.Name, "' has already been mapped");
        m_DbgMappedBuffers[pBuffer] = DbgMappedBufferInfo{MapType};
    }
#endif

    pMappedData = nullptr;
    switch (MapType)
    {
        case MAP_READ:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING or USAGE_UNIFIED can be mapped for reading");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_READ), "Buffer being mapped for reading was not created with CPU_ACCESS_READ flag");
            DEV_CHECK_ERR((MapFlags & MAP_FLAG_DISCARD) == 0, "MAP_FLAG_DISCARD is not valid when mapping buffer for reading");
            break;

        case MAP_WRITE:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DYNAMIC || BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING, USAGE_DYNAMIC or USAGE_UNIFIED can be mapped for writing");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Buffer being mapped for writing was not created with CPU_ACCESS_WRITE flag");
            break;

        case MAP_READ_WRITE:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING or USAGE_UNIFIED can be mapped for reading and writing");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Buffer being mapped for reading & writing was not created with CPU_ACCESS_WRITE flag");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_READ), "Buffer being mapped for reading & writing was not created with CPU_ACCESS_READ flag");
            DEV_CHECK_ERR((MapFlags & MAP_FLAG_DISCARD) == 0, "MAP_FLAG_DISCARD is not valid when mapping buffer for reading and writing");
            break;

        default: UNEXPECTED("Unknown map type");
    }

    if (BuffDesc.Usage == USAGE_DYNAMIC)
    {
        DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0 && MapType == MAP_WRITE, "Dynamic buffers can only be mapped for writing with MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flag");
        DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE), "When mapping dynamic buffer, only one of MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flags must be specified");
    }

    if ((MapFlags & MAP_FLAG_DISCARD) != 0)
    {
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DYNAMIC || BuffDesc.Usage == USAGE_STAGING, "Only dynamic and staging buffers can be mapped with discard flag");
        DEV_CHECK_ERR(MapType == MAP_WRITE, "MAP_FLAG_DISCARD is only valid when mapping buffer for writing");
    }

    ++m_Stats.CommandCounters.MapBuffer;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    VERIFY(pBuffer, "pBuffer must not be null");
#ifdef DILIGENT_DEBUG
    {
        auto MappedBufferIt = m_DbgMappedBuffers.find(pBuffer);
        VERIFY(MappedBufferIt != m_DbgMappedBuffers.end(), "Buffer '", pBuffer->GetDesc().Name, "' has not been mapped.");
        VERIFY(MappedBufferIt->second.MapType == MapType, "MapType (", MapType, ") does not match the map type that was used to map the buffer ", MappedBufferIt->second.MapType);
        m_DbgMappedBuffers.erase(MappedBufferIt);
    }
#endif
}


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::UpdateTexture(
    ITexture*                      pTexture,
    Uint32                         MipLevel,
    Uint32                         Slice,
    const Box&                     DstBox,
    const TextureSubResData&       SubresData,
    RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
    RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_TRANSFER, "UpdateTexture");
    DEV_CHECK_ERR(pTexture != nullptr, "pTexture must not be null");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "UpdateTexture command must be used outside of render pass.");

    ValidateUpdateTextureParams(pTexture->GetDesc(), MipLevel, Slice, DstBox, SubresData);
    ++m_Stats.CommandCounters.UpdateTexture;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_TRANSFER, "CopyTexture");
    DEV_CHECK_ERR(CopyAttribs.pSrcTexture, "Src texture must not be null");
    DEV_CHECK_ERR(CopyAttribs.pDstTexture, "Dst texture must not be null");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "CopyTexture command must be used outside of render pass.");

    ValidateCopyTextureParams(CopyAttribs);
    ++m_Stats.CommandCounters.CopyTexture;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::MapTextureSubresource(
    ITexture*                 pTexture,
    Uint32                    MipLevel,
    Uint32                    ArraySlice,
    MAP_TYPE                  MapType,
    MAP_FLAGS                 MapFlags,
    const Box*                pMapRegion,
    MappedTextureSubresource& MappedData)
{
    DEV_CHECK_ERR(pTexture, "pTexture must not be null");
    ValidateMapTextureParams(pTexture->GetDesc(), MipLevel, ArraySlice, MapType, MapFlags, pMapRegion);
    ++m_Stats.CommandCounters.MapTextureSubresource;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::UnmapTextureSubresource(
    ITexture* pTexture,
    Uint32    MipLevel,
    Uint32    ArraySlice)
{
    DEV_CHECK_ERR(pTexture, "pTexture must not be null");
    DEV_CHECK_ERR(MipLevel < pTexture->GetDesc().MipLevels, "Mip level is out of range");
    DEV_CHECK_ERR(ArraySlice < pTexture->GetDesc().GetArraySize(), "Array slice is out of range");
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::GenerateMips(ITextureView* pTexView)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "GenerateMips");
    DEV_CHECK_ERR(pTexView != nullptr, "pTexView must not be null");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "GenerateMips command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pTexView->GetDesc();
        DEV_CHECK_ERR(ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE, "Shader resource view '", ViewDesc.Name,
                      "' can't be used to generate mipmaps because its type is ", GetTexViewTypeLiteralName(ViewDesc.ViewType), ". Required view type: TEXTURE_VIEW_SHADER_RESOURCE.");
        DEV_CHECK_ERR((ViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0, "Shader resource view '", ViewDesc.Name,
                      "' was not created with TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag and can't be used to generate mipmaps.");
    }
#endif
    ++m_Stats.CommandCounters.GenerateMips;
}


template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::ResolveTextureSubresource(
    ITexture*                               pSrcTexture,
    ITexture*                               pDstTexture,
    const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
#ifdef DILIGENT_DEVELOPMENT
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "ResolveTextureSubresource");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "ResolveTextureSubresource command must be used outside of render pass.");

    DEV_CHECK_ERR(pSrcTexture != nullptr && pDstTexture != nullptr, "Src and Dst textures must not be null");
    const auto& SrcTexDesc = pSrcTexture->GetDesc();
    const auto& DstTexDesc = pDstTexture->GetDesc();

    VerifyResolveTextureSubresourceAttribs(ResolveAttribs, SrcTexDesc, DstTexDesc);
#endif
    ++m_Stats.CommandCounters.ResolveTextureSubresource;
}


template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::BuildBLAS(const BuildBLASAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "BuildBLAS");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::BuildBLAS: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::BuildBLAS command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyBuildBLASAttribs(Attribs, m_pDevice), "BuildBLASAttribs are invalid");

    ++m_Stats.CommandCounters.BuildBLAS;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::BuildTLAS(const BuildTLASAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "BuildTLAS");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::BuildTLAS: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::BuildTLAS command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyBuildTLASAttribs(Attribs, m_pDevice->GetAdapterInfo().RayTracing), "BuildTLASAttribs are invalid");

    ++m_Stats.CommandCounters.BuildTLAS;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::CopyBLAS(const CopyBLASAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "CopyBLAS");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::CopyBLAS: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::CopyBLAS command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyCopyBLASAttribs(m_pDevice, Attribs), "CopyBLASAttribs are invalid");

    ++m_Stats.CommandCounters.CopyBLAS;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::CopyTLAS(const CopyTLASAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "CopyTLAS");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::CopyTLAS: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::CopyTLAS command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyCopyTLASAttribs(Attribs), "CopyTLASAttribs are invalid");
    DEV_CHECK_ERR(ClassPtrCast<TopLevelASType>(Attribs.pSrc)->ValidateContent(), "IDeviceContext::CopyTLAS: pSrc acceleration structure is not valid");

    ++m_Stats.CommandCounters.CopyTLAS;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "WriteBLASCompactedSize");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::WriteBLASCompactedSize: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::WriteBLASCompactedSize: command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyWriteBLASCompactedSizeAttribs(m_pDevice, Attribs), "WriteBLASCompactedSizeAttribs are invalid");

    ++m_Stats.CommandCounters.WriteBLASCompactedSize;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "WriteTLASCompactedSize");
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::WriteTLASCompactedSize: ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::WriteTLASCompactedSize: command must be performed outside of render pass");
    DEV_CHECK_ERR(VerifyWriteTLASCompactedSizeAttribs(m_pDevice, Attribs), "WriteTLASCompactedSizeAttribs are invalid");

    ++m_Stats.CommandCounters.WriteTLASCompactedSize;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::TraceRays(const TraceRaysAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "TraceRays");

    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing,
                  "IDeviceContext::TraceRays: ray tracing is not supported by this device");
    const auto& RTProps = m_pDevice->GetAdapterInfo().RayTracing;
    DEV_CHECK_ERR((RTProps.CapFlags & RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS) != 0,
                  "IDeviceContext::TraceRays: standalone ray tracing shaders are not supported by this device");
    DEV_CHECK_ERR(m_pPipelineState,
                  "IDeviceContext::TraceRays command arguments are invalid: no pipeline state is bound.");
    DEV_CHECK_ERR(m_pPipelineState->GetDesc().IsRayTracingPipeline(),
                  "IDeviceContext::TraceRays command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name, "' is not a ray tracing pipeline.");

    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::TraceRays must be performed outside of render pass");

    DEV_CHECK_ERR(VerifyTraceRaysAttribs(Attribs), "TraceRaysAttribs are invalid");

    DEV_CHECK_ERR(PipelineStateImplType::IsSameObject(m_pPipelineState, ClassPtrCast<PipelineStateImplType>(Attribs.pSBT->GetDesc().pPSO)),
                  "IDeviceContext::TraceRays command arguments are invalid: currently bound pipeline '", m_pPipelineState->GetDesc().Name,
                  "' doesn't match the pipeline '", Attribs.pSBT->GetDesc().pPSO->GetDesc().Name, "' that was used in ShaderBindingTable");

    const auto* pSBTImpl = ClassPtrCast<const ShaderBindingTableImplType>(Attribs.pSBT);
    DEV_CHECK_ERR(!pSBTImpl->HasPendingData(), "IDeviceContext::TraceRaysIndirect command arguments are invalid: SBT '",
                  pSBTImpl->GetDesc().Name, "' has uncommitted changes, call UpdateSBT() first");

    VERIFY(pSBTImpl->GetInternalBuffer() != nullptr,
           "SBT '", pSBTImpl->GetDesc().Name, "' internal buffer must not be null, this should never happen, ",
           "because HasPendingData() must've returned true triggering the assert above.");
    VERIFY(pSBTImpl->GetInternalBuffer()->CheckState(RESOURCE_STATE_RAY_TRACING),
           "SBT '", pSBTImpl->GetDesc().Name, "' internal buffer is expected to be in RESOURCE_STATE_RAY_TRACING, but current state is ",
           GetResourceStateString(pSBTImpl->GetInternalBuffer()->GetState()));

    DEV_CHECK_ERR((Attribs.DimensionX * Attribs.DimensionY * Attribs.DimensionZ) <= RTProps.MaxRayGenThreads,
                  "IDeviceContext::TraceRays command arguments are invalid: the dimension must not exceed the ",
                  RTProps.MaxRayGenThreads, " threads");

    ++m_Stats.CommandCounters.TraceRays;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_COMPUTE, "TraceRaysIndirect");

    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing,
                  "IDeviceContext::TraceRaysIndirect: ray tracing is not supported by this device");
    const auto& RTProps = m_pDevice->GetAdapterInfo().RayTracing;
    DEV_CHECK_ERR((RTProps.CapFlags & RAY_TRACING_CAP_FLAG_INDIRECT_RAY_TRACING) != 0,
                  "IDeviceContext::TraceRays: indirect ray tracing is not supported by this device");
    DEV_CHECK_ERR(m_pPipelineState,
                  "IDeviceContext::TraceRaysIndirect command arguments are invalid: no pipeline state is bound.");
    DEV_CHECK_ERR(m_pPipelineState->GetDesc().IsRayTracingPipeline(),
                  "IDeviceContext::TraceRaysIndirect command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name,
                  "' is not a ray tracing pipeline.");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr,
                  "IDeviceContext::TraceRaysIndirect must be performed outside of render pass");

    DEV_CHECK_ERR(VerifyTraceRaysIndirectAttribs(m_pDevice, Attribs, TraceRaysIndirectCommandSize),
                  "TraceRaysIndirectAttribs are invalid");

    DEV_CHECK_ERR(PipelineStateImplType::IsSameObject(m_pPipelineState, ClassPtrCast<PipelineStateImplType>(Attribs.pSBT->GetDesc().pPSO)),
                  "IDeviceContext::TraceRaysIndirect command arguments are invalid: currently bound pipeline '", m_pPipelineState->GetDesc().Name,
                  "' doesn't match the pipeline '", Attribs.pSBT->GetDesc().pPSO->GetDesc().Name, "' that was used in ShaderBindingTable");

    const auto* pSBTImpl = ClassPtrCast<const ShaderBindingTableImplType>(Attribs.pSBT);
    DEV_CHECK_ERR(!pSBTImpl->HasPendingData(),
                  "IDeviceContext::TraceRaysIndirect command arguments are invalid: SBT '",
                  pSBTImpl->GetDesc().Name, "' has uncommitted changes, call UpdateSBT() first");


    VERIFY(pSBTImpl->GetInternalBuffer() != nullptr,
           "SBT '", pSBTImpl->GetDesc().Name, "' internal buffer must not be null, this should never happen, ",
           "because HasPendingData() must've returned true triggering the assert above.");
    VERIFY(pSBTImpl->GetInternalBuffer()->CheckState(RESOURCE_STATE_RAY_TRACING),
           "SBT '", pSBTImpl->GetDesc().Name, "' internal buffer is expected to be in RESOURCE_STATE_RAY_TRACING, but current state is ",
           GetResourceStateString(pSBTImpl->GetInternalBuffer()->GetState()));

    ++m_Stats.CommandCounters.TraceRaysIndirect;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs, int)
{
    DEV_CHECK_ERR(m_pDevice->GetFeatures().RayTracing, "IDeviceContext::UpdateSBT: ray tracing is not supported by this device");
    DEV_CHECK_ERR((m_pDevice->GetAdapterInfo().RayTracing.CapFlags & RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS) != 0,
                  "IDeviceContext::UpdateSBT: standalone ray tracing shaders are not supported by this device");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "IDeviceContext::UpdateSBT must be performed outside of render pass");
    DEV_CHECK_ERR(pSBT != nullptr, "IDeviceContext::UpdateSBT command arguments are invalid: pSBT must not be null");

    if (pUpdateIndirectBufferAttribs != nullptr)
    {
        DEV_CHECK_ERR(pUpdateIndirectBufferAttribs->pAttribsBuffer != nullptr,
                      "IDeviceContext::UpdateSBT command arguments are invalid: pUpdateIndirectBufferAttribs->pAttribsBuffer must not be null");
    }

    ++m_Stats.CommandCounters.UpdateSBT;
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::BeginDebugGroup(const Char* Name, const float* pColor, int)
{
    DEV_CHECK_ERR(Name != nullptr, "Name must not be null");
#ifdef DILIGENT_DEVELOPMENT
    ++m_DvpDebugGroupCount;
#endif
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::EndDebugGroup(int)
{
#ifdef DILIGENT_DEVELOPMENT
    DEV_CHECK_ERR(m_DvpDebugGroupCount > 0, "There is no active debug group to end");
    --m_DvpDebugGroupCount;
#endif
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::InsertDebugLabel(const Char* Label, const float* pColor, int) const
{
    DEV_CHECK_ERR(Label != nullptr, "Label must not be null");
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::SetShadingRate(SHADING_RATE BaseRate, SHADING_RATE_COMBINER PrimitiveCombiner, SHADING_RATE_COMBINER TextureCombiner, int) const
{
#ifdef DILIGENT_DEVELOPMENT
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "SetShadingRate");

    DEV_CHECK_ERR(IsPowerOfTwo(PrimitiveCombiner), "Only one primitive combiner must be specified");
    DEV_CHECK_ERR(IsPowerOfTwo(TextureCombiner), "Only one texture combiner must be specified");
    DEV_CHECK_ERR(m_pDevice->GetDeviceInfo().Features.VariableRateShading, "IDeviceContext::SetShadingRate: VariableRateShading feature must be enabled");

    const auto& SRProps = m_pDevice->GetAdapterInfo().ShadingRate;
    DEV_CHECK_ERR(SRProps.CapFlags & (SHADING_RATE_CAP_FLAG_PER_DRAW | SHADING_RATE_CAP_FLAG_PER_PRIMITIVE | SHADING_RATE_CAP_FLAG_TEXTURE_BASED),
                  "IDeviceContext::SetShadingRate: requires one of the following capabilities: SHADING_RATE_CAP_FLAG_PER_DRAW, "
                  "SHADING_RATE_CAP_FLAG_PER_PRIMITIVE, or SHADING_RATE_CAP_FLAG_TEXTURE_BASED");
    if (SRProps.CapFlags & SHADING_RATE_CAP_FLAG_PER_PRIMITIVE)
        DEV_CHECK_ERR(SRProps.Combiners & PrimitiveCombiner, "IDeviceContext::SetShadingRate: PrimitiveCombiner must be one of the supported combiners");
    else
        DEV_CHECK_ERR(PrimitiveCombiner == SHADING_RATE_COMBINER_PASSTHROUGH, "IDeviceContext::SetShadingRate: PrimitiveCombiner must be PASSTHROUGH when per primitive shading is not supported");

    if (SRProps.CapFlags & SHADING_RATE_CAP_FLAG_TEXTURE_BASED)
        DEV_CHECK_ERR(SRProps.Combiners & TextureCombiner, "IDeviceContext::SetShadingRate: TextureCombiner must be one of the supported combiners");
    else
        DEV_CHECK_ERR(TextureCombiner == SHADING_RATE_COMBINER_PASSTHROUGH, "IDeviceContext::SetShadingRate: TextureCombiner must be PASSTHROUGH when texture based shading is not supported");

    bool IsSupported = false;
    for (Uint32 i = 0; i < SRProps.NumShadingRates && !IsSupported; ++i)
    {
        IsSupported = (SRProps.ShadingRates[i].Rate == BaseRate);
    }
    DEV_CHECK_ERR(IsSupported, "IDeviceContext::SetShadingRate: BaseRate must be one of the supported shading rates");
#endif
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs, int)
{
    DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_SPARSE_BINDING, "BindSparseResourceMemory");

    DEV_CHECK_ERR(!IsDeferred(), "BindSparseResourceMemory() should only be called for immediate contexts.");
    DEV_CHECK_ERR(m_pDevice->GetDeviceInfo().Features.SparseResources, "IDeviceContext::BindSparseResourceMemory: SparseResources feature must be enabled");
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Can not bind sparse memory inside an active render pass.");
    DEV_CHECK_ERR(VerifyBindSparseResourceMemoryAttribs(m_pDevice, Attribs), "BindSparseResourceMemoryAttribs are invalid");

    ++m_Stats.CommandCounters.BindSparseResourceMemory;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::PrepareCommittedResources(CommittedShaderResources& Resources, Uint32& DvpCompatibleSRBCount)
{
    const auto SignCount = m_pPipelineState->GetResourceSignatureCount();

    Resources.ActiveSRBMask = 0;
    for (Uint32 i = 0; i < SignCount; ++i)
    {
        const auto* pSignature = m_pPipelineState->GetResourceSignature(i);
        if (pSignature == nullptr || pSignature->GetTotalResourceCount() == 0)
            continue;

        Resources.ActiveSRBMask |= 1u << i;
    }

    DvpCompatibleSRBCount = 0;

#ifdef DILIGENT_DEVELOPMENT
    // Layout compatibility means that descriptor sets can be bound to a command buffer
    // for use by any pipeline created with a compatible pipeline layout, and without having bound
    // a particular pipeline first. It also means that descriptor sets can remain valid across
    // a pipeline change, and the same resources will be accessible to the newly bound pipeline.
    // (14.2.2. Pipeline Layouts, clause 'Pipeline Layout Compatibility')
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#descriptorsets-compatibility

    // Find the number of SRBs compatible with signatures in the current pipeline
    for (; DvpCompatibleSRBCount < SignCount; ++DvpCompatibleSRBCount)
    {
        const auto pSRB = Resources.SRBs[DvpCompatibleSRBCount].Lock();

        const auto* pPSOSign = m_pPipelineState->GetResourceSignature(DvpCompatibleSRBCount);
        const auto* pSRBSign = pSRB ? pSRB->GetSignature() : nullptr;

        if ((pPSOSign == nullptr || pPSOSign->GetTotalResourceCount() == 0) !=
            (pSRBSign == nullptr || pSRBSign->GetTotalResourceCount() == 0))
        {
            // One signature is null or empty while the other is not - SRB is not compatible with the PSO.
            break;
        }

        if (pPSOSign != nullptr && pSRBSign != nullptr && pPSOSign->IsIncompatibleWith(*pSRBSign))
        {
            // Signatures are incompatible
            break;
        }
    }

    // Unbind incompatible shader resources
    // A consequence of layout compatibility is that when the implementation compiles a pipeline
    // layout and maps pipeline resources to implementation resources, the mechanism for set N
    // should only be a function of sets [0..N].
    for (Uint32 sign = DvpCompatibleSRBCount; sign < SignCount; ++sign)
    {
        Resources.Set(sign, nullptr);
    }

    Resources.ResourcesValidated = false;
#endif
}

inline Uint32 GetPrimitiveCount(PRIMITIVE_TOPOLOGY Topology, Uint32 Elements)
{
    if (Topology >= PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST && Topology <= PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST)
    {
        return Elements / (Topology - PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1);
    }
    else
    {
        switch (Topology)
        {
            case PRIMITIVE_TOPOLOGY_UNDEFINED:
                UNEXPECTED("Undefined primitive topology");
                return 0;

            // clang-format off
            case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:      return Elements / 3;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:     return std::max(Elements, 2u) - 2;
            case PRIMITIVE_TOPOLOGY_POINT_LIST:         return Elements;
            case PRIMITIVE_TOPOLOGY_LINE_LIST:          return Elements / 2;
            case PRIMITIVE_TOPOLOGY_LINE_STRIP:         return std::max(Elements, 1u) - 1;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJ:  return Elements / 6;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ: return std::max(Elements, 4u) - 4;
            case PRIMITIVE_TOPOLOGY_LINE_LIST_ADJ:      return Elements / 4;
            case PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ:     return std::max(Elements, 3u) - 3;
            // clang-format on
            default: UNEXPECTED("Unexpected primitive topology"); return 0;
        }
    }
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::Draw(const DrawAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "Draw");

        DEV_CHECK_ERR(m_pPipelineState, "Draw command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "Draw command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(VerifyDrawAttribs(Attribs), "DrawAttribs are invalid");
    }
#endif
    if (m_pPipelineState)
    {
        const auto Topology = m_pPipelineState->GetGraphicsPipelineDesc().PrimitiveTopology;
        m_Stats.PrimitiveCounts[Topology] += GetPrimitiveCount(Topology, Attribs.NumVertices);
    }
    ++m_Stats.CommandCounters.Draw;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DrawIndexed(const DrawIndexedAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "DrawIndexed");

        DEV_CHECK_ERR(m_pPipelineState, "DrawIndexed command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "DrawIndexed command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(m_pIndexBuffer, "DrawIndexed command arguments are invalid: no index buffer is bound.");

        DEV_CHECK_ERR(VerifyDrawIndexedAttribs(Attribs), "DrawIndexedAttribs are invalid");
    }
#endif
    if (m_pPipelineState)
    {
        const auto Topology = m_pPipelineState->GetGraphicsPipelineDesc().PrimitiveTopology;
        m_Stats.PrimitiveCounts[Topology] += GetPrimitiveCount(Topology, Attribs.NumIndices);
    }
    ++m_Stats.CommandCounters.DrawIndexed;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DrawMesh(const DrawMeshAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "DrawMesh");

        DEV_CHECK_ERR(m_pDevice->GetFeatures().MeshShaders, "DrawMesh: mesh shaders are not supported by this device");

        DEV_CHECK_ERR(m_pPipelineState, "DrawMesh command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_MESH,
                      "DrawMesh command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a mesh pipeline.");

        DEV_CHECK_ERR(VerifyDrawMeshAttribs(m_pDevice->GetAdapterInfo().MeshShader, Attribs), "DrawMeshAttribs are invalid");
    }
#endif
    ++m_Stats.CommandCounters.DrawMesh;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DrawIndirect(const DrawIndirectAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "DrawIndirect");

        DEV_CHECK_ERR(Attribs.pCounterBuffer == nullptr || (m_pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER) != 0,
                      "DrawIndirect command arguments are invalid: counter buffer requires DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER capability");
        // There is no need to check DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT because an indirect buffer can only be created if this capability is supported.

        DEV_CHECK_ERR(m_pPipelineState, "DrawIndirect command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "DrawIndirect command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(m_pActiveRenderPass == nullptr || Attribs.AttribsBufferStateTransitionMode != RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                      "Resource state transitions are not allowed inside a render pass and may result in an undefined behavior. "
                      "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

        DEV_CHECK_ERR(VerifyDrawIndirectAttribs(Attribs), "DrawIndirectAttribs are invalid");
    }
#endif
    ++m_Stats.CommandCounters.DrawIndirect;
}


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "DrawIndexedIndirect");

        DEV_CHECK_ERR(Attribs.pCounterBuffer == nullptr || (m_pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER) != 0,
                      "DrawIndexedIndirect command arguments are invalid: counter buffer requires DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER capability");
        // There is no need to check DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT because an indirect buffer can only be created if this capability is supported.

        DEV_CHECK_ERR(m_pPipelineState, "DrawIndexedIndirect command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "DrawIndexedIndirect command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(m_pIndexBuffer, "DrawIndexedIndirect command arguments are invalid: no index buffer is bound.");

        DEV_CHECK_ERR(m_pActiveRenderPass == nullptr || Attribs.AttribsBufferStateTransitionMode != RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                      "Resource state transitions are not allowed inside a render pass and may result in an undefined behavior. "
                      "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

        DEV_CHECK_ERR(VerifyDrawIndexedIndirectAttribs(Attribs), "DrawIndexedIndirectAttribs are invalid");
    }
#endif
    ++m_Stats.CommandCounters.DrawIndexedIndirect;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "DrawMeshIndirect");

        DEV_CHECK_ERR(m_pDevice->GetFeatures().MeshShaders, "DrawMeshIndirect: mesh shaders are not supported by this device");

        DEV_CHECK_ERR(Attribs.pCounterBuffer == nullptr || (m_pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER) != 0,
                      "DrawMeshIndirect command arguments are invalid: counter buffer requires DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER capability");
        // There is no need to check DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT because an indirect buffer can only be created if this capability is supported.

        DEV_CHECK_ERR(m_pPipelineState, "DrawMeshIndirect command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_MESH,
                      "DrawMeshIndirect command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a mesh pipeline.");

        DEV_CHECK_ERR(VerifyDrawMeshIndirectAttribs(Attribs, DrawMeshIndirectCommandStride), "DrawMeshIndirectAttribs are invalid");
    }
#endif
    ++m_Stats.CommandCounters.DrawMeshIndirect;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::MultiDraw(const MultiDrawAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "MultiDraw");

        DEV_CHECK_ERR(m_pPipelineState, "MultiDraw command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "MultiDraw command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(VerifyMultiDrawAttribs(Attribs), "MultiDrawAttribs are invalid");
    }
#endif
    if (m_pPipelineState)
    {
        const auto Topology = m_pPipelineState->GetGraphicsPipelineDesc().PrimitiveTopology;
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
            m_Stats.PrimitiveCounts[Topology] += GetPrimitiveCount(Topology, Attribs.pDrawItems[i].NumVertices);
    }
    if (m_NativeMultiDrawSupported)
        ++m_Stats.CommandCounters.MultiDraw;
    else
        m_Stats.CommandCounters.Draw += Attribs.DrawCount;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::MultiDrawIndexed(const MultiDrawIndexedAttribs& Attribs, int)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) != 0)
    {
        DVP_CHECK_QUEUE_TYPE_COMPATIBILITY(COMMAND_QUEUE_TYPE_GRAPHICS, "MultiDrawIndexed");

        DEV_CHECK_ERR(m_pPipelineState, "MultiDrawIndexed command arguments are invalid: no pipeline state is bound.");

        DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_GRAPHICS,
                      "MultiDrawIndexed command arguments are invalid: pipeline state '",
                      m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");

        DEV_CHECK_ERR(m_pIndexBuffer, "MultiDrawIndexed command arguments are invalid: no index buffer is bound.");

        DEV_CHECK_ERR(VerifyMultiDrawIndexedAttribs(Attribs), "MultiDrawIndexedAttribs are invalid");
    }
#endif
    if (m_pPipelineState)
    {
        const auto Topology = m_pPipelineState->GetGraphicsPipelineDesc().PrimitiveTopology;
        for (Uint32 i = 0; i < Attribs.DrawCount; ++i)
            m_Stats.PrimitiveCounts[Topology] += GetPrimitiveCount(Topology, Attribs.pDrawItems[i].NumIndices);
    }
    if (m_NativeMultiDrawSupported)
        ++m_Stats.CommandCounters.MultiDrawIndexed;
    else
        m_Stats.CommandCounters.DrawIndexed += Attribs.DrawCount;
}

#ifdef DILIGENT_DEVELOPMENT
template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DvpVerifyRenderTargets() const
{
    DEV_CHECK_ERR(m_pPipelineState, "No pipeline state is bound");

    const auto& PSODesc = m_pPipelineState->GetDesc();
    DEV_CHECK_ERR(PSODesc.IsAnyGraphicsPipeline() || PSODesc.IsTilePipeline(),
                  "Pipeline state '", PSODesc.Name, "' is not a graphics pipeline");

    TEXTURE_FORMAT BoundRTVFormats[MAX_RENDER_TARGETS] = {};
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (const auto* pRT = m_pBoundRenderTargets[rt].RawPtr())
            BoundRTVFormats[rt] = pRT->GetDesc().Format;
        else
            BoundRTVFormats[rt] = TEX_FORMAT_UNKNOWN;
    }
    const auto BoundDSVFormat = m_pBoundDepthStencil ? m_pBoundDepthStencil->GetDesc().Format : TEX_FORMAT_UNKNOWN;

    Uint32                NumPipelineRenderTargets = 0;
    const TEXTURE_FORMAT* PipelineRTVFormats       = nullptr;
    TEXTURE_FORMAT        PipelineDSVFormat        = TEX_FORMAT_UNKNOWN;
    if (PSODesc.IsAnyGraphicsPipeline())
    {
        const auto& GraphicsPipeline = m_pPipelineState->GetGraphicsPipelineDesc();
        NumPipelineRenderTargets     = GraphicsPipeline.NumRenderTargets;
        PipelineRTVFormats           = GraphicsPipeline.RTVFormats;
        PipelineDSVFormat            = GraphicsPipeline.DSVFormat;
    }
    else if (PSODesc.IsTilePipeline())
    {
        const auto& TilePipeline = m_pPipelineState->GetTilePipelineDesc();
        NumPipelineRenderTargets = TilePipeline.NumRenderTargets;
        PipelineRTVFormats       = TilePipeline.RTVFormats;
        PipelineDSVFormat        = BoundDSVFormat; // to disable warning
    }
    else
    {
        UNEXPECTED("Unexpected pipeline type");
    }

    if (NumPipelineRenderTargets != m_NumBoundRenderTargets)
    {
        LOG_WARNING_MESSAGE("The number of currently bound render targets (", m_NumBoundRenderTargets,
                            ") does not match the number of outputs specified by the PSO '", PSODesc.Name,
                            "' (", NumPipelineRenderTargets, ").");
    }

    if (BoundDSVFormat != PipelineDSVFormat)
    {
        LOG_WARNING_MESSAGE("Currently bound depth-stencil buffer format (", GetTextureFormatAttribs(BoundDSVFormat).Name,
                            ") does not match the DSV format specified by the PSO '", PSODesc.Name,
                            "' (", GetTextureFormatAttribs(PipelineDSVFormat).Name, ").");
    }

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        auto BoundFmt = BoundRTVFormats[rt];
        auto PSOFmt   = PipelineRTVFormats[rt];
        if (BoundFmt != PSOFmt)
        {
            // NB: Vulkan requires exact match. In particular, if a PSO does not use an RTV, this RTV
            //     must be null.
            LOG_WARNING_MESSAGE("Render target bound to slot ", rt, " (", GetTextureFormatAttribs(BoundFmt).Name,
                                ") does not match the RTV format specified by the PSO '", PSODesc.Name,
                                "' (", GetTextureFormatAttribs(PSOFmt).Name, ").");
        }
    }

    // For compatibility with Vulkan, pipeline created to be used with shading rate texture must be used only when shading rate map is bound.
    if (m_pPipelineState->GetDesc().IsAnyGraphicsPipeline())
    {
        const bool PipelineWithVRSTexture = (m_pPipelineState->GetGraphicsPipelineDesc().ShadingRateFlags & PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED) != 0;
        if (PipelineWithVRSTexture)
        {
            DEV_CHECK_ERR(m_pBoundShadingRateMap != nullptr,
                          "Draw command uses pipeline state '", m_pPipelineState->GetDesc().Name,
                          "' that was created with ShadingRateFlags = PIPELINE_SHADING_RATE_TEXTURE_BASED, ",
                          "but shading rate texture is not bound; use IDeviceContext::SetRenderTargetsExt() with non-null pShadingRateMap "
                          "to bind the shading rate texture.");
        }
        else if (m_pBoundShadingRateMap != nullptr)
        {
            DEV_CHECK_ERR(PipelineWithVRSTexture,
                          "Draw command uses pipeline state '", m_pPipelineState->GetDesc().Name,
                          "' that was created without PIPELINE_SHADING_RATE_TEXTURE_BASED flag, ",
                          "but shading rate texture is bound; use IDeviceContext::SetRenderTargetsExt() with pShadingRateMap = null "
                          "to unbind the shading rate texture.");
        }
    }
}
#endif


template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DispatchCompute(const DispatchComputeAttribs& Attribs, int)
{
    DEV_CHECK_ERR(m_pPipelineState, "DispatchCompute command arguments are invalid: no pipeline state is bound.");

    DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_COMPUTE,
                  "DispatchCompute command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name,
                  "' is not a compute pipeline.");

    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr,
                  "DispatchCompute command must be performed outside of render pass");

    DEV_CHECK_ERR(VerifyDispatchComputeAttribs(Attribs), "DispatchComputeAttribs attribs");

    ++m_Stats.CommandCounters.DispatchCompute;
}

template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, int)
{
    DEV_CHECK_ERR(m_pPipelineState, "DispatchComputeIndirect command arguments are invalid: no pipeline state is bound.");

    DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_COMPUTE,
                  "DispatchComputeIndirect command arguments are invalid: pipeline state '",
                  m_pPipelineState->GetDesc().Name, "' is not a compute pipeline.");

    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "DispatchComputeIndirect command must be performed outside of render pass");

    DEV_CHECK_ERR(VerifyDispatchComputeIndirectAttribs(Attribs), "DispatchComputeIndirectAttribs are invalid");

    ++m_Stats.CommandCounters.DispatchComputeIndirect;
}

#ifdef DILIGENT_DEVELOPMENT
template <typename ImplementationTraits>
inline void DeviceContextBase<ImplementationTraits>::DvpVerifyDispatchTileArguments(const DispatchTileAttribs& Attribs) const
{
    DEV_CHECK_ERR(m_pPipelineState, "DispatchTile command arguments are invalid: no pipeline state is bound.");

    DEV_CHECK_ERR(m_pPipelineState->GetDesc().PipelineType == PIPELINE_TYPE_TILE,
                  "DispatchTile command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name,
                  "' is not a tile pipeline.");
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier) const
{
    DEV_CHECK_ERR(VerifyStateTransitionDesc(m_pDevice, Barrier, GetExecutionCtxId(), this->m_Desc), "StateTransitionDesc are invalid");
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifyTextureState(
    const TextureImplType& Texture,
    RESOURCE_STATE         RequiredState,
    const char*            OperationName) const
{
    if (Texture.IsInKnownState() && !Texture.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires texture '", Texture.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual texture state: ", GetResourceStateString(Texture.GetState()),
                          ". Use appropriate state transition flags or explicitly transition the texture using IDeviceContext::TransitionResourceStates() method.");
    }
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifyBufferState(
    const BufferImplType& Buffer,
    RESOURCE_STATE        RequiredState,
    const char*           OperationName) const
{
    if (Buffer.IsInKnownState() && !Buffer.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires buffer '", Buffer.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual buffer state: ", GetResourceStateString(Buffer.GetState()),
                          ". Use appropriate state transition flags or explicitly transition the buffer using IDeviceContext::TransitionResourceStates() method.");
    }
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifyBLASState(
    const BottomLevelASType& BLAS,
    RESOURCE_STATE           RequiredState,
    const char*              OperationName) const
{
    if (BLAS.IsInKnownState() && !BLAS.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires BLAS '", BLAS.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual BLAS state: ", GetResourceStateString(BLAS.GetState()),
                          ". Use appropriate state transition flags or explicitly transition the BLAS using IDeviceContext::TransitionResourceStates() method.");
    }
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifyTLASState(
    const TopLevelASType& TLAS,
    RESOURCE_STATE        RequiredState,
    const char*           OperationName) const
{
    if (TLAS.IsInKnownState() && !TLAS.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires TLAS '", TLAS.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual TLAS state: ", GetResourceStateString(TLAS.GetState()),
                          ". Use appropriate state transition flags or explicitly transition the TLAS using IDeviceContext::TransitionResourceStates() method.");
    }
}

template <typename ImplementationTraits>
void DeviceContextBase<ImplementationTraits>::DvpVerifySRBCompatibility(
    CommittedShaderResources&                                 Resources,
    std::function<PipelineResourceSignatureImplType*(Uint32)> CustomGetSignature) const
{
    DEV_CHECK_ERR(m_pPipelineState, "No PSO is bound in the context");

    const auto SignCount = m_pPipelineState->GetResourceSignatureCount();
    for (Uint32 sign = 0; sign < SignCount; ++sign)
    {
        const auto* const pPSOSign = CustomGetSignature ? CustomGetSignature(sign) : m_pPipelineState->GetResourceSignature(sign);
        if (pPSOSign == nullptr || pPSOSign->GetTotalResourceCount() == 0)
            continue; // Skip null and empty signatures

        VERIFY_EXPR(sign < MAX_RESOURCE_SIGNATURES);
        VERIFY_EXPR(pPSOSign->GetDesc().BindingIndex == sign);

        const auto  pSRB   = Resources.SRBs[sign].Lock();
        const auto* pCache = Resources.ResourceCaches[sign];
        if (pCache != nullptr)
        {
            DEV_CHECK_ERR(pSRB, "Shader resource cache pointer at index ", sign,
                          " is non-null, but the corresponding SRB is null. This indicates that the SRB has been released while still "
                          "being used by the context commands. This usage is invalid. A resource must be released only after "
                          "the last command that uses it.");
        }
        else
        {
            VERIFY(!pSRB, "Shader resource cache pointer is null, but SRB is not null. This is unexpected and is likely a bug.");
        }

        DEV_CHECK_ERR(pSRB, "Pipeline state '", m_pPipelineState->GetDesc().Name, "' requires SRB at index ", sign,
                      ", but none is bound in the device context. Did you call CommitShaderResources()?");

        VERIFY_EXPR(pCache == &pSRB->GetResourceCache());

        const auto* const pSRBSign = pSRB->GetSignature();
        DEV_CHECK_ERR(pPSOSign->IsCompatibleWith(pSRBSign), "Shader resource binding at index ", sign, " with signature '",
                      pSRBSign->GetDesc().Name, "' is not compatible with the signature in PSO '",
                      m_pPipelineState->GetDesc().Name, "'.");
    }
}
#endif // DILIGENT_DEVELOPMENT

#undef DVP_CHECK_QUEUE_TYPE_COMPATIBILITY

} // namespace Diligent
