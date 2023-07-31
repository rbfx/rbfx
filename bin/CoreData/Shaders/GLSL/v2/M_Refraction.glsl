#define URHO3D_PIXEL_NEED_TEXCOORD

#define URHO3D_PIXEL_NEED_EYE_VECTOR
#define URHO3D_SURFACE_NEED_BACKGROUND_COLOR
#ifdef URHO3D_HAS_READABLE_DEPTH
    #define URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
#endif
#define URHO3D_SURFACE_NEED_NORMAL
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#ifndef NORMALMAP
    #define NORMALMAP
#endif

#ifndef ENVCUBEMAP
    #define ENVCUBEMAP
#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_DefaultSamplers.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half cNoiseStrength)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

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
    SurfaceData surfaceData;

    Surface_SetCommon(surfaceData);
    Surface_SetAmbient(surfaceData, sEmissiveMap, vTexCoord2);
    Surface_SetNormal(surfaceData, vNormal, sNormalMap, vTexCoord, vTangent, vBitangentXY);
    Surface_SetBaseAlbedo(surfaceData, cMatDiffColor, cAlphaCutoff, vColor, sDiffMap, vTexCoord, URHO3D_MATERIAL_DIFFUSE_HINT);
    Surface_SetBaseSpecular(surfaceData, cMatSpecColor, cMatEnvMapColor, sSpecMap, vTexCoord);
    Surface_SetAlbedoSpecular(surfaceData);

    // Apply noise to screen position used for background sampling
    half2 distortionDistance = surfaceData.normalInTangentSpace.xy * (cNoiseStrength * surfaceData.albedo.a);
    surfaceData.screenPos += distortionDistance;

    Surface_SetBackground(surfaceData, sEmissiveMap, sDepthBuffer);

    gl_FragColor = vec4(surfaceData.backgroundColor,1);
}
#endif