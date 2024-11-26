#include "_Config.glsl"

SAMPLER(0, sampler2D sAlbedo)
VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)

#ifdef URHO3D_VERTEX_SHADER
VERTEX_INPUT(vec4 iPos)
void main()
{
    gl_Position = iPos;

    // See GetQuadTexCoord
    vTexCoord.x = gl_Position.x / gl_Position.w * 0.5 + 0.5;
#ifdef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
    vTexCoord.y = -gl_Position.y / gl_Position.w * 0.5 + 0.5;
#else
    vTexCoord.y = gl_Position.y / gl_Position.w * 0.5 + 0.5;
#endif
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    gl_FragColor = texture(sAlbedo, vTexCoord);
}
#endif
