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

/// \file
/// Routines that initialize D3D12-based engine implementation

#include "pch.h"

#include "EngineFactoryD3D12.h"

#include <array>
#include <string>

#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "SwapChainD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "EngineFactoryD3DBase.hpp"
#include "StringTools.hpp"
#include "EngineFactoryBase.hpp"
#include "EngineMemory.h"
#include "CommandQueueD3D12Impl.hpp"
#include "DearchiverD3D12Impl.hpp"

#include "WinHPreface.h"
#include <Windows.h>
#include <dxgi1_4.h>
#include "WinHPostface.h"

namespace Diligent
{

bool CheckAdapterD3D12Compatibility(IDXGIAdapter1*    pDXGIAdapter,
                                    D3D_FEATURE_LEVEL FeatureLevel)
{
    auto hr = D3D12CreateDevice(pDXGIAdapter, FeatureLevel, _uuidof(ID3D12Device), nullptr);
    return SUCCEEDED(hr);
}

/// Engine factory for D3D12 implementation
class EngineFactoryD3D12Impl : public EngineFactoryD3DBase<IEngineFactoryD3D12, RENDER_DEVICE_TYPE_D3D12>
{
public:
    static EngineFactoryD3D12Impl* GetInstance()
    {
        static EngineFactoryD3D12Impl TheFactory;
        return &TheFactory;
    }

    using TBase = EngineFactoryD3DBase<IEngineFactoryD3D12, RENDER_DEVICE_TYPE_D3D12>;

    EngineFactoryD3D12Impl() :
        TBase{IID_EngineFactoryD3D12}
    {}

    bool DILIGENT_CALL_TYPE LoadD3D12(const char* DllName) override final;

    virtual void DILIGENT_CALL_TYPE CreateDeviceAndContextsD3D12(const EngineD3D12CreateInfo& EngineCI,
                                                                 IRenderDevice**              ppDevice,
                                                                 IDeviceContext**             ppContexts) override final;

    virtual void DILIGENT_CALL_TYPE CreateCommandQueueD3D12(void*                pd3d12NativeDevice,
                                                            void*                pd3d12NativeCommandQueue,
                                                            IMemoryAllocator*    pRawMemAllocator,
                                                            ICommandQueueD3D12** ppCommandQueue) override final;

    virtual void DILIGENT_CALL_TYPE AttachToD3D12Device(void*                        pd3d12NativeDevice,
                                                        Uint32                       CommandQueueCount,
                                                        ICommandQueueD3D12**         ppCommandQueues,
                                                        const EngineD3D12CreateInfo& EngineCI,
                                                        IRenderDevice**              ppDevice,
                                                        IDeviceContext**             ppContexts) override final;

    virtual void DILIGENT_CALL_TYPE CreateSwapChainD3D12(IRenderDevice*            pDevice,
                                                         IDeviceContext*           pImmediateContext,
                                                         const SwapChainDesc&      SwapChainDesc,
                                                         const FullScreenModeDesc& FSDesc,
                                                         const NativeWindow&       Window,
                                                         ISwapChain**              ppSwapChain) override final;

    virtual void DILIGENT_CALL_TYPE EnumerateAdapters(Version              MinFeatureLevel,
                                                      Uint32&              NumAdapters,
                                                      GraphicsAdapterInfo* Adapters) const override final;

    virtual void DILIGENT_CALL_TYPE EnumerateDisplayModes(Version             MinFeatureLevel,
                                                          Uint32              AdapterId,
                                                          Uint32              OutputId,
                                                          TEXTURE_FORMAT      Format,
                                                          Uint32&             NumDisplayModes,
                                                          DisplayModeAttribs* DisplayModes) override final;

    virtual GraphicsAdapterInfo GetGraphicsAdapterInfo(void*          pd3dDevice,
                                                       IDXGIAdapter1* pDXIAdapter) const override final;


    virtual void DILIGENT_CALL_TYPE CreateDearchiver(const DearchiverCreateInfo& CreateInfo,
                                                     IDearchiver**               ppDearchiver) const override final
    {
        TBase::CreateDearchiver<DearchiverD3D12Impl>(CreateInfo, ppDearchiver);
    }

private:
#if USE_D3D12_LOADER
    HMODULE     m_hD3D12Dll = NULL;
    std::string m_DllName;
#endif
};

bool EngineFactoryD3D12Impl::LoadD3D12(const char* DllName)
{
#if USE_D3D12_LOADER
    if (m_hD3D12Dll == NULL)
    {
        m_hD3D12Dll = LoadD3D12Dll(DllName);
        if (m_hD3D12Dll == NULL)
        {
            LOG_ERROR_MESSAGE("Failed to load Direct3D12 DLL (", DllName, "). Check that the system supports Direct3D12 and that the dll is present on the system.");
            return false;
        }

        if (m_DllName.empty())
            m_DllName = DllName;
        else
        {
            if (StrCmpNoCase(m_DllName.c_str(), DllName) != 0)
            {
                LOG_WARNING_MESSAGE("D3D12 DLL has already been loaded as '", m_DllName,
                                    "'. New name '", DllName, "' will be ignored.");
            }
        }
    }
#endif

    return true;
}

namespace
{

void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL FeatureLevel)
{
    CComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex, adapter.Release())
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Skip software devices
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter, FeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

CComPtr<IDXGIAdapter1> DXGIAdapterFromD3D12Device(ID3D12Device* pd3d12Device)
{
    CComPtr<IDXGIFactory4> pDXIFactory;

    HRESULT hr = CreateDXGIFactory1(__uuidof(pDXIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&pDXIFactory)));
    if (SUCCEEDED(hr))
    {
        auto AdapterLUID = pd3d12Device->GetAdapterLuid();

        CComPtr<IDXGIAdapter1> pDXGIAdapter1;
        pDXIFactory->EnumAdapterByLuid(AdapterLUID, __uuidof(pDXGIAdapter1), reinterpret_cast<void**>(static_cast<IDXGIAdapter1**>(&pDXGIAdapter1)));
        return pDXGIAdapter1;
    }
    else
    {
        LOG_ERROR("Unable to create DXIFactory");
    }
    return nullptr;
}

