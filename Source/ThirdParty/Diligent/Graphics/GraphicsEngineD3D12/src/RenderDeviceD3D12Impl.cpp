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

#include "RenderDeviceD3D12Impl.hpp"

#include <vector>

#include "WinHPreface.h"
#include <dxgi1_4.h>
#include "WinHPostface.h"

#include "PipelineStateD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "SamplerD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "FenceD3D12Impl.hpp"
#include "QueryD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"
#include "FramebufferD3D12Impl.hpp"
#include "BottomLevelASD3D12Impl.hpp"
#include "TopLevelASD3D12Impl.hpp"
#include "ShaderBindingTableD3D12Impl.hpp"
#include "DeviceMemoryD3D12Impl.hpp"
#include "PipelineStateCacheD3D12Impl.hpp"
#include "PipelineResourceSignatureD3D12Impl.hpp"

#include "EngineMemory.h"
#include "D3D12TypeConversions.hpp"
#include "DXGITypeConversions.hpp"
#include "QueryManagerD3D12.hpp"


namespace Diligent
{

namespace
{

D3D_FEATURE_LEVEL GetD3DFeatureLevelFromDevice(ID3D12Device* pd3d12Device)
{
    D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0 //
        };
    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevelsData{};
    FeatureLevelsData.pFeatureLevelsRequested = FeatureLevels;
    FeatureLevelsData.NumFeatureLevels        = _countof(FeatureLevels);
    pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevelsData, sizeof(FeatureLevelsData));
    return FeatureLevelsData.MaxSupportedFeatureLevel;
}

CComPtr<ID3D12Heap> CreateDummyNVApiHeap(ID3D12Device* pd3d12Device)
{
    CComPtr<ID3D12Heap> pNVApiHeap;

#ifdef DILIGENT_ENABLE_D3D_NVAPI
    D3D12_HEAP_DESC d3d12HeapDesc{};
    d3d12HeapDesc.SizeInBytes                     = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
    d3d12HeapDesc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    d3d12HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    d3d12HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    d3d12HeapDesc.Properties.CreationNodeMask     = 1;
    d3d12HeapDesc.Properties.VisibleNodeMask      = 1;
    d3d12HeapDesc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    bool HasResourceHeapTier2 = false;

    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Features = {};
    if (SUCCEEDED(pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Features, sizeof(d3d12Features))))
    {
        HasResourceHeapTier2 = d3d12Features.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2;
    }

    // From NVAPI docs:
    //      pHeap is necessary when bTexture2DArrayMipPack is true.
    //      pHeap can be any heap and this API doesn't change anything to it.
    //
    // On D3D12_RESOURCE_HEAP_TIER_1 hardware, we need to specify the heap usage. Use NON_RT_DS_TEXTURES as the
    // most logical for sparse 2D arrays (the documentation says that pHeap can be any heap anyway).
    d3d12HeapDesc.Flags = HasResourceHeapTier2 ? D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

    if (NvAPI_D3D12_CreateHeap(pd3d12Device, &d3d12HeapDesc, IID_PPV_ARGS(&pNVApiHeap)) != NVAPI_OK)
        LOG_ERROR_MESSAGE("Failed to create default sparse heap using NVApi");
#endif

    return pNVApiHeap;
}

} // namespace

