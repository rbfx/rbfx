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
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "ShaderMacroHelper.hpp"
#include "MapHelper.hpp"
#include "Align.hpp"
#include "BasicMath.hpp"

#if METAL_SUPPORTED
namespace Diligent
{
namespace Testing
{

extern void CreateSparseTextureMtl(IRenderDevice*     pDevice,
                                   const TextureDesc& TexDesc,
                                   IDeviceMemory*     pMemory,
                                   ITexture**         ppTexture);

} // namespace Testing
} // namespace Diligent
#endif

#include "InlineShaders/SparseResourcesTest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class SparseResourceTest : public testing::TestWithParam<int>
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        if (!pDevice->GetDeviceInfo().Features.SparseResources)
            return;

        // Find context.
        constexpr auto QueueTypeMask = COMMAND_QUEUE_TYPE_SPARSE_BINDING;
        for (Uint32 CtxInd = 0; CtxInd < pEnv->GetNumImmediateContexts(); ++CtxInd)
        {
            auto*       Ctx  = pEnv->GetDeviceContext(CtxInd);
            const auto& Desc = Ctx->GetDesc();

            if ((Desc.QueueType & QueueTypeMask) == QueueTypeMask)
            {
                sm_pSparseBindingCtx = Ctx;
                break;
            }
        }

        if (!sm_pSparseBindingCtx)
            return;

        auto* pContext = pEnv->GetDeviceContext();

        // Fill buffer PSO
        {
            BufferDesc BuffDesc;
            BuffDesc.Name           = "Fill buffer parameters";
            BuffDesc.Size           = sizeof(Uint32) * 4;
            BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage          = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &sm_pFillBufferParams);
            ASSERT_NE(sm_pFillBufferParams, nullptr);

            StateTransitionDesc Barrier;
            Barrier.pResource = sm_pFillBufferParams;
            Barrier.OldState  = RESOURCE_STATE_UNKNOWN;
            Barrier.NewState  = RESOURCE_STATE_CONSTANT_BUFFER;
            Barrier.Flags     = STATE_TRANSITION_FLAG_UPDATE_STATE;

            pContext->TransitionResourceStates(1, &Barrier);

            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.UseCombinedTextureSamplers = true;
            ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
            ShaderCI.EntryPoint                 = "main";
            ShaderCI.Desc.Name                  = "Fill buffer CS";
            ShaderCI.Source                     = HLSL::FillBuffer_CS.c_str();
            RefCntAutoPtr<IShader> pCS;
            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);

            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = "Fill buffer PSO";
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pCS;

            const ShaderResourceVariableDesc Variables[] =
                {
                    {SHADER_TYPE_COMPUTE, "CB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
                    {SHADER_TYPE_COMPUTE, "g_DstBuffer", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS} //
                };
            PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Variables;
            PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Variables);

            pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pFillBufferPSO);
            ASSERT_NE(sm_pFillBufferPSO, nullptr);

            sm_pFillBufferPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "CB")->Set(sm_pFillBufferParams);

            sm_pFillBufferPSO->CreateShaderResourceBinding(&sm_pFillBufferSRB, true);
            ASSERT_NE(sm_pFillBufferSRB, nullptr);
        }

        // Fullscreen quad to fill 2D texture
        {
            BufferDesc BuffDesc;
            BuffDesc.Name           = "Fill texture 2D parameters";
            BuffDesc.Size           = sizeof(float4);
            BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage          = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &sm_pFillTexture2DParams);
            ASSERT_NE(sm_pFillTexture2DParams, nullptr);

            GraphicsPipelineStateCreateInfo PSOCreateInfo;

            auto& PSODesc          = PSOCreateInfo.PSODesc;
            auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

            PSODesc.Name = "Fill texture 2D";

            PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

            GraphicsPipeline.NumRenderTargets                     = 1;
            GraphicsPipeline.RTVFormats[0]                        = sm_TexFormat;
            GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
            GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
            GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;
            GraphicsPipeline.RasterizerDesc.ScissorEnable         = True;
            GraphicsPipeline.DepthStencilDesc.DepthEnable         = False;

            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

            RefCntAutoPtr<IShader> pVS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Fill texture 2D PS";
                ShaderCI.Source          = HLSL::SparseResTest_VS.c_str();

                pDevice->CreateShader(ShaderCI, &pVS);
                ASSERT_NE(pVS, nullptr);
            }

            RefCntAutoPtr<IShader> pPS;
            {
                ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
                ShaderCI.EntryPoint      = "main";
                ShaderCI.Desc.Name       = "Fill texture 2D PS";
                ShaderCI.Source          = HLSL::FillTexture2D_PS.c_str();

                pDevice->CreateShader(ShaderCI, &pPS);
                ASSERT_NE(pPS, nullptr);
            }

            PSOCreateInfo.pVS = pVS;
            PSOCreateInfo.pPS = pPS;

            pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pFillTexture2DPSO);
            ASSERT_NE(sm_pFillTexture2DPSO, nullptr);

            sm_pFillTexture2DPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "CB")->Set(sm_pFillTexture2DParams);

            sm_pFillTexture2DPSO->CreateShaderResourceBinding(&sm_pFillTexture2DSRB, true);
            ASSERT_NE(sm_pFillTexture2DSRB, nullptr);
        }

        // Fill texture 3D PSO
        {
            BufferDesc BuffDesc;
            BuffDesc.Name           = "Fill texture 3D parameters";
            BuffDesc.Size           = sizeof(Uint32) * 4 * 3;
            BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage          = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &sm_pFillTexture3DParams);
            ASSERT_NE(sm_pFillTexture3DParams, nullptr);

            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.UseCombinedTextureSamplers = true;
            ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
            ShaderCI.EntryPoint                 = "main";
            ShaderCI.Desc.Name                  = "Fill texture 3D CS";
            ShaderCI.Source                     = HLSL::FillTexture3D_CS.c_str();
            RefCntAutoPtr<IShader> pCS;
            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);

            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = "Fill texture 3D PSO";
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pCS;

            const ShaderResourceVariableDesc Variables[] =
                {
                    {SHADER_TYPE_COMPUTE, "CB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
                    {SHADER_TYPE_COMPUTE, "g_DstTexture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
                };
            PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Variables;
            PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Variables);

            pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pFillTexture3DPSO);
            ASSERT_NE(sm_pFillTexture3DPSO, nullptr);

            sm_pFillTexture3DPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "CB")->Set(sm_pFillTexture3DParams);

            sm_pFillTexture3DPSO->CreateShaderResourceBinding(&sm_pFillTexture3DSRB, true);
            ASSERT_NE(sm_pFillTexture3DSRB, nullptr);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pSparseBindingCtx.Release();

        sm_pFillBufferPSO.Release();
        sm_pFillBufferSRB.Release();
        sm_pFillBufferParams.Release();

        sm_pFillTexture2DPSO.Release();
        sm_pFillTexture2DSRB.Release();
        sm_pFillTexture2DParams.Release();

        sm_pFillTexture3DPSO.Release();
        sm_pFillTexture3DSRB.Release();
        sm_pFillTexture3DParams.Release();

        sm_pTempSRB.Release();
    }

    static RefCntAutoPtr<IBuffer> CreateSparseBuffer(Uint64 Size, BIND_FLAGS BindFlags, bool Aliasing = false, const Uint32 Stride = 4u)
    {
        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        BufferDesc Desc;
        Desc.Name              = "Sparse buffer";
        Desc.Size              = AlignUp(Size, Stride);
        Desc.BindFlags         = BindFlags | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS; // UAV for fill buffer, SRV to read in PS
        Desc.Usage             = USAGE_SPARSE;
        Desc.MiscFlags         = (Aliasing ? MISC_BUFFER_FLAG_SPARSE_ALIASING : MISC_BUFFER_FLAG_NONE);
        Desc.Mode              = BUFFER_MODE_STRUCTURED;
        Desc.ElementByteStride = Stride;

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(Desc, nullptr, &pBuffer);
        return pBuffer;
    }

    static RefCntAutoPtr<IBuffer> CreateBuffer(Uint64 Size, BIND_FLAGS BindFlags, const Uint32 Stride = 4u)
    {
        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        BufferDesc Desc;
        Desc.Name              = "Reference buffer";
        Desc.Size              = AlignUp(Size, Stride);
        Desc.BindFlags         = BindFlags | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS; // UAV for fill buffer, SRV to read in PS
        Desc.Usage             = USAGE_DEFAULT;
        Desc.Mode              = BUFFER_MODE_STRUCTURED;
        Desc.ElementByteStride = Stride;

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(Desc, nullptr, &pBuffer);
        return pBuffer;
    }

    static RefCntAutoPtr<IDeviceMemory> CreateMemory(Uint32 PageSize, Uint32 NumPages, IDeviceObject* pCompatibleResource)
    {
        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        DeviceMemoryCreateInfo MemCI;
        MemCI.Desc.Name             = "Memory for sparse resources";
        MemCI.Desc.Type             = DEVICE_MEMORY_TYPE_SPARSE;
        MemCI.Desc.PageSize         = PageSize;
        MemCI.InitialSize           = NumPages * PageSize;
        MemCI.ppCompatibleResources = &pCompatibleResource;
        MemCI.NumResources          = pCompatibleResource == nullptr ? 0 : 1;

        RefCntAutoPtr<IDeviceMemory> pMemory;
        pDevice->CreateDeviceMemory(MemCI, &pMemory);
        if (pMemory == nullptr)
            return {};

        // Even if resize is not supported function must return 'true'
        if (!pMemory->Resize(MemCI.InitialSize))
            return {};

        VERIFY_EXPR(pMemory->GetCapacity() == NumPages * PageSize);

        return pMemory;
    }

    struct TextureAndMemory
    {
        RefCntAutoPtr<ITexture>      pTexture;
        RefCntAutoPtr<IDeviceMemory> pMemory;
    };
    static TextureAndMemory CreateSparseTextureAndMemory(const uint4& Dim, BIND_FLAGS BindFlags, Uint32 NumMemoryPages, bool Aliasing = false)
    {
        auto*      pDevice   = GPUTestingEnvironment::GetInstance()->GetDevice();
        const auto BlockSize = pDevice->GetAdapterInfo().SparseResources.StandardBlockSize;

        TextureDesc Desc;
        Desc.BindFlags = BindFlags;
        if (Dim.z > 1)
        {
            VERIFY_EXPR(Dim.w <= 1);
            Desc.Type  = RESOURCE_DIM_TEX_3D;
            Desc.Depth = static_cast<Uint32>(Dim.z);
        }
        else
        {
            VERIFY_EXPR(Dim.z <= 1);
            Desc.Type      = Dim.w > 1 ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            Desc.ArraySize = static_cast<Uint32>(Dim.w);
        }

        Desc.Width       = static_cast<Uint32>(Dim.x);
        Desc.Height      = static_cast<Uint32>(Dim.y);
        Desc.Format      = sm_TexFormat;
        Desc.MipLevels   = 0; // full mip chain
        Desc.SampleCount = 1;
        Desc.Usage       = USAGE_SPARSE;
        Desc.MiscFlags   = (Aliasing ? MISC_TEXTURE_FLAG_SPARSE_ALIASING : MISC_TEXTURE_FLAG_NONE);

        TextureAndMemory Result;
#if METAL_SUPPORTED
        if (pDevice->GetDeviceInfo().IsMetalDevice())
        {
            Result.pMemory = CreateMemory(AlignUp(64u << 10, BlockSize), NumMemoryPages, nullptr);
            if (Result.pMemory == nullptr)
                return {};

            CreateSparseTextureMtl(pDevice, Desc, Result.pMemory, &Result.pTexture);
        }
        else
#endif
        {
            pDevice->CreateTexture(Desc, nullptr, &Result.pTexture);
            if (Result.pTexture == nullptr)
                return {};

            Result.pMemory = CreateMemory(BlockSize, NumMemoryPages, Result.pTexture);
        }
        return Result;
    }

    static RefCntAutoPtr<ITexture> CreateTexture(const uint4& Dim, BIND_FLAGS BindFlags)
    {
        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        TextureDesc Desc;
        Desc.BindFlags = BindFlags | BIND_SHADER_RESOURCE; // SRV to read in PS
        if (Dim.z > 1)
        {
            VERIFY_EXPR(Dim.w <= 1);
            Desc.Type  = RESOURCE_DIM_TEX_3D;
            Desc.Depth = static_cast<Uint32>(Dim.z);
        }
        else
        {
            VERIFY_EXPR(Dim.z <= 1);
            Desc.Type      = Dim.w > 1 ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            Desc.ArraySize = static_cast<Uint32>(Dim.w);
        }

        Desc.Width       = static_cast<Uint32>(Dim.x);
        Desc.Height      = static_cast<Uint32>(Dim.y);
        Desc.Format      = sm_TexFormat;
        Desc.MipLevels   = 0; // full mip chain
        Desc.SampleCount = 1;
        Desc.Usage       = USAGE_DEFAULT;

        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture(Desc, nullptr, &pTexture);
        return pTexture;
    }

    static RefCntAutoPtr<IFence> CreateFence()
    {
        auto* pDevice = GPUTestingEnvironment::GetInstance()->GetDevice();

        if (pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_D3D11)
            return {};

        FenceDesc Desc;
        Desc.Name = "Fence";
        Desc.Type = FENCE_TYPE_GENERAL;

        RefCntAutoPtr<IFence> pFence;
        pDevice->CreateFence(Desc, &pFence);

        return pFence;
    }

    static void FillBuffer(IDeviceContext* pContext, IBuffer* pBuffer, Uint64 Offset, Uint32 Size, Uint32 Pattern)
    {
        auto* pView = pBuffer->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
        VERIFY_EXPR(pView != nullptr);

        sm_pFillBufferSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstBuffer")->Set(pView);

        const auto Stride = pBuffer->GetDesc().ElementByteStride;

        struct CB
        {
            Uint32 Offset;
            Uint32 Size;
            Uint32 Pattern;
            Uint32 padding;
        };
        {
            MapHelper<CB> CBConstants(pContext, sm_pFillBufferParams, MAP_WRITE, MAP_FLAG_DISCARD);
            CBConstants->Offset  = StaticCast<Uint32>(Offset / Stride);
            CBConstants->Size    = Size / Stride;
            CBConstants->Pattern = Pattern;
        }

        pContext->SetPipelineState(sm_pFillBufferPSO);
        pContext->CommitShaderResources(sm_pFillBufferSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        DispatchComputeAttribs CompAttrs;
        CompAttrs.ThreadGroupCountX = (Size / Stride + 63) / 64;
        CompAttrs.ThreadGroupCountY = 1;
        CompAttrs.ThreadGroupCountZ = 1;
        pContext->DispatchCompute(CompAttrs);
    }

    static void FillTextureMip(IDeviceContext* pContext, ITexture* pTexture, Uint32 MipLevel, Uint32 Slice, const float4& Color)
    {
        const auto& Desc = pTexture->GetDesc();
        const Rect  Region{0, 0, static_cast<int>(std::max(1u, Desc.Width >> MipLevel)), static_cast<int>(std::max(1u, Desc.Height >> MipLevel))};

        FillTexture(pContext, pTexture, Region, MipLevel, Slice, Color);
    }

    static void FillTexture(IDeviceContext* pContext, ITexture* pTexture, const Rect& Region, Uint32 MipLevel, Uint32 Slice, const float4& Color)
    {
        VERIFY_EXPR(pTexture->GetDesc().Is2D());

        TextureViewDesc Desc;
        Desc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
        Desc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
        Desc.MostDetailedMip = MipLevel;
        Desc.NumMipLevels    = 1;
        Desc.FirstArraySlice = Slice;
        Desc.NumArraySlices  = 1;

        RefCntAutoPtr<ITextureView> pView;
        pTexture->CreateView(Desc, &pView);
        VERIFY_EXPR(pView != nullptr);

        ITextureView* pRTV = pView;

        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetScissorRects(1, &Region, 0, 0);

        pContext->SetPipelineState(sm_pFillTexture2DPSO);
        pContext->CommitShaderResources(sm_pFillTexture2DSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        {
            MapHelper<float4> CBConstants(pContext, sm_pFillTexture2DParams, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBConstants = Color;
        }

        DrawAttribs DrawAttrs{3, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);

        pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);
    }

    static void ClearTexture(IDeviceContext* pContext, ITexture* pTexture)
    {
        // sparse render target must be cleared

        VERIFY_EXPR(pTexture->GetDesc().Is2D());

        const auto& TexDesc = pTexture->GetDesc();
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            for (Uint32 Mip = 0; Mip < TexDesc.MipLevels; ++Mip)
            {
                TextureViewDesc Desc;
                Desc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
                Desc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                Desc.MostDetailedMip = Mip;
                Desc.NumMipLevels    = 1;
                Desc.FirstArraySlice = Slice;
                Desc.NumArraySlices  = 1;

                RefCntAutoPtr<ITextureView> pView;
                pTexture->CreateView(Desc, &pView);
                VERIFY_EXPR(pView != nullptr);

                ITextureView* pRTV = pView;

                pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                const float ClearColor[4] = {};
                pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);

                pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);
            }
        }
    }

    static void FillTexture3DMip(IDeviceContext* pContext, ITexture* pTexture, Uint32 MipLevel, const float4& Color)
    {
        const auto& Desc = pTexture->GetDesc();
        const Box   Region{
            0u, std::max(1u, Desc.Width >> MipLevel),
            0u, std::max(1u, Desc.Height >> MipLevel),
            0u, std::max(1u, Desc.Depth >> MipLevel)};

        FillTexture3D(pContext, pTexture, Region, MipLevel, Color);
    }

    static void FillTexture3D(IDeviceContext* pContext, ITexture* pTexture, const Box& Region, Uint32 MipLevel, const float4& Color)
    {
        VERIFY_EXPR(pTexture->GetDesc().Is3D());

        TextureViewDesc Desc;
        Desc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
        Desc.TextureDim      = RESOURCE_DIM_TEX_3D;
        Desc.MostDetailedMip = MipLevel;
        Desc.NumMipLevels    = 1;
        Desc.FirstDepthSlice = 0;
        Desc.NumDepthSlices  = 0; // all slices

        RefCntAutoPtr<ITextureView> pView;
        pTexture->CreateView(Desc, &pView);
        VERIFY_EXPR(pView);

        sm_pFillTexture3DSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pView);

        struct CB
        {
            uint4  Offset;
            uint4  Size;
            float4 Color;
        };
        {
            MapHelper<CB> CBConstants(pContext, sm_pFillTexture3DParams, MAP_WRITE, MAP_FLAG_DISCARD);
            CBConstants->Offset = {Region.MinX, Region.MinY, Region.MinZ, 0u};
            CBConstants->Size   = {Region.Width(), Region.Height(), Region.Depth(), 0u};
            CBConstants->Color  = Color;
        }

        pContext->SetPipelineState(sm_pFillTexture3DPSO);
        pContext->CommitShaderResources(sm_pFillTexture3DSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DispatchComputeAttribs CompAttrs;
        CompAttrs.ThreadGroupCountX = (Region.Width() + 3) / 4;
        CompAttrs.ThreadGroupCountY = (Region.Height() + 3) / 4;
        CompAttrs.ThreadGroupCountZ = (Region.Depth() + 3) / 4;
        pContext->DispatchCompute(CompAttrs);
    }

    static void DrawFSQuad(IDeviceContext* pContext, IPipelineState* pPSO, IShaderResourceBinding* pSRB)
    {
        auto* pEnv       = GPUTestingEnvironment::GetInstance();
        auto* pSwapChain = pEnv->GetSwapChain();

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[4] = {0.f, 0.f, 0.f, 1.f};
        pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);

        DrawAttribs DrawAttrs{3, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);
    }

    static void DrawFSQuadWithBuffer(IDeviceContext* pContext, IPipelineState* pPSO, IBuffer* pBuffer)
    {
        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        pPSO->CreateShaderResourceBinding(&pSRB);
        if (pSRB == nullptr)
            return;

        auto* pView = pBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
        VERIFY_EXPR(pView != nullptr);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer")->Set(pView);

        DrawFSQuad(pContext, pPSO, pSRB);

        sm_pTempSRB = std::move(pSRB);
    }

    static void DrawFSQuadWithTexture(IDeviceContext* pContext, IPipelineState* pPSO, ITexture* pTexture)
    {
        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        pPSO->CreateShaderResourceBinding(&pSRB);
        if (pSRB == nullptr)
            return;

        auto* pView = pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        VERIFY_EXPR(pView);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pView);

        DrawFSQuad(pContext, pPSO, pSRB);

        sm_pTempSRB = std::move(pSRB);
    }

    static void CreateGraphicsPSOForBuffer(const char* Name, const String& PSSource, Uint32 BufferElementCount, RefCntAutoPtr<IPipelineState>& pPSO)
    {
        const auto Stride = 4u;
        CreateGraphicsPSO(Name, PSSource, /*Is2DArray*/ false, /*IsMSL*/ false, BufferElementCount / Stride, pPSO);
    }

    static void CreateGraphicsPSOForTexture(const char* Name, const String& PSSource, bool Is2DArray, RefCntAutoPtr<IPipelineState>& pPSO)
    {
        CreateGraphicsPSO(Name, PSSource, Is2DArray, /*IsMSL*/ false, 0, pPSO);
    }

    static void CreateGraphicsPSOForTextureWithMSL(const char* Name, const String& PSSource, bool Is2DArray, RefCntAutoPtr<IPipelineState>& pPSO)
    {
        CreateGraphicsPSO(Name, PSSource, Is2DArray, /*IsMSL*/ true, 0, pPSO);
    }

    static void CreateGraphicsPSO(const char* Name, const String& PSSource, bool Is2DArray, bool IsMSL, Uint32 BufferElementCount, RefCntAutoPtr<IPipelineState>& pPSO)
    {
        auto*       pEnv       = GPUTestingEnvironment::GetInstance();
        auto*       pDevice    = pEnv->GetDevice();
        auto*       pSwapChain = pEnv->GetSwapChain();
        const auto& SCDesc     = pSwapChain->GetDesc();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = Name;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        GraphicsPipeline.NumRenderTargets                     = 1;
        GraphicsPipeline.RTVFormats[0]                        = SCDesc.ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
        GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
        GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;
        GraphicsPipeline.DepthStencilDesc.DepthEnable         = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.UseCombinedTextureSamplers = true;

        if (pDevice->GetDeviceInfo().IsVulkanDevice())
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC; // glslang does not support sparse residency status

        ShaderMacroHelper Macros;
        Macros.AddShaderMacro("SCREEN_WIDTH", SCDesc.Width);
        Macros.AddShaderMacro("SCREEN_HEIGHT", SCDesc.Height);
        Macros.AddShaderMacro("TEXTURE_2D_ARRAY", Is2DArray);
        Macros.AddShaderMacro("BUFFER_ELEMENT_COUNT", BufferElementCount); // GetDimensions() can not be used for root view in D3D12
        ShaderCI.Macros = Macros;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Sparse resource test - VS";
            ShaderCI.Source          = HLSL::SparseResTest_VS.c_str();

            pDevice->CreateShader(ShaderCI, &pVS);
            if (pVS == nullptr)
                return;
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.SourceLanguage  = IsMSL ? SHADER_SOURCE_LANGUAGE_MSL : SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "PSmain";
            ShaderCI.Desc.Name       = "Sparse resource test - PS";
            ShaderCI.Source          = PSSource.c_str();
            if (IsMSL)
            {
                // We need to disable reflection as defines in the shader function
                // declaration are not handled by MSL parser.
                ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_SKIP_REFLECTION;
            }

            pDevice->CreateShader(ShaderCI, &pPS);
            if (pPS == nullptr)
                return;
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        if (pPSO == nullptr)
            return;
    }

    // Generates reproducible color sequence
    static void RestartColorRandomizer()
    {
        sm_RndColorIndex = 0.f;
    }
    static float4 RandomColor()
    {
        float h = FastFrac(sm_RndColorIndex) / 1.35f;
        sm_RndColorIndex += 0.27f;
        float3 col = float3{std::abs(h * 6.0f - 3.0f) - 1.0f, 2.0f - std::abs(h * 6.0f - 2.0f), 2.0f - std::abs(h * 6.0f - 4.0f)};
        return float4{clamp(col, float3{}, float3{1.0f}), 1.0f};
    }
    static Uint32 RandomColorU()
    {
        return F4Color_To_RGBA8Unorm(RandomColor());
    }

    static float4 GetNullBoundTileColor()
    {
        return float4{1.0, 0.0, 1.0, 1.0};
    }

    static RefCntAutoPtr<IDeviceContext> sm_pSparseBindingCtx;

    static RefCntAutoPtr<IPipelineState>         sm_pFillBufferPSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pFillBufferSRB;
    static RefCntAutoPtr<IBuffer>                sm_pFillBufferParams;

    static RefCntAutoPtr<IPipelineState>         sm_pFillTexture2DPSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pFillTexture2DSRB;
    static RefCntAutoPtr<IBuffer>                sm_pFillTexture2DParams;

    static RefCntAutoPtr<IPipelineState>         sm_pFillTexture3DPSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pFillTexture3DSRB;
    static RefCntAutoPtr<IBuffer>                sm_pFillTexture3DParams;

    static RefCntAutoPtr<IShaderResourceBinding> sm_pTempSRB;

    static const TEXTURE_FORMAT sm_TexFormat = TEX_FORMAT_RGBA8_UNORM;

    static float sm_RndColorIndex;
};
RefCntAutoPtr<IDeviceContext>         SparseResourceTest::sm_pSparseBindingCtx;
RefCntAutoPtr<IPipelineState>         SparseResourceTest::sm_pFillBufferPSO;
RefCntAutoPtr<IShaderResourceBinding> SparseResourceTest::sm_pFillBufferSRB;
RefCntAutoPtr<IBuffer>                SparseResourceTest::sm_pFillBufferParams;
RefCntAutoPtr<IPipelineState>         SparseResourceTest::sm_pFillTexture2DPSO;
RefCntAutoPtr<IShaderResourceBinding> SparseResourceTest::sm_pFillTexture2DSRB;
RefCntAutoPtr<IBuffer>                SparseResourceTest::sm_pFillTexture2DParams;
RefCntAutoPtr<IPipelineState>         SparseResourceTest::sm_pFillTexture3DPSO;
RefCntAutoPtr<IShaderResourceBinding> SparseResourceTest::sm_pFillTexture3DSRB;
RefCntAutoPtr<IBuffer>                SparseResourceTest::sm_pFillTexture3DParams;
float                                 SparseResourceTest::sm_RndColorIndex;
RefCntAutoPtr<IShaderResourceBinding> SparseResourceTest::sm_pTempSRB;


