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

#include <array>
#include <limits>

#include "GPUTestingEnvironment.hpp"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static constexpr char g_TrivialVSSource[] = R"(
void main(out float4 pos : SV_Position)
{
    pos = float4(0.0, 0.0, 0.0, 0.0);
}
)";

static constexpr char g_TrivialPSSource[] = R"(
float4 main() : SV_Target
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static constexpr char g_TexturePSSource[] = R"(
Texture2D g_Texture;
float4 main() : SV_Target
{
    return g_Texture.Load(int3(0,0,0));
}
)";

static constexpr char g_TrivialMSSource[] = R"(
struct VertexOut
{
    float4 Pos : SV_Position;
};

[numthreads(1,1,1)]
[outputtopology("triangle")]
void main(out indices  uint3     tris[1],
          out vertices VertexOut verts[3])
{
    SetMeshOutputCounts(4, 2);

    tris[0] = uint3(0, 1, 2);
    verts[0].Pos = float4(0.0, 0.0, 0.0, 1.0);
    verts[1].Pos = float4(-1.0, 1.0, 0.0, 1.0);
    verts[2].Pos = float4(1.0, 1.0, 0.0, 1.0);
}
)";

static constexpr char g_TrivialCSSource[] = R"(
[numthreads(8,8,1)]
void main()
{
}
)";

static constexpr char g_TrivialRGenSource[] = R"(
[shader("raygeneration")]
void main()
{}
)";

static constexpr char g_TrivialRMissSource[] = R"(
struct RTPayload { float4 Color; };
[shader("miss")]
void main(inout RTPayload payload)
{}
)";

static constexpr char g_TrivialRCHitSource[] = R"(
struct RTPayload { float4 Color; };
[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{}
)";

static constexpr char g_TrivialRAHitSource[] = R"(
struct RTPayload { float4 Color; };
[shader("anyhit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{}
)";

static constexpr char g_TrivialRIntSource[] = R"(
[shader("intersection")]
void main()
{}
)";

static constexpr char g_TrivialRCallSource[] = R"(
struct Params { float4 Col; };
[shader("callable")]
void main(inout Params params)
{}
)";


class PSOCreationFailureTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        auto* const pDevice    = pEnv->GetDevice();
        const auto& DeviceInfo = pDevice->GetDeviceInfo();

        sm_HasMeshShader = DeviceInfo.Features.MeshShaders && pEnv->HasDXCompiler();
        sm_HasRayTracing = pEnv->SupportsRayTracing();

        ShaderCreateInfo ShaderCI;
        ShaderCI.Source         = g_TrivialVSSource;
        ShaderCI.EntryPoint     = "main";
        ShaderCI.Desc           = {"TrivialVS (PSOCreationFailureTest)", SHADER_TYPE_VERTEX, true};
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        pDevice->CreateShader(ShaderCI, &sm_pTrivialVS);
        ASSERT_TRUE(sm_pTrivialVS);

        ShaderCI.Source          = g_TrivialPSSource;
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "TrivialPS (PSOCreationFailureTest)";
        pDevice->CreateShader(ShaderCI, &sm_pTrivialPS);
        ASSERT_TRUE(sm_pTrivialPS);

        ShaderCI.Source          = g_TexturePSSource;
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "TexturePS (PSOCreationFailureTest)";
        pDevice->CreateShader(ShaderCI, &sm_pTexturePS);
        ASSERT_TRUE(sm_pTexturePS);

        ShaderCI.Source          = g_TrivialCSSource;
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.Desc.Name       = "TrivialCS (PSOCreationFailureTest)";
        pDevice->CreateShader(ShaderCI, &sm_pTrivialCS);
        ASSERT_TRUE(sm_pTrivialCS);

        sm_DefaultGraphicsPsoCI.PSODesc.Name                      = "PSOCreationFailureTest - default graphics PSO desc";
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.NumRenderTargets = 1;
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM;
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_D32_FLOAT;

        sm_DefaultGraphicsPsoCI.pVS = sm_pTrivialVS;
        sm_DefaultGraphicsPsoCI.pPS = sm_pTrivialPS;

        {
            RefCntAutoPtr<IPipelineState> pGraphicsPSO;
            pDevice->CreateGraphicsPipelineState(GetGraphicsPSOCreateInfo("PSOCreationFailureTest - OK graphics PSO"), &pGraphicsPSO);
            ASSERT_TRUE(pGraphicsPSO);
        }

        sm_DefaultComputePsoCI.PSODesc.Name = "PSOCreationFailureTest - default compute PSO desc";
        sm_DefaultComputePsoCI.pCS          = sm_pTrivialCS;

        {
            RefCntAutoPtr<IPipelineState> pComputePSO;
            pDevice->CreateComputePipelineState(GetComputePSOCreateInfo("PSOCreationFailureTest - OK compute PSO"), &pComputePSO);
            ASSERT_TRUE(pComputePSO);
        }

        if (sm_HasMeshShader)
        {
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;

            ShaderCI.Source          = g_TrivialMSSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
            ShaderCI.Desc.Name       = "TrivialMS DXC (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialMS);
            ASSERT_TRUE(sm_pTrivialMS);

            ShaderCI.Source          = g_TrivialPSSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.Desc.Name       = "TrivialPS DXC (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialPS_DXC);
            ASSERT_TRUE(sm_pTrivialPS_DXC);

            sm_DefaultMeshPsoCI.PSODesc.Name                      = "PSOCreationFailureTest - default mesh PSO desc";
            sm_DefaultMeshPsoCI.GraphicsPipeline.NumRenderTargets = 1;
            sm_DefaultMeshPsoCI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM;
            sm_DefaultMeshPsoCI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_D32_FLOAT;
            sm_DefaultMeshPsoCI.PSODesc.PipelineType              = PIPELINE_TYPE_MESH;

            sm_DefaultMeshPsoCI.pMS = sm_pTrivialMS;
            sm_DefaultMeshPsoCI.pPS = sm_pTrivialPS_DXC;

            RefCntAutoPtr<IPipelineState> pMeshPSO;
            pDevice->CreateGraphicsPipelineState(GetMeshPSOCreateInfo("PSOCreationFailureTest - OK mesh PSO"), &pMeshPSO);
            ASSERT_TRUE(pMeshPSO);
        }

        if (sm_HasRayTracing)
        {
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
            ShaderCI.HLSLVersion    = {6, 3};

            ShaderCI.Source          = g_TrivialRGenSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_GEN;
            ShaderCI.Desc.Name       = "TrivialRGen (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRG);
            ASSERT_TRUE(sm_pTrivialRG);

            ShaderCI.Source          = g_TrivialRMissSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_MISS;
            ShaderCI.Desc.Name       = "TrivialRMiss (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRMiss);
            ASSERT_TRUE(sm_pTrivialRMiss);

            ShaderCI.Source          = g_TrivialRCallSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_CALLABLE;
            ShaderCI.Desc.Name       = "TrivialRCall (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRCall);
            ASSERT_TRUE(sm_pTrivialRCall);

            ShaderCI.Source          = g_TrivialRCHitSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_CLOSEST_HIT;
            ShaderCI.Desc.Name       = "TrivialRCHit (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRCHit);
            ASSERT_TRUE(sm_pTrivialRCHit);

            ShaderCI.Source          = g_TrivialRAHitSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_ANY_HIT;
            ShaderCI.Desc.Name       = "TrivialRAHit (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRAHit);
            ASSERT_TRUE(sm_pTrivialRAHit);

            ShaderCI.Source          = g_TrivialRIntSource;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_INTERSECTION;
            ShaderCI.Desc.Name       = "TrivialRInt (PSOCreationFailureTest)";
            pDevice->CreateShader(ShaderCI, &sm_pTrivialRInt);
            ASSERT_TRUE(sm_pTrivialRInt);

            sm_DefaultRayTracingPsoCI.PSODesc.Name         = "PSOCreationFailureTest - default ray tracing PSO desc";
            sm_DefaultRayTracingPsoCI.PSODesc.PipelineType = PIPELINE_TYPE_RAY_TRACING;

            sm_DefaultRayTracingPsoCI.RayTracingPipeline.MaxRecursionDepth = 1;

            sm_GeneralGroups[0]                          = {"Main", sm_pTrivialRG};
            sm_DefaultRayTracingPsoCI.pGeneralShaders    = sm_GeneralGroups.data();
            sm_DefaultRayTracingPsoCI.GeneralShaderCount = static_cast<Uint32>(sm_GeneralGroups.size());

            RefCntAutoPtr<IPipelineState> pRayTracingPSO;
            pDevice->CreateRayTracingPipelineState(GetRayTracingPSOCreateInfo("PSOCreationFailureTest - OK ray tracing PSO"), &pRayTracingPSO);
            ASSERT_TRUE(pRayTracingPSO);
        }

        RenderPassDesc RPDesc;
        RPDesc.Name = "PSOCreationFailureTest - render pass";
        RenderPassAttachmentDesc Attachments[2]{};
        Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
        Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
        Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
        Attachments[1].Format       = TEX_FORMAT_D32_FLOAT;
        Attachments[1].InitialState = RESOURCE_STATE_DEPTH_WRITE;
        Attachments[1].FinalState   = RESOURCE_STATE_DEPTH_WRITE;
        RPDesc.AttachmentCount      = _countof(Attachments);
        RPDesc.pAttachments         = Attachments;

        AttachmentReference ColorAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
        AttachmentReference DepthAttachmentRef{1, RESOURCE_STATE_DEPTH_WRITE};
        SubpassDesc         Subpasses[1]{};
        Subpasses[0].RenderTargetAttachmentCount = 1;
        Subpasses[0].pRenderTargetAttachments    = &ColorAttachmentRef;
        Subpasses[0].pDepthStencilAttachment     = &DepthAttachmentRef;

        RPDesc.SubpassCount = _countof(Subpasses);
        RPDesc.pSubpasses   = Subpasses;

        pDevice->CreateRenderPass(RPDesc, &sm_pRenderPass);
        ASSERT_TRUE(sm_pRenderPass);

        {
            RefCntAutoPtr<IPipelineState> pGraphicsPSO;
            pDevice->CreateGraphicsPipelineState(GetGraphicsPSOCreateInfo("PSOCreationFailureTest - OK PSO with render pass", true), &pGraphicsPSO);
            ASSERT_TRUE(pGraphicsPSO);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            ImmutableSamplerDesc ImmutableSmplers[] //
                {
                    ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}} //
                };

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                 = "PRS0";
            PRSDesc.NumResources         = _countof(Resources);
            PRSDesc.Resources            = Resources;
            PRSDesc.NumImmutableSamplers = _countof(ImmutableSmplers);
            PRSDesc.ImmutableSamplers    = ImmutableSmplers;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature0);
            ASSERT_TRUE(sm_pSignature0);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS0A";
            PRSDesc.NumResources = _countof(Resources);
            PRSDesc.Resources    = Resources;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature0A);
            ASSERT_TRUE(sm_pSignature0A);
        }

        if (DeviceInfo.Features.GeometryShaders)
        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS1";
            PRSDesc.BindingIndex = 1;
            PRSDesc.NumResources = _countof(Resources);
            PRSDesc.Resources    = Resources;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature1);
            ASSERT_TRUE(sm_pSignature1);
        }

        if (DeviceInfo.Features.GeometryShaders)
        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_GEOMETRY, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            ImmutableSamplerDesc ImmutableSmplers[] //
                {
                    ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture_sampler", SamplerDesc{}} //
                };

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                 = "PRS1A";
            PRSDesc.BindingIndex         = 1;
            PRSDesc.NumResources         = _countof(Resources);
            PRSDesc.Resources            = Resources;
            PRSDesc.NumImmutableSamplers = _countof(ImmutableSmplers);
            PRSDesc.ImmutableSamplers    = ImmutableSmplers;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature1A);
            ASSERT_TRUE(sm_pSignature1A);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pTrivialVS.Release();
        sm_pTrivialPS.Release();
        sm_pTrivialPS_DXC.Release();
        sm_pTexturePS.Release();
        sm_pTrivialMS.Release();
        sm_pTrivialCS.Release();
        sm_pTrivialRG.Release();
        sm_pTrivialRMiss.Release();
        sm_pTrivialRCall.Release();
        sm_pTrivialRCHit.Release();
        sm_pTrivialRAHit.Release();
        sm_pTrivialRInt.Release();
        sm_pRenderPass.Release();
        sm_pSignature0.Release();
        sm_pSignature0A.Release();
        sm_pSignature1.Release();
        sm_pSignature1A.Release();
    }

    static GraphicsPipelineStateCreateInfo GetGraphicsPSOCreateInfo(const char* Name, bool UseRenderPass = false)
    {
        auto CI{sm_DefaultGraphicsPsoCI};
        CI.PSODesc.Name = Name;
        if (UseRenderPass)
        {
            CI.GraphicsPipeline.NumRenderTargets = 0;
            CI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.pRenderPass      = sm_pRenderPass;
        }
        return CI;
    }

    static GraphicsPipelineStateCreateInfo GetMeshPSOCreateInfo(const char* Name, bool UseRenderPass = false)
    {
        VERIFY_EXPR(HasMeshShader());

        auto CI{sm_DefaultMeshPsoCI};
        CI.PSODesc.Name = Name;
        if (UseRenderPass)
        {
            CI.GraphicsPipeline.NumRenderTargets = 0;
            CI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.pRenderPass      = sm_pRenderPass;
        }
        return CI;
    }

    static ComputePipelineStateCreateInfo GetComputePSOCreateInfo(const char* Name)
    {
        auto CI{sm_DefaultComputePsoCI};
        CI.PSODesc.Name = Name;
        return CI;
    }

    static RayTracingPipelineStateCreateInfo GetRayTracingPSOCreateInfo(const char* Name)
    {
        VERIFY_EXPR(HasRayTracing());

        auto CI{sm_DefaultRayTracingPsoCI};
        CI.PSODesc.Name = Name;
        return CI;
    }

    static IShader* GetVS() { return sm_pTrivialVS; }
    static IShader* GetPS() { return sm_pTrivialPS; }
    static IShader* GetMS() { return sm_pTrivialMS; }

    static IShader* GetTexturePS() { return sm_pTexturePS; }

    static IShader* GetRayGen() { return sm_pTrivialRG; }
    static IShader* GetRayMiss() { return sm_pTrivialRMiss; }
    static IShader* GetCallable() { return sm_pTrivialRCall; }
    static IShader* GetRayClosestHit() { return sm_pTrivialRCHit; }
    static IShader* GetRayAnyHit() { return sm_pTrivialRAHit; }
    static IShader* GetRayIntersection() { return sm_pTrivialRInt; }

    static bool HasMeshShader() { return sm_HasMeshShader; }
    static bool HasRayTracing() { return sm_HasRayTracing; }

    static void TestCreatePSOFailure(GraphicsPipelineStateCreateInfo CI, const char* ExpectedErrorSubstring)
    {
        auto* const pEnv    = GPUTestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        RefCntAutoPtr<IPipelineState> pPSO;
        pEnv->SetErrorAllowance(2, "Errors below are expected: testing PSO creation failure\n");
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateGraphicsPipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        CI.PSODesc.Name = nullptr;
        pEnv->SetErrorAllowance(2);
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateGraphicsPipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        pEnv->SetErrorAllowance(0);
    }

    static void TestCreatePSOFailure(ComputePipelineStateCreateInfo CI, const char* ExpectedErrorSubstring)
    {
        auto* const pEnv    = GPUTestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        RefCntAutoPtr<IPipelineState> pPSO;

        pEnv->SetErrorAllowance(2, "Errors below are expected: testing PSO creation failure\n");
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateComputePipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        CI.PSODesc.Name = nullptr;
        pEnv->SetErrorAllowance(2);
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateComputePipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        pEnv->SetErrorAllowance(0);
    }

    static void TestCreatePSOFailure(RayTracingPipelineStateCreateInfo CI, const char* ExpectedErrorSubstring)
    {
        auto* const pEnv    = GPUTestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        RefCntAutoPtr<IPipelineState> pPSO;

        pEnv->SetErrorAllowance(2, "Errors below are expected: testing PSO creation failure\n");
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateRayTracingPipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        CI.PSODesc.Name = nullptr;
        pEnv->SetErrorAllowance(2);
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateRayTracingPipelineState(CI, &pPSO);
        ASSERT_FALSE(pPSO);

        pEnv->SetErrorAllowance(0);
    }

