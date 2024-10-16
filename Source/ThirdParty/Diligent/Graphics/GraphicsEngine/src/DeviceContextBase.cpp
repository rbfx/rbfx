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

#include "DeviceContextBase.hpp"

#include "GraphicsAccessories.hpp"

namespace Diligent
{

#ifdef DILIGENT_DEVELOPMENT

#    define CHECK_PARAMETER DEV_CHECK_ERR

#else

#    define CHECK_PARAMETER(Expr, ...)          \
        do                                      \
        {                                       \
            if (!(Expr))                        \
            {                                   \
                LOG_ERROR_MESSAGE(__VA_ARGS__); \
                return false;                   \
            }                                   \
        } while (false)

#endif

bool VerifyDrawAttribs(const DrawAttribs& Attribs)
{
    if (Attribs.NumVertices == 0)
        LOG_INFO_MESSAGE("DrawAttribs.NumVertices is 0. This is OK as the draw command will be ignored, but may be unintentional.");
    if (Attribs.NumInstances == 0)
        LOG_INFO_MESSAGE("DrawAttribs.NumInstances is 0. This is OK as the draw command will be ignored, but may be unintentional.");

    return true;
}

bool VerifyDrawIndexedAttribs(const DrawIndexedAttribs& Attribs)
{
#define CHECK_DRAW_INDEXED_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw indexed attribs are invalid: ", __VA_ARGS__)

    CHECK_DRAW_INDEXED_ATTRIBS(Attribs.IndexType == VT_UINT16 || Attribs.IndexType == VT_UINT32,
                               "IndexType (", GetValueTypeString(Attribs.IndexType), ") must be VT_UINT16 or VT_UINT32.");

    if (Attribs.NumIndices == 0)
        LOG_INFO_MESSAGE("DrawIndexedAttribs.NumIndices is 0. This is OK as the draw command will be ignored, but may be unintentional.");
    if (Attribs.NumInstances == 0)
        LOG_INFO_MESSAGE("DrawAttribs.NumInstances is 0. This is OK as the draw command will be ignored, but may be unintentional.");

#undef CHECK_DRAW_INDEXED_ATTRIBS

    return true;
}

bool VerifyDrawMeshAttribs(const MeshShaderProperties& MeshShaderProps, const DrawMeshAttribs& Attribs)
{
#define CHECK_DRAW_MESH_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw mesh attribs are invalid: ", __VA_ARGS__)

    if (Attribs.ThreadGroupCountX == 0)
        LOG_INFO_MESSAGE("DrawMeshAttribs.ThreadGroupCountX is 0. This is OK as the draw command will be ignored, but may be unintentional.");
    if (Attribs.ThreadGroupCountY == 0)
        LOG_INFO_MESSAGE("DrawMeshAttribs.ThreadGroupCountY is 0. This is OK as the draw command will be ignored, but may be unintentional.");
    if (Attribs.ThreadGroupCountZ == 0)
        LOG_INFO_MESSAGE("DrawMeshAttribs.ThreadGroupCountZ is 0. This is OK as the draw command will be ignored, but may be unintentional.");

    CHECK_DRAW_MESH_ATTRIBS(Attribs.ThreadGroupCountX <= MeshShaderProps.MaxThreadGroupCountX,
                            "ThreadGroupCountX (", Attribs.ThreadGroupCountX, ") must not exceed MeshShaderProps.MaxThreadGroupCountX (",
                            MeshShaderProps.MaxThreadGroupCountX, ").");
    CHECK_DRAW_MESH_ATTRIBS(Attribs.ThreadGroupCountY <= MeshShaderProps.MaxThreadGroupCountY,
                            "ThreadGroupCountY (", Attribs.ThreadGroupCountY, ") must not exceed MeshShaderProps.MaxThreadGroupCountY (",
                            MeshShaderProps.MaxThreadGroupCountY, ").");
    CHECK_DRAW_MESH_ATTRIBS(Attribs.ThreadGroupCountZ <= MeshShaderProps.MaxThreadGroupCountZ,
                            "ThreadGroupCountZ (", Attribs.ThreadGroupCountZ, ") must not exceed MeshShaderProps.MaxThreadGroupCountZ (",
                            MeshShaderProps.MaxThreadGroupCountZ, ").");

    const auto TotalGroups = Uint64{Attribs.ThreadGroupCountX} * Uint64{Attribs.ThreadGroupCountY} * Uint64{Attribs.ThreadGroupCountZ};
    CHECK_DRAW_MESH_ATTRIBS(TotalGroups <= MeshShaderProps.MaxThreadGroupTotalCount,
                            "Total thread group count (", TotalGroups, ") must not exceed MeshShaderProps.MaxThreadGroupTotalCount (",
                            MeshShaderProps.MaxThreadGroupTotalCount, ").");

#undef CHECK_DRAW_MESH_ATTRIBS

    return true;
}

bool VerifyDrawIndirectAttribs(const DrawIndirectAttribs& Attribs)
{
    const auto* pAttribsBuffer = Attribs.pAttribsBuffer;
    const auto* pCounterBuffer = Attribs.pCounterBuffer;

#define CHECK_DRAW_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw indirect attribs are invalid: ", __VA_ARGS__)

    CHECK_DRAW_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect draw arguments buffer must not be null.");
    const auto& AttrBuffDesc = pAttribsBuffer->GetDesc();
    CHECK_DRAW_INDIRECT_ATTRIBS((AttrBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0,
                                "indirect draw arguments buffer '", AttrBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");

    if (Attribs.DrawCount > 1)
    {
        CHECK_DRAW_INDIRECT_ATTRIBS(Attribs.DrawArgsStride >= sizeof(Uint32) * 4, "stride must be greater than 16 bytes");
        CHECK_DRAW_INDIRECT_ATTRIBS(Attribs.DrawArgsStride % 4 == 0, "stride must be a multiple of 4");
    }

    const auto ReqAttrBufSize = Attribs.DrawArgsOffset + (Attribs.DrawCount > 1 ? Attribs.DrawCount * Attribs.DrawArgsStride : Uint32{sizeof(Uint32)} * 4);
    CHECK_DRAW_INDIRECT_ATTRIBS(ReqAttrBufSize <= AttrBuffDesc.Size, "invalid DrawArgsOffset (", Attribs.DrawArgsOffset,
                                ") or indirect draw arguments buffer '", AttrBuffDesc.Name, "' size must be at least ", ReqAttrBufSize, " bytes");

    if (pCounterBuffer != nullptr)
    {
        const auto& CntBuffDesc = pCounterBuffer->GetDesc();
        CHECK_DRAW_INDIRECT_ATTRIBS((CntBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0, "indirect counter buffer '",
                                    CntBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
        const auto ReqCountBufSize = Attribs.CounterOffset + sizeof(Uint32);
        CHECK_DRAW_INDIRECT_ATTRIBS(ReqCountBufSize <= CntBuffDesc.Size, "invalid CounterOffset (", Attribs.CounterOffset,
                                    ") or counter buffer '", CntBuffDesc.Name, "' size must be at least ", ReqCountBufSize, " bytes");
    }

#undef CHECK_DRAW_INDIRECT_ATTRIBS

    return true;
}

bool VerifyDrawIndexedIndirectAttribs(const DrawIndexedIndirectAttribs& Attribs)
{
    const auto* pAttribsBuffer = Attribs.pAttribsBuffer;
    const auto* pCounterBuffer = Attribs.pCounterBuffer;

#define CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw indexed indirect attribs are invalid: ", __VA_ARGS__)

    CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect draw arguments buffer must not null.");
    CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(Attribs.IndexType == VT_UINT16 || Attribs.IndexType == VT_UINT32,
                                        "IndexType (", GetValueTypeString(Attribs.IndexType), ") must be VT_UINT16 or VT_UINT32.");
    const auto& AttrBuffDesc = pAttribsBuffer->GetDesc();
    CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS((AttrBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0,
                                        "indirect draw arguments buffer '", AttrBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");

    if (Attribs.DrawCount > 1)
    {
        CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(Attribs.DrawArgsStride >= sizeof(Uint32) * 5, "stride must be greater than 20 bytes");
        CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(Attribs.DrawArgsStride % 4 == 0, "stride must be a multiple of 4");
    }

    const auto ReqAttrBufSize = Attribs.DrawArgsOffset + (Attribs.DrawCount > 1 ? Attribs.DrawCount * Attribs.DrawArgsStride : Uint32{sizeof(Uint32)} * 5);
    CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(ReqAttrBufSize <= AttrBuffDesc.Size, "invalid DrawArgsOffset (", Attribs.DrawArgsOffset,
                                        ") or indirect draw arguments buffer '", AttrBuffDesc.Name, "' size must be at least ", ReqAttrBufSize, " bytes");

    if (pCounterBuffer != nullptr)
    {
        const auto& CntBuffDesc = pCounterBuffer->GetDesc();
        CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS((CntBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0, "indirect counter buffer '",
                                            CntBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
        const auto ReqCountBufSize = Attribs.CounterOffset + sizeof(Uint32);
        CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS(ReqCountBufSize <= CntBuffDesc.Size, "invalid CounterOffset (", Attribs.CounterOffset,
                                            ") or counter buffer '", CntBuffDesc.Name, "' size must be at least ", ReqCountBufSize, " bytes");
    }

#undef CHECK_DRAW_INDEXED_INDIRECT_ATTRIBS

    return true;
}

bool VerifyDrawMeshIndirectAttribs(const DrawMeshIndirectAttribs& Attribs, Uint32 IndirectCmdStride)
{
    const auto* pAttribsBuffer = Attribs.pAttribsBuffer;
    const auto* pCounterBuffer = Attribs.pCounterBuffer;

#define CHECK_DRAW_MESH_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw mesh indirect attribs are invalid: ", __VA_ARGS__)

    CHECK_DRAW_MESH_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect draw arguments buffer must not be null.");
    const auto& ArgsBuffDesc = pAttribsBuffer->GetDesc();
    CHECK_DRAW_MESH_INDIRECT_ATTRIBS((ArgsBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0,
                                     "indirect draw arguments buffer '", ArgsBuffDesc.Name,
                                     "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
    const auto ReqAttrBufSize = Attribs.DrawArgsOffset + Uint64{IndirectCmdStride} * Uint64{Attribs.CommandCount};
    CHECK_DRAW_MESH_INDIRECT_ATTRIBS(ReqAttrBufSize <= ArgsBuffDesc.Size, "invalid DrawArgsOffset (", Attribs.DrawArgsOffset,
                                     ") or indirect draw arguments buffer '", ArgsBuffDesc.Name, "' size must be at least ", ReqAttrBufSize, " bytes");


    if (pCounterBuffer != nullptr)
    {
        const auto& CntBuffDesc = pCounterBuffer->GetDesc();
        CHECK_DRAW_MESH_INDIRECT_ATTRIBS((CntBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0, "indirect counter buffer '",
                                         CntBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
        const auto ReqCountBufSize = Attribs.CounterOffset + sizeof(Uint32);
        CHECK_DRAW_MESH_INDIRECT_ATTRIBS(ReqCountBufSize <= CntBuffDesc.Size, "invalid CounterOffset (", Attribs.CounterOffset,
                                         ") or counter buffer '", CntBuffDesc.Name, "' size must be at least ", ReqCountBufSize, " bytes");
    }
#undef CHECK_DRAW_MESH_INDIRECT_ATTRIBS

    return true;
}

bool VerifyMultiDrawAttribs(const MultiDrawAttribs& Attribs)
{
    DEV_CHECK_ERR(Attribs.DrawCount == 0 || Attribs.pDrawItems != nullptr, "DrawCount is ", Attribs.DrawCount, ", but pDrawItems is null.");

    if (Attribs.NumInstances == 0)
        LOG_INFO_MESSAGE("MultiDrawAttribs.NumInstances is 0. This is OK as the draw command will be ignored, but may be unintentional.");

    return true;
}

bool VerifyMultiDrawIndexedAttribs(const MultiDrawIndexedAttribs& Attribs)
{
    DEV_CHECK_ERR(Attribs.DrawCount == 0 || Attribs.pDrawItems != nullptr, "DrawCount is ", Attribs.DrawCount, ", but pDrawItems is null.");

#define CHECK_MULTI_DRAW_INDEXED_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Draw indexed attribs are invalid: ", __VA_ARGS__)

    CHECK_MULTI_DRAW_INDEXED_ATTRIBS(Attribs.IndexType == VT_UINT16 || Attribs.IndexType == VT_UINT32,
                                     "IndexType (", GetValueTypeString(Attribs.IndexType), ") must be VT_UINT16 or VT_UINT32.");

    if (Attribs.NumInstances == 0)
        LOG_INFO_MESSAGE("MultiDrawAttribs.NumInstances is 0. This is OK as the draw command will be ignored, but may be unintentional.");

#undef CHECK_MULTI_DRAW_INDEXED_ATTRIBS

    return true;
}

bool VerifyDispatchComputeAttribs(const DispatchComputeAttribs& Attribs)
{
    if (Attribs.ThreadGroupCountX == 0)
        LOG_INFO_MESSAGE("DispatchComputeAttribs.ThreadGroupCountX is 0. This is OK as the dispatch command will be ignored, but may be unintentional.");
    if (Attribs.ThreadGroupCountY == 0)
        LOG_INFO_MESSAGE("DispatchComputeAttribs.ThreadGroupCountY is 0. This is OK as the dispatch command will be ignored, but may be unintentional.");
    if (Attribs.ThreadGroupCountZ == 0)
        LOG_INFO_MESSAGE("DispatchComputeAttribs.ThreadGroupCountZ is 0. This is OK as the dispatch command will be ignored, but may be unintentional.");

    return true;
}

bool VerifyDispatchComputeIndirectAttribs(const DispatchComputeIndirectAttribs& Attribs)
{
    const auto* pAttribsBuffer = Attribs.pAttribsBuffer;

#define CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Dispatch compute indirect attribs are invalid: ", __VA_ARGS__)

    CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect dispatch arguments buffer must not be null.");
    const auto& AttrBuffDesc = pAttribsBuffer->GetDesc();
    CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS((AttrBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0, "indirect dispatch arguments buffer '",
                                            AttrBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
    const auto ReqAttrBufSize = sizeof(Uint32) * 3 + Attribs.DispatchArgsByteOffset;
    CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(ReqAttrBufSize <= AttrBuffDesc.Size, "invalid DispatchArgsByteOffset (", Attribs.DispatchArgsByteOffset,
                                            ") or indirect dispatch arguments buffer '", AttrBuffDesc.Name, "' size must be at least ", ReqAttrBufSize, " bytes");


#undef CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS

    return true;
}

bool VerifyResolveTextureSubresourceAttribs(const ResolveTextureSubresourceAttribs& ResolveAttribs,
                                            const TextureDesc&                      SrcTexDesc,
                                            const TextureDesc&                      DstTexDesc)
{
#define CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Resolve texture subresource attribs are invalid: ", __VA_ARGS__)

    CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(SrcTexDesc.SampleCount > 1, "source texture '", SrcTexDesc.Name, "' of a resolve operation is not multi-sampled.");
    CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(DstTexDesc.SampleCount == 1, "destination texture '", DstTexDesc.Name, "' of a resolve operation is multi-sampled.");

    auto SrcMipLevelProps = GetMipLevelProperties(SrcTexDesc, ResolveAttribs.SrcMipLevel);
    auto DstMipLevelProps = GetMipLevelProperties(DstTexDesc, ResolveAttribs.DstMipLevel);
    CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(SrcMipLevelProps.LogicalWidth == DstMipLevelProps.LogicalWidth && SrcMipLevelProps.LogicalHeight == DstMipLevelProps.LogicalHeight,
                                     "the size (", SrcMipLevelProps.LogicalWidth, "x", SrcMipLevelProps.LogicalHeight,
                                     ") of the source subresource of a resolve operation (texture '",
                                     SrcTexDesc.Name, "', mip ", ResolveAttribs.SrcMipLevel, ", slice ", ResolveAttribs.SrcSlice,
                                     ") does not match the size (", DstMipLevelProps.LogicalWidth, "x", DstMipLevelProps.LogicalHeight,
                                     ") of the destination subresource (texture '", DstTexDesc.Name, "', mip ", ResolveAttribs.DstMipLevel, ", slice ",
                                     ResolveAttribs.DstSlice, ").");

    const auto& SrcFmtAttribs     = GetTextureFormatAttribs(SrcTexDesc.Format);
    const auto& DstFmtAttribs     = GetTextureFormatAttribs(DstTexDesc.Format);
    const auto& ResolveFmtAttribs = GetTextureFormatAttribs(ResolveAttribs.Format);
    if (!SrcFmtAttribs.IsTypeless && !DstFmtAttribs.IsTypeless)
    {
        CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(SrcTexDesc.Format == DstTexDesc.Format,
                                         "source (", SrcFmtAttribs.Name, ") and destination (", DstFmtAttribs.Name,
                                         ") texture formats of a resolve operation must match exactly or be compatible typeless formats.");
        CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(ResolveAttribs.Format == TEX_FORMAT_UNKNOWN || SrcTexDesc.Format == ResolveAttribs.Format, "Invalid format of a resolve operation.");
    }
    if (SrcFmtAttribs.IsTypeless && DstFmtAttribs.IsTypeless)
    {
        CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(ResolveAttribs.Format != TEX_FORMAT_UNKNOWN,
                                         "format of a resolve operation must not be unknown when both src and dst texture formats are typeless.");
    }
    if (SrcFmtAttribs.IsTypeless || DstFmtAttribs.IsTypeless)
    {
        CHECK_RESOLVE_TEX_SUBRES_ATTRIBS(!ResolveFmtAttribs.IsTypeless,
                                         "format of a resolve operation must not be typeless when one of the texture formats is typeless.");
    }
#undef CHECK_RESOLVE_TEX_SUBRES_ATTRIBS

    return true;
}

bool VerifyBeginRenderPassAttribs(const BeginRenderPassAttribs& Attribs)
{
#define CHECK_BEGIN_RENDER_PASS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Begin render pass attribs are invalid: ", __VA_ARGS__)

    CHECK_BEGIN_RENDER_PASS_ATTRIBS(Attribs.pRenderPass != nullptr, "pRenderPass pass must not be null.");
    CHECK_BEGIN_RENDER_PASS_ATTRIBS(Attribs.pFramebuffer != nullptr, "pFramebuffer must not be null.");

    const auto& RPDesc = Attribs.pRenderPass->GetDesc();

    Uint32 NumRequiredClearValues = 0;
    for (Uint32 i = 0; i < RPDesc.AttachmentCount; ++i)
    {
        const auto& Attchmnt = RPDesc.pAttachments[i];
        if (Attchmnt.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
            NumRequiredClearValues = i + 1;

        const auto& FmtAttribs = GetTextureFormatAttribs(Attchmnt.Format);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (Attchmnt.StencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR)
                NumRequiredClearValues = i + 1;
        }
    }

    CHECK_BEGIN_RENDER_PASS_ATTRIBS(Attribs.ClearValueCount >= NumRequiredClearValues,
                                    "at least ", NumRequiredClearValues, " clear values are required, but only ",
                                    Uint32{Attribs.ClearValueCount}, " are provided.");
    CHECK_BEGIN_RENDER_PASS_ATTRIBS(Attribs.ClearValueCount == 0 || Attribs.pClearValues != nullptr,
                                    "pClearValues must not be null when ClearValueCount (", Attribs.ClearValueCount, ") is not zero.");

#undef CHECK_BEGIN_RENDER_PASS_ATTRIBS

    return true;
}

bool VerifyResourceState(RESOURCE_STATE States, COMMAND_QUEUE_TYPE QueueType, const char* Name)
{
    QueueType &= COMMAND_QUEUE_TYPE_PRIMARY_MASK;

    bool Result = true;
    while (States != 0)
    {
        auto State = ExtractLSB(States);

        static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "Please update the switch below to handle the new resource state");
        switch (State)
        {
            case RESOURCE_STATE_UNDEFINED:
            case RESOURCE_STATE_COPY_DEST:
            case RESOURCE_STATE_COPY_SOURCE:
            case RESOURCE_STATE_COMMON:
                if ((QueueType & COMMAND_QUEUE_TYPE_TRANSFER) != COMMAND_QUEUE_TYPE_TRANSFER)
                {
                    Result = false;
                    CHECK_PARAMETER(Name, " contains state ", GetResourceStateFlagString(State), " that is not supported in ", GetCommandQueueTypeString(QueueType), " context");
                }
                break;

            case RESOURCE_STATE_CONSTANT_BUFFER:
            case RESOURCE_STATE_UNORDERED_ACCESS:
            case RESOURCE_STATE_SHADER_RESOURCE:
            case RESOURCE_STATE_INDIRECT_ARGUMENT:
            case RESOURCE_STATE_BUILD_AS_READ:
            case RESOURCE_STATE_BUILD_AS_WRITE:
            case RESOURCE_STATE_RAY_TRACING:
                if ((QueueType & COMMAND_QUEUE_TYPE_COMPUTE) != COMMAND_QUEUE_TYPE_COMPUTE)
                {
                    Result = false;
                    CHECK_PARAMETER(Name, " contains state ", GetResourceStateFlagString(State), " that is not supported in ", GetCommandQueueTypeString(QueueType), " context");
                }
                break;

            case RESOURCE_STATE_VERTEX_BUFFER:
            case RESOURCE_STATE_INDEX_BUFFER:
            case RESOURCE_STATE_RENDER_TARGET:
            case RESOURCE_STATE_DEPTH_WRITE:
            case RESOURCE_STATE_DEPTH_READ:
            case RESOURCE_STATE_STREAM_OUT:
            case RESOURCE_STATE_RESOLVE_DEST:
            case RESOURCE_STATE_RESOLVE_SOURCE:
            case RESOURCE_STATE_INPUT_ATTACHMENT:
            case RESOURCE_STATE_PRESENT:
            case RESOURCE_STATE_SHADING_RATE:
                if ((QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
                {
                    Result = false;
                    CHECK_PARAMETER(Name, " contains state ", GetResourceStateFlagString(State), " that is not supported in ", GetCommandQueueTypeString(QueueType), " context");
                }
                break;

            default:
                UNEXPECTED("Unexpected resource state");
                break;
        }
    }
    return Result;
}

#define CHECK_STATE_TRANSITION_DESC(Expr, ...) CHECK_PARAMETER(Expr, "State transition parameters are invalid: ", __VA_ARGS__)
static bool VerifyAliasingBarrierDesc(const StateTransitionDesc& Barrier)
{
    VERIFY_EXPR(Barrier.Flags & STATE_TRANSITION_FLAG_ALIASING);

    auto VerifySparseAliasedResource = [](IDeviceObject* pResource) //
    {
        if (pResource == nullptr)
            return RESOURCE_DIM_UNDEFINED;

        if (RefCntAutoPtr<ITexture> pTexture{pResource, IID_Texture})
        {
            const auto& TexDesc = pTexture->GetDesc();
            DEV_CHECK_ERR(TexDesc.Usage == USAGE_SPARSE,
                          "Texture '", TexDesc.Name, "' used in an aliasing barrier is not a sparse resource");
            DEV_CHECK_ERR((TexDesc.MiscFlags & MISC_TEXTURE_FLAG_SPARSE_ALIASING) != 0,
                          "Texture '", TexDesc.Name, "' used in an aliasing barrier was not created with MISC_TEXTURE_FLAG_SPARSE_ALIASING flag");

            return TexDesc.Type;
        }
        else if (RefCntAutoPtr<IBuffer> pBuffer{pResource, IID_Buffer})
        {
            const auto& BuffDesc = pBuffer->GetDesc();

            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_SPARSE,
                          "Buffer '", BuffDesc.Name, "' used in an aliasing barrier is not a sparse resource");
            DEV_CHECK_ERR((BuffDesc.MiscFlags & MISC_BUFFER_FLAG_SPARSE_ALIASING) != 0,
                          "Buffer '", BuffDesc.Name, "' used in an aliasing barrier was not created with MISC_BUFFER_FLAG_SPARSE_ALIASING flag");

            return RESOURCE_DIM_BUFFER;
        }
        else
        {
            DEV_ERROR("Only textures and buffers are allowed in aliasing barriers");
            return RESOURCE_DIM_UNDEFINED;
        }
    };

    auto BeforeDim = VerifySparseAliasedResource(Barrier.pResourceBefore);
    auto AfterDim  = VerifySparseAliasedResource(Barrier.pResource);
    if (BeforeDim != RESOURCE_DIM_UNDEFINED && AfterDim != RESOURCE_DIM_UNDEFINED)
    {
        CHECK_STATE_TRANSITION_DESC((BeforeDim == RESOURCE_DIM_BUFFER) == (AfterDim == RESOURCE_DIM_BUFFER),
                                    "Both before- and after-resources must either be buffers or textures. "
                                    "Sparse aliasing between textures and buffers is not allowed.");
    }

    CHECK_STATE_TRANSITION_DESC(Barrier.OldState == RESOURCE_STATE_UNKNOWN && Barrier.NewState == RESOURCE_STATE_UNKNOWN,
                                "Aliasing barrier does not support state transitions. OldState and NewState must both be RESOURCE_STATE_UNKNOWN");

    CHECK_STATE_TRANSITION_DESC((Barrier.FirstMipLevel == 0 && Barrier.MipLevelsCount == REMAINING_MIP_LEVELS &&
                                 Barrier.FirstArraySlice == 0 && Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES),
                                "Aliasing barrier is applied to all subresources. FirstMipLevel, MipLevelsCount, FirstArraySlice and ArraySliceCount must be default");

    return true;
}

bool VerifyStateTransitionDesc(const IRenderDevice*       pDevice,
                               const StateTransitionDesc& Barrier,
                               DeviceContextIndex         ExecutionCtxId,
                               const DeviceContextDesc&   CtxDesc)
{
    if (Barrier.Flags & STATE_TRANSITION_FLAG_ALIASING)
    {
        CHECK_STATE_TRANSITION_DESC((Barrier.Flags & ~STATE_TRANSITION_FLAG_ALIASING) == 0,
                                    "STATE_TRANSITION_FLAG_ALIASING flag is not compatible with other flags");
        return VerifyAliasingBarrierDesc(Barrier);
    }

    CHECK_STATE_TRANSITION_DESC(Barrier.pResourceBefore == nullptr, "pResourceBefore is only used for aliasing barrier and must be null otherwise");
    CHECK_STATE_TRANSITION_DESC(Barrier.NewState != RESOURCE_STATE_UNKNOWN && Barrier.NewState != RESOURCE_STATE_UNDEFINED,
                                "NewState must not be UNKNOWN or UNDEFINED");


    CHECK_STATE_TRANSITION_DESC(Barrier.pResource != nullptr, "pResource must not be null.");

    RESOURCE_STATE OldState             = RESOURCE_STATE_UNKNOWN;
    Uint64         ImmediateContextMask = 0;

    if (RefCntAutoPtr<ITexture> pTexture{Barrier.pResource, IID_Texture})
    {
        const auto& TexDesc  = pTexture->GetDesc();
        ImmediateContextMask = TexDesc.ImmediateContextMask;

        CHECK_STATE_TRANSITION_DESC(VerifyResourceStates(Barrier.NewState, true), "invalid new state specified for texture '", TexDesc.Name, "'.");
        OldState = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : pTexture->GetState();
        CHECK_STATE_TRANSITION_DESC(OldState != RESOURCE_STATE_UNKNOWN,
                                    "the state of texture '", TexDesc.Name,
                                    "' is unknown to the engine and is not explicitly specified in the barrier.");
        CHECK_STATE_TRANSITION_DESC(VerifyResourceStates(OldState, true), "invalid old state specified for texture '", TexDesc.Name, "'.");

        CHECK_STATE_TRANSITION_DESC(Barrier.FirstMipLevel < TexDesc.MipLevels, "first mip level (", Barrier.FirstMipLevel,
                                    ") specified by the barrier is out of range. Texture '",
                                    TexDesc.Name, "' has only ", TexDesc.MipLevels, " mip level(s).");
        CHECK_STATE_TRANSITION_DESC(Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.FirstMipLevel + Barrier.MipLevelsCount <= TexDesc.MipLevels,
                                    "mip level range ", Barrier.FirstMipLevel, "..", Barrier.FirstMipLevel + Barrier.MipLevelsCount - 1,
                                    " specified by the barrier is out of range. Texture '",
                                    TexDesc.Name, "' has only ", TexDesc.MipLevels, " mip level(s).");

        CHECK_STATE_TRANSITION_DESC(Barrier.FirstArraySlice < TexDesc.GetArraySize(), "first array slice (", Barrier.FirstArraySlice,
                                    ") specified by the barrier is out of range. Array size of texture '",
                                    TexDesc.Name, "' is ", TexDesc.GetArraySize());
        CHECK_STATE_TRANSITION_DESC(Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.FirstArraySlice + Barrier.ArraySliceCount <= TexDesc.GetArraySize(),
                                    "array slice range ", Barrier.FirstArraySlice, "..", Barrier.FirstArraySlice + Barrier.ArraySliceCount - 1,
                                    " specified by the barrier is out of range. Array size of texture '",
                                    TexDesc.Name, "' is ", TexDesc.GetArraySize());

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        if (DeviceType != RENDER_DEVICE_TYPE_D3D12 && DeviceType != RENDER_DEVICE_TYPE_VULKAN)
        {
            CHECK_STATE_TRANSITION_DESC(Barrier.FirstMipLevel == 0 && (Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.MipLevelsCount == TexDesc.MipLevels),
                                        "failed to transition texture '", TexDesc.Name, "': only whole resources can be transitioned on this device.");
            CHECK_STATE_TRANSITION_DESC(Barrier.FirstArraySlice == 0 && (Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.ArraySliceCount == TexDesc.GetArraySize()),
                                        "failed to transition texture '", TexDesc.Name, "': only whole resources can be transitioned on this device.");
        }
    }
    else if (RefCntAutoPtr<IBuffer> pBuffer{Barrier.pResource, IID_Buffer})
    {
        const auto& BuffDesc = pBuffer->GetDesc();
        ImmediateContextMask = BuffDesc.ImmediateContextMask;
        CHECK_STATE_TRANSITION_DESC(VerifyResourceStates(Barrier.NewState, false), "invalid new state specified for buffer '", BuffDesc.Name, "'.");
        OldState = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : pBuffer->GetState();
        CHECK_STATE_TRANSITION_DESC(OldState != RESOURCE_STATE_UNKNOWN, "the state of buffer '", BuffDesc.Name, "' is unknown to the engine and is not explicitly specified in the barrier.");
        CHECK_STATE_TRANSITION_DESC(VerifyResourceStates(OldState, false), "invalid old state specified for buffer '", BuffDesc.Name, "'.");
    }
    else if (RefCntAutoPtr<IBottomLevelAS> pBottomLevelAS{Barrier.pResource, IID_BottomLevelAS})
    {
        const auto& BLASDesc = pBottomLevelAS->GetDesc();
        ImmediateContextMask = BLASDesc.ImmediateContextMask;
        OldState             = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : pBottomLevelAS->GetState();
        CHECK_STATE_TRANSITION_DESC(OldState != RESOURCE_STATE_UNKNOWN, "the state of BLAS '", BLASDesc.Name, "' is unknown to the engine and is not explicitly specified in the barrier.");
        CHECK_STATE_TRANSITION_DESC(Barrier.NewState == RESOURCE_STATE_BUILD_AS_READ || Barrier.NewState == RESOURCE_STATE_BUILD_AS_WRITE,
                                    "invalid new state specified for BLAS '", BLASDesc.Name, "'.");
        CHECK_STATE_TRANSITION_DESC(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE, "split barriers are not supported for BLAS.");
    }
    else if (RefCntAutoPtr<ITopLevelAS> pTopLevelAS{Barrier.pResource, IID_TopLevelAS})
    {
        const auto& TLASDesc = pTopLevelAS->GetDesc();
        ImmediateContextMask = TLASDesc.ImmediateContextMask;
        OldState             = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : pTopLevelAS->GetState();
        CHECK_STATE_TRANSITION_DESC(OldState != RESOURCE_STATE_UNKNOWN, "the state of TLAS '", TLASDesc.Name, "' is unknown to the engine and is not explicitly specified in the barrier.");
        CHECK_STATE_TRANSITION_DESC(Barrier.NewState == RESOURCE_STATE_BUILD_AS_READ || Barrier.NewState == RESOURCE_STATE_BUILD_AS_WRITE || Barrier.NewState == RESOURCE_STATE_RAY_TRACING,
                                    "invalid new state specified for TLAS '", TLASDesc.Name, "'.");
        CHECK_STATE_TRANSITION_DESC(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE, "split barriers are not supported for TLAS.");
    }
    else
    {
        UNEXPECTED("unsupported resource type");
    }

    CHECK_STATE_TRANSITION_DESC((ImmediateContextMask & (Uint64{1} << Uint64{ExecutionCtxId})) != 0,
                                "resource was created with ImmediateContextMask 0x", std::hex, ImmediateContextMask, " and can not be used in device context '", CtxDesc.Name, "'.");

    if (OldState == RESOURCE_STATE_UNORDERED_ACCESS && Barrier.NewState == RESOURCE_STATE_UNORDERED_ACCESS)
    {
        CHECK_STATE_TRANSITION_DESC(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE,
                                    "for UAV barriers, transition type must be STATE_TRANSITION_TYPE_IMMEDIATE.");
    }

    switch (Barrier.TransitionType)
    {
        case STATE_TRANSITION_TYPE_IMMEDIATE:
            break;

        case STATE_TRANSITION_TYPE_BEGIN:
            CHECK_STATE_TRANSITION_DESC((Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) == 0, "resource state can't be updated in begin-split barrier.");
            break;

        case STATE_TRANSITION_TYPE_END:
            break;

        default:
            UNEXPECTED("Unexpected transition type");
    }

    VerifyResourceState(Barrier.OldState, CtxDesc.QueueType, "OldState");
    VerifyResourceState(Barrier.NewState, CtxDesc.QueueType, "NewState");


    return true;
}

#undef CHECK_STATE_TRANSITION_DESC


bool VerifyBuildBLASAttribs(const BuildBLASAttribs& Attribs, const IRenderDevice* pDevice)
{
#define CHECK_BUILD_BLAS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Build BLAS attribs are invalid: ", __VA_ARGS__)

    const auto& RTProps    = pDevice->GetAdapterInfo().RayTracing;
    const auto  DeviceType = pDevice->GetDeviceInfo().Type;

    CHECK_BUILD_BLAS_ATTRIBS(Attribs.pBLAS != nullptr, "pBLAS must not be null.");
    CHECK_BUILD_BLAS_ATTRIBS(Attribs.pScratchBuffer != nullptr, "pScratchBuffer must not be null.");
    CHECK_BUILD_BLAS_ATTRIBS((Attribs.BoxDataCount != 0) ^ (Attribs.TriangleDataCount != 0), "exactly one of TriangleDataCount and BoxDataCount must be non-zero.");
    CHECK_BUILD_BLAS_ATTRIBS(Attribs.pBoxData != nullptr || Attribs.BoxDataCount == 0, "BoxDataCount is ", Attribs.BoxDataCount, ", but pBoxData is null.");
    CHECK_BUILD_BLAS_ATTRIBS(Attribs.pTriangleData != nullptr || Attribs.TriangleDataCount == 0, "TriangleDataCount is ", Attribs.TriangleDataCount, ", but pTriangleData is null.");

    const auto& BLASDesc = Attribs.pBLAS->GetDesc();

    CHECK_BUILD_BLAS_ATTRIBS(Attribs.BoxDataCount <= BLASDesc.BoxCount, "BoxDataCount (", Attribs.BoxDataCount, ") must be less than or equal to pBLAS->GetDesc().BoxCount (", BLASDesc.BoxCount, ").");
    CHECK_BUILD_BLAS_ATTRIBS(Attribs.TriangleDataCount <= BLASDesc.TriangleCount, "TriangleDataCount (", Attribs.TriangleDataCount, ") must be less than or equal to pBLAS->GetDesc().TriangleCount (", BLASDesc.TriangleCount, ").");

    if (Attribs.Update)
    {
        CHECK_BUILD_BLAS_ATTRIBS((BLASDesc.Flags & RAYTRACING_BUILD_AS_ALLOW_UPDATE) == RAYTRACING_BUILD_AS_ALLOW_UPDATE,
                                 "Update is true, but BLAS was created without RAYTRACING_BUILD_AS_ALLOW_UPDATE flag.");

        const Uint32 GeomCount = Attribs.pBLAS->GetActualGeometryCount();
        CHECK_BUILD_BLAS_ATTRIBS(Attribs.BoxDataCount == 0 || Attribs.BoxDataCount == GeomCount,
                                 "Update is true, but BoxDataCount (", Attribs.BoxDataCount, ") does not match the previous value (", GeomCount, ").");
        CHECK_BUILD_BLAS_ATTRIBS(Attribs.TriangleDataCount == 0 || Attribs.TriangleDataCount == GeomCount,
                                 "Update is true, but TriangleDataCount (", Attribs.TriangleDataCount, ") does not match the previous value (", GeomCount, ").");
    }

    for (Uint32 i = 0; i < Attribs.TriangleDataCount; ++i)
    {
        const auto&  tri       = Attribs.pTriangleData[i];
        const Uint32 GeomIndex = Attribs.pBLAS->GetGeometryDescIndex(tri.GeometryName);

        CHECK_BUILD_BLAS_ATTRIBS(GeomIndex != INVALID_INDEX,
                                 "pTriangleData[", i, "].GeometryName (", tri.GeometryName, ") is not found in BLAS description.");

        const auto&  TriDesc         = BLASDesc.pTriangles[GeomIndex];
        const Uint32 VertexValueSize = GetValueSize(TriDesc.VertexValueType);
        const Uint32 VertexSize      = VertexValueSize * tri.VertexComponentCount;
        const Uint32 VertexDataSize  = tri.VertexStride * tri.VertexCount;
        const Uint32 VertStrideAlign = DeviceType == RENDER_DEVICE_TYPE_METAL ? RTProps.VertexBufferAlignment : VertexValueSize;
        const Uint32 VertOffsetAlign = DeviceType == RENDER_DEVICE_TYPE_METAL ? tri.VertexStride : VertexValueSize;

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexValueType == VT_UNDEFINED || tri.VertexValueType == TriDesc.VertexValueType,
                                 "pTriangleData[", i, "].VertexValueType must be undefined or match the VertexValueType in geometry description.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexComponentCount == 0 || tri.VertexComponentCount == TriDesc.VertexComponentCount,
                                 "pTriangleData[", i, "].VertexComponentCount (", Uint32{tri.VertexComponentCount}, ") must be 0 or match the VertexComponentCount (",
                                 Uint32{TriDesc.VertexComponentCount}, ") in geometry description.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexCount <= TriDesc.MaxVertexCount,
                                 "pTriangleData[", i, "].VertexCount (", tri.VertexCount, ") must not be greater than MaxVertexCount(", TriDesc.MaxVertexCount, ").");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexStride >= VertexSize,
                                 "pTriangleData[", i, "].VertexStride (", tri.VertexStride, ") must be at least ", VertexSize, " bytes.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexStride % VertStrideAlign == 0,
                                 "pTriangleData[", i, "].VertexStride (", tri.VertexStride, ") must be be aligned by ", VertStrideAlign, ".");

        CHECK_BUILD_BLAS_ATTRIBS(tri.pVertexBuffer != nullptr, "pTriangleData[", i, "].pVertexBuffer must not be null.");

        const BufferDesc& VertBufDesc = tri.pVertexBuffer->GetDesc();
        CHECK_BUILD_BLAS_ATTRIBS((VertBufDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                                 "pTriangleData[", i, "].pVertexBuffer was not created with BIND_RAY_TRACING flag.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexOffset % VertOffsetAlign == 0,
                                 "pTriangleData[", i, "].VertexOffset (", tri.VertexOffset, ") must be aligned by ", VertOffsetAlign, ".");

        CHECK_BUILD_BLAS_ATTRIBS(tri.VertexOffset + VertexDataSize <= VertBufDesc.Size,
                                 "pTriangleData[", i, "].pVertexBuffer is too small for the specified VertexStride (", tri.VertexStride, ") and VertexCount (",
                                 tri.VertexCount, "): at least ", tri.VertexOffset + VertexDataSize, " bytes are required.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.IndexType == VT_UNDEFINED || tri.IndexType == TriDesc.IndexType,
                                 "pTriangleData[", i, "].IndexType (", GetValueTypeString(tri.IndexType), ") must match the IndexType (",
                                 GetValueTypeString(TriDesc.IndexType), ") in geometry description.");

        CHECK_BUILD_BLAS_ATTRIBS(tri.PrimitiveCount <= TriDesc.MaxPrimitiveCount,
                                 "pTriangleData[", i, "].PrimitiveCount (", tri.PrimitiveCount, ") must not be greater than MaxPrimitiveCount (",
                                 TriDesc.MaxPrimitiveCount, ").");

        if (TriDesc.IndexType != VT_UNDEFINED)
        {
            CHECK_BUILD_BLAS_ATTRIBS(tri.pIndexBuffer != nullptr, "pTriangleData[", i, "].pIndexBuffer must not be null.");

            const BufferDesc& InstBufDesc   = tri.pIndexBuffer->GetDesc();
            const Uint32      IndexDataSize = tri.PrimitiveCount * 3 * GetValueSize(tri.IndexType);

            CHECK_BUILD_BLAS_ATTRIBS((InstBufDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                                     "pTriangleData[", i, "].pIndexBuffer was not created with BIND_RAY_TRACING flag.");

            CHECK_BUILD_BLAS_ATTRIBS(tri.IndexOffset + IndexDataSize <= InstBufDesc.Size,
                                     "pTriangleData[", i, "].pIndexBuffer is too small for specified IndexType and IndexCount: at least ",
                                     tri.IndexOffset + IndexDataSize, " bytes are required.");

            CHECK_BUILD_BLAS_ATTRIBS(tri.IndexOffset % GetValueSize(TriDesc.IndexType) == 0,
                                     "pTriangleData[", i, "].IndexOffset (", tri.IndexOffset, ") must be a multiple of (", GetValueSize(TriDesc.IndexType), ") bytes.");

            CHECK_BUILD_BLAS_ATTRIBS(tri.IndexOffset % RTProps.IndexBufferAlignment == 0,
                                     "pTriangleData[", i, "].IndexOffset (", tri.IndexOffset, ") must be aligned by ", RTProps.IndexBufferAlignment,
                                     " (RayTracingProperties::IndexBufferAlignment).");
        }
        else
        {
            CHECK_BUILD_BLAS_ATTRIBS(tri.VertexCount == tri.PrimitiveCount * 3,
                                     "pTriangleData[", i, "].VertexCount (", tri.VertexCount, ") must be equal to PrimitiveCount * 3 (",
                                     tri.PrimitiveCount * 3, ").");

            CHECK_BUILD_BLAS_ATTRIBS(tri.pIndexBuffer == nullptr, "pTriangleData[", i, "].pIndexBuffer must be null if IndexType is VT_UNDEFINED.");
        }

        if (tri.pTransformBuffer != nullptr)
        {
            const auto& TrfrmBufDesc = tri.pTransformBuffer->GetDesc();

            CHECK_BUILD_BLAS_ATTRIBS((TrfrmBufDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                                     "pTriangleData[", i, "].pTransformBuffer was not created with BIND_RAY_TRACING flag.");

            CHECK_BUILD_BLAS_ATTRIBS(TriDesc.AllowsTransforms, "pTriangleData[", i, "] uses transform buffer, but AllowsTransforms is false.");

            CHECK_BUILD_BLAS_ATTRIBS(tri.TransformBufferOffset + sizeof(InstanceMatrix) <= TrfrmBufDesc.Size,
                                     "pTriangleData[", i, "].pTransformBuffer is too small: at least ", tri.TransformBufferOffset + sizeof(InstanceMatrix), " bytes are required.");

            CHECK_BUILD_BLAS_ATTRIBS(tri.TransformBufferOffset % RTProps.TransformBufferAlignment == 0,
                                     "pTriangleData[", i, "].TransformBufferOffset (", tri.TransformBufferOffset, ") must be aligned by ", RTProps.TransformBufferAlignment,
                                     " (RayTracingProperties::TransformBufferAlignment).");
        }
    }

    for (Uint32 i = 0; i < Attribs.BoxDataCount; ++i)
    {
        const auto&  box           = Attribs.pBoxData[i];
        const Uint32 BoxSize       = sizeof(float) * 6;
        const Uint32 BoxBufferSize = box.BoxCount * box.BoxStride;
        const Uint32 GeomIndex     = Attribs.pBLAS->GetGeometryDescIndex(box.GeometryName);

        CHECK_BUILD_BLAS_ATTRIBS(GeomIndex != INVALID_INDEX,
                                 "pBoxData[", i, "].GeometryName (", box.GeometryName, ") is not found in BLAS description.");

        const auto& BoxDesc = BLASDesc.pBoxes[GeomIndex];

        CHECK_BUILD_BLAS_ATTRIBS(box.BoxCount <= BoxDesc.MaxBoxCount,
                                 "pBoxData[", i, "].BoxCount (", box.BoxCount, ") must not be greater than MaxBoxCount (", BoxDesc.MaxBoxCount, ").");

        CHECK_BUILD_BLAS_ATTRIBS(box.BoxStride >= BoxSize,
                                 "pBoxData[", i, "].BoxStride (", box.BoxStride, ") must be at least ", BoxSize, " bytes.");

        CHECK_BUILD_BLAS_ATTRIBS(box.BoxStride % RTProps.BoxBufferAlignment == 0,
                                 "pBoxData[", i, "].BoxStride (", box.BoxStride, ") must be aligned by ", RTProps.BoxBufferAlignment,
                                 " (RayTracingProperties::BoxBufferAlignment).");

        CHECK_BUILD_BLAS_ATTRIBS(box.BoxOffset % RTProps.BoxBufferAlignment == 0,
                                 "pBoxData[", i, "].BoxOffset (", box.BoxOffset, ") must be aligned by ", RTProps.BoxBufferAlignment,
                                 " (RayTracingProperties::BoxBufferAlignment).");

        if (DeviceType == RENDER_DEVICE_TYPE_METAL)
        {
            CHECK_BUILD_BLAS_ATTRIBS(box.BoxOffset % box.BoxStride == 0,
                                     "pBoxData[", i, "].BoxOffset (", box.BoxOffset, ") must be a multiple of BoxStride (", box.BoxStride, ").");
        }

        CHECK_BUILD_BLAS_ATTRIBS(box.pBoxBuffer != nullptr, "pBoxData[", i, "].pBoxBuffer must not be null.");

        const auto& BoxBufDesc = box.pBoxBuffer->GetDesc();

        CHECK_BUILD_BLAS_ATTRIBS(box.BoxOffset + BoxBufferSize <= BoxBufDesc.Size,
                                 "pBoxData[", i, "].pBoxBuffer is too small for the specified BoxStride (", box.BoxStride,
                                 ") and BoxCount (", box.BoxCount, "): at least", box.BoxOffset + BoxBufferSize, " bytes are required.");

        CHECK_BUILD_BLAS_ATTRIBS((BoxBufDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                                 "pBoxData[", i, "].pBoxBuffer was not created with BIND_RAY_TRACING flag.");
    }

    const auto& ScratchDesc = Attribs.pScratchBuffer->GetDesc();

    CHECK_BUILD_BLAS_ATTRIBS(Attribs.ScratchBufferOffset <= ScratchDesc.Size,
                             "ScratchBufferOffset (", Attribs.ScratchBufferOffset, ") is greater than the buffer size (", ScratchDesc.Size, ").");

    CHECK_BUILD_BLAS_ATTRIBS(Attribs.ScratchBufferOffset % RTProps.ScratchBufferAlignment == 0,
                             "ScratchBufferOffset (", Attribs.ScratchBufferOffset, ") must be aligned by ", RTProps.ScratchBufferAlignment,
                             " (RayTracingProperties::ScratchBufferAlignment).");

    if (Attribs.Update)
    {
        CHECK_BUILD_BLAS_ATTRIBS(ScratchDesc.Size - Attribs.ScratchBufferOffset >= Attribs.pBLAS->GetScratchBufferSizes().Update,
                                 "pScratchBuffer size is too small, use pBLAS->GetScratchBufferSizes().Update to get the required size for the scratch buffer.");
    }
    else
    {
        CHECK_BUILD_BLAS_ATTRIBS(ScratchDesc.Size - Attribs.ScratchBufferOffset >= Attribs.pBLAS->GetScratchBufferSizes().Build,
                                 "pScratchBuffer size is too small, use pBLAS->GetScratchBufferSizes().Build to get the required size for the scratch buffer.");
    }

    CHECK_BUILD_BLAS_ATTRIBS((ScratchDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                             "pScratchBuffer was not created with BIND_RAY_TRACING flag.");

#undef CHECK_BUILD_BLAS_ATTRIBS

    return true;
}


bool VerifyBuildTLASAttribs(const BuildTLASAttribs& Attribs, const RayTracingProperties& RTProps)
{
#define CHECK_BUILD_TLAS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Build TLAS attribs are invalid: ", __VA_ARGS__)

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.pTLAS != nullptr, "pTLAS must not be null.");
    CHECK_BUILD_TLAS_ATTRIBS(Attribs.pScratchBuffer != nullptr, "pScratchBuffer must not be null.");
    CHECK_BUILD_TLAS_ATTRIBS(Attribs.pInstances != nullptr, "pInstances must not be null.");
    CHECK_BUILD_TLAS_ATTRIBS(Attribs.pInstanceBuffer != nullptr, "pInstanceBuffer must not be null.");

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.BindingMode == HIT_GROUP_BINDING_MODE_USER_DEFINED || Attribs.HitGroupStride != 0,
                             "HitGroupStride must be greater than 0 if BindingMode is not HIT_GROUP_BINDING_MODE_USER_DEFINED.");

    const auto& TLASDesc = Attribs.pTLAS->GetDesc();

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.InstanceCount <= TLASDesc.MaxInstanceCount,
                             "InstanceCount (", Attribs.InstanceCount, ") must be less than or equal to pTLAS->GetDesc().MaxInstanceCount (",
                             TLASDesc.MaxInstanceCount, ").");

    if (Attribs.Update)
    {
        CHECK_BUILD_TLAS_ATTRIBS((TLASDesc.Flags & RAYTRACING_BUILD_AS_ALLOW_UPDATE) == RAYTRACING_BUILD_AS_ALLOW_UPDATE,
                                 "Update is true, but TLAS created without RAYTRACING_BUILD_AS_ALLOW_UPDATE flag.");

        const Uint32 PrevInstanceCount = Attribs.pTLAS->GetBuildInfo().InstanceCount;
        CHECK_BUILD_TLAS_ATTRIBS(PrevInstanceCount == Attribs.InstanceCount,
                                 "Update is true, but InstanceCount (", Attribs.InstanceCount, ") does not match the previous value (", PrevInstanceCount, ").");
    }

    const auto& InstDesc          = Attribs.pInstanceBuffer->GetDesc();
    const auto  InstDataSize      = size_t{Attribs.InstanceCount} * size_t{TLAS_INSTANCE_DATA_SIZE};
    Uint32      AutoOffsetCounter = 0;

    // Calculate instance data size
    for (Uint32 i = 0; i < Attribs.InstanceCount; ++i)
    {
        constexpr Uint32 BitMask = (1u << 24) - 1;
        const auto&      Inst    = Attribs.pInstances[i];

        VERIFY((Inst.CustomId & ~BitMask) == 0, "Only the lower 24 bits are used.");

        VERIFY(Inst.ContributionToHitGroupIndex == TLAS_INSTANCE_OFFSET_AUTO ||
                   (Inst.ContributionToHitGroupIndex & ~BitMask) == 0,
               "Only the lower 24 bits are used.");

        CHECK_BUILD_TLAS_ATTRIBS(Inst.InstanceName != nullptr, "pInstances[", i, "].InstanceName must not be null.");
        CHECK_BUILD_TLAS_ATTRIBS(Inst.pBLAS != nullptr, "pInstances[", i, "].pBLAS must not be null.");

        if (Attribs.Update)
        {
            const TLASInstanceDesc IDesc = Attribs.pTLAS->GetInstanceDesc(Inst.InstanceName);
            CHECK_BUILD_TLAS_ATTRIBS(IDesc.InstanceIndex != INVALID_INDEX, "Update is true, but pInstances[", i, "].InstanceName does not exists.");
        }

        if (Inst.ContributionToHitGroupIndex == TLAS_INSTANCE_OFFSET_AUTO)
            ++AutoOffsetCounter;

        CHECK_BUILD_TLAS_ATTRIBS(Attribs.BindingMode == HIT_GROUP_BINDING_MODE_USER_DEFINED || Inst.ContributionToHitGroupIndex == TLAS_INSTANCE_OFFSET_AUTO,
                                 "pInstances[", i,
                                 "].ContributionToHitGroupIndex must be TLAS_INSTANCE_OFFSET_AUTO "
                                 "if BindingMode is not HIT_GROUP_BINDING_MODE_USER_DEFINED.");
    }

    CHECK_BUILD_TLAS_ATTRIBS(AutoOffsetCounter == 0 || AutoOffsetCounter == Attribs.InstanceCount,
                             "all pInstances[i].ContributionToHitGroupIndex must be TLAS_INSTANCE_OFFSET_AUTO, or none of them should.");

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.InstanceBufferOffset <= InstDesc.Size,
                             "InstanceBufferOffset (", Attribs.InstanceBufferOffset, ") is greater than the buffer size (", InstDesc.Size, ").");

    CHECK_BUILD_TLAS_ATTRIBS(InstDesc.Size - Attribs.InstanceBufferOffset >= InstDataSize,
                             "pInstanceBuffer size (", InstDesc.Size, ") is too small: at least ",
                             InstDataSize + Attribs.InstanceBufferOffset, " bytes are required.");

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.InstanceBufferOffset % RTProps.InstanceBufferAlignment == 0,
                             "InstanceBufferOffset (", Attribs.InstanceBufferOffset, ") must be aligned by ", RTProps.InstanceBufferAlignment,
                             " (RayTracingProperties::InstanceBufferAlignment).");

    CHECK_BUILD_TLAS_ATTRIBS((InstDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                             "pInstanceBuffer was not created with BIND_RAY_TRACING flag.");

    const auto& ScratchDesc = Attribs.pScratchBuffer->GetDesc();

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.ScratchBufferOffset <= ScratchDesc.Size,
                             "ScratchBufferOffset (", Attribs.ScratchBufferOffset, ") is greater than the buffer size (", ScratchDesc.Size, ").");

    CHECK_BUILD_TLAS_ATTRIBS(Attribs.ScratchBufferOffset % RTProps.ScratchBufferAlignment == 0,
                             "ScratchBufferOffset (", Attribs.ScratchBufferOffset, ") must be aligned by ", RTProps.ScratchBufferAlignment,
                             " (RayTracingProperties::ScratchBufferAlignment).");

    if (Attribs.Update)
    {
        CHECK_BUILD_TLAS_ATTRIBS(ScratchDesc.Size - Attribs.ScratchBufferOffset >= Attribs.pTLAS->GetScratchBufferSizes().Update,
                                 "pScratchBuffer size is too small, use pTLAS->GetScratchBufferSizes().Update to get the required size for scratch buffer.");
    }
    else
    {
        CHECK_BUILD_TLAS_ATTRIBS(ScratchDesc.Size - Attribs.ScratchBufferOffset >= Attribs.pTLAS->GetScratchBufferSizes().Build,
                                 "pScratchBuffer size is too small, use pTLAS->GetScratchBufferSizes().Build to get the required size for scratch buffer.");
    }

    CHECK_BUILD_TLAS_ATTRIBS((ScratchDesc.BindFlags & BIND_RAY_TRACING) == BIND_RAY_TRACING,
                             "pScratchBuffer was not created with BIND_RAY_TRACING flag.");
#undef CHECK_BUILD_TLAS_ATTRIBS

    return true;
}


bool VerifyCopyBLASAttribs(const IRenderDevice* pDevice, const CopyBLASAttribs& Attribs)
{
#define CHECK_COPY_BLAS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Copy BLAS attribs are invalid: ", __VA_ARGS__)

    CHECK_COPY_BLAS_ATTRIBS(Attribs.pSrc != nullptr, "pSrc must not be null.");
    CHECK_COPY_BLAS_ATTRIBS(Attribs.pDst != nullptr, "pDst must not be null.");

    if (Attribs.Mode == COPY_AS_MODE_CLONE)
    {
        if (pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_VULKAN)
        {
            auto& SrcDesc = Attribs.pSrc->GetDesc();
            auto& DstDesc = Attribs.pDst->GetDesc();

            CHECK_COPY_BLAS_ATTRIBS(SrcDesc.TriangleCount == DstDesc.TriangleCount,
                                    "Src BLAS triangle count (", SrcDesc.TriangleCount, ") must be equal to the dst BLAS triangle count (", DstDesc.TriangleCount, ").");

            CHECK_COPY_BLAS_ATTRIBS(SrcDesc.BoxCount == DstDesc.BoxCount,
                                    "Src BLAS box count (", SrcDesc.BoxCount, ") must be equal to the dst BLAS box count (", DstDesc.BoxCount, ").");

            CHECK_COPY_BLAS_ATTRIBS(SrcDesc.Flags == DstDesc.Flags,
                                    "Source and destination BLASes must have been created with the same flags.");

            for (Uint32 i = 0; i < SrcDesc.TriangleCount; ++i)
            {
                const BLASTriangleDesc& SrcTri = SrcDesc.pTriangles[i];
                const Uint32            Index  = Attribs.pDst->GetGeometryDescIndex(SrcTri.GeometryName);
                CHECK_COPY_BLAS_ATTRIBS(Index != INVALID_INDEX,
                                        "Src GeometryName ('", SrcTri.GeometryName, "') at index ", i, " is not found in pDst.");
                const BLASTriangleDesc& DstTri = DstDesc.pTriangles[Index];

                CHECK_COPY_BLAS_ATTRIBS(SrcTri.MaxVertexCount == DstTri.MaxVertexCount,
                                        "MaxVertexCount value (", SrcTri.MaxVertexCount, ") in source triangle description at index ", i,
                                        " does not match MaxVertexCount value (", DstTri.MaxVertexCount, ") in the destination description.");
                CHECK_COPY_BLAS_ATTRIBS(SrcTri.VertexValueType == DstTri.VertexValueType,
                                        "VertexValueType value (", GetValueTypeString(SrcTri.VertexValueType), ") in source triangle description at index ", i,
                                        " does not match VertexValueType value (", GetValueTypeString(DstTri.VertexValueType), ") in destination description.");
                CHECK_COPY_BLAS_ATTRIBS(SrcTri.VertexComponentCount == DstTri.VertexComponentCount,
                                        "VertexComponentCount value (", Uint32{SrcTri.VertexComponentCount}, ") in source triangle description at index ", i,
                                        " does not match VertexComponentCount value (", Uint32{DstTri.VertexComponentCount}, ") in destination description.");
                CHECK_COPY_BLAS_ATTRIBS(SrcTri.MaxPrimitiveCount == DstTri.MaxPrimitiveCount,
                                        "MaxPrimitiveCount value (", SrcTri.MaxPrimitiveCount, ") in source triangle description at index ", i,
                                        " does not match MaxPrimitiveCount value (", DstTri.MaxPrimitiveCount, ") in destination description.");
                CHECK_COPY_BLAS_ATTRIBS(SrcTri.IndexType == DstTri.IndexType,
                                        "IndexType value (", GetValueTypeString(SrcTri.IndexType), ") in source triangle description at index ", i,
                                        " does not match IndexType value (", GetValueTypeString(DstTri.IndexType), ") in destination description.");
                CHECK_COPY_BLAS_ATTRIBS(SrcTri.AllowsTransforms == DstTri.AllowsTransforms,
                                        "AllowsTransforms value (", (SrcTri.AllowsTransforms ? "true" : "false"), ") in source triangle description at index ", i,
                                        " does not match AllowsTransforms value (", (DstTri.AllowsTransforms ? "true" : "false"), ") in destination description.");
            }

            for (Uint32 i = 0; i < SrcDesc.BoxCount; ++i)
            {
                const BLASBoundingBoxDesc& SrcBox = SrcDesc.pBoxes[i];
                const Uint32               Index  = Attribs.pDst->GetGeometryDescIndex(SrcBox.GeometryName);
                if (Index == INVALID_INDEX)
                {
                    LOG_ERROR_MESSAGE("Copy BLAS attribs are invalid: pSrc->GetDesc().pBoxes[", i, "].GeometryName ('", SrcBox.GeometryName, "') is not found in pDst.");
                    return false;
                }
                const BLASBoundingBoxDesc& DstBox = DstDesc.pBoxes[Index];

                CHECK_COPY_BLAS_ATTRIBS(SrcBox.MaxBoxCount == DstBox.MaxBoxCount,
                                        "MaxBoxCount value (", SrcBox.MaxBoxCount, ") in source box description at index ", i,
                                        " does not match MaxBoxCount value (", DstBox.MaxBoxCount, ") in destination description.");
            }
        }
    }
    else if (Attribs.Mode == COPY_AS_MODE_COMPACT)
    {
        auto& SrcDesc = Attribs.pSrc->GetDesc();
        auto& DstDesc = Attribs.pDst->GetDesc();

        CHECK_COPY_BLAS_ATTRIBS((SrcDesc.Flags & RAYTRACING_BUILD_AS_ALLOW_COMPACTION) == RAYTRACING_BUILD_AS_ALLOW_COMPACTION, "must be have been create with RAYTRACING_BUILD_AS_ALLOW_COMPACTION flag.");
        CHECK_COPY_BLAS_ATTRIBS(DstDesc.CompactedSize != 0, "pDst must have been create with non-zero CompactedSize.");
    }
    else
    {
        LOG_ERROR_MESSAGE("IDeviceContext::CopyBLAS: unknown Mode.");
        return false;
    }

#undef CHECK_COPY_BLAS_ATTRIBS

    return true;
}


bool VerifyCopyTLASAttribs(const CopyTLASAttribs& Attribs)
{
#define CHECK_COPY_TLAS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Copy TLAS attribs are invalid: ", __VA_ARGS__)

    CHECK_COPY_TLAS_ATTRIBS(Attribs.pSrc != nullptr, "pSrc must not be null.");
    CHECK_COPY_TLAS_ATTRIBS(Attribs.pDst != nullptr, "pDst must not be null.");

    if (Attribs.Mode == COPY_AS_MODE_CLONE)
    {
        auto& SrcDesc = Attribs.pSrc->GetDesc();
        auto& DstDesc = Attribs.pDst->GetDesc();

        CHECK_COPY_TLAS_ATTRIBS(SrcDesc.MaxInstanceCount == DstDesc.MaxInstanceCount && SrcDesc.Flags == DstDesc.Flags,
                                "pDst must have been created with the same parameters as pSrc.");
    }
    else if (Attribs.Mode == COPY_AS_MODE_COMPACT)
    {
        auto& SrcDesc = Attribs.pSrc->GetDesc();
        auto& DstDesc = Attribs.pDst->GetDesc();

        CHECK_COPY_TLAS_ATTRIBS((SrcDesc.Flags & RAYTRACING_BUILD_AS_ALLOW_COMPACTION) == RAYTRACING_BUILD_AS_ALLOW_COMPACTION, "pSrc was not created with RAYTRACING_BUILD_AS_ALLOW_COMPACTION flag.");
        CHECK_COPY_TLAS_ATTRIBS(DstDesc.CompactedSize != 0, "pDst must have been create with non-zero CompactedSize.");
    }
    else
    {
        LOG_ERROR_MESSAGE("IDeviceContext::CopyTLAS: unknown Mode.");
        return false;
    }
#undef CHECK_COPY_TLAS_ATTRIBS

    return true;
}

bool VerifyWriteBLASCompactedSizeAttribs(const IRenderDevice* pDevice, const WriteBLASCompactedSizeAttribs& Attribs)
{
#define CHECK_WRITE_BLAS_SIZE_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Write compacted BLAS size attribs are invalid: ", __VA_ARGS__)

    CHECK_WRITE_BLAS_SIZE_ATTRIBS(Attribs.pBLAS != nullptr, "pBLAS must not be null.");
    CHECK_WRITE_BLAS_SIZE_ATTRIBS((Attribs.pBLAS->GetDesc().Flags & RAYTRACING_BUILD_AS_ALLOW_COMPACTION) == RAYTRACING_BUILD_AS_ALLOW_COMPACTION,
                                  "pBLAS was not created with RAYTRACING_BUILD_AS_ALLOW_COMPACTION flag.");

    CHECK_WRITE_BLAS_SIZE_ATTRIBS(Attribs.pDestBuffer != nullptr, "pDestBuffer must not be null.");

    const BufferDesc& DstDesc = Attribs.pDestBuffer->GetDesc();
    CHECK_WRITE_BLAS_SIZE_ATTRIBS(Attribs.DestBufferOffset + sizeof(Uint64) <= DstDesc.Size, "pDestBuffer is too small.");

    if (pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_D3D12)
    {
        CHECK_WRITE_BLAS_SIZE_ATTRIBS((DstDesc.BindFlags & BIND_UNORDERED_ACCESS) == BIND_UNORDERED_ACCESS,
                                      "pDestBuffer must have been created with BIND_UNORDERED_ACCESS flag in Direct3D12.");
    }

#undef CHECK_WRITE_BLAS_SIZE_ATTRIBS

    return true;
}

bool VerifyWriteTLASCompactedSizeAttribs(const IRenderDevice* pDevice, const WriteTLASCompactedSizeAttribs& Attribs)
{
#define CHECK_WRITE_TLAS_SIZE_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Write compacted TLAS size attribs are invalid: ", __VA_ARGS__)

    CHECK_WRITE_TLAS_SIZE_ATTRIBS(Attribs.pTLAS != nullptr, "pTLAS must not be null.");
    CHECK_WRITE_TLAS_SIZE_ATTRIBS((Attribs.pTLAS->GetDesc().Flags & RAYTRACING_BUILD_AS_ALLOW_COMPACTION) == RAYTRACING_BUILD_AS_ALLOW_COMPACTION,
                                  "pTLAS was not created with RAYTRACING_BUILD_AS_ALLOW_COMPACTION flag.");

    CHECK_WRITE_TLAS_SIZE_ATTRIBS(Attribs.pDestBuffer != nullptr, "pDestBuffer must not be null.");

    const BufferDesc& DstDesc = Attribs.pDestBuffer->GetDesc();
    CHECK_WRITE_TLAS_SIZE_ATTRIBS(Attribs.DestBufferOffset + sizeof(Uint64) <= DstDesc.Size, "pDestBuffer is too small.");

    if (pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_D3D12)
    {
        CHECK_WRITE_TLAS_SIZE_ATTRIBS((DstDesc.BindFlags & BIND_UNORDERED_ACCESS) == BIND_UNORDERED_ACCESS,
                                      "pDestBuffer must have been created with BIND_UNORDERED_ACCESS flag.");
    }

#undef CHECK_WRITE_TLAS_SIZE_ATTRIBS

    return true;
}

bool VerifyTraceRaysAttribs(const TraceRaysAttribs& Attribs)
{
#define CHECK_TRACE_RAYS_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Trace rays attribs are invalid: ", __VA_ARGS__)
    CHECK_TRACE_RAYS_ATTRIBS(Attribs.pSBT != nullptr, "pSBT must not be null.");

#ifdef DILIGENT_DEVELOPMENT
    CHECK_TRACE_RAYS_ATTRIBS(Attribs.pSBT->Verify(VERIFY_SBT_FLAG_SHADER_ONLY | VERIFY_SBT_FLAG_TLAS),
                             "not all shaders in SBT are bound or instance to shader mapping is incorrect.");
#endif // DILIGENT_DEVELOPMENT

    CHECK_TRACE_RAYS_ATTRIBS(Attribs.DimensionX != 0, "DimensionX must not be zero.");
    CHECK_TRACE_RAYS_ATTRIBS(Attribs.DimensionY != 0, "DimensionY must not be zero.");
    CHECK_TRACE_RAYS_ATTRIBS(Attribs.DimensionZ != 0, "DimensionZ must not be zero.");

#undef CHECK_TRACE_RAYS_ATTRIBS

    return true;
}

bool VerifyTraceRaysIndirectAttribs(const IRenderDevice* pDevice, const TraceRaysIndirectAttribs& Attribs, Uint32 SBTSize)
{
    const IBuffer* pAttribsBuffer = Attribs.pAttribsBuffer;

#define CHECK_TRACE_RAYS_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Trace rays indirect attribs are invalid: ", __VA_ARGS__)
    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS(Attribs.pSBT != nullptr, "pSBT must not be null");

#ifdef DILIGENT_DEVELOPMENT
    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS(Attribs.pSBT->Verify(VERIFY_SBT_FLAG_SHADER_ONLY | VERIFY_SBT_FLAG_TLAS),
                                      "not all shaders in SBT are bound or instance to shader mapping is incorrect.");
#endif // DILIGENT_DEVELOPMENT

    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect dispatch arguments buffer must not be null.");

    const auto& Desc = pAttribsBuffer->GetDesc();
    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS((Desc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0,
                                      "indirect trace rays arguments buffer '", Desc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS((Desc.BindFlags & BIND_RAY_TRACING) != 0,
                                      "indirect trace rays arguments buffer '", Desc.Name, "' was not created with BIND_RAY_TRACING flag.");
    CHECK_TRACE_RAYS_INDIRECT_ATTRIBS(Attribs.ArgsByteOffset + SBTSize <= Desc.Size,
                                      "indirect trace rays arguments buffer '", Desc.Name, "' is too small");

#undef CHECK_TRACE_RAYS_INDIRECT_ATTRIBS

    return true;
}

bool VerifyBindSparseResourceMemoryAttribs(const IRenderDevice* pDevice, const BindSparseResourceMemoryAttribs& Attribs)
{
#define CHECK_BIND_SPARSE_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Bind sparse memory attribs are invalid: ", __VA_ARGS__)

    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (Attribs.NumBufferBinds != 0)
    {
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.pBufferBinds != nullptr, "NumBufferBinds is ", Attribs.NumBufferBinds, ", but pBufferBinds is null");
        CHECK_BIND_SPARSE_ATTRIBS((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) != 0, "NumBufferBinds must be zero if SPARSE_RESOURCE_CAP_FLAG_BUFFER capability is not supported");
    }
    if (Attribs.NumTextureBinds != 0)
    {
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.pTextureBinds != nullptr, "NumTextureBinds is ", Attribs.NumTextureBinds, ", but pTextureBinds is null");
        CHECK_BIND_SPARSE_ATTRIBS((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) != 0, "NumTextureBinds must be zero if SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D capability is not supported");
    }

    CHECK_BIND_SPARSE_ATTRIBS(Attribs.NumBufferBinds > 0 || Attribs.NumTextureBinds > 0, "One of NumBufferBinds and NumTextureBinds must not be zero");

#ifdef DILIGENT_DEVELOPMENT
    const auto IsMetal = pDevice->GetDeviceInfo().IsMetalDevice();

    for (Uint32 i = 0; i < Attribs.NumBufferBinds; ++i)
    {
        const auto& Bind = Attribs.pBufferBinds[i];
        CHECK_BIND_SPARSE_ATTRIBS(Bind.pBuffer != nullptr, "pBufferBinds[", i, "].pBuffer must not be null");
        CHECK_BIND_SPARSE_ATTRIBS(Bind.NumRanges != 0, "pBufferBinds[", i, "].NumRanges must not be zero");
        CHECK_BIND_SPARSE_ATTRIBS(Bind.pRanges != nullptr, "pBufferBinds[", i, "].pRanges must not be null");

        if (Bind.pBuffer == nullptr)
            continue;

        const auto& Desc = Bind.pBuffer->GetDesc();
        CHECK_BIND_SPARSE_ATTRIBS(Desc.Usage == USAGE_SPARSE, "pBufferBinds[", i, "].pBuffer must be created with USAGE_SPARSE");

        if (Bind.pRanges == nullptr)
            continue;

        const auto& BuffSparseProps = Bind.pBuffer->GetSparseProperties();
        for (Uint32 r = 0; r < Bind.NumRanges; ++r)
        {
            const auto& Range = Bind.pRanges[r];
            CHECK_BIND_SPARSE_ATTRIBS(Range.BufferOffset + Range.MemorySize <= Desc.Size,
                                      "pBufferBinds[", i, "].pRanges[", r, "] specifies a range with offset ", Range.BufferOffset, " and size ",
                                      Range.MemorySize, " that exceeds the buffer size (", Desc.Size, ")");
            CHECK_BIND_SPARSE_ATTRIBS(Range.BufferOffset % BuffSparseProps.BlockSize == 0,
                                      "pBufferBinds[", i, "].pRanges[", r, "].BufferOffset (", Range.BufferOffset,
                                      ") must be a multiple of the buffer block size (", BuffSparseProps.BlockSize, ")");

            CHECK_BIND_SPARSE_ATTRIBS(Range.MemorySize != 0,
                                      "pBufferBinds[", i, "].pRanges[", r, "].MemorySize must not be zero");
            CHECK_BIND_SPARSE_ATTRIBS(Range.MemorySize % BuffSparseProps.BlockSize == 0,
                                      "pBufferBinds[", i, "].pRanges[", r, "].MemorySize must be a multiple of the buffer block size (", BuffSparseProps.BlockSize, ")");

            if (Range.pMemory != nullptr)
            {
                CHECK_BIND_SPARSE_ATTRIBS(Range.pMemory->IsCompatible(Bind.pBuffer),
                                          "pBufferBinds[", i, "].pRanges[", r, "].pMemory must be compatible with pBuffer");

                const auto Capacity = Range.pMemory->GetCapacity();
                CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset + Range.MemorySize <= Capacity,
                                          "pBufferBinds[", i, "].pRanges[", r, "] specifies a range with offset ", Range.MemoryOffset,
                                          " and size ", Range.MemorySize, " that exceeds the device memory size (", Capacity, ")");
                // Can not check here because final memory offset depends on the device memory object implementation
                //CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset % BuffSparseProps.BlockSize == 0)

                const auto PageSize = Range.pMemory->GetDesc().PageSize;
                CHECK_BIND_SPARSE_ATTRIBS((Range.MemoryOffset % PageSize) + Range.MemorySize <= PageSize,
                                          "pBufferBinds[", i, "].pRanges[", r, "] specifies a range with offset ", Range.MemoryOffset,
                                          " and size ", Range.MemorySize,
                                          " that does not fit into a single page. in Direct3D12 and Vulkan this will be an error");
            }
            else
            {
                CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset == 0,
                                          "pBufferBinds[", i, "].pRanges[", r, "]: if pMemory is null, but MemoryOffset is not zero");
            }
        }
    }

    for (Uint32 i = 0; i < Attribs.NumTextureBinds; ++i)
    {
        const auto& Bind = Attribs.pTextureBinds[i];
        CHECK_BIND_SPARSE_ATTRIBS(Bind.pTexture != nullptr, "pTextureBinds[", i, "].pTexture must not be null");
        CHECK_BIND_SPARSE_ATTRIBS(Bind.NumRanges != 0, "pTextureBinds[", i, "].NumRanges must not be zero");
        CHECK_BIND_SPARSE_ATTRIBS(Bind.pRanges != nullptr, "pTextureBinds[", i, "].pRanges must not be null");

        if (Bind.pTexture == nullptr)
            continue;

        const auto& Desc = Bind.pTexture->GetDesc();
        CHECK_BIND_SPARSE_ATTRIBS(Desc.Usage == USAGE_SPARSE, "pTextureBinds[", i, "].pTexture must be created with USAGE_SPARSE");

        if (Bind.pRanges == nullptr)
            continue;

        const auto& TexSparseProps = Bind.pTexture->GetSparseProperties();
        for (Uint32 r = 0; r < Bind.NumRanges; ++r)
        {
            const auto& Range     = Bind.pRanges[r];
            const auto& Region    = Range.Region;
            const auto  MipProps  = GetMipLevelProperties(Desc, Range.MipLevel);
            const auto  MipWidth  = MipProps.StorageWidth;
            const auto  MipHeight = MipProps.StorageHeight;
            const auto  MipDepth  = MipProps.Depth;

            CHECK_BIND_SPARSE_ATTRIBS(Range.MipLevel < Desc.MipLevels,
                                      "pTextureBinds[", i, "].pRanges[", r, "].MipLevel (", Range.MipLevel,
                                      ") must be less than the mip level count (", Desc.MipLevels, ").");
            CHECK_BIND_SPARSE_ATTRIBS(Range.MipLevel <= TexSparseProps.FirstMipInTail,
                                      "pTextureBinds[", i, "].pRanges[", r, "].MipLevel (", Range.MipLevel,
                                      ") must not exceed the first mip in tail (", TexSparseProps.FirstMipInTail, ")");

            if (Range.MipLevel < TexSparseProps.FirstMipInTail)
            {
                CHECK_BIND_SPARSE_ATTRIBS(Range.OffsetInMipTail == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "].OffsetInMipTail must be zero when Range.MipLevel is not a tail mip");
                CHECK_BIND_SPARSE_ATTRIBS(Region.IsValid(),
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region must be valid");
                CHECK_BIND_SPARSE_ATTRIBS(Region.MaxX <= MipWidth || Region.MaxX == AlignUp(MipWidth, TexSparseProps.TileSize[0]),
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MaxX (", Region.MaxX, ") must not exceed the mip level width (", MipWidth, ")");
                CHECK_BIND_SPARSE_ATTRIBS(Region.MaxY <= MipHeight || Region.MaxY == AlignUp(MipHeight, TexSparseProps.TileSize[1]),
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MaxY (", Region.MaxY, ") must not exceed the mip level height (", MipHeight, ")");
                CHECK_BIND_SPARSE_ATTRIBS(Region.MaxZ <= MipDepth || Region.MaxZ == AlignUp(MipDepth, TexSparseProps.TileSize[2]),
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MaxZ (", Region.MaxZ, ") must not exceed the mip level depth (", MipDepth, ")");

                CHECK_BIND_SPARSE_ATTRIBS((Region.MinX % TexSparseProps.TileSize[0]) == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MinX (", Region.MinX, ") must be a multiple of the tile width (", TexSparseProps.TileSize[0], ")");
                CHECK_BIND_SPARSE_ATTRIBS((Region.MinY % TexSparseProps.TileSize[1]) == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MinY (", Region.MinY, ") must be a multiple of the tile height (", TexSparseProps.TileSize[1], ")");
                CHECK_BIND_SPARSE_ATTRIBS((Region.MinZ % TexSparseProps.TileSize[2]) == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region.MinZ (", Region.MinZ, ") must be a multiple of the tile depth (", TexSparseProps.TileSize[2], ")");
                CHECK_BIND_SPARSE_ATTRIBS((Region.Width() % TexSparseProps.TileSize[0]) == 0 || Region.MaxX == MipWidth,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region width (", Region.Width(), ") must be a multiple of the tile width (", TexSparseProps.TileSize[0], ")");
                CHECK_BIND_SPARSE_ATTRIBS((Region.Height() % TexSparseProps.TileSize[1]) == 0 || Region.MaxY == MipHeight,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region height (", Region.Height(), ") must be multiple of the tile height (", TexSparseProps.TileSize[1], ")");
                CHECK_BIND_SPARSE_ATTRIBS((Region.Depth() % TexSparseProps.TileSize[2]) == 0 || Region.MaxZ == MipDepth,
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region depth (", Region.Depth(), ") must be a multiple of the tile depth (", TexSparseProps.TileSize[2], ")");
            }
            else
            {
                CHECK_BIND_SPARSE_ATTRIBS(Region == Box{},
                                          "pTextureBinds[", i, "].pRanges[", r, "].Region must be default when Range.MipLevel is a tail mip");
                CHECK_BIND_SPARSE_ATTRIBS(Range.OffsetInMipTail + Range.MemorySize <= TexSparseProps.MipTailSize,
                                          "pTextureBinds[", i, "].pRanges[", r, "] specifies OffsetInMipTail (", Range.OffsetInMipTail,
                                          ") and MemorySize (", Range.MemorySize, ") that exceed the mip tail size (", TexSparseProps.MipTailSize, ")");
                CHECK_BIND_SPARSE_ATTRIBS(Range.OffsetInMipTail % TexSparseProps.BlockSize == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "].OffsetInMipTail (", Range.OffsetInMipTail,
                                          ") must be a multiple of the texture block size (", TexSparseProps.BlockSize, ")");

                if (TexSparseProps.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL)
                {
                    CHECK_BIND_SPARSE_ATTRIBS(Range.ArraySlice == 0,
                                              "pTextureBinds[", i, "].pRanges[", r, "].ArraySlice (", Range.ArraySlice, ") must be 0 for a single mip tail");
                }

                if (IsMetal)
                {
                    CHECK_BIND_SPARSE_ATTRIBS(Range.OffsetInMipTail == 0 && (Range.MemorySize == 0 || Range.MemorySize == TexSparseProps.MipTailSize),
                                              "pTextureBinds[", i, "].pRanges[", r, "]: in Metal, mip tail must be bound in a single memory range: ",
                                              "OffsetInMipTail (", Range.OffsetInMipTail, ") must be zero and MemorySize (", Range.MemorySize,
                                              ") must be equal to the mip tail size (", TexSparseProps.MipTailSize, ") or zero");
                }
            }

            CHECK_BIND_SPARSE_ATTRIBS(Range.ArraySlice < Desc.GetArraySize(),
                                      "pTextureBinds[", i, "].pRanges[", r, "].ArraySlice (", Range.ArraySlice, ") must be less than the array size (", Desc.GetArraySize(), ")");

            // MemorySize can be zero
            CHECK_BIND_SPARSE_ATTRIBS(Range.MemorySize % TexSparseProps.BlockSize == 0,
                                      "pTextureBinds[", i, "].pRanges[", r, "].MemorySize (", Range.MemorySize,
                                      ") must be a multiple of the texture block size (", TexSparseProps.BlockSize, ")");

            // In Metal, MemorySize is not defined and not used
            if (!IsMetal)
            {
                if (Range.MemorySize != 0 && Range.MipLevel < TexSparseProps.FirstMipInTail)
                {
                    const uint3 TilesInBox = GetNumSparseTilesInBox(Region, TexSparseProps.TileSize);
                    const auto  NumBlocks  = TilesInBox.x * TilesInBox.y * TilesInBox.z;
                    CHECK_BIND_SPARSE_ATTRIBS(Uint64{NumBlocks} * Uint64{TexSparseProps.BlockSize} == Range.MemorySize,
                                              "pTextureBinds[", i, "].pRanges[", r, "].MemorySize (", Range.MemorySize, ") does not match the sparse memory blocks count (",
                                              NumBlocks, ") in the specified region");
                }

                // packed mip tail
                if (Range.MipLevel == TexSparseProps.FirstMipInTail)
                {
                    // MemorySize is used to specify how much block must be bound/unbound
                    CHECK_BIND_SPARSE_ATTRIBS(Range.MemorySize != 0,
                                              "pTextureBinds[", i, "].pRanges[", r, "].MemorySize must not be zero");
                }

                if (Range.pMemory != nullptr)
                {
                    CHECK_BIND_SPARSE_ATTRIBS(Range.pMemory->IsCompatible(Bind.pTexture),
                                              "pTextureBinds[", i, "].pRanges[", r, "].pMemory must be compatible with pTexture");

                    const auto Capacity = Range.pMemory->GetCapacity();
                    CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset + Range.MemorySize <= Capacity,
                                              "pTextureBinds[", i, "].pRanges[", r, "] specifies MemoryOffset (", Range.MemoryOffset, ") and MemorySize (",
                                              Range.MemorySize, ") that exceed the memory capacity (", Capacity, ")");
                    // Can not check here because the final memory offset depends on the device memory object implementation
                    //CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset % BuffSparseProps.BlockSize == 0)

                    const auto PageSize = Range.pMemory->GetDesc().PageSize;
                    CHECK_BIND_SPARSE_ATTRIBS((Range.MemoryOffset % PageSize) + Range.MemorySize <= PageSize,
                                              "pTextureBinds[", i, "].pRanges[", r, "] specifies MemoryOffset (", Range.MemoryOffset, ") and MemorySize (", Range.MemorySize,
                                              ") that don't fit into a single page. In Direct3D12 and Vulkan this will be an error");
                }
            }

            if (Range.pMemory == nullptr)
            {
                CHECK_BIND_SPARSE_ATTRIBS(Range.MemoryOffset == 0,
                                          "pTextureBinds[", i, "].pRanges[", r, "]: pMemory is null, but MemoryOffset is not zero");
            }
        }
    }
#endif // DILIGENT_DEVELOPMENT

    if (Attribs.NumWaitFences != 0)
    {
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppWaitFences != nullptr, "ppWaitFences must not be null if NumWaitFences is not zero");
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.pWaitFenceValues != nullptr, "pWaitFenceValues must not be null if NumWaitFences is not zero");

        if (Attribs.pWaitFenceValues != nullptr)
        {
            for (Uint32 i = 0; i < Attribs.NumWaitFences; ++i)
            {
                CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppWaitFences[i] != nullptr, "ppWaitFences[", i, "] must not be null");
                CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppWaitFences[i]->GetDesc().Type == FENCE_TYPE_GENERAL, "ppWaitFences[", i, "] must be GENERAL type");
            }
        }
    }

    if (Attribs.NumSignalFences != 0)
    {
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppSignalFences != nullptr, "ppSignalFences must not be null if NumSignalFences is not zero");
        CHECK_BIND_SPARSE_ATTRIBS(Attribs.pSignalFenceValues != nullptr, "pSignalFenceValues must not be null if NumSignalFences is not zero");

        if (Attribs.pSignalFenceValues != nullptr)
        {
            for (Uint32 i = 0; i < Attribs.NumSignalFences; ++i)
            {
                CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppSignalFences[i] != nullptr, "ppSignalFences[", i, "] must not be null");
                CHECK_BIND_SPARSE_ATTRIBS(Attribs.ppSignalFences[i]->GetDesc().Type == FENCE_TYPE_GENERAL, "ppSignalFences[", i, "] must be GENERAL type");
            }
        }
    }

#undef CHECK_BIND_SPARSE_ATTRIBS

    return true;
}

} // namespace Diligent
