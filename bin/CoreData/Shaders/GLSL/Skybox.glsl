#ifdef BGFX_SHADER
#include "varying_skybox.def.sc"
#include "urho3d_compatibility.sh"
#ifdef COMPILEVS
    $input a_position _INSTANCED
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

varying vec3 vTexCoord;

#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix, iPos);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iPos.xyz;
}

void PS()
{
    vec4 sky = cMatDiffColor * textureCube(sDiffCubeMap, vTexCoord);
    #ifdef HDRSCALE
        sky = pow(sky + clamp((cAmbientColor.a - 1.0) * 0.1, 0.0, 0.25), max(vec4(cAmbientColor.a), 1.0)) * clamp(cAmbientColor.a, 0.0, 1.0);
    #endif
    gl_FragColor = sky;
}
