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
    // Strength of the effect
    UNIFORM(float cSSAOStrength)
    // Radius
    UNIFORM(float cSSAORadius)
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
   vec3 normal = (vec4(texture2D(sNormalMap, texcoords).xyz * 2.0 - vec3(1.0,1.0,1.0), 0.0) * cCameraView).xyz;
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

vec4 samplesToFilter[14];

float SampleAO(int dx, int dy)
{
   return texture2D(sDiffMap, vTexCoord + vec2(dx,dy)*cInputInvSize).r;
}

// Collect AO samples to blur in a 7x7 window around the central pixel.
void CollectSamples()
{
   samplesToFilter[0] = vec4(SampleAO(-3, -3), SampleAO(-2, -3), SampleAO(-1, -3), SampleAO(0, -3));
   samplesToFilter[1] = vec4(SampleAO(-3, -2), SampleAO(-2, -2), SampleAO(-1, -2), SampleAO(0, -2));
   samplesToFilter[2] = vec4(SampleAO(-3, -1), SampleAO(-2, -1), SampleAO(-1, -1), SampleAO(0, -1));
   samplesToFilter[3] = vec4(SampleAO(-3, 0), SampleAO(-2, 0), SampleAO(-1, 0), SampleAO(0, 0));
   samplesToFilter[4] = vec4(SampleAO(-3, 1), SampleAO(-2, 1), SampleAO(-1, 1), SampleAO(0, 1));
   samplesToFilter[5] = vec4(SampleAO(-3, 2), SampleAO(-2, 2), SampleAO(-1, 2), SampleAO(0, 2));
   samplesToFilter[6] = vec4(SampleAO(-3, 3), SampleAO(-2, 3), SampleAO(-1, 3), SampleAO(0, 3));
   samplesToFilter[7] = vec4(samplesToFilter[0].a, SampleAO(0, -3), SampleAO(1, -3), SampleAO(2, -3));
   samplesToFilter[8] = vec4(samplesToFilter[1].a, SampleAO(0, -2), SampleAO(1, -2), SampleAO(2, -2));
   samplesToFilter[9] = vec4(samplesToFilter[2].a, SampleAO(0, -1), SampleAO(1, -1), SampleAO(2, -1));
   samplesToFilter[10] = vec4(samplesToFilter[3].a, SampleAO(0, 0), SampleAO(1, 0), SampleAO(2, 0));
   samplesToFilter[11] = vec4(samplesToFilter[4].a, SampleAO(0, 1), SampleAO(1, 1), SampleAO(2, 1));
   samplesToFilter[12] = vec4(samplesToFilter[5].a, SampleAO(0, 2), SampleAO(1, 2), SampleAO(2, 2));
   samplesToFilter[13] = vec4(samplesToFilter[6].a, SampleAO(0, 3), SampleAO(1, 3), SampleAO(2, 3));
}

vec2 FilterWindow(vec4 s0, vec4 s1, vec4 s2, vec4 s3, vec2 meanAndMinVariance) {
  const vec4 one4 = vec4(1.0, 1.0, 1.0, 1.0);
  float meanTemp = (dot(s0,one4)+dot(s1,one4)+dot(s2,one4)+dot(s3,one4))/16.0;
  // Calculate average value
  const vec4 mean4 = vec4(meanTemp, meanTemp, meanTemp, meanTemp);
  // Calculate standard deviation
  const vec4 diff0 = s0-mean4;
  const vec4 diff1 = s1-mean4;
  const vec4 diff2 = s2-mean4;
  const vec4 diff3 = s3-mean4;
  float variance = (dot(diff0, diff0) + dot(diff1, diff1) + dot(diff1, diff1) + dot(diff1, diff1)) / 16.0;

  // Return best match
  return (variance < meanAndMinVariance.y) ? vec2(meanTemp,variance) : meanAndMinVariance;
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
   vec3 randomVector = normalize((texture2D(sDiffMap,vTexCoord/cInputInvSize/4.0).xyz - vec3(0.5f,0.5f,0.5f)) * 2.0f);

   // Sample points around position
   float weightSum = 0.00001;
   float occlusion = 0.0;
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
      // Sample depth texture
      vec3 sampleDepth = SampleDepth(clipSpaceRay.xy/clipSpaceRay.w);
      // Evalue difference between predicted and actual sample depth
      float difference = hemispherePosition.z - sampleDepth.z;
      // Fade out AO when sampled points are too far away
      float weight = smoothstep(cSSAORadius*8.0, cSSAORadius, difference);
      weightSum += weight;
      occlusion += step(falloff, difference) * weight;
   }
  
   // Normalize accumulated occlustion and fade it with distance
   float ao = 1.0 - cSSAOStrength * smoothstep(fadeDistance,0,position.z) * occlusion * (1.0 / weightSum);
   float res = clamp(ao, 0.0, 1.0);
   gl_FragColor = vec4(res, res, res, 1.0);

#endif

#ifdef BLUR

// Set start values
vec2 meanAndMinVariance = vec2(0.0, 3.402823466e+38);

CollectSamples();

meanAndMinVariance = FilterWindow(samplesToFilter[0], samplesToFilter[1], samplesToFilter[2], samplesToFilter[3], meanAndMinVariance);
meanAndMinVariance = FilterWindow(samplesToFilter[3], samplesToFilter[4], samplesToFilter[5], samplesToFilter[6], meanAndMinVariance);
meanAndMinVariance = FilterWindow(samplesToFilter[7], samplesToFilter[8], samplesToFilter[9], samplesToFilter[10], meanAndMinVariance);
meanAndMinVariance = FilterWindow(samplesToFilter[10], samplesToFilter[11], samplesToFilter[12], samplesToFilter[13], meanAndMinVariance);

// Set AO value to alpha for alpha blending.
float ao = 1.0 - meanAndMinVariance.x;
gl_FragColor = vec4(0.0, 0.0, 0.0, ao);

#endif

#ifdef PREVIEW

gl_FragColor = texture2D(sDiffMap, vTexCoord);

#endif
}
#endif

