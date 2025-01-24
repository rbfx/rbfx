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
/// Definition of the Diligent::ITextureViewWebGPU interface

#include "../../GraphicsEngine/interface/TextureView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {241F05D0-70B5-4722-AF35-43B875822BD5}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_TextureViewWebGPU =
    {0x241F05D0, 0x70B5, 0x4722, {0xAF, 0x35, 0x43, 0xB8, 0x75, 0x82, 0x2D, 0xD5}};

#define DILIGENT_INTERFACE_NAME ITextureViewWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureViewWebGPUInclusiveMethods \
    ITextureViewInclusiveMethods;          \
    ITextureViewWebGPUMethods TextureViewWebGPU

// clang-format off

/// Exposes WebGPU-specific functionality of a texture view object.
DILIGENT_BEGIN_INTERFACE(ITextureViewWebGPU, ITextureView)
{
    /// Returns WebGPU texture view handle
    VIRTUAL WGPUTextureView METHOD(GetWebGPUTextureView)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureViewWebGPU_GetWebGPUTextureView(This) CALL_IFACE_METHOD(TextureViewWebGPU, GetWebGPUTextureView, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
