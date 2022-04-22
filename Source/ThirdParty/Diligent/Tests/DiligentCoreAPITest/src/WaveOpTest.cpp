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

#include "GPUTestingEnvironment.hpp"
#include "ShaderMacroHelper.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace Diligent
{

TEST(WaveOpTest, CompileShader_HLSL)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    auto* const pDevice = pEnv->GetDevice();

    if (!pDevice->GetDeviceInfo().Features.WaveOp)
    {
        GTEST_SKIP() << "Wave operations are not supported by this device";
    }
    if (!pEnv->HasDXCompiler())
    {
        GTEST_SKIP() << "HLSL source code with wave operations can be compiled only by DXC";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    const auto& WaveOpProps = pDevice->GetAdapterInfo().WaveOp;

    auto WaveOpFeatures = WaveOpProps.Features;

    Uint32 DxcMajorVer = 0;
    Uint32 DxcMinorVer = 0;
    pEnv->GetDXCompilerVersion(DxcMajorVer, DxcMinorVer);
    if (!(DxcMajorVer >= 2 || (DxcMajorVer == 1 && DxcMinorVer >= 5)))
    {
        // There is a bug in older versions of DXC that causes the following error:
        //      opcode 'QuadReadAcross' should only be used in 'Pixel Shader'
        WaveOpFeatures &= ~WAVE_FEATURE_QUAD;
    }

    ASSERT_NE(WaveOpFeatures, WAVE_FEATURE_UNKNOWN);
    ASSERT_TRUE((WaveOpFeatures & WAVE_FEATURE_BASIC) != 0);

    ASSERT_NE(WaveOpProps.SupportedStages, SHADER_TYPE_UNKNOWN);
    ASSERT_TRUE((WaveOpProps.SupportedStages & SHADER_TYPE_COMPUTE) != 0);

    ASSERT_GT(WaveOpProps.MinSize, 0u);
    ASSERT_GE(WaveOpProps.MaxSize, WaveOpProps.MinSize);

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("SUBGROUP_SIZE", WaveOpProps.MinSize);

    // clang-format off
    Macros.AddShaderMacro("WAVE_FEATURE_BASIC",      (WaveOpFeatures & WAVE_FEATURE_BASIC)      != 0);
    Macros.AddShaderMacro("WAVE_FEATURE_VOTE",       (WaveOpFeatures & WAVE_FEATURE_VOTE)       != 0);
    Macros.AddShaderMacro("WAVE_FEATURE_ARITHMETIC", (WaveOpFeatures & WAVE_FEATURE_ARITHMETIC) != 0);
    Macros.AddShaderMacro("WAVE_FEATURE_BALLOUT",    (WaveOpFeatures & WAVE_FEATURE_BALLOUT)    != 0);
    Macros.AddShaderMacro("WAVE_FEATURE_QUAD",       (WaveOpFeatures & WAVE_FEATURE_QUAD)       != 0);
    // clang-format on

    static const char Source[] = R"(
RWByteAddressBuffer g_RWBuffer;

[numthreads(SUBGROUP_SIZE, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
    uint Accum = 0;
    #if WAVE_FEATURE_BASIC
    {
        uint  laneCount = WaveGetLaneCount();
        uint  lineIndex = WaveGetLaneIndex();
        Accum += (lineIndex % laneCount);
    }
    #endif
    #if WAVE_FEATURE_VOTE
    {
        if (WaveActiveAllTrue(Accum > 0xFFFF))
            Accum += 1;
    }
    #endif
    #if WAVE_FEATURE_ARITHMETIC
    {
        uint sum = WaveActiveSum(DTid);
        Accum += (sum & 1);
    }
    #endif
    #if WAVE_FEATURE_BALLOUT
    {
        uint count = WaveActiveCountBits((DTid & 2) == 0);
        Accum += (count & 1);
    }
    #endif
    #if WAVE_FEATURE_QUAD
    {
        uint diag = QuadReadAcrossDiagonal(DTid);
        Accum += (diag & 1);
    }
    #endif

    g_RWBuffer.Store(DTid, Accum);
}
)";

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler  = SHADER_COMPILER_DXC;
    ShaderCI.HLSLVersion     = {6, 0};
    ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
    ShaderCI.Desc.Name       = "Wave op test - CS";
    ShaderCI.EntryPoint      = "main";
    ShaderCI.Source          = Source;
    ShaderCI.Macros          = Macros;

    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);


    ComputePipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Wave op test";
    PSOCreateInfo.pCS          = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);
}


