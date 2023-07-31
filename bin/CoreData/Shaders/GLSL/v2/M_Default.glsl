#define URHO3D_PIXEL_NEED_TEXCOORD

#include "_Material.glsl"
#include "_DefaultSamplers.glsl"

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    Vertex_SetAll(vertexTransform, cNormalScale, cUOffset, cVOffset, cLMOffset);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef URHO3D_DEPTH_ONLY_PASS
    Pixel_DepthOnly(sAlbedo, vTexCoord);
#else
    SurfaceData surfaceData;

    Surface_SetCommon(surfaceData);
    Surface_SetAmbient(surfaceData, sEmission, vTexCoord2);
    Surface_SetNormal(surfaceData, vNormal, sNormal, vTexCoord, vTangent, vBitangentXY);
    Surface_SetPhysicalProperties(surfaceData, cRoughness, cMetallic, cDielectricReflectance, sProperties, vTexCoord);
    Surface_SetLegacyProperties(surfaceData, cMatSpecColor.a, sEmission, vTexCoord);
    Surface_SetCubeReflection(surfaceData, sReflection0, sReflection1, vReflectionVec, vWorldPos);
    Surface_SetPlanarReflection(surfaceData, sReflection0, cReflectionPlaneX, cReflectionPlaneY);
    Surface_SetBackground(surfaceData, sEmission, sDepthBuffer);
    Surface_SetBaseAlbedo(surfaceData, cMatDiffColor, cAlphaCutoff, vColor, sAlbedo, vTexCoord, URHO3D_MATERIAL_DIFFUSE_HINT);
    Surface_SetBaseSpecular(surfaceData, cMatSpecColor, cMatEnvMapColor, sProperties, vTexCoord);
    Surface_SetAlbedoSpecular(surfaceData);
    Surface_SetEmission(surfaceData, cMatEmissiveColor, sEmission, vTexCoord, URHO3D_MATERIAL_EMISSIVE_HINT);
    Surface_ApplySoftFadeOut(surfaceData, vWorldDepth, cFadeOffsetScale);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceData.albedo.a, surfaceData.fogFactor);
#endif
}
#endif
