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
#include <vector>

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"
#include "ShaderMacroHelper.hpp"
#include "GraphicsAccessories.hpp"
#include "ResourceLayoutTestCommon.hpp"

#if VULKAN_SUPPORTED
#    include "Vulkan/TestingEnvironmentVk.hpp"
#endif

#include "gtest/gtest.h"

#include "InlineShaders/DrawCommandTestHLSL.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace Diligent
{

class PipelineResourceSignatureTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/PipelineResourceSignature", &pShaderSourceFactory);
    }

    static void TearDownTestSuite()
    {
        pShaderSourceFactory.Release();
        GPUTestingEnvironment::GetInstance()->Reset();
    }

    static RefCntAutoPtr<IPipelineState> CreateGraphicsPSO(IShader*                                                         pVS,
                                                           IShader*                                                         pPS,
                                                           std::initializer_list<RefCntAutoPtr<IPipelineResourceSignature>> pSignatures)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        PSOCreateInfo.PSODesc.Name = "Resource signature test";

        std::vector<IPipelineResourceSignature*> Signatures;
        for (auto& pPRS : pSignatures)
            Signatures.push_back(pPRS.RawPtr<IPipelineResourceSignature>());

        PSOCreateInfo.ppResourceSignatures    = Signatures.data();
        PSOCreateInfo.ResourceSignaturesCount = static_cast<Uint32>(Signatures.size());

        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.NumRenderTargets  = 1;
        GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
        GraphicsPipeline.DSVFormat         = TEX_FORMAT_UNKNOWN;

        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;

        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        return pPSO;
    }

    template <typename ModifyCIHandlerType>
    static RefCntAutoPtr<IShader> CreateShaderFromFile(SHADER_TYPE         ShaderType,
                                                       const char*         File,
                                                       const char*         EntryPoint,
                                                       const char*         Name,
                                                       const ShaderMacro*  Macros,
                                                       ModifyCIHandlerType ModifyCIHandler)
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.FilePath                   = File;
        ShaderCI.Macros                     = Macros;
        ShaderCI.Desc.Name                  = Name;
        ShaderCI.EntryPoint                 = EntryPoint;
        ShaderCI.Desc.ShaderType            = ShaderType;
        ShaderCI.UseCombinedTextureSamplers = false;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        if (pDevice->GetDeviceInfo().IsGLDevice())
            ShaderCI.UseCombinedTextureSamplers = true;

        ModifyCIHandler(ShaderCI);
        RefCntAutoPtr<IShader> pShader;
        pDevice->CreateShader(ShaderCI, &pShader);
        return pShader;
    }

    static RefCntAutoPtr<IShader> CreateShaderFromFile(SHADER_TYPE        ShaderType,
                                                       const char*        File,
                                                       const char*        EntryPoint,
                                                       const char*        Name,
                                                       const ShaderMacro* Macros = nullptr)
    {
        return CreateShaderFromFile(ShaderType, File, EntryPoint, Name, Macros, [&](ShaderCreateInfo& ShaderCI) {});
    }

    static RefCntAutoPtr<IShader> CreateShaderFromFileDXC(SHADER_TYPE        ShaderType,
                                                          const char*        File,
                                                          const char*        EntryPoint,
                                                          const char*        Name,
                                                          const ShaderMacro* Macros = nullptr)
    {
        return CreateShaderFromFile(ShaderType, File, EntryPoint, Name, Macros, [&](ShaderCreateInfo& ShaderCI) { ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC; });
    }

    static void TestFormattedOrStructuredBuffer(BUFFER_MODE BufferMode);
    static void TestCombinedImageSamplers(SHADER_SOURCE_LANGUAGE ShaderLang, bool UseEmulatedSamplers = false);

    enum SRB_COMPAT_MODE
    {
        SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END,
        SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_MIDDLE,
    };
    static void TestSRBCompatibility(SRB_COMPAT_MODE Mode);

    static RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
};

RefCntAutoPtr<IShaderSourceInputStreamFactory> PipelineResourceSignatureTest::pShaderSourceFactory;


#define SET_STATIC_VAR(PRS, ShaderFlags, VarName, SetMethod, ...)                                \
    do                                                                                           \
    {                                                                                            \
        auto pStaticVar = PRS->GetStaticVariableByName(ShaderFlags, VarName);                    \
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

TEST_F(PipelineResourceSignatureTest, VariableTypes)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto*       pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.125, 0.25, 0.375, 0.5};
    RenderDrawCommandReference(pSwapChain, ClearColor);

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
    static constexpr size_t Tex2D_StaticIdx = 2;
    static constexpr size_t Tex2D_MutIdx    = 0;
    static constexpr size_t Tex2D_DynIdx    = 1;

    static constexpr size_t Tex2DArr_StaticIdx = 7;
    static constexpr size_t Tex2DArr_MutIdx    = 3;
    static constexpr size_t Tex2DArr_DynIdx    = 9;

    ShaderMacroHelper Macros;

    Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
    Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
    Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

    RefTextures.ClearUsedValues();

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

    auto ModifyShaderCI = [pEnv](ShaderCreateInfo& ShaderCI) {
        if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // As of Windows version 2004 (build 19041), there is a bug in D3D12 WARP rasterizer:
            // Shader resource array indexing always references array element 0 when shaders are compiled
            // with shader model 5.1.
            // Use SM5.0 with old compiler as a workaround.
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };
    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "shaders/ShaderResourceLayout/Textures.hlsl", "VSMain", "PRS variable types test: VS", Macros, ModifyShaderCI);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "shaders/ShaderResourceLayout/Textures.hlsl", "PSMain", "PRS variable types test: PS", Macros, ModifyShaderCI);
    ASSERT_TRUE(pVS && pPS);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Variable types test";

    constexpr auto SHADER_TYPE_VS_PS = SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL;
    // clang-format off
    PipelineResourceDesc Resources[]
    {
        {SHADER_TYPE_VS_PS, "g_Tex2D_Static", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VS_PS, "g_Tex2D_Mut",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VS_PS, "g_Tex2D_Dyn",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VS_PS, "g_Tex2DArr_Static", StaticTexArraySize,  SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VS_PS, "g_Tex2DArr_Mut",    MutableTexArraySize, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VS_PS, "g_Tex2DArr_Dyn",    DynamicTexArraySize, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VS_PS, "g_Sampler",         1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    };
    // clang-format on
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_TRUE(pPRS);

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
    ASSERT_TRUE(pPSO);

    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Tex2D_Static", Set, RefTextures.GetViewObjects(Tex2D_StaticIdx)[0]);
    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Tex2DArr_Static", SetArray, RefTextures.GetViewObjects(Tex2DArr_StaticIdx), 0, StaticTexArraySize);

    if (!pDevice->GetDeviceInfo().IsGLDevice())
    {
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc{}, &pSampler);
        SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Sampler", Set, pSampler);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPRS->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Mut", Set, RefTextures.GetViewObjects(Tex2D_MutIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut", SetArray, RefTextures.GetViewObjects(Tex2DArr_MutIdx), 0, MutableTexArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, RefTextures.GetViewObjects(Tex2D_DynIdx)[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Dyn", SetArray, RefTextures.GetViewObjects(Tex2DArr_DynIdx), 0, DynamicTexArraySize);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}


