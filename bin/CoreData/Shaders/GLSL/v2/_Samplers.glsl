#ifndef _SAMPLERS_GLSL_
#define _SAMPLERS_GLSL_

#ifndef _CONFIG_GLSL_
    #error Include _Config.glsl before _Samplers.glsl
#endif

#ifdef URHO3D_PIXEL_SHADER

#ifndef URHO3D_DISABLE_TEXTURES
SAMPLER(0, sampler2D sDiffMap)
SAMPLER(1, sampler2D sNormalMap)
SAMPLER(2, sampler2D sSpecMap)
SAMPLER(3, sampler2D sEmissiveMap)
#ifdef URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT
    SAMPLER(4, sampler2D sEnvMap)
#else
    SAMPLER(4, samplerCube sEnvMap)
#endif
SAMPLER(8, sampler2D sLightRampMap)
#ifndef URHO3D_LIGHT_POINT
    SAMPLER(9, sampler2D sLightSpotMap)
#else
    SAMPLER(9, samplerCube sLightSpotMap)
#endif
#if defined(URHO3D_VARIANCE_SHADOW_MAP)
    SAMPLER_HIGHP(10, sampler2D sShadowMap)
#else
    SAMPLER_HIGHP(10, sampler2DShadow sShadowMap)
#endif
SAMPLER(13, sampler2D sDepthBuffer)
SAMPLER(15, samplerCube sZoneCubeMap)
#endif

/// Helpers to sample sDiffMap in specified color space.
#ifdef URHO3D_MATERIAL_DIFFUSE_HINT
    #if URHO3D_MATERIAL_DIFFUSE_HINT == 0
        #define DiffMap_ToGamma(color)  Texture_ToGammaAlpha_0(color)
        #define DiffMap_ToLinear(color) Texture_ToLinearAlpha_0(color)
        #define DiffMap_ToLight(color)  Texture_ToLightAlpha_0(color)
    #elif URHO3D_MATERIAL_DIFFUSE_HINT == 1
        #define DiffMap_ToGamma(color)  Texture_ToGammaAlpha_1(color)
        #define DiffMap_ToLinear(color) Texture_ToLinearAlpha_1(color)
        #define DiffMap_ToLight(color)  Texture_ToLightAlpha_1(color)
    #endif
#endif

/// Helpers to sample sEmissiveMap in specified color space.
#ifdef URHO3D_MATERIAL_EMISSIVE_HINT
    #if URHO3D_MATERIAL_EMISSIVE_HINT == 0
        #define EmissiveMap_ToGamma(color)  Texture_ToGamma_0(color)
        #define EmissiveMap_ToLinear(color) Texture_ToLinear_0(color)
        #define EmissiveMap_ToLight(color)  Texture_ToLight_0(color)
    #elif URHO3D_MATERIAL_EMISSIVE_HINT == 1
        #define EmissiveMap_ToGamma(color)  Texture_ToGamma_1(color)
        #define EmissiveMap_ToLinear(color) Texture_ToLinear_1(color)
        #define EmissiveMap_ToLight(color)  Texture_ToLight_1(color)
    #endif
#endif

/// Convert sampled value from sNormalMap to normal in tangent space.
half3 DecodeNormal(half4 normalInput)
{
    #ifdef PACKEDNORMAL
        half3 normal;
        normal.xy = normalInput.ag * 2.0 - 1.0;
        normal.z = sqrt(max(1.0 - dot(normal.xy, normal.xy), 0.0));
        return normal;
    #else
        return normalize(normalInput.rgb * 2.0 - 1.0);
    #endif
}

/// Convert sampled depth buffer value to linear depth in [0, 1] range.
/// For orthographic cameras, 0 is near plane.
/// For perspective cameras, 0 is focus point and is never actually returned.
float ReconstructDepth(float hwDepth)
{
    // May be undefined for orthographic projection
    float depth = cDepthReconstruct.y / (hwDepth - cDepthReconstruct.x);
    return cDepthReconstruct.z != 0.0 ? hwDepth : depth;
}

#endif // URHO3D_PIXEL_SHADER
#endif // _SAMPLERS_GLSL_
