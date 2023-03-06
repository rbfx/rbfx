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

#include "GraphicsAccessories.hpp"
#include "../../../Graphics/GraphicsEngineD3DBase/interface/ShaderD3D.h"

#include "GPUTestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestShaderResourceVariableCInterface(void* pVar, void* pObjectToSet);
    int TestShaderResourceBindingCInterface(void* pSRB);
    int TestShaderCInterface(void* pShader);
    int TestPipelineStateCInterface(void* pPSO);
}

namespace Diligent
{

namespace Testing
{

void PrintShaderResources(IShader* pShader)
{
    RefCntAutoPtr<IShaderD3D> pShaderD3D(pShader, IID_ShaderD3D);

    std::stringstream ss;
    ss << "Resources of shader '" << pShader->GetDesc().Name << "':" << std::endl;
    for (Uint32 res = 0; res < pShader->GetResourceCount(); ++res)
    {
        ShaderResourceDesc ResDec;
        pShader->GetResourceDesc(res, ResDec);

        std::stringstream name_ss;
        name_ss << ResDec.Name;
        if (ResDec.ArraySize > 1)
            name_ss << "[" << ResDec.ArraySize << "]";
        ss << std::setw(2) << std::right << res << ": " << std::setw(25) << std::left << name_ss.str();
        if (pShaderD3D)
        {
            HLSLShaderResourceDesc HLSLResDesc;
            pShaderD3D->GetHLSLResource(res, HLSLResDesc);
            ss << "  hlsl register " << std::setw(2) << HLSLResDesc.ShaderRegister;
        }
        ss << "   " << GetShaderResourceTypeLiteralName(ResDec.Type) << std::endl;
    }
    LOG_INFO_MESSAGE(ss.str());
}

} // namespace Testing

} // namespace Diligent
namespace
{

TEST(ShaderResourceLayout, VariableAccess)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (!DeviceInfo.Features.SeparablePrograms)
    {
        GTEST_SKIP() << "Shader variable access test requires separate programs";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    ShaderCreateInfo ShaderCI;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.EntryPoint                 = "main";

    RefCntAutoPtr<ISampler> pSamplers[2];
    IDeviceObject*          pSams[2] = {};
    for (size_t i = 0; i < _countof(pSams); ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &(pSamplers[i]));
        pSams[i] = pSamplers[i];
    }

    RefCntAutoPtr<ITexture> pTex[2];
    TextureDesc             TexDesc;
    TexDesc.Type            = RESOURCE_DIM_TEX_2D;
    TexDesc.Width           = 1024;
    TexDesc.Height          = 1024;
    TexDesc.Format          = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags       = BIND_SHADER_RESOURCE;
    IDeviceObject* pSRVs[2] = {};
    for (size_t i = 0; i < _countof(pSRVs); ++i)
    {
        pDevice->CreateTexture(TexDesc, nullptr, &(pTex[i]));
        auto* pSRV = pTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pSRV->SetSampler(pSamplers[i]);
        pSRVs[i] = pSRV;
    }

    RefCntAutoPtr<ITexture> pRWTex[8];
    IDeviceObject*          pTexUAVs[_countof(pRWTex)]   = {};
    IDeviceObject*          pRWTexSRVs[_countof(pRWTex)] = {};
    TexDesc.BindFlags                                    = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    TexDesc.Format                                       = TEX_FORMAT_RGBA32_FLOAT;
    for (size_t i = 0; i < _countof(pRWTex); ++i)
    {
        pDevice->CreateTexture(TexDesc, nullptr, &(pRWTex[i]));
        pTexUAVs[i]   = pRWTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
        pRWTexSRVs[i] = pRWTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }


    //RefCntAutoPtr<ITexture> pStorageTex[4];
    //IDeviceObject *pUAVs[4];
    //for (int i = 0; i < 4; ++i)
    //{
    //    pDevice->CreateTexture(TexDesc, TextureData{}, &(pStorageTex[i]));
    //    pUAVs[i] = pStorageTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
    //}

    constexpr auto RTVFormat = TEX_FORMAT_RGBA8_UNORM;
    constexpr auto DSVFormat = TEX_FORMAT_D32_FLOAT;

    TexDesc.Format    = RTVFormat;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RefCntAutoPtr<ITexture> pRenderTarget;
    pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
    auto* pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

    TexDesc.Format    = DSVFormat;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL;
    RefCntAutoPtr<ITexture> pDepthTex;
    pDevice->CreateTexture(TexDesc, nullptr, &pDepthTex);
    auto* pDSV = pDepthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    BufferDesc BuffDesc;
    BuffDesc.Size      = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[2];
    IDeviceObject*         pUBs[2];
    for (size_t i = 0; i < _countof(pUniformBuffs); ++i)
    {
        pDevice->CreateBuffer(BuffDesc, nullptr, &(pUniformBuffs[i]));
        pUBs[i] = pUniformBuffs[i];
    }

