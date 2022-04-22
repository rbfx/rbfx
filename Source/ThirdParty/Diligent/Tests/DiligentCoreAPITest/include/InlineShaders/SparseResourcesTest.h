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

namespace HLSL
{

const std::string FillBuffer_CS{R"hlsl(
RWStructuredBuffer<uint> g_DstBuffer;

cbuffer CB
{
    uint Offset;
    uint Size;
    uint Pattern;
    uint padding;
};

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
    if (DTid < Size)
    {
        g_DstBuffer[Offset + DTid] = Pattern;
    }
}
)hlsl"};


const std::string FillTexture2D_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

cbuffer CB
{
    float4 Color;
};

float4 main(in PSInput PSIn) : SV_Target
{
    return Color;
}
)hlsl"};


const std::string FillTexture3D_CS{R"hlsl(
RWTexture3D<float4> g_DstTexture;

cbuffer CB
{
    uint3  Offset;
    uint3  Size;
    float4 Color;
};

[numthreads(4, 4, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (all(DTid < Size))
    {
        g_DstTexture[Offset + DTid] = Color;
    }
}
)hlsl"};


const std::string SparseResTest_VS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

void main(in uint vid : SV_VertexID,
          out PSInput PSIn) 
{
    // fullscreen triangle
    float2 uv = float2(vid >> 1, vid & 1) * 2.0;
    PSIn.Pos  = float4(uv * 2.0 - 1.0, 0.0, 1.0);
}
)hlsl"};


const std::string SparseBuffer_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

StructuredBuffer<uint> g_Buffer;

float4 PSmain(in PSInput PSIn) : SV_Target
{
    uint Idx         = uint(PSIn.Pos.x) + uint(PSIn.Pos.y) * SCREEN_WIDTH;
    uint PackedColor = 0;

    [branch]
    if (Idx < BUFFER_ELEMENT_COUNT)
        PackedColor = g_Buffer[Idx];

    float4 Color;
    Color.r = (PackedColor & 0xFF) / 255.0;
    Color.g = ((PackedColor >> 8) & 0xFF) / 255.0;
    Color.b = ((PackedColor >> 16) & 0xFF) / 255.0;
    Color.a = 1.0;

    return Color;
}
)hlsl"};


const std::string SparseTexture_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

#if TEXTURE_2D_ARRAY
    Texture2DArray<float4> g_Texture;
#else
    Texture2D<float4> g_Texture;
#endif

float4 PSmain(in PSInput PSIn) : SV_Target
{
    int4 Coord     = int4(PSIn.Pos.x, PSIn.Pos.y, 0, 0); // u, v, Layer, LOD
    int  MipHeight = SCREEN_HEIGHT / 2;

#if TEXTURE_2D_ARRAY
    int  TexWidth = SCREEN_WIDTH / 2;
    if (PSIn.Pos.x > TexWidth)
    {
        Coord.x %= TexWidth;
        Coord.z = PSIn.Pos.x / TexWidth;
    }
#endif

    while (Coord.y > MipHeight && MipHeight > 1)
    {
        Coord.y   -= MipHeight;
        Coord.w   += 1;
        MipHeight >>= 1;
    }

#if TEXTURE_2D_ARRAY
    return g_Texture.Load(Coord);
#else
    return g_Texture.Load(Coord.xyw);
#endif
}
)hlsl"};


const std::string SparseTextureResidency_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

#if TEXTURE_2D_ARRAY
    Texture2DArray<float4> g_Texture;
#else
    Texture2D<float4> g_Texture;
#endif

float4 PSmain(in PSInput PSIn) : SV_Target
{
    int4 Coord     = int4(PSIn.Pos.x, PSIn.Pos.y, 0, 0); // u, v, Layer, LOD
    int  MipHeight = SCREEN_HEIGHT / 2;

#if TEXTURE_2D_ARRAY
    int  TexWidth = SCREEN_WIDTH / 2;
    if (PSIn.Pos.x > TexWidth)
    {
        Coord.x %= TexWidth;
        Coord.z = PSIn.Pos.x / TexWidth;
    }
#endif

    while (Coord.y > MipHeight && MipHeight > 1)
    {
        Coord.y   -= MipHeight;
        Coord.w   += 1;
        MipHeight >>= 1;
    }

    uint Status;
#if TEXTURE_2D_ARRAY
    float4 Color = g_Texture.Load(Coord, /*offset*/int2(0,0), Status);
#else
    float4 Color = g_Texture.Load(Coord.xyw, /*offset*/int2(0,0), Status);
#endif

    if (!CheckAccessFullyMapped(Status))
        return float4(1.0, 0.0, 1.0, 1.0);

    return Color;
}
)hlsl"};


const std::string SparseTexture3D_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

Texture3D<float4> g_Texture;

float4 PSmain(in PSInput PSIn) : SV_Target
{
    int4 Coord     = int4(PSIn.Pos.x, PSIn.Pos.y, 0, 0); // u, v, w, LOD
    int  MipHeight = SCREEN_HEIGHT / 8;
    int  TexWidth  = SCREEN_WIDTH / 8;
    if (PSIn.Pos.x > TexWidth)
    {
        Coord.x %= TexWidth;
        Coord.z = PSIn.Pos.x / TexWidth;
    }

    while (Coord.y > MipHeight && MipHeight > 1)
    {
        Coord.y   -= MipHeight;
        Coord.w   += 1;
        MipHeight >>= 1;
    }

    return g_Texture.Load(Coord);
}
)hlsl"};

} // namespace HLSL


namespace MSL
{

const std::string SparseTextureResidency_PS{R"msl(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct VSOut
{
    float4 Pos [[position]];
};

fragment float4 PSmain(VSOut PSIn [[stage_in]],
                       #if TEXTURE_2D_ARRAY
                           texture2d_array<float> g_Texture [[texture(0)]]
                       #else
                           texture2d<float>       g_Texture [[texture(0)]]
                       #endif
                      )
{
    int4 Coord     = int4(PSIn.Pos.x, PSIn.Pos.y, 0, 0); // u, v, Layer, LOD
    int  MipHeight = SCREEN_HEIGHT / 2;

#if TEXTURE_2D_ARRAY
    int  TexWidth = SCREEN_WIDTH / 2;
    if (PSIn.Pos.x > TexWidth)
    {
        Coord.x %= TexWidth;
        Coord.z = PSIn.Pos.x / TexWidth;
    }
#endif

    while (Coord.y > MipHeight && MipHeight > 1)
    {
        Coord.y   -= MipHeight;
        Coord.w   += 1;
        MipHeight >>= 1;
    }

#if TEXTURE_2D_ARRAY
    sparse_color<float4> Color = g_Texture.sparse_read(uint2(Coord.xy), Coord.z, Coord.w);
#else
    sparse_color<float4> Color = g_Texture.sparse_read(uint2(Coord.xy), Coord.w);
#endif

    if (!Color.resident())
        return float4(1.0, 0.0, 1.0, 1.0);

    return Color.value();
}
)msl"};

} // namespace MSL

} // namespace
