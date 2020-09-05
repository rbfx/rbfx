#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

struct VS_INPUT
{
    float4 iPos : POSITION;
    float3 iNormal : NORMAL;
};

struct VS_OUTPUT
{
    float4 oPos : OUTPOSITION;
    float4 oWorldPos : TEXCOORD0;
    float3 oNormal : TEXCOORD1;
    float4 oModelPos : TEXCOORD2;
};

struct GS_OUTPUT
{
    float4 oPos : SV_POSITION;
    float4 oColor : TEXCOORD0;
};

#ifdef COMPILEVS
VS_OUTPUT VS(in VS_INPUT input)
{
    VS_OUTPUT outData = (VS_OUTPUT)0;

    float4x3 modelMatrix = cModel;
    float3 worldPos = mul(input.iPos, cModel);
    outData.oPos = GetClipPos(worldPos);
    outData.oWorldPos = float4(worldPos, GetDepth(outData.oPos));
    outData.oNormal = normalize(mul(input.iNormal, (float3x3)cModel));

    return outData;
}
#endif

#if defined(COMPILEGS)

void CreateVertex(inout TriangleStream<GS_OUTPUT> triStream, float3 pos, float4 col)
{
    GS_OUTPUT temp = (GS_OUTPUT)0;
    temp.oPos = GetClipPos(pos.xyz);
    temp.oColor = col;
    triStream.Append(temp);
}

[maxvertexcount(16)]
void GS(triangle in VS_OUTPUT vertices[3], inout TriangleStream<GS_OUTPUT> triStream)
{
    VS_OUTPUT v1 = vertices[0],
        v2 = vertices[1],
        v3 = vertices[2];
        
    float4 colors[] = {
        float4(0.8, 0.6, 0.3, 0.5),
        float4(0.2, 0.9, 0.5, 0.5),
        float4(0.1, 0.33, 0.7, 0.5),
        float4(0.6, 0.73, 0.2, 0.5),
        float4(0.4, 0.73, 0.1, 0.5),
    };

    CreateVertex(triStream, v1.oWorldPos, colors[0]);
    CreateVertex(triStream, v2.oWorldPos, colors[0]);
    CreateVertex(triStream, v3.oWorldPos, colors[0]);
    triStream.RestartStrip();

    for (int i = 0; i < 4; ++i)
    {
        CreateVertex(triStream, v1.oWorldPos + v1.oNormal*0.3 * (i+1), colors[i+1]);
        CreateVertex(triStream, v2.oWorldPos + v2.oNormal*0.3 * (i+1), colors[i+1]);
        CreateVertex(triStream, v3.oWorldPos + v3.oNormal*0.3 * (i+1), colors[i+1]);
        triStream.RestartStrip();
    }
}
#endif

float4 PS(in GS_OUTPUT gsData) : SV_TARGET
{
    return gsData.oColor;
}

