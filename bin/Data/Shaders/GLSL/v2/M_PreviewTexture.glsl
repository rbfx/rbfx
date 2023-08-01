#define URHO3D_PIXEL_NEED_TEXCOORD

#include "_Config.glsl"
#include "_GammaCorrection.glsl"

#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"

#include "_VertexTransform.glsl"

#ifdef TEXTURE2DARRAY
    SAMPLER(6, sampler2DArray sTexture)
#endif
#ifdef TEXTURE3D
    SAMPLER(6, sampler3D sTexture)
#endif

VERTEX_OUTPUT_HIGHP(vec3 vWorldPos)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    ApplyClipPlane(gl_Position);

    vWorldPos = vertexTransform.position.xyz;
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    vec3 texCoord = vWorldPos;

    #ifdef TEXTURE2DARRAY
        float numSlices = textureSize(sTexture, 0).z;
        texCoord.z = mod(texCoord.z * 10 + 0.5, numSlices) - 0.5;
    #endif

    half4 diffColor = texture(sTexture, texCoord);
    gl_FragColor = GammaToLightSpaceAlpha(diffColor);
}
#endif
