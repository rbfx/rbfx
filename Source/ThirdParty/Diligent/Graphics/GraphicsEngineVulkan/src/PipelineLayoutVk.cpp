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

#include "PipelineLayoutVk.hpp"

#include <algorithm>
#include <limits>

#include "RenderDeviceVkImpl.hpp"
#include "PipelineResourceSignatureVkImpl.hpp"

#include "VulkanTypeConversions.hpp"
#include "StringTools.hpp"

namespace Diligent
{

PipelineLayoutVk::PipelineLayoutVk()
{
    m_FirstDescrSetIndex.fill(std::numeric_limits<FirstDescrSetIndexArrayType::value_type>::max());
}

PipelineLayoutVk::~PipelineLayoutVk()
{
    VERIFY(!m_VkPipelineLayout, "Pipeline layout have not been released!");
}

void PipelineLayoutVk::Release(RenderDeviceVkImpl* pDeviceVk, Uint64 CommandQueueMask)
{
    if (m_VkPipelineLayout)
    {
        pDeviceVk->SafeReleaseDeviceObject(std::move(m_VkPipelineLayout), CommandQueueMask);
    }
}

void PipelineLayoutVk::Create(RenderDeviceVkImpl* pDeviceVk, RefCntAutoPtr<PipelineResourceSignatureVkImpl> ppSignatures[], Uint32 SignatureCount) noexcept(false)
{
    VERIFY(m_DescrSetCount == 0 && !m_VkPipelineLayout, "This pipeline layout is already initialized");

    std::array<VkDescriptorSetLayout, MAX_RESOURCE_SIGNATURES * PipelineResourceSignatureVkImpl::MAX_DESCRIPTOR_SETS> DescSetLayouts;

    Uint32 DescSetLayoutCount        = 0;
    Uint32 DynamicUniformBufferCount = 0;
    Uint32 DynamicStorageBufferCount = 0;

    for (Uint32 BindInd = 0; BindInd < SignatureCount; ++BindInd)
    {
        // Signatures are arranged by binding index by PipelineStateBase::CopyResourceSignatures
        const auto& pSignature = ppSignatures[BindInd];
        if (pSignature == nullptr)
            continue;

        VERIFY(DescSetLayoutCount <= std::numeric_limits<FirstDescrSetIndexArrayType::value_type>::max(),
               "Descriptor set layout count (", DescSetLayoutCount, ") exceeds the maximum representable value");
        m_FirstDescrSetIndex[BindInd] = static_cast<FirstDescrSetIndexArrayType::value_type>(DescSetLayoutCount);

        for (auto SetId : {PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE, PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC})
        {
            if (pSignature->HasDescriptorSet(SetId))
                DescSetLayouts[DescSetLayoutCount++] = pSignature->GetVkDescriptorSetLayout(SetId);
        }

        DynamicUniformBufferCount += pSignature->GetDynamicUniformBufferCount();
        DynamicStorageBufferCount += pSignature->GetDynamicStorageBufferCount();
#ifdef DILIGENT_DEBUG
        m_DbgMaxBindIndex = std::max(m_DbgMaxBindIndex, Uint32{pSignature->GetDesc().BindingIndex});
#endif
    }
    VERIFY_EXPR(DescSetLayoutCount <= MAX_RESOURCE_SIGNATURES * 2);

    const auto& Limits = pDeviceVk->GetPhysicalDevice().GetProperties().limits;
    if (DescSetLayoutCount > Limits.maxBoundDescriptorSets)
    {
        LOG_ERROR_AND_THROW("The total number of descriptor sets (", DescSetLayoutCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxBoundDescriptorSets, ")");
    }

    if (DynamicUniformBufferCount > Limits.maxDescriptorSetUniformBuffersDynamic)
    {
        LOG_ERROR_AND_THROW("The number of dynamic uniform buffers  (", DynamicUniformBufferCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxDescriptorSetUniformBuffersDynamic, ")");
    }

    if (DynamicStorageBufferCount > Limits.maxDescriptorSetStorageBuffersDynamic)
    {
        LOG_ERROR_AND_THROW("The number of dynamic storage buffers (", DynamicStorageBufferCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxDescriptorSetStorageBuffersDynamic, ")");
    }

    VERIFY(m_DescrSetCount <= std::numeric_limits<decltype(m_DescrSetCount)>::max(),
           "Descriptor set count (", DescSetLayoutCount, ") exceeds the maximum representable value");

    VkPipelineLayoutCreateInfo PipelineLayoutCI = {};

    PipelineLayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCI.pNext                  = nullptr;
    PipelineLayoutCI.flags                  = 0; // reserved for future use
    PipelineLayoutCI.setLayoutCount         = DescSetLayoutCount;
    PipelineLayoutCI.pSetLayouts            = DescSetLayoutCount ? DescSetLayouts.data() : nullptr;
    PipelineLayoutCI.pushConstantRangeCount = 0;
    PipelineLayoutCI.pPushConstantRanges    = nullptr;
    m_VkPipelineLayout                      = pDeviceVk->GetLogicalDevice().CreatePipelineLayout(PipelineLayoutCI);

    m_DescrSetCount = static_cast<Uint8>(DescSetLayoutCount);
}

} // namespace Diligent
