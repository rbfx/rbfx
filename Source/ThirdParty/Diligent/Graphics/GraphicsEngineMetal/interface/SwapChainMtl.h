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
/// Definition of the Diligent::ISwapChainMtl interface

#include "../../GraphicsEngine/interface/SwapChain.h"
#include "TextureViewMtl.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {8ACDD0D9-FF1C-4A78-9866-924459A0D456}
static const INTERFACE_ID IID_SwapChainMtl =
    {0x8acdd0d9, 0xff1c, 0x4a78, {0x98, 0x66, 0x92, 0x44, 0x59, 0xa0, 0xd4, 0x56}};

#define DILIGENT_INTERFACE_NAME ISwapChainMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainMtlInclusiveMethods \
    ISwapChainInclusiveMethods
//ISwapChainMtlMethods SwapChainMtl

#if DILIGENT_CPP_INTERFACE

/// Exposes Metal-specific functionality of a swap chain.
DILIGENT_BEGIN_INTERFACE(ISwapChainMtl, ISwapChain){};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

typedef struct ISwapChainMtlVtbl
{
    ISwapChainMtlInclusiveMethods;
} ISwapChainMtlVtbl;

#endif


DILIGENT_END_NAMESPACE // namespace Diligent
