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

#include "TestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/RasterizationRateMapTestMSL.h"
#include "VariableShadingRateTestConstants.hpp"

#include <Metal/Metal.h>
#include "DeviceContextMtl.h"
#include "RenderDeviceMtl.h"

namespace Diligent
{
namespace Testing
{

void RasterizationRateMapReferenceMtl(ISwapChain* pSwapChain);

} // namespace Testing
} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

using VRSTestingConstants::TextureBased::GenColRowFp32;

constexpr Uint32 TileSize = 4;

void CreateRasterizationRateMap(RefCntAutoPtr<ITextureView>& pShadingRateMap,
                                RefCntAutoPtr<IBuffer>&      pShadingRateParamBuffer,
                                RefCntAutoPtr<ITexture>&     pIntermediateRT) API_AVAILABLE(ios(13), macosx(10.15.4))
{
    auto*       pEnv    = GPUTestingEnvironment::GetInstance();
    auto*       pDevice = pEnv->GetDevice();
    const auto& SCDesc  = pEnv->GetSwapChain()->GetDesc();

    RasterizationRateMapCreateInfo RasterRateMapCI;
    RasterRateMapCI.Desc.ScreenWidth  = SCDesc.Width;
    RasterRateMapCI.Desc.ScreenHeight = SCDesc.Height;
    RasterRateMapCI.Desc.LayerCount   = 1;

    std::vector<float> Horizontal(RasterRateMapCI.Desc.ScreenWidth / TileSize);
    std::vector<float> Vertical(RasterRateMapCI.Desc.ScreenHeight / TileSize);

    for (size_t i = 0; i < Horizontal.size(); ++i)
        Horizontal[i] = GenColRowFp32(i, Horizontal.size());

    for (size_t i = 0; i < Vertical.size(); ++i)
        Vertical[i] = GenColRowFp32(i, Vertical.size());

    RasterizationRateLayerDesc Layer;
    Layer.HorizontalCount   = static_cast<Uint32>(Horizontal.size());
    Layer.VerticalCount     = static_cast<Uint32>(Vertical.size());
    Layer.pHorizontal       = Horizontal.data();
    Layer.pVertical         = Vertical.data();
    RasterRateMapCI.pLayers = &Layer;

    RefCntAutoPtr<IRenderDeviceMtl> pDeviceMtl{pDevice, IID_RenderDeviceMtl};
    RefCntAutoPtr<IRasterizationRateMapMtl> pRasterizationRateMap;
    pDeviceMtl->CreateRasterizationRateMap(RasterRateMapCI, &pRasterizationRateMap);
    ASSERT_NE(pRasterizationRateMap, nullptr);
    pShadingRateMap = pRasterizationRateMap->GetView();

    Uint64 BufferSize;
    Uint32 BufferAlign;
    pRasterizationRateMap->GetParameterBufferSizeAndAlign(BufferSize, BufferAlign);

    BufferDesc BuffDesc;
    BuffDesc.Name           = "RRM parameters buffer";
    BuffDesc.Size           = BufferSize;
    BuffDesc.Usage          = USAGE_UNIFIED;       // buffer is used for host access and will be accessed in shader
    BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER; // only uniform buffer is compatible with unified memory
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pShadingRateParamBuffer);

    pRasterizationRateMap->CopyParameterDataToBuffer(pShadingRateParamBuffer, 0);
    ASSERT_NE(pShadingRateParamBuffer, nullptr);

    Uint32 Width, Height;
    pRasterizationRateMap->GetPhysicalSizeForLayer(0, Width, Height);

    TextureDesc TexDesc;
    TexDesc.Name      = "Intermediate render target";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = Width;
    TexDesc.Height    = Height;
    TexDesc.Format    = SCDesc.ColorBufferFormat;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

    pDevice->CreateTexture(TexDesc, nullptr, &pIntermediateRT);
    ASSERT_NE(pIntermediateRT, nullptr);
}


