/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
/// Definition of the Diligent::ITextureViewMtl interface

#include "../../GraphicsEngine/interface/TextureView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {94C0D9C3-61E7-4358-AB9F-066EAD84D6F1}
static const INTERFACE_ID IID_TextureViewMtl =
    {0x94c0d9c3, 0x61e7, 0x4358, {0xab, 0x9f, 0x6, 0x6e, 0xad, 0x84, 0xd6, 0xf1}};

#define DILIGENT_INTERFACE_NAME ITextureViewMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureViewMtlInclusiveMethods \
    ITextureViewInclusiveMethods;       \
    ITextureViewMtlMethods TextureViewMtl

// clang-format off

/// Exposes Metal-specific functionality of a texture view object.
DILIGENT_BEGIN_INTERFACE(ITextureViewMtl, ITextureView)
{
    /// Returns a pointer to Metal texture view (MTLTexture)
    VIRTUAL id<MTLTexture> METHOD(GetMtlTexture)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureViewMtl_GetMtlTexture(This) CALL_IFACE_METHOD(TextureViewMtl, GetMtlTexture, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
