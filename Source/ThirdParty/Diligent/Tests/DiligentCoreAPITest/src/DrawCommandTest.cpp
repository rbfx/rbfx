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

#include <thread>
#include <array>
#include <atomic>

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"
#include "BasicMath.hpp"
#include "MapHelper.hpp"
#include "FastRand.hpp"
#include "ThreadSignal.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void RenderDrawCommandReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if D3D12_SUPPORTED
void RenderDrawCommandReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void RenderDrawCommandReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if VULKAN_SUPPORTED
void RenderDrawCommandReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if METAL_SUPPORTED
void RenderDrawCommandReferenceMtl(ISwapChain* pSwapChain, const float* pClearColor);
#endif

void RenderDrawCommandReference(ISwapChain* pSwapChain, const float* pClearColor = nullptr)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderDrawCommandReferenceD3D11(pSwapChain, pClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderDrawCommandReferenceD3D12(pSwapChain, pClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderDrawCommandReferenceGL(pSwapChain, pClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderDrawCommandReferenceVk(pSwapChain, pClearColor);
                break;
#endif

#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                RenderDrawCommandReferenceMtl(pSwapChain, pClearColor);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
}

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace
{

namespace HLSL
{

// clang-format off
const std::string DrawTest_VS{
R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct VSInput
{
    float4 Pos   : ATTRIB0;
    float3 Color : ATTRIB1;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos   = VSIn.Pos;
    PSIn.Color = VSIn.Color;
}
)"
};


const std::string DrawTest_VSInstanced{
R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct VSInput
{
    float4 Pos       : ATTRIB0;
    float3 Color     : ATTRIB1;
    float4 ScaleBias : ATTRIB2;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos.xy = VSIn.Pos.xy * VSIn.ScaleBias.xy + VSIn.ScaleBias.zw;
    PSIn.Pos.zw = VSIn.Pos.zw;
    PSIn.Color  = VSIn.Color;
}
)"
};

const std::string DrawTest_DynamicBuffers{
R"(

cbuffer DynamicCB0
{
    float4 Positions[4];
}

cbuffer DynamicCB1
{
    float4 Colors[4];
}

cbuffer ImmutableCB
{
    float4 PositionZW;
}

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = float4(Positions[VertId].xy, PositionZW.xy);
    PSIn.Color = Colors[VertId].rgb;
}
)"
};

const std::string DrawTest_VSStructuredBuffers{
R"(
struct PosData
{
    float4 data;
};
struct ColData
{
    float4 data;
};

StructuredBuffer<PosData> g_Positions;
StructuredBuffer<ColData> g_Colors;

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = g_Positions[VertId].data;
    PSIn.Color = g_Colors[VertId].data.rgb;
}
)"
};

const std::string DrawTest_VSStructuredBufferArray{
R"(
struct BufferData
{
    float4 data;
};

StructuredBuffer<BufferData> g_Buffers[2];

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = g_Buffers[0][VertId].data;
    PSIn.Color = g_Buffers[1][VertId].data.rgb;
}
)"
};


const std::string DrawTest_VSFormattedBuffers{
R"(
Buffer<float4> g_Positions;
Buffer<float4> g_Colors;

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = g_Positions.Load(VertId);
    PSIn.Color = g_Colors.Load(VertId).rgb;
}
)"
};


const std::string DrawTest_VSFormattedBufferArray{
R"(
Buffer<float4> g_Buffers[2];

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = g_Buffers[0].Load(VertId);
    PSIn.Color = g_Buffers[1].Load(VertId).rgb;
}
)"
};


const std::string DrawTest_VSUniformBuffers{
R"(

cbuffer cbPositions
{
    float4 Positions[3];
}

cbuffer cbColors
{
    float4 Colors[3];
}

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.Pos   = Positions[VertId];
    PSIn.Color = Colors[VertId].rgb;
}
)"
};
// clang-format on

} // namespace HLSL



namespace GLSL
{
// clang-format off
const std::string DrawTest_VSStructuredBufferArray{
R"(
layout(std140) readonly buffer g_Buffers
{
    vec4 data[3];
}g_StorageBuffers[2];

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

layout(location = 0)out vec3 out_Color;

void main()
{
    gl_Position = g_StorageBuffers[0].data[gl_VertexID];
    out_Color = g_StorageBuffers[1].data[gl_VertexID].rgb;
}
)"
};
// clang-format on
} // namespace GLSL

namespace MSL
{
// clang-format off
const std::string DrawTest_ConstantBuffers{
R"(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct PosData
{
    float4 Positions[3];
};

struct ColData
{
    float4 Colors[3];
};

struct VSMain_out
{
    float3 Color    [[user(locn0)]];
    float4 Position [[position]];
};

vertex VSMain_out VSMain(constant PosData& cbPositions [[buffer(0)]],
                         constant ColData& cbColors    [[buffer(1)]],
                         uint     VertexId             [[vertex_id]])
{
    VSMain_out out = {};
    out.Position = cbPositions.Positions[VertexId];
    out.Color    = cbColors.Colors[VertexId].xyz;
    return out;
}

struct PS_out
{
    float4 Color [[color(0)]];
};

fragment PS_out PSMain(VSMain_out in [[stage_in]])
{
    PS_out out = {};
    out.Color = float4(in.Color, 1.0);
    return out;
}
)"
};
// clang-format on
} // namespace MSL

struct Vertex
{
    float4 Pos;
    float3 Color;
};

// clang-format off
float4 Pos[] =
{
    float4(-1.0f,  -0.5f,  0.f,  1.f),
    float4(-0.5f,  +0.5f,  0.f,  1.f),
    float4( 0.0f,  -0.5f,  0.f,  1.f),

    float4(+0.0f,  -0.5f,  0.f,  1.f),
    float4(+0.5f,  +0.5f,  0.f,  1.f),
    float4(+1.0f,  -0.5f,  0.f,  1.f)
};

float3 Color[] =
{
    float3(1.f,  0.f,  0.f),
    float3(0.f,  1.f,  0.f),
    float3(0.f,  0.f,  1.f),
};

Vertex Vert[] =
{
    {Pos[0], Color[0]},
    {Pos[1], Color[1]},
    {Pos[2], Color[2]},

    {Pos[3], Color[0]},
    {Pos[4], Color[1]},
    {Pos[5], Color[2]}
};

Vertex VertInst[] =
{
    {float4(-1.0,  0.0,  0.0,  1.0), Color[0]},
    {float4( 0.0, +2.0,  0.0,  1.0), Color[1]},
    {float4(+1.0,  0.0,  0.0,  1.0), Color[2]}
};
// clang-format on

class DrawCommandTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pDevice    = pEnv->GetDevice();
        auto* pSwapChain = pEnv->GetSwapChain();

        GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Draw command test - procedural triangles";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pProceduralVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test procedural vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &pProceduralVS);
            ASSERT_NE(pProceduralVS, nullptr);
        }

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_VS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pInstancedVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test instanced vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_VSInstanced.c_str();
            pDevice->CreateShader(ShaderCI, &pInstancedVS);
            ASSERT_NE(pInstancedVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test pixel shader";
            ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSODesc.Name = "Draw command test - procedural tris";

        PSOCreateInfo.pVS = pProceduralVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawProceduralPSO);
        ASSERT_NE(sm_pDrawProceduralPSO, nullptr);

        PSODesc.Name = "Draw command test";

        // clang-format off
        LayoutElement Elems[] =
        {
            LayoutElement{ 0, 0, 4, VT_FLOAT32},
            LayoutElement{ 1, 0, 3, VT_FLOAT32}
        };
        // clang-format on

        GraphicsPipeline.InputLayout.LayoutElements = Elems;
        GraphicsPipeline.InputLayout.NumElements    = _countof(Elems);

        PSOCreateInfo.pVS                  = pVS;
        PSOCreateInfo.pPS                  = pPS;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawPSO);
        ASSERT_NE(sm_pDrawPSO, nullptr);


        PSODesc.Name = "Draw command test - 2x VB stride";

        Elems[0].Stride = sizeof(Vertex) * 2;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDraw_2xStride_PSO);
        ASSERT_NE(sm_pDraw_2xStride_PSO, nullptr);

        PSODesc.Name = "Instanced draw command test";
        // clang-format off
        LayoutElement InstancedElems[] =
        {
            LayoutElement{ 0, 0, 4, VT_FLOAT32},
            LayoutElement{ 1, 0, 3, VT_FLOAT32},
            LayoutElement{ 2, 1, 4, VT_FLOAT32, false, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}
        };
        // clang-format on

        GraphicsPipeline.InputLayout.LayoutElements = InstancedElems;
        GraphicsPipeline.InputLayout.NumElements    = _countof(InstancedElems);

        PSOCreateInfo.pVS = pInstancedVS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawInstancedPSO);
        ASSERT_NE(sm_pDrawInstancedPSO, nullptr);
    }

    static void TearDownTestSuite()
    {
        sm_pDrawProceduralPSO.Release();
        sm_pDrawPSO.Release();
        sm_pDraw_2xStride_PSO.Release();
        sm_pDrawInstancedPSO.Release();

        auto* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void SetRenderTargets(IPipelineState* pPSO)
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pContext   = pEnv->GetDeviceContext();
        auto* pSwapChain = pEnv->GetSwapChain();

        // Use random clear color for each test
        const float ClearColor[] = {sm_Rnd(), sm_Rnd(), sm_Rnd(), sm_Rnd()};
        RenderDrawCommandReference(pSwapChain, ClearColor);

        ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(pPSO);
    }

    static void Present()
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        pSwapChain->Present();

        pContext->Flush();
        pContext->InvalidateState();
    }

    RefCntAutoPtr<IBuffer> CreateVertexBuffer(const void* VertexData, Uint32 DataSize)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Test vertex buffer";
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Size      = DataSize;

        BufferData InitialData;
        InitialData.pData    = VertexData;
        InitialData.DataSize = DataSize;

        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    RefCntAutoPtr<IBuffer> CreateIndexBuffer(const Uint32* Indices, Uint32 NumIndices)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Test index buffer";
        BuffDesc.BindFlags = BIND_INDEX_BUFFER;
        BuffDesc.Size      = sizeof(Uint32) * NumIndices;

        BufferData InitialData;
        InitialData.pData    = Indices;
        InitialData.DataSize = BuffDesc.Size;

        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    RefCntAutoPtr<IBuffer> CreateIndirectDrawArgsBuffer(const Uint32* Data, Uint32 DataSize)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Test index buffer";
        BuffDesc.BindFlags = BIND_INDIRECT_DRAW_ARGS;
        BuffDesc.Size      = DataSize;

        BufferData InitialData;
        InitialData.pData    = Data;
        InitialData.DataSize = BuffDesc.Size;

        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    static void TestDynamicBufferUpdates(IShader*                      pVS,
                                         IShader*                      pPS,
                                         IBuffer*                      pDynamicCB0,
                                         IBuffer*                      pDynamicCB1,
                                         IBuffer*                      pImmutableCB,
                                         SHADER_RESOURCE_VARIABLE_TYPE DynamicCB0Type,
                                         SHADER_RESOURCE_VARIABLE_TYPE DynamicCB1Type,
                                         SHADER_RESOURCE_VARIABLE_TYPE ImmutableCBType);

    static void DrawWithStructuredOrFormattedBuffers(bool                          UseArray,
                                                     IShader*                      pVS,
                                                     IShader*                      pPS,
                                                     IBufferView*                  pPositionsBuffView,
                                                     IBufferView*                  pColorsBuffView,
                                                     SHADER_RESOURCE_VARIABLE_TYPE PosBuffType,
                                                     SHADER_RESOURCE_VARIABLE_TYPE ColBuffType);

    static void TestStructuredOrFormattedBuffers(BUFFER_MODE BuffMode,
                                                 bool        UseArray);

    static void TestUniOrStructBufferOffsets(BUFFER_MODE BuffMode);

    static void DrawWithUniOrStructBufferOffsets(IShader*                      pVS,
                                                 IShader*                      pPS,
                                                 BUFFER_MODE                   BuffMode,
                                                 SHADER_RESOURCE_VARIABLE_TYPE CBType,
                                                 USAGE                         BufferUsage,
                                                 SHADER_VARIABLE_FLAGS         VarFlags);

    static RefCntAutoPtr<IPipelineState> sm_pDrawProceduralPSO;
    static RefCntAutoPtr<IPipelineState> sm_pDrawPSO;
    static RefCntAutoPtr<IPipelineState> sm_pDraw_2xStride_PSO;
    static RefCntAutoPtr<IPipelineState> sm_pDrawInstancedPSO;
    static FastRandFloat                 sm_Rnd;
};

RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawProceduralPSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawPSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDraw_2xStride_PSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawInstancedPSO;
FastRandFloat                 DrawCommandTest::sm_Rnd{0, 0.f, 1.f};

TEST_F(DrawCommandTest, DrawProcedural)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawProceduralPSO);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}


// Non-indexed draw calls


TEST_F(DrawCommandTest, Draw)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto     pVB    = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[] = {pVB};
    pContext->SetVertexBuffers(0, 1, pVBs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices using StartVertexLocation
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto         pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices using buffer offset
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto         pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices using buffer offset
        {}, {},     // Skip 2 vertices using StartVertexLocation
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto         pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex_VBOffset_2xStride)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDraw_2xStride_PSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {},     // Skip 3 * sizeof(Vertex) using buffer offset
        {}, {}, {}, {}, // Skip 2 vertices using StartVertexLocation
        Vert[0], {}, Vert[1], {}, Vert[2], {},
        Vert[3], {}, Vert[4], {}, Vert[5], {}
    };
    // clang-format on

    auto         pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}



