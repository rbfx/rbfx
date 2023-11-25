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

#include "ImGuiDiligentRenderer.hpp"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Diligent/Graphics/GraphicsTools/interface/MapHelper.hpp>

#include <cstddef>

namespace Diligent
{

static const char* VertexShaderHLSL = R"(
cbuffer Constants
{
    float4x4 ProjectionMatrix;
}

struct VSInput
{
    float2 pos : ATTRIB0;
    float2 uv  : ATTRIB1;
    float4 col : ATTRIB2;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.pos = mul(ProjectionMatrix, float4(VSIn.pos.xy, 0.0, 1.0));
    PSIn.col = VSIn.col;
    PSIn.uv  = VSIn.uv;
}
)";

static const char* PixelShaderHLSL = R"(
struct PSInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

Texture2D    Texture;
SamplerState Texture_sampler;

float4 main(in PSInput PSIn) : SV_Target
{
    return PSIn.col * Texture.Sample(Texture_sampler, PSIn.uv);
}
)";



static const char* VertexShaderGLSL = R"(
#ifdef VULKAN
#   define BINDING(X) layout(binding=X)
#   define OUT_LOCATION(X) layout(location=X) // Requires separable programs
#else
#   define BINDING(X)
#   define OUT_LOCATION(X)
#endif
BINDING(0) uniform Constants
{
    mat4 ProjectionMatrix;
};

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_col;

OUT_LOCATION(0) out vec4 vsout_col;
OUT_LOCATION(1) out vec2 vsout_uv;

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    gl_Position = ProjectionMatrix * vec4(in_pos.xy, 0.0, 1.0);
    vsout_col = in_col;
    vsout_uv  = in_uv;
}
)";

static const char* PixelShaderGLSL = R"(
#ifdef VULKAN
#   define BINDING(X) layout(binding=X)
#   define IN_LOCATION(X) layout(location=X) // Requires separable programs
#else
#   define BINDING(X)
#   define IN_LOCATION(X)
#endif
BINDING(0) uniform sampler2D Texture;

IN_LOCATION(0) in vec4 vsout_col;
IN_LOCATION(1) in vec2 vsout_uv;

layout(location = 0) out vec4 psout_col;

void main()
{
    psout_col = vsout_col * texture(Texture, vsout_uv);
}
)";


// clang-format off

// glslangValidator.exe -V -e main --vn VertexShader_SPIRV ImGUI.vert

