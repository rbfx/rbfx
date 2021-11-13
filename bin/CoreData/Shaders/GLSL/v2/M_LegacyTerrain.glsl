#define URHO3D_CUSTOM_MATERIAL_UNIFORMS
#define URHO3D_DISABLE_DIFFUSE_SAMPLING
#define URHO3D_DISABLE_NORMAL_SAMPLING
#define URHO3D_DISABLE_SPECULAR_SAMPLING
#define URHO3D_DISABLE_EMISSIVE_SAMPLING

#ifndef URHO3D_PIXEL_NEED_SCREEN_POSITION
    #define URHO3D_PIXEL_NEED_SCREEN_POSITION
#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half2 cDetailTiling)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vDetailTexCoord)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
    vDetailTexCoord = vTexCoord * cDetailTiling;
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceBackground(surfaceData);

    half3 weights = texture2D(sDiffMap, vTexCoord).rgb;
    half sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    surfaceData.albedo =
        weights.r * texture2D(sNormalMap, vDetailTexCoord) +
        weights.g * texture2D(sSpecMap, vDetailTexCoord) +
        weights.b * texture2D(sEmissiveMap, vDetailTexCoord);
#ifdef URHO3D_PHYSICAL_MATERIAL
    surfaceData.albedo.rgb = GammaToLightSpace(cMatDiffColor.rgb) * LinearToLightSpace(surfaceData.albedo.rgb);
#else
    surfaceData.albedo.rgb = GammaToLightSpace(cMatDiffColor.rgb) * GammaToLightSpace(surfaceData.albedo.rgb);
#endif

#ifdef URHO3D_PIXEL_NEED_VERTEX_COLOR
    albedo *= LinearToLightSpaceAlpha(vColor);
#endif

#ifdef URHO3D_PHYSICAL_MATERIAL
    surfaceData.specular = surfaceData.albedo.rgb * (1.0 - surfaceData.oneMinusReflectivity);
    surfaceData.albedo.rgb *= surfaceData.oneMinusReflectivity;
#else
    surfaceData.specular = GammaToLightSpace(cMatSpecColor.rgb);
#endif    

#ifdef URHO3D_SURFACE_NEED_AMBIENT
    surfaceData.emission = GammaToLightSpace(cMatEmissiveColor);
#endif

    half3 finalColor = GetFinalColor(surfaceData);
    gl_FragColor.rgb = ApplyFog(finalColor, surfaceData.fogFactor);
    gl_FragColor.a = GetFinalAlpha(surfaceData);
}
#endif
