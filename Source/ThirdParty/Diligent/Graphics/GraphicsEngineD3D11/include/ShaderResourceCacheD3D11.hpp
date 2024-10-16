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

/// \file
/// Declaration of Diligent::ShaderResourceCacheD3D11 class

#include <array>
#include <memory>
#include <utility>

#include "MemoryAllocator.h"
#include "ShaderResourceCacheCommon.hpp"
#include "PipelineResourceAttribsD3D11.hpp"

#include "TextureBaseD3D11.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "BufferD3D11Impl.hpp"
#include "BufferViewD3D11Impl.hpp"
#include "SamplerD3D11Impl.hpp"

#include "Align.hpp"

namespace Diligent
{

/// The class implements a cache that holds resources bound to all shader stages.
// All resources are stored in the continuous memory using the following layout:
//
//   |         CachedCB         |      ID3D11Buffer*     ||       CachedResource     | ID3D11ShaderResourceView* ||         CachedSampler        |      ID3D11SamplerState*    ||      CachedResource     | ID3D11UnorderedAccessView*||
//   |--------------------------|------------------------||--------------------------|---------------------------||------------------------------|-----------------------------||-------------------------|---------------------------||
//   |  0 | 1 | ... | CBCount-1 | 0 | 1 | ...| CBCount-1 || 0 | 1 | ... | SRVCount-1 | 0 | 1 |  ... | SRVCount-1 || 0 | 1 | ... | SamplerCount-1 | 0 | 1 | ...| SamplerCount-1 ||0 | 1 | ... | UAVCount-1 | 0 | 1 | ...  | UAVCount-1 ||
//    --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
class ShaderResourceCacheD3D11 : public ShaderResourceCacheBase
{
public:
    explicit ShaderResourceCacheD3D11(ResourceCacheContentType ContentType) noexcept :
        m_ContentType{ContentType}
    {}

    ~ShaderResourceCacheD3D11();

    // clang-format off
    ShaderResourceCacheD3D11             (const ShaderResourceCacheD3D11&) = delete;
    ShaderResourceCacheD3D11& operator = (const ShaderResourceCacheD3D11&) = delete;
    ShaderResourceCacheD3D11             (ShaderResourceCacheD3D11&&)      = delete;
    ShaderResourceCacheD3D11& operator = (ShaderResourceCacheD3D11&&)      = delete;
    // clang-format on

    template <D3D11_RESOURCE_RANGE>
    struct CachedResourceTraits;

    /// Cached constant buffer.
    struct CachedCB
    {
        /// Strong reference to the buffer
        RefCntAutoPtr<BufferD3D11Impl> pBuff;

        // Base offset and range size in bytes
        Uint32 BaseOffset = 0;
        Uint32 RangeSize  = 0;

        // Dynamic offset in bytes
        Uint32 DynamicOffset = 0;

        explicit operator bool() const noexcept
        {
            return pBuff;
        }

        bool operator==(const CachedCB& rhs) const noexcept
        {
            // clang-format off
            return pBuff         == rhs.pBuff      &&
                   BaseOffset    == rhs.BaseOffset &&
                   RangeSize     == rhs.RangeSize  &&
                   DynamicOffset == rhs.DynamicOffset;
            // clang-format on
        }

        __forceinline void Set(RefCntAutoPtr<BufferD3D11Impl> _pBuff, Uint64 _BaseOffset, Uint64 _RangeSize)
        {
            // Buffer offset in Direct3D11 must be a multiple of 16 float4 constants (16*16 bytes), and so must the range.
            // We, however, can't align the buffer size because UpdateSubresource() in Direct3D11 must be called for the
            // whole buffer (which will not be the case if we extend the buffer size).
            // We align the range when we bind the buffer, and it is legal if it extends past the end of the buffer.
            constexpr Uint32 CBOffsetAlignment = 256;
            DEV_CHECK_ERR(_BaseOffset + _RangeSize <= (_pBuff ? _pBuff->GetDesc().Size : 0), "The range is out of buffer bounds");
            DEV_CHECK_ERR((_BaseOffset % CBOffsetAlignment) == 0, "Buffer offset must be a multiple of ", CBOffsetAlignment);

            pBuff = std::move(_pBuff);

            BaseOffset = StaticCast<Uint32>(_BaseOffset);
            RangeSize  = StaticCast<Uint32>(_RangeSize);

            if (RangeSize == 0 && pBuff)
                RangeSize = StaticCast<Uint32>(pBuff->GetDesc().Size - BaseOffset);

            DynamicOffset = 0;
        }

        IDeviceObject* Get() const
        {
            return pBuff;
        }

        // Returns true if the bound constant buffer allows setting dynamic offset,
        // i.e. the buffer is not bound as a whole (irrespective of the variable type or
        // whether the buffer is USAGE_DYNAMIC or not).
        bool AllowsDynamicOffset() const
        {
            return pBuff && RangeSize != 0 && RangeSize < pBuff->GetDesc().Size;
        }