enum TestMode
{
    BeginRange = 0,
    POT_2D     = BeginRange,
    POT_2DArray,
    NonPOT_2D,
    NonPOT_2DArray,
    EndRange
};

bool TestMode_IsTexArray(Uint32 Mode)
{
    return Mode == POT_2DArray || Mode == NonPOT_2DArray;
}

const auto TestParamRange = testing::Range(int{BeginRange}, int{EndRange});

String TestIdToString(const testing::TestParamInfo<int>& info)
{
    String name;
    switch (info.param)
    {
        // clang-format off
        case POT_2D:         name = "POT_2D";          break;
        case NonPOT_2D:      name = "NonPOT_2D";       break;
        case POT_2DArray:    name += "POT_2DArray";    break;
        case NonPOT_2DArray: name += "NonPOT_2DArray"; break;
        default:             name = std::to_string(info.param); UNEXPECTED("unsupported TestId");
            // clang-format on
    }
    return name;
}

int4 TestIdToTextureDim(Uint32 TestId)
{
    switch (TestId)
    {
        // clang-format off
        case POT_2D:         return int4{256, 256, 1, 1};
        case NonPOT_2D:      return int4{253, 249, 1, 1};
        case POT_2DArray:    return int4{256, 256, 1, 2};
        case NonPOT_2DArray: return int4{248, 254, 1, 2};
        default:             return int4{};
            // clang-format on
    }
}

