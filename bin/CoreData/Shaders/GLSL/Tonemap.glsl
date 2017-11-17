#ifdef BGFX_SHADER
#include "urho3d_compatibility.sh"
#ifdef BGFX_SHADER_TYPE_VERTEX == 1
    $input a_position
    $output vScreenPos
#endif
#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
    $input vScreenPos
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

varying vec2 vScreenPos;

#endif

#ifdef COMPILEPS
uniform float cTonemapExposureBias;
uniform float cTonemapMaxWhite;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

void PS()
{
    #ifdef REINHARDEQ3
    vec3 color = ReinhardEq3Tonemap(max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0));
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef REINHARDEQ4
    vec3 color = ReinhardEq4Tonemap(max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0), cTonemapMaxWhite);
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef UNCHARTED2
    vec3 color = Uncharted2Tonemap(max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0)) / 
        Uncharted2Tonemap(vec3(cTonemapMaxWhite, cTonemapMaxWhite, cTonemapMaxWhite));
    gl_FragColor = vec4(color, 1.0);
    #endif
}

