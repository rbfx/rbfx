#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_DefaultSamplers.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER
UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(int cMaxSteps)
    UNIFORM(half cMaxDistance)
    UNIFORM(half cThickness)
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
    half3 scene = texture(sAlbedo, vScreenPos).rgb;
    // Conservative fallback reflection probe: screen-space grazing-angle tint.
    half grazing = clamp(abs(vScreenPos.x - 0.5) * 2.0 + cThickness * 0.05, 0.0, 1.0);
    half quality = clamp(float(cMaxSteps) / 64.0, 0.0, 1.0) * clamp(cMaxDistance / 100.0, 0.0, 1.0);
    half3 reflection = scene.bgr * (0.04 + 0.12 * quality);
    gl_FragColor = vec4(mix(scene, reflection, grazing * 0.25), 1.0);
}
#endif
