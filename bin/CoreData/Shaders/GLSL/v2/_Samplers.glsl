#ifndef _SAMPLERS_GLSL_
#define _SAMPLERS_GLSL_

#ifndef _CONFIG_GLSL_
    #error Include _Config.glsl before _Samplers.glsl
#endif

#ifdef URHO3D_PIXEL_SHADER

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

#endif // URHO3D_PIXEL_SHADER
#endif // _SAMPLERS_GLSL_
