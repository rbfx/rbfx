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

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <atomic>

#include "GPUTestingEnvironment.hpp"
#include "PlatformDebug.hpp"
#include "TestingSwapChainBase.hpp"
#include "StringTools.hpp"
#include "GraphicsAccessories.hpp"

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#endif

#if METAL_SUPPORTED
#    include "EngineFactoryMtl.h"
#endif

#if ARCHIVER_SUPPORTED
#    include "ArchiverFactoryLoader.h"
#endif


namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
GPUTestingEnvironment* CreateTestingEnvironmentD3D11(const GPUTestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if D3D12_SUPPORTED
GPUTestingEnvironment* CreateTestingEnvironmentD3D12(const GPUTestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
GPUTestingEnvironment* CreateTestingEnvironmentGL(const GPUTestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if VULKAN_SUPPORTED
GPUTestingEnvironment* CreateTestingEnvironmentVk(const GPUTestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if METAL_SUPPORTED
GPUTestingEnvironment* CreateTestingEnvironmentMtl(const GPUTestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

Uint32 GPUTestingEnvironment::FindAdapter(const std::vector<GraphicsAdapterInfo>& Adapters,
                                          ADAPTER_TYPE                            AdapterType,
                                          Uint32                                  AdapterId)
{
    if (AdapterId != DEFAULT_ADAPTER_ID && AdapterId >= Adapters.size())
    {
        LOG_ERROR_MESSAGE("Adapter ID (", AdapterId, ") is invalid. Only ", Adapters.size(), " adapter(s) found on the system");
        AdapterId = DEFAULT_ADAPTER_ID;
    }

    if (AdapterId == DEFAULT_ADAPTER_ID && AdapterType != ADAPTER_TYPE_UNKNOWN)
    {
        for (Uint32 i = 0; i < Adapters.size(); ++i)
        {
            if (Adapters[i].Type == AdapterType)
            {
                AdapterId     = i;
                m_AdapterType = AdapterType;
                break;
            }
        }
        if (AdapterId == DEFAULT_ADAPTER_ID)
            LOG_WARNING_MESSAGE("Unable to find the requested adapter type. Using default adapter.");
    }

    if (AdapterId != DEFAULT_ADAPTER_ID)
        LOG_INFO_MESSAGE("Using adapter ", AdapterId, ": '", Adapters[AdapterId].Description, "'");

    return AdapterId;
}

GPUTestingEnvironment::GPUTestingEnvironment(const CreateInfo& EnvCI, const SwapChainDesc& SCDesc) :
    m_DeviceType{EnvCI.deviceType}
{
    Uint32 NumDeferredCtx = 0;

    std::vector<IDeviceContext*>            ppContexts;
    std::vector<GraphicsAdapterInfo>        Adapters;
    std::vector<ImmediateContextCreateInfo> ContextCI;

    auto EnumerateAdapters = [&Adapters](IEngineFactory* pFactory, Version MinVersion, auto EnumerateDisplayModes) //
    {
        Uint32 NumAdapters = 0;
        pFactory->EnumerateAdapters(MinVersion, NumAdapters, 0);
        if (NumAdapters > 0)
        {
            Adapters.resize(NumAdapters);
            pFactory->EnumerateAdapters(MinVersion, NumAdapters, Adapters.data());

            // Validate adapter info
            for (auto& Adapter : Adapters)
            {
                VERIFY_EXPR(Adapter.NumQueues >= 1);
            }
        }

        LOG_INFO_MESSAGE("Found ", Adapters.size(), " compatible ", (Adapters.size() == 1 ? "adapter" : "adapters"));
        for (Uint32 i = 0; i < Adapters.size(); ++i)
        {
            const auto& AdapterInfo  = Adapters[i];
            const auto  DisplayModes = EnumerateDisplayModes(AdapterInfo, i);

            const char* AdapterTypeStr = nullptr;
            switch (AdapterInfo.Type)
            {
                case ADAPTER_TYPE_DISCRETE:
                case ADAPTER_TYPE_INTEGRATED: AdapterTypeStr = " (HW)"; break;
                case ADAPTER_TYPE_SOFTWARE: AdapterTypeStr = " (SW)"; break;
                default: AdapterTypeStr = "";
            }
            std::stringstream ss;
            ss << "Adapter " << i << ": '" << AdapterInfo.Description << '\'' << AdapterTypeStr
               << ", Local/Host-Visible/Unified Memory: "
               << AdapterInfo.Memory.LocalMemory / (1 << 20) << " MB / "
               << AdapterInfo.Memory.HostVisibleMemory / (1 << 20) << " MB / "
               << AdapterInfo.Memory.UnifiedMemory / (1 << 20) << " MB";
            if (!DisplayModes.empty())
                ss << "; " << DisplayModes.size() << (DisplayModes.size() == 1 ? " display mode" : " display modes");

            LOG_INFO_MESSAGE(ss.str());
        }
    };

#if D3D12_SUPPORTED || VULKAN_SUPPORTED || METAL_SUPPORTED
    auto AddContext = [&ContextCI, &Adapters](COMMAND_QUEUE_TYPE Type, const char* Name, Uint32 AdapterId) //
    {
        if (AdapterId >= Adapters.size())
            AdapterId = 0;

        constexpr auto QueueMask = COMMAND_QUEUE_TYPE_PRIMARY_MASK;
        auto*          Queues    = Adapters[AdapterId].Queues;
        for (Uint32 q = 0, Count = Adapters[AdapterId].NumQueues; q < Count; ++q)
        {
            auto& CurQueue = Queues[q];
            if (CurQueue.MaxDeviceContexts == 0)
                continue;

            if ((CurQueue.QueueType & QueueMask) == Type)
            {
                CurQueue.MaxDeviceContexts -= 1;

                ImmediateContextCreateInfo Ctx{};
                Ctx.QueueId  = static_cast<Uint8>(q);
                Ctx.Name     = Name;
                Ctx.Priority = QUEUE_PRIORITY_MEDIUM;
                ContextCI.push_back(Ctx);
                return;
            }
        }
    };
#endif

    {
        bool FeaturesPrinted = false;
        DeviceFeatures::Enumerate(EnvCI.Features,
                                  [&FeaturesPrinted](const char* FeatName, DEVICE_FEATURE_STATE State) //
                                  {
                                      if (State != DEVICE_FEATURE_STATE_OPTIONAL)
                                      {
                                          std::cout << "Features." << FeatName << " = " << (State == DEVICE_FEATURE_STATE_ENABLED ? "On" : "Off") << '\n';
                                          FeaturesPrinted = true;
                                      }
                                      return true;
                                  });
        if (FeaturesPrinted)
            std::cout << '\n';
    }

    switch (m_DeviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
        {
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D11() function
            auto GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
            if (GetEngineFactoryD3D11 == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif

            auto* pFactoryD3D11 = GetEngineFactoryD3D11();
            pFactoryD3D11->SetMessageCallback(MessageCallback);

            EngineD3D11CreateInfo EngineCI;
            EngineCI.GraphicsAPIVersion = Version{11, 0};
            EngineCI.Features           = EnvCI.Features;
#    ifdef DILIGENT_DEVELOPMENT
            EngineCI.SetValidationLevel(VALIDATION_LEVEL_2);
#    endif
            EnumerateAdapters(pFactoryD3D11, EngineCI.GraphicsAPIVersion,
                              [pFactoryD3D11, &EngineCI](const GraphicsAdapterInfo& AdapterInfo, Uint32 AdapterId) {
                                  std::vector<DisplayModeAttribs> DisplayModes;
                                  if (AdapterInfo.NumOutputs > 0)
                                  {
                                      Uint32 NumDisplayModes = 0;
                                      pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, AdapterId, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                                      DisplayModes.resize(NumDisplayModes);
                                      pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, AdapterId, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                                  }
                                  return DisplayModes;
                              });

            EngineCI.AdapterId           = FindAdapter(Adapters, EnvCI.AdapterType, EnvCI.AdapterId);
            NumDeferredCtx               = EnvCI.NumDeferredContexts;
            EngineCI.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(std::max(size_t{1}, ContextCI.size()) + NumDeferredCtx);
            pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
        {
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D12() function
            auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
            if (GetEngineFactoryD3D12 == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif
            auto* pFactoryD3D12 = GetEngineFactoryD3D12();
            pFactoryD3D12->SetMessageCallback(MessageCallback);

            if (!pFactoryD3D12->LoadD3D12())
            {
                LOG_ERROR_AND_THROW("Failed to load d3d12 dll");
            }

            EngineD3D12CreateInfo EngineCI;
            EngineCI.GraphicsAPIVersion = Version{11, 0};

            EnumerateAdapters(pFactoryD3D12, EngineCI.GraphicsAPIVersion,
                              [pFactoryD3D12, &EngineCI](const GraphicsAdapterInfo& AdapterInfo, Uint32 AdapterId) {
                                  std::vector<DisplayModeAttribs> DisplayModes;
                                  if (AdapterInfo.NumOutputs > 0)
                                  {
                                      Uint32 NumDisplayModes = 0;
                                      pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, AdapterId, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                                      DisplayModes.resize(NumDisplayModes);
                                      pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, AdapterId, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                                  }
                                  return DisplayModes;
                              });

            // Always enable validation
            EngineCI.SetValidationLevel(VALIDATION_LEVEL_1);
            EngineCI.Features = EnvCI.Features;

            EngineCI.AdapterId = FindAdapter(Adapters, EnvCI.AdapterType, EnvCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_COMPUTE, "Compute", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_TRANSFER, "Transfer", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics 2", EngineCI.AdapterId);
            EngineCI.NumImmediateContexts  = static_cast<Uint32>(ContextCI.size());
            EngineCI.pImmediateContextInfo = EngineCI.NumImmediateContexts > 0 ? ContextCI.data() : nullptr;

            //EngineCI.EnableGPUBasedValidation                = true;
            EngineCI.CPUDescriptorHeapAllocationSize[0]      = 64; // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            EngineCI.CPUDescriptorHeapAllocationSize[1]      = 32; // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            EngineCI.CPUDescriptorHeapAllocationSize[2]      = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            EngineCI.CPUDescriptorHeapAllocationSize[3]      = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
            EngineCI.DynamicDescriptorAllocationChunkSize[0] = 8;  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            EngineCI.DynamicDescriptorAllocationChunkSize[1] = 8;  // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER

            NumDeferredCtx               = EnvCI.NumDeferredContexts;
            EngineCI.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(std::max(size_t{1}, ContextCI.size()) + NumDeferredCtx);
            pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
        {
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
            // Declare function pointer
            // Load the dll and import GetEngineFactoryOpenGL() function
            auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
            if (GetEngineFactoryOpenGL == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif
            auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
            pFactoryOpenGL->SetMessageCallback(MessageCallback);
            EnumerateAdapters(pFactoryOpenGL, Version{},
                              [](const GraphicsAdapterInfo& AdapterInfo, Uint32 AdapterId) {
                                  return std::vector<DisplayModeAttribs>{};
                              });
            auto Window = CreateNativeWindow();

            EngineGLCreateInfo EngineCI;

            // Always enable validation
            EngineCI.SetValidationLevel(VALIDATION_LEVEL_1);

            EngineCI.Window   = Window;
            EngineCI.Features = EnvCI.Features;
            NumDeferredCtx    = 0;
            ppContexts.resize(std::max(size_t{1}, ContextCI.size()) + NumDeferredCtx);
            RefCntAutoPtr<ISwapChain> pSwapChain; // We will use testing swap chain instead
            pFactoryOpenGL->CreateDeviceAndSwapChainGL(
                EngineCI, &m_pDevice, ppContexts.data(), SCDesc, &pSwapChain);
        }
        break;
#endif

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
        {
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
            // Load the dll and import GetEngineFactoryVk() function
            auto GetEngineFactoryVk = LoadGraphicsEngineVk();
            if (GetEngineFactoryVk == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif

            auto* pFactoryVk = GetEngineFactoryVk();
            pFactoryVk->SetMessageCallback(MessageCallback);

            if (EnvCI.EnableDeviceSimulation)
                pFactoryVk->EnableDeviceSimulation();

            EnumerateAdapters(pFactoryVk, Version{},
                              [](const GraphicsAdapterInfo& AdapterInfo, Uint32 AdapterId) {
                                  return std::vector<DisplayModeAttribs>{};
                              });

            EngineVkCreateInfo EngineCI;
            EngineCI.AdapterId = FindAdapter(Adapters, EnvCI.AdapterType, EnvCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_COMPUTE, "Compute", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_TRANSFER, "Transfer", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics 2", EngineCI.AdapterId);

            // Always enable validation
            EngineCI.SetValidationLevel(VALIDATION_LEVEL_1);

            EngineCI.NumImmediateContexts      = static_cast<Uint32>(ContextCI.size());
            EngineCI.pImmediateContextInfo     = EngineCI.NumImmediateContexts > 0 ? ContextCI.data() : nullptr;
            EngineCI.MainDescriptorPoolSize    = VulkanDescriptorPoolSize{64, 64, 256, 256, 64, 32, 32, 32, 32, 16, 16};
            EngineCI.DynamicDescriptorPoolSize = VulkanDescriptorPoolSize{64, 64, 256, 256, 64, 32, 32, 32, 32, 16, 16};
            EngineCI.UploadHeapPageSize        = 32 * 1024;
            //EngineCI.DeviceLocalMemoryReserveSize = 32 << 20;
            //EngineCI.HostVisibleMemoryReserveSize = 48 << 20;
            EngineCI.Features = EnvCI.Features;

            NumDeferredCtx               = EnvCI.NumDeferredContexts;
            EngineCI.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(std::max(size_t{1}, ContextCI.size()) + NumDeferredCtx);
            pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if METAL_SUPPORTED
        case RENDER_DEVICE_TYPE_METAL:
        {
            auto* pFactoryMtl = GetEngineFactoryMtl();
            pFactoryMtl->SetMessageCallback(MessageCallback);

            EnumerateAdapters(pFactoryMtl, Version{},
                              [](const GraphicsAdapterInfo& AdapterInfo, Uint32 AdapterId) {
                                  return std::vector<DisplayModeAttribs>{};
                              });

            EngineMtlCreateInfo EngineCI;
            EngineCI.AdapterId = FindAdapter(Adapters, EnvCI.AdapterType, EnvCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_COMPUTE, "Compute", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_TRANSFER, "Transfer", EngineCI.AdapterId);
            AddContext(COMMAND_QUEUE_TYPE_GRAPHICS, "Graphics 2", EngineCI.AdapterId);

            EngineCI.NumImmediateContexts  = static_cast<Uint32>(ContextCI.size());
            EngineCI.pImmediateContextInfo = EngineCI.NumImmediateContexts ? ContextCI.data() : nullptr;
            EngineCI.Features              = EnvCI.Features;

            // Always enable validation
            EngineCI.SetValidationLevel(VALIDATION_LEVEL_1);

            NumDeferredCtx               = EnvCI.NumDeferredContexts;
            EngineCI.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(std::max(size_t{1}, ContextCI.size()) + NumDeferredCtx);
            pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &m_pDevice, ppContexts.data());
        }
        break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unknown device type");
            break;
    }

    {
        const auto& ActualFeats = m_pDevice->GetDeviceInfo().Features;
#define CHECK_FEATURE_STATE(Feature)                                                                                                      \
    if (EnvCI.Features.Feature != DEVICE_FEATURE_STATE_OPTIONAL && EnvCI.Features.Feature != ActualFeats.Feature)                         \
    {                                                                                                                                     \
        LOG_ERROR_MESSAGE("requested state (", GetDeviceFeatureStateString(EnvCI.Features.Feature), ") of the '", #Feature,               \
                          "' feature does not match the actual feature state (", GetDeviceFeatureStateString(ActualFeats.Feature), ")."); \
        UNEXPECTED("Requested feature state does not match the actual state.");                                                           \
    }

        ENUMERATE_DEVICE_FEATURES(CHECK_FEATURE_STATE)

#undef CHECK_FEATURE_STATE
    }

    constexpr Uint8 InvalidQueueId = 64; // MAX_COMMAND_QUEUES
    m_NumImmediateContexts         = std::max(1u, static_cast<Uint32>(ContextCI.size()));
    m_pDeviceContexts.resize(ppContexts.size());
    for (size_t i = 0; i < ppContexts.size(); ++i)
    {
        if (ppContexts[i] == nullptr)
            LOG_ERROR_AND_THROW("Context must not be null");

        const auto CtxDesc = ppContexts[i]->GetDesc();
        VERIFY(CtxDesc.ContextId == static_cast<Uint8>(i), "Invalid context index");
        if (i < m_NumImmediateContexts)
        {
            VERIFY(!CtxDesc.IsDeferred, "Immediate context expected");
        }
        else
        {
            VERIFY(CtxDesc.IsDeferred, "Deferred context expected");
            VERIFY(CtxDesc.QueueId >= InvalidQueueId, "Hardware queue id must be invalid");
        }
        m_pDeviceContexts[i].Attach(ppContexts[i]);
    }

    for (size_t i = 0; i < ContextCI.size(); ++i)
    {
        const auto& CtxCI   = ContextCI[i];
        const auto  CtxDesc = m_pDeviceContexts[i]->GetDesc();
        if (CtxCI.QueueId != CtxDesc.QueueId)
            LOG_ERROR_MESSAGE("QueueId mismatch");
        if (i != CtxDesc.ContextId)
            LOG_ERROR_MESSAGE("CommandQueueId mismatch");
    }

    const auto& AdapterInfo = m_pDevice->GetAdapterInfo();
    std::string AdapterInfoStr;
    AdapterInfoStr = "Adapter description: ";
    AdapterInfoStr += AdapterInfo.Description;
    AdapterInfoStr += ". Vendor: ";
    static_assert(ADAPTER_VENDOR_LAST == 10, "Please update the switch below to handle the new adapter type");
    switch (AdapterInfo.Vendor)
    {
        case ADAPTER_VENDOR_NVIDIA:
            AdapterInfoStr += "NVidia";
            break;
        case ADAPTER_VENDOR_AMD:
            AdapterInfoStr += "AMD";
            break;
        case ADAPTER_VENDOR_INTEL:
            AdapterInfoStr += "Intel";
            break;
        case ADAPTER_VENDOR_ARM:
            AdapterInfoStr += "ARM";
            break;
        case ADAPTER_VENDOR_QUALCOMM:
            AdapterInfoStr += "Qualcomm";
            break;
        case ADAPTER_VENDOR_IMGTECH:
            AdapterInfoStr += "Imagination tech";
            break;
        case ADAPTER_VENDOR_MSFT:
            AdapterInfoStr += "Microsoft";
            break;
        case ADAPTER_VENDOR_APPLE:
            AdapterInfoStr += "Apple";
            break;
        case ADAPTER_VENDOR_MESA:
            AdapterInfoStr += "Mesa";
            break;
        case ADAPTER_VENDOR_BROADCOM:
            AdapterInfoStr += "Broadcom";
            break;
        default:
            AdapterInfoStr += "Unknown";
    }
    AdapterInfoStr += ". Local memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.Memory.LocalMemory >> 20);
    AdapterInfoStr += " MB. Host-visible memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.Memory.HostVisibleMemory >> 20);
    AdapterInfoStr += " MB. Unified memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.Memory.UnifiedMemory >> 20);
    AdapterInfoStr += " MB.";
    LOG_INFO_MESSAGE(AdapterInfoStr);

#if ARCHIVER_SUPPORTED
    // Create archiver factory
    {
#    if EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL
        auto GetArchiverFactory = LoadArchiverFactory();
        if (GetArchiverFactory != nullptr)
        {
            m_ArchiverFactory = GetArchiverFactory();
        }
#    else
        m_ArchiverFactory = Diligent::GetArchiverFactory();
#    endif
        m_ArchiverFactory->SetMessageCallback(MessageCallback);
    }
#endif
}

GPUTestingEnvironment::~GPUTestingEnvironment()
{
    for (Uint32 i = 0; i < GetNumImmediateContexts(); ++i)
    {
        auto* pCtx = GetDeviceContext(i);
        pCtx->Flush();
        pCtx->FinishFrame();
    }
}

// Override this to define how to set up the environment.
void GPUTestingEnvironment::SetUp()
{
}

// Override this to define how to tear down the environment.
void GPUTestingEnvironment::TearDown()
{
}

void GPUTestingEnvironment::ReleaseResources()
{
    // It is necessary to call Flush() to force the driver to release resources.
    // Without flushing the command buffer, the memory may not be released until sometimes
    // later causing out-of-memory error.
    for (Uint32 i = 0; i < GetNumImmediateContexts(); ++i)
    {
        auto* pCtx = GetDeviceContext(i);
        pCtx->Flush();
        pCtx->FinishFrame();
        pCtx->WaitForIdle();
    }
    m_pDevice->ReleaseStaleResources();
}

void GPUTestingEnvironment::Reset()
{
    for (Uint32 i = 0; i < GetNumImmediateContexts(); ++i)
    {
        auto* pCtx = GetDeviceContext(i);
        pCtx->Flush();
        pCtx->FinishFrame();
        pCtx->InvalidateState();
    }
    m_pDevice->IdleGPU();
    m_pDevice->ReleaseStaleResources();
    m_NumAllowedErrors = 0;
}

RefCntAutoPtr<ITexture> GPUTestingEnvironment::CreateTexture(const char* Name, TEXTURE_FORMAT Fmt, BIND_FLAGS BindFlags, Uint32 Width, Uint32 Height, void* pInitData)
{
    TextureDesc TexDesc;

    TexDesc.Name      = Name;
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Format    = Fmt;
    TexDesc.BindFlags = BindFlags;
    TexDesc.Width     = Width;
    TexDesc.Height    = Height;

    const auto        FmtAttribs = GetTextureFormatAttribs(Fmt);
    TextureSubResData Mip0Data{pInitData, FmtAttribs.ComponentSize * FmtAttribs.NumComponents * Width};
    TextureData       TexData{&Mip0Data, 1};

    RefCntAutoPtr<ITexture> pTexture;
    m_pDevice->CreateTexture(TexDesc, pInitData ? &TexData : nullptr, &pTexture);
    VERIFY_EXPR(pTexture != nullptr);

    return pTexture;
}

RefCntAutoPtr<ISampler> GPUTestingEnvironment::CreateSampler(const SamplerDesc& Desc)
{
    RefCntAutoPtr<ISampler> pSampler;
    m_pDevice->CreateSampler(Desc, &pSampler);
    return pSampler;
}

void GPUTestingEnvironment::SetDefaultCompiler(SHADER_COMPILER compiler)
{
    switch (m_pDevice->GetDeviceInfo().Type)
    {
        case RENDER_DEVICE_TYPE_D3D12:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_FXC:
                case SHADER_COMPILER_DXC:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Direct3D12 backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_D3D11:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_FXC:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Direct3D11 backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by OpenGL/GLES backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_VULKAN:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_GLSLANG:
                    m_ShaderCompiler = compiler;
                    break;

                case SHADER_COMPILER_DXC:
                    if (HasDXCompiler())
                    {
                        m_ShaderCompiler = compiler;
                    }
                    else
                    {
                        LOG_WARNING_MESSAGE("DXC is not available. Using default shader compiler");
                        m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
                    }
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Vulkan backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_METAL:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Metal backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        default:
            LOG_WARNING_MESSAGE("Unexpected device type");
            m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
    }

    LOG_INFO_MESSAGE("Selected shader compiler: ", GetShaderCompilerTypeString(m_ShaderCompiler));
}

SHADER_COMPILER GPUTestingEnvironment::GetDefaultCompiler(SHADER_SOURCE_LANGUAGE lang) const
{
    if (m_pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_VULKAN &&
        lang != SHADER_SOURCE_LANGUAGE_HLSL)
        return SHADER_COMPILER_GLSLANG;
    else
        return m_ShaderCompiler;
}

static bool ParseFeatureState(const char* Arg, DeviceFeatures& Features)
{
    static const std::string ArgStart = "--Features.";
    if (ArgStart.compare(0, ArgStart.length(), Arg, ArgStart.length()) != 0)
        return false;

    Arg += ArgStart.length();
    DeviceFeatures::Enumerate(Features,
                              [Arg](const char* FeatName, DEVICE_FEATURE_STATE& State) //
                              {
                                  const auto NameLen = strlen(FeatName);
                                  if (strncmp(FeatName, Arg, NameLen) != 0)
                                      return true;

                                  if (Arg[NameLen] != '=')
                                      return true; // Continue processing

                                  const auto Value = Arg + NameLen + 1;

                                  static const std::string Off      = "Off";
                                  static const std::string On       = "On";
                                  static const std::string Disabled = "Disabled";
                                  static const std::string Enabled  = "Enabled";

                                  if (StrCmpNoCase(Value, On.c_str()) == 0 || StrCmpNoCase(Value, Enabled.c_str()) == 0)
                                      State = DEVICE_FEATURE_STATE_ENABLED;
                                  else if (StrCmpNoCase(Value, Off.c_str()) == 0 || StrCmpNoCase(Value, Disabled.c_str()) == 0)
                                      State = DEVICE_FEATURE_STATE_DISABLED;
                                  else
                                  {
                                      LOG_ERROR_MESSAGE('\'', Value, "' is not a valid value for feature '", FeatName, "'. The following values are allowed: '",
                                                        Off, "', '", Disabled, "', '", On, "', '", Enabled, "'.");
                                  }

                                  return false;
                              });

    return false;
}

GPUTestingEnvironment* GPUTestingEnvironment::Initialize(int argc, char** argv)
{

    GPUTestingEnvironment::CreateInfo TestEnvCI;
    SHADER_COMPILER                   ShCompiler = SHADER_COMPILER_DEFAULT;
    for (int i = 1; i < argc; ++i)
    {
        const std::string AdapterArgName = "--adapter=";

        const auto* arg = argv[i];
        if (strcmp(arg, "--mode=d3d11") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_D3D11;
        }
        else if (strcmp(arg, "--mode=d3d11_sw") == 0)
        {
            TestEnvCI.deviceType  = RENDER_DEVICE_TYPE_D3D11;
            TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
        }
        else if (strcmp(arg, "--mode=d3d12") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_D3D12;
        }
        else if (strcmp(arg, "--mode=d3d12_sw") == 0)
        {
            TestEnvCI.deviceType  = RENDER_DEVICE_TYPE_D3D12;
            TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
        }
        else if (strcmp(arg, "--mode=vk") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_VULKAN;
        }
        else if (strcmp(arg, "--mode=vk_sw") == 0)
        {
            TestEnvCI.deviceType  = RENDER_DEVICE_TYPE_VULKAN;
            TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
        }
        else if (strcmp(arg, "--mode=gl") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_GL;
        }
        else if (strcmp(arg, "--mode=mtl") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_METAL;
        }
        else if (AdapterArgName.compare(0, AdapterArgName.length(), arg, AdapterArgName.length()) == 0)
        {
            const auto* AdapterStr = arg + AdapterArgName.length();
            if (strcmp(AdapterStr, "sw") == 0)
                TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
            else
                TestEnvCI.AdapterId = static_cast<Uint32>(atoi(AdapterStr));
        }
        else if (strcmp(arg, "--shader_compiler=dxc") == 0)
        {
            ShCompiler = SHADER_COMPILER_DXC;
        }
        else if (strcmp(arg, "--non_separable_progs") == 0)
        {
            TestEnvCI.Features.SeparablePrograms = DEVICE_FEATURE_STATE_DISABLED;
        }
        else if (strcmp(arg, "--vk_dev_sim") == 0)
        {
            TestEnvCI.EnableDeviceSimulation = true;
        }
        else if (ParseFeatureState(arg, TestEnvCI.Features))
        {
            // Feature state has been updated by ParseFeatureState
        }
    }

    if (TestEnvCI.deviceType == RENDER_DEVICE_TYPE_UNDEFINED)
    {
        LOG_ERROR_MESSAGE("Device type is not specified");
        return nullptr;
    }

    SwapChainDesc SCDesc;
    SCDesc.Width             = 512;
    SCDesc.Height            = 512;
    SCDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
    SCDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

    Diligent::Testing::GPUTestingEnvironment* pEnv = nullptr;
    try
    {
        std::cout << "\n\n\n==================== Running tests in "
                  << GetRenderDeviceTypeString(TestEnvCI.deviceType)
                  << (TestEnvCI.AdapterType == ADAPTER_TYPE_SOFTWARE ? "-SW" : "")
                  << " mode ====================\n\n";

        switch (TestEnvCI.deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                pEnv = CreateTestingEnvironmentD3D11(TestEnvCI, SCDesc);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                pEnv = CreateTestingEnvironmentD3D12(TestEnvCI, SCDesc);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                pEnv = CreateTestingEnvironmentGL(TestEnvCI, SCDesc);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                pEnv = CreateTestingEnvironmentVk(TestEnvCI, SCDesc);
                break;
#endif

#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                pEnv = CreateTestingEnvironmentMtl(TestEnvCI, SCDesc);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        const auto DeviceType = pEnv->GetDevice()->GetDeviceInfo().Type;
        if (DeviceType != TestEnvCI.deviceType)
        {
            delete pEnv;
            LOG_ERROR_AND_THROW("Requested device type (", GetRenderDeviceTypeString(TestEnvCI.deviceType),
                                ") does not match the type of the device that was created (", GetRenderDeviceTypeString(DeviceType), ").");
        }

        const auto AdapterType = pEnv->GetDevice()->GetAdapterInfo().Type;
        if (TestEnvCI.AdapterType != ADAPTER_TYPE_UNKNOWN && TestEnvCI.AdapterType != AdapterType)
        {
            delete pEnv;
            LOG_ERROR_AND_THROW("Requested adapter type (", GetAdapterTypeString(TestEnvCI.AdapterType),
                                ") does not match the type of the adapter that was created (", GetAdapterTypeString(AdapterType), ").");
        }
    }
    catch (...)
    {
        return nullptr;
    }

    pEnv->SetDefaultCompiler(ShCompiler);

    return pEnv;
}

} // namespace Testing

} // namespace Diligent
