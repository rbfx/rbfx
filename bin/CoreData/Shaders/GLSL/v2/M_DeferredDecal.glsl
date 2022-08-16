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
    //vFarRay = GetFarRay(gl_Position);
    #ifdef URHO3D_ORTHOGRAPHIC_DEPTH
        vNearRay = GetDeferredNearRay(gl_Position);
    #endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER

#ifdef URHO3D_ORTHOGRAPHIC_DEPTH
    #define GetDeferredWorldPos(depth) mix(vNearRay, vFarRay, depth)/vScreenPos.w
#else
    #define GetDeferredWorldPos(depth) (vFarRay * depth)/vScreenPos.w
#endif

void main()
{
#ifdef URHO3D_HAS_READABLE_DEPTH
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceBackgroundDepth(surfaceData);
    vec4 worldPos = vec4(GetDeferredWorldPos(surfaceData.backgroundDepth)+cCameraPos, 1.0);
    vec4 modelSpace = worldPos * vToModelSpace;
    vTexCoord = (modelSpace.xy) + vec2(0.5, 0.5);
    vec3 crop = step(abs(modelSpace.xyz), vec3(0.5,0.5,0.5));
    if (crop.x*crop.y*crop.z < 0.5)
    {
        discard;
    }

    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceAlbedoSpecular(surfaceData);
    FillSurfaceEmission(surfaceData);

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData) * smoothstep(0.5, 0.45, abs(modelSpace.z));

    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
#else
    discard;
#endif
}
#endif
