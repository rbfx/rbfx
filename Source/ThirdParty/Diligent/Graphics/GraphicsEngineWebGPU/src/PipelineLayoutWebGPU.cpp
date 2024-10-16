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

#include "PipelineLayoutWebGPU.hpp"

#include <algorithm>
#include <limits>

#include "RenderDeviceWebGPUImpl.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"

#include "StringTools.hpp"

namespace Diligent
{

PipelineLayoutWebGPU::PipelineLayoutWebGPU()
{
    m_FirstBindGroupIndex.fill(std::numeric_limits<FirstBindGroupIndexArrayType::value_type>::max());
}

PipelineLayoutWebGPU::~PipelineLayoutWebGPU()
{
}

struct PipelineLayoutWebGPU::WGPUPipelineLayoutCreateInfo
{
    RefCntAutoPtr<RenderDeviceWebGPUImpl>                           DeviceWebGPU;
    std::vector<RefCntAutoPtr<PipelineResourceSignatureWebGPUImpl>> Signatures;
};

void PipelineLayoutWebGPU::Create(RenderDeviceWebGPUImpl* pDeviceWebGPU, RefCntAutoPtr<PipelineResourceSignatureWebGPUImpl> ppSignatures[], Uint32 SignatureCount) noexcept(false)
{
    VERIFY(m_BindGroupCount == 0 && !m_wgpuPipelineLayout, "This pipeline layout is already initialized");

    m_PipelineLayoutCreateInfo               = std::make_unique<WGPUPipelineLayoutCreateInfo>();
    m_PipelineLayoutCreateInfo->DeviceWebGPU = pDeviceWebGPU;

    Uint32 BindGroupLayoutCount      = 0;
    Uint32 DynamicUniformBufferCount = 0;
    Uint32 DynamicStorageBufferCount = 0;

    m_PipelineLayoutCreateInfo->Signatures.reserve(SignatureCount);
    for (Uint32 BindInd = 0; BindInd < SignatureCount; ++BindInd)
    {
        // Signatures are arranged by binding index by PipelineStateBase::CopyResourceSignatures
        const auto& pSignature = ppSignatures[BindInd];
        if (pSignature == nullptr)
            continue;

        m_PipelineLayoutCreateInfo->Signatures.push_back(pSignature);

        VERIFY(BindGroupLayoutCount <= std::numeric_limits<FirstBindGroupIndexArrayType::value_type>::max(),
               "Bind group layout count (", BindGroupLayoutCount, ") exceeds the maximum representable value");
        m_FirstBindGroupIndex[BindInd] = static_cast<FirstBindGroupIndexArrayType::value_type>(BindGroupLayoutCount);

        for (auto GroupId : {PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_STATIC_MUTABLE, PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_DYNAMIC})
        {
            if (pSignature->HasBindGroup(GroupId))
                ++BindGroupLayoutCount;
        }

        DynamicUniformBufferCount += pSignature->GetDynamicUniformBufferCount();
        DynamicStorageBufferCount += pSignature->GetDynamicStorageBufferCount();
#ifdef DILIGENT_DEBUG
        m_DbgMaxBindIndex = std::max(m_DbgMaxBindIndex, Uint32{pSignature->GetDesc().BindingIndex});
#endif
    }
    VERIFY_EXPR(BindGroupLayoutCount <= MAX_RESOURCE_SIGNATURES * 2);

    const WGPULimits& Limits = pDeviceWebGPU->GetLimits();

    if (BindGroupLayoutCount > Limits.maxBindGroups)
    {
        LOG_ERROR_AND_THROW("The total number of descriptor sets (", BindGroupLayoutCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxBindGroups, ")");
    }

    if (DynamicUniformBufferCount > Limits.maxDynamicUniformBuffersPerPipelineLayout)
    {
        LOG_ERROR_AND_THROW("The number of dynamic uniform buffers  (", DynamicUniformBufferCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxDynamicUniformBuffersPerPipelineLayout, ")");
    }

    if (DynamicStorageBufferCount > Limits.maxDynamicStorageBuffersPerPipelineLayout)
    {
        LOG_ERROR_AND_THROW("The number of dynamic storage buffers (", DynamicStorageBufferCount,
                            ") used by the pipeline layout exceeds device limit (", Limits.maxDynamicStorageBuffersPerPipelineLayout, ")");
    }

    VERIFY(m_BindGroupCount <= std::numeric_limits<decltype(m_BindGroupCount)>::max(),
           "Descriptor set count (", BindGroupLayoutCount, ") exceeds the maximum representable value");

    m_BindGroupCount = static_cast<Uint8>(BindGroupLayoutCount);
}

WGPUPipelineLayout PipelineLayoutWebGPU::GetWebGPUPipelineLayout()
{
    if (m_PipelineLayoutCreateInfo)
    {
        std::array<WGPUBindGroupLayout, MAX_RESOURCE_SIGNATURES * PipelineResourceSignatureWebGPUImpl::MAX_BIND_GROUPS> BindGroupLayouts{};

        Uint32 BindGroupLayoutCount = 0;
        for (auto& Signature : m_PipelineLayoutCreateInfo->Signatures)
        {
            VERIFY_EXPR(Signature);
            for (auto GroupId : {PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_STATIC_MUTABLE, PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_DYNAMIC})
            {
                if (Signature->HasBindGroup(GroupId))
                    BindGroupLayouts[BindGroupLayoutCount++] = Signature->GetWGPUBindGroupLayout(GroupId);
            }
        }
        VERIFY_EXPR(BindGroupLayoutCount == m_BindGroupCount);

        WGPUPipelineLayoutDescriptor LayoutDescr{};
        LayoutDescr.label                = "Diligent::PipelineLayoutWebGPU";
        LayoutDescr.bindGroupLayoutCount = BindGroupLayoutCount;
        LayoutDescr.bindGroupLayouts     = BindGroupLayoutCount ? BindGroupLayouts.data() : nullptr;

        m_wgpuPipelineLayout.Reset(wgpuDeviceCreatePipelineLayout(m_PipelineLayoutCreateInfo->DeviceWebGPU->GetWebGPUDevice(), &LayoutDescr));
        VERIFY_EXPR(m_wgpuPipelineLayout);

        m_PipelineLayoutCreateInfo.reset();
    }

    return m_wgpuPipelineLayout;
}

} // namespace Diligent
