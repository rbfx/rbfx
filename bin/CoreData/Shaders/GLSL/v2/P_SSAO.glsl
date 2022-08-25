#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_Samplers.glsl"
#include "_GammaCorrection.glsl"

/// Enables better normal reconstruction.
#define SSAO_QUALITY_MID
/// Number of samples for SSAO.
#define NUM_OCCLUSION_SAMPLES 16
/// Number of samples for SSAO blur.
#define NUM_BLUR_SAMPLES 4

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)

#ifdef URHO3D_PIXEL_SHADER

UNIFORM_BUFFER_BEGIN(6, Custom)
    /// Converts from view space to input texture space.
    UNIFORM(mat4 cViewToTexture)
    /// Converts from input texture space to view space.
    UNIFORM(mat4 cTextureToView)
    /// Converts world space to view space.
    UNIFORM(mat4 cWorldToView)

    /// Inverted size of the input depth and normal textures.
    UNIFORM_HIGHP(vec2 cInputInvSize)

    /// Radius function: radius = clamp(z * cRadiusInfo.x + cRadiusInfo.y, cRadiusInfo.z, cRadiusInfo.w).
    UNIFORM(half4 cRadiusInfo)
    /// Fade out distances.
    UNIFORM(half2 cFadeDistance)
    /// Strength of the effect
    UNIFORM(half cStrength)
    /// Exponent of the effect
    UNIFORM(half cExponent)

    /// Blur step, aka inverted size of intermediate textures with either x or y set to 0.
    UNIFORM_HIGHP(vec2 cBlurStep)
    /// Blur depth threshold.
    UNIFORM(half cBlurZThreshold)
    /// Blur normal threshold.
    UNIFORM(half cBlurNormalInvThreshold)
UNIFORM_BUFFER_END(6, Custom)

/// Sample view-space position from depth texture.
vec3 SamplePosition(const vec2 texCoord)
{
    float depth = texture2D(sDepthBuffer, texCoord).r;
    vec4 position = vec4(texCoord, depth, 1.0) * cTextureToView;
    return position.xyz / position.w;
}

/// Sample view-space normal from normal texture.
#ifdef DEFERRED
half3 SampleNormal(const vec2 texCoord)
{
    half3 worldNormal = DecodeNormal(texture2D(sNormalMap, texCoord));
    half3 normal = (vec4(worldNormal, 0.0) * cWorldToView).xyz;
    return normal;
}
#endif

/// Sample world-space normal from normal texture.
half3 SampleWorldNormal(const vec2 texCoord)
{
#ifdef DEFERRED
    return DecodeNormal(texture2D(sNormalMap, texCoord));
#else
    return vec3(0.0, 0.0, 0.0);
#endif
}

/// Reconstruct view-space normal from depth buffer.
half3 ReconstructNormal(const vec3 centerPos, const vec2 texCoord)
{
    vec2 offsetY = vec2(0.0, cInputInvSize.y);
    vec2 offsetX = vec2(cInputInvSize.x, 0.0);

#ifdef SSAO_QUALITY_MID
    vec3 upPos = SamplePosition(Saturate(texCoord - offsetY));
    vec3 downPos = SamplePosition(Saturate(texCoord + offsetY));
    vec3 leftPos = SamplePosition(Saturate(texCoord - offsetX));
    vec3 rightPos = SamplePosition(Saturate(texCoord + offsetX));

    half3 p1 = (abs(downPos.z - centerPos.z) < abs(centerPos.z - upPos.z)) ? (downPos - centerPos) : (centerPos - upPos);
    half3 p2 = (abs(rightPos.z - centerPos.z) < abs(centerPos.z - leftPos.z)) ? (rightPos - centerPos) : (centerPos - leftPos);
#else
    vec3 downPos = SamplePosition(Saturate(texCoord + offsetY));
    vec3 rightPos = SamplePosition(Saturate(texCoord + offsetX));

    half3 p1 = downPos - centerPos;
    half3 p2 = rightPos - centerPos;
#endif

    half3 normal = cross(p1, p2);
    return normalize(normal);
}

/// Get view-space normal via preferred method.
vec3 SampleOrReconstructNormal(const vec3 centerPos, const vec2 texCoord)
{
#ifdef DEFERRED
    return SampleNormal(texCoord);
#else
    return ReconstructNormal(centerPos, texCoord);
#endif
}

/// Project vector to hemisphere.
half3 ToHemisphere(const half3 direction, const half3 normal)
{
    half proj = dot(direction, normal);
    half factor = proj > 0.0 ? 1.0 : -1.0;
    const half bias = 0.03; // TODO: Configure
    return direction * factor + normal * bias;
}

/// Calculate occlusion for single sample around given position.
half2 CalculateOcclusion(half3 offsetDirection, half radius, vec3 position, half3 normal)
{
    // Find sample position by projecting random offset to surface hemisphere.
    half3 offset = ToHemisphere(radius * offsetDirection, normal);
    vec3 expectedPosition = position + offset;

    // Find sample tex coord
    vec4 sampleTexCoordRaw = vec4(expectedPosition, 1.0) * cViewToTexture;
    vec2 sampleTexCoord = sampleTexCoordRaw.xy / sampleTexCoordRaw.w;

    // Calculate difference between expected and actual position.
    vec3 realPosition = SamplePosition(sampleTexCoord);
    half difference = expectedPosition.z - realPosition.z;

    // Fade out AO when sampled points are too far away
    half weight = 1.0 - smoothstep(radius, radius * 3.0, difference);

    half occlusion = step(0.01, difference) * weight;
    return vec2(occlusion, weight);
}

