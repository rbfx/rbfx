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

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

const std::string FenceTest_CS{R"(
RWTexture2D<float4/* format = rgba8 */> g_DstTexture;

cbuffer Constants
{
    float4 g_Time;
};

[numthreads(4, 4, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 Dim;
    g_DstTexture.GetDimensions(Dim.x, Dim.y);
    if (DTid.x >= Dim.x || DTid.y >= Dim.y)
        return;

    float2 uv  = float2(DTid.xy) / float2(Dim) * 10.0;
    float4 col = float(0.0).xxxx;

    col.r = sin(uv.x + g_Time.x) * cos(uv.y + g_Time.y);
    col.g = frac(uv.x + g_Time.z) * frac(uv.y + g_Time.w);

    g_DstTexture[DTid.xy] = col;
}
)"};

class FenceTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        ComputePipelineStateCreateInfo PSOCreateInfo;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        RefCntAutoPtr<IShader> pCS;
        {
            ShaderCI.Desc       = {"Fence test - CS", SHADER_TYPE_COMPUTE, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = FenceTest_CS.c_str();
            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);
        }

        PSOCreateInfo.PSODesc.Name = "Fence test - compute PSO";
        PSOCreateInfo.pCS          = pCS;

        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

        pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pCompPSO);
        ASSERT_NE(sm_pCompPSO, nullptr);

        sm_pCompPSO->CreateShaderResourceBinding(&sm_pCompSRB, true);
        ASSERT_NE(sm_pCompSRB, nullptr);

        auto*       pSwapChain = pEnv->GetSwapChain();
        const auto& SCDesc     = pSwapChain->GetDesc();

        sm_dispatchSize = uint2{(SCDesc.Width + 3) / 4, (SCDesc.Height + 3) / 4}; // must be same as numthreads(...)
    }

    static void TearDownTestSuite()
    {
        sm_pCompPSO.Release();
        sm_pCompSRB.Release();

        auto* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static RefCntAutoPtr<IPipelineState>         sm_pCompPSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pCompSRB;
    static uint2                                 sm_dispatchSize;
};
RefCntAutoPtr<IPipelineState>         FenceTest::sm_pCompPSO;
RefCntAutoPtr<IShaderResourceBinding> FenceTest::sm_pCompSRB;
uint2                                 FenceTest::sm_dispatchSize;


TEST_F(FenceTest, GPUWaitForCPU)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.NativeFence)
    {
        GTEST_SKIP() << "NativeFence feature is not supported";
    }

    auto* pContext   = pEnv->GetDeviceContext();
    auto* pSwapChain = pEnv->GetSwapChain();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Constants 1";
    BuffDesc.Size           = sizeof(float4);
    BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    BuffDesc.Usage          = USAGE_DYNAMIC;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    RefCntAutoPtr<IBuffer> pConstants1;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pConstants1);
    ASSERT_NE(pConstants1, nullptr);

    const float4 ConstData = float4(1.2f, 0.25f, 1.1f, 0.14f);

    auto pBackBufferUAV = pTestingSwapChain->GetCurrentBackBufferUAV();

    // Draw reference
    {
        void* pMappedData = nullptr;
        pContext->MapBuffer(pConstants1, MAP_WRITE, MAP_FLAG_DISCARD, pMappedData);
        *static_cast<float4*>(pMappedData) = ConstData;
        pContext->UnmapBuffer(pConstants1, MAP_WRITE);

        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pBackBufferUAV);
        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants1);

        pContext->SetPipelineState(sm_pCompPSO);
        pContext->CommitShaderResources(sm_pCompSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->DispatchCompute(DispatchComputeAttribs{sm_dispatchSize.x, sm_dispatchSize.y, 1});

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pBackBufferUAV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);
        pContext->WaitForIdle();

        pTestingSwapChain->TakeSnapshot(pBackBufferUAV->GetTexture());
    }

    RefCntAutoPtr<IBuffer> pConstants2;
    BuffDesc.Name = "Constants 2";
    pDevice->CreateBuffer(BuffDesc, nullptr, &pConstants2);
    ASSERT_NE(pConstants2, nullptr);

    FenceDesc FenceCI;
    FenceCI.Name = "CPU-GPU sync";
    FenceCI.Type = FENCE_TYPE_GENERAL;

    RefCntAutoPtr<IFence> pFence;
    pDevice->CreateFence(FenceCI, &pFence);
    ASSERT_NE(pFence, nullptr);

    void* pMappedData = nullptr;
    pContext->MapBuffer(pConstants2, MAP_WRITE, MAP_FLAG_DISCARD, pMappedData);
    *static_cast<float4*>(pMappedData) = ConstData;
    pContext->UnmapBuffer(pConstants2, MAP_WRITE);

    sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pBackBufferUAV);
    sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants2);

    pContext->SetPipelineState(sm_pCompPSO);
    pContext->CommitShaderResources(sm_pCompSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->DispatchCompute(DispatchComputeAttribs{sm_dispatchSize.x, sm_dispatchSize.y, 1});

    pContext->DeviceWaitForFence(pFence, 100);
    pContext->Flush();

    // GPU waits for fence signal
    pFence->Signal(100);

    pContext->WaitForIdle();

    // Testing swapchain copy data from GPU side to CPU side and must be used after fence signal.
    // The default implementation of swapchain can be presented before fence signal command.
    pSwapChain->Present();
}


