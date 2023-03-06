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

#include "gtest/gtest.h"

#include "CommonlyUsedStates.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char* VS0 = R"(
float4 main() : SV_Position
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char* PS0 = R"(
float4 main() : SV_Target
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char* PS_Tex = R"(
Texture2D<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D.Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_Tex2 = R"(
Texture2D<float4> g_tex2D2;
SamplerState g_tex2D2_sampler;
float4 main() : SV_Target
{
    return g_tex2D2.Sample(g_tex2D2_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_ArrOfTex = R"(
Texture2D<float4> g_tex2D[2];
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D[0].Sample(g_tex2D_sampler, float2(0.0, 0.0)) + g_tex2D[1].Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_TexArr = R"(
Texture2DArray<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D.Sample(g_tex2D_sampler, float3(0.0, 0.0, 0.0));
}
)";


static const char* PS_CB = R"(
cbuffer Test
{
    float4 g_Test;
};

float4 main() : SV_Target
{
    return g_Test;
}
)";

static const char* PS1_CB = R"(
cbuffer Test
{
    float4 g_Test;
    float4 g_Test2;
};

float4 main() : SV_Target
{
    return g_Test + g_Test2;
}
)";

static const char* PS_2CB = R"(
cbuffer Test
{
    float4 g_Test;
};

cbuffer Test2
{
    float4 g_Test2;
};

float4 main() : SV_Target
{
    return g_Test + g_Test2;
}
)";

static const char* PS_TexCB = R"(
cbuffer Test
{
    float4 g_Test;
};

cbuffer Test2
{
    float4 g_Test2;
};

Texture2D<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_Test + g_Test2 + g_tex2D.Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_TexCB2 = R"(
cbuffer TestA
{
    float4 g_Test;
};

cbuffer Test2A
{
    float4 g_Test2;
};

Texture2D<float4> g_tex2DA;
SamplerState g_tex2DA_sampler;
float4 main() : SV_Target
{
    return g_Test + g_Test2 + g_tex2DA.Sample(g_tex2DA_sampler, float2(0.0, 0.0));
}
)";

static const char* CS_RwBuff = R"(
RWTexture2D<float/* format=r32f */> g_RWTex;

[numthreads(1,1,1)]
void main()
{
    g_RWTex[int2(0,0)] = 0.0;
}
)";

static const char* CS_RwBuff2 = R"(
RWTexture2D<float/* format=r32f */> g_RWTex2;

[numthreads(1,1,1)]
void main()
{
    g_RWTex2[int2(0,0)] = 0.0;
}
)";

static const char* CS_RwBuff3 = R"(
RWTexture2D<float/* format=r32f */> g_RWTex;
RWTexture2D<float/* format=r32f */> g_RWTex2;

[numthreads(1,1,1)]
void main()
{
    g_RWTex[int2(0,0)] = 0.0;
    g_RWTex2[int2(0,0)] = 0.0;
}
)";


RefCntAutoPtr<IPipelineState> CreateGraphicsPSO(GPUTestingEnvironment* pEnv, const char* VSSource, const char* PSSource)
{
    auto* pDevice = pEnv->GetDevice();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name                                  = "PSO Compatibility test";
    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo CreationAttrs;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.ShaderCompiler = pEnv->GetDefaultCompiler(CreationAttrs.SourceLanguage);

    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc       = {"PSO Compatibility test VS", SHADER_TYPE_VERTEX, true};
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Source     = VSSource;
        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS != nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc       = {"PSO Compatibility test PS", SHADER_TYPE_PIXEL, true};
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Source     = PSSource;
        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS != nullptr);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    VERIFY_EXPR(pPSO != nullptr);

    return pPSO;
}

RefCntAutoPtr<IPipelineState> CreateComputePSO(GPUTestingEnvironment* pEnv, const char* CSSource)
{
    auto*                          pDevice = pEnv->GetDevice();
    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    ShaderCreateInfo CreationAttrs;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.ShaderCompiler = pEnv->GetDefaultCompiler(CreationAttrs.SourceLanguage);

    RefCntAutoPtr<IShader> pCS;
    {
        CreationAttrs.Desc       = {"PSO Compatibility test CS", SHADER_TYPE_COMPUTE, true};
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Source     = CSSource;
        pDevice->CreateShader(CreationAttrs, &pCS);
        VERIFY_EXPR(pCS != nullptr);
    }
    PSOCreateInfo.pCS = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    VERIFY_EXPR(pPSO != nullptr);

    return pPSO;
}

