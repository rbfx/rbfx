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

const std::string DrawTestFunctions{
R"(

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct VSOut
{
    float3 Color [[user(locn0)]];
    float4 Position [[position]];
};

vertex VSOut TrisVS(uint VertexId [[vertex_id]])
{
    float4 Pos[6] =
    {
        float4(-1.0, -0.5, 0.0, 1.0),
        float4(-0.5, 0.5, 0.0, 1.0),
        float4(0.0, -0.5, 0.0, 1.0),
        float4(0.0, -0.5, 0.0, 1.0),
        float4(0.5, 0.5, 0.0, 1.0),
        float4(1.0, -0.5, 0.0, 1.0)
    };
    float3 Col[6] =
    {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0),
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VSOut out = {};
    out.Position = Pos[VertexId];
    out.Color    = Col[VertexId];
    return out;
}

struct FSOut
{
    float4 Color [[color(0)]];
};

fragment FSOut TrisFS(VSOut in [[stage_in]])
{
    FSOut out = {float4(in.Color.rgb, 1.0)};
    return out;
}

float4 ComputeColor(float3 Color, float4 Input)
{
    Color.rgb *= 0.125;
    Color.rgb += (float3(1.0, 1.0, 1.0) - Input.brg) * 0.875;
    return float4(Color.rgb, 1.0);
}

fragment FSOut InptAttFS(VSOut            in           [[stage_in]],
                         texture2d<float> SubpassInput [[texture(0)]])
{
    FSOut out;
    out.Color = ComputeColor(in.Color, SubpassInput.read(uint2(in.Position.xy)));
    return out;
}


#if __METAL_VERSION__ >= 230

struct FSOut1
{
    float4 Color [[color(1)]];
};

fragment FSOut1 InptAttFetchFS(VSOut  in           [[stage_in]],
                               float4 SubpassInput [[color(0)]])
{
    FSOut1 out;
    out.Color = ComputeColor(in.Color, SubpassInput);
    return out;
}

#endif

)"
};

// clang-format on

} // namespace MSL

} // namespace
