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

#include "FBOCache.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "GLContextState.hpp"
#include "GLTypeConversions.hpp"

namespace Diligent
{

bool FBOCache::FBOCacheKey::operator==(const FBOCacheKey& Key) const noexcept
{
    if (Hash != 0 && Key.Hash != 0 && Hash != Key.Hash)
        return false;

    if (NumRenderTargets != Key.NumRenderTargets || Width != Key.Width || Height != Key.Height)
        return false;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        if (RTIds[rt] != Key.RTIds[rt])
            return false;
        if (RTIds[rt])
        {
            if (!(RTVDescs[rt] == Key.RTVDescs[rt]))
                return false;
        }
    }
    if (DSId != Key.DSId)
        return false;
    if (DSId)
    {
        if (!(DSVDesc == Key.DSVDesc))
            return false;
    }
    return true;
}

std::size_t FBOCache::FBOCacheKeyHashFunc::operator()(const FBOCacheKey& Key) const noexcept
{
    if (Key.Hash == 0)
    {
        Key.Hash = 0;
        HashCombine(Key.Hash, Key.NumRenderTargets, Key.Width, Key.Height);
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
        {
            HashCombine(Key.Hash, Key.RTIds[rt]);
            if (Key.RTIds[rt])
                HashCombine(Key.Hash, Key.RTVDescs[rt]);
        }
        HashCombine(Key.Hash, Key.DSId);
        if (Key.DSId)
            HashCombine(Key.Hash, Key.DSVDesc);
    }
    return Key.Hash;
}


FBOCache::FBOCache()
{
    m_Cache.max_load_factor(0.5f);
    m_TexIdToKey.max_load_factor(0.5f);
}

FBOCache::~FBOCache()
{
#ifdef DILIGENT_DEBUG
    for (const auto& fbo_it : m_Cache)
    {
        const auto& Key = fbo_it.first;
        VERIFY(Key.NumRenderTargets == 0 && Key.DSId == 0, "Only framebuffers without attachments can be left in the cache");
    }
#endif
    VERIFY(m_TexIdToKey.empty(), "TexIdToKey cache is not empty.");
}

void FBOCache::OnReleaseTexture(ITexture* pTexture)
{
    Threading::SpinLockGuard CacheGuard{m_CacheLock};

    auto* pTexGL = ClassPtrCast<TextureBaseGL>(pTexture);
    // Find all FBOs that this texture used in
    auto EqualRange = m_TexIdToKey.equal_range(pTexGL->GetUniqueID());
    for (auto It = EqualRange.first; It != EqualRange.second; ++It)
    {
        m_Cache.erase(It->second);
    }
    m_TexIdToKey.erase(EqualRange.first, EqualRange.second);
}

void FBOCache::Clear()
{
    Threading::SpinLockGuard CacheGuard{m_CacheLock};

    m_Cache.clear();
    m_TexIdToKey.clear();
}

