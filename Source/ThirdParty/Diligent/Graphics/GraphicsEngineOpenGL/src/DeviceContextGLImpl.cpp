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

#include "pch.h"

#include "DeviceContextGLImpl.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <array>

#include "SwapChainGL.h"

#include "RenderDeviceGLImpl.hpp"
#include "BufferGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "Texture1D_GL.hpp"
#include "Texture1DArray_GL.hpp"
#include "Texture2D_GL.hpp"
#include "Texture2DArray_GL.hpp"
#include "Texture3D_GL.hpp"
#include "SamplerGLImpl.hpp"
#include "BufferViewGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"
#include "FenceGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"

#include "GLTypeConversions.hpp"
#include "VAOCache.hpp"
#include "GraphicsAccessories.hpp"


namespace Diligent
{

DeviceContextGLImpl::DeviceContextGLImpl(IReferenceCounters*      pRefCounters,
                                         RenderDeviceGLImpl*      pDeviceGL,
                                         const DeviceContextDesc& Desc) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDeviceGL,
        Desc
    },
    m_ContextState{pDeviceGL},
    m_DefaultFBO  {false    }
// clang-format on
{
    m_BoundWritableTextures.reserve(16);
    m_BoundWritableBuffers.reserve(16);
}

IMPLEMENT_QUERY_INTERFACE(DeviceContextGLImpl, IID_DeviceContextGL, TDeviceContextBase)


void DeviceContextGLImpl::Begin(Uint32 ImmediateContextId)
{
    UNEXPECTED("OpenGL does not support deferred contexts");
    (void)(ImmediateContextId);
}

void DeviceContextGLImpl::SetPipelineState(IPipelineState* pPipelineState)
{
    VERIFY_EXPR(pPipelineState != nullptr);

    RefCntAutoPtr<PipelineStateGLImpl> pPipelineStateGLImpl{pPipelineState, PipelineStateGLImpl::IID_InternalImpl};
    VERIFY(pPipelineState == nullptr || pPipelineStateGLImpl != nullptr, "Unknown pipeline state object implementation");
    if (PipelineStateGLImpl::IsSameObject(m_pPipelineState, pPipelineStateGLImpl))
        return;

    TDeviceContextBase::SetPipelineState(std::move(pPipelineStateGLImpl), 0 /*Dummy*/);

    const auto& Desc = m_pPipelineState->GetDesc();
    if (Desc.PipelineType == PIPELINE_TYPE_COMPUTE)
    {
    }
    else if (Desc.PipelineType == PIPELINE_TYPE_GRAPHICS)
    {
        const auto& GraphicsPipeline = m_pPipelineState->GetGraphicsPipelineDesc();
        // Set rasterizer state
        {
            const auto& RasterizerDesc = GraphicsPipeline.RasterizerDesc;

            m_ContextState.SetFillMode(RasterizerDesc.FillMode);
            m_ContextState.SetCullMode(RasterizerDesc.CullMode);
            m_ContextState.SetFrontFace(RasterizerDesc.FrontCounterClockwise);
            m_ContextState.SetDepthBias(static_cast<Float32>(RasterizerDesc.DepthBias), RasterizerDesc.SlopeScaledDepthBias);
            if (RasterizerDesc.DepthBiasClamp != 0)
                LOG_WARNING_MESSAGE("Depth bias clamp is not supported on OpenGL");

            // Enabling depth clamping in GL is the same as disabling clipping in Direct3D.
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
            // https://www.khronos.org/opengl/wiki/GLAPI/glEnable
            m_ContextState.SetDepthClamp(!RasterizerDesc.DepthClipEnable);

            m_ContextState.EnableScissorTest(RasterizerDesc.ScissorEnable);
            if (RasterizerDesc.AntialiasedLineEnable)
                LOG_WARNING_MESSAGE("Line antialiasing is not supported on OpenGL");
        }

        // Set blend state
        {
            const auto& BSDsc = GraphicsPipeline.BlendDesc;
            m_ContextState.SetBlendState(BSDsc, GraphicsPipeline.SampleMask);
        }

        // Set depth-stencil state
        {
            const auto& DepthStencilDesc = GraphicsPipeline.DepthStencilDesc;

            m_ContextState.EnableDepthTest(DepthStencilDesc.DepthEnable);
            m_ContextState.EnableDepthWrites(DepthStencilDesc.DepthWriteEnable);
            m_ContextState.SetDepthFunc(DepthStencilDesc.DepthFunc);
            m_ContextState.EnableStencilTest(DepthStencilDesc.StencilEnable);
            m_ContextState.SetStencilWriteMask(DepthStencilDesc.StencilWriteMask);

            {
                const auto& FrontFace = DepthStencilDesc.FrontFace;
                m_ContextState.SetStencilFunc(GL_FRONT, FrontFace.StencilFunc, m_StencilRef, DepthStencilDesc.StencilReadMask);
                m_ContextState.SetStencilOp(GL_FRONT, FrontFace.StencilFailOp, FrontFace.StencilDepthFailOp, FrontFace.StencilPassOp);
            }

            {
                const auto& BackFace = DepthStencilDesc.BackFace;
                m_ContextState.SetStencilFunc(GL_BACK, BackFace.StencilFunc, m_StencilRef, DepthStencilDesc.StencilReadMask);
                m_ContextState.SetStencilOp(GL_BACK, BackFace.StencilFailOp, BackFace.StencilDepthFailOp, BackFace.StencilPassOp);
            }
        }
        m_ContextState.InvalidateVAO();
    }
    else
    {
        LOG_ERROR_MESSAGE(GetPipelineTypeString(Desc.PipelineType), " pipeline '", Desc.Name, "' is not supported in OpenGL");
        return;
    }

    // Note that the program may change if a shader is created after the call
    // (ShaderResourcesGL needs to bind a program to load uniforms), but before
    // the draw command.
    m_pPipelineState->CommitProgram(m_ContextState);

    Uint32 DvpCompatibleSRBCount = 0;
    PrepareCommittedResources(m_BindInfo, DvpCompatibleSRBCount);
}

void DeviceContextGLImpl::TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding)
{
    DEV_CHECK_ERR(!m_pActiveRenderPass, "State transitions are not allowed inside a render pass.");
}

void DeviceContextGLImpl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    DeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0);

    auto* const pShaderResBindingGL = ClassPtrCast<ShaderResourceBindingGLImpl>(pShaderResourceBinding);
    const auto  SRBIndex            = pShaderResBindingGL->GetBindingIndex();

    m_BindInfo.Set(SRBIndex, pShaderResBindingGL);

#ifdef DILIGENT_DEBUG
    pShaderResBindingGL->GetResourceCache().DbgVerifyDynamicBufferMasks();
#endif
}

void DeviceContextGLImpl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        m_ContextState.SetStencilRef(GL_FRONT, StencilRef);
        m_ContextState.SetStencilRef(GL_BACK, StencilRef);
    }
}

void DeviceContextGLImpl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        m_ContextState.SetBlendFactors(m_BlendFactors);
    }
}

void DeviceContextGLImpl::SetVertexBuffers(Uint32                         StartSlot,
                                           Uint32                         NumBuffersSet,
                                           IBuffer**                      ppBuffers,
                                           const Uint64*                  pOffsets,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                           SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);
    m_ContextState.InvalidateVAO();
}

void DeviceContextGLImpl::InvalidateState()
{
    TDeviceContextBase::InvalidateState();

    m_ContextState.Invalidate();
    m_BindInfo.Invalidate();
    m_BoundWritableTextures.clear();
    m_BoundWritableBuffers.clear();
    m_IsDefaultFBOBound = false;
}

void DeviceContextGLImpl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint64 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);
    m_ContextState.InvalidateVAO();
}