void CreatePass1PSO(RefCntAutoPtr<IPipelineState>& pPSO,
                    IRenderPass*                   pRenderPass = nullptr) API_AVAILABLE(ios(13), macosx(10.15.4))
{
    auto*       pEnv    = GPUTestingEnvironment::GetInstance();
    auto*       pDevice = pEnv->GetDevice();
    const auto& SCDesc  = pEnv->GetSwapChain()->GetDesc();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Raster rate map Pass1 PSO";

    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;

    if (pRenderPass)
    {
        GraphicsPipeline.pRenderPass = pRenderPass;
    }
    else
    {
        GraphicsPipeline.NumRenderTargets = 1;
        GraphicsPipeline.RTVFormats[0]    = SCDesc.ColorBufferFormat;
    }

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    GraphicsPipeline.ShadingRateFlags             = PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED;

    const LayoutElement Elements[] = {
        {0, 0, 2, VT_FLOAT32, False, offsetof(PosAndRate, Pos)},
        {1, 0, 1, VT_UINT32, False, offsetof(PosAndRate, Rate)} //
    };
    GraphicsPipeline.InputLayout.NumElements    = _countof(Elements);
    GraphicsPipeline.InputLayout.LayoutElements = Elements;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "VSmain";
        ShaderCI.Desc.Name       = "Raster rate map Pass1 - VS";
        ShaderCI.Source          = MSL::RasterRateMapTest_Pass1.c_str();

        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "PSmain";
        ShaderCI.Desc.Name       = "Raster rate map Pass1 - PS";
        ShaderCI.Source          = MSL::RasterRateMapTest_Pass1.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);
}


void CreatePass2PSO(RefCntAutoPtr<IPipelineState>&         pPSO,
                    RefCntAutoPtr<IShaderResourceBinding>& pSRB) API_AVAILABLE(ios(13), macosx(10.15.4))
{
    auto*       pEnv    = GPUTestingEnvironment::GetInstance();
    auto*       pDevice = pEnv->GetDevice();
    const auto& SCDesc  = pEnv->GetSwapChain()->GetDesc();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Raster rate map Pass1 PSO";

    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = SCDesc.ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;
    GraphicsPipeline.DepthStencilDesc.DepthEnable         = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "VSmain";
        ShaderCI.Desc.Name       = "Raster rate map Pass2 - VS";
        ShaderCI.Source          = MSL::RasterRateMapTest_Pass2.c_str();

        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "PSmain";
        ShaderCI.Desc.Name       = "Raster rate map Pass2 - PS";
        ShaderCI.Source          = MSL::RasterRateMapTest_Pass2.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pPSO->CreateShaderResourceBinding(&pSRB);
}


void CreateVertexBuffer(RefCntAutoPtr<IBuffer>& pVB, Uint32& VertCount)
{
    auto*       pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();
    const auto& Verts   = VRSTestingConstants::PerPrimitive::Vertices;

    BufferData BuffData{Verts, sizeof(Verts)};
    BufferDesc BuffDesc;
    BuffDesc.Name      = "Vertex buffer";
    BuffDesc.Size      = BuffData.DataSize;
    BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    BuffDesc.Usage     = USAGE_IMMUTABLE;

    pDevice->CreateBuffer(BuffDesc, &BuffData, &pVB);
    ASSERT_NE(pVB, nullptr);

    VertCount = _countof(Verts);
}


