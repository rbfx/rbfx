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

#include <array>
#include <vector>

#include "BufferGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "SamplerGLImpl.hpp"
#include "ShaderResourceCacheCommon.hpp"

namespace Diligent
{

// All resources are stored in the continuous memory using the following layout:
//
//   |        Cached UBs        |     Cached Textures     |       Cached Images      | Cached Storage Blocks     |
//   |----------------------------------------------------|--------------------------|---------------------------|
//   |  0 | 1 | ... | UBCount-1 | 0 | 1 | ...| SmpCount-1 | 0 | 1 | ... | ImgCount-1 | 0 | 1 |  ... | SBOCount-1 |
//    -----------------------------------------------------------------------------------------------------------
//
class ShaderResourceCacheGL : public ShaderResourceCacheBase
{
public:
    explicit ShaderResourceCacheGL(ResourceCacheContentType ContentType) noexcept :
        m_ContentType{ContentType}
    {}

    ~ShaderResourceCacheGL();

    // clang-format off
    ShaderResourceCacheGL             (const ShaderResourceCacheGL&) = delete;
    ShaderResourceCacheGL& operator = (const ShaderResourceCacheGL&) = delete;
    ShaderResourceCacheGL             (ShaderResourceCacheGL&&)      = delete;
    ShaderResourceCacheGL& operator = (ShaderResourceCacheGL&&)      = delete;
    // clang-format on

    /// Describes a resource bound to a uniform buffer slot
    struct CachedUB
    {
        /// Strong reference to the buffer
        RefCntAutoPtr<BufferGLImpl> pBuffer;

        Uint32 BaseOffset    = 0;
        Uint32 RangeSize     = 0;
        Uint32 DynamicOffset = 0;

        // In OpenGL dynamic buffers are only those that are not bound as a whole and
        // can use a dynamic offset, irrespective of the variable type or whether the
        // buffer is USAGE_DYNAMIC or not.
        bool IsDynamic() const
        {
            return pBuffer && RangeSize < pBuffer->GetDesc().Size;
        }
    };

    /// Describes a resource bound to a sampler or an image slot
    struct CachedResourceView
    {
        /// We keep strong reference to the view instead of the reference
        /// to the texture or buffer because this is more efficient from
        /// performance point of view: this avoids one pair of
        /// AddStrongRef()/ReleaseStrongRef(). The view holds a strong reference
        /// to the texture or the buffer, so it makes no difference.
        RefCntAutoPtr<IDeviceObject> pView;

        TextureBaseGL* pTexture = nullptr;
        union
        {
            BufferGLImpl*  pBuffer = nullptr; // When pTexture == nullptr
            SamplerGLImpl* pSampler;          // When pTexture != nullptr
        };
        CachedResourceView() noexcept {}

        void Set(RefCntAutoPtr<TextureViewGLImpl>&& pTexView, bool SetSampler)
        {
            // Do not null out pSampler as it could've been initialized by PipelineResourceSignatureGLImpl::InitSRBResourceCache!
            // pSampler = nullptr;

            // Avoid unnecessary virtual call
            pTexture = pTexView ? pTexView->GetTexture<TextureBaseGL>() : nullptr;
            if (pTexView && SetSampler)
            {
                pSampler = pTexView->GetSampler<SamplerGLImpl>();
            }

            pView = std::move(pTexView);
        }

        void Set(RefCntAutoPtr<BufferViewGLImpl>&& pBufView)
        {
            pTexture = nullptr;
            // Avoid unnecessary virtual call
            pBuffer = pBufView ? pBufView->GetBuffer<BufferGLImpl>() : nullptr;
            pView   = std::move(pBufView);
        }
    };

    struct CachedSSBO
    {
        /// Strong reference to the buffer
        RefCntAutoPtr<BufferViewGLImpl> pBufferView;

        Uint32 DynamicOffset = 0;

        bool IsDynamic() const
        {
            if (pBufferView)
            {
                const auto* pBuff = pBufferView->GetBuffer<const BufferGLImpl>();
                return pBufferView->GetDesc().ByteWidth < pBuff->GetDesc().Size;
            }

            return false;
        }
    };

    using TResourceCount = std::array<Uint16, 4>; // same as PipelineResourceSignatureGLImpl::TBindings.
    static size_t GetRequiredMemorySize(const TResourceCount& ResCount);

    void Initialize(const TResourceCount& Count, IMemoryAllocator& MemAllocator, Uint64 DynamicUBOSlotMask, Uint64 DynamicSSBOSlotMask);

