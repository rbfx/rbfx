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
/// Definition of the Diligent::IPipelineStateCacheMtl interface

#include "../../GraphicsEngine/interface/PipelineStateCache.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {03CF61A7-BBAF-47E6-8E09-A9BCAD701A27}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_PipelineStateCacheMtl =
    {0x3cf61a7, 0xbbaf, 0x47e6, {0x8e, 0x9, 0xa9, 0xbc, 0xad, 0x70, 0x1a, 0x27}};


#define DILIGENT_INTERFACE_NAME IPipelineStateCacheMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateCacheMtlInclusiveMethods \
    IPipelineStateCacheInclusiveMethods;       \
    IPipelineStateCacheMtlMethods PipelineStateCacheMtl

// clang-format off

/// Exposes Metal-specific functionality of a pipeline state cache object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateCacheMtl, IPipelineStateCache)
{
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE


#endif

DILIGENT_END_NAMESPACE // namespace Diligent