        // Returns ID3D11Buffer
        template <D3D11_RESOURCE_RANGE ResRange>
        typename CachedResourceTraits<ResRange>::D3D11ResourceType* GetD3D11Resource();
    };

    /// Cached sampler.
    struct CachedSampler
    {
        /// Strong reference to the sampler
        RefCntAutoPtr<SamplerD3D11Impl> pSampler;

        explicit operator bool() const
        {
            return pSampler;
        }

        bool operator==(const CachedSampler& rhs) const noexcept
        {
            return pSampler == rhs.pSampler;
        }

        __forceinline void Set(SamplerD3D11Impl* pSam)
        {
            pSampler = pSam;
        }

        // Returns ID3D11SamplerState
        template <D3D11_RESOURCE_RANGE ResRange>
        typename CachedResourceTraits<ResRange>::D3D11ResourceType* GetD3D11Resource();

        IDeviceObject* Get() const
        {
            return pSampler;
        }
    };

    /// Cached SRV or a UAV
    struct CachedResource
    {
        /// We keep strong reference to the view instead of the reference
        /// to the texture or buffer because this is more efficient from
        /// performance point of view: this avoids one pair of
        /// AddStrongRef()/ReleaseStrongRef(). The view holds strong reference
        /// to the texture or the buffer, so it makes no difference.
        RefCntAutoPtr<IDeviceObject> pView;

        TextureBaseD3D11* pTexture = nullptr;
        BufferD3D11Impl*  pBuffer  = nullptr;

        // There is no need to keep strong reference to D3D11 resource as
        // it is already kept by either pTexture or pBuffer.
        ID3D11Resource* pd3d11Resource = nullptr;

        CachedResource() noexcept {}

        explicit operator bool() const
        {
            VERIFY_EXPR((pView && pd3d11Resource != nullptr) || (!pView && pd3d11Resource == nullptr));
            VERIFY_EXPR(pTexture == nullptr || pBuffer == nullptr);
            VERIFY_EXPR((pView && (pTexture != nullptr || pBuffer != nullptr)) || (!pView && (pTexture == nullptr && pBuffer == nullptr)));
            return pView;
        }

        bool operator==(const CachedResource& rhs) const noexcept
        {
            // clang-format off
            return pView          == rhs.pView    &&
                   pTexture       == rhs.pTexture &&
                   pBuffer        == rhs.pBuffer  &&
                   pd3d11Resource == rhs.pd3d11Resource;
            // clang-format on
        }

        __forceinline void Set(RefCntAutoPtr<TextureViewD3D11Impl> pTexView)
        {
            pBuffer        = nullptr;
            pTexture       = pTexView ? pTexView->GetTexture<TextureBaseD3D11>() : nullptr;
            pView          = std::move(pTexView);
            pd3d11Resource = pTexture ? pTexture->TextureBaseD3D11::GetD3D11Texture() : nullptr;
        }

        __forceinline void Set(RefCntAutoPtr<BufferViewD3D11Impl> pBufView)
        {
            pTexture       = nullptr;
            pBuffer        = pBufView ? pBufView->GetBuffer<BufferD3D11Impl>() : nullptr;
            pView          = std::move(pBufView);
            pd3d11Resource = pBuffer ? pBuffer->BufferD3D11Impl::GetD3D11Buffer() : nullptr;
        }

        IDeviceObject* Get() const
        {
            return pView;
        }

        // Returns ID3D11ShaderResourceView or ID3D11UnorderedAccessView (not pd3d11Resource!)
        template <D3D11_RESOURCE_RANGE ResRange>
        typename CachedResourceTraits<ResRange>::D3D11ResourceType* GetD3D11Resource();
    };

    static constexpr int NumShaderTypes = D3D11ResourceBindPoints::NumShaderTypes;

    static size_t GetRequiredMemorySize(const D3D11ShaderResourceCounters& ResCount);

    // pDynamicCBSlotsMask is the optional pointer to the array of dynamic constant buffer
    // slot masks for each shader stage. Dynamic constant buffer in D3D11 is a buffer that
    // is not bound as a whole.
    // Static resource cache does not allow dynamic buffers (pDynamicCBSlotsMask == null).
    void Initialize(const D3D11ShaderResourceCounters&        ResCount,
                    IMemoryAllocator&                         MemAllocator,
                    const std::array<Uint16, NumShaderTypes>* pDynamicCBSlotsMask);

    template <D3D11_RESOURCE_RANGE ResRange, typename TSrcResourceType, typename... ExtraArgsType>
    inline void SetResource(const D3D11ResourceBindPoints& BindPoints,
                            TSrcResourceType               pResource,
                            const ExtraArgsType&... ExtraArgs);

