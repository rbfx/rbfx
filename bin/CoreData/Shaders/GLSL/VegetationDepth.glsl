#ifdef BGFX_SHADER
#include "varying_scenepass_depth.def.sc"
#include "urho3d_compatibility.sh"
#ifdef COMPILEVS
    $input a_position a_texcoord0 _SKINNED _INSTANCED
    $output vTexCoord
#endif
#ifdef COMPILEPS
    $input vTexCoord
#endif

#include "common.sh"

#include "uniforms.sh"
#include "transform.sh"

uniform vec4 u_WindHeightFactor;
uniform vec4 u_WindHeightPivot;
uniform vec4 u_WindPeriod;
uniform vec4 u_WindWorldSpacing;
#define cWindHeightFactor u_WindHeightFactor.x
#define cWindHeightPivot u_WindHeightPivot.x
#define cWindPeriod u_WindPeriod.x
#define cWindWorldSpacing vec2(u_WindWorldSpacing.xy)

#else

#include "Uniforms.glsl"
#include "Transform.glsl"

uniform float cWindHeightFactor;
uniform float cWindHeightPivot;
uniform float cWindPeriod;
uniform vec2 cWindWorldSpacing;

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

