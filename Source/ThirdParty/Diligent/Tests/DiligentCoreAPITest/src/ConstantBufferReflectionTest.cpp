/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

static const char g_TestShaderSource_HLSL[] = R"(

Texture2D    g_Tex1;
SamplerState g_Tex1_sampler;

Texture2D    g_Tex2;
SamplerState g_Tex2_sampler;

Buffer<float4> g_Buffer;

struct Struct1
{
    float4 f4[2];
    uint4  u4;
};

StructuredBuffer<Struct1> g_StructBuff;

struct Struct2
{
    uint4   u4;
    Struct1 s1;
};

struct Struct3
{
    Struct1 s1[2];
    int4    i4;
    Struct2 s2;
};

cbuffer CBuffer1
{
    float f;
    uint  u;
    int   i;
    bool  b;

    float4 f4;

    float4x4 f4x4;
    float4x2 f4x2;

    Struct1 s1;

    float4   af4[2];
    float4x4 af4x4[4];
}

cbuffer CBuffer2
{
    uint4    u4;
    int4     i4;
    float4   f4_2;
    Struct2  s2;
    float4x4 f4x4_2;
    Struct3  s3;
}

void main(out float4 pos : SV_POSITION)
{
    pos = f4;
    pos += s1.f4[1];
    pos += s2.s1.f4[1];
    pos += s3.s1[1].f4[1];
    pos += s3.s2.s1.f4[1];
    pos += g_Tex1.SampleLevel(g_Tex1_sampler, float2(0.5, 0.5), 0.0);
    pos += g_Tex2.SampleLevel(g_Tex2_sampler, float2(0.5, 0.5), 0.0);
    pos += g_Buffer.Load(0);
    pos += g_StructBuff[0].f4[1];
}
)";

using BufferDescMappingType = std::vector<std::pair<std::string, const ShaderCodeBufferDesc&>>;
void CheckShaderConstantBuffers(IShader* pShader, bool PrintBufferContents, const BufferDescMappingType& Buffers)
{
    auto ResCount = pShader->GetResourceCount();
    for (Uint32 i = 0; i < ResCount; ++i)
    {
        ShaderResourceDesc ResDesc;
        pShader->GetResourceDesc(i, ResDesc);
        if (ResDesc.Type != SHADER_RESOURCE_TYPE_CONSTANT_BUFFER)
            continue;

        auto it = Buffers.begin();
        while (it != Buffers.end() && it->first != ResDesc.Name)
            ++it;

        if (it != Buffers.end())
        {
            const auto* pBuffDesc = pShader->GetConstantBufferDesc(i);
            EXPECT_NE(pBuffDesc, nullptr);
            if (pBuffDesc != nullptr)
            {
                const auto& RefDesc = it->second;
                EXPECT_EQ(*pBuffDesc, RefDesc);
                if (PrintBufferContents)
                {
                    std::cout << std::endl
                              << ResDesc.Name << ':' << std::endl
                              << GetShaderCodeBufferDescString(*pBuffDesc, 4);
                }
            }
        }
        else
        {
            GTEST_FAIL() << "Unexpected constant buffer " << ResDesc.Name;
        }
    }

    if (PrintBufferContents)
        std::cout << std::endl;
}

