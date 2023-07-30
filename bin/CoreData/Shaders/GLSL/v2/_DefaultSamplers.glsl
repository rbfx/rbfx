#ifndef _DEFAULT_SAMPLERS_GLSL_
#define _DEFAULT_SAMPLERS_GLSL_

#ifndef _CONFIG_GLSL_
    #error Include _Config.glsl before _DefaultSamplers.glsl
#endif

#ifdef URHO3D_PIXEL_SHADER

SAMPLER(3, sampler2D sDiffMap)
SAMPLER(4, sampler2D sNormalMap)
SAMPLER(5, sampler2D sSpecMap)
SAMPLER(6, sampler2D sEmissiveMap)
#ifdef URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT
    SAMPLER(7, sampler2D sEnvMap)
#else
    SAMPLER(8, samplerCube sEnvMap)
#endif
SAMPLER(9, sampler2D sDepthBuffer)
SAMPLER(10, samplerCube sZoneCubeMap)

#endif // URHO3D_PIXEL_SHADER
#endif // _DEFAULT_SAMPLERS_GLSL_
