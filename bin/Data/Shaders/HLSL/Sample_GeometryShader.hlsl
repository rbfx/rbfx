#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "Fog.hlsl"

// Preprocessor Options
//      TESS_FIXED: tessellation level is explicit instead of variable
//      TESS_PHONG: use Phong-interpolation to smooth the tessellated output

// level to tessellate to, if TESS_FIXED is used then
cbuffer CustomPS : register(b6)
{
    // X =  maximum level to tessellate to, if TESS_FIXED then this value will always be used
    // Y = distance to vary tessellation over, unused if TESS_FIXED is defined
    // Z = offset distance, shifts the distance, unused if TESS_FIXED is defined
    // Z = phong power, unused if TESS_PHONG is not defined, values between 0.3 and 0.7 are sane
    float4 cTessParams;
}

#ifdef COMPILEVS

void VS(float4 iPos : POSITION,
    #ifndef NOUV
        float2 iTexCoord : TEXCOORD0,
    #endif
    #ifdef TESS_PHONG
        float3 iNormal : NORMAL,
    #endif
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #ifdef SKINNED
        float4 iBlendWeights : BLENDWEIGHT,
        int4 iBlendIndices : BLENDINDICES,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    out float2 oTexCoord : TEXCOORD0,
    #ifdef TESS_PHONG
        out float3 oNormal : NORMAL0,
    #endif
    out float4 oWorldPos : TEXCOORD2,
    #ifdef VERTEXCOLOR
        out float4 oColor : COLOR0,
    #endif
    out float4 oPos : OUTPOSITION
    )
{
    // Define a 0,0 UV coord if not expected from the vertex data
    #ifdef NOUV
    float2 iTexCoord = float2(0.0, 0.0);
    #endif

    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oTexCoord = GetTexCoord(iTexCoord);
    oWorldPos = float4(worldPos, GetDepth(oPos));
    
    #ifdef TESS_PHONG
        oNormal = GetWorldNormal(modelMatrix);
    #endif

    #if defined(D3D11) && defined(CLIPPLANE)
        oClip = dot(oPos, cClipPlane);
    #endif
    
    #ifdef VERTEXCOLOR
        oColor = iColor;
    #endif
}

#endif

struct VS_OUTPUT
{
    float2 oTexCoord : TEXCOORD0;
    #ifdef TESS_PHONG
        float3 oNormal : NORMAL0;
    #endif
    float4 oWorldPos : TEXCOORD2;
    #ifdef VERTEXCOLOR
        float4 oColor : COLOR0;
    #endif
};

struct HULL_OUTPUT
{
    float4 oPos : SV_Position;
    float2 oTexCoord : TEXCOORD0;
    #ifdef TESS_PHONG
        float3 oNormal : NORMAL0;
    #endif
    float4 oWorldPos : TEXCOORD2;
    #ifdef VERTEXCOLOR
        float4 oColor : COLOR0;
    #endif
};

struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess  : SV_InsideTessFactor;
};

#ifdef COMPILEHS

float PointLineDistance(float3 p, in float3 a, in float3 b)
{
    float3 ab = b - a;
	float t = saturate(dot(p - a, ab) / dot(ab, ab));
	return abs(length((ab*t + a) - p)) * 0.5;
}

bool EdgeCull(float4 a, float4 b, float3 aN, float3 bN)
{
    float3 norm = normalize((aN + bN) * 0.5);
    float3 midPoint = (a.xyz + b.xyz) * 0.5;
    float edgeDot = dot(normalize(cCameraPos - midPoint), norm);
    return edgeDot < -0.1;
}

PatchTess PatchHS(InputPatch<VS_OUTPUT,3> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    #ifndef TESS_FIXED
        // Tessellate based on the distance from the camera

        float distA = max(0, PointLineDistance(cCameraPos, patch[0].oWorldPos, patch[1].oWorldPos) - cTessParams.z);
        float distB = max(0, PointLineDistance(cCameraPos, patch[1].oWorldPos, patch[2].oWorldPos) - cTessParams.z);
        float distC = max(0, PointLineDistance(cCameraPos, patch[2].oWorldPos, patch[0].oWorldPos) - cTessParams.z);
        
        const float tessAdaptDist = max(0.01, cTessParams.y - cTessParams.z);
        const float maxTessLevel = max(1, min(cTessParams.x, 16));
        
        float minDist = min(distA, min(distB, distC));
        float tessLevelA = max(1, (1.0 - saturate(distA / tessAdaptDist)) * maxTessLevel);
        float tessLevelB = max(1, (1.0 - saturate(distB / tessAdaptDist)) * maxTessLevel);
        float tessLevelC = max(1, (1.0 - saturate(distC / tessAdaptDist)) * maxTessLevel);
        
        // note edge order
        pt.EdgeTess[2] = tessLevelA;
        pt.EdgeTess[0] = tessLevelB;
        pt.EdgeTess[1] = tessLevelC;
        pt.InsideTess  = (pt.EdgeTess[0] + pt.EdgeTess[1] + pt.EdgeTess[2]) / 3;
        
    #else
        // clip tess level to prevent accidental entry of excessive values
        float tessLevel = max(1, min(cTessParams.x, 16));
        pt.EdgeTess[0] = tessLevel;
        pt.EdgeTess[1] = tessLevel;
        pt.EdgeTess[2] = tessLevel;
        pt.InsideTess = (pt.EdgeTess[0] + pt.EdgeTess[1] + pt.EdgeTess[2]) / 3;
    
    #endif
    
    #if TESS_PHONG
    
        // Perform basic culling
        bool aCull = EdgeCull(patch[0].oWorldPos, patch[1].oWorldPos, patch[0].oNormal, patch[1].oNormal);
        bool bCull = EdgeCull(patch[1].oWorldPos, patch[2].oWorldPos, patch[1].oNormal, patch[2].oNormal);
        bool cCull = EdgeCull(patch[2].oWorldPos, patch[0].oWorldPos, patch[2].oNormal, patch[0].oNormal);
        if (aCull && bCull && cCull)
            pt.EdgeTess[0] = pt.EdgeTess[1] = pt.EdgeTess[2] = pt.InsideTess = 0;
        
    #endif
    
    return pt;
}