void DeviceContextGLImpl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);

    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");
    if (NumViewports == 1)
    {
        const auto& vp = m_Viewports[0];
        // Note that OpenGL and DirectX use different origin of
        // the viewport in window coordinates:
        //
        // DirectX (0,0)
        //     \ ____________
        //      |            |
        //      |            |
        //      |            |
        //      |            |
        //      |____________|
        //     /
        //  OpenGL (0,0)
        //
        float BottomLeftY = static_cast<float>(RTHeight) - (vp.TopLeftY + vp.Height);
        float BottomLeftX = vp.TopLeftX;

        Int32 x = static_cast<int>(BottomLeftX);
        Int32 y = static_cast<int>(BottomLeftY);
        Int32 w = static_cast<int>(vp.Width);
        Int32 h = static_cast<int>(vp.Height);
        if (static_cast<float>(x) == BottomLeftX &&
            static_cast<float>(y) == BottomLeftY &&
            static_cast<float>(w) == vp.Width &&
            static_cast<float>(h) == vp.Height)
        {
            // GL_INVALID_VALUE is generated if either width or height is negative
            // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glViewport.xml
            glViewport(x, y, w, h);
        }
        else
        {
            // GL_INVALID_VALUE is generated if either width or height is negative
            // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glViewportIndexed.xhtml
            glViewportIndexedf(0, BottomLeftX, BottomLeftY, vp.Width, vp.Height);
        }
        DEV_CHECK_GL_ERROR("Failed to set viewport");

        glDepthRangef(vp.MinDepth, vp.MaxDepth);
        DEV_CHECK_GL_ERROR("Failed to set depth range");
    }
    else
    {
        for (Uint32 i = 0; i < NumViewports; ++i)
        {
            const auto& vp          = m_Viewports[i];
            float       BottomLeftY = static_cast<float>(RTHeight) - (vp.TopLeftY + vp.Height);
            float       BottomLeftX = vp.TopLeftX;
            glViewportIndexedf(i, BottomLeftX, BottomLeftY, vp.Width, vp.Height);
            DEV_CHECK_GL_ERROR("Failed to set viewport #", i);
            glDepthRangeIndexed(i, vp.MinDepth, vp.MaxDepth);
            DEV_CHECK_GL_ERROR("Failed to set depth range for viewport #", i);
        }
    }
}

void DeviceContextGLImpl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
    if (NumRects == 1)
    {
        const auto& Rect = m_ScissorRects[0];
        // Note that OpenGL and DirectX use different origin
        // of the viewport in window coordinates:
        //
        // DirectX (0,0)
        //     \ ____________
        //      |            |
        //      |            |
        //      |            |
        //      |            |
        //      |____________|
        //     /
        //  OpenGL (0,0)
        //
        auto glBottom = RTHeight - Rect.bottom;

        auto width  = Rect.right - Rect.left;
        auto height = Rect.bottom - Rect.top;
        glScissor(Rect.left, glBottom, width, height);
        DEV_CHECK_GL_ERROR("Failed to set scissor rect");
    }
    else
    {
        for (Uint32 sr = 0; sr < NumRects; ++sr)
        {
            const auto& Rect     = m_ScissorRects[sr];
            auto        glBottom = RTHeight - Rect.bottom;
            auto        width    = Rect.right - Rect.left;
            auto        height   = Rect.bottom - Rect.top;
            glScissorIndexed(sr, Rect.left, glBottom, width, height);
            DEV_CHECK_GL_ERROR("Failed to set scissor rect #", sr);
        }
    }
}

void DeviceContextGLImpl::SetSwapChain(ISwapChainGL* pSwapChain)
{
    m_pSwapChain = pSwapChain;
}

void DeviceContextGLImpl::CommitRenderTargets()
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "This method must not be called inside render pass");

    if (!m_IsDefaultFBOBound && m_NumBoundRenderTargets == 0 && !m_pBoundDepthStencil)
        return;

    if (m_IsDefaultFBOBound)
    {
        GLuint DefaultFBOHandle = m_pSwapChain->GetDefaultFBO();
        if (m_DefaultFBO != DefaultFBOHandle)
        {
            m_DefaultFBO = GLObjectWrappers::GLFrameBufferObj{true, GLObjectWrappers::GLFBOCreateReleaseHelper(DefaultFBOHandle)};
        }
        m_ContextState.BindFBO(m_DefaultFBO);
    }
    else
    {
        VERIFY(m_NumBoundRenderTargets != 0 || m_pBoundDepthStencil, "At least one render target or a depth stencil is expected");

        Uint32 NumRenderTargets = m_NumBoundRenderTargets;
        VERIFY(NumRenderTargets < MAX_RENDER_TARGETS, "Too many render targets (", NumRenderTargets, ") are being set");
        NumRenderTargets = std::min(NumRenderTargets, MAX_RENDER_TARGETS);

        const auto& CtxCaps = m_ContextState.GetContextCaps();
        VERIFY(NumRenderTargets < static_cast<Uint32>(CtxCaps.MaxDrawBuffers), "This device only supports ", CtxCaps.MaxDrawBuffers, " draw buffers, but ", NumRenderTargets, " are being set");
        NumRenderTargets = std::min(NumRenderTargets, static_cast<Uint32>(CtxCaps.MaxDrawBuffers));

        TextureViewGLImpl* pBoundRTVs[MAX_RENDER_TARGETS] = {};
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            pBoundRTVs[rt] = m_pBoundRenderTargets[rt];
            DEV_CHECK_ERR(!pBoundRTVs[rt] || pBoundRTVs[rt]->GetTexture<TextureBaseGL>()->GetGLHandle(),
                          "Color buffer of the default framebuffer can only be bound with the default framebuffer's depth buffer "
                          "and cannot be combined with any other render target or depth buffer in OpenGL backend.");
        }

        DEV_CHECK_ERR(!m_pBoundDepthStencil || m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle(),
                      "Depth buffer of the default framebuffer can only be bound with the default framebuffer's color buffer "
                      "and cannot be combined with any other render target in OpenGL backend.");

        auto        CurrentNativeGLContext = m_ContextState.GetCurrentGLContext();
        auto&       FBOCache               = m_pDevice->GetFBOCache(CurrentNativeGLContext);
        const auto& FBO                    = FBOCache.GetFBO(NumRenderTargets, pBoundRTVs, m_pBoundDepthStencil, m_ContextState);
        // Even though the write mask only applies to writes to a framebuffer, the mask state is NOT
        // Framebuffer state. So it is NOT part of a Framebuffer Object or the Default Framebuffer.
        // Binding a new framebuffer will NOT affect the mask.
        m_ContextState.BindFBO(FBO);
    }
    // Set the viewport to match the render target size
    SetViewports(1, nullptr, 0, 0);
}

void DeviceContextGLImpl::SetRenderTargetsExt(const SetRenderTargetsAttribs& Attribs)
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Calling SetRenderTargets inside active render pass is invalid. End the render pass first");

    if (TDeviceContextBase::SetRenderTargets(Attribs))
    {
        if (m_NumBoundRenderTargets == 1 && m_pBoundRenderTargets[0] && m_pBoundRenderTargets[0]->GetTexture<TextureBaseGL>()->GetGLHandle() == 0)
        {
            DEV_CHECK_ERR(!m_pBoundDepthStencil || m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle() == 0,
                          "Attempting to bind texture '", m_pBoundDepthStencil->GetTexture()->GetDesc().Name,
                          "' as depth buffer with the default framebuffer's color buffer: color buffer of the default framebuffer "
                          "can only be bound with the default framebuffer's depth buffer and cannot be combined with any other depth buffer in OpenGL backend.");
            m_IsDefaultFBOBound = true;
        }
        else if (m_NumBoundRenderTargets == 0 && m_pBoundDepthStencil && m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle() == 0)
        {
            m_IsDefaultFBOBound = true;
        }
        else
        {
            m_IsDefaultFBOBound = false;
        }

        CommitRenderTargets();
    }
}

void DeviceContextGLImpl::ResetRenderTargets()
{
    TDeviceContextBase::ResetRenderTargets();
    m_IsDefaultFBOBound = false;
    m_ContextState.InvalidateFBO();
}

