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

#include <sstream>
#include <vector>
#include <thread>

#include "GPUTestingEnvironment.hpp"
#include "ThreadSignal.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestQueryCInterface(void* pQuery);
}

namespace
{

// clang-format off
const std::string QueryTest_ProceduralQuadVS{
R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
};

void main(in uint VertId : SV_VertexID,
          out PSInput PSIn)
{
    float HalfTexel = 0.5 / 512.0;
    float size = 0.25;
    float4 Pos[4];

    Pos[0] = float4(-size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[1] = float4(-size-HalfTexel, +size-HalfTexel, 0.0, 1.0);
    Pos[2] = float4(+size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[3] = float4(+size-HalfTexel, +size-HalfTexel, 0.0, 1.0);

    PSIn.Pos = Pos[VertId];
}
)"
};

const std::string QueryTest_PS{
R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
};

float4 main(in PSInput PSIn) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
)"
};
// clang-format on


class QueryTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        TextureDesc TexDesc;
        TexDesc.Name      = "Mips generation test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Width     = sm_TextureSize;
        TexDesc.Height    = sm_TextureSize;
        TexDesc.BindFlags = BIND_RENDER_TARGET;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        RefCntAutoPtr<ITexture> pRenderTarget;
        pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
        ASSERT_NE(pRenderTarget, nullptr) << "TexDesc:\n"
                                          << TexDesc;
        sm_pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        ASSERT_NE(sm_pRTV, nullptr);

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name                 = "Query command test - procedural quad";
        PSODesc.ImmediateContextMask = ~0ull;

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = TexDesc.Format;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc       = {"Query test vertex shader", SHADER_TYPE_VERTEX, true};
            ShaderCI.Source     = QueryTest_ProceduralQuadVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc       = {"Query test pixel shader", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = QueryTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pVS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pPSO);
        ASSERT_NE(sm_pPSO, nullptr);
    }

    static void TearDownTestSuite()
    {
        sm_pPSO.Release();
        sm_pRTV.Release();

        auto* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void DrawQuad(IDeviceContext* pContext)
    {
        ITextureView* pRTVs[] = {sm_pRTV};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
        pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(sm_pPSO);

        DrawAttribs drawAttrs{4, DRAW_FLAG_VERIFY_ALL, 32};
        pContext->Draw(drawAttrs);
    }

    static constexpr Uint32 sm_NumTestQueries = 3;
    static constexpr Uint32 sm_NumFrames      = 5;

    void InitTestQueries(IDeviceContext* pContext, std::vector<RefCntAutoPtr<IQuery>>& Queries, const QueryDesc& queryDesc)
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pDevice    = pEnv->GetDevice();
        auto  deviceInfo = pDevice->GetDeviceInfo();

        if (Queries.empty())
        {
            Queries.resize(sm_NumTestQueries);
            for (auto& pQuery : Queries)
            {
                pDevice->CreateQuery(queryDesc, &pQuery);
                ASSERT_NE(pQuery, nullptr) << "Failed to create pipeline stats query";
            }
        }

        // Nested queries are not supported by OpenGL and Vulkan
        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            pContext->BeginQuery(Queries[i]);
            for (Uint32 j = 0; j < i + 1; ++j)
                DrawQuad(pContext);
            pContext->EndQuery(Queries[i]);
            Queries[i]->GetData(nullptr, 0);

            if (deviceInfo.IsMetalDevice())
            {
                // Metal may not support queries for draw calls.
                // Flush() is one of the ways to begin new render pass.
                pContext->Flush();
            }
        }

        if (queryDesc.Type == QUERY_TYPE_DURATION)
        {
            // FinishFrame() must be called to finish the disjoint query
            pContext->Flush();
            pContext->FinishFrame();
        }
        pContext->WaitForIdle();
        if (deviceInfo.IsGLDevice())
        {
            // glFinish() is not a guarantee that queries will become available.
            // Even using glFenceSync + glClientWaitSync does not help.
            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                WaitForQuery(Queries[i]);
            }
        }
    }

    static void WaitForQuery(IQuery* pQuery)
    {
        while (!pQuery->GetData(nullptr, 0))
            std::this_thread::sleep_for(std::chrono::microseconds{1});
    }

    static constexpr Uint32 sm_TextureSize = 512;

    static RefCntAutoPtr<ITextureView>   sm_pRTV;
    static RefCntAutoPtr<IPipelineState> sm_pPSO;
};

