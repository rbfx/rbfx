/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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
/// Definition of the engine constants

#include "../../../Primitives/interface/BasicTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// The maximum number of input buffer slots.
/// D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT == 32
#define DILIGENT_MAX_BUFFER_SLOTS 32

/// The maximum number of simultaneous render targets.
#define DILIGENT_MAX_RENDER_TARGETS 8

/// The maximum number of viewports.
#define DILIGENT_MAX_VIEWPORTS 16

/// The maximum number of resource signatures that one pipeline can use
#define DILIGENT_MAX_RESOURCE_SIGNATURES 8

/// The maximum number of queues in graphics adapter description.
#define DILIGENT_MAX_ADAPTER_QUEUES 16

/// Special constant for the default adapter index.
#define DILIGENT_DEFAULT_ADAPTER_ID 0xFFFFFFFFU

/// Special constant for the default queue index.
#define DILIGENT_DEFAULT_QUEUE_ID 0xFF

/// The maximum number of shading rate modes.
#define DILIGENT_MAX_SHADING_RATES 9

/// Bit shift for the the shading X-axis rate.
#define DILIGENT_SHADING_RATE_X_SHIFT 2

static const Uint32 MAX_BUFFER_SLOTS        = DILIGENT_MAX_BUFFER_SLOTS;
static const Uint32 MAX_RENDER_TARGETS      = DILIGENT_MAX_RENDER_TARGETS;
static const Uint32 MAX_VIEWPORTS           = DILIGENT_MAX_VIEWPORTS;
static const Uint32 MAX_RESOURCE_SIGNATURES = DILIGENT_MAX_RESOURCE_SIGNATURES;
static const Uint32 MAX_ADAPTER_QUEUES      = DILIGENT_MAX_ADAPTER_QUEUES;
static const Uint32 DEFAULT_ADAPTER_ID      = DILIGENT_DEFAULT_ADAPTER_ID;
static const Uint8  DEFAULT_QUEUE_ID        = DILIGENT_DEFAULT_QUEUE_ID;
static const Uint32 MAX_SHADING_RATES       = DILIGENT_MAX_SHADING_RATES;
static const Uint32 SHADING_RATE_X_SHIFT    = DILIGENT_SHADING_RATE_X_SHIFT;

DILIGENT_END_NAMESPACE // namespace Diligent
