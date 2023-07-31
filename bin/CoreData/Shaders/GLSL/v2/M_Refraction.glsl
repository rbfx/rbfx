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
    FillVertexOutputs(vertexTransform, cNormalScale, cUOffset, cVOffset, cLMOffset);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceAmbient(surfaceData, sEmissiveMap, vTexCoord2);
    FillSurfaceNormal(surfaceData, vNormal, sNormalMap, vTexCoord, vTangent, vBitangentXY);
    FillSurfaceBaseAlbedo(surfaceData, cMatDiffColor, vColor, sDiffMap, vTexCoord, URHO3D_MATERIAL_DIFFUSE_HINT);
    FillSurfaceBaseSpecular(surfaceData, sSpecMap, vTexCoord);
    FillSurfaceAlbedoSpecular(surfaceData);

    // Apply noise to screen position used for background sampling
    half2 distortionDistance = surfaceData.normalInTangentSpace.xy * (cNoiseStrength * surfaceData.albedo.a);
    surfaceData.screenPos += distortionDistance;

    FillSurfaceBackground(surfaceData, sEmissiveMap, sDepthBuffer);

    gl_FragColor = vec4(surfaceData.backgroundColor,1);
}
#endif