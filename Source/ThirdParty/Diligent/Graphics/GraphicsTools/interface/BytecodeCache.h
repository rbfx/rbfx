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
// clang-format off
/// \file
/// Defines Diligent::IBytecodeCache interface
#include "../../GraphicsEngine/interface/RenderDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

struct BytecodeCacheCreateInfo
{
    enum RENDER_DEVICE_TYPE DeviceType DEFAULT_INITIALIZER(RENDER_DEVICE_TYPE_UNDEFINED);
};
typedef struct BytecodeCacheCreateInfo BytecodeCacheCreateInfo;

// clang-format on

// {D1F8295F-F9D7-4CD4-9D13-D950FE7572C1}
static const INTERFACE_ID IID_BytecodeCache = {0xD1F8295F, 0xF9D7, 0x4CD4, {0x9D, 0x13, 0xD9, 0x50, 0xFE, 0x75, 0x72, 0xC1}};

#define DILIGENT_INTERFACE_NAME IBytecodeCache
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBytecodeCacheInclusiveMethods \
    IObjectInclusiveMethods;           \
    IBytecodeCacheMethods BytecodeCache

// clang-format off

/// Byte code cache interface
DILIGENT_BEGIN_INTERFACE(IBytecodeCache, IObject)
{
    /// Loads the cache data from the binary blob
    ///
    /// \param [in] pData - A pointer to the cache data.
    /// \return     true if the data was loaded successfully, and false otherwise.
    VIRTUAL bool METHOD(Load)(THIS_
                              IDataBlob* pData) PURE;

    /// Returns the byte code for the requested shader create parameters.

    /// \param [in]  ShaderCI   - Shader create info to find the byte code for.
    /// \param [out] ppByteCode - Address of the memory location where a pointer to the
    ///                           data blob containing the byte code will be written.
    ///                           The function calls AddRef(), so that the new object will have
    ///                           one reference.
    VIRTUAL void METHOD(GetBytecode)(THIS_
                                     const ShaderCreateInfo REF ShaderCI,
                                     IDataBlob**                ppByteCode) PURE;

    /// Adds the byte code to the cache.

    /// \param [in] ShaderCI  - Shader create parameters for the byte code to add.
    /// \param [in] pByteCode - A pointer to the byte code to add to the cache.
    ///
    /// \remarks    If the byte code for the given shader create parameters is already present
    ///             in the cache, it is replaced.
    VIRTUAL void METHOD(AddBytecode)(THIS_ 
                                     const ShaderCreateInfo REF ShaderCI,
                                     IDataBlob*                 pByteCode) PURE;

    /// Removes the byte code from the cache.

    /// \param [in] ShaderCI - Shader create information for the byte code to remove.
    VIRTUAL void METHOD(RemoveBytecode)(THIS_ 
                                        const ShaderCreateInfo REF ShaderCI) PURE;

    /// Writes the cache data to the binary data blob.

    /// \param [out] ppDataBlob - Address of the memory location where a pointer to the
    ///                           data blob containing the cache data will be written.
    ///                           The function calls AddRef(), so that the new object will have
    ///                           one reference.
    ///
    /// \remarks    The data produced by this method is intended to be used by the Load method.
    VIRTUAL void METHOD(Store)(THIS_
                               IDataBlob** ppDataBlob) PURE;


    /// Clears the cache and resets it to default state.
    VIRTUAL void METHOD(Clear)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off
#    define IBytecodeCache_Load(This, ...)           CALL_IFACE_METHOD(BytecodeCache, Load,           This, __VA_ARGS__)
#    define IBytecodeCache_GetBytecode(This, ...)    CALL_IFACE_METHOD(BytecodeCache, GetBytecode,    This, __VA_ARGS__)
#    define IBytecodeCache_AddBytecode(This, ...)    CALL_IFACE_METHOD(BytecodeCache, AddBytecode,    This, __VA_ARGS__)
#    define IBytecodeCache_RemoveBytecode(This, ...) CALL_IFACE_METHOD(BytecodeCache, RemoveBytecode, This, __VA_ARGS__)
#    define IBytecodeCache_Store(This, ...)          CALL_IFACE_METHOD(BytecodeCache, Store,          This, __VA_ARGS__)
#    define IBytecodeCache_Clear(This)               CALL_IFACE_METHOD(BytecodeCache, Clear,          This)
// clang-format on

#endif

#include "../../../Primitives/interface/DefineGlobalFuncHelperMacros.h"

void DILIGENT_GLOBAL_FUNCTION(CreateBytecodeCache)(const BytecodeCacheCreateInfo REF CreateInfo,
                                                   IBytecodeCache**                  ppCache);

#include "../../../Primitives/interface/UndefGlobalFuncHelperMacros.h"

DILIGENT_END_NAMESPACE // namespace Diligent