    void SetUniformBuffer(Uint32 CacheOffset, RefCntAutoPtr<BufferGLImpl>&& pBuff, Uint64 BaseOffset, Uint64 RangeSize)
    {
        DEV_CHECK_ERR(BaseOffset + RangeSize <= (pBuff ? pBuff->GetDesc().Size : 0), "The range is out of buffer bounds");
        if (pBuff)
        {
            if (RangeSize == 0)
                RangeSize = pBuff->GetDesc().Size - BaseOffset;
        }

        auto& UB = GetUB(CacheOffset);

        UB.pBuffer       = std::move(pBuff);
        UB.BaseOffset    = StaticCast<Uint32>(BaseOffset);
        UB.RangeSize     = StaticCast<Uint32>(RangeSize);
        UB.DynamicOffset = 0;

        Uint64 UBBit = Uint64{1} << Uint64{CacheOffset};
        if (m_DynamicUBOSlotMask & UBBit)
        {
            // Only set the flag for those slots that allow dynamic buffers
            // (i.e. the variable was not created with NO_DYNAMIC_BUFFERS flag).
            if (UB.IsDynamic())
                m_DynamicUBOMask |= UBBit;
            else
                m_DynamicUBOMask &= ~UBBit;
        }
        else
        {
            VERIFY((m_DynamicUBOMask & UBBit) == 0, "Dynamic UBO bit should never be set when corresponding bit in m_DynamicUBOSlotMask is not set");
        }
        UpdateRevision();
    }

    void SetDynamicUBOffset(Uint32 CacheOffset, Uint32 DynamicOffset)
    {
        DEV_CHECK_ERR((m_DynamicUBOSlotMask & (Uint64{1} << Uint64{CacheOffset})) != 0, "Attempting to set dynamic offset for a non-dynamic UBO slot");
        GetUB(CacheOffset).DynamicOffset = DynamicOffset;
    }

    void SetTexture(Uint32 CacheOffset, RefCntAutoPtr<TextureViewGLImpl>&& pTexView, bool SetSampler)
    {
        GetTexture(CacheOffset).Set(std::move(pTexView), SetSampler);
        UpdateRevision();
    }

    void SetSampler(Uint32 CacheOffset, ISampler* pSampler)
    {
        GetTexture(CacheOffset).pSampler = ClassPtrCast<SamplerGLImpl>(pSampler);
        UpdateRevision();
    }

    void SetTexelBuffer(Uint32 CacheOffset, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        GetTexture(CacheOffset).Set(std::move(pBuffView));
        UpdateRevision();
    }

    void SetTexImage(Uint32 CacheOffset, RefCntAutoPtr<TextureViewGLImpl>&& pTexView)
    {
        GetImage(CacheOffset).Set(std::move(pTexView), false);
        UpdateRevision();
    }

    void SetBufImage(Uint32 CacheOffset, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        GetImage(CacheOffset).Set(std::move(pBuffView));
        UpdateRevision();
    }

    void SetSSBO(Uint32 CacheOffset, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        auto& SSBO = GetSSBO(CacheOffset);

        SSBO.pBufferView   = std::move(pBuffView);
        SSBO.DynamicOffset = 0;

        Uint64 SSBOBit = Uint64{1} << Uint64{CacheOffset};
        if (m_DynamicSSBOSlotMask & SSBOBit)
        {
            // Only set the flag for those slots that allow dynamic buffers
            // (i.e. the variable was not created with NO_DYNAMIC_BUFFERS flag).
            if (SSBO.IsDynamic())
                m_DynamicSSBOMask |= SSBOBit;
            else
                m_DynamicSSBOMask &= ~SSBOBit;
        }
        else
        {
            VERIFY((m_DynamicSSBOMask & SSBOBit) == 0, "Dynamic SSBO bit should never be set when corresponding bit in m_DynamicSSBOSlotMask is not set");
        }
        UpdateRevision();
    }

    void SetDynamicSSBOOffset(Uint32 CacheOffset, Uint32 DynamicOffset)
    {
        DEV_CHECK_ERR((m_DynamicSSBOSlotMask & (Uint64{1} << Uint64{CacheOffset})) != 0, "Attempting to set dynamic offset for a non-dynamic SSBO slot");
        GetSSBO(CacheOffset).DynamicOffset = DynamicOffset;
    }


    bool IsUBBound(Uint32 CacheOffset) const
    {
        if (CacheOffset >= GetUBCount())
            return false;

        const auto& UB = GetConstUB(CacheOffset);
        return UB.pBuffer;
    }

    bool IsTextureBound(Uint32 CacheOffset, bool dbgIsTextureView) const
    {
        if (CacheOffset >= GetTextureCount())
            return false;

        const auto& Texture = GetConstTexture(CacheOffset);
        VERIFY_EXPR(dbgIsTextureView || Texture.pTexture == nullptr);
        return Texture.pView;
    }

    bool IsImageBound(Uint32 CacheOffset, bool dbgIsTextureView) const
    {
        if (CacheOffset >= GetImageCount())
            return false;

        const auto& Image = GetConstImage(CacheOffset);
        VERIFY_EXPR(dbgIsTextureView || Image.pTexture == nullptr);
        return Image.pView;
    }

    bool IsSSBOBound(Uint32 CacheOffset) const
    {
        if (CacheOffset >= GetSSBOCount())
            return false;

        const auto& SSBO = GetConstSSBO(CacheOffset);
        return SSBO.pBufferView;
    }

