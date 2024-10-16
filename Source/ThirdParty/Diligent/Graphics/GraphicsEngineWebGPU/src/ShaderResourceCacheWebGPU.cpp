/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "ShaderResourceCacheWebGPU.hpp"
#include "EngineMemory.h"
#include "RenderDeviceWebGPUImpl.hpp"
#include "BufferWebGPUImpl.hpp"
#include "TextureWebGPUImpl.hpp"
#include "SamplerWebGPUImpl.hpp"

namespace Diligent
{

size_t ShaderResourceCacheWebGPU::GetRequiredMemorySize(Uint32 NumGroups, const Uint32* GroupSizes)
{
    Uint32 TotalResources = 0;
    for (Uint32 t = 0; t < NumGroups; ++t)
    {
        TotalResources += GroupSizes[t];
    }
    size_t MemorySize =
        NumGroups * sizeof(BindGroup) +
        TotalResources * sizeof(Resource) +
        TotalResources * sizeof(WGPUBindGroupEntry);
    return MemorySize;
}

ShaderResourceCacheWebGPU::ShaderResourceCacheWebGPU(ResourceCacheContentType ContentType) noexcept :
    m_ContentType{static_cast<Uint32>(ContentType)}
{
}

ShaderResourceCacheWebGPU::~ShaderResourceCacheWebGPU()
{
    if (m_pMemory)
    {
        Resource* pResources = GetFirstResourcePtr();
        for (Uint32 res = 0; res < m_TotalResources; ++res)
            pResources[res].~Resource();
        for (Uint32 t = 0; t < m_NumBindGroups; ++t)
            GetBindGroup(t).~BindGroup();
    }
}

void ShaderResourceCacheWebGPU::InitializeGroups(IMemoryAllocator& MemAllocator, Uint32 NumGroups, const Uint32* GroupSizes)
{
    VERIFY(!m_pMemory, "Memory has already been allocated");

    // Memory layout:
    //
    //  m_pMemory
    //  |
    //  V
    // ||  BindGroup[0]  |   ....    |  BindGroup[Ng-1]  |  Res[0]  |  ... |  Res[n-1]  |   ....   | Res[0]  |  ... |  Res[m-1]  | wgpuEntry[0] | ... | wgpuEntry[n-1] |   ....   | wgpuEntry[0] | ... | wgpuEntry[m-1] | DynOffset[0] | ... | DynOffset[k-1] ||
    //
    //
    //  Ng = m_NumBindGroups

    m_NumBindGroups = static_cast<Uint16>(NumGroups);
    VERIFY(m_NumBindGroups == NumGroups, "NumGroups (", NumGroups, ") exceeds maximum representable value");

    m_TotalResources = 0;
    for (Uint32 t = 0; t < NumGroups; ++t)
    {
        VERIFY_EXPR(GroupSizes[t] > 0);
        m_TotalResources += GroupSizes[t];
    }

    const size_t MemorySize =
        NumGroups * sizeof(BindGroup) +
        m_TotalResources * sizeof(Resource) +
        m_TotalResources * sizeof(WGPUBindGroupEntry);
    VERIFY_EXPR(MemorySize == GetRequiredMemorySize(NumGroups, GroupSizes));
#ifdef DILIGENT_DEBUG
    m_DbgInitializedResources.resize(m_NumBindGroups);
#endif

    if (MemorySize > 0)
    {
        m_pMemory = decltype(m_pMemory){
            ALLOCATE_RAW(MemAllocator, "Memory for shader resource cache data", MemorySize),
            STDDeleter<void, IMemoryAllocator>(MemAllocator),
        };
        memset(m_pMemory.get(), 0, MemorySize);

        BindGroup*          pGroups           = reinterpret_cast<BindGroup*>(m_pMemory.get());
        Resource*           pCurrResPtr       = reinterpret_cast<Resource*>(pGroups + m_NumBindGroups);
        WGPUBindGroupEntry* pCurrWGPUEntryPtr = reinterpret_cast<WGPUBindGroupEntry*>(pCurrResPtr + m_TotalResources);
        for (Uint32 t = 0; t < NumGroups; ++t)
        {
            const Uint32 GroupSize = GroupSizes[t];
            for (Uint32 Entry = 0; Entry < GroupSize; ++Entry)
            {
                pCurrWGPUEntryPtr[Entry]         = {};
                pCurrWGPUEntryPtr[Entry].binding = Entry;
            }

            new (&GetBindGroup(t)) BindGroup{
                GroupSize,
                GroupSize > 0 ? pCurrResPtr : nullptr,
                GroupSize > 0 ? pCurrWGPUEntryPtr : nullptr,
            };

            pCurrResPtr += GroupSize;
            pCurrWGPUEntryPtr += GroupSize;

#ifdef DILIGENT_DEBUG
            m_DbgInitializedResources[t].resize(GroupSize);
#endif
        }
    }
}

void ShaderResourceCacheWebGPU::Resource::SetUniformBuffer(RefCntAutoPtr<IDeviceObject>&& _pBuffer, Uint64 _BaseOffset, Uint64 _RangeSize)
{
    VERIFY_EXPR(Type == BindGroupEntryType::UniformBuffer ||
                Type == BindGroupEntryType::UniformBufferDynamic);

    pObject = std::move(_pBuffer);

    const BufferWebGPUImpl* pBuffWGPU = pObject.ConstPtr<BufferWebGPUImpl>();
#ifdef DILIGENT_DEBUG
    if (pBuffWGPU != nullptr)
    {
        VERIFY_EXPR((pBuffWGPU->GetDesc().BindFlags & BIND_UNIFORM_BUFFER) != 0);
        VERIFY(Type == BindGroupEntryType::UniformBufferDynamic || pBuffWGPU->GetDesc().Usage != USAGE_DYNAMIC,
               "Dynamic buffer must be used with UniformBufferDynamic descriptor");
    }
#endif

    VERIFY(_BaseOffset + _RangeSize <= (pBuffWGPU != nullptr ? pBuffWGPU->GetDesc().Size : 0), "Specified range is out of buffer bounds");
    BufferBaseOffset = _BaseOffset;
    BufferRangeSize  = _RangeSize;
    if (BufferRangeSize == 0)
        BufferRangeSize = pBuffWGPU != nullptr ? (pBuffWGPU->GetDesc().Size - BufferBaseOffset) : 0;

    // Reset dynamic offset
    BufferDynamicOffset = 0;
}

void ShaderResourceCacheWebGPU::Resource::SetStorageBuffer(RefCntAutoPtr<IDeviceObject>&& _pBufferView)
{
    VERIFY_EXPR(Type == BindGroupEntryType::StorageBuffer ||
                Type == BindGroupEntryType::StorageBufferDynamic ||
                Type == BindGroupEntryType::StorageBuffer_ReadOnly ||
                Type == BindGroupEntryType::StorageBufferDynamic_ReadOnly);

    pObject = std::move(_pBufferView);

    BufferDynamicOffset = 0; // It is essential to reset dynamic offset
    BufferBaseOffset    = 0;
    BufferRangeSize     = 0;

    if (!pObject)
        return;

    const BufferViewWebGPUImpl* pBuffViewWGPU = pObject.ConstPtr<BufferViewWebGPUImpl>();
    const BufferViewDesc&       ViewDesc      = pBuffViewWGPU->GetDesc();

    BufferBaseOffset = ViewDesc.ByteOffset;
    BufferRangeSize  = ViewDesc.ByteWidth;

#ifdef DILIGENT_DEBUG
    {
        const BufferWebGPUImpl* pBuffWGPU = pBuffViewWGPU->GetBuffer<const BufferWebGPUImpl>();
        const BufferDesc&       BuffDesc  = pBuffWGPU->GetDesc();
        VERIFY(Type == BindGroupEntryType::StorageBufferDynamic || Type == BindGroupEntryType::StorageBufferDynamic_ReadOnly || BuffDesc.Usage != USAGE_DYNAMIC,
               "Dynamic buffer must be used with StorageBufferDynamic or StorageBufferDynamic_ReadOnly descriptor");

        VERIFY(BufferBaseOffset + BufferRangeSize <= BuffDesc.Size, "Specified view range is out of buffer bounds");

        if (Type == BindGroupEntryType::StorageBuffer_ReadOnly ||
            Type == BindGroupEntryType::StorageBufferDynamic_ReadOnly)
        {
            VERIFY(ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Attempting to bind buffer view '", ViewDesc.Name,
                   "' as read-only storage buffer. Expected view type is BUFFER_VIEW_SHADER_RESOURCE. Actual type: ",
                   GetBufferViewTypeLiteralName(ViewDesc.ViewType));
            VERIFY((BuffDesc.BindFlags & BIND_SHADER_RESOURCE) != 0,
                   "Buffer '", BuffDesc.Name, "' being set as read-only storage buffer was not created with BIND_SHADER_RESOURCE flag");
        }
        else if (Type == BindGroupEntryType::StorageBuffer ||
                 Type == BindGroupEntryType::StorageBufferDynamic)
        {
            VERIFY(ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS, "Attempting to bind buffer view '", ViewDesc.Name,
                   "' as writable storage buffer. Expected view type is BUFFER_VIEW_UNORDERED_ACCESS. Actual type: ",
                   GetBufferViewTypeLiteralName(ViewDesc.ViewType));
            VERIFY((BuffDesc.BindFlags & BIND_UNORDERED_ACCESS) != 0,
                   "Buffer '", BuffDesc.Name, "' being set as writable storage buffer was not created with BIND_UNORDERED_ACCESS flag");
        }
        else
        {
            UNEXPECTED("Unexpected resource type");
        }
    }
#endif
}

template <>
Uint32 ShaderResourceCacheWebGPU::Resource::GetDynamicBufferOffset<BufferWebGPUImpl>(DeviceContextIndex CtxId) const
{
    Uint32 Offset = BufferDynamicOffset;
    if (const BufferWebGPUImpl* pBufferWGPU = pObject.ConstPtr<BufferWebGPUImpl>())
    {
        // Do not verify dynamic allocation here as there may be some buffers that are not used by the PSO.
        // The allocations of the buffers that are actually used will be verified by
        // PipelineResourceSignatureWebGPUImpl::DvpValidateCommittedResource().
        Offset += StaticCast<Uint32>(pBufferWGPU->GetDynamicOffset(CtxId, nullptr /* Do not verify allocation*/));
    }
    return Offset;
}

template <>
Uint32 ShaderResourceCacheWebGPU::Resource::GetDynamicBufferOffset<BufferViewWebGPUImpl>(DeviceContextIndex CtxId) const
{
    Uint32 Offset = BufferDynamicOffset;
    if (const BufferViewWebGPUImpl* pBufferWGPUView = pObject.ConstPtr<BufferViewWebGPUImpl>())
    {
        if (const BufferWebGPUImpl* pBufferWGPU = pBufferWGPUView->GetBuffer<const BufferWebGPUImpl>())
        {
            // Do not verify dynamic allocation here as there may be some buffers that are not used by the PSO.
            // The allocations of the buffers that are actually used will be verified by
            // PipelineResourceSignatureWebGPUImpl::DvpValidateCommittedResource().
            Offset += StaticCast<Uint32>(pBufferWGPU->GetDynamicOffset(CtxId, nullptr /* Do not verify allocation*/));
        }
    }
    return Offset;
}

void ShaderResourceCacheWebGPU::InitializeResources(Uint32 GroupIdx, Uint32 Offset, Uint32 ArraySize, BindGroupEntryType Type, bool HasImmutableSampler)
{
    BindGroup& Group = GetBindGroup(GroupIdx);
    for (Uint32 res = 0; res < ArraySize; ++res)
    {
        new (&Group.GetResource(Offset + res)) Resource{Type, HasImmutableSampler};
#ifdef DILIGENT_DEBUG
        m_DbgInitializedResources[GroupIdx][size_t{Offset} + res] = true;
#endif
    }
}


inline bool IsDynamicGroupEntryType(BindGroupEntryType EntryType)
{
    return (EntryType == BindGroupEntryType::UniformBufferDynamic ||
            EntryType == BindGroupEntryType::StorageBufferDynamic ||
            EntryType == BindGroupEntryType::StorageBufferDynamic_ReadOnly);
}

static bool IsDynamicBuffer(const ShaderResourceCacheWebGPU::Resource& Res)
{
    if (!Res.pObject)
        return false;

    const BufferWebGPUImpl* pBuffer = nullptr;
    static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
    switch (Res.Type)
    {
        case BindGroupEntryType::UniformBuffer:
        case BindGroupEntryType::UniformBufferDynamic:
            pBuffer = Res.pObject.ConstPtr<BufferWebGPUImpl>();
            break;

        case BindGroupEntryType::StorageBuffer:
        case BindGroupEntryType::StorageBufferDynamic:
        case BindGroupEntryType::StorageBuffer_ReadOnly:
        case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
            pBuffer = Res.pObject ? Res.pObject.ConstPtr<const BufferViewWebGPUImpl>()->GetBuffer<const BufferWebGPUImpl>() : nullptr;
            break;

        default:
            VERIFY_EXPR(Res.BufferRangeSize == 0);
            // Do nothing
            break;
    }

    if (pBuffer == nullptr)
        return false;

    const BufferDesc& BuffDesc = pBuffer->GetDesc();

    bool IsDynamic = (BuffDesc.Usage == USAGE_DYNAMIC);
    if (!IsDynamic)
    {
        // Buffers that are not bound as a whole to a dynamic descriptor are also counted as dynamic
        IsDynamic = (IsDynamicGroupEntryType(Res.Type) && Res.BufferRangeSize != 0 && Res.BufferRangeSize < BuffDesc.Size);
    }

    DEV_CHECK_ERR(!IsDynamic || IsDynamicGroupEntryType(Res.Type), "Dynamic buffers must only be used with dynamic descriptor type");
    return IsDynamic;
}

const ShaderResourceCacheWebGPU::Resource& ShaderResourceCacheWebGPU::SetResource(
    Uint32                       BindGroupIdx,
    Uint32                       CacheOffset,
    RefCntAutoPtr<IDeviceObject> pObject,
    Uint64                       BufferBaseOffset,
    Uint64                       BufferRangeSize)
{
    BindGroup& Group  = GetBindGroup(BindGroupIdx);
    Resource&  DstRes = Group.GetResource(CacheOffset);

    if (IsDynamicBuffer(DstRes))
    {
        VERIFY(m_NumDynamicBuffers > 0, "Dynamic buffers counter must be greater than zero when there is at least one dynamic buffer bound in the resource cache");
        --m_NumDynamicBuffers;
    }

    if ((DstRes.pObject != nullptr ? DstRes.pObject->GetUniqueID() : 0) != (pObject != nullptr ? pObject->GetUniqueID() : 0))
        Group.m_IsDirty = true;

    static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
    switch (DstRes.Type)
    {
        case BindGroupEntryType::UniformBuffer:
        case BindGroupEntryType::UniformBufferDynamic:
            DstRes.SetUniformBuffer(std::move(pObject), BufferBaseOffset, BufferRangeSize);
            break;

        case BindGroupEntryType::StorageBuffer:
        case BindGroupEntryType::StorageBufferDynamic:
        case BindGroupEntryType::StorageBuffer_ReadOnly:
        case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
            DstRes.SetStorageBuffer(std::move(pObject));
            break;

        default:
            VERIFY(BufferBaseOffset == 0 && BufferRangeSize == 0, "Buffer range can only be specified for uniform buffers");
            DstRes.pObject = std::move(pObject);
    }

    if (IsDynamicBuffer(DstRes))
    {
        ++m_NumDynamicBuffers;
    }

    if (DstRes.pObject)
    {
        WGPUBindGroupEntry& wgpuEntry = Group.GetWGPUEntry(CacheOffset);
        VERIFY_EXPR(wgpuEntry.binding == CacheOffset);

        static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
        switch (DstRes.Type)
        {
            case BindGroupEntryType::UniformBuffer:
            case BindGroupEntryType::UniformBufferDynamic:
            {
                const BufferWebGPUImpl* pBuffWGPU = DstRes.pObject.ConstPtr<BufferWebGPUImpl>();

                wgpuEntry.buffer = pBuffWGPU->GetWebGPUBuffer();
                VERIFY_EXPR(DstRes.BufferBaseOffset + DstRes.BufferRangeSize <= pBuffWGPU->GetDesc().Size);
                if (wgpuEntry.offset != DstRes.BufferBaseOffset || wgpuEntry.size != DstRes.BufferRangeSize)
                {
                    Group.m_IsDirty = true;
                }
                wgpuEntry.offset = DstRes.BufferBaseOffset;
                wgpuEntry.size   = DstRes.BufferRangeSize;
            }
            break;

            case BindGroupEntryType::StorageBuffer:
            case BindGroupEntryType::StorageBufferDynamic:
            case BindGroupEntryType::StorageBuffer_ReadOnly:
            case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
            {
                const BufferViewWebGPUImpl* pBuffViewWGPU = DstRes.pObject.ConstPtr<BufferViewWebGPUImpl>();
                const BufferWebGPUImpl*     pBuffWGPU     = pBuffViewWGPU->GetBuffer<const BufferWebGPUImpl>();

                wgpuEntry.buffer = pBuffWGPU->GetWebGPUBuffer();
                VERIFY_EXPR(DstRes.BufferBaseOffset + DstRes.BufferRangeSize <= pBuffWGPU->GetDesc().Size);
                if (wgpuEntry.offset != DstRes.BufferBaseOffset ||
                    wgpuEntry.size != DstRes.BufferRangeSize)
                {
                    Group.m_IsDirty = true;
                }
                wgpuEntry.offset = DstRes.BufferBaseOffset;
                wgpuEntry.size   = DstRes.BufferRangeSize;
            }
            break;

            case BindGroupEntryType::Texture:
            case BindGroupEntryType::StorageTexture_WriteOnly:
            case BindGroupEntryType::StorageTexture_ReadOnly:
            case BindGroupEntryType::StorageTexture_ReadWrite:
            {
                const TextureViewWebGPUImpl* pTexViewWGPU = DstRes.pObject.ConstPtr<TextureViewWebGPUImpl>();

                wgpuEntry.textureView = pTexViewWGPU->GetWebGPUTextureView();
            }
            break;

            case BindGroupEntryType::ExternalTexture:
            {
                UNSUPPORTED("External textures are not currently supported");
            }
            break;

            case BindGroupEntryType::Sampler:
            {
                const SamplerWebGPUImpl* pSamplerWGPU = DstRes.pObject.ConstPtr<SamplerWebGPUImpl>();

                wgpuEntry.sampler = pSamplerWGPU->GetWebGPUSampler();
            }
            break;

            default:
                UNEXPECTED("Unexpected resource type");
        }
    }

    UpdateRevision();

    return DstRes;
}


void ShaderResourceCacheWebGPU::SetDynamicBufferOffset(Uint32 BindGroupIndex,
                                                       Uint32 CacheOffset,
                                                       Uint32 DynamicBufferOffset)
{
    BindGroup& Group  = GetBindGroup(BindGroupIndex);
    Resource&  DstRes = Group.GetResource(CacheOffset);

    DEV_CHECK_ERR(DstRes.pObject, "Setting dynamic offset when no object is bound");
    const BufferWebGPUImpl* pBufferWGPU = (DstRes.Type == BindGroupEntryType::UniformBuffer ||
                                           DstRes.Type == BindGroupEntryType::UniformBufferDynamic) ?
        DstRes.pObject.ConstPtr<BufferWebGPUImpl>() :
        DstRes.pObject.ConstPtr<BufferViewWebGPUImpl>()->GetBuffer<const BufferWebGPUImpl>();
    DEV_CHECK_ERR(DstRes.BufferBaseOffset + DstRes.BufferRangeSize + DynamicBufferOffset <= pBufferWGPU->GetDesc().Size,
                  "Specified offset is out of buffer bounds");

    DstRes.BufferDynamicOffset = DynamicBufferOffset;
}

WGPUBindGroup ShaderResourceCacheWebGPU::UpdateBindGroup(WGPUDevice wgpuDevice, Uint32 GroupIndex, WGPUBindGroupLayout wgpuGroupLayout)
{
    BindGroup& Group = GetBindGroup(GroupIndex);
    if (!Group.m_wgpuBindGroup || Group.m_IsDirty)
    {
        WGPUBindGroupDescriptor wgpuBindGroupDescriptor;
        wgpuBindGroupDescriptor.nextInChain = nullptr;
        wgpuBindGroupDescriptor.label       = nullptr;
        wgpuBindGroupDescriptor.layout      = wgpuGroupLayout;
        wgpuBindGroupDescriptor.entryCount  = Group.m_NumResources;
        wgpuBindGroupDescriptor.entries     = Group.m_wgpuEntries;

        Group.m_wgpuBindGroup.Reset(wgpuDeviceCreateBindGroup(wgpuDevice, &wgpuBindGroupDescriptor));
        Group.m_IsDirty = false;
    }

    return Group.m_wgpuBindGroup;
}

bool ShaderResourceCacheWebGPU::GetDynamicBufferOffsets(DeviceContextIndex     CtxId,
                                                        std::vector<uint32_t>& Offsets,
                                                        Uint32                 GroupIdx) const
{
    // In each bind group, all uniform buffers with dynamic offsets (BindGroupEntryType::UniformBufferDynamic)
    // for every shader stage come first, followed by all storage buffers with dynamic offsets
    // (BindGroupEntryType::StorageBufferDynamic and BindGroupEntryType::StorageBufferDynamic_ReadOnly) for every shader stage,
    // followed by all other resources.

    const BindGroup& Group     = GetBindGroup(GroupIdx);
    const Uint32     GroupSize = Group.GetSize();

    Uint32 res            = 0;
    Uint32 OffsetInd      = 0;
    bool   OffsetsChanged = false;
    while (res < GroupSize)
    {
        const Resource& Res = Group.GetResource(res);
        if (Res.Type == BindGroupEntryType::UniformBufferDynamic)
        {
            const Uint32 Offset    = Res.GetDynamicBufferOffset<BufferWebGPUImpl>(CtxId);
            Uint32&      DstOffset = Offsets[OffsetInd++];
            if (DstOffset != Offset)
                OffsetsChanged = true;
            DstOffset = Offset;
            ++res;
        }
        else
            break;
    }

    while (res < GroupSize)
    {
        const Resource& Res = Group.GetResource(res);
        if (Res.Type == BindGroupEntryType::StorageBufferDynamic ||
            Res.Type == BindGroupEntryType::StorageBufferDynamic_ReadOnly)
        {
            const Uint32 Offset    = Res.GetDynamicBufferOffset<BufferViewWebGPUImpl>(CtxId);
            Uint32&      DstOffset = Offsets[OffsetInd++];
            if (DstOffset != Offset)
                OffsetsChanged = true;
            DstOffset = Offset;
            ++res;
        }
        else
            break;
    }

#ifdef DILIGENT_DEBUG
    for (; res < GroupSize; ++res)
    {
        const Resource& Res = Group.GetResource(res);
        VERIFY((Res.Type != BindGroupEntryType::UniformBufferDynamic &&
                Res.Type != BindGroupEntryType::StorageBufferDynamic &&
                Res.Type != BindGroupEntryType::StorageBufferDynamic_ReadOnly),
               "All dynamic uniform and storage buffers are expected to go first in the beginning of each bind group");
    }
#endif

    VERIFY(OffsetInd == Offsets.size(),
           "The number of dynamic offsets written (", OffsetInd, ") does not match the expected number (", Offsets.size(),
           "). This likely indicates the mismatch between the SRB and the PSO");

    return OffsetsChanged;
}

#ifdef DILIGENT_DEBUG
void ShaderResourceCacheWebGPU::DbgVerifyResourceInitialization() const
{
    for (const auto& SetFlags : m_DbgInitializedResources)
    {
        for (bool ResInitialized : SetFlags)
            VERIFY(ResInitialized, "Not all resources in the cache have been initialized. This is a bug.");
    }
}

void ShaderResourceCacheWebGPU::DbgVerifyDynamicBuffersCounter() const
{
    const Resource* pResources        = GetFirstResourcePtr();
    Uint32          NumDynamicBuffers = 0;
    for (Uint32 res = 0; res < m_TotalResources; ++res)
    {
        if (IsDynamicBuffer(pResources[res]))
            ++NumDynamicBuffers;
    }
    VERIFY(NumDynamicBuffers == m_NumDynamicBuffers, "The number of dynamic buffers (", m_NumDynamicBuffers, ") does not match the actual number (", NumDynamicBuffers, ")");
}
#endif

} // namespace Diligent
