#define URHO3D_PIXEL_NEED_TEXCOORD

#include "_Config.glsl"
#include "_GammaCorrection.glsl"
#include "_BRDF.glsl"

#include "_Uniforms.glsl"
#include "_Samplers.glsl"
#include "_VertexLayout.glsl"

#include "_VertexTransform.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
#ifdef URHO3D_VERTEX_HAS_COLOR
    VERTEX_OUTPUT(half4 vColor)
#endif

#ifdef URHO3D_VERTEX_SHADER

SAMPLER(0, sampler2D sDiffMap)
SAMPLER(1, sampler2D sNormalMap)

VertexTransform GetCustomVertexTransform()
{
    mat4 modelMatrix = GetModelMatrix();

    vec2 offset = vec2(0, fract(cElapsedTime*0.144));
    VertexTransform result;

    vec3 pos = texture2D(sDiffMap, iTexCoord + offset).xyz;
    result.position = vec4(pos, 1) * modelMatrix;

    #ifdef URHO3D_VERTEX_NEED_NORMAL
        mediump mat3 normalMatrix = GetNormalMatrix(modelMatrix);
        vec3 norm = (texture2D(sNormalMap, iTexCoord + offset).xyz - vec3(0.5, 0.5, 0.5)) * vec3(2.0, 2.0, 2.0);
        result.normal = normalize(norm * normalMatrix);

        ApplyShadowNormalOffset(result.position, result.normal);

        #ifdef URHO3D_VERTEX_NEED_TANGENT
            result.tangent = normalize(iTangent.xyz * normalMatrix);
            result.bitangent = cross(result.tangent, result.normal) * iTangent.w;
        #endif
    #endif

    return result;
}

void main()
{
    VertexTransform vertexTransform = GetCustomVertexTransform();
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
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    half4 diffColor = cMatDiffColor;

    #ifdef URHO3D_VERTEX_HAS_COLOR
        diffColor *= vColor;
    #endif

    gl_FragColor = GammaToLightSpaceAlpha(diffColor);
}
#endif