TEST_F(PipelineResourceSignatureTest, MultiSignatures)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto*       pContext = pEnv->GetDeviceContext();

    if (!pDevice->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP();
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.875, 0.125, 0.5, 0.625};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        8,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_1_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Tex2D_2_Ref", RefTextures.GetColor(1));
    Macros.AddShaderMacro("Tex2D_3_Ref", RefTextures.GetColor(2));
    Macros.AddShaderMacro("Tex2D_4_Ref", RefTextures.GetColor(3));
    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "MultiSignatures.hlsl", "VSMain", "PRS multi signatures test: VS", Macros);

    Macros.UpdateMacro("Tex2D_1_Ref", RefTextures.GetColor(4));
    Macros.UpdateMacro("Tex2D_2_Ref", RefTextures.GetColor(5));
    Macros.UpdateMacro("Tex2D_3_Ref", RefTextures.GetColor(6));
    Macros.UpdateMacro("Tex2D_4_Ref", RefTextures.GetColor(7));
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "MultiSignatures.hlsl", "PSMain", "PRS multi signatures test: PS", Macros);
    ASSERT_TRUE(pVS && pPS);

    PipelineResourceSignatureDesc PRSDesc;

    RefCntAutoPtr<IPipelineResourceSignature> pPRS[3];
    RefCntAutoPtr<IShaderResourceBinding>     pSRB[3];
    std::vector<PipelineResourceDesc>         Resources[3];
    // clang-format off
    Resources[0].emplace_back(SHADER_TYPE_VERTEX, "g_Tex2D_1", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    Resources[0].emplace_back(SHADER_TYPE_PIXEL,  "g_Tex2D_2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    Resources[0].emplace_back(SHADER_TYPE_PIXEL,  "g_Tex2D_3", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

    Resources[1].emplace_back(SHADER_TYPE_PIXEL,  "g_Tex2D_1", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    Resources[1].emplace_back(SHADER_TYPE_VERTEX, "g_Tex2D_2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    Resources[1].emplace_back(SHADER_TYPE_VERTEX, "g_Tex2D_3", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

    Resources[2].emplace_back(SHADER_TYPE_PIXEL,  "g_Tex2D_4", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
    Resources[2].emplace_back(SHADER_TYPE_VERTEX, "g_Tex2D_4", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    Resources[2].emplace_back(SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "g_Sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    // clang-format on

    for (Uint32 i = 0; i < _countof(pPRS); ++i)
    {
        std::string PRSName  = "Multi signatures " + std::to_string(i);
        PRSDesc.Name         = PRSName.c_str();
        PRSDesc.BindingIndex = static_cast<Uint8>(i);

        PRSDesc.Resources    = Resources[i].data();
        PRSDesc.NumResources = static_cast<Uint32>(Resources[i].size());

        pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS[i]);
        ASSERT_TRUE(pPRS[i]);
    }

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS[0], pPRS[1], pPRS[2]});
    ASSERT_TRUE(pPSO);

    SET_STATIC_VAR(pPRS[0], SHADER_TYPE_VERTEX, "g_Tex2D_1", Set, RefTextures.GetView(0));
    SET_STATIC_VAR(pPRS[1], SHADER_TYPE_VERTEX, "g_Tex2D_3", Set, RefTextures.GetView(2));

    if (!pDevice->GetDeviceInfo().IsGLDevice())
    {
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc{}, &pSampler);
        SET_STATIC_VAR(pPRS[2], SHADER_TYPE_PIXEL, "g_Sampler", Set, pSampler);
    }

    for (Uint32 i = 0; i < _countof(pPRS); ++i)
    {
        pPRS[i]->CreateShaderResourceBinding(&pSRB[i], true);
        ASSERT_NE(pSRB[i], nullptr);
    }

    SET_SRB_VAR(pSRB[0], SHADER_TYPE_PIXEL, "g_Tex2D_2", Set, RefTextures.GetView(5));
    SET_SRB_VAR(pSRB[1], SHADER_TYPE_PIXEL, "g_Tex2D_1", Set, RefTextures.GetView(4));
    SET_SRB_VAR(pSRB[2], SHADER_TYPE_PIXEL, "g_Tex2D_4", Set, RefTextures.GetView(7));

    SET_SRB_VAR(pSRB[0], SHADER_TYPE_PIXEL, "g_Tex2D_3", Set, RefTextures.GetView(6));
    SET_SRB_VAR(pSRB[1], SHADER_TYPE_VERTEX, "g_Tex2D_2", Set, RefTextures.GetView(1));
    SET_SRB_VAR(pSRB[2], SHADER_TYPE_VERTEX, "g_Tex2D_4", Set, RefTextures.GetView(3));

    for (Uint32 i = 0; i < _countof(pSRB); ++i)
    {
        pContext->CommitShaderResources(pSRB[i], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}


TEST_F(PipelineResourceSignatureTest, SingleVarType)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto*       pContext = pEnv->GetDeviceContext();

    if (!pDevice->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP();
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.375, 0.875, 0.125, 0.0625};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        2,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };
    ReferenceBuffers RefBuffers{
        2,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_1_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Tex2D_2_Ref", RefTextures.GetColor(1));
    Macros.AddShaderMacro("CB_1_Ref", RefBuffers.GetValue(0));
    Macros.AddShaderMacro("CB_2_Ref", RefBuffers.GetValue(1));

    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "SingleVarType.hlsl", "VSMain", "PRS single var type test: VS", Macros);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "SingleVarType.hlsl", "PSMain", "PRS single var type test: PS", Macros);
    ASSERT_TRUE(pVS && pPS);

    for (Uint32 var_type = 0; var_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++var_type)
    {
        const auto VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(var_type);

        const PipelineResourceDesc Resources[] = //
            {
                {SHADER_TYPE_ALL_GRAPHICS, "g_Tex2D_1", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "g_Tex2D_2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "ConstBuff_1", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "ConstBuff_2", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType}, //
            };

        std::string Name = std::string{"PRS test - "} + GetShaderVariableTypeLiteralName(VarType) + " vars";

        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name         = Name.c_str();
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);

        ImmutableSamplerDesc ImmutableSamplers[] = //
            {
                {SHADER_TYPE_ALL_GRAPHICS, "g_Sampler", SamplerDesc{FILTER_TYPE_POINT, FILTER_TYPE_POINT, FILTER_TYPE_POINT}} //
            };
        ImmutableSamplers[0].Desc.Name = "Default sampler";

        PRSDesc.ImmutableSamplers    = ImmutableSamplers;
        PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

        RefCntAutoPtr<IPipelineResourceSignature> pPRS;
        pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
        ASSERT_TRUE(pPRS);

        EXPECT_EQ(pPRS->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Sampler"), nullptr);
        EXPECT_EQ(pPRS->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Sampler"), nullptr);

        if (VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        {
            SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Tex2D_1", Set, RefTextures.GetView(0));
            SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Tex2D_2", Set, RefTextures.GetView(1));
            SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "ConstBuff_1", Set, RefBuffers.GetBuffer(0));
            SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "ConstBuff_2", Set, RefBuffers.GetBuffer(1));
        }

        RefCntAutoPtr<IShaderResourceBinding> pSRB;

        auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
        ASSERT_TRUE(pPSO);

        pPRS->CreateShaderResourceBinding(&pSRB, true);
        ASSERT_NE(pSRB, nullptr);

        EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Sampler"), nullptr);
        EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sampler"), nullptr);

        if (VarType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        {
            SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_1", Set, RefTextures.GetView(0));
            SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_2", Set, RefTextures.GetView(1));
            SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "ConstBuff_1", Set, RefBuffers.GetBuffer(0));
            SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "ConstBuff_2", Set, RefBuffers.GetBuffer(1));
        }

        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(pPSO);

        DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);

        pSwapChain->Present();
        std::cout << TestingEnvironment::GetCurrentTestStatusString() << ' '
                  << GetShaderVariableTypeLiteralName(VarType) << " vars" << std::endl;
    }
}