void ValidateD3D12CreateInfo(const EngineD3D12CreateInfo& EngineCI) noexcept(false)
{
    for (Uint32 Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++Type)
    {
        auto   CPUHeapAllocSize = EngineCI.CPUDescriptorHeapAllocationSize[Type];
        Uint32 MaxSize          = 1 << 20;
        if (CPUHeapAllocSize > 1 << 20)
        {
            LOG_ERROR_AND_THROW("CPU Heap allocation size is too large (", CPUHeapAllocSize, "). Max allowed size is ", MaxSize);
        }

        if ((CPUHeapAllocSize % 16) != 0)
        {
            LOG_ERROR_AND_THROW("CPU Heap allocation size (", CPUHeapAllocSize, ") is expected to be multiple of 16");
        }
    }
}

RefCntAutoPtr<CommandQueueD3D12Impl> CreateCommandQueueD3D12(ID3D12Device*       pd3d12Device,
                                                             ID3D12CommandQueue* pd3d12Queue,
                                                             LPCWSTR             FenceName)
{

    CComPtr<ID3D12Fence> pd3d12Fence;

    auto hr = pd3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(pd3d12Fence), reinterpret_cast<void**>(static_cast<ID3D12Fence**>(&pd3d12Fence)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create command queue fence");
    hr = pd3d12Fence->SetName(FenceName);
    VERIFY_EXPR(SUCCEEDED(hr));

    return RefCntAutoPtr<CommandQueueD3D12Impl>{NEW_RC_OBJ(GetRawAllocator(), "CommandQueueD3D12 instance", CommandQueueD3D12Impl)(pd3d12Queue, pd3d12Fence)};
}

} // namespace

void EngineFactoryD3D12Impl::CreateDeviceAndContextsD3D12(const EngineD3D12CreateInfo& EngineCI,
                                                          IRenderDevice**              ppDevice,
                                                          IDeviceContext**             ppContexts)
{
    if (EngineCI.EngineAPIVersion != DILIGENT_API_VERSION)
    {
        LOG_ERROR_MESSAGE("Diligent Engine runtime (", DILIGENT_API_VERSION, ") is not compatible with the client API version (", EngineCI.EngineAPIVersion, ")");
        return;
    }

    if (!LoadD3D12(EngineCI.D3D12DllName))
        return;

    VERIFY(ppDevice && ppContexts, "Null pointer provided");
    if (!ppDevice || !ppContexts)
        return;

    *ppDevice = nullptr;
    memset(ppContexts, 0, sizeof(*ppContexts) * (size_t{std::max(1u, EngineCI.NumImmediateContexts)} + size_t{EngineCI.NumDeferredContexts}));

    std::vector<RefCntAutoPtr<CommandQueueD3D12Impl>> CmdQueueD3D12Refs;
    CComPtr<ID3D12Device>                             d3d12Device;
    std::vector<ICommandQueueD3D12*>                  CmdQueues;
    try
    {
        ValidateD3D12CreateInfo(EngineCI);
        SetRawAllocator(EngineCI.pRawMemAllocator);

        // Enable the D3D12 debug layer.
        if (EngineCI.EnableValidation)
        {
            CComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(debugController), reinterpret_cast<void**>(static_cast<ID3D12Debug**>(&debugController)))))
            {
                debugController->EnableDebugLayer();
                //static_cast<ID3D12Debug1*>(debugController.p)->SetEnableSynchronizedCommandQueueValidation(FALSE);
                if (EngineCI.D3D12ValidationFlags & D3D12_VALIDATION_FLAG_ENABLE_GPU_BASED_VALIDATION)
                {
                    CComPtr<ID3D12Debug1> debugController1;
                    debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
                    if (debugController1)
                    {
                        debugController1->SetEnableGPUBasedValidation(true);
                    }
                }
            }
        }

        CComPtr<IDXGIFactory4> factory;

        HRESULT hr = CreateDXGIFactory1(__uuidof(factory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&factory)));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create DXGI factory");

        // Direct3D12 does not allow feature levels below 11.0 (D3D12CreateDevice fails to create a device).
        const auto MinimumFeatureLevel = Version::Max(EngineCI.GraphicsAPIVersion, Version{11, 0});

        CComPtr<IDXGIAdapter1> hardwareAdapter;
        if (EngineCI.AdapterId == DEFAULT_ADAPTER_ID)
        {
            GetHardwareAdapter(factory, &hardwareAdapter, GetD3DFeatureLevel(MinimumFeatureLevel));
            if (hardwareAdapter == nullptr)
                LOG_ERROR_AND_THROW("No suitable hardware adapter found");
        }
        else
        {
            auto Adapters = FindCompatibleAdapters(MinimumFeatureLevel);
            if (EngineCI.AdapterId < Adapters.size())
                hardwareAdapter = Adapters[EngineCI.AdapterId];
            else
            {
                LOG_ERROR_AND_THROW(EngineCI.AdapterId, " is not a valid hardware adapter id. Total number of compatible adapters available on this system: ", Adapters.size());
            }
        }

        {
            DXGI_ADAPTER_DESC1 desc;
            hardwareAdapter->GetDesc1(&desc);
            LOG_INFO_MESSAGE("D3D12-capable adapter found: ", NarrowString(desc.Description), " (", desc.DedicatedVideoMemory >> 20, " MB)");
        }

        const Version FeatureLevelList[] = {{12, 1}, {12, 0}, {11, 1}, {11, 0}};
        for (auto FeatureLevel : FeatureLevelList)
        {
            auto d3dFeatureLevel = GetD3DFeatureLevel(FeatureLevel);

            hr = D3D12CreateDevice(hardwareAdapter, d3dFeatureLevel, __uuidof(d3d12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&d3d12Device)));
            if (SUCCEEDED(hr))
            {
                VERIFY_EXPR(d3d12Device);
                break;
            }
        }
        if (FAILED(hr))
        {
            LOG_WARNING_MESSAGE("Failed to create hardware device. Attempting to create WARP device");

            CComPtr<IDXGIAdapter> warpAdapter;
            hr = factory->EnumWarpAdapter(__uuidof(warpAdapter), reinterpret_cast<void**>(static_cast<IDXGIAdapter**>(&warpAdapter)));
            CHECK_D3D_RESULT_THROW(hr, "Failed to enum warp adapter");

            for (auto FeatureLevel : FeatureLevelList)
            {
                auto d3dFeatureLevel = GetD3DFeatureLevel(FeatureLevel);

                hr = D3D12CreateDevice(warpAdapter, d3dFeatureLevel, __uuidof(d3d12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&d3d12Device)));
                if (SUCCEEDED(hr))
                {
                    VERIFY_EXPR(d3d12Device);
                    break;
                }
            }
            CHECK_D3D_RESULT_THROW(hr, "Failed to create warp device");
        }

        if (EngineCI.EnableValidation)
        {
            CComPtr<ID3D12InfoQueue> pInfoQueue;
            hr = d3d12Device.QueryInterface(&pInfoQueue);
            if (SUCCEEDED(hr))
            {
                // Suppress whole categories of messages
                //D3D12_MESSAGE_CATEGORY Categories[] = {};

                // Suppress messages based on their severity level
                D3D12_MESSAGE_SEVERITY Severities[] =
                    {
                        D3D12_MESSAGE_SEVERITY_INFO //
                    };

                // Suppress individual messages by their ID
                D3D12_MESSAGE_ID DenyIds[] =
                    {
                        // D3D12 WARNING: ID3D12CommandList::ClearRenderTargetView: The clear values do not match those passed to resource creation.
                        // The clear operation is typically slower as a result; but will still clear to the desired value.
                        // [ EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]
                        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,

                        // D3D12 WARNING: ID3D12CommandList::ClearDepthStencilView: The clear values do not match those passed to resource creation.
                        // The clear operation is typically slower as a result; but will still clear to the desired value.
                        // [ EXECUTION WARNING #821: CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE]
                        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE //
                    };

                D3D12_INFO_QUEUE_FILTER NewFilter = {};
                //NewFilter.DenyList.NumCategories = _countof(Categories);
                //NewFilter.DenyList.pCategoryList = Categories;
                NewFilter.DenyList.NumSeverities = _countof(Severities);
                NewFilter.DenyList.pSeverityList = Severities;
                NewFilter.DenyList.NumIDs        = _countof(DenyIds);
                NewFilter.DenyList.pIDList       = DenyIds;

                hr = pInfoQueue->PushStorageFilter(&NewFilter);
                VERIFY(SUCCEEDED(hr), "Failed to push storage filter");

                if (EngineCI.D3D12ValidationFlags & D3D12_VALIDATION_FLAG_BREAK_ON_CORRUPTION)
                {
                    hr = pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                    VERIFY(SUCCEEDED(hr), "Failed to set break on corruption");
                }

                if (EngineCI.D3D12ValidationFlags & D3D12_VALIDATION_FLAG_BREAK_ON_ERROR)
                {
                    hr = pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                    VERIFY(SUCCEEDED(hr), "Failed to set break on error");
                }
            }
        }

