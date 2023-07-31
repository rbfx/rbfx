/// _Material_Pixel_Fill.glsl
/// Don't include!
/// Fill SurfaceData structure.

/// =================================== Non-customizable properties ===================================

/// @def FillSurfaceFogFactor(surfaceData)
/// @brief Fill fog factor in SurfaceData.
/// @param[out] surfaceData.fogFactor

/// @def FillSurfaceEyeVector(surfaceData)
/// @brief Fill eye vector in SurfaceData.
/// @param[out] surfaceData.eyeVec

/// @def FillSurfaceScreenPosition(surfaceData)
/// @brief Fill screen position in SurfaceData.
/// @param[out] surfaceData.screenPos

/// @def FillSurfaceCommon(surfaceData)
/// @brief Same as FillSurfaceFogFactor, FillSurfaceEyeVector, FillSurfaceScreenPosition together.

#define FillSurfaceFogFactor(surfaceData) \
    surfaceData.fogFactor = GetFogFactor(vWorldDepth)

#ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
    #define FillSurfaceEyeVector(surfaceData) \
        surfaceData.eyeVec = normalize(vEyeVec)
#else
    #define FillSurfaceEyeVector(surfaceData)
#endif

#ifdef URHO3D_PIXEL_NEED_SCREEN_POSITION
    #define FillSurfaceScreenPosition(surfaceData) \
        surfaceData.screenPos = vScreenPos.xy / vScreenPos.w
#else
    #define FillSurfaceScreenPosition(surfaceData)
#endif

#define FillSurfaceCommon(surfaceData) \
{ \
    FillSurfaceFogFactor(surfaceData); \
    FillSurfaceEyeVector(surfaceData); \
    FillSurfaceScreenPosition(surfaceData); \
}

/// =================================== Ambient lighting ===================================

/// @def FillSurfaceAmbient(surfaceData, lightMap, texCoord)
/// @brief Fill ambient lighting in SurfaceData.
/// @param[in,optional] lightMap Lightmap texture.
///     Ignored if URHO3D_HAS_LIGHTMAP is not defined.
/// @param[in,optional] texCoord Texture coordinate for lightmap lookup.
///     Ignored if URHO3D_HAS_LIGHTMAP is not defined.
/// @param[out] surfaceData.ambientLighting

#ifdef URHO3D_SURFACE_NEED_AMBIENT
    #ifdef URHO3D_HAS_LIGHTMAP
        half3 _GetSurfaceLightmap(sampler2D lightMap, vec2 texCoord)
        {
            // TODO: Support lightmaps in linear space.
            return GammaToLightSpace(2.0 * texture(lightMap, texCoord).rgb);
        }

        #define FillSurfaceAmbient(surfaceData, lightMap, texCoord) \
            surfaceData.ambientLighting = vAmbientAndVertexLigthing + _GetSurfaceLightmap(lightMap, texCoord)
    #else
        #define FillSurfaceAmbient(surfaceData, lightMap, texCoord) \
            surfaceData.ambientLighting = vAmbientAndVertexLigthing
    #endif
#else
    #define FillSurfaceAmbient(surfaceData, lightMap, texCoord)
#endif

/// =================================== Surface normal ===================================

/// @def FillSurfaceNormal(surfaceData, normal, normalMap, texCoord, tangent, bitangentXY)
/// @brief Fill surface normal in SurfaceData.
/// @param[in] vertexNormal Vertex normal.
/// @param[in,optional] normalMap Normal map texture.
///     Ignored if URHO3D_NORMAL_MAPPING is not defined.
/// @param[in,optional] texCoord Texture coordinate for normal map lookup.
///     Ignored if URHO3D_NORMAL_MAPPING is not defined.
/// @param[in,optional] vertexTangent Vertex tangent.
///     Ignored if URHO3D_NORMAL_MAPPING is not defined.
/// @param[in,optional] vertexBitangentXY Vertex bitangent XY.
///     Ignored if URHO3D_NORMAL_MAPPING is not defined.
/// @param[out] surfaceData.normal
/// @param[out] surfaceData.normalInTangentSpace