RenderDeviceD3D12Impl::RenderDeviceD3D12Impl(IReferenceCounters*          pRefCounters,
                                             IMemoryAllocator&            RawMemAllocator,
                                             IEngineFactory*              pEngineFactory,
                                             const EngineD3D12CreateInfo& EngineCI,
                                             const GraphicsAdapterInfo&   AdapterInfo,
                                             ID3D12Device*                pd3d12Device,
                                             size_t                       CommandQueueCount,
                                             ICommandQueueD3D12**         ppCmdQueues) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        CommandQueueCount,
        ppCmdQueues,
        EngineCI,
        AdapterInfo
    },
    m_pd3d12Device   {pd3d12Device},
    m_CPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[2], D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[3], D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE}
    },
    m_GPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[0], EngineCI.GPUDescriptorHeapDynamicSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE},
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[1], EngineCI.GPUDescriptorHeapDynamicSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}
    },
    m_CmdListManagers
    {
        {*this, D3D12_COMMAND_LIST_TYPE_DIRECT},
        {*this, D3D12_COMMAND_LIST_TYPE_COMPUTE},
        {*this, D3D12_COMMAND_LIST_TYPE_COPY}
    },
    m_DynamicMemoryManager  {GetRawAllocator(), *this, EngineCI.NumDynamicHeapPagesToReserve, EngineCI.DynamicHeapPageSize},
    m_MipsGenerator         {pd3d12Device},
    m_pDxCompiler           {CreateDXCompiler(DXCompilerTarget::Direct3D12, 0, EngineCI.pDxCompilerPath)},
    m_RootSignatureAllocator{GetRawAllocator(), sizeof(RootSignatureD3D12), 128},
    m_RootSignatureCache    {*this}
// clang-format on
{
    m_DeviceInfo.Type = RENDER_DEVICE_TYPE_D3D12;

    try
    {
        // Enable requested device features
        m_DeviceInfo.Features = EnableDeviceFeatures(m_AdapterInfo.Features, EngineCI.Features);

        auto FeatureLevel = GetD3DFeatureLevelFromDevice(m_pd3d12Device);
        switch (FeatureLevel)
        {
            case D3D_FEATURE_LEVEL_12_1: m_DeviceInfo.APIVersion = {12, 1}; break;
            case D3D_FEATURE_LEVEL_12_0: m_DeviceInfo.APIVersion = {12, 0}; break;
            case D3D_FEATURE_LEVEL_11_1: m_DeviceInfo.APIVersion = {11, 1}; break;
            case D3D_FEATURE_LEVEL_11_0: m_DeviceInfo.APIVersion = {11, 0}; break;
            case D3D_FEATURE_LEVEL_10_1: m_DeviceInfo.APIVersion = {10, 1}; break;
            case D3D_FEATURE_LEVEL_10_0: m_DeviceInfo.APIVersion = {10, 0}; break;
            default: UNEXPECTED("Unexpected D3D feature level");
        }

        // Detect maximum  shader model.
        {
            // Direct3D12 supports shader model 5.1 on all feature levels.
            // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
            D3D_SHADER_MODEL MaxShaderModel = D3D_SHADER_MODEL_5_1;

            // Header may not have constants for D3D_SHADER_MODEL_6_1 and above.
            const D3D_SHADER_MODEL Models[] = //
                {
                    static_cast<D3D_SHADER_MODEL>(0x67),
                    static_cast<D3D_SHADER_MODEL>(0x66),
                    static_cast<D3D_SHADER_MODEL>(0x65), // minimum required for mesh shader and DXR 1.1
                    static_cast<D3D_SHADER_MODEL>(0x64),
                    static_cast<D3D_SHADER_MODEL>(0x63), // minimum required for DXR 1.0
                    static_cast<D3D_SHADER_MODEL>(0x62),
                    static_cast<D3D_SHADER_MODEL>(0x61),
                    D3D_SHADER_MODEL_6_0 //
                };

            for (auto Model : Models)
            {
                D3D12_FEATURE_DATA_SHADER_MODEL ShaderModel = {Model};
                if (SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModel, sizeof(ShaderModel))))
                {
                    MaxShaderModel = ShaderModel.HighestShaderModel;
                    break;
                }
            }

            auto& MaxHLSLVersion = m_DeviceInfo.MaxShaderVersion.HLSL;
            MaxHLSLVersion.Major = (MaxShaderModel >> 4) & 0xFu;
            MaxHLSLVersion.Minor = MaxShaderModel & 0xFu;

            LOG_INFO_MESSAGE("Max device shader model: ", Uint32{MaxHLSLVersion.Major}, '_', Uint32{MaxHLSLVersion.Minor} & 0xF);
        }

