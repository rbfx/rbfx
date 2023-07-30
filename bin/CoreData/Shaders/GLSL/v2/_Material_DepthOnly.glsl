/// _Material_Pixel_DepthOnly.glsl
/// Don't include!
/// Default depth-only vertex and pixel shaders.

/// @def FillVertexOutputs(vertexTransform)
/// @brief Fill vertex outputs that may be used by the pixel shader.
/// @param[in] vertexTransform VertexTransform structure.
/// @param[out] gl_Position Fills the position in clip space.
/// @param[out,optional] gl_ClipDistance[0] Fills the clip plane distance, if enabled.
/// @param[out,optional] vTexCoord Fills the texture coordinate, if requested.
/// @param[out,optional] vDepth Fills the depth, if VSM is used.

/// @def DepthOnlyPixelShader(albedoMap, texCoord)
/// @brief Default depth-only pixel shader.
/// @param[in,optional] albedoMap Albedo map texture used for alpha cutout. Ignored unless ALPHAMASK is defined.
/// @param[in,optional] texCoord Texture coordinate for albedo map sampling. Ignored unless ALPHAMASK is defined.
/// @param[out,optional] gl_FragColor Fills the color with (depth, depth^2), if VSM is used.

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
#ifdef URHO3D_VARIANCE_SHADOW_MAP
    VERTEX_OUTPUT_HIGHP(vec2 vDepth)
#endif

#ifdef URHO3D_VERTEX_SHADER

void _FillVertexOutputs(VertexTransform vertexTransform)
{
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    ApplyClipPlane(gl_Position);
#ifdef URHO3D_PIXEL_NEED_TEXCOORD
    vTexCoord = GetTransformedTexCoord();
#endif
#ifdef URHO3D_VARIANCE_SHADOW_MAP
    vDepth = gl_Position.zw;
#endif
}

#define FillVertexOutputs(vertexTransform, normalScale) \
    _FillVertexOutputs(vertexTransform)

#endif // URHO3D_VERTEX_SHADER

#ifdef URHO3D_PIXEL_SHADER

#ifdef URHO3D_VARIANCE_SHADOW_MAP
    void _CopyDepthToColor()
    {
        float depth = vDepth.x / vDepth.y;
        #ifndef URHO3D_OPENGL
            // Remap from [-1, 1] to [0, 1] for OpenGL
            depth = depth * 0.5 + 0.5;
        #endif
        gl_FragColor = vec4(depth, depth * depth, 1.0, 1.0);
    }
#else
    #define _CopyDepthToColor()
#endif

#ifdef ALPHAMASK
    void _DepthOnlyPixelShader(sampler2D albedoMap, vec2 texCoord)
    {
        fixed alpha = texture(albedoMap, texCoord).a;
        if (alpha < 0.5)
            discard;

        _CopyDepthToColor();
    }

    #define DepthOnlyPixelShader(albedoMap, texcoord) _DepthOnlyPixelShader(albedoMap, (texcoord).xy)
#else
    #define DepthOnlyPixelShader(albedoMap, texcoord) _CopyDepthToColor()
#endif

#endif // URHO3D_PIXEL_SHADER