void CheckSparseTextureProperties(ITexture* pTexture)
{
    const auto& Desc       = pTexture->GetDesc();
    const auto& Props      = pTexture->GetSparseProperties();
    const bool  IsStdBlock = (Props.Flags & SPARSE_TEXTURE_FLAG_NONSTANDARD_BLOCK_SIZE) == 0;
    const auto& SparseRes  = GPUTestingEnvironment::GetInstance()->GetDevice()->GetAdapterInfo().SparseResources;

    EXPECT_GT(Props.AddressSpaceSize, 0u);
    EXPECT_GT(Props.BlockSize, 0u);
    EXPECT_TRUE(Props.AddressSpaceSize % Props.BlockSize == 0);

    if (IsStdBlock)
        EXPECT_EQ(Props.BlockSize, SparseRes.StandardBlockSize);

    EXPECT_LE(Props.FirstMipInTail, Desc.MipLevels);
    EXPECT_LT(Props.MipTailOffset, Props.AddressSpaceSize);
    EXPECT_TRUE(Props.MipTailOffset % Props.BlockSize == 0);

    // Props.MipTailSize can be zero
    EXPECT_TRUE(Props.MipTailSize % Props.BlockSize == 0);

    if (Desc.Type == RESOURCE_DIM_TEX_3D || Desc.ArraySize == 1)
    {
        EXPECT_GE(Props.AddressSpaceSize, Props.MipTailOffset + Props.MipTailSize);
    }
    else if (Props.MipTailStride != 0) // zero in Metal
    {
        EXPECT_EQ(Props.MipTailStride * Desc.ArraySize, Props.AddressSpaceSize);
        EXPECT_GE(Props.MipTailStride, Props.MipTailOffset + Props.MipTailSize);
    }

    if (Desc.Type == RESOURCE_DIM_TEX_3D)
    {
        EXPECT_GT(Props.TileSize[0], 1u);
        EXPECT_GT(Props.TileSize[1], 1u);
        EXPECT_GE(Props.TileSize[2], 1u); // can be 1 on Metal

        if (IsStdBlock)
        {
            EXPECT_TRUE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_STANDARD_3D_TILE_SHAPE) != 0);
            EXPECT_EQ(Props.TileSize[0], 32u);
            EXPECT_EQ(Props.TileSize[1], 32u);
            EXPECT_EQ(Props.TileSize[2], 16u);
        }
    }
    else
    {
        EXPECT_GT(Props.TileSize[0], 1u);
        EXPECT_GT(Props.TileSize[1], 1u);
        EXPECT_EQ(Props.TileSize[2], 1u);

        if (IsStdBlock)
        {
            EXPECT_TRUE((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_STANDARD_2D_TILE_SHAPE) != 0);
            EXPECT_EQ(Props.TileSize[0], 128u);
            EXPECT_EQ(Props.TileSize[1], 128u);
            EXPECT_EQ(Props.TileSize[2], 1u);
        }
    }
}


