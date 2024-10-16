/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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

#include "RenderDeviceWebGPUImpl.hpp"
#include "DeviceContextWebGPUImpl.hpp"
#include "RenderPassWebGPUImpl.hpp"
#include "TextureWebGPUImpl.hpp"
#include "BufferWebGPUImpl.hpp"
#include "PipelineStateWebGPUImpl.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"
#include "PipelineResourceAttribsWebGPU.hpp"
#include "ShaderResourceCacheWebGPU.hpp"
#include "RenderPassWebGPUImpl.hpp"
#include "ShaderWebGPUImpl.hpp"
#include "FramebufferWebGPUImpl.hpp"
#include "SamplerWebGPUImpl.hpp"
#include "FenceWebGPUImpl.hpp"
#include "QueryWebGPUImpl.hpp"
#include "AttachmentCleanerWebGPU.hpp"
#include "WebGPUStubs.hpp"

#if !DILIGENT_NO_GLSLANG
#    include "GLSLangUtils.hpp"
#endif

#if PLATFORM_EMSCRIPTEN
#    include <emscripten.h>
#endif

namespace Diligent
{

class BottomLevelASWebGPUImpl
{};

class TopLevelASWebGPUImpl
{};

class ShaderBindingTableWebGPUImpl
{};

class DeviceMemoryWebGPUImpl
{};

static void DebugMessengerCallback(WGPUErrorType MessageType, const char* Message, void* pUserData)
{
    if (Message != nullptr)
        LOG_DEBUG_MESSAGE(DEBUG_MESSAGE_SEVERITY_ERROR, "WebGPU: ", Message);
}

RenderDeviceWebGPUImpl::RenderDeviceWebGPUImpl(IReferenceCounters*           pRefCounters,
                                               IMemoryAllocator&             RawMemAllocator,
                                               IEngineFactory*               pEngineFactory,
                                               const EngineWebGPUCreateInfo& EngineCI,
                                               const GraphicsAdapterInfo&    AdapterInfo,
                                               WGPUInstance                  wgpuInstance,
                                               WGPUAdapter                   wgpuAdapter,
                                               WGPUDevice                    wgpuDevice) :

    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        EngineCI,
        AdapterInfo
    },
    m_wgpuInstance(wgpuInstance),
    m_wgpuAdapter{wgpuAdapter},
    m_wgpuDevice{wgpuDevice}
// clang-format on
{
    WGPUSupportedLimits wgpuSupportedLimits{};
    wgpuDeviceGetLimits(m_wgpuDevice, &wgpuSupportedLimits);
    m_wgpuLimits = wgpuSupportedLimits.limits;

    wgpuDeviceSetUncapturedErrorCallback(m_wgpuDevice, DebugMessengerCallback, nullptr);
    FindSupportedTextureFormats();

    m_DeviceInfo.Type = RENDER_DEVICE_TYPE_WEBGPU;
    m_DeviceInfo.NDC  = NDCAttribs{0.0f, 1.0f, -0.5f};

    m_DeviceInfo.Features = EnableDeviceFeatures(m_AdapterInfo.Features, EngineCI.Features);

    m_pUploadMemoryManager  = std::make_unique<UploadMemoryManagerWebGPU>(m_wgpuDevice, EngineCI.UploadHeapPageSize);
    m_pDynamicMemoryManager = std::make_unique<DynamicMemoryManagerWebGPU>(m_wgpuDevice, EngineCI.DynamicHeapPageSize, EngineCI.DynamicHeapSize);
    m_pAttachmentCleaner    = std::make_unique<AttachmentCleanerWebGPU>(*this);
    m_pMipsGenerator        = std::make_unique<GenerateMipsHelperWebGPU>(*this);
    m_pQueryManager         = std::make_unique<QueryManagerWebGPU>(this, EngineCI.QueryPoolSizes);

#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::InitializeGlslang();
#endif

    InitShaderCompilationThreadPool(EngineCI.pAsyncShaderCompilationThreadPool, EngineCI.NumAsyncShaderCompilationThreads);
}

RenderDeviceWebGPUImpl::~RenderDeviceWebGPUImpl()
{
#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::FinalizeGlslang();
#endif
#if !PLATFORM_EMSCRIPTEN
    IdleGPU();
#endif
}

