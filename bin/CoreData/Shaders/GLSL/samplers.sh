#ifndef __SAMPLERS_SH__
#define __SAMPLERS_SH__

#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
SAMPLER2D(sDiffMap, 0);
SAMPLERCUBE(sDiffCubeMap, 0);
SAMPLER2D(sNormalMap, 1);
SAMPLER2D(sSpecMap, 2);
SAMPLER2D(sEmissiveMap, 3);
SAMPLER2D(sEnvMap, 4);
SAMPLERCUBE(sEnvCubeMap, 4);
#ifndef GL_ES
    SAMPLER2D(sAlbedoBuffer, 0);
    SAMPLER2D(sNormalBuffer, 1);
    SAMPLERCUBE(sLightCubeMap, 6);
    SAMPLER3D(sVolumeMap, 5);
    SAMPLER2D(sLightRampMap, 8);
    SAMPLER2D(sLightSpotMap, 9);
    SAMPLER2D(sDepthBuffer, 13);
    SAMPLER2D(sLightBuffer, 14);
    #ifdef VSM_SHADOW
        SAMPLER2D(sShadowMap, 10);
    #else
        SAMPLER2DSHADOW(sShadowMap, 10);
    #endif
    SAMPLERCUBE(sFaceSelectCubeMap, 11);
    SAMPLERCUBE(sIndirectionCubeMap, 12);
    SAMPLERCUBE(sZoneCubeMap, 15);
    SAMPLER3D(sZoneVolumeMap, 15);
#else
    SAMPLER2D(sLightRampMap, 5);
    SAMPLER2D(sLightSpotMap, 6);
    SAMPLER2DSHADOW(sShadowMap, 7);
#endif

vec3 DecodeNormal(vec4 normalInput)
{
    #ifdef PACKEDNORMAL
        vec3 normal;
        normal.xy = normalInput.ag * 2.0 - 1.0;
        normal.z = sqrt(max(1.0 - dot(normal.xy, normal.xy), 0.0));
        return normal;
    #else
        return normalize(normalInput.rgb * 2.0 - 1.0);
    #endif
}

vec3 EncodeDepth(float depth)
{
    #ifndef GL3
        vec3 ret;
        depth *= 255.0;
        ret.x = floor(depth);
        depth = (depth - ret.x) * 255.0;
        ret.y = floor(depth);
        ret.z = (depth - ret.y);
        ret.xy *= 1.0 / 255.0;
        return ret;
    #else
        // OpenGL 3 can use different MRT formats, so no need for encoding
        return vec3(depth, 0.0, 0.0);
    #endif
}

float DecodeDepth(vec3 depth)
{
    #ifndef GL3
        const vec3 dotValues = vec3(1.0, 1.0 / 255.0, 1.0 / (255.0 * 255.0));
        return dot(depth, dotValues);
    #else
        // OpenGL 3 can use different MRT formats, so no need for encoding
        return depth.r;
    #endif
}

float ReconstructDepth(float hwDepth)
{
    return dot(vec2(hwDepth, cDepthReconstruct.y / (hwDepth - cDepthReconstruct.x)), cDepthReconstruct.zw);
}
#endif

#endif
