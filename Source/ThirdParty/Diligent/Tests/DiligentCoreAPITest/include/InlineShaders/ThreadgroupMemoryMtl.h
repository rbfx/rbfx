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

#include <string>

namespace
{

namespace MSL
{
// clang-format off

const std::string SetComputeThreadgroupMemoryLength_CS{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

kernel void CSmain(threadgroup float4*              groupPixels  [[threadgroup(0)]],
                    texture2d<float, access::write> g_OutImage   [[texture(0)]],
                    uint2                           globalId     [[thread_position_in_grid]],
                    uint2                           globalSize   [[threads_per_grid]],
                    uint2                           localId      [[thread_position_in_threadgroup]],
                    uint2                           localSize    [[dispatch_threads_per_threadgroup]] )
{
    if (globalId.x >= g_OutImage.get_width() || globalId.y >= g_OutImage.get_height())
        return;

    // pass 1
    {
        float2 uv  = float2(globalId) / float2(globalSize) * 10.0;
        float4 col = float4(1.0);
        float4 t   = float4(1.2f, 0.25f, 1.1f, 0.14f);

        col.r = sin(uv.x + t.x) * cos(uv.y + t.y);
        col.g = fract(uv.x + t.z) * fract(uv.y + t.w);

        uint idx = localId.x + localId.y * localSize.x;
        groupPixels[idx] = col;
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // pass 2
    {
        float4 col = float4(0.0);
        col += groupPixels[ ((localId.x + 0) % localSize.x) + ((localId.y + 0) % localSize.y) * localSize.x ];
        col += groupPixels[ ((localId.x - 1) % localSize.x) + ((localId.y + 1) % localSize.y) * localSize.x ];
        col += groupPixels[ ((localId.x - 1) % localSize.x) + ((localId.y - 1) % localSize.y) * localSize.x ];
        col += groupPixels[ ((localId.x + 1) % localSize.x) + ((localId.y - 1) % localSize.y) * localSize.x ];
        col += groupPixels[ ((localId.x + 1) % localSize.x) + ((localId.y + 1) % localSize.y) * localSize.x ];
        col /= 5.0;

        g_OutImage.write(col, globalId);
    }
}
)msl"};



// clang-format on

} // namespace MSL

} // namespace
