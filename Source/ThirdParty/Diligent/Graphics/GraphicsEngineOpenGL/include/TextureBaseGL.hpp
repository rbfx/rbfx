/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "EngineGLImplTraits.hpp"
#include "TextureBase.hpp"
#include "TextureViewGLImpl.hpp" // Required by TextureBase
#include "GLObjectWrapper.hpp"
#include "AsyncWritableResource.hpp"
#include "GLContextState.hpp"

namespace Diligent
{

/// Base implementation of a texture object in OpenGL backend.
class TextureBaseGL : public TextureBase<EngineGLImplTraits>, public AsyncWritableResource
{
public:
    using TTextureBase = TextureBase<EngineGLImplTraits>;
    using ViewImplType = TextureViewGLImpl;

    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  const TextureDesc&         TexDesc,
                  GLenum                     BindTarget,
                  const TextureData*         pInitData         = nullptr,
                  bool                       bIsDeviceInternal = false);

    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  GLContextState&            GLState,
                  const TextureDesc&         TexDesc,
                  GLuint                     GLTextureHandle,
                  GLenum                     BindTarget,
                  bool                       bIsDeviceInternal);

    /// Initializes a dummy texture (dummy textures are used by the swap chain to
    /// proxy default framebuffer).
    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  const TextureDesc&         TexDesc,
                  bool                       bIsDeviceInternal);

    ~TextureBaseGL();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    const GLObjectWrappers::GLTextureObj& GetGLHandle() const { return m_GlTexture; }

    /// Implementation of ITextureGL::GetBindTarget().
    virtual GLenum DILIGENT_CALL_TYPE GetBindTarget() const override final { return m_BindTarget; }

    GLenum GetGLTexFormat() const { return m_GLTexFormat; }

    __forceinline void TextureMemoryBarrier(MEMORY_BARRIER RequiredBarriers, class GLContextState& GLContextState);

    enum FRAMEBUFFER_TARGET_FLAGS : Uint32
    {
        FRAMEBUFFER_TARGET_FLAG_NONE      = 0u,
        FRAMEBUFFER_TARGET_FLAG_READ      = 1u << 0u,
        FRAMEBUFFER_TARGET_FLAG_DRAW      = 1u << 1u,
        FRAMEBUFFER_TARGET_FLAG_READ_DRAW = FRAMEBUFFER_TARGET_FLAG_READ | FRAMEBUFFER_TARGET_FLAG_DRAW
    };
    virtual void AttachToFramebuffer(const struct TextureViewDesc& ViewDesc, GLenum AttachmentPoint, FRAMEBUFFER_TARGET_FLAGS Targets) = 0;

    void CopyData(DeviceContextGLImpl* pDeviceCtxGL,
                  TextureBaseGL*       pSrcTextureGL,
                  Uint32               SrcMipLevel,
                  Uint32               SrcSlice,
                  const Box*           pSrcBox,
                  Uint32               DstMipLevel,
                  Uint32               DstSlice,
                  Uint32               DstX,
                  Uint32               DstY,
                  Uint32               DstZ);

    /// Implementation of ITextureGL::GetGLTextureHandle().
    virtual GLuint DILIGENT_CALL_TYPE GetGLTextureHandle() const override final { return GetGLHandle(); }

    /// Implementation of ITexture::GetNativeHandle() in OpenGL backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetGLTextureHandle()); }

    virtual void UpdateData(class GLContextState&    CtxState,
                            Uint32                   MipLevel,
                            Uint32                   Slice,
                            const Box&               DstBox,
                            const TextureSubResData& SubresData) = 0;

    static constexpr Uint32 PBOOffsetAlignment = 4;

    IBuffer* GetPBO()
    {
        return m_pPBO;
    }

protected:
    virtual void CreateViewInternal(const TextureViewDesc& ViewDesc,
                                    ITextureView**         ppView,
                                    bool                   bIsDefaultView) override;

    void SetDefaultGLParameters();

    struct CopyTexSubimageAttribs
    {
        const Box& SrcBox;

        GLint DstMip   = 0;
        GLint DstLayer = 0;
        GLint DstX     = 0;
        GLint DstY     = 0;
        GLint DstZ     = 0;
    };
    virtual void CopyTexSubimage(GLContextState& GLState, const CopyTexSubimageAttribs& Attribs) = 0;

protected:
    GLObjectWrappers::GLTextureObj m_GlTexture;
    RefCntAutoPtr<IBuffer>         m_pPBO; // For staging textures
    const GLenum                   m_BindTarget;
    const GLenum                   m_GLTexFormat;
    //Uint32 m_uiMapTarget;
};

DEFINE_FLAG_ENUM_OPERATORS(TextureBaseGL::FRAMEBUFFER_TARGET_FLAGS);

void TextureBaseGL::TextureMemoryBarrier(MEMORY_BARRIER RequiredBarriers, GLContextState& GLContextState)
{
#if GL_ARB_shader_image_load_store
#    ifdef DILIGENT_DEBUG
    {
        constexpr Uint32 TextureBarriers = MEMORY_BARRIER_ALL_TEXTURE_BARRIERS;
        VERIFY((RequiredBarriers & TextureBarriers) != 0, "At least one texture memory barrier flag should be set");
        VERIFY((RequiredBarriers & ~TextureBarriers) == 0, "Inappropriate texture memory barrier flag");
    }
#    endif

    GLContextState.EnsureMemoryBarrier(RequiredBarriers, this);
#endif
}

} // namespace Diligent
