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
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <array>

#include "ShaderMacroHelper.hpp"
#include "GraphicsAccessories.hpp"
#include "BasicMath.hpp"

#include "GPUTestingEnvironment.hpp"
#include "ResourceLayoutTestCommon.hpp"
#include "TestingSwapChainBase.hpp"

#if VULKAN_SUPPORTED
#    include "Vulkan/TestingEnvironmentVk.hpp"
#endif

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class ShaderResourceLayoutTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
    }

    static void TearDownTestSuite()
    {
        GPUTestingEnvironment::GetInstance()->Reset();
    }

    static void VerifyShaderResources(IShader*                 pShader,
                                      const ShaderResourceDesc ExpectedResources[],
                                      Uint32                   ExpectedResCount)
    {
        auto ResCount = pShader->GetResourceCount();
        EXPECT_EQ(ResCount, ExpectedResCount) << "Actual number of resources (" << ResCount << ") in shader '"
                                              << pShader->GetDesc().Name << "' does not match the expected number of resources (" << ExpectedResCount << ')';
        std::unordered_map<std::string, ShaderResourceDesc> Resources;
        for (Uint32 i = 0; i < ResCount; ++i)
        {
            ShaderResourceDesc ResDesc;
            pShader->GetResourceDesc(i, ResDesc);
            Resources.emplace(ResDesc.Name, ResDesc);
        }

        for (Uint32 i = 0; i < ExpectedResCount; ++i)
        {
            const auto& ExpectedRes = ExpectedResources[i];

            auto it = Resources.find(ExpectedRes.Name);
            if (it != Resources.end())
            {
                EXPECT_EQ(it->second.Type, ExpectedRes.Type) << "Unexpected type of resource '" << ExpectedRes.Name << '\'';
                EXPECT_EQ(it->second.ArraySize, ExpectedRes.ArraySize) << "Unexpected array size of resource '" << ExpectedRes.Name << '\'';
                Resources.erase(it);
            }
            else
            {
                ADD_FAILURE() << "Unable to find resource '" << ExpectedRes.Name << "' in shader '" << pShader->GetDesc().Name << "'";
            }
        }

        for (auto it : Resources)
        {
            ADD_FAILURE() << "Unexpected resource '" << it.second.Name << "' in shader '" << pShader->GetDesc().Name << "'";
        }
    }

    template <typename TModifyShaderCI>
    static RefCntAutoPtr<IShader> CreateShader(const char*               ShaderName,
                                               const char*               FileName,
                                               const char*               EntryPoint,
                                               SHADER_TYPE               ShaderType,
                                               SHADER_SOURCE_LANGUAGE    SrcLang,
                                               const ShaderMacro*        Macros,
                                               const ShaderResourceDesc* ExpectedResources,
                                               Uint32                    NumExpectedResources,
                                               TModifyShaderCI           ModifyShaderCI)
    {
        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        auto* const pDevice    = pEnv->GetDevice();
        const auto& deviceCaps = pDevice->GetDeviceInfo();

        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/ShaderResourceLayout", &pShaderSourceFactory);

        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.UseCombinedTextureSamplers = deviceCaps.IsGLDevice();

        ShaderCI.FilePath        = FileName;
        ShaderCI.Desc.Name       = ShaderName;
        ShaderCI.EntryPoint      = EntryPoint;
        ShaderCI.Desc.ShaderType = ShaderType;
        ShaderCI.SourceLanguage  = SrcLang;
        ShaderCI.Macros          = Macros;
        ShaderCI.ShaderCompiler  = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        ModifyShaderCI(ShaderCI);

        RefCntAutoPtr<IShader> pShader;
        pDevice->CreateShader(ShaderCI, &pShader);

        if (pShader && deviceCaps.Features.ShaderResourceQueries)
        {
            VerifyShaderResources(pShader, ExpectedResources, NumExpectedResources);
            Diligent::Testing::PrintShaderResources(pShader);
        }

        return pShader;
    }

    static RefCntAutoPtr<IShader> CreateShader(const char*               ShaderName,
                                               const char*               FileName,
                                               const char*               EntryPoint,
                                               SHADER_TYPE               ShaderType,
                                               SHADER_SOURCE_LANGUAGE    SrcLang,
                                               const ShaderMacro*        Macros,
                                               const ShaderResourceDesc* ExpectedResources,
                                               Uint32                    NumExpectedResources)
    {
        return CreateShader(ShaderName,
                            FileName,
                            EntryPoint,
                            ShaderType,
                            SrcLang,
                            Macros,
                            ExpectedResources,
                            NumExpectedResources,
                            [](const ShaderCreateInfo&) {});
    }


    static void CreateGraphicsPSO(IShader*                               pVS,
                                  IShader*                               pPS,
                                  const PipelineResourceLayoutDesc&      ResourceLayout,
                                  RefCntAutoPtr<IPipelineState>&         pPSO,
                                  RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        PSODesc.Name                     = "Shader resource layout test";
        PSODesc.ResourceLayout           = ResourceLayout;
        PSODesc.SRBAllocationGranularity = 16;

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.NumRenderTargets  = 1;
        GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
        GraphicsPipeline.DSVFormat         = TEX_FORMAT_UNKNOWN;

        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        if (pPSO)
            pPSO->CreateShaderResourceBinding(&pSRB, false);
    }

    static void CreateComputePSO(IShader*                               pCS,
                                 const PipelineResourceLayoutDesc&      ResourceLayout,
                                 RefCntAutoPtr<IPipelineState>&         pPSO,
                                 RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        ComputePipelineStateCreateInfo PSOCreateInfo;

        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        PSODesc.Name           = "Shader resource layout test";
        PSODesc.PipelineType   = PIPELINE_TYPE_COMPUTE;
        PSODesc.ResourceLayout = ResourceLayout;
        PSOCreateInfo.pCS      = pCS;

        pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
        if (pPSO)
            pPSO->CreateShaderResourceBinding(&pSRB, false);
    }

    void TestTexturesAndImtblSamplers(bool TestImtblSamplers, SHADER_SOURCE_LANGUAGE ShaderLang);
    void TestStructuredOrFormattedBuffer(bool IsFormatted);
    void TestRWStructuredOrFormattedBuffer(bool IsFormatted);
};

#define SET_STATIC_VAR(PSO, ShaderFlags, VarName, SetMethod, ...)                                \
    do                                                                                           \
    {                                                                                            \
        auto pStaticVar = PSO->GetStaticVariableByName(ShaderFlags, VarName);                    \
        EXPECT_NE(pStaticVar, nullptr) << "Unable to find static variable '" << VarName << '\''; \
        if (pStaticVar != nullptr)                                                               \
            pStaticVar->SetMethod(__VA_ARGS__);                                                  \
    } while (false)


