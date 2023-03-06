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
#include "GraphicsAccessories.hpp"

#include <utility>

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

#ifdef DILIGENT_DEVELOPMENT

RefCntAutoPtr<IShader> CreateShader(const char*            Name,
                                    const char*            Source,
                                    SHADER_TYPE            ShaderType,
                                    bool                   UseCombinedSamplers = true,
                                    SHADER_SOURCE_LANGUAGE Language            = SHADER_SOURCE_LANGUAGE_HLSL)
{
    ShaderCreateInfo ShaderCI;
    ShaderCI.EntryPoint     = "main";
    ShaderCI.SourceLanguage = Language;
    ShaderCI.Source         = Source;

    ShaderCI.Desc = {Name, ShaderType, UseCombinedSamplers};

    RefCntAutoPtr<IShader> pShader;
    GPUTestingEnvironment::GetInstance()->GetDevice()->CreateShader(ShaderCI, &pShader);
    return pShader;
}


void DrawWithNullResources(IShader* pVS, IShader* pPS, SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    auto* const pContext   = pEnv->GetDeviceContext();
    auto* const pSwapChain = pEnv->GetSwapChain();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name                                  = "Null resource test PSO";
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSODesc.ResourceLayout.DefaultVariableType = VarType;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, false);

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttribs{0, DRAW_FLAG_VERIFY_ALL});
}

void DispatchWithNullResources(IShader* pCS, SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name                               = "Null resource test PSO";
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = VarType;
    PSOCreateInfo.pCS                                        = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, false);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->DispatchCompute(DispatchComputeAttribs{0, 0, 0});
}


class NullConstantBuffer : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullConstantBufferVS[] = R"(
cbuffer MissingVSBuffer
{
    float4 g_f4Position;
}
float4 main() : SV_Position
{
    return g_f4Position;
}
)";

        constexpr char NullConstantBufferPS[] = R"(
cbuffer MissingPSBuffer
{
    float4 g_f4Color;
}
float4 main() : SV_Target
{
    return g_f4Color;
}
)";

        pVS = CreateShader("Null CB binding VS", NullConstantBufferVS, SHADER_TYPE_VERTEX);
        pPS = CreateShader("Null CB binding PS", NullConstantBufferPS, SHADER_TYPE_PIXEL);
    }

    static void TearDownTestSuite()
    {
        pVS.Release();
        pPS.Release();
    }
    static RefCntAutoPtr<IShader> pVS;
    static RefCntAutoPtr<IShader> pPS;
};
RefCntAutoPtr<IShader> NullConstantBuffer::pVS;
RefCntAutoPtr<IShader> NullConstantBuffer::pPS;

TEST_P(NullConstantBuffer, Test)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
    const auto  VarType    = GetParam();

    if (!DeviceInfo.Features.SeparablePrograms)
        GTEST_SKIP() << "Separable programs are required";

    pEnv->SetErrorAllowance(2, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'MissingPSBuffer'");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'MissingVSBuffer'", false);

    DrawWithNullResources(pVS, pPS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullConstantBuffer,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //




class NullStructBuffer : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullStructBufferVS[] = R"(
struct BufferData
{
    float4 Data;
};
StructuredBuffer<BufferData> g_MissingVSStructBuffer;
float4 main() : SV_Position
{
    return g_MissingVSStructBuffer[0].Data;
}
)";

        constexpr char NullStructBufferPS[] = R"(
struct BufferData
{
    float4 Data;
};
StructuredBuffer<BufferData> g_MissingPSStructBuffer;
float4 main() : SV_Target
{
    return g_MissingPSStructBuffer[0].Data;
}
)";

        pVS = CreateShader("Null struct buffer binding VS", NullStructBufferVS, SHADER_TYPE_VERTEX);
        pPS = CreateShader("Null struct buffer binding PS", NullStructBufferPS, SHADER_TYPE_PIXEL);
    }

    static void TearDownTestSuite()
    {
        pVS.Release();
        pPS.Release();
    }
    static RefCntAutoPtr<IShader> pVS;
    static RefCntAutoPtr<IShader> pPS;
};
RefCntAutoPtr<IShader> NullStructBuffer::pVS;
RefCntAutoPtr<IShader> NullStructBuffer::pPS;

