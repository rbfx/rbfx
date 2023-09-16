/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
#include "PipelineStateCacheVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "DataBlobImpl.hpp"

namespace Diligent
{

PipelineStateCacheVkImpl::PipelineStateCacheVkImpl(IReferenceCounters*                 pRefCounters,
                                                   RenderDeviceVkImpl*                 pRenderDeviceVk,
                                                   const PipelineStateCacheCreateInfo& CreateInfo) :
    // clang-format off
    TPipelineStateCacheBase
    {
        pRefCounters,
        pRenderDeviceVk,
        CreateInfo,
        false
    }
// clang-format on
{
    // Separate load/store is not supported in Vulkan.
    m_Desc.Mode |= PSO_CACHE_MODE_LOAD | PSO_CACHE_MODE_STORE;

    VkPipelineCacheCreateInfo VkPipelineStateCacheCI{};
    VkPipelineStateCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (CreateInfo.pCacheData != nullptr && CreateInfo.CacheDataSize > sizeof(VkPipelineCacheHeaderVersionOne))
    {
        const auto& Props = GetDevice()->GetPhysicalDevice().GetProperties();

        VkPipelineCacheHeaderVersionOne HeaderVersion;
        std::memcpy(&HeaderVersion, CreateInfo.pCacheData, sizeof(HeaderVersion));

        if (HeaderVersion.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
            HeaderVersion.headerSize == 32 && // from specs
            HeaderVersion.deviceID == Props.deviceID &&
            HeaderVersion.vendorID == Props.vendorID &&
            std::memcmp(HeaderVersion.pipelineCacheUUID, Props.pipelineCacheUUID, sizeof(HeaderVersion.pipelineCacheUUID)) == 0)
        {
            VkPipelineStateCacheCI.initialDataSize = CreateInfo.CacheDataSize;
            VkPipelineStateCacheCI.pInitialData    = CreateInfo.pCacheData;
        }
    }

    m_PipelineStateCache = m_pDevice->GetLogicalDevice().CreatePipelineCache(VkPipelineStateCacheCI, m_Desc.Name);
}

PipelineStateCacheVkImpl::~PipelineStateCacheVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    if (m_PipelineStateCache != VK_NULL_HANDLE)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_PipelineStateCache), ~Uint64{0});
}

void PipelineStateCacheVkImpl::GetData(IDataBlob** ppBlob)
{
    DEV_CHECK_ERR(ppBlob != nullptr, "ppBlob must not be null");
    *ppBlob = nullptr;

    const auto vkDevice = m_pDevice->GetLogicalDevice().GetVkDevice();

    size_t DataSize = 0;
    if (vkGetPipelineCacheData(vkDevice, m_PipelineStateCache, &DataSize, nullptr) != VK_SUCCESS)
        return;

    auto pDataBlob = DataBlobImpl::Create(DataSize);

    if (vkGetPipelineCacheData(vkDevice, m_PipelineStateCache, &DataSize, pDataBlob->GetDataPtr()) != VK_SUCCESS)
        return;

    *ppBlob = pDataBlob.Detach();
}

} // namespace Diligent
