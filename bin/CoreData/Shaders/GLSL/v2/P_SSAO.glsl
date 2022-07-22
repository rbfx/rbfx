#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_Samplers.glsl"
#include "_GammaCorrection.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)

#ifdef URHO3D_PIXEL_SHADER

UNIFORM_BUFFER_BEGIN(6, Custom)
    // Inverted input texture size
    UNIFORM(vec2 cInputInvSize)
    // Blur step
    UNIFORM(vec2 cBlurStep)
    // Strength of the effect
    UNIFORM(float cSSAOStrength)
    // Exponent of the effect
    UNIFORM(float cSSAOExponent)
    // Radius
    UNIFORM(float cSSAORadius)
    // Blur depth threshold.
    UNIFORM(float cSSAODepthThreshold)
    // Blur normal threshold.
    UNIFORM(float cSSAONormalThreshold)
    // Projection matrix from texture-adjusted clip space to view space
    UNIFORM(mat4 cProj)
    // Inverted projection matrix from view space to texture-adjusted clip space
    UNIFORM(mat4 cInvProj)
    // Camera view matrix to translate normals from world space to view space
    UNIFORM(mat4 cCameraView)
UNIFORM_BUFFER_END(6, Custom)

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

vec3 RGBToNormal(vec3 rgb)
{
   return (rgb-vec3(0.5, 0.5, 0.5))*2.0;
}
vec3 NormalToRGB(vec3 xyz)
{
   return (xyz+vec3(1.0, 1.0, 1.0))*0.5;
}
vec2 RGBToNormal(vec2 rgb)
{
   return (rgb-vec2(0.5, 0.5))*2.0;
}
vec2 NormalToRGB(vec2 xyz)
{
   return (xyz+vec2(1.0, 1.0))*0.5;
}

half UVToConfidence(vec2 uv)
{
    vec2 weights = step(vec2(0.0,0.0), uv) * step(uv, vec2(1.0,1.0));
    return weights.x * weights.y;
}

// Sample depth texture at screen coordinates
vec3 SampleDepth(vec2 texCoord)
{
    vec2 coord = clamp(texCoord, 0.0, 1.0);
    vec4 res = vec4(coord, texture2D(sDepthBuffer, coord).r, 1.0) * cInvProj;
    return res.xyz/res.w;
}

// Reconstruct or sample normal
vec3 EvaluateNormal(vec3 center, vec2 texcoords) {
#ifdef DEFERRED
   // Sample normal render target texture when available
   vec3 normal = (vec4(RGBToNormal(texture2D(sNormalMap, texcoords).xyz), 0.0) * cCameraView).xyz;
#ifdef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
   // Fip Y to match quad texture coordinates
   normal.y = -normal.y;
#endif
   return normalize(normal);

#else
   // Reconstruct normal from depth samples
   const vec2 offsetY = vec2(0.0,cInputInvSize.y);
   const vec2 offsetX = vec2(cInputInvSize.x,0.0);

#define SSAO_QUALITY_MID

#ifdef SSAO_QUALITY_MID
   vec3 up = SampleDepth(texcoords-offsetY);
   vec3 down = SampleDepth(texcoords+offsetY);
   vec3 left = SampleDepth(texcoords-offsetX);
   vec3 right = SampleDepth(texcoords+offsetX);

   vec3 p1 = (abs(down.z-center.z)<abs(center.z-up.z))?(down - center):(center-up);
   vec3 p2 = (abs(right.z-center.z)<abs(center.z-left.z))?(right - center):(center-left);
#else
   vec3 down = SampleDepth(texcoords+offsetY);
   vec3 right = SampleDepth(texcoords+offsetX);

   vec3 p1 = down - center;
   vec3 p2 = right - center;
#endif
   vec3 normal = cross(p1, p2);

   return normalize(normal);
#endif
}

#ifdef BLUR

void SampleNormalAndOcclustion(vec2 uv, out vec2 n, out half confidence, out half occlusion)
{
   vec4 color = texture2D(sDiffMap, uv);
   occlusion = color.a;
   confidence = color.z * UVToConfidence(uv);
   n = RGBToNormal(color.xy);
}

float CompareDepthAndNormal(vec2 uv, vec3 pos, vec2 norm, vec2 sampleNorm)
{
   vec2 normDiff = abs(norm-sampleNorm);
   float normalMask = step(normDiff.x + normDiff.y, cSSAONormalThreshold);

   float depthDiff = abs(SampleDepth(uv).z - pos.z);
   float depthMask = step(depthDiff, cSSAODepthThreshold);

   return normalMask * depthMask;
}

#endif