void RenderDeviceWebGPUImpl::CreateBuffer(const BufferDesc& BuffDesc,
                                          const BufferData* pBuffData,
                                          IBuffer**         ppBuffer,
                                          bool              bIsDeviceInternal)
{
    CreateBufferImpl(ppBuffer, BuffDesc, pBuffData, bIsDeviceInternal);
}

void RenderDeviceWebGPUImpl::CreateBuffer(const BufferDesc& BuffDesc,
                                          const BufferData* pBuffData,
                                          IBuffer**         ppBuffer)
{
    CreateBuffer(BuffDesc, pBuffData, ppBuffer, false);
}

void RenderDeviceWebGPUImpl::CreateTexture(const TextureDesc& TexDesc,
                                           const TextureData* pData,
                                           ITexture**         ppTexture)
{
    CreateTexture(TexDesc, pData, ppTexture, false);
}

void RenderDeviceWebGPUImpl::CreateTexture(const TextureDesc& TexDesc,
                                           const TextureData* pData,
                                           ITexture**         ppTexture,
                                           bool               IsDeviceInternal)
{
    CreateTextureImpl(ppTexture, TexDesc, pData, IsDeviceInternal);
}

void RenderDeviceWebGPUImpl::CreateSampler(const SamplerDesc& SamplerDesc,
                                           ISampler**         ppSampler,
                                           bool               IsDeviceInternal)
{
    CreateSamplerImpl(ppSampler, SamplerDesc, IsDeviceInternal);
}

void RenderDeviceWebGPUImpl::CreateSampler(const SamplerDesc& SamplerDesc,
                                           ISampler**         ppSampler)
{
    CreateSampler(SamplerDesc, ppSampler, false);
}

void RenderDeviceWebGPUImpl::CreateShader(const ShaderCreateInfo& ShaderCI,
                                          IShader**               ppShader,
                                          IDataBlob**             ppCompilerOutput)
{
    const ShaderWebGPUImpl::CreateInfo wgpuShaderCI{
        GetDeviceInfo(),
        GetAdapterInfo(),
        ppCompilerOutput,
        m_pShaderCompilationThreadPool,
    };
    CreateShaderImpl(ppShader, ShaderCI, wgpuShaderCI);
}

void RenderDeviceWebGPUImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                             IPipelineResourceSignature**         ppSignature)
{
    CreatePipelineResourceSignature(Desc, ppSignature, SHADER_TYPE_UNKNOWN, false);
}

void RenderDeviceWebGPUImpl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                                         IPipelineState**                       ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceWebGPUImpl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                                        IPipelineState**                      ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceWebGPUImpl::CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo,
                                                           IPipelineState**                         ppPipelineState)
{
    UNSUPPORTED("Ray tracing is not supported in WebGPU");
    *ppPipelineState = nullptr;
}

void RenderDeviceWebGPUImpl::CreateFence(const FenceDesc& Desc,
                                         IFence**         ppFence)
{
    CreateFenceImpl(ppFence, Desc);
}

void RenderDeviceWebGPUImpl::CreateQuery(const QueryDesc& Desc,
                                         IQuery**         ppQuery)
{
    CreateQueryImpl(ppQuery, Desc);
}

void RenderDeviceWebGPUImpl::CreateRenderPass(const RenderPassDesc& Desc,
                                              IRenderPass**         ppRenderPass)
{
    CreateRenderPassImpl(ppRenderPass, Desc);
}

void RenderDeviceWebGPUImpl::CreateFramebuffer(const FramebufferDesc& Desc,
                                               IFramebuffer**         ppFramebuffer)
{
    CreateFramebufferImpl(ppFramebuffer, Desc);
}

void RenderDeviceWebGPUImpl::CreateBLAS(const BottomLevelASDesc& Desc,
                                        IBottomLevelAS**         ppBLAS)
{
    UNSUPPORTED("CreateBLAS is not supported in WebGPU");
    *ppBLAS = nullptr;
}

void RenderDeviceWebGPUImpl::CreateTLAS(const TopLevelASDesc& Desc,
                                        ITopLevelAS**         ppTLAS)
{
    UNSUPPORTED("CreateTLAS is not supported in WebGPU");
    *ppTLAS = nullptr;
}

