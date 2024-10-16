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
/// Routines that initialize OpenGL/GLES-based engine implementation

#include "pch.h"
#include "EngineFactoryOpenGL.h"
#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "EngineFactoryBase.hpp"
#include "DearchiverGLImpl.hpp"
#include "EngineMemory.h"

#if !DILIGENT_NO_HLSL
#    include "HLSL2GLSLConverterObject.hpp"
#endif

#include "EngineFactoryBase.hpp"

#if PLATFORM_IOS
#    include "SwapChainGLIOS.hpp"
#else
#    include "SwapChainGLImpl.hpp"
#endif

#if PLATFORM_ANDROID
#    include "RenderDeviceGLESImpl.hpp"
#    include "FileStream.h"
#    include "FileSystem.hpp"
#endif

#if PLATFORM_EMSCRIPTEN
#    include "RenderDeviceGLESImpl.hpp"
#endif

namespace Diligent
{

#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_LINUX || PLATFORM_MACOS
using TRenderDeviceGLImpl = RenderDeviceGLImpl;
using TSwapChain          = SwapChainGLImpl;
#elif PLATFORM_ANDROID || PLATFORM_EMSCRIPTEN
using TRenderDeviceGLImpl = RenderDeviceGLESImpl;
using TSwapChain          = SwapChainGLImpl;
#elif PLATFORM_IOS
using TRenderDeviceGLImpl = RenderDeviceGLImpl;
using TSwapChain          = SwapChainGLIOS;
#else
#    error Unsupported platform
#endif

/// Engine factory for OpenGL implementation
class EngineFactoryOpenGLImpl : public EngineFactoryBase<IEngineFactoryOpenGL>
{
public:
    static EngineFactoryOpenGLImpl* GetInstance()
    {
        static EngineFactoryOpenGLImpl TheFactory;
        return &TheFactory;
    }

    using TBase = EngineFactoryBase<IEngineFactoryOpenGL>;
    EngineFactoryOpenGLImpl() :
        TBase{IID_EngineFactoryOpenGL}
    {}

    virtual void DILIGENT_CALL_TYPE CreateDeviceAndSwapChainGL(const EngineGLCreateInfo& EngineCI,
                                                               IRenderDevice**           ppDevice,
                                                               IDeviceContext**          ppImmediateContext,
                                                               const SwapChainDesc&      SCDesc,
                                                               ISwapChain**              ppSwapChain) override final;

    virtual void DILIGENT_CALL_TYPE CreateHLSL2GLSLConverter(IHLSL2GLSLConverter** ppConverter) override final;

    virtual void DILIGENT_CALL_TYPE AttachToActiveGLContext(const EngineGLCreateInfo& EngineCI,
                                                            IRenderDevice**           ppDevice,
                                                            IDeviceContext**          ppImmediateContext) override final;

    virtual void DILIGENT_CALL_TYPE EnumerateAdapters(Version              MinVersion,
                                                      Uint32&              NumAdapters,
                                                      GraphicsAdapterInfo* Adapters) const override final;

