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
/// Definition of the Diligent::ITextureWebGPU interface

#include "../../GraphicsEngine/interface/Texture.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {A362522B-F9F8-4A5A-967F-DAB2EE2F9C26}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_TextureWebGPU =
    {0xA362522B, 0xF9F8, 0x4A5A, {0x96, 0x7F, 0xDA, 0xB2, 0xEE, 0x2F, 0x9C, 0x26}};

#define DILIGENT_INTERFACE_NAME ITextureWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureWebGPUInclusiveMethods \
    ITextureInclusiveMethods;          \
    ITextureWebGPUMethods TextureWebGPU

// clang-format off

/// Exposes WebGPU-specific functionality of a texture object.
DILIGENT_BEGIN_INTERFACE(ITextureWebGPU, ITexture)
{
    /// Returns WebGPU texture handle.
    /// The application must not destroy the returned texture
    VIRTUAL WGPUTexture METHOD(GetWebGPUTexture)(THIS) CONST PURE;

};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureWebGPU_GetWebGPUTexture(This) CALL_IFACE_METHOD(TextureWebGPU, GetWebGPUTexture, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