void DeviceContextGLImpl::BeginSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& SubpassDesc = RPDesc.pSubpasses[m_SubpassIndex];
    const auto& FBDesc      = m_pBoundFramebuffer->GetDesc();

    const auto& RenderTargetFBO = m_pBoundFramebuffer->GetSubpassFramebuffer(m_SubpassIndex).RenderTarget;
    if (RenderTargetFBO != 0)
    {
        m_ContextState.BindFBO(RenderTargetFBO);
    }
    else
    {
        GLuint DefaultFBOHandle = m_pSwapChain->GetDefaultFBO();
        if (m_DefaultFBO != DefaultFBOHandle)
        {
            m_DefaultFBO = GLObjectWrappers::GLFrameBufferObj{true, GLObjectWrappers::GLFBOCreateReleaseHelper(DefaultFBOHandle)};
        }
        m_ContextState.BindFBO(m_DefaultFBO);
    }

    for (Uint32 rt = 0; rt < SubpassDesc.RenderTargetAttachmentCount; ++rt)
    {
        const auto& RTAttachmentRef = SubpassDesc.pRenderTargetAttachments[rt];
        if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            auto* const pRTV = ClassPtrCast<TextureViewGLImpl>(FBDesc.ppAttachments[RTAttachmentRef.AttachmentIndex]);
            if (pRTV == nullptr)
                continue;

            auto* const pColorTexGL = pRTV->GetTexture<TextureBaseGL>();
            pColorTexGL->TextureMemoryBarrier(
                MEMORY_BARRIER_FRAMEBUFFER, // Reads and writes via framebuffer object attachments after the
                                            // barrier will reflect data written by shaders prior to the barrier.
                                            // Additionally, framebuffer writes issued after the barrier will wait
                                            // on the completion of all shader writes issued prior to the barrier.
                m_ContextState);

            const auto& AttachmentDesc = RPDesc.pAttachments[RTAttachmentRef.AttachmentIndex];
            auto        FirstLastUse   = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTAttachmentRef.AttachmentIndex);
            if (FirstLastUse.first == m_SubpassIndex && AttachmentDesc.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
            {
                ClearRenderTarget(pRTV, m_AttachmentClearValues[RTAttachmentRef.AttachmentIndex].Color, RESOURCE_STATE_TRANSITION_MODE_NONE);
            }
        }
    }

    if (SubpassDesc.pDepthStencilAttachment != nullptr)
    {
        const auto DepthAttachmentIndex = SubpassDesc.pDepthStencilAttachment->AttachmentIndex;
        if (DepthAttachmentIndex != ATTACHMENT_UNUSED)
        {
            auto* const pDSV = ClassPtrCast<TextureViewGLImpl>(FBDesc.ppAttachments[DepthAttachmentIndex]);
            if (pDSV != nullptr)
            {
                auto* pDepthTexGL = pDSV->GetTexture<TextureBaseGL>();
                pDepthTexGL->TextureMemoryBarrier(MEMORY_BARRIER_FRAMEBUFFER, m_ContextState);

                const auto& AttachmentDesc = RPDesc.pAttachments[DepthAttachmentIndex];
                auto        FirstLastUse   = m_pActiveRenderPass->GetAttachmentFirstLastUse(DepthAttachmentIndex);
                if (FirstLastUse.first == m_SubpassIndex && AttachmentDesc.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
                {
                    const auto& FmtAttribs = GetTextureFormatAttribs(AttachmentDesc.Format);

                    auto ClearFlags = CLEAR_DEPTH_FLAG;
                    if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
                        ClearFlags |= CLEAR_STENCIL_FLAG;

                    const auto& ClearVal = m_AttachmentClearValues[DepthAttachmentIndex].DepthStencil;
                    ClearDepthStencil(pDSV, ClearFlags, ClearVal.Depth, ClearVal.Stencil, RESOURCE_STATE_TRANSITION_MODE_NONE);
                }
            }
        }
    }
}

void DeviceContextGLImpl::EndSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& SubpassDesc = RPDesc.pSubpasses[m_SubpassIndex];

    const auto& SubpassFBOs = m_pBoundFramebuffer->GetSubpassFramebuffer(m_SubpassIndex);
#ifdef DILIGENT_DEBUG
    {
        GLint glCurrReadFB = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &glCurrReadFB);
        CHECK_GL_ERROR("Failed to get current read framebuffer");
        GLuint glExpectedReadFB = SubpassFBOs.RenderTarget != 0 ? static_cast<GLuint>(SubpassFBOs.RenderTarget) : m_pSwapChain->GetDefaultFBO();
        VERIFY(static_cast<GLuint>(glCurrReadFB) == glExpectedReadFB, "Unexpected read framebuffer");
    }
#endif

    if (SubpassDesc.pResolveAttachments != nullptr)
    {
        GLuint ResolveDstFBO = SubpassFBOs.Resolve;
        if (ResolveDstFBO == 0)
        {
            ResolveDstFBO = m_pSwapChain.RawPtr<ISwapChainGL>()->GetDefaultFBO();
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ResolveDstFBO);
        DEV_CHECK_GL_ERROR("Failed to bind resolve destination FBO as draw framebuffer");

        const auto& FBODesc = m_pBoundFramebuffer->GetDesc();
        glBlitFramebuffer(0, 0, static_cast<GLint>(FBODesc.Width), static_cast<GLint>(FBODesc.Height),
                          0, 0, static_cast<GLint>(FBODesc.Width), static_cast<GLint>(FBODesc.Height),
                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST // Filter is ignored
        );
        DEV_CHECK_GL_ERROR("glBlitFramebuffer() failed when resolving multi-sampled attachments");
    }

    if (glInvalidateFramebuffer != nullptr)
    {
        // It is crucially important to invalidate the framebuffer while it is bound
        // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/mali-performance-2-how-to-correctly-handle-framebuffers
        GLsizei InvalidateAttachmentsCount = 0;

        std::array<GLenum, MAX_RENDER_TARGETS + 1> InvalidateAttachments;
        for (Uint32 rt = 0; rt < SubpassDesc.RenderTargetAttachmentCount; ++rt)
        {
            const auto RTAttachmentIdx = SubpassDesc.pRenderTargetAttachments[rt].AttachmentIndex;
            if (RTAttachmentIdx != ATTACHMENT_UNUSED)
            {
                auto AttachmentLastUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTAttachmentIdx).second;
                if (AttachmentLastUse == m_SubpassIndex && RPDesc.pAttachments[RTAttachmentIdx].StoreOp == ATTACHMENT_STORE_OP_DISCARD)
                {
                    if (SubpassFBOs.RenderTarget == 0)
                    {
                        VERIFY(rt == 0, "Default framebuffer can only have single color attachment");
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_COLOR;
                    }
                    else
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_COLOR_ATTACHMENT0 + rt;
                    }
                }
            }
        }

        if (SubpassDesc.pDepthStencilAttachment != nullptr)
        {
            const auto DSAttachmentIdx = SubpassDesc.pDepthStencilAttachment->AttachmentIndex;
            if (DSAttachmentIdx != ATTACHMENT_UNUSED)
            {
                auto AttachmentLastUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(DSAttachmentIdx).second;
                if (AttachmentLastUse == m_SubpassIndex && RPDesc.pAttachments[DSAttachmentIdx].StoreOp == ATTACHMENT_STORE_OP_DISCARD)
                {
                    const auto& FmtAttribs = GetTextureFormatAttribs(RPDesc.pAttachments[DSAttachmentIdx].Format);
                    VERIFY_EXPR(FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH || FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL);
                    if (SubpassFBOs.RenderTarget == 0)
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_DEPTH;
                        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
                            InvalidateAttachments[InvalidateAttachmentsCount++] = GL_STENCIL;
                    }
                    else
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
                    }
                }
            }
        }

        if (InvalidateAttachmentsCount > 0)
        {
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, InvalidateAttachmentsCount, InvalidateAttachments.data());
            DEV_CHECK_GL_ERROR("glInvalidateFramebuffer() failed");
        }
    }

    // TODO: invalidate input attachments using glInvalidateTexImage

    m_ContextState.InvalidateFBO();
}

void DeviceContextGLImpl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);

    m_AttachmentClearValues.resize(Attribs.ClearValueCount);
    for (Uint32 i = 0; i < Attribs.ClearValueCount; ++i)
        m_AttachmentClearValues[i] = Attribs.pClearValues[i];

    VERIFY_EXPR(m_pBoundFramebuffer);

    SetViewports(1, nullptr, 0, 0);

    BeginSubpass();
}

void DeviceContextGLImpl::NextSubpass()
{
    EndSubpass();
    TDeviceContextBase::NextSubpass();
    BeginSubpass();
    m_AttachmentClearValues.clear();
}

void DeviceContextGLImpl::EndRenderPass()
{
    EndSubpass();
    TDeviceContextBase::EndRenderPass();
    m_ContextState.InvalidateFBO();
}

#ifdef DILIGENT_DEVELOPMENT
void DeviceContextGLImpl::DvpValidateCommittedShaderResources()
{
    if (m_BindInfo.ResourcesValidated)
        return;

    DvpVerifySRBCompatibility(m_BindInfo);

    m_pPipelineState->DvpVerifySRBResources(m_BindInfo.ResourceCaches, m_BindInfo.BaseBindings);
    m_BindInfo.ResourcesValidated = true;
}
#endif