void main()
{ 
#ifdef EVALUATE_OCCLUSION
  
  const float falloff = 0.01;
  vec4 clipSpaceFadeOut = vec4(0.0, 0.0, 0.9995, 1.0) * cInvProj;
  const float fadeDistance = clipSpaceFadeOut.z/clipSpaceFadeOut.w;
  
  const int samples = 16;
  vec3 sample_sphere[samples] = {
      vec3(-0.3991061, -0.2619659, 0.7481203), //Length: 0.887466
      vec3(0.5641699, -0.1403742, -0.5268592), //Length: 0.7845847
      vec3(-0.4665807, 0.3778321, -0.06707126), //Length: 0.6041135
      vec3(-0.1905782, -0.8022718, -0.2076609), //Length: 0.850343
      vec3(0.6526242, 0.3760332, -0.09132737), //Length: 0.7587227
      vec3(-0.08275586, 0.411534, 0.7324651), //Length: 0.8442239
      vec3(-0.8905346, -0.2907455, 0.01784503), //Length: 0.9369649
      vec3(-0.13476, -0.2325878, -0.5808671), //Length: 0.6400499
      vec3(0.08244705, -0.03393286, 0.06123221), //Length: 0.1081589
      vec3(0.3083563, 0.7715228, -0.4268006), //Length: 0.9340716
      vec3(-0.3208538, -0.4802179, 0.2167356), //Length: 0.6168718
      vec3(0.4159057, -0.6026651, -0.02300626), //Length: 0.7326064
      vec3(0.3815646, 0.6793259, 0.3596187), //Length: 0.858138
      vec3(0.2557214, -0.3414094, 0.8358757), //Length: 0.9384254
      vec3(-0.5374408, 0.1441735, 0.4326658), //Length: 0.7048605
      vec3(0.4646511, 0.1468607, 0.4646904) //Length: 0.6733543
      };
   vec3 position = SampleDepth(vTexCoord);
   vec3 normal = EvaluateNormal(position, vTexCoord);

   // Sample random vector from SSAO random texture
   vec3 randomVector = normalize(RGBToNormal(texture2D(sDiffMap,vTexCoord/cInputInvSize/4.0).xyz));

   // Sample points around position
   half weightSum = 0.00001;
   half occlusion = 0.0;
   for(int i=0; i < samples; i++)
   {
      // Evaluate randomized sample offset
      vec3 sampleOffset = cSSAORadius * reflect(sample_sphere[i], randomVector);
      // Evaluate offset projection on surface normal
      float offsetProj = dot(sampleOffset, normal);
      // Mirror the offset if necessary to build a hemisphere
      vec3 hemispherePosition = position + sampleOffset - (step(0, -offsetProj) * 2.0 * offsetProj * normal);// + normal * (1/128.0);
      // Translate view space hemisphere point to clip space
      vec4 clipSpaceRay = vec4(hemispherePosition, 1.0) * cProj;
      vec2 sampleUV = clipSpaceRay.xy/clipSpaceRay.w;
      // Sample depth texture
      vec3 sampleDepth = SampleDepth(sampleUV);
      // Evalue difference between predicted and actual sample depth
      float difference = hemispherePosition.z - sampleDepth.z;
      // Fade out AO when sampled points are too far away
      half weight = UVToConfidence(sampleUV) * smoothstep(cSSAORadius*3.0, cSSAORadius, difference);
      weightSum += weight;
      occlusion += step(falloff, difference) * weight;
   }
  
   // Normalize accumulated occlustion and fade it with distance
   float ao = 1.0 - cSSAOStrength * smoothstep(fadeDistance,0,position.z) * occlusion * (1.0 / weightSum);
   float finalAO = pow(clamp(ao, 0.0, 1.0), cSSAOExponent);
   gl_FragColor = vec4(NormalToRGB(normal.xy), weightSum / samples, finalAO);

#endif

#ifdef BLUR
#define WINDOW_WIDTH 4

vec3 position = SampleDepth(vTexCoord);
half confidence;
half occlusion;
vec2 normal;
SampleNormalAndOcclustion(vTexCoord, normal, confidence, occlusion);

occlusion *= confidence;
half weightSum = confidence;
half maxSum = 1.0;

float sampleWeights[WINDOW_WIDTH+1] = {
   1.0f,
   0.9071823f,
   0.683296f,
   0.442073f,
   0.2665483f
};

for (int i=-WINDOW_WIDTH; i<0;++i)
{
   vec2 uv = vTexCoord + cBlurStep*i;
   vec2 sampleNormal;
   half sampleConfidence;
   half ao;
   SampleNormalAndOcclustion(uv, sampleNormal, sampleConfidence, ao);
   half sampleFactor = sampleWeights[abs(i)];
   maxSum += sampleFactor;
   half weight = sampleConfidence * CompareDepthAndNormal(uv, position, normal, sampleNormal) * sampleFactor;
   occlusion += ao * weight;
   weightSum += weight;
}
for (int i=1; i<=WINDOW_WIDTH;++i)
{
   vec2 uv = vTexCoord + cBlurStep*i;
   vec2 sampleNormal;
   half sampleConfidence;
   half ao;
   SampleNormalAndOcclustion(uv, sampleNormal, sampleConfidence, ao);
   half sampleFactor = sampleWeights[abs(i)];
   maxSum += sampleFactor;
   half weight = sampleConfidence * CompareDepthAndNormal(uv, position, normal,  sampleNormal) * sampleFactor;
   occlusion += ao * weight;
   weightSum += weight;
}
gl_FragColor = vec4(NormalToRGB(normal), weightSum/maxSum, occlusion/weightSum);

#endif

#ifdef PREVIEW

vec4 ao = texture2D(sDiffMap, vTexCoord);
gl_FragColor = vec4(ao.a, ao.a, ao.a, 1.0);

#endif

#ifdef COMBINE

float ao = 1.0 - texture2D(sDiffMap, vTexCoord).a;
gl_FragColor = vec4(0.0, 0.0, 0.0, ao);

#endif
}
#endif