TEST_F(FenceTest, ContextWaitForAnotherContext)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.NativeFence)
    {
        GTEST_SKIP() << "NativeFence feature is not supported";
    }

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
            else if (!pComputeCtx && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_COMPUTE)
                pComputeCtx = Ctx;
            else if (!pGraphicsCtx2 && (Desc.QueueType & QueueTypeMask) == COMMAND_QUEUE_TYPE_GRAPHICS)
                pGraphicsCtx2 = Ctx;
        }

        if (pComputeCtx == nullptr)
            pComputeCtx = pGraphicsCtx2;
    }
    if (!pGraphicsCtx || !pComputeCtx)
    {
        GTEST_SKIP() << "Unable to find two immediate contexts";
    }
    if (pGraphicsCtx->GetDesc().QueueId == pComputeCtx->GetDesc().QueueId)
    {
        GTEST_SKIP() << "At least two different hardware queues are required";
    }
    ASSERT_NE(pGraphicsCtx->GetDesc().ContextId, pComputeCtx->GetDesc().ContextId);

    auto* pSwapChain = pEnv->GetSwapChain();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

    const float4 ConstData = float4(1.2f, 0.25f, 1.1f, 0.14f);

    auto pBackBufferUAV = pTestingSwapChain->GetCurrentBackBufferUAV();

    BufferDesc BuffDesc;
    BuffDesc.Name      = "Constants 1";
    BuffDesc.Size      = sizeof(float4);
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    BuffDesc.Usage     = USAGE_DEFAULT;

    RefCntAutoPtr<IBuffer> pConstants1;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pConstants1);
    ASSERT_NE(pConstants1, nullptr);

    // Draw reference
    {
        pGraphicsCtx->UpdateBuffer(pConstants1, 0, sizeof(float4), &ConstData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pBackBufferUAV);
        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants1);

        pGraphicsCtx->SetPipelineState(sm_pCompPSO);
        pGraphicsCtx->CommitShaderResources(sm_pCompSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pGraphicsCtx->DispatchCompute(DispatchComputeAttribs{sm_dispatchSize.x, sm_dispatchSize.y, 1});

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pBackBufferUAV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pGraphicsCtx->TransitionResourceStates(1, &Barrier);
        pGraphicsCtx->WaitForIdle();

        pTestingSwapChain->TakeSnapshot(pBackBufferUAV->GetTexture());
    }

    RefCntAutoPtr<IBuffer> pConstants2;
    BuffDesc.Name                 = "Constants 2";
    BuffDesc.ImmediateContextMask = (1ull << pGraphicsCtx->GetDesc().ContextId) | (1ull << pComputeCtx->GetDesc().ContextId);
    pDevice->CreateBuffer(BuffDesc, nullptr, &pConstants2);
    ASSERT_NE(pConstants2, nullptr);

    FenceDesc FenceCI;
    FenceCI.Name = "sync between queues";
    FenceCI.Type = FENCE_TYPE_GENERAL;

    RefCntAutoPtr<IFence> pFence;
    pDevice->CreateFence(FenceCI, &pFence);
    ASSERT_NE(pFence, nullptr);

    // first context
    {
        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pBackBufferUAV);
        sm_pCompSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(pConstants1);

        pGraphicsCtx->SetPipelineState(sm_pCompPSO);
        pGraphicsCtx->CommitShaderResources(sm_pCompSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pGraphicsCtx->DispatchCompute(DispatchComputeAttribs{sm_dispatchSize.x, sm_dispatchSize.y, 1});

        pGraphicsCtx->DeviceWaitForFence(pFence, 100);
        pGraphicsCtx->Flush();
    }

    // second context
    {
        pComputeCtx->UpdateBuffer(pConstants1, 0, sizeof(float4), &ConstData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pComputeCtx->EnqueueSignal(pFence, 100);
        pComputeCtx->Flush();
    }

    pGraphicsCtx->WaitForIdle();
    pSwapChain->Present();
}

} // namespace
