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

#include "D3D12/TestingEnvironmentD3D12.hpp"
#include "D3D12/TestingSwapChainD3D12.hpp"

#include "../include/DXGITypeConversions.hpp"
#include "../include/d3dx12_win.h"

#include "RenderDeviceD3D12.h"
#include "DeviceContextD3D12.h"

#include "InlineShaders/VariableShadingRateTestHLSL.h"
#include "VariableShadingRateTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

#ifdef NTDDI_WIN10_19H1

void VariableShadingRatePerDrawTestReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pVSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::PerDrawShadingRate_VS, L"main", nullptr, 0, L"vs_6_4", &pVSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    hr = pEnv->CompileDXILShader(HLSL::PerDrawShadingRate_PS, L"main", nullptr, 0, L"ps_6_4", &pPSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    D3D12_ROOT_SIGNATURE_DESC    RootSignatureDesc = {};
    CComPtr<ID3DBlob>            signature;
    CComPtr<ID3D12RootSignature> pd3d12RootSignature;

    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pd3d12RootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

    PSODesc.pRootSignature     = pd3d12RootSignature;
    PSODesc.VS.pShaderBytecode = pVSByteCode->GetBufferPointer();
    PSODesc.VS.BytecodeLength  = pVSByteCode->GetBufferSize();
    PSODesc.PS.pShaderBytecode = pPSByteCode->GetBufferPointer();
    PSODesc.PS.BytecodeLength  = pPSByteCode->GetBufferSize();

    PSODesc.BlendState        = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    PSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{D3D12_DEFAULT};

    PSODesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
    PSODesc.DepthStencilState.DepthEnable    = FALSE;
    PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    PSODesc.SampleMask = 0xFFFFFFFF;

    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.NumRenderTargets      = 1;
    PSODesc.RTVFormats[0]         = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    PSODesc.SampleDesc.Count      = 1;
    PSODesc.SampleDesc.Quality    = 0;
    PSODesc.NodeMask              = 0;
    PSODesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    CComPtr<ID3D12PipelineState> pd3d12PSO;
    hr = pd3d12Device->CreateGraphicsPipelineState(&PSODesc, __uuidof(pd3d12PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&pd3d12PSO)));
    VERIFY_EXPR(SUCCEEDED(hr));

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto RTVDescriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();
    pCmdList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, nullptr);

    const float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDescriptorHandle, ClearColor, 0, nullptr);

    D3D12_VIEWPORT d3d12VP{};
    d3d12VP.Width    = static_cast<float>(SCDesc.Width);
    d3d12VP.Height   = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);
    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CComPtr<ID3D12GraphicsCommandList5> pCmdList5;
    ASSERT_HRESULT_SUCCEEDED(pCmdList->QueryInterface(IID_PPV_ARGS(&pCmdList5))) << "Failed to get ID3D12GraphicsCommandList5";

    pCmdList5->RSSetShadingRate(D3D12_SHADING_RATE_2X2, nullptr);

    pCmdList->DrawInstanced(3, 1, 0, 0);

    pCmdList->Close();
    pEnv->ExecuteCommandList(pCmdList);
}


void VariableShadingRatePerPrimitiveTestReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pVSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::PerPrimitiveShadingRate_VS, L"main", nullptr, 0, L"vs_6_4", &pVSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    hr = pEnv->CompileDXILShader(HLSL::PerPrimitiveShadingRate_PS, L"main", nullptr, 0, L"ps_6_4", &pPSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    D3D12_ROOT_SIGNATURE_DESC    RootSignatureDesc = {};
    CComPtr<ID3DBlob>            signature;
    CComPtr<ID3D12RootSignature> pd3d12RootSignature;

    RootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pd3d12RootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

    PSODesc.pRootSignature     = pd3d12RootSignature;
    PSODesc.VS.pShaderBytecode = pVSByteCode->GetBufferPointer();
    PSODesc.VS.BytecodeLength  = pVSByteCode->GetBufferSize();
    PSODesc.PS.pShaderBytecode = pPSByteCode->GetBufferPointer();
    PSODesc.PS.BytecodeLength  = pPSByteCode->GetBufferSize();

    PSODesc.BlendState        = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    PSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{D3D12_DEFAULT};

    PSODesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
    PSODesc.DepthStencilState.DepthEnable    = FALSE;
    PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    PSODesc.SampleMask = 0xFFFFFFFF;

    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.NumRenderTargets      = 1;
    PSODesc.RTVFormats[0]         = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    PSODesc.SampleDesc.Count      = 1;
    PSODesc.SampleDesc.Quality    = 0;
    PSODesc.NodeMask              = 0;
    PSODesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    const D3D12_INPUT_ELEMENT_DESC Elements[] =
        {
            {"ATTRIB", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(PosAndRate, Pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"ATTRIB", 1, DXGI_FORMAT_R32_UINT, 0, offsetof(PosAndRate, Rate), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} //
        };
    PSODesc.InputLayout.NumElements        = _countof(Elements);
    PSODesc.InputLayout.pInputElementDescs = Elements;

    CComPtr<ID3D12PipelineState> pd3d12PSO;
    hr = pd3d12Device->CreateGraphicsPipelineState(&PSODesc, __uuidof(pd3d12PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&pd3d12PSO)));
    VERIFY_EXPR(SUCCEEDED(hr));

    const auto& Verts = VRSTestingConstants::PerPrimitive::Vertices;

    RefCntAutoPtr<IBuffer> pVB;
    {
        BufferData BuffData{Verts, sizeof(Verts)};
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Vertex buffer";
        BuffDesc.Size      = BuffData.DataSize;
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage     = USAGE_IMMUTABLE;

        pEnv->GetDevice()->CreateBuffer(BuffDesc, &BuffData, &pVB);
        ASSERT_NE(pVB, nullptr);
    }
    auto* pVBD3D12 = reinterpret_cast<ID3D12Resource*>(pVB->GetNativeHandle());

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto RTVDescriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();
    pCmdList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, nullptr);

    const float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDescriptorHandle, ClearColor, 0, nullptr);

    D3D12_VIEWPORT d3d12VP{};
    d3d12VP.Width    = static_cast<float>(SCDesc.Width);
    d3d12VP.Height   = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);
    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CComPtr<ID3D12GraphicsCommandList5> pCmdList5;
    ASSERT_HRESULT_SUCCEEDED(pCmdList->QueryInterface(IID_PPV_ARGS(&pCmdList5))) << "Failed to get ID3D12GraphicsCommandList5";

    const D3D12_SHADING_RATE_COMBINER Combiners[] = {D3D12_SHADING_RATE_COMBINER_OVERRIDE, D3D12_SHADING_RATE_COMBINER_PASSTHROUGH};
    pCmdList5->RSSetShadingRate(D3D12_SHADING_RATE_1X1, Combiners);

    D3D12_VERTEX_BUFFER_VIEW VBView{};
    VBView.BufferLocation = pVBD3D12->GetGPUVirtualAddress();
    VBView.StrideInBytes  = sizeof(Verts[0]);
    VBView.SizeInBytes    = sizeof(Verts);
    pCmdList->IASetVertexBuffers(0, 1, &VBView);

    pCmdList->DrawInstanced(_countof(Verts), 1, 0, 0);

    pCmdList->Close();
    pEnv->ExecuteCommandList(pCmdList);
}