/// Calculate importance weight for a given sample during SSAO blur.
half GetSampleWeight(float baseZ, float sampleZ, half3 baseNormal, half3 sampleNormal)
{
#ifdef DEFERRED
    half normalWeight = 1.0 - step(dot(baseNormal, sampleNormal), cBlurNormalInvThreshold);
#else
    half normalWeight = 1.0;
#endif

    half depthWeight = step(abs(sampleZ - baseZ), cBlurZThreshold);
    return normalWeight * depthWeight;
}

/// Calculate blurred SSAO color from neighbour sample.
void CalculateBlur(inout half4 finalColor, inout half finalWeight,
    const vec2 texCoord, const vec3 basePosition, const half3 baseNormal, const half weightFactor)
{
    half4 color = texture2D(sDiffMap, texCoord);

    vec3 position = SamplePosition(texCoord);
    half3 normal = SampleWorldNormal(texCoord);
    half weight = weightFactor * GetSampleWeight(basePosition.z, position.z, baseNormal, normal);

    finalColor += color * weight;
    finalWeight += weight;
}

#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vTexCoord = GetQuadTexCoord(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef EVALUATE_OCCLUSION
    const half3 sampleOffsets[NUM_OCCLUSION_SAMPLES] = half3[NUM_OCCLUSION_SAMPLES] (
        vec3(-0.3991061,  -0.2619659,   0.7481203),  // Length: 0.887466
        vec3( 0.5641699,  -0.1403742,  -0.5268592),  // Length: 0.7845847
        vec3(-0.4665807,   0.3778321,  -0.06707126), // Length: 0.6041135
        vec3(-0.1905782,  -0.8022718,  -0.2076609),  // Length: 0.850343
        vec3( 0.6526242,   0.3760332,  -0.09132737), // Length: 0.7587227
        vec3(-0.08275586,  0.411534,    0.7324651),  // Length: 0.8442239
        vec3(-0.8905346,  -0.2907455,   0.01784503), // Length: 0.9369649
        vec3(-0.13476,    -0.2325878,  -0.5808671),  // Length: 0.6400499
        vec3( 0.08244705, -0.03393286,  0.06123221), // Length: 0.1081589
        vec3( 0.3083563,   0.7715228,  -0.4268006),  // Length: 0.9340716
        vec3(-0.3208538,  -0.4802179,   0.2167356),  // Length: 0.6168718
        vec3( 0.4159057,  -0.6026651,  -0.02300626), // Length: 0.7326064
        vec3( 0.3815646,   0.6793259,   0.3596187),  // Length: 0.858138
        vec3( 0.2557214,  -0.3414094,   0.8358757),  // Length: 0.9384254
        vec3(-0.5374408,   0.1441735,   0.4326658),  // Length: 0.7048605
        vec3( 0.4646511,   0.1468607,   0.4646904)   // Length: 0.6733543
    );

    // Sample textures at the position
    vec3 position = SamplePosition(vTexCoord);
    half3 normal = SampleOrReconstructNormal(position, vTexCoord);
    half3 noise = DecodeNormal(texture2D(sDiffMap, vTexCoord / cInputInvSize / 4.0));

    // Sample points around position
    half weightSum = 0.00001;
    half occlusionSum = 0.0;
    half radius = clamp(cRadiusInfo.x * position.z + cRadiusInfo.y, cRadiusInfo.z, cRadiusInfo.w);
    for (int i = 0; i < NUM_OCCLUSION_SAMPLES; i++)
    {
        half3 offsetDirection = reflect(sampleOffsets[i], noise);
        half2 sampleOcclusion = CalculateOcclusion(offsetDirection, radius, position, normal);
        occlusionSum += sampleOcclusion.x;
        weightSum += sampleOcclusion.y;
    }

    // Normalize accumulated occlustion and fade it with distance
    half distanceFactor = 1.0 - smoothstep(cFadeDistance.x, cFadeDistance.y, position.z);
    half occlusionFactor = 1.0 - pow(1.0 - occlusionSum / weightSum, cExponent);
    half finalOcclusion = 1.0 - cStrength * distanceFactor * occlusionFactor;
    gl_FragColor = vec4(0.0, 0.0, 0.0, finalOcclusion);
#endif

#ifdef BLUR
    const half sampleWeights[NUM_BLUR_SAMPLES + 1] = half[NUM_BLUR_SAMPLES + 1] (
        1.0,
        0.9071823,
        0.683296,
        0.442073,
        0.2665483
    );

    vec2 baseTexCoord = Saturate(vTexCoord);
    vec3 basePosition = SamplePosition(baseTexCoord);
    half3 baseNormal = SampleWorldNormal(baseTexCoord);

    half4 occlusion = sampleWeights[0] * texture2D(sDiffMap, baseTexCoord);
    half weightSum = sampleWeights[0];

    for (int i = 1; i <= NUM_BLUR_SAMPLES; ++i)
    {
        CalculateBlur(occlusion, weightSum, Saturate(vTexCoord + cBlurStep * i), basePosition, baseNormal, sampleWeights[i]);
        CalculateBlur(occlusion, weightSum, Saturate(vTexCoord - cBlurStep * i), basePosition, baseNormal, sampleWeights[i]);
    }

    gl_FragColor = occlusion / weightSum;
#endif

#ifdef PREVIEW
    half4 ao = texture2D(sDiffMap, vTexCoord);
    gl_FragColor = vec4(ao.a, ao.a, ao.a, 1.0);
#endif

#ifdef COMBINE
    half4 ao = texture2D(sDiffMap, vTexCoord);
    gl_FragColor = vec4(ao.xyz, 1.0 - ao.a);
#endif
}
#endif
