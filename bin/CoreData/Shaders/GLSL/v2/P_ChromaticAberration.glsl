#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_Samplers.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
float remap( float t, float a, float b ) {
    return clamp( (t - a) / (b - a), 0.0, 1.0 );
}
vec2 remap( vec2 t, vec2 a, vec2 b ) {
    return clamp( (t - a) / (b - a), 0.0, 1.0 );
}


//note: input [0;1]
vec3 spectrum_offset_rgb( float t )
{
    float t0 = 3.0 * t - 1.5;
    return clamp( vec3( -t0, 1.0-abs(t0), t0), 0.0, 1.0);
}

const float gamma = 2.2;
vec3 lin2srgb( vec3 c )
{
    return pow( c, vec3(gamma) );
}
vec3 srgb2lin( vec3 c )
{
    return pow( c, vec3(1.0/gamma));
}


vec3 yCgCo2rgb(vec3 ycc)
{
    float R = ycc.x - ycc.y + ycc.z;
    float G = ycc.x + ycc.y;
    float B = ycc.x - ycc.y - ycc.z;
    return vec3(R,G,B);
}

vec3 spectrum_offset_ycgco( float t )
{
    vec3 ygo = vec3( 1.0, 0.0, -1.25*t ); //cyan-orange
    return yCgCo2rgb( ygo );
}

vec3 yuv2rgb( vec3 yuv )
{
    vec3 rgb;
    rgb.r = yuv.x + yuv.z * 1.13983;
    rgb.g = yuv.x + dot( vec2(-0.39465, -0.58060), yuv.yz );
    rgb.b = yuv.x + yuv.y * 2.03211;
    return rgb;
}

//note: from https://www.shadertoy.com/view/XslGz8
vec2 radialdistort(vec2 coord, vec2 amt)
{
    vec2 cc = coord - 0.5;
    return coord + 2.0 * cc * amt;
}

// Given a vec2 in [-1,+1], generate a texture coord in [0,+1]
vec2 barrelDistortion( vec2 p, vec2 amt )
{
    p = 2.0 * p - 1.0;
    float maxBarrelPower = sqrt(5.0);
    float radius = dot(p,p);
    p *= pow(vec2(radius), maxBarrelPower * amt);

    return p * 0.5 + 0.5;
}

//note: from https://www.shadertoy.com/view/MlSXR3
vec2 brownConradyDistortion(vec2 uv, float dist)
{
    uv = uv * 2.0 - 1.0;
    // positive values of K1 give barrel distortion, negative give pincushion
    float barrelDistortion1 = 0.1 * dist; // K1 in text books
    float barrelDistortion2 = -0.025 * dist; // K2 in text books

    float r2 = dot(uv,uv);
    uv *= 1.0 + barrelDistortion1 * r2 + barrelDistortion2 * r2 * r2;

    return uv * 0.5 + 0.5;
}

vec2 distort( vec2 uv, float t, vec2 min_distort, vec2 max_distort )
{
    vec2 dist = mix( min_distort, max_distort, t );
    return brownConradyDistortion( uv, 75.0 * dist.x );
}

vec3 spectrum_offset_yuv( float t )
{
    vec3 yuv = vec3( 1.0, 0.0, -1.0*t ); //cyan-orange
    return yuv2rgb( yuv );
}

vec3 spectrum_offset( float t )
{
      return spectrum_offset_rgb( t );
}

vec3 render( vec2 uv )
{
    return srgb2lin(texture( sDiffMap, uv ).rgb );
}

float nrand( vec2 n )
{
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

void main()
{
    vec2 screenSize = textureSize(sDiffMap, 0);

    const float MAX_DIST_PX = 50.0;
    float max_distort_px = MAX_DIST_PX * 0.5; // INTENSITY
    vec2 max_distort = vec2(max_distort_px) / screenSize.xy;
    vec2 min_distort = 0.5 * max_distort;

    vec2 oversiz = distort( vec2(1.0), 1.0, min_distort, max_distort );
    vec2 uv = remap( vScreenPos, 1.0-oversiz, oversiz );

    const int num_iter = 7;
    const float stepsiz = 1.0 / (float(num_iter)-1.0);
    float rnd = nrand( uv + fract(cElapsedTime));
    float t = rnd * stepsiz;

    vec3 sumcol = vec3(0.0);
    vec3 sumw = vec3(0.0);
    for ( int i=0; i<num_iter; ++i )
    {
        vec3 w = spectrum_offset( t );
        sumw += w;
        vec2 uvd = distort(uv, t, min_distort, max_distort ); //TODO: move out of loop
        sumcol += w * render( uvd );
        t += stepsiz;
    }
    sumcol.rgb /= sumw;

    vec3 outcol = sumcol.rgb;
    outcol = lin2srgb( outcol );
    outcol += rnd/255.0;

    gl_FragColor = vec4( outcol, 1.0);
}
#endif
