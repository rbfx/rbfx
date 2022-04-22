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

namespace HLSL
{

const std::string RayTracingTest_Payload = R"(
struct RTPayload
{
    float4 Color;
};
)";


// clang-format off
const std::string RayTracingTest1_RG = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS        : register(t0);
RWTexture2D<float4>             g_ColorBuffer : register(u0);

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RTPayload payload = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,         // Acceleration Structure
             RAY_FLAG_NONE,  // Ray Flags
             ~0,             // Instance Inclusion Mask
             0,              // Ray Contribution To Hit Group Index
             1,              // Multiplier For Geometry Contribution To Hit Group Index
             0,              // Miss Shader Index
             ray,
             payload);

    g_ColorBuffer[DispatchRaysIndex().xy] = payload.Color;
}
)hlsl";

const std::string RayTracingTest1_RM = RayTracingTest_Payload +
R"hlsl(
[shader("miss")]
void main(inout RTPayload payload)
{
    payload.Color = float4(1.0, 0.0, 0.0, 1.0);
}
)hlsl";

const std::string RayTracingTest1_RCH = RayTracingTest_Payload +
R"hlsl(
[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.Color = float4(barycentrics, 1.0);
}
)hlsl";


const std::string RayTracingTest2_RG = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS        : register(t0);
RWTexture2D<float4>             g_ColorBuffer : register(u0);

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RTPayload payload = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,                           // Acceleration Structure
             RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             ~0,                               // Instance Inclusion Mask
             0,                                // Ray Contribution To Hit Group Index
             1,                                // Multiplier For Geometry Contribution To Hit Group Index
             0,                                // Miss Shader Index
             ray,
             payload);

    g_ColorBuffer[DispatchRaysIndex().xy] = payload.Color;
}
)hlsl";

const std::string RayTracingTest2_RM = RayTracingTest_Payload +
R"hlsl(
[shader("miss")]
void main(inout RTPayload payload)
{
    payload.Color = float4(0.0, 0.0, 0.0, 0.0);
}
)hlsl";

const std::string RayTracingTest2_RCH = RayTracingTest_Payload +
R"hlsl(
[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.Color *= 4.0;
}
)hlsl";

const std::string RayTracingTest2_RAH = RayTracingTest_Payload +
R"hlsl(
[shader("anyhit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    if (barycentrics.y > barycentrics.x)
        IgnoreHit();
    else
        payload.Color += float4(barycentrics, 1.0) / 3.0;
}
)hlsl";


const std::string RayTracingTest3_RG = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS        : register(t0);
RWTexture2D<float4>             g_ColorBuffer : register(u0);

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, 0.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 4.0;

    RTPayload payload = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,         // Acceleration Structure
             RAY_FLAG_NONE,  // Ray Flags
             ~0,             // Instance Inclusion Mask
             0,              // Ray Contribution To Hit Group Index
             1,              // Multiplier For Geometry Contribution To Hit Group Index
             0,              // Miss Shader Index
             ray,
             payload);

    g_ColorBuffer[DispatchRaysIndex().xy] = payload.Color;
}
)hlsl";

const std::string RayTracingTest3_RM = RayTracingTest_Payload +
R"hlsl(
[shader("miss")]
void main(inout RTPayload payload)
{
    payload.Color = float4(0.0, 0.15, 0.0, 1.0);
}
)hlsl";

const std::string RayTracingTest3_RCH = RayTracingTest_Payload +
R"hlsl(
struct SphereIntersectionAttributes
{
    float3 value;
};

[shader("closesthit")]
void main(inout RTPayload payload, in SphereIntersectionAttributes attr)
{
    payload.Color = float4(attr.value.x, RayTCurrent() / 4.0, float(HitKind()) * 0.2, 1.0);
}
)hlsl";

const std::string RayTracingTest3_RI = RayTracingTest_Payload +
R"hlsl(
struct SphereIntersectionAttributes
{
    float3 value;
};

[shader("intersection")]
void main()
{
    const float radius = 0.5;
    const float3 center = float3(0.25, 0.5, 2.0); // must match with AABB center

    // ray sphere intersection
    float3 oc = WorldRayOrigin() - center;
    float  a  = dot(WorldRayDirection(), WorldRayDirection());
    float  b  = 2.0 * dot(oc, WorldRayDirection());
    float  c  = dot(oc, oc) - radius * radius;
    float  d  = b * b - 4 * a * c;

    if (d >= 0)
    {
        float hitT = (-b - sqrt(d)) / (2.0 * a);
        SphereIntersectionAttributes attr = {float3(0.5, 0.5, 0.5)};
        ReportHit(hitT, 3, attr);
    }
}
)hlsl";


