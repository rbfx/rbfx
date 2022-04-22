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

const std::string TileShaderTest1{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct VSOut
{
    float3 Color    [[user(locn0)]];
    float4 Position [[position]];
};

struct FSOut
{
    float4 Color [[color(0)]];
};

vertex VSOut VSmain(uint VertexId [[vertex_id]])
{
    float2 uv  = float2(VertexId & 1, VertexId >> 1);
    VSOut  out = {};
    out.Position = float4((uv * 2.f - 1.f) * 0.9f, 0.0f, 1.0f);
    out.Color    = float3(VertexId & 1, VertexId >> 1, VertexId >> 2);
    return out;
}

fragment FSOut PSmain(VSOut in [[stage_in]])
{
    FSOut out = {float4(in.Color.rgb, 1.0)};
    return out;
}

kernel void TLSmain(imageblock<FSOut> Attachments,
                    ushort2           TileCoord [[ thread_position_in_threadgroup ]],
                    uint              QuadId    [[ thread_index_in_quadgroup ]],
                    uint2             GroupId   [[ threadgroup_position_in_grid ]],
                    ushort2           BlockDim  [[ threads_per_threadgroup ]] )
{
    for (ushort y = 0; y < Attachments.get_height(); ++y)
    {
        for (ushort x = 0; x < Attachments.get_width(); ++x)
        {
            FSOut att = Attachments.read(ushort2(x, y));
            att.Color = 1.0 - att.Color;
            att.Color.r += float(GroupId.x & 1) * 0.2f;
            att.Color.g += float(GroupId.y & 1) * 0.2f;
            att.Color.a = 1.0;
            Attachments.write(att, ushort2(x, y));
        }
    }
}
)msl"};

// clang-format on

} // namespace MSL

} // namespace
