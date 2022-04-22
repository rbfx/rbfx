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

#include "BasicMath.hpp"

namespace Diligent
{

struct PosAndRate
{
    float2 Pos;
    Uint32 Rate;
};

namespace VRSTestingConstants
{

namespace PerPrimitive
{

// clang-format off
static const PosAndRate Vertices[] =
{
    PosAndRate{{-0.9f,  0.0f}, SHADING_RATE_4X4},  PosAndRate{{-0.9f,  0.9f}, SHADING_RATE_4X4},  PosAndRate{{-0.2f,  0.9f}, SHADING_RATE_4X4},
    PosAndRate{{-1.0f, -0.3f}, SHADING_RATE_2X2},  PosAndRate{{ 0.0f,  1.0f}, SHADING_RATE_2X2},  PosAndRate{{ 0.0f, -0.3f}, SHADING_RATE_2X2},
    PosAndRate{{ 0.1f, -0.1f}, SHADING_RATE_2X4},  PosAndRate{{ 0.1f,  0.9f}, SHADING_RATE_2X4},  PosAndRate{{ 0.9f, -0.1f}, SHADING_RATE_2X4},
    PosAndRate{{ 0.1f,  1.0f}, SHADING_RATE_4X2},  PosAndRate{{ 1.0f,  1.0f}, SHADING_RATE_4X2},  PosAndRate{{ 1.0f, -0.1f}, SHADING_RATE_4X2},
    PosAndRate{{-0.9f, -1.0f}, SHADING_RATE_1X1},  PosAndRate{{-0.9f, -0.4f}, SHADING_RATE_1X1},  PosAndRate{{ 0.2f, -0.4f}, SHADING_RATE_1X1},
    PosAndRate{{ 1.0f, -0.1f}, SHADING_RATE_1X2},  PosAndRate{{ 1.0f, -1.0f}, SHADING_RATE_1X2},  PosAndRate{{-0.5f, -1.0f}, SHADING_RATE_1X2}
};
// clang-format on

} // namespace PerPrimitive

namespace TextureBased
{

inline SHADING_RATE GenTexture(Uint32 X, Uint32 Y, Uint32 W, Uint32 H)
{
    auto XDist = std::abs(0.5f - static_cast<float>(X) / W) * 2.0f;
    auto YDist = std::abs(0.5f - static_cast<float>(Y) / H) * 2.0f;

    auto XRate = AXIS_SHADING_RATE_MAX - clamp(static_cast<Uint32>(XDist * AXIS_SHADING_RATE_MAX + 0.5f), 0u, Uint32{AXIS_SHADING_RATE_MAX});
    auto YRate = AXIS_SHADING_RATE_MAX - clamp(static_cast<Uint32>(YDist * AXIS_SHADING_RATE_MAX + 0.5f), 0u, Uint32{AXIS_SHADING_RATE_MAX});

    return static_cast<SHADING_RATE>((XRate << SHADING_RATE_X_SHIFT) | YRate);
}

inline float GenColRowFp32(size_t X, size_t W)
{
    return 1.0f - clamp(std::abs(static_cast<float>(X) / W), 0.f, 1.0f);
}

} // namespace TextureBased

} // namespace VRSTestingConstants

} // namespace Diligent
