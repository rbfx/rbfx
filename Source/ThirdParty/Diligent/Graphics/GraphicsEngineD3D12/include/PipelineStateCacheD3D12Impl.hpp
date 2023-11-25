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

#pragma once

/// \file
/// Declaration of Diligent::PipelineStateCacheD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "PipelineStateCacheBase.hpp"

namespace Diligent
{

/// Pipeline state cache implementation in Direct3D12 backend.
class PipelineStateCacheD3D12Impl final : public PipelineStateCacheBase<EngineD3D12ImplTraits>
{
public:
    using TPipelineStateCacheBase = PipelineStateCacheBase<EngineD3D12ImplTraits>;

    PipelineStateCacheD3D12Impl(IReferenceCounters*                 pRefCounters,
                                RenderDeviceD3D12Impl*              pDevice,
                                const PipelineStateCacheCreateInfo& CreateInfo);
    ~PipelineStateCacheD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_PipelineStateCacheD3D12, TPipelineStateCacheBase)

    /// Implementation of IPipelineStateCache::GetData().
    virtual void DILIGENT_CALL_TYPE GetData(IDataBlob** ppBlob) override final;

    CComPtr<ID3D12DeviceChild> LoadComputePipeline(const wchar_t* Name, const D3D12_COMPUTE_PIPELINE_STATE_DESC& Desc);
    CComPtr<ID3D12DeviceChild> LoadGraphicsPipeline(const wchar_t* Name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& Desc);

    bool StorePipeline(const wchar_t* Name, ID3D12DeviceChild* pPSO);

private:
    CComPtr<ID3D12PipelineLibrary> m_pLibrary;
};

} // namespace Diligent
