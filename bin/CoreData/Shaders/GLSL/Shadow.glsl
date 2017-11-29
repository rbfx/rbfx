#ifdef BGFX_SHADER
#include "varying_shadow.def.sc"
#include "urho3d_compatibility.sh"
#ifdef COMPILEVS
    $input a_position _NORMAL _TEXCOORD0 _COLOR0 _TEXCOORD1 _ATANGENT _SKINNED _INSTANCED
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

#ifdef VSM_SHADOW
    varying vec4 vTexCoord;
#else
    varying vec2 vTexCoord;
#endif

#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    #ifdef VSM_SHADOW
        vTexCoord = vec4(GetTexCoord(iTexCoord), gl_Position.z, gl_Position.w);
    #else
        vTexCoord = GetTexCoord(iTexCoord);
    #endif
}

void PS()
{
    #ifdef ALPHAMASK
        float alpha = texture2D(sDiffMap, vTexCoord.xy).a;
        if (alpha < 0.5)
            discard;
    #endif

    #ifdef VSM_SHADOW
        float depth = vTexCoord.z / vTexCoord.w * 0.5 + 0.5;
        gl_FragColor = vec4(depth, depth * depth, 1.0, 1.0);
    #else
        gl_FragColor = vec4(1.0);
    #endif
}
