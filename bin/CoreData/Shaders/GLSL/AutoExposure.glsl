#ifdef BGFX_SHADER
#include "varying_quad.def.sc"
#include "urho3d_compatibility.sh"
#ifdef COMPILEVS
    $input a_position
    $output vTexCoord, vScreenPos
#endif
#ifdef COMPILEPS
    $input vTexCoord, vScreenPos
#endif

#include "common.sh"

#include "uniforms.sh"
#include "samplers.sh"
#include "transform.sh"
#include "screen_pos.sh"
#include "post_process.sh"

#else

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "PostProcess.glsl"

varying vec2 vTexCoord;
varying vec2 vScreenPos;

#endif // BGFX_SHADER

#ifdef COMPILEPS
#ifdef BGFX_SHADER
uniform vec4 u_AutoExposureAdaptRate;
uniform vec4 u_AutoExposureLumRange;
uniform vec4 u_AutoExposureMiddleGrey;
uniform vec4 u_HDR128InvSize;
uniform vec4 u_Lum64InvSize;
uniform vec4 u_Lum16InvSize;
uniform vec4 u_Lum4InvSize;

#define cAutoExposureAdaptRate u_AutoExposureAdaptRate.x
#define cAutoExposureLumRange vec2(u_AutoExposureLumRange.xy)
#define cAutoExposureMiddleGrey u_AutoExposureMiddleGrey.x
#define cHDR128InvSize vec2(u_HDR128InvSize.xy)
#define cLum64InvSize vec2(u_Lum64InvSize.xy)
#define cLum16InvSize vec2(u_Lum16InvSize.xy)
#define cLum4InvSize vec2(u_Lum4InvSize.xy)
#else
uniform float cAutoExposureAdaptRate;
uniform vec2 cAutoExposureLumRange;
uniform float cAutoExposureMiddleGrey;
uniform vec2 cHDR128InvSize;
uniform vec2 cLum64InvSize;
uniform vec2 cLum16InvSize;
uniform vec2 cLum4InvSize;
#endif

float GatherAvgLum(sampler2D texSampler, vec2 texCoord, vec2 texelSize)
{
    float lumAvg = 0.0;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, -1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(-1.0, 1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, 1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, -1.0) * texelSize).r;
    return lumAvg / 4.0;
}
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetQuadTexCoord(gl_Position);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

void PS()
{
    #ifdef LUMINANCE64
    float logLumSum = 0.0;
    logLumSum += log(dot(texture2D(sDiffMap, vTexCoord + vec2(-1.0, -1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture2D(sDiffMap, vTexCoord + vec2(-1.0, 1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture2D(sDiffMap, vTexCoord + vec2(1.0, 1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture2D(sDiffMap, vTexCoord + vec2(1.0, -1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    gl_FragColor.r = logLumSum;
    #endif

    #ifdef LUMINANCE16
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum64InvSize);
    #endif

    #ifdef LUMINANCE4
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum16InvSize);
    #endif

    #ifdef LUMINANCE1
    gl_FragColor.r = exp(GatherAvgLum(sDiffMap, vTexCoord, cLum4InvSize) / 16.0);
    #endif

    #ifdef ADAPTLUMINANCE
    float adaptedLum = texture2D(sDiffMap, vTexCoord).r;
    float lum = clamp(texture2D(sNormalMap, vTexCoord).r, cAutoExposureLumRange.x, cAutoExposureLumRange.y);
    gl_FragColor.r = adaptedLum + (lum - adaptedLum) * (1.0 - exp(-cDeltaTimePS * cAutoExposureAdaptRate));
    #endif

    #ifdef EXPOSE
    vec3 color = texture2D(sDiffMap, vScreenPos).rgb;
    float adaptedLum = texture2D(sNormalMap, vTexCoord).r;
    gl_FragColor = vec4(color * (cAutoExposureMiddleGrey / adaptedLum), 1.0);
    #endif
}