#ifdef DILIGENT_DEVELOPMENT
#    define CHECK_D3D12_DEVICE_VERSION(Version)               \
        if (CComQIPtr<ID3D12Device##Version>{m_pd3d12Device}) \
            m_MaxD3D12DeviceVersion = Version;

        CHECK_D3D12_DEVICE_VERSION(1)
        CHECK_D3D12_DEVICE_VERSION(2)
        CHECK_D3D12_DEVICE_VERSION(3)
        CHECK_D3D12_DEVICE_VERSION(4)
        CHECK_D3D12_DEVICE_VERSION(5)

#    undef CHECK_D3D12_DEVICE_VERSION
#endif

        m_QueryMgrs.reserve(CommandQueueCount);
        for (Uint32 q = 0; q < CommandQueueCount; ++q)
        {
            const auto d3d12CmdListType = ppCmdQueues[q]->GetD3D12CommandQueueDesc().Type;
            const auto HWQueueId        = D3D12CommandListTypeToQueueId(d3d12CmdListType);
            m_QueryMgrs.emplace_back(std::make_unique<QueryManagerD3D12>(this, EngineCI.QueryPoolSizes, SoftwareQueueIndex{q}, HWQueueId));
        }

        if (IsNvApiEnabled())
            m_pNVApiHeap = CreateDummyNVApiHeap(m_pd3d12Device);

        // Check PSO cache support
        {
            D3D12_FEATURE_DATA_SHADER_CACHE ShaderCacheFeature{};
            if (SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &ShaderCacheFeature, sizeof(ShaderCacheFeature))))
            {
                // AZ TODO: add support for D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO
                m_IsPSOCacheSupported = (ShaderCacheFeature.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0;
            }
        }

        InitShaderCompilationThreadPool(EngineCI.pAsyncShaderCompilationThreadPool, EngineCI.NumAsyncShaderCompilationThreads);
    }
    catch (...)
    {
        m_DynamicMemoryManager.Destroy();
        throw;
    }
}

CommandListManager& RenderDeviceD3D12Impl::GetCmdListManager(SoftwareQueueIndex CommandQueueId)
{
    return GetCmdListManager(GetCommandQueueType(CommandQueueId));
}

CommandListManager& RenderDeviceD3D12Impl::GetCmdListManager(D3D12_COMMAND_LIST_TYPE CmdListType)
{
    return m_CmdListManagers[D3D12CommandListTypeToQueueId(CmdListType)];
}