#ifndef RELEASE
        // Prevent the GPU from overclocking or underclocking to get consistent timings
        //d3d12Device->SetStablePowerState(TRUE);
#endif

        {
            auto       pDXGIAdapter1 = DXGIAdapterFromD3D12Device(d3d12Device);
            const auto AdapterInfo   = GetGraphicsAdapterInfo(d3d12Device, pDXGIAdapter1);
            VerifyEngineCreateInfo(EngineCI, AdapterInfo);
        }

        // Describe and create the command queue.
        const auto CreateQueue = [&](const ImmediateContextCreateInfo& ContextCI) //
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Priority = QueuePriorityToD3D12QueuePriority(ContextCI.Priority);
            queueDesc.Type     = QueueIdToD3D12CommandListType(HardwareQueueIndex{ContextCI.QueueId});

            CComPtr<ID3D12CommandQueue> pd3d12CmdQueue;
            hr = d3d12Device->CreateCommandQueue(&queueDesc, __uuidof(pd3d12CmdQueue), reinterpret_cast<void**>(static_cast<ID3D12CommandQueue**>(&pd3d12CmdQueue)));
            CHECK_D3D_RESULT_THROW(hr, "Failed to create command queue");
            hr = pd3d12CmdQueue->SetName(WidenString(ContextCI.Name).c_str());
            VERIFY_EXPR(SUCCEEDED(hr));

            auto pCmdQueueD3D12 = Diligent::CreateCommandQueueD3D12(d3d12Device, pd3d12CmdQueue, (WidenString(ContextCI.Name) + L" Fence").c_str());
            CmdQueueD3D12Refs.push_back(pCmdQueueD3D12);
            CmdQueues.push_back(pCmdQueueD3D12);
        };

        if (EngineCI.NumImmediateContexts > 0)
        {
            VERIFY(EngineCI.pImmediateContextInfo != nullptr, "Must have been caught by VerifyEngineCreateInfo()");
            for (Uint32 CtxInd = 0; CtxInd < EngineCI.NumImmediateContexts; ++CtxInd)
                CreateQueue(EngineCI.pImmediateContextInfo[CtxInd]);
        }
        else
        {
            ImmediateContextCreateInfo DefaultContext;
            DefaultContext.Name    = "Default immediate context";
            DefaultContext.QueueId = 0;

            CreateQueue(DefaultContext);
        }
    }
    catch (const std::runtime_error&)
    {
        LOG_ERROR("Failed to initialize D3D12 resources");
        return;
    }

    AttachToD3D12Device(d3d12Device, static_cast<Uint32>(CmdQueues.size()), CmdQueues.data(), EngineCI, ppDevice, ppContexts);
}


void EngineFactoryD3D12Impl::CreateCommandQueueD3D12(void*                pd3d12NativeDevice,
                                                     void*                pd3d12NativeCommandQueue,
                                                     IMemoryAllocator*    pRawMemAllocator,
                                                     ICommandQueueD3D12** ppCommandQueue)
{
    VERIFY(pd3d12NativeDevice && pd3d12NativeCommandQueue && ppCommandQueue, "Null pointer provided");
    if (!pd3d12NativeDevice || !pd3d12NativeCommandQueue || !ppCommandQueue)
        return;

    *ppCommandQueue = nullptr;

    try
    {
        SetRawAllocator(pRawMemAllocator);
        auto* d3d12Device   = reinterpret_cast<ID3D12Device*>(pd3d12NativeDevice);
        auto* d3d12CmdQueue = reinterpret_cast<ID3D12CommandQueue*>(pd3d12NativeCommandQueue);

        auto pCmdQueueD3D12 = Diligent::CreateCommandQueueD3D12(d3d12Device, d3d12CmdQueue, L"Fence for user-provided command queue");
        *ppCommandQueue     = pCmdQueueD3D12.Detach();
    }
    catch (const std::runtime_error&)
    {
        LOG_ERROR("Failed to initialize D3D12 resources");
        return;
    }
}

