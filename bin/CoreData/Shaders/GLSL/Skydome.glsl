#ifdef BGFX_SHADER
#include "varying_scenepass.def.sc"
#include "urho3d_compatibility.sh"
#ifdef COMPILEVS
    $input a_position
    $output vTexCoord
#endif
#ifdef COMPILEPS
    $input vTexCoord
#endif

#include "common.sh"

#include "uniforms.sh"
#include "samplers.sh"
#include "transform.sh"

#else

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;

#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix, iPos);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iTexCoord.xy;
}

void PS()
{
    gl_FragColor = texture2D(sDiffMap, vTexCoord);
}
