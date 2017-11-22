#ifdef BGFX_SHADER
#include "varying_scenepass.def.sc"
#include "urho3d_compatibility.sh"
#ifdef BGFX_SHADER_TYPE_VERTEX == 1
    $input a_position
    $output
#endif
#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
    $input
#endif

#include "common.sh"

#include "uniforms.sh"
#include "transform.sh"

#else

#include "Uniforms.glsl"
#include "Transform.glsl"

#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
}

void PS()
{
    gl_FragColor = vec4(1.0);
}

