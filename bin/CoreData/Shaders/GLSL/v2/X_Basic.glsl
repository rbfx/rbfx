#include "_Config.glsl"

// Samplers
SAMPLER(0, sampler2D sDiffMap)

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
    vec4 diffColor = vec4(1.0, 1.0, 1.0, 1.0);

    #ifdef VERTEXCOLOR
        diffColor *= vColor;
    #endif

    #ifdef DIFFMAP
        vec4 diffInput = texture(sDiffMap, vTexCoord);
        #ifdef ALPHAMASK
            if (diffInput.a < 0.5)
                discard;
        #endif
        diffColor *= diffInput;
    #endif
    #ifdef ALPHAMAP
        float alphaInput = texture(sDiffMap, vTexCoord).r;
        diffColor.a *= alphaInput;
    #endif

    #ifdef URHO3D_LINEAR_OUTPUT
        diffColor.rgb *= diffColor.rgb * (diffColor.rgb * 0.305306011 + 0.682171111) + 0.012522878;
    #endif
    gl_FragColor = diffColor;
}
#endif