void DeviceContextGLImpl::BindProgramResources(Uint32 BindSRBMask)
{
    VERIFY_EXPR(BindSRBMask != 0);
    //if (m_CommittedResourcesTentativeBarriers != 0)
    //    LOG_INFO_MESSAGE("Not all tentative resource barriers have been executed since the last call to CommitShaderResources(). Did you forget to call Draw()/DispatchCompute() ?");

    VERIFY_EXPR(m_BoundWritableTextures.empty());
    VERIFY_EXPR(m_BoundWritableBuffers.empty());

    m_CommittedResourcesTentativeBarriers = MEMORY_BARRIER_NONE;

    while (BindSRBMask != 0)
    {
        auto SignBit = ExtractLSB(BindSRBMask);
        auto sign    = PlatformMisc::GetLSB(SignBit);
        VERIFY_EXPR(sign < m_pPipelineState->GetResourceSignatureCount());
        const auto& BaseBindings = m_pPipelineState->GetBaseBindings(sign);
#ifdef DILIGENT_DEVELOPMENT
        m_BindInfo.BaseBindings[sign] = BaseBindings;
#endif

        const auto* pResourceCache = m_BindInfo.ResourceCaches[sign];
        DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at index ", sign, " is null");
        if (m_BindInfo.StaleSRBMask & SignBit)
            pResourceCache->BindResources(GetContextState(), BaseBindings, m_BoundWritableTextures, m_BoundWritableBuffers);
        else
        {
            VERIFY((m_BindInfo.DynamicSRBMask & SignBit) != 0,
                   "When bit in StaleSRBMask is not set, the same bit in DynamicSRBMask must be set. Check GetCommitMask().");
            DEV_CHECK_ERR(pResourceCache->HasDynamicResources(),
                          "Bit in DynamicSRBMask is set, but the cache does not contain dynamic resources. This may indicate that resources "
                          "in the cache have changed, but the SRB has not been committed before the draw/dispatch command.");
            pResourceCache->BindDynamicBuffers(GetContextState(), BaseBindings);
        }
    }
    m_BindInfo.StaleSRBMask &= ~m_BindInfo.ActiveSRBMask;


#if GL_ARB_shader_image_load_store
    // Go through the list of textures bound as AUVs and set the required memory barriers
    for (auto* pWritableTex : m_BoundWritableTextures)
    {
        constexpr MEMORY_BARRIER TextureMemBarriers = MEMORY_BARRIER_ALL_TEXTURE_BARRIERS;

        m_CommittedResourcesTentativeBarriers |= TextureMemBarriers;

        // Set new required barriers for the time when texture is used next time
        pWritableTex->SetPendingMemoryBarriers(TextureMemBarriers);
    }
    m_BoundWritableTextures.clear();

    for (auto* pWritableBuff : m_BoundWritableBuffers)
    {
        constexpr MEMORY_BARRIER BufferMemoryBarriers = MEMORY_BARRIER_ALL_BUFFER_BARRIERS;

        m_CommittedResourcesTentativeBarriers |= BufferMemoryBarriers;
        // Set new required barriers for the time when buffer is used next time
        pWritableBuff->SetPendingMemoryBarriers(BufferMemoryBarriers);
    }
    m_BoundWritableBuffers.clear();
#endif
}

void DeviceContextGLImpl::PrepareForDraw(DRAW_FLAGS Flags, bool IsIndexed, GLenum& GlTopology)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();
#endif

    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (ShaderResourcesGL needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);
    if (Uint32 BindSRBMask = m_BindInfo.GetCommitMask(Flags & DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT))
    {
        BindProgramResources(BindSRBMask);
    }

#ifdef DILIGENT_DEVELOPMENT
    // Must be called after BindProgramResources as it needs BaseBindings
    DvpValidateCommittedShaderResources();
#endif

    const auto  CurrNativeGLContext = m_pDevice->m_GLContext.GetCurrentNativeGLContext();
    const auto& PipelineDesc        = m_pPipelineState->GetGraphicsPipelineDesc();
    if (!m_ContextState.IsValidVAOBound())
    {
        auto& VaoCache     = m_pDevice->GetVAOCache(CurrNativeGLContext);
        auto* pIndexBuffer = IsIndexed ? m_pIndexBuffer.RawPtr() : nullptr;
        if (PipelineDesc.InputLayout.NumElements > 0 || pIndexBuffer != nullptr)
        {
            VAOCache::VAOAttribs vaoAttribs //
                {
                    *m_pPipelineState,
                    pIndexBuffer,
                    m_VertexStreams,
                    m_NumVertexStreams //
                };
            const auto& VAO = VaoCache.GetVAO(vaoAttribs, m_ContextState);
            m_ContextState.BindVAO(VAO);
        }
        else
        {
            // Draw command will fail if no VAO is bound. If no vertex description is set
            // (which is the case if, for instance, the command only inputs VertexID),
            // use empty VAO
            const auto& VAO = VaoCache.GetEmptyVAO();
            m_ContextState.BindVAO(VAO);
        }
    }

    auto Topology = PipelineDesc.PrimitiveTopology;
    if (Topology >= PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
    {
#if GL_ARB_tessellation_shader
        GlTopology       = GL_PATCHES;
        auto NumVertices = static_cast<Int32>(Topology - PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1);
        m_ContextState.SetNumPatchVertices(NumVertices);
#else
        UNSUPPORTED("Tessellation is not supported");
#endif
    }
    else
    {
        GlTopology = PrimitiveTopologyToGLTopology(Topology);
    }
}

void DeviceContextGLImpl::PrepareForIndexedDraw(VALUE_TYPE IndexType, Uint32 FirstIndexLocation, GLenum& GLIndexType, size_t& FirstIndexByteOffset)
{
    GLIndexType = TypeToGLType(IndexType);
    VERIFY(GLIndexType == GL_UNSIGNED_BYTE || GLIndexType == GL_UNSIGNED_SHORT || GLIndexType == GL_UNSIGNED_INT,
           "Unsupported index type");
    VERIFY(m_pIndexBuffer, "Index Buffer is not bound to the pipeline");
    FirstIndexByteOffset = StaticCast<size_t>(GetValueSize(IndexType) * Uint64{FirstIndexLocation} + m_IndexDataStartOffset);
}

void DeviceContextGLImpl::PostDraw()
{
    // IMPORTANT: new pending memory barriers in the context must be set
    // after all previous barriers have been executed.
    // m_CommittedResourcesTentativeBarriers contains memory barriers that will be required
    // AFTER the actual draw/dispatch command is executed.
    m_ContextState.SetPendingMemoryBarriers(m_CommittedResourcesTentativeBarriers);
    m_CommittedResourcesTentativeBarriers = MEMORY_BARRIER_NONE;
}

void DeviceContextGLImpl::Draw(const DrawAttribs& Attribs)
{
    DvpVerifyDrawArguments(Attribs);

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, false, GlTopology);

    if (Attribs.NumVertices > 0 && Attribs.NumInstances > 0)
    {
        if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
        {
            if (Attribs.FirstInstanceLocation != 0)
                glDrawArraysInstancedBaseInstance(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices, Attribs.NumInstances, Attribs.FirstInstanceLocation);
            else
                glDrawArraysInstanced(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices, Attribs.NumInstances);
        }
        else
        {
            glDrawArrays(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices);
        }
        DEV_CHECK_GL_ERROR("OpenGL draw command failed");
    }

    PostDraw();
}

void DeviceContextGLImpl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    DvpVerifyDrawIndexedArguments(Attribs);

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);
    GLenum GLIndexType;
    size_t FirstIndexByteOffset;
    PrepareForIndexedDraw(Attribs.IndexType, Attribs.FirstIndexLocation, GLIndexType, FirstIndexByteOffset);

    // NOTE: Base Vertex and Base Instance versions are not supported even in OpenGL ES 3.1
    // This functionality can be emulated by adjusting stream offsets. This, however may cause
    // errors in case instance data is read from the same stream as vertex data. Thus handling
    // such cases is left to the application

    if (Attribs.NumIndices > 0 && Attribs.NumInstances > 0)
    {
        if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
        {
            if (Attribs.BaseVertex > 0)
            {
                if (Attribs.FirstInstanceLocation != 0)
                    glDrawElementsInstancedBaseVertexBaseInstance(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
                else
                    glDrawElementsInstancedBaseVertex(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.BaseVertex);
            }
            else
            {
                if (Attribs.FirstInstanceLocation != 0)
                    glDrawElementsInstancedBaseInstance(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.FirstInstanceLocation);
                else
                    glDrawElementsInstanced(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances);
            }
        }
        else
        {
            if (Attribs.BaseVertex > 0)
                glDrawElementsBaseVertex(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.BaseVertex);
            else
                glDrawElements(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)));
        }
        DEV_CHECK_GL_ERROR("OpenGL draw command failed");
    }

    PostDraw();
}

void DeviceContextGLImpl::PrepareForIndirectDraw(IBuffer* pAttribsBuffer)
{
#if GL_ARB_draw_indirect
    auto* pIndirectDrawAttribsGL = ClassPtrCast<BufferGLImpl>(pAttribsBuffer);
    // The indirect rendering functions take their data from the buffer currently bound to the
    // GL_DRAW_INDIRECT_BUFFER binding. Thus, any of indirect draw functions will fail if no buffer is
    // bound to that binding.
    pIndirectDrawAttribsGL->BufferMemoryBarrier(
        MEMORY_BARRIER_INDIRECT_BUFFER, // Command data sourced from buffer objects by
                                        // Draw*Indirect and DispatchComputeIndirect commands after the barrier
                                        // will reflect data written by shaders prior to the barrier.The buffer
                                        // objects affected by this bit are derived from the DRAW_INDIRECT_BUFFER
                                        // and DISPATCH_INDIRECT_BUFFER bindings.
        m_ContextState);
    constexpr bool ResetVAO = false; // GL_DRAW_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectDrawAttribsGL->m_GlBuffer, ResetVAO);
#endif
}