static constexpr uint32_t VertexShader_SPIRV[] =
{
    0x07230203,0x00010000,0x0008000a,0x00000028,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000b000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x00000016,0x00000020,
	0x00000022,0x00000025,0x00000026,0x00030003,0x00000002,0x000001a4,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00060005,0x00000008,0x505f6c67,0x65567265,0x78657472,0x00000000,
	0x00060006,0x00000008,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000000a,
	0x00000000,0x00050005,0x0000000e,0x736e6f43,0x746e6174,0x00000073,0x00080006,0x0000000e,
	0x00000000,0x6a6f7250,0x69746365,0x614d6e6f,0x78697274,0x00000000,0x00030005,0x00000010,
	0x00000000,0x00040005,0x00000016,0x705f6e69,0x0000736f,0x00050005,0x00000020,0x756f7376,
	0x6f635f74,0x0000006c,0x00040005,0x00000022,0x635f6e69,0x00006c6f,0x00050005,0x00000025,
	0x756f7376,0x76755f74,0x00000000,0x00040005,0x00000026,0x755f6e69,0x00000076,0x00050048,
	0x00000008,0x00000000,0x0000000b,0x00000000,0x00030047,0x00000008,0x00000002,0x00040048,
	0x0000000e,0x00000000,0x00000005,0x00050048,0x0000000e,0x00000000,0x00000023,0x00000000,
	0x00050048,0x0000000e,0x00000000,0x00000007,0x00000010,0x00030047,0x0000000e,0x00000002,
	0x00040047,0x00000010,0x00000022,0x00000000,0x00040047,0x00000010,0x00000021,0x00000000,
	0x00040047,0x00000016,0x0000001e,0x00000000,0x00040047,0x00000020,0x0000001e,0x00000000,
	0x00040047,0x00000022,0x0000001e,0x00000002,0x00040047,0x00000025,0x0000001e,0x00000001,
	0x00040047,0x00000026,0x0000001e,0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,
	0x0003001e,0x00000008,0x00000007,0x00040020,0x00000009,0x00000003,0x00000008,0x0004003b,
	0x00000009,0x0000000a,0x00000003,0x00040015,0x0000000b,0x00000020,0x00000001,0x0004002b,
	0x0000000b,0x0000000c,0x00000000,0x00040018,0x0000000d,0x00000007,0x00000004,0x0003001e,
	0x0000000e,0x0000000d,0x00040020,0x0000000f,0x00000002,0x0000000e,0x0004003b,0x0000000f,
	0x00000010,0x00000002,0x00040020,0x00000011,0x00000002,0x0000000d,0x00040017,0x00000014,
	0x00000006,0x00000002,0x00040020,0x00000015,0x00000001,0x00000014,0x0004003b,0x00000015,
	0x00000016,0x00000001,0x0004002b,0x00000006,0x00000018,0x00000000,0x0004002b,0x00000006,
	0x00000019,0x3f800000,0x00040020,0x0000001e,0x00000003,0x00000007,0x0004003b,0x0000001e,
	0x00000020,0x00000003,0x00040020,0x00000021,0x00000001,0x00000007,0x0004003b,0x00000021,
	0x00000022,0x00000001,0x00040020,0x00000024,0x00000003,0x00000014,0x0004003b,0x00000024,
	0x00000025,0x00000003,0x0004003b,0x00000015,0x00000026,0x00000001,0x00050036,0x00000002,
	0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000011,0x00000012,
	0x00000010,0x0000000c,0x0004003d,0x0000000d,0x00000013,0x00000012,0x0004003d,0x00000014,
	0x00000017,0x00000016,0x00050051,0x00000006,0x0000001a,0x00000017,0x00000000,0x00050051,
	0x00000006,0x0000001b,0x00000017,0x00000001,0x00070050,0x00000007,0x0000001c,0x0000001a,
	0x0000001b,0x00000018,0x00000019,0x00050091,0x00000007,0x0000001d,0x00000013,0x0000001c,
	0x00050041,0x0000001e,0x0000001f,0x0000000a,0x0000000c,0x0003003e,0x0000001f,0x0000001d,
	0x0004003d,0x00000007,0x00000023,0x00000022,0x0003003e,0x00000020,0x00000023,0x0004003d,
	0x00000014,0x00000027,0x00000026,0x0003003e,0x00000025,0x00000027,0x000100fd,0x00010038
};

static constexpr uint32_t FragmentShader_SPIRV[] =
{
	0x07230203,0x00010000,0x0008000a,0x00000018,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000b,0x00000014,
	0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001a4,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00050005,0x00000009,0x756f7370,0x6f635f74,0x0000006c,0x00050005,
	0x0000000b,0x756f7376,0x6f635f74,0x0000006c,0x00040005,0x00000010,0x74786554,0x00657275,
	0x00050005,0x00000014,0x756f7376,0x76755f74,0x00000000,0x00040047,0x00000009,0x0000001e,
	0x00000000,0x00040047,0x0000000b,0x0000001e,0x00000000,0x00040047,0x00000010,0x00000022,
	0x00000000,0x00040047,0x00000010,0x00000021,0x00000000,0x00040047,0x00000014,0x0000001e,
	0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040020,0x0000000a,0x00000001,
	0x00000007,0x0004003b,0x0000000a,0x0000000b,0x00000001,0x00090019,0x0000000d,0x00000006,
	0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000e,
	0x0000000d,0x00040020,0x0000000f,0x00000000,0x0000000e,0x0004003b,0x0000000f,0x00000010,
	0x00000000,0x00040017,0x00000012,0x00000006,0x00000002,0x00040020,0x00000013,0x00000001,
	0x00000012,0x0004003b,0x00000013,0x00000014,0x00000001,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x00000007,0x0000000c,0x0000000b,
	0x0004003d,0x0000000e,0x00000011,0x00000010,0x0004003d,0x00000012,0x00000015,0x00000014,
	0x00050057,0x00000007,0x00000016,0x00000011,0x00000015,0x00050085,0x00000007,0x00000017,
	0x0000000c,0x00000016,0x0003003e,0x00000009,0x00000017,0x000100fd,0x00010038
};
// clang-format on


static const char* ShadersMSL = R"(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct VSConstants
{
    float4x4 ProjectionMatrix;
};

struct VSIn
{
    float2 pos [[attribute(0)]];
    float2 uv  [[attribute(1)]];
    float4 col [[attribute(2)]];
};

