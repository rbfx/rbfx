/// _Material_Vertex.glsl
/// Don't include!
/// Material vertex shader implementation.

/// @def Vertex_SetTransform(vertexTransform, normalScale)
/// @brief Fill vertex attributes related to vertex position, whichever requested.
/// @param[in] vertexTransform VertexTransform structure.
/// @param[in] normalScale Tangent and binormal axes scale. Used to fade out the influence of normal map. 1 is default.
/// @param[out] gl_Position
/// @param[out] vWorldDepth
/// @param[out] vWorldPos
/// @param[out] vNormal
/// @param[out] vTangent
/// @param[out] vBitangentXY

/// @def Vertex_SetTexCoord(uOffset, vOffset)
/// @brief Fill texture coordinates if requested.
/// @param[in] uOffset U offset for texture coordinate.
/// @param[in] vOffset V offset for texture coordinate.
/// @param[out] vTexCoord

/// @def Vertex_SetLightMapTexCoord(scaleOffset)
/// @brief Fill lightmap texture coordinates if requested.
/// @param[in] scaleOffset XY scale and XY offset for lightmap texture coordinate.
/// @param[out] vTexCoord2

/// @def Vertex_SetColor()
/// @brief Fill vertex color if requested.
/// @param[out] vColor

void Vertex_SetTransform(VertexTransform vertexTransform, half normalScale)
{
    gl_Position = WorldToClipSpace(vertexTransform.position.xyz);
    vWorldDepth = GetDepth(gl_Position);

#ifdef URHO3D_PIXEL_NEED_WORLD_POSITION
    vWorldPos = vertexTransform.position.xyz;
#endif

#ifdef URHO3D_PIXEL_NEED_NORMAL
    vNormal = vertexTransform.normal;
#endif

#ifdef URHO3D_PIXEL_NEED_TANGENT
    vTangent = normalScale * vec4(vertexTransform.tangent.xyz, vertexTransform.bitangent.z);
    vBitangentXY = normalScale * vertexTransform.bitangent.xy;
#endif
}

#ifdef URHO3D_PIXEL_NEED_TEXCOORD
    #define Vertex_SetTexCoord(uOffset, vOffset) \
        vTexCoord = GetTransformedTexCoord(uOffset, vOffset);
#else
    #define Vertex_SetTexCoord(uOffset, vOffset)
#endif

#ifdef URHO3D_PIXEL_NEED_LIGHTMAP_UV
    #define Vertex_SetLightMapTexCoord(scaleOffset) \
        vTexCoord2 = iTexCoord1 * scaleOffset.xy + scaleOffset.zw;
#else
    #define Vertex_SetLightMapTexCoord(scaleOffset)
#endif

#ifdef URHO3D_PIXEL_NEED_VERTEX_COLOR
    #define Vertex_SetColor(void) \
        vColor = iColor;
#else
    #define Vertex_SetColor(void)
#endif

/// Fill lighting attributes:
/// - vAmbientAndVertexLigthing
/// - vLightVec
/// - vShadowPos
/// - vShapePos
void Vertex_SetLight(VertexTransform vertexTransform)
{
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    vAmbientAndVertexLigthing = GetAmbientAndVertexLights(vertexTransform.position.xyz, vertexTransform.normal);
#endif

#ifdef URHO3D_PIXEL_NEED_LIGHT_VECTOR
    vLightVec = GetLightVector(vertexTransform.position.xyz);
#endif

#ifdef URHO3D_PIXEL_NEED_SHADOW_POS
    WorldSpaceToShadowCoord(vShadowPos, vertexTransform.position);
#endif

#ifdef URHO3D_PIXEL_NEED_LIGHT_SHAPE_POS
    vShapePos = vertexTransform.position * cLightShapeMatrix;
#endif
}

/// Fill position at render target:
/// - vScreenPos
#ifdef URHO3D_PIXEL_NEED_SCREEN_POSITION
    #define Vertex_SetScreenPos(clipPos) \
        vScreenPos = GetScreenPos(clipPos)
#else
    #define Vertex_SetScreenPos(clipPos)
#endif

/// Fill eye vector:
/// - vEyeVec
#ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
    #define Vertex_SetEyeVector(worldPos) \
        vEyeVec = cCameraPos - worldPos
#else
    #define Vertex_SetEyeVector(worldPos)
#endif

/// Fill reflection vector:
/// - vReflectionVec
#ifdef URHO3D_PIXEL_NEED_REFLECTION_VECTOR
    #ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
        #define Vertex_SetReflectionVector(worldPos) \
            vReflectionVec = reflect(-vEyeVec, vNormal)
    #else
        #define Vertex_SetReflectionVector(worldPos) \
            vReflectionVec = reflect(worldPos - cCameraPos, vNormal)
    #endif
#else
    #define Vertex_SetReflectionVector(worldPos)
#endif

/// Fill all abovementioned outputs.
#define Vertex_SetAll(vertexTransform, normalScale, uOffset, vOffset, lightMapScaleOffset) \
    Vertex_SetTransform(vertexTransform, normalScale); \
    Vertex_SetTexCoord(uOffset, vOffset); \
    Vertex_SetLightMapTexCoord(lightMapScaleOffset); \
    Vertex_SetColor(void); \
    Vertex_SetLight(vertexTransform); \
    Vertex_SetScreenPos(gl_Position); \
    Vertex_SetEyeVector(vertexTransform.position.xyz); \
    Vertex_SetReflectionVector(vertexTransform.position.xyz); \
    ApplyClipPlane(gl_Position)
