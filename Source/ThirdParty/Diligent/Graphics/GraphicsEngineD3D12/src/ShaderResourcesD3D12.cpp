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

#include "ShaderResourcesD3D12.hpp"

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "ShaderD3DBase.hpp"
#include "ShaderBase.hpp"
#include "DXCompiler.hpp"

#include "dxc/dxcapi.h"

namespace Diligent
{

template <>
Uint32 GetRegisterSpace<>(const D3D12_SHADER_INPUT_BIND_DESC& BindingDesc)
{
    return BindingDesc.Space;
}

ShaderResourcesD3D12::ShaderResourcesD3D12(IDataBlob*        pShaderBytecode,
                                           const ShaderDesc& ShdrDesc,
                                           const char*       CombinedSamplerSuffix,
                                           IDXCompiler*      pDXCompiler,
                                           bool              LoadConstantBufferReflection) :
    ShaderResources{ShdrDesc.ShaderType}
{
    CComPtr<ID3D12ShaderReflection> pShaderReflection;
    if (IsDXILBytecode(pShaderBytecode->GetConstDataPtr(), pShaderBytecode->GetSize()))
    {
        VERIFY(pDXCompiler != nullptr, "DXC is not initialized");
        CComPtr<IDxcBlob> pDxcBytecode;
        CreateDxcBlobWrapper(pShaderBytecode, &pDxcBytecode);
        pDXCompiler->GetD3D12ShaderReflection(pDxcBytecode, &pShaderReflection);
        if (!pShaderReflection)
        {
            LOG_ERROR_AND_THROW("Failed to read shader reflection from DXIL container");
        }
    }
    else
    {
        // Use D3D compiler to get reflection.
        auto hr = D3DReflect(pShaderBytecode->GetConstDataPtr(), pShaderBytecode->GetSize(), __uuidof(pShaderReflection), reinterpret_cast<void**>(&pShaderReflection));
        CHECK_D3D_RESULT_THROW(hr, "Failed to get the shader reflection");
    }

    class NewResourceHandler
    {
    public:
        // clang-format off
        void OnNewCB         (const D3DShaderResourceAttribs& CBAttribs)     {}
        void OnNewTexUAV     (const D3DShaderResourceAttribs& TexUAV)        {}
        void OnNewBuffUAV    (const D3DShaderResourceAttribs& BuffUAV)       {}
        void OnNewBuffSRV    (const D3DShaderResourceAttribs& BuffSRV)       {}
        void OnNewSampler    (const D3DShaderResourceAttribs& SamplerAttribs){}
        void OnNewTexSRV     (const D3DShaderResourceAttribs& TexAttribs)    {}
        void OnNewAccelStruct(const D3DShaderResourceAttribs& ASAttribs)     {}
        // clang-format on
    };
    struct D3D12ReflectionTraits
    {
        using D3D_SHADER_DESC            = D3D12_SHADER_DESC;
        using D3D_SHADER_INPUT_BIND_DESC = D3D12_SHADER_INPUT_BIND_DESC;
        using D3D_SHADER_BUFFER_DESC     = D3D12_SHADER_BUFFER_DESC;
        using D3D_SHADER_VARIABLE_DESC   = D3D12_SHADER_VARIABLE_DESC;
        using D3D_SHADER_TYPE_DESC       = D3D12_SHADER_TYPE_DESC;
    };
    Initialize<D3D12ReflectionTraits, ID3D12ShaderReflection>(
        pShaderReflection,
        NewResourceHandler{},
        ShdrDesc.Name,
        CombinedSamplerSuffix,
        LoadConstantBufferReflection);
}

} // namespace Diligent