TEST_F(SparseResourceTest, SparseBuffer)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse buffer is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const auto BlockSize = 64u << 10;
    const auto BuffSize  = BlockSize * 4;

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForBuffer("Sparse buffer test", HLSL::SparseBuffer_PS, BuffSize, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](IBuffer* pBuffer) //
    {
        // To avoid UAV barriers between FillBuffer()
        {
            StateTransitionDesc Barrier;
            Barrier.pResource = pBuffer;
            Barrier.OldState  = RESOURCE_STATE_UNKNOWN;
            Barrier.NewState  = RESOURCE_STATE_UNORDERED_ACCESS;
            Barrier.Flags     = STATE_TRANSITION_FLAG_UPDATE_STATE;

            pContext->TransitionResourceStates(1, &Barrier);
        }
        RestartColorRandomizer();
        FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 1, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, RandomColorU());
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pBuffer = CreateBuffer(BuffSize, BIND_NONE);
        ASSERT_NE(pBuffer, nullptr);

        Fill(pBuffer);
        DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto MemBlockSize = BlockSize;
    auto       pMemory      = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory},
            {BlockSize * 1, MemBlockSize * 2, BlockSize, pMemory},
            {BlockSize * 2, MemBlockSize * 3, BlockSize, pMemory},
            {BlockSize * 3, MemBlockSize * 6, BlockSize, pMemory} //
        };

        SparseBufferMemoryBindInfo SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds = 1;
        BindSparseAttrs.pBufferBinds   = &SparseBuffBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }

        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        Fill(pBuffer);
    }

    DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

    pSwapChain->Present();
}


TEST_F(SparseResourceTest, SparseResidentBuffer)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse buffer is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const auto BlockSize = 64u << 10;
    const auto BuffSize  = BlockSize * 8;

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForBuffer("Sparse residency buffer test", HLSL::SparseBuffer_PS, BuffSize, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](IBuffer* pBuffer) //
    {
        // To avoid UAV barriers between FillBuffer()
        {
            StateTransitionDesc Barrier;
            Barrier.pResource = pBuffer;
            Barrier.OldState  = RESOURCE_STATE_UNKNOWN;
            Barrier.NewState  = RESOURCE_STATE_UNORDERED_ACCESS;
            Barrier.Flags     = STATE_TRANSITION_FLAG_UPDATE_STATE;

            pContext->TransitionResourceStates(1, &Barrier);
        }
        RestartColorRandomizer();
        FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 6, BlockSize, RandomColorU());

        if (pBuffer->GetDesc().Usage != USAGE_SPARSE)
        {
            FillBuffer(pContext, pBuffer, BlockSize * 1, BlockSize, 0);
            FillBuffer(pContext, pBuffer, BlockSize * 4, BlockSize, 0);
            FillBuffer(pContext, pBuffer, BlockSize * 5, BlockSize, 0);
            FillBuffer(pContext, pBuffer, BlockSize * 7, BlockSize, 0);
        }
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pBuffer = CreateBuffer(BuffSize, BIND_NONE);
        ASSERT_NE(pBuffer, nullptr);

        Fill(pBuffer);
        DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto MemBlockSize = BlockSize;
    auto       pMemory      = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            // clang-format off
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory},
            {BlockSize * 1,                0, BlockSize, nullptr}, // same as keep range unbounded
            {BlockSize * 2, MemBlockSize * 2, BlockSize, pMemory},
            {BlockSize * 3, MemBlockSize * 3, BlockSize, pMemory},
            {BlockSize * 6, MemBlockSize * 6, BlockSize, pMemory}
            // clang-format on
        };

        SparseBufferMemoryBindInfo SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds = 1;
        BindSparseAttrs.pBufferBinds   = &SparseBuffBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        Fill(pBuffer);
    }

    DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

    pSwapChain->Present();
}


