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

#include <algorithm>
#include <cctype>
#include <thread>
#include <vector>
#include <atomic>

#include "GPUTestingEnvironment.hpp"
#include "Sampler.h"
#include "GraphicsAccessories.hpp"

#include "ResourceLayoutTestCommon.hpp"
#include "FastRand.hpp"
#include "ThreadSignal.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestSamplerCInterface(void* pSampler);
}

namespace
{

class FilterTypeTest : public testing::TestWithParam<std::tuple<FILTER_TYPE, FILTER_TYPE, FILTER_TYPE>>
{
protected:
    static void TearDownTestSuite()
    {
        GPUTestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(FilterTypeTest, CreateSampler)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    SamplerDesc SamplerDesc;
    SamplerDesc.Name      = "FilterTypeTest.CreateSampler";
    const auto& Param     = GetParam();
    SamplerDesc.MinFilter = std::get<0>(Param);
    SamplerDesc.MagFilter = std::get<1>(Param);
    SamplerDesc.MipFilter = std::get<2>(Param);
    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc, &pSampler);
    ASSERT_TRUE(pSampler);
    EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);

    TestSamplerCInterface(pSampler);
}

std::string GetSamplerFilterTestName(const testing::TestParamInfo<std::tuple<FILTER_TYPE, FILTER_TYPE, FILTER_TYPE>>& info) //
{
    std::string name =
        std::string{GetFilterTypeLiteralName(std::get<0>(info.param), false)} + std::string{"__"} +
        std::string{GetFilterTypeLiteralName(std::get<1>(info.param), false)} + std::string{"__"} +
        std::string{GetFilterTypeLiteralName(std::get<2>(info.param), false)};
    for (size_t i = 0; i < name.length(); ++i)
    {
        if (!std::isalnum(name[i]))
            name[i] = '_';
    }
    return name;
}


INSTANTIATE_TEST_SUITE_P(RegularFilters,
                         FilterTypeTest,
                         testing::Combine(
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR),
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR),
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR)),
                         GetSamplerFilterTestName);

INSTANTIATE_TEST_SUITE_P(ComparisonFilters,
                         FilterTypeTest,
                         testing::Combine(
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR),
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR),
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR)),
                         GetSamplerFilterTestName);

TEST(FilterTypeTest, AnisotropicFilter)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetAdapterInfo().Sampler.AnisotropicFilteringSupported)
        GTEST_SKIP() << "Anisotropic filtering is not supported by this device";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.Name      = "FilterTypeTest.AnisotropicFilter";
        SamplerDesc.MinFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_ANISOTROPIC;

        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc, &pSampler);
        ASSERT_TRUE(pSampler);
        EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
    }

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.Name      = "FilterTypeTest.AnisotropicFilter2";
        SamplerDesc.MinFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;

        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc, &pSampler);
        ASSERT_TRUE(pSampler);
        EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
    }
}

TEST(FilterTypeTest, UnnormalizedCoords)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().IsVulkanDevice() && !pDevice->GetDeviceInfo().IsMetalDevice())
        GTEST_SKIP() << "Unnormalized coordinates are only supported by Vulkan and Metal";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.Name               = "FilterTypeTest.UnnormalizedCoords";
        SamplerDesc.MinFilter          = FILTER_TYPE_LINEAR;
        SamplerDesc.MagFilter          = FILTER_TYPE_LINEAR;
        SamplerDesc.MipFilter          = FILTER_TYPE_POINT;
        SamplerDesc.UnnormalizedCoords = true;

        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc, &pSampler);
        ASSERT_TRUE(pSampler);
        EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
    }
}


class AddressModeTest : public testing::TestWithParam<TEXTURE_ADDRESS_MODE>
{
protected:
    static void TearDownTestSuite()
    {
        GPUTestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(AddressModeTest, CreateSampler)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    SamplerDesc SamplerDesc;
    SamplerDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = GetParam();
    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc, &pSampler);
    ASSERT_TRUE(pSampler);
    EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
}

INSTANTIATE_TEST_SUITE_P(AddressModes,
                         AddressModeTest,
                         testing::Values(
                             TEXTURE_ADDRESS_WRAP,
                             TEXTURE_ADDRESS_MIRROR,
                             TEXTURE_ADDRESS_CLAMP,
                             TEXTURE_ADDRESS_BORDER),
                         [](const testing::TestParamInfo<TEXTURE_ADDRESS_MODE>& info) //
                         {
                             return std::string{GetTextureAddressModeLiteralName(info.param, true)};
                         } //
);