// Indexed draw calls (glDrawElements/DrawIndexed)

TEST_F(DrawCommandTest, DrawIndexed)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    const Uint32 Indices[] = {2,4,7, 8,12,11};
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexed_IBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    const Uint32 Indices[] = {0,0,0,0, 2,4,7, 8,12,11}; // Skip 4 indices using index buffer offset
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, sizeof(Uint32) * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexed_IBOffset_BaseVertex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    Uint32 bv = 2; // Base vertex
    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    const Uint32 Indices[] = {0,0,0,0, 2-bv,4-bv,7-bv, 8-bv,12-bv,11-bv};
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, sizeof(Uint32) * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.BaseVertex = bv;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


// Instanced non-indexed draw calls (glDrawArraysInstanced/DrawInstanced)

TEST_F(DrawCommandTest, DrawInstanced)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[] = {pVB, pInstVB};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2; // Draw two instances of the same triangle
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices with VB offset
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, // Skip 3 instances with VB offset
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {2 * sizeof(Vertex), 3 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2; // Draw two instances of the same triangle
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_StartVertex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with start vertex
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances        = 2; // Draw two instances of the same triangle
    drawAttrs.StartVertexLocation = 4; // Skip 4 vertices
    pContext->Draw(drawAttrs);

    Present();
}


// Instanced draw calls with first instance (glDrawArraysInstancedBaseInstance/DrawInstanced)