void EngineFactoryD3D12Impl::AttachToD3D12Device(void*                        pd3d12NativeDevice,
                                                 const Uint32                 CommandQueueCount,
                                                 ICommandQueueD3D12**         ppCommandQueues,
                                                 const EngineD3D12CreateInfo& EngineCI,
                                                 IRenderDevice**              ppDevice,
                                                 IDeviceContext**             ppContexts)
{
    if (EngineCI.EngineAPIVersion != DILIGENT_API_VERSION)
    {
        LOG_ERROR_MESSAGE("Diligent Engine runtime (", DILIGENT_API_VERSION, ") is not compatible with the client API version (", EngineCI.EngineAPIVersion, ")");
        return;
    }

    if (!LoadD3D12(EngineCI.D3D12DllName))
        return;

    VERIFY(pd3d12NativeDevice && ppCommandQueues && ppDevice && ppContexts, "Null pointer provided");
    if (!pd3d12NativeDevice || !ppCommandQueues || !ppDevice || !ppContexts)
        return;

    ImmediateContextCreateInfo DefaultImmediateCtxCI;

    const auto        NumImmediateContexts  = EngineCI.NumImmediateContexts > 0 ? EngineCI.NumImmediateContexts : 1;
    const auto* const pImmediateContextInfo = EngineCI.NumImmediateContexts > 0 ? EngineCI.pImmediateContextInfo : &DefaultImmediateCtxCI;

    VERIFY_EXPR(NumImmediateContexts == CommandQueueCount);

    *ppDevice = nullptr;
    memset(ppContexts, 0, sizeof(*ppContexts) * (size_t{CommandQueueCount} + size_t{EngineCI.NumDeferredContexts}));

    if (EngineCI.NumImmediateContexts > 0)
    {
        if (CommandQueueCount != EngineCI.NumImmediateContexts)
        {
            LOG_ERROR_MESSAGE("EngineCI.NumImmediateContexts (", EngineCI.NumImmediateContexts, ") must be the same as CommandQueueCount (", CommandQueueCount, ") or zero.");
            return;
        }
        for (Uint32 q = 0; q < CommandQueueCount; ++q)
        {
            auto Desc        = ppCommandQueues[q]->GetD3D12CommandQueue()->GetDesc();
            auto CmdListType = QueueIdToD3D12CommandListType(HardwareQueueIndex{pImmediateContextInfo[q].QueueId});

            if (Desc.Type != CmdListType)
            {
                LOG_ERROR_MESSAGE("ppCommandQueues[", q, "] has type ", GetCommandQueueTypeString(D3D12CommandListTypeToCmdQueueType(Desc.Type)),
                                  ", but EngineCI.pImmediateContextInfo[", q, "] has incompatible type ", GetCommandQueueTypeString(D3D12CommandListTypeToCmdQueueType(CmdListType)), ".");
                return;
            }
        }
    }

    try
    {
        SetRawAllocator(EngineCI.pRawMemAllocator);
        auto& RawMemAllocator = GetRawAllocator();
        auto  d3d12Device     = reinterpret_cast<ID3D12Device*>(pd3d12NativeDevice);
        auto  pDXGIAdapter1   = DXGIAdapterFromD3D12Device(d3d12Device);

        ValidateD3D12CreateInfo(EngineCI);

        const auto AdapterInfo = GetGraphicsAdapterInfo(pd3d12NativeDevice, pDXGIAdapter1);
        VerifyEngineCreateInfo(EngineCI, AdapterInfo);

        RenderDeviceD3D12Impl* pRenderDeviceD3D12{
            NEW_RC_OBJ(RawMemAllocator, "RenderDeviceD3D12Impl instance", RenderDeviceD3D12Impl)(RawMemAllocator, this, EngineCI, AdapterInfo, d3d12Device, CommandQueueCount, ppCommandQueues)};
        pRenderDeviceD3D12->QueryInterface(IID_RenderDevice, reinterpret_cast<IObject**>(ppDevice));

        for (Uint32 CtxInd = 0; CtxInd < NumImmediateContexts; ++CtxInd)
        {
            const auto d3d12CmdListType = ppCommandQueues[CtxInd]->GetD3D12CommandQueueDesc().Type;
            const auto QueueId          = D3D12CommandListTypeToQueueId(d3d12CmdListType);

            RefCntAutoPtr<DeviceContextD3D12Impl> pImmediateCtxD3D12{
                NEW_RC_OBJ(RawMemAllocator, "DeviceContextD3D12Impl instance", DeviceContextD3D12Impl)(
                    pRenderDeviceD3D12,
                    EngineCI,
                    DeviceContextDesc{
                        pImmediateContextInfo[CtxInd].Name,
                        AdapterInfo.Queues[QueueId].QueueType,
                        false,   // IsDeferred
                        CtxInd,  // Context index
                        QueueId} //
                    )};
            // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceD3D12 will
            // keep a weak reference to the context
            pImmediateCtxD3D12->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppContexts + CtxInd));
            pRenderDeviceD3D12->SetImmediateContext(CtxInd, pImmediateCtxD3D12);
        }

        for (Uint32 DeferredCtx = 0; DeferredCtx < EngineCI.NumDeferredContexts; ++DeferredCtx)
        {
            RefCntAutoPtr<DeviceContextD3D12Impl> pDeferredCtxD3D12{
                NEW_RC_OBJ(RawMemAllocator, "DeviceContextD3D12Impl instance", DeviceContextD3D12Impl)(
                    pRenderDeviceD3D12,
                    EngineCI,
                    DeviceContextDesc{
                        nullptr,
                        COMMAND_QUEUE_TYPE_UNKNOWN,
                        true,                               // IsDeferred
                        NumImmediateContexts + DeferredCtx, // Context index
                    }                                       //
                    )};
            // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceD3D12 will
            // keep a weak reference to the context
            pDeferredCtxD3D12->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppContexts + NumImmediateContexts + DeferredCtx));
            pRenderDeviceD3D12->SetDeferredContext(DeferredCtx, pDeferredCtxD3D12);
        }
    }
    catch (const std::runtime_error&)
    {
        if (*ppDevice)
        {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
        }
        for (Uint32 ctx = 0; ctx < NumImmediateContexts + EngineCI.NumDeferredContexts; ++ctx)
        {
            if (ppContexts[ctx] != nullptr)
            {
                ppContexts[ctx]->Release();
                ppContexts[ctx] = nullptr;
            }
        }

        LOG_ERROR("Failed to create device and contexts");
    }
}