[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
[domain("tri")]
VS_OUTPUT HS(InputPatch<VS_OUTPUT,3> patch, uint pointIdx : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    VS_OUTPUT dataOut;
    dataOut.oWorldPos = patch[pointIdx].oWorldPos;
    dataOut.oTexCoord = patch[pointIdx].oTexCoord;
    
    #ifdef TESS_PHONG
        dataOut.oNormal = patch[pointIdx].oNormal;
    #endif
    
    #ifdef VERTEXCOLOR
        dataOut.oColor = patch[pointIdx].oColor;
    #endif
    return dataOut;
}

#endif

#ifdef COMPILEDS

[domain("tri")]
HULL_OUTPUT DS(PatchTess input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<VS_OUTPUT, 3> patch)
{
    HULL_OUTPUT dataOut;
    
    float4 vertexPosition = uvwCoord.x * patch[0].oWorldPos + uvwCoord.y * patch[1].oWorldPos + uvwCoord.z * patch[2].oWorldPos;
    float2 texCoord = uvwCoord.x * patch[0].oTexCoord + uvwCoord.y * patch[1].oTexCoord + uvwCoord.z * patch[2].oTexCoord;
    #ifdef TESS_PHONG
        float3 vertNor = normalize(uvwCoord.x * patch[0].oNormal + uvwCoord.y * patch[1].oNormal + uvwCoord.z * patch[2].oNormal);
        
        float3 curveProjectionA = dot(patch[0].oWorldPos.xyz - vertexPosition.xyz, patch[0].oNormal) * patch[0].oNormal;
        float3 curveProjectionB = dot(patch[1].oWorldPos.xyz - vertexPosition.xyz, patch[1].oNormal) * patch[1].oNormal;
        float3 curveProjectionC = dot(patch[2].oWorldPos.xyz - vertexPosition.xyz, patch[2].oNormal) * patch[2].oNormal;
        vertexPosition.xyz += (uvwCoord.x * curveProjectionA + uvwCoord.y * curveProjectionB + uvwCoord.z * curveProjectionC) * cTessParams.w;
        
    #endif
    
    dataOut.oPos = GetClipPos(vertexPosition.xyz);
    dataOut.oWorldPos = float4(vertexPosition.xyz, GetDepth(dataOut.oPos));
    dataOut.oTexCoord = texCoord;
    #ifdef TESS_PHONG
        dataOut.oNormal = vertNor;
    #endif    
    #ifdef VERTEXCOLOR
        dataOut.oColor = uvwCoord.x * patch[0].oColor + uvwCoord.y * patch[1].oColor + uvwCoord.z * patch[2].oColor;
    #endif
    return dataOut;
}

#endif

#ifdef COMPILEPS

void PS(HULL_OUTPUT data,
    #if defined(D3D11) && defined(CLIPPLANE)
        float iClip : SV_CLIPDISTANCE0,
    #endif
    #ifdef PREPASS
        out float4 oDepth : OUTCOLOR1,
    #endif
    #ifdef DEFERRED
        out float4 oAlbedo : OUTCOLOR1,
        out float4 oNormal : OUTCOLOR2,
        out float4 oDepth : OUTCOLOR3,
    #endif
    out float4 oColor : OUTCOLOR0)
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        float4 diffColor = cMatDiffColor * Sample2D(DiffMap, data.oTexCoord);
        #ifdef ALPHAMASK
            if (diffColor.a < 0.5)
                discard;
        #endif
    #else
        float4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= data.oColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(data.oWorldPos.w, iWorldPos.y);
    #else
        float fogFactor = GetFogFactor(data.oWorldPos.w);
    #endif

    #if defined(PREPASS)
        // Fill light pre-pass G-Buffer
        oColor = float4(0.5, 0.5, 0.5, 1.0);
        oDepth = data.oWorldPos.w;
    #elif defined(DEFERRED)
        // Fill deferred G-buffer
        oColor = float4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
        oAlbedo = float4(0.0, 0.0, 0.0, 0.0);
        oNormal = float4(0.5, 0.5, 0.5, 1.0);
        oDepth = data.oWorldPos.w;
    #else
        oColor = float4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
    #endif
}

#endif