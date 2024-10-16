/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

    if (HLSLVersion == ShaderVersion{0, 0})
    {
        // Limit shader version to 6.6 to avoid issues with byte code changes in newer untested versions of DXC.
        return ShaderVersion::Min(MaxSupportedSM, ShaderVersion{6, 6});
    }
    else
    {
        return ShaderVersion::Min(HLSLVersion, MaxSupportedSM);
    }
}

ShaderD3D12Impl::ShaderD3D12Impl(IReferenceCounters*     pRefCounters,
                                 RenderDeviceD3D12Impl*  pRenderDeviceD3D12,
                                 const ShaderCreateInfo& ShaderCI,
                                 const CreateInfo&       D3D12ShaderCI,
                                 bool                    IsDeviceInternal) :
    TShaderBase //
    {
        pRefCounters,
        pRenderDeviceD3D12,
        ShaderCI,
        D3D12ShaderCI,
        IsDeviceInternal,
        GetD3D12ShaderModel(ShaderCI, D3D12ShaderCI.pDXCompiler, D3D12ShaderCI.MaxShaderVersion),
        [pDXCompiler      = D3D12ShaderCI.pDXCompiler,
         LoadCBReflection = ShaderCI.LoadConstantBufferReflection](const ShaderDesc& Desc, IDataBlob* pShaderByteCode) {
            auto& Allocator  = GetRawAllocator();
            auto* pRawMem    = ALLOCATE(Allocator, "Allocator for ShaderResources", ShaderResourcesD3D12, 1);
            auto* pResources = new (pRawMem) ShaderResourcesD3D12 //
                {
                    pShaderByteCode,
                    Desc,
                    Desc.UseCombinedTextureSamplers ? Desc.CombinedSamplerSuffix : nullptr,
                    pDXCompiler,
                    LoadCBReflection,
                };
            return std::shared_ptr<const ShaderResourcesD3D12>{pResources, STDDeleterRawMem<ShaderResourcesD3D12>(Allocator)};
        },
    },
    m_EntryPoint{ShaderCI.EntryPoint}
{
}

ShaderD3D12Impl::~ShaderD3D12Impl()
{
    // Make sure that asynchrous task is complete as it references the shader object.
    // This needs to be done in the final class before the destruction begins.
    GetStatus(/*WaitForCompletion = */ true);
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
