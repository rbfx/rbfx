/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "pch.h"

#include "RenderDeviceD3D11Impl.hpp"

#include "DeviceContextD3D11Impl.hpp"
#include "BufferD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "Texture1D_D3D11.hpp"
#include "Texture2D_D3D11.hpp"
#include "Texture3D_D3D11.hpp"
#include "SamplerD3D11Impl.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "PipelineResourceSignatureD3D11Impl.hpp"
#include "FenceD3D11Impl.hpp"
#include "QueryD3D11Impl.hpp"
#include "RenderPassD3D11Impl.hpp"
#include "FramebufferD3D11Impl.hpp"
#include "DeviceMemoryD3D11Impl.hpp"

#include "D3D11TypeConversions.hpp"
#include "EngineMemory.h"

namespace Diligent
{

class BottomLevelASD3D11Impl
{};
class TopLevelASD3D11Impl
{};
class ShaderBindingTableD3D11Impl
{};

RenderDeviceD3D11Impl::RenderDeviceD3D11Impl(IReferenceCounters*          pRefCounters,
                                             IMemoryAllocator&            RawMemAllocator,
                                             IEngineFactory*              pEngineFactory,
                                             const EngineD3D11CreateInfo& EngineCI,
                                             const GraphicsAdapterInfo&   AdapterInfo,
                                             ID3D11Device*                pd3d11Device) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        EngineCI,
        AdapterInfo
    },
    m_pd3d11Device{pd3d11Device}
// clang-format on
{
    m_DeviceInfo.Type = RENDER_DEVICE_TYPE_D3D11;
    switch (m_pd3d11Device->GetFeatureLevel())
    {
        case D3D_FEATURE_LEVEL_11_1:
            m_DeviceInfo.APIVersion            = {11, 1};
            m_DeviceInfo.MaxShaderVersion.HLSL = {5, 1};
            break;
        case D3D_FEATURE_LEVEL_11_0:
            m_DeviceInfo.APIVersion            = {11, 0};
            m_DeviceInfo.MaxShaderVersion.HLSL = {5, 0};
            break;

        case D3D_FEATURE_LEVEL_10_1:
            m_DeviceInfo.APIVersion            = {10, 1};
            m_DeviceInfo.MaxShaderVersion.HLSL = {4, 1};
            break;

        case D3D_FEATURE_LEVEL_10_0:
            m_DeviceInfo.APIVersion            = {10, 0};
            m_DeviceInfo.MaxShaderVersion.HLSL = {4, 0};
            break;

        default: UNEXPECTED("Unexpected D3D feature level");
    }

#ifdef DILIGENT_DEVELOPMENT
#    define CHECK_D3D11_DEVICE_VERSION(Version)               \
        if (CComQIPtr<ID3D11Device##Version>{m_pd3d11Device}) \
            m_MaxD3D11DeviceVersion = Version;

#    if D3D11_VERSION >= 1
    CHECK_D3D11_DEVICE_VERSION(1)
#    endif
#    if D3D11_VERSION >= 2
    CHECK_D3D11_DEVICE_VERSION(2)
#    endif
#    if D3D11_VERSION >= 3
    CHECK_D3D11_DEVICE_VERSION(3)
#    endif
#    if D3D11_VERSION >= 4
    CHECK_D3D11_DEVICE_VERSION(4)
#    endif

#    undef CHECK_D3D11_DEVICE_VERSION
#endif

    // Initialize device features
    m_DeviceInfo.Features = EnableDeviceFeatures(m_AdapterInfo.Features, EngineCI.Features);

    InitShaderCompilationThreadPool(EngineCI.pAsyncShaderCompilationThreadPool, EngineCI.NumAsyncShaderCompilationThreads);
}

RenderDeviceD3D11Impl::~RenderDeviceD3D11Impl()
{
}

void RenderDeviceD3D11Impl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto DXGIFormat = TexFormatToDXGI_Format(TexFormat);

    UINT FormatSupport = 0;
    auto hr            = m_pd3d11Device->CheckFormatSupport(DXGIFormat, &FormatSupport);
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("CheckFormatSupport() failed for format ", DXGIFormat);
        return;
    }

    TexFormatInfo.Filterable =
        ((FormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) != 0) ||
        ((FormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON) != 0);

    TexFormatInfo.BindFlags = BIND_SHADER_RESOURCE;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0)
        TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
        TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW) != 0)
        TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;

    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE1D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D | RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D | RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE3D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURECUBE) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE | RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;

    TexFormatInfo.SampleCounts = SAMPLE_COUNT_NONE;
    for (Uint32 SampleCount = 1; SampleCount <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount *= 2)
    {
        UINT QualityLevels = 0;

        hr = m_pd3d11Device->CheckMultisampleQualityLevels(DXGIFormat, SampleCount, &QualityLevels);
        if (SUCCEEDED(hr) && QualityLevels > 0)
            TexFormatInfo.SampleCounts |= static_cast<SAMPLE_COUNT>(SampleCount);
    }
}

IMPLEMENT_QUERY_INTERFACE(RenderDeviceD3D11Impl, IID_RenderDeviceD3D11, TRenderDeviceBase)

void RenderDeviceD3D11Impl::CreateBufferFromD3DResource(ID3D11Buffer* pd3d11Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, InitialState, pd3d11Buffer);
}

void RenderDeviceD3D11Impl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, pBuffData);
}