    __forceinline void SetDynamicCBOffset(const D3D11ResourceBindPoints& BindPoints, Uint32 DynamicOffset);


    template <D3D11_RESOURCE_RANGE ResRange>
    __forceinline const typename CachedResourceTraits<ResRange>::CachedResourceType& GetResource(const D3D11ResourceBindPoints& BindPoints) const
    {
        VERIFY(BindPoints.GetActiveStages() != SHADER_TYPE_UNKNOWN, "No active shader stage");
        const auto FirstStageInd     = GetFirstShaderStageIndex(BindPoints.GetActiveStages());
        const auto FirstStageBinding = BindPoints[FirstStageInd];
        VERIFY(FirstStageBinding < GetResourceCount<ResRange>(FirstStageInd), "Resource slot is out of range");
        const auto  FirstStageResArrays = GetConstResourceArrays<ResRange>(FirstStageInd);
        const auto& CachedRes           = FirstStageResArrays.first[FirstStageBinding];
#ifdef DILIGENT_DEBUG
        {
            const auto* pd3d11Res = FirstStageResArrays.second[FirstStageBinding];
            for (auto ActiveStages = BindPoints.GetActiveStages(); ActiveStages != SHADER_TYPE_UNKNOWN;)
            {
                const auto ShaderInd = ExtractFirstShaderStageIndex(ActiveStages);
                const auto ResArrays = GetConstResourceArrays<ResRange>(ShaderInd);
                VERIFY(CachedRes == ResArrays.first[BindPoints[ShaderInd]], "Cached resources are not consistent between stages. This is a bug.");
                VERIFY(pd3d11Res == ResArrays.second[BindPoints[ShaderInd]], "Cached d3d11 resources are not consistent between stages. This is a bug.");
            }
        }
#endif
        return CachedRes;
    }

    template <D3D11_RESOURCE_RANGE ResRange>
    void UpdateDynamicCBOffsetFlag(const typename CachedResourceTraits<ResRange>::CachedResourceType& CachedRes, Uint32 ShaderInd, Uint32 Binding)
    {
    }

    template <D3D11_RESOURCE_RANGE ResRange>
    bool CopyResource(const ShaderResourceCacheD3D11& SrcCache, const D3D11ResourceBindPoints& BindPoints);

    template <D3D11_RESOURCE_RANGE ResRange>
    __forceinline bool IsResourceBound(const D3D11ResourceBindPoints& BindPoints) const
    {
        if (BindPoints.IsEmpty())
            return false;

        auto       ActiveStages   = BindPoints.GetActiveStages();
        const auto FirstShaderInd = ExtractFirstShaderStageIndex(ActiveStages);
        const bool IsBound        = IsResourceBound<ResRange>(FirstShaderInd, BindPoints[FirstShaderInd]);

#ifdef DILIGENT_DEBUG
        while (ActiveStages != SHADER_TYPE_UNKNOWN)
        {
            const Uint32 ShaderInd = ExtractFirstShaderStageIndex(ActiveStages);
            VERIFY(IsBound == IsResourceBound<ResRange>(ShaderInd, BindPoints[ShaderInd]), "Bound resources are not consistent between stages. This is a bug.");
        }
#endif

        return IsBound;
    }

    // clang-format off
    __forceinline Uint32 GetCBCount     (Uint32 ShaderInd) const { return (m_Offsets[FirstCBOffsetIdx  + ShaderInd + 1] - m_Offsets[FirstCBOffsetIdx  + ShaderInd]) / (sizeof(CachedCB)       + sizeof(ID3D11Buffer*));             }
    __forceinline Uint32 GetSRVCount    (Uint32 ShaderInd) const { return (m_Offsets[FirstSRVOffsetIdx + ShaderInd + 1] - m_Offsets[FirstSRVOffsetIdx + ShaderInd]) / (sizeof(CachedResource) + sizeof(ID3D11ShaderResourceView*)); }
    __forceinline Uint32 GetSamplerCount(Uint32 ShaderInd) const { return (m_Offsets[FirstSamOffsetIdx + ShaderInd + 1] - m_Offsets[FirstSamOffsetIdx + ShaderInd]) / (sizeof(CachedSampler)  + sizeof(ID3D11SamplerState*));       }
    __forceinline Uint32 GetUAVCount    (Uint32 ShaderInd) const { return (m_Offsets[FirstUAVOffsetIdx + ShaderInd + 1] - m_Offsets[FirstUAVOffsetIdx + ShaderInd]) / (sizeof(CachedResource) + sizeof(ID3D11UnorderedAccessView*));}
    // clang-format on

    template <D3D11_RESOURCE_RANGE>
    __forceinline Uint32 GetResourceCount(Uint32 ShaderInd) const;

    bool IsInitialized() const { return m_IsInitialized; }

    ResourceCacheContentType GetContentType() const { return m_ContentType; }

    struct MinMaxSlot
    {
        UINT MinSlot = UINT_MAX;
        UINT MaxSlot = 0;