RefCntAutoPtr<ITextureView>   QueryTest::sm_pRTV;
RefCntAutoPtr<IPipelineState> QueryTest::sm_pPSO;

TEST_F(QueryTest, PipelineStats)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!DeviceInfo.Features.PipelineStatisticsQueries)
    {
        GTEST_SKIP() << "Pipeline statistics queries are not supported by this device";
    }

    const auto IsGL = DeviceInfo.IsGLDevice();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = GPUTestingEnvironment::GetInstance();
    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pContext = pEnv->GetDeviceContext(q);

        if ((pContext->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        QueryDesc queryDesc;
        queryDesc.Name = "Pipeline stats query";
        queryDesc.Type = QUERY_TYPE_PIPELINE_STATISTICS;

        std::vector<RefCntAutoPtr<IQuery>> Queries;
        for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
        {
            InitTestQueries(pContext, Queries, queryDesc);

            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                Uint32 DrawCounter = 1 + i;

                QueryDataPipelineStatistics QueryData;

                auto QueryReady = Queries[i]->GetData(nullptr, 0);
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                if (!IsGL)
                {
                    EXPECT_GE(QueryData.InputVertices, 4 * DrawCounter);
                    EXPECT_GE(QueryData.InputPrimitives, 2 * DrawCounter);
                    EXPECT_GE(QueryData.ClippingPrimitives, 2 * DrawCounter);
                    EXPECT_GE(QueryData.VSInvocations, 4 * DrawCounter);
                    auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
                    EXPECT_GE(QueryData.PSInvocations, NumPixels * DrawCounter);
                }
                EXPECT_GE(QueryData.ClippingInvocations, 2 * DrawCounter);
            }
        }
    }
}

TEST_F(QueryTest, Occlusion)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!DeviceInfo.Features.OcclusionQueries)
    {
        GTEST_SKIP() << "Occlusion queries are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = GPUTestingEnvironment::GetInstance();
    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pContext = pEnv->GetDeviceContext(q);

        if ((pContext->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        QueryDesc queryDesc;
        queryDesc.Name = "Occlusion query";
        queryDesc.Type = QUERY_TYPE_OCCLUSION;

        std::vector<RefCntAutoPtr<IQuery>> Queries;
        for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
        {
            InitTestQueries(pContext, Queries, queryDesc);

            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                Uint32 DrawCounter = 1 + i;

                QueryDataOcclusion QueryData;

                auto QueryReady = Queries[i]->GetData(nullptr, 0);
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
                EXPECT_GE(QueryData.NumSamples, NumPixels * DrawCounter);
            }
        }
    }
}

TEST_F(QueryTest, BinaryOcclusion)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!DeviceInfo.Features.BinaryOcclusionQueries)
    {
        GTEST_SKIP() << "Binary occlusion queries are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = GPUTestingEnvironment::GetInstance();
    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pContext = pEnv->GetDeviceContext(q);

        if ((pContext->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        QueryDesc queryDesc;
        queryDesc.Name = "Binary occlusion query";
        queryDesc.Type = QUERY_TYPE_BINARY_OCCLUSION;

        std::vector<RefCntAutoPtr<IQuery>> Queries;
        for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
        {
            InitTestQueries(pContext, Queries, queryDesc);

            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                QueryDataBinaryOcclusion QueryData;

                auto QueryReady = Queries[i]->GetData(nullptr, 0);
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                Queries[i]->GetData(&QueryData, sizeof(QueryData));
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                EXPECT_TRUE(QueryData.AnySamplePassed);
            }
        }
    }
}

