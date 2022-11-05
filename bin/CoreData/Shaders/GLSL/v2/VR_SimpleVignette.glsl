
#include "_Config.glsl"

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM_HIGHP(mat4 cProjection)
    UNIFORM_HIGHP(vec4 cInsideColor)
    UNIFORM_HIGHP(vec4 cOutsideColor)
    UNIFORM_HIGHP(float cBlendPower)
UNIFORM_BUFFER_END(6, Custom)

#ifdef COMPILEVS
    VERTEX_INPUT(vec3 iPos)
    VERTEX_INPUT(vec4 iColor0)
#endif

VERTEX_OUTPUT(vec4 oColor)

#ifdef COMPILEVS

void main()
{
    gl_Position = vec4(iPos, 1.0f);
    oColor = mix(cInsideColor, cOutsideColor, pow(iColor0.a, cBlendPower));
}

#endif

#ifdef COMPILEPS

void main()
{
    gl_FragColor = oColor;
}

#endif