protected:
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature0;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature0A;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature1;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature1A;

private:
    static RefCntAutoPtr<IShader>     sm_pTrivialVS;
    static RefCntAutoPtr<IShader>     sm_pTrivialPS;
    static RefCntAutoPtr<IShader>     sm_pTrivialPS_DXC;
    static RefCntAutoPtr<IShader>     sm_pTexturePS;
    static RefCntAutoPtr<IShader>     sm_pTrivialMS;
    static RefCntAutoPtr<IShader>     sm_pTrivialRG;
    static RefCntAutoPtr<IShader>     sm_pTrivialRMiss;
    static RefCntAutoPtr<IShader>     sm_pTrivialRCHit;
    static RefCntAutoPtr<IShader>     sm_pTrivialRAHit;
    static RefCntAutoPtr<IShader>     sm_pTrivialRInt;
    static RefCntAutoPtr<IShader>     sm_pTrivialRCall;
    static RefCntAutoPtr<IShader>     sm_pTrivialCS;
    static RefCntAutoPtr<IRenderPass> sm_pRenderPass;

    static GraphicsPipelineStateCreateInfo   sm_DefaultGraphicsPsoCI;
    static GraphicsPipelineStateCreateInfo   sm_DefaultMeshPsoCI;
    static ComputePipelineStateCreateInfo    sm_DefaultComputePsoCI;
    static RayTracingPipelineStateCreateInfo sm_DefaultRayTracingPsoCI;

    static std::array<RayTracingGeneralShaderGroup, 1> sm_GeneralGroups;

    static bool sm_HasMeshShader;
    static bool sm_HasRayTracing;
};

RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialVS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialPS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTexturePS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialPS_DXC;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialMS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialCS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRG;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRMiss;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRCHit;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRAHit;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRInt;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialRCall;
RefCntAutoPtr<IRenderPass>                PSOCreationFailureTest::sm_pRenderPass;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature0;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature0A;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature1;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature1A;

GraphicsPipelineStateCreateInfo   PSOCreationFailureTest::sm_DefaultGraphicsPsoCI;
GraphicsPipelineStateCreateInfo   PSOCreationFailureTest::sm_DefaultMeshPsoCI;
ComputePipelineStateCreateInfo    PSOCreationFailureTest::sm_DefaultComputePsoCI;
RayTracingPipelineStateCreateInfo PSOCreationFailureTest::sm_DefaultRayTracingPsoCI;

std::array<RayTracingGeneralShaderGroup, 1> PSOCreationFailureTest::sm_GeneralGroups;

bool PSOCreationFailureTest::sm_HasMeshShader;
bool PSOCreationFailureTest::sm_HasRayTracing;


TEST_F(PSOCreationFailureTest, InvalidGraphicsPipelineType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Graphics Pipeline Type")};
    PsoCI.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    TestCreatePSOFailure(PsoCI, "Pipeline type must be GRAPHICS or MESH");
}

TEST_F(PSOCreationFailureTest, NoVS)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - no VS")};
    PsoCI.pVS = nullptr;
    TestCreatePSOFailure(PsoCI, "Vertex shader must not be null");
}

TEST_F(PSOCreationFailureTest, IncorrectVSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect VS Type")};
    PsoCI.pVS = GetPS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_PIXEL is not a valid type for vertex shader");
}

TEST_F(PSOCreationFailureTest, IncorrectPSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect PS Type")};
    PsoCI.pPS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for pixel shader");
}

TEST_F(PSOCreationFailureTest, IncorrectGSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect GS Type")};
    PsoCI.pGS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for geometry shader");
}

TEST_F(PSOCreationFailureTest, IncorrectDSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect DS Type")};
    PsoCI.pDS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for domain shader");
}

TEST_F(PSOCreationFailureTest, IncorrectHSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect HS Type")};
    PsoCI.pHS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for hull shader");
}

TEST_F(PSOCreationFailureTest, WrongSubpassIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - wrong subpass index")};
    PsoCI.GraphicsPipeline.SubpassIndex = 1;
    TestCreatePSOFailure(PsoCI, "Subpass index (1) must be 0");
}

TEST_F(PSOCreationFailureTest, UndefinedFillMode)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Undefined Fill Mode")};
    PsoCI.GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "RasterizerDesc.FillMode must not be FILL_MODE_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, UndefinedCullMode)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Undefined Cull Mode")};
    PsoCI.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "RasterizerDesc.CullMode must not be CULL_MODE_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDepthFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Depth Func")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.DepthFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable           = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable          = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilDepthFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilDepthFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable                = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilDepthFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilDepthFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilDepthFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable               = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilDepthFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilPassOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilPassOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable           = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilPassOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilPassOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilPassOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable          = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilPassOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilFunc")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable         = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilFunc")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable        = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidSrcBlend)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid SrcBlend")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend    = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].SrcBlend must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDestBlend)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid DestBlend")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend   = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].DestBlend must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBlendOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid BlendOp")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp     = BLEND_OPERATION_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].BlendOp must not be BLEND_OPERATION_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidSrcBlendAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid SrcBlendAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable   = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].SrcBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDestBlendAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid DestBlendAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable    = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].DestBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBlendOpAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid BlendOpAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable  = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = BLEND_OPERATION_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].BlendOpAlpha must not be BLEND_OPERATION_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, NullVariableName)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - null variable name")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.Variables[1].Name must not be null");
}

TEST_F(PSOCreationFailureTest, EmptyVariableName)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - empty variable name")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.Variables[1].Name must not be empty");
}

TEST_F(PSOCreationFailureTest, UnknownVariableShaderStage)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - unknown variable shader stage")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_UNKNOWN, "g_Texture2", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.Variables[1].ShaderStages must not be SHADER_TYPE_UNKNOWN");
}