        void Add(UINT Slot)
        {
            MinSlot = std::min(MinSlot, Slot);

            VERIFY_EXPR(Slot >= MaxSlot);
            MaxSlot = Slot;
        }

        explicit operator bool() const
        {
            return MinSlot <= MaxSlot;
        }
    };

    template <D3D11_RESOURCE_RANGE Range>
    inline MinMaxSlot BindResources(Uint32                                                   ShaderInd,
                                    typename CachedResourceTraits<Range>::D3D11ResourceType* CommittedD3D11Resources[],
                                    const D3D11ShaderResourceCounters&                       BaseBindings) const;

    template <D3D11_RESOURCE_RANGE Range>
    inline MinMaxSlot BindResourceViews(Uint32                                                   ShaderInd,
                                        typename CachedResourceTraits<Range>::D3D11ResourceType* CommittedD3D11Views[],
                                        ID3D11Resource*                                          CommittedD3D11Resources[],
                                        const D3D11ShaderResourceCounters&                       BaseBindings) const;

    inline MinMaxSlot BindCBs(Uint32                             ShaderInd,
                              ID3D11Buffer*                      CommittedD3D11Resources[],
                              UINT                               FirstConstants[],
                              UINT                               NumConstants[],
                              const D3D11ShaderResourceCounters& BaseBindings) const;

    template <typename BindHandlerType>
    inline void BindDynamicCBs(Uint32                             ShaderInd,
                               ID3D11Buffer*                      CommittedD3D11Resources[],
                               UINT                               FirstConstants[],
                               UINT                               NumConstants[],
                               const D3D11ShaderResourceCounters& BaseBindings,
                               BindHandlerType&&                  BindHandler) const;

    enum class StateTransitionMode
    {
        Transition,
        Verify
    };
    // Transitions all resources in the cache
    template <StateTransitionMode Mode>
    void TransitionResourceStates(DeviceContextD3D11Impl& Ctx);

    Uint32 GetDynamicCBOffsetsMask(Uint32 ShaderInd) const
    {
        return m_DynamicCBOffsetsMask[ShaderInd];
    }

    bool HasDynamicResources() const
    {
        for (auto Mask : m_DynamicCBOffsetsMask)
        {
            if (Mask != 0)
                return true;
        }
        return false;
    }

#ifdef DILIGENT_DEBUG
    void DbgVerifyDynamicBufferMasks() const;
#endif

private:
    template <D3D11_RESOURCE_RANGE>
    __forceinline Uint32 GetResourceDataOffset(Uint32 ShaderInd) const;

    template <D3D11_RESOURCE_RANGE ResRange>
    __forceinline std::pair<typename CachedResourceTraits<ResRange>::CachedResourceType*,
                            typename CachedResourceTraits<ResRange>::D3D11ResourceType**>
    GetResourceArrays(Uint32 ShaderInd) const
    {
        using CachedResourceType = typename CachedResourceTraits<ResRange>::CachedResourceType;
        using D3D11ResourceType  = typename CachedResourceTraits<ResRange>::D3D11ResourceType;
        static_assert(alignof(CachedResourceType) == alignof(D3D11ResourceType*), "Alignment mismatch, pointer to D3D11 resource may not be properly aligned");

        const auto  DataOffset      = GetResourceDataOffset<ResRange>(ShaderInd);
        const auto  ResCount        = GetResourceCount<ResRange>(ShaderInd);
        auto* const pResources      = reinterpret_cast<CachedResourceType*>(m_pResourceData.get() + DataOffset);
        auto* const pd3d11Resources = reinterpret_cast<D3D11ResourceType**>(pResources + ResCount);
        return std::make_pair(pResources, pd3d11Resources);
    }

    template <D3D11_RESOURCE_RANGE ResRange>
    __forceinline std::pair<typename CachedResourceTraits<ResRange>::CachedResourceType const*,
                            typename CachedResourceTraits<ResRange>::D3D11ResourceType* const*>
    GetConstResourceArrays(Uint32 ShaderInd) const
    {
        const auto ResArrays = GetResourceArrays<ResRange>(ShaderInd);
        return std::make_pair(ResArrays.first, ResArrays.second);
    }

    template <D3D11_RESOURCE_RANGE ResRange>
    __forceinline bool IsResourceBound(Uint32 ShaderInd, Uint32 Offset) const
    {
        const auto ResCount = GetResourceCount<ResRange>(ShaderInd);
        VERIFY(Offset < ResCount, "Offset is out of range");
        const auto ResArrays = GetConstResourceArrays<ResRange>(ShaderInd);
        return Offset < ResCount && ResArrays.first[Offset];
    }

    // Transitions or verifies the resource state.
    template <StateTransitionMode Mode>
    void TransitionResources(DeviceContextD3D11Impl& Ctx, const ID3D11Buffer* /*Selector*/) const;

