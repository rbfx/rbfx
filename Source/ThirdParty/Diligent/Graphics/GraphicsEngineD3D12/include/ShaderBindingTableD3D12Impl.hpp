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
/// Declaration of Diligent::ShaderBindingTableD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "ShaderBindingTableBase.hpp"
#include "TopLevelASD3D12Impl.hpp"
#include "D3D12ResourceBase.hpp"

namespace Diligent
{

/// Shader binding table object implementation in Direct3D12 backend.
class ShaderBindingTableD3D12Impl final : public ShaderBindingTableBase<EngineD3D12ImplTraits>, public D3D12ResourceBase
{
public:
    using TShaderBindingTableBase = ShaderBindingTableBase<EngineD3D12ImplTraits>;

    ShaderBindingTableD3D12Impl(IReferenceCounters*           pRefCounters,
                                class RenderDeviceD3D12Impl*  pDeviceD3D12,
                                const ShaderBindingTableDesc& Desc,
                                bool                          bIsDeviceInternal = false);
    ~ShaderBindingTableD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ShaderBindingTableD3D12, TShaderBindingTableBase)

    virtual const D3D12_DISPATCH_RAYS_DESC& DILIGENT_CALL_TYPE GetD3D12BindingTable() const override final { return m_d3d12DispatchDesc; }

    using BindingTable = TShaderBindingTableBase::BindingTable;
    void GetData(BufferD3D12Impl*& pSBTBufferD3D12,
                 BindingTable&     RayGenShaderRecord,
                 BindingTable&     MissShaderTable,
                 BindingTable&     HitGroupTable,
                 BindingTable&     CallableShaderTable);

private:
    D3D12_DISPATCH_RAYS_DESC m_d3d12DispatchDesc = {};
};

} // namespace Diligent
