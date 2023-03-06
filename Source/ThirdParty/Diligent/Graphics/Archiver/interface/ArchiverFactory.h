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
/// Defines Diligent::IArchiverFactory interface and related structures.

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/DebugOutput.h"
#include "SerializationDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {F20B91EB-BDE3-4615-81CC-F720AA32410E}
static const INTERFACE_ID IID_ArchiverFactory =
    {0xf20b91eb, 0xbde3, 0x4615, {0x81, 0xcc, 0xf7, 0x20, 0xaa, 0x32, 0x41, 0xe}};

#define DILIGENT_INTERFACE_NAME IArchiverFactory
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IArchiverFactoryInclusiveMethods \
    IObjectInclusiveMethods;             \
    IArchiverFactoryMethods ArchiverFactory

// clang-format off

/// Serialization device attributes for Direct3D11 backend
struct SerializationDeviceD3D11Info
{
    /// Direct3D11 feature level
    Version FeatureLevel DEFAULT_INITIALIZER(Version(11, 0));

#if DILIGENT_CPP_INTERFACE
    /// Tests if two structures are equivalent
    constexpr bool operator==(const SerializationDeviceD3D11Info& RHS) const
    {
        return FeatureLevel == RHS.FeatureLevel;
    }
    constexpr bool operator!=(const SerializationDeviceD3D11Info& RHS) const
    {
        return !(*this == RHS);
    }
#endif

};
typedef struct SerializationDeviceD3D11Info SerializationDeviceD3D11Info;


/// Serialization device attributes for Direct3D12 backend
struct SerializationDeviceD3D12Info
{
    /// Shader version supported by the device
    Version     ShaderVersion  DEFAULT_INITIALIZER(Version(6, 0));

    /// DX Compiler path
    const Char* DxCompilerPath DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    /// Tests if two structures are equivalent
    bool operator==(const SerializationDeviceD3D12Info& RHS) const noexcept
    {
        return ShaderVersion == RHS.ShaderVersion && SafeStrEqual(DxCompilerPath, RHS.DxCompilerPath);
    }
    bool operator!=(const SerializationDeviceD3D12Info& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif

};
typedef struct SerializationDeviceD3D12Info SerializationDeviceD3D12Info;


/// Serialization device attributes for Vulkan backend
struct SerializationDeviceVkInfo
{
    /// Vulkan API version
    Version     ApiVersion      DEFAULT_INITIALIZER(Version(1, 0));

    /// Indicates whether the device supports SPIRV 1.4 or above
    Bool        SupportsSpirv14 DEFAULT_INITIALIZER(False);

    /// Path to DX compiler for Vulkan
    const Char* DxCompilerPath  DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    /// Tests if two structures are equivalent
    bool operator==(const SerializationDeviceVkInfo& RHS) const noexcept
    {
        return ApiVersion      == RHS.ApiVersion &&
               SupportsSpirv14 == RHS.SupportsSpirv14 &&
               SafeStrEqual(DxCompilerPath, RHS.DxCompilerPath);
    }
    bool operator!=(const SerializationDeviceVkInfo& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif

};
typedef struct SerializationDeviceVkInfo SerializationDeviceVkInfo;


/// Serialization device attributes for Metal backend
struct SerializationDeviceMtlInfo
{
    /// Additional compilation options for Metal command-line compiler for MacOS.
    const Char* CompileOptionsMacOS DEFAULT_INITIALIZER("-sdk macosx metal");

    /// Additional compilation options for Metal command-line compiler for iOS.
    const Char* CompileOptionsiOS   DEFAULT_INITIALIZER("-sdk iphoneos metal");

    /// Name of the command-line application that is used to preprocess Metal shader source before compiling to bytecode.
    const Char* MslPreprocessorCmd DEFAULT_INITIALIZER(nullptr);