void RenderDeviceWebGPUImpl::CreateSBT(const ShaderBindingTableDesc& Desc,
                                       IShaderBindingTable**         ppSBT)
{
    UNSUPPORTED("CreateSBT is not supported in WebGPU");
    *ppSBT = nullptr;
}

void RenderDeviceWebGPUImpl::CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo,
                                                IDeviceMemory**               ppMemory)
{
    UNSUPPORTED("CreateDeviceMemory is not supported in WebGPU");
    *ppMemory = nullptr;
}

void RenderDeviceWebGPUImpl::CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                      IPipelineStateCache**               ppPSOCache)
{
    *ppPSOCache = nullptr;
}

SparseTextureFormatInfo RenderDeviceWebGPUImpl::GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                           RESOURCE_DIMENSION Dimension,
                                                                           Uint32             SampleCount) const
{
    UNSUPPORTED("GetSparseTextureFormatInfo is not supported in WebGPU");
    return {};
}

WGPUInstance RenderDeviceWebGPUImpl::GetWebGPUInstance() const
{
    return m_wgpuInstance.Get();
}

WGPUAdapter RenderDeviceWebGPUImpl::GetWebGPUAdapter() const
{
    return m_wgpuAdapter.Get();
}

WGPUDevice RenderDeviceWebGPUImpl::GetWebGPUDevice() const
{
    return m_wgpuDevice;
}

void RenderDeviceWebGPUImpl::IdleGPU()
{
#if PLATFORM_EMSCRIPTEN
    LOG_ERROR_MESSAGE("IRenderDevice::IdleGPU() is not supported on the Web. Use non-blocking synchronization methods.");
#else
    VERIFY_EXPR(m_wpImmediateContexts.size() == 1);
    if (auto pImmediateCtx = m_wpImmediateContexts[0].Lock())
        pImmediateCtx->WaitForIdle();
#endif
}

void RenderDeviceWebGPUImpl::CreateTextureFromWebGPUTexture(WGPUTexture        wgpuTexture,
                                                            const TextureDesc& TexDesc,
                                                            RESOURCE_STATE     InitialState,
                                                            ITexture**         ppTexture)
{
    CreateTextureImpl(ppTexture, TexDesc, InitialState, wgpuTexture, false);
}

void RenderDeviceWebGPUImpl::CreateBufferFromWebGPUBuffer(WGPUBuffer        wgpuBuffer,
                                                          const BufferDesc& BuffDesc,
                                                          RESOURCE_STATE    InitialState,
                                                          IBuffer**         ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, InitialState, wgpuBuffer, false);
}

void RenderDeviceWebGPUImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&               Desc,
                                                             const PipelineResourceSignatureInternalDataWebGPU& InternalData,
                                                             IPipelineResourceSignature**                       ppSignature)
{

    CreatePipelineResourceSignatureImpl(ppSignature, Desc, InternalData);
}

void RenderDeviceWebGPUImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                             IPipelineResourceSignature**         ppSignature,
                                                             SHADER_TYPE                          ShaderStages,
                                                             bool                                 IsDeviceInternal)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, ShaderStages, IsDeviceInternal);
}

GenerateMipsHelperWebGPU& RenderDeviceWebGPUImpl::GetMipsGenerator() const
{
    return *m_pMipsGenerator.get();
}

AttachmentCleanerWebGPU& RenderDeviceWebGPUImpl::GetAttachmentCleaner() const
{
    return *m_pAttachmentCleaner.get();
}

UploadMemoryManagerWebGPU::Page RenderDeviceWebGPUImpl::GetUploadMemoryPage(size_t Size)
{
    return m_pUploadMemoryManager->GetPage(Size);
}

DynamicMemoryManagerWebGPU::Page RenderDeviceWebGPUImpl::GetDynamicMemoryPage(size_t Size)
{
    return m_pDynamicMemoryManager->GetPage(Size);
}

void RenderDeviceWebGPUImpl::DeviceTick()
{
#if !PLATFORM_EMSCRIPTEN
    wgpuDeviceTick(m_wgpuDevice);
#endif
}

void RenderDeviceWebGPUImpl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    VERIFY(m_TextureFormatsInfo[TexFormat].Supported, "Texture format is not supported");
}