TEST_F(QueryTest, Timestamp)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (!DeviceInfo.Features.TimestampQueries)
    {
        GTEST_SKIP() << "Timestamp queries are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pContext = pEnv->GetDeviceContext(q);

        if ((pContext->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        QueryDesc queryDesc;
        queryDesc.Name = "Timestamp query";
        queryDesc.Type = QUERY_TYPE_TIMESTAMP;

        RefCntAutoPtr<IQuery> pQueryStart;
        pDevice->CreateQuery(queryDesc, &pQueryStart);
        ASSERT_NE(pQueryStart, nullptr) << "Failed to create timestamp query";

        RefCntAutoPtr<IQuery> pQueryEnd;
        pDevice->CreateQuery(queryDesc, &pQueryEnd);
        ASSERT_NE(pQueryEnd, nullptr) << "Failed to create timestamp query";

        for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
        {
            pContext->EndQuery(pQueryStart);
            pQueryStart->GetData(nullptr, 0);
            DrawQuad(pContext);
            pContext->EndQuery(pQueryEnd);
            pQueryEnd->GetData(nullptr, 0);

            pContext->Flush();
            pContext->FinishFrame();
            pContext->WaitForIdle();
            if (pDevice->GetDeviceInfo().IsGLDevice())
            {
                // glFinish() is not a guarantee that queries will become available
                // Even using glFenceSync + glClientWaitSync does not help.
                WaitForQuery(pQueryStart);
                WaitForQuery(pQueryEnd);
            }

            QueryDataTimestamp QueryStartData, QueryEndData;

            auto QueryReady = pQueryStart->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            QueryReady = pQueryStart->GetData(&QueryStartData, sizeof(QueryStartData));
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";

            QueryReady = pQueryEnd->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            QueryReady = pQueryEnd->GetData(&QueryEndData, sizeof(QueryEndData), false);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            EXPECT_EQ(TestQueryCInterface(pQueryEnd.RawPtr()), 0);
            EXPECT_TRUE(QueryStartData.Frequency == 0 || QueryEndData.Frequency == 0 || QueryEndData.Counter > QueryStartData.Counter);
        }
    }
}


TEST_F(QueryTest, Duration)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!DeviceInfo.Features.DurationQueries)
    {
        GTEST_SKIP() << "Duration queries are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = GPUTestingEnvironment::GetInstance();
    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pContext = pEnv->GetDeviceContext(q);

        if ((pContext->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        QueryDesc queryDesc;
        queryDesc.Name = "Duration query";
        queryDesc.Type = QUERY_TYPE_DURATION;

        std::vector<RefCntAutoPtr<IQuery>> Queries;
        for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
        {
            InitTestQueries(pContext, Queries, queryDesc);

            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                QueryDataDuration QueryData;

                auto QueryReady = Queries[i]->GetData(nullptr, 0);
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                Queries[i]->GetData(&QueryData, sizeof(QueryData));
                ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
                EXPECT_TRUE(QueryData.Frequency == 0 || QueryData.Duration > 0);
            }
        }
    }
}