    /// Optional directory to dump converted MSL source code and temporary files produced by the Metal toolchain.
    const Char* DumpDirectory      DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    /// Tests if two structures are equivalent
    bool operator==(const SerializationDeviceMtlInfo& RHS) const noexcept
    {
        return SafeStrEqual(CompileOptionsMacOS, RHS.CompileOptionsMacOS) &&
               SafeStrEqual(CompileOptionsiOS,   RHS.CompileOptionsiOS)   &&
               SafeStrEqual(MslPreprocessorCmd,  RHS.MslPreprocessorCmd);
    }
    bool operator!=(const SerializationDeviceMtlInfo& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct SerializationDeviceMtlInfo SerializationDeviceMtlInfo;


/// Serialization device creation information
struct SerializationDeviceCreateInfo
{
    /// Device info, contains enabled device features.
    /// Can be used to validate shader, render pass, resource signature and pipeline state.
    ///
    /// \note For OpenGL that does not support separable programs, disable the SeparablePrograms feature.
    RenderDeviceInfo    DeviceInfo;

    /// Adapter info, contains device parameters.
    /// Can be used to validate shader, render pass, resource signature and pipeline state.
    GraphicsAdapterInfo AdapterInfo;

    /// Direct3D11 attributes, see Diligent::SerializationDeviceD3D11Info.
    SerializationDeviceD3D11Info D3D11;

    /// Direct3D12 attributes, see Diligent::SerializationDeviceD3D12Info.
    SerializationDeviceD3D12Info D3D12;

    /// Vulkan attributes, see Diligent::SerializationDeviceVkInfo.
    SerializationDeviceVkInfo Vulkan;

    /// Metal attributes, see Diligent::SerializationDeviceMtlInfo.
    SerializationDeviceMtlInfo Metal;

#if DILIGENT_CPP_INTERFACE
    SerializationDeviceCreateInfo() noexcept
    {
        DeviceInfo.Features  = DeviceFeatures{DEVICE_FEATURE_STATE_ENABLED};
        AdapterInfo.Features = DeviceFeatures{DEVICE_FEATURE_STATE_ENABLED};
        // Disable subpass framebuffer fetch by default to allow backwards compatibility on Metal.
        DeviceInfo.Features.SubpassFramebufferFetch = DEVICE_FEATURE_STATE_DISABLED;
    }
#endif
};
typedef struct SerializationDeviceCreateInfo SerializationDeviceCreateInfo;


/// Archiver factory interface
DILIGENT_BEGIN_INTERFACE(IArchiverFactory, IObject)
{
    /// Creates a serialization device.

    /// \param [in]  CreateInfo - Serialization device create information, see Diligent::SerializationDeviceCreateInfo.
    /// \param [out] ppDevice   - Address of the memory location where a pointer to the
    ///                           device interface will be written.
    VIRTUAL void METHOD(CreateSerializationDevice)(THIS_
                                                   const SerializationDeviceCreateInfo REF CreateInfo,
                                                   ISerializationDevice**                  ppDevice) PURE;

    /// Creates an archiver.

    /// \param [in]  pDevice    - Pointer to the serialization device.
    /// \param [out] ppArchiver - Address of the memory location where a pointer to the
    ///                           archiver interface will be written.
    VIRTUAL void METHOD(CreateArchiver)(THIS_
                                        ISerializationDevice* pDevice,
                                        IArchiver**           ppArchiver) PURE;

    /// Creates a default shader source input stream factory

    /// \param [in]  SearchDirectories           - Semicolon-separated list of search directories.
    /// \param [out] ppShaderSourceStreamFactory - Memory address where a pointer to the shader source
    ///                                            stream factory will be written.
    VIRTUAL void METHOD(CreateDefaultShaderSourceStreamFactory)(
                        THIS_
                        const Char*                              SearchDirectories,
                        struct IShaderSourceInputStreamFactory** ppShaderSourceFactory) CONST PURE;


    /// Removes device-specific data from the archive and writes a new archive to the stream.

    /// \param [in]  pSrcArchive  - Source archive from which device specific-data will be removed.
    /// \param [in]  DeviceFlags  - Combination of device types that will be removed.
    /// \param [out] ppDstArchive - Memory address where a pointer to the new archive will be written.
    /// \return     true if the device-specific data was successfully removed, and false otherwise.
    VIRTUAL Bool METHOD(RemoveDeviceData)(THIS_
                                          const IDataBlob*          pSrcArchive,
                                          ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
                                          IDataBlob**               ppDstArchive) CONST PURE;


    /// Copies device-specific data from the source archive to the destination and writes a new archive to the stream.

    /// \param [in]  pSrcArchive    - Source archive to which new device-specific data will be added.
    /// \param [in]  DeviceFlags    - Combination of device types that will be copied.
    /// \param [in]  pDeviceArchive - Archive that contains the same common data and additional device-specific data.
    /// \param [out] ppDstArchive   - Memory address where a pointer to the new archive will be written.
    /// \return     true if the device-specific data was successfully added, and false otherwise.
    VIRTUAL Bool METHOD(AppendDeviceData)(THIS_
                                          const IDataBlob*          pSrcArchive,
                                          ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
                                          const IDataBlob*          pDeviceArchive,
                                          IDataBlob**               ppDstArchive) CONST PURE;


    /// Merges multiple archives into one.

    /// \param [in]  ppSrcArchives   - An array of pointers to the source archives.
    /// \param [in]  NumSrcArchives  - The number of elements in ppArchives array.
    /// \param [out] ppDstArchive    - Memory address where a pointer to the merged archive will be written.
    /// \return     true if the archives were successfully merged, and false otherwise.
    VIRTUAL Bool METHOD(MergeArchives)(THIS_
                                       const IDataBlob* ppSrcArchives[],
                                       Uint32           NumSrcArchives,
                                       IDataBlob**      ppDstArchive) CONST PURE;


    /// Prints archive content for debugging and validation.
    VIRTUAL Bool METHOD(PrintArchiveContent)(THIS_
                                             const IDataBlob* pArchive) CONST PURE;

    /// Sets a user-provided debug message callback.

    /// \param [in]     MessageCallback - Debug message callback function to use instead of the default one.
    VIRTUAL void METHOD(SetMessageCallback)(THIS_
                                            DebugMessageCallbackType MessageCallback) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IArchiverFactory_CreateArchiver(This, ...)                          CALL_IFACE_METHOD(ArchiverFactory, CreateArchiver,                         This, __VA_ARGS__)
#    define IArchiverFactory_CreateSerializationDevice(This, ...)               CALL_IFACE_METHOD(ArchiverFactory, CreateSerializationDevice,              This, __VA_ARGS__)
#    define IArchiverFactory_CreateDefaultShaderSourceStreamFactory(This, ...)  CALL_IFACE_METHOD(ArchiverFactory, CreateDefaultShaderSourceStreamFactory, This, __VA_ARGS__)
#    define IArchiverFactory_RemoveDeviceData(This, ...)                        CALL_IFACE_METHOD(ArchiverFactory, RemoveDeviceData,                       This, __VA_ARGS__)
#    define IArchiverFactory_AppendDeviceData(This, ...)                        CALL_IFACE_METHOD(ArchiverFactory, AppendDeviceData,                       This, __VA_ARGS__)
#    define IArchiverFactory_MergeArchives(This, ...)                           CALL_IFACE_METHOD(ArchiverFactory, MergeArchives,                          This, __VA_ARGS__)
#    define IArchiverFactory_PrintArchiveContent(This, ...)                     CALL_IFACE_METHOD(ArchiverFactory, PrintArchiveContent,                    This, __VA_ARGS__)
#    define IArchiverFactory_SetMessageCallback(This, ...)                      CALL_IFACE_METHOD(ArchiverFactory, SetMessageCallback,                     This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
