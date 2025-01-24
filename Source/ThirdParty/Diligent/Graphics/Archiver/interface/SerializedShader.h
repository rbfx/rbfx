/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Defines Diligent::ISerializedShader interface

#include "../../GraphicsEngine/interface/GraphicsTypes.h"
#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {53A9A017-6A34-4AE9-AA23-C8E587023F9E}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SerializedShader =
    {0x53a9a017, 0x6a34, 0x4ae9, {0xaa, 0x23, 0xc8, 0xe5, 0x87, 0x2, 0x3f, 0x9e}};

#define DILIGENT_INTERFACE_NAME ISerializedShader
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISerializedShaderInclusiveMethods \
    IShaderInclusiveMethods;              \
    ISerializedShaderMethods SerializedShader

// clang-format off

/// Serialized shader interface
DILIGENT_BEGIN_INTERFACE(ISerializedShader, IShader)
{
    /// Returns a device-specific shader for the given render device type.
    ///
    /// \note   In order for the returned shader object to be fully initialized
    ///         and suitable for use in rendering commands, a corresponding render
    ///         device must have been initialized in the serialization device through
    ///         ISerializationDevice::AddRenderDevice().
    VIRTUAL IShader* METHOD(GetDeviceShader)(
        THIS_
        enum RENDER_DEVICE_TYPE DeviceType) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ISerializedShader_GetDeviceShader(This, ...) CALL_IFACE_METHOD(SerializedShader, GetDeviceShader, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
