#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half3 cStepOffsetWidthBrightness)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
}
#endif

#ifdef URHO3D_PIXEL_SHADER

// Based on https://github.com/glslify/glsl-aastep
// Under MIT License
float AntiAliasedStep(float threshold, float value)
{
    float afwidth = length(vec2(dFdx(value), dFdy(value))) * 0.7071068;
    return smoothstep(threshold - afwidth, threshold + afwidth, value);
}

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
    FillSurfaceEmission(surfaceData);

#ifdef URHO3D_AMBIENT_PASS
    half3 surfaceColor = CalculateAmbientLighting(surfaceData);
#else
    half3 surfaceColor = vec3(0.0);
#endif

#ifdef URHO3D_GBUFFER_PASS
    #ifdef URHO3D_PHYSICAL_MATERIAL
        half roughness = surfaceData.roughness;
    #else
        half roughness = 1.0 - cMatSpecColor.a / 255.0;
    #endif

    gl_FragData[1] = vec4(surfaceData.fogFactor * surfaceData.albedo.rgb, 0.0);
    gl_FragData[2] = vec4(surfaceData.fogFactor * surfaceData.specular, roughness);
    gl_FragData[3] = vec4(surfaceData.normal * 0.5 + 0.5, 0.0);
#elif defined(URHO3D_LIGHT_PASS)
    DirectLightData lightData = GetForwardDirectLightData();
    float NoL = dot(surfaceData.normal, lightData.lightVec.xyz);
    float shadowStep = AntiAliasedStep(cStepOffsetWidthBrightness.x-cStepOffsetWidthBrightness.y*0.5, NoL) * cStepOffsetWidthBrightness.z;
    float lightStep = AntiAliasedStep(cStepOffsetWidthBrightness.x+cStepOffsetWidthBrightness.y*0.5, NoL);
    float steppedNoL = mix(shadowStep, 1.0, lightStep);
 #if (URHO3D_SPECULAR > 0)
    half3 halfVec = normalize(surfaceData.eyeVec + lightData.lightVec.xyz);
    float brdf = BRDF_Direct_BlinnPhongSpecular(surfaceData.normal, halfVec, cMatSpecColor.a);
    half3 specular = max(NoL, 0.0) * surfaceData.specular * (brdf * cLightColor.a);
#else
    half3 specular = half3(0.0, 0.0, 0.0);
#endif
    surfaceColor += lightData.lightColor * (steppedNoL * surfaceData.albedo.rgb + specular) * GetDirectLightAttenuation(lightData);

#endif

    half surfaceAlpha = GetSurfaceAlpha(surfaceData);
    gl_FragColor = GetFragmentColorAlpha(surfaceColor, surfaceAlpha, surfaceData.fogFactor);
#endif

}
#endif