    //BuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
    //BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
    //BuffDesc.ElementByteStride = 16;
    //RefCntAutoPtr<IBuffer> pStorgeBuffs[4];
    //IDeviceObject *pSBUAVs[4];
    //for (int i = 0; i < 4; ++i)
    //{
    //    pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pStorgeBuffs[i]));
    //    pSBUAVs[i] = pStorgeBuffs[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    //}

    RefCntAutoPtr<IBuffer>     pFormattedBuff0, pFormattedBuff[4], pRawBuff[2];
    IDeviceObject *            pFormattedBuffSRV = nullptr, *pFormattedBuffUAV[4] = {}, *pFormattedBuffSRVs[4] = {};
    RefCntAutoPtr<IBufferView> spFormattedBuffSRV, spFormattedBuffUAV[4], spFormattedBuffSRVs[4];
    RefCntAutoPtr<IBufferView> spRawBuffUAV[2], spRawBuffSRVs[2];
    {
        BufferDesc TxlBuffDesc;
        TxlBuffDesc.Name              = "Uniform texel buffer test";
        TxlBuffDesc.Size              = 256;
        TxlBuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
        TxlBuffDesc.Usage             = USAGE_DEFAULT;
        TxlBuffDesc.ElementByteStride = 16;
        TxlBuffDesc.Mode              = BUFFER_MODE_FORMATTED;
        pDevice->CreateBuffer(TxlBuffDesc, nullptr, &pFormattedBuff0);

        BufferViewDesc ViewDesc;
        ViewDesc.ViewType             = BUFFER_VIEW_SHADER_RESOURCE;
        ViewDesc.Format.ValueType     = VT_FLOAT32;
        ViewDesc.Format.NumComponents = 4;
        ViewDesc.Format.IsNormalized  = false;
        pFormattedBuff0->CreateView(ViewDesc, &spFormattedBuffSRV);
        pFormattedBuffSRV = spFormattedBuffSRV;

        for (size_t i = 0; i < _countof(pFormattedBuff); ++i)
        {
            TxlBuffDesc.Name      = "UAV buffer test";
            TxlBuffDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
            pDevice->CreateBuffer(TxlBuffDesc, nullptr, &(pFormattedBuff[i]));

            ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
            pFormattedBuff[i]->CreateView(ViewDesc, &(spFormattedBuffUAV[i]));
            pFormattedBuffUAV[i] = spFormattedBuffUAV[i];

            ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
            pFormattedBuff[i]->CreateView(ViewDesc, &(spFormattedBuffSRVs[i]));
            pFormattedBuffSRVs[i] = spFormattedBuffSRVs[i];
        }

        TxlBuffDesc.Mode          = BUFFER_MODE_RAW;
        ViewDesc.Format.ValueType = VT_UNDEFINED;
        for (size_t i = 0; i < _countof(pRawBuff); ++i)
        {
            TxlBuffDesc.Name = "Raw buffer test";
            pDevice->CreateBuffer(TxlBuffDesc, nullptr, &(pRawBuff[i]));
            spRawBuffUAV[i]  = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
            spRawBuffSRVs[i] = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
        }
    }

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc = {"Shader variable access test VS", SHADER_TYPE_VERTEX, true};
        if (DeviceInfo.IsD3DDevice())
        {
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.FilePath       = "ShaderVariableAccessTestDX.vsh";
        }
        else
        {
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
            ShaderCI.FilePath       = "ShaderVariableAccessTestGL.vsh";
        }
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
        TestShaderCInterface(pVS);

