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

#include "ShaderD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

constexpr INTERFACE_ID ShaderD3D11Impl::IID_InternalImpl;

static const ShaderVersion HLSLValidateShaderVersion(const ShaderVersion& Version, const ShaderVersion& MaxVersion)
{
    ShaderVersion ModelVer;
    if (Version > MaxVersion)
    {
        ModelVer = MaxVersion;
        LOG_ERROR_MESSAGE("Shader model ", Uint32{Version.Major}, "_", Uint32{Version.Minor},
                          " is not supported by this device. Attempting to use the maximum supported model ",
                          Uint32{MaxVersion.Major}, "_", Uint32{MaxVersion.Minor}, '.');
    }
    else
    {
        ModelVer = Version;
    }
    return ModelVer;
}

static const ShaderVersion GetD3D11ShaderModel(D3D_FEATURE_LEVEL d3dDeviceFeatureLevel, const ShaderVersion& HLSLVersion)
{
    switch (d3dDeviceFeatureLevel)
    {
        // Direct3D11 only supports shader model 5.0 even if the device feature level is
        // above 11.0 (for example, 11.1 or 12.0).
        // https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro#overview-for-each-feature-level
#if defined(_WIN32_WINNT_WIN10) && (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
#endif
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return (HLSLVersion == ShaderVersion{0, 0}) ?
                ShaderVersion{5, 0} :
                HLSLValidateShaderVersion(HLSLVersion, {5, 0});

        case D3D_FEATURE_LEVEL_10_1:
            return (HLSLVersion == ShaderVersion{0, 0}) ?
                ShaderVersion{4, 1} :
                HLSLValidateShaderVersion(HLSLVersion, {4, 1});

        case D3D_FEATURE_LEVEL_10_0:
            return (HLSLVersion == ShaderVersion{0, 0}) ?
                ShaderVersion{4, 0} :
                HLSLValidateShaderVersion(HLSLVersion, {4, 0});

        default:
            UNEXPECTED("Unexpected D3D feature level ", static_cast<Uint32>(d3dDeviceFeatureLevel));
            return ShaderVersion{4, 0};
    }
}

ShaderD3D11Impl::ShaderD3D11Impl(IReferenceCounters*     pRefCounters,
                                 RenderDeviceD3D11Impl*  pRenderDeviceD3D11,
                                 const ShaderCreateInfo& ShaderCI,
                                 const CreateInfo&       D3D11ShaderCI,
                                 bool                    IsDeviceInternal) :
    TShaderBase //
    {
        pRefCounters,
        pRenderDeviceD3D11,
        ShaderCI,
        D3D11ShaderCI,
        IsDeviceInternal,
        GetD3D11ShaderModel(D3D11ShaderCI.FeatureLevel, ShaderCI.HLSLVersion),
        [LoadConstantBufferReflection = ShaderCI.LoadConstantBufferReflection](const ShaderDesc& Desc, IDataBlob* pShaderByteCode) {
            auto& Allocator  = GetRawAllocator();
            auto* pRawMem    = ALLOCATE(Allocator, "Allocator for ShaderResources", ShaderResourcesD3D11, 1);
            auto* pResources = new (pRawMem) ShaderResourcesD3D11 //
                {
                    pShaderByteCode,
                    Desc,
                    Desc.UseCombinedTextureSamplers ? Desc.CombinedSamplerSuffix : nullptr,
                    LoadConstantBufferReflection,
                };
            return std::shared_ptr<const ShaderResourcesD3D11>{pResources, STDDeleterRawMem<ShaderResourcesD3D11>(Allocator)};
        },
    }
{
}

ShaderD3D11Impl::~ShaderD3D11Impl()
{
    // Make sure that asynchrous task is complete as it references the shader object.
    // This needs to be done in the final class before the destruction begins.
    GetStatus(/*WaitForCompletion = */ true);
}

void ShaderD3D11Impl::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;
    if (IID == IID_ShaderD3D || IID == IID_ShaderD3D11 || IID == IID_InternalImpl)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        TShaderBase::QueryInterface(IID, ppInterface);
    }
}

ID3D11DeviceChild* ShaderD3D11Impl::GetD3D11Shader(IDataBlob* pBytecode) noexcept(false)
{
    std::lock_guard<std::mutex> Lock{m_d3dShaderCacheMtx};

    BlobHashKey BlobKey{pBytecode};

    auto it = m_d3dShaderCache.find(BlobKey);
    if (it != m_d3dShaderCache.end())
    {
        return it->second;
    }

    VERIFY(pBytecode->GetSize() == m_pShaderByteCode->GetSize(), "The byte code size does not match the size of the original byte code");

    auto* pd3d11Device = GetDevice()->GetD3D11Device();

    CComPtr<ID3D11DeviceChild> pd3d11Shader;
    switch (m_Desc.ShaderType)
    {
#define CREATE_SHADER(SHADER_NAME, ShaderName)                                                                                                        \
    case SHADER_TYPE_##SHADER_NAME:                                                                                                                   \
    {                                                                                                                                                 \
        CComPtr<ID3D11##ShaderName##Shader> pd3d11SpecificShader;                                                                                     \
                                                                                                                                                      \
        HRESULT hr = pd3d11Device->Create##ShaderName##Shader(pBytecode->GetConstDataPtr(), pBytecode->GetSize(),                                     \
                                                              NULL, &pd3d11SpecificShader);                                                           \
        CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D11 shader");                                                                                  \
        pd3d11SpecificShader->QueryInterface(__uuidof(ID3D11DeviceChild), reinterpret_cast<void**>(static_cast<ID3D11DeviceChild**>(&pd3d11Shader))); \
        break;                                                                                                                                        \
    }

        // clang-format off
        CREATE_SHADER(VERTEX,   Vertex)
        CREATE_SHADER(PIXEL,    Pixel)
        CREATE_SHADER(GEOMETRY, Geometry)
        CREATE_SHADER(DOMAIN,   Domain)
        CREATE_SHADER(HULL,     Hull)
        CREATE_SHADER(COMPUTE,  Compute)
        // clang-format on
#undef CREATE_SHADER

        default: UNEXPECTED("Unexpected shader type");
    }

    if (!pd3d11Shader)
        LOG_ERROR_AND_THROW("Failed to create shader from the byte code");

    if (*m_Desc.Name != 0)
    {
        auto hr = pd3d11Shader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(m_Desc.Name)), m_Desc.Name);
        DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to set shader name");
    }

    return m_d3dShaderCache.emplace(BlobKey, std::move(pd3d11Shader)).first->second;
}

} // namespace Diligent
