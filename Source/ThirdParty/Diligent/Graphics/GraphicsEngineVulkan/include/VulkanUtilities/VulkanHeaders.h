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

#if DILIGENT_USE_VOLK
#    define VK_NO_PROTOTYPES
#endif

#if PLATFORM_WIN32
#    include "WinHPreface.h"
#endif

#define VK_ENABLE_BETA_EXTENSIONS
#include "vulkan/vulkan.h"

#define VK_FORMAT_RANGE_SIZE (VK_FORMAT_ASTC_12x12_SRGB_BLOCK - VK_FORMAT_UNDEFINED + 1)

#if DILIGENT_USE_VOLK
#    include "volk.h"
#endif

#if PLATFORM_WIN32
#    include "WinHPostface.h"
#endif

#ifdef _WINBASE_
#    undef CreateSemaphore
#    undef MemoryBarrier
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(_X11_XLIB_H_)

// Undef symbols defined by XLib
#    ifdef Bool
#        undef Bool
#    endif
#    ifdef True
#        undef True
#    endif
#    ifdef False
#        undef False
#    endif

#endif

namespace VulkanUtilities
{

constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_ALL_SHADERS =
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV |
    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;

constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_ALL_TRANSFER =
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_TRANSFER_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_HOST_BIT |
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

} // namespace VulkanUtilities
