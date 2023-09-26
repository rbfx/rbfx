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
/// Defines Diligent::ISerializedPipelineState interface

#include "../../GraphicsEngine/interface/PipelineState.h"
#include "Archiver.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {23DBAA36-B34E-438E-800C-D28C66237361}
static const INTERFACE_ID IID_SerializedPipelineState =
    {0x23dbaa36, 0xb34e, 0x438e, {0x80, 0xc, 0xd2, 0x8c, 0x66, 0x23, 0x73, 0x61}};

#define DILIGENT_INTERFACE_NAME ISerializedPipelineState
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISerializedPipelineStateInclusiveMethods \
    IPipelineStateInclusiveMethods;              \
    ISerializedPipelineStateMethods SerializedPipelineState

// clang-format off

/// Serialized pipeline state interface
DILIGENT_BEGIN_INTERFACE(ISerializedPipelineState, IPipelineState)
{
    /// Returns the number of patched shaders for the given device type.

    /// \param [in] DeviceType  - Device type for which to get the shader count.
    /// \return     The number of patched shaders for this device type.
    VIRTUAL Uint32 METHOD(GetPatchedShaderCount)(
        THIS_
        ARCHIVE_DEVICE_DATA_FLAGS DeviceType) CONST PURE;

    /// Returns the patched shader create information for the given device type and shader index.

    /// \param [in] DeviceType  - Device type for which to get the shader.
    /// \param [in]             - Shader index.
    ///                           The index must be between 0 and the shader count
    ///                           returned by the GetPatchedShaderCount() for this
    ///                           device type.
    /// \return Shader create information for the requested device type and shader index.
    VIRTUAL ShaderCreateInfo METHOD(GetPatchedShaderCreateInfo)(
        THIS_
        ARCHIVE_DEVICE_DATA_FLAGS DeviceType,
        Uint32                    ShaderIndex) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ISerializedPipelineState_GetPatchedShaderCount(This, ...)      CALL_IFACE_METHOD(SerializedPipelineState, GetPatchedShaderCount,      This, __VA_ARGS__)
#    define ISerializedPipelineState_GetPatchedShaderCreateInfo(This, ...) CALL_IFACE_METHOD(SerializedPipelineState, GetPatchedShaderCreateInfo, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