struct VSOut
{
    float4 col [[user(locn0)]];
    float2 uv  [[user(locn1)]];
    float4 pos [[position]];
};

vertex VSOut vs_main(VSIn in [[stage_in]], constant VSConstants& Constants [[buffer(0)]])
{
    VSOut out = {};
    out.pos = Constants.ProjectionMatrix * float4(in.pos, 0.0, 1.0);
    out.col = in.col;
    out.uv  = in.uv;
    return out;
}

struct PSOut
{
    float4 col [[color(0)]];
};

fragment PSOut ps_main(VSOut in [[stage_in]],
                       texture2d<float> Texture [[texture(0)]],
                       sampler Texture_sampler  [[sampler(0)]])
{
    PSOut out = {};
    out.col = in.col * Texture.sample(Texture_sampler, in.uv);
    return out;
}
)";

ImGuiDiligentRenderer::ImGuiDiligentRenderer(IRenderDevice* pDevice,
                                             TEXTURE_FORMAT BackBufferFmt,
                                             TEXTURE_FORMAT DepthBufferFmt,
                                             Uint32         InitialVertexBufferSize,
                                             Uint32         InitialIndexBufferSize) :
    // clang-format off
    m_pDevice         {pDevice},
    m_BackBufferFmt   {BackBufferFmt},
    m_DepthBufferFmt  {DepthBufferFmt},
    m_VertexBufferSize{InitialVertexBufferSize},
    m_IndexBufferSize {InitialIndexBufferSize}
// clang-format on
{
    //Check support vertex offset
    m_BaseVertexSupported = pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_BASE_VERTEX;

    // Setup back-end capabilities flags
    IMGUI_CHECKVERSION();
    ImGuiIO& IO = ImGui::GetIO();

    IO.BackendRendererName = "ImGuiDiligentRenderer";
    if (m_BaseVertexSupported)
        IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    CreateDeviceObjects();
}

ImGuiDiligentRenderer::~ImGuiDiligentRenderer()
{
}

void ImGuiDiligentRenderer::NewFrame(Uint32            RenderSurfaceWidth,
                                     Uint32            RenderSurfaceHeight,
                                     SURFACE_TRANSFORM SurfacePreTransform)
{
    if (!m_pPSO)
        CreateDeviceObjects();
    m_RenderSurfaceWidth  = RenderSurfaceWidth;
    m_RenderSurfaceHeight = RenderSurfaceHeight;
    m_SurfacePreTransform = SurfacePreTransform;
}

void ImGuiDiligentRenderer::EndFrame()
{
}

void ImGuiDiligentRenderer::InvalidateDeviceObjects()
{
    m_pVB.Release();
    m_pIB.Release();
    m_pVertexConstantBuffer.Release();
    m_pPSO.Release();
    m_pFontSRV.Release();
    m_pSRB.Release();
}

