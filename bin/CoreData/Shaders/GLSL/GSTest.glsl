#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

#line 7

#ifdef COMPILEVS

out VSDataOut {
    vec4 worldPos;
} vData;
    
void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = (iPos * modelMatrix).xyz;
    vec4 clipPos = GetClipPos(worldPos);
    gl_Position = clipPos;
    vData.worldPos = clipPos;
}

#endif

#ifdef COMPILEGS

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

in VSDataOut {
    vec4 worldPos;
} gsDataIn[];


out GSDataOut {
    vec4 worldPos;
    vec4 color;
} gsDataOut;

void CreateVertex(vec4 pos, vec4 col)
{
    gl_Position = pos;
    gsDataOut.worldPos = pos;
    gsDataOut.color = col;
    EmitVertex();
}

void GS()
{
    const float gLayers = 3.0;
    float length = 0.1;

    vec4 offsets[] = {
        vec4(3, 3, 0, 0),
        vec4(-3, -3, 0, 0),
        vec4(3, 0, -3, 0)
    };
    vec4 colors[] = {
        vec4(0.8, 0.6, 0.3, 0.5),
        vec4(0.2, 0.9, 0.5, 0.5),
        vec4(0.1, 0.33, 0.7, 0.5),
    };

    float offset = length / gLayers;
    
    CreateVertex(gsDataIn[0].worldPos, colors[0]);
    CreateVertex(gsDataIn[1].worldPos, colors[0]);
    CreateVertex(gsDataIn[2].worldPos, colors[0]);
    EndPrimitive();

    for (int i = 0; i < 3; ++i)
    {
        vec4 norm = offsets[i];
        CreateVertex(gsDataIn[0].worldPos + vec4(norm*0.03) + gsDataIn[0].worldPos*0.1, colors[i]);
        CreateVertex(gsDataIn[1].worldPos, colors[i]);
        CreateVertex(gsDataIn[2].worldPos + vec4(norm*0.06) + gsDataIn[2].worldPos*0.1, colors[i]);
        EndPrimitive();
    }
}

#endif

#ifdef COMPILEPS

out vec4 oColor;

in GSDataOut {
    vec4 worldPos;
    vec4 color;
} gsDataOut;

void PS()
{
    oColor = gsDataOut.color;
}

#endif