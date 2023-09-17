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

#include "ShaderResourcesD3D11.hpp"

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

static constexpr Uint32 GetRegisterSpace(const D3D11_SHADER_INPUT_BIND_DESC&)
{
    return 0;
}

ShaderResourcesD3D11::ShaderResourcesD3D11(ID3DBlob*         pShaderBytecode,
                                           const ShaderDesc& ShdrDesc,
                                           const char*       CombinedSamplerSuffix,
                                           bool              LoadConstantBufferReflection) :
    ShaderResources{ShdrDesc.ShaderType}
{
    class NewResourceHandler
    {
    public:
        NewResourceHandler(const ShaderDesc&     _ShdrDesc,
                           const char*           _CombinedSamplerSuffix,
                           ShaderResourcesD3D11& _Resources) :
            // clang-format off
            ShdrDesc             {_ShdrDesc             },
            CombinedSamplerSuffix{_CombinedSamplerSuffix},
            Resources            {_Resources            }
        // clang-format on
        {}

        void OnNewCB(const D3DShaderResourceAttribs& CBAttribs)
        {
            VERIFY(CBAttribs.BindPoint + CBAttribs.BindCount - 1 <= MaxAllowedBindPoint, "CB bind point exceeds supported range");
            Resources.m_MaxCBBindPoint = std::max(Resources.m_MaxCBBindPoint, static_cast<MaxBindPointType>(CBAttribs.BindPoint + CBAttribs.BindCount - 1));
        }

        void OnNewTexUAV(const D3DShaderResourceAttribs& TexUAV)
        {
            VERIFY(TexUAV.BindPoint + TexUAV.BindCount - 1 <= MaxAllowedBindPoint, "Tex UAV bind point exceeds supported range");
            Resources.m_MaxUAVBindPoint = std::max(Resources.m_MaxUAVBindPoint, static_cast<MaxBindPointType>(TexUAV.BindPoint + TexUAV.BindCount - 1));
        }

        void OnNewBuffUAV(const D3DShaderResourceAttribs& BuffUAV)
        {
            VERIFY(BuffUAV.BindPoint + BuffUAV.BindCount - 1 <= MaxAllowedBindPoint, "Buff UAV bind point exceeds supported range");
            Resources.m_MaxUAVBindPoint = std::max(Resources.m_MaxUAVBindPoint, static_cast<MaxBindPointType>(BuffUAV.BindPoint + BuffUAV.BindCount - 1));
        }

        void OnNewBuffSRV(const D3DShaderResourceAttribs& BuffSRV)
        {
            VERIFY(BuffSRV.BindPoint + BuffSRV.BindCount - 1 <= MaxAllowedBindPoint, "Buff SRV bind point exceeds supported range");
            Resources.m_MaxSRVBindPoint = std::max(Resources.m_MaxSRVBindPoint, static_cast<MaxBindPointType>(BuffSRV.BindPoint + BuffSRV.BindCount - 1));
        }

        void OnNewSampler(const D3DShaderResourceAttribs& SamplerAttribs)
        {
            VERIFY(SamplerAttribs.BindPoint + SamplerAttribs.BindCount - 1 <= MaxAllowedBindPoint, "Sampler bind point exceeds supported range");
            Resources.m_MaxSamplerBindPoint = std::max(Resources.m_MaxSamplerBindPoint, static_cast<MaxBindPointType>(SamplerAttribs.BindPoint + SamplerAttribs.BindCount - 1));
        }

        void OnNewTexSRV(const D3DShaderResourceAttribs& TexAttribs)
        {
            VERIFY(TexAttribs.BindPoint + TexAttribs.BindCount - 1 <= MaxAllowedBindPoint, "Tex SRV bind point exceeds supported range");
            Resources.m_MaxSRVBindPoint = std::max(Resources.m_MaxSRVBindPoint, static_cast<MaxBindPointType>(TexAttribs.BindPoint + TexAttribs.BindCount - 1));
        }

        void OnNewAccelStruct(const D3DShaderResourceAttribs& ASAttribs)
        {
            UNEXPECTED("Acceleration structure is not supported in DirectX 11");
        }

        ~NewResourceHandler()
        {
        }

    private:
        const ShaderDesc&     ShdrDesc;
        const char*           CombinedSamplerSuffix;
        ShaderResourcesD3D11& Resources;
    };

    CComPtr<ID3D11ShaderReflection> pShaderReflection;
    HRESULT                         hr = D3DReflect(pShaderBytecode->GetBufferPointer(), pShaderBytecode->GetBufferSize(), __uuidof(pShaderReflection), reinterpret_cast<void**>(&pShaderReflection));
    CHECK_D3D_RESULT_THROW(hr, "Failed to get the shader reflection");


    struct D3D11ReflectionTraits
    {
        using D3D_SHADER_DESC            = D3D11_SHADER_DESC;
        using D3D_SHADER_INPUT_BIND_DESC = D3D11_SHADER_INPUT_BIND_DESC;
        using D3D_SHADER_BUFFER_DESC     = D3D11_SHADER_BUFFER_DESC;
        using D3D_SHADER_VARIABLE_DESC   = D3D11_SHADER_VARIABLE_DESC;
        using D3D_SHADER_TYPE_DESC       = D3D11_SHADER_TYPE_DESC;
    };
    Initialize<D3D11ReflectionTraits>(
        static_cast<ID3D11ShaderReflection*>(pShaderReflection),
        NewResourceHandler{ShdrDesc, CombinedSamplerSuffix, *this},
        ShdrDesc.Name,
        CombinedSamplerSuffix,
        LoadConstantBufferReflection);
}


ShaderResourcesD3D11::~ShaderResourcesD3D11()
{
}

} // namespace Diligent