void ImGuiDiligentRenderer::CreateDeviceObjects()
{
    InvalidateDeviceObjects();

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;

    const auto DeviceType = m_pDevice->GetDeviceInfo().Type;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc = {"Imgui VS", SHADER_TYPE_VERTEX, true};
        switch (DeviceType)
        {
            case RENDER_DEVICE_TYPE_VULKAN:
                ShaderCI.ByteCode     = VertexShader_SPIRV;
                ShaderCI.ByteCodeSize = sizeof(VertexShader_SPIRV);
                break;

            case RENDER_DEVICE_TYPE_D3D11:
            case RENDER_DEVICE_TYPE_D3D12:
                ShaderCI.Source = VertexShaderHLSL;
                break;

            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                ShaderCI.Source = VertexShaderGLSL;
                break;

            case RENDER_DEVICE_TYPE_METAL:
                ShaderCI.Source     = ShadersMSL;
                ShaderCI.EntryPoint = "vs_main";
                break;

            default:
                UNEXPECTED("Unknown render device type");
        }
        m_pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc = {"Imgui PS", SHADER_TYPE_PIXEL, true};
        switch (DeviceType)
        {
            case RENDER_DEVICE_TYPE_VULKAN:
                ShaderCI.ByteCode     = FragmentShader_SPIRV;
                ShaderCI.ByteCodeSize = sizeof(FragmentShader_SPIRV);
                break;

            case RENDER_DEVICE_TYPE_D3D11:
            case RENDER_DEVICE_TYPE_D3D12:
                ShaderCI.Source = PixelShaderHLSL;
                break;

            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                ShaderCI.Source = PixelShaderGLSL;
                break;

            case RENDER_DEVICE_TYPE_METAL:
                ShaderCI.Source     = ShadersMSL;
                ShaderCI.EntryPoint = "ps_main";
                break;

            default:
                UNEXPECTED("Unknown render device type");
        }
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "ImGUI PSO";
    auto& GraphicsPipeline     = PSOCreateInfo.GraphicsPipeline;

    GraphicsPipeline.NumRenderTargets  = 1;
    GraphicsPipeline.RTVFormats[0]     = m_BackBufferFmt;
    GraphicsPipeline.DSVFormat         = m_DepthBufferFmt;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.RasterizerDesc.ScissorEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    auto& RT0                 = GraphicsPipeline.BlendDesc.RenderTargets[0];
    RT0.BlendEnable           = True;
    RT0.SrcBlend              = BLEND_FACTOR_SRC_ALPHA;
    RT0.DestBlend             = BLEND_FACTOR_INV_SRC_ALPHA;
    RT0.BlendOp               = BLEND_OPERATION_ADD;
    RT0.SrcBlendAlpha         = BLEND_FACTOR_INV_SRC_ALPHA;
    RT0.DestBlendAlpha        = BLEND_FACTOR_ZERO;
    RT0.BlendOpAlpha          = BLEND_OPERATION_ADD;
    RT0.RenderTargetWriteMask = COLOR_MASK_ALL;

    LayoutElement VSInputs[] //
        {
            {0, 0, 2, VT_FLOAT32},    // pos
            {1, 0, 2, VT_FLOAT32},    // uv
            {2, 0, 4, VT_UINT8, True} // col
        };
    GraphicsPipeline.InputLayout.NumElements    = _countof(VSInputs);
    GraphicsPipeline.InputLayout.LayoutElements = VSInputs;

    ShaderResourceVariableDesc Variables[] =
        {
            {SHADER_TYPE_PIXEL, "Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
        };
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Variables;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc SamLinearWrap;
    SamLinearWrap.AddressU = TEXTURE_ADDRESS_WRAP;
    SamLinearWrap.AddressV = TEXTURE_ADDRESS_WRAP;
    SamLinearWrap.AddressW = TEXTURE_ADDRESS_WRAP;
    ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_PIXEL, "Texture", SamLinearWrap} //
        };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    {
        BufferDesc BuffDesc;
        BuffDesc.Size           = sizeof(float4x4);
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pVertexConstantBuffer);
    }
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pVertexConstantBuffer);

    CreateFontsTexture();
}

void ImGuiDiligentRenderer::CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& IO = ImGui::GetIO();

    unsigned char* pData  = nullptr;
    int            Width  = 0;
    int            Weight = 0;
    IO.Fonts->GetTexDataAsRGBA32(&pData, &Width, &Weight);

    TextureDesc FontTexDesc;
    FontTexDesc.Name      = "Imgui font texture";
    FontTexDesc.Type      = RESOURCE_DIM_TEX_2D;
    FontTexDesc.Width     = static_cast<Uint32>(Width);
    FontTexDesc.Height    = static_cast<Uint32>(Weight);
    FontTexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
    FontTexDesc.BindFlags = BIND_SHADER_RESOURCE;
    FontTexDesc.Usage     = USAGE_IMMUTABLE;

    TextureSubResData Mip0Data[] = {{pData, 4 * Uint64{FontTexDesc.Width}}};
    TextureData       InitData(Mip0Data, _countof(Mip0Data));

    RefCntAutoPtr<ITexture> pFontTex;
    m_pDevice->CreateTexture(FontTexDesc, &InitData, &pFontTex);
    m_pFontSRV = pFontTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    m_pSRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
    m_pTextureVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Texture");
    VERIFY_EXPR(m_pTextureVar != nullptr);

    // Store our identifier
    IO.Fonts->TexID = (ImTextureID)m_pFontSRV;
}