void RenderDeviceWebGPUImpl::FindSupportedTextureFormats()
{
    const TextureProperties& TexCaps = GetAdapterInfo().Texture;

    constexpr Uint32 FMT_FLAG_NONE   = 0x00;
    constexpr Uint32 FMT_FLAG_MSAA   = 0x01;
    constexpr Uint32 FMT_FLAG_FILTER = 0x02;

    constexpr BIND_FLAGS BIND_SRU = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
    constexpr BIND_FLAGS BIND_SR  = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    constexpr BIND_FLAGS BIND_S   = BIND_SHADER_RESOURCE;
    constexpr BIND_FLAGS BIND_SU  = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    constexpr BIND_FLAGS BIND_D   = BIND_DEPTH_STENCIL;

    SAMPLE_COUNT SupportedSampleCounts = SAMPLE_COUNT_1 | SAMPLE_COUNT_4; // We can't query supported sample counts in WebGPU

    auto SetTexFormatInfo = [&](std::initializer_list<TEXTURE_FORMAT> Formats, BIND_FLAGS BindFlags, Uint32 FmtFlags) {
        for (TEXTURE_FORMAT Fmt : Formats)
        {
            TextureFormatInfoExt& FmtInfo = m_TextureFormatsInfo[Fmt];
            VERIFY(!FmtInfo.Supported, "The format has already been initialized");

            FmtInfo.Supported = true;
            FmtInfo.BindFlags = BindFlags;

            FmtInfo.SampleCounts = SAMPLE_COUNT_1;
            if ((FmtFlags & FMT_FLAG_MSAA) != 0)
            {
                VERIFY_EXPR((FmtInfo.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) != 0 || FmtInfo.IsTypeless);
                FmtInfo.SampleCounts |= SupportedSampleCounts;
            }

            // clang-format off
            FmtInfo.Dimensions =
                RESOURCE_DIMENSION_SUPPORT_TEX_2D       |
                RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY |
                RESOURCE_DIMENSION_SUPPORT_TEX_CUBE;

            if (TexCaps.CubemapArraysSupported)
                FmtInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;

            if (!(FmtInfo.ComponentType == COMPONENT_TYPE_COMPRESSED ||
                  FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH ||
                  FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL))
            {
                FmtInfo.Dimensions |=
                    RESOURCE_DIMENSION_SUPPORT_TEX_1D       |
                    RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY |
                    RESOURCE_DIMENSION_SUPPORT_TEX_3D;
            }
            // clang-format on
            FmtInfo.Filterable = (FmtFlags & FMT_FLAG_FILTER) != 0;
        }
    };

    const bool BGRA8UnormStorageSupported       = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_BGRA8UnormStorage);
    const bool Float32FilterableSupported       = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_Float32Filterable);
    const bool RG11B10UfloatRenderableSupported = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_RG11B10UfloatRenderable);
    const bool Depth32FloatStencil8Supported    = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_Depth32FloatStencil8);
    const bool TextureCompressionBCSupported    = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_TextureCompressionBC);
    const bool R16UnormSupported                = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_Unorm16TextureFormats);
    const bool R16SnormSupported                = wgpuDeviceHasFeature(m_wgpuDevice, WGPUFeatureName_Snorm16TextureFormats);

    // https://www.w3.org/TR/webgpu/#texture-format-caps

    // Color formats with 8-bits per channel
    SetTexFormatInfo({TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_R8_SNORM}, BIND_S, FMT_FLAG_FILTER);
    SetTexFormatInfo({TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SINT}, BIND_SR, FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RG8_SNORM}, BIND_S, FMT_FLAG_FILTER);
    SetTexFormatInfo({TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SINT}, BIND_SR, FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM}, BIND_SRU, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RGBA8_UNORM_SRGB}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RGBA8_SNORM}, BIND_SU, FMT_FLAG_FILTER);
    SetTexFormatInfo({TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SINT}, BIND_SRU, FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM}, BGRA8UnormStorageSupported ? BIND_SRU : BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_BGRA8_UNORM_SRGB}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);

    // Color formats with 16-bits per channel
    SetTexFormatInfo({TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SINT}, BIND_SR, FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_R16_FLOAT, TEX_FORMAT_R16_TYPELESS}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SINT}, BIND_SR, FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_TYPELESS}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SINT}, BIND_SRU, FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_TYPELESS}, BIND_SRU, FMT_FLAG_FILTER | FMT_FLAG_MSAA);

    // Color formats with 32-bits per channel
    SetTexFormatInfo({TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT, TEX_FORMAT_R32_TYPELESS}, BIND_SRU, FMT_FLAG_NONE);
    SetTexFormatInfo({TEX_FORMAT_R32_FLOAT}, BIND_SRU, Float32FilterableSupported ? FMT_FLAG_FILTER | FMT_FLAG_MSAA : FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT, TEX_FORMAT_RG32_TYPELESS}, BIND_SRU, FMT_FLAG_NONE);
    SetTexFormatInfo({TEX_FORMAT_RG32_FLOAT}, BIND_SRU, Float32FilterableSupported ? FMT_FLAG_FILTER : FMT_FLAG_NONE);

    SetTexFormatInfo({TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT, TEX_FORMAT_RGBA32_TYPELESS}, BIND_SRU, FMT_FLAG_NONE);
    SetTexFormatInfo({TEX_FORMAT_RGBA32_FLOAT}, BIND_SRU, Float32FilterableSupported ? FMT_FLAG_FILTER : FMT_FLAG_NONE);

    // Color formats with mixed width
    SetTexFormatInfo({TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_RGB10A2_UINT}, BIND_SR, FMT_FLAG_MSAA);

    SetTexFormatInfo({TEX_FORMAT_R11G11B10_FLOAT}, RG11B10UfloatRenderableSupported ? BIND_SR : BIND_S, RG11B10UfloatRenderableSupported ? FMT_FLAG_FILTER | FMT_FLAG_MSAA : FMT_FLAG_FILTER);

    // Depth-stencil formats
    SetTexFormatInfo({TEX_FORMAT_D16_UNORM}, BIND_D, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_D24_UNORM_S8_UINT}, BIND_D, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    SetTexFormatInfo({TEX_FORMAT_D32_FLOAT}, BIND_D, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    if (Depth32FloatStencil8Supported)
        SetTexFormatInfo({TEX_FORMAT_D32_FLOAT_S8X24_UINT}, BIND_D, FMT_FLAG_FILTER | FMT_FLAG_MSAA);

    // Packed formats
    SetTexFormatInfo({TEX_FORMAT_RGB9E5_SHAREDEXP}, BIND_S, FMT_FLAG_FILTER);

    if (TextureCompressionBCSupported)
    {
        SetTexFormatInfo({TEX_FORMAT_BC1_TYPELESS,
                          TEX_FORMAT_BC1_UNORM,
                          TEX_FORMAT_BC1_UNORM_SRGB,
                          TEX_FORMAT_BC2_TYPELESS,
                          TEX_FORMAT_BC2_UNORM,
                          TEX_FORMAT_BC2_UNORM_SRGB,
                          TEX_FORMAT_BC3_TYPELESS,
                          TEX_FORMAT_BC3_UNORM,
                          TEX_FORMAT_BC3_UNORM_SRGB,
                          TEX_FORMAT_BC4_TYPELESS,
                          TEX_FORMAT_BC4_UNORM,
                          TEX_FORMAT_BC4_SNORM,
                          TEX_FORMAT_BC5_TYPELESS,
                          TEX_FORMAT_BC5_UNORM,
                          TEX_FORMAT_BC5_SNORM,
                          TEX_FORMAT_BC6H_TYPELESS,
                          TEX_FORMAT_BC6H_UF16,
                          TEX_FORMAT_BC6H_SF16,
                          TEX_FORMAT_BC7_TYPELESS,
                          TEX_FORMAT_BC7_UNORM,
                          TEX_FORMAT_BC7_UNORM_SRGB},
                         BIND_S, FMT_FLAG_FILTER);
    }

    if (R16UnormSupported)
    {
        SetTexFormatInfo({TEX_FORMAT_R16_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
        SetTexFormatInfo({TEX_FORMAT_RG16_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
        SetTexFormatInfo({TEX_FORMAT_RGBA16_UNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    }

    if (R16SnormSupported)
    {
        SetTexFormatInfo({TEX_FORMAT_R16_SNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
        SetTexFormatInfo({TEX_FORMAT_RG16_SNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
        SetTexFormatInfo({TEX_FORMAT_RGBA16_SNORM}, BIND_SR, FMT_FLAG_FILTER | FMT_FLAG_MSAA);
    }
}

} // namespace Diligent
