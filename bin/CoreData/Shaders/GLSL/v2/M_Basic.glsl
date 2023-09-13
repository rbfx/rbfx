#define URHO3D_PIXEL_NEED_TEXCOORD

#include "_Config.glsl"
#include "_GammaCorrection.glsl"
#include "_BRDF.glsl"

// JS: this has to be before samplers so we can see it for depth reconstruction
#ifdef URHO3D_XR
    VERTEX_OUTPUT(int vInstID)
#endif

#include "_Uniforms.glsl"
#include "_DefaultSamplers.glsl"
#include "_SamplerUtils.glsl"
#include "_VertexLayout.glsl"

#include "_VertexTransform.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
#ifdef URHO3D_VERTEX_HAS_COLOR
    VERTEX_OUTPUT(half4 vColor)
#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    ApplyClipPlane(gl_Position);

    #ifdef URHO3D_VERTEX_HAS_TEXCOORD0
        vTexCoord = iTexCoord;
    #else
        vTexCoord = vec2(0.0);
    #endif

    #ifdef URHO3D_VERTEX_HAS_COLOR
        vColor = iColor;
    #endif

    #ifdef URHO3D_XR
        vInstID = gl_InstanceID;
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    half4 diffColor = cMatDiffColor;

    #ifdef URHO3D_VERTEX_HAS_COLOR
        diffColor *= vColor;
    #endif

    #if URHO3D_TEXTURE_ALBEDO
        #ifdef ALPHAMAP
            half alphaInput = texture(sAlbedo, vTexCoord).r;
            diffColor.a *= alphaInput;
        #else
            half4 diffInput = texture(sAlbedo, vTexCoord);
            #ifdef ALPHAMASK
                if (diffInput.a < 0.5)
                    discard;
            #endif
            diffColor *= diffInput;
        #endif
    #endif

    gl_FragColor = GammaToLightSpaceAlpha(diffColor);
}
#endif