    virtual void DILIGENT_CALL_TYPE CreateDearchiver(const DearchiverCreateInfo& CreateInfo,
                                                     IDearchiver**               ppDearchiver) const override final
    {
        TBase::CreateDearchiver<DearchiverGLImpl>(CreateInfo, ppDearchiver);
    }

#if PLATFORM_ANDROID
    virtual void InitAndroidFileSystem(struct AAssetManager* AssetManager,
                                       const char*           ExternalFilesDir,
                                       const char*           OutputFilesDir) const override final;
#endif
};

static void SetDefaultGraphicsAdapterInfo(GraphicsAdapterInfo& AdapterInfo)
{
    AdapterInfo = GraphicsAdapterInfo{};

#if PLATFORM_ANDROID || PLATFORM_IOS
    AdapterInfo.Type = ADAPTER_TYPE_INTEGRATED;
#else
    AdapterInfo.Type = ADAPTER_TYPE_UNKNOWN;
#endif

    AdapterInfo.NumQueues = 1;

    AdapterInfo.Queues[0].QueueType                 = COMMAND_QUEUE_TYPE_GRAPHICS;
    AdapterInfo.Queues[0].MaxDeviceContexts         = 1;
    AdapterInfo.Queues[0].TextureCopyGranularity[0] = 1;
    AdapterInfo.Queues[0].TextureCopyGranularity[1] = 1;
    AdapterInfo.Queues[0].TextureCopyGranularity[2] = 1;
}

static void SetPreferredAdapter(const EngineGLCreateInfo& EngineCI)
{
    if (EngineCI.PreferredAdapterType == ADAPTER_TYPE_DISCRETE)
    {
#if PLATFORM_WIN32
        const HMODULE ModuleHandle                         = GetModuleHandle(nullptr);
        Uint64* const NvOptimusEnablement                  = reinterpret_cast<Uint64*>(GetProcAddress(ModuleHandle, "NvOptimusEnablement"));
        Uint64* const AmdPowerXpressRequestHighPerformance = reinterpret_cast<Uint64*>(GetProcAddress(ModuleHandle, "AmdPowerXpressRequestHighPerformance"));
        if (!NvOptimusEnablement && !AmdPowerXpressRequestHighPerformance)
        {
            LOG_WARNING_MESSAGE("Neither NvOptimusEnablement nor AmdPowerXpressRequestHighPerformance symbols found. "
                                "You need to explicitly define these variables in your executable file: "
                                "https://gist.github.com/statico/6809850727c708f08458, "
                                "or you can use the `Diligent-GLAdapterSelector` object library as source input to your executable target: "
                                "`target_sources(MyExecutable PRIVATE $<TARGET_OBJECTS:Diligent-GLAdapterSelector>)`, "
                                "see https://cmake.org/cmake/help/v3.16/manual/cmake-buildsystem.7.html#object-libraries.");
        }
        if (AmdPowerXpressRequestHighPerformance)
        {
            *AmdPowerXpressRequestHighPerformance = 1;
        }
        if (NvOptimusEnablement)
        {
            *NvOptimusEnablement = 1;
        }
#elif PLATFORM_LINUX
        setenv("DRI_PRIME", "1", 1);
#else
        LOG_WARNING_MESSAGE("Setting preferred adapter type isn't supported on this platform");
#endif
    }
}

void EngineFactoryOpenGLImpl::EnumerateAdapters(Version              MinVersion,
                                                Uint32&              NumAdapters,
                                                GraphicsAdapterInfo* Adapters) const
{
    if (Adapters == nullptr)
    {
        NumAdapters = 1;
        return;
    }

    NumAdapters = std::min(1u, NumAdapters);
    if (NumAdapters > 0)
    {
        SetDefaultGraphicsAdapterInfo(*Adapters);
    }
}

/// Creates render device, device context and swap chain for OpenGL/GLES-based engine implementation

/// \param [in]  EngineCI           - Engine creation attributes.
/// \param [out] ppDevice           - Address of the memory location where pointer to
///                                   the created device will be written.
/// \param [out] ppImmediateContext - Address of the memory location where pointers to
///                                   the immediate context will be written.
/// \param [in]  SCDesc             - Swap chain description.
/// \param [out] ppSwapChain        - Address of the memory location where pointer to the new
///                                   swap chain will be written.
void EngineFactoryOpenGLImpl::CreateDeviceAndSwapChainGL(const EngineGLCreateInfo& EngineCI,
                                                         IRenderDevice**           ppDevice,
                                                         IDeviceContext**          ppImmediateContext,
                                                         const SwapChainDesc&      SCDesc,
                                                         ISwapChain**              ppSwapChain)
{
    if (EngineCI.EngineAPIVersion != DILIGENT_API_VERSION)
    {
        LOG_ERROR_MESSAGE("Diligent Engine runtime (", DILIGENT_API_VERSION, ") is not compatible with the client API version (", EngineCI.EngineAPIVersion, ")");
        return;
    }

    VERIFY(ppDevice && ppImmediateContext && ppSwapChain, "Null pointer provided");
    if (!ppDevice || !ppImmediateContext || !ppSwapChain)
        return;

    if (EngineCI.NumDeferredContexts > 0)
    {
        LOG_ERROR_MESSAGE("OpenGL back-end does not support deferred contexts");
        return;
    }

    if (EngineCI.NumImmediateContexts > 1)
    {
        LOG_ERROR_MESSAGE("OpenGL back-end does not support multiple immediate contexts");
        return;
    }

    *ppDevice           = nullptr;
    *ppImmediateContext = nullptr;
    *ppSwapChain        = nullptr;

    try
    {
        GraphicsAdapterInfo AdapterInfo;
        SetDefaultGraphicsAdapterInfo(AdapterInfo);
        VerifyEngineCreateInfo(EngineCI, AdapterInfo);

        SetRawAllocator(EngineCI.pRawMemAllocator);
        auto& RawMemAllocator = GetRawAllocator();

        SetPreferredAdapter(EngineCI);
        RenderDeviceGLImpl* pRenderDeviceOpenGL{
            NEW_RC_OBJ(RawMemAllocator, "TRenderDeviceGLImpl instance", TRenderDeviceGLImpl)(
                RawMemAllocator, this, EngineCI, &SCDesc //
                )                                        //
        };
        pRenderDeviceOpenGL->QueryInterface(IID_RenderDevice, reinterpret_cast<IObject**>(ppDevice));

        DeviceContextGLImpl* pDeviceContextOpenGL{
            NEW_RC_OBJ(RawMemAllocator, "DeviceContextGLImpl instance", DeviceContextGLImpl)(
                pRenderDeviceOpenGL,
                DeviceContextDesc{
                    EngineCI.pImmediateContextInfo ? EngineCI.pImmediateContextInfo[0].Name : nullptr,
                    COMMAND_QUEUE_TYPE_GRAPHICS,
                    False, // IsDeferred
                    0,     // Context id
                    0      // QueueId
                })         //
        };
        // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceOpenGL will
        // keep a weak reference to the context
        pDeviceContextOpenGL->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppImmediateContext));
        pRenderDeviceOpenGL->SetImmediateContext(0, pDeviceContextOpenGL);