const std::string RayTracingTest4_RG = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS        : register(t0);
RWTexture2D<float4>             g_ColorBuffer : register(u0);

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RTPayload payload = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,         // Acceleration Structure
             RAY_FLAG_NONE,  // Ray Flags
             ~0,             // Instance Inclusion Mask
             0,              // Ray Contribution To Hit Group Index
             1,              // Multiplier For Geometry Contribution To Hit Group Index
             0,              // Miss Shader Index
             ray,
             payload);

    g_ColorBuffer[DispatchRaysIndex().xy] = payload.Color;
}
)hlsl";

const std::string RayTracingTest4_RM = RayTracingTest_Payload +
R"hlsl(
[shader("miss")]
void main(inout RTPayload payload)
{
    payload.Color = float4(0.0, 0.0, 0.2, 1.0);
}
)hlsl";

const std::string RayTracingTest4_Uniforms = RayTracingTest_Payload +
R"hlsl(
struct Vertex
{
    float4 Pos;
    float4 Color1;
    float4 Color2;
};
StructuredBuffer<Vertex> g_Vertices       : register(t1); // array size = 16
StructuredBuffer<uint>   g_PerInstance[2] : register(t2); // array size = 3
StructuredBuffer<uint4>  g_Primitives     : register(t4); // array size = 9

// local root constants
struct LocalRootConst
{
    float4 Weight;
    uint   GeometryID; // same as GeometryIndex() in DXR 1.1
};
#ifdef VULKAN
  [[vk::shader_record_ext]]
#endif
ConstantBuffer<LocalRootConst> g_LocalRoot : register(b0);
)hlsl";

const std::string RayTracingTest4_RCH1 = RayTracingTest4_Uniforms +
R"hlsl(
[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y) * g_LocalRoot.Weight.xyz;
    uint   primOffset   = g_PerInstance[InstanceIndex()][g_LocalRoot.GeometryID];
    uint4  triFace      = g_Primitives[primOffset + PrimitiveIndex()];
    Vertex v0           = g_Vertices[triFace.x];
    Vertex v1           = g_Vertices[triFace.y];
    Vertex v2           = g_Vertices[triFace.z];
    float4 col          = v0.Color2 * barycentrics.x + v1.Color2 * barycentrics.y + v2.Color2 * barycentrics.z;
    payload.Color = col;
}
)hlsl";

const std::string RayTracingTest4_RCH2 = RayTracingTest4_Uniforms +
R"hlsl(
[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y) * g_LocalRoot.Weight.xyz;
    uint   primOffset   = g_PerInstance[InstanceIndex()][g_LocalRoot.GeometryID];
    uint4  triFace      = g_Primitives[primOffset + PrimitiveIndex()];
    Vertex v0           = g_Vertices[triFace.x];
    Vertex v1           = g_Vertices[triFace.y];
    Vertex v2           = g_Vertices[triFace.z];
    float4 col          = v0.Color1 * barycentrics.x + v1.Color1 * barycentrics.y + v2.Color1 * barycentrics.z;
    payload.Color = col;
}
)hlsl";


const std::string RayTracingTest5_RG = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS;
RWTexture2D<float4>             g_ColorBuffer;

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RTPayload payload = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,         // Acceleration Structure
             RAY_FLAG_NONE,  // Ray Flags
             ~0,             // Instance Inclusion Mask
             0,              // Ray Contribution To Hit Group Index
             1,              // Multiplier For Geometry Contribution To Hit Group Index
             0,              // Miss Shader Index
             ray,
             payload);

    g_ColorBuffer[DispatchRaysIndex().xy] = payload.Color;
}
)hlsl";

const std::string RayTracingTest5_RM = RayTracingTest_Payload +
R"hlsl(
[shader("miss")]
void main(inout RTPayload payload)
{
    payload.Color = float4(1.0, 0.0, 0.0, 1.0);
}
)hlsl";

const std::string RayTracingTest5_RCH1 = RayTracingTest_Payload +
R"hlsl(
Texture2D     g_Texture1;
SamplerState  g_Texture1_sampler;

[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float2 uv = attr.barycentrics.xy;
    payload.Color = g_Texture1.SampleLevel(g_Texture1_sampler, uv, 0);
}
)hlsl";

const std::string RayTracingTest5_RCH2 = RayTracingTest_Payload +
R"hlsl(
RaytracingAccelerationStructure g_TLAS;

Texture2D     g_Texture2;
SamplerState  g_Texture2_sampler;

