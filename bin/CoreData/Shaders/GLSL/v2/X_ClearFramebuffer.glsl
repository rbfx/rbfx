#include "_Config.glsl"

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(half4 cColor)
    UNIFORM_HIGHP(float cDepth)
UNIFORM_BUFFER_END(6, Custom)

#ifdef URHO3D_VERTEX_SHADER
VERTEX_INPUT(vec4 iPos)

void main()
{
    gl_Position = vec4(iPos.x, iPos.y, cDepth, 1.0);
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    gl_FragColor = cColor;
}
#endif