void EngineFactoryD3D12Impl::CreateSwapChainD3D12(IRenderDevice*            pDevice,
                                                  IDeviceContext*           pImmediateContext,
                                                  const SwapChainDesc&      SCDesc,
                                                  const FullScreenModeDesc& FSDesc,
                                                  const NativeWindow&       Window,
                                                  ISwapChain**              ppSwapChain)
{
    VERIFY(ppSwapChain, "Null pointer provided");
    if (!ppSwapChain)
        return;

    *ppSwapChain = nullptr;

    try
    {
        auto* pDeviceD3D12        = ClassPtrCast<RenderDeviceD3D12Impl>(pDevice);
        auto* pDeviceContextD3D12 = ClassPtrCast<DeviceContextD3D12Impl>(pImmediateContext);
        auto& RawMemAllocator     = GetRawAllocator();

        auto* pSwapChainD3D12 = NEW_RC_OBJ(RawMemAllocator, "SwapChainD3D12Impl instance", SwapChainD3D12Impl)(SCDesc, FSDesc, pDeviceD3D12, pDeviceContextD3D12, Window);
        pSwapChainD3D12->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
    }
    catch (const std::runtime_error&)
    {
        if (*ppSwapChain)
        {
            (*ppSwapChain)->Release();
            *ppSwapChain = nullptr;
        }

        LOG_ERROR("Failed to create the swap chain");
    }
}

void EngineFactoryD3D12Impl::EnumerateAdapters(Version              MinFeatureLevel,
                                               Uint32&              NumAdapters,
                                               GraphicsAdapterInfo* Adapters) const
{
#if USE_D3D12_LOADER
    if (m_hD3D12Dll == NULL)
    {
        LOG_ERROR_MESSAGE("D3D12 has not been loaded. Please use IEngineFactoryD3D12::LoadD3D12() to load the library and entry points.");
        return;
    }
#endif
    TBase::EnumerateAdapters(MinFeatureLevel, NumAdapters, Adapters);
}

void EngineFactoryD3D12Impl::EnumerateDisplayModes(Version             MinFeatureLevel,
                                                   Uint32              AdapterId,
                                                   Uint32              OutputId,
                                                   TEXTURE_FORMAT      Format,
                                                   Uint32&             NumDisplayModes,
                                                   DisplayModeAttribs* DisplayModes)
{
#if USE_D3D12_LOADER
    if (m_hD3D12Dll == NULL)
    {
        LOG_ERROR_MESSAGE("D3D12 has not been loaded. Please use IEngineFactoryD3D12::LoadD3D12() to load the library and entry points.");
        return;
    }
#endif
    TBase::EnumerateDisplayModes(MinFeatureLevel, AdapterId, OutputId, Format, NumDisplayModes, DisplayModes);
}


GraphicsAdapterInfo EngineFactoryD3D12Impl::GetGraphicsAdapterInfo(void*          pd3dDevice,
                                                                   IDXGIAdapter1* pDXIAdapter) const
{
    auto AdapterInfo = TBase::GetGraphicsAdapterInfo(pd3dDevice, pDXIAdapter);

    CComPtr<ID3D12Device> d3d12Device{reinterpret_cast<ID3D12Device*>(pd3dDevice)};
    if (!d3d12Device)
    {
        const Version FeatureLevelList[] = {{12, 1}, {12, 0}, {11, 1}, {11, 0}};
        for (auto FeatureLevel : FeatureLevelList)
        {
            auto d3dFeatureLevel = GetD3DFeatureLevel(FeatureLevel);

            auto hr = D3D12CreateDevice(pDXIAdapter, d3dFeatureLevel, __uuidof(d3d12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&d3d12Device)));
            if (SUCCEEDED(hr))
            {
                VERIFY_EXPR(d3d12Device);
                break;
            }
        }
    }

    {
        D3D12_FEATURE_DATA_ARCHITECTURE DataArch = {};
        if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &DataArch, sizeof(DataArch))))
        {
            if (AdapterInfo.Type != ADAPTER_TYPE_SOFTWARE && (DataArch.UMA || DataArch.CacheCoherentUMA))
                AdapterInfo.Type = ADAPTER_TYPE_INTEGRATED;
        }
    }

    // Set queue info
    {
        AdapterInfo.NumQueues = 3;
        for (Uint32 q = 0; q < AdapterInfo.NumQueues; ++q)
        {
            auto& Queue                     = AdapterInfo.Queues[q];
            Queue.QueueType                 = D3D12CommandListTypeToCmdQueueType(QueueIdToD3D12CommandListType(HardwareQueueIndex{q}));
            Queue.MaxDeviceContexts         = 0xFF;
            Queue.TextureCopyGranularity[0] = 1;
            Queue.TextureCopyGranularity[1] = 1;
            Queue.TextureCopyGranularity[2] = 1;
        }
    }

    // Enable features and set properties
    {
        auto& Features = AdapterInfo.Features;

        // Direct3D12 supports shader model 5.1 on all feature levels (even on 11.0),
        // so bindless resources are always available.
        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
        Features.BindlessResources = DEVICE_FEATURE_STATE_ENABLED;

        Features.VertexPipelineUAVWritesAndAtomics = DEVICE_FEATURE_STATE_ENABLED;
        Features.NativeFence                       = DEVICE_FEATURE_STATE_OPTIONAL; // can be disabled
        Features.TextureComponentSwizzle           = DEVICE_FEATURE_STATE_ENABLED;

        // Check if mesh shader is supported.
        bool MeshShadersSupported = false;
#ifdef D3D12_H_HAS_MESH_SHADER
        {
            D3D12_FEATURE_DATA_SHADER_MODEL ShaderModel = {static_cast<D3D_SHADER_MODEL>(0x65)};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModel, sizeof(ShaderModel))))
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS7 FeatureData = {};

                MeshShadersSupported =
                    SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &FeatureData, sizeof(FeatureData))) &&
                    FeatureData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
            }
        }
