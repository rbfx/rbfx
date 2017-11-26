#ifdef BGFX_SHADER
#include "varying_water.def.sc"
#include "urho3d_compatibility.sh"
#ifdef BGFX_SHADER_TYPE_VERTEX == 1
    $input a_position a_texcoord0 _INSTANCED
    $output vScreenPos, vReflectUV, vWaterUV, vNormal, vEyeVec
#endif
#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
    $input vScreenPos, vReflectUV, vWaterUV, vNormal, vEyeVec
#endif

#include "common.sh"

#include "uniforms.sh"
#include "samplers.sh"
#include "transform.sh"
#include "screen_pos.sh"
#include "fog.sh"

#else

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Fog.glsl"

#ifndef GL_ES
varying vec4 vScreenPos;
varying vec2 vReflectUV;
varying vec2 vWaterUV;
varying vec4 vEyeVec;
#else
varying highp vec4 vScreenPos;
varying highp vec2 vReflectUV;
varying highp vec2 vWaterUV;
varying highp vec4 vEyeVec;
#endif
varying vec3 vNormal;

#endif // BGFX_SHADER

#ifdef COMPILEVS
#ifdef BGFX_SHADER
uniform vec4 u_NoiseSpeed;
uniform vec4 u_NoiseTiling;
#define cNoiseSpeed vec2(u_NoiseSpeed.xy)
#define cNoiseTiling u_NoiseTiling.x
#else
uniform vec2 cNoiseSpeed;
uniform float cNoiseTiling;
#endif
#endif
#ifdef COMPILEPS
#ifdef BGFX_SHADER
uniform vec4 u_NoiseStrength;
uniform vec4 u_FresnelPower;
uniform vec4 u_WaterTint;
#define cNoiseStrength u_NoiseStrength.x
#define cFresnelPower u_FresnelPower.x
#define cWaterTint vec3(u_WaterTint.xyz)
#else
uniform float cNoiseStrength;
uniform float cFresnelPower;
uniform vec3 cWaterTint;
#endif
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPos(gl_Position);
    // GetQuadTexCoord() returns a vec2 that is OK for quad rendering; multiply it with output W
    // coordinate to make it work with arbitrary meshes such as the water plane (perform divide in pixel shader)
    // Also because the quadTexCoord is based on the clip position, and Y is flipped when rendering to a texture
    // on OpenGL, must flip again to cancel it out
    vReflectUV = GetQuadTexCoord(gl_Position);
    vReflectUV.y = 1.0 - vReflectUV.y;
    vReflectUV *= gl_Position.w;
    vWaterUV = iTexCoord * cNoiseTiling + cElapsedTime * cNoiseSpeed;
    vNormal = GetWorldNormal(modelMatrix);
    vEyeVec = vec4(cCameraPos - worldPos, GetDepth(gl_Position));
}

void PS()
{
    vec2 refractUV = vScreenPos.xy / vScreenPos.w;
    vec2 reflectUV = vReflectUV.xy / vScreenPos.w;

    vec2 noise = (texture2D(sNormalMap, vWaterUV).rg - 0.5) * cNoiseStrength;
    refractUV += noise;
    // Do not shift reflect UV coordinate upward, because it will reveal the clipping of geometry below water
    if (noise.y < 0.0)
        noise.y = 0.0;
    reflectUV += noise;

    float fresnel = pow(1.0 - clamp(dot(normalize(vEyeVec.xyz), vNormal), 0.0, 1.0), cFresnelPower);
    vec3 refractColor = texture2D(sEnvMap, refractUV).rgb * cWaterTint;
    vec3 reflectColor = texture2D(sDiffMap, reflectUV).rgb;
    vec3 finalColor = mix(refractColor, reflectColor, fresnel);

    gl_FragColor = vec4(GetFog(finalColor, GetFogFactor(vEyeVec.w)), 1.0);
}
