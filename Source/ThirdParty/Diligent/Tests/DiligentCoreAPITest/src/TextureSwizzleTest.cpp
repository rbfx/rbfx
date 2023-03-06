/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "FastRand.hpp"
#include "GraphicsUtilities.h"
#include "MapHelper.hpp"

#include "InlineShaders/DrawCommandTestHLSL.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace Diligent
{
namespace Testing
{
void RenderDrawCommandReference(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
}
} // namespace Diligent


namespace
{

namespace HLSL
{

// clang-format off
const std::string TextureSwizzleTest_PS{
R"(

cbuffer Constants
{
    float4 g_Reference;
}

Texture2D    g_Tex;
SamplerState g_Tex_sampler;

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

float4 main(in PSInput PSIn) : SV_Target
{
    float4 TexVal = g_Tex.SampleLevel(g_Tex_sampler, float2(0.5, 0.5), 0);
    return float4(PSIn.Color.rgb, 1.0) * CheckValue(TexVal, g_Reference);
}
)"
};
// clang-format on

} // namespace HLSL

class TextureSwizzleTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        auto* const pDevice    = pEnv->GetDevice();
        auto* const pSwapChain = pEnv->GetSwapChain();

        CreateUniformBuffer(pDevice, 64, "Texture Swizzle Test constants", &Res.pConstants);
        ASSERT_TRUE(Res.pConstants);

        ShaderCreateInfo ShaderCI;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Source         = HLSL::DrawTest_ProceduralTriangleVS.c_str();
            ShaderCI.Desc           = {"Texture Swizzle Test - VS", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint     = "main";
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_TRUE(pVS);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Source         = HLSL::TextureSwizzleTest_PS.c_str();
            ShaderCI.Desc           = {"Texture Swizzle Test - PS", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint     = "main";
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_TRUE(pPS);
        }

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Texture Swizzle Test";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        constexpr ShaderResourceVariableDesc Resources[] =
            {
                {SHADER_TYPE_PIXEL, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
                {SHADER_TYPE_PIXEL, "g_Tex", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            };
        PSODesc.ResourceLayout.NumVariables = _countof(Resources);
        PSODesc.ResourceLayout.Variables    = Resources;

        constexpr ImmutableSamplerDesc ImmutableSamplers[] =
            {
                {SHADER_TYPE_PIXEL, "g_Tex", SamplerDesc{}},
            };
        PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);
        PSODesc.ResourceLayout.ImmutableSamplers    = ImmutableSamplers;

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &Res.pPSO);
        ASSERT_NE(Res.pPSO, nullptr);

        Res.pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(Res.pConstants);

        Res.pPSO->CreateShaderResourceBinding(&Res.pSRB, true);
        ASSERT_NE(Res.pSRB, nullptr);
    }

    static void TearDownTestSuite()
    {
        GPUTestingEnvironment::GetInstance()->Reset();
        Res = {};
    }

    void RunTest(Uint32 RGBA, const TextureComponentMapping& Swizzle, const float4& ExpectedValue)
    {
        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        auto* const pDevice    = pEnv->GetDevice();
        auto* const pContext   = pEnv->GetDeviceContext();
        auto* const pSwapChain = pEnv->GetSwapChain();
        const auto& DeviceInfo = pDevice->GetDeviceInfo();

        if (!DeviceInfo.Features.TextureComponentSwizzle)
        {
            EXPECT_FALSE(DeviceInfo.Type == RENDER_DEVICE_TYPE_D3D12 || DeviceInfo.Type == RENDER_DEVICE_TYPE_VULKAN || DeviceInfo.Type == RENDER_DEVICE_TYPE_METAL);
            GTEST_SKIP();
        }

        constexpr Uint32    Dim = 128;
        std::vector<Uint32> Data(Dim * Dim, RGBA);
        auto                pTex = pEnv->CreateTexture("Texture swizzle test", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, Dim, Dim, Data.data());
        ASSERT_TRUE(pTex);

        TextureViewDesc ViewDesc;
        ViewDesc.Name     = "Texture swizzle test";
        ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
        ViewDesc.Swizzle  = Swizzle;
        RefCntAutoPtr<ITextureView> pTexView;
        pTex->CreateView(ViewDesc, &pTexView);
        ASSERT_TRUE(pTexView);

        Res.pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Tex")->Set(pTexView);

        static FastRandFloat rnd{0, 0, 1};
        const float          ClearColor[] = {rnd(), rnd(), rnd(), rnd()};
        RenderDrawCommandReference(pSwapChain, ClearColor);

        {
            MapHelper<float4> pBuffData{pContext, Res.pConstants, MAP_WRITE, MAP_FLAG_DISCARD};
            *pBuffData = ExpectedValue;
        }

        ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(Res.pPSO);
        pContext->CommitShaderResources(Res.pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);

        pSwapChain->Present();
    }

    struct Resources
    {
        RefCntAutoPtr<IPipelineState>         pPSO;
        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        RefCntAutoPtr<IBuffer>                pConstants;
    };
    static Resources Res;
};