RenderDeviceD3D12Impl::~RenderDeviceD3D12Impl()
{
    // Wait for the GPU to complete all its operations
    IdleGPU();
    ReleaseStaleResources(true);

#ifdef DILIGENT_DEVELOPMENT
    for (size_t i = 0; i < _countof(m_CPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_CPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All CPU descriptor heap allocations must be released");
    }
    for (size_t i = 0; i < _countof(m_GPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_GPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All GPU descriptor heap allocations must be released");
    }
#endif

    DEV_CHECK_ERR(m_DynamicMemoryManager.GetAllocatedPageCounter() == 0, "All allocated dynamic pages must have been returned to the manager at this point.");
    m_DynamicMemoryManager.Destroy();

    for (auto& CmdListMngr : m_CmdListManagers)
        DEV_CHECK_ERR(CmdListMngr.GetAllocatorCounter() == 0, "All allocators must have been returned to the manager at this point.");
    DEV_CHECK_ERR(m_AllocatedCtxCounter == 0, "All contexts must have been released.");

    m_ContextPool.clear();
    DestroyCommandQueues();
}

void RenderDeviceD3D12Impl::DisposeCommandContext(PooledCommandContext&& Ctx)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    Ctx->Close(pAllocator);
    // Since allocator has not been used, we cmd list manager can put it directly into the free allocator list

    auto& CmdListMngr = GetCmdListManager(Ctx->GetCommandListType());
    CmdListMngr.FreeAllocator(std::move(pAllocator));
    FreeCommandContext(std::move(Ctx));
}

void RenderDeviceD3D12Impl::FreeCommandContext(PooledCommandContext&& Ctx)
{
    const auto CmdListType = Ctx->GetCommandListType();

    std::lock_guard<std::mutex> Guard{m_ContextPoolMutex};
    m_ContextPool.emplace(CmdListType, std::move(Ctx));
#ifdef DILIGENT_DEVELOPMENT
    m_AllocatedCtxCounter.fetch_add(-1);
#endif
}

void RenderDeviceD3D12Impl::CloseAndExecuteTransientCommandContext(SoftwareQueueIndex CommandQueueId, PooledCommandContext&& Ctx)
{
    auto& CmdListMngr = GetCmdListManager(CommandQueueId);
    VERIFY_EXPR(CmdListMngr.GetCommandListType() == Ctx->GetCommandListType());

    CComPtr<ID3D12CommandAllocator> pAllocator;
    ID3D12CommandList* const        pCmdList = Ctx->Close(pAllocator);
    VERIFY(pCmdList != nullptr, "Command list must not be null");
    Uint64 FenceValue = 0;
    // Execute command list directly through the queue to avoid interference with command list numbers in the queue
    LockCmdQueueAndRun(CommandQueueId,
                       [&](ICommandQueueD3D12* pCmdQueue) //
                       {
                           FenceValue = pCmdQueue->Submit(1, &pCmdList);
                       });
    CmdListMngr.ReleaseAllocator(std::move(pAllocator), CommandQueueId, FenceValue);
    FreeCommandContext(std::move(Ctx));
}

Uint64 RenderDeviceD3D12Impl::CloseAndExecuteCommandContexts(SoftwareQueueIndex                                     CommandQueueId,
                                                             Uint32                                                 NumContexts,
                                                             PooledCommandContext                                   pContexts[],
                                                             bool                                                   DiscardStaleObjects,
                                                             std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>* pSignalFences,
                                                             std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>* pWaitFences)
{
    VERIFY_EXPR(NumContexts > 0 && pContexts != 0);

    // TODO: use small_vector
    std::vector<ID3D12CommandList*>              d3d12CmdLists;
    std::vector<CComPtr<ID3D12CommandAllocator>> CmdAllocators;
    d3d12CmdLists.reserve(NumContexts);
    CmdAllocators.reserve(NumContexts);

    auto& CmdListMngr = GetCmdListManager(CommandQueueId);
    for (Uint32 i = 0; i < NumContexts; ++i)
    {
        auto& pCtx = pContexts[i];
        VERIFY_EXPR(pCtx);
        VERIFY_EXPR(CmdListMngr.GetCommandListType() == pCtx->GetCommandListType());
        CComPtr<ID3D12CommandAllocator> pAllocator;
        d3d12CmdLists.emplace_back(pCtx->Close(pAllocator));
        CmdAllocators.emplace_back(std::move(pAllocator));
    }

    Uint64 FenceValue = 0;
    {
        // Stale objects should only be discarded when submitting cmd list from
        // the immediate context, otherwise the basic requirement may be violated
        // as in the following scenario
        //
        //  Signaled        |                                        |
        //  Fence Value     |        Immediate Context               |            InitContext            |
        //                  |                                        |                                   |
        //    N             |  Draw(ResourceX)                       |                                   |
        //                  |  Release(ResourceX)                    |                                   |
        //                  |   - (ResourceX, N) -> Release Queue    |                                   |
        //                  |                                        | CopyResource()                    |
        //   N+1            |                                        | CloseAndExecuteCommandContext()   |
        //                  |                                        |                                   |
        //   N+2            |  CloseAndExecuteCommandContext()       |                                   |
        //                  |   - Cmd list is submitted with number  |                                   |
        //                  |     N+1, but resource it references    |                                   |
        //                  |     was added to the delete queue      |                                   |
        //                  |     with number N                      |                                   |
        if (pWaitFences != nullptr)
            WaitFences(CommandQueueId, *pWaitFences);
        auto SubmittedCmdBuffInfo = TRenderDeviceBase::SubmitCommandBuffer(CommandQueueId, true, NumContexts, d3d12CmdLists.data());
        FenceValue                = SubmittedCmdBuffInfo.FenceValue;
        if (pSignalFences != nullptr)
            SignalFences(CommandQueueId, *pSignalFences);
    }

    for (Uint32 i = 0; i < NumContexts; ++i)
    {
        CmdListMngr.ReleaseAllocator(std::move(CmdAllocators[i]), CommandQueueId, FenceValue);
        FreeCommandContext(std::move(pContexts[i]));
    }

    PurgeReleaseQueue(CommandQueueId);

    return FenceValue;
}

void RenderDeviceD3D12Impl::SignalFences(SoftwareQueueIndex CommandQueueId, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>& SignalFences)
{
    auto& CmdQueue = m_CommandQueues[CommandQueueId].CmdQueue;
    for (auto& val_fence : SignalFences)
    {
        auto* pFenceD3D12Impl = val_fence.second.RawPtr<FenceD3D12Impl>();
        auto* pd3d12Fence     = pFenceD3D12Impl->GetD3D12Fence();
        CmdQueue->EnqueueSignal(pd3d12Fence, val_fence.first);
        pFenceD3D12Impl->DvpSignal(val_fence.first);
    }
}

void RenderDeviceD3D12Impl::WaitFences(SoftwareQueueIndex CommandQueueId, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>& WaitFences)
{
    auto& CmdQueue = m_CommandQueues[CommandQueueId].CmdQueue;
    for (auto& val_fence : WaitFences)
    {
        auto* pFenceD3D12Impl = val_fence.second.RawPtr<FenceD3D12Impl>();
        auto* pd3d12Fence     = pFenceD3D12Impl->GetD3D12Fence();
        CmdQueue->WaitFence(pd3d12Fence, val_fence.first);
        pFenceD3D12Impl->DvpDeviceWait(val_fence.first);
    }
}

void RenderDeviceD3D12Impl::IdleGPU()
{
    IdleAllCommandQueues(true);
    ReleaseStaleResources();
}

void RenderDeviceD3D12Impl::FlushStaleResources(SoftwareQueueIndex CommandQueueId)
{
    // Submit empty command list to the queue. This will effectively signal the fence and
    // discard all resources
    TRenderDeviceBase::SubmitCommandBuffer(CommandQueueId, true, 0, nullptr);
}

void RenderDeviceD3D12Impl::ReleaseStaleResources(bool ForceRelease)
{
    PurgeReleaseQueues(ForceRelease);
}


RenderDeviceD3D12Impl::PooledCommandContext RenderDeviceD3D12Impl::AllocateCommandContext(SoftwareQueueIndex CommandQueueId, const Char* ID)
{
    auto& CmdListMngr = GetCmdListManager(CommandQueueId);
    {
        std::lock_guard<std::mutex> Guard{m_ContextPoolMutex};

        auto pool_it = m_ContextPool.find(CmdListMngr.GetCommandListType());
        if (pool_it != m_ContextPool.end())
        {
            PooledCommandContext Ctx = std::move(pool_it->second);
            m_ContextPool.erase(pool_it);
            Ctx->Reset(CmdListMngr);
            Ctx->SetID(ID);
#ifdef DILIGENT_DEVELOPMENT
            m_AllocatedCtxCounter.fetch_add(1);
#endif
            return Ctx;
        }
    }

    auto& CmdCtxAllocator = GetRawAllocator();
    auto* pRawMem         = ALLOCATE(CmdCtxAllocator, "CommandContext instance", CommandContext, 1);
    auto  pCtx            = new (pRawMem) CommandContext(CmdListMngr);
    pCtx->SetID(ID);
#ifdef DILIGENT_DEVELOPMENT
    m_AllocatedCtxCounter.fetch_add(1);
#endif
    return PooledCommandContext(pCtx, CmdCtxAllocator);
}

void RenderDeviceD3D12Impl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto DXGIFormat = TexFormatToDXGI_Format(TexFormat);

    D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = {DXGIFormat, {}, {}};

    auto hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("CheckFormatSupport() failed for format ", DXGIFormat);
        return;
    }

    TexFormatInfo.Filterable =
        ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0) ||
        ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON) != 0);

    TexFormatInfo.BindFlags = BIND_SHADER_RESOURCE;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0)
        TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0)
        TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;

    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE1D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D | RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D | RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE3D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE | RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;

    TexFormatInfo.SampleCounts = SAMPLE_COUNT_NONE;
    for (Uint32 SampleCount = 1; SampleCount <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount *= 2)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {DXGIFormat, SampleCount, {}, {}};

        hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels));
        if (SUCCEEDED(hr) && QualityLevels.NumQualityLevels > 0)
            TexFormatInfo.SampleCounts |= static_cast<SAMPLE_COUNT>(SampleCount);
    }
}

