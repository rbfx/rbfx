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

const std::string RasterRateMapTest_Pass1{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

struct Vertex
{
    float PosX;
    float PosY;
    uint  Rate; // ignored
};

struct VSOut
{
    float4 Pos   [[position]];
    float4 Color [[user(locn0)]];
};

vertex
VSOut VSmain(             uint    VertexId    [[vertex_id]],
             const device Vertex* g_Vertices  [[buffer(30)]] )
{
    Vertex vert = g_Vertices[VertexId];
    VSOut      out;
    out.Pos   = float4(vert.PosX, vert.PosY, 0.0, 1.0);
    out.Color = float4(vert.Rate * 0.1);
    return out;
}

fragment
float4 PSmain(VSOut in [[stage_in]] )
{
    return in.Color;
}
)msl"};


const std::string RasterRateMapTest_Pass2{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

struct VSOut
{
    float4 Pos [[position]];
    float2 UV;
};

vertex
VSOut VSmain(uint VertexId [[vertex_id]])
{
    // generate fullscreen triangle
    float2 uv = float2(VertexId >> 1, VertexId & 1) * 2.0;
    VSOut  out;
    out.Pos = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    out.UV  = float2(uv.x, 1.0 - uv.y);
    return out;
}

float4 ShadingRateToColor(float Factor)
{
    float  h   = Factor / 1.35;
    float3 col = float3(abs(h * 6.0 - 3.0) - 1.0, 2.0 - abs(h * 6.0 - 2.0), 2.0 - abs(h * 6.0 - 4.0));
    return float4(clamp(col, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0)), 1.0);
}

fragment
float4 PSmain(         VSOut                        in          [[stage_in]],
              constant rasterization_rate_map_data& g_RRMData   [[buffer(0)]],
                       texture2d<float>             g_Texture   [[texture(0)]] )
{
    constexpr sampler readSampler(coord::pixel, address::clamp_to_zero, filter::linear);

    rasterization_rate_map_decoder Decoder(g_RRMData);

    float2 uv        = in.Pos.xy;
    float2 ScreenPos = Decoder.map_screen_to_physical_coordinates(uv);
    float4 RTCol     = float4(g_Texture.sample(readSampler, ScreenPos));

    float  dx    = Decoder.map_screen_to_physical_coordinates(uv - float2(2,0)).x - Decoder.map_screen_to_physical_coordinates(uv + float2(2,0)).x;
    float  dy    = Decoder.map_screen_to_physical_coordinates(uv - float2(0,2)).y - Decoder.map_screen_to_physical_coordinates(uv + float2(0,2)).y;
    float4 SRCol = ShadingRateToColor(clamp(1.0 / (dx * dy), 0.0, 1.0));
    return (RTCol + SRCol) * 0.5;
}
)msl"};


} // namespace MSL

} // namespace
