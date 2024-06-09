#define URHO3D_PIXEL_NEED_TEXCOORD
#define URHO3D_CUSTOM_LIGHT_UNIFORMS
#define URHO3D_CUSTOM_MATERIAL_UNIFORMS
#ifdef URHO3D_VERTEX_HAS_COLOR
    #undef URHO3D_VERTEX_HAS_COLOR
#endif

#include "_Config.glsl"
#include "_Uniforms.glsl"

// JS: this has to be before samplers so we can see it for depth reconstruction
#ifdef URHO3D_XR
    VERTEX_OUTPUT_QUAL(flat, int vInstID)
#endif

#include "_DefaultSamplers.glsl"
#include "_SamplerUtils.glsl"
#include "_Material_Common.glsl"

UNIFORM_BUFFER_BEGIN(6, Custom)
    UNIFORM(half4 cOutlineColor)
UNIFORM_BUFFER_END(6, Custom)

#ifdef URHO3D_VERTEX_SHADER
    #error This file should not be compiled as vertex shader
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef ALPHAMASK
    half alpha = texture(sAlbedo, vTexCoord).a;
    if (alpha < 0.5)
        discard;
#endif

    // Don't do any color space conversions
    gl_FragColor = vec4(cOutlineColor.rgb, 1.0);
}
#endif
