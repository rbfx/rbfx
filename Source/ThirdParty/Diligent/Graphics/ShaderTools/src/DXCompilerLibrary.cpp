/*
 *  Copyright 2024 Diligent Graphics LLC
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

#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS
#    include "WinHPreface.h"
#    include <atlbase.h>
#    include "WinHPostface.h"
#endif

#include "DXCompilerLibrary.hpp"

namespace Diligent
{

void DXCompilerLibrary::InitVersion()
{
    VERIFY_EXPR(m_DxcCreateInstance != nullptr);

    CComPtr<IDxcValidator> pdxcValidator;
    if (SUCCEEDED(m_DxcCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(&pdxcValidator))))
    {
        CComPtr<IDxcVersionInfo> pdxcVerInfo;
        if (SUCCEEDED(pdxcValidator->QueryInterface(IID_PPV_ARGS(&pdxcVerInfo))))
        {
            UINT32 MajorVer = 0;
            UINT32 MinorVer = 0;
            pdxcVerInfo->GetVersion(&MajorVer, &MinorVer);
            m_Version = {MajorVer, MinorVer};
        }
        else
        {
            UNEXPECTED("Failed to query IDxcVersionInfo interface from DXC validator");
        }
    }
    else
    {
        UNEXPECTED("Failed to create DXC validator instance");
    }
}

#define CHECK_D3D_RESULT(Expr, Message)   \
    do                                    \
    {                                     \
        HRESULT hr = Expr;                \
        if (FAILED(hr))                   \
        {                                 \
            LOG_ERROR_AND_THROW(Message); \
        }                                 \
    } while (false)


void DXCompilerLibrary::DetectMaxShaderModel()
{
    VERIFY_EXPR(m_DxcCreateInstance != nullptr);
    try
    {
        CComPtr<IDxcLibrary> pdxcLibrary;
        CHECK_D3D_RESULT(m_DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pdxcLibrary)), "Failed to create DXC Library");

        CComPtr<IDxcCompiler> pdxcCompiler;
        CHECK_D3D_RESULT(m_DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pdxcCompiler)), "Failed to create DXC Compiler");

        constexpr char TestShader[] = R"(
float4 main() : SV_Target0
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

        CComPtr<IDxcBlobEncoding> pSourceBlob;
        CHECK_D3D_RESULT(pdxcLibrary->CreateBlobWithEncodingFromPinned(TestShader, sizeof(TestShader), CP_UTF8, &pSourceBlob), "Failed to create DXC Blob Encoding");

        ShaderVersion MaxSM{6, 0};

        std::vector<const wchar_t*> DxilArgs;
        if (m_Target == DXCompilerTarget::Vulkan)
            DxilArgs.push_back(L"-spirv");

        for (Uint32 MinorVer = 1;; ++MinorVer)
        {
            std::wstring Profile = L"ps_6_";
            Profile += std::to_wstring(MinorVer);

            CComPtr<IDxcOperationResult> pdxcResult;

            auto hr = pdxcCompiler->Compile(
                pSourceBlob,
                L"",
                L"main",
                Profile.c_str(),
                !DxilArgs.empty() ? DxilArgs.data() : nullptr,
                static_cast<UINT32>(DxilArgs.size()),
                nullptr, // Array of defines
                0,       // Number of defines
                nullptr, // Include handler
                &pdxcResult);
            if (FAILED(hr))
                break;

            HRESULT status = E_FAIL;
            if (FAILED(pdxcResult->GetStatus(&status)))
                break;
            if (FAILED(status))
                break;

            MaxSM.Minor = MinorVer;
        }

        m_MaxShaderModel = MaxSM;
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to detect max shader model for DXC compiler");
    }
}

} // namespace Diligent
