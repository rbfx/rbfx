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

#include <vector>
#include <exception>
#include <algorithm>

#include "WinHPreface.h"

#include <d3d12.h>
#include <atlbase.h>

#if USE_D3D12_LOADER
// On Win32 we manually load d3d12.dll and get entry points,
// but UWP does not support this, so we link with d3d12.lib
#    include "D3D12Loader.hpp"
#endif

#include "WinHPostface.h"

#ifndef NTDDI_WIN10_VB // First defined in Win SDK 10.0.19041.0
#    define D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS static_cast<D3D12_INDIRECT_ARGUMENT_TYPE>(D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW + 1)
#    define D3D12_RAYTRACING_TIER_1_1                  static_cast<D3D12_RAYTRACING_TIER>(11)
#    define D3D12_HEAP_FLAG_CREATE_NOT_ZEROED          D3D12_HEAP_FLAG_NONE
#endif

#ifndef NTDDI_WIN10_19H1 // First defined in Win SDK 10.0.18362.0
enum D3D12_SHADING_RATE
{
};
enum D3D12_SHADING_RATE_COMBINER
{
};

constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE = static_cast<D3D12_RESOURCE_STATES>(0x1000000);
#endif

#include "PlatformDefinitions.h"
#include "Errors.hpp"
#include "RefCntAutoPtr.hpp"
#include "DebugUtilities.hpp"
#include "D3DErrors.hpp"
#include "Cast.hpp"
#include "STDAllocator.hpp"
