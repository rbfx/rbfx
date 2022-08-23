#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_Samplers.glsl"
#include "_GammaCorrection.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)

#ifdef URHO3D_PIXEL_SHADER

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(vec2 cInputInvSize)
UNIFORM_BUFFER_END(6, Custom)

#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vTexCoord = GetQuadTexCoord(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER

vec4 Sample(vec2 offset)
{
    return texture2D(sDiffMap, vTexCoord + offset * cInputInvSize);
}

void main()
{
    #define EDGEDETECTION // TODO: Make configurable
    half cThickness = 1.3; // TODO: make this a uniform

    half3 offset = vec3(0.0, cThickness, -cThickness);
    half4 sample0 = Sample(offset.xx);
    half4 sample1 = Sample(offset.zz);
    half4 sample2 = Sample(offset.zy);
    half4 sample3 = Sample(offset.yz);
    half4 sample4 = Sample(offset.yy);

    half3 averageColor = (2 * sample0.rgb + sample1.rgb + sample2.rgb + sample3.rgb + sample4.rgb) * 0.2;
    half averageAlpha = (2 * sample0.a + sample1.a + sample2.a + sample3.a + sample4.a) * 0.2;

#ifdef EDGEDETECTION
    // Edge detection for color and alpha, with heuristics to scale up edge between two solid colors
    half4 edge4 = abs(4 * sample0 - sample1 - sample2 - sample3 - sample4);
    half edgeScale = max(0.0, averageAlpha);
    half edge = edgeScale * max(max(edge4.r, edge4.g), max(edge4.b, edge4.a));
#else
    // Simple edge detection based on alpha
    half edge = sin(averageAlpha * 3.1415926535897932384626433832795);
#endif

    half3 color = averageColor / max(0.001, averageAlpha);
    half alpha = clamp(edge, 0.0, 1.0);

    gl_FragColor = vec4(GammaToLightSpace(color), alpha);
}
#endif
