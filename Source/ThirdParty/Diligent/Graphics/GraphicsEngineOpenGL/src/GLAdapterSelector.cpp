/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include <cinttypes>

#if PLATFORM_WIN32
#    define DILIGENT_INTERFACE_EXPORT __declspec(dllexport)
#elif PLATFORM_LINUX
#    define DILIGENT_INTERFACE_EXPORT __attribute__((visibility("default")))
#else
#    define DILIGENT_INTERFACE_EXPORT
#endif

/// Let the NVIDIA and AMD know we want to use their graphics card on a dual graphics card system.
extern "C"
{
    /// https://community.amd.com/thread/169965
    DILIGENT_INTERFACE_EXPORT uint64_t AmdPowerXpressRequestHighPerformance = 0x0;

    /// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
    DILIGENT_INTERFACE_EXPORT uint64_t NvOptimusEnablement = 0x0;
}