TEST_F(PipelineResourceSignatureTest, ImmutableSamplers)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto*       pContext = pEnv->GetDeviceContext();

    if (!pDevice->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP();
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.5, 0.375, 0.25, 0.75};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        3,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };
    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_1_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Tex2D_2_Ref", RefTextures.GetColor(1));
    Macros.AddShaderMacro("Tex2D_3_Ref", RefTextures.GetColor(2));

    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "ImmutableSamplers.hlsl", "VSMain", "PRS static samplers test: VS", Macros);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "ImmutableSamplers.hlsl", "PSMain", "PRS static samplers test: PS", Macros);
    ASSERT_TRUE(pVS && pPS);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Variable types test";

    constexpr auto SHADER_TYPE_VS_PS = SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL;
    // clang-format off
    PipelineResourceDesc Resources[]
    {
        {SHADER_TYPE_VS_PS, "g_Tex2D_Static", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VS_PS, "g_Tex2D_Mut",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VS_PS, "g_Tex2D_Dyn",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    ImmutableSamplerDesc ImmutableSamplers[] = //
        {
            {SHADER_TYPE_VERTEX, "g_Sampler", SamplerDesc{}},
            {SHADER_TYPE_PIXEL, "g_Sampler", SamplerDesc{}} //
        };
    PRSDesc.ImmutableSamplers          = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers       = _countof(ImmutableSamplers);
    PRSDesc.UseCombinedTextureSamplers = false;

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_TRUE(pPRS);

    EXPECT_EQ(pPRS->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Sampler"), nullptr);
    EXPECT_EQ(pPRS->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Sampler"), nullptr);

    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Tex2D_Static", Set, RefTextures.GetView(0));

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPRS->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Sampler"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sampler"), nullptr);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Mut", Set, RefTextures.GetView(1));
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, RefTextures.GetView(2));

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
    ASSERT_TRUE(pPSO);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}


