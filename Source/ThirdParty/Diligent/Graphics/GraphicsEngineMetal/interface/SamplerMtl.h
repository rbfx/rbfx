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
/// Definition of the Diligent::ISamplerMtl interface

#include "../../GraphicsEngine/interface/Sampler.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {73F8C099-049B-4C81-AD19-C98963AC7FEB}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SamplerMtl =
    {0x73f8c099, 0x49b, 0x4c81, {0xad, 0x19, 0xc9, 0x89, 0x63, 0xac, 0x7f, 0xeb}};

#define DILIGENT_INTERFACE_NAME ISamplerMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISamplerMtlInclusiveMethods \
    ISamplerInclusiveMethods;       \
    ISamplerMtlMethods SamplerMtl

/// Exposes Metal-specific functionality of a sampler object.
DILIGENT_BEGIN_INTERFACE(ISamplerMtl, ISampler)
{
    VIRTUAL id<MTLSamplerState> METHOD(GetMtlSampler)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ISamplerMtl_GetMtlSampler(This) CALL_IFACE_METHOD(SamplerMtl, GetMtlSampler, This)

#endif


DILIGENT_END_NAMESPACE // namespace Diligent
