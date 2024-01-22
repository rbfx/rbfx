#ifndef UNLIT
    #define UNLIT
#endif
#ifdef URHO3D_PIXEL_NEED_SCREEN_POSITION
    #define URHO3D_PIXEL_NEED_SCREEN_POSITION
#endif
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#include "_Config.glsl"
#include "_Uniforms.glsl"

// JS: this has to be before samplers so we can see it for depth reconstruction
#ifdef URHO3D_XR
    VERTEX_OUTPUT_QUAL(flat, int vInstID)
#endif

#include "_SamplerUtils.glsl"
#include "_VertexLayout.glsl"

#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_GammaCorrection.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(vec2 cAlignment)
    UNIFORM(float cPixelPerfect)
UNIFORM_BUFFER_END(4, Material)

SAMPLER(3, sampler2D sAlbedo)

VERTEX_OUTPUT_HIGHP(vec4 vScreenPos)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    mat4 modelMatrix = GetModelMatrix();
    vec4 worldPos = vec4(iPos.xyz, 0.0) * modelMatrix;
    worldPos.xyz += STEREO_VAR(cCameraPos).xyz;
    worldPos.w = 1.0;

    gl_Position = WorldToClipSpace(worldPos.xyz);
    gl_Position.z = gl_Position.w;

    vScreenPos = GetScreenPos(gl_Position);
    #ifndef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
        vScreenPos.y = 1.0 - vScreenPos.y;
    #endif

    #ifdef URHO3D_XR
        vInstID = gl_InstanceID;
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    vec2 screenPos = vScreenPos.xy / vScreenPos.w;
    vec2 screenSizePx = vec2(1.0, 1.0) / cGBufferInvSize.xy;
    vec2 screenPosPx = screenPos * screenSizePx;
    vec2 textureSizePx = vec2(textureSize(sAlbedo, 0));
    vec2 scalePerDirection = screenSizePx / textureSizePx;
    float fillScale = max(scalePerDirection.x, scalePerDirection.y);
    vec2 targetTextureSizePx = textureSizePx * mix(fillScale, 1.0, cPixelPerfect);
    vec2 uv = (screenPosPx - screenSizePx * cAlignment) / targetTextureSizePx + cAlignment;
    half4 textureInput = texture(sAlbedo, uv);
    half4 textureColor = (URHO3D_TEXTURE_ALBEDO == 1 ? Texture_ToLightAlpha_1(textureInput) : Texture_ToLightAlpha_2(textureInput));
    gl_FragColor = GammaToLightSpaceAlpha(cMatDiffColor) * textureColor;
}
#endif
