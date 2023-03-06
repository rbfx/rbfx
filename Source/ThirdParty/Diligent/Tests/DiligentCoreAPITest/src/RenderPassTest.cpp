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

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/DrawCommandTestGLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void RenderPassMSResolveReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if D3D12_SUPPORTED
void RenderPassMSResolveReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void RenderPassMSResolveReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if VULKAN_SUPPORTED
void RenderPassMSResolveReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if METAL_SUPPORTED
void RenderPassMSResolveReferenceMtl(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceMtl(ISwapChain* pSwapChain, const float* pClearColor, bool UseFramebufferFetch);
#endif

void RenderDrawCommandReference(ISwapChain* pSwapChain, const float* pClearColor);

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace
{


class RenderPassTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        {
            ShaderCI.Desc       = {"Render pass test vertex shader", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_pVS);
            ASSERT_NE(sm_pVS, nullptr);
        }

        {
            ShaderCI.Desc       = {"Render pass test pixel shader", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = HLSL::DrawTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_pPS);
            ASSERT_NE(sm_pPS, nullptr);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pVS.Release();
        sm_pPS.Release();

        auto* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void CreateDrawTrisPSO(IRenderPass*                   pRenderPass,
                                  Uint8                          SampleCount,
                                  RefCntAutoPtr<IPipelineState>& pPSO)
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Render pass test - draw triangles";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.pRenderPass                  = pRenderPass;
        GraphicsPipeline.SubpassIndex                 = 0;
        GraphicsPipeline.SmplDesc.Count               = SampleCount;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        PSOCreateInfo.pVS = sm_pVS;
        PSOCreateInfo.pPS = sm_pPS;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        ASSERT_NE(pPSO, nullptr);
    }

    static void DrawTris(IRenderPass*    pRenderPass,
                         IFramebuffer*   pFramebuffer,
                         IPipelineState* pPSO,
                         const float     ClearColor[])
    {
        auto* pEnv     = GPUTestingEnvironment::GetInstance();
        auto* pContext = pEnv->GetDeviceContext();

        pContext->SetPipelineState(pPSO);

        BeginRenderPassAttribs RPBeginInfo;
        RPBeginInfo.pRenderPass  = pRenderPass;
        RPBeginInfo.pFramebuffer = pFramebuffer;

        OptimizedClearValue ClearValues[1];
        ClearValues[0].Color[0] = ClearColor[0];
        ClearValues[0].Color[1] = ClearColor[1];
        ClearValues[0].Color[2] = ClearColor[2];
        ClearValues[0].Color[3] = ClearColor[3];

        RPBeginInfo.pClearValues        = ClearValues;
        RPBeginInfo.ClearValueCount     = _countof(ClearValues);
        RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        pContext->BeginRenderPass(RPBeginInfo);

        DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);

        pContext->EndRenderPass();
    }

    static void TestMSResolve(bool UseMemoryless);
    static void TestInputAttachment(bool UseSignature, bool UseMemoryless);
    static void TestInputAttachmentGeneralLayout(bool UseSignature);

    static void Present()
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        pSwapChain->Present();

        pContext->Flush();
        pContext->InvalidateState();
    }

    static RefCntAutoPtr<IShader> sm_pVS;
    static RefCntAutoPtr<IShader> sm_pPS;
};

RefCntAutoPtr<IShader> RenderPassTest::sm_pVS;
RefCntAutoPtr<IShader> RenderPassTest::sm_pPS;