TEST_F(PSOCreationFailureTest, OverlappingVariableStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Overlapping Variable Stages")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "'g_Texture' is defined in overlapping shader stages (SHADER_TYPE_VERTEX, SHADER_TYPE_GEOMETRY and SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL)");
}


TEST_F(PSOCreationFailureTest, NullImmutableSamplerName)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - null immutable sampler name")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.ImmutableSamplers[1].SamplerOrTextureName must not be null");
}

TEST_F(PSOCreationFailureTest, EmptyImmutableSamplerName)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - empty immutable sampler name")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "", SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.ImmutableSamplers[1].SamplerOrTextureName must not be empty");
}

TEST_F(PSOCreationFailureTest, UndefinedImmutableSamplerShaderStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - undefined immutable sampler shader stages")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_UNKNOWN, "g_Texture_sampler2", SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    TestCreatePSOFailure(PsoCI, "ResourceLayout.ImmutableSamplers[1].ShaderStages must not be SHADER_TYPE_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, OverlappingImmutableSamplerStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Overlapping Immutable Sampler Stages")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture_sampler", SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    TestCreatePSOFailure(PsoCI, "'g_Texture_sampler' is defined in overlapping shader stages (SHADER_TYPE_VERTEX, SHADER_TYPE_GEOMETRY and SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL)");
}

TEST_F(PSOCreationFailureTest, RenderPassWithNonZeroNumRenderTargets)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With non-zero NumRenderTargets", true)};
    PsoCI.GraphicsPipeline.NumRenderTargets = 1;
    TestCreatePSOFailure(PsoCI, "NumRenderTargets must be 0");
}

TEST_F(PSOCreationFailureTest, RenderPassWithDSVFormat)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With defined DSV format", true)};
    PsoCI.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
    TestCreatePSOFailure(PsoCI, "DSVFormat must be TEX_FORMAT_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, RenderPassWithRTVFormat)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With defined RTV format", true)};
    PsoCI.GraphicsPipeline.RTVFormats[1] = TEX_FORMAT_RGBA8_UNORM;
    TestCreatePSOFailure(PsoCI, "RTVFormats[1] must be TEX_FORMAT_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, RenderPassWithInvalidSubpassIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With invalid Subpass index", true)};
    PsoCI.GraphicsPipeline.SubpassIndex = 2;
    TestCreatePSOFailure(PsoCI, "Subpass index (2) exceeds the number of subpasses (1)");
}

TEST_F(PSOCreationFailureTest, NullResourceSignatures)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Null Resource Signatures", true)};
    PsoCI.ResourceSignaturesCount = 2;
    TestCreatePSOFailure(PsoCI, "ppResourceSignatures is null, but ResourceSignaturesCount (2) is not zero");
}

TEST_F(PSOCreationFailureTest, ZeroResourceSignaturesCount)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Zero Resource Signatures Count", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = 0;
    TestCreatePSOFailure(PsoCI, "ppResourceSignatures is not null, but ResourceSignaturesCount is zero.");
}


TEST_F(PSOCreationFailureTest, SignatureWithNonZeroNumVariables)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Resource Signature With non-zero NumVariables", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    PsoCI.PSODesc.ResourceLayout.NumVariables = 3;
    TestCreatePSOFailure(PsoCI, "The number of variables defined through resource layout (3) must be zero");
}

TEST_F(PSOCreationFailureTest, SignatureWithNonZeroNumImmutableSamplers)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Resource Signature With non-zero NumImmutableSamplers", true)};

    IPipelineResourceSignature* pSignatures[]         = {sm_pSignature0};
    PsoCI.ppResourceSignatures                        = pSignatures;
    PsoCI.ResourceSignaturesCount                     = _countof(pSignatures);
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = 4;
    TestCreatePSOFailure(PsoCI, "The number of immutable samplers defined through resource layout (4) must be zero");
}

TEST_F(PSOCreationFailureTest, NullSignature)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Null Signature", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, nullptr};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "signature at index 1 is null");
}

TEST_F(PSOCreationFailureTest, ConflictingSignatureBindIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Conflicting Signature Bind Index", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature0A};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "'PRS0A' at binding index 0 conflicts with another resource signature 'PRS0'");
}

TEST_F(PSOCreationFailureTest, ConflictingSignatureResourceStages)
{
    if (!sm_pSignature1)
        GTEST_SKIP();

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - conflicting signature resource stages", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature1};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "Shader resource 'g_Texture' is found in more than one resource signature ('PRS1' and 'PRS0')");
}

TEST_F(PSOCreationFailureTest, ConflictingImmutableSamplerStages)
{
    if (!sm_pSignature1A)
        GTEST_SKIP();

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - conflicting signature immutable sampler stages", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature1A};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);

    // In case of non-separable programs, there is another error - a resource ("g_Texture") is found in different signatures
    const auto* ExpectedError = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.SeparablePrograms ?
        "Immutable sampler 'g_Texture_sampler' is found in more than one resource signature ('PRS1A' and 'PRS0')" :
        "shader resource 'g_Texture' is found in more than one resource signature ('PRS1A' and 'PRS0')";
    TestCreatePSOFailure(PsoCI, ExpectedError);
}

TEST_F(PSOCreationFailureTest, InvalidComputePipelineType)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - Invalid Compute Pipeline Type")};
    PsoCI.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    TestCreatePSOFailure(PsoCI, "Pipeline type must be COMPUTE");
}

TEST_F(PSOCreationFailureTest, NoCS)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - no CS")};
    PsoCI.pCS = nullptr;
    TestCreatePSOFailure(PsoCI, "Compute shader must not be null");
}

TEST_F(PSOCreationFailureTest, InvalidCS)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - invalid CS")};
    PsoCI.pCS = GetPS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_PIXEL is not a valid type for compute shader");
}