    template <StateTransitionMode Mode>
    void TransitionResources(DeviceContextD3D11Impl& Ctx, const ID3D11ShaderResourceView* /*Selector*/) const;

    template <StateTransitionMode Mode>
    void TransitionResources(DeviceContextD3D11Impl& Ctx, const ID3D11SamplerState* /*Selector*/) const;

    template <StateTransitionMode Mode>
    void TransitionResources(DeviceContextD3D11Impl& Ctx, const ID3D11UnorderedAccessView* /*Selector*/) const;

    template <D3D11_RESOURCE_RANGE RangeType>
    void ConstructResources(Uint32 ShaderInd);

    template <D3D11_RESOURCE_RANGE RangeType>
    void DestructResources(Uint32 ShaderInd);

private:
    using OffsetType = Uint16;

    static constexpr size_t MaxAlignment = std::max(std::max(std::max(alignof(CachedCB), alignof(CachedResource)), alignof(CachedSampler)), alignof(IUnknown*));

    static constexpr size_t FirstCBOffsetIdx  = 0;                                      // | VS CB  | PS CB  | GS CB  | HS CB  | DS CB  | CS CB  |
    static constexpr size_t FirstSRVOffsetIdx = FirstCBOffsetIdx + NumShaderTypes;      // | VS SRV | PS SRV | GS SRV | HS SRV | DS SRV | CS SRV |
    static constexpr size_t FirstSamOffsetIdx = FirstSRVOffsetIdx + NumShaderTypes;     // | VS Sam | PS Sam | GS Sam | HS Sam | DS Sam | CS Sam |
    static constexpr size_t FirstUAVOffsetIdx = FirstSamOffsetIdx + NumShaderTypes;     // | VS UAV | PS UAV | GS UAV | HS UAV | DS UAV | CS UAV |
    static constexpr size_t MaxOffsets        = FirstUAVOffsetIdx + NumShaderTypes + 1; // | Count  |

    std::array<OffsetType, MaxOffsets> m_Offsets = {};

    bool m_IsInitialized = false;

    // Indicates what types of resources are stored in the cache
    const ResourceCacheContentType m_ContentType;

    // Indicates which slots may contain constant buffers with dynamic offsets
    std::array<Uint16, NumShaderTypes> m_DynamicCBSlotsMask{};

    // Indicates which slots actually contain constant buffers with dynamic offsets
    std::array<Uint16, NumShaderTypes> m_DynamicCBOffsetsMask{};
    static_assert(sizeof(m_DynamicCBOffsetsMask[0]) * 8 >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, "Not enough bits for all dynamic buffer slots");

