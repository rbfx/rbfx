#define URHO3D_PIXEL_NEED_EYE_VECTOR
#define URHO3D_SURFACE_NEED_BACKGROUND_COLOR
#ifdef URHO3D_HAS_READABLE_DEPTH
    #define URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
#endif
#define URHO3D_SURFACE_NEED_NORMAL
#define URHO3D_SURFACE_NEED_NORMAL_IN_TANGENT_SPACE
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS

#ifndef NORMALMAP
    #define NORMALMAP
#endif

#ifndef ENVCUBEMAP
    #define ENVCUBEMAP
#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half2 cNoiseSpeed)
    UNIFORM(half2 cSaturationLightnessFactor)
    UNIFORM(half cNoiseStrength)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
    vTexCoord += cElapsedTime * cNoiseSpeed;
}
#endif

#ifdef URHO3D_PIXEL_SHADER
//Color space conversion functions from https://github.com/tobspr/GLSL-Color-Spaces/blob/master/ColorSpaces.inc.glsl (MIT license)

// Converts a value from linear RGB to HCV (Hue, Chroma, Value)
vec3 LinearToHCV(vec3 rgb)
{
    // Based on work by Sam Hocevar and Emil Persson
    vec4 P = (rgb.g < rgb.b) ? vec4(rgb.bg, -1.0, 2.0/3.0) : vec4(rgb.gb, 0.0, -1.0/3.0);
    vec4 Q = (rgb.r < P.x) ? vec4(P.xyw, rgb.r) : vec4(rgb.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6.0 * C + 1e-10) + Q.z);
    return vec3(H, C, Q.x);
}

// Converts from linear rgb to HSL
vec3 LinearToHSL(vec3 rgb)
{
    vec3 HCV = LinearToHCV(rgb);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1.0 - abs(L * 2.0 - 1.0) + 1e-10);
    return vec3(HCV.x, S, L);
}

// Converts from pure Hue to linear RGB
vec3 HueToLinear(float hue)
{
    float R = abs(hue * 6.0 - 3.0) - 1.0;
    float G = 2.0 - abs(hue * 6.0 - 2.0);
    float B = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(R,G,B), 0.0, 1.0);
}

// Converts from HSL to linear RGB
vec3 HSLToLinear(vec3 hsl)
{
    vec3 rgb = HueToLinear(hsl.x);
    float C = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y;
    return (rgb - 0.5) * C + hsl.z;
}

void main()
{
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);

    // Apply noise to screen position used for background sampling
    surfaceData.screenPos += surfaceData.normalInTangentSpace.xy * cNoiseStrength;

#ifndef URHO3D_ADDITIVE_BLENDING
    FillSurfaceBackground(surfaceData);
    vec3 hsl = clamp(LinearToHSL(surfaceData.backgroundColor) * vec3(1.0, cSaturationLightnessFactor), 0.0, 1.0);
    surfaceData.backgroundColor = HSLToLinear(hsl);
#endif

    // Water doesn't accept diffuse lighting, set albedo to zero
    surfaceData.albedo = vec4(0.0);
    surfaceData.specular = GammaToLightSpace(cMatSpecColor.rgb) * (1.0 - surfaceData.oneMinusReflectivity);
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    surfaceData.emission = GammaToLightSpace(cMatEmissiveColor);
#endif

#ifdef URHO3D_AMBIENT_PASS
    half NoV = clamp(dot(surfaceData.normal, surfaceData.eyeVec), 0.0, 1.0);
    #ifdef URHO3D_PHYSICAL_MATERIAL
        half4 reflectedColor = Indirect_PBRWater(surfaceData.reflectionColor[0].rgb, surfaceData.specular, NoV);
    #else
        half4 reflectedColor = Indirect_SimpleWater(surfaceData.reflectionColor[0].rgb, cMatEnvMapColor, NoV);
    #endif
#else
    half4 reflectedColor = vec4(0.0);
#endif

#ifdef URHO3D_LIGHT_PASS
    reflectedColor.rgb += CalculateDirectLighting(surfaceData);
#endif

    // Water uses background color and so doesn't need alpha blending
    const fixed finalAlpha = 1.0;

#ifdef URHO3D_ADDITIVE_BLENDING
    gl_FragColor = GetFragmentColorAlpha(reflectedColor.rgb, finalAlpha, surfaceData.fogFactor);
#else
    half3 waterDepthColor = GetFragmentColor(GammaToLightSpace(cMatDiffColor.rgb), finalAlpha, surfaceData.fogFactor);

    #ifdef URHO3D_HAS_READABLE_DEPTH
        half depthDelta = surfaceData.backgroundDepth - vWorldDepth - cFadeOffsetScale.x;
        half opacity = clamp(depthDelta * cFadeOffsetScale.y, 0.0, 1.0);
        surfaceData.backgroundColor = mix(surfaceData.backgroundColor, waterDepthColor, opacity);
    #else
        surfaceData.backgroundColor = mix(surfaceData.backgroundColor, waterDepthColor, cMatDiffColor.a);
    #endif

    half3 waterColor = GetFragmentColorAlpha(reflectedColor.rgb, 1.0, surfaceData.fogFactor).rgb;
    gl_FragColor = vec4(mix(surfaceData.backgroundColor, waterColor, reflectedColor.a), 1.0);
#endif

}
#endif
