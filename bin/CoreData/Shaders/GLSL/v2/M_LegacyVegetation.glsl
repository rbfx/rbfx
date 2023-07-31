#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_DefaultSamplers.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(float cWindHeightFactor)
    UNIFORM(float cWindHeightPivot)
    UNIFORM(float cWindPeriod)
    UNIFORM(vec2 cWindWorldSpacing)
#ifdef WINDSTEMAXIS
    UNIFORM(vec3 cWindStemAxis)
#endif
UNIFORM_BUFFER_END(4, Material)


#include "_Material.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vDetailTexCoord)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();

    vec3 worldPos = vertexTransform.position.xyz;
    #ifdef WINDSTEMAXIS
        float stemDistance = dot(iPos.xyz, cWindStemAxis);
    #else
        float stemDistance = iPos.y;
    #endif
    float windStrength = max(stemDistance - cWindHeightPivot, 0.0) * cWindHeightFactor;
    float windPeriod = cElapsedTime * cWindPeriod + dot(worldPos.xz, cWindWorldSpacing);
    worldPos.x += windStrength * sin(windPeriod);
    worldPos.z -= windStrength * cos(windPeriod);
    vertexTransform.position.xyz = worldPos;

    Vertex_SetAll(vertexTransform, cNormalScale, cUOffset, cVOffset, cLMOffset);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef URHO3D_DEPTH_ONLY_PASS
    Pixel_DepthOnly(sDiffMap, vTexCoord);
#else
    SurfaceData surfaceData;

    Surface_SetCommon(surfaceData);
    Surface_SetAmbient(surfaceData, sEmissiveMap, vTexCoord2);
    Surface_SetNormal(surfaceData, vNormal, sNormalMap, vTexCoord, vTangent, vBitangentXY);
    Surface_SetPhysicalProperties(surfaceData, cRoughness, cMetallic, cDielectricReflectance, sSpecMap, vTexCoord);
    Surface_SetLegacyProperties(surfaceData, cMatSpecColor.a, sEmissiveMap, vTexCoord);
    Surface_SetCubeReflection(surfaceData, sEnvMap, sZoneCubeMap, vReflectionVec, vWorldPos);
    Surface_SetPlanarReflection(surfaceData, sEnvMap, cReflectionPlaneX, cReflectionPlaneY);
    Surface_SetBackground(surfaceData, sEmissiveMap, sDepthBuffer);
    Surface_SetBaseAlbedo(surfaceData, cMatDiffColor, cAlphaCutoff, vColor, sDiffMap, vTexCoord, URHO3D_MATERIAL_DIFFUSE_HINT);
    Surface_SetBaseSpecular(surfaceData, cMatSpecColor, cMatEnvMapColor, sSpecMap, vTexCoord);
    Surface_SetAlbedoSpecular(surfaceData);
    Surface_SetEmission(surfaceData, cMatEmissiveColor, sEmissiveMap, vTexCoord, URHO3D_MATERIAL_EMISSIVE_HINT);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData, cFadeOffsetScale);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
#endif
}
#endif