#ifdef URHO3D_SURFACE_NEED_NORMAL
    #ifdef URHO3D_NORMAL_MAPPING
        half3 _GetSurfaceNormal(
            out half3 normalInTangentSpace,
            sampler2D normalMap, vec2 texCoord, half3 vertexNormal, half4 vertexTangent, half2 vertexBitangentXY)
        {
            normalInTangentSpace = DecodeNormal(texture(normalMap, texCoord));
            mediump mat3 tbn = mat3(vertexTangent.xyz, vec3(vertexBitangentXY, vertexTangent.w), vertexNormal);
            half3 normal = normalize(tbn * normalInTangentSpace);

        #ifdef URHO3D_SURFACE_TWO_SIDED
            normal *= SELECT_FRONT_BACK_FACE(1.0, -1.0);
        #endif

            return normal;
        }

        #define FillSurfaceNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY) \
            surfaceData.normal = _GetSurfaceNormal( \
                surfaceData.normalInTangentSpace, normalMap, texCoord, vertexNormal, vertexTangent, vertexBitangentXY)
    #else
        half3 _GetSurfaceNormal(out half3 normalInTangentSpace, half3 vertexNormal)
        {
            normalInTangentSpace = vec3(0.0, 0.0, 1.0);
            half3 normal = normalize(vertexNormal);

        #ifdef URHO3D_SURFACE_TWO_SIDED
            normal *= SELECT_FRONT_BACK_FACE(1.0, -1.0);
        #endif

            return normal;
        }

        #define FillSurfaceNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY) \
            surfaceData.normal = _GetSurfaceNormal(surfaceData.normalInTangentSpace, vertexNormal)
    #endif
#else
    #define FillSurfaceNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY)
#endif

/// =================================== Surface metalness/roughness/occlusion ===================================

/// @def FillSurfaceMetallicRoughnessOcclusion(surfaceData, rmoMap, rmoTexCoord, occlusionMap, occlusionTexCoord)
/// @brief Fill surface metallness aka reflectivity, roughness and occlusion in SurfaceData.
/// @param[in,optional] rmoMap Properties texture to simultaneously load roughness, metallness and occlusion.
///     Ignored if URHO3D_PHYSICAL_MATERIAL is not defined or URHO3D_MATERIAL_HAS_SPECULAR is not defined.
/// @param[in,optional] rmoTexCoord Texture coordinate for properties map lookup.
///     Ignored if URHO3D_PHYSICAL_MATERIAL is not defined or URHO3D_MATERIAL_HAS_SPECULAR is not defined.
/// @param[in,optional] occlusionMap Occlusion map texture for non-PBR material.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined, URHO3D_PHYSICAL_MATERIAL is defined,
///     AO is not defined or URHO3D_MATERIAL_HAS_EMISSIVE is not defined.
/// @param[in,optional] occlusionTexCoord Texture coordinate for occlusion map lookup.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined, URHO3D_PHYSICAL_MATERIAL is defined,
///     AO is not defined or URHO3D_MATERIAL_HAS_EMISSIVE is not defined.
/// @param[in,optional] surfaceData.normal
/// @param[out] surfaceData.oneMinusReflectivity
/// @param[out] surfaceData.roughness
/// @param[out] surfaceData.occlusion

