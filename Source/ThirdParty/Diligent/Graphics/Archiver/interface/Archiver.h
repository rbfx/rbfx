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

#pragma once

/// \file
/// Defines Diligent::IArchiver interface

#include "../../GraphicsEngine/interface/Dearchiver.h"
#include "../../GraphicsEngine/interface/PipelineResourceSignature.h"
#include "../../GraphicsEngine/interface/PipelineState.h"
#include "../../GraphicsEngine/interface/DeviceObjectArchive.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {D8EBEC99-5A44-41A3-968F-1D7127ABEC79}
static const INTERFACE_ID IID_Archiver =
    {0xd8ebec99, 0x5a44, 0x41a3, {0x96, 0x8f, 0x1d, 0x71, 0x27, 0xab, 0xec, 0x79}};

#define DILIGENT_INTERFACE_NAME IArchiver
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IArchiverInclusiveMethods \
    IObjectInclusiveMethods;      \
    IArchiverMethods Archiver

// clang-format off

/// Flags that indicate which device data will be serialized.
DILIGENT_TYPED_ENUM(ARCHIVE_DEVICE_DATA_FLAGS, Uint32)
{
    /// No data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_NONE        = 0u,

    /// Direct3D11 device data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_D3D11       = 1u << RENDER_DEVICE_TYPE_D3D11,

    /// Direct3D12 device data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_D3D12       = 1u << RENDER_DEVICE_TYPE_D3D12,

    /// OpenGL device data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_GL          = 1u << RENDER_DEVICE_TYPE_GL,

    /// OpenGLES device data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_GLES        = 1u << RENDER_DEVICE_TYPE_GLES,

    /// Vulkan device data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_VULKAN      = 1u << RENDER_DEVICE_TYPE_VULKAN,

    /// Metal device data for MacOS will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS = 1u << RENDER_DEVICE_TYPE_METAL,

    /// Metal device data for iOS will be serialized.
    ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS   = 2u << RENDER_DEVICE_TYPE_METAL,

    ARCHIVE_DEVICE_DATA_FLAG_LAST        = ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS
};
DEFINE_FLAG_ENUM_OPERATORS(ARCHIVE_DEVICE_DATA_FLAGS)


/// Render state object archiver interface
DILIGENT_BEGIN_INTERFACE(IArchiver, IObject)
{
    /// Writes an archive to a memory blob

    /// \note
    ///     The method is *not* thread-safe and must not be called from multiple threads simultaneously.
    VIRTUAL Bool METHOD(SerializeToBlob)(THIS_
                                         IDataBlob** ppBlob) PURE;

    /// Writes an archive to a file stream

    /// \note
    ///     The method is *not* thread-safe and must not be called from multiple threads simultaneously.
    VIRTUAL Bool METHOD(SerializeToStream)(THIS_
                                           IFileStream* pStream) PURE;


    /// Adds a pipeline state to the archive.

    /// \note
    ///     Pipeline state must have been created by the serialization device.
    ///
    ///     Multiple pipeline states may be packed into the same archive as long as they use unique names.
    ///     All dependent objects (render pass, resource signatures, shaders) will be added to the archive
    ///     and must also use unique names.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL Bool METHOD(AddPipelineState)(THIS_
                                          IPipelineState* pPSO) PURE;


    /// Adds a pipeline resource signature to the archive.

    /// \note
    ///     Pipeline resource signature must have been created by the serialization device.
    ///
    ///     Multiple PSOs and signatures may be packed into the same archive as long as they use distinct names.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL Bool METHOD(AddPipelineResourceSignature)(THIS_
                                                      IPipelineResourceSignature* pSignature) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IArchiver_SerializeToBlob(This, ...)              CALL_IFACE_METHOD(Archiver, SerializeToBlob,              This, __VA_ARGS__)
#    define IArchiver_SerializeToStream(This, ...)            CALL_IFACE_METHOD(Archiver, SerializeToStream,            This, __VA_ARGS__)
#    define IArchiver_AddPipelineState(This, ...)             CALL_IFACE_METHOD(Archiver, AddPipelineState,             This, __VA_ARGS__)
#    define IArchiver_AddPipelineResourceSignature(This, ...) CALL_IFACE_METHOD(Archiver, AddPipelineResourceSignature, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
