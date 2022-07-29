#ifndef _FOG_GLSL_
#define _FOG_GLSL_

#ifndef _UNIFORMS_GLSL_
    #error Include _Uniforms.glsl before _Fog.glsl
#endif

/// Evaluate fog factor for given linear depth. 0 is pure fog, 1 is pure color.
fixed GetFogFactor(const half depth)
{
    return clamp((cFogParams.x - depth) * cFogParams.y, 0.0, 1.0);
}

#ifdef URHO3D_PIXEL_SHADER
    /// Evaluate background or fog color according to premultiplied alpha.
    #ifdef URHO3D_PREMULTIPLY_ALPHA
        #define GetConstantFragmentColor(color, alpha) ((color) * (alpha))
    #else
        #define GetConstantFragmentColor(color, alpha) (color)
    #endif

    /// Evaluate final color of the fragment.
    half3 GetFragmentColor(const half3 color, const half alpha, const fixed fogFactor)
    {
        #ifdef URHO3D_ADDITIVE_BLENDING
            return color * (alpha * fogFactor);
        #else
            return mix(GetConstantFragmentColor(cFogColor, alpha), color, fogFactor);
        #endif
    }

    /// Evaluate final color and alpha of the fragment.
    #ifndef URHO3D_ADDITIVE_BLENDING
        #define GetFragmentColorAlpha(color, alpha, fogFactor) vec4(GetFragmentColor(color, alpha, fogFactor), alpha)
    #else
        #define GetFragmentColorAlpha(color, alpha, fogFactor) vec4(GetFragmentColor(color, alpha, fogFactor), 0.0)
    #endif
#endif // URHO3D_PIXEL_SHADER

#endif // _FOG_GLSL_
