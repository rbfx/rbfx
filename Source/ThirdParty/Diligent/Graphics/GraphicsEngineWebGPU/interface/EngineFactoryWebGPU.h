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

#pragma once

/// \file
/// Declaration of functions that initialize WebGPU-based engine implementation

#include "../../GraphicsEngine/interface/EngineFactory.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/SwapChain.h"

#if PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_EMSCRIPTEN || (PLATFORM_WIN32 && !defined(_MSC_VER))
// https://gcc.gnu.org/wiki/Visibility
#    define API_QUALIFIER __attribute__((visibility("default")))
#elif PLATFORM_WIN32
#    define API_QUALIFIER
#else
#    error Unsupported platform
#endif

#if ENGINE_DLL && PLATFORM_WIN32 && defined(_MSC_VER)
#    include "../../GraphicsEngine/interface/LoadEngineDll.h"
#    define EXPLICITLY_LOAD_ENGINE_WEBGPU_DLL 1
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {CF7F4278-4EA7-491A-8575-161A5F3D95EC}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_EngineFactoryWebGPU =
    {0xCF7F4278, 0x4EA7, 0x491A, {0x85, 0x75, 0x16, 0x1A, 0x5F, 0x3D, 0x95, 0xEC}};

#define DILIGENT_INTERFACE_NAME IEngineFactoryWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IEngineFactoryWebGPUInclusiveMethods \
    IEngineFactoryInclusiveMethods;          \
    IEngineFactoryWebGPUMethods EngineFactoryWebGPU

// clang-format off

/// Engine factory for WebGPU rendering backend.
DILIGENT_BEGIN_INTERFACE(IEngineFactoryWebGPU, IEngineFactory)
{
    /// Creates a render device and device contexts for WebGPU-based engine implementation.

    /// \param [in] EngineCI  - Engine creation info.
    /// \param [out] ppDevice - Address of the memory location where pointer to
    ///                         the created device will be written.
    /// \param [out] ppContexts - Address of the memory location where pointers to
    ///                           the contexts will be written. Immediate context goes at
    ///                           position 0. If EngineCI.NumDeferredContexts > 0,
    ///                           pointers to deferred contexts are written afterwards.
    VIRTUAL void METHOD(CreateDeviceAndContextsWebGPU)(THIS_
                                                      const EngineWebGPUCreateInfo REF EngineCI,
                                                      IRenderDevice**                    ppDevice,
                                                      IDeviceContext**                   ppContexts) PURE;

    /// Creates a swap chain for WebGPU-based engine implementation.

    /// \param [in] pDevice           - Pointer to the render device.
    /// \param [in] pImmediateContext - Pointer to the immediate device context.
    /// \param [in] SCDesc            - Swap chain description.
    /// \param [in] Window            - Platform-specific native window description that
    ///                                 the swap chain will be associated with:
    ///                                 * On Win32 platform, this is the window handle (HWND)
    ///                                 * On Universal Windows Platform, this is the reference
    ///                                   to the core window (Windows::UI::Core::CoreWindow)
    ///
    /// \param [out] ppSwapChain    - Address of the memory location where pointer to the new
    ///                               swap chain will be written.
    VIRTUAL void METHOD(CreateSwapChainWebGPU)(THIS_
                                              IRenderDevice*               pDevice,
                                              IDeviceContext*              pImmediateContext,
                                              const SwapChainDesc REF      SCDesc,
                                              const NativeWindow REF       Window,
                                              ISwapChain**                 ppSwapChain) PURE;

    /// Attaches to existing WebGPU render device, adapter and instance.

    /// \param [in] wgpuInstance     - pointer to the native WebGPU instance.
    /// \param [in] wgpuAdapter      - pointer to the native WebGPU adapter.
    /// \param [in] wgpuDevice       - pointer to the native WebGPU device.
    /// \param [in] EngineCI         - Engine creation info.
    /// \param [out] ppDevice        - Address of the memory location where pointer to
    ///                                      the created device will be written.
    /// \param [out] ppContexts - Address of the memory location where pointers to
    ///                           the contexts will be written. Immediate context goes at
    ///                           position 0. If EngineCI.NumDeferredContexts > 0,
    ///                           pointers to the deferred contexts are written afterwards.
    VIRTUAL void METHOD(AttachToWebGPUDevice)(THIS_ 
                                              void*                            wgpuInstance,
                                              void*                            wgpuAdapter,
                                              void*                            wgpuDevice,
                                              const EngineWebGPUCreateInfo REF EngineCI,
                                              IRenderDevice**                  ppDevice,
                                              IDeviceContext**                 ppContexts) PURE;

    /// Return the pointer to DawnProcTable
    VIRTUAL CONST void* METHOD(GetProcessTable)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IEngineFactoryWebGPU_CreateDeviceAndContextsWebGPU(This, ...) CALL_IFACE_METHOD(EngineFactoryWebGPU, CreateDeviceAndContextsWebGPU, This, __VA_ARGS__)
#    define IEngineFactoryWebGPU_CreateSwapChainWebGPU(This, ...)         CALL_IFACE_METHOD(EngineFactoryWebGPU, CreateSwapChainWebGPU,         This, __VA_ARGS__)

// clang-format on

#endif


#if EXPLICITLY_LOAD_ENGINE_WEBGPU_DLL

typedef struct IEngineFactoryWebGPU* (*GetEngineFactoryWebGPUType)();

inline GetEngineFactoryWebGPUType DILIGENT_GLOBAL_FUNCTION(LoadGraphicsEngineWebGPU)()
{
    return (GetEngineFactoryWebGPUType)LoadEngineDll("GraphicsEngineWebGPU", "GetEngineFactoryWebGPU");
}

#else

API_QUALIFIER
struct IEngineFactoryWebGPU* DILIGENT_GLOBAL_FUNCTION(GetEngineFactoryWebGPU)();

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
