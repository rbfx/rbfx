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

#pragma once

/// \file
/// Declaration of Diligent::TopLevelASD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "TopLevelASBase.hpp"
#include "D3D12ResourceBase.hpp"
#include "BottomLevelASD3D12Impl.hpp" // Required by TopLevelASBase
#include "DescriptorHeap.hpp"

namespace Diligent
{

/// Top-level acceleration structure object implementation in Direct3D12 backend.
class TopLevelASD3D12Impl final : public TopLevelASBase<EngineD3D12ImplTraits>, public D3D12ResourceBase
{
public:
    using TTopLevelASBase = TopLevelASBase<EngineD3D12ImplTraits>;

    TopLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                        class RenderDeviceD3D12Impl* pDeviceD3D12,
                        const TopLevelASDesc&        Desc);
    TopLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                        class RenderDeviceD3D12Impl* pDeviceD3D12,
                        const TopLevelASDesc&        Desc,
                        RESOURCE_STATE               InitialState,
                        ID3D12Resource*              pd3d12TLAS);
    ~TopLevelASD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TopLevelASD3D12, TTopLevelASBase)

    /// Implementation of ITopLevelASD3D12D3D12::GetD3D12TLAS().
    virtual ID3D12Resource* DILIGENT_CALL_TYPE GetD3D12TLAS() override final { return GetD3D12Resource(); }

    /// Implementation of ITopLevelASD3D12::GetNativeHandle() in Direct3D12 backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetD3D12TLAS()); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress()
    {
        return GetD3D12Resource()->GetGPUVirtualAddress();
    }

    /// Implementation of ITopLevelASD3D12::GetCPUDescriptorHandle() in Direct3D12 backend.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE DILIGENT_CALL_TYPE GetCPUDescriptorHandle() override final
    {
        return m_DescriptorHandle.GetCpuHandle();
    }

private:
    // Allocation in a CPU-only descriptor heap
    DescriptorHeapAllocation m_DescriptorHandle;
};

} // namespace Diligent
