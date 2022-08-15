#ifdef URHO3D_HAS_READABLE_DEPTH
    #define URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
#endif
#define URHO3D_PIXEL_CALCULATES_TEXCOORD
#define URHO3D_PIXEL_NEED_SCREEN_POSITION

//#ifdef URHO3D_PIXEL_SHADER
vec2 vTexCoord;
//#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_Material.glsl"
#include "_VertexScreenPos.glsl"
#include "_DeferredLighting.glsl"

VERTEX_OUTPUT(mat4 vToModelSpace)
VERTEX_OUTPUT_HIGHP(vec3 vFarRay)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
    vToModelSpace = inverse(cModel);
    vScreenPos = GetDeferredScreenPos(gl_Position);
    vFarRay = GetDeferredFarRay(gl_Position);
    #ifdef URHO3D_ORTHOGRAPHIC_DEPTH
        vNearRay = GetDeferredNearRay(gl_Position);
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER

#ifdef URHO3D_ORTHOGRAPHIC_DEPTH
    #define GetDeferredWorldPos(depth) mix(vNearRay, vFarRay, depth)
#else
    #define GetDeferredWorldPos(depth) (vFarRay * depth)
#endif

void main()
{
#ifdef URHO3D_HAS_READABLE_DEPTH
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceBackgroundDepth(surfaceData);
    vec4 worldPos = vec4(GetDeferredWorldPos(surfaceData.backgroundDepth) / vScreenPos.w, 1.0);
    vec4 modelSpace = vToModelSpace * worldPos;
    vTexCoord = modelSpace.xy / modelSpace.w;

    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceAlbedoSpecular(surfaceData);
    FillSurfaceEmission(surfaceData);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData);

    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
    gl_FragColor = vec4(modelSpace.xyz,1);
#else
    discard;
#endif
}
#endif
