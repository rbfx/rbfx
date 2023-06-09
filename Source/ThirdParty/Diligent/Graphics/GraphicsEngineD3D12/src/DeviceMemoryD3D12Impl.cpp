/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "DeviceMemoryD3D12Impl.hpp"

#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"

#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

namespace
{

D3D12_HEAP_FLAGS GetD3D12HeapFlags(ID3D12Device*   pd3d12Device,
                                   IDeviceObject** ppResources,
                                   Uint32          NumResources,
                                   bool&           AllowMSAA,
                                   bool&           UseNVApi) noexcept(false)
{
    AllowMSAA = false;
    UseNVApi  = false;

    // NB: D3D12_RESOURCE_HEAP_TIER_1 hardware requires exactly one of the
    //     flags below left unset when creating a heap.
    constexpr auto D3D12_HEAP_FLAG_DENY_ALL =
        D3D12_HEAP_FLAG_DENY_BUFFERS |
        D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;

    auto HeapFlags = D3D12_HEAP_FLAG_DENY_ALL;

    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Features{};
    if (SUCCEEDED(pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Features, sizeof(d3d12Features))))
    {
        if (d3d12Features.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_1)
        {
            if (NumResources == 0)
            {
                LOG_ERROR_AND_THROW("D3D12_RESOURCE_HEAP_TIER_1 hardware requires that at least one comptaible resource is provided. "
                                    "See SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT capability.");
            }
        }
        else if (d3d12Features.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2)
        {
            // D3D12_RESOURCE_HEAP_TIER_2 hardware allows any combination of resources
            // to be placed in the heap
            HeapFlags = D3D12_HEAP_FLAG_NONE;
        }
    }

    if (NumResources == 0)
        return HeapFlags;

    Uint32 UsingNVApiCount    = 0;
    Uint32 NotUsingNVApiCount = 0;

    static_assert(BIND_FLAG_LAST == 1u << 11u, "Did you add a new bind flag? You may need to update the logic below.");
    for (Uint32 res = 0; res < NumResources; ++res)
    {
        auto* pResource = ppResources[res];
        if (pResource == nullptr)
            continue;

        if (RefCntAutoPtr<ITextureD3D12> pTexture{pResource, IID_TextureD3D12})
        {
            const auto* pTexD3D12Impl = pTexture.ConstPtr<TextureD3D12Impl>();
            const auto& TexDesc       = pTexD3D12Impl->GetDesc();

            if (TexDesc.Usage != USAGE_SPARSE)
                LOG_ERROR_AND_THROW("Resource must be created with USAGE_SPARSE");

            if (TexDesc.SampleCount > 1)
                AllowMSAA = true;

            if (pTexD3D12Impl->IsUsingNVApi())
                ++UsingNVApiCount;
            else
                ++NotUsingNVApiCount;

            if (TexDesc.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL))
                HeapFlags &= ~D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;

            if (TexDesc.BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_INPUT_ATTACHMENT))
                HeapFlags &= ~D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;

            if (TexDesc.BindFlags & BIND_UNORDERED_ACCESS)
                HeapFlags |= D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS;
        }
        else if (RefCntAutoPtr<IBufferD3D12> pBuffer{pResource, IID_BufferD3D12})
        {
            const auto& BuffDesc = pBuffer.ConstPtr<BufferD3D12Impl>()->GetDesc();

            if (BuffDesc.Usage != USAGE_SPARSE)
                LOG_ERROR_AND_THROW("Resource must be created with USAGE_SPARSE");

            HeapFlags &= ~D3D12_HEAP_FLAG_DENY_BUFFERS;
            if (BuffDesc.BindFlags & BIND_UNORDERED_ACCESS)
                HeapFlags |= D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS;
        }
        else
        {
            UNEXPECTED("unsupported resource type");
        }
    }

    if (d3d12Features.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_1)
    {
        const auto NumDenyFlags = PlatformMisc::CountOneBits(static_cast<Uint32>(HeapFlags & D3D12_HEAP_FLAG_DENY_ALL));
        if (NumDenyFlags != 2)
        {
            LOG_ERROR_AND_THROW("On D3D12_RESOURCE_HEAP_TIER_1 hadrware, only single resource usage for the heap is allowed: "
                                "buffers, RT_DS_TEXTURES (BIND_RENDER_TARGET, BIND_DEPTH_STENCIL), or NON_RT_DS_TEXTURES "
                                "(BIND_SHADER_RESOURCE, BIND_UNORDERED_ACCESS, BIND_INPUT_ATTACHMENT). "
                                "See SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT capability.");
        }
    }

    if (UsingNVApiCount > 0)
        UseNVApi = true;

    if (UseNVApi && NotUsingNVApiCount > 0)
        LOG_ERROR_AND_THROW("Resources that use NVApi are incompatible with the resources that don't");

    return HeapFlags;
}