TEST_F(QueryTest, DeferredContexts)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (!DeviceInfo.Features.DurationQueries && !DeviceInfo.Features.TimestampQueries)
    {
        GTEST_SKIP() << "Time queries are not supported by this device";
    }

    Uint32 NumDeferredCtx = static_cast<Uint32>(pEnv->GetNumDeferredContexts());
    if (NumDeferredCtx == 0)
    {
        GTEST_SKIP() << "Deferred contexts are not supported by this device";
    }

    if (DeviceInfo.IsMetalDevice())
    {
        GTEST_SKIP() << "Queries are not supported in deferred contexts on Metal";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    std::vector<RefCntAutoPtr<IQuery>> pDurations;
    if (DeviceInfo.Features.DurationQueries)
    {
        QueryDesc queryDesc;
        queryDesc.Name = "Duration query";
        queryDesc.Type = QUERY_TYPE_DURATION;
        pDurations.resize(NumDeferredCtx);
        for (Uint32 i = 0; i < NumDeferredCtx; ++i)
        {
            pDevice->CreateQuery(queryDesc, &pDurations[i]);
            ASSERT_NE(pDurations[i], nullptr);
        }
    }

    std::vector<RefCntAutoPtr<IQuery>> pStartTimestamps, pEndTimestamps;
    if (DeviceInfo.Features.TimestampQueries)
    {
        QueryDesc queryDesc;
        queryDesc.Type = QUERY_TYPE_TIMESTAMP;
        pStartTimestamps.resize(NumDeferredCtx);
        pEndTimestamps.resize(NumDeferredCtx);
        for (Uint32 i = 0; i < NumDeferredCtx; ++i)
        {
            queryDesc.Name = "Start timestamp query";
            pDevice->CreateQuery(queryDesc, &pStartTimestamps[i]);
            ASSERT_NE(pStartTimestamps[i], nullptr);

            queryDesc.Name = "End timestamp query";
            pDevice->CreateQuery(queryDesc, &pEndTimestamps[i]);
            ASSERT_NE(pEndTimestamps[i], nullptr);
        }
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    for (Uint32 q = 0; q < pEnv->GetNumImmediateContexts(); ++q)
    {
        auto* pImmediateCtx = pEnv->GetDeviceContext(q);

        if ((pImmediateCtx->GetDesc().QueueType & COMMAND_QUEUE_TYPE_GRAPHICS) != COMMAND_QUEUE_TYPE_GRAPHICS)
            continue;

        const float ClearColor[] = {0.25f, 0.5f, 0.75f, 1.0f};

        ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pImmediateCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pImmediateCtx->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        std::vector<std::thread>                 WorkerThreads(NumDeferredCtx);
        std::vector<RefCntAutoPtr<ICommandList>> CmdLists(NumDeferredCtx);
        std::vector<ICommandList*>               CmdListPtrs(NumDeferredCtx);

        std::atomic<Uint32> NumCmdListsReady{0};
        Threading::Signal   FinishFrameSignal;
        Threading::Signal   ExecuteCommandListsSignal;
        for (Uint32 i = 0; i < NumDeferredCtx; ++i)
        {
            WorkerThreads[i] = std::thread(
                [&](Uint32 thread_id) //
                {
                    auto* pCtx = pEnv->GetDeferredContext(thread_id);

                    pCtx->Begin(pImmediateCtx->GetDesc().ContextId);
                    pCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

                    pCtx->SetPipelineState(sm_pPSO);

                    if (DeviceInfo.Features.DurationQueries)
                        pCtx->BeginQuery(pDurations[thread_id]);

                    if (DeviceInfo.Features.TimestampQueries)
                        pCtx->EndQuery(pStartTimestamps[thread_id]);

                    DrawAttribs drawAttrs{4, DRAW_FLAG_VERIFY_ALL, 32};
                    pCtx->Draw(drawAttrs);

                    if (DeviceInfo.Features.DurationQueries)
                        pCtx->EndQuery(pDurations[thread_id]);

                    if (DeviceInfo.Features.TimestampQueries)
                        pCtx->EndQuery(pEndTimestamps[thread_id]);

                    pCtx->FinishCommandList(&CmdLists[thread_id]);
                    CmdListPtrs[thread_id] = CmdLists[thread_id];

                    // Atomically increment the number of completed threads
                    const auto NumReadyLists = NumCmdListsReady.fetch_add(1) + 1;
                    if (NumReadyLists == NumDeferredCtx)
                        ExecuteCommandListsSignal.Trigger();

                    FinishFrameSignal.Wait(true, NumDeferredCtx);

                    // IMPORTANT: In Metal backend FinishFrame must be called from the same
                    //            thread that issued rendering commands.
                    pCtx->FinishFrame();
                },
                i);
        }

        // Wait for the worker threads
        ExecuteCommandListsSignal.Wait(true, 1);

        pImmediateCtx->ExecuteCommandLists(NumDeferredCtx, CmdListPtrs.data());

        FinishFrameSignal.Trigger(true);
        for (auto& t : WorkerThreads)
            t.join();

        pImmediateCtx->WaitForIdle();

        for (IQuery* pQuery : pDurations)
        {
            QueryDataDuration QueryData;

            auto QueryReady = pQuery->GetData(nullptr, 0);
            EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";
            pQuery->GetData(&QueryData, sizeof(QueryData));
            EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";
            EXPECT_TRUE(QueryData.Frequency == 0 || QueryData.Duration > 0);
        }

        if (DeviceInfo.Features.TimestampQueries)
        {
            for (Uint32 i = 0; i < NumDeferredCtx; ++i)
            {
                IQuery* pQueryStart = pStartTimestamps[i];
                IQuery* pQueryEnd   = pEndTimestamps[i];

                QueryDataTimestamp QueryStartData, QueryEndData;

                auto QueryReady = pQueryStart->GetData(nullptr, 0);
                EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";
                QueryReady = pQueryStart->GetData(&QueryStartData, sizeof(QueryStartData));
                EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";

                QueryReady = pQueryEnd->GetData(nullptr, 0);
                EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";
                QueryReady = pQueryEnd->GetData(&QueryEndData, sizeof(QueryEndData), false);
                EXPECT_TRUE(QueryReady) << "Query data must be available after idling the context";
                EXPECT_TRUE(QueryStartData.Frequency == 0 || QueryEndData.Frequency == 0 || QueryEndData.Counter > QueryStartData.Counter);
            }
        }
    }
}

} // namespace
