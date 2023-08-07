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
#include "PipelineStateCacheD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "DataBlobImpl.hpp"
#include "StringTools.hpp"

namespace Diligent
{

PipelineStateCacheD3D12Impl::PipelineStateCacheD3D12Impl(IReferenceCounters*                 pRefCounters,
                                                         RenderDeviceD3D12Impl*              pRenderDeviceD3D12,
                                                         const PipelineStateCacheCreateInfo& CreateInfo) :
    // clang-format off
    TPipelineStateCacheBase
    {
        pRefCounters,
        pRenderDeviceD3D12,
        CreateInfo,
        false
    }
// clang-format on
{
    auto hr = pRenderDeviceD3D12->GetD3D12Device1()->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&m_pLibrary));
    if (FAILED(hr))
        LOG_ERROR_AND_THROW("Failed to create D3D12 pipeline library");
}

PipelineStateCacheD3D12Impl::~PipelineStateCacheD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    GetDevice()->SafeReleaseDeviceObject(std::move(m_pLibrary), ~Uint64{0});
}

CComPtr<ID3D12DeviceChild> PipelineStateCacheD3D12Impl::LoadComputePipeline(const wchar_t* Name, const D3D12_COMPUTE_PIPELINE_STATE_DESC& Desc)
{
    if (Name == nullptr)
    {
        DEV_ERROR("Pipeline name must not be null");
        return {};
    }

    CComPtr<ID3D12DeviceChild> d3d12PSO;
    if ((m_Desc.Mode & PSO_CACHE_MODE_LOAD) != 0)
    {
        auto hr = m_pLibrary->LoadComputePipeline(Name, &Desc, IID_PPV_ARGS(&d3d12PSO));
        if (FAILED(hr) && (m_Desc.Flags & PSO_CACHE_FLAG_VERBOSE) != 0)
            LOG_ERROR_MESSAGE("Failed to load compute pipeline '", NarrowString(Name), "' from the library");
    }
    return d3d12PSO;
}

CComPtr<ID3D12DeviceChild> PipelineStateCacheD3D12Impl::LoadGraphicsPipeline(const wchar_t* Name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& Desc)
{
    if (Name == nullptr)
    {
        DEV_ERROR("Pipeline name must not be null");
        return {};
    }

    CComPtr<ID3D12DeviceChild> d3d12PSO;
    if ((m_Desc.Mode & PSO_CACHE_MODE_LOAD) != 0)
    {
        auto hr = m_pLibrary->LoadGraphicsPipeline(Name, &Desc, IID_PPV_ARGS(&d3d12PSO));
        if (FAILED(hr) && (m_Desc.Flags & PSO_CACHE_FLAG_VERBOSE) != 0)
            LOG_ERROR_MESSAGE("Failed to load graphics pipeline '", NarrowString(Name), "' from the library");
    }
    return d3d12PSO;
}

bool PipelineStateCacheD3D12Impl::StorePipeline(const wchar_t* Name, ID3D12DeviceChild* pPSO)
{
    VERIFY_EXPR(Name != nullptr);
    if ((m_Desc.Mode & PSO_CACHE_MODE_STORE) == 0)
        return false;

    auto hr = m_pLibrary->StorePipeline(Name, static_cast<ID3D12PipelineState*>(pPSO));
    if (FAILED(hr) && (m_Desc.Flags & PSO_CACHE_FLAG_VERBOSE) != 0)
        LOG_ERROR_MESSAGE("Failed to add pipeline '", NarrowString(Name), "' to the library");

    return SUCCEEDED(hr);
}

void PipelineStateCacheD3D12Impl::GetData(IDataBlob** ppBlob)
{
    DEV_CHECK_ERR(ppBlob != nullptr, "ppBlob must not be null");
    *ppBlob = nullptr;

    auto pDataBlob = DataBlobImpl::Create(m_pLibrary->GetSerializedSize());

    auto hr = m_pLibrary->Serialize(pDataBlob->GetDataPtr(), pDataBlob->GetSize());
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("Failed to serialize D3D12 pipeline library");
        return;
    }

    *ppBlob = pDataBlob.Detach();
}

} // namespace Diligent
