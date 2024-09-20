#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_DefaultSamplers.glsl"
#include "_SamplerUtils.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER
    UNIFORM_BUFFER_BEGIN(6, Custom)
        UNIFORM(mediump float cIntensity)
        UNIFORM(mediump float cRadius)
        UNIFORM(mediump vec4 cColor)
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
    // Sample screen buffer
    half4 rgb = texture(sAlbedo, vScreenPos);

    // Calculate scale for screen UV coordinates to preserve shape of the circle
    vec2 aspectRatioCorrection = min(vec2(1.0, 1.0), vec2(cGBufferInvSize.y/cGBufferInvSize.x, cGBufferInvSize.x/cGBufferInvSize.y));

    // Calculate screen coordinates in range -1 .. +1
    vec2 screenPos = (vScreenPos.xy * 2.0 - 1.0) * aspectRatioCorrection;

    // Calculate distance from screen center to the current pixel in range 0 .. +1
    float len = length(screenPos);
    float vignette = mix(1.0-cColor.a, 1.0, smoothstep(cRadius, cRadius - max(0.0, cIntensity), len));
    gl_FragColor = vec4(mix(rgb.rgb * cColor.rgb, rgb.rgb, vignette), rgb.a);
}
#endif
