#include "_Config.glsl"

// Samplers
SAMPLER(0, sampler2D sAlbedo)

// Uniforms
UNIFORM_BUFFER_BEGIN(1, Camera)
    UNIFORM_HIGHP(mat4 cViewProj)
UNIFORM_BUFFER_END(1, Camera)

UNIFORM_BUFFER_BEGIN(5, Object)
    UNIFORM_HIGHP(mat4 cModel)
UNIFORM_BUFFER_END(5, Object)

// Vertex outputs
#if defined(DIFFMAP) || defined(ALPHAMAP)
    VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
#endif
#ifdef VERTEXCOLOR
    VERTEX_OUTPUT(half4 vColor)
#endif

#ifdef URHO3D_VERTEX_SHADER

// Vertex inputs
VERTEX_INPUT(vec4 iPos)
#if defined(DIFFMAP) || defined(ALPHAMAP)
    VERTEX_INPUT(vec2 iTexCoord)
#endif
#ifdef VERTEXCOLOR
    VERTEX_INPUT(vec4 iColor)
#endif

void main()
{
    vec3 worldPos = (iPos * cModel).xyz;
    gl_Position = vec4(worldPos, 1.0) * cViewProj;

    #if defined(DIFFMAP) || defined(ALPHAMAP)
        vTexCoord = iTexCoord;
    #endif
    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    const float cellSize = 32.0;
    const fixed4 colorA = vec4(0.0, 0.0, 0.0, 1.0);
    const fixed4 colorB = vec4(1.0, 0.0, 1.0, 1.0);

    ivec2 cellPosition = ivec2(gl_FragCoord.xy / cellSize);
    int colorIndex = (cellPosition.x + cellPosition.y) % 2;

    gl_FragColor = colorIndex == 0 ? colorA : colorB;
}
#endif
