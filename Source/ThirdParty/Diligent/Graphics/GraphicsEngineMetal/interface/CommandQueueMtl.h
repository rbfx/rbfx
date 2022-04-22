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
/// Definition of the Diligent::ICommandQueueMtl interface

#include "../../GraphicsEngine/interface/CommandQueue.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {1C0013CB-41B8-453D-8983-4D935F5973B0}
static const INTERFACE_ID IID_CommandQueueMtl =
    {0x1c0013cb, 0x41b8, 0x453d, {0x89, 0x83, 0x4d, 0x93, 0x5f, 0x59, 0x73, 0xb0}};

#define DILIGENT_INTERFACE_NAME ICommandQueueMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ICommandQueueMtlInclusiveMethods \
    ICommandQueueInclusiveMethods;       \
    ICommandQueueMtlMethods CommandQueueMtl

// clang-format off

/// Command queue interface
DILIGENT_BEGIN_INTERFACE(ICommandQueueMtl, ICommandQueue)
{
    /// Returns a pointer to Metal command queue (MTLCommandQueue)
    VIRTUAL id<MTLCommandQueue> METHOD(GetMtlCommandQueue)(THIS) CONST PURE;

    /// Submits a given command buffer to the command queue

    /// \return Fence value associated with the submitted command buffer
    VIRTUAL Uint64 METHOD(Submit)(THIS_
                                  id<MTLCommandBuffer> mtlCommandBuffer) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ICommandQueueMtl_GetMtlCommandQueue(This)  CALL_IFACE_METHOD(CommandQueueMtl, GetMtlCommandQueue, This)
#    define ICommandQueueMtl_Submit(This, ...)         CALL_IFACE_METHOD(CommandQueueMtl, Submit,             This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
