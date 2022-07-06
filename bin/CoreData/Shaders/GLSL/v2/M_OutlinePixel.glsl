#include "_Config.glsl"
#include "_Uniforms.glsl"
#include "_Samplers.glsl"
#include "_Material_Common.glsl"

#ifdef URHO3D_VERTEX_SHADER
    #error This file should not be compiled as vertex shader
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef ALPHAMASK
    half alpha = texture2D(sDiffMap, vTexCoord).a;
    if (alpha < 0.5)
        discard;
#endif

    // Don't do any color space conversions
    gl_FragColor = vec4(cMatDiffColor.rgb, 1.0);
}
#endif
