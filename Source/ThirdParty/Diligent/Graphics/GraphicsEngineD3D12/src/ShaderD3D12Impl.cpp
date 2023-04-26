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

#include "ShaderD3D12Impl.hpp"

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D12Impl.hpp"
#include "DataBlobImpl.hpp"

namespace Diligent
{

constexpr INTERFACE_ID ShaderD3D12Impl::IID_InternalImpl;

static ShaderVersion GetD3D12ShaderModel(const ShaderCreateInfo& ShaderCI,
                                         IDXCompiler*            pDXCompiler,
                                         const ShaderVersion&    DeviceSM)
{
    auto HLSLVersion = ShaderCI.HLSLVersion;
    if (HLSLVersion > DeviceSM)
    {
        LOG_WARNING_MESSAGE("Requested shader model ", Uint32{HLSLVersion.Major}, '_', Uint32{HLSLVersion.Minor},
                            " exceeds maximum version supported by device (",
                            Uint32{DeviceSM.Major}, '_', Uint32{DeviceSM.Minor}, ").");
    }

    auto MaxSupportedSM = DeviceSM;
    if (ShaderCI.Source != nullptr || ShaderCI.FilePath != nullptr)
    {
        ShaderVersion CompilerSM;
        if (ShaderCI.ShaderCompiler == SHADER_COMPILER_DXC)
        {
            if (pDXCompiler && pDXCompiler->IsLoaded())
            {
                CompilerSM = pDXCompiler->GetMaxShaderModel();
            }
            else
            {
                LOG_ERROR_MESSAGE("DXC compiler is not loaded");
                CompilerSM = ShaderVersion{5, 1};
            }
        }
        else
        {
            VERIFY(ShaderCI.ShaderCompiler == SHADER_COMPILER_FXC || ShaderCI.ShaderCompiler == SHADER_COMPILER_DEFAULT, "Unexpected compiler");
            // Direct3D12 supports shader model 5.1 on all feature levels.
            // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
            CompilerSM = ShaderVersion{5, 1};
        }

        if (HLSLVersion > CompilerSM)
        {
            LOG_WARNING_MESSAGE("Requested shader model ", Uint32{HLSLVersion.Major}, '_', Uint32{HLSLVersion.Minor},
                                " exceeds maximum version supported by compiler (",
                                Uint32{CompilerSM.Major}, '_', Uint32{CompilerSM.Minor}, ").");
        }

        MaxSupportedSM = ShaderVersion::Min(MaxSupportedSM, CompilerSM);
    }
    else
    {
        VERIFY(ShaderCI.ByteCode != nullptr, "ByteCode must not be null when both Source and FilePath are null");
    }

    return (HLSLVersion == ShaderVersion{0, 0}) ?
        MaxSupportedSM :
        ShaderVersion::Min(HLSLVersion, MaxSupportedSM);
}

ShaderD3D12Impl::ShaderD3D12Impl(IReferenceCounters*     pRefCounters,
                                 RenderDeviceD3D12Impl*  pRenderDeviceD3D12,
                                 const ShaderCreateInfo& ShaderCI,
                                 const CreateInfo&       D3D12ShaderCI,
                                 bool                    IsDeviceInternal) :
    // clang-format off
    TShaderBase
    {
        pRefCounters,
        pRenderDeviceD3D12,
        ShaderCI.Desc,
        D3D12ShaderCI.DeviceInfo,
        D3D12ShaderCI.AdapterInfo,
        IsDeviceInternal
    },
    ShaderD3DBase
    {
        ShaderCI,
        GetD3D12ShaderModel(ShaderCI, D3D12ShaderCI.pDXCompiler, D3D12ShaderCI.MaxShaderVersion),
        D3D12ShaderCI.pDXCompiler
    },
    m_EntryPoint{ShaderCI.EntryPoint}
// clang-format on
{
    // Load shader resources
    if ((ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_SKIP_REFLECTION) == 0)
    {
        auto& Allocator  = GetRawAllocator();
        auto* pRawMem    = ALLOCATE(Allocator, "Allocator for ShaderResources", ShaderResourcesD3D12, 1);
        auto* pResources = new (pRawMem) ShaderResourcesD3D12 //
            {
                m_pShaderByteCode,
                m_Desc,
                m_Desc.UseCombinedTextureSamplers ? m_Desc.CombinedSamplerSuffix : nullptr,
                D3D12ShaderCI.pDXCompiler,
                ShaderCI.LoadConstantBufferReflection //
            };
        m_pShaderResources.reset(pResources, STDDeleterRawMem<ShaderResourcesD3D12>(Allocator));
    }
}

ShaderD3D12Impl::~ShaderD3D12Impl()
{
}

void ShaderD3D12Impl::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;
    if (IID == IID_ShaderD3D || IID == IID_ShaderD3D12 || IID == IID_InternalImpl)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        TShaderBase::QueryInterface(IID, ppInterface);
    }
}

} // namespace Diligent