#if (URHO3D_SPECULAR == 2) && defined(URHO3D_PHYSICAL_MATERIAL)
    half _GetAdjustedSurfaceRoughness(half roughness, half3 normal)
    {
        half3 dNdx = dFdx(normal);
        half3 dNdy = dFdy(normal);
        const half specularAntiAliasingVariance = 0.15;
        const half specularAntiAliasingThreshold = 0.2;
        half variance = specularAntiAliasingVariance * max(dot(dNdx, dNdx), dot(dNdy, dNdy));

        half physicalRoughness = roughness * roughness;
        half kernelRoughness = min(2.0 * variance, specularAntiAliasingThreshold);
        half squareRoughness = min(1.0, physicalRoughness * physicalRoughness + kernelRoughness);
        return sqrt(sqrt(squareRoughness));
    }

    #define _AdjustFragmentRoughness(surfaceData) \
        surfaceData.roughness = _GetAdjustedSurfaceRoughness(surfaceData.roughness, surfaceData.normal)
#else
    #define _AdjustFragmentRoughness(surfaceData)
#endif

#ifdef URHO3D_PHYSICAL_MATERIAL
    void _GetSurfaceMRO(out half oneMinusReflectivity, out half roughness, out half occlusion, half3 rmo)
    {
        const half minRoughness = 0.089;
        half oneMinusDielectricReflectivity = 1.0 - 0.16 * cDielectricReflectance * cDielectricReflectance;

        roughness = max(rmo.x, minRoughness);
        oneMinusReflectivity = oneMinusDielectricReflectivity - oneMinusDielectricReflectivity * rmo.y;
        occlusion = rmo.z;
    }

    #ifdef URHO3D_MATERIAL_HAS_SPECULAR
        half3 _GetBaseRMO(sampler2D propertiesMap, vec2 texCoord)
        {
            half3 rmo = texture(propertiesMap, texCoord).rga;
            rmo.xy *= vec2(cRoughness, cMetallic);
            return rmo;
        }
    #else
        #define _GetBaseRMO(propertiesMap, texCoord) vec3(cRoughness, cMetallic, 1.0)
    #endif

    #define FillSurfaceMetallicRoughnessOcclusion(surfaceData, rmoMap, rmoTexCoord, occlusionMap, occlusionTexCoord) \
    { \
        _GetSurfaceMRO(surfaceData.oneMinusReflectivity, surfaceData.roughness, surfaceData.occlusion, \
            _GetBaseRMO(rmoMap, rmoTexCoord)); \
        _AdjustFragmentRoughness(surfaceData); \
    }
#else
    void _GetSurfaceMR(out half oneMinusReflectivity, out half roughness)
    {
        // Consider non-PBR materials either non-reflective or 100% reflective
    #ifdef ENVCUBEMAP
        oneMinusReflectivity = 0.0;
    #else
        oneMinusReflectivity = 1.0;
    #endif

        roughness = 0.5;
    }

    #if !defined(URHO3D_HAS_LIGHTMAP) && defined(AO) && defined(URHO3D_MATERIAL_HAS_EMISSIVE)
        #define _GetSurfaceOcclusion(occlusionMap, texCoord) texture(occlusionMap, texCoord).r
    #else
        #define _GetSurfaceOcclusion(occlusionMap, texCoord) 1.0
    #endif

    #define FillSurfaceMetallicRoughnessOcclusion(surfaceData, rmoMap, rmoTexCoord, occlusionMap, occlusionTexCoord) \
    { \
        _GetSurfaceMR(surfaceData.oneMinusReflectivity, surfaceData.roughness); \
        surfaceData.occlusion = _GetSurfaceOcclusion(occlusionMap, occlusionTexCoord); \
        _AdjustFragmentRoughness(surfaceData); \
    }
#endif

/// =================================== Surface albedo and specular ===================================

/// @def CutoutByAlpha(alpha)
/// @brief Discard fragment if alpha is less than cAlphaCutoff and if ALPHAMASK is defined.
/// @param[in] alpha Alpha value to test.

/// @def ModulateAlbedoByVertexColor(albedo, vertexColor)
/// @brief Modulate albedo by vertex color.
/// @param[in,out] albedo Albedo value.
/// @param[in,optional] vertexColor Vertex color.
///     Ignored if URHO3D_PIXEL_NEED_VERTEX_COLOR is not defined.

