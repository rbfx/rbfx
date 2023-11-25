/// _Material_Pixel_DepthOnly.glsl
/// Don't include!
/// Default depth-only vertex and pixel shaders.

/// @def Vertex_SetTransform(vertexTransform)
/// @brief Fill vertex position and clip distance, if enabled.
/// @param[in] vertexTransform VertexTransform structure.
/// @param[out] gl_Position Fills the position in clip space.
/// @param[out,optional] gl_ClipDistance[0]

/// @def Vertex_SetTexCoord(uOffset, vOffset)
/// @brief Fill texture coordinates if requested.
/// @param[in] uOffset U offset for texture coordinate.
/// @param[in] vOffset V offset for texture coordinate.
/// @param[out] vTexCoord

/// @def Vertex_SetDepth()
/// @brief Fill depth if VSM is used.
/// @param[out] vDepth

/// @def Pixel_DepthOnly(albedoMap, texCoord)
/// @brief Default depth-only pixel shader.
/// @param[in,optional] albedoMap Albedo map texture used for alpha cutout. Ignored unless ALPHAMASK is defined.
/// @param[in,optional] texCoord Texture coordinate for albedo map sampling. Ignored unless ALPHAMASK is defined.
/// @param[out,optional] gl_FragColor Fills the color with (depth, depth^2), if VSM is used.

VERTEX_OUTPUT_HIGHP(vec2 vTexCoord)
#ifdef URHO3D_VARIANCE_SHADOW_MAP
    VERTEX_OUTPUT_HIGHP(vec2 vDepth)
#endif

#ifdef URHO3D_VERTEX_SHADER

void Vertex_SetTransform(VertexTransform vertexTransform)
{
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    ApplyClipPlane(gl_Position);
}

#ifdef URHO3D_PIXEL_NEED_TEXCOORD
    #define Vertex_SetTexCoord(uOffset, vOffset) \
        vTexCoord = GetTransformedTexCoord(uOffset, vOffset);
#else
    #define Vertex_SetTexCoord(uOffset, vOffset)
#endif

#ifdef URHO3D_VARIANCE_SHADOW_MAP
    #define Vertex_SetDepth(void) \
        vDepth = gl_Position.zw;
#else
    #define Vertex_SetDepth(void)
#endif

#define Vertex_SetAll(vertexTransform, normalScale, uOffset, vOffset, lightMapScaleOffset) \
{ \
    Vertex_SetTransform(vertexTransform); \
    Vertex_SetTexCoord(uOffset, vOffset); \
    Vertex_SetDepth(void); \
}

#endif // URHO3D_VERTEX_SHADER

#ifdef URHO3D_PIXEL_SHADER

#ifdef URHO3D_VARIANCE_SHADOW_MAP
    void _CopyDepthToColor(vec2 depthW)
    {
        float depth = depthW.x / depthW.y;
        #ifdef URHO3D_OPENGL
            // Remap from [-1, 1] to [0, 1] for OpenGL
            depth = depth * 0.5 + 0.5;
        #endif
        gl_FragColor = vec4(depth, depth * depth, 1.0, 1.0);
    }
#else
    #define _CopyDepthToColor(depthW)
#endif

#ifdef ALPHAMASK
    void _Pixel_DepthOnly(sampler2D albedoMap, vec2 texCoord)
    {
        fixed alpha = texture(albedoMap, texCoord).a;
        if (alpha < 0.5)
            discard;

        _CopyDepthToColor(vDepth);
    }

    #define Pixel_DepthOnly(albedoMap, texcoord) _Pixel_DepthOnly(albedoMap, (texcoord).xy)
#else
    #define Pixel_DepthOnly(albedoMap, texcoord) _CopyDepthToColor(vDepth)
#endif

#endif // URHO3D_PIXEL_SHADER