RefCntAutoPtr<ITextureView> CreateShadingRateTexture(IRenderDevice* pDevice, ISwapChain* pSwapChain, Uint32 SampleCount = 1, Uint32 ArraySize = 1);

void VariableShadingRateTextureBasedTestReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pVSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::TextureBasedShadingRate_VS, L"main", nullptr, 0, L"vs_6_4", &pVSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    hr = pEnv->CompileDXILShader(HLSL::TextureBasedShadingRate_PS, L"main", nullptr, 0, L"ps_6_4", &pPSByteCode);
    VERIFY_EXPR(SUCCEEDED(hr));

    D3D12_ROOT_SIGNATURE_DESC    RootSignatureDesc = {};
    CComPtr<ID3DBlob>            signature;
    CComPtr<ID3D12RootSignature> pd3d12RootSignature;

    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pd3d12RootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

    PSODesc.pRootSignature     = pd3d12RootSignature;
    PSODesc.VS.pShaderBytecode = pVSByteCode->GetBufferPointer();
    PSODesc.VS.BytecodeLength  = pVSByteCode->GetBufferSize();
    PSODesc.PS.pShaderBytecode = pPSByteCode->GetBufferPointer();
    PSODesc.PS.BytecodeLength  = pPSByteCode->GetBufferSize();

    PSODesc.BlendState        = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    PSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{D3D12_DEFAULT};

    PSODesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
    PSODesc.DepthStencilState.DepthEnable    = FALSE;
    PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    PSODesc.SampleMask = 0xFFFFFFFF;

    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.NumRenderTargets      = 1;
    PSODesc.RTVFormats[0]         = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    PSODesc.SampleDesc.Count      = 1;
    PSODesc.SampleDesc.Quality    = 0;
    PSODesc.NodeMask              = 0;
    PSODesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    CComPtr<ID3D12PipelineState> pd3d12PSO;
    hr = pd3d12Device->CreateGraphicsPipelineState(&PSODesc, __uuidof(pd3d12PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&pd3d12PSO)));
    VERIFY_EXPR(SUCCEEDED(hr));

    auto pVRSView = CreateShadingRateTexture(pEnv->GetDevice(), pSwapChain);
    ASSERT_NE(pVRSView, nullptr);

    auto* pSRTexD3D12 = reinterpret_cast<ID3D12Resource*>(pVRSView->GetTexture()->GetNativeHandle());

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto RTVDescriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();
    pCmdList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, nullptr);

    const float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDescriptorHandle, ClearColor, 0, nullptr);

    D3D12_VIEWPORT d3d12VP{};
    d3d12VP.Width    = static_cast<float>(SCDesc.Width);
    d3d12VP.Height   = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);
    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CComPtr<ID3D12GraphicsCommandList5> pCmdList5;
    ASSERT_HRESULT_SUCCEEDED(pCmdList->QueryInterface(IID_PPV_ARGS(&pCmdList5))) << "Failed to get ID3D12GraphicsCommandList5";

    const D3D12_SHADING_RATE_COMBINER Combiners[] = {D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, D3D12_SHADING_RATE_COMBINER_OVERRIDE};
    pCmdList5->RSSetShadingRate(D3D12_SHADING_RATE_1X1, Combiners);
    pCmdList5->RSSetShadingRateImage(pSRTexD3D12);

    D3D12_RESOURCE_BARRIER Barrier{};
    Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.pResource   = pSRTexD3D12;
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    pCmdList->ResourceBarrier(1, &Barrier);

    pCmdList->DrawInstanced(3, 1, 0, 0);

    pCmdList->Close();
    pEnv->ExecuteCommandList(pCmdList);
}

#else

void VariableShadingRatePerDrawTestReferenceD3D12(ISwapChain* pSwapChain)
{}
void VariableShadingRatePerPrimitiveTestReferenceD3D12(ISwapChain* pSwapChain) {}
void VariableShadingRateTextureBasedTestReferenceD3D12(ISwapChain* pSwapChain) {}

#endif // NTDDI_WIN10_19H1

} // namespace Testing
} // namespace Diligent
