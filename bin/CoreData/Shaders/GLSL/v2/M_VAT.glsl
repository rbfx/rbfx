#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_DISABLE_NORMAL_SAMPLING
#define URHO3D_DISABLE_SPECULAR_SAMPLING
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(float cTextureHeight)
    UNIFORM(float cNumFrames)
    UNIFORM(float cRowsPerFrame)
    UNIFORM(float cNormalizedTime)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

#ifdef URHO3D_VERTEX_SHADER

SAMPLER(3, sampler2D sEmissiveMap)
SAMPLER(1, sampler2D sNormalMap)
#ifdef URHO3D_DEPTH_ONLY_PASS
    VERTEX_INPUT(vec2 iTexCoord1)
#endif

vec3 SampleVec3(sampler2D sampler, vec2 uv)
{
#ifdef HIGHPRECISION
    uv = uv * vec2(0.5, 1.0);
    //#ifdef texture2DLod
    //    vec3 high = texture2DLod(sampler, uv, 0).xyz;
    //    vec3 low = texture2DLod(sampler, uv + vec2(0.5, 0), 0).xyz;
    //    return high * (65280.0/65535.0) + low * (255.0/65535.0);
    //#else
        vec3 high = texture2D(sampler, uv).xyz;
        vec3 low = texture2D(sampler, uv + vec2(0.5, 0)).xyz;
        vec3 result = high * (65280.0/65535.0) + low * (255.0/65535.0);
    //#endif
#else
    vec3 result = texture2D(sampler, uv).xyz;
#endif
    return result - 0.5;
}

VertexTransform GetCustomVertexTransform()
{
    mat4 modelMatrix = GetModelMatrix();

    VertexTransform result;
    float time = clamp(cNormalizedTime, 0.0f, 1.0f);
    float offset = round(time*(cNumFrames-1.0))*cRowsPerFrame/cTextureHeight;
    vec2 uv = iTexCoord1 + vec2(0, offset);
    vec3 pos = SampleVec3(sEmissiveMap, uv).xyz;
    result.position = vec4(pos, 1) * modelMatrix;

    #ifdef URHO3D_VERTEX_NEED_NORMAL
        mediump mat3 normalMatrix = GetNormalMatrix(modelMatrix);
        vec3 norm = normalize(SampleVec3(sNormalMap, uv).xyz);
        result.normal = normalize(norm * normalMatrix);

        ApplyShadowNormalOffset(result.position, result.normal);

        #ifdef URHO3D_VERTEX_NEED_TANGENT
            result.tangent = normalize(iTangent.xyz * normalMatrix);
            result.bitangent = cross(result.tangent, result.normal) * iTangent.w;
        #endif
    #endif

    return result;
}

void main()
{
    VertexTransform vertexTransform = GetCustomVertexTransform();
    //VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef URHO3D_DEPTH_ONLY_PASS
    DefaultPixelShader();
#else
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceBackground(surfaceData);
    FillSurfaceAlbedoSpecular(surfaceData);
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    surfaceData.emission = vec3(0.0);
#endif

    half3 surfaceColor = GetSurfaceColor(surfaceData);
    half surfaceAlpha = GetSurfaceAlpha(surfaceData);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
#endif
}
#endif
