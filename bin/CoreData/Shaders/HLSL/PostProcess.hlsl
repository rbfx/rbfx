
static const float PI = 3.14159265;

float2 Noise(float2 coord)
{
    float noiseX = clamp(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0);
    float noiseY = clamp(frac(sin(dot(coord, float2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0);
    return float2(noiseX, noiseY);
}

// Adapted: http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
#ifndef D3D11
float4 GaussianBlur(int blurKernelSize, float2 blurDir, float2 blurRadius, float sigma, sampler2D texSampler, float2 texCoord)
#else
float4 GaussianBlur(int blurKernelSize, float2 blurDir, float2 blurRadius, float sigma, Texture2D tex, SamplerState texSampler, float2 texCoord)
#endif
{
    const int blurKernelHalfSize = blurKernelSize / 2;

    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    float3 gaussCoeff;
    gaussCoeff.x = 1.0 / (sqrt(2.0 * PI) * sigma);
    gaussCoeff.y = exp(-0.5 / (sigma * sigma));
    gaussCoeff.z = gaussCoeff.y * gaussCoeff.y;

    float2 blurVec = blurRadius * blurDir;
    float4 avgValue = float4(0.0, 0.0, 0.0, 0.0);
    float gaussCoeffSum = 0.0;

    #ifndef D3D11
    avgValue += tex2D(texSampler, texCoord) * gaussCoeff.x;
    #else
    avgValue += tex.Sample(texSampler, texCoord) * gaussCoeff.x;
    #endif

    gaussCoeffSum += gaussCoeff.x;
    gaussCoeff.xy *= gaussCoeff.yz;

    for (int i = 1; i <= blurKernelHalfSize; i++)
    {
        #ifndef D3D11
        avgValue += tex2D(texSampler, texCoord - i * blurVec) * gaussCoeff.x;
        avgValue += tex2D(texSampler, texCoord + i * blurVec) * gaussCoeff.x;
        #else
        avgValue += tex.Sample(texSampler, texCoord - i * blurVec) * gaussCoeff.x;
        avgValue += tex.Sample(texSampler, texCoord + i * blurVec) * gaussCoeff.x;
        #endif

        gaussCoeffSum += 2.0 * gaussCoeff.x;
        gaussCoeff.xy *= gaussCoeff.yz;
    }

    return avgValue / gaussCoeffSum;
}

static const float3 LumWeights = float3(0.2126, 0.7152, 0.0722);

float3 ReinhardEq3Tonemap(float3 x)
{
    return x / (x + 1.0);
}

float3 ReinhardEq4Tonemap(float3 x, float white)
{
    return x * (1.0 + x / white) / (1.0 + x);
}

// Unchared2 tone mapping (See http://filmicgames.com)
static const float A = 0.15;
static const float B = 0.50;
static const float C = 0.10;
static const float D = 0.20;
static const float E = 0.02;
static const float F = 0.30;

float3 Uncharted2Tonemap(float3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

#ifndef D3D11
float3 ColorCorrection(float3 color, sampler3D lut)
#else
float3 ColorCorrection(float3 color, Texture3D lut, SamplerState lutSampler)
#endif
{
    float lutSize = 16.0;
    float scale = (lutSize - 1.0) / lutSize;
    float offset = 1.0 / (2.0 * lutSize);
    #ifndef D3D11
        return tex3D(lut, clamp(color, 0.0, 1.0) * scale + offset).rgb;
    #else
        return lut.Sample(lutSampler, clamp(color, 0.0, 1.0) * scale + offset).rgb;
    #endif
}

static const float Gamma = 2.2;
static const float InverseGamma = 1.0 / 2.2;

float3 ToGamma(float3 color)
{
    float3 linearRGBLo = color / 12.92;
    float3 linearRGBHi = pow (( color + 0.055) / 1.055 , 2.4) ;
    float3 linearRGB = ( color <= 0.04045) ? linearRGBLo : linearRGBHi ;
    return linearRGB ;

}

float3 ToInverseGamma(float3 color)
{
    float3 sRGBLo = color * 12.92;
    float3 sRGBHi = ( pow( abs ( color ) , 1.0/2.4) * 1.055) - 0.055;
    float3 sRGB = ( color <= 0.0031308) ? sRGBLo : sRGBHi ;
    return sRGB ;
}
