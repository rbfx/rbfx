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
        UNIFORM(mediump float cAmount)
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
    // Calculate scale for screen UV coordinates to preserve shape of the circle
    vec2 aspectRatioCorrection = min(vec2(1.0, 1.0), vec2(cGBufferInvSize.y/cGBufferInvSize.x, cGBufferInvSize.x/cGBufferInvSize.y));

    // Calculate screen coordinates in range -1 .. +1
    vec2 screenPos = (vScreenPos.xy * 2.0 - 1.0) * aspectRatioCorrection;

    float distanceToCenter = dot(screenPos, screenPos);

    // Calculate the offset for each color channel
    vec2 offset = cGBufferInvSize.xy * cAmount * screenPos * distanceToCenter;

    // Sample the texture at the offset positions
    float red = texture(sAlbedo, vScreenPos - offset).r;
    vec2 greenAlpha = texture(sAlbedo, vScreenPos).ga;
    float blue = texture(sAlbedo, vScreenPos + offset).b;

    gl_FragColor = vec4(red, greenAlpha.x, blue, greenAlpha.y);
}
#endif
