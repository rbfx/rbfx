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

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char g_BrokenHLSL[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
    pos = float3(0.0, 0.0, 0.0, 0.0);
}
)";

static const char g_BrokenGLSL[] = R"(
void VSMain()
{
    gl_Position = vec3(0.0, 0.0, 0.0);
}
)";

static const char g_BrokenMSL[] = R"(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct VSOut
{
    float4 pos [[position]];
};

vertex VSOut VSMain()
{
    VSOut out = {};
    out.pos = float3(0.0, 0.0, 0.0);
    return out;
}
)";

void TestBrokenShader(const char* Source, const char* Name, SHADER_SOURCE_LANGUAGE SourceLanguage, int ErrorAllowance)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    ShaderCreateInfo ShaderCI;
    ShaderCI.Source                     = Source;
    ShaderCI.EntryPoint                 = "VSMain";
    ShaderCI.Desc.ShaderType            = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name                  = Name;
    ShaderCI.SourceLanguage             = SourceLanguage;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    ShaderMacro Macros[] = {{"TEST", "MACRO"}, {}};
    ShaderCI.Macros      = Macros;

    IDataBlob* pErrors        = nullptr;
    ShaderCI.ppCompilerOutput = &pErrors;

    pEnv->SetErrorAllowance(ErrorAllowance, "\n\nNo worries, testing broken shader...\n\n");
    RefCntAutoPtr<IShader> pBrokenShader;
    pDevice->CreateShader(ShaderCI, &pBrokenShader);
    EXPECT_FALSE(pBrokenShader);
    ASSERT_NE(pErrors, nullptr);
    const char* Msg = reinterpret_cast<const char*>(pErrors->GetDataPtr());
    LOG_INFO_MESSAGE("Compiler output:\n", Msg);
    pErrors->Release();
}

TEST(Shader, BrokenHLSL)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    // HLSL is supported in all backends
    TestBrokenShader(g_BrokenHLSL, "Broken HLSL test", SHADER_SOURCE_LANGUAGE_HLSL,
                     DeviceInfo.IsGLDevice() || DeviceInfo.IsD3DDevice() ? 2 : 3);
}

TEST(Shader, BrokenGLSL)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (DeviceInfo.IsD3DDevice())
    {
        GTEST_SKIP() << "GLSL is not supported in Direct3D";
    }

    TestBrokenShader(g_BrokenGLSL, "Broken GLSL test", SHADER_SOURCE_LANGUAGE_GLSL,
                     DeviceInfo.IsGLDevice() ? 2 : 3);
}

TEST(Shader, BrokenMSL)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!DeviceInfo.IsMetalDevice())
    {
        GTEST_SKIP() << "MSL is only supported in Metal";
    }

    TestBrokenShader(g_BrokenMSL, "Broken MSL test", SHADER_SOURCE_LANGUAGE_MSL, 2);
}

} // namespace
