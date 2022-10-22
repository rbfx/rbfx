
#include "_Config.glsl"

UNIFORM_BUFFER_BEGIN(6, Constants)
    UNIFORM_HIGHP(float4x4 cProjection)
    UNIFORM_HIGHP(float4 cInsideColor)
    UNIFORM_HIGHP(float4 cOutsideColor)
    UNIFORM_HIGHP(float cBlendPower)
UNIFORM_BUFFER_END(6, Constants)

VERTEX_INPUT(vec3 iPosition)
VERTEX_INPUT(vec4 iColor0)
VERTEX_OUTPUT(vec4 oColor)

#ifdef COMPILE_VS

void main()
{
#ifdef OPEN_XR
    gl_Position = vec4(cProjection * iPos, 1.0f);
#endif
    oColor = lerp(cInsideColor, cOutsideColor, pow(iColor0.a, cBlendPower));
}

#endif

#ifdef COMPILE_PS

void main()
{
    gl_FragColor = oColor;
}

#endif