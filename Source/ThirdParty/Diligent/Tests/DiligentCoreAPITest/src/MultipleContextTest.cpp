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
#include "BasicMath.hpp"
#include "Align.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

// clang-format off
const std::string MultipleContextTest_QuadVS{R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
    float2 UV    : TEXCOORD;
};

void main(in uint vid : SV_VertexID,
          out PSInput PSIn)
{
    float2 uv  = float2(vid & 1, vid >> 1);
    PSIn.Pos   = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    PSIn.UV    = float2(uv.x, 1.0 - uv.y);
    PSIn.Color = float3(vid & 1, (vid + 1) & 1, (vid + 2) & 1);
}
)"};

const std::string MultipleContextTest_BlendPS{R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
    float2 UV    : TEXCOORD;
};

Texture2D<float4> g_Texture1;
Texture2D<float4> g_Texture2;
SamplerState      g_Sampler;

float4 main(in PSInput PSIn) : SV_Target
{
    float4 Color1 = g_Texture1.Sample(g_Sampler, PSIn.UV, 0);
    float4 Color2 = g_Texture2.Sample(g_Sampler, PSIn.UV, 0);

    return (Color1 + Color2) * 0.5;
}
)"};

const std::string MultipleContextTest_ProceduralSrc{R"(
cbuffer Constants
{
    float4 g_Time;
};

float4 GenColor(float2 uv)
{
    uv *= 10.0;
    float4 col = float(0.0).xxxx;

    col.r = sin(uv.x + g_Time.x) * cos(uv.y + g_Time.y);
    col.g = frac(uv.x + g_Time.z) * frac(uv.y + g_Time.w);
    return col;
}
)"};

const std::string MultipleContextTest_ProceduralPS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
    float2 UV    : TEXCOORD;
};)"
+ MultipleContextTest_ProceduralSrc +
R"(
float4 main(in PSInput PSIn) : SV_Target
{
    return GenColor(PSIn.UV);
}
)";

const std::string MultipleContextTest_ProceduralCS = R"(
RWTexture2D<float4> g_DstTexture;
)"
+ MultipleContextTest_ProceduralSrc +
R"(
[numthreads(4, 4, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 Dim;
    g_DstTexture.GetDimensions(Dim.x, Dim.y);
    if (DTid.x >= Dim.x || DTid.y >= Dim.y)
        return;

    g_DstTexture[DTid.xy] = GenColor((float2(DTid.xy) + 0.5) / float2(Dim));
}
)";
// clang-format on


class MultipleContextTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pDevice    = pEnv->GetDevice();
        auto* pSwapChain = pEnv->GetSwapChain();

        if (pEnv->GetNumImmediateContexts() <= 1)
            return;

        GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        // Graphics PSO
        {
            GraphicsPipelineStateCreateInfo PSOCreateInfo;
            auto&                           PSODesc          = PSOCreateInfo.PSODesc;
            auto&                           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

            PSODesc.Name                 = "Multiple context test - procedural graphics PSO";
            PSODesc.PipelineType         = PIPELINE_TYPE_GRAPHICS;
            PSODesc.ImmediateContextMask = ~0ull;

            PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

            GraphicsPipeline.NumRenderTargets             = 1;
            GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
            GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
            GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

            RefCntAutoPtr<IShader> pVS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Multiple context test - VS";
                ShaderCI.Source          = MultipleContextTest_QuadVS.c_str();
                pDevice->CreateShader(ShaderCI, &pVS);
                ASSERT_NE(pVS, nullptr);
            }

            RefCntAutoPtr<IShader> pProcPS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Multiple context test - procedural PS";
                ShaderCI.Source          = MultipleContextTest_ProceduralPS.c_str();
                pDevice->CreateShader(ShaderCI, &pProcPS);
                ASSERT_NE(pProcPS, nullptr);
            }

            PSOCreateInfo.pVS = pVS;
            PSOCreateInfo.pPS = pProcPS;
            pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawProceduralPSO);
            ASSERT_NE(sm_pDrawProceduralPSO, nullptr);


            SamplerDesc SamLinearWrapDesc{
                FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
                TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
            ImmutableSamplerDesc ImmutableSamplers[] =
                {
                    {SHADER_TYPE_PIXEL, "g_Sampler", SamLinearWrapDesc} //
                };
            ShaderResourceVariableDesc Veriables[] =
                {
                    {SHADER_TYPE_PIXEL, "g_Sampler", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
                };
            PSODesc.ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
            PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);
            PSODesc.ResourceLayout.Variables            = Veriables;
            PSODesc.ResourceLayout.NumVariables         = _countof(Veriables);
            PSODesc.ResourceLayout.DefaultVariableType  = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

            RefCntAutoPtr<IShader> pBlendPS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Multiple context test - blend PS";
                ShaderCI.Source          = MultipleContextTest_BlendPS.c_str();
                pDevice->CreateShader(ShaderCI, &pBlendPS);
                ASSERT_NE(pBlendPS, nullptr);
            }

            PSODesc.Name      = "Multiple context test - blend tex graphics PSO";
            PSOCreateInfo.pPS = pBlendPS;
            pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pBlendTexPSO);
            ASSERT_NE(sm_pBlendTexPSO, nullptr);
        }

        // Compute PSO
        {
            ComputePipelineStateCreateInfo PSOCreateInfo;

            RefCntAutoPtr<IShader> pCS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Multiple context test - procedural CS";
                ShaderCI.Source          = MultipleContextTest_ProceduralCS.c_str();
                pDevice->CreateShader(ShaderCI, &pCS);
                ASSERT_NE(pCS, nullptr);
            }

            PSOCreateInfo.PSODesc.Name = "Multiple context test - procedural compute PSO";
            PSOCreateInfo.pCS          = pCS;

            PSOCreateInfo.PSODesc.ImmediateContextMask               = ~0ull;
            PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

            pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pCompProceduralPSO);
            ASSERT_NE(sm_pCompProceduralPSO, nullptr);
        }

        sm_pBlendTexPSO->CreateShaderResourceBinding(&sm_pBlendTexSRB, true);
        ASSERT_NE(sm_pBlendTexSRB, nullptr);
        sm_pDrawProceduralPSO->CreateShaderResourceBinding(&sm_pDrawProceduralSRB, true);
        ASSERT_NE(sm_pDrawProceduralSRB, nullptr);
        sm_pCompProceduralPSO->CreateShaderResourceBinding(&sm_pCompProceduralSRB, true);
        ASSERT_NE(sm_pCompProceduralSRB, nullptr);

        const auto& SCDesc = pSwapChain->GetDesc();
        sm_DispatchSize    = uint2{(SCDesc.Width + 3) / 4, (SCDesc.Height + 3) / 4}; // must be same as numthreads(...)
    }

    static void TearDownTestSuite()
    {
        sm_pBlendTexPSO.Release();
        sm_pDrawProceduralPSO.Release();
        sm_pCompProceduralPSO.Release();

        sm_pBlendTexSRB.Release();
        sm_pDrawProceduralSRB.Release();
        sm_pCompProceduralSRB.Release();

        auto* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static RefCntAutoPtr<ITexture> CreateTexture(BIND_FLAGS Flags, Uint64 QueueMask, const char* Name, IDeviceContext* pInitialCtx)
    {
        auto*       pEnv       = GPUTestingEnvironment::GetInstance();
        auto*       pDevice    = pEnv->GetDevice();
        auto*       pSwapChain = pEnv->GetSwapChain();
        const auto& SCDesc     = pSwapChain->GetDesc();

        TextureDesc Desc;
        Desc.Name                 = Name;
        Desc.Type                 = RESOURCE_DIM_TEX_2D;
        Desc.Width                = SCDesc.Width;
        Desc.Height               = SCDesc.Height;
        Desc.Format               = TEX_FORMAT_RGBA8_UNORM;
        Desc.Usage                = USAGE_DEFAULT;
        Desc.BindFlags            = Flags;
        Desc.ImmediateContextMask = QueueMask | (1ull << pInitialCtx->GetDesc().ContextId);

        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture(Desc, nullptr, &pTexture);
        return pTexture;
    }

    static RefCntAutoPtr<IPipelineState> sm_pBlendTexPSO;
    static RefCntAutoPtr<IPipelineState> sm_pDrawProceduralPSO;
    static RefCntAutoPtr<IPipelineState> sm_pCompProceduralPSO;

    static RefCntAutoPtr<IShaderResourceBinding> sm_pBlendTexSRB;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pDrawProceduralSRB;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pCompProceduralSRB;

    static uint2 sm_DispatchSize;
};