#define SET_SRB_VAR(SRB, ShaderFlags, VarName, SetMethod, ...)                          \
    do                                                                                  \
    {                                                                                   \
        auto pVar = SRB->GetVariableByName(ShaderFlags, VarName);                       \
        EXPECT_NE(pVar, nullptr) << "Unable to find SRB variable '" << VarName << '\''; \
        if (pVar != nullptr)                                                            \
            pVar->SetMethod(__VA_ARGS__);                                               \
    } while (false)


void ShaderResourceLayoutTest::TestTexturesAndImtblSamplers(bool TestImtblSamplers, SHADER_SOURCE_LANGUAGE ShaderLang)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pSwapChain = pEnv->GetSwapChain();
    const auto& deviceCaps = pDevice->GetDeviceInfo();

    if (ShaderLang != SHADER_SOURCE_LANGUAGE_HLSL && deviceCaps.IsD3DDevice())
        GTEST_SKIP() << "Direct3D backends support HLSL only";

    float ClearColor[] = {0.25, 0.5, 0.75, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    // Prepare reference textures filled with different colors
    // Texture array sizes in the shader
    static constexpr Uint32 StaticTexArraySize  = 2;
    static constexpr Uint32 MutableTexArraySize = 4;
    static constexpr Uint32 DynamicTexArraySize = 3;

    ReferenceTextures RefTextures{
        3 + StaticTexArraySize + MutableTexArraySize + DynamicTexArraySize,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    // Texture indices for vertex/shader bindings
    static constexpr size_t Tex2D_StaticIdx[] = {2, 10};
    static constexpr size_t Tex2D_MutIdx[]    = {0, 11};
    static constexpr size_t Tex2D_DynIdx[]    = {1, 9};

    static constexpr size_t Tex2DArr_StaticIdx[] = {7, 0};
    static constexpr size_t Tex2DArr_MutIdx[]    = {3, 5};
    static constexpr size_t Tex2DArr_DynIdx[]    = {9, 2};

    const Uint32 VSResArrId = 0;
    const Uint32 PSResArrId = deviceCaps.Features.SeparablePrograms ? 1 : 0;
    VERIFY_EXPR(deviceCaps.IsGLDevice() || PSResArrId != VSResArrId);

    // clang-format off
    std::vector<ShaderResourceDesc> Resources =
    {
        ShaderResourceDesc{"g_Tex2D_Static",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Mut",         SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Dyn",         SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2DArr_Static",   SHADER_RESOURCE_TYPE_TEXTURE_SRV, StaticTexArraySize},
        ShaderResourceDesc{"g_Tex2DArr_Mut",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, MutableTexArraySize},
        ShaderResourceDesc{"g_Tex2DArr_Dyn",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, DynamicTexArraySize}
    };
    // clang-format on
    if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL && !deviceCaps.IsGLDevice())
    {
        if (TestImtblSamplers)
        {
            // clang-format off
            Resources.emplace_back("g_Tex2D_Static_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2D_Mut_sampler",       SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2D_Dyn_sampler",       SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2DArr_Static_sampler", SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2DArr_Mut_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, MutableTexArraySize);
            Resources.emplace_back("g_Tex2DArr_Dyn_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, DynamicTexArraySize);
            // clang-format on
        }
        else
        {
            Resources.emplace_back("g_Sampler", SHADER_RESOURCE_TYPE_SAMPLER, 1);
        }
    }

    ShaderMacroHelper Macros;

    auto PrepareMacros = [&](Uint32 s) {
        Macros.Clear();

        if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
            Macros.AddShaderMacro("float4", "vec4");

        Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
        Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
        Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

        RefTextures.ClearUsedValues();

        // Add macros that define reference colors
        Macros.AddShaderMacro("Tex2D_Static_Ref", RefTextures.GetColor(Tex2D_StaticIdx[s]));
        Macros.AddShaderMacro("Tex2D_Mut_Ref", RefTextures.GetColor(Tex2D_MutIdx[s]));
        Macros.AddShaderMacro("Tex2D_Dyn_Ref", RefTextures.GetColor(Tex2D_DynIdx[s]));

        for (Uint32 i = 0; i < StaticTexArraySize; ++i)
            Macros.AddShaderMacro((std::string{"Tex2DArr_Static_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_StaticIdx[s] + i));

        for (Uint32 i = 0; i < MutableTexArraySize; ++i)
            Macros.AddShaderMacro((std::string{"Tex2DArr_Mut_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_MutIdx[s] + i));

        for (Uint32 i = 0; i < DynamicTexArraySize; ++i)
            Macros.AddShaderMacro((std::string{"Tex2DArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_DynIdx[s] + i));

        return static_cast<const ShaderMacro*>(Macros);
    };

    auto ModifyShaderCI = [TestImtblSamplers, ShaderLang, pEnv](ShaderCreateInfo& ShaderCI) {
        if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL)
        {
            if (TestImtblSamplers)
            {
                ShaderCI.UseCombinedTextureSamplers = true;
                // Immutable sampler arrays are not allowed in 5.1, and DXC only supports 6.0+
                ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
                ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
            }

            if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
            {
                // Due to bug in D3D12 WARP, we have to use SM5.0 with old compiler
                ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
                ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
            }
        }
        else if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
        {
            ShaderCI.UseCombinedTextureSamplers = true;
        }
        else
        {
            UNEXPECTED("Unexpected shader language");
        }
    };

    const char* ShaderPath = nullptr;
    std::string Name       = "ShaderResourceLayoutTest.";
    switch (ShaderLang)
    {
        case SHADER_SOURCE_LANGUAGE_HLSL:
            ShaderPath = TestImtblSamplers ? "ImmutableSamplers.hlsl" : "Textures.hlsl";
            Name += TestImtblSamplers ? "ImtblSamplers" : "Textures";
            break;

        case SHADER_SOURCE_LANGUAGE_GLSL:
            ShaderPath = "CombinedSamplers.glsl";
            Name += TestImtblSamplers ? "CombinedImtblSamplers_GLSL" : "CombinedSamplers_GLSL";
            break;

        default:
            UNSUPPORTED("Unsupported shader language");
    }

    auto pVS = CreateShader((Name + " - VS").c_str(), ShaderPath,
                            ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL ? "main" : "VSMain",
                            SHADER_TYPE_VERTEX, ShaderLang, PrepareMacros(VSResArrId),
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);

    auto pPS = CreateShader((Name + " - PS").c_str(), ShaderPath,
                            ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL ? "main" : "PSMain",
                            SHADER_TYPE_PIXEL, ShaderLang, PrepareMacros(PSResArrId),
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    std::vector<ShaderResourceVariableDesc> Vars;

    auto AddVar = [&](const char* Name, SHADER_RESOURCE_VARIABLE_TYPE VarType) //
    {
        if (deviceCaps.Features.SeparablePrograms)
        {
            // Use separate variables for each stage
            Vars.emplace_back(SHADER_TYPE_VERTEX, Name, VarType);
            Vars.emplace_back(SHADER_TYPE_PIXEL, Name, VarType);
        }
        else
        {
            // Use one shared variable
            Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, Name, VarType);
        }
    };
    // clang-format off
    AddVar("g_Tex2D_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    AddVar("g_Tex2D_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    AddVar("g_Tex2D_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

    AddVar("g_Tex2DArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    AddVar("g_Tex2DArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    AddVar("g_Tex2DArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    // clang-format on

    std::vector<ImmutableSamplerDesc> ImtblSamplers;
    if (TestImtblSamplers)
    {
        // clang-format off
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Static",    SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Mut",       SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",       SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Static", SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut",    SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn",    SamplerDesc{});
        // clang-format on
    }
    else
    {
        if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL)
        {
            if (!deviceCaps.IsGLDevice())
                ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sampler", SamplerDesc{});
        }
        else if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
        {
            RefCntAutoPtr<ISampler> pSampler;
            pDevice->CreateSampler(SamplerDesc{}, &pSampler);
            for (Uint32 i = 0; i < RefTextures.GetTextureCount(); ++i)
                RefTextures.GetView(i)->SetSampler(pSampler);
        }
        else
        {
            UNEXPECTED("Unexpected shader language");
        }
    }

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables            = Vars.data();
    ResourceLayout.NumVariables         = static_cast<Uint32>(Vars.size());
    ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
    ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    auto BindResources = [&](SHADER_TYPE ShaderType) {
        const auto id = ShaderType == SHADER_TYPE_VERTEX ? VSResArrId : PSResArrId;

        SET_STATIC_VAR(pPSO, ShaderType, "g_Tex2D_Static", Set, RefTextures.GetViewObjects(Tex2D_StaticIdx[id])[0]);
        SET_STATIC_VAR(pPSO, ShaderType, "g_Tex2DArr_Static", SetArray, RefTextures.GetViewObjects(Tex2DArr_StaticIdx[id]), 0, StaticTexArraySize);

        SET_SRB_VAR(pSRB, ShaderType, "g_Tex2D_Mut", Set, RefTextures.GetViewObjects(Tex2D_MutIdx[id])[0]);
        SET_SRB_VAR(pSRB, ShaderType, "g_Tex2DArr_Mut", SetArray, RefTextures.GetViewObjects(Tex2DArr_MutIdx[id]), 0, MutableTexArraySize);

        // Bind 0 for dynamic resources - will rebind for the second draw
        SET_SRB_VAR(pSRB, ShaderType, "g_Tex2D_Dyn", Set, RefTextures.GetViewObjects(0)[0]);
        SET_SRB_VAR(pSRB, ShaderType, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(0), 0, DynamicTexArraySize);
    };
    BindResources(SHADER_TYPE_VERTEX);
    BindResources(SHADER_TYPE_PIXEL);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Dyn", Set, RefTextures.GetViewObjects(Tex2D_DynIdx[VSResArrId])[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx[VSResArrId]), 0, 1);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx[VSResArrId] + 1), 1, DynamicTexArraySize - 1);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, RefTextures.GetViewObjects(Tex2D_DynIdx[PSResArrId])[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx[PSResArrId]), 0, 1);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx[PSResArrId] + 1), 1, DynamicTexArraySize - 1);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

TEST_F(ShaderResourceLayoutTest, Textures)
{
    TestTexturesAndImtblSamplers(false /*TestImtblSamplers*/, SHADER_SOURCE_LANGUAGE_HLSL);
}

TEST_F(ShaderResourceLayoutTest, ImmutableSamplers)
{
    TestTexturesAndImtblSamplers(true /*TestImtblSamplers*/, SHADER_SOURCE_LANGUAGE_HLSL);
}

TEST_F(ShaderResourceLayoutTest, CombinedSamplers_GLSL)
{
    TestTexturesAndImtblSamplers(false /*TestImtblSamplers*/, SHADER_SOURCE_LANGUAGE_GLSL);
}

TEST_F(ShaderResourceLayoutTest, CombinedImmutableSamplers_GLSL)
{
    TestTexturesAndImtblSamplers(true /*TestImtblSamplers*/, SHADER_SOURCE_LANGUAGE_GLSL);
}


void ShaderResourceLayoutTest::TestStructuredOrFormattedBuffer(bool IsFormatted)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pSwapChain = pEnv->GetSwapChain();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    float ClearColor[] = {0.625, 0.125, 0.25, 0.875};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    static constexpr int StaticBuffArraySize  = 4;
    static constexpr int MutableBuffArraySize = 3;
    static constexpr int DynamicBuffArraySize = 2;

    // Prepare buffers with reference values
    ReferenceBuffers RefBuffers{
        3 + StaticBuffArraySize + MutableBuffArraySize + DynamicBuffArraySize,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        BUFFER_VIEW_SHADER_RESOURCE,
        IsFormatted ? BUFFER_MODE_FORMATTED : BUFFER_MODE_STRUCTURED //
    };

    // Buffer indices for vertex/shader bindings
    static constexpr size_t Buff_StaticIdx[] = {2, 11};
    static constexpr size_t Buff_MutIdx[]    = {0, 10};
    static constexpr size_t Buff_DynIdx[]    = {1, 9};

    static constexpr size_t BuffArr_StaticIdx[] = {8, 0};
    static constexpr size_t BuffArr_MutIdx[]    = {3, 4};
    static constexpr size_t BuffArr_DynIdx[]    = {6, 7};

    const Uint32 VSResArrId = 0;
    const Uint32 PSResArrId = DeviceInfo.Features.SeparablePrograms ? 1 : 0;
    VERIFY_EXPR(DeviceInfo.IsGLDevice() || PSResArrId != VSResArrId);

    ShaderMacroHelper Macros;

    auto PrepareMacros = [&](Uint32 s, SHADER_SOURCE_LANGUAGE Lang) {
        Macros.Clear();

        if (Lang == SHADER_SOURCE_LANGUAGE_GLSL)
            Macros.AddShaderMacro("float4", "vec4");

        Macros.AddShaderMacro("STATIC_BUFF_ARRAY_SIZE", StaticBuffArraySize);
        Macros.AddShaderMacro("MUTABLE_BUFF_ARRAY_SIZE", MutableBuffArraySize);
        Macros.AddShaderMacro("DYNAMIC_BUFF_ARRAY_SIZE", DynamicBuffArraySize);

        RefBuffers.ClearUsedValues();

        // Add macros that define reference colors
        Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(Buff_StaticIdx[s]));
        Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(Buff_MutIdx[s]));
        Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(Buff_DynIdx[s]));

        for (Uint32 i = 0; i < StaticBuffArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Static_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_StaticIdx[s] + i));

        for (Uint32 i = 0; i < MutableBuffArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Mut_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_MutIdx[s] + i));

        for (Uint32 i = 0; i < DynamicBuffArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_DynIdx[s] + i));

        return static_cast<const ShaderMacro*>(Macros);
    };

    // Vulkan only allows 16 dynamic storage buffer bindings among all stages, so
    // use arrays only in fragment shader for structured buffer test.
    const auto UseArraysInPSOnly = !IsFormatted && (DeviceInfo.IsVulkanDevice() || DeviceInfo.IsMetalDevice());

    // clang-format off
    std::vector<ShaderResourceDesc> Resources =
    {
        {"g_Buff_Static", SHADER_RESOURCE_TYPE_BUFFER_SRV, 1},
        {"g_Buff_Mut",    SHADER_RESOURCE_TYPE_BUFFER_SRV, 1},
        {"g_Buff_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_SRV, 1}
    };

    auto AddArrayResources = [&Resources]()
    {
        Resources.emplace_back("g_BuffArr_Static", SHADER_RESOURCE_TYPE_BUFFER_SRV, StaticBuffArraySize);
        Resources.emplace_back("g_BuffArr_Mut",    SHADER_RESOURCE_TYPE_BUFFER_SRV, MutableBuffArraySize);
        Resources.emplace_back("g_BuffArr_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_SRV, DynamicBuffArraySize);
    };
    // clang-format on
    if (!UseArraysInPSOnly)
    {
        AddArrayResources();
    }

    const char*            ShaderFileName = nullptr;
    SHADER_SOURCE_LANGUAGE SrcLang        = SHADER_SOURCE_LANGUAGE_DEFAULT;
    if (DeviceInfo.IsD3DDevice())
    {
        ShaderFileName = IsFormatted ? "FormattedBuffers.hlsl" : "StructuredBuffers.hlsl";
        SrcLang        = SHADER_SOURCE_LANGUAGE_HLSL;
    }
    else if (DeviceInfo.IsVulkanDevice() || DeviceInfo.IsGLDevice() || DeviceInfo.IsMetalDevice())
    {
        ShaderFileName = IsFormatted ? "FormattedBuffers.hlsl" : "StructuredBuffers.glsl";
        SrcLang        = IsFormatted ? SHADER_SOURCE_LANGUAGE_HLSL : SHADER_SOURCE_LANGUAGE_GLSL;
    }
    else
    {
        GTEST_FAIL() << "Unexpected device type";
    }

    auto ModifyShaderCI = [pEnv](ShaderCreateInfo& ShaderCI) {
        if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // Due to bug in D3D12 WARP, we have to use SM5.0 with old compiler
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };
    auto pVS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.FormattedBuffers - VS" : "ShaderResourceLayoutTest.StructuredBuffers - VS",
                            ShaderFileName, SrcLang == SHADER_SOURCE_LANGUAGE_HLSL ? "VSMain" : "main",
                            SHADER_TYPE_VERTEX, SrcLang, PrepareMacros(VSResArrId, SrcLang),
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);
    if (UseArraysInPSOnly)
    {
        AddArrayResources();
    }

    auto pPS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.FormattedBuffers - PS" : "ShaderResourceLayoutTest.StructuredBuffers - PS",
                            ShaderFileName, SrcLang == SHADER_SOURCE_LANGUAGE_HLSL ? "PSMain" : "main",
                            SHADER_TYPE_PIXEL, SrcLang, PrepareMacros(PSResArrId, SrcLang),
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    std::vector<ShaderResourceVariableDesc> Vars;

    auto AddVar = [&](const char* Name, SHADER_RESOURCE_VARIABLE_TYPE VarType) //
    {
        if (DeviceInfo.Features.SeparablePrograms)
        {
            // Use separate variables for each stage
            Vars.emplace_back(SHADER_TYPE_VERTEX, Name, VarType);
            Vars.emplace_back(SHADER_TYPE_PIXEL, Name, VarType);
        }
        else
        {
            // Use one shared variable
            Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, Name, VarType);
        }
    };
    // clang-format off
    AddVar("g_Buff_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    AddVar("g_Buff_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    AddVar("g_Buff_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

    AddVar("g_BuffArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    AddVar("g_BuffArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    AddVar("g_BuffArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars.data();
    ResourceLayout.NumVariables = static_cast<Uint32>(Vars.size());

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    auto BindResources = [&](SHADER_TYPE ShaderType) {
        const auto id = ShaderType == SHADER_TYPE_VERTEX ? VSResArrId : PSResArrId;

        SET_STATIC_VAR(pPSO, ShaderType, "g_Buff_Static", Set, RefBuffers.GetViewObjects(Buff_StaticIdx[id])[0]);

        if (ShaderType == SHADER_TYPE_PIXEL || !UseArraysInPSOnly)
        {
            SET_STATIC_VAR(pPSO, ShaderType, "g_BuffArr_Static", SetArray, RefBuffers.GetViewObjects(BuffArr_StaticIdx[id]), 0, StaticBuffArraySize);
        }
        else
        {
            EXPECT_EQ(pPSO->GetStaticVariableByName(ShaderType, "g_BuffArr_Static"), nullptr);
        }


        SET_SRB_VAR(pSRB, ShaderType, "g_Buff_Mut", Set, RefBuffers.GetViewObjects(Buff_MutIdx[id])[0]);
        SET_SRB_VAR(pSRB, ShaderType, "g_Buff_Dyn", Set, RefBuffers.GetViewObjects(0)[0]); // Will rebind for the second draw

        if (ShaderType == SHADER_TYPE_PIXEL || !UseArraysInPSOnly)
        {
            SET_SRB_VAR(pSRB, ShaderType, "g_BuffArr_Mut", SetArray, RefBuffers.GetViewObjects(BuffArr_MutIdx[id]), 0, MutableBuffArraySize);
            SET_SRB_VAR(pSRB, ShaderType, "g_BuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(0), 0, DynamicBuffArraySize); // Will rebind for the second draw
        }
        else
        {
            EXPECT_EQ(pSRB->GetVariableByName(ShaderType, "g_BuffArr_Mut"), nullptr);
            EXPECT_EQ(pSRB->GetVariableByName(ShaderType, "g_BuffArr_Dyn"), nullptr);
        }
    };
    BindResources(SHADER_TYPE_VERTEX);
    BindResources(SHADER_TYPE_PIXEL);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Buff_Dyn", Set, RefBuffers.GetViewObjects(Buff_DynIdx[VSResArrId])[0]);
    if (!UseArraysInPSOnly)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx[VSResArrId] + 0), 0, 1);
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx[VSResArrId] + 1), 1, 1);
    }

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Buff_Dyn", Set, RefBuffers.GetViewObjects(Buff_DynIdx[PSResArrId])[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_BuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx[PSResArrId]), 0, DynamicBuffArraySize);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

TEST_F(ShaderResourceLayoutTest, FormattedBuffers)
{
    TestStructuredOrFormattedBuffer(true /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, StructuredBuffers)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceInfo().IsGLDevice())
    {
        GTEST_SKIP() << "Read-only structured buffers in glsl are currently "
                        "identified as UAVs in OpenGL backend because "
                        "there seems to be no way to detect read-only property on the host";
    }

    TestStructuredOrFormattedBuffer(false /*IsFormatted*/);
}


void ShaderResourceLayoutTest::TestRWStructuredOrFormattedBuffer(bool IsFormatted)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pSwapChain = pEnv->GetSwapChain();

    ComputeShaderReference(pSwapChain);

    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    constexpr Uint32 MaxStaticBuffArraySize  = 4;
    constexpr Uint32 MaxMutableBuffArraySize = 3;
    constexpr Uint32 MaxDynamicBuffArraySize = 2;
    constexpr Uint32 MaxUAVBuffers =
        MaxStaticBuffArraySize +
        MaxMutableBuffArraySize +
        MaxDynamicBuffArraySize +
        3 /*non array resources*/ +
        1 /*output UAV texture*/;
    (void)MaxUAVBuffers;

    bool UseReducedUAVCount = false;
    switch (DeviceInfo.Type)
    {
        case RENDER_DEVICE_TYPE_D3D11:
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
        case RENDER_DEVICE_TYPE_METAL:
            UseReducedUAVCount = true;
            break;

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
        {
            const auto* pEnvVk = static_cast<const TestingEnvironmentVk*>(pEnv);
            const auto& Limits = pEnvVk->DeviceProps.limits;
            if (Limits.maxPerStageDescriptorStorageImages < 8)
            {
                GTEST_SKIP() << "The number of supported UAV buffers is too small.";
            }
            else if (Limits.maxPerStageDescriptorStorageImages < MaxUAVBuffers)
                UseReducedUAVCount = true;
            break;
        }
#endif

        case RENDER_DEVICE_TYPE_D3D12:
            break;

        default:
            UNEXPECTED("Unexpected device type");
    }

    // Prepare buffers with reference values
    ReferenceBuffers RefBuffers{
        3 + MaxStaticBuffArraySize + MaxMutableBuffArraySize + MaxDynamicBuffArraySize + 1, // Extra buffer for dynamic variables
        USAGE_DEFAULT,
        BIND_UNORDERED_ACCESS,
        BUFFER_VIEW_UNORDERED_ACCESS,
        IsFormatted ? BUFFER_MODE_FORMATTED : BUFFER_MODE_STRUCTURED //
    };

    const Uint32 StaticBuffArraySize  = UseReducedUAVCount ? 1 : MaxStaticBuffArraySize;
    const Uint32 MutableBuffArraySize = UseReducedUAVCount ? 1 : MaxMutableBuffArraySize;
    const Uint32 DynamicBuffArraySize = MaxDynamicBuffArraySize;

    static constexpr size_t Buff_StaticIdx = 0;
    static constexpr size_t Buff_MutIdx    = 1;
    static constexpr size_t Buff_DynIdx    = 2;

    static constexpr size_t BuffArr_StaticIdx = 3;
    static constexpr size_t BuffArr_MutIdx    = 7;
    static constexpr size_t BuffArr_DynIdx    = 10;

    // clang-format off
    ShaderResourceDesc Resources[] =
    {
        {"g_tex2DUAV",         SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWBuff_Static",    SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuff_Mut",       SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuff_Dyn",       SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuffArr_Static", SHADER_RESOURCE_TYPE_BUFFER_UAV, StaticBuffArraySize },
        {"g_RWBuffArr_Mut",    SHADER_RESOURCE_TYPE_BUFFER_UAV, MutableBuffArraySize},
        {"g_RWBuffArr_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_UAV, DynamicBuffArraySize}
    };
    // clang-format on

    const char*            ShaderFileName = nullptr;
    SHADER_SOURCE_LANGUAGE SrcLang        = SHADER_SOURCE_LANGUAGE_DEFAULT;
    if (pDevice->GetDeviceInfo().IsD3DDevice())
    {
        ShaderFileName = IsFormatted ? "RWFormattedBuffers.hlsl" : "RWStructuredBuffers.hlsl";
        SrcLang        = SHADER_SOURCE_LANGUAGE_HLSL;
    }
    else if (DeviceInfo.IsVulkanDevice() || DeviceInfo.IsGLDevice() || DeviceInfo.IsMetalDevice())
    {
        ShaderFileName = IsFormatted ? "RWFormattedBuffers.hlsl" : "RWStructuredBuffers.glsl";
        SrcLang        = IsFormatted ? SHADER_SOURCE_LANGUAGE_HLSL : SHADER_SOURCE_LANGUAGE_GLSL;
    }
    else
    {
        GTEST_FAIL() << "Unexpected device type";
    }

    ShaderMacroHelper Macros;
    if (SrcLang == SHADER_SOURCE_LANGUAGE_GLSL)
        Macros.AddShaderMacro("float4", "vec4");

    Macros.AddShaderMacro("STATIC_BUFF_ARRAY_SIZE", static_cast<int>(StaticBuffArraySize));
    Macros.AddShaderMacro("MUTABLE_BUFF_ARRAY_SIZE", static_cast<int>(MutableBuffArraySize));
    Macros.AddShaderMacro("DYNAMIC_BUFF_ARRAY_SIZE", static_cast<int>(DynamicBuffArraySize));

    // Add macros that define reference colors
    Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(Buff_StaticIdx));
    Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(Buff_MutIdx));
    Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(Buff_DynIdx));

    for (Uint32 i = 0; i < StaticBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Static_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_StaticIdx + i));

    for (Uint32 i = 0; i < MutableBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Mut_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_MutIdx + i));

    for (Uint32 i = 0; i < DynamicBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_DynIdx + i));

    auto ModifyShaderCI = [pEnv](ShaderCreateInfo& ShaderCI) {
        if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // Due to bug in D3D12 WARP, we have to use SM5.0 with old compiler
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };

    auto pCS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.RWFormattedBuffers - CS" : "ShaderResourceLayoutTest.RWtructuredBuffers - CS",
                            ShaderFileName, "main",
                            SHADER_TYPE_COMPUTE, SrcLang, Macros,
                            Resources, _countof(Resources), ModifyShaderCI);
    ASSERT_NE(pCS, nullptr);

    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateComputePSO(pCS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    ASSERT_TRUE(pTestingSwapChain);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_tex2DUAV", Set, pTestingSwapChain->GetCurrentBackBufferUAV());

    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWBuff_Static", Set, RefBuffers.GetViewObjects(Buff_StaticIdx)[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Static", SetArray, RefBuffers.GetViewObjects(BuffArr_StaticIdx), 0, StaticBuffArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Mut", Set, RefBuffers.GetViewObjects(Buff_MutIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Mut", SetArray, RefBuffers.GetViewObjects(BuffArr_MutIdx), 0, MutableBuffArraySize);

    // In Direct3D11 UAVs must not overlap!
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn", Set, RefBuffers.GetViewObjects(BuffArr_DynIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx + 1), 0, DynamicBuffArraySize);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const auto&            SCDesc = pSwapChain->GetDesc();
    DispatchComputeAttribs DispatchAttribs((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1);
    pContext->DispatchCompute(DispatchAttribs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn", Set, RefBuffers.GetViewObjects(Buff_DynIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx), 0, DynamicBuffArraySize);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

TEST_F(ShaderResourceLayoutTest, FormattedRWBuffers)
{
    TestRWStructuredOrFormattedBuffer(true /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, StructuredRWBuffers)
{
    TestRWStructuredOrFormattedBuffer(false /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, RWTextures)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    ComputeShaderReference(pSwapChain);

    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    constexpr Uint32 MaxStaticTexArraySize  = 2;
    constexpr Uint32 MaxMutableTexArraySize = 4;
    constexpr Uint32 MaxDynamicTexArraySize = 3;
    constexpr Uint32 MaxUAVTextures =
        MaxStaticTexArraySize +
        MaxMutableTexArraySize +
        MaxDynamicTexArraySize +
        3 /*non array resources*/ +
        1 /*output UAV texture*/;
    (void)MaxUAVTextures;

    bool UseReducedUAVCount = false;
    switch (DeviceInfo.Type)
    {
        case RENDER_DEVICE_TYPE_D3D11:
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
        case RENDER_DEVICE_TYPE_METAL:
            UseReducedUAVCount = true;
            break;

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
        {
            const auto* pEnvVk = static_cast<TestingEnvironmentVk*>(pEnv);
            const auto& Limits = pEnvVk->DeviceProps.limits;
            if (Limits.maxPerStageDescriptorStorageImages < 8)
            {
                GTEST_SKIP() << "The number of supported UAV textures is too small.";
            }
            else if (Limits.maxPerStageDescriptorStorageImages < MaxUAVTextures)
                UseReducedUAVCount = true;
            break;
        }
#endif

        case RENDER_DEVICE_TYPE_D3D12:
            break;

        default:
            UNEXPECTED("Unexpected device type");
    }

    const Uint32 StaticTexArraySize  = MaxStaticTexArraySize;
    const Uint32 MutableTexArraySize = UseReducedUAVCount ? 1 : MaxMutableTexArraySize;
    const Uint32 DynamicTexArraySize = UseReducedUAVCount ? 1 : MaxDynamicTexArraySize;

    ReferenceTextures RefTextures{
        3 + MaxStaticTexArraySize + MaxMutableTexArraySize + MaxDynamicTexArraySize + 1, // Extra texture for dynamic variables
        128, 128,
        USAGE_DEFAULT,
        BIND_UNORDERED_ACCESS,
        TEXTURE_VIEW_UNORDERED_ACCESS //
    };

    static constexpr size_t Tex2D_StaticIdx = 0;
    static constexpr size_t Tex2D_MutIdx    = 1;
    static constexpr size_t Tex2D_DynIdx    = 2;

    static constexpr size_t Tex2DArr_StaticIdx = 3;
    static constexpr size_t Tex2DArr_MutIdx    = 5;
    static constexpr size_t Tex2DArr_DynIdx    = 9;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
    Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
    Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

    // Add macros that define reference colors
    Macros.AddShaderMacro("Tex2D_Static_Ref", RefTextures.GetColor(Tex2D_StaticIdx));
    Macros.AddShaderMacro("Tex2D_Mut_Ref", RefTextures.GetColor(Tex2D_MutIdx));
    Macros.AddShaderMacro("Tex2D_Dyn_Ref", RefTextures.GetColor(Tex2D_DynIdx));

    for (Uint32 i = 0; i < StaticTexArraySize; ++i)
        Macros.AddShaderMacro((std::string{"Tex2DArr_Static_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_StaticIdx + i));

    for (Uint32 i = 0; i < MutableTexArraySize; ++i)
        Macros.AddShaderMacro((std::string{"Tex2DArr_Mut_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_MutIdx + i));

    for (Uint32 i = 0; i < DynamicTexArraySize; ++i)
        Macros.AddShaderMacro((std::string{"Tex2DArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_DynIdx + i));

    // clang-format off
    ShaderResourceDesc Resources[] =
    {
        {"g_tex2DUAV",          SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2D_Static",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2D_Mut",       SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2D_Dyn",       SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2DArr_Static", SHADER_RESOURCE_TYPE_TEXTURE_UAV, StaticTexArraySize },
        {"g_RWTex2DArr_Mut",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, MutableTexArraySize},
        {"g_RWTex2DArr_Dyn",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, DynamicTexArraySize}
    };

    auto ModifyShaderCI = [pEnv](ShaderCreateInfo& ShaderCI) {
        if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // Due to bug in D3D12 WARP, we have to use SM5.0 with old compiler
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };

    auto pCS = CreateShader("ShaderResourceLayoutTest.RWTextures - CS",
                            "RWTextures.hlsl", "main",
                            SHADER_TYPE_COMPUTE, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources), ModifyShaderCI);
    ASSERT_NE(pCS, nullptr);

    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateComputePSO(pCS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    ASSERT_TRUE(pTestingSwapChain);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_tex2DUAV", Set, pTestingSwapChain->GetCurrentBackBufferUAV());

    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWTex2D_Static", Set, RefTextures.GetViewObjects(Tex2D_StaticIdx)[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Static", SetArray, RefTextures.GetViewObjects(Tex2DArr_StaticIdx), 0, StaticTexArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Mut", Set, RefTextures.GetViewObjects(Tex2D_MutIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Mut", SetArray, RefTextures.GetViewObjects(Tex2DArr_MutIdx), 0, MutableTexArraySize);

    // In Direct3D11 UAVs must not overlap!
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn", Set, RefTextures.GetViewObjects(Tex2DArr_DynIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx + 1), 0, DynamicTexArraySize);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const auto&            SCDesc = pSwapChain->GetDesc();
    DispatchComputeAttribs DispatchAttribs((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1);
    pContext->DispatchCompute(DispatchAttribs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn", Set, RefTextures.GetViewObjects(Tex2D_DynIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx), 0, DynamicTexArraySize);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

TEST_F(ShaderResourceLayoutTest, ConstantBuffers)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    float ClearColor[] = {0.875, 0.75, 0.625, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    // Prepare buffers with reference values
    ReferenceBuffers RefBuffers{
        3 + 2 + 4 + 3,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    // Buffer indices for vertex/shader bindings
    static constexpr size_t Buff_StaticIdx[] = {2, 11};
    static constexpr size_t Buff_MutIdx[]    = {0, 10};
    static constexpr size_t Buff_DynIdx[]    = {1, 9};

    static constexpr size_t BuffArr_StaticIdx[] = {10, 0};
    static constexpr size_t BuffArr_MutIdx[]    = {3, 5};
    static constexpr size_t BuffArr_DynIdx[]    = {7, 2};

    const Uint32 VSResArrId = 0;
    const Uint32 PSResArrId = DeviceInfo.Features.SeparablePrograms ? 1 : 0;
    VERIFY_EXPR(DeviceInfo.IsGLDevice() || PSResArrId != VSResArrId);

    //  Vulkan allows 15 dynamic uniform buffer bindings among all stages
    const Uint32 StaticCBArraySize  = 2;
    const Uint32 MutableCBArraySize = DeviceInfo.IsVulkanDevice() ? 1 : 4;
    const Uint32 DynamicCBArraySize = DeviceInfo.IsVulkanDevice() ? 1 : 3;

    const auto CBArraysSupported =
        DeviceInfo.Type == RENDER_DEVICE_TYPE_D3D12 ||
        DeviceInfo.Type == RENDER_DEVICE_TYPE_VULKAN ||
        DeviceInfo.Type == RENDER_DEVICE_TYPE_METAL;

    ShaderMacroHelper Macros;

    auto PrepareMacros = [&](Uint32 s) {
        Macros.Clear();

        Macros.AddShaderMacro("ARRAYS_SUPPORTED", CBArraysSupported);

        Macros.AddShaderMacro("STATIC_CB_ARRAY_SIZE", static_cast<int>(StaticCBArraySize));
        Macros.AddShaderMacro("MUTABLE_CB_ARRAY_SIZE", static_cast<int>(MutableCBArraySize));
        Macros.AddShaderMacro("DYNAMIC_CB_ARRAY_SIZE", static_cast<int>(DynamicCBArraySize));

        RefBuffers.ClearUsedValues();

        // Add macros that define reference colors
        Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(Buff_StaticIdx[s]));
        Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(Buff_MutIdx[s]));
        Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(Buff_DynIdx[s]));

        for (Uint32 i = 0; i < StaticCBArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Static_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_StaticIdx[s] + i));

        for (Uint32 i = 0; i < MutableCBArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Mut_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_MutIdx[s] + i));

        for (Uint32 i = 0; i < DynamicCBArraySize; ++i)
            Macros.AddShaderMacro((std::string{"BuffArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_DynIdx[s] + i));

        return static_cast<const ShaderMacro*>(Macros);
    };

    // clang-format off
    std::vector<ShaderResourceDesc> Resources =
    {
        ShaderResourceDesc{"UniformBuff_Stat", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Mut",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Dyn",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1}
    };

    if (CBArraysSupported)
    {
        Resources.emplace_back("UniformBuffArr_Stat", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, StaticCBArraySize);
        Resources.emplace_back("UniformBuffArr_Mut",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, MutableCBArraySize);
        Resources.emplace_back("UniformBuffArr_Dyn",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, DynamicCBArraySize);
    }
    // clang-format on

    // Even though shader array indexing is generally broken in D3D12 WARP,
    // constant buffers seem to be working fine.

    auto pVS = CreateShader("ShaderResourceLayoutTest.ConstantBuffers - VS",
                            "ConstantBuffers.hlsl",
                            "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, PrepareMacros(VSResArrId),
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    auto pPS = CreateShader("ShaderResourceLayoutTest.ConstantBuffers - PS",
                            "ConstantBuffers.hlsl",
                            "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, PrepareMacros(PSResArrId),
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);

    std::vector<ShaderResourceVariableDesc> Vars;

    auto AddVar = [&](const char* Name, SHADER_RESOURCE_VARIABLE_TYPE VarType) //
    {
        if (DeviceInfo.Features.SeparablePrograms)
        {
            // Use separate variables for each stage
            Vars.emplace_back(SHADER_TYPE_VERTEX, Name, VarType);
            Vars.emplace_back(SHADER_TYPE_PIXEL, Name, VarType);
        }
        else
        {
            // Use one shared variable
            Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, Name, VarType);
        }
    };

    // clang-format off
    AddVar("UniformBuff_Stat", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    AddVar("UniformBuff_Mut",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    AddVar("UniformBuff_Dyn",  SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

    if (CBArraysSupported)
    {
        AddVar("UniformBuffArr_Stat", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        AddVar("UniformBuffArr_Mut",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        AddVar("UniformBuffArr_Dyn",  SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars.data();
    ResourceLayout.NumVariables = static_cast<Uint32>(Vars.size());

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    auto BindResources = [&](SHADER_TYPE ShaderType) {
        const auto id = ShaderType == SHADER_TYPE_VERTEX ? VSResArrId : PSResArrId;

        SET_STATIC_VAR(pPSO, ShaderType, "UniformBuff_Stat", Set, RefBuffers.GetBuffObjects(Buff_StaticIdx[id])[0]);

        if (CBArraysSupported)
        {
            SET_STATIC_VAR(pPSO, ShaderType, "UniformBuffArr_Stat", SetArray, RefBuffers.GetBuffObjects(BuffArr_StaticIdx[id]), 0, StaticCBArraySize);
        }

        SET_SRB_VAR(pSRB, ShaderType, "UniformBuff_Mut", Set, RefBuffers.GetBuffObjects(Buff_MutIdx[id])[0]);
        SET_SRB_VAR(pSRB, ShaderType, "UniformBuff_Dyn", Set, RefBuffers.GetBuffObjects(0)[0]); // Will rebind for the second draw

        if (CBArraysSupported)
        {
            SET_SRB_VAR(pSRB, ShaderType, "UniformBuffArr_Mut", SetArray, RefBuffers.GetBuffObjects(BuffArr_MutIdx[id]), 0, MutableCBArraySize);
            SET_SRB_VAR(pSRB, ShaderType, "UniformBuffArr_Dyn", SetArray, RefBuffers.GetBuffObjects(0), 0, DynamicCBArraySize); // Will rebind for the second draw
        }
    };
    BindResources(SHADER_TYPE_VERTEX);
    BindResources(SHADER_TYPE_PIXEL);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuff_Dyn", Set, RefBuffers.GetBuffObjects(Buff_DynIdx[VSResArrId])[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuff_Dyn", Set, RefBuffers.GetBuffObjects(Buff_DynIdx[PSResArrId])[0]);
    if (CBArraysSupported)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn", SetArray, RefBuffers.GetBuffObjects(BuffArr_DynIdx[VSResArrId]), 0, DynamicCBArraySize);
        SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn", SetArray, RefBuffers.GetBuffObjects(BuffArr_DynIdx[PSResArrId]), 0, DynamicCBArraySize);
    }
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

TEST_F(ShaderResourceLayoutTest, Samplers)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceInfo().IsGLDevice())
    {
        GTEST_SKIP() << "OpenGL does not support separate samplers";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.5, 0.25, 0.875, 0.5};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    static constexpr Uint32 StaticSamArraySize  = 2;
    static constexpr Uint32 MutableSamArraySize = 4;
    static constexpr Uint32 DynamicSamArraySize = 3;
    ShaderMacroHelper       Macros;
    Macros.AddShaderMacro("STATIC_SAM_ARRAY_SIZE", static_cast<int>(StaticSamArraySize));
    Macros.AddShaderMacro("MUTABLE_SAM_ARRAY_SIZE", static_cast<int>(MutableSamArraySize));
    Macros.AddShaderMacro("DYNAMIC_SAM_ARRAY_SIZE", static_cast<int>(DynamicSamArraySize));

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    // clang-format off
    ShaderResourceDesc Resources[] =
    {
        {"g_Sam_Static",      SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_Sam_Mut",         SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_Sam_Dyn",         SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_SamArr_Static",   SHADER_RESOURCE_TYPE_SAMPLER,     StaticSamArraySize},
        {"g_SamArr_Mut",      SHADER_RESOURCE_TYPE_SAMPLER,     MutableSamArraySize},
        {"g_SamArr_Dyn",      SHADER_RESOURCE_TYPE_SAMPLER,     DynamicSamArraySize},
        {"g_Tex2D",           SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
    };
    // clang-format on
    auto pVS = CreateShader("ShaderResourceLayoutTest.Samplers - VS", "Samplers.hlsl", "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources));
    auto pPS = CreateShader("ShaderResourceLayoutTest.Samplers - PS", "Samplers.hlsl", "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources));
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_VERTEX,  "g_Tex2D",     SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL,   "g_Tex2D",     SHADER_RESOURCE_VARIABLE_TYPE_STATIC},

        {SHADER_TYPE_VERTEX, "g_Sam_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL,  "g_Sam_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Sam_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL,  "g_Sam_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "g_Sam_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL,  "g_Sam_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_VERTEX, "g_SamArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL,  "g_SamArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_SamArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL,  "g_SamArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "g_SamArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL,  "g_SamArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto MaxSamplers = std::max(std::max(StaticSamArraySize, MutableSamArraySize), DynamicSamArraySize);

    std::vector<RefCntAutoPtr<ISampler>> pSamplers(MaxSamplers);
    std::vector<IDeviceObject*>          pSamObjs(MaxSamplers);

    for (Uint32 i = 0; i < MaxSamplers; ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &pSamplers[i]);
        ASSERT_NE(pSamplers[i], nullptr);
        pSamObjs[i] = pSamplers[i];
    }

    constexpr Uint32    TexWidth  = 256;
    constexpr Uint32    TexHeight = 256;
    std::vector<Uint32> TexData(TexWidth * TexHeight, 0x00FF00FFu);

    auto  pTex2D    = pEnv->CreateTexture("ShaderResourceLayoutTest: test RTV", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, TexWidth, TexHeight, TexData.data());
    auto* pTex2DSRV = pTex2D->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Tex2D", Set, pTex2DSRV);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Tex2D", Set, pTex2DSRV);


    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Sam_Static", Set, pSamObjs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_SamArr_Static", SetArray, pSamObjs.data(), 0, StaticSamArraySize);

    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Sam_Static", Set, pSamObjs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_SamArr_Static", SetArray, pSamObjs.data(), 0, StaticSamArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Mut", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Dyn", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Mut", SetArray, pSamObjs.data(), 0, MutableSamArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 0, DynamicSamArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Mut", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Dyn", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Mut", SetArray, pSamObjs.data(), 0, MutableSamArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 0, DynamicSamArraySize);

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Dyn", Set, pSamObjs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 1, DynamicSamArraySize - 1);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Dyn", Set, pSamObjs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 1, DynamicSamArraySize - 1);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}


TEST_F(ShaderResourceLayoutTest, MergedVarStages)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    const auto& DeviceProps = pDevice->GetDeviceInfo();

    float ClearColor[] = {0.125, 0.875, 0.25, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    // Prepare buffers and textures with reference values
    ReferenceBuffers RefBuffers{
        3,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };
    ReferenceTextures RefTextures{
        3,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc{}, &pSampler);
    for (Uint32 i = 0; i < RefTextures.GetTextureCount(); ++i)
        RefTextures.GetView(i)->SetSampler(pSampler);

    ShaderMacroHelper Macros;

    // Add macros that define reference colors
    Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(0));
    Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(1));
    Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(2));

    Macros.AddShaderMacro("Tex2D_Static_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Tex2D_Mut_Ref", RefTextures.GetColor(1));
    Macros.AddShaderMacro("Tex2D_Dyn_Ref", RefTextures.GetColor(2));


    // clang-format off
    std::vector<ShaderResourceDesc> Resources =
    {
        ShaderResourceDesc{"UniformBuff_Stat", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Mut",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Dyn",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"g_Tex2D_Static", SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Mut",    SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Dyn",    SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1}
    };
    if (!DeviceProps.IsGLDevice())
    {
        Resources.emplace_back("g_Tex2D_Static_sampler", SHADER_RESOURCE_TYPE_SAMPLER, 1);
        Resources.emplace_back("g_Tex2D_Mut_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, 1);
        Resources.emplace_back("g_Tex2D_Dyn_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, 1);
    }
    // clang-format on

    auto ModifyShaderCI = [](ShaderCreateInfo& ShaderCI) {
        ShaderCI.UseCombinedTextureSamplers = true;
    };
    auto pVS = CreateShader("ShaderResourceLayoutTest.MergedVarStages - VS",
                            "MergedVarStages.hlsl",
                            "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()), ModifyShaderCI);
    auto pPS = CreateShader("ShaderResourceLayoutTest.MergedVarStages - PS",
                            "MergedVarStages.hlsl",
                            "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()), ModifyShaderCI);
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);

    // clang-format off
    const ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Stat", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Mut",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Dyn",  SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "UniformBuff_Stat", Set, RefBuffers.GetBuffer(0));
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuff_Mut", Set, RefBuffers.GetBuffer(1));
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuff_Dyn", Set, RefBuffers.GetBuffer(2));

    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Tex2D_Static", Set, RefTextures.GetView(0));
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Mut", Set, RefTextures.GetView(1));

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, RefTextures.GetView(0));
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, nullptr); // Test resetting combined texture to null
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, RefTextures.GetView(2));

    pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

} // namespace
