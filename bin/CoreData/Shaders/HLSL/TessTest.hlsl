#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

#define OUTPUT_PATCH_SIZE 3

struct VS_INPUT
{
    float4 iPos : POSITION;
    float3 iNormal : NORMAL;
};

struct VS_OUTPUT
{
    float4 oPos : OUTPOSITION;
    float4 oWorldPos : TEXCOORD0;
    float4 oColor : TEXCOORD1;
};

struct HULL_OUTPUT
{
    float4 oPos : SV_POSITION;
    float4 oWorldPos : TEXCOORD0;
    float4 oColor : TEXCOORD1;
};

#ifdef COMPILEVS
VS_OUTPUT VS(in VS_INPUT input)
{
    VS_OUTPUT outData = (VS_OUTPUT)0;

    float4x3 modelMatrix = cModel;
    float3 worldPos = mul(input.iPos, cModel);
    outData.oPos = GetClipPos(worldPos);
    outData.oWorldPos = float4(worldPos, GetDepth(outData.oPos));
    outData.oColor = float4(1, 0, 0, 1);

    return outData;
}
#endif

struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess  : SV_InsideTessFactor;
};

#if defined(COMPILEHS)

PatchTess PatchHS(InputPatch<VS_OUTPUT,3> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // just using a constant factor of 2
    pt.EdgeTess[0] = 4;
    pt.EdgeTess[1] = 4;
    pt.EdgeTess[2] = 4;
    pt.InsideTess  = 3;
    
    return pt;
}


[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
[domain("tri")]
HULL_OUTPUT HS(InputPatch<VS_OUTPUT,3> patch, uint pointIdx : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    HULL_OUTPUT dataOut;
    //dataOut.oPos = patch[pointIdx].oWorldPos;
    dataOut.oWorldPos = patch[pointIdx].oWorldPos;
    dataOut.oColor = patch[pointIdx].oColor;
    return dataOut;
}
#endif

#if defined(COMPILEDS)
#line 79
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t );
}

//--------------------------------------------------------------------------------------
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( -3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t );
}

//--------------------------------------------------------------------------------------
float3 EvaluateBezier( const OutputPatch<HULL_OUTPUT, 3> bezpatch, float4 BasisU, float4 BasisV )
{
    float3 Value = float3(0,0,0);
    Value  = BasisV.x * ( bezpatch[0].oWorldPos * BasisU.x + bezpatch[1].oWorldPos * BasisU.y);
    Value += BasisV.y * ( bezpatch[1].oWorldPos * BasisU.x + bezpatch[2].oWorldPos * BasisU.y);
    Value += BasisV.z * ( bezpatch[2].oWorldPos * BasisU.x + bezpatch[0].oWorldPos * BasisU.y);

    return Value;
}

[domain("tri")]
HULL_OUTPUT DS(PatchTess input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<HULL_OUTPUT, 3> patch)
{
    HULL_OUTPUT dataOut;
    
    float4 vertexPosition = uvwCoord.x * patch[0].oWorldPos + uvwCoord.y * patch[1].oWorldPos + uvwCoord.z * patch[2].oWorldPos;
    dataOut.oPos = GetClipPos(vertexPosition.xyz);
    dataOut.oWorldPos = GetClipPos(vertexPosition);
    dataOut.oColor = float4(1, 0, 0, 1);
    return dataOut;
}

#endif

#if defined(COMPILEPS)

float4 PS(in HULL_OUTPUT gsData) : SV_TARGET
{
    return gsData.oColor;
}

#endif