TEST_F(PSOCreationFailureTest, NullMS)
{
    if (!HasMeshShader())
        GTEST_SKIP();

    auto PsoCI{GetMeshPSOCreateInfo("PSO Create Failure - null MS")};
    PsoCI.pMS = nullptr;
    TestCreatePSOFailure(PsoCI, "Mesh shader must not be null");
}

TEST_F(PSOCreationFailureTest, InvalidMS)
{
    if (!HasMeshShader())
        GTEST_SKIP();

    auto PsoCI{GetMeshPSOCreateInfo("PSO Create Failure - Invalid MS")};
    PsoCI.pMS = GetPS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_PIXEL is not a valid type for mesh shader");
}

TEST_F(PSOCreationFailureTest, NullRG)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - NUll ray-gen shader")};
    PsoCI.pGeneralShaders    = nullptr;
    PsoCI.GeneralShaderCount = 0;
    TestCreatePSOFailure(PsoCI, "At least one shader with type SHADER_TYPE_RAY_GEN must be provided");
}

TEST_F(PSOCreationFailureTest, InvalidRTPipelineType)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - Invalid RT pipeline type")};
    PsoCI.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    TestCreatePSOFailure(PsoCI, "Pipeline type must be RAY_TRACING");
}

TEST_F(PSOCreationFailureTest, InvalidShaderRecord)
{
    if (!HasRayTracing() || !GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().IsD3DDevice())
        GTEST_SKIP();

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - Invalid shader record")};
    PsoCI.pShaderRecordName                   = "ShaderRecord";
    PsoCI.RayTracingPipeline.ShaderRecordSize = 0;
    TestCreatePSOFailure(PsoCI, "pShaderRecordName must not be null if RayTracingPipeline.ShaderRecordSize is not zero");
}

TEST_F(PSOCreationFailureTest, TooBigRayRecursionDepth)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - too big ray recursion depth")};
    PsoCI.RayTracingPipeline.MaxRecursionDepth = std::numeric_limits<decltype(PsoCI.RayTracingPipeline.MaxRecursionDepth)>::max();
    TestCreatePSOFailure(PsoCI, "MaxRecursionDepth (255) exceeds device limit");
}

TEST_F(PSOCreationFailureTest, NullShaderGroupName)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingGeneralShaderGroup GeneralGroups[] = {{"Main", GetRayGen()}, {nullptr, GetRayMiss()}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - null shader group name")};
    PsoCI.pGeneralShaders    = GeneralGroups;
    PsoCI.GeneralShaderCount = _countof(GeneralGroups);
    TestCreatePSOFailure(PsoCI, "pGeneralShaders[1].Name must not be null");
}

TEST_F(PSOCreationFailureTest, EmptyShaderGroupName)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingGeneralShaderGroup GeneralGroups[] = {{"Main", GetRayGen()}, {"", GetRayMiss()}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - empty shader group name")};
    PsoCI.pGeneralShaders    = GeneralGroups;
    PsoCI.GeneralShaderCount = _countof(GeneralGroups);
    TestCreatePSOFailure(PsoCI, "pGeneralShaders[1].Name must not be empty");
}

TEST_F(PSOCreationFailureTest, NonUniqueShaderGroupName)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingGeneralShaderGroup GeneralGroups[] = {{"Main", GetRayGen()}, {"Main", GetRayMiss()}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - non-unique shader group name")};
    PsoCI.pGeneralShaders    = GeneralGroups;
    PsoCI.GeneralShaderCount = _countof(GeneralGroups);
    TestCreatePSOFailure(PsoCI, "pGeneralShaders[1].Name ('Main') has already been assigned to another group. All group names must be unique.");
}

TEST_F(PSOCreationFailureTest, NullGeneralShader)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingGeneralShaderGroup GeneralGroups[] = {{"Main", GetRayGen()}, {"Entry", nullptr}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - null general shader")};
    PsoCI.pGeneralShaders    = GeneralGroups;
    PsoCI.GeneralShaderCount = _countof(GeneralGroups);
    TestCreatePSOFailure(PsoCI, "pGeneralShaders[1].pShader must not be null");
}

TEST_F(PSOCreationFailureTest, NullTriHitShader)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingTriangleHitShaderGroup HitGroups[] = {{"ClosestHit", nullptr}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - null triangle closest hit shader")};
    PsoCI.pTriangleHitShaders    = HitGroups;
    PsoCI.TriangleHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "pTriangleHitShaders[0].pClosestHitShader must not be null");
}

