#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_Samplers.glsl"
#include "_GammaCorrection.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(vec4 cSelectionColor)
    UNIFORM(vec2 cInputInvSize)
UNIFORM_BUFFER_END(6, Custom)

#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vTexCoord = GetQuadTexCoord(gl_Position);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER

vec4 SampleDiff(vec2 offset)
{
    vec2 uv = vTexCoord + offset * cInputInvSize;
    vec2 alphaScale = step(vec2(0.0, 0.0), uv) * (1.0 - step(vec2(1.0, 1.0), uv));
    return texture2D(sDiffMap, uv) * vec4(1,1,1,alphaScale.x* alphaScale.y);
}

void main()
{
    vec4 inputColor = vec4(0,0,0,0);// = texture2D(sDiffMap, vTexCoord).rgba;

    inputColor += SampleDiff(vec2(-1.0, 0.0)) * 0.25;
    inputColor += SampleDiff(vec2(0.0, 1.0)) * 0.25;
    inputColor += SampleDiff(vec2(1.0, 0.0)) * 0.25;
    inputColor += SampleDiff(vec2(0.0, -1.0)) * 0.25;
    inputColor.a = sin(inputColor.a * 3.1415926535897932384626433832795);

    if (inputColor.a < 0.1)
         discard;

    // vec4 inputColor = texture2D(sDiffMap, vTexCoord).rgba;
    // if (inputColor.a < 0.1)
    //     discard;
    gl_FragColor = vec4(LinearToGammaSpace(cSelectionColor.rgb), inputColor.a);
}
#endif