TEST_F(DrawCommandTest, DrawInstanced_FirstInstance)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstanceLocation
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_FirstInstance_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices with buffer offset
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {},         // Skip 2 instances with buffer offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstanceLocation
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {3 * sizeof(Vertex), 2 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->Draw(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawInstanced_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4;
    drawAttrs.StartVertexLocation   = 3;
    pContext->Draw(drawAttrs);

    Present();
}

// Instanced indexed draw calls (glDrawElementsInstanced/DrawIndexedInstanced)

TEST_F(DrawCommandTest, DrawIndexedInstanced)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_IBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 5 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices with VBOffset
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, // Skip 4 instances with VB offset
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {2 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstIndex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances       = 2;
    drawAttrs.FirstIndexLocation = 5;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


// Instanced indexed draw calls with first instance (glDrawElementsInstancedBaseInstance/DrawInstanced)


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_IBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_IBOffset_FirstIndex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0, 0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.FirstIndexLocation    = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}



// Instanced draw commands with base vertex (glDrawElementsInstancedBaseVertex/DrawInstanced)

TEST_F(DrawCommandTest, DrawIndexedInstanced_BaseVertex)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    drawAttrs.BaseVertex   = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_BaseVertex_VBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.BaseVertex            = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0, 0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.BaseVertex            = 3;
    drawAttrs.FirstIndexLocation    = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


//  Indirect draw calls

TEST_F(DrawCommandTest, DrawInstancedIndirect_FirstInstance_BaseVertex_FirstIndex_VBOffset_InstOffset)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";

    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3, // NumVertices
            2, // NumInstances
            3, // StartVertexLocation
            4  // FirstInstanceLocation
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndirectAttribs drawAttrs{pIndirectArgsBuff, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    pContext->DrawIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstancedIndirect_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset_InstOffset)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";

    auto* pContext = pEnv->GetDeviceContext();
    SetRenderTargets(sm_pDrawInstancedPSO);

    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0, 0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {},     // Skip 4 instances with VB offset
        {}, {}, {}, {}, {}, // Skip 5 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, IsGL ? 0 : 3 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3,             // NumIndices
            2,             // NumInstances
            IsGL ? 7u : 4, // FirstIndexLocation
            3,             // BaseVertex
            5,             // FirstInstanceLocation
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndexedIndirectAttribs drawAttrs{VT_UINT32, pIndirectArgsBuff, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    pContext->DrawIndexedIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, MultiDrawIndirect)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";

    auto* pContext = pEnv->GetDeviceContext();
    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    const Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3, // NumVertices
            1, // NumInstances
            3, // StartVertexLocation
            4, // FirstInstanceLocation
            0, // Test padding
            0, // Test padding

            3, // NumVertices
            1, // NumInstances
            3, // StartVertexLocation
            5, // FirstInstanceLocation
            0, // Test padding
            0  // Test padding
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pIndirectArgsBuff;
    drawAttrs.DrawCount                        = 2;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    drawAttrs.DrawArgsStride                   = 6 * sizeof(Uint32);
    pContext->DrawIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, MultiDrawIndexedIndirect)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";

    auto* pContext = pEnv->GetDeviceContext();
    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {},     // Skip 4 instances with VB offset
        {}, {}, {}, {}, {}, // Skip 5 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3, // NumIndices
            1, // NumInstances
            4, // FirstIndexLocation
            3, // BaseVertex
            5, // FirstInstanceLocation
            0, // Test padding
            0, // Test padding

            3, // NumIndices
            1, // NumInstances
            4, // FirstIndexLocation
            3, // BaseVertex
            6, // FirstInstanceLocation
            0, // Test padding
            0, // Test padding
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndexedIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pIndirectArgsBuff;
    drawAttrs.IndexType                        = VT_UINT32;
    drawAttrs.DrawCount                        = 2;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    drawAttrs.DrawArgsStride                   = 7 * sizeof(Uint32);
    pContext->DrawIndexedIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, MultiDrawIndirectCount)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";
    if ((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER) == 0)
        GTEST_SKIP() << "Indirect multi draw with counter buffer is not supported on this device";

    auto* pContext = pEnv->GetDeviceContext();
    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    const Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3, // NumVertices
            1, // NumInstances
            3, // StartVertexLocation
            4, // FirstInstanceLocation
            0,
            0,

            3, // NumVertices
            1, // NumInstances
            3, // StartVertexLocation
            5, // FirstInstanceLocation
            0,
            0 //
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    const Uint32 DrawCount[] =
        {
            0, 0, // Offset
            2     //
        };
    auto pCountBuff = CreateIndirectDrawArgsBuffer(DrawCount, sizeof(DrawCount));

    DrawIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pIndirectArgsBuff;
    drawAttrs.DrawCount                        = 2;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    drawAttrs.DrawArgsStride                   = 6 * sizeof(Uint32);
    drawAttrs.pCounterBuffer                   = pCountBuff;
    drawAttrs.CounterBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.CounterOffset                    = 2 * sizeof(Uint32);
    pContext->DrawIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, MultiDrawIndexedIndirectCount)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    const auto  DrawCaps = pDevice->GetAdapterInfo().DrawCommand.CapFlags;
    ASSERT_TRUE((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0) << "Indirect rendering must be supported on all desktop platforms";
    if ((DrawCaps & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER) == 0)
        GTEST_SKIP() << "Indirect multi draw with counter buffer is not supported on this device";

    auto* pContext = pEnv->GetDeviceContext();
    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    const Uint32 Indices[] = {0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] =
    {
        {}, {}, {}, {},     // Skip 4 instances with VB offset
        {}, {}, {}, {}, {}, // Skip 5 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {4 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            3, // NumIndices
            1, // NumInstances
            4, // FirstIndexLocation
            3, // BaseVertex
            5, // FirstInstanceLocation
            0,
            0,

            3, // NumIndices
            1, // NumInstances
            4, // FirstIndexLocation
            3, // BaseVertex
            6, // FirstInstanceLocation
            0,
            0 //
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    const Uint32 DrawCount[] =
        {
            0, 0, // Offset
            2     //
        };
    auto pCountBuff = CreateIndirectDrawArgsBuffer(DrawCount, sizeof(DrawCount));

    DrawIndexedIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pIndirectArgsBuff;
    drawAttrs.IndexType                        = VT_UINT32;
    drawAttrs.DrawCount                        = 2;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = 5 * sizeof(Uint32);
    drawAttrs.DrawArgsStride                   = 7 * sizeof(Uint32);
    drawAttrs.pCounterBuffer                   = pCountBuff;
    drawAttrs.CounterBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.CounterOffset                    = 2 * sizeof(Uint32);
    pContext->DrawIndexedIndirect(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_InstanceDataStepRate)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.InstanceDataStepRate)
        GTEST_SKIP() << "InstanceDataStepRate is not supported";

    auto* pContext   = pEnv->GetDeviceContext();
    auto* pSwapChain = pEnv->GetSwapChain();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pInstancedVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test instanced vertex shader";
        ShaderCI.Source          = HLSL::DrawTest_VSInstanced.c_str();
        pDevice->CreateShader(ShaderCI, &pInstancedVS);
        ASSERT_NE(pInstancedVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test pixel shader";
        ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    // clang-format off
    LayoutElement InstancedElems[] =
    {
        LayoutElement{ 0, 0, 4, VT_FLOAT32},
        LayoutElement{ 1, 0, 3, VT_FLOAT32},
        LayoutElement{ 2, 1, 4, VT_FLOAT32, false, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE, 2}
    };
    // clang-format on

    GraphicsPipeline.InputLayout.LayoutElements = InstancedElems;
    GraphicsPipeline.InputLayout.NumElements    = _countof(InstancedElems);

    PSOCreateInfo.pVS = pInstancedVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);


    SetRenderTargets(pPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] =
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f},
        float4{0.1f,  0.1f,  -0.5f, -0.75f}, // These should not
        float4{0.1f,  0.1f,  +0.5f, -0.75f}  // be used
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer*     pVBs[]    = {pVB, pInstVB};
    const Uint64 Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 4; // Draw two instances of triangles 0 and 1
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DeferredContexts)
{
    auto* pEnv = GPUTestingEnvironment::GetInstance();
    if (pEnv->GetNumDeferredContexts() == 0)
    {
        GTEST_SKIP() << "Deferred contexts are not supported by this device";
    }
    VERIFY(pEnv->GetNumDeferredContexts() >= 2, "At least two deferred contexts are expected");

    auto* pSwapChain    = pEnv->GetSwapChain();
    auto* pImmediateCtx = pEnv->GetDeviceContext();

    const float ClearColor[] = {sm_Rnd(), sm_Rnd(), sm_Rnd(), sm_Rnd()};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    const Uint32 Indices[] = {0, 1, 2, 3, 4, 5};
    auto         pVB       = CreateVertexBuffer(Vert, sizeof(Vert));
    auto         pIB       = CreateIndexBuffer(Indices, _countof(Indices));

    StateTransitionDesc Barriers[] = //
        {
            {pVB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE},
            {pIB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE} //
        };
    pImmediateCtx->TransitionResourceStates(_countof(Barriers), Barriers);

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pImmediateCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pImmediateCtx->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    constexpr Uint32                                    NumThreads = 2;
    std::array<std::thread, NumThreads>                 WorkerThreads;
    std::array<RefCntAutoPtr<ICommandList>, NumThreads> CmdLists;
    std::array<ICommandList*, NumThreads>               CmdListPtrs;

    std::atomic<Uint32>    NumCmdListsReady{0};
    ThreadingTools::Signal FinishFrameSignal;
    ThreadingTools::Signal ExecuteCommandListsSignal;
    for (Uint32 i = 0; i < NumThreads; ++i)
    {
        WorkerThreads[i] = std::thread(
            [&](Uint32 thread_id) //
            {
                auto* pCtx = pEnv->GetDeferredContext(thread_id);

                pCtx->Begin(0);
                pCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

                IBuffer*     pVBs[]    = {pVB};
                const Uint64 Offsets[] = {0};
                pCtx->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);
                pCtx->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

                pCtx->SetPipelineState(sm_pDrawPSO);

                DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
                drawAttrs.FirstIndexLocation = 3 * thread_id;
                pCtx->DrawIndexed(drawAttrs);

                pCtx->FinishCommandList(&CmdLists[thread_id]);
                CmdListPtrs[thread_id] = CmdLists[thread_id];

                // Atomically increment the number of completed threads
                const auto NumReadyLists = NumCmdListsReady.fetch_add(1) + 1;
                if (NumReadyLists == NumThreads)
                    ExecuteCommandListsSignal.Trigger();

                FinishFrameSignal.Wait(true, NumThreads);

                // IMPORTANT: In Metal backend FinishFrame must be called from the same
                //            thread that issued rendering commands.
                pCtx->FinishFrame();
            },
            i);
    }

    // Wait for the worker threads
    ExecuteCommandListsSignal.Wait(true, 1);

    pImmediateCtx->ExecuteCommandLists(NumThreads, CmdListPtrs.data());

    FinishFrameSignal.Trigger(true);
    for (auto& t : WorkerThreads)
        t.join();

    Present();
}