TEST_F(RenderPassTest, CreateRenderPassAndFramebuffer)
{
    auto*      pDevice    = GPUTestingEnvironment::GetInstance()->GetDevice();
    auto*      pContext   = GPUTestingEnvironment::GetInstance()->GetDeviceContext();
    const auto DeviceType = pDevice->GetDeviceInfo().Type;

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    RenderPassAttachmentDesc Attachments[6];
    Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[0].SampleCount  = 4;
    Attachments[0].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_LOAD;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[1].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[1].SampleCount  = 4;
    Attachments[1].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[2].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[2].SampleCount  = 1;
    Attachments[2].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[2].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[2].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[2].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[3].Format         = TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    Attachments[3].SampleCount    = 4;
    Attachments[3].InitialState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[3].FinalState     = RESOURCE_STATE_DEPTH_WRITE;
    Attachments[3].LoadOp         = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[3].StoreOp        = ATTACHMENT_STORE_OP_DISCARD;
    Attachments[3].StencilLoadOp  = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[3].StencilStoreOp = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[4].Format       = TEX_FORMAT_RGBA32_FLOAT;
    Attachments[4].SampleCount  = 1;
    Attachments[4].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[4].FinalState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[4].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[4].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[5].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[5].SampleCount  = 1;
    Attachments[5].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[5].FinalState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[5].LoadOp       = ATTACHMENT_LOAD_OP_LOAD;
    Attachments[5].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[2];

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET},
        {1, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference RslvAttachmentRefs0[] =
    {
        {ATTACHMENT_UNUSED, RESOURCE_STATE_RESOLVE_DEST},
        {2, RESOURCE_STATE_RESOLVE_DEST}
    };
    // clang-format on
    AttachmentReference DSAttachmentRef0{3, RESOURCE_STATE_DEPTH_WRITE};
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;
    Subpasses[0].pResolveAttachments         = RslvAttachmentRefs0;
    Subpasses[0].pDepthStencilAttachment     = &DSAttachmentRef0;

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs1[] =
    {
        {4, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference InptAttachmentRefs1[] =
    {
        {2, RESOURCE_STATE_INPUT_ATTACHMENT},
        {5, RESOURCE_STATE_INPUT_ATTACHMENT}
    };
    constexpr Uint32 PrsvAttachmentRefs1[] =
    {
        0
    };
    // clang-format on
    Subpasses[1].InputAttachmentCount        = _countof(InptAttachmentRefs1);
    Subpasses[1].pInputAttachments           = InptAttachmentRefs1;
    Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
    Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
    Subpasses[1].PreserveAttachmentCount     = _countof(PrsvAttachmentRefs1);
    Subpasses[1].pPreserveAttachments        = PrsvAttachmentRefs1;

    SubpassDependencyDesc Dependencies[2] = {};
    Dependencies[0].SrcSubpass            = 0;
    Dependencies[0].DstSubpass            = 1;
    Dependencies[0].SrcStageMask          = PIPELINE_STAGE_FLAG_VERTEX_SHADER;
    Dependencies[0].DstStageMask          = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[0].SrcAccessMask         = ACCESS_FLAG_SHADER_WRITE;
    Dependencies[0].DstAccessMask         = ACCESS_FLAG_SHADER_READ;

    Dependencies[1].SrcSubpass    = 0;
    Dependencies[1].DstSubpass    = 1;
    Dependencies[1].SrcStageMask  = PIPELINE_STAGE_FLAG_VERTEX_INPUT;
    Dependencies[1].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[1].SrcAccessMask = ACCESS_FLAG_INDEX_READ;
    Dependencies[1].DstAccessMask = ACCESS_FLAG_SHADER_READ;


    RenderPassDesc RPDesc;
    RPDesc.Name            = "Test render pass";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;
    RPDesc.DependencyCount = _countof(Dependencies);
    RPDesc.pDependencies   = Dependencies;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    const auto& RPDesc2 = pRenderPass->GetDesc();
    EXPECT_EQ(RPDesc.AttachmentCount, RPDesc2.AttachmentCount);
    for (Uint32 i = 0; i < std::min(RPDesc.AttachmentCount, RPDesc2.AttachmentCount); ++i)
        EXPECT_EQ(RPDesc.pAttachments[i], RPDesc2.pAttachments[i]);

    EXPECT_EQ(RPDesc.SubpassCount, RPDesc2.SubpassCount);
    if (DeviceType != RENDER_DEVICE_TYPE_VULKAN)
    {
        for (Uint32 i = 0; i < std::min(RPDesc.SubpassCount, RPDesc2.SubpassCount); ++i)
            EXPECT_EQ(RPDesc.pSubpasses[i], RPDesc2.pSubpasses[i]);
    }
    else
    {
        auto CompareSubpassDescVk = [](const SubpassDesc& SP1, const SubpassDesc& SP2) //
        {
            if (SP1.InputAttachmentCount != SP2.InputAttachmentCount ||
                SP1.RenderTargetAttachmentCount != SP2.RenderTargetAttachmentCount ||
                SP1.PreserveAttachmentCount != SP2.PreserveAttachmentCount)
                return false;

            for (Uint32 i = 0; i < SP1.InputAttachmentCount; ++i)
            {
                if (SP1.pInputAttachments[i] != SP2.pInputAttachments[i])
                    return false;
            }

            for (Uint32 i = 0; i < SP1.RenderTargetAttachmentCount; ++i)
            {
                if (SP1.pRenderTargetAttachments[i] != SP2.pRenderTargetAttachments[i])
                    return false;
            }

            if ((SP1.pResolveAttachments == nullptr && SP2.pResolveAttachments != nullptr) ||
                (SP1.pResolveAttachments != nullptr && SP2.pResolveAttachments == nullptr))
                return false;

            if (SP1.pResolveAttachments != nullptr && SP2.pResolveAttachments != nullptr)
            {
                for (Uint32 i = 0; i < SP1.RenderTargetAttachmentCount; ++i)
                {
                    if (SP1.pResolveAttachments[i].AttachmentIndex != SP2.pResolveAttachments[i].AttachmentIndex)
                        return false;

                    if (!((SP1.pResolveAttachments[i].State == SP2.pResolveAttachments[i].State) ||
                          (SP1.pResolveAttachments[i].State == RESOURCE_STATE_RESOLVE_DEST && SP2.pResolveAttachments[i].State == RESOURCE_STATE_RENDER_TARGET)))
                        return false;
                }
            }

            if ((SP1.pDepthStencilAttachment == nullptr && SP2.pDepthStencilAttachment != nullptr) ||
                (SP1.pDepthStencilAttachment != nullptr && SP2.pDepthStencilAttachment == nullptr))
                return false;

            if (SP1.pDepthStencilAttachment != nullptr && SP2.pDepthStencilAttachment != nullptr)
            {
                if (*SP1.pDepthStencilAttachment != *SP2.pDepthStencilAttachment)
                    return false;
            }

            if ((SP1.pPreserveAttachments == nullptr && SP2.pPreserveAttachments != nullptr) ||
                (SP1.pPreserveAttachments != nullptr && SP2.pPreserveAttachments == nullptr))
                return false;

            if (SP1.pPreserveAttachments != nullptr && SP2.pPreserveAttachments != nullptr)
            {
                for (Uint32 i = 0; i < SP1.PreserveAttachmentCount; ++i)
                {
                    if (SP1.pPreserveAttachments[i] != SP2.pPreserveAttachments[i])
                        return false;
                }
            }

            return true;
        };
        // Resolve attachment states may be corrected in Vulkan, so we can't use comparison operator
        for (Uint32 i = 0; i < std::min(RPDesc.SubpassCount, RPDesc2.SubpassCount); ++i)
        {
            EXPECT_TRUE(CompareSubpassDescVk(RPDesc.pSubpasses[i], RPDesc2.pSubpasses[i]));
        }
    }

    EXPECT_EQ(RPDesc.DependencyCount, RPDesc2.DependencyCount);
    for (Uint32 i = 0; i < std::min(RPDesc.DependencyCount, RPDesc2.DependencyCount); ++i)
        EXPECT_EQ(RPDesc.pDependencies[i], RPDesc2.pDependencies[i]);

    RefCntAutoPtr<ITexture> pTextures[_countof(Attachments)];
    ITextureView*           pTexViews[_countof(Attachments)] = {};
    for (Uint32 i = 0; i < _countof(pTextures); ++i)
    {
        TextureDesc TexDesc;
        std::string Name = "Test framebuffer attachment ";
        Name += std::to_string(i);
        TexDesc.Name        = Name.c_str();
        TexDesc.Type        = RESOURCE_DIM_TEX_2D;
        TexDesc.Format      = Attachments[i].Format;
        TexDesc.Width       = 1024;
        TexDesc.Height      = 1024;
        TexDesc.SampleCount = Attachments[i].SampleCount;

        const auto FmtAttribs = pDevice->GetTextureFormatInfo(TexDesc.Format);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            TexDesc.BindFlags = BIND_DEPTH_STENCIL;
        else
            TexDesc.BindFlags = BIND_RENDER_TARGET;

        if (i == 2 || i == 5)
            TexDesc.BindFlags |= BIND_INPUT_ATTACHMENT;

        const auto InitialState = Attachments[i].InitialState;
        if (InitialState == RESOURCE_STATE_SHADER_RESOURCE)
            TexDesc.BindFlags |= BIND_SHADER_RESOURCE;

        pDevice->CreateTexture(TexDesc, nullptr, &pTextures[i]);

        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            pTexViews[i] = pTextures[i]->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        else
            pTexViews[i] = pTextures[i]->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    }

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pTexViews;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    const auto& FBDesc2 = pFramebuffer->GetDesc();
    EXPECT_EQ(FBDesc2.AttachmentCount, FBDesc.AttachmentCount);
    for (Uint32 i = 0; i < std::min(FBDesc.AttachmentCount, FBDesc2.AttachmentCount); ++i)
        EXPECT_EQ(FBDesc2.ppAttachments[i], FBDesc.ppAttachments[i]);

    BeginRenderPassAttribs RPBeginInfo;
    RPBeginInfo.pRenderPass  = pRenderPass;
    RPBeginInfo.pFramebuffer = pFramebuffer;
    OptimizedClearValue ClearValues[5];
    RPBeginInfo.pClearValues        = ClearValues;
    RPBeginInfo.ClearValueCount     = _countof(ClearValues);
    RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(RPBeginInfo);

    if (DeviceType != RENDER_DEVICE_TYPE_D3D12 &&
        DeviceType != RENDER_DEVICE_TYPE_METAL)
    {
        // ClearDepthStencil is not allowed inside a render pass in Direct3D12 and Metal
        pContext->ClearDepthStencil(pTexViews[3], CLEAR_DEPTH_FLAG, 1.0, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    pContext->NextSubpass();

    if (DeviceType != RENDER_DEVICE_TYPE_D3D12 &&
        DeviceType != RENDER_DEVICE_TYPE_METAL)
    {
        // ClearRenderTarget is not allowed inside a render pass in Direct3D12 and Metal
        constexpr float ClearColor[] = {0, 0, 0, 0};
        pContext->ClearRenderTarget(pTexViews[4], ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    pContext->EndRenderPass();
}

TEST_F(RenderPassTest, Draw)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    constexpr float ClearColor[] = {0.2f, 0.375f, 0.5f, 0.75f};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    const auto&              SCDesc = pSwapChain->GetDesc();
    RenderPassAttachmentDesc Attachments[1];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1];

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass draw test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateDrawTrisPSO(pRenderPass, 1, pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    ITextureView* pRTAttachments[] = {pSwapChain->GetCurrentBackBufferRTV()};

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass draw test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    DrawTris(pRenderPass, pFramebuffer, pPSO, ClearColor);

    Present();
}

void RenderPassTest::TestMSResolve(bool UseMemoryless)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    constexpr float ClearColor[] = {0.25f, 0.5f, 0.375f, 0.5f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderPassMSResolveReferenceD3D11(pSwapChain, ClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderPassMSResolveReferenceD3D12(pSwapChain, ClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderPassMSResolveReferenceGL(pSwapChain, ClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderPassMSResolveReferenceVk(pSwapChain, ClearColor);
                break;
#endif

#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                RenderPassMSResolveReferenceMtl(pSwapChain, ClearColor);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    const auto& SCDesc = pSwapChain->GetDesc();

    RenderPassAttachmentDesc Attachments[2];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].SampleCount  = 4;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[1].Format       = SCDesc.ColorBufferFormat;
    Attachments[1].SampleCount  = 1;
    Attachments[1].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    RefCntAutoPtr<ITexture> pMSTex;
    {
        TextureDesc TexDesc;
        TexDesc.Type        = RESOURCE_DIM_TEX_2D;
        TexDesc.Format      = SCDesc.ColorBufferFormat;
        TexDesc.Width       = SCDesc.Width;
        TexDesc.Height      = SCDesc.Height;
        TexDesc.BindFlags   = BIND_RENDER_TARGET;
        TexDesc.MipLevels   = 1;
        TexDesc.SampleCount = Attachments[0].SampleCount;
        TexDesc.Usage       = USAGE_DEFAULT;
        TexDesc.MiscFlags   = UseMemoryless ? MISC_TEXTURE_FLAG_MEMORYLESS : MISC_TEXTURE_FLAG_NONE;

        pDevice->CreateTexture(TexDesc, nullptr, &pMSTex);
        ASSERT_NE(pMSTex, nullptr);
    }

    SubpassDesc Subpasses[1];

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference RslvAttachmentRefs0[] =
    {
        {1, RESOURCE_STATE_RESOLVE_DEST}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;
    Subpasses[0].pResolveAttachments         = RslvAttachmentRefs0;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass MS resolve test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateDrawTrisPSO(pRenderPass, 4, pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    ITextureView* pRTAttachments[] = //
        {
            pMSTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
            pSwapChain->GetCurrentBackBufferRTV() //
        };

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass resolve test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    DrawTris(pRenderPass, pFramebuffer, pPSO, ClearColor);

    Present();
}

TEST_F(RenderPassTest, MSResolve)
{
    TestMSResolve(false);
}

TEST_F(RenderPassTest, MemorylessMSResolve)
{
    const auto  RequiredBindFlags = BIND_RENDER_TARGET;
    const auto& MemoryInfo        = GPUTestingEnvironment::GetInstance()->GetDevice()->GetAdapterInfo().Memory;

    if ((MemoryInfo.MemorylessTextureBindFlags & RequiredBindFlags) != RequiredBindFlags)
    {
        GTEST_SKIP() << "Memoryless attachment is not supported by device";
    }
    TestMSResolve(true);
}

void RenderPassTest::TestInputAttachment(bool UseSignature, bool UseMemoryless)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    constexpr float ClearColor[] = {0.5f, 0.125f, 0.25f, 0.25f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderPassInputAttachmentReferenceD3D11(pSwapChain, ClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderPassInputAttachmentReferenceD3D12(pSwapChain, ClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderPassInputAttachmentReferenceGL(pSwapChain, ClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderPassInputAttachmentReferenceVk(pSwapChain, ClearColor);
                break;
#endif

#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                RenderPassInputAttachmentReferenceMtl(
                    pSwapChain, ClearColor, pDevice->GetDeviceInfo().Features.SubpassFramebufferFetch);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    const auto& SCDesc = pSwapChain->GetDesc();

    RenderPassAttachmentDesc Attachments[2];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].SampleCount  = 1;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_INPUT_ATTACHMENT;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[1].Format       = SCDesc.ColorBufferFormat;
    Attachments[1].SampleCount  = 1;
    Attachments[1].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    RefCntAutoPtr<ITexture> pTex;
    {
        TextureDesc TexDesc;
        TexDesc.Name      = "Input attachment test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = SCDesc.ColorBufferFormat;
        TexDesc.Width     = SCDesc.Width;
        TexDesc.Height    = SCDesc.Height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        TexDesc.MiscFlags = UseMemoryless ? MISC_TEXTURE_FLAG_MEMORYLESS : MISC_TEXTURE_FLAG_NONE;

        pDevice->CreateTexture(TexDesc, nullptr, &pTex);
        ASSERT_NE(pTex, nullptr);
    }

    SubpassDesc Subpasses[2];

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference RTAttachmentRefs1[] =
    {
        {1, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference InputAttachmentRefs1[] =
    {
        {0, RESOURCE_STATE_INPUT_ATTACHMENT}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;

    Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
    Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
    Subpasses[1].InputAttachmentCount        = _countof(InputAttachmentRefs1);
    Subpasses[1].pInputAttachments           = InputAttachmentRefs1;

    SubpassDependencyDesc Dependencies[1];
    Dependencies[0].SrcSubpass    = 0;
    Dependencies[0].DstSubpass    = 1;
    Dependencies[0].SrcStageMask  = PIPELINE_STAGE_FLAG_RENDER_TARGET;
    Dependencies[0].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[0].SrcAccessMask = ACCESS_FLAG_RENDER_TARGET_WRITE;
    Dependencies[0].DstAccessMask = ACCESS_FLAG_SHADER_READ;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass input attachment test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;
    RPDesc.DependencyCount = _countof(Dependencies);
    RPDesc.pDependencies   = Dependencies;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateDrawTrisPSO(pRenderPass, 1, pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    RefCntAutoPtr<IPipelineState>         pInputAttachmentPSO;
    RefCntAutoPtr<IShaderResourceBinding> pInputAttachmentSRB;
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Render pass test - input attachment";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.pRenderPass                  = pRenderPass;
        GraphicsPipeline.SubpassIndex                 = 1;
        GraphicsPipeline.SmplDesc.Count               = 1;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        auto UseGLSL =
            pEnv->GetDevice()->GetDeviceInfo().IsVulkanDevice() ||
            pEnv->GetDevice()->GetDeviceInfo().IsMetalDevice();

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = UseGLSL ?
            SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM :
            SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);


        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc       = {"Input attachment test VS", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = UseGLSL ?
                GLSL::DrawTest_ProceduralTriangleVS.c_str() :
                HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc       = {"Input attachment test PS", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = UseGLSL ?
                GLSL::InputAttachmentTest_FS.c_str() :
                HLSL::InputAttachmentTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        RefCntAutoPtr<IPipelineResourceSignature> pSignature;
        IPipelineResourceSignature*               ppSignatures[1]{};
        if (UseSignature)
        {
            PipelineResourceDesc Resources[] = //
                {
                    {SHADER_TYPE_PIXEL, "g_SubpassInput", 1, SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                       = "Render pass test - signature";
            PRSDesc.UseCombinedTextureSamplers = true;
            PRSDesc.Resources                  = Resources;
            PRSDesc.NumResources               = _countof(Resources);

            pDevice->CreatePipelineResourceSignature(PRSDesc, &pSignature);
            ASSERT_NE(pSignature, nullptr);
            ppSignatures[0]                       = pSignature;
            PSOCreateInfo.ppResourceSignatures    = ppSignatures;
            PSOCreateInfo.ResourceSignaturesCount = 1;
        }

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pInputAttachmentPSO);
        ASSERT_NE(pInputAttachmentPSO, nullptr);
        if (pSignature)
        {
            if (auto* pSubpassInput = pSignature->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SubpassInput"))
                pSubpassInput->Set(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            pSignature->CreateShaderResourceBinding(&pInputAttachmentSRB, true);
        }
        else
        {
            if (auto* pSubpassInput = pInputAttachmentPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SubpassInput"))
                pSubpassInput->Set(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            pInputAttachmentPSO->CreateShaderResourceBinding(&pInputAttachmentSRB, true);
        }
        ASSERT_NE(pInputAttachmentSRB, nullptr);
    }

    ITextureView* pRTAttachments[] = //
        {
            pTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
            pSwapChain->GetCurrentBackBufferRTV() //
        };

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass input attachment test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    pContext->SetPipelineState(pPSO);

    BeginRenderPassAttribs RPBeginInfo;
    RPBeginInfo.pRenderPass  = pRenderPass;
    RPBeginInfo.pFramebuffer = pFramebuffer;

    OptimizedClearValue ClearValues[2];
    ClearValues[0].Color[0] = 0;
    ClearValues[0].Color[1] = 0;
    ClearValues[0].Color[2] = 0;
    ClearValues[0].Color[3] = 0;

    ClearValues[1].Color[0] = ClearColor[0];
    ClearValues[1].Color[1] = ClearColor[1];
    ClearValues[1].Color[2] = ClearColor[2];
    ClearValues[1].Color[3] = ClearColor[3];

    RPBeginInfo.pClearValues        = ClearValues;
    RPBeginInfo.ClearValueCount     = _countof(ClearValues);
    RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(RPBeginInfo);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pContext->NextSubpass();

    pContext->SetPipelineState(pInputAttachmentPSO);
    pContext->CommitShaderResources(pInputAttachmentSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->Draw(DrawAttrs);

    pContext->EndRenderPass();

    Present();
}

TEST_F(RenderPassTest, InputAttachment)
{
    TestInputAttachment(false, false);
}

TEST_F(RenderPassTest, InputAttachmentWithSignature)
{
    TestInputAttachment(true, false);
}

TEST_F(RenderPassTest, MemorylessInputAttachment)
{
    const auto  RequiredBindFlags = BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;
    const auto& MemoryInfo        = GPUTestingEnvironment::GetInstance()->GetDevice()->GetAdapterInfo().Memory;

    if ((MemoryInfo.MemorylessTextureBindFlags & RequiredBindFlags) != RequiredBindFlags)
    {
        GTEST_SKIP() << "Memoryless attachment is not supported by device";
    }
    TestInputAttachment(false, true);
}


void RenderPassTest::TestInputAttachmentGeneralLayout(bool UseSignature)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    if (!pDevice->GetDeviceInfo().IsVulkanDevice())
    {
        GTEST_SKIP() << "Input attachment with general layout is not supported by device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    constexpr float ClearColor[] = {0.5f, 0.125f, 0.25f, 0.25f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderPassInputAttachmentReferenceVk(pSwapChain, ClearColor);
                break;
#endif

            case RENDER_DEVICE_TYPE_UNDEFINED: // to avoid compilation error
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    const auto& SCDesc = pSwapChain->GetDesc();

    RenderPassAttachmentDesc Attachments[1];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].SampleCount  = 1;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    RefCntAutoPtr<ITexture> pTex;
    {
        TextureDesc TexDesc;
        TexDesc.Name      = "Input attachment test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = SCDesc.ColorBufferFormat;
        TexDesc.Width     = SCDesc.Width;
        TexDesc.Height    = SCDesc.Height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        TexDesc.MiscFlags = MISC_TEXTURE_FLAG_NONE;

        pDevice->CreateTexture(TexDesc, nullptr, &pTex);
        ASSERT_NE(pTex, nullptr);
    }

    SubpassDesc Subpasses[2];

    // clang-format off
    constexpr AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    constexpr AttachmentReference RTAttachmentRefs1[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET} // auto replaced with general layout
    };
    constexpr AttachmentReference InputAttachmentRefs1[] =
    {
        {0, RESOURCE_STATE_INPUT_ATTACHMENT} // auto replaced with general layout
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;

    Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
    Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
    Subpasses[1].InputAttachmentCount        = _countof(InputAttachmentRefs1);
    Subpasses[1].pInputAttachments           = InputAttachmentRefs1;

    SubpassDependencyDesc Dependencies[1];
    Dependencies[0].SrcSubpass    = 0;
    Dependencies[0].DstSubpass    = 1;
    Dependencies[0].SrcStageMask  = PIPELINE_STAGE_FLAG_RENDER_TARGET;
    Dependencies[0].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER | PIPELINE_STAGE_FLAG_RENDER_TARGET;
    Dependencies[0].SrcAccessMask = ACCESS_FLAG_RENDER_TARGET_WRITE;
    Dependencies[0].DstAccessMask = ACCESS_FLAG_SHADER_READ | ACCESS_FLAG_RENDER_TARGET_WRITE;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass general input attachment test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;
    RPDesc.DependencyCount = _countof(Dependencies);
    RPDesc.pDependencies   = Dependencies;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateDrawTrisPSO(pRenderPass, 1, pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    RefCntAutoPtr<IPipelineState>         pInputAttachmentPSO;
    RefCntAutoPtr<IShaderResourceBinding> pInputAttachmentSRB;
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Render pass test - input attachment";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.pRenderPass                  = pRenderPass;
        GraphicsPipeline.SubpassIndex                 = 1;
        GraphicsPipeline.SmplDesc.Count               = 1;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc       = {"Input attachment test VS", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = GLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc       = {"Input attachment test PS", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = GLSL::InputAttachmentTest_FS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        constexpr ShaderResourceVariableDesc Variables[] =
            {
                {SHADER_TYPE_PIXEL, "g_SubpassInput", SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_VARIABLE_FLAG_GENERAL_INPUT_ATTACHMENT} //
            };

        RefCntAutoPtr<IPipelineResourceSignature> pSignature;
        IPipelineResourceSignature*               ppSignatures[1]{};
        if (UseSignature)
        {
            constexpr PipelineResourceDesc Resources[] = //
                {
                    {SHADER_TYPE_PIXEL, "g_SubpassInput", 1, SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                       = "Render pass test - signature";
            PRSDesc.UseCombinedTextureSamplers = true;
            PRSDesc.Resources                  = Resources;
            PRSDesc.NumResources               = _countof(Resources);

            pDevice->CreatePipelineResourceSignature(PRSDesc, &pSignature);
            ASSERT_NE(pSignature, nullptr);
            ppSignatures[0]                       = pSignature;
            PSOCreateInfo.ppResourceSignatures    = ppSignatures;
            PSOCreateInfo.ResourceSignaturesCount = 1;
        }
        else
        {
            PSODesc.ResourceLayout.Variables    = Variables;
            PSODesc.ResourceLayout.NumVariables = _countof(Variables);
        }

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pInputAttachmentPSO);
        ASSERT_NE(pInputAttachmentPSO, nullptr);
        if (pSignature)
        {
            pSignature->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SubpassInput")->Set(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            pSignature->CreateShaderResourceBinding(&pInputAttachmentSRB, true);
        }
        else
        {
            pInputAttachmentPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SubpassInput")->Set(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            pInputAttachmentPSO->CreateShaderResourceBinding(&pInputAttachmentSRB, true);
        }
        ASSERT_NE(pInputAttachmentSRB, nullptr);
    }

    ITextureView* pRTAttachments[] = {pTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass input attachment test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    pContext->SetPipelineState(pPSO);

    BeginRenderPassAttribs RPBeginInfo;
    RPBeginInfo.pRenderPass  = pRenderPass;
    RPBeginInfo.pFramebuffer = pFramebuffer;

    OptimizedClearValue ClearValues[1];
    ClearValues[0].Color[0] = ClearColor[0];
    ClearValues[0].Color[1] = ClearColor[1];
    ClearValues[0].Color[2] = ClearColor[2];
    ClearValues[0].Color[3] = ClearColor[3];

    RPBeginInfo.pClearValues        = ClearValues;
    RPBeginInfo.ClearValueCount     = _countof(ClearValues);
    RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(RPBeginInfo);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pContext->NextSubpass();

    pContext->SetPipelineState(pInputAttachmentPSO);
    pContext->CommitShaderResources(pInputAttachmentSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->Draw(DrawAttrs);

    pContext->EndRenderPass();

    CopyTextureAttribs CopyAttrs;
    CopyAttrs.pSrcTexture = pTex;
    CopyAttrs.pDstTexture = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

    CopyAttrs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    CopyAttrs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->CopyTexture(CopyAttrs);

    Present();
}

TEST_F(RenderPassTest, InputAttachmentGeneralLayout)
{
    TestInputAttachmentGeneralLayout(false);
}

TEST_F(RenderPassTest, InputAttachmentGeneralLayoutWithSignature)
{
    TestInputAttachmentGeneralLayout(true);
}

} // namespace
