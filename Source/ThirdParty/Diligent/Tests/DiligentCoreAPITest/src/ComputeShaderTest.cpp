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

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/ComputeShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void ComputeShaderReferenceD3D11(ISwapChain* pSwapChain);
#endif

#if D3D12_SUPPORTED
void ComputeShaderReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void ComputeShaderReferenceGL(ISwapChain* pSwapChain);
#endif

#if VULKAN_SUPPORTED
void ComputeShaderReferenceVk(ISwapChain* pSwapChain);
#endif

#if METAL_SUPPORTED
void ComputeShaderReferenceMtl(ISwapChain* pSwapChain);
#endif

void ComputeShaderReference(ISwapChain* pSwapChain)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    auto deviceType = pDevice->GetDeviceInfo().Type;
    switch (deviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
            ComputeShaderReferenceD3D11(pSwapChain);
            break;
#endif

#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
            ComputeShaderReferenceD3D12(pSwapChain);
            break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            ComputeShaderReferenceGL(pSwapChain);
            break;

#endif

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
            ComputeShaderReferenceVk(pSwapChain);
            break;
#endif

#if METAL_SUPPORTED
        case RENDER_DEVICE_TYPE_METAL:
            ComputeShaderReferenceMtl(pSwapChain);
            break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unsupported device type");
    }

    if (RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain})
    {
        pTestingSwapChain->TakeSnapshot();
    }
}

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ComputeShaderTest, FillTexture)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.ComputeShaders)
    {
        GTEST_SKIP() << "Compute shaders are not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    if (!pTestingSwapChain)
    {
        GTEST_SKIP() << "Compute shader test requires testing swap chain";
    }

    pContext->Flush();
    pContext->InvalidateState();

    ComputeShaderReference(pSwapChain);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.Desc           = {"Compute shader test", SHADER_TYPE_COMPUTE, true};
    ShaderCI.EntryPoint     = "main";
    ShaderCI.Source         = HLSL::FillTextureCS.c_str();
    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);

    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Compute shader test";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    PSOCreateInfo.pCS                  = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto& SCDesc = pSwapChain->GetDesc();

    pPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "g_tex2DUAV")->Set(pTestingSwapChain->GetCurrentBackBufferUAV());

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DispatchComputeAttribs DispatchAttribs;
    DispatchAttribs.ThreadGroupCountX = (SCDesc.Width + 15) / 16;
    DispatchAttribs.ThreadGroupCountY = (SCDesc.Height + 15) / 16;
    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

// Test that GenerateMips does not mess up compute pipeline in D3D12
TEST(ComputeShaderTest, GenerateMips_CSInterference)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.ComputeShaders)
    {
        GTEST_SKIP() << "Compute shaders are not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    if (!pTestingSwapChain)
    {
        GTEST_SKIP() << "Compute shader test requires testing swap chain";
    }

    pContext->Flush();
    pContext->InvalidateState();

    ComputeShaderReference(pSwapChain);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.Desc           = {"Compute shader test", SHADER_TYPE_COMPUTE, true};
    ShaderCI.EntryPoint     = "main";
    ShaderCI.Source         = HLSL::FillTextureCS2.c_str();
    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);

    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Generate Mips - CS interference test";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    PSOCreateInfo.pCS                  = pCS;

    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto& SCDesc = pSwapChain->GetDesc();

    RefCntAutoPtr<ITexture> pWhiteTex;
    {
        std::vector<Uint8> WhiteRGBA(SCDesc.Width * SCDesc.Width * 4, 255);
        pWhiteTex = pEnv->CreateTexture("White Texture", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, SCDesc.Width, SCDesc.Width, WhiteRGBA.data());
        ASSERT_NE(pWhiteTex, nullptr);
    }

    RefCntAutoPtr<ITexture> pBlackTex;
    {
        TextureDesc TexDesc{"Black texture", RESOURCE_DIM_TEX_2D, SCDesc.Width, SCDesc.Height, 1, TEX_FORMAT_RGBA8_UNORM, 4, 1, USAGE_DEFAULT, BIND_SHADER_RESOURCE};
        TexDesc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;

        std::vector<Uint8>             BlackRGBA(SCDesc.Width * SCDesc.Width * 4);
        std::vector<TextureSubResData> MipData(TexDesc.MipLevels);
        for (Uint32 i = 0; i < TexDesc.MipLevels; ++i)
        {
            MipData[i] = TextureSubResData{BlackRGBA.data(), SCDesc.Width * 4};
        }
        TextureData InitData{MipData.data(), TexDesc.MipLevels};

        pDevice->CreateTexture(TexDesc, &InitData, &pBlackTex);
        ASSERT_NE(pBlackTex, nullptr);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex2DWhiteTexture")->Set(pWhiteTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex2DUAV")->Set(pTestingSwapChain->GetCurrentBackBufferUAV());

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Do not populate the entire texture
    DispatchComputeAttribs DispatchAttribs;
    DispatchAttribs.ThreadGroupCountX = 1;
    DispatchAttribs.ThreadGroupCountY = 1;
    pContext->DispatchCompute(DispatchAttribs);

    // In D3D12 generate mips uses compute pipeline
    pContext->GenerateMips(pBlackTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

    DispatchAttribs.ThreadGroupCountX = (SCDesc.Width + 15) / 16;
    DispatchAttribs.ThreadGroupCountY = (SCDesc.Height + 15) / 16;
    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

} // namespace
