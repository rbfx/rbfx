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

#include "ShaderBindingTableD3D12Impl.hpp"

#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"

#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "DXGITypeConversions.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"

namespace Diligent
{

ShaderBindingTableD3D12Impl::ShaderBindingTableD3D12Impl(IReferenceCounters*           pRefCounters,
                                                         class RenderDeviceD3D12Impl*  pDeviceD3D12,
                                                         const ShaderBindingTableDesc& Desc,
                                                         bool                          bIsDeviceInternal) :
    TShaderBindingTableBase{pRefCounters, pDeviceD3D12, Desc, bIsDeviceInternal}
{
}

ShaderBindingTableD3D12Impl::~ShaderBindingTableD3D12Impl()
{
}

void ShaderBindingTableD3D12Impl::GetData(BufferD3D12Impl*& pSBTBufferD3D12,
                                          BindingTable&     RayGenShaderRecord,
                                          BindingTable&     MissShaderTable,
                                          BindingTable&     HitGroupTable,
                                          BindingTable&     CallableShaderTable)
{
    TShaderBindingTableBase::GetData(pSBTBufferD3D12, RayGenShaderRecord, MissShaderTable, HitGroupTable, CallableShaderTable);

    m_d3d12DispatchDesc.RayGenerationShaderRecord.StartAddress = pSBTBufferD3D12->GetGPUAddress() + RayGenShaderRecord.Offset;
    m_d3d12DispatchDesc.RayGenerationShaderRecord.SizeInBytes  = RayGenShaderRecord.Size;

    m_d3d12DispatchDesc.MissShaderTable.StartAddress  = pSBTBufferD3D12->GetGPUAddress() + MissShaderTable.Offset;
    m_d3d12DispatchDesc.MissShaderTable.SizeInBytes   = MissShaderTable.Size;
    m_d3d12DispatchDesc.MissShaderTable.StrideInBytes = MissShaderTable.Stride;

    m_d3d12DispatchDesc.HitGroupTable.StartAddress  = pSBTBufferD3D12->GetGPUAddress() + HitGroupTable.Offset;
    m_d3d12DispatchDesc.HitGroupTable.SizeInBytes   = HitGroupTable.Size;
    m_d3d12DispatchDesc.HitGroupTable.StrideInBytes = HitGroupTable.Stride;

    m_d3d12DispatchDesc.CallableShaderTable.StartAddress  = pSBTBufferD3D12->GetGPUAddress() + CallableShaderTable.Offset;
    m_d3d12DispatchDesc.CallableShaderTable.SizeInBytes   = CallableShaderTable.Size;
    m_d3d12DispatchDesc.CallableShaderTable.StrideInBytes = CallableShaderTable.Stride;

    VERIFY_EXPR(m_d3d12DispatchDesc.RayGenerationShaderRecord.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0);
    VERIFY_EXPR(m_d3d12DispatchDesc.MissShaderTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0);
    VERIFY_EXPR(m_d3d12DispatchDesc.HitGroupTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0);
    VERIFY_EXPR(m_d3d12DispatchDesc.CallableShaderTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0);
}

} // namespace Diligent