TEST_P(NullStructBuffer, Test)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
    const auto  VarType    = GetParam();

    if (!DeviceInfo.Features.SeparablePrograms)
        GTEST_SKIP() << "Separable programs are required";

    pEnv->SetErrorAllowance(2, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingPSStructBuffer'");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingVSStructBuffer'", false);

    DrawWithNullResources(pVS, pPS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullStructBuffer,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //



class NullFormattedBuffer : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullFormattedBufferVS[] = R"(
Buffer<float4> g_MissingVSFmtBuffer;
float4 main() : SV_Position
{
    return g_MissingVSFmtBuffer.Load(0);
}
)";

        constexpr char NullFormattedBufferPS[] = R"(
Buffer<float4> g_MissingPSFmtBuffer;
float4 main() : SV_Target
{
    return g_MissingPSFmtBuffer.Load(0);
}
)";

        pVS = CreateShader("Null formatted buffer binding VS", NullFormattedBufferVS, SHADER_TYPE_VERTEX);
        pPS = CreateShader("Null formatted buffer binding PS", NullFormattedBufferPS, SHADER_TYPE_PIXEL);
    }

    static void TearDownTestSuite()
    {
        pVS.Release();
        pPS.Release();
    }
    static RefCntAutoPtr<IShader> pVS;
    static RefCntAutoPtr<IShader> pPS;
};
RefCntAutoPtr<IShader> NullFormattedBuffer::pVS;
RefCntAutoPtr<IShader> NullFormattedBuffer::pPS;

TEST_P(NullFormattedBuffer, Test)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
    const auto  VarType    = GetParam();

    if (!DeviceInfo.Features.SeparablePrograms)
        GTEST_SKIP() << "Separable programs are required";

    pEnv->SetErrorAllowance(2, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingPSFmtBuffer'");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingVSFmtBuffer'", false);

    DrawWithNullResources(pVS, pPS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullFormattedBuffer,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //




class NullTexture : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullTextureVS[] = R"(
Texture2D<float4> g_MissingVSTexture;
float4 main() : SV_Position
{
    return g_MissingVSTexture.Load(int3(0,0,0));
}
)";

        constexpr char NullTexturePS[] = R"(
Texture2D<float4> g_MissingPSTexture;
float4 main() : SV_Target
{
    return g_MissingPSTexture.Load(int3(0,0,0));
}
)";

        pVS = CreateShader("Null texture binding VS", NullTextureVS, SHADER_TYPE_VERTEX);
        pPS = CreateShader("Null texture binding PS", NullTexturePS, SHADER_TYPE_PIXEL);
    }

    static void TearDownTestSuite()
    {
        pVS.Release();
        pPS.Release();
    }

    static RefCntAutoPtr<IShader> pVS;
    static RefCntAutoPtr<IShader> pPS;
};

RefCntAutoPtr<IShader> NullTexture::pVS;
RefCntAutoPtr<IShader> NullTexture::pPS;

TEST_P(NullTexture, Test)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
    const auto  VarType    = GetParam();

    if (!DeviceInfo.Features.SeparablePrograms)
        GTEST_SKIP() << "Separable programs are required";

    pEnv->SetErrorAllowance(2, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingPSTexture'");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingVSTexture'", false);

    DrawWithNullResources(pVS, pPS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullTexture,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //




class NullSampler : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullSamplerVS[] = R"(
Texture2D<float4> g_MissingVSTexture;
SamplerState      g_MissingVSSampler;
float4 main() : SV_Position
{
    return g_MissingVSTexture.SampleLevel(g_MissingVSSampler, float2(0.0, 0.0), 0);
}
)";

        constexpr char NullSamplerPS[] = R"(
Texture2D<float4> g_MissingPSTexture;
SamplerState      g_MissingPSSampler;
float4 main() : SV_Target
{
    return g_MissingPSTexture.Sample(g_MissingPSSampler, float2(0.0, 0.0));
}
)";

        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
        if (!DeviceInfo.IsGLDevice())
        {
            pVS = CreateShader("Null texture binding VS", NullSamplerVS, SHADER_TYPE_VERTEX, false);
            pPS = CreateShader("Null texture binding PS", NullSamplerPS, SHADER_TYPE_PIXEL, false);
        }
    }

    static void TearDownTestSuite()
    {
        pVS.Release();
        pPS.Release();
    }

    static RefCntAutoPtr<IShader> pVS;
    static RefCntAutoPtr<IShader> pPS;
};

RefCntAutoPtr<IShader> NullSampler::pVS;
RefCntAutoPtr<IShader> NullSampler::pPS;