#endif

        if (MeshShadersSupported)
        {
            Features.MeshShaders = DEVICE_FEATURE_STATE_ENABLED;

            auto& MeshProps{AdapterInfo.MeshShader};
            MeshProps.MaxThreadGroupCountX     = 65536; // from specs: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#dispatchmesh-api
            MeshProps.MaxThreadGroupCountY     = 65536;
            MeshProps.MaxThreadGroupCountZ     = 65536;
            MeshProps.MaxThreadGroupTotalCount = 1u << 22u;
            ASSERT_SIZEOF(MeshProps, 16, "Did you add a new member to MeshShaderProperties? Please initialize it here.");
        }

        Features.ShaderResourceRuntimeArrays = DEVICE_FEATURE_STATE_ENABLED;

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Features = {};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Features, sizeof(d3d12Features))))
            {
                if (d3d12Features.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
                {
                    Features.ShaderFloat16 = DEVICE_FEATURE_STATE_ENABLED;
                }

                if (d3d12Features.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_1)
                {
                    NVApiLoader NVApi;
                    if (AdapterInfo.Vendor == ADAPTER_VENDOR_NVIDIA)
                        NVApi.Load();

                    Features.SparseResources = DEVICE_FEATURE_STATE_ENABLED;

                    auto& SparseRes{AdapterInfo.SparseResources};
                    SparseRes.StandardBlockSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

                    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT d3d12Address = {};
                    if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &d3d12Address, sizeof(d3d12Address))))
                    {
                        SparseRes.AddressSpaceSize  = Uint64{1} << d3d12Address.MaxGPUVirtualAddressBitsPerProcess;
                        SparseRes.ResourceSpaceSize = Uint64{1} << d3d12Address.MaxGPUVirtualAddressBitsPerResource;
                    }
                    else
                    {
                        SparseRes.AddressSpaceSize  = Uint64{1} << d3d12Features.MaxGPUVirtualAddressBitsPerResource;
                        SparseRes.ResourceSpaceSize = Uint64{1} << d3d12Features.MaxGPUVirtualAddressBitsPerResource;
                    }

                    SparseRes.CapFlags =
                        SPARSE_RESOURCE_CAP_FLAG_BUFFER |
                        SPARSE_RESOURCE_CAP_FLAG_BUFFER_STANDARD_BLOCK |
                        SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D |
                        SPARSE_RESOURCE_CAP_FLAG_STANDARD_2D_TILE_SHAPE |
                        SPARSE_RESOURCE_CAP_FLAG_ALIASED |
                        SPARSE_RESOURCE_CAP_FLAG_NON_RESIDENT_SAFE;

                    // No 2, 8 or 16 sample multisample antialiasing (MSAA) support. Only 4x is required, except no 128 bpp formats.
                    SparseRes.CapFlags |=
                        SPARSE_RESOURCE_CAP_FLAG_TEXTURE_4_SAMPLES |
                        SPARSE_RESOURCE_CAP_FLAG_STANDARD_2DMS_TILE_SHAPE;

                    if (d3d12Features.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2)
                    {
                        SparseRes.CapFlags |=
                            SPARSE_RESOURCE_CAP_FLAG_SHADER_RESOURCE_RESIDENCY |
                            SPARSE_RESOURCE_CAP_FLAG_NON_RESIDENT_STRICT;
                    }
                    if (d3d12Features.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_3)
                    {
                        SparseRes.CapFlags |=
                            SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D |
                            SPARSE_RESOURCE_CAP_FLAG_STANDARD_3D_TILE_SHAPE;
                    }
                    if (NVApi)
                    {
                        SparseRes.CapFlags |= SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL;
                    }
                    if (d3d12Features.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2)
                    {
                        SparseRes.CapFlags |= SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT;
                    }

                    // Some features are not correctly working in software renderer.
                    if (AdapterInfo.Type == ADAPTER_TYPE_SOFTWARE)
                    {
                        // Reading from null-mapped tile doesn't return zero
                        SparseRes.CapFlags &= ~SPARSE_RESOURCE_CAP_FLAG_NON_RESIDENT_STRICT;
                        // CheckAccessFullyMapped() in shader doesn't work.
                        SparseRes.CapFlags &= ~SPARSE_RESOURCE_CAP_FLAG_SHADER_RESOURCE_RESIDENCY;
                        // Mip tails are not supported at all.
                        SparseRes.CapFlags &= ~SPARSE_RESOURCE_CAP_FLAG_ALIGNED_MIP_SIZE;
                    }

                    SparseRes.BufferBindFlags =
                        BIND_VERTEX_BUFFER |
                        BIND_INDEX_BUFFER |
                        BIND_UNIFORM_BUFFER |
                        BIND_SHADER_RESOURCE |
                        BIND_UNORDERED_ACCESS |
                        BIND_INDIRECT_DRAW_ARGS |
                        BIND_RAY_TRACING;

                    for (Uint32 q = 0; q < AdapterInfo.NumQueues; ++q)
                        AdapterInfo.Queues[q].QueueType |= COMMAND_QUEUE_TYPE_SPARSE_BINDING;

                    ASSERT_SIZEOF(SparseRes, 32, "Did you add a new member to SparseResourceProperties? Please initialize it here.");
                }
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12Features1 = {};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12Features1, sizeof(d3d12Features1))))
            {
                if (d3d12Features1.WaveOps != FALSE)
                {
                    Features.WaveOp = DEVICE_FEATURE_STATE_ENABLED;

                    auto& WaveOpProps{AdapterInfo.WaveOp};
                    WaveOpProps.MinSize         = d3d12Features1.WaveLaneCountMin;
                    WaveOpProps.MaxSize         = d3d12Features1.WaveLaneCountMax;
                    WaveOpProps.SupportedStages = SHADER_TYPE_PIXEL | SHADER_TYPE_COMPUTE;
                    WaveOpProps.Features        = WAVE_FEATURE_BASIC | WAVE_FEATURE_VOTE | WAVE_FEATURE_ARITHMETIC | WAVE_FEATURE_BALLOUT | WAVE_FEATURE_QUAD;
                    if (MeshShadersSupported)
                        WaveOpProps.SupportedStages |= SHADER_TYPE_AMPLIFICATION | SHADER_TYPE_MESH;

                    ASSERT_SIZEOF(WaveOpProps, 16, "Did you add a new member to WaveOpProperties? Please initialize it here.");
                }
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS3 d3d12Features3 = {};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &d3d12Features3, sizeof(d3d12Features3))))
            {
                if (d3d12Features3.CopyQueueTimestampQueriesSupported)
                    Features.TransferQueueTimestampQueries = DEVICE_FEATURE_STATE_ENABLED;
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12Features4{};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12Features4, sizeof(d3d12Features4))))
            {
                if (d3d12Features4.Native16BitShaderOpsSupported)
                {
                    Features.ResourceBuffer16BitAccess = DEVICE_FEATURE_STATE_ENABLED;
                    Features.UniformBuffer16BitAccess  = DEVICE_FEATURE_STATE_ENABLED;
                    Features.ShaderInputOutput16       = DEVICE_FEATURE_STATE_ENABLED;
                }
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12Features5{};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12Features5, sizeof(d3d12Features5))))
            {
                auto& RayTracingProps{AdapterInfo.RayTracing};
                if (d3d12Features5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
                {
                    Features.RayTracing = DEVICE_FEATURE_STATE_ENABLED;

                    RayTracingProps.MaxRecursionDepth        = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
                    RayTracingProps.ShaderGroupHandleSize    = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    RayTracingProps.MaxShaderRecordStride    = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
                    RayTracingProps.ShaderGroupBaseAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
                    RayTracingProps.MaxRayGenThreads         = D3D12_RAYTRACING_MAX_RAY_GENERATION_SHADER_THREADS;
                    RayTracingProps.MaxInstancesPerTLAS      = D3D12_RAYTRACING_MAX_INSTANCES_PER_TOP_LEVEL_ACCELERATION_STRUCTURE;
                    RayTracingProps.MaxPrimitivesPerBLAS     = D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
                    RayTracingProps.MaxGeometriesPerBLAS     = D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
                    RayTracingProps.VertexBufferAlignment    = 1;
                    RayTracingProps.IndexBufferAlignment     = 1;
                    RayTracingProps.TransformBufferAlignment = D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT;
                    RayTracingProps.BoxBufferAlignment       = D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT;
                    RayTracingProps.ScratchBufferAlignment   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
                    RayTracingProps.InstanceBufferAlignment  = D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT;
                    RayTracingProps.CapFlags |= RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS;
                }
                if (d3d12Features5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
                {
                    RayTracingProps.CapFlags |= RAY_TRACING_CAP_FLAG_INLINE_RAY_TRACING | RAY_TRACING_CAP_FLAG_INDIRECT_RAY_TRACING;
                }
                ASSERT_SIZEOF(RayTracingProps, 60, "Did you add a new member to RayTracingProperites? Please initialize it here.");
            }

#ifdef NTDDI_WIN10_19H1
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12Features6{};
            if (SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12Features6, sizeof(d3d12Features6))))
            {
                // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
                auto& ShadingRateProps{AdapterInfo.ShadingRate};
                auto  AddShadingRate = [&ShadingRateProps](SHADING_RATE Rate, SAMPLE_COUNT SampleBits) {
                    VERIFY_EXPR(ShadingRateProps.NumShadingRates < DILIGENT_MAX_SHADING_RATES);
                    ShadingRateProps.ShadingRates[ShadingRateProps.NumShadingRates++] = {Rate, SampleBits};
                };
                if (d3d12Features6.AdditionalShadingRatesSupported != FALSE)
                {
                    AddShadingRate(SHADING_RATE_4X4, SAMPLE_COUNT_1);
                    AddShadingRate(SHADING_RATE_4X2, SAMPLE_COUNT_1 | SAMPLE_COUNT_2);
                    AddShadingRate(SHADING_RATE_2X4, SAMPLE_COUNT_1 | SAMPLE_COUNT_2);
                }
                if (d3d12Features6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
                {
                    Features.VariableRateShading = DEVICE_FEATURE_STATE_ENABLED;

                    ShadingRateProps.Format = SHADING_RATE_FORMAT_PALETTE;
                    ShadingRateProps.Combiners |= SHADING_RATE_COMBINER_PASSTHROUGH;
                    ShadingRateProps.CapFlags |= SHADING_RATE_CAP_FLAG_PER_DRAW;

                    // 1x1, 1x2, 2x1, 2x2 are always supported
                    AddShadingRate(SHADING_RATE_2X2, SAMPLE_COUNT_1 | SAMPLE_COUNT_2 | SAMPLE_COUNT_4);
                    AddShadingRate(SHADING_RATE_2X1, SAMPLE_COUNT_1 | SAMPLE_COUNT_2 | SAMPLE_COUNT_4);
                    AddShadingRate(SHADING_RATE_1X2, SAMPLE_COUNT_1 | SAMPLE_COUNT_2 | SAMPLE_COUNT_4);
                    AddShadingRate(SHADING_RATE_1X1, SAMPLE_COUNT_ALL);
                }
                if (d3d12Features6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
                {
                    ShadingRateProps.CapFlags |=
                        SHADING_RATE_CAP_FLAG_PER_PRIMITIVE |
                        SHADING_RATE_CAP_FLAG_TEXTURE_BASED |
                        SHADING_RATE_CAP_FLAG_NON_SUBSAMPLED_RENDER_TARGET |
                        SHADING_RATE_CAP_FLAG_SAMPLE_MASK |
                        SHADING_RATE_CAP_FLAG_SHADER_SAMPLE_MASK |
                        SHADING_RATE_CAP_FLAG_SHADING_RATE_SHADER_INPUT;
                    ShadingRateProps.MinTileSize[0] = d3d12Features6.ShadingRateImageTileSize;
                    ShadingRateProps.MinTileSize[1] = d3d12Features6.ShadingRateImageTileSize;
                    ShadingRateProps.MaxTileSize[0] = d3d12Features6.ShadingRateImageTileSize;
                    ShadingRateProps.MaxTileSize[1] = d3d12Features6.ShadingRateImageTileSize;
                    ShadingRateProps.Combiners |=
                        SHADING_RATE_COMBINER_OVERRIDE |
                        SHADING_RATE_COMBINER_MIN |
                        SHADING_RATE_COMBINER_MAX |
                        SHADING_RATE_COMBINER_SUM;
                    ShadingRateProps.BindFlags                = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_SHADING_RATE;
                    ShadingRateProps.ShadingRateTextureAccess = SHADING_RATE_TEXTURE_ACCESS_ON_GPU;
                }
                if (d3d12Features6.PerPrimitiveShadingRateSupportedWithViewportIndexing != FALSE)
                {
                    ShadingRateProps.CapFlags |= SHADING_RATE_CAP_FLAG_PER_PRIMITIVE_WITH_MULTIPLE_VIEWPORTS;
                }
                // Export of depth and stencil is not supported
                // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#export-of-depth-and-stencil

                // TODO: add support for D3D12_FEATURE_DATA_D3D12_OPTIONS10

                ASSERT_SIZEOF(ShadingRateProps, 52, "Did you add a new member to ShadingRateProperties? Please initialize it here.");
            }
#endif // NTDDI_WIN10_19H1
        }

        // Buffer properties
        {
            auto& BufferProps{AdapterInfo.Buffer};
            BufferProps.ConstantBufferOffsetAlignment   = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
            BufferProps.StructuredBufferOffsetAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
            ASSERT_SIZEOF(BufferProps, 8, "Did you add a new member to BufferProperites? Please initialize it here.");
        }
    }

    // Texture properties
    {
        auto& TexProps{AdapterInfo.Texture};
        TexProps.MaxTexture1DDimension      = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        TexProps.MaxTexture1DArraySlices    = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
        TexProps.MaxTexture2DDimension      = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        TexProps.MaxTexture2DArraySlices    = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        TexProps.MaxTexture3DDimension      = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        TexProps.MaxTextureCubeDimension    = D3D12_REQ_TEXTURECUBE_DIMENSION;
        TexProps.Texture2DMSSupported       = True;
        TexProps.Texture2DMSArraySupported  = True;
        TexProps.TextureViewSupported       = True;
        TexProps.CubemapArraysSupported     = True;
        TexProps.TextureView2DOn3DSupported = True;
        ASSERT_SIZEOF(TexProps, 32, "Did you add a new member to TextureProperites? Please initialize it here.");
    }

    // Sampler properties
    {

        auto& SamProps{AdapterInfo.Sampler};
        SamProps.BorderSamplingModeSupported = True;
        SamProps.MaxAnisotropy               = D3D12_DEFAULT_MAX_ANISOTROPY;
        SamProps.LODBiasSupported            = True;
        ASSERT_SIZEOF(SamProps, 3, "Did you add a new member to SamplerProperites? Please initialize it here.");
    }

    // Compute shader properties
    {
        auto& CompProps{AdapterInfo.ComputeShader};
        CompProps.SharedMemorySize          = 32u << 10;
        CompProps.MaxThreadGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        CompProps.MaxThreadGroupSizeX       = D3D12_CS_THREAD_GROUP_MAX_X;
        CompProps.MaxThreadGroupSizeY       = D3D12_CS_THREAD_GROUP_MAX_Y;
        CompProps.MaxThreadGroupSizeZ       = D3D12_CS_THREAD_GROUP_MAX_Z;
        CompProps.MaxThreadGroupCountX      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        CompProps.MaxThreadGroupCountY      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        CompProps.MaxThreadGroupCountZ      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        ASSERT_SIZEOF(CompProps, 32, "Did you add a new member to ComputeShaderProperties? Please initialize it here.");
    }

    // Draw command properties
    {
        auto& DrawCommandProps{AdapterInfo.DrawCommand};
#if D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP >= 32
        DrawCommandProps.MaxIndexValue = ~0u;
#else
        DrawCommandProps.MaxIndexValue = 1u << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP;
#endif
        DrawCommandProps.CapFlags |=
            DRAW_COMMAND_CAP_FLAG_BASE_VERTEX |
            DRAW_COMMAND_CAP_FLAG_NATIVE_MULTI_DRAW_INDIRECT |
            DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER;
        ASSERT_SIZEOF(DrawCommandProps, 12, "Did you add a new member to DrawCommandProperties? Please initialize it here.");
    }

    ASSERT_SIZEOF(DeviceFeatures, 46, "Did you add a new feature to DeviceFeatures? Please handle its status here.");

    return AdapterInfo;
}

#ifdef DOXYGEN
/// Loads Direct3D12-based engine implementation and exports factory functions
///
/// \return - Pointer to the function that returns factory for D3D12 engine implementation.
///           See Duiligent::EngineFactoryD3D12Impl.
///
/// \remarks Depending on the configuration and platform, the function loads different dll:
///
/// Platform\\Configuration   |           Debug               |        Release
/// --------------------------|-------------------------------|----------------------------
///         x86               | GraphicsEngineD3D12_32d.dll   |    GraphicsEngineD3D12_32r.dll
///         x64               | GraphicsEngineD3D12_64d.dll   |    GraphicsEngineD3D12_64r.dll
///
GetEngineFactoryD3D12Type LoadGraphicsEngineD3D12()
{
// This function is only required because DoxyGen refuses to generate documentation for a static function when SHOW_FILES==NO
#    error This function must never be compiled;
}
#endif


IEngineFactoryD3D12* GetEngineFactoryD3D12()
{
    return EngineFactoryD3D12Impl::GetInstance();
}

} // namespace Diligent

extern "C"
{
    Diligent::IEngineFactoryD3D12* Diligent_GetEngineFactoryD3D12()
    {
        return Diligent::GetEngineFactoryD3D12();
    }
}