TEST(VariableShadingRateTest, RasterRateMap)
{
    if (@available(macos 10.15.4, ios 13.0, *))
    {
        auto*       pEnv       = GPUTestingEnvironment::GetInstance();
        auto*       pDevice    = pEnv->GetDevice();
        const auto& deviceInfo = pDevice->GetDeviceInfo();

        if (!deviceInfo.IsMetalDevice())
            GTEST_SKIP();

        if (!deviceInfo.Features.VariableRateShading)
            GTEST_SKIP();

        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
        if (pTestingSwapChain)
        {
            pContext->Flush();
            pContext->InvalidateState();

            RasterizationRateMapReferenceMtl(pSwapChain);
            pTestingSwapChain->TakeSnapshot();
        }

        GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

        RefCntAutoPtr<IPipelineState> pPSOPass1;
        CreatePass1PSO(pPSOPass1);

        RefCntAutoPtr<ITextureView> pShadingRateMap;
        RefCntAutoPtr<IBuffer>      pShadingRateParamBuffer;
        RefCntAutoPtr<ITexture>     pIntermediateRT;
        CreateRasterizationRateMap(pShadingRateMap, pShadingRateParamBuffer, pIntermediateRT);

        RefCntAutoPtr<IPipelineState>         pPSOPass2;
        RefCntAutoPtr<IShaderResourceBinding> pSRBPass2;
        CreatePass2PSO(pPSOPass2, pSRBPass2);
        pSRBPass2->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pIntermediateRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRBPass2->GetVariableByName(SHADER_TYPE_PIXEL, "g_RRMData")->Set(pShadingRateParamBuffer);

        RefCntAutoPtr<IBuffer> pVB;
        Uint32                 VertCount = 0;
        CreateVertexBuffer(pVB, VertCount);

        // Pass 1
        {
            ITextureView*           pRTVs[] = {pIntermediateRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
            SetRenderTargetsAttribs RTAttrs;
            RTAttrs.NumRenderTargets    = 1;
            RTAttrs.ppRenderTargets     = pRTVs;
            RTAttrs.pShadingRateMap     = pShadingRateMap;
            RTAttrs.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            pContext->SetRenderTargetsExt(RTAttrs);

            const float ClearColor[] = {1.f, 0.f, 0.f, 1.f};
            pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            pContext->SetShadingRate(SHADING_RATE_1X1, SHADING_RATE_COMBINER_PASSTHROUGH, SHADING_RATE_COMBINER_OVERRIDE);

            pContext->SetPipelineState(pPSOPass1);

            IBuffer*     VBuffers[] = {pVB};
            const Uint64 Offsets[]  = {0};
            pContext->SetVertexBuffers(0, 1, VBuffers, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

            DrawAttribs drawAttrs{VertCount, DRAW_FLAG_VERIFY_ALL};
            pContext->Draw(drawAttrs);
        }

        // Pass 2
        {
            ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
            pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            pContext->SetPipelineState(pPSOPass2);
            pContext->CommitShaderResources(pSRBPass2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
            pContext->Draw(drawAttrs);
        }

        pSwapChain->Present();
    }
}


TEST(VariableShadingRateTest, RasterRateMapWithRenderPass)
{
    if (@available(macos 10.15.4, ios 13.0, *))
    {
        auto*       pEnv       = GPUTestingEnvironment::GetInstance();
        auto*       pDevice    = pEnv->GetDevice();
        const auto& deviceInfo = pDevice->GetDeviceInfo();

        if (!deviceInfo.IsMetalDevice())
            GTEST_SKIP();

        if (!deviceInfo.Features.VariableRateShading)
            GTEST_SKIP();

        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
        if (pTestingSwapChain)
        {
            pContext->Flush();
            pContext->InvalidateState();

            RasterizationRateMapReferenceMtl(pSwapChain);
            pTestingSwapChain->TakeSnapshot();
        }

        GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

        RefCntAutoPtr<ITextureView> pShadingRateMap;
        RefCntAutoPtr<IBuffer>      pShadingRateParamBuffer;
        RefCntAutoPtr<ITexture>     pIntermediateRT;
        CreateRasterizationRateMap(pShadingRateMap, pShadingRateParamBuffer, pIntermediateRT);

        RefCntAutoPtr<IRenderPass> pRenderPass;
        {
            RenderPassAttachmentDesc Attachments[2];

            Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
            Attachments[0].SampleCount  = 1;
            Attachments[0].InitialState = pIntermediateRT->GetState();
            Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
            Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
            Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

            Attachments[1].Format       = TEX_FORMAT_RG32_FLOAT; // format ignored but for validation it must not be unknown
            Attachments[1].SampleCount  = 1;
            Attachments[1].InitialState = RESOURCE_STATE_SHADING_RATE;
            Attachments[1].FinalState   = RESOURCE_STATE_SHADING_RATE;
            Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_LOAD;
            Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

            SubpassDesc           Subpass;
            AttachmentReference   RTAttachmentRef = {0, RESOURCE_STATE_RENDER_TARGET};
            ShadingRateAttachment SRAttachment    = {{1, RESOURCE_STATE_SHADING_RATE}, 0, 0}; // tile size is ignored in Metal

            Subpass.RenderTargetAttachmentCount = 1;
            Subpass.pRenderTargetAttachments    = &RTAttachmentRef;
            Subpass.pShadingRateAttachment      = &SRAttachment;

            RenderPassDesc RPDesc;
            RPDesc.Name            = "Render pass with rasterization rate";
            RPDesc.AttachmentCount = _countof(Attachments);
            RPDesc.pAttachments    = Attachments;
            RPDesc.SubpassCount    = 1;
            RPDesc.pSubpasses      = &Subpass;

            pDevice->CreateRenderPass(RPDesc, &pRenderPass);
            ASSERT_NE(pRenderPass, nullptr);
        }

        RefCntAutoPtr<IFramebuffer> pFramebuffer;
        {
            ITextureView* pTexViews[] = {
                pIntermediateRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
                pShadingRateMap //
            };

            FramebufferDesc FBDesc;
            FBDesc.Name            = "Framebuffer with rasterization rate";
            FBDesc.pRenderPass     = pRenderPass;
            FBDesc.AttachmentCount = _countof(pTexViews);
            FBDesc.ppAttachments   = pTexViews;

            pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
            ASSERT_NE(pFramebuffer, nullptr);
        }

        RefCntAutoPtr<IPipelineState> pPSOPass1;
        CreatePass1PSO(pPSOPass1, pRenderPass);

        RefCntAutoPtr<IPipelineState>         pPSOPass2;
        RefCntAutoPtr<IShaderResourceBinding> pSRBPass2;
        CreatePass2PSO(pPSOPass2, pSRBPass2);
        pSRBPass2->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pIntermediateRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRBPass2->GetVariableByName(SHADER_TYPE_PIXEL, "g_RRMData")->Set(pShadingRateParamBuffer);

        RefCntAutoPtr<IBuffer> pVB;
        Uint32                 VertCount = 0;
        CreateVertexBuffer(pVB, VertCount);

        StateTransitionDesc Barriers[] = {
            {pIntermediateRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET, STATE_TRANSITION_FLAG_UPDATE_STATE},
            {pVB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE},
        };
        pContext->TransitionResourceStates(_countof(Barriers), Barriers);

        // Pass 1
        {
            OptimizedClearValue ClearValues[1];
            ClearValues[0].SetColor(TEX_FORMAT_RGBA8_UNORM, 1.f, 0.f, 0.f, 1.f);

            BeginRenderPassAttribs RPBeginInfo;
            RPBeginInfo.pRenderPass         = pRenderPass;
            RPBeginInfo.pFramebuffer        = pFramebuffer;
            RPBeginInfo.pClearValues        = ClearValues;
            RPBeginInfo.ClearValueCount     = _countof(ClearValues);
            RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            pContext->BeginRenderPass(RPBeginInfo);

            pContext->SetShadingRate(SHADING_RATE_1X1, SHADING_RATE_COMBINER_PASSTHROUGH, SHADING_RATE_COMBINER_OVERRIDE);

            pContext->SetPipelineState(pPSOPass1);

            IBuffer* VBuffers[] = {pVB};
            pContext->SetVertexBuffers(0, 1, VBuffers, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);

            DrawAttribs drawAttrs{VertCount, DRAW_FLAG_VERIFY_ALL};
            pContext->Draw(drawAttrs);

            pContext->EndRenderPass();
        }

        // Pass 2
        {
            ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
            pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            pContext->SetPipelineState(pPSOPass2);
            pContext->CommitShaderResources(pSRBPass2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
            pContext->Draw(drawAttrs);
        }

        pSwapChain->Present();
    }
}

} // namespace
