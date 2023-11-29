#ifndef UNLIT
    #define UNLIT
#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"

// JS: this has to be before samplers so we can see it for depth reconstruction
#ifdef URHO3D_XR
    VERTEX_OUTPUT_QUAL(flat, int vInstID)
#endif

#include "_SamplerUtils.glsl"
#include "_VertexLayout.glsl"

#include "_VertexTransform.glsl"
#include "_GammaCorrection.glsl"

SAMPLER(3, sampler2D sAlbedo)

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    mat4 modelMatrix = GetModelMatrix();
    vec4 worldPos = vec4(iPos.xyz, 0.0) * modelMatrix;
    worldPos.xyz += STEREO_VAR(cCameraPos).xyz;
    worldPos.w = 1.0;

    gl_Position = WorldToClipSpace(worldPos.xyz);
    gl_Position.z = gl_Position.w;
    vTexCoord = iTexCoord;

    #ifdef URHO3D_XR
        vInstID = gl_InstanceID;
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    half4 textureInput = texture(sAlbedo, vTexCoord);
    half4 textureColor = (URHO3D_TEXTURE_ALBEDO == 1 ? Texture_ToLightAlpha_1(textureInput) : Texture_ToLightAlpha_2(textureInput));
    gl_FragColor = GammaToLightSpaceAlpha(cMatDiffColor) * textureColor;
}
#endif
