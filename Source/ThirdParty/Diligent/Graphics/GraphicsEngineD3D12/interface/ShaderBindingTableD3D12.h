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

#pragma once

/// \file
/// Definition of the Diligent::IShaderBindingTableD3D12 interface

#include "../../GraphicsEngine/interface/ShaderBindingTable.h"
#include "DeviceContextD3D12.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {DCA2FAD9-2C41-4419-9D16-79731C0ED9D8}
static const INTERFACE_ID IID_ShaderBindingTableD3D12 =
    {0xdca2fad9, 0x2c41, 0x4419, {0x9d, 0x16, 0x79, 0x73, 0x1c, 0xe, 0xd9, 0xd8}};

#define DILIGENT_INTERFACE_NAME IShaderBindingTableD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderBindingTableD3D12InclusiveMethods \
    IShaderBindingTableInclusiveMethods;         \
    IShaderBindingTableD3D12Methods ShaderBindingTableD3D12
// clang-format off


/// Exposes Direct3D12-specific functionality of a shader binding table object.
DILIGENT_BEGIN_INTERFACE(IShaderBindingTableD3D12, IShaderBindingTable)
{
    /// Returns the structure that can be used with ID3D12GraphicsCommandList4::DispatchRays() call.
    ///
    /// \remarks  The method is not thread-safe. An application must externally synchronize the access
    ///           to the shader binding table.
    VIRTUAL const D3D12_DISPATCH_RAYS_DESC REF METHOD(GetD3D12BindingTable)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IShaderBindingTableD3D12_GetD3D12BindingTable(This) CALL_IFACE_METHOD(ShaderBindingTableD3D12, GetD3D12BindingTable, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