GLObjectWrappers::GLFrameBufferObj FBOCache::CreateFBO(GLContextState&    ContextState,
                                                       Uint32             NumRenderTargets,
                                                       TextureViewGLImpl* ppRTVs[],
                                                       TextureViewGLImpl* pDSV,
                                                       Uint32             DefaultWidth,
                                                       Uint32             DefaultHeight)
{
    GLObjectWrappers::GLFrameBufferObj FBO{true};

    ContextState.BindFBO(FBO);

    // Initialize the FBO
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        if (auto* pRTView = ppRTVs[rt])
        {
            const auto& RTVDesc     = pRTView->GetDesc();
            auto*       pColorTexGL = pRTView->GetTexture<TextureBaseGL>();
            pColorTexGL->AttachToFramebuffer(RTVDesc, GL_COLOR_ATTACHMENT0 + rt, TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_READ_DRAW);
        }
    }

    if (pDSV != nullptr)
    {
        const auto& DSVDesc         = pDSV->GetDesc();
        auto*       pDepthTexGL     = pDSV->GetTexture<TextureBaseGL>();
        GLenum      AttachmentPoint = 0;
        if (DSVDesc.Format == TEX_FORMAT_D32_FLOAT ||
            DSVDesc.Format == TEX_FORMAT_D16_UNORM)
        {
#ifdef DILIGENT_DEBUG
            {
                const auto GLTexFmt = pDepthTexGL->GetGLTexFormat();
                VERIFY(GLTexFmt == GL_DEPTH_COMPONENT32F || GLTexFmt == GL_DEPTH_COMPONENT16,
                       "Inappropriate internal texture format (", GLTexFmt,
                       ") for depth attachment. GL_DEPTH_COMPONENT32F or GL_DEPTH_COMPONENT16 is expected");
            }
#endif
            AttachmentPoint = GL_DEPTH_ATTACHMENT;
        }
        else if (DSVDesc.Format == TEX_FORMAT_D32_FLOAT_S8X24_UINT ||
                 DSVDesc.Format == TEX_FORMAT_D24_UNORM_S8_UINT)
        {
#ifdef DILIGENT_DEBUG
            {
                const auto GLTexFmt = pDepthTexGL->GetGLTexFormat();
                VERIFY(GLTexFmt == GL_DEPTH24_STENCIL8 || GLTexFmt == GL_DEPTH32F_STENCIL8,
                       "Inappropriate internal texture format (", GLTexFmt,
                       ") for depth-stencil attachment. GL_DEPTH24_STENCIL8 or GL_DEPTH32F_STENCIL8 is expected");
            }
#endif
            AttachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        else
        {
            UNEXPECTED(GetTextureFormatAttribs(DSVDesc.Format).Name, " is not valid depth-stencil view format");
        }
        VERIFY_EXPR(DSVDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL || DSVDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL);
        pDepthTexGL->AttachToFramebuffer(DSVDesc, AttachmentPoint,
                                         DSVDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL ?
                                             TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_READ_DRAW :
                                             TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_READ);
    }

    if (NumRenderTargets > 0)
    {
        // We now need to set mapping between shader outputs and
        // color attachments. This largely redundant step is performed
        // by glDrawBuffers()
        static constexpr GLenum DrawBuffers[] =
            {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
                GL_COLOR_ATTACHMENT8,
                GL_COLOR_ATTACHMENT9,
                GL_COLOR_ATTACHMENT10,
                GL_COLOR_ATTACHMENT11,
                GL_COLOR_ATTACHMENT12,
                GL_COLOR_ATTACHMENT13,
                GL_COLOR_ATTACHMENT14,
                GL_COLOR_ATTACHMENT15,
            };

        // The state set by glDrawBuffers() is part of the state of the framebuffer.
        // So it can be set up once and left it set.
        glDrawBuffers(NumRenderTargets, DrawBuffers);
        DEV_CHECK_GL_ERROR("Failed to set draw buffers via glDrawBuffers()");
    }
    else if (pDSV == nullptr)
    {
        // Framebuffer without attachments
        DEV_CHECK_ERR(DefaultWidth > 0 && DefaultHeight > 0, "Framebuffer without attachment requires non-zero default width and height");
#ifdef GL_ARB_framebuffer_no_attachments
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, DefaultWidth);
        DEV_CHECK_GL_ERROR("Failed to set framebuffer default width");

        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, DefaultHeight);
        DEV_CHECK_GL_ERROR("Failed to set framebuffer default height");

        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1);
        DEV_CHECK_GL_ERROR("Failed to set framebuffer default layer count");

        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1);
        DEV_CHECK_GL_ERROR("Failed to set framebuffer default sample count");
#else
        DEV_ERROR("Framebuffers without attachments are not supported on this platform");
#endif
    }

#ifdef DILIGENT_DEVELOPMENT
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE)
    {
        const Char* StatusString = GetFramebufferStatusString(Status);
        LOG_ERROR("Framebuffer is incomplete. FB status: ", StatusString);
        UNEXPECTED("Framebuffer is incomplete");
    }
#endif

    return FBO;
}

const GLObjectWrappers::GLFrameBufferObj& FBOCache::GetFBO(Uint32             NumRenderTargets,
                                                           TextureViewGLImpl* ppRTVs[],
                                                           TextureViewGLImpl* pDSV,
                                                           GLContextState&    ContextState)
{
    // Pop null render targets from the end of the list
    while (NumRenderTargets > 0 && ppRTVs[NumRenderTargets - 1] == nullptr)
        --NumRenderTargets;

    VERIFY(NumRenderTargets != 0 || pDSV != nullptr, "At least one render target or a depth-stencil buffer must be provided");

    // Construct the key
    FBOCacheKey Key;
    VERIFY(NumRenderTargets <= MAX_RENDER_TARGETS, "Too many render targets are being set");
    NumRenderTargets     = std::min(NumRenderTargets, MAX_RENDER_TARGETS);
    Key.NumRenderTargets = NumRenderTargets;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        auto* pRTView = ppRTVs[rt];
        if (pRTView == nullptr)
            continue;

        auto* pColorTexGL = pRTView->GetTexture<TextureBaseGL>();
        pColorTexGL->TextureMemoryBarrier(
            MEMORY_BARRIER_FRAMEBUFFER, // Reads and writes via framebuffer object attachments after the
                                        // barrier will reflect data written by shaders prior to the barrier.
                                        // Additionally, framebuffer writes issued after the barrier will wait
                                        // on the completion of all shader writes issued prior to the barrier.
            ContextState);

        Key.RTIds[rt]    = pColorTexGL->GetUniqueID();
        Key.RTVDescs[rt] = pRTView->GetDesc();
    }

    if (pDSV)
    {
        auto* pDepthTexGL = pDSV->GetTexture<TextureBaseGL>();
        pDepthTexGL->TextureMemoryBarrier(MEMORY_BARRIER_FRAMEBUFFER, ContextState);
        Key.DSId    = pDepthTexGL->GetUniqueID();
        Key.DSVDesc = pDSV->GetDesc();
    }

    // Lock the cache
    Threading::SpinLockGuard CacheGuard{m_CacheLock};

    // Try to find FBO in the map
    auto fbo_it = m_Cache.find(Key);
    if (fbo_it == m_Cache.end())
    {
        // Create a new FBO
        auto NewFBO = CreateFBO(ContextState, NumRenderTargets, ppRTVs, pDSV);

        auto it_inserted = m_Cache.emplace(Key, std::move(NewFBO));
        // New FBO must be actually inserted
        VERIFY(it_inserted.second, "New FBO was not inserted");
        if (Key.DSId != 0)
            m_TexIdToKey.emplace(Key.DSId, Key);
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            if (Key.RTIds[rt] != 0)
                m_TexIdToKey.emplace(Key.RTIds[rt], Key);
        }

        fbo_it = it_inserted.first;
    }
    return fbo_it->second;
}