        TSwapChain* pSwapChainGL = NEW_RC_OBJ(RawMemAllocator, "SwapChainGLImpl instance", TSwapChain)(EngineCI, SCDesc, pRenderDeviceOpenGL, pDeviceContextOpenGL);
        pSwapChainGL->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));

        pDeviceContextOpenGL->SetSwapChain(pSwapChainGL);
    }
    catch (const std::runtime_error&)
    {
        if (*ppDevice)
        {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
        }

        if (*ppImmediateContext)
        {
            (*ppImmediateContext)->Release();
            *ppImmediateContext = nullptr;
        }

        if (*ppSwapChain)
        {
            (*ppSwapChain)->Release();
            *ppSwapChain = nullptr;
        }

        LOG_ERROR("Failed to initialize OpenGL-based render device");
    }
}


/// Creates render device, device context and attaches to existing GL context

/// \param [in] EngineCI - Engine creation attributes.
/// \param [out] ppDevice - Address of the memory location where pointer to
///                         the created device will be written.
/// \param [out] ppImmediateContext - Address of the memory location where pointers to
///                                   the immediate context will be written.
void EngineFactoryOpenGLImpl::AttachToActiveGLContext(const EngineGLCreateInfo& EngineCI,
                                                      IRenderDevice**           ppDevice,
                                                      IDeviceContext**          ppImmediateContext)
{
    if (EngineCI.EngineAPIVersion != DILIGENT_API_VERSION)
    {
        LOG_ERROR_MESSAGE("Diligent Engine runtime (", DILIGENT_API_VERSION, ") is not compatible with the client API version (", EngineCI.EngineAPIVersion, ")");
        return;
    }

    VERIFY(ppDevice && ppImmediateContext, "Null pointer provided");
    if (!ppDevice || !ppImmediateContext)
        return;

    if (EngineCI.NumDeferredContexts > 0)
    {
        LOG_ERROR_MESSAGE("OpenGL back-end does not support deferred contexts");
        return;
    }

    if (EngineCI.NumImmediateContexts > 1)
    {
        LOG_ERROR_MESSAGE("OpenGL back-end does not support multiple immediate contexts");
        return;
    }

    *ppDevice           = nullptr;
    *ppImmediateContext = nullptr;

    try
    {
        GraphicsAdapterInfo AdapterInfo;
        SetDefaultGraphicsAdapterInfo(AdapterInfo);
        VerifyEngineCreateInfo(EngineCI, AdapterInfo);

        SetRawAllocator(EngineCI.pRawMemAllocator);
        auto& RawMemAllocator = GetRawAllocator();

        SetPreferredAdapter(EngineCI);
        RenderDeviceGLImpl* pRenderDeviceOpenGL{
            NEW_RC_OBJ(RawMemAllocator, "TRenderDeviceGLImpl instance", TRenderDeviceGLImpl)(
                RawMemAllocator, this, EngineCI //
                )                               //
        };
        pRenderDeviceOpenGL->QueryInterface(IID_RenderDevice, reinterpret_cast<IObject**>(ppDevice));

        DeviceContextGLImpl* pDeviceContextOpenGL{
            NEW_RC_OBJ(RawMemAllocator, "DeviceContextGLImpl instance", DeviceContextGLImpl)(
                pRenderDeviceOpenGL,
                DeviceContextDesc{
                    EngineCI.pImmediateContextInfo ? EngineCI.pImmediateContextInfo[0].Name : nullptr,
                    COMMAND_QUEUE_TYPE_GRAPHICS,
                    False, // IsDeferred
                    0,     // Context Id
                    0      // Queue Id
                })         //
        };
        // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceOpenGL will
        // keep a weak reference to the context
        pDeviceContextOpenGL->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppImmediateContext));
        pRenderDeviceOpenGL->SetImmediateContext(0, pDeviceContextOpenGL);
    }
    catch (const std::runtime_error&)
    {
        if (*ppDevice)
        {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
        }

        if (*ppImmediateContext)
        {
            (*ppImmediateContext)->Release();
            *ppImmediateContext = nullptr;
        }

        LOG_ERROR("Failed to initialize OpenGL-based render device");
    }
}