/// @def DeduceAlbedoSpecularForPBR(albedo, specular, oneMinusReflectivity)
/// @brief Deduce effective albedo and specular for PBR material from base albedo and metallness.
/// @param[in,out] albedo Base albedo as input, effective albedo as output.
/// @param[out] specular Effective specular.
/// @param[in] oneMinusReflectivity Inverse of metallness.

/// @def AdjustAlbedoForPremultiplyAlpha(albedo, oneMinusReflectivity)
/// @brief Adjust albedo (and alpha) value in premultiplied alpha mode.
/// @param[in,out] albedo Albedo value with alpha.
/// @param[in] oneMinusReflectivity Inverse of metallness.

/// @def FillSurfaceBaseAlbedo(surfaceData, albedoColor, vertexColor, albedoMap, albedoTexCoord, colorSpace)
/// @brief Fill surface base albedo value.
///     For PBR material, this is used for both albedo and specular, depending on metallness.
///     For non-PBR material, this is used for diffuse color.
/// @param[in] albedoColor Albedo color in gamma space. If texture is present, it will be modulated by this color.
/// @param[in,optional] vertexColor Vertex color to modulate texture and material.
///     Ignored if URHO3D_PIXEL_NEED_VERTEX_COLOR is not defined.
/// @param[in,optional] albedoMap Albedo map.
///     Ignored if URHO3D_MATERIAL_HAS_DIFFUSE is not defined.
/// @param[in,optional] albedoTexCoord Texture coordinate for albedo map lookup.
///     Ignored if URHO3D_MATERIAL_HAS_DIFFUSE is not defined.
/// @param[in,optional] colorSpace Color space of albedo map. 0 for sRGB, 1 for linear.
///     Ignored if URHO3D_MATERIAL_HAS_DIFFUSE is not defined.
/// @param[out] surfaceData.albedo

/// @def FillSurfaceBaseSpecular(surfaceData, specMap, specTexCoord)
/// @brief Fill surface base specular value.
///     For PBR material, this is no-op because specular PBR workflow is not supported now.
///     For non-PBR material, this is used for specular color.
/// @param[in,optional] specMap Specular map.
///     Ignored if URHO3D_MATERIAL_HAS_SPECULAR is not defined.
/// @param[in,optional] specTexCoord Texture coordinate for specular map lookup.
///     Ignored if URHO3D_MATERIAL_HAS_SPECULAR is not defined.
/// @param[out] surfaceData.specular

/// @def FillSurfaceAlbedoSpecular(surfaceData)
/// @brief Finalize surface albedo and specular value (which should be properly filled).
///     Same as DeduceAlbedoSpecularForPBR and AdjustAlbedoForPremultiplyAlpha together.
/// @param[in,out] surfaceData.albedo
/// @param[in,out] surfaceData.specular
/// @param[in] surfaceData.oneMinusReflectivity

#ifdef ALPHAMASK
    #define CutoutByAlpha(alpha) { if (alpha < cAlphaCutoff) discard; }
#else
    #define CutoutByAlpha(alpha)
#endif

// TODO: Why linear color space?
#ifdef URHO3D_PIXEL_NEED_VERTEX_COLOR
    #define ModulateAlbedoByVertexColor(albedo, vertexColor) \
        albedo *= LinearToLightSpaceAlpha(vertexColor)
#else
    #define ModulateAlbedoByVertexColor(albedo, vertexColor)
#endif

#ifdef URHO3D_PHYSICAL_MATERIAL
    #define DeduceAlbedoSpecularForPBR(albedo, specular, oneMinusReflectivity) \
    { \
        specular = albedo.rgb * (1.0 - oneMinusReflectivity); \
        albedo.rgb *= oneMinusReflectivity; \
    }
#else
    #define DeduceAlbedoSpecularForPBR(albedo, specular, oneMinusReflectivity)
#endif