TEST(WaveOpTest, CompileShader_GLSL)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    if (!DeviceInfo.IsVulkanDevice() && !DeviceInfo.IsGLDevice())
    {
        GTEST_SKIP();
    }
    if (!DeviceInfo.Features.WaveOp)
    {
        GTEST_SKIP() << "Wave operations are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    const auto& WaveOpProps = pDevice->GetAdapterInfo().WaveOp;

    ASSERT_NE(WaveOpProps.Features, WAVE_FEATURE_UNKNOWN);
    ASSERT_TRUE((WaveOpProps.Features & WAVE_FEATURE_BASIC) != 0);

    ASSERT_NE(WaveOpProps.SupportedStages, SHADER_TYPE_UNKNOWN);
    ASSERT_TRUE((WaveOpProps.SupportedStages & SHADER_TYPE_COMPUTE) != 0);

    ASSERT_GT(WaveOpProps.MinSize, 0u);
    ASSERT_GE(WaveOpProps.MaxSize, WaveOpProps.MinSize);

    std::stringstream ShaderSourceStream;
    ShaderSourceStream << "#version 450\n\n";
    ShaderSourceStream << "#define SUBGROUP_SIZE " << WaveOpProps.MinSize << "\n";

    // clang-format off
    ShaderSourceStream << "#define WAVE_FEATURE_BASIC "            << int{(WaveOpProps.Features & WAVE_FEATURE_BASIC)            != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_VOTE "             << int{(WaveOpProps.Features & WAVE_FEATURE_VOTE)             != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_ARITHMETIC "       << int{(WaveOpProps.Features & WAVE_FEATURE_ARITHMETIC)       != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_BALLOUT "          << int{(WaveOpProps.Features & WAVE_FEATURE_BALLOUT)          != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_SHUFFLE "          << int{(WaveOpProps.Features & WAVE_FEATURE_SHUFFLE)          != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_SHUFFLE_RELATIVE " << int{(WaveOpProps.Features & WAVE_FEATURE_SHUFFLE_RELATIVE) != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_CLUSTERED "        << int{(WaveOpProps.Features & WAVE_FEATURE_CLUSTERED)        != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_QUAD "             << int{(WaveOpProps.Features & WAVE_FEATURE_QUAD)             != 0} << "\n";
    // clang-format on

    static const char ShaderBody[] = R"(
#if WAVE_FEATURE_BASIC
#    extension GL_KHR_shader_subgroup_basic: enable
#endif
#if WAVE_FEATURE_VOTE
#    extension GL_KHR_shader_subgroup_vote: enable
#endif
#if WAVE_FEATURE_BALLOUT
#    extension GL_KHR_shader_subgroup_ballot: enable
#endif
#if WAVE_FEATURE_ARITHMETIC
#    extension GL_KHR_shader_subgroup_arithmetic: enable
#endif
#if WAVE_FEATURE_SHUFFLE
#    extension GL_KHR_shader_subgroup_shuffle: enable
#endif
#if WAVE_FEATURE_SHUFFLE_RELATIVE
#    extension GL_KHR_shader_subgroup_shuffle_relative: enable
#endif
#if WAVE_FEATURE_CLUSTERED
#    extension GL_KHR_shader_subgroup_clustered: enable
#endif
#if WAVE_FEATURE_QUAD
#    extension GL_KHR_shader_subgroup_quad: enable
#endif

layout(local_size_x = SUBGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std140) writeonly buffer WBuffer
{
    uint g_WBuffer[];
};

void main()
{
    const uint DTid = gl_LocalInvocationID.x;

    uint Accum = 0;
    #if WAVE_FEATURE_BASIC
    {
        uint  laneCount = gl_SubgroupSize;
        uint  lineIndex = gl_SubgroupInvocationID;
        Accum += (lineIndex % laneCount);
    }
    #endif
    #if WAVE_FEATURE_VOTE
    {
        if (subgroupAll(Accum > 0xFFFF))
            Accum += 1;
    }
    #endif
    #if WAVE_FEATURE_ARITHMETIC
    {
        uint sum = subgroupAdd(DTid);
        Accum += (sum & 1);
    }
    #endif
    #if WAVE_FEATURE_BALLOUT
    {
        uint count = subgroupBallotExclusiveBitCount(subgroupBallot((DTid & 1) == 0));
        Accum += (count & 1);
    }
    #endif
    #if WAVE_FEATURE_SHUFFLE
    {
        vec4 temp      = vec4(float(DTid));
        vec4 blendWith = subgroupShuffle(temp, (DTid + 5) & 7);
        Accum += (dot(blendWith, blendWith) < 0.0 ? 1 : 0);
    }
    #endif
    #if WAVE_FEATURE_SHUFFLE_RELATIVE
    {
        vec4 temp = vec4(float(DTid));
        for (uint i = 2; i < gl_SubgroupSize; i *= 2)
        {
            vec4 other = subgroupShuffleUp(temp, i);

            if (i <= gl_SubgroupInvocationID)
                temp = temp * other;
        }
        Accum += (dot(temp, temp) > 0.5 ? 1 : 0);
    }
    #endif
    #if WAVE_FEATURE_CLUSTERED
    {
        uint maxId = subgroupClusteredMax(DTid, 4*4);
        Accum += (maxId+1 == SUBGROUP_SIZE ? 1 : 0);
    }
    #endif
    #if WAVE_FEATURE_QUAD
    {
        uint diag = subgroupQuadSwapDiagonal(DTid);
        Accum += (diag & 1);
    }
    #endif

    g_WBuffer[DTid] = Accum;
}
)";
    ShaderSourceStream << ShaderBody;
    const String Source = ShaderSourceStream.str();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
    ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
    ShaderCI.Desc.Name       = "Wave op test - CS";
    ShaderCI.EntryPoint      = "main";
    ShaderCI.Source          = Source.c_str();

    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);


    ComputePipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Wave op test";
    PSOCreateInfo.pCS          = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);
}


TEST(WaveOpTest, CompileShader_MSL)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    if (!DeviceInfo.IsMetalDevice())
    {
        GTEST_SKIP();
    }
    if (!DeviceInfo.Features.WaveOp)
    {
        GTEST_SKIP() << "Wave operations are not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    const auto& WaveOpProps = pDevice->GetAdapterInfo().WaveOp;

    ASSERT_NE(WaveOpProps.Features, WAVE_FEATURE_UNKNOWN);
    ASSERT_TRUE((WaveOpProps.Features & WAVE_FEATURE_BASIC) != 0);

    ASSERT_NE(WaveOpProps.SupportedStages, SHADER_TYPE_UNKNOWN);
    ASSERT_TRUE((WaveOpProps.SupportedStages & SHADER_TYPE_COMPUTE) != 0);

    ASSERT_GT(WaveOpProps.MinSize, 0u);
    ASSERT_GE(WaveOpProps.MaxSize, WaveOpProps.MinSize);

    std::stringstream ShaderSourceStream;
    // clang-format off
    ShaderSourceStream << "#define WAVE_FEATURE_BASIC "            << int{(WaveOpProps.Features & WAVE_FEATURE_BASIC)            != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_VOTE "             << int{(WaveOpProps.Features & WAVE_FEATURE_VOTE)             != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_ARITHMETIC "       << int{(WaveOpProps.Features & WAVE_FEATURE_ARITHMETIC)       != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_BALLOUT "          << int{(WaveOpProps.Features & WAVE_FEATURE_BALLOUT)          != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_SHUFFLE "          << int{(WaveOpProps.Features & WAVE_FEATURE_SHUFFLE)          != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_SHUFFLE_RELATIVE " << int{(WaveOpProps.Features & WAVE_FEATURE_SHUFFLE_RELATIVE) != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_CLUSTERED "        << int{(WaveOpProps.Features & WAVE_FEATURE_CLUSTERED)        != 0} << "\n";
    ShaderSourceStream << "#define WAVE_FEATURE_QUAD "             << int{(WaveOpProps.Features & WAVE_FEATURE_QUAD)             != 0} << "\n";
    // clang-format on

    static const char ShaderBody[] = R"(
#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_simdgroup>
using namespace metal;

kernel void CSMain(
#if WAVE_FEATURE_BASIC
    uint LaneIndex  [[thread_index_in_simdgroup]],
    uint WaveSize   [[threads_per_simdgroup]],
#endif
#if WAVE_FEATURE_QUAD
    uint QuadId     [[thread_index_in_quadgroup]],
#endif
    device uint* g_WBuffer [[buffer(0)]],
    uint         DTid      [[thread_index_in_threadgroup]]
)
{
    uint Accum = 0;
    #if WAVE_FEATURE_BASIC
    {
        Accum += (LaneIndex % WaveSize);
    }
    #endif
    #if WAVE_FEATURE_VOTE
    {
        if (simd_all(Accum > 0xFFFF))
            Accum += 1;
    }
    #endif
    #if WAVE_FEATURE_ARITHMETIC
    {
        uint sum = simd_sum(DTid);
        Accum += (sum & 1);
    }
    #endif
    #if WAVE_FEATURE_BALLOUT
    {
        float val = simd_broadcast(float(DTid) * 0.1f, ushort(LaneIndex));
        Accum += (val > 3.5f);
    }
    #endif
    #if WAVE_FEATURE_SHUFFLE
    {
        float4 temp      = float4(float(DTid));
        float4 blendWith = simd_shuffle(temp, ushort((DTid + 5) & 7));
        Accum += (dot(blendWith, blendWith) < 0.0 ? 1 : 0);
    }
    #endif
    #if WAVE_FEATURE_SHUFFLE_RELATIVE
    {
        float4 temp = float4(float(DTid));
        for (uint i = 2; i < WaveSize; i *= 2)
        {
            float4 other = simd_shuffle_up(temp, ushort(i));

            if (i <= LaneIndex)
                temp = temp * other;
        }
        Accum += (dot(temp, temp) > 0.5 ? 1 : 0);
    }
    #endif
    #if WAVE_FEATURE_QUAD
    {
        float val = quad_broadcast(float(DTid) * 0.1f, ushort(LaneIndex));
        Accum += (val > 2.5f);
    }
    #endif

    g_WBuffer[DTid] = Accum;
}
)";

    ShaderSourceStream << ShaderBody;
    const String Source = ShaderSourceStream.str();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_MSL;
    ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
    ShaderCI.Desc.Name       = "Wave op test - CS";
    ShaderCI.EntryPoint      = "CSMain";
    ShaderCI.Source          = Source.c_str();

    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);

    ComputePipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Wave op test";
    PSOCreateInfo.pCS          = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);
}

} // namespace Diligent
