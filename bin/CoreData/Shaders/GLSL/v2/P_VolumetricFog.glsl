#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_DefaultSamplers.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER
UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(half cDensity)
    UNIFORM(half cHeightFalloff)
    UNIFORM(half cAnisotropy)
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
    half heightTerm = 1.0 - vScreenPos.y;
    half opticalDepth = clamp(cDensity * mix(1.0, heightTerm, clamp(cHeightFalloff, 0.0, 1.0)), 0.0, 1.0);
    half phase = 0.5 + 0.5 * cAnisotropy;
    half3 fogColor = vec3(0.48, 0.60, 0.72) * phase;
    gl_FragColor = vec4(mix(scene, fogColor, opticalDepth), 1.0);
}
#endif