#ifdef URHO3D_PREMULTIPLY_ALPHA
    #ifdef URHO3D_PHYSICAL_MATERIAL
        #define AdjustAlbedoForPremultiplyAlpha(albedo, oneMinusReflectivity) \
            albedo = vec4(albedo.rgb * albedo.a, 1.0 - oneMinusReflectivity + albedo.a * oneMinusReflectivity)
    #else
        #define AdjustAlbedoForPremultiplyAlpha(albedo, oneMinusReflectivity) \
            albedo.rgb *= albedo.a
    #endif
#else
    #define AdjustAlbedoForPremultiplyAlpha(albedo, oneMinusReflectivity)
#endif

#ifdef URHO3D_MATERIAL_HAS_DIFFUSE
    #define _FillSurfaceBaseAlbedo(surfaceData, albedoColor, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    { \
        half4 albedoInput = texture(albedoMap, albedoTexCoord); \
        CutoutByAlpha(albedoInput.a); \
        surfaceData.albedo = GammaToLightSpaceAlpha(albedoColor) * Texture_ToLightAlpha_##colorSpace(albedoInput); \
        ModulateAlbedoByVertexColor(surfaceData.albedo, vertexColor); \
    }
#else
    #define _FillSurfaceBaseAlbedo(surfaceData, albedoColor, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    { \
        surfaceData.albedo = GammaToLightSpaceAlpha(albedoColor); \
        ModulateAlbedoByVertexColor(surfaceData.albedo, vertexColor); \
    }
#endif

// Force macro expansion for colorSpace.
#define FillSurfaceBaseAlbedo(surfaceData, albedoColor, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    _FillSurfaceBaseAlbedo(surfaceData, albedoColor, vertexColor, albedoMap, albedoTexCoord, colorSpace)

#ifdef URHO3D_PHYSICAL_MATERIAL
    // Specular workflow is not supported for PBR materials.
    #define FillSurfaceBaseSpecular(surfaceData, specMap, specTexCoord) \
        surfaceData.specular = vec3(0.0, 0.0, 0.0)
#else
    #ifdef URHO3D_MATERIAL_HAS_SPECULAR
        #define FillSurfaceBaseSpecular(surfaceData, specMap, specTexCoord) \
            surfaceData.specular = GammaToLightSpace(cMatSpecColor.rgb * texture(specMap, specTexCoord).rgb)
    #else
        #define FillSurfaceBaseSpecular(surfaceData, specMap, specTexCoord) \
            surfaceData.specular = GammaToLightSpace(cMatSpecColor.rgb)
    #endif
#endif

#define FillSurfaceAlbedoSpecular(surfaceData) \
{ \
    DeduceAlbedoSpecularForPBR(surfaceData.albedo, surfaceData.specular, surfaceData.oneMinusReflectivity); \
    AdjustAlbedoForPremultiplyAlpha(surfaceData.albedo, surfaceData.oneMinusReflectivity); \
}

/// =================================== Surface emission ===================================

/// @def FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace)
/// @brief Fill surface emission value.
/// @param[in,optional] emissiveMap Emissive map.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_HAS_EMISSIVE is not defined,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[in,optional] texCoord Texture coordinate for emissive map lookup.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_HAS_EMISSIVE is not defined,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[in,optional] colorSpace Color space of emissive map. 0 for sRGB, 1 for linear.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_HAS_EMISSIVE is not defined,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[out] surfaceData.emission

#ifdef URHO3D_SURFACE_NEED_AMBIENT
    #ifndef URHO3D_HAS_LIGHTMAP
        #if defined(URHO3D_MATERIAL_HAS_EMISSIVE) && !defined(AO)
            #define _FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace) \
                surfaceData.emission = GammaToLightSpace(cMatEmissiveColor) * Texture_ToLight_##colorSpace(texture(emissiveMap, texCoord).rgb)
        #else
            #define _FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace) \
                surfaceData.emission = GammaToLightSpace(cMatEmissiveColor)
        #endif
    #else
        #define _FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace) \
            surfaceData.emission = vec3(0.0)
    #endif