[shader("closesthit")]
void main(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float2 uv = attr.barycentrics.xy;
    payload.Color = g_Texture2.SampleLevel(g_Texture2_sampler, uv, 0);

    RayDesc ray;
    ray.Origin    = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = WorldRayDirection();
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RTPayload payload2 = {float4(0, 0, 0, 0)};
    TraceRay(g_TLAS,         // Acceleration Structure
             RAY_FLAG_NONE,  // Ray Flags
             ~0,             // Instance Inclusion Mask
             0,              // Ray Contribution To Hit Group Index
             1,              // Multiplier For Geometry Contribution To Hit Group Index
             0,              // Miss Shader Index
             ray,
             payload2);

    payload.Color += payload2.Color;
}
)hlsl";


const std::string RayTracingTest6_RG{R"hlsl(
RaytracingAccelerationStructure g_TLAS;
RWTexture2D<float4>             g_ColorBuffer;

float4 HitShader(float2 attrBarycentrics)
{
    float3 barycentrics = float3(1.0 - attrBarycentrics.x - attrBarycentrics.y, attrBarycentrics.x, attrBarycentrics.y);
    return float4(barycentrics, 1.0);
}

float4 MissShader()
{
    return float4(1.0, 0.0, 0.0, 1.0);
}

[shader("raygeneration")]
void main()
{
    const float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RayQuery<RAY_FLAG_NONE> q;

    q.TraceRayInline(g_TLAS,         // Acceleration Structure
                     RAY_FLAG_NONE,  // Ray Flags
                     ~0,             // Instance Inclusion Mask
                     ray);

    q.Proceed();

    float4 Color;
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Color = HitShader(q.CommittedTriangleBarycentrics());
    }
    else
    {
        Color = MissShader();
    }
    g_ColorBuffer[DispatchRaysIndex().xy] = Color;
}
)hlsl"};


const std::string RayTracingTest7_VS{R"hlsl(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD;
};

void main(in uint vid : SV_VertexID,
          out PSInput PSIn)
{
    PSIn.UV  = float2(vid & 1, vid >> 1);
    PSIn.Pos = float4(PSIn.UV * 2.0 - 1.0, 0.0, 1.0);
}
)hlsl"};

const std::string RayTracingTest7_PS{R"hlsl(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD;
};

RaytracingAccelerationStructure g_TLAS;

float4 HitShader(float2 attrBarycentrics)
{
    float3 barycentrics = float3(1.0 - attrBarycentrics.x - attrBarycentrics.y, attrBarycentrics.x, attrBarycentrics.y);
    return float4(barycentrics, 1.0);
}

float4 MissShader()
{
    return float4(1.0, 0.0, 0.0, 1.0);
}

float4 main(in PSInput PSIn) : SV_Target
{
    const float2 uv = PSIn.UV;

    RayDesc ray;
    ray.Origin    = float3(uv.x, uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RayQuery<RAY_FLAG_NONE> q;

    q.TraceRayInline(g_TLAS,         // Acceleration Structure
                     RAY_FLAG_NONE,  // Ray Flags
                     ~0,             // Instance Inclusion Mask
                     ray);

    q.Proceed();

    float4 Color;
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Color = HitShader(q.CommittedTriangleBarycentrics());
    }
    else
    {
        Color = MissShader();
    }
    return Color;
}
)hlsl"};


const std::string RayTracingTest8_CS{R"hlsl(
RaytracingAccelerationStructure g_TLAS;
RWTexture2D<float4>             g_ColorBuffer;

float4 HitShader(float2 attrBarycentrics)
{
    float3 barycentrics = float3(1.0 - attrBarycentrics.x - attrBarycentrics.y, attrBarycentrics.x, attrBarycentrics.y);
    return float4(barycentrics, 1.0);
}

float4 MissShader()
{
    return float4(1.0, 0.0, 0.0, 1.0);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 Dim;
    g_ColorBuffer.GetDimensions(Dim.x, Dim.y);
    if (DTid.x >= Dim.x || DTid.y >= Dim.y)
        return;

    const float2 uv = (float2(DTid.xy) + 0.5) / float2(Dim.xy);

    RayDesc ray;
    ray.Origin    = float3(uv.x, 1.0 - uv.y, -1.0);
    ray.Direction = float3(0.0, 0.0, 1.0);
    ray.TMin      = 0.01;
    ray.TMax      = 10.0;

    RayQuery<RAY_FLAG_NONE> q;

    q.TraceRayInline(g_TLAS,         // Acceleration Structure
                     RAY_FLAG_NONE,  // Ray Flags
                     ~0,             // Instance Inclusion Mask
                     ray);

    q.Proceed();

    float4 Color;
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Color = HitShader(q.CommittedTriangleBarycentrics());
    }
    else
    {
        Color = MissShader();
    }
    g_ColorBuffer[DTid.xy] = Color;
}
)hlsl"};
// clang-format on

} // namespace HLSL

} // namespace