TEST_F(PSOCreationFailureTest, NullProcHitShader)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingProceduralHitShaderGroup HitGroups[] = {{"Intersection", nullptr}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - null procedural intersection shader")};
    PsoCI.pProceduralHitShaders    = HitGroups;
    PsoCI.ProceduralHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "pProceduralHitShaders[0].pIntersectionShader must not be null");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinGeneralGroup)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingGeneralShaderGroup GeneralGroups[] = {{"Main", GetRayGen()}, {"Miss", GetRayMiss()}, {"Call", GetCallable()}, {"Hit", GetRayClosestHit()}};

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in general group")};
    PsoCI.pGeneralShaders    = GeneralGroups;
    PsoCI.GeneralShaderCount = _countof(GeneralGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_RAY_CLOSEST_HIT is not a valid type for ray tracing general shader");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinTriangleHitGroup1)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingTriangleHitShaderGroup HitGroups[] = {
        {"CHit", GetRayClosestHit(), nullptr},
        {"CHit-AHit", GetRayClosestHit(), GetRayAnyHit()},
        {"Miss", GetRayMiss()} //
    };

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in triangle hit group - 1")};
    PsoCI.pTriangleHitShaders    = HitGroups;
    PsoCI.TriangleHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_RAY_MISS is not a valid type for ray tracing triangle closest hit");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinTriangleHitGroup2)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingTriangleHitShaderGroup HitGroups[] = {
        {"CHit", GetRayClosestHit(), nullptr},
        {"CHit-AHit", GetRayClosestHit(), GetRayAnyHit()},
        {"CHit-Miss", GetRayClosestHit(), GetRayIntersection()} //
    };

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in triangle hit group - 2")};
    PsoCI.pTriangleHitShaders    = HitGroups;
    PsoCI.TriangleHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_RAY_INTERSECTION is not a valid type for ray tracing triangle any hit");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinProceduralHitGroup1)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingProceduralHitShaderGroup HitGroups[] = {
        {"Int", GetRayIntersection(), nullptr, nullptr},
        {"Int-CHit", GetRayIntersection(), GetRayClosestHit(), nullptr},
        {"Int-CHit-AHit", GetRayIntersection(), GetRayClosestHit(), GetRayAnyHit()},
        {"Call", GetCallable(), nullptr, nullptr} //
    };

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in procedural hit group - 1")};
    PsoCI.pProceduralHitShaders    = HitGroups;
    PsoCI.ProceduralHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_CALLABLE is not a valid type for ray tracing procedural intersection");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinProceduralHitGroup2)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingProceduralHitShaderGroup HitGroups[] = {
        {"Int", GetRayIntersection(), nullptr, nullptr},
        {"Int-CHit", GetRayIntersection(), GetRayClosestHit(), nullptr},
        {"Int-CHit-AHit", GetRayIntersection(), GetRayClosestHit(), GetRayAnyHit()},
        {"Int-RG", GetRayIntersection(), GetRayGen(), nullptr} //
    };

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in procedural hit group - 2")};
    PsoCI.pProceduralHitShaders    = HitGroups;
    PsoCI.ProceduralHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_RAY_GEN is not a valid type for ray tracing procedural closest hit");
}

TEST_F(PSOCreationFailureTest, InvalidShaderinProceduralHitGroup3)
{
    if (!HasRayTracing())
        GTEST_SKIP();

    const RayTracingProceduralHitShaderGroup HitGroups[] = {
        {"Int", GetRayIntersection(), nullptr, nullptr},
        {"Int-CHit", GetRayIntersection(), GetRayClosestHit(), nullptr},
        {"Int-CHit-AHit", GetRayIntersection(), GetRayClosestHit(), GetRayAnyHit()},
        {"Int-CHit-CHit", GetRayIntersection(), GetRayClosestHit(), GetRayClosestHit()} //
    };

    auto PsoCI{GetRayTracingPSOCreateInfo("PSO Create Failure - invalid shader in procedural hit group - 3")};
    PsoCI.pProceduralHitShaders    = HitGroups;
    PsoCI.ProceduralHitShaderCount = _countof(HitGroups);
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_RAY_CLOSEST_HIT is not a valid type for ray tracing procedural any hit");
}

TEST_F(PSOCreationFailureTest, MissingResource)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "PSO Create Failure - missing resource";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_AnotherTexture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);

    auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_NE(pPRS, nullptr);

    IPipelineResourceSignature* ppSignatures[] = {pPRS};

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - missing resource")};
    PsoCI.ppResourceSignatures    = ppSignatures;
    PsoCI.ResourceSignaturesCount = _countof(ppSignatures);

    PsoCI.pPS = GetTexturePS();

    std::string ExpectedErrorString = "contains resource 'g_Texture' that is not present in any pipeline resource signature";
    if (pDevice->GetDeviceInfo().Features.SeparablePrograms)
        ExpectedErrorString = std::string{"Shader 'TexturePS (PSOCreationFailureTest)' "} + ExpectedErrorString;
    // In non-separable programs case, PSO name is used instead of the shader name

    TestCreatePSOFailure(PsoCI, ExpectedErrorString.c_str());
}

TEST_F(PSOCreationFailureTest, InvalidResourceType)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "PSO Create Failure - Invalid Resource Type";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);

    auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_NE(pPRS, nullptr);

    IPipelineResourceSignature* ppSignatures[] = {pPRS};

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Resource Type")};
    PsoCI.ppResourceSignatures    = ppSignatures;
    PsoCI.ResourceSignaturesCount = _countof(ppSignatures);

    PsoCI.pPS = GetTexturePS();

    std::string ExpectedErrorString = "contains resource with name 'g_Texture' and type 'texture SRV' that is not compatible with type 'buffer SRV'";
    if (pDevice->GetDeviceInfo().Features.SeparablePrograms)
        ExpectedErrorString = std::string{"Shader 'TexturePS (PSOCreationFailureTest)' "} + ExpectedErrorString;
    // In non-separable programs case, PSO name is used of the shader name

    TestCreatePSOFailure(PsoCI, ExpectedErrorString.c_str());
}