const GLObjectWrappers::GLFrameBufferObj& FBOCache::GetFBO(Uint32 Width, Uint32 Height, GLContextState& ContextState)
{
    FBOCacheKey Key;
    Key.Width  = Width;
    Key.Height = Height;

    // Lock the cache
    Threading::SpinLockGuard CacheGuard{m_CacheLock};

    // Try to find FBO in the map
    auto fbo_it = m_Cache.find(Key);
    if (fbo_it == m_Cache.end())
    {
        // Create a new FBO
        auto NewFBO = CreateFBO(ContextState, 0, nullptr, nullptr, Width, Height);

        auto it_inserted = m_Cache.emplace(Key, std::move(NewFBO));
        // New FBO must be actually inserted
        VERIFY(it_inserted.second, "New FBO was not inserted");
        fbo_it = it_inserted.first;
    }
    return fbo_it->second;
}

inline GLenum GetFramebufferAttachmentPoint(TEXTURE_FORMAT Format)
{
    const auto& FmtAttribs = GetTextureFormatAttribs(Format);
    switch (FmtAttribs.ComponentType)
    {
        case COMPONENT_TYPE_DEPTH:
            return GL_DEPTH_ATTACHMENT;
        case COMPONENT_TYPE_DEPTH_STENCIL:
            return GL_DEPTH_STENCIL_ATTACHMENT;
        default:
            return GL_COLOR_ATTACHMENT0;
    }
}

const GLObjectWrappers::GLFrameBufferObj& FBOCache::GetFBO(TextureBaseGL*                          pTex,
                                                           Uint32                                  ArraySlice,
                                                           Uint32                                  MipLevel,
                                                           TextureBaseGL::FRAMEBUFFER_TARGET_FLAGS Targets)
{
    const auto& TexDesc = pTex->GetDesc();

    FBOCacheKey Key;
    Key.NumRenderTargets = 1;
    Key.RTIds[0]         = pTex->GetUniqueID();

    auto& RTV0{Key.RTVDescs[0]};
    RTV0.Format     = TexDesc.Format;
    RTV0.TextureDim = TexDesc.Type;
    RTV0.ViewType   = (Targets & TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_DRAW) ?
        TEXTURE_VIEW_RENDER_TARGET : // Also OK for depth attachments
        TEXTURE_VIEW_SHADER_RESOURCE;

    RTV0.FirstArraySlice = ArraySlice;
    RTV0.MostDetailedMip = MipLevel;
    RTV0.NumArraySlices  = 1;

    // Lock the cache
    Threading::SpinLockGuard CacheGuard{m_CacheLock};

    // Try to find FBO in the map
    auto fbo_it = m_Cache.find(Key);
    if (fbo_it == m_Cache.end())
    {
        // Create a new FBO
        GLObjectWrappers::GLFrameBufferObj NewFBO{true};

        if (Targets & TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_READ)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, NewFBO);
            DEV_CHECK_GL_ERROR("Failed to bind new FBO as read framebuffer");
        }
        if (Targets & TextureBaseGL::FRAMEBUFFER_TARGET_FLAG_DRAW)
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, NewFBO);
            DEV_CHECK_GL_ERROR("Failed to bind new FBO as draw framebuffer");
        }

        pTex->AttachToFramebuffer(RTV0, GetFramebufferAttachmentPoint(TexDesc.Format), Targets);

#ifdef DILIGENT_DEVELOPMENT
        GLenum Status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (Status != GL_FRAMEBUFFER_COMPLETE)
        {
            const Char* StatusString = GetFramebufferStatusString(Status);
            LOG_ERROR("Read framebuffer is incomplete. FB status: ", StatusString);
            UNEXPECTED("Read framebuffer is incomplete");
        }
#endif

        auto it_inserted = m_Cache.emplace(Key, std::move(NewFBO));
        // New FBO must be actually inserted
        VERIFY(it_inserted.second, "New FBO was not inserted");
        m_TexIdToKey.emplace(Key.RTIds[0], Key);

        fbo_it = it_inserted.first;
    }
    return fbo_it->second;
}

} // namespace Diligent