    std::unique_ptr<Uint8, STDDeleter<Uint8, IMemoryAllocator>> m_pResourceData;
};

template <>
struct ShaderResourceCacheD3D11::CachedResourceTraits<D3D11_RESOURCE_RANGE_CBV>
{
    using CachedResourceType = CachedCB;
    using D3D11ResourceType  = ID3D11Buffer;
    static const char* Name;
};

template <>
struct ShaderResourceCacheD3D11::CachedResourceTraits<D3D11_RESOURCE_RANGE_SAMPLER>
{
    using CachedResourceType = CachedSampler;
    using D3D11ResourceType  = ID3D11SamplerState;
    static const char* Name;
};

template <>
struct ShaderResourceCacheD3D11::CachedResourceTraits<D3D11_RESOURCE_RANGE_SRV>
{
    using CachedResourceType = CachedResource;
    using D3D11ResourceType  = ID3D11ShaderResourceView;
    static const char* Name;
};

template <>
struct ShaderResourceCacheD3D11::CachedResourceTraits<D3D11_RESOURCE_RANGE_UAV>
{
    using CachedResourceType = CachedResource;
    using D3D11ResourceType  = ID3D11UnorderedAccessView;
    static const char* Name;
};


template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceCount<D3D11_RESOURCE_RANGE_CBV>(Uint32 ShaderInd) const
{
    return GetCBCount(ShaderInd);
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceCount<D3D11_RESOURCE_RANGE_SRV>(Uint32 ShaderInd) const
{
    return GetSRVCount(ShaderInd);
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceCount<D3D11_RESOURCE_RANGE_UAV>(Uint32 ShaderInd) const
{
    return GetUAVCount(ShaderInd);
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceCount<D3D11_RESOURCE_RANGE_SAMPLER>(Uint32 ShaderInd) const
{
    return GetSamplerCount(ShaderInd);
}


template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceDataOffset<D3D11_RESOURCE_RANGE_CBV>(Uint32 ShaderInd) const
{
    return m_Offsets[FirstCBOffsetIdx + ShaderInd];
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceDataOffset<D3D11_RESOURCE_RANGE_SRV>(Uint32 ShaderInd) const
{
    return m_Offsets[FirstSRVOffsetIdx + ShaderInd];
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceDataOffset<D3D11_RESOURCE_RANGE_SAMPLER>(Uint32 ShaderInd) const
{
    return m_Offsets[FirstSamOffsetIdx + ShaderInd];
}

template <>
__forceinline Uint32 ShaderResourceCacheD3D11::GetResourceDataOffset<D3D11_RESOURCE_RANGE_UAV>(Uint32 ShaderInd) const
{
    return m_Offsets[FirstUAVOffsetIdx + ShaderInd];
}

template <D3D11_RESOURCE_RANGE Range>
inline ShaderResourceCacheD3D11::MinMaxSlot ShaderResourceCacheD3D11::BindResources(
    Uint32                                                   ShaderInd,
    typename CachedResourceTraits<Range>::D3D11ResourceType* CommittedD3D11Resources[],
    const D3D11ShaderResourceCounters&                       BaseBindings) const
{
    const auto   ResCount    = GetResourceCount<Range>(ShaderInd);
    const auto   ResArrays   = GetConstResourceArrays<Range>(ShaderInd);
    const Uint32 BaseBinding = BaseBindings[Range][ShaderInd];

    MinMaxSlot Slots;
    for (Uint32 res = 0; res < ResCount; ++res)
    {
        const Uint32 Slot = BaseBinding + res;
        if (CommittedD3D11Resources[Slot] != ResArrays.second[res])
            Slots.Add(Slot);

        // Note that a resource is allowed to be null if it is not used by the PSO.
        // Resources actually used by the PSO will be validated by PipelineStateD3D11Impl::DvpVerifySRBResources and
        // null resources will be reported.

        CommittedD3D11Resources[Slot] = ResArrays.second[res];
    }

    return Slots;
}


template <D3D11_RESOURCE_RANGE Range>
inline ShaderResourceCacheD3D11::MinMaxSlot ShaderResourceCacheD3D11::BindResourceViews(
    Uint32                                                   ShaderInd,
    typename CachedResourceTraits<Range>::D3D11ResourceType* CommittedD3D11Views[],
    ID3D11Resource*                                          CommittedD3D11Resources[],
    const D3D11ShaderResourceCounters&                       BaseBindings) const
{
    const auto   ResCount    = GetResourceCount<Range>(ShaderInd);
    const auto   ResArrays   = GetConstResourceArrays<Range>(ShaderInd);
    const Uint32 BaseBinding = BaseBindings[Range][ShaderInd];

    MinMaxSlot Slots;
    for (Uint32 res = 0; res < ResCount; ++res)
    {
        const Uint32 Slot = BaseBinding + res;
        if (CommittedD3D11Views[Slot] != ResArrays.second[res])
            Slots.Add(Slot);

        // Note that a resource is allowed to be null if it is not used by the PSO.
        // Resources actually used by the PSO will be validated by PipelineStateD3D11Impl::DvpVerifySRBResources and
        // null resources will be reported.

        CommittedD3D11Resources[Slot] = ResArrays.first[res].pd3d11Resource;
        CommittedD3D11Views[Slot]     = ResArrays.second[res];
    }

    return Slots;
}

inline ShaderResourceCacheD3D11::MinMaxSlot ShaderResourceCacheD3D11::BindCBs(
    Uint32                             ShaderInd,
    ID3D11Buffer*                      CommittedD3D11Resources[],
    UINT                               FirstConstants[],
    UINT                               NumConstants[],
    const D3D11ShaderResourceCounters& BaseBindings) const
{
    constexpr auto Range = D3D11_RESOURCE_RANGE_CBV;

    const auto   ResCount    = GetResourceCount<Range>(ShaderInd);
    const auto   ResArrays   = GetConstResourceArrays<Range>(ShaderInd);
    const Uint32 BaseBinding = BaseBindings[Range][ShaderInd];

    MinMaxSlot Slots;
    for (Uint32 res = 0; res < ResCount; ++res)
    {
        const Uint32 Slot     = BaseBinding + res;
        auto* const  pd3d11CB = ResArrays.second[res];
        // Offsets in Direct3D11 are measure in float4 constants.
        const auto FirstCBConstant = StaticCast<UINT>((ResArrays.first[res].BaseOffset + ResArrays.first[res].DynamicOffset) / 16u);
        // The number of constants must be a multiple of 16 constants. It is OK if it is past the end of the buffer.
        const auto NumCBConstants = StaticCast<UINT>(AlignUp(ResArrays.first[res].RangeSize / 16u, 16u));
        // clang-format off
        if (CommittedD3D11Resources[Slot] != pd3d11CB        ||
            FirstConstants[Slot]          != FirstCBConstant ||
            NumConstants[Slot]            != NumCBConstants)
        // clang-format on
        {
            Slots.Add(Slot);
        }

        // Note that a constant buffer is allowed to be null if it is not used by the PSO.
        // Resources actually used by the PSO will be validated by PipelineStateD3D11Impl::DvpVerifySRBResources and
        // null resources will be reported.

        CommittedD3D11Resources[Slot] = pd3d11CB;
        FirstConstants[Slot]          = FirstCBConstant;
        NumConstants[Slot]            = NumCBConstants;
    }

    return Slots;
}

template <typename BindHandlerType>
inline void ShaderResourceCacheD3D11::BindDynamicCBs(Uint32                             ShaderInd,
                                                     ID3D11Buffer*                      CommittedD3D11Resources[],
                                                     UINT                               FirstConstants[],
                                                     UINT                               NumConstants[],
                                                     const D3D11ShaderResourceCounters& BaseBindings,
                                                     BindHandlerType&&                  BindHandler) const
{
    constexpr auto Range = D3D11_RESOURCE_RANGE_CBV;

    const auto   ResArrays   = GetConstResourceArrays<Range>(ShaderInd);
    const Uint32 BaseBinding = BaseBindings[Range][ShaderInd];

    for (Uint32 DynamicCBMask = m_DynamicCBOffsetsMask[ShaderInd]; DynamicCBMask != 0;)
    {
        const auto CBBit   = ExtractLSB(DynamicCBMask);
        const auto Binding = PlatformMisc::GetLSB(CBBit);

        const Uint32 Slot = BaseBinding + Binding;
        const auto&  CB   = ResArrays.first[Binding];
        VERIFY_EXPR(CB.AllowsDynamicOffset() && (m_DynamicCBSlotsMask[ShaderInd] & CBBit) != 0);
        auto* const pd3d11CB = ResArrays.second[Binding];
        // Offsets in Direct3D11 are measure in float4 constants.
        const auto FirstCBConstant = StaticCast<UINT>((CB.BaseOffset + CB.DynamicOffset) / 16u);
        // The number of constants must be a multiple of 16 constants. It is OK if it is past the end of the buffer.
        const auto NumCBConstants = StaticCast<UINT>(AlignUp(CB.RangeSize / 16u, 16u));
        // clang-format off
        if (CommittedD3D11Resources[Slot] != pd3d11CB        ||
            FirstConstants[Slot]          != FirstCBConstant ||
            NumConstants[Slot]            != NumCBConstants)
        // clang-format on
        {
            // Note that a constant buffer is allowed to be null if it is not used by the PSO.
            // Resources actually used by the PSO will be validated by PipelineStateD3D11Impl::DvpVerifySRBResources and
            // null resources will be reported.

            CommittedD3D11Resources[Slot] = pd3d11CB;
            FirstConstants[Slot]          = FirstCBConstant;
            NumConstants[Slot]            = NumCBConstants;

            BindHandler(Slot);
        }
    }
}


__forceinline void ShaderResourceCacheD3D11::SetDynamicCBOffset(const D3D11ResourceBindPoints& BindPoints,
                                                                Uint32                         DynamicOffset)
{
    for (auto ActiveStages = BindPoints.GetActiveStages(); ActiveStages != SHADER_TYPE_UNKNOWN;)
    {
        const Uint32 ShaderInd = ExtractFirstShaderStageIndex(ActiveStages);
        const Uint32 Binding   = BindPoints[ShaderInd];
        VERIFY(Binding < GetResourceCount<D3D11_RESOURCE_RANGE_CBV>(ShaderInd), "Cache offset is out of range");
        VERIFY((m_DynamicCBSlotsMask[ShaderInd] & (1u << Binding)) != 0, "Attempting to set dynamic offset for a non-dynamic CB slot");

        const auto ResArrays = GetResourceArrays<D3D11_RESOURCE_RANGE_CBV>(ShaderInd);

        ResArrays.first[Binding].DynamicOffset = DynamicOffset;
    }
}

template <>
inline void ShaderResourceCacheD3D11::UpdateDynamicCBOffsetFlag<D3D11_RESOURCE_RANGE_CBV>(
    const ShaderResourceCacheD3D11::CachedCB& CB,
    Uint32                                    ShaderInd,
    Uint32                                    Binding)
{
    const auto BufferBit = Uint32{1} << Binding;
    if (m_DynamicCBSlotsMask[ShaderInd] & BufferBit)
    {
        // Only set the flag for those slots that allow dynamic buffers
        // (i.e. the variable was not created with NO_DYNAMIC_BUFFERS flag).
        if (CB.AllowsDynamicOffset())
            m_DynamicCBOffsetsMask[ShaderInd] |= BufferBit;
        else
            m_DynamicCBOffsetsMask[ShaderInd] &= ~BufferBit;
    }
    else
    {
        VERIFY((m_DynamicCBOffsetsMask[ShaderInd] & BufferBit) == 0,
               "A bit in m_DynamicCBOffsetsMask should never be set when corresponding bit in m_DynamicCBOffsetsMask is not set");
    }
}


template <D3D11_RESOURCE_RANGE ResRange>
bool ShaderResourceCacheD3D11::CopyResource(const ShaderResourceCacheD3D11& SrcCache, const D3D11ResourceBindPoints& BindPoints)
{
    bool IsBound = true;
    for (auto ActiveStages = BindPoints.GetActiveStages(); ActiveStages != SHADER_TYPE_UNKNOWN;)
    {
        const auto ShaderInd = ExtractFirstShaderStageIndex(ActiveStages);

        auto SrcResArrays = SrcCache.GetConstResourceArrays<ResRange>(ShaderInd);
        auto DstResArrays = GetResourceArrays<ResRange>(ShaderInd);

        const Uint32 Binding = BindPoints[ShaderInd];
        VERIFY(Binding < GetResourceCount<ResRange>(ShaderInd), "Index is out of range");
        VERIFY(Binding < SrcCache.GetResourceCount<ResRange>(ShaderInd), "Index is out of range");
        if (SrcResArrays.first[Binding])
        {
            DstResArrays.first[Binding]  = SrcResArrays.first[Binding];
            DstResArrays.second[Binding] = SrcResArrays.second[Binding];

            UpdateDynamicCBOffsetFlag<ResRange>(DstResArrays.first[Binding], ShaderInd, Binding);
        }
        else if (!DstResArrays.first[Binding])
        {
            IsBound = false;
        }
    }

    this->UpdateRevision();

    VERIFY_EXPR(IsBound == IsResourceBound<ResRange>(BindPoints));
    return IsBound;
}

template <>
__forceinline ID3D11Buffer* ShaderResourceCacheD3D11::CachedCB::GetD3D11Resource<D3D11_RESOURCE_RANGE_CBV>()
{
    return pBuff ? pBuff->GetD3D11Buffer() : nullptr;
}

template <>
__forceinline ID3D11SamplerState* ShaderResourceCacheD3D11::CachedSampler::GetD3D11Resource<D3D11_RESOURCE_RANGE_SAMPLER>()
{
    return pSampler ? pSampler->GetD3D11SamplerState() : nullptr;
}

template <>
__forceinline ID3D11ShaderResourceView* ShaderResourceCacheD3D11::CachedResource::GetD3D11Resource<D3D11_RESOURCE_RANGE_SRV>()
{
    if (pTexture != nullptr)
        return static_cast<ID3D11ShaderResourceView*>(pView.RawPtr<TextureViewD3D11Impl>()->GetD3D11View());
    else if (pBuffer != nullptr)
        return static_cast<ID3D11ShaderResourceView*>(pView.RawPtr<BufferViewD3D11Impl>()->GetD3D11View());
    else
        return nullptr;
}

template <>
__forceinline ID3D11UnorderedAccessView* ShaderResourceCacheD3D11::CachedResource::GetD3D11Resource<D3D11_RESOURCE_RANGE_UAV>()
{
    if (pTexture != nullptr)
        return static_cast<ID3D11UnorderedAccessView*>(pView.RawPtr<TextureViewD3D11Impl>()->GetD3D11View());
    else if (pBuffer != nullptr)
        return static_cast<ID3D11UnorderedAccessView*>(pView.RawPtr<BufferViewD3D11Impl>()->GetD3D11View());
    else
        return nullptr;
}


template <D3D11_RESOURCE_RANGE ResRange, typename TSrcResourceType, typename... ExtraArgsType>
inline void ShaderResourceCacheD3D11::SetResource(
    const D3D11ResourceBindPoints& BindPoints,
    TSrcResourceType               pResource,
    const ExtraArgsType&... ExtraArgs)
{
    for (auto ActiveStages = BindPoints.GetActiveStages(); ActiveStages != SHADER_TYPE_UNKNOWN;)
    {
        const Uint32 ShaderInd = ExtractFirstShaderStageIndex(ActiveStages);
        const Uint32 Binding   = BindPoints[ShaderInd];
        VERIFY(Binding < GetResourceCount<ResRange>(ShaderInd), "Cache offset is out of range");

        auto  ResArrays = GetResourceArrays<ResRange>(ShaderInd);
        auto& CachedRes = ResArrays.first[Binding];
        auto& pd3d11Res = ResArrays.second[Binding];
        // Do not move the resource as we need to set it for multiple stages!
        CachedRes.Set(pResource, ExtraArgs...);
        pd3d11Res = ResArrays.first[Binding].template GetD3D11Resource<ResRange>();

        VERIFY((CachedRes && pd3d11Res != nullptr || !CachedRes && pd3d11Res == nullptr),
               "Resource and D3D11 resource must be set/unset atomically");

        UpdateDynamicCBOffsetFlag<ResRange>(ResArrays.first[Binding], ShaderInd, Binding);
    }

    this->UpdateRevision();
}

} // namespace Diligent
