#ifndef __SAMPLERS_SH__
#define __SAMPLERS_SH__

#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
SAMPLER2D(u_DiffMap, 0);
SAMPLERCUBE(u_DiffCubeMap, 0);
SAMPLER2D(u_NormalMap, 1);
SAMPLER2D(u_SpecMap, 2);
SAMPLER2D(u_EmissiveMap, 3);
SAMPLER2D(u_EnvMap, 4);
SAMPLERCUBE(u_EnvCubeMap, 4);
#ifndef URHO3D_MOBILE
    SAMPLER2D(u_AlbedoBuffer, 0);
    SAMPLER2D(u_NormalBuffer, 1);
    SAMPLERCUBE(u_LightCubeMap, 6);
    SAMPLER3D(u_VolumeMap, 5);
    SAMPLER2D(u_LightRampMap, 8);
    SAMPLER2D(u_LightSpotMap, 9);
    SAMPLER2D(u_DepthBuffer, 13);
    SAMPLER2D(u_LightBuffer, 14);
    #ifdef VSM_SHADOW
        SAMPLER2D(u_ShadowMap, 10);
    #else
        SAMPLER2DSHADOW(u_ShadowMap, 10);
    #endif
    SAMPLERCUBE(u_FaceSelectCubeMap, 11);
    SAMPLERCUBE(u_IndirectionCubeMap, 12);
    SAMPLERCUBE(u_ZoneCubeMap, 15);
    SAMPLER3D(u_ZoneVolumeMap, 15);
#else
    SAMPLER2D(u_LightRampMap, 5);
    SAMPLER2D(u_LightSpotMap, 6);
    SAMPLER2DSHADOW(u_ShadowMap, 7);
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