class ComparisonFuncTest : public testing::TestWithParam<COMPARISON_FUNCTION>
{
protected:
    static void TearDownTestSuite()
    {
        GPUTestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(ComparisonFuncTest, CreateSampler)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    SamplerDesc SamplerDesc;
    SamplerDesc.MinFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.MagFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.MipFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.ComparisonFunc = GetParam();
    RefCntAutoPtr<ISampler> pSampler1, pSampler2;
    SamplerDesc.Name = "Sam1";
    pDevice->CreateSampler(SamplerDesc, &pSampler1);
    SamplerDesc.Name = "Sam2";
    pDevice->CreateSampler(SamplerDesc, &pSampler2);
    ASSERT_TRUE(pSampler1);
    ASSERT_TRUE(pSampler2);
    EXPECT_EQ(pSampler1, pSampler2);
    EXPECT_EQ(pSampler1->GetDesc(), SamplerDesc);
}

INSTANTIATE_TEST_SUITE_P(ComparisonFunctions,
                         ComparisonFuncTest,
                         testing::Values(
                             COMPARISON_FUNC_NEVER,
                             COMPARISON_FUNC_LESS,
                             COMPARISON_FUNC_EQUAL,
                             COMPARISON_FUNC_LESS_EQUAL,
                             COMPARISON_FUNC_GREATER,
                             COMPARISON_FUNC_NOT_EQUAL,
                             COMPARISON_FUNC_GREATER_EQUAL,
                             COMPARISON_FUNC_ALWAYS),
                         [](const testing::TestParamInfo<COMPARISON_FUNCTION>& info) //
                         {
                             return std::string{GetComparisonFunctionLiteralName(info.param, true)};
                         } //
);


void TestSamplerCorrectness(IShader*                      pVS,
                            IShader*                      pPS,
                            ITextureView*                 pDefaultSRV,
                            ITextureView*                 pClampSRV,
                            ITextureView*                 pWrapSRV,
                            ITextureView*                 pMirrorSRV,
                            SHADER_RESOURCE_VARIABLE_TYPE VarType,
                            bool                          IsImmutable)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pContext   = pEnv->GetDeviceContext();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    static FastRandFloat Rnd{0, 0, 1};

    const float ClearColor[] = {Rnd(), Rnd(), Rnd(), Rnd()};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Sampler correctness test";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;


    ShaderResourceVariableDesc Variables[] =
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DClamp", VarType},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DWrap", VarType},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DMirror", VarType} //
        };
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    PSODesc.ResourceLayout.Variables    = Variables;

    std::vector<ImmutableSamplerDesc> ImtblSamplers;
    if (IsImmutable)
    {
        SamplerDesc ClampSamDesc;
        ClampSamDesc.Name     = "Clamp sampler";
        ClampSamDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
        ClampSamDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
        ClampSamDesc.AddressW = TEXTURE_ADDRESS_CLAMP;

        SamplerDesc WrapSamDesc;
        WrapSamDesc.Name     = "Wrap sampler";
        WrapSamDesc.AddressU = TEXTURE_ADDRESS_WRAP;
        WrapSamDesc.AddressV = TEXTURE_ADDRESS_WRAP;
        WrapSamDesc.AddressW = TEXTURE_ADDRESS_WRAP;

        SamplerDesc MirrorSamDesc;
        MirrorSamDesc.Name     = "Mirror sampler";
        MirrorSamDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
        MirrorSamDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
        MirrorSamDesc.AddressW = TEXTURE_ADDRESS_MIRROR;

        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DClamp", ClampSamDesc);
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DWrap", WrapSamDesc);
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DMirror", MirrorSamDesc);
        PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, false);
    ASSERT_TRUE(pSRB != nullptr);

    if (VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
    {
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DClamp")->Set(IsImmutable ? pDefaultSRV : pClampSRV);
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DWrap")->Set(IsImmutable ? pDefaultSRV : pWrapSRV);
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DMirror")->Set(IsImmutable ? pDefaultSRV : pMirrorSRV);
    }
    else
    {
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DClamp")->Set(IsImmutable ? pDefaultSRV : pClampSRV);
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DWrap")->Set(IsImmutable ? pDefaultSRV : pWrapSRV);
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Tex2DMirror")->Set(IsImmutable ? pDefaultSRV : pMirrorSRV);
    }
    pPSO->InitializeStaticSRBResources(pSRB);

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    pSwapChain->Present();
}