TEST_F(SparseResourceTest, SparseResidentAliasedBuffer)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse buffer is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_ALIASED) == 0)
    {
        GTEST_SKIP() << "Sparse aliased resources is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const auto BlockSize = 64u << 10;
    const auto BuffSize  = BlockSize * 8;

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForBuffer("Sparse residency aliased buffer test", HLSL::SparseBuffer_PS, BuffSize, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](IBuffer* pBuffer) //
    {
        // To avoid UAV barriers between FillBuffer()
        {
            StateTransitionDesc Barrier;
            Barrier.pResource = pBuffer;
            Barrier.OldState  = RESOURCE_STATE_UNKNOWN;
            Barrier.NewState  = RESOURCE_STATE_UNORDERED_ACCESS;
            Barrier.Flags     = STATE_TRANSITION_FLAG_UPDATE_STATE;

            pContext->TransitionResourceStates(1, &Barrier);
        }
        RestartColorRandomizer();
        const auto Col0 = RandomColorU();
        const auto Col1 = RandomColorU();
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, Col0); // aliased
        FillBuffer(pContext, pBuffer, BlockSize * 1, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, RandomColorU());
        FillBuffer(pContext, pBuffer, BlockSize * 5, BlockSize, RandomColorU());

        if (pBuffer->GetDesc().Usage != USAGE_SPARSE)
        {
            FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, Col1);
            FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, Col1);
            FillBuffer(pContext, pBuffer, BlockSize * 4, BlockSize, 0);
            FillBuffer(pContext, pBuffer, BlockSize * 6, BlockSize, 0);
            FillBuffer(pContext, pBuffer, BlockSize * 7, BlockSize, 0);
        }
        else
        {
            // aliased buffer ranges
            StateTransitionDesc Barrier{pBuffer, pBuffer};
            pContext->TransitionResourceStates(1, &Barrier);

            FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, Col1); // aliased
        }
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pBuffer = CreateBuffer(BuffSize, BIND_NONE);
        ASSERT_NE(pBuffer, nullptr);

        Fill(pBuffer);
        DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE, /*Aliasing*/ true);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto MemBlockSize = BlockSize;
    auto       pMemory      = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory}, // --|
            {BlockSize * 1, MemBlockSize * 2, BlockSize, pMemory}, //   |-- 2 aliased blocks
            {BlockSize * 2, MemBlockSize * 0, BlockSize, pMemory}, // --|
            {BlockSize * 3, MemBlockSize * 1, BlockSize, pMemory},
            {BlockSize * 5, MemBlockSize * 6, BlockSize, pMemory} //
        };

        SparseBufferMemoryBindInfo SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds = 1;
        BindSparseAttrs.pBufferBinds   = &SparseBuffBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        Fill(pBuffer);
    }

    DrawFSQuadWithBuffer(pContext, pPSO, pBuffer);

    pSwapChain->Present();
}


TEST_P(SparseResourceTest, SparseTexture)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT) == 0)
    {
        GTEST_SKIP() << "This device does not support texture RTVs and SRVs in one memory object";
    }
    if (TestMode_IsTexArray(TestId) && (SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D array with mipmap tail is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const auto                    TexSize = TestIdToTextureDim(TestId);
    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForTexture("Sparse texture test", HLSL::SparseTexture_PS, TexSize.w > 1, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](ITexture* pTexture) //
    {
        RestartColorRandomizer();
        const auto& TexDesc = pTexture->GetDesc();
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            // clang-format off
            FillTexture(pContext, pTexture, Rect{  0,   0,       128,       128}, 0, Slice, RandomColor());
            FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,       128}, 0, Slice, RandomColor());
            FillTexture(pContext, pTexture, Rect{  0, 128,       128, TexSize.y}, 0, Slice, RandomColor());
            FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x, TexSize.y}, 0, Slice, RandomColor());
            // clang-format on

            for (Uint32 Mip = 1; Mip < TexDesc.MipLevels; ++Mip)
                FillTextureMip(pContext, pTexture, Mip, Slice, RandomColor());
        }
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pRefTexture = CreateTexture(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET);
        ASSERT_NE(pRefTexture, nullptr);

        Fill(pRefTexture);
        DrawFSQuadWithTexture(pContext, pPSO, pRefTexture);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    const auto BlockSize = SparseRes.StandardBlockSize;

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET | BIND_SHADER_RESOURCE, 14 * TexSize.w);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, pMemory->GetCapacity());

    auto pFence = CreateFence();

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        Uint64 MemOffset = 0;
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            for (Uint32 Mip = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
            {
                const auto Width  = std::max(1u, TexDesc.Width >> Mip);
                const auto Height = std::max(1u, TexDesc.Height >> Mip);
                for (Uint32 y = 0; y < Height; y += TexSparseProps.TileSize[1])
                {
                    for (Uint32 x = 0; x < Width; x += TexSparseProps.TileSize[0])
                    {
                        BindRanges.emplace_back();
                        auto& Range        = BindRanges.back();
                        Range.MipLevel     = Mip;
                        Range.ArraySlice   = Slice;
                        Range.Region.MinX  = x;
                        Range.Region.MaxX  = std::min(Width, x + TexSparseProps.TileSize[0]);
                        Range.Region.MinY  = y;
                        Range.Region.MaxY  = std::min(Height, y + TexSparseProps.TileSize[1]);
                        Range.Region.MinZ  = 0;
                        Range.Region.MaxZ  = 1;
                        Range.MemoryOffset = MemOffset;
                        Range.MemorySize   = BlockSize;
                        Range.pMemory      = pMemory;
                        MemOffset += Range.MemorySize;
                    }
                }
            }

            // Mip tail
            if (Slice == 0 || (TexSparseProps.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL) == 0)
            {
                const bool IsMetal = pDevice->GetDeviceInfo().IsMetalDevice();
                for (Uint64 OffsetInMipTail = 0; OffsetInMipTail < TexSparseProps.MipTailSize;)
                {
                    BindRanges.emplace_back();
                    auto& Range           = BindRanges.back();
                    Range.MipLevel        = TexSparseProps.FirstMipInTail;
                    Range.ArraySlice      = Slice;
                    Range.OffsetInMipTail = OffsetInMipTail;
                    Range.MemoryOffset    = MemOffset;
                    Range.MemorySize      = IsMetal ? TexSparseProps.MipTailSize : BlockSize;
                    Range.pMemory         = pMemory;
                    MemOffset += Range.MemorySize;
                    OffsetInMipTail += Range.MemorySize;
                }
            }
        }
        VERIFY_EXPR(MemOffset <= pMemory->GetCapacity());

        SparseTextureMemoryBindInfo SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds = 1;
        BindSparseAttrs.pTextureBinds   = &SparseTexBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        ClearTexture(pContext, pTexture);
        Fill(pTexture);
    }

    DrawFSQuadWithTexture(pContext, pPSO, pTexture);

    pSwapChain->Present();
}


