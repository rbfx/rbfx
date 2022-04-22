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

#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"

void TestDeviceContextCInterface(struct IDeviceContext* pCtx)
{
    const struct DeviceContextDesc* pDesc       = NULL;
    Uint64                          FrameNumber = 0;

    pDesc = IDeviceContext_GetDesc(pCtx);
    (void)(pDesc);

    IDeviceContext_Begin(pCtx, 0u);

    IDeviceContext_TransitionShaderResources(pCtx, (struct IPipelineState*)NULL, (struct IShaderResourceBinding*)NULL);
    IDeviceContext_TransitionResourceStates(pCtx, 1u, (const struct StateTransitionDesc*)NULL);

    IDeviceContext_SetPipelineState(pCtx, (struct IPipelineState*)NULL);
    IDeviceContext_CommitShaderResources(pCtx, (struct IShaderResourceBinding*)NULL, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_SetStencilRef(pCtx, 1u);
    IDeviceContext_SetBlendFactors(pCtx, (const float*)NULL);
    IDeviceContext_SetVertexBuffers(pCtx, 0u, 1u, (struct IBuffer**)NULL, (const Uint64*)NULL, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET);
    IDeviceContext_InvalidateState(pCtx);
    IDeviceContext_SetIndexBuffer(pCtx, (struct IBuffer*)NULL, (Uint64)0, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_SetViewports(pCtx, 1u, (const struct Viewport*)NULL, 512u, 512u);
    IDeviceContext_SetScissorRects(pCtx, 1u, (const struct Rect*)NULL, 512u, 512u);
    IDeviceContext_SetRenderTargets(pCtx, 1u, (struct ITextureView**)NULL, (struct ITextureView*)NULL, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_SetRenderTargetsExt(pCtx, (struct SetRenderTargetsAttribs*)NULL);
    IDeviceContext_BeginRenderPass(pCtx, (const struct BeginRenderPassAttribs*)NULL);
    IDeviceContext_NextSubpass(pCtx);
    IDeviceContext_EndRenderPass(pCtx);
    IDeviceContext_ClearDepthStencil(pCtx, (struct ITextureView*)NULL, CLEAR_DEPTH_FLAG, 1.0f, (Uint8)0, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_ClearRenderTarget(pCtx, (struct ITextureView*)NULL, (const float*)NULL, RESOURCE_STATE_TRANSITION_MODE_NONE);

    IDeviceContext_Draw(pCtx, (struct DrawAttribs*)NULL);
    IDeviceContext_DrawIndexed(pCtx, (struct DrawIndexedAttribs*)NULL);
    IDeviceContext_DrawIndirect(pCtx, (struct DrawIndirectAttribs*)NULL);
    IDeviceContext_DrawIndexedIndirect(pCtx, (struct DrawIndexedIndirectAttribs*)NULL);
    IDeviceContext_DrawMesh(pCtx, (struct DrawMeshAttribs*)NULL);
    IDeviceContext_DrawMeshIndirect(pCtx, (struct DrawMeshIndirectAttribs*)NULL);

    IDeviceContext_DispatchCompute(pCtx, (const DispatchComputeAttribs*)NULL);
    IDeviceContext_DispatchComputeIndirect(pCtx, (const DispatchComputeIndirectAttribs*)NULL);

    IDeviceContext_FinishCommandList(pCtx, (struct ICommandList**)NULL);
    IDeviceContext_ExecuteCommandLists(pCtx, 2u, (struct ICommandList* const*)NULL);

    IDeviceContext_EnqueueSignal(pCtx, (struct IFence*)NULL, (Uint64)1);
    IDeviceContext_DeviceWaitForFence(pCtx, (struct IFence*)NULL, (Uint64)1);
    IDeviceContext_WaitForIdle(pCtx);
    IDeviceContext_Flush(pCtx);
    IDeviceContext_FinishFrame(pCtx);

    FrameNumber = IDeviceContext_GetFrameNumber(pCtx);
    (void)(FrameNumber);

    IDeviceContext_BeginQuery(pCtx, (struct IQuery*)NULL);
    IDeviceContext_EndQuery(pCtx, (struct IQuery*)NULL);

    IDeviceContext_UpdateBuffer(pCtx, (struct IBuffer*)NULL, (Uint64)1, (Uint64)1, NULL, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_CopyBuffer(pCtx, (struct IBuffer*)NULL, (Uint64)0, RESOURCE_STATE_TRANSITION_MODE_NONE, (struct IBuffer*)NULL, (Uint64)0, (Uint64)128, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_MapBuffer(pCtx, (struct IBuffer*)NULL, MAP_WRITE, MAP_FLAG_DISCARD, (void**)NULL);
    IDeviceContext_UnmapBuffer(pCtx, (struct IBuffer*)NULL, MAP_WRITE);
    IDeviceContext_UpdateTexture(pCtx, (struct ITexture*)NULL, 0u, 0u, (const struct Box*)NULL, (const struct TextureSubResData*)NULL, RESOURCE_STATE_TRANSITION_MODE_NONE, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IDeviceContext_CopyTexture(pCtx, (const struct CopyTextureAttribs*)NULL);
    IDeviceContext_MapTextureSubresource(pCtx, (struct ITexture*)NULL, 0u, 0u, MAP_WRITE, MAP_FLAG_DISCARD, (const struct Box*)NULL, (struct MappedTextureSubresource*)NULL);
    IDeviceContext_UnmapTextureSubresource(pCtx, (struct ITexture*)NULL, 0u, 0u);
    IDeviceContext_GenerateMips(pCtx, (struct ITextureView*)NULL);
    IDeviceContext_ResolveTextureSubresource(pCtx, (struct ITexture*)NULL, (struct ITexture*)NULL, (const struct ResolveTextureSubresourceAttribs*)NULL);

    IDeviceContext_BuildBLAS(pCtx, (struct BuildBLASAttribs*)NULL);
    IDeviceContext_BuildTLAS(pCtx, (struct BuildTLASAttribs*)NULL);
    IDeviceContext_CopyBLAS(pCtx, (struct CopyBLASAttribs*)NULL);
    IDeviceContext_CopyTLAS(pCtx, (struct CopyTLASAttribs*)NULL);
    IDeviceContext_WriteBLASCompactedSize(pCtx, (struct WriteBLASCompactedSizeAttribs*)NULL);
    IDeviceContext_WriteTLASCompactedSize(pCtx, (struct WriteTLASCompactedSizeAttribs*)NULL);
    IDeviceContext_TraceRays(pCtx, (struct TraceRaysAttribs*)NULL);
    IDeviceContext_TraceRaysIndirect(pCtx, (struct TraceRaysIndirectAttribs*)NULL);
    IDeviceContext_UpdateSBT(pCtx, (struct IShaderBindingTable*)NULL, (const struct UpdateIndirectRTBufferAttribs*)NULL);

    struct IObject* pUserData = NULL;
    IDeviceContext_SetUserData(pCtx, pUserData);
    pUserData = IDeviceContext_GetUserData(pCtx);

    IDeviceContext_BeginDebugGroup(pCtx, (const Char*)NULL, (const float*)NULL);
    IDeviceContext_EndDebugGroup(pCtx);
    IDeviceContext_InsertDebugLabel(pCtx, (const Char*)NULL, (const float*)NULL);

    struct ICommandQueue* pQueue = IDeviceContext_LockCommandQueue(pCtx);
    (void)pQueue;
    IDeviceContext_UnlockCommandQueue(pCtx);

    IDeviceContext_DispatchTile(pCtx, (const struct DispatchTileAttribs*)NULL);
    IDeviceContext_GetTileSize(pCtx, (Uint32*)NULL, (Uint32*)NULL);

    IDeviceContext_SetShadingRate(pCtx, SHADING_RATE_1X1, SHADING_RATE_COMBINER_PASSTHROUGH, SHADING_RATE_COMBINER_PASSTHROUGH);

    IDeviceContext_BindSparseResourceMemory(pCtx, (const BindSparseResourceMemoryAttribs*)NULL);
}