void RenderDeviceD3D12Impl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceD3D12Impl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceD3D12Impl::CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo);
}

void RenderDeviceD3D12Impl::CreateBufferFromD3DResource(ID3D12Resource* pd3d12Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, InitialState, pd3d12Buffer);
}

void RenderDeviceD3D12Impl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateBufferImpl(ppBuffer, BuffDesc, pBuffData);
}


void RenderDeviceD3D12Impl::CreateShader(const ShaderCreateInfo& ShaderCI,
                                         IShader**               ppShader,
                                         IDataBlob**             ppCompilerOutput)
{
    const ShaderD3D12Impl::CreateInfo D3D12ShaderCI{
        {
            GetDeviceInfo(),
            GetAdapterInfo(),
            GetDxCompiler(),
            ppCompilerOutput,
            m_pShaderCompilationThreadPool,
        },
        m_DeviceInfo.MaxShaderVersion.HLSL,
    };
    CreateShaderImpl(ppShader, ShaderCI, D3D12ShaderCI);
}

void RenderDeviceD3D12Impl::CreateTextureFromD3DResource(ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    TextureDesc TexDesc;
    TexDesc.Name = "Texture from d3d12 resource";
    CreateTextureImpl(ppTexture, TexDesc, InitialState, pd3d12Texture);
}

