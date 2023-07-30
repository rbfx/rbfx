#define URHO3D_PIXEL_NEED_TEXCOORD

#include "_Material.glsl"
#include "_DefaultSamplers.glsl"

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform, cNormalScale);
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
    FillSurfaceCubeReflection(surfaceData, sEnvMap, sZoneCubeMap, vReflectionVec, vWorldPos);
    FillSurfacePlanarReflection(surfaceData, sEnvMap, cReflectionPlaneX, cReflectionPlaneY);
    FillSurfaceBackground(surfaceData, sEmissiveMap, sDepthBuffer);
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
