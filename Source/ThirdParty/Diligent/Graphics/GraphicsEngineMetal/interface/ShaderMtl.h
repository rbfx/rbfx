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
/// Definition of the Diligent::IShaderMtl interface

#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {07182C29-CC3B-43B2-99D8-A77F6FECBA82}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_ShaderMtl =
    {0x7182c29, 0xcc3b, 0x43b2, {0x99, 0xd8, 0xa7, 0x7f, 0x6f, 0xec, 0xba, 0x82}};

#define DILIGENT_INTERFACE_NAME IShaderMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderMtlInclusiveMethods \
    IShaderInclusiveMethods;       \
    IShaderMtlMethods ShaderMtl

/// Exposes Metal-specific functionality of a shader object.
DILIGENT_BEGIN_INTERFACE(IShaderMtl, IShader)
{
    /// Returns a point to the Metal shader function (MTLFunction)
    /// compiled without resource remapping.
    ///
    /// \remarks Pipeline state objects that use explicit resource signatures
    ///          remap resources in each shader function to make them
    ///          match the signatures.
    ///          This method returns a pointer the shader function compiled before
    ///          any remapping has been applied.
    VIRTUAL id<MTLFunction> METHOD(GetMtlShaderFunction)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IShaderMtl_GetMtlShaderFunction(This) CALL_IFACE_METHOD(ShaderMtl, GetMtlShaderFunction, This)

#endif


DILIGENT_END_NAMESPACE // namespace Diligent
