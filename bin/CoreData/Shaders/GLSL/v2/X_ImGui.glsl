#include "_Config.glsl"
#include "_GammaCorrection.glsl"

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
VERTEX_OUTPUT(half4 vColor)

#ifdef URHO3D_VERTEX_SHADER

UNIFORM_BUFFER_BEGIN(0, Camera)
    UNIFORM_HIGHP(mat4 cProjectionMatrix)
UNIFORM_BUFFER_END(0, Camera)

VERTEX_INPUT(vec2 iPos)
VERTEX_INPUT(vec2 iTexCoord)
VERTEX_INPUT(vec4 iColor)

void main()
{
    gl_Position = cProjectionMatrix * vec4(iPos, 0.0, 1.0);
    vTexCoord  = iTexCoord;
    vColor = iColor;
}

#endif

#ifdef URHO3D_PIXEL_SHADER

SAMPLER(0, sampler2D sTexture)
void main()
{
    vec4 color = vColor * texture(sTexture, vTexCoord);
#ifdef URHO3D_LINEAR_OUTPUT
    color.rgb = GammaToLinearSpace(color.rgb);
#endif
    gl_FragColor = color;
}

#endif