void RenderDeviceD3D12Impl::CreateTexture(const TextureDesc& TexDesc, ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, TextureD3D12Impl** ppTexture)
{
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureD3D12Impl* pTextureD3D12{NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, InitialState, pd3d12Texture)};
                           pTextureD3D12->QueryInterface(IID_TextureD3D12, reinterpret_cast<IObject**>(ppTexture));
                       });
}

void RenderDeviceD3D12Impl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateTextureImpl(ppTexture, TexDesc, pData);
}

void RenderDeviceD3D12Impl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateSamplerImpl(ppSampler, SamplerDesc);
}

void RenderDeviceD3D12Impl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateFenceImpl(ppFence, Desc);
}

void RenderDeviceD3D12Impl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateQueryImpl(ppQuery, Desc);
}

void RenderDeviceD3D12Impl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateRenderPassImpl(ppRenderPass, Desc);
}

void RenderDeviceD3D12Impl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateFramebufferImpl(ppFramebuffer, Desc);
}

void RenderDeviceD3D12Impl::CreateBLASFromD3DResource(ID3D12Resource*          pd3d12BLAS,
                                                      const BottomLevelASDesc& Desc,
                                                      RESOURCE_STATE           InitialState,
                                                      IBottomLevelAS**         ppBLAS)
{
    CreateBLASImpl(ppBLAS, Desc, InitialState, pd3d12BLAS);
}

