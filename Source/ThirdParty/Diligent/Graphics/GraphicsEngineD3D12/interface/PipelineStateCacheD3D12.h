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
/// Definition of the Diligent::IPipelineStateCacheD3D12 interface

#include "../../GraphicsEngine/interface/PipelineStateCache.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {47B8CD9A-9156-4209-B133-D7D10D2DD218}
static const INTERFACE_ID IID_PipelineStateCacheD3D12 =
    {0x47b8cd9a, 0x9156, 0x4209, {0xb1, 0x33, 0xd7, 0xd1, 0xd, 0x2d, 0xd2, 0x18}};

#define DILIGENT_INTERFACE_NAME IPipelineStateCacheD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateCacheD3D12InclusiveMethods \
    IPipelineStateCacheInclusiveMethods;         \
    IPipelineStateCacheD3D12Methods PipelineStateCacheD3D12

// clang-format off

/// Exposes Direct3D12-specific functionality of a pipeline state cache object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateCacheD3D12, IPipelineStateCache)
{
    // AZ TODO
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// AZ TODO

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