#else
    #define _FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace)
#endif

// Force macro expansion for colorSpace.
#define FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace) \
    _FillSurfaceEmission(surfaceData, emissiveMap, texCoord, colorSpace)

/// =================================== Surface reflection color ===================================

/// @def FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos)
/// @brief Fill surface reflection color(s) from cubemap.
/// @param[in] refMap0 Primary reflection map.
/// @param[in,optional] refMap1 Secondary reflection map.
///     Ignored if URHO3D_SURFACE_NEED_REFLECTION_COLOR or URHO3D_BLEND_REFLECTIONS are not defined.
/// @param[in,optional] reflectionVec Crude approximaton of reflection vector provided by vertex shader.
///     It may cause better performance on mobile devices at the cost of reflection quality.
///     Ignored unless URHO3D_VERTEX_REFLECTION is defined.
/// @param[in,optional] worldPos World position of the surface.
///     Ignored unless URHO3D_BOX_PROJECTION is defined.
/// @param[in,optional] surfaceData.screenPos
///     Ignored unless URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT is defined.
/// @param[in,optional] surfaceData.normal
///     Ignored if URHO3D_VERTEX_REFLECTION is defined.
/// @param[in,optional] surfaceData.eyeVec
///     Ignored if URHO3D_VERTEX_REFLECTION is defined.
/// @param[in,optional] surfaceData.roughness
///     Ignored unless URHO3D_BLUR_REFLECTION is defined.
/// @param[out] surfaceData.reflectionColor

/// @def FillSurfacePlanarReflection(surfaceData, refMap, planeX, planeY)
/// @brief Fill surface reflection color from flat texture.
/// @param[in] refMap Reflection map.
/// @param[in] surfaceData.screenPos
/// @param[in] surfaceData.normal
/// @param[out] surfaceData.reflectionColor

#ifdef URHO3D_BOX_PROJECTION
    half3 ApplyBoxProjection(half3 reflectionVec, vec3 worldPos, vec4 cubemapCenter, vec3 boxMin, vec3 boxMax)
    {
        if (cubemapCenter.w > 0.0)
        {
            // Compute intersection distance to the projection box
            vec3 maxIntersection = (boxMax - worldPos) / reflectionVec;
            vec3 minIntersection = (boxMin - worldPos) / reflectionVec;
            vec3 intersection = max(maxIntersection, minIntersection);
            float intersectionDistance = min(min(intersection.x, intersection.y), intersection.z);

            // Recalculate reflection vector
            vec3 intersectWorldPos = worldPos + reflectionVec * intersectionDistance;
            reflectionVec = intersectWorldPos - cubemapCenter.xyz;
        }
        return reflectionVec;
    }

    #define AdjustReflectionVector(reflectionVector, worldPos, center, boxMin, boxMax) \
        reflectionVector = ApplyBoxProjection(reflectionVector, worldPos, center, boxMin.xyz, boxMax.xyz)
#else
    #define AdjustReflectionVector(reflectionVector, worldPos, center, boxMin, boxMax)
#endif

