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

const std::string RayTracingTest8_CS{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_raytracing>

using namespace metal;
using namespace raytracing;

float4 HitShader(float2 attrBarycentrics)
{
    float3 barycentrics = float3(1.0 - attrBarycentrics.x - attrBarycentrics.y, attrBarycentrics.x, attrBarycentrics.y);
    return float4(barycentrics, 1.0);
}

float4 MissShader()
{
    return float4(1.0, 0.0, 0.0, 1.0);
}

[[kernel]]
void CSMain(uint2						     DTid		    [[thread_position_in_grid]],
            texture2d<float, access::write>	 g_ColorBuffer  [[texture(0)]],
            instance_acceleration_structure	 g_TLAS         [[buffer(0)]])
{
    if (DTid.x >= g_ColorBuffer.get_width() || DTid.y >= g_ColorBuffer.get_height())
        return;

    ray	Ray;
    Ray.origin       = float3(float(DTid.x) / float(g_ColorBuffer.get_width()), 1.0 - float(DTid.y) / float(g_ColorBuffer.get_height()), -1.0);
    Ray.direction    = float3(0.0, 0.0, 1.0);
    Ray.min_distance = 0.01;
    Ray.max_distance = 10.0;

    intersector<triangle_data, instancing> Intersector;
    Intersector.assume_geometry_type( geometry_type::triangle );
    Intersector.force_opacity( forced_opacity::opaque );
    Intersector.accept_any_intersection( false );

    intersection_result<triangle_data, instancing> Intersection = Intersector.intersect(Ray, g_TLAS, 0xFF);

    float4 color;
    if (Intersection.type != intersection_type::none)
    {
        color = HitShader(Intersection.triangle_barycentric_coord);
    }
    else
    {
        color = MissShader();
    }

    g_ColorBuffer.write(color, DTid.xy);
}
)msl"};


const std::string RayTracingTest9_CS{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_raytracing>

using namespace metal;
using namespace raytracing;

#ifndef CONST_BUFFER_1
#   define CONST_BUFFER_1 0
#endif
#ifndef CONST_BUFFER_2
#   define CONST_BUFFER_2 1
#endif
#ifndef CONST_BUFFER_3
#   define CONST_BUFFER_3 2
#endif
#ifndef TLAS_1
#   define TLAS_1 3
#endif
#ifndef TLAS_2
#   define TLAS_2 4
#endif

kernel
void CSMain(uint2						     DTid		    [[thread_position_in_grid]],
            constant float4&                 g_Constant1    [[buffer(CONST_BUFFER_1)]],
            constant float4&                 g_Constant2    [[buffer(CONST_BUFFER_2)]],
            const device float4&             g_Constant3    [[buffer(CONST_BUFFER_3)]],
            texture2d<float, access::write>	 g_ColorBuffer  [[texture(0)]],
            instance_acceleration_structure	 g_TLAS1        [[buffer(TLAS_1)]],
            instance_acceleration_structure	 g_TLAS2        [[buffer(TLAS_2)]] )
{
    if (DTid.x >= g_ColorBuffer.get_width() || DTid.y >= g_ColorBuffer.get_height())
        return;

    ray	Ray;
    Ray.origin       = float3((float(DTid.x) + 0.5) / float(g_ColorBuffer.get_width()), 1.0 - (float(DTid.y) + 0.5) / float(g_ColorBuffer.get_height()), -1.0);
    Ray.direction    = float3(0.0, 0.0, 1.0);
    Ray.min_distance = 0.01;
    Ray.max_distance = 10.0;

    intersector<triangle_data, instancing> Intersector;
    Intersector.assume_geometry_type( geometry_type::triangle );
    Intersector.force_opacity( forced_opacity::opaque );
    Intersector.accept_any_intersection( false );

    intersection_result<triangle_data, instancing> Intersection = Intersector.intersect(Ray, g_TLAS1, 0x0F);

    float4 color;
    if (Intersection.type != intersection_type::none)
    {
        float3 barycentrics;
        barycentrics.yz = Intersection.triangle_barycentric_coord;
        barycentrics.x  = 1.0 - barycentrics.x - barycentrics.y;

        color = float4(barycentrics, 1.0) * g_Constant1;
    }
    else
    {
        Ray.origin.z  = 1.0;
        Ray.direction = float3(0.0, 0.0, 1.0);
        intersection_result<triangle_data, instancing> Intersection2 = Intersector.intersect(Ray, g_TLAS2, 0xF0);

        if (Intersection2.type != intersection_type::none)
            color = g_Constant2;
        else
            color = g_Constant3;
    }

    g_ColorBuffer.write(color, DTid.xy);
}
)msl"};

// clang-format on

} // namespace MSL

} // namespace