float4 ImGuiDiligentRenderer::TransformClipRect(const ImVec2& DisplaySize, const float4& rect) const
{
    switch (m_SurfacePreTransform)
    {
        case SURFACE_TRANSFORM_IDENTITY:
            return rect;

        case SURFACE_TRANSFORM_ROTATE_90:
        {
            // The image content is rotated 90 degrees clockwise. The origin is in the left-top corner.
            //
            //                                                             DsplSz.y
            //                a.x                                            -a.y     a.y     Old origin
            //              0---->|                                       0------->|<------| /
            //           0__|_____|____________________                0__|________|_______|/
            //            | |     '                    |                | |        '       |
            //        a.y | |     '                    |            a.x | |        '       |
            //           _V_|_ _ _a____b               |               _V_|_ _d'___a'      |
            //            A |     |    |               |                  |   |    |       |
            //  DsplSz.y  | |     |____|               |                  |   |____|       |
            //    -a.y    | |     d    c               |                  |   c'   b'      |
            //           _|_|__________________________|                  |                |
            //              A                                             |                |
            //              |-----> Y'                                    |                |
            //         New Origin                                         |________________|
            //
            float2 a{rect.x, rect.y};
            float2 c{rect.z, rect.w};
            return float4 //
                {
                    DisplaySize.y - c.y, // min_x = c'.x
                    a.x,                 // min_y = a'.y
                    DisplaySize.y - a.y, // max_x = a'.x
                    c.x                  // max_y = c'.y
                };
        }

        case SURFACE_TRANSFORM_ROTATE_180:
        {
            // The image content is rotated 180 degrees clockwise. The origin is in the left-top corner.
            //
            //                a.x                                               DsplSz.x - a.x
            //              0---->|                                         0------------------>|
            //           0__|_____|____________________                 0_ _|___________________|______
            //            | |     '                    |                  | |                   '      |
            //        a.y | |     '                    |        DsplSz.y  | |              c'___d'     |
            //           _V_|_ _ _a____b               |          -a.y    | |              |    |      |
            //              |     |    |               |                 _V_|_ _ _ _ _ _ _ |____|      |
            //              |     |____|               |                    |              b'   a'     |
            //              |     d    c               |                    |                          |
            //              |__________________________|                    |__________________________|
            //                                         A                                               A
            //                                         |                                               |
            //                                     New Origin                                      Old Origin
            float2 a{rect.x, rect.y};
            float2 c{rect.z, rect.w};
            return float4 //
                {
                    DisplaySize.x - c.x, // min_x = c'.x
                    DisplaySize.y - c.y, // min_y = c'.y
                    DisplaySize.x - a.x, // max_x = a'.x
                    DisplaySize.y - a.y  // max_y = a'.y
                };
        }

        case SURFACE_TRANSFORM_ROTATE_270:
        {
            // The image content is rotated 270 degrees clockwise. The origin is in the left-top corner.
            //
            //              0  a.x     DsplSz.x-a.x   New Origin              a.y
            //              |---->|<-------------------|                    0----->|
            //          0_ _|_____|____________________V                 0 _|______|_________
            //            | |     '                    |                  | |      '         |
            //            | |     '                    |                  | |      '         |
            //        a.y_V_|_ _ _a____b               |        DsplSz.x  | |      '         |
            //              |     |    |               |          -a.x    | |      '         |
            //              |     |____|               |                  | |      b'___c'   |
            //              |     d    c               |                  | |      |    |    |
            //  DsplSz.y _ _|__________________________|                 _V_|_ _ _ |____|    |
            //                                                              |      a'   d'   |
            //                                                              |                |
            //                                                              |________________|
            //                                                              A
            //                                                              |
            //                                                            Old origin
            float2 a{rect.x, rect.y};
            float2 c{rect.z, rect.w};
            return float4 //
                {
                    a.y,                 // min_x = a'.x
                    DisplaySize.x - c.x, // min_y = c'.y
                    c.y,                 // max_x = c'.x
                    DisplaySize.x - a.x  // max_y = a'.y
                };
        }

        case SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
            return rect;

        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
            UNEXPECTED("Mirror transforms are not supported");
            return rect;

        default:
            UNEXPECTED("Unknown transform");
            return rect;
    }
}