inline CComPtr<ID3D12Heap> CreateD3D12Heap(RenderDeviceD3D12Impl* pDevice, const D3D12_HEAP_DESC& d3d12HeapDesc, bool UseNVApi)
{
    auto* const pd3d12Device = pDevice->GetD3D12Device();

    CComPtr<ID3D12Heap> pd3d12Heap;
#ifdef DILIGENT_ENABLE_D3D_NVAPI
    if (UseNVApi)
    {
        if (NvAPI_D3D12_CreateHeap(pd3d12Device, &d3d12HeapDesc, IID_PPV_ARGS(&pd3d12Heap)) != NVAPI_OK)
        {
            LOG_ERROR_MESSAGE("Failed to create D3D12 heap using NVApi");
            return {};
        }
    }
    else
#endif
    {
        if (FAILED(pd3d12Device->CreateHeap(&d3d12HeapDesc, IID_PPV_ARGS(&pd3d12Heap))))
        {
            LOG_ERROR_MESSAGE("Failed to create D3D12 heap");
            return {};
        }
    }

    return pd3d12Heap;
}

} // namespace

DeviceMemoryD3D12Impl::DeviceMemoryD3D12Impl(IReferenceCounters*           pRefCounters,
                                             RenderDeviceD3D12Impl*        pDeviceD3D11,
                                             const DeviceMemoryCreateInfo& MemCI) :
    TDeviceMemoryBase{pRefCounters, pDeviceD3D11, MemCI}
{
    m_d3d12HeapFlags = GetD3D12HeapFlags(m_pDevice->GetD3D12Device(), MemCI.ppCompatibleResources, MemCI.NumResources, m_AllowMSAA, m_UseNVApi);

    if (!Resize(MemCI.InitialSize))
        LOG_ERROR_AND_THROW("Failed to allocate device memory");
}

DeviceMemoryD3D12Impl::~DeviceMemoryD3D12Impl()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_Pages), m_Desc.ImmediateContextMask);
}

IMPLEMENT_QUERY_INTERFACE(DeviceMemoryD3D12Impl, IID_DeviceMemoryD3D12, TDeviceMemoryBase)


Bool DeviceMemoryD3D12Impl::Resize(Uint64 NewSize)
{
    DvpVerifyResize(NewSize);

    D3D12_HEAP_DESC d3d12HeapDesc{};
    d3d12HeapDesc.SizeInBytes                     = m_Desc.PageSize;
    d3d12HeapDesc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    d3d12HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    d3d12HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    d3d12HeapDesc.Properties.CreationNodeMask     = 1;
    d3d12HeapDesc.Properties.VisibleNodeMask      = 1;
    d3d12HeapDesc.Alignment                       = m_AllowMSAA ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    d3d12HeapDesc.Flags                           = m_d3d12HeapFlags;

    const auto NewPageCount = StaticCast<size_t>(NewSize / m_Desc.PageSize);
    m_Pages.reserve(NewPageCount);

    while (m_Pages.size() < NewPageCount)
    {
        if (auto pHeap = CreateD3D12Heap(m_pDevice, d3d12HeapDesc, m_UseNVApi))
        {
            pHeap->SetName(L"Device memory page");
            m_Pages.emplace_back(std::move(pHeap));
        }
        else
        {
            return false;
        }
    }

    while (m_Pages.size() > NewPageCount)
    {
        m_pDevice->SafeReleaseDeviceObject(std::move(m_Pages.back()), m_Desc.ImmediateContextMask);
        m_Pages.pop_back();
    }

    return true;
}

Uint64 DeviceMemoryD3D12Impl::GetCapacity() const
{
    return m_Desc.PageSize * m_Pages.size();
}

Bool DeviceMemoryD3D12Impl::IsCompatible(IDeviceObject* pResource) const
{
    try
    {
        bool AllowMSAA              = false;
        bool UseNVApi               = false;
        auto d3d12RequiredHeapFlags = GetD3D12HeapFlags(m_pDevice->GetD3D12Device(), &pResource, 1, AllowMSAA, UseNVApi);
        return ((m_d3d12HeapFlags & d3d12RequiredHeapFlags) == d3d12RequiredHeapFlags) && (!AllowMSAA || m_AllowMSAA) && (UseNVApi == m_UseNVApi);
    }
    catch (...)
    {
        return false;
    }
}

DeviceMemoryRangeD3D12 DeviceMemoryD3D12Impl::GetRange(Uint64 Offset, Uint64 Size) const
{
    const auto PageIdx = static_cast<size_t>(Offset / m_Desc.PageSize);

    DeviceMemoryRangeD3D12 Range{};
    if (PageIdx >= m_Pages.size())
    {
        DEV_ERROR("DeviceMemoryD3D12Impl::GetRange(): Offset is out of bounds of allocated space");
        return Range;
    }

    const auto OffsetInPage = Offset % m_Desc.PageSize;
    if (OffsetInPage + Size > m_Desc.PageSize)
    {
        DEV_ERROR("DeviceMemoryD3D12Impl::GetRange(): Offset and Size must be inside a single page");
        return Range;
    }

    Range.Offset  = OffsetInPage;
    Range.pHandle = m_Pages[PageIdx];
    Range.Size    = std::min(m_Desc.PageSize - OffsetInPage, Size);

    return Range;
}

} // namespace Diligent