void CheckConstantBufferReflectionHLSL(IShader* pShader, bool PrintBufferContents = false)
{
    auto*      pEnv  = GPUTestingEnvironment::GetInstance();
    const auto IsGL  = pEnv->GetDevice()->GetDeviceInfo().IsGLDevice();
    const auto IsVK  = pEnv->GetDevice()->GetDeviceInfo().IsVulkanDevice();
    const auto IsMtl = pEnv->GetDevice()->GetDeviceInfo().IsMetalDevice();

    const auto* _bool_    = IsVK || IsMtl ? "uint" : "bool";
    const auto  BOOL_TYPE = IsVK || IsMtl ? SHADER_CODE_BASIC_TYPE_UINT : SHADER_CODE_BASIC_TYPE_BOOL;
    const auto* _Struct1_ = !IsGL ? "Struct1" : "";
    const auto* _Struct2_ = !IsGL ? "Struct2" : "";
    const auto* _Struct3_ = !IsGL ? "Struct3" : "";

    static constexpr ShaderCodeVariableDesc Struct1[] =
        {
            {"f4", "float4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 4, 0, 2},
            {"u4", "uint4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 1, 4, 32},
        };

    const ShaderCodeVariableDesc Struct2[] =
        {
            {"u4", "uint4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 1, 4, 0},
            {"s1", _Struct1_, _countof(Struct1), Struct1, 16},
        };

    const ShaderCodeVariableDesc Struct3[] =
        {
            {"s1", _Struct1_, _countof(Struct1), Struct1, 0, 2},
            {"i4", "int4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_INT, 1, 4, 96},
            {"s2", _Struct2_, _countof(Struct2), Struct2, 112},
        };

    const ShaderCodeVariableDesc CBuffer1Vars[] =
        {
            {"f", "float", SHADER_CODE_BASIC_TYPE_FLOAT, 0},
            {"u", "uint", SHADER_CODE_BASIC_TYPE_UINT, 4},
            {"i", "int", SHADER_CODE_BASIC_TYPE_INT, 8},
            {"b", _bool_, BOOL_TYPE, 12},

            {"f4", "float4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 4, 16},
            {"f4x4", "float4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4, 32},
            {"f4x2", "float4x2", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 2, 96},

            {"s1", _Struct1_, _countof(Struct1), Struct1, 128},

            {"af4", "float4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 4, 176, 2},
            {"af4x4", "float4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4, 208, 4},
        };

    const ShaderCodeBufferDesc CBuffer1{464, _countof(CBuffer1Vars), CBuffer1Vars};

    const ShaderCodeVariableDesc CBuffer2Vars[] =
        {
            {"u4", "uint4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 1, 4, 0},
            {"i4", "int4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_INT, 1, 4, 16},
            {"f4_2", "float4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 4, 32},
            {"s2", _Struct2_, _countof(Struct2), Struct2, 48},
            {"f4x4_2", "float4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4, 112},
            {"s3", _Struct3_, _countof(Struct3), Struct3, 176},
        };

    const ShaderCodeBufferDesc CBuffer2{352, _countof(CBuffer2Vars), CBuffer2Vars};

    BufferDescMappingType BufferMapping;
    BufferMapping.emplace_back("CBuffer1", CBuffer1);
    BufferMapping.emplace_back("CBuffer2", CBuffer2);
    CheckShaderConstantBuffers(pShader, PrintBufferContents, BufferMapping);
}

std::pair<RefCntAutoPtr<IShader>, RefCntAutoPtr<IShader>> CreateTestShaders(const char* Source, SHADER_COMPILER Compiler, SHADER_SOURCE_LANGUAGE Language)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage               = Language;
    ShaderCI.ShaderCompiler               = Compiler;
    ShaderCI.Desc                         = {"Constant buffer reflection test", SHADER_TYPE_VERTEX, true};
    ShaderCI.EntryPoint                   = "main";
    ShaderCI.Source                       = Source;
    ShaderCI.LoadConstantBufferReflection = true;

    RefCntAutoPtr<IShader> pShaderSrc;
    pDevice->CreateShader(ShaderCI, &pShaderSrc);
    if (!pShaderSrc)
        return {};

    // Create shader from byte code
    RefCntAutoPtr<IShader> pShaderBC;
    if (pDevice->GetDeviceInfo().IsD3DDevice() || pDevice->GetDeviceInfo().IsVulkanDevice())
    {
        Uint64 ByteCodeSize = 0;
        pShaderSrc->GetBytecode(&ShaderCI.ByteCode, ByteCodeSize);
        ShaderCI.ByteCodeSize = static_cast<size_t>(ByteCodeSize);
        ShaderCI.Source       = nullptr;

        pDevice->CreateShader(ShaderCI, &pShaderBC);
        if (!pShaderBC)
            return {};
    }

    return {pShaderSrc, pShaderBC};
}

TEST(ConstantBufferReflectionTest, HLSL)
{
    auto*       pEnv       = GPUTestingEnvironment::GetInstance();
    auto*       pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (DeviceInfo.IsGLDevice() && !DeviceInfo.Features.SeparablePrograms)
        GTEST_SKIP();

    auto TestShaders = CreateTestShaders(g_TestShaderSource_HLSL, SHADER_COMPILER_DEFAULT, SHADER_SOURCE_LANGUAGE_HLSL);
    ASSERT_TRUE(TestShaders.first);

    constexpr auto PrintBufferContents = true;
    CheckConstantBufferReflectionHLSL(TestShaders.first, PrintBufferContents);

    if (DeviceInfo.IsD3DDevice() || DeviceInfo.IsVulkanDevice())
    {
        ASSERT_TRUE(TestShaders.second);
        CheckConstantBufferReflectionHLSL(TestShaders.second);
    }
}

TEST(ConstantBufferReflectionTest, HLSL_DXC)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D12 && pDevice->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_VULKAN)
        GTEST_SKIP();

    auto TestShaders = CreateTestShaders(g_TestShaderSource_HLSL, SHADER_COMPILER_DXC, SHADER_SOURCE_LANGUAGE_HLSL);
    ASSERT_TRUE(TestShaders.first && TestShaders.second);

    CheckConstantBufferReflectionHLSL(TestShaders.first);
    CheckConstantBufferReflectionHLSL(TestShaders.second);
}


static const char g_TestShaderSourceD3D[] = R"(

Texture2D    g_Tex1;
SamplerState g_Tex1_sampler;

Texture2D    g_Tex2;
SamplerState g_Tex2_sampler;

Buffer<float4> g_Buffer;

struct Struct1
{
    float4 f4;
    uint4  u4;
};

StructuredBuffer<Struct1> g_StructBuff;

cbuffer CBuffer
{
    bool  b;
    int   i;
    bool2 b2;

    bool4 b4;

    bool4x4 b4x4;
    bool4x2 b4x2;

    int4x4 i4x4;
    int4x2 i4x2;

    uint4x4 u4x4;
    uint4x2 u4x2;

    float4 f4;
}

void main(out float4 pos : SV_POSITION)
{
    pos = f4;
    pos += g_Tex1.SampleLevel(g_Tex1_sampler, float2(0.5, 0.5), 0.0);
    pos += g_Tex2.SampleLevel(g_Tex2_sampler, float2(0.5, 0.5), 0.0);
    pos += g_Buffer.Load(0);
    pos += g_StructBuff[0].f4;
}
)";

void CheckConstantBufferReflectionD3D(IShader* pShader, bool PrintBufferContents = false)
{
    static constexpr ShaderCodeVariableDesc CBufferVars[] =
        {
            {"b", "bool", SHADER_CODE_BASIC_TYPE_BOOL, 0},
            {"i", "int", SHADER_CODE_BASIC_TYPE_INT, 4},
            {"b2", "bool2", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_BOOL, 1, 2, 8},
            {"b4", "bool4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_BOOL, 1, 4, 16},
            {"b4x4", "bool4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_BOOL, 4, 4, 32},
            {"b4x2", "bool4x2", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_BOOL, 4, 2, 96},
            {"i4x4", "int4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_INT, 4, 4, 128},
            {"i4x2", "int4x2", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_INT, 4, 2, 192},
            {"u4x4", "uint4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_UINT, 4, 4, 224},
            {"u4x2", "uint4x2", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_UINT, 4, 2, 288},
            {"f4", "float4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 4, 320},
        };

    static constexpr ShaderCodeBufferDesc CBuffer{336, _countof(CBufferVars), CBufferVars};

    BufferDescMappingType BufferMapping;
    BufferMapping.emplace_back("CBuffer", CBuffer);
    CheckShaderConstantBuffers(pShader, PrintBufferContents, BufferMapping);
}

TEST(ConstantBufferReflectionTest, HLSL_D3D)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().IsD3DDevice())
        GTEST_SKIP();

    auto TestShaders = CreateTestShaders(g_TestShaderSourceD3D, SHADER_COMPILER_DEFAULT, SHADER_SOURCE_LANGUAGE_HLSL);
    ASSERT_TRUE(TestShaders.first && TestShaders.second);

    constexpr auto PrintBufferContents = true;
    CheckConstantBufferReflectionD3D(TestShaders.first, PrintBufferContents);
    CheckConstantBufferReflectionD3D(TestShaders.second);
}

TEST(ConstantBufferReflectionTest, HLSL_D3D_DXC)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D12)
        GTEST_SKIP();

    auto TestShaders = CreateTestShaders(g_TestShaderSourceD3D, SHADER_COMPILER_DXC, SHADER_SOURCE_LANGUAGE_HLSL);
    ASSERT_TRUE(TestShaders.first && TestShaders.second);

    CheckConstantBufferReflectionD3D(TestShaders.first);
    CheckConstantBufferReflectionD3D(TestShaders.second);
}



static const char g_TestShaderSourceGLSL[] = R"(

uniform sampler2D g_Tex2D;

layout(std140) readonly buffer g_Buff
{
    vec4 data;
}g_StorageBuff;


struct Struct1
{
    vec4  f4;
    ivec4 i4;
};

struct Struct2
{
    vec4    f4;
    Struct1 s1;
    uvec4   u4;
};

layout(std140) uniform UBuffer 
{
    float f;
    uint  u;
    int   i;
    bool  b;

    vec4  f4;
    uvec4 u4;
    ivec4 i4;
    bvec4 b4;

    vec2  f2;
    uvec2 u2;
    ivec2 i2;
    bvec2 b2;

    Struct1 s1;
    Struct2 s2;

    mat2x4 m2x4;
    mat4x4 m4x4;

    vec4   af4[2];
    mat4x4 am4x4[3];
};

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    gl_Position = f4;
    gl_Position += s1.f4;
    gl_Position += s2.s1.f4;
    gl_Position += af4[0] + af4[1];
    gl_Position += am4x4[0][0] + am4x4[2][0];

    gl_Position += textureLod(g_Tex2D, vec2(0.5,0.5), 0.0);
    gl_Position += g_StorageBuff.data;
}
)";

void CheckConstantBufferReflectionGLSL(IShader* pShader, bool PrintBufferContents = false)
{
    auto*      pEnv = GPUTestingEnvironment::GetInstance();
    const auto IsGL = pEnv->GetDevice()->GetDeviceInfo().IsGLDevice();

    static constexpr ShaderCodeVariableDesc Struct1[] =
        {
            {"f4", "vec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 1, 0},
            {"i4", "ivec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_INT, 4, 1, 16},
        };

    static const ShaderCodeVariableDesc Struct2[] =
        {
            {"f4", "vec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 1, 0},
            {"s1", !IsGL ? "Struct1" : "", _countof(Struct1), Struct1, 16},
            {"u4", "uvec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 4, 1, 48},
        };

    const auto* _bool_    = IsGL ? "bool" : "uint";
    const auto* _bool2_   = IsGL ? "bvec2" : "uvec2";
    const auto* _bool4_   = IsGL ? "bvec4" : "uvec4";
    const auto  BOOL_TYPE = IsGL ? SHADER_CODE_BASIC_TYPE_BOOL : SHADER_CODE_BASIC_TYPE_UINT;

    static const ShaderCodeVariableDesc UBufferVars[] =
        {
            {"f", "float", SHADER_CODE_BASIC_TYPE_FLOAT, 0},
            {"u", "uint", SHADER_CODE_BASIC_TYPE_UINT, 4},
            {"i", "int", SHADER_CODE_BASIC_TYPE_INT, 8},
            {"b", _bool_, BOOL_TYPE, 12},

            {"f4", "vec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 1, 16},
            {"u4", "uvec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 4, 1, 32},
            {"i4", "ivec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_INT, 4, 1, 48},
            {"b4", _bool4_, SHADER_CODE_VARIABLE_CLASS_VECTOR, BOOL_TYPE, 4, 1, 64},

            {"f2", "vec2", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 2, 1, 80},
            {"u2", "uvec2", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_UINT, 2, 1, 88},
            {"i2", "ivec2", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_INT, 2, 1, 96},
            {"b2", _bool2_, SHADER_CODE_VARIABLE_CLASS_VECTOR, BOOL_TYPE, 2, 1, 104},

            {"s1", !IsGL ? "Struct1" : "", _countof(Struct1), Struct1, 112},
            {"s2", !IsGL ? "Struct2" : "", _countof(Struct2), Struct2, 144},

            {"m2x4", "mat2x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 2, 208},
            {"m4x4", "mat4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4, 240},

            {"af4", "vec4", SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 1, 304, 2},
            {"am4x4", "mat4x4", SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4, 336, 3},
        };

    static constexpr ShaderCodeBufferDesc UBuffer{528, _countof(UBufferVars), UBufferVars};

    BufferDescMappingType BufferMapping;
    BufferMapping.emplace_back("UBuffer", UBuffer);
    CheckShaderConstantBuffers(pShader, PrintBufferContents, BufferMapping);
}

TEST(ConstantBufferReflectionTest, GLSL)
{
    auto*       pEnv       = GPUTestingEnvironment::GetInstance();
    auto*       pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (!(DeviceInfo.IsVulkanDevice() || DeviceInfo.IsMetalDevice() || (DeviceInfo.IsGLDevice() && DeviceInfo.Features.SeparablePrograms)))
        GTEST_SKIP();

    auto TestShaders = CreateTestShaders(g_TestShaderSourceGLSL, SHADER_COMPILER_DEFAULT, SHADER_SOURCE_LANGUAGE_GLSL);

    ASSERT_TRUE(TestShaders.first);
    constexpr auto PrintBufferContents = true;
    CheckConstantBufferReflectionGLSL(TestShaders.first, PrintBufferContents);

    if (DeviceInfo.IsD3DDevice() || DeviceInfo.IsVulkanDevice())
    {
        ASSERT_TRUE(TestShaders.second);
        CheckConstantBufferReflectionGLSL(TestShaders.second);
    }
}

} // namespace
