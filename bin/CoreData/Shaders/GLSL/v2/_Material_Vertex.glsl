/// _Material_Vertex.glsl
/// Don't include!
/// Material vertex shader implementation.

/// @def FillVertexTransformOutputs(vertexTransform, normalScale)
/// @brief Fill vertex attributes related to vertex position, whichever requested.
/// @param[in] vertexTransform VertexTransform structure.
/// @param[in] normalScale Tangent and binormal axes scale. Used to fade out the influence of normal map. 1 is default.
/// @param[out] gl_Position
/// @param[out] vWorldDepth
/// @param[out] vWorldPos
/// @param[out] vNormal
/// @param[out] vTangent
/// @param[out] vBitangentXY

/// @def FillTexCoordOutput(uOffset, vOffset)
/// @brief Fill texture coordinates if requested.
/// @param[in] uOffset U offset for texture coordinate.
/// @param[in] vOffset V offset for texture coordinate.
/// @param[out] vTexCoord

/// @def FillLightMapTexCoordOutput(scaleOffset)
/// @brief Fill lightmap texture coordinates if requested.
/// @param[in] scaleOffset XY scale and XY offset for lightmap texture coordinate.
/// @param[out] vTexCoord2

/// @def FillColorOutput()
/// @brief Fill vertex color if requested.
/// @param[out] vColor

void FillVertexTransformOutputs(VertexTransform vertexTransform, half normalScale)
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
    #define FillTexCoordOutput(uOffset, vOffset) \
        vTexCoord = GetTransformedTexCoord(uOffset, vOffset);
#else
    #define FillTexCoordOutput(uOffset, vOffset)
#endif

#ifdef URHO3D_PIXEL_NEED_LIGHTMAP_UV
    #define FillLightMapTexCoordOutput(scaleOffset) \
        vTexCoord2 = iTexCoord1 * scaleOffset.xy + scaleOffset.zw;
#else
    #define FillLightMapTexCoordOutput(scaleOffset)
#endif

#ifdef URHO3D_PIXEL_NEED_VERTEX_COLOR
    #define FillColorOutput() \
        vColor = iColor;
#else
    #define FillColorOutput()
#endif

/// Fill lighting attributes:
/// - vAmbientAndVertexLigthing
/// - vLightVec
/// - vShadowPos
/// - vShapePos
void FillLightOutputs(VertexTransform vertexTransform)
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
    #define FillScreenPosOutput(clipPos) \
        vScreenPos = GetScreenPos(clipPos)
#else
    #define FillScreenPosOutput(clipPos)
#endif

/// Fill eye vector:
/// - vEyeVec
#ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
    #define FillEyeVectorOutput(worldPos) \
        vEyeVec = cCameraPos - worldPos
#else
    #define FillEyeVectorOutput(worldPos)
#endif

/// Fill reflection vector:
/// - vReflectionVec
#ifdef URHO3D_PIXEL_NEED_REFLECTION_VECTOR
    #ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
        #define FillReflectionVectorOutput(worldPos) \
            vReflectionVec = reflect(-vEyeVec, vNormal)
    #else
        #define FillReflectionVectorOutput(worldPos) \
            vReflectionVec = reflect(worldPos - cCameraPos, vNormal)
    #endif
#else
    #define FillReflectionVectorOutput(worldPos)
#endif

/// Fill eye and/or reflection vector:
/// - vEyeVec
/// - vReflectionVec
#define FillEyeAndReflectionVectorOutputs(worldPos) \
    FillEyeVectorOutput(worldPos); \
    FillReflectionVectorOutput(worldPos)

/// Fill all abovementioned outputs.
#define FillVertexOutputs(vertexTransform, normalScale, uOffset, vOffset, lightMapScaleOffset) \
    FillVertexTransformOutputs(vertexTransform, normalScale); \
    FillTexCoordOutput(uOffset, vOffset); \
    FillLightMapTexCoordOutput(lightMapScaleOffset); \
    FillColorOutput(); \
    FillLightOutputs(vertexTransform); \
    FillScreenPosOutput(gl_Position); \
    FillEyeAndReflectionVectorOutputs(vertexTransform.position.xyz) \
    ApplyClipPlane(gl_Position)