void RenderDeviceD3D11Impl::CreateShader(const ShaderCreateInfo& ShaderCI,
                                         IShader**               ppShader,
                                         IDataBlob**             ppCompilerOutput)
{
    const ShaderD3D11Impl::CreateInfo D3D11ShaderCI{
        {
            GetDeviceInfo(),
            GetAdapterInfo(),
            nullptr, // pDXCompiler
            ppCompilerOutput,
            m_pShaderCompilationThreadPool,
        },
        GetD3D11Device()->GetFeatureLevel(),
    };
    CreateShaderImpl(ppShader, ShaderCI, D3D11ShaderCI);
}

void RenderDeviceD3D11Impl::CreateTexture1DFromD3DResource(ID3D11Texture1D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture1D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_D3D11 instance", Texture1D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                       });
}

void RenderDeviceD3D11Impl::CreateTexture2DFromD3DResource(ID3D11Texture2D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture2D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_D3D11 instance", Texture2D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                       });
}

void RenderDeviceD3D11Impl::CreateTexture3DFromD3DResource(ID3D11Texture3D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture3D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_D3D11 instance", Texture3D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                       });
}


void RenderDeviceD3D11Impl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11 = nullptr;
                           switch (TexDesc.Type)
                           {
                               case RESOURCE_DIM_TEX_1D:
                               case RESOURCE_DIM_TEX_1D_ARRAY:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_D3D11 instance", Texture1D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               case RESOURCE_DIM_TEX_2D:
                               case RESOURCE_DIM_TEX_2D_ARRAY:
                               case RESOURCE_DIM_TEX_CUBE:
                               case RESOURCE_DIM_TEX_CUBE_ARRAY:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_D3D11 instance", Texture2D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               case RESOURCE_DIM_TEX_3D:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_D3D11 instance", Texture3D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               default: LOG_ERROR_AND_THROW("Unknown texture type. (Did you forget to initialize the Type member of TextureDesc structure?)");
                           }
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                       });
}

void RenderDeviceD3D11Impl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateSamplerImpl(ppSampler, SamplerDesc);
}

void RenderDeviceD3D11Impl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceD3D11Impl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceD3D11Impl::CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    UNSUPPORTED("Ray tracing is not supported in DirectX 11");
    *ppPipelineState = nullptr;
}

void RenderDeviceD3D11Impl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateFenceImpl(ppFence, Desc);
}

void RenderDeviceD3D11Impl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateQueryImpl(ppQuery, Desc);
}

void RenderDeviceD3D11Impl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateRenderPassImpl(ppRenderPass, Desc);
}

void RenderDeviceD3D11Impl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateFramebufferImpl(ppFramebuffer, Desc);
}

void RenderDeviceD3D11Impl::CreateBLAS(const BottomLevelASDesc& Desc,
                                       IBottomLevelAS**         ppBLAS)
{
    UNSUPPORTED("CreateBLAS is not supported in DirectX 11");
    *ppBLAS = nullptr;
}

void RenderDeviceD3D11Impl::CreateTLAS(const TopLevelASDesc& Desc,
                                       ITopLevelAS**         ppTLAS)
{
    UNSUPPORTED("CreateTLAS is not supported in DirectX 11");
    *ppTLAS = nullptr;
}

void RenderDeviceD3D11Impl::CreateSBT(const ShaderBindingTableDesc& Desc,
                                      IShaderBindingTable**         ppSBT)
{
    UNSUPPORTED("CreateSBT is not supported in DirectX 11");
    *ppSBT = nullptr;
}

void RenderDeviceD3D11Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                            IPipelineResourceSignature**         ppSignature)
{
    CreatePipelineResourceSignature(Desc, ppSignature, SHADER_TYPE_UNKNOWN, false);
}

void RenderDeviceD3D11Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                            IPipelineResourceSignature**         ppSignature,
                                                            SHADER_TYPE                          ShaderStages,
                                                            bool                                 IsDeviceInternal)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, ShaderStages, IsDeviceInternal);
}

void RenderDeviceD3D11Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&              Desc,
                                                            const PipelineResourceSignatureInternalDataD3D11& InternalData,
                                                            IPipelineResourceSignature**                      ppSignature)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, InternalData);
}

void RenderDeviceD3D11Impl::CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo, IDeviceMemory** ppMemory)
{
    CreateDeviceMemoryImpl(ppMemory, CreateInfo);
}

void RenderDeviceD3D11Impl::CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                     IPipelineStateCache**               ppPSOCache)
{
    *ppPSOCache = nullptr;
}

void RenderDeviceD3D11Impl::IdleGPU()
{
    VERIFY_EXPR(m_wpImmediateContexts.size() == 1);

    if (auto pImmediateCtx = m_wpImmediateContexts[0].Lock())
    {
        pImmediateCtx->WaitForIdle();
    }
}

SparseTextureFormatInfo RenderDeviceD3D11Impl::GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                          RESOURCE_DIMENSION Dimension,
                                                                          Uint32             SampleCount) const
{
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport2{
        TexFormatToDXGI_Format(TexFormat), // .InFormat
        {}};
    if (FAILED(m_pd3d11Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport2, sizeof(FormatSupport2))) ||
        (FormatSupport2.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_TILED) != D3D11_FORMAT_SUPPORT2_TILED)
    {
        return {};
    }

    return TRenderDeviceBase::GetSparseTextureFormatInfo(TexFormat, Dimension, SampleCount);
}

} // namespace Diligent
