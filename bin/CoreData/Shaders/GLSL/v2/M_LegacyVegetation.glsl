#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#include "_Config.glsl"
#include "_Uniforms.glsl"

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

    FillVertexOutputs(vertexTransform);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef URHO3D_DEPTH_ONLY_PASS
    DepthOnlyPixelShader(sDiffMap, vTexCoord);
#else
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceAmbient(surfaceData, sEmissiveMap, vTexCoord2);
    FillSurfaceNormal(surfaceData, vNormal, sNormalMap, vTexCoord, vTangent, vBitangentXY);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData, sSpecMap, vTexCoord, sEmissiveMap, vTexCoord);
    FillSurfaceReflectionColor(surfaceData, sEnvMap, sZoneCubeMap, vReflectionVec, vWorldPos);
    FillSurfaceBackground(surfaceData);
    FillSurfaceBaseAlbedo(surfaceData, vColor, sDiffMap, vTexCoord, URHO3D_MATERIAL_DIFFUSE_HINT);
    FillSurfaceBaseSpecular(surfaceData, sSpecMap, vTexCoord);
    FillSurfaceAlbedoSpecular(surfaceData);
    FillSurfaceEmission(surfaceData, sEmissiveMap, vTexCoord, URHO3D_MATERIAL_EMISSIVE_HINT);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
#endif
}
#endif