TEST_F(PipelineResourceSignatureTest, ImmutableSamplers2)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.625, 0.25, 0.375, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        1,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };
    ReferenceBuffers RefBuffers{
        1,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    RefCntAutoPtr<IPipelineResourceSignature> pSignature0;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "Constants", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Name         = "ImmutableSamplers2 - PRS1";
        Desc.Resources    = Resources;
        Desc.NumResources = _countof(Resources);
        Desc.BindingIndex = 0;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature0);
        ASSERT_NE(pSignature0, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignature2;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
            };

        SamplerDesc SamLinearWrapDesc{
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
        ImmutableSamplerDesc ImmutableSamplers[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SamLinearWrapDesc} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Name                       = "ImmutableSamplers2 - PRS2";
        Desc.Resources                  = Resources;
        Desc.NumResources               = _countof(Resources);
        Desc.ImmutableSamplers          = ImmutableSamplers;
        Desc.NumImmutableSamplers       = _countof(ImmutableSamplers);
        Desc.UseCombinedTextureSamplers = true;
        Desc.CombinedSamplerSuffix      = "_sampler";
        Desc.BindingIndex               = 2;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature2);
        ASSERT_NE(pSignature2, nullptr);

        EXPECT_EQ(pSignature2->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Texture"), nullptr);
        EXPECT_EQ(pSignature2->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Texture_sampler"), nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignature3;
    {
        PipelineResourceSignatureDesc Desc;
        Desc.Name         = "ImmutableSamplers2 - PRS3";
        Desc.BindingIndex = 3;
        pDevice->CreatePipelineResourceSignature(Desc, &pSignature3);
        ASSERT_NE(pSignature3, nullptr);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "ImmutableSamplers2 PSO";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Buff_Ref", RefBuffers.GetValue(0));

    auto SetUseCombinedSamplers = [](ShaderCreateInfo& ShaderCI) {
        ShaderCI.UseCombinedTextureSamplers = true;
    };
    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "ImmutableSamplers2.hlsl", "VSMain", "PRS static samplers test: VS", Macros, SetUseCombinedSamplers);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "ImmutableSamplers2.hlsl", "PSMain", "PRS static samplers test: PS", Macros, SetUseCombinedSamplers);
    ASSERT_TRUE(pVS && pPS);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    IPipelineResourceSignature* Signatures[] = {pSignature0, pSignature2, pSignature3};

    PSOCreateInfo.ppResourceSignatures    = Signatures;
    PSOCreateInfo.ResourceSignaturesCount = _countof(Signatures);

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    ASSERT_EQ(pPSO->GetResourceSignatureCount(), 4u);
    ASSERT_EQ(pPSO->GetResourceSignature(0), pSignature0);
    ASSERT_EQ(pPSO->GetResourceSignature(1), nullptr);
    ASSERT_EQ(pPSO->GetResourceSignature(2), pSignature2);
    ASSERT_EQ(pPSO->GetResourceSignature(3), pSignature3);

    RefCntAutoPtr<IShaderResourceBinding> pSRB0;
    pSignature0->CreateShaderResourceBinding(&pSRB0, true);
    ASSERT_NE(pSRB0, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB2;
    pSignature2->CreateShaderResourceBinding(&pSRB2, true);
    ASSERT_NE(pSRB2, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB3;
    pSignature3->CreateShaderResourceBinding(&pSRB3, true);
    ASSERT_NE(pSRB3, nullptr);

    pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(RefBuffers.GetBuffer(0));
    pSRB2->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(RefTextures.GetView(0));

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->CommitShaderResources(pSRB0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->CommitShaderResources(pSRB2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->CommitShaderResources(pSRB3, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    pSwapChain->Present();
}


void PipelineResourceSignatureTest::TestSRBCompatibility(const SRB_COMPAT_MODE Mode)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.25, 0.625, 0.375, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        2,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };
    ReferenceBuffers RefBuffers{
        1,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    RefCntAutoPtr<IPipelineResourceSignature> pSignature0;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "Constants", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Resources    = Resources;
        Desc.NumResources = _countof(Resources);
        Desc.BindingIndex = 0;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature0);
        ASSERT_NE(pSignature0, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pEmptySignature;
    {
        PipelineResourceSignatureDesc Desc;
        Desc.BindingIndex = 1;

        pDevice->CreatePipelineResourceSignature(Desc, &pEmptySignature);
        ASSERT_NE(pEmptySignature, nullptr);
        EXPECT_TRUE(pEmptySignature->IsCompatibleWith(nullptr));
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignature2;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        SamplerDesc SamLinearWrapDesc{
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
        ImmutableSamplerDesc ImmutableSamplers[] = {{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SamLinearWrapDesc}};

        PipelineResourceSignatureDesc Desc;
        Desc.Resources                  = Resources;
        Desc.NumResources               = _countof(Resources);
        Desc.ImmutableSamplers          = ImmutableSamplers;
        Desc.NumImmutableSamplers       = _countof(ImmutableSamplers);
        Desc.UseCombinedTextureSamplers = true;
        Desc.CombinedSamplerSuffix      = "_sampler";
        Desc.BindingIndex               = 2;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature2);
        ASSERT_NE(pSignature2, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignature3;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture2_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        SamplerDesc SamLinearWrapDesc{
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
        ImmutableSamplerDesc ImmutableSamplers[] = {{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture2", SamLinearWrapDesc}};

        PipelineResourceSignatureDesc Desc;
        Desc.Resources                  = Resources;
        Desc.NumResources               = _countof(Resources);
        Desc.ImmutableSamplers          = ImmutableSamplers;
        Desc.NumImmutableSamplers       = _countof(ImmutableSamplers);
        Desc.UseCombinedTextureSamplers = true;
        Desc.CombinedSamplerSuffix      = "_sampler";
        Desc.BindingIndex               = (Mode == SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END ? 3 : 1);

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature3);
        ASSERT_NE(pSignature3, nullptr);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "PRS test";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Buff_Ref", RefBuffers.GetValue(0));

    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, "SRBCompatibility1.hlsl", "VSMain", "SRBCompatibility1 VS", Macros);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, "SRBCompatibility1.hlsl", "PSMain", "SRBCompatibility1 PS", Macros);
    ASSERT_TRUE(pVS && pPS);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    IPipelineResourceSignature* Signatures1[] = {pSignature0, pSignature2};

    PSOCreateInfo.ppResourceSignatures    = Signatures1;
    PSOCreateInfo.ResourceSignaturesCount = _countof(Signatures1);

    RefCntAutoPtr<IPipelineState> pPSO0x2;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO0x2);
    ASSERT_NE(pPSO0x2, nullptr);

    EXPECT_EQ(pPSO0x2->GetResourceSignatureCount(), 3u);
    EXPECT_EQ(pPSO0x2->GetResourceSignature(0), pSignature0);
    EXPECT_EQ(pPSO0x2->GetResourceSignature(1), nullptr);
    EXPECT_EQ(pPSO0x2->GetResourceSignature(2), pSignature2);

    Macros.AddShaderMacro("Tex2D_2_Ref", RefTextures.GetColor(1));

    auto pVS2 = CreateShaderFromFile(SHADER_TYPE_VERTEX, "SRBCompatibility2.hlsl", "VSMain", "SRBCompatibility2 VS", Macros);
    auto pPS2 = CreateShaderFromFile(SHADER_TYPE_PIXEL, "SRBCompatibility2.hlsl", "PSMain", "SRBCompatibility2 PS", Macros);
    ASSERT_TRUE(pVS2 && pPS2);

    PSOCreateInfo.pVS = pVS2;
    PSOCreateInfo.pPS = pPS2;

    RefCntAutoPtr<IPipelineState> pPSO0x23, pPSO032;
    if (Mode == SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END)
    {
        IPipelineResourceSignature* Signatures2[] = {pSignature0, pEmptySignature, pSignature2, pSignature3};

        PSOCreateInfo.ppResourceSignatures    = Signatures2;
        PSOCreateInfo.ResourceSignaturesCount = _countof(Signatures2);

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO0x23);
        ASSERT_NE(pPSO0x23, nullptr);

        ASSERT_EQ(pPSO0x23->GetResourceSignatureCount(), 4u);
        ASSERT_EQ(pPSO0x23->GetResourceSignature(0), pSignature0);
        ASSERT_EQ(pPSO0x23->GetResourceSignature(1), pEmptySignature);
        ASSERT_EQ(pPSO0x23->GetResourceSignature(2), pSignature2);
        ASSERT_EQ(pPSO0x23->GetResourceSignature(3), pSignature3);
    }
    else
    {
        IPipelineResourceSignature* Signatures2[] = {pSignature0, pSignature2, pSignature3};

        PSOCreateInfo.ppResourceSignatures    = Signatures2;
        PSOCreateInfo.ResourceSignaturesCount = _countof(Signatures2);

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO032);
        ASSERT_NE(pPSO032, nullptr);

        ASSERT_EQ(pPSO032->GetResourceSignatureCount(), 3u);
        ASSERT_EQ(pPSO032->GetResourceSignature(0), pSignature0);
        ASSERT_EQ(pPSO032->GetResourceSignature(1), pSignature3);
        ASSERT_EQ(pPSO032->GetResourceSignature(2), pSignature2);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB0;
    pSignature0->CreateShaderResourceBinding(&pSRB0, true);
    ASSERT_NE(pSRB0, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB2;
    pSignature2->CreateShaderResourceBinding(&pSRB2, true);
    ASSERT_NE(pSRB2, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB3;
    pSignature3->CreateShaderResourceBinding(&pSRB3, true);
    ASSERT_NE(pSRB3, nullptr);

    pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(RefBuffers.GetBuffer(0));
    pSRB2->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(RefTextures.GetView(0));
    pSRB3->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture2")->Set(RefTextures.GetView(1));

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // draw 1
        pContext->CommitShaderResources(pSRB0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 0
        pContext->CommitShaderResources(pSRB2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 2

        pContext->SetPipelineState(pPSO0x2);

        DrawAttribs drawAttrs(3, DRAW_FLAG_VERIFY_ALL);
        pContext->Draw(drawAttrs);
    }
    {
        // draw 2
        if (Mode == SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END)
        {
            pContext->SetPipelineState(pPSO0x23);
            // reuse pSRB0, pSRB2
            pContext->CommitShaderResources(pSRB3, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 3
        }
        else
        {
            pContext->SetPipelineState(pPSO032);
            // reuse pSRB0
            pContext->CommitShaderResources(pSRB3, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 1
            pContext->CommitShaderResources(pSRB2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 2
        }

        DrawAttribs drawAttrs(3, DRAW_FLAG_VERIFY_ALL);
        pContext->Draw(drawAttrs);
    }
    {
        // draw 3
        pContext->SetPipelineState(pPSO0x2);
        if (Mode == SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END)
        {
            // reuse pSRB0, pSRB2
        }
        else
        {
            // reuse pSRB0
            pContext->CommitShaderResources(pSRB2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 2
        }

        DrawAttribs drawAttrs(3, DRAW_FLAG_VERIFY_ALL);
        pContext->Draw(drawAttrs);
    }
    pSwapChain->Present();
}

TEST_F(PipelineResourceSignatureTest, SRBCompatibility1)
{
    TestSRBCompatibility(SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_END);
}

TEST_F(PipelineResourceSignatureTest, SRBCompatibility2)
{
    TestSRBCompatibility(SRB_COMPAT_MODE_INSERT_SIGNATURE_TO_MIDDLE);
}

TEST_F(PipelineResourceSignatureTest, GraphicsAndMeshShader)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    if (!pDevice->GetDeviceInfo().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();

    float ClearColor[] = {0.25, 0.625, 0.375, 0.125};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ReferenceTextures RefTextures{
        1,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };
    ReferenceBuffers RefBuffers{
        2,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    RefCntAutoPtr<IPipelineResourceSignature> pSignaturePS;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                {SHADER_TYPE_PIXEL, "g_Texture_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        SamplerDesc SamLinearWrapDesc{
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
        ImmutableSamplerDesc ImmutableSamplers[] = {{SHADER_TYPE_PIXEL, "g_Texture", SamLinearWrapDesc}};

        PipelineResourceSignatureDesc Desc;
        Desc.Resources                  = Resources;
        Desc.NumResources               = _countof(Resources);
        Desc.ImmutableSamplers          = ImmutableSamplers;
        Desc.NumImmutableSamplers       = _countof(ImmutableSamplers);
        Desc.UseCombinedTextureSamplers = true;
        Desc.CombinedSamplerSuffix      = "_sampler";
        Desc.BindingIndex               = 0;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignaturePS);
        ASSERT_NE(pSignaturePS, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignatureVS;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_VERTEX, "Constants", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Resources    = Resources;
        Desc.NumResources = _countof(Resources);
        Desc.BindingIndex = 1;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignatureVS);
        ASSERT_NE(pSignatureVS, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignatureMS;
    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_MESH, "Constants", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Resources    = Resources;
        Desc.NumResources = _countof(Resources);
        Desc.BindingIndex = 1;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignatureMS);
        ASSERT_NE(pSignatureMS, nullptr);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Graphics PSO";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("Tex2D_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Buff_Ref", RefBuffers.GetValue(0));

    auto pVS = CreateShaderFromFileDXC(SHADER_TYPE_VERTEX, "GraphicsAndMeshShader.hlsl", "VSMain", "GraphicsAndMeshShader VS", Macros);
    auto pPS = CreateShaderFromFileDXC(SHADER_TYPE_PIXEL, "GraphicsAndMeshShader.hlsl", "PSMain", "GraphicsAndMeshShader PS", Macros);
    ASSERT_TRUE(pVS && pPS);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    IPipelineResourceSignature* GraphicsSignatures[] = {pSignatureVS, pSignaturePS};

    PSOCreateInfo.ppResourceSignatures    = GraphicsSignatures;
    PSOCreateInfo.ResourceSignaturesCount = _countof(GraphicsSignatures);

    RefCntAutoPtr<IPipelineState> pGraphicsPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pGraphicsPSO);
    ASSERT_NE(pGraphicsPSO, nullptr);

    ASSERT_EQ(pGraphicsPSO->GetResourceSignatureCount(), 2u);
    ASSERT_EQ(pGraphicsPSO->GetResourceSignature(0), pSignaturePS);
    ASSERT_EQ(pGraphicsPSO->GetResourceSignature(1), pSignatureVS);


    PSODesc.Name = "Mesh PSO";

    PSODesc.PipelineType               = PIPELINE_TYPE_MESH;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED; // unused

    Macros.UpdateMacro("Buff_Ref", RefBuffers.GetValue(1));

    auto pMS = CreateShaderFromFileDXC(SHADER_TYPE_MESH, "GraphicsAndMeshShader.hlsl", "MSMain", "GraphicsAndMeshShader MS", Macros);
    ASSERT_TRUE(pMS);

    PSOCreateInfo.pVS = nullptr;
    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;

    IPipelineResourceSignature* MeshSignatures[] = {pSignatureMS, pSignaturePS};

    PSOCreateInfo.ppResourceSignatures    = MeshSignatures;
    PSOCreateInfo.ResourceSignaturesCount = _countof(MeshSignatures);

    RefCntAutoPtr<IPipelineState> pMeshPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pMeshPSO);
    ASSERT_NE(pMeshPSO, nullptr);

    ASSERT_EQ(pMeshPSO->GetResourceSignatureCount(), 2u);
    ASSERT_EQ(pMeshPSO->GetResourceSignature(0), pSignaturePS);
    ASSERT_EQ(pMeshPSO->GetResourceSignature(1), pSignatureMS);


    RefCntAutoPtr<IShaderResourceBinding> PixelSRB;
    pSignaturePS->CreateShaderResourceBinding(&PixelSRB, true);
    ASSERT_NE(PixelSRB, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> VertexSRB;
    pSignatureVS->CreateShaderResourceBinding(&VertexSRB, true);
    ASSERT_NE(VertexSRB, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> MeshSRB;
    pSignatureMS->CreateShaderResourceBinding(&MeshSRB, true);
    ASSERT_NE(MeshSRB, nullptr);

    PixelSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(RefTextures.GetView(0));
    VertexSRB->GetVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(RefBuffers.GetBuffer(0));
    MeshSRB->GetVariableByName(SHADER_TYPE_MESH, "Constants")->Set(RefBuffers.GetBuffer(1));

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // draw triangles
    {
        pContext->CommitShaderResources(PixelSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);  // BindingIndex == 0
        pContext->CommitShaderResources(VertexSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 1

        pContext->SetPipelineState(pGraphicsPSO);

        DrawAttribs drawAttrs(3, DRAW_FLAG_VERIFY_ALL);
        pContext->Draw(drawAttrs);
    }

    // draw meshes
    {
        pContext->SetPipelineState(pMeshPSO);

        pContext->CommitShaderResources(MeshSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); // BindingIndex == 1
        // reuse PixelSRB

        DrawMeshAttribs drawMeshAttrs(1, DRAW_FLAG_VERIFY_ALL);
        pContext->DrawMesh(drawMeshAttrs);
    }

    pSwapChain->Present();
}


void PipelineResourceSignatureTest::TestCombinedImageSamplers(SHADER_SOURCE_LANGUAGE ShaderLang, bool UseEmulatedSamplers)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    auto*       pContext   = pEnv->GetDeviceContext();
    auto*       pSwapChain = pEnv->GetSwapChain();

    if (DeviceInfo.IsD3DDevice() && ShaderLang != SHADER_SOURCE_LANGUAGE_HLSL)
        GTEST_SKIP() << "Direct3D supports HLSL only";

    float ClearColor[] = {0.625, 0.25, 0.375, 1.0};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    ReferenceTextures RefTextures{
        9,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    ShaderMacroHelper Macros;
    if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
        Macros.AddShaderMacro("float4", "vec4");
    Macros.AddShaderMacro("Tex2D_Static_Ref", RefTextures.GetColor(0));
    Macros.AddShaderMacro("Tex2DArr_Static_Ref0", RefTextures.GetColor(1));
    Macros.AddShaderMacro("Tex2DArr_Static_Ref1", RefTextures.GetColor(2));
    Macros.AddShaderMacro("Tex2D_Mut_Ref", RefTextures.GetColor(3));
    Macros.AddShaderMacro("Tex2DArr_Mut_Ref0", RefTextures.GetColor(4));
    Macros.AddShaderMacro("Tex2DArr_Mut_Ref1", RefTextures.GetColor(5));
    Macros.AddShaderMacro("Tex2D_Dyn_Ref", RefTextures.GetColor(6));
    Macros.AddShaderMacro("Tex2DArr_Dyn_Ref0", RefTextures.GetColor(7));
    Macros.AddShaderMacro("Tex2DArr_Dyn_Ref1", RefTextures.GetColor(8));

    ShaderCreateInfo ShaderCI;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.SourceLanguage             = ShaderLang;
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.Macros                     = Macros;
    ShaderCI.ShaderCompiler             = ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL ? SHADER_COMPILER_DEFAULT : pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.HLSLVersion                = {5, 0};

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.FilePath        = ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL ? "CombinedImageSamplers.hlsl" : "CombinedImageSamplersGL.vsh";
        ShaderCI.EntryPoint      = ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL ? "VSMain" : "main";
        ShaderCI.Desc.Name       = "CombinedImageSamplers - VS";
        pDevice->CreateShader(ShaderCI, &pVS);
    }
    ASSERT_NE(pVS, nullptr);

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.FilePath        = ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL ? "CombinedImageSamplers.hlsl" : "CombinedImageSamplersGL.psh";
        ShaderCI.EntryPoint      = ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL ? "PSMain" : "main";
        ShaderCI.Desc.Name       = "CombinedImageSamplers - PS";
        pDevice->CreateShader(ShaderCI, &pPS);
    }
    ASSERT_NE(pPS, nullptr);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Combined image samplers test";

    PRSDesc.UseCombinedTextureSamplers = true;

    auto ResFlag = PIPELINE_RESOURCE_FLAG_NONE;
    if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
    {
        // Native combined samplers in GLSL
        ResFlag = PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
        VERIFY_EXPR(!UseEmulatedSamplers);
        VERIFY_EXPR(!DeviceInfo.IsD3DDevice());
    }
    else if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL)
    {
        if (DeviceInfo.IsD3DDevice() || DeviceInfo.IsMetalDevice())
            ResFlag = UseEmulatedSamplers ? PIPELINE_RESOURCE_FLAG_NONE : PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
        else if (DeviceInfo.IsVulkanDevice())
        {
            // When compiling HLSL to SPIRV, we have to explicitly add samplers because PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER flag is used
            // to identify native combined samplers
            UseEmulatedSamplers = true;
            VERIFY_EXPR(ResFlag == PIPELINE_RESOURCE_FLAG_NONE);
        }
    }
    else
    {
        UNEXPECTED("Unexpected shader language");
    }

    VERIFY_EXPR(ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL || !UseEmulatedSamplers);

    constexpr auto SHADER_TYPE_VS_PS = SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL;
    // clang-format off
    std::vector<PipelineResourceDesc> Resources =
    {
        {SHADER_TYPE_VS_PS, "g_tex2D_Static",   1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC,  ResFlag},
        {SHADER_TYPE_VS_PS, "g_tex2D_Mut",      1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, ResFlag},
        {SHADER_TYPE_VS_PS, "g_tex2D_Dyn",      1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, ResFlag},
        {SHADER_TYPE_VS_PS, "g_tex2D_StaticArr",2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC,  ResFlag},
        {SHADER_TYPE_VS_PS, "g_tex2D_MutArr",   2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, ResFlag},
        {SHADER_TYPE_VS_PS, "g_tex2D_DynArr",   2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, ResFlag}
    };

    if (UseEmulatedSamplers)
    {
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_Static_sampler",   1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_Mut_sampler",      1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_Dyn_sampler",      1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_StaticArr_sampler",2, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_MutArr_sampler",   2, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        Resources.emplace_back(SHADER_TYPE_VS_PS, "g_tex2D_DynArr_sampler",   2, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    }
    // clang-format on
    PRSDesc.Resources    = Resources.data();
    PRSDesc.NumResources = static_cast<Uint32>(Resources.size());

    ImmutableSamplerDesc ImmutableSamplers[] = //
        {
            {SHADER_TYPE_ALL_GRAPHICS, "g_tex2D_StaticArr", SamplerDesc{}},
            {SHADER_TYPE_ALL_GRAPHICS, "g_tex2D_MutArr", SamplerDesc{}},
            {SHADER_TYPE_ALL_GRAPHICS, "g_tex2D_DynArr", SamplerDesc{}} //
        };
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_TRUE(pPRS);

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
    ASSERT_TRUE(pPSO);

    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc{}, &pSampler);

    RefTextures.GetView(0)->SetSampler(pSampler);
    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_tex2D_Static", Set, RefTextures.GetView(0));
    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_tex2D_StaticArr", SetArray, RefTextures.GetViewObjects(1), 0, 2);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPRS->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    RefTextures.GetView(3)->SetSampler(pSampler);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_tex2D_Mut", Set, RefTextures.GetView(3));
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_tex2D_MutArr", SetArray, RefTextures.GetViewObjects(4), 0, 2);

    RefTextures.GetView(6)->SetSampler(pSampler);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_tex2D_Dyn", Set, RefTextures.GetView(6));
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_tex2D_DynArr", SetArray, RefTextures.GetViewObjects(7), 0, 2);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

TEST_F(PipelineResourceSignatureTest, CombinedImageSamplers_HLSL)
{
    TestCombinedImageSamplers(SHADER_SOURCE_LANGUAGE_HLSL);
}

TEST_F(PipelineResourceSignatureTest, CombinedImageSamplers_HLSL_Emulated)
{
    TestCombinedImageSamplers(SHADER_SOURCE_LANGUAGE_HLSL, true);
}

TEST_F(PipelineResourceSignatureTest, CombinedImageSamplers_GLSL)
{
    TestCombinedImageSamplers(SHADER_SOURCE_LANGUAGE_GLSL);
}

void PipelineResourceSignatureTest::TestFormattedOrStructuredBuffer(BUFFER_MODE BufferMode)
{
    VERIFY_EXPR(BufferMode == BUFFER_MODE_FORMATTED || BufferMode == BUFFER_MODE_STRUCTURED);

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pContext   = pEnv->GetDeviceContext();
    auto* const pSwapChain = pEnv->GetSwapChain();
    const auto& deviceCaps = pDevice->GetDeviceInfo();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    float ClearColor[] = {0.875, 0.125, 0.75, 0.75};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    static constexpr Uint32 StaticBuffArraySize  = 4;
    static constexpr Uint32 MutableBuffArraySize = 3;
    static constexpr Uint32 DynamicBuffArraySize = 2;

    ReferenceBuffers RefBuffers{
        3 + StaticBuffArraySize + MutableBuffArraySize + DynamicBuffArraySize,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        BUFFER_VIEW_SHADER_RESOURCE,
        BufferMode //
    };

    // Buffer indices for vertex/shader bindings
    static constexpr size_t Buff_StaticIdx = 0;
    static constexpr size_t Buff_MutIdx    = 1;
    static constexpr size_t Buff_DynIdx    = 2;

    static constexpr size_t BuffArr_StaticIdx = 3;
    static constexpr size_t BuffArr_MutIdx    = BuffArr_StaticIdx + StaticBuffArraySize;
    static constexpr size_t BuffArr_DynIdx    = BuffArr_MutIdx + MutableBuffArraySize;

    ShaderMacroHelper Macros;

    const char*            ShaderPath  = BufferMode == BUFFER_MODE_FORMATTED ? "shaders/ShaderResourceLayout/FormattedBuffers.hlsl" : "shaders/ShaderResourceLayout/StructuredBuffers.hlsl";
    const char*            VSEntry     = "VSMain";
    const char*            PSEntry     = "PSMain";
    SHADER_SOURCE_LANGUAGE SrcLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    if (!deviceCaps.IsD3DDevice() && BufferMode == BUFFER_MODE_STRUCTURED)
    {
        ShaderPath  = "shaders/ShaderResourceLayout/StructuredBuffers.glsl";
        VSEntry     = "main";
        PSEntry     = "main";
        SrcLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        Macros.AddShaderMacro("float4", "vec4");
    }

    Macros.AddShaderMacro("STATIC_BUFF_ARRAY_SIZE", static_cast<int>(StaticBuffArraySize));
    Macros.AddShaderMacro("MUTABLE_BUFF_ARRAY_SIZE", static_cast<int>(MutableBuffArraySize));
    Macros.AddShaderMacro("DYNAMIC_BUFF_ARRAY_SIZE", static_cast<int>(DynamicBuffArraySize));

    Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(Buff_StaticIdx));
    Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(Buff_MutIdx));
    Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(Buff_DynIdx));

    for (Uint32 i = 0; i < StaticBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Static_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_StaticIdx + i));

    for (Uint32 i = 0; i < MutableBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Mut_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_MutIdx + i));

    for (Uint32 i = 0; i < DynamicBuffArraySize; ++i)
        Macros.AddShaderMacro((std::string{"BuffArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefBuffers.GetValue(BuffArr_DynIdx + i));

    auto ModifyShaderCI = [&](ShaderCreateInfo& ShaderCI) {
        ShaderCI.SourceLanguage = SrcLanguage;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // As of Windows version 2004 (build 19041), there is a bug in D3D12 WARP rasterizer:
            // Shader resource array indexing always references array element 0 when shaders are compiled
            // with shader model 5.1.
            // Use SM5.0 with old compiler as a workaround.
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };

    auto pVS = CreateShaderFromFile(SHADER_TYPE_VERTEX, ShaderPath, VSEntry, "PRS FormattedBuffers - VS", Macros, ModifyShaderCI);
    auto pPS = CreateShaderFromFile(SHADER_TYPE_PIXEL, ShaderPath, PSEntry, "PRS FormattedBuffers - PS", Macros, ModifyShaderCI);
    ASSERT_TRUE(pVS && pPS);

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Formatted buffer test";

    constexpr auto SHADER_TYPE_VS_PS   = SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL;
    const auto     FormattedBufferFlag = BufferMode == BUFFER_MODE_FORMATTED ? PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER : PIPELINE_RESOURCE_FLAG_NONE;
    // clang-format off
    PipelineResourceDesc Resources[]
    {
        {SHADER_TYPE_VS_PS, "g_Buff_Static",   1, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC,  FormattedBufferFlag | PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
        {SHADER_TYPE_VS_PS, "g_Buff_Mut",      1, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, FormattedBufferFlag | PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
        {SHADER_TYPE_VS_PS, "g_Buff_Dyn",      1, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, FormattedBufferFlag | PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
        {SHADER_TYPE_VS_PS, "g_BuffArr_Static",StaticBuffArraySize,  SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC,  FormattedBufferFlag},
        {SHADER_TYPE_VS_PS, "g_BuffArr_Mut",   MutableBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, FormattedBufferFlag},
        {SHADER_TYPE_VS_PS, "g_BuffArr_Dyn",   DynamicBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, FormattedBufferFlag}
    };
    // clang-format on
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_TRUE(pPRS);

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
    ASSERT_TRUE(pPSO);

    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_Buff_Static", Set, RefBuffers.GetView(Buff_StaticIdx));
    SET_STATIC_VAR(pPRS, SHADER_TYPE_VERTEX, "g_BuffArr_Static", SetArray, RefBuffers.GetViewObjects(BuffArr_StaticIdx), 0, StaticBuffArraySize);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPRS->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Buff_Mut", Set, RefBuffers.GetView(Buff_MutIdx));
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_BuffArr_Mut", SetArray, RefBuffers.GetViewObjects(BuffArr_MutIdx), 0, MutableBuffArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Buff_Dyn", Set, RefBuffers.GetView(Buff_DynIdx));
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Dyn", SetArray, RefBuffers.GetViewObjects(BuffArr_DynIdx), 0, DynamicBuffArraySize);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

TEST_F(PipelineResourceSignatureTest, FormattedBuffers)
{
    TestFormattedOrStructuredBuffer(BUFFER_MODE_FORMATTED);
}

TEST_F(PipelineResourceSignatureTest, StructuredBuffers)
{
    TestFormattedOrStructuredBuffer(BUFFER_MODE_STRUCTURED);
}

static void TestRunTimeResourceArray(bool IsGLSL, IShaderSourceInputStreamFactory* pShaderSourceFactory)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& deviceCaps = pDevice->GetDeviceInfo();
    if (!deviceCaps.Features.ShaderResourceRuntimeArray)
    {
        GTEST_SKIP() << "Shader Resource Runtime Arrays are not supported by this device";
    }

    if (IsGLSL && deviceCaps.IsD3DDevice())
    {
        GTEST_SKIP() << "Direct3D does not support GLSL";
    }

    if (deviceCaps.IsVulkanDevice() && !IsGLSL && !pEnv->HasDXCompiler())
    {
        GTEST_SKIP() << "Vulkan requires DXCompiler which is not found";
    }

    bool ConstantBufferNonUniformIndexing = true;
    bool SRVBufferNonUniformIndexing      = true;
    bool UAVBufferNonUniformIndexing      = true;
    bool SRVTextureNonUniformIndexing     = true;
    bool UAVTextureNonUniformIndexing     = true;

#if VULKAN_SUPPORTED
    if (pDevice->GetDeviceInfo().IsVulkanDevice())
    {
        auto* pEnvVk                     = static_cast<TestingEnvironmentVk*>(pEnv);
        ConstantBufferNonUniformIndexing = (pEnvVk->DescriptorIndexing.shaderUniformBufferArrayNonUniformIndexing == VK_TRUE);
        SRVBufferNonUniformIndexing      = (pEnvVk->DescriptorIndexing.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE);
        UAVBufferNonUniformIndexing      = SRVBufferNonUniformIndexing;
        SRVTextureNonUniformIndexing     = (pEnvVk->DescriptorIndexing.shaderSampledImageArrayNonUniformIndexing == VK_TRUE);
        UAVTextureNonUniformIndexing     = (pEnvVk->DescriptorIndexing.shaderStorageImageArrayNonUniformIndexing == VK_TRUE);
    }
#endif

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pContext   = pEnv->GetDeviceContext();
    auto* pSwapChain = pEnv->GetSwapChain();

    ComputeShaderReference(pSwapChain);

    constexpr Uint32  TexArraySize = 8;
    ReferenceTextures RefTextures{
        TexArraySize,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    constexpr Uint32  RWTexArraySize = 3;
    ReferenceTextures RefRWTextures{
        RWTexArraySize,
        128, 128,
        USAGE_DEFAULT,
        BIND_UNORDERED_ACCESS,
        TEXTURE_VIEW_UNORDERED_ACCESS //
    };

    constexpr Uint32 SamArraySize = 3;

    constexpr Uint32 ConstBuffArraySize = 7;
    ReferenceBuffers RefConstBuffers{
        ConstBuffArraySize,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };

    constexpr Uint32 FmtBuffArraySize = 5;
    ReferenceBuffers RefFmtBuffers{
        FmtBuffArraySize,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        BUFFER_VIEW_SHADER_RESOURCE,
        BUFFER_MODE_FORMATTED //
    };

    constexpr Uint32 StructBuffArraySize = 3;
    ReferenceBuffers RefStructBuffers{
        StructBuffArraySize,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        BUFFER_VIEW_SHADER_RESOURCE,
        BUFFER_MODE_STRUCTURED //
    };

    constexpr Uint32 RWStructBuffArraySize = 4;
    ReferenceBuffers RefRWStructBuffers{
        RWStructBuffArraySize,
        USAGE_DEFAULT,
        BIND_UNORDERED_ACCESS,
        BUFFER_VIEW_UNORDERED_ACCESS,
        BUFFER_MODE_STRUCTURED //
    };

    constexpr Uint32 RWFormattedBuffArraySize = 2;
    ReferenceBuffers RefRWFormattedBuffers{
        RWFormattedBuffArraySize,
        USAGE_DEFAULT,
        BIND_UNORDERED_ACCESS,
        BUFFER_VIEW_UNORDERED_ACCESS,
        BUFFER_MODE_FORMATTED //
    };

    RefCntAutoPtr<IPipelineResourceSignature> pSignature;

    {
        const PipelineResourceDesc Resources[] =
            {
                {SHADER_TYPE_COMPUTE, "g_Textures", TexArraySize, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_Samplers", SamArraySize, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_ConstantBuffers", ConstBuffArraySize, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_FormattedBuffers", FmtBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_StructuredBuffers", StructBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_RWTextures", RWTexArraySize, SHADER_RESOURCE_TYPE_TEXTURE_UAV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_RWStructBuffers", RWStructBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_RWFormattedBuffers", RWFormattedBuffArraySize, SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                {SHADER_TYPE_COMPUTE, "g_OutImage", 1, SHADER_RESOURCE_TYPE_TEXTURE_UAV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        PipelineResourceSignatureDesc Desc;
        Desc.Resources    = Resources;
        Desc.NumResources = _countof(Resources);
        Desc.BindingIndex = 0;

        pDevice->CreatePipelineResourceSignature(Desc, &pSignature);
        ASSERT_NE(pSignature, nullptr);
    }

    SamplerDesc SamLinearWrapDesc{
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP};
    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamLinearWrapDesc, &pSampler);
    ASSERT_NE(pSampler, nullptr);
    IDeviceObject* ppSamplers[] = {pSampler, pSampler, pSampler};
    static_assert(_countof(ppSamplers) == SamArraySize, "Invalid sampler array size");

    ComputePipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc = PSOCreateInfo.PSODesc;

    PSODesc.Name         = "PRS descriptor indexing test";
    PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;

    ShaderMacroHelper Macros;

    Macros.AddShaderMacro("NUM_TEXTURES", TexArraySize);
    Macros.AddShaderMacro("NUM_SAMPLERS", SamArraySize);
    Macros.AddShaderMacro("NUM_CONST_BUFFERS", ConstBuffArraySize);
    Macros.AddShaderMacro("NUM_FMT_BUFFERS", FmtBuffArraySize);
    Macros.AddShaderMacro("NUM_STRUCT_BUFFERS", StructBuffArraySize);
    Macros.AddShaderMacro("NUM_RWTEXTURES", RWTexArraySize);
    Macros.AddShaderMacro("NUM_RWSTRUCT_BUFFERS", RWStructBuffArraySize);
    Macros.AddShaderMacro("NUM_RWFMT_BUFFERS", RWFormattedBuffArraySize);

    Macros.AddShaderMacro("TEXTURES_NONUNIFORM_INDEXING", SRVTextureNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("CONST_BUFFERS_NONUNIFORM_INDEXING", ConstantBufferNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("FMT_BUFFERS_NONUNIFORM_INDEXING", SRVBufferNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("STRUCT_BUFFERS_NONUNIFORM_INDEXING", SRVBufferNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("RWTEXTURES_NONUNIFORM_INDEXING", UAVTextureNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("RWSTRUCT_BUFFERS_NONUNIFORM_INDEXING", UAVBufferNonUniformIndexing ? 1 : 0);
    Macros.AddShaderMacro("RWFMT_BUFFERS_NONUNIFORM_INDEXING", UAVBufferNonUniformIndexing ? 1 : 0);

    if (pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
    {
        // Constant buffer indexing does not work properly in D3D12 WARP - only the 0th element is accessed correctly
        Macros.AddShaderMacro("USE_D3D12_WARP_BUG_WORKAROUND", 1);
    }

    if (IsGLSL)
        Macros.AddShaderMacro("float4", "vec4");
    for (Uint32 i = 0; i < TexArraySize; ++i)
        Macros.AddShaderMacro((String{"Tex2D_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(i));
    for (Uint32 i = 0; i < ConstBuffArraySize; ++i)
        Macros.AddShaderMacro((String{"ConstBuff_Ref"} + std::to_string(i)).c_str(), RefConstBuffers.GetValue(i));
    for (Uint32 i = 0; i < FmtBuffArraySize; ++i)
        Macros.AddShaderMacro((String{"FmtBuff_Ref"} + std::to_string(i)).c_str(), RefFmtBuffers.GetValue(i));
    for (Uint32 i = 0; i < StructBuffArraySize; ++i)
        Macros.AddShaderMacro((String{"StructBuff_Ref"} + std::to_string(i)).c_str(), RefStructBuffers.GetValue(i));
    for (Uint32 i = 0; i < RWTexArraySize; ++i)
        Macros.AddShaderMacro((String{"RWTex2D_Ref"} + std::to_string(i)).c_str(), RefRWTextures.GetColor(i));
    for (Uint32 i = 0; i < RWStructBuffArraySize; ++i)
        Macros.AddShaderMacro((String{"RWStructBuff_Ref"} + std::to_string(i)).c_str(), RefRWStructBuffers.GetValue(i));
    for (Uint32 i = 0; i < RWFormattedBuffArraySize; ++i)
        Macros.AddShaderMacro((String{"RWFmtBuff_Ref"} + std::to_string(i)).c_str(), RefRWFormattedBuffers.GetValue(i));

    RefCntAutoPtr<IShader> pCS;
    {
        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint                 = "main";
        ShaderCI.Desc.Name                  = "RunTimeResourceArray - CS";
        ShaderCI.Macros                     = Macros;
        ShaderCI.SourceLanguage             = IsGLSL ? SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM : SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.FilePath                   = IsGLSL ? "RunTimeResourceArray.glsl" : "RunTimeResourceArray.hlsl";
        ShaderCI.CompileFlags               = SHADER_COMPILE_FLAG_ENABLE_UNBOUNDED_ARRAYS;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        if (!deviceCaps.IsD3DDevice() && !IsGLSL)
        {
            // Run-time resource arrays are not handled well by GLSLang: NonUniformResourceIndex is not defined;
            // Constant buffer, structured buffer and RW structured buffer arrays have issues.
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
        }

        pDevice->CreateShader(ShaderCI, &pCS);
        ASSERT_NE(pCS, nullptr);
    }

    PSOCreateInfo.pCS = pCS;

    IPipelineResourceSignature* Signatures[] = {pSignature};

    PSOCreateInfo.ppResourceSignatures    = Signatures;
    PSOCreateInfo.ResourceSignaturesCount = _countof(Signatures);

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    ASSERT_EQ(pPSO->GetResourceSignatureCount(), 1u);
    ASSERT_EQ(pPSO->GetResourceSignature(0), pSignature);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pSignature->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    ASSERT_TRUE(pTestingSwapChain);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_OutImage")->Set(pTestingSwapChain->GetCurrentBackBufferUAV());
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Textures")->SetArray(RefTextures.GetViewObjects(0), 0, TexArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Samplers")->SetArray(ppSamplers, 0, SamArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ConstantBuffers")->SetArray(RefConstBuffers.GetBuffObjects(0), 0, ConstBuffArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_FormattedBuffers")->SetArray(RefFmtBuffers.GetViewObjects(0), 0, FmtBuffArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_StructuredBuffers")->SetArray(RefStructBuffers.GetViewObjects(0), 0, StructBuffArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_RWTextures")->SetArray(RefRWTextures.GetViewObjects(0), 0, RWTexArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_RWStructBuffers")->SetArray(RefRWStructBuffers.GetViewObjects(0), 0, RWStructBuffArraySize);
    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_RWFormattedBuffers")->SetArray(RefRWFormattedBuffers.GetViewObjects(0), 0, RWFormattedBuffArraySize);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    const auto&            SCDesc = pSwapChain->GetDesc();
    DispatchComputeAttribs DispatchAttribs((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1);
    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

TEST_F(PipelineResourceSignatureTest, RunTimeResourceArray_GLSL)
{
    TestRunTimeResourceArray(true, pShaderSourceFactory);
}

TEST_F(PipelineResourceSignatureTest, RunTimeResourceArray_HLSL)
{
    TestRunTimeResourceArray(false, pShaderSourceFactory);
}


TEST_F(PipelineResourceSignatureTest, UnusedNullResources)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pContext   = pEnv->GetDeviceContext();
    auto* pSwapChain = pEnv->GetSwapChain();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    float ClearColor[] = {0.875, 0.375, 0.125, 0.25};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Triangle VS";
        ShaderCI.Source          = HLSL::DrawTest_ProceduralTriangleVS.c_str();
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Triangle PS";
        ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Unused dynamic buffer test";

    // clang-format off
    const PipelineResourceDesc Resources[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_UnmappedStaticBuffer",  1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_UnmappedMutableBuffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_UnmappedDynamicBuffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullMutableBuffer",     2, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullDynamicBuffer",     2, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullMutableTexture",    4, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullDynamicTexture",    4, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullMutableBuffSRV",    2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullDynamicBuffSRV",    2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullMutableNoDynBuffSRV",      2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullDynamicNoDynBuffSRV",      2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullMutableFormattedBuffSRV",  2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_NullDynamicFormattedBuffSRV",  2, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}
    };
    // clang-format on

    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    RefCntAutoPtr<IPipelineResourceSignature> pPRS;
    pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
    ASSERT_TRUE(pPRS);

    auto pPSO = CreateGraphicsPSO(pVS, pPS, {pPRS});
    ASSERT_TRUE(pPSO);

    RefCntAutoPtr<IBuffer> pBuffer;
    {
        BufferDesc BuffDesc{"Unused dynamic buffer", 512, BIND_UNIFORM_BUFFER, USAGE_DYNAMIC, CPU_ACCESS_WRITE};
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    }
    ASSERT_TRUE(pBuffer);

    pPRS->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_UnmappedStaticBuffer")->Set(pBuffer);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPRS->CreateShaderResourceBinding(&pSRB, true);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_UnmappedMutableBuffer")->Set(pBuffer);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_UnmappedDynamicBuffer")->Set(pBuffer);

    auto  pTexture = pEnv->CreateTexture("Dummy texture", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, 256, 256);
    auto* pTexSRV  = pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    IDeviceObject* ppTexObjs[] = {pTexSRV, pTexSRV};
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_NullMutableTexture")->SetArray(ppTexObjs, 1, 2);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_NullDynamicTexture")->SetArray(ppTexObjs, 1, 1);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_NullDynamicTexture")->SetArray(ppTexObjs, 3, 1);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

} // namespace Diligent