RefCntAutoPtr<IPipelineState> MultipleContextTest::sm_pBlendTexPSO;
RefCntAutoPtr<IPipelineState> MultipleContextTest::sm_pDrawProceduralPSO;
RefCntAutoPtr<IPipelineState> MultipleContextTest::sm_pCompProceduralPSO;

RefCntAutoPtr<IShaderResourceBinding> MultipleContextTest::sm_pBlendTexSRB;
RefCntAutoPtr<IShaderResourceBinding> MultipleContextTest::sm_pDrawProceduralSRB;
RefCntAutoPtr<IShaderResourceBinding> MultipleContextTest::sm_pCompProceduralSRB;

uint2 MultipleContextTest::sm_DispatchSize;


TEST_F(MultipleContextTest, GraphicsAndComputeQueue)
{
    auto*           pEnv         = GPUTestingEnvironment::GetInstance();
    auto*           pDevice      = pEnv->GetDevice();
    auto*           pSwapChain   = pEnv->GetSwapChain();
    IDeviceContext* pGraphicsCtx = nullptr;
    IDeviceContext* pComputeCtx  = nullptr;
    {
        constexpr auto  QueueTypeMask = COMMAND_QUEUE_TYPE_GRAPHICS | COMMAND_QUEUE_TYPE_COMPUTE;
        IDeviceContext* pGraphicsCtx2 = nullptr;

        for (Uint32 CtxInd = 0; CtxInd < pEnv->GetNumImmediateContexts(); ++CtxInd)
        {
            auto*       Ctx  = pEnv->GetDeviceContext(CtxInd);
            const auto& Desc = Ctx->GetDesc();

            if (!pGraphicsCtx && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_GRAPHICS)
                pGraphicsCtx = Ctx;
            else if (!pGraphicsCtx2 && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_GRAPHICS)
                pGraphicsCtx2 = Ctx;
            else if (!pComputeCtx && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_COMPUTE)
                pComputeCtx = Ctx;
        }

        if (!pComputeCtx)
            pComputeCtx = pGraphicsCtx2;
    }
    if (!pGraphicsCtx || !pComputeCtx)
    {
        GTEST_SKIP() << "Compute queue is not supported by this device";
    }
    ASSERT_NE(pGraphicsCtx->GetDesc().ContextId, pComputeCtx->GetDesc().ContextId);

    const Uint64 QueueMask = (1ull << pGraphicsCtx->GetDesc().ContextId) | (1ull << pComputeCtx->GetDesc().ContextId);

    RefCntAutoPtr<IBuffer> pConstants1;
    RefCntAutoPtr<IBuffer> pConstants2;
    {
        const float4 ConstData1 = float4(1.2f, 0.25f, 1.1f, 0.14f);
        const float4 ConstData2 = float4(0.8f, 1.53f, 0.6f, 1.72f);

        BufferDesc BuffDesc;
        BuffDesc.Name                 = "Constants";
        BuffDesc.Size                 = sizeof(ConstData1);
        BuffDesc.BindFlags            = BIND_UNIFORM_BUFFER;
        BuffDesc.ImmediateContextMask = QueueMask;

        BufferData BuffData1{&ConstData1, sizeof(ConstData1)};
        pDevice->CreateBuffer(BuffDesc, &BuffData1, &pConstants1);
        ASSERT_NE(pConstants1, nullptr);

        BufferData BuffData2{&ConstData2, sizeof(ConstData2)};
        pDevice->CreateBuffer(BuffDesc, &BuffData2, &pConstants2);
        ASSERT_NE(pConstants2, nullptr);
    }

    // Draw reference in single queue
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto* pRTV        = pSwapChain->GetCurrentBackBufferRTV();
        auto  pTextureRT  = CreateTexture(BIND_SHADER_RESOURCE | BIND_RENDER_TARGET, 0, "TextureRT", pGraphicsCtx);
        auto  pTextureUAV = CreateTexture(BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS, 0, "TextureUAV", pGraphicsCtx);
        ASSERT_NE(pTextureRT, nullptr);
        ASSERT_NE(pTextureUAV, nullptr);

        const auto DefaultTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        // graphics pass
        {
            sm_pDrawProceduralSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(pConstants1);

            ITextureView* pRTVs[] = {pTextureRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
            pGraphicsCtx->SetRenderTargets(1, pRTVs, nullptr, DefaultTransitionMode);

            pGraphicsCtx->SetPipelineState(sm_pDrawProceduralPSO);
            pGraphicsCtx->CommitShaderResources(sm_pDrawProceduralSRB, DefaultTransitionMode);
            pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_VERIFY_ALL});
        }

        // compute pass
        {
            sm_pCompProceduralSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants2);
            sm_pCompProceduralSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pTextureUAV->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));

            pGraphicsCtx->SetPipelineState(sm_pCompProceduralPSO);
            pGraphicsCtx->CommitShaderResources(sm_pCompProceduralSRB, DefaultTransitionMode);
            pGraphicsCtx->DispatchCompute(DispatchComputeAttribs{sm_DispatchSize.x, sm_DispatchSize.y, 1});
        }

        // blend pass
        {
            sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture1")->Set(pTextureRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture2")->Set(pTextureUAV->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

            pGraphicsCtx->SetRenderTargets(1, &pRTV, nullptr, DefaultTransitionMode);

            pGraphicsCtx->SetPipelineState(sm_pBlendTexPSO);
            pGraphicsCtx->CommitShaderResources(sm_pBlendTexSRB, DefaultTransitionMode);
            pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_VERIFY_ALL});

            pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

            // Transition to CopySrc state to use in TakeSnapshot()
            StateTransitionDesc Barrier{pRTV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
            pGraphicsCtx->TransitionResourceStates(1, &Barrier);
        }

        pGraphicsCtx->WaitForIdle();
        pTestingSwapChain->TakeSnapshot(pRTV->GetTexture());
    }


    // Graphics:  |- draw -|  |- blend -|- present -|
    // Compute:   |- compute -|

    RefCntAutoPtr<IFence> pGraphicsFence;
    RefCntAutoPtr<IFence> pComputeFence;
    {
        FenceDesc FenceCI;
        FenceCI.Type = FENCE_TYPE_GENERAL;
        FenceCI.Name = "Graphics sync";
        pDevice->CreateFence(FenceCI, &pGraphicsFence);
        ASSERT_NE(pGraphicsFence, nullptr);

        FenceCI.Name = "Compute sync";
        pDevice->CreateFence(FenceCI, &pComputeFence);
        ASSERT_NE(pComputeFence, nullptr);
    }

    auto pTextureRT  = CreateTexture(BIND_SHADER_RESOURCE | BIND_RENDER_TARGET, QueueMask, "TextureRT", pGraphicsCtx);
    auto pTextureUAV = CreateTexture(BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS, QueueMask, "TextureUAV", pComputeCtx);
    ASSERT_NE(pTextureRT, nullptr);
    ASSERT_NE(pTextureUAV, nullptr);

    const Uint64 GraphicsFenceValue    = 11;
    const Uint64 ComputeFenceValue     = 22;
    const auto   DefaultTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;

    // graphics pass
    {
        sm_pDrawProceduralSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(pConstants1);

        // initial -> render_target
        const StateTransitionDesc Barrier1 = {pTextureRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET};
        pGraphicsCtx->TransitionResourceStates(1, &Barrier1);
        pTextureRT->SetState(RESOURCE_STATE_UNKNOWN); // disable implicit state transitions

        ITextureView* pRTVs[] = {pTextureRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
        pGraphicsCtx->SetRenderTargets(1, pRTVs, nullptr, DefaultTransitionMode);

        pGraphicsCtx->SetPipelineState(sm_pDrawProceduralPSO);
        pGraphicsCtx->CommitShaderResources(sm_pDrawProceduralSRB, DefaultTransitionMode);
        pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_NONE});

        pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, DefaultTransitionMode);

        // render_target -> shader_resource
        const StateTransitionDesc Barrier2 = {pTextureRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE};
        pGraphicsCtx->TransitionResourceStates(1, &Barrier2);

        pGraphicsCtx->EnqueueSignal(pGraphicsFence, GraphicsFenceValue);
        pGraphicsCtx->Flush();
    }

    // compute pass
    {
        sm_pCompProceduralSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants2);
        sm_pCompProceduralSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pTextureUAV->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));

        // initial -> UAV
        const StateTransitionDesc Barrier1 = {pTextureUAV, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_UNORDERED_ACCESS};
        pComputeCtx->TransitionResourceStates(1, &Barrier1);
        pTextureUAV->SetState(RESOURCE_STATE_UNKNOWN); // disable implicit state transitions

        pComputeCtx->SetPipelineState(sm_pCompProceduralPSO);
        pComputeCtx->CommitShaderResources(sm_pCompProceduralSRB, DefaultTransitionMode);
        pComputeCtx->DispatchCompute(DispatchComputeAttribs{sm_DispatchSize.x, sm_DispatchSize.y, 1});

        pComputeCtx->EnqueueSignal(pComputeFence, ComputeFenceValue);
        pComputeCtx->Flush();
    }

    // blend and present
    {
        sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture1")->Set(pTextureRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture2")->Set(pTextureUAV->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pGraphicsCtx->DeviceWaitForFence(pGraphicsFence, GraphicsFenceValue);
        pGraphicsCtx->DeviceWaitForFence(pComputeFence, ComputeFenceValue);

        auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();

        const StateTransitionDesc Barriers[] = {
            {pRTV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET, STATE_TRANSITION_FLAG_UPDATE_STATE}, // prev_state -> render_target
            {pTextureUAV, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE}                                  // UAV -> shader_resource
        };
        pGraphicsCtx->TransitionResourceStates(_countof(Barriers), Barriers);

        pGraphicsCtx->SetRenderTargets(1, &pRTV, nullptr, DefaultTransitionMode);

        pGraphicsCtx->SetPipelineState(sm_pBlendTexPSO);
        pGraphicsCtx->CommitShaderResources(sm_pBlendTexSRB, DefaultTransitionMode);
        pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_NONE});

        pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, DefaultTransitionMode);

        pGraphicsCtx->WaitForIdle();
        pSwapChain->Present();
    }

    pGraphicsCtx->FinishFrame();
    pComputeCtx->FinishFrame();

    pGraphicsFence->Wait(GraphicsFenceValue);
    pComputeFence->Wait(ComputeFenceValue);
}


