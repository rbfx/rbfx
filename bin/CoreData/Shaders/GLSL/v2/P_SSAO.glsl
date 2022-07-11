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
    UNIFORM(vec2 cInputInvSize)
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
float saturate(float x)
{
    return clamp(x,0.0,1.0);
}
vec2 saturate(vec2 x)
{
    return clamp(x,0.0,1.0);
}
float SampleDepth(vec2 texCoord)
{
    return ReconstructDepth(texture2D(sDepthBuffer, clamp(texCoord,0.0,1.0)).r) * cFarClip;
}

vec3 normal_from_depth(float depth, vec2 texcoords) {
  
   const vec2 offset1 = vec2(0.0,cInputInvSize.y);
   const vec2 offset2 = vec2(cInputInvSize.x,0.0);
  
   float depth1 = SampleDepth(texcoords+offset1);
   float depth2 = SampleDepth(texcoords+offset2);
  
   vec3 p1 = vec3(offset1, depth1 - depth);
   vec3 p2 = vec3(offset2, depth2 - depth);
  
   vec3 normal = cross(p1, p2);
   normal.z = -normal.z;
  
  return normalize(normal);
}

float rand(vec2 co)
{
 	return fract(sin(dot(co ,vec2(12.9898,78.233))) * 43758.5453);
}

const int windowSize = 5;
const int kernelSize = (windowSize * 2 + 1) * (windowSize * 2 + 1);

float values[kernelSize];

vec2 findMean(int i0, int i1, int j0, int j1, vec2 meanAndMinVariance) {
  float meanTemp = 0.0;
  float variance = 0.0;
  int count    = 0;
  float color;
  int i,j;
  for (i = i0; i <= i1; ++i) {
    for (j = j0; j <= j1; ++j) {
      color = texture2D(sDiffMap, vTexCoord + vec2(i,j)*(2.0*cInputInvSize)).r;
      meanTemp += color;
      values[count] = color;
      count += 1;
    }
  }

  meanTemp /= count;
  for (i = 0; i < count; ++i) {
    variance += pow(values[i] - meanTemp, 2);
  }

  variance /= count;

  if (variance < meanAndMinVariance.y) {
    return vec2(meanTemp,variance);
  }
  return meanAndMinVariance;
}

void main()
{ 
#ifdef EVALUATE_OCCLUSTION
  
  const float total_strength = 1.0;
  const float base = 0.2;
  
  const float falloff = 0.001;
  const float radius = 0.3;
  
  const int samples = 16;
  vec3 sample_sphere[samples] = {
      vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430),
      vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019),
      vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843),
      vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344),
       vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158),
       vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287),
       vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411),
       vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271)
  };
  
  float depth = SampleDepth(vTexCoord);
 
   vec3 position = vec3(vTexCoord, depth);
   vec3 normal = normal_from_depth(depth, vTexCoord);
  
   float radius_depth = min(radius/depth, radius);
   float occlusion = 0.0;
   vec3 random = normalize(vec3(rand(vTexCoord), rand(vTexCoord.yx), 0));

   for(int i=0; i < samples; i++) {
  
    vec3 ray = radius_depth * reflect(sample_sphere[i], random);// reflect(sample_sphere[i], random);
    vec3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = SampleDepth(saturate(hemi_ray.xy));
    float difference = depth - occ_depth;
    
    occlusion += step(falloff, difference) * smoothstep(radius*8.0, radius, difference);
   }
  
   float ao = 1.0 - occlusion * (1.0 / samples);
   float res = saturate(ao + base);
   res = res*res;
   gl_FragColor = vec4(res, res, res, 1.0);
#endif

#ifdef BLUR

vec2 meanAndMinVariance = vec2(0.0, 3.402823466e+38);

int size = 2;//int(parameters.x);
//if (size <= 0) { return; }

// Lower Left

meanAndMinVariance = findMean(-size, 0, -size, 0, meanAndMinVariance);

// Upper Right

meanAndMinVariance = findMean(0, size, 0, size, meanAndMinVariance);

// Upper Left

meanAndMinVariance = findMean(-size, 0, 0, size, meanAndMinVariance);

// Lower Right

meanAndMinVariance = findMean(0, size, -size, 0, meanAndMinVariance);

gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0 - meanAndMinVariance.x);

#endif
}
#endif

