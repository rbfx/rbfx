
#include "_Config.glsl"

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM_HIGHP(mat4 cProjection)
UNIFORM_BUFFER_END(6, Custom)

#ifdef COMPILEVS

VERTEX_INPUT(vec4 iPos)

void main()
{
    gl_Position = iPos;
}

#endif

#ifdef COMPILEPS

void main()
{
    gl_FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

#endif