TEST_F(PSOCreationFailureTest, InvalidArraySize)
{
    static constexpr char PSSource[] = R"(
    Texture2D g_Texture[3];
    float4 main() : SV_Target
    {
        return g_Texture[0].Load(int3(0,0,0)) + g_Texture[1].Load(int3(0,0,0)) + g_Texture[2].Load(int3(0,0,0));
    }
    )";

    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    auto* const pDevice = pEnv->GetDevice();

    ShaderCreateInfo ShaderCI;
    ShaderCI.Source         = PSSource;
    ShaderCI.Desc           = {"Invalid Array Size (PSOCreationFailureTest)", SHADER_TYPE_PIXEL, true};
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    RefCntAutoPtr<IShader> pPS;
    pDevice->CreateShader(ShaderCI, &pPS);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "PSO Create Failure - Invalid Array Size";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_NE(pPRS, nullptr);

    IPipelineResourceSignature* ppSignatures[] = {pPRS};

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Array Size")};
    PsoCI.ppResourceSignatures    = ppSignatures;
    PsoCI.ResourceSignaturesCount = _countof(ppSignatures);

    PsoCI.pPS = pPS;

    std::string ExpectedErrorString = "contains resource 'g_Texture' whose array size (3) is greater than the array size (2)";
    if (pDevice->GetDeviceInfo().Features.SeparablePrograms)
        ExpectedErrorString = std::string{"Shader 'Invalid Array Size (PSOCreationFailureTest)' "} + ExpectedErrorString;
    // In non-separable programs case, PSO name is used

    TestCreatePSOFailure(PsoCI, ExpectedErrorString.c_str());
}


TEST_F(PSOCreationFailureTest, InvalidRunTimeArray)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    if (!DeviceInfo.Features.ShaderResourceRuntimeArray)
    {
        GTEST_SKIP();
    }

    static constexpr char PSSource_HLSL[] = R"(
    Texture2D g_Texture[];
    cbuffer ConstBuffer
    {
        uint Index;
    }
    float4 main() : SV_Target
    {
        return g_Texture[Index].Load(int3(0,0,0));
    }
    )";

    static constexpr char PSSource_GLSL[] = R"(
    #version 460 core
    #extension GL_EXT_nonuniform_qualifier : require
    #extension GL_EXT_samplerless_texture_functions : require

    uniform texture2D g_Texture[];
    layout(std140) uniform ConstBuffer
    {
        uint Index;
    };
    layout(location=0) out vec4 out_Color;

    void main()
    {
        out_Color = texelFetch(g_Texture[nonuniformEXT(Index)], ivec2(0,0), 0);
    }
    )";

    ShaderCreateInfo ShaderCI;
    ShaderCI.Desc           = {"Invalid Run-Time Array (PSOCreationFailureTest)", SHADER_TYPE_PIXEL, true};
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    if (DeviceInfo.IsD3DDevice())
    {
        ShaderCI.Source         = PSSource_HLSL;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    }
    else
    {
        ShaderCI.Source         = PSSource_GLSL;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
    }
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_ENABLE_UNBOUNDED_ARRAYS;
    RefCntAutoPtr<IShader> pPS;
    pDevice->CreateShader(ShaderCI, &pPS);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "PSO Create Failure - Invalid Run-Time Array";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "ConstBuffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_Texture", 2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_NE(pPRS, nullptr);

    IPipelineResourceSignature* ppSignatures[] = {pPRS};

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Run-Time Array")};
    PsoCI.ppResourceSignatures    = ppSignatures;
    PsoCI.ResourceSignaturesCount = _countof(ppSignatures);

    PsoCI.pPS = pPS;

    TestCreatePSOFailure(PsoCI, "Shader 'Invalid Run-Time Array (PSOCreationFailureTest)' contains resource 'g_Texture' that is a runtime-sized array, "
                                "but in the resource signature 'PSO Create Failure - Invalid Run-Time Array' the resource is defined without the PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY flag.");
}

TEST_F(PSOCreationFailureTest, NonSeparablePrograms_SeparateResources)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP();
    }

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Non Separable Programs - Separate Resources")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "there are separate resources with the name 'g_Texture' in shader stages SHADER_TYPE_PIXEL and SHADER_TYPE_VERTEX");
}

TEST_F(PSOCreationFailureTest, NonSeparablePrograms_SeparateImmutableSamplers)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP();
    }

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Non Separable Programs - Separate Immutable Samplers")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    TestCreatePSOFailure(PsoCI, "there are separate immutable samplers with the name 'g_Texture_sampler' in shader stages SHADER_TYPE_PIXEL and SHADER_TYPE_VERTEX");
}


TEST_F(PSOCreationFailureTest, MissingCombinedImageSampler)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    if (!DeviceInfo.IsVulkanDevice() && !DeviceInfo.IsGLDevice())
    {
        GTEST_SKIP() << "Combined image samplers are only available in GL and Vulkan";
    }

    static constexpr char VSSource_GLSL[] = R"(
    #ifndef GL_ES
    out gl_PerVertex
    {
        vec4 gl_Position;
    };
    #endif

    void main()
    {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    }
    )";

    static constexpr char PSSource_GLSL[] = R"(
    uniform sampler2D g_Texture;
    layout(location=0) out vec4 out_Color;
    void main()
    {
        out_Color = texture(g_Texture, vec2(0.5, 0.5));
    }
    )";

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc   = {"Missing combined image sampler (PSOCreationFailureTest) VS", SHADER_TYPE_VERTEX, true};
        ShaderCI.Source = VSSource_GLSL;
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_TRUE(pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc   = {"Missing combined image sampler (PSOCreationFailureTest) PS", SHADER_TYPE_PIXEL, true};
        ShaderCI.Source = PSSource_GLSL;
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_TRUE(pPS);
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "PSO Create Failure - Missing Combined Image Sampler";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_NE(pPRS, nullptr);

    IPipelineResourceSignature* ppSignatures[] = {pPRS};

    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Missing Combined Image Sampler")};
    PsoCI.ppResourceSignatures    = ppSignatures;
    PsoCI.ResourceSignaturesCount = _countof(ppSignatures);

    PsoCI.pVS = pVS;
    PsoCI.pPS = pPS;

    TestCreatePSOFailure(PsoCI, "contains combined image sampler 'g_Texture', while the same resource is defined by the pipeline "
                                "resource signature 'PSO Create Failure - Missing Combined Image Sampler' as separate image");
}

} // namespace
