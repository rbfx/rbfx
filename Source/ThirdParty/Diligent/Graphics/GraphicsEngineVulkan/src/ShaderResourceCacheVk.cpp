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

#include "ShaderResourceCacheVk.hpp"

#include "DeviceContextVkImpl.hpp"
#include "BufferViewVkImpl.hpp"
#include "TextureViewVkImpl.hpp"
#include "TextureVkImpl.hpp"
#include "SamplerVkImpl.hpp"
#include "TopLevelASVkImpl.hpp"

#include "VulkanTypeConversions.hpp"

namespace Diligent
{

size_t ShaderResourceCacheVk::GetRequiredMemorySize(Uint32 NumSets, const Uint32* SetSizes)
{
    Uint32 TotalResources = 0;
    for (Uint32 t = 0; t < NumSets; ++t)
        TotalResources += SetSizes[t];
    size_t MemorySize = NumSets * sizeof(DescriptorSet) + TotalResources * sizeof(Resource);
    return MemorySize;
}

void ShaderResourceCacheVk::InitializeSets(IMemoryAllocator& MemAllocator, Uint32 NumSets, const Uint32* SetSizes)
{
    VERIFY(!m_pMemory, "Memory has already been allocated");

    // Memory layout:
    //
    //  m_pMemory
    //  |
    //  V
    // ||  DescriptorSet[0]  |   ....    |  DescriptorSet[Ns-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  ||
    //
    //
    //  Ns = m_NumSets

    m_NumSets = static_cast<Uint16>(NumSets);
    VERIFY(m_NumSets == NumSets, "NumSets (", NumSets, ") exceed maximum representable value");

    m_TotalResources = 0;
    for (Uint32 t = 0; t < NumSets; ++t)
    {
        VERIFY_EXPR(SetSizes[t] > 0);
        m_TotalResources += SetSizes[t];
    }

    const auto MemorySize = NumSets * sizeof(DescriptorSet) + m_TotalResources * sizeof(Resource);
    VERIFY_EXPR(MemorySize == GetRequiredMemorySize(NumSets, SetSizes));
#ifdef DILIGENT_DEBUG
    m_DbgInitializedResources.resize(m_NumSets);
#endif
    if (MemorySize > 0)
    {
        m_pMemory = decltype(m_pMemory){
            ALLOCATE_RAW(MemAllocator, "Memory for shader resource cache data", MemorySize),
            STDDeleter<void, IMemoryAllocator>(MemAllocator) //
        };

        DescriptorSet* pSets       = reinterpret_cast<DescriptorSet*>(m_pMemory.get());
        Resource*      pCurrResPtr = reinterpret_cast<Resource*>(pSets + m_NumSets);
        for (Uint32 t = 0; t < NumSets; ++t)
        {
            new (&GetDescriptorSet(t)) DescriptorSet{SetSizes[t], SetSizes[t] > 0 ? pCurrResPtr : nullptr};
            pCurrResPtr += SetSizes[t];
#ifdef DILIGENT_DEBUG
            m_DbgInitializedResources[t].resize(SetSizes[t]);
#endif
        }
        VERIFY_EXPR((char*)pCurrResPtr == (char*)m_pMemory.get() + MemorySize);
    }
}

void ShaderResourceCacheVk::InitializeResources(Uint32 Set, Uint32 Offset, Uint32 ArraySize, DescriptorType Type, bool HasImmutableSampler)
{
    auto& DescrSet = GetDescriptorSet(Set);
    for (Uint32 res = 0; res < ArraySize; ++res)
    {
        new (&DescrSet.GetResource(Offset + res)) Resource{Type, HasImmutableSampler};
#ifdef DILIGENT_DEBUG
        m_DbgInitializedResources[Set][size_t{Offset} + res] = true;
#endif
    }
}

inline bool IsDynamicDescriptorType(DescriptorType DescrType)
{
    return (DescrType == DescriptorType::UniformBufferDynamic ||
            DescrType == DescriptorType::StorageBufferDynamic ||
            DescrType == DescriptorType::StorageBufferDynamic_ReadOnly);
}

static bool IsDynamicBuffer(const ShaderResourceCacheVk::Resource& Res)
{
    if (!Res.pObject)
        return false;

    const BufferVkImpl* pBuffer = nullptr;
    switch (Res.Type)
    {
        case DescriptorType::UniformBufferDynamic:
        case DescriptorType::UniformBuffer:
            pBuffer = Res.pObject.ConstPtr<BufferVkImpl>();
            break;

        case DescriptorType::StorageBuffer:
        case DescriptorType::StorageBuffer_ReadOnly:
        case DescriptorType::StorageBufferDynamic:
        case DescriptorType::StorageBufferDynamic_ReadOnly:
            pBuffer = Res.pObject ? Res.pObject.ConstPtr<BufferViewVkImpl>()->GetBuffer<const BufferVkImpl>() : nullptr;
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
        IsDynamic = (IsDynamicDescriptorType(Res.Type) && Res.BufferRangeSize != 0 && Res.BufferRangeSize < BuffDesc.Size);
    }

    DEV_CHECK_ERR(!IsDynamic || IsDynamicDescriptorType(Res.Type),
                  "Dynamic buffers must only be used with dynamic descriptor type");

    return IsDynamic;
}


#ifdef DILIGENT_DEBUG
void ShaderResourceCacheVk::DbgVerifyResourceInitialization() const
{
    for (const auto& SetFlags : m_DbgInitializedResources)
    {
        for (bool ResInitialized : SetFlags)
            VERIFY(ResInitialized, "Not all resources in the cache have been initialized. This is a bug.");
    }
}

void ShaderResourceCacheVk::DbgVerifyDynamicBuffersCounter() const
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

ShaderResourceCacheVk::~ShaderResourceCacheVk()
{
    if (m_pMemory)
    {
        Resource* pResources = GetFirstResourcePtr();
        for (Uint32 res = 0; res < m_TotalResources; ++res)
            pResources[res].~Resource();
        for (Uint32 t = 0; t < m_NumSets; ++t)
            GetDescriptorSet(t).~DescriptorSet();
    }
}

void ShaderResourceCacheVk::Resource::SetUniformBuffer(RefCntAutoPtr<IDeviceObject>&& _pBuffer, Uint64 _BaseOffset, Uint64 _RangeSize)
{
    VERIFY_EXPR(Type == DescriptorType::UniformBuffer ||
                Type == DescriptorType::UniformBufferDynamic);

    pObject = std::move(_pBuffer);

    const BufferVkImpl* pBuffVk = pObject.ConstPtr<BufferVkImpl>();

#ifdef DILIGENT_DEBUG
    if (pBuffVk != nullptr)
    {
        // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC descriptor type require
        // buffer to be created with VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        VERIFY_EXPR((pBuffVk->GetDesc().BindFlags & BIND_UNIFORM_BUFFER) != 0);
        VERIFY(Type == DescriptorType::UniformBufferDynamic || pBuffVk->GetDesc().Usage != USAGE_DYNAMIC,
               "Dynamic buffer must be used with UniformBufferDynamic descriptor");
    }
#endif

    VERIFY(_BaseOffset + _RangeSize <= (pBuffVk != nullptr ? pBuffVk->GetDesc().Size : 0), "Specified range is out of buffer bounds");
    BufferBaseOffset = _BaseOffset;
    BufferRangeSize  = _RangeSize;
    if (BufferRangeSize == 0)
        BufferRangeSize = pBuffVk != nullptr ? (pBuffVk->GetDesc().Size - BufferBaseOffset) : 0;

    // Reset dynamic offset
    BufferDynamicOffset = 0;
}

void ShaderResourceCacheVk::Resource::SetStorageBuffer(RefCntAutoPtr<IDeviceObject>&& _pBufferView)
{
    VERIFY_EXPR(Type == DescriptorType::StorageBuffer ||
                Type == DescriptorType::StorageBufferDynamic ||
                Type == DescriptorType::StorageBuffer_ReadOnly ||
                Type == DescriptorType::StorageBufferDynamic_ReadOnly);

    pObject = std::move(_pBufferView);

    BufferDynamicOffset = 0; // It is essential to reset dynamic offset
    BufferBaseOffset    = 0;
    BufferRangeSize     = 0;

    if (!pObject)
        return;

    const BufferViewVkImpl* pBuffViewVk = pObject.ConstPtr<BufferViewVkImpl>();
    const BufferViewDesc&   ViewDesc    = pBuffViewVk->GetDesc();

    BufferBaseOffset = ViewDesc.ByteOffset;
    BufferRangeSize  = ViewDesc.ByteWidth;

#ifdef DILIGENT_DEBUG
    {
        const BufferVkImpl* pBuffVk  = pBuffViewVk->GetBuffer<const BufferVkImpl>();
        const BufferDesc&   BuffDesc = pBuffVk->GetDesc();
        VERIFY(Type == DescriptorType::StorageBufferDynamic || Type == DescriptorType::StorageBufferDynamic_ReadOnly || BuffDesc.Usage != USAGE_DYNAMIC,
               "Dynamic buffer must be used with StorageBufferDynamic or StorageBufferDynamic_ReadOnly descriptor");

        VERIFY(BufferBaseOffset + BufferRangeSize <= BuffDesc.Size,
               "Specified view range is out of buffer bounds");

        // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC descriptor type
        // require buffer to be created with VK_BUFFER_USAGE_STORAGE_BUFFER_BIT (13.2.4)
        if (Type == DescriptorType::StorageBuffer_ReadOnly || Type == DescriptorType::StorageBufferDynamic_ReadOnly)
        {
            // HLSL buffer SRVs are mapped to read-only storage buffers in SPIR-V
            VERIFY(ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Attempting to bind buffer view '", ViewDesc.Name,
                   "' as read-only storage buffer. Expected view type is BUFFER_VIEW_SHADER_RESOURCE. Actual type: ",
                   GetBufferViewTypeLiteralName(ViewDesc.ViewType));
            VERIFY((BuffDesc.BindFlags & BIND_SHADER_RESOURCE) != 0,
                   "Buffer '", BuffDesc.Name, "' being set as read-only storage buffer was not created with BIND_SHADER_RESOURCE flag");
        }
        else if (Type == DescriptorType::StorageBuffer || Type == DescriptorType::StorageBufferDynamic)
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

const ShaderResourceCacheVk::Resource& ShaderResourceCacheVk::SetResource(
    const VulkanUtilities::VulkanLogicalDevice* pLogicalDevice,
    Uint32                                      DescrSetIndex,
    Uint32                                      CacheOffset,
    SetResourceInfo&&                           SrcRes)
{
    DescriptorSet& DescrSet = GetDescriptorSet(DescrSetIndex);
    Resource&      DstRes   = DescrSet.GetResource(CacheOffset);

    if (IsDynamicBuffer(DstRes))
    {
        VERIFY(m_NumDynamicBuffers > 0, "Dynamic buffers counter must be greater than zero when there is at least one dynamic buffer bound in the resource cache");
        --m_NumDynamicBuffers;
    }

    static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
    switch (DstRes.Type)
    {
        case DescriptorType::UniformBuffer:
        case DescriptorType::UniformBufferDynamic:
            DstRes.SetUniformBuffer(std::move(SrcRes.pObject), SrcRes.BufferBaseOffset, SrcRes.BufferRangeSize);
            break;

        case DescriptorType::StorageBuffer:
        case DescriptorType::StorageBuffer_ReadOnly:
        case DescriptorType::StorageBufferDynamic:
        case DescriptorType::StorageBufferDynamic_ReadOnly:
            DstRes.SetStorageBuffer(std::move(SrcRes.pObject));
            break;

        default:
            VERIFY(SrcRes.BufferBaseOffset == 0 && SrcRes.BufferRangeSize == 0, "Buffer range can only be specified for uniform buffers");
            DstRes.pObject = std::move(SrcRes.pObject);
    }

    if (IsDynamicBuffer(DstRes))
    {
        ++m_NumDynamicBuffers;
    }

    VkDescriptorSet vkSet = DescrSet.GetVkDescriptorSet();
    if (vkSet != VK_NULL_HANDLE && DstRes.pObject)
    {
        VERIFY(pLogicalDevice != nullptr, "Logical device must not be null to write descriptor to a non-null set");

        VkWriteDescriptorSet WriteDescrSet;
        WriteDescrSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        WriteDescrSet.pNext           = nullptr;
        WriteDescrSet.dstSet          = vkSet;
        WriteDescrSet.dstBinding      = SrcRes.BindingIndex;
        WriteDescrSet.dstArrayElement = SrcRes.ArrayIndex;
        WriteDescrSet.descriptorCount = 1;
        // descriptorType must be the same type as that specified in VkDescriptorSetLayoutBinding for dstSet at dstBinding.
        // The type of the descriptor also controls which array the descriptors are taken from. (13.2.4)
        WriteDescrSet.descriptorType   = DescriptorTypeToVkDescriptorType(DstRes.Type);
        WriteDescrSet.pImageInfo       = nullptr;
        WriteDescrSet.pBufferInfo      = nullptr;
        WriteDescrSet.pTexelBufferView = nullptr;

        // Do not zero-initialize!
        union
        {
            VkDescriptorImageInfo                        vkDescrImageInfo;
            VkDescriptorBufferInfo                       vkDescrBufferInfo;
            VkBufferView                                 vkDescrBufferView;
            VkWriteDescriptorSetAccelerationStructureKHR vkDescrAccelStructInfo;
        };

        static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
        switch (DstRes.Type)
        {
            case DescriptorType::Sampler:
                vkDescrImageInfo         = DstRes.GetSamplerDescriptorWriteInfo();
                WriteDescrSet.pImageInfo = &vkDescrImageInfo;
                break;

            case DescriptorType::CombinedImageSampler:
            case DescriptorType::SeparateImage:
            case DescriptorType::StorageImage:
                vkDescrImageInfo         = DstRes.GetImageDescriptorWriteInfo();
                WriteDescrSet.pImageInfo = &vkDescrImageInfo;
                break;

            case DescriptorType::UniformTexelBuffer:
            case DescriptorType::StorageTexelBuffer:
            case DescriptorType::StorageTexelBuffer_ReadOnly:
                vkDescrBufferView              = DstRes.GetBufferViewWriteInfo();
                WriteDescrSet.pTexelBufferView = &vkDescrBufferView;
                break;

            case DescriptorType::UniformBuffer:
            case DescriptorType::UniformBufferDynamic:
                vkDescrBufferInfo         = DstRes.GetUniformBufferDescriptorWriteInfo();
                WriteDescrSet.pBufferInfo = &vkDescrBufferInfo;
                break;

            case DescriptorType::StorageBuffer:
            case DescriptorType::StorageBuffer_ReadOnly:
            case DescriptorType::StorageBufferDynamic:
            case DescriptorType::StorageBufferDynamic_ReadOnly:
                vkDescrBufferInfo         = DstRes.GetStorageBufferDescriptorWriteInfo();
                WriteDescrSet.pBufferInfo = &vkDescrBufferInfo;
                break;

            case DescriptorType::InputAttachment:
            case DescriptorType::InputAttachment_General:
                vkDescrImageInfo         = DstRes.GetInputAttachmentDescriptorWriteInfo();
                WriteDescrSet.pImageInfo = &vkDescrImageInfo;
                break;

            case DescriptorType::AccelerationStructure:
                vkDescrAccelStructInfo = DstRes.GetAccelerationStructureWriteInfo();
                WriteDescrSet.pNext    = &vkDescrAccelStructInfo;
                break;

            default:
                UNEXPECTED("Unexpected descriptor type");
        }

        pLogicalDevice->UpdateDescriptorSets(1, &WriteDescrSet, 0, nullptr);
    }

    UpdateRevision();

    return DstRes;
}

void ShaderResourceCacheVk::SetDynamicBufferOffset(Uint32 DescrSetIndex,
                                                   Uint32 CacheOffset,
                                                   Uint32 DynamicBufferOffset)
{
    DescriptorSet& DescrSet = GetDescriptorSet(DescrSetIndex);
    Resource&      DstRes   = DescrSet.GetResource(CacheOffset);
    VERIFY(IsDynamicDescriptorType(DstRes.Type), "Dynamic offsets can only be set of dynamic uniform or storage buffers");

    DEV_CHECK_ERR(DstRes.pObject, "Setting dynamic offset when no object is bound");
    const BufferVkImpl* pBufferVk = DstRes.Type == DescriptorType::UniformBufferDynamic ?
        DstRes.pObject.ConstPtr<BufferVkImpl>() :
        DstRes.pObject.ConstPtr<BufferViewVkImpl>()->GetBuffer<const BufferVkImpl>();
    DEV_CHECK_ERR(DstRes.BufferBaseOffset + DstRes.BufferRangeSize + DynamicBufferOffset <= pBufferVk->GetDesc().Size,
                  "Specified offset is out of buffer bounds");

    DstRes.BufferDynamicOffset = DynamicBufferOffset;
}


namespace
{

RESOURCE_STATE DescriptorTypeToResourceState(DescriptorType Type)
{
    static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
    switch (Type)
    {
            // clang-format off
        case DescriptorType::Sampler:                       return RESOURCE_STATE_UNKNOWN;
        case DescriptorType::CombinedImageSampler:          return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::SeparateImage:                 return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::StorageImage:                  return RESOURCE_STATE_UNORDERED_ACCESS;
        case DescriptorType::UniformTexelBuffer:            return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::StorageTexelBuffer:            return RESOURCE_STATE_UNORDERED_ACCESS;
        case DescriptorType::StorageTexelBuffer_ReadOnly:   return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::UniformBuffer:                 return RESOURCE_STATE_CONSTANT_BUFFER;
        case DescriptorType::UniformBufferDynamic:          return RESOURCE_STATE_CONSTANT_BUFFER;
        case DescriptorType::StorageBuffer:                 return RESOURCE_STATE_UNORDERED_ACCESS;
        case DescriptorType::StorageBuffer_ReadOnly:        return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::StorageBufferDynamic:          return RESOURCE_STATE_UNORDERED_ACCESS;
        case DescriptorType::StorageBufferDynamic_ReadOnly: return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::InputAttachment:               return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::InputAttachment_General:       return RESOURCE_STATE_SHADER_RESOURCE;
        case DescriptorType::AccelerationStructure:         return RESOURCE_STATE_RAY_TRACING;
        // clang-format on
        default:
            UNEXPECTED("unknown descriptor type");
            return RESOURCE_STATE_UNKNOWN;
    }
}

template <bool VerifyOnly>
inline void TransitionUniformBuffer(DeviceContextVkImpl* pCtxVkImpl,
                                    BufferVkImpl*        pBufferVk,
                                    DescriptorType       DescrType)
{
    if (pBufferVk == nullptr || !pBufferVk->IsInKnownState())
        return;

    constexpr RESOURCE_STATE RequiredState = RESOURCE_STATE_CONSTANT_BUFFER;
    VERIFY_EXPR(DescriptorTypeToResourceState(DescrType) == RequiredState);
    VERIFY_EXPR((ResourceStateFlagsToVkAccessFlags(RequiredState) & VK_ACCESS_UNIFORM_READ_BIT) == VK_ACCESS_UNIFORM_READ_BIT);
    const bool IsInRequiredState = pBufferVk->CheckState(RequiredState);
    if (VerifyOnly)
    {
        if (!IsInRequiredState)
        {
            LOG_ERROR_MESSAGE("State of buffer '", pBufferVk->GetDesc().Name, "' is incorrect. Required state: ",
                              GetResourceStateString(RequiredState), ". Actual state: ",
                              GetResourceStateString(pBufferVk->GetState()),
                              ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                              "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                              "with IDeviceContext::TransitionResourceStates().");
        }
    }
    else
    {
        if (!IsInRequiredState)
        {
            pCtxVkImpl->TransitionBufferState(*pBufferVk, RESOURCE_STATE_UNKNOWN, RequiredState, true);
        }
        VERIFY_EXPR(pBufferVk->CheckAccessFlags(VK_ACCESS_UNIFORM_READ_BIT));
    }
}

template <bool VerifyOnly>
inline void TransitionBufferView(DeviceContextVkImpl* pCtxVkImpl,
                                 BufferViewVkImpl*    pBuffViewVk,
                                 DescriptorType       DescrType)
{
    if (pBuffViewVk == nullptr)
        return;

    BufferVkImpl* pBufferVk = pBuffViewVk->GetBuffer<BufferVkImpl>();
    if (!pBufferVk->IsInKnownState())
        return;

    const RESOURCE_STATE RequiredState = DescriptorTypeToResourceState(DescrType);
#ifdef DILIGENT_DEBUG
    const VkAccessFlags RequiredAccessFlags = (RequiredState == RESOURCE_STATE_SHADER_RESOURCE) ?
        VK_ACCESS_SHADER_READ_BIT :
        (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    VERIFY_EXPR((ResourceStateFlagsToVkAccessFlags(RequiredState) & RequiredAccessFlags) == RequiredAccessFlags);
#endif
    const bool IsInRequiredState = pBufferVk->CheckState(RequiredState);

    if (VerifyOnly)
    {
        if (!IsInRequiredState)
        {
            LOG_ERROR_MESSAGE("State of buffer '", pBufferVk->GetDesc().Name, "' is incorrect. Required state: ",
                              GetResourceStateString(RequiredState), ". Actual state: ",
                              GetResourceStateString(pBufferVk->GetState()),
                              ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                              "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                              "with IDeviceContext::TransitionResourceStates().");
        }
    }
    else
    {
        // When both old and new states are RESOURCE_STATE_UNORDERED_ACCESS, we need to execute UAV barrier
        // to make sure that all UAV writes are complete and visible.
        if (!IsInRequiredState || RequiredState == RESOURCE_STATE_UNORDERED_ACCESS)
        {
            pCtxVkImpl->TransitionBufferState(*pBufferVk, RESOURCE_STATE_UNKNOWN, RequiredState, true);
        }
        VERIFY_EXPR(pBufferVk->CheckAccessFlags(RequiredAccessFlags));
    }
}

template <bool VerifyOnly>
inline void TransitionTextureView(DeviceContextVkImpl* pCtxVkImpl,
                                  TextureViewVkImpl*   pTextureViewVk,
                                  DescriptorType       DescrType)
{
    if (pTextureViewVk == nullptr)
        return;

    TextureVkImpl* pTextureVk = pTextureViewVk->GetTexture<TextureVkImpl>();
    if (!pTextureVk->IsInKnownState())
        return;

    // The image subresources for a storage image must be in the VK_IMAGE_LAYOUT_GENERAL layout in
    // order to access its data in a shader (13.1.1)
    // The image subresources for a sampled image or a combined image sampler must be in the
    // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    // or VK_IMAGE_LAYOUT_GENERAL layout in order to access its data in a shader (13.1.3, 13.1.4).
    RESOURCE_STATE RequiredState;
    if (DescrType == DescriptorType::StorageImage)
    {
        RequiredState = RESOURCE_STATE_UNORDERED_ACCESS;
        VERIFY_EXPR(ResourceStateToVkImageLayout(RequiredState) == VK_IMAGE_LAYOUT_GENERAL);
    }
    else
    {
        if (pTextureVk->GetDesc().BindFlags & BIND_DEPTH_STENCIL)
        {
            // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL must only be used as a read - only depth / stencil attachment
            // in a VkFramebuffer and/or as a read - only image in a shader (which can be read as a sampled image, combined
            // image / sampler and /or input attachment). This layout is valid only for image subresources of images created
            // with the VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT usage bit enabled. (11.4)
            RequiredState = RESOURCE_STATE_DEPTH_READ;
            VERIFY_EXPR(ResourceStateToVkImageLayout(RequiredState) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        }
        else
        {
            RequiredState = RESOURCE_STATE_SHADER_RESOURCE;
            VERIFY_EXPR(ResourceStateToVkImageLayout(RequiredState) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    const bool IsInRequiredState = pTextureVk->CheckState(RequiredState);

    if (VerifyOnly)
    {
        if (!IsInRequiredState)
        {
            LOG_ERROR_MESSAGE("State of texture '", pTextureVk->GetDesc().Name, "' is incorrect. Required state: ",
                              GetResourceStateString(RequiredState), ". Actual state: ",
                              GetResourceStateString(pTextureVk->GetState()),
                              ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                              "when calling IDeviceContext::CommitShaderResources() or explicitly transition the texture state "
                              "with IDeviceContext::TransitionResourceStates().");
        }
    }
    else
    {
        // When both old and new states are RESOURCE_STATE_UNORDERED_ACCESS, we need to execute UAV barrier
        // to make sure that all UAV writes are complete and visible.
        if (!IsInRequiredState || RequiredState == RESOURCE_STATE_UNORDERED_ACCESS)
        {
            pCtxVkImpl->TransitionTextureState(*pTextureVk, RESOURCE_STATE_UNKNOWN, RequiredState, STATE_TRANSITION_FLAG_UPDATE_STATE);
        }
    }
}

template <bool VerifyOnly>
inline void TransitionAccelStruct(DeviceContextVkImpl* pCtxVkImpl,
                                  TopLevelASVkImpl*    pTLASVk,
                                  DescriptorType       DescrType)
{
    if (pTLASVk == nullptr || !pTLASVk->IsInKnownState())
        return;

    constexpr RESOURCE_STATE RequiredState = RESOURCE_STATE_RAY_TRACING;
    VERIFY_EXPR(DescriptorTypeToResourceState(DescrType) == RequiredState);
    const bool IsInRequiredState = pTLASVk->CheckState(RequiredState);
    if (VerifyOnly)
    {
        if (!IsInRequiredState)
        {
            LOG_ERROR_MESSAGE("State of TLAS '", pTLASVk->GetDesc().Name, "' is incorrect. Required state: ",
                              GetResourceStateString(RequiredState), ". Actual state: ",
                              GetResourceStateString(pTLASVk->GetState()),
                              ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                              "when calling IDeviceContext::CommitShaderResources() or explicitly transition the TLAS state "
                              "with IDeviceContext::TransitionResourceStates().");
        }
    }
    else
    {
        if (!IsInRequiredState)
        {
            pCtxVkImpl->TransitionTLASState(*pTLASVk, RESOURCE_STATE_UNKNOWN, RequiredState, true);
        }
    }

#ifdef DILIGENT_DEVELOPMENT
    pTLASVk->ValidateContent();
#endif
}

} // namespace

template <bool VerifyOnly>
void ShaderResourceCacheVk::TransitionResources(DeviceContextVkImpl* pCtxVkImpl)
{
    Resource* pResources = GetFirstResourcePtr();
    for (Uint32 res = 0; res < m_TotalResources; ++res)
    {
        Resource& Res = pResources[res];

        static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
        switch (Res.Type)
        {
            case DescriptorType::UniformBuffer:
            case DescriptorType::UniformBufferDynamic:
                TransitionUniformBuffer<VerifyOnly>(pCtxVkImpl, Res.pObject.RawPtr<BufferVkImpl>(), Res.Type);
                break;

            case DescriptorType::StorageBuffer:
            case DescriptorType::StorageBufferDynamic:
            case DescriptorType::StorageBuffer_ReadOnly:
            case DescriptorType::StorageBufferDynamic_ReadOnly:
            case DescriptorType::UniformTexelBuffer:
            case DescriptorType::StorageTexelBuffer:
            case DescriptorType::StorageTexelBuffer_ReadOnly:
                TransitionBufferView<VerifyOnly>(pCtxVkImpl, Res.pObject.RawPtr<BufferViewVkImpl>(), Res.Type);
                break;

            case DescriptorType::CombinedImageSampler:
            case DescriptorType::SeparateImage:
            case DescriptorType::StorageImage:
                TransitionTextureView<VerifyOnly>(pCtxVkImpl, Res.pObject.RawPtr<TextureViewVkImpl>(), Res.Type);
                break;

            case DescriptorType::Sampler:
                // Nothing to do with samplers
                break;

            case DescriptorType::InputAttachment:
            case DescriptorType::InputAttachment_General:
                // Nothing to do with input attachments - they are transitioned by the render pass.
                // There is nothing we can validate here - a texture may be in different state at
                // the beginning of the render pass before being transitioned to INPUT_ATTACHMENT state.
                break;

            case DescriptorType::AccelerationStructure:
                TransitionAccelStruct<VerifyOnly>(pCtxVkImpl, Res.pObject.RawPtr<TopLevelASVkImpl>(), Res.Type);
                break;

            default: UNEXPECTED("Unexpected resource type");
        }
    }
}

template void ShaderResourceCacheVk::TransitionResources<false>(DeviceContextVkImpl* pCtxVkImpl);
template void ShaderResourceCacheVk::TransitionResources<true>(DeviceContextVkImpl* pCtxVkImpl);


VkDescriptorBufferInfo ShaderResourceCacheVk::Resource::GetUniformBufferDescriptorWriteInfo() const
{
    VERIFY((Type == DescriptorType::UniformBuffer ||
            Type == DescriptorType::UniformBufferDynamic),
           "Uniform buffer resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get uniform buffer write info: cached object is null");

    const BufferVkImpl* pBuffVk = pObject.ConstPtr<BufferVkImpl>();
    // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC descriptor type require
    // buffer to be created with VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    VERIFY(Type == DescriptorType::UniformBufferDynamic || pBuffVk->GetDesc().Usage != USAGE_DYNAMIC,
           "Dynamic buffer must be used with UniformBufferDynamic descriptor");

    VkDescriptorBufferInfo DescrBuffInfo;
    DescrBuffInfo.buffer = pBuffVk->GetVkBuffer();
    // If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, the offset member
    // of each element of pBufferInfo must be a multiple of VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment (13.2.4)
    VERIFY_EXPR(BufferBaseOffset + BufferRangeSize <= pBuffVk->GetDesc().Size);
    DescrBuffInfo.offset = BufferBaseOffset;
    DescrBuffInfo.range  = BufferRangeSize;
    return DescrBuffInfo;
}

VkDescriptorBufferInfo ShaderResourceCacheVk::Resource::GetStorageBufferDescriptorWriteInfo() const
{
    VERIFY((Type == DescriptorType::StorageBuffer ||
            Type == DescriptorType::StorageBufferDynamic ||
            Type == DescriptorType::StorageBuffer_ReadOnly ||
            Type == DescriptorType::StorageBufferDynamic_ReadOnly),
           "Storage buffer resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get storage buffer write info: cached object is null");

    const BufferViewVkImpl* pBuffViewVk = pObject.ConstPtr<BufferViewVkImpl>();
    const BufferVkImpl*     pBuffVk     = pBuffViewVk->GetBuffer<const BufferVkImpl>();
    VERIFY(Type == DescriptorType::StorageBufferDynamic || Type == DescriptorType::StorageBufferDynamic_ReadOnly || pBuffVk->GetDesc().Usage != USAGE_DYNAMIC,
           "Dynamic buffer must be used with StorageBufferDynamic or StorageBufferDynamic_ReadOnly descriptor");

    VkDescriptorBufferInfo DescrBuffInfo;
    DescrBuffInfo.buffer = pBuffVk->GetVkBuffer();
    // If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, the offset member
    // of each element of pBufferInfo must be a multiple of VkPhysicalDeviceLimits::minStorageBufferOffsetAlignment (13.2.4)
    DescrBuffInfo.offset = BufferBaseOffset;
    DescrBuffInfo.range  = BufferRangeSize;
    return DescrBuffInfo;
}

VkDescriptorImageInfo ShaderResourceCacheVk::Resource::GetImageDescriptorWriteInfo() const
{
    VERIFY((Type == DescriptorType::StorageImage ||
            Type == DescriptorType::SeparateImage ||
            Type == DescriptorType::CombinedImageSampler),
           "Storage image, separate image or sampled image resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get image descriptor write info: cached object is null");

    bool IsStorageImage = (Type == DescriptorType::StorageImage);

    const TextureViewVkImpl* pTexViewVk = pObject.ConstPtr<TextureViewVkImpl>();
    VERIFY_EXPR(pTexViewVk->GetDesc().ViewType == (IsStorageImage ? TEXTURE_VIEW_UNORDERED_ACCESS : TEXTURE_VIEW_SHADER_RESOURCE));

    VkDescriptorImageInfo DescrImgInfo;
    DescrImgInfo.sampler = VK_NULL_HANDLE;
    VERIFY(Type == DescriptorType::CombinedImageSampler || !HasImmutableSampler,
           "Immutable sampler can't be assigned to separate image or storage image");
    if (Type == DescriptorType::CombinedImageSampler && !HasImmutableSampler)
    {
        // Immutable samplers are permanently bound into the set layout; later binding a sampler
        // into an immutable sampler slot in a descriptor set is not allowed (13.2.1)
        const SamplerVkImpl* pSamplerVk = pTexViewVk->GetSampler<const SamplerVkImpl>();
        if (pSamplerVk != nullptr)
        {
            // If descriptorType is VK_DESCRIPTOR_TYPE_SAMPLER or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            // and dstSet was not allocated with a layout that included immutable samplers for dstBinding with
            // descriptorType, the sampler member of each element of pImageInfo must be a valid VkSampler
            // object (13.2.4)
            DescrImgInfo.sampler = pSamplerVk->GetVkSampler();
        }
#ifdef DILIGENT_DEVELOPMENT
        else
        {
            LOG_ERROR_MESSAGE("No sampler is assigned to texture view '", pTexViewVk->GetDesc().Name, "'");
        }
#endif
    }
    DescrImgInfo.imageView = pTexViewVk->GetVulkanImageView();

    // If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, for each descriptor that will be accessed
    // via load or store operations the imageLayout member for corresponding elements of pImageInfo
    // MUST be VK_IMAGE_LAYOUT_GENERAL (13.2.4)
    if (IsStorageImage)
        DescrImgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    else
    {
        if (ClassPtrCast<const TextureVkImpl>(pTexViewVk->GetTexture())->GetDesc().BindFlags & BIND_DEPTH_STENCIL)
            DescrImgInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else
            DescrImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return DescrImgInfo;
}

VkBufferView ShaderResourceCacheVk::Resource::GetBufferViewWriteInfo() const
{
    VERIFY((Type == DescriptorType::UniformTexelBuffer ||
            Type == DescriptorType::StorageTexelBuffer ||
            Type == DescriptorType::StorageTexelBuffer_ReadOnly),
           "Uniform or storage buffer resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get buffer view write info: cached object is null");

    // The following bits must have been set at buffer creation time:
    //  * VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  ->  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
    //  * VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  ->  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
    const BufferViewVkImpl* pBuffViewVk = pObject.ConstPtr<BufferViewVkImpl>();
    return pBuffViewVk->GetVkBufferView();
}

VkDescriptorImageInfo ShaderResourceCacheVk::Resource::GetSamplerDescriptorWriteInfo() const
{
    VERIFY(Type == DescriptorType::Sampler, "Separate sampler resource is expected");
    VERIFY(!HasImmutableSampler, "Separate immutable samplers can't be updated");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get separate sampler descriptor write info: cached object is null");

    const SamplerVkImpl* pSamplerVk = pObject.ConstPtr<SamplerVkImpl>();

    VkDescriptorImageInfo DescrImgInfo;
    // For VK_DESCRIPTOR_TYPE_SAMPLER, only the sample member of each element of VkWriteDescriptorSet::pImageInfo is accessed (13.2.4)
    DescrImgInfo.sampler     = pSamplerVk->GetVkSampler();
    DescrImgInfo.imageView   = VK_NULL_HANDLE;
    DescrImgInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return DescrImgInfo;
}

VkDescriptorImageInfo ShaderResourceCacheVk::Resource::GetInputAttachmentDescriptorWriteInfo() const
{
    VERIFY((Type == DescriptorType::InputAttachment ||
            Type == DescriptorType::InputAttachment_General),
           "Input attachment resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get input attachment write info: cached object is null");

    const TextureViewVkImpl* pTexViewVk = pObject.ConstPtr<TextureViewVkImpl>();
    VERIFY_EXPR(pTexViewVk->GetDesc().ViewType == TEXTURE_VIEW_SHADER_RESOURCE);

    VkDescriptorImageInfo DescrImgInfo;
    DescrImgInfo.sampler     = VK_NULL_HANDLE;
    DescrImgInfo.imageView   = pTexViewVk->GetVulkanImageView();
    DescrImgInfo.imageLayout = Type == DescriptorType::InputAttachment_General ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return DescrImgInfo;
}

VkWriteDescriptorSetAccelerationStructureKHR ShaderResourceCacheVk::Resource::GetAccelerationStructureWriteInfo() const
{
    VERIFY(Type == DescriptorType::AccelerationStructure, "Acceleration structure resource is expected");
    DEV_CHECK_ERR(pObject != nullptr, "Unable to get acceleration structure write info: cached object is null");

    const TopLevelASVkImpl* pTLASVk = pObject.ConstPtr<TopLevelASVkImpl>();

    VkWriteDescriptorSetAccelerationStructureKHR DescrAS;
    DescrAS.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    DescrAS.pNext                      = nullptr;
    DescrAS.accelerationStructureCount = 1;
    DescrAS.pAccelerationStructures    = pTLASVk->GetVkTLASPtr();

    return DescrAS;
}

} // namespace Diligent
