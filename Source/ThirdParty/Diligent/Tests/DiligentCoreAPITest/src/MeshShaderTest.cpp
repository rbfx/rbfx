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
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/MeshShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D12_SUPPORTED
void MeshShaderDrawReferenceD3D12(ISwapChain* pSwapChain);
void MeshShaderIndirectDrawReferenceD3D12(ISwapChain* pSwapChain);
void AmplificationShaderDrawReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if VULKAN_SUPPORTED
void MeshShaderDrawReferenceVk(ISwapChain* pSwapChain);
void MeshShaderIndirectDrawReferenceVk(ISwapChain* pSwapChain);
void AmplificationShaderDrawReferenceVk(ISwapChain* pSwapChain);
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(MeshShaderTest, DrawTriangle)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                MeshShaderDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                MeshShaderDrawReferenceVk(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_UNDEFINED: // to avoid empty switch
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Mesh shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_UNDEFINED; // unused
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - MS";
        ShaderCI.Source          = HLSL::MeshShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - PS";
        ShaderCI.Source          = HLSL::MeshShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSODesc.Name = "Mesh shader test";

    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawMeshAttribs drawAttrs(1, DRAW_FLAG_VERIFY_ALL);
    pContext->DrawMesh(drawAttrs);

    pSwapChain->Present();
}


TEST(MeshShaderTest, DrawTriangleIndirect)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                MeshShaderIndirectDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                MeshShaderIndirectDrawReferenceVk(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_D3D11:
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
            case RENDER_DEVICE_TYPE_METAL:
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Mesh shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = pDevice->GetDeviceInfo().IsGLDevice();

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - MS";
        ShaderCI.Source          = HLSL::MeshShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - PS";
        ShaderCI.Source          = HLSL::MeshShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    struct IndirectAndCountBuffData
    {
        char   Unused[16];
        Uint32 IndirectData[3] = {};
        Uint32 End;
    };
    IndirectAndCountBuffData Data;

    if (pDevice->GetDeviceInfo().IsVulkanDevice())
    {
        Data.IndirectData[0] = 1;   // TaskCount
        Data.IndirectData[1] = 0;   // FirstTask
        Data.IndirectData[2] = ~0u; // ignored
    }
    else
    {
        Data.IndirectData[0] = 1; // ThreadGroupCountX
        Data.IndirectData[1] = 1; // ThreadGroupCountY
        Data.IndirectData[2] = 1; // ThreadGroupCountZ
    }

    BufferDesc IndirectBufferDesc;
    IndirectBufferDesc.Name      = "Indirect buffer";
    IndirectBufferDesc.Usage     = USAGE_IMMUTABLE;
    IndirectBufferDesc.Size      = sizeof(Data);
    IndirectBufferDesc.BindFlags = BIND_INDIRECT_DRAW_ARGS;

    BufferData InitData{&Data, sizeof(Data)};

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(IndirectBufferDesc, &InitData, &pBuffer);

    pContext->SetPipelineState(pPSO);

    DrawMeshIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pBuffer;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.DrawArgsOffset                   = offsetof(IndirectAndCountBuffData, IndirectData);
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->DrawMeshIndirect(drawAttrs);

    pSwapChain->Present();
}


TEST(MeshShaderTest, DrawTriangleIndirectCount)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                MeshShaderIndirectDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                MeshShaderIndirectDrawReferenceVk(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_D3D11:
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
            case RENDER_DEVICE_TYPE_METAL:
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Mesh shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = pDevice->GetDeviceInfo().IsGLDevice();

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - MS";
        ShaderCI.Source          = HLSL::MeshShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - PS";
        ShaderCI.Source          = HLSL::MeshShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    struct IndirectAndCountBuffData
    {
        char   Unused[16];
        Uint32 Count;
        Uint32 IndirectData[3];
    };
    IndirectAndCountBuffData Data;

    if (pDevice->GetDeviceInfo().IsVulkanDevice())
    {
        Data.IndirectData[0] = 1;   // TaskCount
        Data.IndirectData[1] = 0;   // FirstTask
        Data.IndirectData[2] = ~0u; // ignored
    }
    else
    {
        Data.IndirectData[0] = 1; // ThreadGroupCountX
        Data.IndirectData[1] = 1; // ThreadGroupCountY
        Data.IndirectData[2] = 1; // ThreadGroupCountZ
    }
    Data.Count = 1;

    BufferDesc IndirectBufferDesc;
    IndirectBufferDesc.Name      = "Indirect & Count buffer";
    IndirectBufferDesc.Usage     = USAGE_IMMUTABLE;
    IndirectBufferDesc.Size      = sizeof(Data);
    IndirectBufferDesc.BindFlags = BIND_INDIRECT_DRAW_ARGS;

    BufferData InitData{&Data, sizeof(Data)};

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(IndirectBufferDesc, &InitData, &pBuffer);

    pContext->SetPipelineState(pPSO);

    DrawMeshIndirectAttribs drawAttrs;
    drawAttrs.pAttribsBuffer                   = pBuffer;
    drawAttrs.pCounterBuffer                   = pBuffer;
    drawAttrs.Flags                            = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.CounterOffset                    = offsetof(IndirectAndCountBuffData, Count);
    drawAttrs.CounterBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.DrawArgsOffset                   = offsetof(IndirectAndCountBuffData, IndirectData);
    drawAttrs.AttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    drawAttrs.CommandCount                     = Data.Count;

    pContext->DrawMeshIndirect(drawAttrs);

    pSwapChain->Present();
}


TEST(MeshShaderTest, DrawTrisWithAmplificationShader)
{
    auto* pEnv    = GPUTestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceInfo().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceInfo().Type;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                AmplificationShaderDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                AmplificationShaderDrawReferenceVk(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_UNDEFINED: // to avoid empty switch
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Amplification shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_UNDEFINED; // unused
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pAS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_AMPLIFICATION;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - AS";
        ShaderCI.Source          = HLSL::AmplificationShaderTest_AS.c_str();

        pDevice->CreateShader(ShaderCI, &pAS);
        ASSERT_NE(pAS, nullptr);
    }

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - MS";
        ShaderCI.Source          = HLSL::AmplificationShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - PS";
        ShaderCI.Source          = HLSL::AmplificationShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pAS = pAS;
    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawMeshAttribs drawAttrs(8, DRAW_FLAG_VERIFY_ALL);
    pContext->DrawMesh(drawAttrs);

    pSwapChain->Present();
}

} // namespace