TEST_F(MultipleContextTest, GraphicsAndTransferQueue)
{
    auto*           pEnv         = GPUTestingEnvironment::GetInstance();
    auto*           pDevice      = pEnv->GetDevice();
    auto*           pSwapChain   = pEnv->GetSwapChain();
    const auto&     SCDesc       = pSwapChain->GetDesc();
    IDeviceContext* pGraphicsCtx = nullptr;
    IDeviceContext* pTransferCtx = nullptr;
    {
        constexpr auto  QueueTypeMask = COMMAND_QUEUE_TYPE_GRAPHICS | COMMAND_QUEUE_TYPE_COMPUTE | COMMAND_QUEUE_TYPE_TRANSFER;
        IDeviceContext* pGraphicsCtx2 = nullptr;

        for (Uint32 CtxInd = 0; CtxInd < pEnv->GetNumImmediateContexts(); ++CtxInd)
        {
            auto*       Ctx  = pEnv->GetDeviceContext(CtxInd);
            const auto& Desc = Ctx->GetDesc();

            if (!pGraphicsCtx && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_GRAPHICS)
                pGraphicsCtx = Ctx;
            else if (!pGraphicsCtx2 && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_GRAPHICS)
                pGraphicsCtx2 = Ctx;
            else if (!pTransferCtx && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_TRANSFER)
                pTransferCtx = Ctx;
        }

        if (!pTransferCtx)
            pTransferCtx = pGraphicsCtx2;
    }
    if (!pGraphicsCtx || !pTransferCtx)
    {
        GTEST_SKIP() << "Transfer queue is not supported by this device";
    }
    ASSERT_NE(pGraphicsCtx->GetDesc().ContextId, pTransferCtx->GetDesc().ContextId);

    std::vector<Uint8> Pixels;
    {
        Pixels.resize(SCDesc.Height * SCDesc.Width * 4);
        for (Uint32 y = 0; y < SCDesc.Height; ++y)
        {
            for (Uint32 x = 0; x < SCDesc.Width; ++x)
            {
                Uint32 ix = x >> 4;
                Uint32 iy = y >> 4;
                Uint32 a1 = (ix >> 1) & 1;
                Uint32 a2 = (iy >> 2) & 5;

                iy = (iy << a1) | (iy >> a1);
                ix = (ix << a2) | (ix >> a2);

                const Uint32 i = (x + SCDesc.Width * y) * 4;
                Pixels[i + 0]  = ((ix | iy) & 1) ? 255 : 0;
                Pixels[i + 1]  = ((ix ^ iy) & 2) ? 255 : 0;
                Pixels[i + 2]  = 0;
                Pixels[i + 3]  = 255;
            }
        }
    }

    const Uint64 QueueMask = (1ull << pGraphicsCtx->GetDesc().ContextId) | (1ull << pTransferCtx->GetDesc().ContextId);

    RefCntAutoPtr<IBuffer> pConstants;
    {
        const float4 ConstData = float4(0.8f, 1.53f, 0.6f, 1.72f);

        BufferDesc BuffDesc;
        BuffDesc.Name                 = "Constants";
        BuffDesc.Size                 = sizeof(ConstData);
        BuffDesc.BindFlags            = BIND_UNIFORM_BUFFER;
        BuffDesc.ImmediateContextMask = QueueMask;

        BufferData BuffData{&ConstData, sizeof(ConstData)};
        pDevice->CreateBuffer(BuffDesc, &BuffData, &pConstants);
        ASSERT_NE(pConstants, nullptr);
    }

    // Draw reference in single queue
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto* pRTV           = pSwapChain->GetCurrentBackBufferRTV();
        auto  pTextureRT     = CreateTexture(BIND_SHADER_RESOURCE | BIND_RENDER_TARGET, 0, "TextureRT", pGraphicsCtx);
        auto  pUploadTexture = CreateTexture(BIND_SHADER_RESOURCE, 0, "Upload Texture", pGraphicsCtx);
        ASSERT_NE(pTextureRT, nullptr);
        ASSERT_NE(pUploadTexture, nullptr);

        const auto DefaultTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        // graphics pass
        {
            sm_pDrawProceduralSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(pConstants);

            ITextureView* pRTVs[] = {pTextureRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
            pGraphicsCtx->SetRenderTargets(1, pRTVs, nullptr, DefaultTransitionMode);

            pGraphicsCtx->SetPipelineState(sm_pDrawProceduralPSO);
            pGraphicsCtx->CommitShaderResources(sm_pDrawProceduralSRB, DefaultTransitionMode);
            pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_VERIFY_ALL});

            pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);
        }

        // copy pass
        {
            TextureSubResData SubRes;
            SubRes.pData  = Pixels.data();
            SubRes.Stride = SCDesc.Width * 4;
            Box Region{0, SCDesc.Width, 0, SCDesc.Height};
            pGraphicsCtx->UpdateTexture(pUploadTexture, 0, 0, Region, SubRes, DefaultTransitionMode, DefaultTransitionMode);
        }

        // blend pass
        {
            sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture1")->Set(pTextureRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture2")->Set(pUploadTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

            pGraphicsCtx->SetRenderTargets(1, &pRTV, nullptr, DefaultTransitionMode);

            pGraphicsCtx->SetPipelineState(sm_pBlendTexPSO);
            pGraphicsCtx->CommitShaderResources(sm_pBlendTexSRB, DefaultTransitionMode);
            pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_VERIFY_ALL});

            pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

            // Transition to CopySrc state to use in TakeSnapshot()
            StateTransitionDesc Barrier{pRTV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
            pGraphicsCtx->TransitionResourceStates(1, &Barrier);
        }

        pGraphicsCtx->WaitForIdle();
        pTestingSwapChain->TakeSnapshot(pRTV->GetTexture());
    }


    // Graphics:  |- draw -| |- blend -|- present -|
    // Transfer:  |- copy -|

    RefCntAutoPtr<IFence> pGraphicsFence;
    RefCntAutoPtr<IFence> pTransferFence;
    {
        FenceDesc FenceCI;
        FenceCI.Type = FENCE_TYPE_GENERAL;
        FenceCI.Name = "Graphics sync";
        pDevice->CreateFence(FenceCI, &pGraphicsFence);
        ASSERT_NE(pGraphicsFence, nullptr);

        FenceCI.Name = "Transfer sync";
        pDevice->CreateFence(FenceCI, &pTransferFence);
        ASSERT_NE(pTransferFence, nullptr);
    }

    const uint2 Granularity = {pTransferCtx->GetDesc().TextureCopyGranularity[0],
                               pTransferCtx->GetDesc().TextureCopyGranularity[1]};

    auto pTextureRT     = CreateTexture(BIND_SHADER_RESOURCE | BIND_RENDER_TARGET, QueueMask, "TextureRT", pGraphicsCtx);
    auto pUploadTexture = CreateTexture(BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS, QueueMask, "Upload Texture", pTransferCtx);
    ASSERT_NE(pTextureRT, nullptr);
    ASSERT_NE(pUploadTexture, nullptr);

    const Uint64 GraphicsFenceValue    = 11;
    const Uint64 TransferFenceValue    = 22;
    const auto   DefaultTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;

    // graphics queue
    {
        sm_pDrawProceduralSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(pConstants);

        // initial -> render_target
        const StateTransitionDesc Barrier1 = {pTextureRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET};
        pGraphicsCtx->TransitionResourceStates(1, &Barrier1);
        pTextureRT->SetState(RESOURCE_STATE_UNKNOWN); // disable implicit state transitions

        ITextureView* pRTVs[] = {pTextureRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
        pGraphicsCtx->SetRenderTargets(1, pRTVs, nullptr, DefaultTransitionMode);

        pGraphicsCtx->SetPipelineState(sm_pDrawProceduralPSO);
        pGraphicsCtx->CommitShaderResources(sm_pDrawProceduralSRB, DefaultTransitionMode);
        pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_NONE});

        pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, DefaultTransitionMode);

        // render_target -> shader_resource
        const StateTransitionDesc Barrier2 = {pTextureRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE};
        pGraphicsCtx->TransitionResourceStates(1, &Barrier2);

        pGraphicsCtx->EnqueueSignal(pGraphicsFence, GraphicsFenceValue);
        pGraphicsCtx->Flush();
    }

    // transfer queue
    {
        const uint2  TexDim    = {pUploadTexture->GetDesc().Width, pUploadTexture->GetDesc().Height};
        const uint2  BlockSize = {AlignUp(TexDim.x / 8, Granularity.x), AlignUp(TexDim.y / 4, Granularity.y)};
        const size_t DataSize  = BlockSize.x * BlockSize.y * 4;

        VERIFY_EXPR(TexDim.x % BlockSize.x == 0);
        VERIFY_EXPR(TexDim.y % BlockSize.y == 0);

        // initial -> copy_dst
        const StateTransitionDesc Barrier1 = {pUploadTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_DEST};
        pTransferCtx->TransitionResourceStates(1, &Barrier1);
        pUploadTexture->SetState(RESOURCE_STATE_UNKNOWN); // disable implicit state transitions

        TextureSubResData SubRes;
        SubRes.Stride = TexDim.x * 4;

        uint2 Offset;
        for (Offset.y = 0; Offset.y < TexDim.y; Offset.y += BlockSize.y)
        {
            for (Offset.x = 0; Offset.x < TexDim.x; Offset.x += BlockSize.x)
            {
                size_t DataOffset = (Offset.x + Offset.y * TexDim.x) * 4;
                VERIFY_EXPR(DataOffset + DataSize <= Pixels.size());

                SubRes.pData = &Pixels[DataOffset];
                Box Region{Offset.x, Offset.x + BlockSize.x, Offset.y, Offset.y + BlockSize.y};
                pTransferCtx->UpdateTexture(pUploadTexture, 0, 0, Region, SubRes, DefaultTransitionMode, DefaultTransitionMode);
            }
        }

        // copy_dst -> common
        const StateTransitionDesc Barrier2 = {pUploadTexture, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON};
        pTransferCtx->TransitionResourceStates(1, &Barrier2);

        pTransferCtx->EnqueueSignal(pTransferFence, TransferFenceValue);
        pTransferCtx->Flush();
    }

    // blend and present
    {
        sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture1")->Set(pTextureRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        sm_pBlendTexSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture2")->Set(pUploadTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pGraphicsCtx->DeviceWaitForFence(pGraphicsFence, GraphicsFenceValue);
        pGraphicsCtx->DeviceWaitForFence(pTransferFence, TransferFenceValue);

        auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();

        const StateTransitionDesc Barriers[] = {
            {pRTV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET, STATE_TRANSITION_FLAG_UPDATE_STATE}, // prev_state -> render_target
            {pUploadTexture, RESOURCE_STATE_COMMON, RESOURCE_STATE_SHADER_RESOURCE}                                         // common -> shader_resource
        };
        pGraphicsCtx->TransitionResourceStates(_countof(Barriers), Barriers);

        pGraphicsCtx->SetRenderTargets(1, &pRTV, nullptr, DefaultTransitionMode);

        pGraphicsCtx->SetPipelineState(sm_pBlendTexPSO);
        pGraphicsCtx->CommitShaderResources(sm_pBlendTexSRB, DefaultTransitionMode);
        pGraphicsCtx->Draw(DrawAttribs{4, DRAW_FLAG_NONE});

        pGraphicsCtx->SetRenderTargets(0, nullptr, nullptr, DefaultTransitionMode);

        pGraphicsCtx->WaitForIdle();
        pSwapChain->Present();
    }

    pGraphicsCtx->FinishFrame();
    pTransferCtx->FinishFrame();

    pGraphicsFence->Wait(GraphicsFenceValue);
    pTransferFence->Wait(TransferFenceValue);
}

} // namespace
