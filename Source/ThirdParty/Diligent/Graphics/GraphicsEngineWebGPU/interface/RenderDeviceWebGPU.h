/*
 *  Copyright 2023-2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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
/// Definition of the Diligent::IRenderDeviceWebGPU interface

#include "../../GraphicsEngine/interface/RenderDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {BB1F1488-C10D-493F-8139-3B9010598B16}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_RenderDeviceWebGPU =
    {0xBB1F1488, 0xC10D, 0x493F, {0x81, 0x39, 0x3B, 0x90, 0x10, 0x59, 0x8B, 0x16}};

#define DILIGENT_INTERFACE_NAME IRenderDeviceWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderDeviceWebGPUInclusiveMethods \
    IRenderDeviceInclusiveMethods;          \
    IRenderDeviceWebGPUMethods RenderDeviceWebGPU

// clang-format off

/// Exposes WebGPU-specific functionality of a render device.
DILIGENT_BEGIN_INTERFACE(IRenderDeviceWebGPU, IRenderDevice)
{
    /// Returns WebGPU instance
    VIRTUAL WGPUInstance METHOD(GetWebGPUInstance)(THIS) CONST PURE;

    /// Returns WebGPU adapter
    VIRTUAL WGPUAdapter METHOD(GetWebGPUAdapter)(THIS) CONST PURE;

    /// Returns WebGPU device
    VIRTUAL WGPUDevice METHOD(GetWebGPUDevice)(THIS) CONST PURE;

    /// Creates a texture object from native WebGPU texture

    /// \param [in]  wgpuTexture  - WebGPU texture handle
    /// \param [in]  TexDesc      - Texture description. WebGPU provides no means to retrieve any
    ///                             texture properties from the texture handle, so complete texture
    ///                             description must be provided
    /// \param [in]  InitialState - Initial texture state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppTexture    - Address of the memory location where the pointer to the
    ///                             texture interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created texture object does not take ownership of the WebGPU texture and will not
    ///        destroy it once released. The application must not destroy the image while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateTextureFromWebGPUTexture)(THIS_
                                                        WGPUTexture           wgpuTexture,
                                                        const TextureDesc REF TexDesc,
                                                        RESOURCE_STATE        InitialState,
                                                        ITexture**            ppTexture) PURE;

    /// Creates a buffer object from native WebGPU buffer

    /// \param [in] wgpuBuffer    - WebGPU buffer handle
    /// \param [in] BuffDesc      - Buffer description. WebGPU provides no means to retrieve any
    ///                             buffer properties from the buffer handle, so complete buffer
    ///                             description must be provided
    /// \param [in]  InitialState - Initial buffer state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppBuffer     - Address of the memory location where the pointer to the
    ///                             buffer interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created buffer object does not take ownership of the WebGPU buffer and will not
    ///        destroy it once released. The application must not destroy Vulkan buffer while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateBufferFromWebGPUBuffer)(THIS_ 
                                                      WGPUBuffer           wgpuBuffer,
                                                      const BufferDesc REF BuffDesc,
                                                      RESOURCE_STATE       InitialState,
                                                      IBuffer**            ppBuffer) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IRenderDeviceWebGPU_GetWebGPUInstance(This)                    CALL_IFACE_METHOD(RenderDeviceWebGPU, GetWebGPUInstance,              This)
#    define IRenderDeviceWebGPU_GetWebGPUAdapter(This)                     CALL_IFACE_METHOD(RenderDeviceWebGPU, GetWebGPUAdapter,               This)
#    define IRenderDeviceWebGPU_GetWebGPUDevice(This)                      CALL_IFACE_METHOD(RenderDeviceWebGPU, GetWebGPUDevice,                This)
#    define IRenderDeviceWebGPU_CreateTextureFromWebGPUTexture(This, ...)  CALL_IFACE_METHOD(RenderDeviceWebGPU, CreateTextureFromWebGPUTexture, This, __VA_ARGS__)
#    define IRenderDeviceWebGPU_CreateBufferFromWebGPUBuffer(This, ...)    CALL_IFACE_METHOD(RenderDeviceWebGPU, CreateBufferFromWebGPUBuffer,   This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