TEST_P(SparseResourceTest, SparseResidencyTexture)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT) == 0)
    {
        GTEST_SKIP() << "This device does not support texture RTVs and SRVs in one memory object";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_SHADER_RESOURCE_RESIDENCY) == 0)
    {
        GTEST_SKIP() << "Shader resource residency is not supported by this device";
    }
    if (TestMode_IsTexArray(TestId) && (SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D array with mipmap tail is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const bool                    IsMetal = pDevice->GetDeviceInfo().IsMetalDevice();
    const auto                    TexSize = TestIdToTextureDim(TestId);
    RefCntAutoPtr<IPipelineState> pPSO;
    if (IsMetal)
        CreateGraphicsPSOForTextureWithMSL("Sparse resident texture test", MSL::SparseTextureResidency_PS, TexSize.w > 1, pPSO);
    else
        CreateGraphicsPSOForTexture("Sparse resident texture test", HLSL::SparseTextureResidency_PS, TexSize.w > 1, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](ITexture* pTexture) //
    {
        RestartColorRandomizer();
        const auto& TexDesc = pTexture->GetDesc();
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            // clang-format off
            FillTexture(pContext, pTexture, Rect{  0,   0,       128,       128}, 0, Slice, RandomColor());
            FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,       128}, 0, Slice, RandomColor()); // -|-- null bound
            FillTexture(pContext, pTexture, Rect{  0, 128,       128, TexSize.y}, 0, Slice, RandomColor()); // -|
            FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x, TexSize.y}, 0, Slice, RandomColor());
            // clang-format on

            for (Uint32 Mip = 1; Mip < TexDesc.MipLevels; ++Mip)
                FillTextureMip(pContext, pTexture, Mip, Slice, RandomColor());

            if (TexDesc.Usage != USAGE_SPARSE)
            {
                // clang-format off
                FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,       128}, 0, Slice, GetNullBoundTileColor());
                FillTexture(pContext, pTexture, Rect{  0, 128,       128, TexSize.y}, 0, Slice, GetNullBoundTileColor());
                // clang-format on
            }
        }
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pRefTexture = CreateTexture(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET);
        ASSERT_NE(pRefTexture, nullptr);

        Fill(pRefTexture);
        DrawFSQuadWithTexture(pContext, pPSO, pRefTexture);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    const auto BlockSize = SparseRes.StandardBlockSize;

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET | BIND_SHADER_RESOURCE, 12 * TexSize.w);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, pMemory->GetCapacity());

    // in Direct3D & Vulkan tile size is always 128x128, but in Metal tile size is implementation defined
    const uint2 TileSize = {128, 128};
    ASSERT_TRUE(TileSize.x % TexSparseProps.TileSize[0] == 0);
    ASSERT_TRUE(TileSize.y % TexSparseProps.TileSize[1] == 0);

    auto pFence = CreateFence();

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        Uint64 MemOffset = 0;
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            for (Uint32 Mip = 0, Idx = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
            {
                const Uint32 Width  = std::max(1u, TexDesc.Width >> Mip);
                const Uint32 Height = std::max(1u, TexDesc.Height >> Mip);
                for (Uint32 TileY = 0; TileY < Height; TileY += TileSize.y)
                {
                    for (Uint32 TileX = 0; TileX < Width; TileX += TileSize.x)
                    {
                        BindRanges.emplace_back();
                        auto& Range       = BindRanges.back();
                        Range.Region.MinX = TileX;
                        Range.Region.MaxX = std::min(Width, TileX + TileSize.x);
                        Range.Region.MinY = TileY;
                        Range.Region.MaxY = std::min(Height, TileY + TileSize.y);
                        Range.Region.MinZ = 0;
                        Range.Region.MaxZ = 1;
                        Range.MipLevel    = Mip;
                        Range.ArraySlice  = Slice;

                        if ((++Idx & 2) == 0 || Mip > 0)
                        {
                            Range.MemorySize   = BlockSize;
                            Range.MemoryOffset = MemOffset;
                            Range.pMemory      = pMemory;
                            MemOffset += Range.MemorySize;
                        }
                    }
                }
            }

            // Mip tail
            if (Slice == 0 || (TexSparseProps.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL) == 0)
            {
                for (Uint64 OffsetInMipTail = 0; OffsetInMipTail < TexSparseProps.MipTailSize;)
                {
                    BindRanges.emplace_back();
                    auto& Range           = BindRanges.back();
                    Range.MipLevel        = TexSparseProps.FirstMipInTail;
                    Range.ArraySlice      = Slice;
                    Range.OffsetInMipTail = OffsetInMipTail;
                    Range.MemoryOffset    = MemOffset;
                    Range.MemorySize      = IsMetal ? TexSparseProps.MipTailSize : BlockSize;
                    Range.pMemory         = pMemory;
                    MemOffset += Range.MemorySize;
                    OffsetInMipTail += Range.MemorySize;
                }
            }
        }
        VERIFY_EXPR(MemOffset <= pMemory->GetCapacity());

        SparseTextureMemoryBindInfo SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds = 1;
        BindSparseAttrs.pTextureBinds   = &SparseTexBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        ClearTexture(pContext, pTexture);
        Fill(pTexture);
    }

    DrawFSQuadWithTexture(pContext, pPSO, pTexture);

    pSwapChain->Present();
}


TEST_P(SparseResourceTest, SparseResidencyAliasedTexture)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT) == 0)
    {
        GTEST_SKIP() << "This device does not support texture RTVs and SRVs in one memory object";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_ALIASED) == 0)
    {
        GTEST_SKIP() << "Sparse aliased resources is not supported by this device";
    }
    if (TestMode_IsTexArray(TestId) && (SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D array with mipmap tail is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    const auto                    TexSize = TestIdToTextureDim(TestId);
    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForTexture("Sparse resident aliased texture test", HLSL::SparseTexture_PS, TexSize.w > 1, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto Fill = [&](ITexture* pTexture) //
    {
        RestartColorRandomizer();
        const auto& TexDesc = pTexture->GetDesc();
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            const auto Col0 = RandomColor();
            const auto Col1 = RandomColor();

            // clang-format off
          //FillTexture(pContext, pTexture, Rect{  0,   0,       128,       128}, 0, Slice, Col0);          // --|-- aliased
          //FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,       128}, 0, Slice, Col1);          // --|
            FillTexture(pContext, pTexture, Rect{  0, 128,       128, TexSize.y}, 0, Slice, RandomColor()); // -|
            FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x, TexSize.y}, 0, Slice, RandomColor()); // -|-- will be overwritten
            // clang-format on

            if (TexDesc.Usage != USAGE_SPARSE)
            {
                // clang-format off
                FillTexture(pContext, pTexture, Rect{  0,   0,       128,       128}, 0, Slice, Col0);
                FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,       128}, 0, Slice, Col1);
                FillTexture(pContext, pTexture, Rect{  0, 128,       128, TexSize.y}, 0, Slice, Col0);
                FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x, TexSize.y}, 0, Slice, Col1);
                // clang-format on
            }
            else
            {
                StateTransitionDesc Barrier{pTexture, pTexture};
                pContext->TransitionResourceStates(1, &Barrier);

                // clang-format off
                FillTexture(pContext, pTexture, Rect{  0, 0,       128, 128}, 0, Slice, Col0);
                FillTexture(pContext, pTexture, Rect{128, 0, TexSize.x, 128}, 0, Slice, Col1);
                // clang-format on
            }

            for (Uint32 Mip = 1; Mip < TexDesc.MipLevels; ++Mip)
                FillTextureMip(pContext, pTexture, Mip, Slice, RandomColor());
        }
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pRefTexture = CreateTexture(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET);
        ASSERT_NE(pRefTexture, nullptr);

        Fill(pRefTexture);
        DrawFSQuadWithTexture(pContext, pPSO, pRefTexture);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    const auto BlockSize = SparseRes.StandardBlockSize;

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize.Recast<Uint32>(), BIND_RENDER_TARGET | BIND_SHADER_RESOURCE, 12 * TexSize.w, /*Aliasing*/ true);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, pMemory->GetCapacity());

    auto pFence = CreateFence();

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        // Mip tail - must not alias with other tiles
        Uint64       InitialOffset = 0;
        const Uint32 MipTailSlices = (TexSparseProps.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL) != 0 ? 1 : TexDesc.ArraySize;
        const bool   IsMetal       = pDevice->GetDeviceInfo().IsMetalDevice();
        for (Uint32 Slice = 0; Slice < MipTailSlices; ++Slice)
        {
            for (Uint64 OffsetInMipTail = 0; OffsetInMipTail < TexSparseProps.MipTailSize;)
            {
                BindRanges.emplace_back();
                auto& Range           = BindRanges.back();
                Range.MipLevel        = TexSparseProps.FirstMipInTail;
                Range.ArraySlice      = Slice;
                Range.OffsetInMipTail = OffsetInMipTail;
                Range.MemoryOffset    = InitialOffset;
                Range.MemorySize      = IsMetal ? TexSparseProps.MipTailSize : BlockSize;
                Range.pMemory         = pMemory;
                InitialOffset += Range.MemorySize;
                OffsetInMipTail += Range.MemorySize;
            }
        }

        // tiles may alias
        for (Uint32 Slice = 0; Slice < TexDesc.ArraySize; ++Slice)
        {
            Uint64 MemOffset = InitialOffset;
            for (Uint32 Mip = 0, Idx = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
            {
                const Uint32 Width  = std::max(1u, TexDesc.Width >> Mip);
                const Uint32 Height = std::max(1u, TexDesc.Height >> Mip);
                for (Uint32 y = 0; y < Height; y += TexSparseProps.TileSize[1])
                {
                    for (Uint32 x = 0; x < Width; x += TexSparseProps.TileSize[0])
                    {
                        if (++Idx > 2 && Mip == 0)
                        {
                            Idx       = 0;
                            MemOffset = InitialOffset;
                        }

                        BindRanges.emplace_back();
                        auto& Range        = BindRanges.back();
                        Range.Region.MinX  = x;
                        Range.Region.MaxX  = x + TexSparseProps.TileSize[0];
                        Range.Region.MinY  = y;
                        Range.Region.MaxY  = y + TexSparseProps.TileSize[1];
                        Range.Region.MinZ  = 0;
                        Range.Region.MaxZ  = 1;
                        Range.MipLevel     = Mip;
                        Range.ArraySlice   = Slice;
                        Range.MemoryOffset = MemOffset;
                        Range.MemorySize   = BlockSize;
                        Range.pMemory      = pMemory;

                        MemOffset += Range.MemorySize;
                        VERIFY_EXPR(MemOffset <= pMemory->GetCapacity());
                    }
                }
            }
            InitialOffset = MemOffset;
        }

        SparseTextureMemoryBindInfo SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds = 1;
        BindSparseAttrs.pTextureBinds   = &SparseTexBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        ClearTexture(pContext, pTexture);
        Fill(pTexture);
    }

    DrawFSQuadWithTexture(pContext, pPSO, pTexture);

    pSwapChain->Present();
}

