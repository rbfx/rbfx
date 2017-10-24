#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "PostProcess.hlsl"

#ifndef D3D11

// D3D9 uniforms
uniform float cTonemapExposureBias;
uniform float cTonemapMaxWhite;

#else

#ifdef COMPILEPS
// D3D11 constant buffers
cbuffer CustomPS : register(b6)
{
    float cTonemapExposureBias;
    float cTonemapMaxWhite;
}
#endif

#endif

void VS(float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    #ifdef REINHARDEQ3
    float3 color = ReinhardEq3Tonemap(max(Sample2D(DiffMap, iScreenPos).rgb * cTonemapExposureBias, 0.0));
    color = ToInverseGamma(color);
    oColor = float4(color, 1.0);
    #endif

    #ifdef REINHARDEQ4
    float3 color = ReinhardEq4Tonemap(max(Sample2D(DiffMap, iScreenPos).rgb * cTonemapExposureBias, 0.0), cTonemapMaxWhite);
    color = ToInverseGamma(color);
    oColor = float4(color, 1.0);
    #endif

    #ifdef REINHARDEQ5
    float3 color = Sample2D(DiffMap, iScreenPos).rgb;
    float white = 2.2;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
    color *= toneMappedLuma / luma;
    color = ToInverseGamma(color);
    oColor = float4(color, 1.0);
    #endif

    #ifdef ACES
    float3 color = Sample2D(DiffMap, iScreenPos).rgb;
    color = ACES(color);
    color = ToInverseGamma(color);
    oColor = float4(color, 1.0);
    #endif

    #ifdef FILMIC
    float3 color = Sample2D(DiffMap, iScreenPos).rgb;
    color = max(float3(0.0, 0.0, 0.0), color - float3(0.004, 0.004, 0.004));
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    oColor = float4(color, 1.0);
    #endif

    #ifdef UNCHARTED2
    float3 color = Uncharted2Tonemap(max(Sample2D(DiffMap, iScreenPos).rgb * cTonemapExposureBias, 0.0)) / 
        Uncharted2Tonemap(float3(cTonemapMaxWhite, cTonemapMaxWhite, cTonemapMaxWhite));
    color = ToInverseGamma(color);
    oColor = float4(color, 1.0);
    #endif
}
