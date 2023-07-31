#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS
#define URHO3D_DISABLE_DIFFUSE_SAMPLING
#define URHO3D_DISABLE_NORMAL_SAMPLING
#define URHO3D_DISABLE_SPECULAR_SAMPLING
#define URHO3D_DISABLE_EMISSIVE_SAMPLING

#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_DefaultSamplers.glsl"

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
    Vertex_SetAll(vertexTransform, cNormalScale, cUOffset, cVOffset, cLMOffset);
    vDetailTexCoord = vTexCoord * cDetailTiling;
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    SurfaceData surfaceData;

    Surface_SetCommon(surfaceData);
    Surface_SetAmbient(surfaceData, sEmissiveMap, vTexCoord2);
    Surface_SetNormal(surfaceData, vNormal, sNormalMap, vTexCoord, vTangent, vBitangentXY);
    Surface_SetPhysicalProperties(surfaceData, cRoughness, cMetallic, cDielectricReflectance, sSpecMap, vTexCoord);
    Surface_SetLegacyProperties(surfaceData, cMatSpecColor.a, sEmissiveMap, vTexCoord);
    Surface_SetCubeReflection(surfaceData, sEnvMap, sZoneCubeMap, vReflectionVec, vWorldPos);
    Surface_SetPlanarReflection(surfaceData, sEnvMap, cReflectionPlaneX, cReflectionPlaneY);
    Surface_SetBackground(surfaceData, sEmissiveMap, sDepthBuffer);

    half3 weights = texture(sDiffMap, vTexCoord).rgb;
    half sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    surfaceData.albedo =
        weights.r * texture(sNormalMap, vDetailTexCoord) +
        weights.g * texture(sSpecMap, vDetailTexCoord) +
        weights.b * texture(sEmissiveMap, vDetailTexCoord);
    surfaceData.albedo = GammaToLightSpaceAlpha(cMatDiffColor) * GammaToLightSpaceAlpha(surfaceData.albedo);

    surfaceData.specular = GammaToLightSpace(cMatSpecColor.rgb);
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    surfaceData.emission = GammaToLightSpace(cMatEmissiveColor);
#endif

    Surface_ApplySoftFadeOut(surfaceData, vWorldDepth, cFadeOffsetScale);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceData.albedo.a, surfaceData.fogFactor);
}
#endif