TEST_P(NullSampler, Test)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();
    const auto  VarType    = GetParam();

    if (DeviceInfo.IsGLDevice())
        GTEST_SKIP() << "Separate samplers are not supported in GL";

    pEnv->SetErrorAllowance(4, "No worries, errors are expected: testing null resource bindings\n");
    const char* Messages[] = //
        {
            "No resource is bound to variable 'g_MissingPSTexture'",
            "No resource is bound to variable 'g_MissingPSSampler'",
            "No resource is bound to variable 'g_MissingVSTexture'",
            "No resource is bound to variable 'g_MissingVSSampler'" //
        };
    size_t MsgOrder[] = {0, 1, 2, 3};
    if (DeviceInfo.IsMetalDevice())
    {
        std::swap(MsgOrder[0], MsgOrder[1]);
        std::swap(MsgOrder[2], MsgOrder[3]);
    }
    for (size_t i = 0; i < _countof(MsgOrder); ++i)
        pEnv->PushExpectedErrorSubstring(Messages[MsgOrder[i]], i == 0);

    DrawWithNullResources(pVS, pPS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullSampler,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //



class NullRWTexture : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullRWTextureCS[] = R"(
RWTexture2D<float4 /*format=rgba32f*/> g_MissingRWTexture;
[numthreads(1, 1, 1)]
void main()
{
    g_MissingRWTexture[int2(0, 0)] = float4(0.0, 0.0, 0.0, 0.0);
}
)";

        pCS = CreateShader("Null RW texture binding CS", NullRWTextureCS, SHADER_TYPE_COMPUTE);
    }

    static void TearDownTestSuite()
    {
        pCS.Release();
    }
    static RefCntAutoPtr<IShader> pCS;
};
RefCntAutoPtr<IShader> NullRWTexture::pCS;

TEST_P(NullRWTexture, Test)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    const auto  VarType = GetParam();

    pEnv->SetErrorAllowance(1, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingRWTexture'", false);

    DispatchWithNullResources(pCS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullRWTexture,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //



class NullRWFmtBuffer : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullRWFmtBufferCS[] = R"(
RWBuffer<float4 /*format=rgba32f*/> g_MissingRWBuffer;
[numthreads(1, 1, 1)]
void main()
{
    g_MissingRWBuffer[0] = float4(0.0, 0.0, 0.0, 0.0);
}
)";

        pCS = CreateShader("Null RW fmt buffer binding CS", NullRWFmtBufferCS, SHADER_TYPE_COMPUTE);
    }

    static void TearDownTestSuite()
    {
        pCS.Release();
    }
    static RefCntAutoPtr<IShader> pCS;
};
RefCntAutoPtr<IShader> NullRWFmtBuffer::pCS;

TEST_P(NullRWFmtBuffer, Test)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    const auto  VarType = GetParam();

    pEnv->SetErrorAllowance(1, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingRWBuffer'", false);

    DispatchWithNullResources(pCS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullRWFmtBuffer,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //



class NullRWStructBuffer : public testing::TestWithParam<SHADER_RESOURCE_VARIABLE_TYPE>
{
protected:
    static void SetUpTestSuite()
    {
        constexpr char NullRWFmtBufferCS_HLSL[] = R"(
struct Data
{
    float4 Data;
};
RWStructuredBuffer<Data> g_MissingRWStructBuffer;
[numthreads(1, 1, 1)]
void main()
{
    g_MissingRWStructBuffer[0].Data = float4(0.0, 0.0, 0.0, 0.0);
}
)";

        constexpr char NullRWFmtBufferCS_GLSL[] = R"(
layout(std140, binding = 0) buffer g_MissingRWStructBuffer
{
    vec4 data[4];
}g_StorageBuff;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    g_StorageBuff.data[0] = vec4(0.0, 0.0, 0.0, 0.0);
}
)";

        auto* const pEnv       = GPUTestingEnvironment::GetInstance();
        const auto& DeviceInfo = pEnv->GetDevice()->GetDeviceInfo();

        pCS = CreateShader("Null RW struct buffer binding CS",
                           DeviceInfo.IsGLDevice() ? NullRWFmtBufferCS_GLSL : NullRWFmtBufferCS_HLSL,
                           SHADER_TYPE_COMPUTE,
                           true, // UseCombinedSamplers
                           DeviceInfo.IsGLDevice() ? SHADER_SOURCE_LANGUAGE_GLSL : SHADER_SOURCE_LANGUAGE_HLSL);
    }

    static void TearDownTestSuite()
    {
        pCS.Release();
    }
    static RefCntAutoPtr<IShader> pCS;
};
RefCntAutoPtr<IShader> NullRWStructBuffer::pCS;

TEST_P(NullRWStructBuffer, Test)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    const auto  VarType = GetParam();

    pEnv->SetErrorAllowance(1, "No worries, errors are expected: testing null resource bindings\n");
    pEnv->PushExpectedErrorSubstring("No resource is bound to variable 'g_MissingRWStructBuffer'", false);

    DispatchWithNullResources(pCS, VarType);
}

INSTANTIATE_TEST_SUITE_P(NullResourceBindings,
                         NullRWStructBuffer,
                         testing::Values(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),
                         [](const testing::TestParamInfo<SHADER_RESOURCE_VARIABLE_TYPE>& info) //
                         {
                             return GetShaderVariableTypeLiteralName(info.param);
                         }); //



#endif // DILIGENT_DEVELOPMENT

} // namespace