        Diligent::Testing::PrintShaderResources(pVS);
    }

    // clang-format off
    std::vector<ShaderResourceVariableDesc> VarDesc =
        {
            {SHADER_TYPE_VERTEX, "g_tex2D_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            {SHADER_TYPE_PIXEL,  "g_tex2D_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},

            {SHADER_TYPE_VERTEX, "g_tex2D_StaticArr", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            {SHADER_TYPE_PIXEL,  "g_tex2D_StaticArr", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},

            {SHADER_TYPE_VERTEX, "g_tex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL,  "g_tex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

            {SHADER_TYPE_VERTEX, "g_tex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL,  "g_tex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

            {SHADER_TYPE_VERTEX, "g_tex2D_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL,  "g_tex2D_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

            {SHADER_TYPE_VERTEX, "g_tex2D_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL,  "g_tex2D_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

            {SHADER_TYPE_VERTEX, "UniformBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL,  "UniformBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

            {SHADER_TYPE_VERTEX, "UniformBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL,  "UniformBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

            {SHADER_TYPE_VERTEX, "g_Buffer_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL,  "g_Buffer_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

            {SHADER_TYPE_VERTEX, "g_Buffer_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL,  "g_Buffer_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

            {SHADER_TYPE_VERTEX, "g_Buffer_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL,  "g_Buffer_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

            {SHADER_TYPE_VERTEX, "g_Buffer_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL,  "g_Buffer_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

            {SHADER_TYPE_PIXEL, "g_rwtex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL, "g_rwBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_rwBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        };
    // clang-format on

    ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Static", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_StaticArr", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Mut", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_MutArr", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Dyn", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_DynArr", SamplerDesc{}},
        };

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc = {"Shader variable access test PS", SHADER_TYPE_PIXEL, true};
        if (DeviceInfo.IsD3DDevice())
        {
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.FilePath       = "ShaderVariableAccessTestDX.psh";
        }
        else
        {
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
            ShaderCI.FilePath       = "ShaderVariableAccessTestGL.psh";
        }
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);

        Diligent::Testing::PrintShaderResources(pPS);
    }


    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& ResourceLayout   = PSODesc.ResourceLayout;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    ResourceLayout.Variables            = VarDesc.data();
    ResourceLayout.NumVariables         = static_cast<Uint32>(VarDesc.size());
    ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    ResourceLayout.ImmutableSamplers    = ImtblSamplers;

    PSODesc.Name                       = "Shader variable access test PSO";
    PSOCreateInfo.pVS                  = pVS;
    PSOCreateInfo.pPS                  = pPS;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.NumRenderTargets  = 1;
    GraphicsPipeline.RTVFormats[0]     = RTVFormat;
    GraphicsPipeline.DSVFormat         = DSVFormat;
    PSODesc.SRBAllocationGranularity   = 16;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pTestPSO);
    ASSERT_NE(pTestPSO, nullptr);
    EXPECT_EQ(TestPipelineStateCInterface(pTestPSO), 0);

    LOG_INFO_MESSAGE("No worries, warnings below are expected - testing variable queries from inactive/invalid shader stages\n");

    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_GEOMETRY), 0u);
    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_DOMAIN), 0u);
    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_HULL), 0u);
    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_COMPUTE), 0u);
    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_AMPLIFICATION), 0u);
    EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_MESH), 0u);

    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_GEOMETRY, "g_tex2D_Static"), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_DOMAIN, "g_tex2D_Static"), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_HULL, "g_tex2D_Static"), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "g_tex2D_Static"), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_AMPLIFICATION, "g_tex2D_Static"), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByName(SHADER_TYPE_MESH, "g_tex2D_Static"), nullptr);

    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_GEOMETRY, 0), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_DOMAIN, 0), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_HULL, 0), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_COMPUTE, 0), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_AMPLIFICATION, 0), nullptr);
    EXPECT_EQ(pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_MESH, 0), nullptr);

    {
        EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_VERTEX), 6u);

        {
            auto tex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static");
            ASSERT_NE(tex2D_Static, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            tex2D_Static->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[0]);

            tex2D_Static->Set(pSRVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[1]);
            tex2D_Static->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Static->Get(), nullptr);

            tex2D_Static->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[0]);
        }

        {
            auto tex2D_Static_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static_sampler");
            EXPECT_EQ(tex2D_Static_sampler, nullptr);
        }

        {
            auto tex2D_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_StaticArr");
            ASSERT_NE(tex2D_StaticArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_StaticArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_StaticArr, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            tex2D_StaticArr->SetArray(pSRVs, 0, 2);
            EXPECT_EQ(tex2D_StaticArr->Get(0), pSRVs[0]);
            EXPECT_EQ(tex2D_StaticArr->Get(1), pSRVs[1]);

            tex2D_StaticArr->SetArray(pSRVs + 1, 0, 1, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_StaticArr->Get(0), pSRVs[1]);
            tex2D_StaticArr->SetArray(pSRVs, 0, 1, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_StaticArr->Get(0), pSRVs[0]);
        }

        {
            auto tex2D_StaticArr_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_StaticArr_sampler");
            EXPECT_EQ(tex2D_StaticArr_sampler, nullptr);
        }

        {
            auto UniformBuff_Stat = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
            ASSERT_NE(UniformBuff_Stat, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Stat->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Stat, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            UniformBuff_Stat->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[0]);

            UniformBuff_Stat->Set(pUBs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[1]);
            UniformBuff_Stat->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Stat->Get(), nullptr);

            UniformBuff_Stat->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[0]);
        }

        {
            auto UniformBuff_Stat2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat2");
            ASSERT_NE(UniformBuff_Stat2, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Stat2->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Stat2, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            UniformBuff_Stat2->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat2->Get(), pUBs[0]);
        }

        {
            auto Buffer_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Static");
            ASSERT_NE(Buffer_Static, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_Static->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Static->Get(), pFormattedBuffSRV);

            Buffer_Static->Set(pFormattedBuffSRVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Static->Get(), pFormattedBuffSRVs[1]);
            Buffer_Static->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Static->Get(), nullptr);

            Buffer_Static->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Static->Get(), pFormattedBuffSRV);
        }

        {
            auto Buffer_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_StaticArr");
            ASSERT_NE(Buffer_StaticArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_StaticArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_StaticArr, pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_StaticArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_StaticArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_StaticArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_StaticArr->Get(1), pFormattedBuffSRV);
        }


        {
            auto tex2D_Mut = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
            EXPECT_EQ(tex2D_Mut, nullptr);
        }
        {
            auto tex2D_Dyn = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn");
            EXPECT_EQ(tex2D_Dyn, nullptr);
        }
        {
            auto tex2D_Mut_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut_sampler");
            EXPECT_EQ(tex2D_Mut_sampler, nullptr);
        }
        {
            auto tex2D_Dyn_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr_sampler");
            EXPECT_EQ(tex2D_Dyn_sampler, nullptr);
        }



        auto NumVSVars = pTestPSO->GetStaticVariableCount(SHADER_TYPE_VERTEX);
        for (Uint32 v = 0; v < NumVSVars; ++v)
        {
            auto pVar = pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_VERTEX, v);
            EXPECT_EQ(pVar->GetIndex(), v);
            EXPECT_EQ(pVar->GetType(), SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
            ShaderResourceDesc ResDesc;
            pVar->GetResourceDesc(ResDesc);
            auto pVar2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name);
            EXPECT_EQ(pVar, pVar2);
        }
    }


    {
        EXPECT_EQ(pTestPSO->GetStaticVariableCount(SHADER_TYPE_PIXEL), 9u);

        {
            auto tex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static");
            ASSERT_NE(tex2D_Static, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_Static->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[0]);

            tex2D_Static->Set(pSRVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[1]);
            tex2D_Static->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Static->Get(), nullptr);

            tex2D_Static->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Static->Get(), pSRVs[0]);
        }

        {
            auto tex2D_Static_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static_sampler");
            EXPECT_EQ(tex2D_Static_sampler, nullptr);
        }

        {
            auto tex2D_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_StaticArr");
            ASSERT_NE(tex2D_StaticArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_StaticArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_StaticArr, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_StaticArr->SetArray(pSRVs, 0, 2);
            EXPECT_EQ(tex2D_StaticArr->Get(0), pSRVs[0]);
            EXPECT_EQ(tex2D_StaticArr->Get(1), pSRVs[1]);
        }
        {
            auto tex2D_StaticArr_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_StaticArr_sampler");
            EXPECT_EQ(tex2D_StaticArr_sampler, nullptr);
        }

        {
            auto UniformBuff_Stat = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat");
            ASSERT_NE(UniformBuff_Stat, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Stat->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Stat, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            UniformBuff_Stat->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[0]);

            UniformBuff_Stat->Set(pUBs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[1]);
            UniformBuff_Stat->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Stat->Get(), nullptr);

            UniformBuff_Stat->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat->Get(), pUBs[0]);
        }

        {
            auto UniformBuff_Stat2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat2");
            ASSERT_NE(UniformBuff_Stat2, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Stat2->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Stat2, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            UniformBuff_Stat2->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Stat2->Get(), pUBs[0]);
        }

        {
            auto Buffer_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Static");
            ASSERT_NE(Buffer_Static, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_Static->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Static->Get(), pFormattedBuffSRV);
        }

        {
            auto Buffer_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_StaticArr");
            ASSERT_NE(Buffer_StaticArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_StaticArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_StaticArr, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_StaticArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_StaticArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_StaticArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_StaticArr->Get(1), pFormattedBuffSRV);
        }


        {
            auto rwtex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Static");
            ASSERT_NE(rwtex2D_Static, nullptr);
            ShaderResourceDesc ResDesc;
            rwtex2D_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwtex2D_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwtex2D_Static->Set(pTexUAVs[0]);
            EXPECT_EQ(rwtex2D_Static->Get(), pTexUAVs[0]);

            rwtex2D_Static->Set(pTexUAVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwtex2D_Static->Get(), pTexUAVs[1]);
            rwtex2D_Static->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwtex2D_Static->Get(), nullptr);

            rwtex2D_Static->Set(pTexUAVs[0]);
            EXPECT_EQ(rwtex2D_Static->Get(), pTexUAVs[0]);
        }


        {
            auto rwtex2D_Static2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Static2");
            ASSERT_NE(rwtex2D_Static2, nullptr);
            ShaderResourceDesc ResDesc;
            rwtex2D_Static2->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwtex2D_Static2, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwtex2D_Static2->Set(pTexUAVs[1]);
            EXPECT_EQ(rwtex2D_Static2->Get(), pTexUAVs[1]);
        }

        {
            auto rwBuff_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Static");
            ASSERT_NE(rwBuff_Static, nullptr);
            ShaderResourceDesc ResDesc;
            rwBuff_Static->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwBuff_Static, pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwBuff_Static->Set(spRawBuffUAV[0]);
            EXPECT_EQ(rwBuff_Static->Get(), spRawBuffUAV[0].RawPtr());

            rwBuff_Static->Set(spRawBuffUAV[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwBuff_Static->Get(), spRawBuffUAV[1].RawPtr());
            rwBuff_Static->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwBuff_Static->Get(), nullptr);

            rwBuff_Static->Set(spRawBuffUAV[0]);
            EXPECT_EQ(rwBuff_Static->Get(), spRawBuffUAV[0].RawPtr());
        }


        {
            auto tex2D_Mut = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut");
            EXPECT_EQ(tex2D_Mut, nullptr);
        }
        {
            auto tex2D_Dyn = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn");
            EXPECT_EQ(tex2D_Dyn, nullptr);
        }
        {
            auto tex2D_Mut_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut_sampler");
            EXPECT_EQ(tex2D_Mut_sampler, nullptr);
        }
        {
            auto tex2D_Dyn_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr_sampler");
            EXPECT_EQ(tex2D_Dyn_sampler, nullptr);
        }


        auto NumPSVars = pTestPSO->GetStaticVariableCount(SHADER_TYPE_PIXEL);
        for (Uint32 v = 0; v < NumPSVars; ++v)
        {
            auto pVar = pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_PIXEL, v);
            EXPECT_EQ(pVar->GetIndex(), v);
            EXPECT_EQ(pVar->GetType(), SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
            ShaderResourceDesc ResDesc;
            pVar->GetResourceDesc(ResDesc);
            auto pVar2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name);
            EXPECT_EQ(pVar, pVar2);
        }
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB, false);
    ASSERT_NE(pSRB, nullptr);
    EXPECT_EQ(TestShaderResourceBindingCInterface(pSRB.RawPtr()), 0);

    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_GEOMETRY), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_HULL), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_DOMAIN), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_COMPUTE), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_AMPLIFICATION), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_MESH), 0u);

    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_GEOMETRY, "g_tex2D_Mut"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_HULL, "g_tex2D_Mut"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_DOMAIN, "g_tex2D_Mut"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex2D_Mut"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_AMPLIFICATION, "g_tex2D_Mut"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_MESH, "g_tex2D_Mut"), nullptr);

    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_GEOMETRY, 0), nullptr);
    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_HULL, 0), nullptr);
    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_DOMAIN, 0), nullptr);
    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_COMPUTE, 0), nullptr);
    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_AMPLIFICATION, 0), nullptr);
    EXPECT_EQ(pSRB->GetVariableByIndex(SHADER_TYPE_MESH, 0), nullptr);

    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_UPDATE_STATIC | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_UPDATE_MUTABLE | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUTABLE);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_UPDATE_DYNAMIC | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_DYNAMIC);

    {
        {
            auto tex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
            ASSERT_NE(tex2D_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Mut, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            tex2D_Mut->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Mut->Get(), pSRVs[0]);

            tex2D_Mut->Set(pSRVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Mut->Get(), pSRVs[1]);
            tex2D_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Mut->Get(), nullptr);

            tex2D_Mut->Set(pSRVs[0]);
            EXPECT_EQ(tex2D_Mut->Get(), pSRVs[0]);
        }

        {
            auto tex2D_Mut_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut_sampler");
            EXPECT_EQ(tex2D_Mut_sampler, nullptr);
        }

        {
            auto tex2D_MutArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_MutArr");
            ASSERT_NE(tex2D_MutArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_MutArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_MutArr, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            tex2D_MutArr->SetArray(pSRVs, 0, 2);
            EXPECT_EQ(tex2D_MutArr->Get(0), pSRVs[0]);
            EXPECT_EQ(tex2D_MutArr->Get(1), pSRVs[1]);
        }

        {
            auto tex2D_MutArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_MutArr_sampler");
            EXPECT_EQ(tex2D_MutArr_sampler, nullptr);
        }

        {
            auto tex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn");
            ASSERT_NE(tex2D_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Dyn, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            //tex2D_Dyn->Set(pSRVs[0]);
            EXPECT_EQ(TestShaderResourceVariableCInterface(tex2D_Dyn, pSRVs[0]), 0);
        }

        {
            auto tex2D_Dyn_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn_sampler");
            EXPECT_EQ(tex2D_Dyn_sampler, nullptr);
        }

        {
            auto tex2D_DynArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr");
            ASSERT_NE(tex2D_DynArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_DynArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_DynArr, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            tex2D_DynArr->SetArray(pSRVs, 0, 2);
            EXPECT_EQ(tex2D_DynArr->Get(0), pSRVs[0]);
            EXPECT_EQ(tex2D_DynArr->Get(1), pSRVs[1]);
        }

        {
            auto tex2D_DynArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr_sampler");
            EXPECT_EQ(tex2D_DynArr_sampler, nullptr);
        }

        {
            auto UniformBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Mut");
            ASSERT_NE(UniformBuff_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Mut, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            UniformBuff_Mut->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Mut->Get(), pUBs[0]);

            UniformBuff_Mut->Set(pUBs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Mut->Get(), pUBs[1]);
            UniformBuff_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Mut->Get(), nullptr);

            UniformBuff_Mut->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Mut->Get(), pUBs[0]);
        }

        {
            auto UniformBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Dyn");
            ASSERT_NE(UniformBuff_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Dyn, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            UniformBuff_Dyn->Set(pUBs[1]);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), pUBs[1]);
            UniformBuff_Dyn->Set(nullptr);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), nullptr);
            UniformBuff_Dyn->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), pUBs[0]);
        }

        {
            auto Buffer_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Mut");
            ASSERT_NE(Buffer_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Mut, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_Mut->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Mut->Get(0), pFormattedBuffSRV);

            Buffer_Mut->Set(pFormattedBuffSRVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Mut->Get(0), pFormattedBuffSRVs[1]);
            Buffer_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Mut->Get(0), nullptr);

            Buffer_Mut->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Mut->Get(0), pFormattedBuffSRV);
        }

        {
            auto Buffer_MutArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_MutArr");
            ASSERT_NE(Buffer_MutArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_MutArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_MutArr, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_MutArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_MutArr->Get(1), pFormattedBuffSRV);
        }

        {
            auto Buffer_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Dyn");
            ASSERT_NE(Buffer_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Dyn, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_Dyn->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Dyn->Get(0), pFormattedBuffSRV);
            Buffer_Dyn->Set(nullptr);
            EXPECT_EQ(Buffer_Dyn->Get(0), nullptr);
            Buffer_Dyn->Set(pFormattedBuffSRV);
            EXPECT_EQ(Buffer_Dyn->Get(0), pFormattedBuffSRV);
        }

        {
            auto Buffer_DynArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_DynArr");
            ASSERT_NE(Buffer_DynArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_DynArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_DynArr, pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name));
            Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_DynArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_DynArr->Get(1), pFormattedBuffSRV);
        }

        {
            auto tex2D_Static = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static");
            EXPECT_EQ(tex2D_Static, nullptr);
        }

        auto UniformBuff_Stat = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
        EXPECT_EQ(UniformBuff_Stat, nullptr);
    }


    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX, nullptr, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX, nullptr, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN);

    {
        {
            auto tex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut");
            ASSERT_NE(tex2D_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Mut, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_Mut->Set(pRWTexSRVs[2]);
            EXPECT_EQ(tex2D_Mut->Get(0), pRWTexSRVs[2]);

            tex2D_Mut->Set(pRWTexSRVs[3], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Mut->Get(0), pRWTexSRVs[3]);
            tex2D_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(tex2D_Mut->Get(0), nullptr);

            tex2D_Mut->Set(pRWTexSRVs[4]);
            EXPECT_EQ(tex2D_Mut->Get(0), pRWTexSRVs[4]);
        }

        {
            auto tex2D_Mut_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut_sampler");
            EXPECT_EQ(tex2D_Mut_sampler, nullptr);
        }

        {
            auto tex2D_MutArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_MutArr");
            ASSERT_NE(tex2D_MutArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_MutArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_MutArr, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_MutArr->SetArray(pRWTexSRVs + 5, 0, 2);
            EXPECT_EQ(tex2D_MutArr->Get(0), pRWTexSRVs[5]);
        }

        {
            auto tex2D_MutArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_MutArr_sampler");
            EXPECT_EQ(tex2D_MutArr_sampler, nullptr);
        }

        {
            auto tex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn");
            ASSERT_NE(tex2D_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(tex2D_Dyn, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_Dyn->Set(pRWTexSRVs[6]);
            EXPECT_EQ(tex2D_Dyn->Get(0), pRWTexSRVs[6]);
            tex2D_Dyn->Set(nullptr);
            EXPECT_EQ(tex2D_Dyn->Get(0), nullptr);
            tex2D_Dyn->Set(pRWTexSRVs[7]);
            EXPECT_EQ(tex2D_Dyn->Get(0), pRWTexSRVs[7]);
        }

        {
            auto tex2D_Dyn_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn_sampler");
            EXPECT_EQ(tex2D_Dyn_sampler, nullptr);
        }

        {
            auto tex2D_DynArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr");
            ASSERT_NE(tex2D_DynArr, nullptr);
            ShaderResourceDesc ResDesc;
            tex2D_DynArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(tex2D_DynArr, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            tex2D_DynArr->SetArray(pSRVs, 0, 2);
            EXPECT_EQ(tex2D_DynArr->Get(0), pSRVs[0]);
            EXPECT_EQ(tex2D_DynArr->Get(1), pSRVs[1]);
        }

        {
            auto tex2D_DynArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr_sampler");
            EXPECT_EQ(tex2D_DynArr_sampler, nullptr);
        }


        {
            auto UniformBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Mut");
            ASSERT_NE(UniformBuff_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Mut, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            UniformBuff_Mut->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Mut->Get(0), pUBs[0]);

            UniformBuff_Mut->Set(pUBs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Mut->Get(), pUBs[1]);
            UniformBuff_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(UniformBuff_Mut->Get(), nullptr);

            UniformBuff_Mut->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Mut->Get(), pUBs[0]);
        }

        {
            auto UniformBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Dyn");
            ASSERT_NE(UniformBuff_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            UniformBuff_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(UniformBuff_Dyn, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            UniformBuff_Dyn->Set(pUBs[1]);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), pUBs[1]);
            UniformBuff_Dyn->Set(nullptr);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), nullptr);
            UniformBuff_Dyn->Set(pUBs[0]);
            EXPECT_EQ(UniformBuff_Dyn->Get(0), pUBs[0]);
        }

        {
            auto Buffer_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Mut");
            ASSERT_NE(Buffer_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Mut, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_Mut->Set(spRawBuffSRVs[1]);
            EXPECT_EQ(Buffer_Mut->Get(0), spRawBuffSRVs[1].RawPtr());

            Buffer_Mut->Set(spRawBuffSRVs[0], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Mut->Get(0), spRawBuffSRVs[0].RawPtr());
            Buffer_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(Buffer_Mut->Get(0), nullptr);

            Buffer_Mut->Set(spRawBuffSRVs[1]);
            EXPECT_EQ(Buffer_Mut->Get(0), spRawBuffSRVs[1].RawPtr());
        }

        {
            auto Buffer_MutArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_MutArr");
            ASSERT_NE(Buffer_MutArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_MutArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_MutArr, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_MutArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_MutArr->Get(1), pFormattedBuffSRV);
        }

        {
            auto Buffer_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Dyn");
            ASSERT_NE(Buffer_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(Buffer_Dyn, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_Dyn->Set(pFormattedBuffSRVs[2]);
            EXPECT_EQ(Buffer_Dyn->Get(0), pFormattedBuffSRVs[2]);
            Buffer_Dyn->Set(nullptr);
            EXPECT_EQ(Buffer_Dyn->Get(0), nullptr);
            Buffer_Dyn->Set(pFormattedBuffSRVs[3]);
            EXPECT_EQ(Buffer_Dyn->Get(0), pFormattedBuffSRVs[3]);
        }

        {
            auto Buffer_DynArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_DynArr");
            ASSERT_NE(Buffer_DynArr, nullptr);
            ShaderResourceDesc ResDesc;
            Buffer_DynArr->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 2u);
            EXPECT_EQ(Buffer_DynArr, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
            Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);
            EXPECT_EQ(Buffer_DynArr->Get(0), pFormattedBuffSRV);
            EXPECT_EQ(Buffer_DynArr->Get(1), pFormattedBuffSRV);
        }

        {
            auto rwtex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Mut");
            ASSERT_NE(rwtex2D_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            rwtex2D_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwtex2D_Mut, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwtex2D_Mut->Set(pTexUAVs[0]);
            EXPECT_EQ(rwtex2D_Mut->Get(0), pTexUAVs[0]);

            rwtex2D_Mut->Set(pTexUAVs[1], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwtex2D_Mut->Get(0), pTexUAVs[1]);
            rwtex2D_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwtex2D_Mut->Get(0), nullptr);

            rwtex2D_Mut->Set(pTexUAVs[2]);
            EXPECT_EQ(rwtex2D_Mut->Get(0), pTexUAVs[2]);
        }

        {
            auto rwtex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn");
            ASSERT_NE(rwtex2D_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            rwtex2D_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwtex2D_Dyn, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwtex2D_Dyn->Set(pTexUAVs[2]);
            EXPECT_EQ(rwtex2D_Dyn->Get(0), pTexUAVs[2]);
            rwtex2D_Dyn->Set(nullptr);
            EXPECT_EQ(rwtex2D_Dyn->Get(0), nullptr);
            rwtex2D_Dyn->Set(pTexUAVs[3]);
            EXPECT_EQ(rwtex2D_Dyn->Get(0), pTexUAVs[3]);
        }

        {
            auto rwBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Mut");
            ASSERT_NE(rwBuff_Mut, nullptr);
            ShaderResourceDesc ResDesc;
            rwBuff_Mut->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwBuff_Mut, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwBuff_Mut->Set(pFormattedBuffUAV[1]);
            EXPECT_EQ(rwBuff_Mut->Get(0), pFormattedBuffUAV[1]);

            rwBuff_Mut->Set(pFormattedBuffUAV[0], SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwBuff_Mut->Get(0), pFormattedBuffUAV[0]);
            rwBuff_Mut->Set(nullptr, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            EXPECT_EQ(rwBuff_Mut->Get(0), nullptr);

            rwBuff_Mut->Set(pFormattedBuffUAV[1]);
            EXPECT_EQ(rwBuff_Mut->Get(0), pFormattedBuffUAV[1]);
        }

        {
            auto rwBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn");
            ASSERT_NE(rwBuff_Dyn, nullptr);
            ShaderResourceDesc ResDesc;
            rwBuff_Dyn->GetResourceDesc(ResDesc);
            EXPECT_EQ(ResDesc.ArraySize, 1u);
            EXPECT_EQ(rwBuff_Dyn, pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name));
            rwBuff_Dyn->Set(pFormattedBuffUAV[1]);
            EXPECT_EQ(rwBuff_Dyn->Get(0), pFormattedBuffUAV[1]);
            rwBuff_Dyn->Set(nullptr);
            EXPECT_EQ(rwBuff_Dyn->Get(0), nullptr);
            rwBuff_Dyn->Set(pFormattedBuffUAV[2]);
            EXPECT_EQ(rwBuff_Dyn->Get(0), pFormattedBuffUAV[2]);
        }

        {
            auto tex2D_Static = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static");
            EXPECT_EQ(tex2D_Static, nullptr);
        }

        auto UniformBuff_Stat = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat");
        EXPECT_EQ(UniformBuff_Stat, nullptr);
    }



    {
        auto NumVSVars = pSRB->GetVariableCount(SHADER_TYPE_VERTEX);
        for (Uint32 v = 0; v < NumVSVars; ++v)
        {
            auto pVar = pSRB->GetVariableByIndex(SHADER_TYPE_VERTEX, v);
            EXPECT_EQ(pVar->GetIndex(), v);
            EXPECT_TRUE(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
            ShaderResourceDesc ResDesc;
            pVar->GetResourceDesc(ResDesc);
            auto pVar2 = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, ResDesc.Name);
            EXPECT_EQ(pVar, pVar2);
        }
    }

    {
        auto NumPSVars = pSRB->GetVariableCount(SHADER_TYPE_PIXEL);
        for (Uint32 v = 0; v < NumPSVars; ++v)
        {
            auto pVar = pSRB->GetVariableByIndex(SHADER_TYPE_PIXEL, v);
            EXPECT_EQ(pVar->GetIndex(), v);
            EXPECT_TRUE(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
            ShaderResourceDesc ResDesc;
            pVar->GetResourceDesc(ResDesc);
            auto pVar2 = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, ResDesc.Name);
            EXPECT_EQ(pVar, pVar2);
        }
    }

    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE);
    EXPECT_EQ(pSRB->CheckResources(SHADER_TYPE_PIXEL, nullptr, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED), SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE);

    pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pTestPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn")->Set(pTexUAVs[7]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pRWTexSRVs[3]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn")->Set(pFormattedBuffUAV[3]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Dyn")->Set(pFormattedBuffSRVs[2]);

    {
        LOG_INFO_MESSAGE("No worries about 3 warnings below: testing accessing variables from inactive shader stage");
        auto pNonExistingVar = pSRB->GetVariableByName(SHADER_TYPE_GEOMETRY, "g_NonExistingVar");
        EXPECT_EQ(pNonExistingVar, nullptr);
        pNonExistingVar = pSRB->GetVariableByIndex(SHADER_TYPE_GEOMETRY, 4);
        EXPECT_EQ(pNonExistingVar, nullptr);
        EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_GEOMETRY), 0u);
    }

    float Zero[4] = {};
    pContext->ClearRenderTarget(pRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->Draw(DrawAttrs);
}


TEST(ShaderResourceLayout, NoResourcesPSO)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pContext   = pEnv->GetDeviceContext();
    auto* const pSwapChain = pEnv->GetSwapChain();

    constexpr char   DummyVS[] = R"(
float4 main() : SV_Position
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";
    constexpr char   DummyPS[] = R"(
float4 main() : SV_Target
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";
    ShaderCreateInfo ShaderCI;
    ShaderCI.EntryPoint     = "main";
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    RefCntAutoPtr<IShader> pVS;
    ShaderCI.Desc   = {"DummyVS", SHADER_TYPE_VERTEX, true};
    ShaderCI.Source = DummyVS;
    pDevice->CreateShader(ShaderCI, &pVS);
    ASSERT_NE(pVS, nullptr);

    RefCntAutoPtr<IShader> pPS;
    ShaderCI.Desc   = {"DummyPS", SHADER_TYPE_PIXEL, true};
    ShaderCI.Source = DummyPS;
    pDevice->CreateShader(ShaderCI, &pPS);
    ASSERT_NE(pPS, nullptr);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name                     = "No resources PSO";
    PSODesc.SRBAllocationGranularity = 16;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    EXPECT_EQ(pPSO->GetStaticVariableCount(SHADER_TYPE_VERTEX), 0u);
    EXPECT_EQ(pPSO->GetStaticVariableCount(SHADER_TYPE_PIXEL), 0u);
    EXPECT_EQ(pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "NonexistentResource"), nullptr);
    EXPECT_EQ(pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "NonexistentResource"), nullptr);
    EXPECT_EQ(pPSO->GetStaticVariableByIndex(SHADER_TYPE_VERTEX, 0), nullptr);
    EXPECT_EQ(pPSO->GetStaticVariableByIndex(SHADER_TYPE_PIXEL, 0), nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);

    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_VERTEX), 0u);
    EXPECT_EQ(pSRB->GetVariableCount(SHADER_TYPE_PIXEL), 0u);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "NonexistentResource"), nullptr);
    EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "NonexistentResource"), nullptr);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float ClearColor[] = {0.125, 0.375, 0.125, 0.75};
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL});
}

} // namespace
