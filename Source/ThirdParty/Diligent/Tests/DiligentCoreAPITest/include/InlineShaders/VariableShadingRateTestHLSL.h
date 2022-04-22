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

const std::string ShadingRatePallete{R"hlsl(
float4 ShadingRateToColor(uint ShadingRate)
{
    float  h   = saturate(ShadingRate * 0.1) / 1.35;
    float3 col = float3(abs(h * 6.0 - 3.0) - 1.0, 2.0 - abs(h * 6.0 - 2.0), 2.0 - abs(h * 6.0 - 4.0));
    return float4(clamp(col, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0)), 1.0);
}
)hlsl"};


const std::string PerDrawShadingRate_VS{R"hlsl(
struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

void main(in  uint    vid : SV_VertexID,
          out PSInput PSIn) 
{
    PSIn.Pos  = float4(float2(vid >> 1, vid & 1) * 4.0 - 1.0, 0.0, 1.0);
    PSIn.Rate = 0; // ignored if combiner is PASSTHROUGH
}
)hlsl"};


const std::string PerDrawShadingRate_PS = ShadingRatePallete + R"hlsl(
struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

float4 main(in PSInput PSIn) : SV_Target
{
    // Rate was overridden by per-draw rate from SetShadingRate()
    return ShadingRateToColor(PSIn.Rate);
}
)hlsl";


const std::string PerPrimitiveShadingRate_VS{R"hlsl(
struct VSInput
{
    float2 Pos  : ATTRIB0;
    uint   Rate : ATTRIB1;
};

struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

void main(in VSInput  VSIn,
          out PSInput PSIn)
{
    PSIn.Pos  = float4(VSIn.Pos, 0.0, 1.0);
    PSIn.Rate = VSIn.Rate;
}
)hlsl"};


const std::string PerPrimitiveShadingRate_PS = ShadingRatePallete + R"hlsl(
struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

float4 main(in PSInput PSIn) : SV_Target
{
    return ShadingRateToColor(PSIn.Rate);
}
)hlsl";


const std::string TextureBasedShadingRate_VS{R"hlsl(
struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

void main(in  uint    vid : SV_VertexID,
          out PSInput PSIn) 
{
    PSIn.Pos  = float4(float2(vid >> 1, vid & 1) * 4.0 - 1.0, 0.0, 1.0);
    PSIn.Rate = 0; // ignored if combiner is PASSTHROUGH
}
)hlsl"};


const std::string TextureBasedShadingRate_PS = ShadingRatePallete + R"hlsl(
struct PSInput
{
                    float4 Pos  : SV_POSITION;
    nointerpolation uint   Rate : SV_ShadingRate;
};

float4 main(in PSInput PSIn) : SV_Target
{
    // Rate was overridden by shading rate texture
    return ShadingRateToColor(PSIn.Rate);
}
)hlsl";


const std::string TextureBasedShadingRateWithTextureArray_VS{R"hlsl(
struct GSInput
{
    float4 Pos : SV_POSITION;
};

void main(in  uint    vid : SV_VertexID,
          out GSInput GSIn) 
{
    GSIn.Pos  = float4(float2(vid >> 1, vid & 1) * 4.0 - 1.0, 0.0, 1.0);
}
)hlsl"};

const std::string TextureBasedShadingRateWithTextureArray_GS{R"hlsl(
struct GSInput
{
    float4 Pos : SV_POSITION;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
	uint   Layer : SV_RenderTargetArrayIndex;

    nointerpolation uint Rate : SV_ShadingRate;
};

[maxvertexcount(3)]
[instance(2)]
void main(          uint                    InstanceID : SV_GSInstanceID, 
          triangle  GSInput                 GSIn[3],
          inout     TriangleStream<PSInput> triStream) 
{
    for (int i = 0; i < 3; ++i)
    {
        PSInput PSIn;
        PSIn.Pos   = GSIn[i].Pos;
        PSIn.Layer = InstanceID;
        PSIn.Rate  = 0; // ignored if combiner is PASSTHROUGH
        triStream.Append(PSIn);
    }
}
)hlsl"};

const std::string TextureBasedShadingRateWithTextureArray_PS = ShadingRatePallete + R"hlsl(
struct PSInput
{
    float4 Pos   : SV_POSITION;
	uint   Layer : SV_RenderTargetArrayIndex;

    nointerpolation uint Rate : SV_ShadingRate;
};

float4 main(in PSInput PSIn) : SV_Target
{
    // Rate was overridden by shading rate texture
    return ShadingRateToColor(PSIn.Rate);
}
)hlsl";

} // namespace HLSL

} // namespace