void DrawCommandTest::TestDynamicBufferUpdates(IShader*                      pVS,
                                               IShader*                      pPS,
                                               IBuffer*                      pDynamicCB0,
                                               IBuffer*                      pDynamicCB1,
                                               IBuffer*                      pImmutableCB,
                                               SHADER_RESOURCE_VARIABLE_TYPE DynamicCB0Type,
                                               SHADER_RESOURCE_VARIABLE_TYPE DynamicCB1Type,
                                               SHADER_RESOURCE_VARIABLE_TYPE ImmutableCBType)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pContext   = pEnv->GetDeviceContext();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Draw command test - dynamic buffer update";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderResourceVariableDesc Variables[] =
        {
            {SHADER_TYPE_VERTEX, "DynamicCB0", DynamicCB0Type},
            {SHADER_TYPE_VERTEX, "DynamicCB1", DynamicCB1Type},
            {SHADER_TYPE_VERTEX, "ImmutableCB", ImmutableCBType} //
        };
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    PSODesc.ResourceLayout.Variables    = Variables;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    if (DynamicCB0Type == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "DynamicCB0")->Set(pDynamicCB0);
    if (DynamicCB1Type == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "DynamicCB1")->Set(pDynamicCB1);
    if (ImmutableCBType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "ImmutableCB")->Set(pImmutableCB);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_TRUE(pSRB != nullptr);

    if (DynamicCB0Type != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "DynamicCB0")->Set(pDynamicCB0);
    if (DynamicCB1Type != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "DynamicCB1")->Set(pDynamicCB1);
    if (ImmutableCBType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ImmutableCB")->Set(pImmutableCB);

    SetRenderTargets(pPSO);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        MapHelper<float4> PosData{pContext, pDynamicCB0, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
        {
            PosData[i] = Pos[i];
        }
    }

    {
        MapHelper<float4> ColorData{pContext, pDynamicCB1, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            ColorData[i] = Color[i];
    }

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    {
        MapHelper<float4> PosData{pContext, pDynamicCB0, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            PosData[i] = Pos[3 + i];
    }

    pContext->Draw(drawAttrs);

    Present();
}

// Test dynamic buffer update between two draw calls without committing and SRB
TEST_F(DrawCommandTest, DynamicUniformBufferUpdates)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test dynamic buffer updates - VS";
        ShaderCI.Source          = HLSL::DrawTest_DynamicBuffers.c_str();
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test dynamic buffer updates - PS";
        ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    RefCntAutoPtr<IBuffer> pDynamicCB0;
    RefCntAutoPtr<IBuffer> pDynamicCB1;
    RefCntAutoPtr<IBuffer> pImmutableCB;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Dynamic buffer update test - dynamic CB0";
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        BuffDesc.Size           = sizeof(float) * 16;

        pDevice->CreateBuffer(BuffDesc, nullptr, &pDynamicCB0);
        ASSERT_NE(pDynamicCB0, nullptr);

        BuffDesc.Name = "Dynamic buffer update test - dynamic CB1";
        pDevice->CreateBuffer(BuffDesc, nullptr, &pDynamicCB1);
        ASSERT_NE(pDynamicCB1, nullptr);

        {
            BuffDesc.Usage          = USAGE_IMMUTABLE;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;
            BuffDesc.Name           = "Dynamic buffer update test - immutable CB";

            float      Data[16] = {0, 1};
            BufferData InitialData;
            InitialData.pData    = Data;
            InitialData.DataSize = sizeof(Data);
            pDevice->CreateBuffer(BuffDesc, &InitialData, &pImmutableCB);
            ASSERT_NE(pImmutableCB, nullptr);
        }
    }

    for (Uint32 cb0_type = 0; cb0_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++cb0_type)
    {
        for (Uint32 cb1_type = 0; cb1_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++cb1_type)
        {
            for (Uint32 cb2_type = 0; cb2_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++cb2_type)
            {
                const auto CB0Type = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(cb0_type);
                const auto CB1Type = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(cb1_type);
                const auto CB2Type = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(cb2_type);

                TestDynamicBufferUpdates(pVS, pPS, pDynamicCB0, pDynamicCB1, pImmutableCB, CB0Type, CB1Type, CB2Type);
                std::cout << TestingEnvironment::GetCurrentTestStatusString() << ' '
                          << " CB0 " << GetShaderVariableTypeLiteralName(CB0Type) << ','
                          << " CB1 " << GetShaderVariableTypeLiteralName(CB1Type) << ','
                          << " CB2 " << GetShaderVariableTypeLiteralName(CB2Type) << std::endl;
            }
        }
    }
}

TEST_F(DrawCommandTest, DynamicVertexBufferUpdate)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = GPUTestingEnvironment::GetInstance()->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    RefCntAutoPtr<IBuffer> pVB;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Dynamic vertex buffer";
        BuffDesc.BindFlags      = BIND_VERTEX_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        BuffDesc.Size           = sizeof(Vertex) * 3;

        pDevice->CreateBuffer(BuffDesc, nullptr, &pVB);
        ASSERT_NE(pVB, nullptr);
    }

    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    {
        MapHelper<Vertex> VertData{pContext, pVB, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            VertData[i] = Vert[i];
    }

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    {
        MapHelper<Vertex> VertData{pContext, pVB, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            VertData[i] = Vert[3 + i];
    }
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DynamicIndexBufferUpdate)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = GPUTestingEnvironment::GetInstance()->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[5], Vert[4]
    };
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));

    RefCntAutoPtr<IBuffer> pIB;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Dynamic index buffer";
        BuffDesc.BindFlags      = BIND_INDEX_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        BuffDesc.Size           = sizeof(Uint32) * 3;

        pDevice->CreateBuffer(BuffDesc, nullptr, &pIB);
        ASSERT_NE(pIB, nullptr);
    }


    IBuffer*     pVBs[]    = {pVB};
    const Uint64 Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        MapHelper<Uint32> IndData{pContext, pIB, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            IndData[i] = i;
    }

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    pContext->DrawIndexed(drawAttrs);

    {
        MapHelper<Uint32> IndData{pContext, pIB, MAP_WRITE, MAP_FLAG_DISCARD};
        for (Uint32 i = 0; i < 3; ++i)
            IndData[i] = 3 + i;
    }

    pContext->DrawIndexed(drawAttrs);

    Present();
}


