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
/// Declaration of Diligent::BottomLevelASD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "BottomLevelASBase.hpp"
#include "D3D12ResourceBase.hpp"

namespace Diligent
{

/// Bottom-level acceleration structure object implementation in Direct3D12 backend.
class BottomLevelASD3D12Impl final : public BottomLevelASBase<EngineD3D12ImplTraits>, public D3D12ResourceBase
{
public:
    using TBottomLevelASBase = BottomLevelASBase<EngineD3D12ImplTraits>;

    BottomLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                           class RenderDeviceD3D12Impl* pDeviceD3D12,
                           const BottomLevelASDesc&     Desc);
    BottomLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                           class RenderDeviceD3D12Impl* pDeviceD3D12,
                           const BottomLevelASDesc&     Desc,
                           RESOURCE_STATE               InitialState,
                           ID3D12Resource*              pd3d12BLAS);
    ~BottomLevelASD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BottomLevelASD3D12, TBottomLevelASBase)

    /// Implementation of IBottomLevelASD3D12::GetD3D12BLAS().
    virtual ID3D12Resource* DILIGENT_CALL_TYPE GetD3D12BLAS() override final { return GetD3D12Resource(); }

    /// Implementation of IBottomLevelAS::GetNativeHandle() in Direct3D12 backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetD3D12BLAS()); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress()
    {
        return GetD3D12Resource()->GetGPUVirtualAddress();
    }
};

} // namespace Diligent
