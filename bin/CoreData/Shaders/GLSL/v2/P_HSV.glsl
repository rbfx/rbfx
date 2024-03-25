#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_VertexLayout.glsl"
#include "_VertexTransform.glsl"
#include "_VertexScreenPos.glsl"
#include "_DefaultSamplers.glsl"
#include "_SamplerUtils.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vScreenPos)

#ifdef URHO3D_PIXEL_SHADER
    UNIFORM_BUFFER_BEGIN(6, Custom)
        UNIFORM(mediump vec4 cHSVParams)
        UNIFORM(mediump vec4 cColorFilter)
        UNIFORM(mediump vec4 cColorOffset)
    UNIFORM_BUFFER_END(6, Custom)
#endif

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}
#endif

#ifdef URHO3D_PIXEL_SHADER

// You can find good explanation of the math here:
// https://blog.en.uwa4d.com/2022/09/29/screen-post-processing-effects-color-models-and-color-grading/

const half M_MEDIUMP_FLT_EPS = 0.0009765626;

half3 RGBToHSV(half3 c)
{
    half4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    half4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    half4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    half d = q.x - min(q.w, q.y);
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + M_MEDIUMP_FLT_EPS)), d / (q.x + M_MEDIUMP_FLT_EPS), q.x);
}

half3 HSVToRGB(half3 c)
{
    half4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    half3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    half3 rgb = texture(sAlbedo, vScreenPos).rgb;
    half3 hsv = RGBToHSV(rgb);
    half3 correctedHsv = vec3(fract(hsv.x+cHSVParams.x), hsv.y*cHSVParams.y, ((hsv.z*cHSVParams.z)-0.5)*cHSVParams.w+0.5);
    half3 correctedRgb = HSVToRGB(correctedHsv) * cColorFilter.rgb + cColorOffset.rgb;
    gl_FragColor = vec4(correctedRgb.r, correctedRgb.g, correctedRgb.b, 1.0);
}
#endif