#ifdef URHO3D_SURFACE_NEED_REFLECTION_COLOR
    #if (defined(URHO3D_REFLECTION_MAPPING) || defined(URHO3D_PHYSICAL_MATERIAL)) \
        && defined(URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT)

        /// Planar reflections don't support reflection blending
        #define FillSurfacePlanarReflection(surfaceData, refMap, planeX, planeY) \
            surfaceData.reflectionColor[0] = texture(refMap, \
                GetPlanarReflectionUV(surfaceData.screenPos, vec4(surfaceData.normal, 1.0), planeX, planeY));

    #elif defined(URHO3D_VERTEX_REFLECTION)

        #ifdef URHO3D_BLEND_REFLECTIONS
            #define FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
            { \
                surfaceData.reflectionColor[0] = texture(refMap0, reflectionVec); \
                surfaceData.reflectionColor[1] = texture(refMap1, reflectionVec); \
            }
        #else
            #define FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
                surfaceData.reflectionColor[0] = texture(refMap0, reflectionVec);
        #endif

    #else

        /// Best quality reflections with optional LOD sampling
        half4 _SampleReflection(in samplerCube source, \
            half3 reflectionVec, half roughness, half roughnessFactor)
        {
        #ifdef URHO3D_BLUR_REFLECTION
            return textureLod(source, reflectionVec, roughness * roughnessFactor);
        #else
            return texture(source, reflectionVec);
        #endif
        }

        #ifdef URHO3D_BLEND_REFLECTIONS
            #define FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
            { \
                half3 refVec0 = reflect(-surfaceData.eyeVec, surfaceData.normal); \
                half3 refVec1 = refVec0; \
                AdjustReflectionVector(refVec1, worldPos, cCubemapCenter1, cProjectionBoxMin1, cProjectionBoxMax1); \
                surfaceData.reflectionColor[1] = \
                    _SampleReflection(refMap1, refVec1, surfaceData.roughness, cRoughnessToLODFactor1); \
                AdjustReflectionVector(refVec0, worldPos, cCubemapCenter0, cProjectionBoxMin0, cProjectionBoxMax0); \
                surfaceData.reflectionColor[0] = \
                    _SampleReflection(refMap0, refVec0, surfaceData.roughness, cRoughnessToLODFactor0); \
            }
        #else
            #define FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
            { \
                half3 refVec0 = reflect(-surfaceData.eyeVec, surfaceData.normal); \
                AdjustReflectionVector(refVec0, worldPos, cCubemapCenter0, cProjectionBoxMin0, cProjectionBoxMax0); \
                surfaceData.reflectionColor[0] = \
                    _SampleReflection(refMap0, refVec0, surfaceData.roughness, cRoughnessToLODFactor0); \
            }
        #endif

    #endif
#endif

#ifndef FillSurfaceCubeReflection
    #define FillSurfaceCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos)
#endif

#ifndef FillSurfacePlanarReflection
    #define FillSurfacePlanarReflection(surfaceData, refMap, planeX, planeY)
#endif

/// =================================== Surface background color and depth ===================================

/// @def FillSurfaceBackgroundColor(surfaceData, colorMap)
/// @brief Fill corresponding background color.
/// @param[in] colorMap Background color map.
/// @param[in] surfaceData.screenPos
/// @param[out] surfaceData.backgroundColor

/// @def FillSurfaceBackgroundDepth(surfaceData, depthMap)
/// @brief Fill corresponding background depth.
/// @param[in] depthMap Background depth map.
/// @param[in] surfaceData.screenPos
/// @param[out] surfaceData.backgroundDepth

/// @def FillSurfaceBackground(surfaceData, colorMap, depthMap)
/// @brief Same as FillSurfaceBackgroundColor and FillSurfaceBackgroundDepth.

#ifdef URHO3D_SURFACE_NEED_BACKGROUND_COLOR
    #define FillSurfaceBackgroundColor(surfaceData, colorMap) \
        surfaceData.backgroundColor = texture(colorMap, surfaceData.screenPos).rgb
#else
    #define FillSurfaceBackgroundColor(surfaceData, colorMap)
#endif

#ifdef URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
    #define FillSurfaceBackgroundDepth(surfaceData, depthMap) \
        surfaceData.backgroundDepth = ReconstructDepth(texture(depthMap, surfaceData.screenPos).r)
#else
    #define FillSurfaceBackgroundDepth(surfaceData, depthMap)
#endif

#define FillSurfaceBackground(surfaceData, colorMap, depthMap) \
{ \
    FillSurfaceBackgroundColor(surfaceData, colorMap); \
    FillSurfaceBackgroundDepth(surfaceData, depthMap); \
}

