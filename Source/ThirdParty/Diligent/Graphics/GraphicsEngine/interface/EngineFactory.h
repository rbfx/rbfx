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

#pragma once

/// \file
/// Defines Diligent::IEngineFactory interface

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/DebugOutput.h"
#include "../../../Primitives/interface/DataBlob.h"
#include "GraphicsTypes.h"


#if PLATFORM_ANDROID
struct ANativeActivity;
struct AAssetManager;
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

struct IShaderSourceInputStreamFactory;
struct IDearchiver;

// {D932B052-4ED6-4729-A532-F31DEEC100F3}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_EngineFactory =
    {0xd932b052, 0x4ed6, 0x4729, {0xa5, 0x32, 0xf3, 0x1d, 0xee, 0xc1, 0x0, 0xf3}};

#define DILIGENT_INTERFACE_NAME IEngineFactory
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IEngineFactoryInclusiveMethods \
    IObjectInclusiveMethods;           \
    IEngineFactoryMethods EngineFactory

// clang-format off


/// Dearchiver create information
struct DearchiverCreateInfo
{
    void* pDummy DEFAULT_INITIALIZER(nullptr);
};
typedef struct DearchiverCreateInfo DearchiverCreateInfo;


/// Engine factory base interface
DILIGENT_BEGIN_INTERFACE(IEngineFactory, IObject)
{
    /// Returns API info structure, see Diligent::APIInfo.
    VIRTUAL const APIInfo REF METHOD(GetAPIInfo)(THIS) CONST PURE;

    /// Creates default shader source input stream factory

    /// \param [in]  SearchDirectories           - Semicolon-separated list of search directories.
    /// \param [out] ppShaderSourceStreamFactory - Memory address where the pointer to the shader source stream factory will be written.
    VIRTUAL void METHOD(CreateDefaultShaderSourceStreamFactory)(
                        THIS_
                        const Char*                              SearchDirectories,
                        struct IShaderSourceInputStreamFactory** ppShaderSourceFactory) CONST PURE;

    /// Creates a data blob.

    /// \param [in]  InitialSize - The size of the internal data buffer.
    /// \param [in]  pData       - Pointer to the data to write to the internal buffer.
    ///                            If null, no data will be written.
    /// \param [out] ppDataBlob  - Memory address where the pointer to the data blob will be written.
    VIRTUAL void METHOD(CreateDataBlob)(THIS_
                                        size_t      InitialSize,
                                        const void* pData,
                                        IDataBlob** ppDataBlob) CONST PURE;

    /// Enumerates adapters available on this machine.

    /// \param [in]     MinVersion  - Minimum required API version (feature level for Direct3D).
    /// \param [in,out] NumAdapters - The number of adapters. If Adapters is null, this value
    ///                               will be overwritten with the number of adapters available
    ///                               on this system. If Adapters is not null, this value should
    ///                               contain the maximum number of elements reserved in the array
    ///                               pointed to by Adapters. In the latter case, this value
    ///                               is overwritten with the actual number of elements written to
    ///                               Adapters.
    /// \param [out]    Adapters - Pointer to the array containing adapter information. If
    ///                            null is provided, the number of available adapters is
    ///                            written to NumAdapters.
    ///
    /// \note OpenGL backend only supports one device; features and properties will have limited information.
    VIRTUAL void METHOD(EnumerateAdapters)(THIS_
                                           Version              MinVersion,
                                           Uint32 REF           NumAdapters,
                                           GraphicsAdapterInfo* Adapters) CONST PURE;

    /// Creates a dearchiver object.

    /// \param [in]  CreateInfo   - Dearchiver create info, see Diligent::DearchiverCreateInfo for details.
    /// \param [out] ppDearchiver - Address of the memory location where a pointer to IDearchiver
    ///                             interface will be written.
    ///                             The function calls AddRef(), so that the new object will have
    ///                             one reference.
    VIRTUAL void METHOD(CreateDearchiver)(THIS_
                                          const DearchiverCreateInfo REF CreateInfo,
                                          struct IDearchiver**           ppDearchiver) CONST PURE;


    /// Sets a user-provided debug message callback.

    /// \param [in]     MessageCallback - Debug message callback function to use instead of the default one.
    VIRTUAL void METHOD(SetMessageCallback)(THIS_
                                            DebugMessageCallbackType MessageCallback) CONST PURE;

    /// Sets whether to break program execution on assertion failure.

    /// \param [in]     BreakOnError - Whether to break on assertion failure.
    VIRTUAL void METHOD(SetBreakOnError)(THIS_
                                         bool BreakOnError) CONST PURE;

#if PLATFORM_ANDROID
    /// On Android platform, it is necessary to initialize the file system before
    /// CreateDefaultShaderSourceStreamFactory() method can be called.

    /// \param [in] AssetManager     - A pointer to the asset manager (AAssetManager).
    /// \param [in] ExternalFilesDir - External files directory.
    /// \param [in] OutputFilesDir   - Output files directory.
    ///
    /// \remarks See AndroidFileSystem::Init.
    VIRTUAL void METHOD(InitAndroidFileSystem)(THIS_
                                               struct AAssetManager* AssetManager,
                                               const char*           ExternalFilesDir DEFAULT_VALUE(nullptr),
                                               const char*           OutputFilesDir   DEFAULT_VALUE(nullptr)) CONST PURE;
#endif
};
DILIGENT_END_INTERFACE

// clang-format on

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IEngineFactory_GetAPIInfo(This)                                  CALL_IFACE_METHOD(EngineFactory, GetAPIInfo,                             This)
#    define IEngineFactory_CreateDefaultShaderSourceStreamFactory(This, ...) CALL_IFACE_METHOD(EngineFactory, CreateDefaultShaderSourceStreamFactory, This, __VA_ARGS__)
#    define IEngineFactory_CreateDataBlob(This, ...)                         CALL_IFACE_METHOD(EngineFactory, CreateDataBlob,                         This, __VA_ARGS__)
#    define IEngineFactory_EnumerateAdapters(This, ...)                      CALL_IFACE_METHOD(EngineFactory, EnumerateAdapters,                      This, __VA_ARGS__)
#    define IEngineFactory_InitAndroidFileSystem(This, ...)                  CALL_IFACE_METHOD(EngineFactory, InitAndroidFileSystem,                  This, __VA_ARGS__)
#    define IEngineFactory_CreateDearchiver(This, ...)                       CALL_IFACE_METHOD(EngineFactory, CreateDearchiver,                       This, __VA_ARGS__)
#    define IEngineFactory_SetMessageCallback(This, ...)                     CALL_IFACE_METHOD(EngineFactory, SetMessageCallback,                     This, __VA_ARGS__)
#    define IEngineFactory_SetBreakOnError(This, ...)                        CALL_IFACE_METHOD(EngineFactory, SetBreakOnError,                        This, __VA_ARGS__)
// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
