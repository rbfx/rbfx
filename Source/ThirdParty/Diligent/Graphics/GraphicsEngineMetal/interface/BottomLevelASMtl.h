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
/// Definition of the Diligent::IBottomLevelASMtl interface

#include "../../GraphicsEngine/interface/BottomLevelAS.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {1D88A872-92F1-46D2-9D70-C31E78E42048}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_BottomLevelASMtl =
    {0x1d88a872, 0x92f1, 0x46d2, {0x9d, 0x70, 0xc3, 0x1e, 0x78, 0xe4, 0x20, 0x48}};

#define DILIGENT_INTERFACE_NAME IBottomLevelASMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBottomLevelASMtlInclusiveMethods \
    IBottomLevelASInclusiveMethods;       \
    IBottomLevelASMtlMethods BottomLevelASMtl

// clang-format off

/// Exposes Metal-specific functionality of a bottom-level acceleration structure object.
DILIGENT_BEGIN_INTERFACE(IBottomLevelASMtl, IBottomLevelAS)
{
    /// Returns a pointer to a Metal acceleration structure object.
    VIRTUAL id<MTLAccelerationStructure> METHOD(GetMtlAccelerationStructure)(THIS) CONST API_AVAILABLE(ios(14), macosx(11.0)) API_UNAVAILABLE(tvos) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBottomLevelASMtl_GetMtlAccelerationStructure(This) CALL_IFACE_METHOD(BottomLevelASMtl, GetMtlAccelerationStructure, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