void ImGuiDiligentRenderer::RenderDrawData(IDeviceContext* pCtx, ImDrawData* pDrawData, IPipelineState* pUserPSO, IShaderResourceBinding* pUserSRB, IShaderResourceVariable* pUserTextureVar, IShaderResourceVariable* pUserConstantsVar)
{
    // Avoid rendering when minimized
    if (pDrawData->DisplaySize.x <= 0.0f || pDrawData->DisplaySize.y <= 0.0f)
        return;

    // Setup pipeline state if provided by the user.
    IPipelineState* pPSO = m_pPSO;
    IShaderResourceBinding* pSRB = m_pSRB;
    IShaderResourceVariable* pTextureVar = m_pTextureVar;
    if (pUserPSO && pUserSRB && pUserTextureVar && pUserConstantsVar)
    {
        pPSO = pUserPSO;
        pSRB = pUserSRB;
        pTextureVar = pUserTextureVar;
        pUserConstantsVar->Set(m_pVertexConstantBuffer);
    }

    // Create and grow vertex/index buffers if needed
    if (!m_pVB || static_cast<int>(m_VertexBufferSize) < pDrawData->TotalVtxCount)
    {
        m_pVB.Release();
        while (static_cast<int>(m_VertexBufferSize) < pDrawData->TotalVtxCount)
            m_VertexBufferSize *= 2;

        BufferDesc VBDesc;
        VBDesc.Name           = "Imgui vertex buffer";
        VBDesc.BindFlags      = BIND_VERTEX_BUFFER;
        VBDesc.Size           = m_VertexBufferSize * sizeof(ImDrawVert);
        VBDesc.Usage          = USAGE_DYNAMIC;
        VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(VBDesc, nullptr, &m_pVB);
    }

    if (!m_pIB || static_cast<int>(m_IndexBufferSize) < pDrawData->TotalIdxCount)
    {
        m_pIB.Release();
        while (static_cast<int>(m_IndexBufferSize) < pDrawData->TotalIdxCount)
            m_IndexBufferSize *= 2;

        BufferDesc IBDesc;
        IBDesc.Name           = "Imgui index buffer";
        IBDesc.BindFlags      = BIND_INDEX_BUFFER;
        IBDesc.Size           = m_IndexBufferSize * sizeof(ImDrawIdx);
        IBDesc.Usage          = USAGE_DYNAMIC;
        IBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(IBDesc, nullptr, &m_pIB);
    }

    {
        MapHelper<ImDrawVert> Verices(pCtx, m_pVB, MAP_WRITE, MAP_FLAG_DISCARD);
        MapHelper<ImDrawIdx>  Indices(pCtx, m_pIB, MAP_WRITE, MAP_FLAG_DISCARD);

        ImDrawVert* pVtxDst = Verices;
        ImDrawIdx*  pIdxDst = Indices;
        for (Int32 CmdListID = 0; CmdListID < pDrawData->CmdListsCount; CmdListID++)
        {
            const ImDrawList* pCmdList = pDrawData->CmdLists[CmdListID];
            memcpy(pVtxDst, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(pIdxDst, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            pVtxDst += pCmdList->VtxBuffer.Size;
            pIdxDst += pCmdList->IdxBuffer.Size;
        }
    }

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from pDrawData->DisplayPos (top left) to pDrawData->DisplayPos+data_data->DisplaySize (bottom right).
    // DisplayPos is (0,0) for single viewport apps.
    {
        // DisplaySize always refers to the logical dimensions that account for pre-transform, hence
        // the aspect ratio will be correct after applying appropriate rotation.
        float L = pDrawData->DisplayPos.x;
        float R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
        float T = pDrawData->DisplayPos.y;
        float B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;

        // clang-format off
        float4x4 Projection
        {
            2.0f / (R - L),                  0.0f,   0.0f,   0.0f,
            0.0f,                  2.0f / (T - B),   0.0f,   0.0f,
            0.0f,                            0.0f,   0.5f,   0.0f,
            (R + L) / (L - R),  (T + B) / (B - T),   0.5f,   1.0f
        };
        // clang-format on

        // Bake pre-transform into projection
        switch (m_SurfacePreTransform)
        {
            case SURFACE_TRANSFORM_IDENTITY:
                // Nothing to do
                break;

            case SURFACE_TRANSFORM_ROTATE_90:
                // The image content is rotated 90 degrees clockwise.
                Projection *= float4x4::RotationZ(-PI_F * 0.5f);
                break;

            case SURFACE_TRANSFORM_ROTATE_180:
                // The image content is rotated 180 degrees clockwise.
                Projection *= float4x4::RotationZ(-PI_F * 1.0f);
                break;

            case SURFACE_TRANSFORM_ROTATE_270:
                // The image content is rotated 270 degrees clockwise.
                Projection *= float4x4::RotationZ(-PI_F * 1.5f);
                break;

            case SURFACE_TRANSFORM_OPTIMAL:
                UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
                break;

            case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
            case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
            case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
            case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
                UNEXPECTED("Mirror transforms are not supported");
                break;

            default:
                UNEXPECTED("Unknown transform");
        }

        MapHelper<float4x4> CBData(pCtx, m_pVertexConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = Projection;
    }

    ITextureView* pLastTextureView = nullptr;
    auto SetupRenderState = [&]() //
    {
        // Setup shader and vertex buffers
        IBuffer* pVBs[] = {m_pVB};
        pCtx->SetVertexBuffers(0, 1, pVBs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        pCtx->SetIndexBuffer(m_pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pCtx->SetPipelineState(pPSO);

        const float blend_factor[4] = {0.f, 0.f, 0.f, 0.f};
        pCtx->SetBlendFactors(blend_factor);

        Viewport vp;
        vp.Width    = static_cast<float>(m_RenderSurfaceWidth);
        vp.Height   = static_cast<float>(m_RenderSurfaceHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0;
        pCtx->SetViewports(1,
                           &vp,
                           static_cast<Uint32>(m_RenderSurfaceWidth),
                           static_cast<Uint32>(m_RenderSurfaceHeight));

        pLastTextureView = nullptr;
    };

    SetupRenderState();

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    Uint32 GlobalIdxOffset = 0;
    Uint32 GlobalVtxOffset = 0;

    for (Int32 CmdListID = 0; CmdListID < pDrawData->CmdListsCount; CmdListID++)
    {
        const ImDrawList* pCmdList = pDrawData->CmdLists[CmdListID];
        for (Int32 CmdID = 0; CmdID < pCmdList->CmdBuffer.Size; CmdID++)
        {
            const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[CmdID];
            if (pCmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pCmd->UserCallback == ImDrawCallback_ResetRenderState)
                    SetupRenderState();
                else
                    pCmd->UserCallback(pCmdList, pCmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                float4 ClipRect //
                    {
                        (pCmd->ClipRect.x - pDrawData->DisplayPos.x) * pDrawData->FramebufferScale.x,
                        (pCmd->ClipRect.y - pDrawData->DisplayPos.y) * pDrawData->FramebufferScale.y,
                        (pCmd->ClipRect.z - pDrawData->DisplayPos.x) * pDrawData->FramebufferScale.x,
                        (pCmd->ClipRect.w - pDrawData->DisplayPos.y) * pDrawData->FramebufferScale.y //
                    };
                // Apply pretransform
                ClipRect = TransformClipRect(pDrawData->DisplaySize, ClipRect);

                Rect Scissor //
                    {
                        static_cast<Int32>(ClipRect.x),
                        static_cast<Int32>(ClipRect.y),
                        static_cast<Int32>(ClipRect.z),
                        static_cast<Int32>(ClipRect.w) //
                    };
                pCtx->SetScissorRects(1,
                                      &Scissor,
                                      static_cast<Uint32>(m_RenderSurfaceWidth),
                                      static_cast<Uint32>(m_RenderSurfaceHeight));

                // Bind texture
                auto* pTextureView = reinterpret_cast<ITextureView*>(pCmd->TextureId);
                VERIFY_EXPR(pTextureView);
                if (pTextureView != pLastTextureView)
                {
                    pLastTextureView = pTextureView;
                    pTextureVar->Set(pTextureView);
                    pCtx->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                }

                DrawIndexedAttribs DrawAttrs{pCmd->ElemCount, sizeof(ImDrawIdx) == sizeof(Uint16) ? VT_UINT16 : VT_UINT32, DRAW_FLAG_VERIFY_STATES};
                DrawAttrs.FirstIndexLocation = pCmd->IdxOffset + GlobalIdxOffset;
                if (m_BaseVertexSupported)
                {
                    DrawAttrs.BaseVertex = pCmd->VtxOffset + GlobalVtxOffset;
                }
                else
                {
                    IBuffer* pVBs[]       = {m_pVB};
                    Uint64   VtxOffsets[] = {sizeof(ImDrawVert) * (size_t{pCmd->VtxOffset} + size_t{GlobalVtxOffset})};
                    pCtx->SetVertexBuffers(0, 1, pVBs, VtxOffsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
                }
                pCtx->DrawIndexed(DrawAttrs);
            }
        }
        GlobalIdxOffset += pCmdList->IdxBuffer.Size;
        GlobalVtxOffset += pCmdList->VtxBuffer.Size;
    }
}

} // namespace Diligent