void DeviceContextGLImpl::PrepareForIndirectDrawCount(IBuffer* pCountBuffer)
{
#if GL_ARB_indirect_parameters
    auto* pCountBufferGL = ClassPtrCast<BufferGLImpl>(pCountBuffer);
    // The indirect rendering functions take their data from the buffer currently bound to the
    // GL_DRAW_INDIRECT_BUFFER binding. Thus, any of indirect draw functions will fail if no buffer is
    // bound to that binding.
    pCountBufferGL->BufferMemoryBarrier(
        MEMORY_BARRIER_INDIRECT_BUFFER, // Command data sourced from buffer objects by
                                        // Draw*Indirect and DispatchComputeIndirect commands after the barrier
                                        // will reflect data written by shaders prior to the barrier.The buffer
                                        // objects affected by this bit are derived from the DRAW_INDIRECT_BUFFER
                                        // and DISPATCH_INDIRECT_BUFFER bindings.
        m_ContextState);
    constexpr bool ResetVAO = false; // GL_DRAW_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_PARAMETER_BUFFER, pCountBufferGL->m_GlBuffer, ResetVAO);
#endif
}

void DeviceContextGLImpl::DrawIndirect(const DrawIndirectAttribs& Attribs)
{
    DvpVerifyDrawIndirectArguments(Attribs);

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);

    // http://www.opengl.org/wiki/Vertex_Rendering
    PrepareForIndirectDraw(Attribs.pAttribsBuffer);

    if (Attribs.pCounterBuffer == nullptr)
    {
        bool NativeMultiDrawExecuted = false;
        if (Attribs.DrawCount > 1)
        {
#if GL_ARB_multi_draw_indirect
            if ((m_pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_NATIVE_MULTI_DRAW_INDIRECT) != 0)
            {
                glMultiDrawArraysIndirect(GlTopology,
                                          reinterpret_cast<const void*>(StaticCast<size_t>(Attribs.DrawArgsOffset)),
                                          Attribs.DrawCount,
                                          Attribs.DrawArgsStride);
                DEV_CHECK_GL_ERROR("glMultiDrawArraysIndirect() failed");
                NativeMultiDrawExecuted = true;
            }
#endif
        }

        if (!NativeMultiDrawExecuted)
        {
#if GL_ARB_draw_indirect
            for (Uint32 draw = 0; draw < Attribs.DrawCount; ++draw)
            {
                auto Offset = Attribs.DrawArgsOffset + draw * Uint64{Attribs.DrawArgsStride};
                //typedef  struct {
                //   GLuint  count;
                //   GLuint  instanceCount;
                //   GLuint  first;
                //   GLuint  baseInstance;
                //} DrawArraysIndirectCommand;
                glDrawArraysIndirect(GlTopology, reinterpret_cast<const void*>(StaticCast<size_t>(Offset)));
                // Note that on GLES 3.1, baseInstance is present but reserved and must be zero
                DEV_CHECK_GL_ERROR("glDrawArraysIndirect() failed");
            }
#else
            LOG_ERROR_MESSAGE("Indirect rendering is not supported");
#endif
        }
    }
    else
    {
        PrepareForIndirectDrawCount(Attribs.pCounterBuffer);

#if GL_VERSION_4_6
        glMultiDrawArraysIndirectCount(GlTopology,
                                       reinterpret_cast<const void*>(StaticCast<size_t>(Attribs.DrawArgsOffset)),
                                       StaticCast<GLintptr>(Attribs.CounterOffset),
                                       Attribs.DrawCount,
                                       Attribs.DrawArgsStride);
        DEV_CHECK_GL_ERROR("glMultiDrawArraysIndirectCount() failed");

        constexpr bool ResetVAO = false; // GL_PARAMETER_BUFFER does not affect VAO
        m_ContextState.BindBuffer(GL_PARAMETER_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);
#else
        LOG_ERROR_MESSAGE("Multi indirect count rendering is not supported");
#endif
    }

    constexpr bool ResetVAO = false; // GL_DRAW_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
}

void DeviceContextGLImpl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs)
{
    DvpVerifyDrawIndexedIndirectArguments(Attribs);

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);
    GLenum GLIndexType;
    size_t FirstIndexByteOffset;
    PrepareForIndexedDraw(Attribs.IndexType, 0, GLIndexType, FirstIndexByteOffset);
    DEV_CHECK_ERR(FirstIndexByteOffset == 0, "Index buffer offset is not supported for DrawIndexedIndirect() in OpenGL");

    // http://www.opengl.org/wiki/Vertex_Rendering
    PrepareForIndirectDraw(Attribs.pAttribsBuffer);

    if (Attribs.pCounterBuffer == nullptr)
    {
        bool NativeMultiDrawExecuted = false;
        if (Attribs.DrawCount > 1)
        {
#if GL_ARB_multi_draw_indirect
            if ((m_pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_NATIVE_MULTI_DRAW_INDIRECT) != 0)
            {
                glMultiDrawElementsIndirect(GlTopology,
                                            GLIndexType,
                                            reinterpret_cast<const void*>(StaticCast<size_t>(Attribs.DrawArgsOffset)),
                                            Attribs.DrawCount,
                                            Attribs.DrawArgsStride);
                DEV_CHECK_GL_ERROR("glMultiDrawElementsIndirect() failed");
                NativeMultiDrawExecuted = true;
            }
#endif
        }

        if (!NativeMultiDrawExecuted)
        {
#if GL_ARB_draw_indirect
            for (Uint32 draw = 0; draw < Attribs.DrawCount; ++draw)
            {
                auto Offset = Attribs.DrawArgsOffset + draw * Uint64{Attribs.DrawArgsStride};
                //typedef  struct {
                //    GLuint  count;
                //    GLuint  instanceCount;
                //    GLuint  firstIndex;
                //    GLuint  baseVertex;
                //    GLuint  baseInstance;
                //} DrawElementsIndirectCommand;
                glDrawElementsIndirect(GlTopology, GLIndexType, reinterpret_cast<const void*>(StaticCast<size_t>(Offset)));
                // Note that on GLES 3.1, baseInstance is present but reserved and must be zero
                DEV_CHECK_GL_ERROR("glDrawElementsIndirect() failed");
            }
#else
            LOG_ERROR_MESSAGE("Indirect rendering is not supported");
#endif
        }
    }
    else
    {
        PrepareForIndirectDrawCount(Attribs.pCounterBuffer);

#if GL_VERSION_4_6
        glMultiDrawElementsIndirectCount(GlTopology,
                                         GLIndexType,
                                         reinterpret_cast<const void*>(StaticCast<size_t>(Attribs.DrawArgsOffset)),
                                         StaticCast<GLintptr>(Attribs.CounterOffset),
                                         Attribs.DrawCount,
                                         Attribs.DrawArgsStride);
        DEV_CHECK_GL_ERROR("glMultiDrawElementsIndirectCount() failed");

        constexpr bool ResetVAO = false; // GL_PARAMETER_BUFFER does not affect VAO
        m_ContextState.BindBuffer(GL_PARAMETER_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);
#else
        LOG_ERROR_MESSAGE("Multi indirect count rendering is not supported");
#endif
    }

    constexpr bool ResetVAO = false; // GL_DISPATCH_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
}

void DeviceContextGLImpl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    UNSUPPORTED("DrawMesh is not supported in OpenGL");
}

void DeviceContextGLImpl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs)
{
    UNSUPPORTED("DrawMeshIndirect is not supported in OpenGL");
}


void DeviceContextGLImpl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    DvpVerifyDispatchArguments(Attribs);

#if GL_ARB_compute_shader
    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (ShaderResourcesGL needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);
    if (Uint32 BindSRBMask = m_BindInfo.GetCommitMask())
    {
        BindProgramResources(BindSRBMask);
    }

#    ifdef DILIGENT_DEVELOPMENT
    // Must be called after BindProgramResources as it needs BaseBindings
    DvpValidateCommittedShaderResources();
#    endif

    if (Attribs.ThreadGroupCountX > 0 && Attribs.ThreadGroupCountY > 0 && Attribs.ThreadGroupCountZ > 0)
    {
        glDispatchCompute(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
        DEV_CHECK_GL_ERROR("glDispatchCompute() failed");
    }

    PostDraw();
#else
    UNSUPPORTED("Compute shaders are not supported");
#endif
}

