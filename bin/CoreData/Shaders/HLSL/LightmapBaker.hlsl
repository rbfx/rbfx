#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

#ifndef D3D11

#error DX9 is not supported

#else

cbuffer LightmapVS : register(b6)
{
    float cLightmapLayer;
    float cLightmapGeometry;
    float2 cLightmapPositionBias;
}

#endif

void VS(float4 iPos : POSITION,
    #if !defined(BILLBOARD) && !defined(TRAILFACECAM)
        float3 iNormal : NORMAL,
    #endif
    #ifdef DIFFMAP
        float2 iTexCoord : TEXCOORD0,
    #endif
    float2 iTexCoord2 : TEXCOORD1,
    #ifdef SKINNED
        float4 iBlendWeights : BLENDWEIGHT,
        int4 iBlendIndices : BLENDINDICES,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    #if defined(BILLBOARD) || defined(DIRBILLBOARD)
        float2 iSize : TEXCOORD1,
    #endif

    out float2 oTexCoord : TEXCOORD0,
    out float3 oNormal : TEXCOORD1,
    out float4 oWorldPos : TEXCOORD2,
    out float4 oMetadata : TEXCOORD3,
    out float4 oPos : OUTPOSITION)
{
    #ifndef DIFFMAP
    float2 iTexCoord = float2(0.0, 0.0);
    #endif

    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    float2 lightmapUV = iTexCoord2 * cLMOffset.xy + cLMOffset.zw;

    oPos = float4(lightmapUV * float2(2, -2) + float2(-1, 1), cLightmapLayer, 1);
    oTexCoord = GetTexCoord(iTexCoord);
    oNormal = GetWorldNormal(modelMatrix);
    oWorldPos = float4(worldPos, 1.0);
    oMetadata = float4(cLightmapGeometry, cLightmapPositionBias.x, cLightmapPositionBias.y, 0.0);
}

void PS(
    float2 iTexCoord : TEXCOORD0,
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,
    float4 iMetadata : TEXCOORD3,

    out float4 oPosition : OUTCOLOR0,
    out float4 oSmoothPosition : OUTCOLOR1,
    out float4 oFaceNormal : OUTCOLOR2,
    out float4 oSmoothNormal : OUTCOLOR3,
    out float4 oAlbedo : OUTCOLOR4,
    out float4 oEmissive : OUTCOLOR5)
{
    float4 diffColor = cMatDiffColor;
    #ifdef DIFFMAP
        diffColor *= Sample2D(DiffMap, iTexCoord.xy);
    #endif

    float3 emissiveColor = cMatEmissiveColor;
    #ifdef EMISSIVEMAP
        emissiveColor *= Sample2D(EmissiveMap, iTexCoord.xy).rgb;
    #endif

    float3 normal = normalize(iNormal);

    float3 dPdx = ddx(iWorldPos.xyz);
    float3 dPdy = ddy(iWorldPos.xyz);
    float3 faceNormal = normalize(cross(dPdx, dPdy));
    if (dot(faceNormal, normal) < 0)
        faceNormal *= -1.0;

    float3 dPmax = max(abs(dPdx), abs(dPdy));
    float texelRadius = max(dPmax.x, max(dPmax.y, dPmax.z)) * 1.4142135 * 0.5;

    float scaledBias = iMetadata.y;
    float constBias = iMetadata.z;
    float3 biasScale = max(abs(iWorldPos.xyz), float3(1.0, 1.0, 1.0));
    float3 position = iWorldPos.xyz + sign(faceNormal) * biasScale * scaledBias + faceNormal * constBias;

    oPosition = float4(position, iMetadata.x);
    oSmoothPosition = float4(position, texelRadius);
    oFaceNormal = float4(faceNormal, 1.0);
    oSmoothNormal = float4(normal, 1.0);
    oAlbedo = float4(diffColor.rgb, 1.0);
    oEmissive = float4(emissiveColor, 1.0);
}
