#ifdef BGFX_SHADER
#include "varying_scenepass_depth.def.sc"
#include "urho3d_compatibility.sh"
#ifdef BGFX_SHADER_TYPE_VERTEX == 1
    $input a_position a_texcoord0 _SKINNED _INSTANCED
    $output vTexCoord
#endif
#ifdef BGFX_SHADER_TYPE_FRAGMENT == 1
    $input vTexCoord
#endif

#include "common.sh"

#include "uniforms.sh"
#include "transform.sh"

#else

#include "Uniforms.glsl"
#include "Transform.glsl"

#endif

uniform float cWindHeightFactor;
uniform float cWindHeightPivot;
uniform float cWindPeriod;
uniform vec2 cWindWorldSpacing;

#ifndef BGFX_SHADER
varying vec3 vTexCoord;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    
    float windStrength = max(iPos.y - cWindHeightPivot, 0.0) * cWindHeightFactor;
    float windPeriod = cElapsedTime * cWindPeriod + dot(worldPos.xz, cWindWorldSpacing);
    worldPos.x += windStrength * sin(windPeriod);
    worldPos.z -= windStrength * cos(windPeriod);

    gl_Position = GetClipPos(worldPos);
    vTexCoord = vec3(GetTexCoord(iTexCoord), GetDepth(gl_Position));
}

