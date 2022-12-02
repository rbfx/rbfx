#ifdef URHO3D_HAS_READABLE_DEPTH
    #define URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
#endif
#define URHO3D_PIXEL_CALCULATES_TEXCOORD
#define URHO3D_PIXEL_NEED_SCREEN_POSITION
#define URHO3D_SURFACE_VOLUMETRIC
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

vec2 vTexCoord;

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half4 cDecalMask)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"
#include "_VertexScreenPos.glsl"
#include "_DeferredLighting.glsl"

VERTEX_OUTPUT(mat4 vToModelSpace)
VERTEX_OUTPUT_HIGHP(vec3 vFarRay)
#ifdef URHO3D_ORTHOGRAPHIC_DEPTH
    VERTEX_OUTPUT_HIGHP(vec3 vNearRay)
#endif

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
    #define GetDeferredWorldPos(depth) mix(vNearRay, vFarRay, depth) / vScreenPos.w
#else
    #define GetDeferredWorldPos(depth) (vFarRay * depth) / vScreenPos.w
#endif

void main()
{
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceBackgroundDepth(surfaceData);

    vec4 worldPos = vec4(GetDeferredWorldPos(surfaceData.backgroundDepth) + cCameraPos, 1.0);
    vec4 modelSpace = worldPos * vToModelSpace;
    vTexCoord = modelSpace.xy + vec2(0.5, 0.5);
    vTexCoord.y = 1.0 - vTexCoord.y;
    vec3 crop = step(abs(modelSpace.xyz), vec3(0.5, 0.5, 0.5));
    if (crop.x*crop.y*crop.z < 0.5)
    {
        discard;
    }

    // Get normal
    #ifdef NORMALMAP
        vec3 localNormal = DecodeNormal(texture2D(sNormalMap, vTexCoord.xy)) * vec3(1.0, 1.0, -1.0);
        vec4 normalInModelSpace = vToModelSpace * vec4(localNormal, 0.0);

        surfaceData.normal = normalize(normalInModelSpace.xyz);
    #else
        surfaceData.normal = normalize((vec4(0.0, 0.0, -1.0, 0.0) * vToModelSpace).xyz);
    #endif

    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceAlbedoSpecular(surfaceData);
    FillSurfaceEmission(surfaceData);

    // TODO: Make configurable
    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData) * smoothstep(0.5, 0.45, abs(modelSpace.z));
    if (surfaceAlpha < 1.0/255.0)
    {
        discard;
    }

    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
    gl_FragData[0].a *= cDecalMask.x;
    gl_FragData[1].a = surfaceAlpha * cDecalMask.y;
    gl_FragData[2].a = surfaceAlpha * cDecalMask.z;
    gl_FragData[3].a = surfaceAlpha * cDecalMask.w;
}
#endif
