/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Definition of the Diligent::IPipelineStateCacheVk interface

#include "../../GraphicsEngine/interface/PipelineStateCache.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C866CC1D-A607-4F55-8D5F-FD6D4F0E051C}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_PipelineStateCacheVk =
    {0xc866cc1d, 0xa607, 0x4f55, {0x8d, 0x5f, 0xfd, 0x6d, 0x4f, 0xe, 0x5, 0x1c}};


#define DILIGENT_INTERFACE_NAME IPipelineStateCacheVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateCacheVkInclusiveMethods \
    IPipelineStateCacheInclusiveMethods;      \
    IPipelineStateCacheVkMethods PipelineStateCacheVk

// clang-format off

/// Exposes Vulkan-specific functionality of a pipeline state cache object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateCacheVk, IPipelineStateCache)
{
    /// Returns a Vulkan handle of the internal pipeline cache object.
    VIRTUAL VkPipelineCache METHOD(GetVkPipelineCache)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IPipelineStateCacheVk_GetVkPipelineCache(This)  CALL_IFACE_METHOD(PipelineStateCacheVk, GetVkPipelineCache, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
