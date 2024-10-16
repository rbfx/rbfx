/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Definition of the Diligent::ITextureMtl interface

#include "../../GraphicsEngine/interface/Texture.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {D3A85032-224D-45E5-9825-3AABD61A5EA5}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_TextureMtl =
    {0xd3a85032, 0x224d, 0x45e5, {0x98, 0x25, 0x3a, 0xab, 0xd6, 0x1a, 0x5e, 0xa5}};

#define DILIGENT_INTERFACE_NAME ITextureMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureMtlInclusiveMethods \
    ITextureInclusiveMethods;       \
    ITextureMtlMethods TextureMtl

// clang-format off

/// Exposes Metal-specific functionality of a texture object.
DILIGENT_BEGIN_INTERFACE(ITextureMtl, ITexture)
{
    /// Returns a pointer to a Metal resource.
    /// For a staging texture, this will be a pointer to a MTLStorageModeShared buffer (MTLBuffer).
    /// For all other texture types, this will be a pointer to Metal texture object (MTLTexture).
    VIRTUAL id<MTLResource> METHOD(GetMtlResource)(THIS) CONST PURE;

    /// Returns a pointer to the Metal heap where the texture is allocated.
    VIRTUAL id<MTLHeap> METHOD(GetMtlHeap)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureMtl_GetMtlResource(This) CALL_IFACE_METHOD(TextureMtl, GetMtlResource, This)
#    define ITextureMtl_GetMtlHeap(This)     CALL_IFACE_METHOD(TextureMtl, GetMtlHeap,     This)

// clang-format on
#endif

DILIGENT_END_NAMESPACE // namespace Diligent