TEST(SamplerTest, Correctness)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.FilePath                   = "SamplerCorrectness.hlsl";

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc       = {"Sampler correctness test - VS", SHADER_TYPE_VERTEX, true};
        ShaderCI.EntryPoint = "VSMain";
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc       = {"Sampler correctness test - PS", SHADER_TYPE_PIXEL, true};
        ShaderCI.EntryPoint = "PSMain";
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    constexpr Uint32 TexDim = 128;

    std::vector<Uint32> TexData(TexDim * TexDim);
    for (Uint32 x = 0; x < TexDim; ++x)
    {
        for (Uint32 y = 0; y < TexDim; ++y)
        {
            // clang-format off
            Uint32 r = (x <  TexDim / 2 && y <  TexDim / 2) ? 255 : 0;
            Uint32 g = (x >= TexDim / 2 && y <  TexDim / 2) ? 255 : 0;
            Uint32 b = (x <  TexDim / 2 && y >= TexDim / 2) ? 255 : 0;
            Uint32 a = (x >= TexDim / 2 && y >= TexDim / 2) ? 255 : 0;
            // clang-format on
            TexData[x + y * TexDim] = r | (g << 8) | (b << 16) | (a << 24);
        }
    }
    auto pTexture = pEnv->CreateTexture("Sampler correctness test", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, TexDim, TexDim, TexData.data());
    ASSERT_NE(pTexture, nullptr);


    auto CreateSamplerView = [&](TEXTURE_ADDRESS_MODE AddressMode, const char* SamName, const char* ViewName) -> RefCntAutoPtr<ITextureView> //
    {
        SamplerDesc SamDesc;
        SamDesc.Name     = SamName;
        SamDesc.AddressU = AddressMode;
        SamDesc.AddressV = AddressMode;
        SamDesc.AddressW = AddressMode;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamDesc, &pSampler);
        if (!pSampler)
            return {};

        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
        ViewDesc.Name     = ViewName;

        RefCntAutoPtr<ITextureView> pSRV;
        pTexture->CreateView(ViewDesc, &pSRV);
        if (pSRV)
        {
            pSRV->SetSampler(pSampler);
        }

        return pSRV;
    };

    auto pClampSRV  = CreateSamplerView(TEXTURE_ADDRESS_CLAMP, "Clamp sampler", "Clamp view");
    auto pWrapSRV   = CreateSamplerView(TEXTURE_ADDRESS_WRAP, "Wrap sampler", "Wrap view");
    auto pMirrorSRV = CreateSamplerView(TEXTURE_ADDRESS_MIRROR, "Mirror sampler", "Mirror view");

    for (Uint32 IsImmutable = 0; IsImmutable < 2; ++IsImmutable)
    {
        for (Uint32 var_type = 0; var_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++var_type)
        {
            const auto VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(var_type);
            TestSamplerCorrectness(pVS,
                                   pPS,
                                   pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE),
                                   pClampSRV,
                                   pWrapSRV,
                                   pMirrorSRV,
                                   VarType,
                                   IsImmutable != 0);
            std::cout << TestingEnvironment::GetCurrentTestStatusString() << ' '
                      << " Var type: " << GetShaderVariableTypeLiteralName(VarType) << ','
                      << " Immutable: " << (IsImmutable ? "Yes" : "No") << std::endl;
        }
    }
}

TEST(SamplerTest, Multithreading)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.MultithreadedResourceCreation)
        GTEST_SKIP() << "This deveice does not supported multithreaded resource creation";

    const auto NumThreads = std::max(std::thread::hardware_concurrency(), 2u);

    GPUTestingEnvironment::ScopedReset AutoReset;

    Threading::Signal StartWorkSignal;
    Threading::Signal WorkCompletedSignal;
    std::atomic_int   NumCompltedThreads{0};

    std::vector<std::thread> Workers(NumThreads);
    for (auto& Worker : Workers)
    {
        Worker = std::thread{
            [&]() //
            {
                const auto  Seed = static_cast<unsigned int>(reinterpret_cast<size_t>(this) & 0xFFFFFFFFu);
                FastRandInt Rnd{Seed, 0, 2};
                while (true)
                {
                    auto SignaledValue = StartWorkSignal.Wait(true, NumThreads);
                    if (SignaledValue < 0)
                        return;

                    SamplerDesc SamDesc;
                    SamDesc.Name      = Rnd() ? "Test sampler" : nullptr;
                    SamDesc.AddressU  = TEXTURE_ADDRESS_CLAMP;
                    SamDesc.AddressV  = TEXTURE_ADDRESS_WRAP;
                    SamDesc.AddressW  = TEXTURE_ADDRESS_MIRROR;
                    SamDesc.MinFilter = FILTER_TYPE_LINEAR;
                    SamDesc.MagFilter = FILTER_TYPE_POINT;

                    RefCntAutoPtr<ISampler> pSampler;
                    pDevice->CreateSampler(SamDesc, &pSampler);
                    EXPECT_TRUE(pSampler);

                    if (NumCompltedThreads.fetch_add(1) + 1 == static_cast<int>(NumThreads))
                        WorkCompletedSignal.Trigger();
                }
            } //
        };
    }

    size_t NumIterations = 100;
    for (size_t i = 0; i < NumIterations; ++i)
    {
        GPUTestingEnvironment::ScopedReleaseResources ReleaseRes;

        NumCompltedThreads.store(0);
        StartWorkSignal.Trigger(true);
        WorkCompletedSignal.Wait(true, 1);
    }

    StartWorkSignal.Trigger(true, -1);

    for (auto& Worker : Workers)
        Worker.join();
}

} // namespace
