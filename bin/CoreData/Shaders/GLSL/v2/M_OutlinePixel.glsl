#include "_Material.glsl"

#ifdef URHO3D_VERTEX_SHADER
    #error This file should not be compiled as vertex shader
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#if defined(URHO3D_MATERIAL_HAS_DIFFUSE) && defined(ALPHAMASK)
    half alpha = texture2D(sDiffMap, vTexCoord).a;
    if (alpha < 0.5)
        discard;
#endif

    gl_FragColor = vec4(cMatDiffColor.rgb, 1.0);
}
#endif