void DeviceContextGLImpl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs)
{
    DvpVerifyDispatchIndirectArguments(Attribs);

#if GL_ARB_compute_shader
    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (ShaderResourcesGL needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);
    if (Uint32 BindSRBMask = m_BindInfo.GetCommitMask())
    {
        BindProgramResources(BindSRBMask);
    }

#    ifdef DILIGENT_DEVELOPMENT
    // Must be called after BindProgramResources as it needs BaseBindings
    DvpValidateCommittedShaderResources();
#    endif

    auto* pBufferGL = ClassPtrCast<BufferGLImpl>(Attribs.pAttribsBuffer);
    pBufferGL->BufferMemoryBarrier(
        MEMORY_BARRIER_INDIRECT_BUFFER, // Command data sourced from buffer objects by
                                        // Draw*Indirect and DispatchComputeIndirect commands after the barrier
                                        // will reflect data written by shaders prior to the barrier.The buffer
                                        // objects affected by this bit are derived from the DRAW_INDIRECT_BUFFER
                                        // and DISPATCH_INDIRECT_BUFFER bindings.
        m_ContextState);

    constexpr bool ResetVAO = false; // GL_DISPATCH_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, pBufferGL->m_GlBuffer, ResetVAO);
    DEV_CHECK_GL_ERROR("Failed to bind a buffer for dispatch indirect command");

    glDispatchComputeIndirect(StaticCast<GLintptr>(Attribs.DispatchArgsByteOffset));
    DEV_CHECK_GL_ERROR("glDispatchComputeIndirect() failed");

    m_ContextState.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
#else
    UNSUPPORTED("Compute shaders are not supported");
#endif
}

void DeviceContextGLImpl::ClearDepthStencil(ITextureView*                  pView,
                                            CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                            float                          fDepth,
                                            Uint8                          Stencil,
                                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::ClearDepthStencil(pView);

    if (pView != m_pBoundDepthStencil)
    {
        LOG_ERROR_MESSAGE("Depth stencil buffer must be bound to the context to be cleared in OpenGL backend");
        return;
    }

    Uint32 glClearFlags = 0;
    if (ClearFlags & CLEAR_DEPTH_FLAG) glClearFlags |= GL_DEPTH_BUFFER_BIT;
    if (ClearFlags & CLEAR_STENCIL_FLAG) glClearFlags |= GL_STENCIL_BUFFER_BIT;
    glClearDepthf(fDepth);
    glClearStencil(Stencil);
    // If depth writes are disabled, glClear() will not clear depth buffer!
    bool DepthWritesEnabled = m_ContextState.GetDepthWritesEnabled();
    m_ContextState.EnableDepthWrites(True);

    // Set stencil write mask
    Uint8 StencilWriteMask = m_ContextState.GetStencilWriteMask();
    m_ContextState.SetStencilWriteMask(0xFF);

    // Unlike OpenGL, in D3D10+, the full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.

    bool ScissorTestEnabled = m_ContextState.GetScissorTestEnabled();
    m_ContextState.EnableScissorTest(False);
    // The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect
    // the operation of glClear. The scissor box bounds the cleared region. Alpha function,
    // blend function, logical operation, stenciling, texture mapping, and depth-buffering
    // are ignored by glClear.
    glClear(glClearFlags);
    DEV_CHECK_GL_ERROR("glClear() failed");
    m_ContextState.SetStencilWriteMask(StencilWriteMask);
    m_ContextState.EnableDepthWrites(DepthWritesEnabled);
    m_ContextState.EnableScissorTest(ScissorTestEnabled);
}

void DeviceContextGLImpl::ClearRenderTarget(ITextureView* pView, const float* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::ClearRenderTarget(pView);

    Int32 RTIndex = -1;
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (m_pBoundRenderTargets[rt] == pView)
        {
            RTIndex = rt;
            break;
        }
    }

    if (RTIndex == -1)
    {
        LOG_ERROR_MESSAGE("Render target must be bound to the context to be cleared in OpenGL backend");
        return;
    }

    static const float Zero[4] = {0, 0, 0, 0};
    if (RGBA == nullptr)
        RGBA = Zero;

    // The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect
    // the operation of glClear. The scissor box bounds the cleared region. Alpha function,
    // blend function, logical operation, stenciling, texture mapping, and depth-buffering
    // are ignored by glClear.

    // Unlike OpenGL, in D3D10+, the full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.

    // Disable scissor test
    bool ScissorTestEnabled = m_ContextState.GetScissorTestEnabled();
    m_ContextState.EnableScissorTest(False);

    // Set write mask
    Uint32 WriteMask         = 0;
    Bool   bIndependentBlend = False;
    m_ContextState.GetColorWriteMask(RTIndex, WriteMask, bIndependentBlend);
    m_ContextState.SetColorWriteMask(RTIndex, COLOR_MASK_ALL, bIndependentBlend);

    glClearBufferfv(GL_COLOR, RTIndex, RGBA);
    DEV_CHECK_GL_ERROR("glClearBufferfv() failed");

    m_ContextState.SetColorWriteMask(RTIndex, WriteMask, bIndependentBlend);
    m_ContextState.EnableScissorTest(ScissorTestEnabled);
}

void DeviceContextGLImpl::Flush()
{
    DEV_CHECK_ERR(m_pActiveRenderPass == nullptr, "Flushing device context inside an active render pass.");

    glFlush();

    m_BindInfo = {};
}

void DeviceContextGLImpl::FinishFrame()
{
    TDeviceContextBase::EndFrame();
}

void DeviceContextGLImpl::FinishCommandList(ICommandList** ppCommandList)
{
    LOG_ERROR("Deferred contexts are not supported in OpenGL mode");
}

void DeviceContextGLImpl::ExecuteCommandLists(Uint32               NumCommandLists,
                                              ICommandList* const* ppCommandLists)
{
    LOG_ERROR("Deferred contexts are not supported in OpenGL mode");
}

void DeviceContextGLImpl::EnqueueSignal(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::EnqueueSignal(pFence, Value, 0);

    GLObjectWrappers::GLSyncObj GLFence{glFenceSync(
        GL_SYNC_GPU_COMMANDS_COMPLETE, // Condition must always be GL_SYNC_GPU_COMMANDS_COMPLETE
        0                              // Flags, must be 0
        )};
    DEV_CHECK_GL_ERROR("Failed to create gl fence");
    auto* pFenceGLImpl = ClassPtrCast<FenceGLImpl>(pFence);
    pFenceGLImpl->AddPendingFence(std::move(GLFence), Value);
}

void DeviceContextGLImpl::DeviceWaitForFence(IFence* pFence, Uint64 Value)
{
    TDeviceContextBase::DeviceWaitForFence(pFence, Value, 0);

    auto* pFenceGLImpl = ClassPtrCast<FenceGLImpl>(pFence);
    pFenceGLImpl->DeviceWait(Value);
    pFenceGLImpl->DvpDeviceWait(Value);
}

void DeviceContextGLImpl::WaitForIdle()
{
    DEV_CHECK_ERR(!IsDeferred(), "Only immediate contexts can be idled");
    Flush();
    glFinish();
}