    // clang-format off
    Uint32 GetUBCount()      const { return (m_TexturesOffset  - m_UBsOffset)      / sizeof(CachedUB);            }
    Uint32 GetTextureCount() const { return (m_ImagesOffset    - m_TexturesOffset) / sizeof(CachedResourceView);  }
    Uint32 GetImageCount()   const { return (m_SSBOsOffset     - m_ImagesOffset)   / sizeof(CachedResourceView);  }
    Uint32 GetSSBOCount()    const { return (m_MemoryEndOffset - m_SSBOsOffset)    / sizeof(CachedSSBO);          }
    // clang-format on

    const CachedUB& GetConstUB(Uint32 CacheOffset) const
    {
        VERIFY(CacheOffset < GetUBCount(), "Uniform buffer index (", CacheOffset, ") is out of range");
        return reinterpret_cast<CachedUB*>(m_pResourceData.get() + m_UBsOffset)[CacheOffset];
    }

    const CachedResourceView& GetConstTexture(Uint32 CacheOffset) const
    {
        VERIFY(CacheOffset < GetTextureCount(), "Texture index (", CacheOffset, ") is out of range");
        return reinterpret_cast<CachedResourceView*>(m_pResourceData.get() + m_TexturesOffset)[CacheOffset];
    }

    const CachedResourceView& GetConstImage(Uint32 CacheOffset) const
    {
        VERIFY(CacheOffset < GetImageCount(), "Image buffer index (", CacheOffset, ") is out of range");
        return reinterpret_cast<CachedResourceView*>(m_pResourceData.get() + m_ImagesOffset)[CacheOffset];
    }

    const CachedSSBO& GetConstSSBO(Uint32 CacheOffset) const
    {
        VERIFY(CacheOffset < GetSSBOCount(), "Shader storage block index (", CacheOffset, ") is out of range");
        return reinterpret_cast<CachedSSBO*>(m_pResourceData.get() + m_SSBOsOffset)[CacheOffset];
    }

    bool IsInitialized() const
    {
        return m_MemoryEndOffset != InvalidResourceOffset;
    }

    ResourceCacheContentType GetContentType() const { return m_ContentType; }

#ifdef DILIGENT_DEVELOPMENT
    void SetStaticResourcesInitialized()
    {
        m_bStaticResourcesInitialized = true;
    }
    bool StaticResourcesInitialized() const { return m_bStaticResourcesInitialized; }
#endif

    // Binds all resources
    void BindResources(GLContextState&              GLState,
                       const std::array<Uint16, 4>& BaseBindings,
                       std::vector<TextureBaseGL*>& WritableTextures,
                       std::vector<BufferGLImpl*>&  WritableBuffers) const;

    // Binds uniform and storage buffers with dynamic offsets only
    void BindDynamicBuffers(GLContextState&              GLState,
                            const std::array<Uint16, 4>& BaseBindings) const;

    bool HasDynamicResources() const
    {
        return m_DynamicUBOMask != 0 || m_DynamicSSBOMask != 0;
    }

#ifdef DILIGENT_DEBUG
    void DbgVerifyDynamicBufferMasks() const;
#endif

private:
    CachedUB& GetUB(Uint32 CacheOffset)
    {
        return const_cast<CachedUB&>(const_cast<const ShaderResourceCacheGL*>(this)->GetConstUB(CacheOffset));
    }

    CachedResourceView& GetTexture(Uint32 CacheOffset)
    {
        return const_cast<CachedResourceView&>(const_cast<const ShaderResourceCacheGL*>(this)->GetConstTexture(CacheOffset));
    }

    CachedResourceView& GetImage(Uint32 CacheOffset)
    {
        return const_cast<CachedResourceView&>(const_cast<const ShaderResourceCacheGL*>(this)->GetConstImage(CacheOffset));
    }

    CachedSSBO& GetSSBO(Uint32 CacheOffset)
    {
        return const_cast<CachedSSBO&>(const_cast<const ShaderResourceCacheGL*>(this)->GetConstSSBO(CacheOffset));
    }

private:
    static constexpr const Uint16 InvalidResourceOffset = 0xFFFF;
    static constexpr const Uint16 m_UBsOffset           = 0;

    Uint16 m_TexturesOffset  = InvalidResourceOffset;
    Uint16 m_ImagesOffset    = InvalidResourceOffset;
    Uint16 m_SSBOsOffset     = InvalidResourceOffset;
    Uint16 m_MemoryEndOffset = InvalidResourceOffset;

    std::unique_ptr<Uint8, STDDeleter<Uint8, IMemoryAllocator>> m_pResourceData;

    // Indicates at which positions dynamic UBOs or SSBOs may be bound
    Uint64 m_DynamicUBOSlotMask  = 0;
    Uint64 m_DynamicSSBOSlotMask = 0;

    // Indicates slot at which dynamic buffers are actually bound
    Uint64 m_DynamicUBOMask  = 0;
    Uint64 m_DynamicSSBOMask = 0;

    // Indicates what types of resources are stored in the cache
    const ResourceCacheContentType m_ContentType;

#ifdef DILIGENT_DEVELOPMENT
    bool m_bStaticResourcesInitialized = false;
#endif
};

} // namespace Diligent