void DrawCommandTest::DrawWithStructuredOrFormattedBuffers(bool                          UseArray,
                                                           IShader*                      pVS,
                                                           IShader*                      pPS,
                                                           IBufferView*                  pPositionsBuffView,
                                                           IBufferView*                  pColorsBuffView,
                                                           SHADER_RESOURCE_VARIABLE_TYPE PosBuffType,
                                                           SHADER_RESOURCE_VARIABLE_TYPE ColBuffType)
{
    if (UseArray)
        VERIFY(ColBuffType == 0, "Color buffer type is ignored when arrays are used");

    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pContext   = pEnv->GetDeviceContext();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Draw command test - structured buffer update";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    std::vector<ShaderResourceVariableDesc> Variables;
    if (UseArray)
    {
        Variables.emplace_back(SHADER_TYPE_VERTEX, "g_Buffers", PosBuffType);
    }
    else
    {
        Variables.emplace_back(SHADER_TYPE_VERTEX, "g_Positions", PosBuffType);
        Variables.emplace_back(SHADER_TYPE_VERTEX, "g_Colors", ColBuffType);
    };
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());
    PSODesc.ResourceLayout.Variables    = Variables.data();

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_TRUE(pPSO != nullptr);

    IDeviceObject* const ppBuffViews[] = {pPositionsBuffView, pColorsBuffView};
    if (UseArray)
    {
        if (PosBuffType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Buffers")->SetArray(ppBuffViews, 0, _countof(ppBuffViews));
    }
    else
    {
        if (PosBuffType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Positions")->Set(pPositionsBuffView);
        if (ColBuffType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Colors")->Set(pColorsBuffView);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_TRUE(pSRB != nullptr);

    if (UseArray)
    {
        if (PosBuffType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffers")->SetArray(ppBuffViews, 0, _countof(ppBuffViews));
    }
    else
    {
        if (PosBuffType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Positions")->Set(pPositionsBuffView);
        if (ColBuffType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Colors")->Set(pColorsBuffView);
    }

    SetRenderTargets(pPSO);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float4 Color4[] =
        {
            float4{1.f, 0.f, 0.f, 0.f},
            float4{0.f, 1.f, 0.f, 0.f},
            float4{0.f, 0.f, 1.f, 0.f},
        };

    auto UpdateBuffer = [pContext](IBufferView* pBuffView, const void* pData, size_t OffsetInFloat4, size_t DataSize) //
    {
        auto* pBuffer = pBuffView->GetBuffer();
        if (pBuffer->GetDesc().Usage == USAGE_DYNAMIC)
        {
            MapHelper<float4> pBuffData{pContext, pBuffer, MAP_WRITE, MAP_FLAG_DISCARD};
            memcpy(pBuffData + OffsetInFloat4, pData, DataSize);
        }
        else
        {
            pContext->UpdateBuffer(pBuffer, static_cast<Uint32>(sizeof(float4) * OffsetInFloat4), static_cast<Uint32>(DataSize), pData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
    };

    UpdateBuffer(pPositionsBuffView, Pos, 0, sizeof(float4) * 3);
    UpdateBuffer(pColorsBuffView, Color4, 4, sizeof(float4) * 3);
    pContext->TransitionShaderResources(pPSO, pSRB);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    UpdateBuffer(pPositionsBuffView, Pos + 3, 0, sizeof(float4) * 3);
    UpdateBuffer(pColorsBuffView, Color4, 4, sizeof(float4) * 3);
    pContext->TransitionShaderResources(pPSO, pSRB);

    pContext->Draw(drawAttrs);

    Present();
}

void DrawCommandTest::TestStructuredOrFormattedBuffers(BUFFER_MODE BuffMode,
                                                       bool        UseArray)
{
    VERIFY_EXPR(BuffMode == BUFFER_MODE_STRUCTURED || BuffMode == BUFFER_MODE_FORMATTED);

    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;

    auto SelectShaderCompiler = [UseArray, pEnv](ShaderCreateInfo& ShaderCI) //
    {
        if (UseArray && pEnv->NeedWARPResourceArrayIndexingBugWorkaround())
        {
            // As of Windows version 2004 (build 19041), there is a bug in D3D12 WARP rasterizer:
            // Shader resource array indexing always references array element 0 when shaders are compiled
            // with shader model 5.1.
            // Use SM5.0 with old compiler as a workaround.
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
        else
        {
            ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        }
    };

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test structured buffers - VS";
        if (BuffMode == BUFFER_MODE_STRUCTURED)
        {
            if (DeviceInfo.IsD3DDevice())
            {
                ShaderCI.Source = UseArray ? HLSL::DrawTest_VSStructuredBufferArray.c_str() : HLSL::DrawTest_VSStructuredBuffers.c_str();
            }
            else
            {
                if (UseArray)
                {
                    ShaderCI.Source         = GLSL::DrawTest_VSStructuredBufferArray.c_str();
                    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
                }
                else
                {
                    ShaderCI.Source = HLSL::DrawTest_VSStructuredBuffers.c_str();
                }
            }
        }
        else
        {
            ShaderCI.Source = UseArray ? HLSL::DrawTest_VSFormattedBufferArray.c_str() : HLSL::DrawTest_VSFormattedBuffers.c_str();
        }
        SelectShaderCompiler(ShaderCI);
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test structured buffers - PS";
        ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
        SelectShaderCompiler(ShaderCI);
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    for (Uint32 UseDynamicBuffers = 0; UseDynamicBuffers < 2; ++UseDynamicBuffers)
    {
        if (BuffMode == BUFFER_MODE_STRUCTURED && UseArray && UseDynamicBuffers && DeviceInfo.Type == RENDER_DEVICE_TYPE_D3D12)
        {
            std::cout << TestingEnvironment::GetTestSkippedString() << " Dynamic structured buffers can't be bound to array variables in D3D12" << std::endl;
            continue;
        }

        BufferDesc BuffDesc;
        BuffDesc.Name              = "Structured buffer draw test - positions";
        BuffDesc.BindFlags         = BIND_SHADER_RESOURCE;
        BuffDesc.Usage             = UseDynamicBuffers ? USAGE_DYNAMIC : USAGE_DEFAULT;
        BuffDesc.CPUAccessFlags    = UseDynamicBuffers ? CPU_ACCESS_WRITE : CPU_ACCESS_NONE;
        BuffDesc.Mode              = BuffMode;
        BuffDesc.ElementByteStride = 16;
        BuffDesc.Size              = sizeof(float4) * 4;

        RefCntAutoPtr<IBuffer> pPositionsBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pPositionsBuffer);
        ASSERT_NE(pPositionsBuffer, nullptr);

        BuffDesc.Name = "Structured buffer draw test - colors";
        BuffDesc.Size = sizeof(float4) * 8;
        RefCntAutoPtr<IBuffer> pColorsBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pColorsBuffer);
        ASSERT_NE(pColorsBuffer, nullptr);

        RefCntAutoPtr<IBufferView> pPosBuffView, pColorBufferView;
        if (BuffMode == BUFFER_MODE_STRUCTURED)
        {
            pPosBuffView = pPositionsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);

            BufferViewDesc BuffSRVDesc;
            BuffSRVDesc.ViewType   = BUFFER_VIEW_SHADER_RESOURCE;
            BuffSRVDesc.ByteOffset = sizeof(float4) * 4;
            pColorsBuffer->CreateView(BuffSRVDesc, &pColorBufferView);
        }
        else
        {
            BufferViewDesc BuffSRVDesc;
            BuffSRVDesc.ViewType             = BUFFER_VIEW_SHADER_RESOURCE;
            BuffSRVDesc.Format.NumComponents = 4;
            BuffSRVDesc.Format.ValueType     = VT_FLOAT32;
            pPositionsBuffer->CreateView(BuffSRVDesc, &pPosBuffView);
            BuffSRVDesc.ByteOffset = sizeof(float4) * 4;
            pColorsBuffer->CreateView(BuffSRVDesc, &pColorBufferView);
        }

        for (Uint32 pos_buff_type = 0; pos_buff_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++pos_buff_type)
        {
            for (Uint32 col_buff_type = 0; col_buff_type < (UseArray ? 1u : SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES); ++col_buff_type)
            {
                const auto PosBuffType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(pos_buff_type);
                const auto ColBuffType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(col_buff_type);
                DrawWithStructuredOrFormattedBuffers(UseArray, pVS, pPS, pPosBuffView, pColorBufferView, PosBuffType, ColBuffType);
                std::cout << TestingEnvironment::GetCurrentTestStatusString() << ' '
                          << (UseDynamicBuffers ? "Dynamic" : "Default")
                          << " buff, " << GetShaderVariableTypeLiteralName(PosBuffType) << " Pos, "
                          << GetShaderVariableTypeLiteralName(ColBuffType) << " Color"
                          << std::endl;
            }
        }
    }
}

// Test draw commands that use structured buffers
TEST_F(DrawCommandTest, StructuredBuffers)
{
    TestStructuredOrFormattedBuffers(BUFFER_MODE_STRUCTURED, false /*UseArray*/);
}

// Test draw commands that use array of structured buffers
TEST_F(DrawCommandTest, StructuredBufferArray)
{
    TestStructuredOrFormattedBuffers(BUFFER_MODE_STRUCTURED, true /*UseArray*/);
}

// Test draw commands that use formatted buffers
TEST_F(DrawCommandTest, FormattedBuffers)
{
    TestStructuredOrFormattedBuffers(BUFFER_MODE_FORMATTED, false /*UseArray*/);
}

// Test draw commands that use formatted buffer array
TEST_F(DrawCommandTest, FormattedBufferArray)
{
    TestStructuredOrFormattedBuffers(BUFFER_MODE_FORMATTED, true /*UseArray*/);
}

void DrawCommandTest::DrawWithUniOrStructBufferOffsets(IShader*                      pVS,
                                                       IShader*                      pPS,
                                                       BUFFER_MODE                   BuffMode,
                                                       SHADER_RESOURCE_VARIABLE_TYPE CBType,
                                                       USAGE                         BufferUsage,
                                                       SHADER_VARIABLE_FLAGS         VarFlags)
{
    auto* pEnv       = GPUTestingEnvironment::GetInstance();
    auto* pContext   = pEnv->GetDeviceContext();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    const auto UseSetOffset = (VarFlags & SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS) == 0 && (CBType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    if (DeviceInfo.Type == RENDER_DEVICE_TYPE_D3D11 && BuffMode == BUFFER_MODE_STRUCTURED && UseSetOffset)
    {
        // Offsets for structured buffers are not supported in D3D11
        return;
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Draw command test - buffer offsets test";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    const char* PosResourceName = BuffMode == BUFFER_MODE_STRUCTURED ? "g_Positions" : "cbPositions";
    const char* ColResourceName = BuffMode == BUFFER_MODE_STRUCTURED ? "g_Colors" : "cbColors";

    ShaderResourceVariableDesc Variables[] = //
        {
            {SHADER_TYPE_VERTEX, PosResourceName, CBType, VarFlags},
            {SHADER_TYPE_VERTEX, ColResourceName, CBType, VarFlags} //
        };
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    PSODesc.ResourceLayout.Variables    = Variables;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO0;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO0);
    ASSERT_TRUE(pPSO0 != nullptr);

    RefCntAutoPtr<IPipelineState> pPSO1;
    if (CBType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
    {
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO1);
        ASSERT_TRUE(pPSO1 != nullptr);
    }
    else
    {
        pPSO1 = pPSO0;
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB0;
    pPSO0->CreateShaderResourceBinding(&pSRB0, false);
    ASSERT_TRUE(pSRB0 != nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB1;
    if (!UseSetOffset)
    {
        pPSO1->CreateShaderResourceBinding(&pSRB1, false);
        ASSERT_TRUE(pSRB1 != nullptr);
    }

    const auto& BufferProps     = pDevice->GetAdapterInfo().Buffer;
    const auto  IsStructured    = BuffMode == BUFFER_MODE_STRUCTURED;
    const auto  OffsetAlignment = IsStructured ? BufferProps.StructuredBufferOffsetAlignment : BufferProps.ConstantBufferOffsetAlignment;

    Uint32 BaseOffset = sizeof(float4) * 4;
    while (BaseOffset < OffsetAlignment)
        BaseOffset *= 2;

    const Uint32 PosOffsets[] = {BaseOffset * 1, BaseOffset * 3};
    const Uint32 ColOffsets[] = {BaseOffset * 2, BaseOffset * 4};

    std::vector<float4> PosData(PosOffsets[1] / sizeof(float4) + 3);
    std::vector<float4> ColData(ColOffsets[1] / sizeof(float4) + 3);
    for (Uint32 tri = 0; tri < 2; ++tri)
    {
        for (Uint32 i = 0; i < 3; ++i)
        {
            PosData[PosOffsets[tri] / sizeof(float4) + i] = Pos[tri * 3 + i];
            ColData[ColOffsets[tri] / sizeof(float4) + i] = Color[i];
        }
    }

    BufferDesc PosBuffDesc;
    PosBuffDesc.Name              = "Buffer offset test pos data";
    PosBuffDesc.BindFlags         = BuffMode == BUFFER_MODE_STRUCTURED ? BIND_SHADER_RESOURCE : BIND_UNIFORM_BUFFER;
    PosBuffDesc.Usage             = BufferUsage;
    PosBuffDesc.Mode              = BuffMode;
    PosBuffDesc.ElementByteStride = BuffMode == BUFFER_MODE_STRUCTURED ? sizeof(float4) : 0;
    PosBuffDesc.CPUAccessFlags    = BufferUsage == USAGE_DYNAMIC ? CPU_ACCESS_WRITE : CPU_ACCESS_NONE;
    PosBuffDesc.Size              = static_cast<Uint32>(PosData.size() * sizeof(float4));

    RefCntAutoPtr<IBuffer> pPosDataBuffer;
    pDevice->CreateBuffer(PosBuffDesc, nullptr, &pPosDataBuffer);
    ASSERT_TRUE(pPosDataBuffer);

    auto ColBuffDesc = PosBuffDesc;
    ColBuffDesc.Name = "Buffer offset test color data";
    ColBuffDesc.Size = static_cast<Uint32>(ColData.size() * sizeof(float4));
    RefCntAutoPtr<IBuffer> pColDataBuffer;
    pDevice->CreateBuffer(ColBuffDesc, nullptr, &pColDataBuffer);
    ASSERT_TRUE(pColDataBuffer);

    if (BufferUsage == USAGE_DYNAMIC)
    {
        {
            MapHelper<float4> PosBufferData{pContext, pPosDataBuffer, MAP_WRITE, MAP_FLAG_DISCARD};
            memcpy(PosBufferData, PosData.data(), StaticCast<size_t>(PosBuffDesc.Size));
        }
        {
            MapHelper<float4> ColBufferData{pContext, pColDataBuffer, MAP_WRITE, MAP_FLAG_DISCARD};
            memcpy(ColBufferData, ColData.data(), StaticCast<size_t>(ColBuffDesc.Size));
        }
    }
    else
    {
        pContext->UpdateBuffer(pPosDataBuffer, 0, PosBuffDesc.Size, PosData.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->UpdateBuffer(pColDataBuffer, 0, ColBuffDesc.Size, ColData.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    enum BufferType
    {
        Pos,
        Col
    };
    auto SetBuffer = [CBType, BuffMode](IPipelineState* pPSO, IShaderResourceBinding* pSRB, const char* Name, IBuffer* pBuffer, Uint32 Offset) //
    {
        RefCntAutoPtr<IBufferView> pBuffView;
        if (BuffMode == BUFFER_MODE_STRUCTURED)
        {
            BufferViewDesc ViewDesc;
            ViewDesc.ViewType   = BUFFER_VIEW_SHADER_RESOURCE;
            ViewDesc.ByteOffset = Offset;
            ViewDesc.ByteWidth  = sizeof(float4) * 3;
            pBuffer->CreateView(ViewDesc, &pBuffView);
        }

        if (CBType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        {
            if (BuffMode == BUFFER_MODE_STRUCTURED)
                pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, Name)->Set(pBuffView);
            else
                pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, Name)->SetBufferRange(pBuffer, Offset, sizeof(float4) * 3);
        }
        else
        {
            if (BuffMode == BUFFER_MODE_STRUCTURED)
                pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Name)->Set(pBuffView);
            else
                pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Name)->SetBufferRange(pBuffer, Offset, sizeof(float4) * 3);
        }
    };

    if (UseSetOffset)
    {
        SetBuffer(pPSO0, pSRB0, PosResourceName, pPosDataBuffer, BaseOffset);
        SetBuffer(pPSO0, pSRB0, ColResourceName, pColDataBuffer, BaseOffset);
    }
    else
    {
        SetBuffer(pPSO0, pSRB0, PosResourceName, pPosDataBuffer, PosOffsets[0]);
        SetBuffer(pPSO1, pSRB1, PosResourceName, pPosDataBuffer, PosOffsets[1]);
        SetBuffer(pPSO0, pSRB0, ColResourceName, pColDataBuffer, ColOffsets[0]);
        SetBuffer(pPSO1, pSRB1, ColResourceName, pColDataBuffer, ColOffsets[1]);
    }

    pPSO0->InitializeStaticSRBResources(pSRB0);
    if (pSRB1)
        pPSO1->InitializeStaticSRBResources(pSRB1);

    SetRenderTargets(pPSO0);

    pContext->CommitShaderResources(pSRB0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (UseSetOffset)
    {
        pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, PosResourceName)->SetBufferOffset(PosOffsets[0] - BaseOffset);
        pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, ColResourceName)->SetBufferOffset(ColOffsets[0] - BaseOffset);
    }

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    if (UseSetOffset)
    {
        pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, PosResourceName)->SetBufferOffset(PosOffsets[1] - BaseOffset);
        if (CBType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
        {
            SetBuffer(nullptr, pSRB0, ColResourceName, pColDataBuffer, ColOffsets[1]);
            pContext->CommitShaderResources(pSRB0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
        else
        {
            pSRB0->GetVariableByName(SHADER_TYPE_VERTEX, ColResourceName)->SetBufferOffset(ColOffsets[1] - BaseOffset);
        }
    }
    else
    {
        pContext->CommitShaderResources(pSRB1, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    pContext->Draw(drawAttrs);

    Present();
}

void DrawCommandTest::TestUniOrStructBufferOffsets(BUFFER_MODE BuffMode)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test buffer offsets - VS";
        ShaderCI.Source          = BuffMode == BUFFER_MODE_STRUCTURED ? HLSL::DrawTest_VSStructuredBuffers.c_str() : HLSL::DrawTest_VSUniformBuffers.c_str();
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Draw command test buffer offsets - PS";
        ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    for (Uint32 cb_type = 0; cb_type < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++cb_type)
    {
        const auto CBType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(cb_type);

        for (Uint32 is_dynamic = 0; is_dynamic < 2; ++is_dynamic)
        {
            const auto Usage = is_dynamic != 0 ? USAGE_DYNAMIC : USAGE_DEFAULT;
            for (Uint32 no_dyn_buffers = 0; no_dyn_buffers < 2; ++no_dyn_buffers)
            {
                const auto ShaderVarFlag = no_dyn_buffers != 0 ? SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS : SHADER_VARIABLE_FLAG_NONE;
                if ((ShaderVarFlag & SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS) && (Usage == USAGE_DYNAMIC))
                    continue;

                DrawWithUniOrStructBufferOffsets(pVS, pPS, BuffMode, CBType, Usage, ShaderVarFlag);
                std::cout << TestingEnvironment::GetCurrentTestStatusString() << ' '
                          << GetShaderVariableTypeLiteralName(CBType)
                          << ", " << GetUsageString(Usage) << " buff"
                          << (no_dyn_buffers != 0 ? ", NO_DYNAMIC_BUFFERS" : "")
                          << std::endl;
            }
        }
    }
}


// Test drawing with uniform buffer offsets
TEST_F(DrawCommandTest, UniformBufferOffsets)
{
    TestUniOrStructBufferOffsets(BUFFER_MODE_UNDEFINED);
}

// Test drawing with structured buffer offsets
TEST_F(DrawCommandTest, StructBufferOffsets)
{
    TestUniOrStructBufferOffsets(BUFFER_MODE_STRUCTURED);
}

// Test uniform buffer offsets with MSL shaders
TEST_F(DrawCommandTest, UniformBufferOffsets_MSL)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().IsMetalDevice())
        GTEST_SKIP() << "This is a Metal-specific test";

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "VSMain";
        ShaderCI.Desc.Name       = "Draw command test buffer offsets - VS";
        ShaderCI.Source          = MSL::DrawTest_ConstantBuffers.c_str();
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "PSMain";
        ShaderCI.Desc.Name       = "Draw command test buffer offsets - PS";
        ShaderCI.Source          = MSL::DrawTest_ConstantBuffers.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    DrawWithUniOrStructBufferOffsets(pVS, pPS, BUFFER_MODE_UNDEFINED, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE,
                                     USAGE_DYNAMIC, SHADER_VARIABLE_FLAG_NONE);
}

} // namespace
