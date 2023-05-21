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

#include "TopLevelASD3D12Impl.hpp"

#include "RenderDeviceD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "DXGITypeConversions.hpp"
#include "StringTools.hpp"

namespace Diligent
{

TopLevelASD3D12Impl::TopLevelASD3D12Impl(IReferenceCounters*    pRefCounters,
                                         RenderDeviceD3D12Impl* pDeviceD3D12,
                                         const TopLevelASDesc&  Desc) :
    TTopLevelASBase{pRefCounters, pDeviceD3D12, Desc}
{
    auto* const pd3d12Device = pDeviceD3D12->GetD3D12Device5();
    const auto& RTProps      = pDeviceD3D12->GetAdapterInfo().RayTracing;

    UINT64 ResultDataMaxSizeInBytes = 0;
    if (m_Desc.CompactedSize > 0)
    {
        ResultDataMaxSizeInBytes = m_Desc.CompactedSize;
    }
    else
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3d12TopLevelInputs{};
        d3d12TopLevelInputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        d3d12TopLevelInputs.Flags       = BuildASFlagsToD3D12ASBuildFlags(m_Desc.Flags);
        d3d12TopLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        d3d12TopLevelInputs.NumDescs    = m_Desc.MaxInstanceCount;

        DEV_CHECK_ERR(m_Desc.MaxInstanceCount <= RTProps.MaxInstancesPerTLAS,
                      "Max instance count (", m_Desc.MaxInstanceCount, ") exceeds device limit (", RTProps.MaxInstancesPerTLAS, ").");

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3d12TopLevelPrebuildInfo{};
        pd3d12Device->GetRaytracingAccelerationStructurePrebuildInfo(&d3d12TopLevelInputs, &d3d12TopLevelPrebuildInfo);
        if (d3d12TopLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
            LOG_ERROR_AND_THROW("Failed to get ray tracing acceleration structure prebuild info.");

        ResultDataMaxSizeInBytes = d3d12TopLevelPrebuildInfo.ResultDataMaxSizeInBytes;

        m_ScratchSize.Build  = d3d12TopLevelPrebuildInfo.ScratchDataSizeInBytes;
        m_ScratchSize.Update = d3d12TopLevelPrebuildInfo.UpdateScratchDataSizeInBytes;
    }

    D3D12_HEAP_PROPERTIES HeapProps{};
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC d3d12ASDesc{};
    d3d12ASDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12ASDesc.Alignment          = 0;
    d3d12ASDesc.Width              = ResultDataMaxSizeInBytes;
    d3d12ASDesc.Height             = 1;
    d3d12ASDesc.DepthOrArraySize   = 1;
    d3d12ASDesc.MipLevels          = 1;
    d3d12ASDesc.Format             = DXGI_FORMAT_UNKNOWN;
    d3d12ASDesc.SampleDesc.Count   = 1;
    d3d12ASDesc.SampleDesc.Quality = 0;
    d3d12ASDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d3d12ASDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                    &d3d12ASDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                    __uuidof(m_pd3d12Resource),
                                                    reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
    if (FAILED(hr))
        LOG_ERROR_AND_THROW("Failed to create D3D12 Top-level acceleration structure");

    if (m_Desc.Name != nullptr && *m_Desc.Name != '\0')
        m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

    m_DescriptorHandle = pDeviceD3D12->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC d3d12SRVDesc{};
    d3d12SRVDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    d3d12SRVDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    d3d12SRVDesc.Format                                   = DXGI_FORMAT_UNKNOWN;
    d3d12SRVDesc.RaytracingAccelerationStructure.Location = GetGPUAddress();
    pd3d12Device->CreateShaderResourceView(nullptr, &d3d12SRVDesc, m_DescriptorHandle.GetCpuHandle());

    DEV_CHECK_ERR(GetGPUAddress() % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0, "GPU virtual address is expected to be at least 256-byte aligned");

    SetState(RESOURCE_STATE_BUILD_AS_READ);
}

TopLevelASD3D12Impl::TopLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                                         class RenderDeviceD3D12Impl* pDeviceD3D12,
                                         const TopLevelASDesc&        Desc,
                                         RESOURCE_STATE               InitialState,
                                         ID3D12Resource*              pd3d12TLAS) :
    TTopLevelASBase{pRefCounters, pDeviceD3D12, Desc}
{
    m_pd3d12Resource = pd3d12TLAS;
    SetState(InitialState);
}

TopLevelASD3D12Impl::~TopLevelASD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    GetDevice()->SafeReleaseDeviceObject(std::move(m_pd3d12Resource), m_Desc.ImmediateContextMask);
}

} // namespace Diligent
