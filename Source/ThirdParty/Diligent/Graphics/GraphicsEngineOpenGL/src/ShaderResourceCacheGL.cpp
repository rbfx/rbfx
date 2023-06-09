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

#include "pch.h"
#include "ShaderResourceCacheGL.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "PipelineResourceSignatureGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "PlatformMisc.hpp"

namespace Diligent
{

size_t ShaderResourceCacheGL::GetRequiredMemorySize(const TResourceCount& ResCount)
{
    static_assert(std::is_same<TResourceCount, PipelineResourceSignatureGLImpl::TBindings>::value,
                  "ShaderResourceCacheGL::TResourceCount must be the same type as PipelineResourceSignatureGLImpl::TBindings");
    // clang-format off
    auto MemSize =
                sizeof(CachedUB)           * ResCount[BINDING_RANGE_UNIFORM_BUFFER] +
                sizeof(CachedResourceView) * ResCount[BINDING_RANGE_TEXTURE]        +
                sizeof(CachedResourceView) * ResCount[BINDING_RANGE_IMAGE]          +
                sizeof(CachedSSBO)         * ResCount[BINDING_RANGE_STORAGE_BUFFER];
    // clang-format on
    VERIFY(MemSize < InvalidResourceOffset, "Memory size exceed the maximum allowed size.");
    return MemSize;
}

void ShaderResourceCacheGL::Initialize(const TResourceCount& ResCount, IMemoryAllocator& MemAllocator, Uint64 DynamicUBOSlotMask, Uint64 DynamicSSBOSlotMask)
{
    m_DynamicUBOSlotMask  = DynamicUBOSlotMask;
    m_DynamicSSBOSlotMask = DynamicSSBOSlotMask;

    VERIFY(!m_pResourceData, "Cache has already been initialized");

    // clang-format off
    m_TexturesOffset  = static_cast<Uint16>(m_UBsOffset      + sizeof(CachedUB)           * ResCount[BINDING_RANGE_UNIFORM_BUFFER]);
    m_ImagesOffset    = static_cast<Uint16>(m_TexturesOffset + sizeof(CachedResourceView) * ResCount[BINDING_RANGE_TEXTURE]);
    m_SSBOsOffset     = static_cast<Uint16>(m_ImagesOffset   + sizeof(CachedResourceView) * ResCount[BINDING_RANGE_IMAGE]);
    m_MemoryEndOffset = static_cast<Uint16>(m_SSBOsOffset    + sizeof(CachedSSBO)         * ResCount[BINDING_RANGE_STORAGE_BUFFER]);

    VERIFY_EXPR(GetUBCount()      == static_cast<Uint32>(ResCount[BINDING_RANGE_UNIFORM_BUFFER]));
    VERIFY_EXPR(GetTextureCount() == static_cast<Uint32>(ResCount[BINDING_RANGE_TEXTURE]));
    VERIFY_EXPR(GetImageCount()   == static_cast<Uint32>(ResCount[BINDING_RANGE_IMAGE]));
    VERIFY_EXPR(GetSSBOCount()    == static_cast<Uint32>(ResCount[BINDING_RANGE_STORAGE_BUFFER]));
    // clang-format on

    size_t BufferSize = m_MemoryEndOffset;

    VERIFY_EXPR(BufferSize == GetRequiredMemorySize(ResCount));

    if (BufferSize > 0)
    {
        m_pResourceData = decltype(m_pResourceData){
            ALLOCATE(MemAllocator, "Shader resource cache data buffer", Uint8, BufferSize),
            STDDeleter<Uint8, IMemoryAllocator>(MemAllocator) //
        };
        memset(m_pResourceData.get(), 0, BufferSize);
    }

    // Explicitly construct all objects
    for (Uint32 cb = 0; cb < GetUBCount(); ++cb)
        new (&GetUB(cb)) CachedUB;

    for (Uint32 s = 0; s < GetTextureCount(); ++s)
        new (&GetTexture(s)) CachedResourceView;

    for (Uint32 i = 0; i < GetImageCount(); ++i)
        new (&GetImage(i)) CachedResourceView;

    for (Uint32 s = 0; s < GetSSBOCount(); ++s)
        new (&GetSSBO(s)) CachedSSBO;
}

ShaderResourceCacheGL::~ShaderResourceCacheGL()
{
    if (IsInitialized())
    {
        for (Uint32 cb = 0; cb < GetUBCount(); ++cb)
            GetUB(cb).~CachedUB();

        for (Uint32 s = 0; s < GetTextureCount(); ++s)
            GetTexture(s).~CachedResourceView();

        for (Uint32 i = 0; i < GetImageCount(); ++i)
            GetImage(i).~CachedResourceView();

        for (Uint32 s = 0; s < GetSSBOCount(); ++s)
            GetSSBO(s).~CachedSSBO();

        m_TexturesOffset  = InvalidResourceOffset;
        m_ImagesOffset    = InvalidResourceOffset;
        m_SSBOsOffset     = InvalidResourceOffset;
        m_MemoryEndOffset = InvalidResourceOffset;
    }
    m_pResourceData.reset();
}

void ShaderResourceCacheGL::BindResources(GLContextState&              GLState,
                                          const std::array<Uint16, 4>& BaseBindings,
                                          std::vector<TextureBaseGL*>& WritableTextures,
                                          std::vector<BufferGLImpl*>&  WritableBuffers) const
{
    for (Uint32 ub = 0, binding = BaseBindings[BINDING_RANGE_UNIFORM_BUFFER]; ub < GetUBCount(); ++ub, ++binding)
    {
        const auto& UB = GetConstUB(ub);
        if (!UB.pBuffer)
            continue;

        UB.pBuffer->BufferMemoryBarrier(
            MEMORY_BARRIER_UNIFORM_BUFFER, // Shader uniforms sourced from buffer objects after the barrier
                                           // will reflect data written by shaders prior to the barrier
            GLState);

        GLState.BindUniformBuffer(binding, UB.pBuffer->GetGLHandle(), static_cast<GLintptr>(UB.BaseOffset) + static_cast<GLintptr>(UB.DynamicOffset), UB.RangeSize);
    }

    for (Uint32 s = 0, binding = BaseBindings[BINDING_RANGE_TEXTURE]; s < GetTextureCount(); ++s, ++binding)
    {
        const auto& Tex = GetConstTexture(s);
        if (!Tex.pView)
            continue;

        // We must check 'pTexture' first as 'pBuffer' is in union with 'pSampler'
        if (Tex.pTexture != nullptr)
        {
            auto* pTexViewGL = Tex.pView.RawPtr<TextureViewGLImpl>();
            auto* pTextureGL = Tex.pTexture;
            VERIFY_EXPR(pTextureGL == pTexViewGL->GetTexture());
            GLState.BindTexture(binding, pTexViewGL->GetBindTarget(), pTexViewGL->GetHandle());

            pTextureGL->TextureMemoryBarrier(
                MEMORY_BARRIER_TEXTURE_FETCH, // Texture fetches from shaders, including fetches from buffer object
                                              // memory via buffer textures, after the barrier will reflect data
                                              // written by shaders prior to the barrier
                GLState);

            if (Tex.pSampler)
            {
                GLState.BindSampler(binding, Tex.pSampler->GetHandle());
            }
            else
            {
                GLState.BindSampler(binding, GLObjectWrappers::GLSamplerObj{false});
            }
        }
        else if (Tex.pBuffer != nullptr)
        {
            auto* pBufViewGL = Tex.pView.RawPtr<BufferViewGLImpl>();
            auto* pBufferGL  = Tex.pBuffer;
            VERIFY_EXPR(pBufferGL == pBufViewGL->GetBuffer());

            GLState.BindTexture(binding, GL_TEXTURE_BUFFER, pBufViewGL->GetTexBufferHandle());
            GLState.BindSampler(binding, GLObjectWrappers::GLSamplerObj{false}); // Use default texture sampling parameters

            pBufferGL->BufferMemoryBarrier(
                MEMORY_BARRIER_TEXEL_BUFFER, // Texture fetches from shaders, including fetches from buffer object
                                             // memory via buffer textures, after the barrier will reflect data
                                             // written by shaders prior to the barrier
                GLState);
        }
    }

#if GL_ARB_shader_image_load_store
    for (Uint32 img = 0, binding = BaseBindings[BINDING_RANGE_IMAGE]; img < GetImageCount(); ++img, ++binding)
    {
        const auto& Img = GetConstImage(img);
        if (!Img.pView)
            continue;

        // We must check 'pTexture' first as 'pBuffer' is in union with 'pSampler'
        if (Img.pTexture != nullptr)
        {
            auto* pTexViewGL = Img.pView.RawPtr<TextureViewGLImpl>();
            auto* pTextureGL = Img.pTexture;
            VERIFY_EXPR(pTextureGL == pTexViewGL->GetTexture());

            const auto& ViewDesc = pTexViewGL->GetDesc();
            VERIFY(ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS, "Unexpected buffer view type");

            if (ViewDesc.AccessFlags & UAV_ACCESS_FLAG_WRITE)
            {
                pTextureGL->TextureMemoryBarrier(
                    MEMORY_BARRIER_STORAGE_IMAGE, // Memory accesses using shader image load, store, and atomic built-in
                                                  // functions issued after the barrier will reflect data written by shaders
                                                  // prior to the barrier. Additionally, image stores and atomics issued after
                                                  // the barrier will not execute until all memory accesses (e.g., loads,
                                                  // stores, texture fetches, vertex fetches) initiated prior to the barrier
                                                  // complete.
                    GLState);
                // We cannot set pending memory barriers here, because
                // if some texture is bound twice, the logic will fail
                WritableTextures.push_back(pTextureGL);
            }

#    ifdef DILIGENT_DEBUG
            // Check that the texture being bound has immutable storage
            {
                GLState.BindTexture(-1, pTexViewGL->GetBindTarget(), pTexViewGL->GetHandle());
                GLint IsImmutable = 0;
                glGetTexParameteriv(pTexViewGL->GetBindTarget(), GL_TEXTURE_IMMUTABLE_FORMAT, &IsImmutable);
                DEV_CHECK_GL_ERROR("glGetTexParameteriv() failed");
                VERIFY(IsImmutable, "Only immutable textures can be bound to pipeline using glBindImageTexture()");
                GLState.BindTexture(-1, pTexViewGL->GetBindTarget(), GLObjectWrappers::GLTextureObj::Null());
            }
#    endif
            auto GlTexFormat = TexFormatToGLInternalTexFormat(ViewDesc.Format);
            // Note that if a format qualifier is specified in the shader, the format
            // must match it

            GLboolean Layered = ViewDesc.NumArraySlices > 1 && ViewDesc.FirstArraySlice == 0;
            // If "layered" is TRUE, the entire Mip level is bound. Layer parameter is ignored in this
            // case. If "layered" is FALSE, only the single layer identified by "layer" will
            // be bound. When "layered" is FALSE, the single bound layer is treated as a 2D texture.
            GLint Layer = ViewDesc.FirstArraySlice;

            auto GLAccess = AccessFlags2GLAccess(ViewDesc.AccessFlags);
            // WARNING: Texture being bound to the image unit must be complete
            // That means that if an integer texture is being bound, its
            // GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER must be NEAREST,
            // otherwise it will be incomplete
            GLState.BindImage(binding, pTexViewGL, ViewDesc.MostDetailedMip, Layered, Layer, GLAccess, GlTexFormat);
            // Do not use binding points from reflection as they may not be initialized
        }
        else if (Img.pBuffer != nullptr)
        {
            auto* pBuffViewGL = Img.pView.RawPtr<BufferViewGLImpl>();
            auto* pBufferGL   = Img.pBuffer;
            VERIFY_EXPR(pBufferGL == pBuffViewGL->GetBuffer());

            const auto& ViewDesc = pBuffViewGL->GetDesc();
            VERIFY(ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS, "Unexpected buffer view type");

            pBufferGL->BufferMemoryBarrier(
                MEMORY_BARRIER_IMAGE_BUFFER, // Memory accesses using shader image load, store, and atomic built-in
                                             // functions issued after the barrier will reflect data written by shaders
                                             // prior to the barrier. Additionally, image stores and atomics issued after
                                             // the barrier will not execute until all memory accesses (e.g., loads,
                                             // stores, texture fetches, vertex fetches) initiated prior to the barrier
                                             // complete.
                GLState);

            WritableBuffers.push_back(pBufferGL);

            auto GlFormat = TypeToGLTexFormat(ViewDesc.Format.ValueType, ViewDesc.Format.NumComponents, ViewDesc.Format.IsNormalized);
            GLState.BindImage(binding, pBuffViewGL, GL_READ_WRITE, GlFormat);
        }
    }
#endif


#if GL_ARB_shader_storage_buffer_object
    for (Uint32 ssbo = 0, binding = BaseBindings[BINDING_RANGE_STORAGE_BUFFER]; ssbo < GetSSBOCount(); ++ssbo, ++binding)
    {
        const auto& SSBO = GetConstSSBO(ssbo);
        if (!SSBO.pBufferView)
            return;

        auto* const pBufferViewGL = SSBO.pBufferView.ConstPtr();
        const auto& ViewDesc      = pBufferViewGL->GetDesc();
        VERIFY(ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS || ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Unexpected buffer view type");

        auto* pBufferGL = pBufferViewGL->GetBuffer<BufferGLImpl>();
        pBufferGL->BufferMemoryBarrier(
            MEMORY_BARRIER_STORAGE_BUFFER, // Accesses to shader storage blocks after the barrier
                                           // will reflect writes prior to the barrier
            GLState);

        GLState.BindStorageBlock(binding,
                                 pBufferGL->GetGLHandle(),
                                 StaticCast<GLintptr>(ViewDesc.ByteOffset + SSBO.DynamicOffset),
                                 StaticCast<GLsizeiptr>(ViewDesc.ByteWidth));

        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
            WritableBuffers.push_back(pBufferGL);
    }
#endif
}

void ShaderResourceCacheGL::BindDynamicBuffers(GLContextState&              GLState,
                                               const std::array<Uint16, 4>& BaseBindings) const
{
    const auto BaseUBOBinding = BaseBindings[BINDING_RANGE_UNIFORM_BUFFER];
    for (Uint64 DynamicUBOMask = m_DynamicUBOMask; DynamicUBOMask != 0;)
    {
        const auto  UBOBit = ExtractLSB(DynamicUBOMask);
        const auto  UBOIdx = PlatformMisc::GetLSB(UBOBit);
        const auto& UB     = GetConstUB(UBOIdx);
        VERIFY_EXPR(UB.IsDynamic());
        GLState.BindUniformBuffer(BaseUBOBinding + UBOIdx, UB.pBuffer->GetGLHandle(),
                                  static_cast<GLintptr>(UB.BaseOffset) + static_cast<GLintptr>(UB.DynamicOffset),
                                  UB.RangeSize);
    }


    const auto BaseSSBOBinding = BaseBindings[BINDING_RANGE_STORAGE_BUFFER];
    for (Uint64 DynamicSSBOMask = m_DynamicSSBOMask; DynamicSSBOMask != 0;)
    {
        const auto  SSBOBit = ExtractLSB(DynamicSSBOMask);
        const auto  SSBOIdx = PlatformMisc::GetLSB(SSBOBit);
        const auto& SSBO    = GetConstSSBO(SSBOIdx);
        VERIFY_EXPR(SSBO.IsDynamic());

        auto* const pBufferViewGL = SSBO.pBufferView.ConstPtr<BufferViewGLImpl>();
        const auto* pBufferGL     = pBufferViewGL->GetBuffer<const BufferGLImpl>();
        const auto& ViewDesc      = pBufferViewGL->GetDesc();

        GLState.BindStorageBlock(BaseSSBOBinding + SSBOIdx,
                                 pBufferGL->GetGLHandle(),
                                 StaticCast<GLintptr>(ViewDesc.ByteOffset + SSBO.DynamicOffset),
                                 StaticCast<GLsizeiptr>(ViewDesc.ByteWidth));
    }
}

#ifdef DILIGENT_DEBUG
void ShaderResourceCacheGL::DbgVerifyDynamicBufferMasks() const
{
    for (Uint32 ub = 0; ub < GetUBCount(); ++ub)
    {
        const auto& UB    = GetConstUB(ub);
        const auto  UBBit = Uint64{1} << Uint64{ub};
        VERIFY(((m_DynamicUBOMask & UBBit) != 0) == (UB.IsDynamic() && (m_DynamicUBOSlotMask & UBBit) != 0), "Bit ", ub, " in m_DynamicUBOMask is invalid");
    }

    for (Uint32 ssbo = 0; ssbo < GetSSBOCount(); ++ssbo)
    {
        const auto& SSBO    = GetConstSSBO(ssbo);
        const auto  SSBOBit = Uint64{1} << Uint64{ssbo};
        VERIFY(((m_DynamicSSBOMask & SSBOBit) != 0) == (SSBO.IsDynamic() && (m_DynamicSSBOSlotMask & SSBOBit) != 0), "Bit ", ssbo, " in m_DynamicSSBOMask is invalid");
    }
}
#endif

} // namespace Diligent