INSTANTIATE_TEST_SUITE_P(Sparse, SparseResourceTest, TestParamRange, TestIdToString);


TEST_F(SparseResourceTest, SparseTexture3D)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 3D is not supported by this device";
    }
    if ((pDevice->GetSparseTextureFormatInfo(TEX_FORMAT_RGBA8_UNORM, RESOURCE_DIM_TEX_3D, 1).BindFlags & BIND_UNORDERED_ACCESS) == 0)
    {
        GTEST_SKIP() << "Sparse texture UAV is not supported by this device";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateGraphicsPSOForTexture("Sparse texture 3d test", HLSL::SparseTexture3D_PS, false, pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto TexSize = uint4{64, 64, 15, 1};

    const auto Fill = [&](ITexture* pTexture) //
    {
        RestartColorRandomizer();
        // clang-format off
        FillTexture3D(pContext, pTexture, Box{ 0u,       32u,   0u,       32u,  0u, TexSize.z}, 0, RandomColor());
        FillTexture3D(pContext, pTexture, Box{32u, TexSize.x,   0u,       32u,  0u, TexSize.z}, 0, RandomColor());
        FillTexture3D(pContext, pTexture, Box{ 0u,       32u,  32u, TexSize.y,  0u, TexSize.z}, 0, RandomColor());
        FillTexture3D(pContext, pTexture, Box{32u, TexSize.x,  32u, TexSize.y,  0u, TexSize.z}, 0, RandomColor());
        // clang-format on

        for (Uint32 Mip = 1, MipLevels = pTexture->GetDesc().MipLevels; Mip < MipLevels; ++Mip)
            FillTexture3DMip(pContext, pTexture, Mip, RandomColor());
    };

    // Draw reference
    {
        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

        auto pRefTexture = CreateTexture(TexSize, BIND_UNORDERED_ACCESS);
        ASSERT_NE(pRefTexture, nullptr);

        Fill(pRefTexture);
        DrawFSQuadWithTexture(pContext, pPSO, pRefTexture);

        auto pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        // Transition to CopySrc state to use in TakeSnapshot()
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    const auto BlockSize = SparseRes.StandardBlockSize;

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE, 16);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, pMemory->GetCapacity());

    auto pFence = CreateFence();

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        Uint64 MemOffset = 0;
        for (Uint32 Mip = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
        {
            const auto Width  = std::max(1u, TexDesc.Width >> Mip);
            const auto Height = std::max(1u, TexDesc.Height >> Mip);
            const auto Depth  = std::max(1u, TexDesc.Depth >> Mip);
            for (Uint32 z = 0; z < Depth; z += TexSparseProps.TileSize[2])
            {
                for (Uint32 y = 0; y < Height; y += TexSparseProps.TileSize[1])
                {
                    for (Uint32 x = 0; x < Width; x += TexSparseProps.TileSize[0])
                    {
                        BindRanges.emplace_back();
                        auto& Range        = BindRanges.back();
                        Range.MipLevel     = Mip;
                        Range.ArraySlice   = 0;
                        Range.Region.MinX  = x;
                        Range.Region.MaxX  = x + TexSparseProps.TileSize[0];
                        Range.Region.MinY  = y;
                        Range.Region.MaxY  = y + TexSparseProps.TileSize[1];
                        Range.Region.MinZ  = z;
                        Range.Region.MaxZ  = z + TexSparseProps.TileSize[2];
                        Range.MemoryOffset = MemOffset;
                        Range.MemorySize   = BlockSize;
                        Range.pMemory      = pMemory;
                        MemOffset += Range.MemorySize;
                    }
                }
            }
        }

        // Mip tail
        const bool IsMetal = pDevice->GetDeviceInfo().IsMetalDevice();
        for (Uint64 OffsetInMipTail = 0; OffsetInMipTail < TexSparseProps.MipTailSize;)
        {
            BindRanges.emplace_back();
            auto& Range           = BindRanges.back();
            Range.MipLevel        = TexSparseProps.FirstMipInTail;
            Range.ArraySlice      = 0;
            Range.OffsetInMipTail = OffsetInMipTail;
            Range.MemoryOffset    = MemOffset;
            Range.MemorySize      = IsMetal ? TexSparseProps.MipTailSize : BlockSize;
            Range.pMemory         = pMemory;
            MemOffset += Range.MemorySize;
            OffsetInMipTail += Range.MemorySize;
        }

        VERIFY_EXPR(MemOffset <= pMemory->GetCapacity());

        SparseTextureMemoryBindInfo SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        BindSparseResourceMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds = 1;
        BindSparseAttrs.pTextureBinds   = &SparseTexBind;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        if (SignalFence)
        {
            BindSparseAttrs.ppSignalFences     = &SignalFence;
            BindSparseAttrs.pSignalFenceValues = &SignalValue;
            BindSparseAttrs.NumSignalFences    = 1;
        }
        sm_pSparseBindingCtx->BindSparseResourceMemory(BindSparseAttrs);

        if (SignalFence)
            pContext->DeviceWaitForFence(SignalFence, SignalValue);

        Fill(pTexture);
    }

    DrawFSQuadWithTexture(pContext, pPSO, pTexture);

    pSwapChain->Present();
}

constexpr auto MaxResourceSpaceSize = Uint64{1} << 40;

TEST_F(SparseResourceTest, LargeBuffer)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse buffer is not supported by this device";
    }

    // Limits which is queried from API is not valid, x/4 works on all tested devices.
    Uint64 BuffSize = AlignUp(std::min(MaxResourceSpaceSize, SparseRes.ResourceSpaceSize) >> 2, 4u);
    if (pDevice->GetDeviceInfo().IsD3DDevice())
        BuffSize = std::min(BuffSize, Uint64{1} << 31);

    BufferDesc Desc;
    Desc.Name      = "Sparse buffer";
    Desc.Size      = BuffSize;
    Desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    Desc.Usage     = USAGE_SPARSE;
    Desc.Mode      = BUFFER_MODE_RAW;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(Desc, nullptr, &pBuffer);

    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    LOG_INFO_MESSAGE("Created sparse buffer with size ", pBuffer->GetDesc().Size >> 20, " Mb");
}


TEST_F(SparseResourceTest, LargeTexture2D)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;
    const auto& TexProps  = pDevice->GetAdapterInfo().Texture;

    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D is not supported by this device";
    }

    uint4      TexSize{TexProps.MaxTexture2DDimension, TexProps.MaxTexture2DDimension, 1u, 1u};
    const auto BPP           = 4u;
    const auto MaxMemorySize = std::min(MaxResourceSpaceSize, SparseRes.ResourceSpaceSize) >> 1;

    if ((Uint64{TexSize.x} * Uint64{TexSize.y} * BPP * 3) / 2 > MaxMemorySize)
        TexSize.y = std::max(1u, static_cast<Uint32>(MaxMemorySize / (Uint64{TexSize.x} * BPP * 3)) * 2);

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_SHADER_RESOURCE, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, SparseRes.ResourceSpaceSize);

    LOG_INFO_MESSAGE("Created sparse 2D texture with dimension ", TexSize.x, "x", TexSize.y, ", size ", TexSparseProps.AddressSpaceSize >> 20, " Mb");
}