void DeviceContextGLImpl::BeginQuery(IQuery* pQuery)
{
    TDeviceContextBase::BeginQuery(pQuery, 0);

    auto* pQueryGLImpl = ClassPtrCast<QueryGLImpl>(pQuery);
    auto  QueryType    = pQueryGLImpl->GetDesc().Type;
    auto  glQuery      = pQueryGLImpl->GetGlQueryHandle();

    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
#if GL_SAMPLES_PASSED
            glBeginQuery(GL_SAMPLES_PASSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_SAMPLES_PASSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("GL_SAMPLES_PASSED query is not supported by this device");
#endif
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            glBeginQuery(GL_ANY_SAMPLES_PASSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_ANY_SAMPLES_PASSED query");
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
#if GL_PRIMITIVES_GENERATED
            glBeginQuery(GL_PRIMITIVES_GENERATED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_PRIMITIVES_GENERATED query");
#else
            LOG_ERROR_MESSAGE_ONCE("GL_PRIMITIVES_GENERATED query is not supported by this device");
#endif
            break;

        case QUERY_TYPE_DURATION:
#if GL_TIME_ELAPSED
            glBeginQuery(GL_TIME_ELAPSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_TIME_ELAPSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("Duration queries are not supported by this device");
#endif
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
}

void DeviceContextGLImpl::EndQuery(IQuery* pQuery)
{
    TDeviceContextBase::EndQuery(pQuery, 0);

    auto* pQueryGLImpl = ClassPtrCast<QueryGLImpl>(pQuery);
    auto  QueryType    = pQueryGLImpl->GetDesc().Type;
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
#if GL_SAMPLES_PASSED
            glEndQuery(GL_SAMPLES_PASSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_SAMPLES_PASSED query");
#endif
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            glEndQuery(GL_ANY_SAMPLES_PASSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_ANY_SAMPLES_PASSED query");
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
#if GL_PRIMITIVES_GENERATED
            glEndQuery(GL_PRIMITIVES_GENERATED);
            DEV_CHECK_GL_ERROR("Failed to end GL_PRIMITIVES_GENERATED query");
#endif
            break;

        case QUERY_TYPE_TIMESTAMP:
#if GL_TIMESTAMP
            if (glQueryCounter != nullptr)
            {
                glQueryCounter(pQueryGLImpl->GetGlQueryHandle(), GL_TIMESTAMP);
                DEV_CHECK_GL_ERROR("glQueryCounter failed");
            }
            else
#endif
            {
                LOG_ERROR_MESSAGE_ONCE("Timer queries are not supported by this device");
            }
            break;

        case QUERY_TYPE_DURATION:
#if GL_TIME_ELAPSED
            glEndQuery(GL_TIME_ELAPSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_TIME_ELAPSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("Duration queries are not supported by this device");
#endif
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
}

bool DeviceContextGLImpl::UpdateCurrentGLContext()
{
    auto NativeGLContext = m_pDevice->m_GLContext.GetCurrentNativeGLContext();
    if (NativeGLContext == NULL)
        return false;

    m_ContextState.SetCurrentGLContext(NativeGLContext);
    return true;
}

void DeviceContextGLImpl::UpdateBuffer(IBuffer*                       pBuffer,
                                       Uint64                         Offset,
                                       Uint64                         Size,
                                       const void*                    pData,
                                       RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    auto* pBufferGL = ClassPtrCast<BufferGLImpl>(pBuffer);
    pBufferGL->UpdateData(m_ContextState, Offset, Size, pData);
}

void DeviceContextGLImpl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                     Uint64                         SrcOffset,
                                     RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                     IBuffer*                       pDstBuffer,
                                     Uint64                         DstOffset,
                                     Uint64                         Size,
                                     RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBufferGL = ClassPtrCast<BufferGLImpl>(pSrcBuffer);
    auto* pDstBufferGL = ClassPtrCast<BufferGLImpl>(pDstBuffer);
    pDstBufferGL->CopyData(m_ContextState, *pSrcBufferGL, SrcOffset, DstOffset, Size);
}

void DeviceContextGLImpl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);
    auto* pBufferGL = ClassPtrCast<BufferGLImpl>(pBuffer);
    pBufferGL->Map(m_ContextState, MapType, MapFlags, pMappedData);
}

void DeviceContextGLImpl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto* pBufferGL = ClassPtrCast<BufferGLImpl>(pBuffer);
    pBufferGL->Unmap(m_ContextState);
}

void DeviceContextGLImpl::UpdateTexture(ITexture*                      pTexture,
                                        Uint32                         MipLevel,
                                        Uint32                         Slice,
                                        const Box&                     DstBox,
                                        const TextureSubResData&       SubresData,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                        RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferStateTransitionMode, TextureStateTransitionMode);
    auto* pTexGL = ClassPtrCast<TextureBaseGL>(pTexture);
    pTexGL->UpdateData(m_ContextState, MipLevel, Slice, DstBox, SubresData);
}

void DeviceContextGLImpl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);
    auto* pSrcTexGL = ClassPtrCast<TextureBaseGL>(CopyAttribs.pSrcTexture);
    auto* pDstTexGL = ClassPtrCast<TextureBaseGL>(CopyAttribs.pDstTexture);

    const auto& SrcTexDesc = pSrcTexGL->GetDesc();
    const auto& DstTexDesc = pDstTexGL->GetDesc();

    auto SrcMipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);

    Box FullSrcBox;
    FullSrcBox.MaxX = SrcMipLevelAttribs.LogicalWidth;
    FullSrcBox.MaxY = SrcMipLevelAttribs.LogicalHeight;
    FullSrcBox.MaxZ = SrcMipLevelAttribs.Depth;
    auto* pSrcBox   = CopyAttribs.pSrcBox != nullptr ? CopyAttribs.pSrcBox : &FullSrcBox;

    if (SrcTexDesc.Usage == USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING)
    {
        TextureSubResData SubresData;
        SubresData.pData      = nullptr;
        SubresData.pSrcBuffer = pSrcTexGL->GetPBO();
        SubresData.SrcOffset =
            GetStagingTextureLocationOffset(SrcTexDesc, CopyAttribs.SrcSlice, CopyAttribs.SrcMipLevel,
                                            TextureBaseGL::PBOOffsetAlignment,
                                            pSrcBox->MinX, pSrcBox->MinY, pSrcBox->MinZ);
        SubresData.Stride      = SrcMipLevelAttribs.RowSize;
        SubresData.DepthStride = SrcMipLevelAttribs.DepthSliceSize;

        Box DstBox;
        DstBox.MinX = CopyAttribs.DstX;
        DstBox.MinY = CopyAttribs.DstY;
        DstBox.MinZ = CopyAttribs.DstZ;
        DstBox.MaxX = DstBox.MinX + pSrcBox->Width();
        DstBox.MaxY = DstBox.MinY + pSrcBox->Height();
        DstBox.MaxZ = DstBox.MinZ + pSrcBox->Depth();
        pDstTexGL->UpdateData(m_ContextState, CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, DstBox, SubresData);
    }
    else if (SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage == USAGE_STAGING)
    {
        if (pSrcTexGL->GetGLTextureHandle() == 0)
        {
            auto*  pSwapChainGL     = m_pSwapChain.RawPtr<ISwapChainGL>();
            GLuint DefaultFBOHandle = pSwapChainGL->GetDefaultFBO();
            glBindFramebuffer(GL_READ_FRAMEBUFFER, DefaultFBOHandle);
            DEV_CHECK_GL_ERROR("Failed to bind default FBO as read framebuffer");
        }
        else
        {
            const auto& FmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
            DEV_CHECK_ERR(FmtAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED,
                          "Reading pixels from compressed-format textures to pixel pack buffer is not supported");

            TextureViewDesc SrcTexViewDesc;
            SrcTexViewDesc.Format = SrcTexDesc.Format;
            SrcTexViewDesc.ViewType =
                (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH || FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL) ?
                TEXTURE_VIEW_DEPTH_STENCIL :
                TEXTURE_VIEW_RENDER_TARGET;
            SrcTexViewDesc.MostDetailedMip = CopyAttribs.SrcMipLevel;
            SrcTexViewDesc.FirstArraySlice = CopyAttribs.SrcSlice;
            SrcTexViewDesc.NumArraySlices  = 1;
            SrcTexViewDesc.NumMipLevels    = 1;
            TextureViewGLImpl SrcTexView //
                {
                    nullptr, // pRefCounters
                    m_pDevice,
                    SrcTexViewDesc,
                    pSrcTexGL,
                    false, // bCreateGLViewTex
                    false  // bIsDefaultView
                };

            auto  CurrNativeGLCtx = m_ContextState.GetCurrentGLContext();
            auto& fboCache        = m_pDevice->GetFBOCache(CurrNativeGLCtx);

            TextureViewGLImpl* pSrcViews[] = {&SrcTexView};

            const auto& SrcFBO =
                (SrcTexViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET) ?
                fboCache.GetFBO(1, pSrcViews, nullptr, m_ContextState) :
                fboCache.GetFBO(0, nullptr, pSrcViews[0], m_ContextState);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcFBO);
            DEV_CHECK_GL_ERROR("Failed to bind FBO as read framebuffer");
        }

        auto* pDstBuffer = ClassPtrCast<BufferGLImpl>(pDstTexGL->GetPBO());
        VERIFY(pDstBuffer != nullptr, "Internal staging buffer must not be null");
        // GetStagingTextureLocationOffset assumes pixels are tightly packed in every subresource - no padding
        // except between subresources.
        const auto DstOffset =
            GetStagingTextureLocationOffset(DstTexDesc, CopyAttribs.DstSlice, CopyAttribs.DstMipLevel,
                                            TextureBaseGL::PBOOffsetAlignment,
                                            CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ);

        m_ContextState.BindBuffer(GL_PIXEL_PACK_BUFFER, pDstBuffer->GetGLHandle(), true);

        const auto& TransferAttribs = GetNativePixelTransferAttribs(SrcTexDesc.Format);
        glReadPixels(pSrcBox->MinX, pSrcBox->MinY, pSrcBox->Width(), pSrcBox->Height(),
                     TransferAttribs.PixelFormat, TransferAttribs.DataType, reinterpret_cast<void*>(StaticCast<size_t>(DstOffset)));
        DEV_CHECK_GL_ERROR("Failed to read pixel from framebuffer to pixel pack buffer");

        m_ContextState.BindBuffer(GL_PIXEL_PACK_BUFFER, GLObjectWrappers::GLBufferObj::Null(), true);
        // Restore original FBO
        m_ContextState.InvalidateFBO();
        CommitRenderTargets();
    }
    else
    {
        VERIFY(SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING, "Copying between staging textures is not supported");
        pDstTexGL->CopyData(this, pSrcTexGL, CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, CopyAttribs.pSrcBox,
                            CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ);
    }
}

