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

#include <string>

namespace
{

namespace MSL
{

// clang-format off
const std::string FillTextureCS{
R"(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

kernel void CSMain(texture2d<float, access::write> g_tex2DUAV            [[texture(0)]],
                   uint3                           gl_GlobalInvocationID [[thread_position_in_grid]])
{
    if (gl_GlobalInvocationID.x < g_tex2DUAV.get_width() &&
        gl_GlobalInvocationID.y < g_tex2DUAV.get_height())
    {
        g_tex2DUAV.write(float4(float2(gl_GlobalInvocationID.xy % uint2(256u)) / 256.0, 0.0, 1.0),
                         uint2(gl_GlobalInvocationID.xy));

    }
}
)"
};
// clang-format on

} // namespace MSL

} // namespace