TEST_F(SparseResourceTest, LargeTexture2DArray)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;
    const auto& TexProps  = pDevice->GetAdapterInfo().Texture;

    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D array with mip tail is not supported by this device";
    }

    uint4      TexSize{TexProps.MaxTexture2DDimension, TexProps.MaxTexture2DDimension, 1u, TexProps.MaxTexture2DArraySlices};
    const auto BPP           = 4u;
    const auto MaxMemorySize = std::min(MaxResourceSpaceSize, SparseRes.ResourceSpaceSize) >> 1;

    if ((Uint64{TexSize.x} * Uint64{TexSize.y} * TexSize.w * BPP * 3) / 2 > MaxMemorySize)
        TexSize.y = std::max(1u, static_cast<Uint32>(MaxMemorySize / (Uint64{TexSize.x} * TexSize.w * BPP * 3)) * 2);

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_SHADER_RESOURCE, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, SparseRes.ResourceSpaceSize);

    LOG_INFO_MESSAGE("Created sparse 2D texture array with dimension ", TexSize.x, "x", TexSize.y, ", layers ", TexSize.w, ", size ", TexSparseProps.AddressSpaceSize >> 20, " Mb");
}


TEST_F(SparseResourceTest, LargeTexture3D)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;
    const auto& TexProps  = pDevice->GetAdapterInfo().Texture;

    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 3D is not supported by this device";
    }

    uint4      TexSize{TexProps.MaxTexture3DDimension, TexProps.MaxTexture3DDimension, TexProps.MaxTexture3DDimension, 1u};
    const auto BPP           = 4u;
    const auto MaxMemorySize = std::min(MaxResourceSpaceSize, SparseRes.ResourceSpaceSize) >> 4;

    if ((Uint64{TexSize.x} * Uint64{TexSize.y} * Uint64{TexSize.z} * BPP * 3) / 2 > MaxMemorySize)
        TexSize.z = std::max(1u, static_cast<Uint32>(MaxMemorySize / (Uint64{TexSize.x} * Uint64{TexSize.y} * BPP * 3)) * 2);

    const auto Bind = pDevice->GetDeviceInfo().IsMetalDevice() ? BIND_RENDER_TARGET : BIND_UNORDERED_ACCESS;

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, Bind, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckSparseTextureProperties(pTexture);
    ASSERT_LE(TexSparseProps.AddressSpaceSize, SparseRes.ResourceSpaceSize);

    LOG_INFO_MESSAGE("Created sparse 3D texture with dimension ", TexSize.x, "x", TexSize.y, "x", TexSize.z, ", size ", TexSparseProps.AddressSpaceSize >> 20, " Mb");
}


TEST_F(SparseResourceTest, GetSparseTextureFormatInfo)
{
    auto*       pEnv      = GPUTestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

    if ((SparseRes.CapFlags & (SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D | SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D)) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D or 3D are not supported by this device";
    }

    const bool IsDirect3D = pDevice->GetDeviceInfo().IsD3DDevice();
    const bool IsMetal    = pDevice->GetDeviceInfo().IsMetalDevice();
    const bool IsVulkan   = pDevice->GetDeviceInfo().IsVulkanDevice();

    const auto CheckInfo = [&](TEXTURE_FORMAT TexFormat, RESOURCE_DIMENSION Dimension, Uint32 SampleCount, const char* FmtName, BIND_FLAGS PossibleBindFlags) //
    {
        SparseTextureFormatInfo Info = pDevice->GetSparseTextureFormatInfo(TexFormat, Dimension, SampleCount);

        if (Info.BindFlags == BIND_NONE)
            return; // not supported

        LOG_INFO_MESSAGE("Supported sparse texture ", GetResourceDimString(Dimension), " with format ", FmtName, ", sample count ", SampleCount,
                         ", tile size ", Info.TileSize[0], "x", Info.TileSize[1], "x", Info.TileSize[2],
                         ", bind flags ", GetBindFlagsString(Info.BindFlags));

        EXPECT_TRUE(IsPowerOfTwo(Info.TileSize[0]));
        EXPECT_TRUE(IsPowerOfTwo(Info.TileSize[1]));
        EXPECT_TRUE(IsPowerOfTwo(Info.TileSize[2]));
        EXPECT_GT(Info.TileSize[0], 1u);
        EXPECT_GT(Info.TileSize[1], 1u);

        if (Dimension == RESOURCE_DIM_TEX_3D)
            EXPECT_GE(Info.TileSize[2], 1u);
        else
            EXPECT_EQ(Info.TileSize[2], 1u);

        EXPECT_TRUE((Info.BindFlags & BIND_SHADER_RESOURCE) != 0);

        if (PossibleBindFlags != BIND_NONE)
            EXPECT_TRUE((Info.BindFlags & PossibleBindFlags) != 0);

        if (IsMetal)
            EXPECT_EQ(Info.Flags, SPARSE_TEXTURE_FLAG_NONSTANDARD_BLOCK_SIZE);
        if (IsDirect3D)
            EXPECT_TRUE((Info.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL) == 0); // single mip tail is not supported in D3D11/12

        if (SampleCount > 1)
            EXPECT_TRUE((Info.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) != 0);

        if ((Info.Flags & SPARSE_TEXTURE_FLAG_NONSTANDARD_BLOCK_SIZE) == 0)
        {
            TextureDesc TexDesc;
            TexDesc.Type        = Dimension;
            TexDesc.Width       = 1024;
            TexDesc.Height      = 1024;
            TexDesc.Format      = TexFormat;
            TexDesc.MipLevels   = 1;
            TexDesc.SampleCount = SampleCount;

            if (TexDesc.IsArray())
                TexDesc.ArraySize = 64;
            if (TexDesc.Is3D())
                TexDesc.Depth = 64;

            auto Props = GetStandardSparseTextureProperties(TexDesc);
            EXPECT_EQ(Info.TileSize[0], Props.TileSize[0]);
            EXPECT_EQ(Info.TileSize[1], Props.TileSize[1]);
            EXPECT_EQ(Info.TileSize[2], Props.TileSize[2]);
        }
    };

    // clang-format off
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,          RESOURCE_DIM_TEX_2D, 1, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA32_FLOAT,         RESOURCE_DIM_TEX_2D, 1, "RGBA32_FLOAT", BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_BC1_UNORM,            RESOURCE_DIM_TEX_2D, 1, "BC1_UNORM",    BIND_NONE);
    CheckInfo(TEX_FORMAT_BC2_UNORM,            RESOURCE_DIM_TEX_2D, 1, "BC2_UNORM",    BIND_NONE);
    CheckInfo(TEX_FORMAT_BC5_UNORM,            RESOURCE_DIM_TEX_2D, 1, "BC5_UNORM",    BIND_NONE);
    CheckInfo(TEX_FORMAT_D24_UNORM_S8_UINT,    RESOURCE_DIM_TEX_2D, 1, "D24_S8",       BIND_DEPTH_STENCIL);
    CheckInfo(TEX_FORMAT_D32_FLOAT_S8X24_UINT, RESOURCE_DIM_TEX_2D, 1, "D32_FLOAT_S8", BIND_DEPTH_STENCIL);
    CheckInfo(TEX_FORMAT_D32_FLOAT,            RESOURCE_DIM_TEX_2D, 1, "D32_FLOAT",    BIND_DEPTH_STENCIL);

    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D_ARRAY,   1, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D_ARRAY,   4, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_BC1_UNORM,    RESOURCE_DIM_TEX_2D_ARRAY,   1, "BC1_UNORM",    BIND_NONE);
    CheckInfo(TEX_FORMAT_BC2_UNORM,    RESOURCE_DIM_TEX_2D_ARRAY,   1, "BC2_UNORM",    BIND_NONE);
    CheckInfo(TEX_FORMAT_BC5_UNORM,    RESOURCE_DIM_TEX_2D_ARRAY,   1, "BC5_UNORM",    BIND_NONE);

    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_CUBE,       1, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_CUBE_ARRAY, 1, "RGBA8_UNORM",  BIND_RENDER_TARGET);

    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D,         2, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D,         4, "RGBA8_UNORM",  BIND_RENDER_TARGET); // Direct3D supports only 4x
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D,         8, "RGBA8_UNORM",  BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_2D,        16, "RGBA8_UNORM",  BIND_RENDER_TARGET);

    CheckInfo(TEX_FORMAT_RGBA8_UNORM,  RESOURCE_DIM_TEX_3D,         1, "RGBA8_UNORM",  IsVulkan ? BIND_NONE : BIND_RENDER_TARGET);
    CheckInfo(TEX_FORMAT_RGBA32_FLOAT, RESOURCE_DIM_TEX_3D,         1, "RGBA32_FLOAT", IsVulkan ? BIND_NONE : BIND_RENDER_TARGET);
    // clang-format on
}

} // namespace
