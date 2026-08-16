#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_DefaultSamplers.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER
UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(half cFeedback)
    UNIFORM(half cSharpness)
    UNIFORM(half cJitterScale)
UNIFORM_BUFFER_END(6, Custom)
#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    half3 current = texture(sAlbedo, vScreenPos).rgb;
    // Stable reconstruction fallback: preserve luminance while applying bounded sharpening.
    half3 sharpened = current + (current - vec3(0.5)) * cSharpness * 0.25;
    half blend = clamp(cFeedback * max(cJitterScale, 0.0), 0.0, 1.0);
    gl_FragColor = vec4(mix(current, sharpened, blend), 1.0);
}
#endif