TEST(PSOCompatibility, IsCompatibleWith)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pContext   = pEnv->GetDeviceContext();
    auto* const pSwapChain = pEnv->GetSwapChain();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto PSO0 = CreateGraphicsPSO(pEnv, VS0, PS0);
    ASSERT_TRUE(PSO0);
    EXPECT_TRUE(PSO0->IsCompatibleWith(PSO0));
    auto PSO0_1 = CreateGraphicsPSO(pEnv, VS0, PS0);
    ASSERT_TRUE(PSO0_1);
    EXPECT_TRUE(PSO0->IsCompatibleWith(PSO0_1));
    EXPECT_TRUE(PSO0_1->IsCompatibleWith(PSO0));

    auto PSO_Tex      = CreateGraphicsPSO(pEnv, VS0, PS_Tex);
    auto PSO_Tex2     = CreateGraphicsPSO(pEnv, VS0, PS_Tex2);
    auto PSO_TexArr   = CreateGraphicsPSO(pEnv, VS0, PS_TexArr);
    auto PSO_ArrOfTex = CreateGraphicsPSO(pEnv, VS0, PS_ArrOfTex);
    ASSERT_TRUE(PSO_Tex);
    ASSERT_TRUE(PSO_Tex2);
    ASSERT_TRUE(PSO_TexArr);
    ASSERT_TRUE(PSO_ArrOfTex);
    EXPECT_TRUE(PSO_Tex->IsCompatibleWith(PSO_Tex2));

    // From resource signature point of view, texture and texture array are compatible
    EXPECT_TRUE(PSO_Tex->IsCompatibleWith(PSO_TexArr));

    EXPECT_FALSE(PSO_Tex->IsCompatibleWith(PSO_ArrOfTex));
    // From resource signature point of view, texture and texture array are compatible
    EXPECT_TRUE(PSO_Tex2->IsCompatibleWith(PSO_TexArr));
    EXPECT_FALSE(PSO_Tex2->IsCompatibleWith(PSO_ArrOfTex));
    EXPECT_FALSE(PSO_TexArr->IsCompatibleWith(PSO_ArrOfTex));

    auto PSO_CB  = CreateGraphicsPSO(pEnv, VS0, PS_CB);
    auto PSO1_CB = CreateGraphicsPSO(pEnv, VS0, PS1_CB);
    auto PSO_2CB = CreateGraphicsPSO(pEnv, VS0, PS_2CB);
    EXPECT_TRUE(PSO_CB->IsCompatibleWith(PSO1_CB));
    EXPECT_FALSE(PSO_CB->IsCompatibleWith(PSO_2CB));

    auto PSO_TexCB  = CreateGraphicsPSO(pEnv, VS0, PS_TexCB);
    auto PSO_TexCB2 = CreateGraphicsPSO(pEnv, VS0, PS_TexCB2);
    EXPECT_TRUE(PSO_TexCB->IsCompatibleWith(PSO_TexCB2));
    EXPECT_TRUE(PSO_TexCB2->IsCompatibleWith(PSO_TexCB));

    if (pDevice->GetDeviceInfo().Features.ComputeShaders)
    {
        auto PSO_RWBuff  = CreateComputePSO(pEnv, CS_RwBuff);
        auto PSO_RWBuff2 = CreateComputePSO(pEnv, CS_RwBuff2);
        auto PSO_RWBuff3 = CreateComputePSO(pEnv, CS_RwBuff3);
        EXPECT_TRUE(PSO_RWBuff);
        EXPECT_TRUE(PSO_RWBuff2);
        EXPECT_TRUE(PSO_RWBuff3);
        EXPECT_TRUE(PSO_RWBuff->IsCompatibleWith(PSO_RWBuff2));
        EXPECT_FALSE(PSO_RWBuff->IsCompatibleWith(PSO_RWBuff3));
    }

    {
        auto  pTex     = pEnv->CreateTexture("PSOCompatibility test text", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, 512, 512);
        auto  pSampler = pEnv->CreateSampler(Sam_LinearClamp);
        auto* pSRV     = pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pSRV->SetSampler(pSampler);
        PSO_Tex->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D")->Set(pSRV);
        RefCntAutoPtr<IShaderResourceBinding> pSRB_Tex;
        PSO_Tex->CreateShaderResourceBinding(&pSRB_Tex, true);

        IDeviceObject* ppSRVs[] = {pSRV, pSRV};
        PSO_ArrOfTex->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D")->SetArray(ppSRVs, 0, 2);
        RefCntAutoPtr<IShaderResourceBinding> pSRB_ArrOfTex;
        PSO_ArrOfTex->CreateShaderResourceBinding(&pSRB_ArrOfTex, true);

        ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(PSO_Tex);
        pContext->CommitShaderResources(pSRB_Tex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(drawAttrs);

        pSRB_Tex.Release();

        EXPECT_FALSE(PSO_Tex->IsCompatibleWith(PSO_ArrOfTex));
        pContext->SetPipelineState(PSO_ArrOfTex);
        pContext->CommitShaderResources(pSRB_ArrOfTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->Draw(drawAttrs);
    }
}

} // namespace