TextureSwizzleTest::Resources TextureSwizzleTest::Res;


TEST_F(TextureSwizzleTest, Red)
{
    constexpr Uint32 RGBA = 0xFFu << 0u;
    RunTest(RGBA, TextureComponentMapping{}, float4{1, 0, 0, 0});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_R,
                TEXTURE_COMPONENT_SWIZZLE_R,
                TEXTURE_COMPONENT_SWIZZLE_R,
                TEXTURE_COMPONENT_SWIZZLE_R},
            float4{1, 1, 1, 1});
}

TEST_F(TextureSwizzleTest, Green)
{
    constexpr Uint32 RGBA = 0xFFu << 8u;
    RunTest(RGBA, TextureComponentMapping{}, float4{0, 1, 0, 0});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_G,
                TEXTURE_COMPONENT_SWIZZLE_G,
                TEXTURE_COMPONENT_SWIZZLE_G,
                TEXTURE_COMPONENT_SWIZZLE_G},
            float4{1, 1, 1, 1});
}

TEST_F(TextureSwizzleTest, Blue)
{
    constexpr Uint32 RGBA = 0xFFu << 16u;
    RunTest(RGBA, TextureComponentMapping{}, float4{0, 0, 1, 0});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_B,
                TEXTURE_COMPONENT_SWIZZLE_B,
                TEXTURE_COMPONENT_SWIZZLE_B,
                TEXTURE_COMPONENT_SWIZZLE_B},
            float4{1, 1, 1, 1});
}

TEST_F(TextureSwizzleTest, Alpha)
{
    constexpr Uint32 RGBA = 0xFFu << 24u;
    RunTest(RGBA, TextureComponentMapping{}, float4{0, 0, 0, 1});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_A,
                TEXTURE_COMPONENT_SWIZZLE_A,
                TEXTURE_COMPONENT_SWIZZLE_A,
                TEXTURE_COMPONENT_SWIZZLE_A},
            float4{1, 1, 1, 1});
}

TEST_F(TextureSwizzleTest, One)
{
    constexpr Uint32 RGBA = 0;
    RunTest(RGBA, TextureComponentMapping{}, float4{0, 0, 0, 0});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_ONE,
                TEXTURE_COMPONENT_SWIZZLE_ONE,
                TEXTURE_COMPONENT_SWIZZLE_ONE,
                TEXTURE_COMPONENT_SWIZZLE_ONE},
            float4{1, 1, 1, 1});
}

TEST_F(TextureSwizzleTest, Zero)
{
    constexpr Uint32 RGBA = 0xFFFFFFFFu;
    RunTest(RGBA, TextureComponentMapping{}, float4{1, 1, 1, 1});

    RunTest(RGBA,
            TextureComponentMapping{
                TEXTURE_COMPONENT_SWIZZLE_ZERO,
                TEXTURE_COMPONENT_SWIZZLE_ZERO,
                TEXTURE_COMPONENT_SWIZZLE_ZERO,
                TEXTURE_COMPONENT_SWIZZLE_ZERO},
            float4{0, 0, 0, 0});
}

} // namespace
