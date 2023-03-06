/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "InlineShaders/TileShaderTestMSL.h"

namespace Diligent
{

namespace Testing
{

#if METAL_SUPPORTED
void TileShaderDrawReferenceMtl(ISwapChain* pSwapChain);
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(TileShaderTest, DrawQuad)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.TileShaders)
    {
        GTEST_SKIP() << "Tile shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto*       pSwapChain = pEnv->GetSwapChain();
    auto*       pContext   = pEnv->GetDeviceContext();
    const auto& SCDesc     = pSwapChain->GetDesc();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                TileShaderDrawReferenceMtl(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_VULKAN:
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }


    RefCntAutoPtr<IPipelineState> pGraphicsPSO;
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Tile shader test - graphics pipeline";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = SCDesc.ColorBufferFormat;
        GraphicsPipeline.DSVFormat                    = SCDesc.DepthBufferFormat;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc       = {"Tile shader test - VS", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint = "VSmain";
            ShaderCI.Source     = MSL::TileShaderTest1.c_str();

            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc       = {"Tile shader test - PS", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "PSmain";
            ShaderCI.Source     = MSL::TileShaderTest1.c_str();

            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pGraphicsPSO);
        ASSERT_NE(pGraphicsPSO, nullptr);
    }

    RefCntAutoPtr<IPipelineState> pTilePSO;
    {
        TilePipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc      = PSOCreateInfo.PSODesc;
        auto& TilePipeline = PSOCreateInfo.TilePipeline;

        PSODesc.Name = "Tile shader test - tile pipeline";

        PSODesc.PipelineType          = PIPELINE_TYPE_TILE;
        TilePipeline.NumRenderTargets = 1;
        TilePipeline.RTVFormats[0]    = SCDesc.ColorBufferFormat;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;

        RefCntAutoPtr<IShader> pTS;
        {
            ShaderCI.Desc       = {"Tile shader test - TLS", SHADER_TYPE_TILE, true};
            ShaderCI.EntryPoint = "TLSmain";
            ShaderCI.Source     = MSL::TileShaderTest1.c_str();

            pDevice->CreateShader(ShaderCI, &pTS);
            ASSERT_NE(pTS, nullptr);
        }

        PSOCreateInfo.pTS = pTS;
        pDevice->CreateTilePipelineState(PSOCreateInfo, &pTilePSO);
        ASSERT_NE(pTilePSO, nullptr);
    }


    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    ITextureView* pDSV    = pSwapChain->GetDepthBufferDSV();
    pContext->SetRenderTargets(1, pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.0f, 0.7f, 1.f, 1.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0F, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Graphics pass
    {
        pContext->SetPipelineState(pGraphicsPSO);

        DrawAttribs drawAttrs{4, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(drawAttrs);
    }

    // Tile shader pass
    {
        pContext->SetPipelineState(pTilePSO);

        Uint32 TileSizeX = 0, TileSizeY = 0;
        pContext->GetTileSize(TileSizeX, TileSizeY);
        ASSERT_NE(TileSizeX, 0u);
        ASSERT_NE(TileSizeY, 0u);

        pContext->DispatchTile(DispatchTileAttribs{1, 1, DRAW_FLAG_VERIFY_RENDER_TARGETS});
    }
    pSwapChain->Present();
}

} // namespace