#ifdef DOXYGEN
/// Loads OpenGL-based engine implementation and exports factory functions
///
/// \return     - Pointer to the function that returns pointer to the factory for
///               the OpenGL engine implementation
///               See EngineFactoryOpenGLImpl::CreateDeviceAndSwapChainGL().
///
/// \remarks Depending on the configuration and platform, the function loads different dll:
///
/// Platform\\Configuration   |           Debug              |         Release
/// --------------------------|------------------------------|----------------------------
///   Win32/x86               | GraphicsEngineOpenGL_32d.dll | GraphicsEngineOpenGL_32r.dll
///   Win32/x64               | GraphicsEngineOpenGL_64d.dll | GraphicsEngineOpenGL_64r.dll
///
/// To load the library on Android, it is necessary to call System.loadLibrary("GraphicsEngineOpenGL") from Java.
GetEngineFactoryOpenGLType LoadGraphicsEngineOpenGL()
{
// This function is only required because DoxyGen refuses to generate documentation for a static function when SHOW_FILES==NO
#    error This function must never be compiled;
}
#endif

void EngineFactoryOpenGLImpl::CreateHLSL2GLSLConverter(IHLSL2GLSLConverter** ppConverter)
{
#if DILIGENT_NO_HLSL
    LOG_ERROR_MESSAGE("Unable to create HLSL2GLSL converter: HLSL support is disabled.");
#else
    HLSL2GLSLConverterObject* pConverter(NEW_RC_OBJ(GetRawAllocator(), "HLSL2GLSLConverterObject instance", HLSL2GLSLConverterObject)());
    pConverter->QueryInterface(IID_HLSL2GLSLConverter, reinterpret_cast<IObject**>(ppConverter));
#endif
}

#if PLATFORM_ANDROID
void EngineFactoryOpenGLImpl::InitAndroidFileSystem(struct AAssetManager* AssetManager,
                                                    const char*           ExternalFilesDir,
                                                    const char*           OutputFilesDir) const
{
    AndroidFileSystem::Init(AssetManager, ExternalFilesDir, OutputFilesDir);
}
#endif


API_QUALIFIER
Diligent::IEngineFactoryOpenGL* GetEngineFactoryOpenGL()
{
    return Diligent::EngineFactoryOpenGLImpl::GetInstance();
}

} // namespace Diligent

extern "C"
{
    API_QUALIFIER
    Diligent::IEngineFactoryOpenGL* Diligent_GetEngineFactoryOpenGL()
    {
        return Diligent::GetEngineFactoryOpenGL();
    }
}