void RenderDeviceD3D12Impl::CreateBLAS(const BottomLevelASDesc& Desc,
                                       IBottomLevelAS**         ppBLAS)
{
    CreateBLASImpl(ppBLAS, Desc);
}

void RenderDeviceD3D12Impl::CreateTLASFromD3DResource(ID3D12Resource*       pd3d12TLAS,
                                                      const TopLevelASDesc& Desc,
                                                      RESOURCE_STATE        InitialState,
                                                      ITopLevelAS**         ppTLAS)
{
    CreateTLASImpl(ppTLAS, Desc, InitialState, pd3d12TLAS);
}

void RenderDeviceD3D12Impl::CreateTLAS(const TopLevelASDesc& Desc,
                                       ITopLevelAS**         ppTLAS)
{
    CreateTLASImpl(ppTLAS, Desc);
}

void RenderDeviceD3D12Impl::CreateSBT(const ShaderBindingTableDesc& Desc,
                                      IShaderBindingTable**         ppSBT)
{
    CreateSBTImpl(ppSBT, Desc);
}

void RenderDeviceD3D12Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                            IPipelineResourceSignature**         ppSignature)
{
    CreatePipelineResourceSignature(Desc, ppSignature, SHADER_TYPE_UNKNOWN, false);
}

void RenderDeviceD3D12Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                            IPipelineResourceSignature**         ppSignature,
                                                            SHADER_TYPE                          ShaderStages,
                                                            bool                                 IsDeviceInternal)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, ShaderStages, IsDeviceInternal);
}

void RenderDeviceD3D12Impl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&              Desc,
                                                            const PipelineResourceSignatureInternalDataD3D12& InternalData,
                                                            IPipelineResourceSignature**                      ppSignature)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, InternalData);
}

DescriptorHeapAllocation RenderDeviceD3D12Impl::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Invalid heap type");
    return m_CPUDescriptorHeaps[Type].Allocate(Count);
}

DescriptorHeapAllocation RenderDeviceD3D12Impl::AllocateGPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type <= D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Invalid heap type");
    return m_GPUDescriptorHeaps[Type].Allocate(Count);
}

void RenderDeviceD3D12Impl::CreateRootSignature(const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl>* ppSignatures, Uint32 SignatureCount, size_t Hash, RootSignatureD3D12** ppRootSig)
{
    RootSignatureD3D12* pRootSigD3D12{NEW_RC_OBJ(m_RootSignatureAllocator, "RootSignatureD3D12 instance", RootSignatureD3D12)(this, ppSignatures, SignatureCount, Hash)};
    pRootSigD3D12->AddRef();
    *ppRootSig = pRootSigD3D12;
}

void RenderDeviceD3D12Impl::CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo, IDeviceMemory** ppMemory)
{
    CreateDeviceMemoryImpl(ppMemory, CreateInfo);
}

void RenderDeviceD3D12Impl::CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                     IPipelineStateCache**               ppPipelineStateCache)
{
    if (m_IsPSOCacheSupported)
        CreatePipelineStateCacheImpl(ppPipelineStateCache, CreateInfo);
    else
    {
        LOG_INFO_MESSAGE("Pipeline state cache is not supported");
        *ppPipelineStateCache = nullptr;
    }
}

SparseTextureFormatInfo RenderDeviceD3D12Impl::GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                          RESOURCE_DIMENSION Dimension,
                                                                          Uint32             SampleCount) const
{
    D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport{
        TexFormatToDXGI_Format(TexFormat), // .Format
        {},
        {}};
    if (FAILED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport))) ||
        (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_TILED) != D3D12_FORMAT_SUPPORT2_TILED)
    {
        return {};
    }

    return TRenderDeviceBase::GetSparseTextureFormatInfo(TexFormat, Dimension, SampleCount);
}

} // namespace Diligent