void DeviceContextGLImpl::MapTextureSubresource(ITexture*                 pTexture,
                                                Uint32                    MipLevel,
                                                Uint32                    ArraySlice,
                                                MAP_TYPE                  MapType,
                                                MAP_FLAGS                 MapFlags,
                                                const Box*                pMapRegion,
                                                MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);
    auto*       pTexGL  = ClassPtrCast<TextureBaseGL>(pTexture);
    const auto& TexDesc = pTexGL->GetDesc();
    if (TexDesc.Usage == USAGE_STAGING)
    {
        auto PBOOffset       = GetStagingTextureSubresourceOffset(TexDesc, ArraySlice, MipLevel, TextureBaseGL::PBOOffsetAlignment);
        auto MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        auto pPBO            = ClassPtrCast<BufferGLImpl>(pTexGL->GetPBO());
        pPBO->MapRange(m_ContextState, MapType, MapFlags, PBOOffset, MipLevelAttribs.MipSize, MappedData.pData);

        MappedData.Stride      = MipLevelAttribs.RowSize;
        MappedData.DepthStride = MipLevelAttribs.MipSize;
    }
    else
    {
        LOG_ERROR_MESSAGE("Only staging textures can be mapped in OpenGL");
        MappedData = MappedTextureSubresource{};
    }
}


void DeviceContextGLImpl::UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);
    auto*       pTexGL  = ClassPtrCast<TextureBaseGL>(pTexture);
    const auto& TexDesc = pTexGL->GetDesc();
    if (TexDesc.Usage == USAGE_STAGING)
    {
        auto pPBO = ClassPtrCast<BufferGLImpl>(pTexGL->GetPBO());
        pPBO->Unmap(m_ContextState);
    }
    else
    {
        LOG_ERROR_MESSAGE("Only staging textures can be mapped in OpenGL");
    }
}

void DeviceContextGLImpl::GenerateMips(ITextureView* pTexView)
{
    TDeviceContextBase::GenerateMips(pTexView);
    auto* pTexViewGL = ClassPtrCast<TextureViewGLImpl>(pTexView);
    auto  BindTarget = pTexViewGL->GetBindTarget();
    m_ContextState.BindTexture(-1, BindTarget, pTexViewGL->GetHandle());
    glGenerateMipmap(BindTarget);
    DEV_CHECK_GL_ERROR("Failed to generate mip maps");
    m_ContextState.BindTexture(-1, BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

void DeviceContextGLImpl::TransitionResourceStates(Uint32 BarrierCount, const StateTransitionDesc* pResourceBarriers)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
}

void DeviceContextGLImpl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                    ITexture*                               pDstTexture,
                                                    const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);
    auto*       pSrcTexGl  = ClassPtrCast<TextureBaseGL>(pSrcTexture);
    auto*       pDstTexGl  = ClassPtrCast<TextureBaseGL>(pDstTexture);
    const auto& SrcTexDesc = pSrcTexGl->GetDesc();
    //const auto& DstTexDesc = pDstTexGl->GetDesc();

    auto  CurrentNativeGLContext = m_ContextState.GetCurrentGLContext();
    auto& FBOCache               = m_pDevice->GetFBOCache(CurrentNativeGLContext);

    GLuint SrcFBOHandle = 0;
    {
        TextureViewDesc SrcTexViewDesc;
        SrcTexViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
        SrcTexViewDesc.MostDetailedMip = ResolveAttribs.SrcMipLevel;
        SrcTexViewDesc.FirstArraySlice = ResolveAttribs.SrcSlice;
        TextureViewGLImpl SrcTexView //
            {
                nullptr, // pRefCounters
                m_pDevice,
                SrcTexViewDesc,
                pSrcTexGl,
                false, // bCreateGLViewTex
                false  // bIsDefaultView
            };

        TextureViewGLImpl* pSrcViews[] = {&SrcTexView};
        const auto&        SrcFBO      = FBOCache.GetFBO(1, pSrcViews, nullptr, m_ContextState);

        SrcFBOHandle = SrcFBO;
    }

    if (pDstTexGl->GetGLHandle())
    {
        TextureViewDesc DstTexViewDesc;
        DstTexViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
        DstTexViewDesc.MostDetailedMip = ResolveAttribs.DstMipLevel;
        DstTexViewDesc.FirstArraySlice = ResolveAttribs.DstSlice;
        TextureViewGLImpl DstTexView //
            {
                nullptr, // pRefCounters
                m_pDevice,
                DstTexViewDesc,
                pDstTexGl,
                false, // bCreateGLViewTex
                false  // bIsDefaultView
            };

        TextureViewGLImpl* pDstViews[] = {&DstTexView};
        const auto&        DstFBO      = FBOCache.GetFBO(1, pDstViews, nullptr, m_ContextState);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DstFBO);
        DEV_CHECK_GL_ERROR("Failed to bind FBO as draw framebuffer");
    }
    else
    {
        auto*  pSwapChainGL     = m_pSwapChain.RawPtr<ISwapChainGL>();
        GLuint DefaultFBOHandle = pSwapChainGL->GetDefaultFBO();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DefaultFBOHandle);
        DEV_CHECK_GL_ERROR("Failed to bind default FBO as draw framebuffer");
    }

    // NB: FBOCache.GetFBO() overwrites framebuffer bindings if it needs to create a new one
    glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcFBOHandle);
    DEV_CHECK_GL_ERROR("Failed to bind FBO as read framebuffer");

    const auto& MipAttribs = GetMipLevelProperties(SrcTexDesc, ResolveAttribs.SrcMipLevel);
    glBlitFramebuffer(0, 0, static_cast<GLint>(MipAttribs.LogicalWidth), static_cast<GLint>(MipAttribs.LogicalHeight),
                      0, 0, static_cast<GLint>(MipAttribs.LogicalWidth), static_cast<GLint>(MipAttribs.LogicalHeight),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST // Filter is ignored
    );
    DEV_CHECK_GL_ERROR("glBlitFramebuffer() failed when resolving multi-sampled texture");

    // Restore original FBO
    m_ContextState.InvalidateFBO();
    CommitRenderTargets();
}

void DeviceContextGLImpl::BuildBLAS(const BuildBLASAttribs& Attribs)
{
    UNSUPPORTED("BuildBLAS is not supported in OpenGL");
}

void DeviceContextGLImpl::BuildTLAS(const BuildTLASAttribs& Attribs)
{
    UNSUPPORTED("BuildTLAS is not supported in OpenGL");
}

void DeviceContextGLImpl::CopyBLAS(const CopyBLASAttribs& Attribs)
{
    UNSUPPORTED("CopyBLAS is not supported in OpenGL");
}

void DeviceContextGLImpl::CopyTLAS(const CopyTLASAttribs& Attribs)
{
    UNSUPPORTED("CopyTLAS is not supported in OpenGL");
}

void DeviceContextGLImpl::WriteBLASCompactedSize(const WriteBLASCompactedSizeAttribs& Attribs)
{
    UNSUPPORTED("WriteBLASCompactedSize is not supported in OpenGL");
}

void DeviceContextGLImpl::WriteTLASCompactedSize(const WriteTLASCompactedSizeAttribs& Attribs)
{
    UNSUPPORTED("WriteTLASCompactedSize is not supported in OpenGL");
}

void DeviceContextGLImpl::TraceRays(const TraceRaysAttribs& Attribs)
{
    UNSUPPORTED("TraceRays is not supported in OpenGL");
}

void DeviceContextGLImpl::TraceRaysIndirect(const TraceRaysIndirectAttribs& Attribs)
{
    UNSUPPORTED("TraceRaysIndirect is not supported in OpenGL");
}

void DeviceContextGLImpl::UpdateSBT(IShaderBindingTable* pSBT, const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs)
{
    UNSUPPORTED("UpdateSBT is not supported in OpenGL");
}

void DeviceContextGLImpl::SetShadingRate(SHADING_RATE BaseRate, SHADING_RATE_COMBINER PrimitiveCombiner, SHADING_RATE_COMBINER TextureCombiner)
{
    UNSUPPORTED("SetShadingRate is not supported in OpenGL");
}

void DeviceContextGLImpl::BindSparseResourceMemory(const BindSparseResourceMemoryAttribs& Attribs)
{
    UNSUPPORTED("BindSparseResourceMemory is not supported in OpenGL");
}

void DeviceContextGLImpl::BeginDebugGroup(const Char* Name, const float* pColor)
{
    TDeviceContextBase::BeginDebugGroup(Name, pColor, 0);

#if GL_KHR_debug
    if (glPushDebugGroup)
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, Name);
#endif
}

void DeviceContextGLImpl::EndDebugGroup()
{
    TDeviceContextBase::EndDebugGroup(0);

#if GL_KHR_debug
    if (glPopDebugGroup)
        glPopDebugGroup();
#endif
}

void DeviceContextGLImpl::InsertDebugLabel(const Char* Label, const float* pColor)
{
    TDeviceContextBase::InsertDebugLabel(Label, pColor, 0);

#if GL_KHR_debug
    if (glDebugMessageInsert)
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_MEDIUM, -1, Label);
#endif
}

} // namespace Diligent
