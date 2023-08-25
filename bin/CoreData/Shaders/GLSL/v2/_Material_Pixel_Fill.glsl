/// _Material_Pixel_Fill.glsl
/// Don't include!
/// Fill SurfaceData structure.

#ifndef URHO3D_MATERIAL_ALBEDO
    #define URHO3D_MATERIAL_ALBEDO 0
#endif
#ifndef URHO3D_MATERIAL_NORMAL
    #define URHO3D_MATERIAL_NORMAL 0
#endif
#ifndef URHO3D_MATERIAL_PROPERTIES
    #define URHO3D_MATERIAL_PROPERTIES 0
#endif
#ifndef URHO3D_MATERIAL_EMISSION
    #define URHO3D_MATERIAL_EMISSION 0
#endif

/// =================================== Non-customizable properties ===================================

/// @def Surface_SetFogFactor(surfaceData)
/// @brief Fill fog factor in SurfaceData.
/// @param[out] surfaceData.fogFactor

/// @def Surface_SetEyeVector(surfaceData)
/// @brief Fill eye vector in SurfaceData.
/// @param[out] surfaceData.eyeVec

/// @def Surface_SetScreenPosition(surfaceData)
/// @brief Fill screen position in SurfaceData.
/// @param[out] surfaceData.screenPos

/// @def Surface_SetCommon(surfaceData)
/// @brief Same as Surface_SetFogFactor, Surface_SetEyeVector, Surface_SetScreenPosition together.

#define Surface_SetFogFactor(surfaceData) \
    surfaceData.fogFactor = GetFogFactor(vWorldDepth)

#ifdef URHO3D_PIXEL_NEED_EYE_VECTOR
    #define Surface_SetEyeVector(surfaceData) \
        surfaceData.eyeVec = normalize(vEyeVec)
#else
    #define Surface_SetEyeVector(surfaceData)
#endif

#ifdef URHO3D_PIXEL_NEED_SCREEN_POSITION
    #define Surface_SetScreenPosition(surfaceData) \
        surfaceData.screenPos = vScreenPos.xy / vScreenPos.w
#else
    #define Surface_SetScreenPosition(surfaceData)
#endif

#define Surface_SetCommon(surfaceData) \
{ \
    Surface_SetFogFactor(surfaceData); \
    Surface_SetEyeVector(surfaceData); \
    Surface_SetScreenPosition(surfaceData); \
}

/// =================================== Ambient lighting ===================================

/// @def Surface_SetAmbient(surfaceData, lightMap, texCoord)
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

        #define Surface_SetAmbient(surfaceData, lightMap, texCoord) \
            surfaceData.ambientLighting = vAmbientAndVertexLigthing + _GetSurfaceLightmap(lightMap, texCoord)
    #else
        #define Surface_SetAmbient(surfaceData, lightMap, texCoord) \
            surfaceData.ambientLighting = vAmbientAndVertexLigthing
    #endif
#else
    #define Surface_SetAmbient(surfaceData, lightMap, texCoord)
#endif

/// =================================== Surface normal ===================================

/// @def Surface_SetNormal(surfaceData, normal, normalMap, texCoord, tangent, bitangentXY)
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

        #define Surface_SetNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY) \
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

        #define Surface_SetNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY) \
            surfaceData.normal = _GetSurfaceNormal(surfaceData.normalInTangentSpace, vertexNormal)
    #endif
#else
    #define Surface_SetNormal(surfaceData, vertexNormal, normalMap, texCoord, vertexTangent, vertexBitangentXY)
#endif

/// =================================== Surface metalness/roughness/occlusion ===================================

/// @def Surface_SetPhysicalProperties(surfaceData, roughnessValue, metalnessValue, dielectricReflectance, rmoMap, rmoTexCoord)
/// @brief Fill surface metalness aka reflectivity, roughness and occlusion in SurfaceData.
/// @param[in] roughnessValue Roughness value. If texture is used, this value is multiplied by texture value.
/// @param[in] metalnessValue Metalness value. If texture is used, this value is multiplied by texture value.
/// @param[in] dielectricReflectance Reflectance value used for dielectrics. It cannot be sampled from texture.
/// @param[in,optional] rmoMap Properties texture to simultaneously load roughness, metalness and occlusion.
///     Ignored if URHO3D_PHYSICAL_MATERIAL is not defined or URHO3D_MATERIAL_PROPERTIES is 0.
/// @param[in,optional] rmoTexCoord Texture coordinate for properties map lookup.
///     Ignored if URHO3D_PHYSICAL_MATERIAL is not defined or URHO3D_MATERIAL_PROPERTIES is 0.
/// @param[in,optional] surfaceData.normal
///     Ignored unless high-quality specular is enabled (URHO3D_SPECULAR == 2).
/// @param[out] surfaceData.oneMinusReflectivity
/// @param[out] surfaceData.roughness
/// @param[out] surfaceData.occlusion

/// @def Surface_SetLegacyProperties(surfaceData, specularPower, occlusionMap, occlusionTexCoord)
/// @brief Fill surface metalness, roughness and occlusion approximately for legacy materials.
/// @param[in] specularPower Specular power value.
/// @param[in,optional] occlusionMap Occlusion map texture for non-PBR material.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined,
///     AO is not defined or URHO3D_MATERIAL_EMISSION is 0.
/// @param[in,optional] occlusionTexCoord Texture coordinate for occlusion map lookup.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined,
///     AO is not defined or URHO3D_MATERIAL_EMISSION is 0.
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

    #define _Surface_AdjustRoughness(surfaceData) \
        surfaceData.roughness = _GetAdjustedSurfaceRoughness(surfaceData.roughness, surfaceData.normal)
#else
    #define _Surface_AdjustRoughness(surfaceData)
#endif

#ifdef URHO3D_PHYSICAL_MATERIAL
    void _GetSurfaceRMO(out half oneMinusReflectivity, out half roughness, out half occlusion,
        half3 rmo, half dielectricReflectance)
    {
        const half minRoughness = 0.089;
        half oneMinusDielectricReflectivity = 1.0 - 0.16 * dielectricReflectance * dielectricReflectance;

        roughness = max(rmo.x, minRoughness);
        oneMinusReflectivity = oneMinusDielectricReflectivity - oneMinusDielectricReflectivity * rmo.y;
        occlusion = rmo.z;
    }

    #if URHO3D_MATERIAL_PROPERTIES
        half3 _GetBaseRMO(sampler2D propertiesMap, vec2 texCoord, half roughnessValue, half metalnessValue)
        {
            half3 rmo = texture(propertiesMap, texCoord).rga;
            rmo.xy *= vec2(roughnessValue, metalnessValue);
            return rmo;
        }
    #else
        #define _GetBaseRMO(propertiesMap, texCoord, roughnessValue, metalnessValue) \
            vec3(roughnessValue, metalnessValue, 1.0)
    #endif

    #define Surface_SetPhysicalProperties(surfaceData, roughnessValue, metalnessValue, dielectricReflectance, rmoMap, rmoTexCoord) \
    { \
        _GetSurfaceRMO(surfaceData.oneMinusReflectivity, surfaceData.roughness, surfaceData.occlusion, \
            _GetBaseRMO(rmoMap, rmoTexCoord, roughnessValue, metalnessValue), dielectricReflectance); \
        _Surface_AdjustRoughness(surfaceData); \
    }
#else
    void _GetSurfaceMR(out half oneMinusReflectivity, out half roughness, half specularPower)
    {
        // Consider non-PBR materials either non-reflective or 100% reflective
    #ifdef ENVCUBEMAP
        oneMinusReflectivity = 0.0;
    #else
        oneMinusReflectivity = 1.0;
    #endif

        roughness = SpecularPowerToRoughness(specularPower);
    }

    #if !defined(URHO3D_HAS_LIGHTMAP) && defined(AO) && URHO3D_MATERIAL_EMISSION
        #define _GetSurfaceOcclusion(occlusionMap, texCoord) texture(occlusionMap, texCoord).r
    #else
        #define _GetSurfaceOcclusion(occlusionMap, texCoord) 1.0
    #endif

    #define Surface_SetLegacyProperties(surfaceData, specularPower, occlusionMap, occlusionTexCoord) \
    { \
        _GetSurfaceMR(surfaceData.oneMinusReflectivity, surfaceData.roughness, specularPower); \
        surfaceData.occlusion = _GetSurfaceOcclusion(occlusionMap, occlusionTexCoord); \
        _Surface_AdjustRoughness(surfaceData); \
    }
#endif

#ifndef Surface_SetPhysicalProperties
    #define Surface_SetPhysicalProperties(surfaceData, roughnessValue, metalnessValue, dielectricReflectance, rmoMap, rmoTexCoord)
#endif

#ifndef Surface_SetLegacyProperties
    #define Surface_SetLegacyProperties(surfaceData, specularPower, occlusionMap, occlusionTexCoord)
#endif

/// =================================== Surface albedo and specular ===================================

/// @def CutoutByAlpha(alpha, cutoff)
/// @brief Discard fragment if alpha is less than cutoff value and if ALPHAMASK is defined.
/// @param[in] alpha Alpha value to test.
/// @param[in] cutoff Alpha cutoff value.

/// @def ModulateAlbedoByVertexColor(albedo, vertexColor)
/// @brief Modulate albedo by vertex color.
/// @param[in,out] albedo Albedo value.
/// @param[in,optional] vertexColor Vertex color.
///     Ignored if URHO3D_PIXEL_NEED_VERTEX_COLOR is not defined.

/// @def DeduceAlbedoSpecularForPBR(albedo, specular, oneMinusReflectivity)
/// @brief Deduce effective albedo and specular for PBR material from base albedo and metalness.
/// @param[in,out] albedo Base albedo as input, effective albedo as output.
/// @param[out] specular Effective specular.
/// @param[in] oneMinusReflectivity Inverse of metalness.

/// @def AdjustAlbedoForPremultiplyAlpha(albedo, oneMinusReflectivity)
/// @brief Adjust albedo (and alpha) value in premultiplied alpha mode.
/// @param[in,out] albedo Albedo value with alpha.
/// @param[in] oneMinusReflectivity Inverse of metalness.

/// @def Surface_SetBaseAlbedo(
///     surfaceData, albedoColor, alphaCutoff, vertexColor, albedoMap, albedoTexCoord, colorSpace)
/// @brief Fill surface base albedo value.
///     For PBR material, this is used for both albedo and specular, depending on metalness.
///     For non-PBR material, this is used for diffuse color.
/// @param[in] albedoColor Albedo color in gamma space. If texture is present, it will be modulated by this color.
/// @param[in] alphaCutoff Alpha cutoff value.
///     Ignored if ALPHAMASK is not defined.
/// @param[in,optional] vertexColor Vertex color to modulate texture and material.
///     Ignored if URHO3D_PIXEL_NEED_VERTEX_COLOR is not defined.
/// @param[in,optional] albedoMap Albedo map.
///     Ignored if URHO3D_MATERIAL_ALBEDO is 0.
/// @param[in,optional] albedoTexCoord Texture coordinate for albedo map lookup.
///     Ignored if URHO3D_MATERIAL_ALBEDO is 0.
/// @param[in,optional] colorSpace Color space of albedo map. 0 for gamma, 1 for linear.
///     Ignored if URHO3D_MATERIAL_ALBEDO is 0.
/// @param[out] surfaceData.albedo

/// @def Surface_SetBaseSpecular(surfaceData, specColor, reflectionColor, specMap, specTexCoord)
/// @brief Fill surface base specular value.
///     For PBR material, this is no-op because specular PBR workflow is not supported now.
///     For non-PBR material, this is used for specular color.
/// @param[in] specColor Specular color in gamma space.
/// @param[in] reflectionColor Reflection color in gamma space.
/// @param[in,optional] specMap Specular map in gamma space.
///     Ignored if URHO3D_MATERIAL_PROPERTIES is 0.
/// @param[in,optional] specTexCoord Texture coordinate for specular map lookup.
///     Ignored if URHO3D_MATERIAL_PROPERTIES is 0.
/// @param[out] surfaceData.specular

/// @def Surface_SetAlbedoSpecular(surfaceData)
/// @brief Finalize surface albedo and specular value (which should be properly filled).
///     Same as DeduceAlbedoSpecularForPBR and AdjustAlbedoForPremultiplyAlpha together.
/// @param[in,out] surfaceData.albedo
/// @param[in,out] surfaceData.specular
/// @param[in] surfaceData.oneMinusReflectivity

#ifdef ALPHAMASK
    #define CutoutByAlpha(alpha, cutoff) { if (alpha < cutoff) discard; }
#else
    #define CutoutByAlpha(alpha, cutoff)
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

#if URHO3D_MATERIAL_ALBEDO
    #define _Surface_SetBaseAlbedo(surfaceData, albedoColor, alphaCutoff, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    { \
        half4 albedoInput = texture(albedoMap, albedoTexCoord); \
        CutoutByAlpha(albedoInput.a, alphaCutoff); \
        surfaceData.albedo = (colorSpace == 1 ? Texture_ToLightAlpha_1(albedoInput) : Texture_ToLightAlpha_2(albedoInput)) \
            * GammaToLightSpaceAlpha(albedoColor); \
        ModulateAlbedoByVertexColor(surfaceData.albedo, vertexColor); \
    }
#else
    #define _Surface_SetBaseAlbedo(surfaceData, albedoColor, alphaCutoff, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    { \
        surfaceData.albedo = GammaToLightSpaceAlpha(albedoColor); \
        ModulateAlbedoByVertexColor(surfaceData.albedo, vertexColor); \
    }
#endif

// Force macro expansion for colorSpace.
#define Surface_SetBaseAlbedo(surfaceData, albedoColor, alphaCutoff, vertexColor, albedoMap, albedoTexCoord, colorSpace) \
    _Surface_SetBaseAlbedo(surfaceData, albedoColor, alphaCutoff, vertexColor, albedoMap, albedoTexCoord, colorSpace)

#ifdef URHO3D_PHYSICAL_MATERIAL
    // Specular workflow is not supported for PBR materials.
    #define Surface_SetBaseSpecular(surfaceData, specColor, reflectionColor, specMap, specTexCoord) \
        surfaceData.specular = vec3(0.0, 0.0, 0.0)
#else
    #if URHO3D_MATERIAL_PROPERTIES
        #define _Surface_SetBaseSpecular(surfaceData, specColor, specMap, specTexCoord) \
            surfaceData.specular = GammaToLightSpace(specColor.rgb * texture(specMap, specTexCoord).rgb)
    #else
        #define _Surface_SetBaseSpecular(surfaceData, specColor, specMap, specTexCoord) \
            surfaceData.specular = GammaToLightSpace(specColor.rgb)
    #endif

    #ifdef URHO3D_REFLECTION_MAPPING
        #define _Surface_SetReflectionTint(surfaceData, value) \
            surfaceData.reflectionTint = value
    #else
        #define _Surface_SetReflectionTint(surfaceData, value)
    #endif

    #define Surface_SetBaseSpecular(surfaceData, specColor, reflectionColor, specMap, specTexCoord) \
    { \
        _Surface_SetBaseSpecular(surfaceData, specColor, specMap, specTexCoord); \
        _Surface_SetReflectionTint(surfaceData, reflectionColor); \
    }
#endif

#define Surface_SetAlbedoSpecular(surfaceData) \
{ \
    DeduceAlbedoSpecularForPBR(surfaceData.albedo, surfaceData.specular, surfaceData.oneMinusReflectivity); \
    AdjustAlbedoForPremultiplyAlpha(surfaceData.albedo, surfaceData.oneMinusReflectivity); \
}

/// =================================== Surface emission ===================================

/// @def Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace)
/// @brief Fill surface emission value.
/// @param[in] emissiveColor Emissive color in gamma space. If texture is present, it will be modulated by this color.
/// @param[in,optional] emissiveMap Emissive map.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_EMISSION is 0,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[in,optional] texCoord Texture coordinate for emissive map lookup.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_EMISSION is 0,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[in,optional] colorSpace Color space of emissive map. 0 for gamma, 1 for linear.
///     Ignored if URHO3D_SURFACE_NEED_AMBIENT is not defined, URHO3D_MATERIAL_EMISSION is 0,
///     AO is defined, or URHO3D_HAS_LIGHTMAP is defined.
/// @param[out] surfaceData.emission

#ifdef URHO3D_SURFACE_NEED_AMBIENT
    #ifndef URHO3D_HAS_LIGHTMAP
        #if URHO3D_MATERIAL_EMISSION && !defined(AO)
            #define _Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace) \
            { \
                half3 emissionInput = texture(emissiveMap, texCoord).rgb; \
                surfaceData.emission = (colorSpace == 1 ? Texture_ToLight_1(emissionInput) : Texture_ToLight_2(emissionInput)) \
                    * GammaToLightSpace(emissiveColor); \
            }
        #else
            #define _Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace) \
                surfaceData.emission = GammaToLightSpace(emissiveColor)
        #endif
    #else
        #define _Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace) \
            surfaceData.emission = vec3(0.0)
    #endif
#else
    #define _Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace)
#endif

// Force macro expansion for colorSpace.
#define Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace) \
    _Surface_SetEmission(surfaceData, emissiveColor, emissiveMap, texCoord, colorSpace)

/// =================================== Surface reflection color ===================================

/// @def Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos)
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

/// @def Surface_SetPlanarReflection(surfaceData, refMap, planeX, planeY)
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
        #define Surface_SetPlanarReflection(surfaceData, refMap, planeX, planeY) \
            surfaceData.reflectionColor[0] = texture(refMap, \
                GetPlanarReflectionUV(surfaceData.screenPos, vec4(surfaceData.normal, 1.0), planeX, planeY));

    #elif defined(URHO3D_VERTEX_REFLECTION)

        #ifdef URHO3D_BLEND_REFLECTIONS
            #define Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
            { \
                surfaceData.reflectionColor[0] = texture(refMap0, reflectionVec); \
                surfaceData.reflectionColor[1] = texture(refMap1, reflectionVec); \
            }
        #else
            #define Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
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
            #define Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
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
            #define Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos) \
            { \
                half3 refVec0 = reflect(-surfaceData.eyeVec, surfaceData.normal); \
                AdjustReflectionVector(refVec0, worldPos, cCubemapCenter0, cProjectionBoxMin0, cProjectionBoxMax0); \
                surfaceData.reflectionColor[0] = \
                    _SampleReflection(refMap0, refVec0, surfaceData.roughness, cRoughnessToLODFactor0); \
            }
        #endif

    #endif
#endif

#ifndef Surface_SetCubeReflection
    #define Surface_SetCubeReflection(surfaceData, refMap0, refMap1, reflectionVec, worldPos)
#endif

#ifndef Surface_SetPlanarReflection
    #define Surface_SetPlanarReflection(surfaceData, refMap, planeX, planeY)
#endif

/// =================================== Surface background color and depth ===================================

/// @def Surface_SetBackgroundColor(surfaceData, colorMap)
/// @brief Fill corresponding background color.
/// @param[in] colorMap Background color map.
/// @param[in] surfaceData.screenPos
/// @param[out] surfaceData.backgroundColor

/// @def Surface_SetBackgroundDepth(surfaceData, depthMap)
/// @brief Fill corresponding background depth.
/// @param[in] depthMap Background depth map.
/// @param[in] surfaceData.screenPos
/// @param[out] surfaceData.backgroundDepth

/// @def Surface_SetBackground(surfaceData, colorMap, depthMap)
/// @brief Same as Surface_SetBackgroundColor and Surface_SetBackgroundDepth.

#ifdef URHO3D_SURFACE_NEED_BACKGROUND_COLOR
    #define Surface_SetBackgroundColor(surfaceData, colorMap) \
        surfaceData.backgroundColor = texture(colorMap, surfaceData.screenPos).rgb
#else
    #define Surface_SetBackgroundColor(surfaceData, colorMap)
#endif

#ifdef URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
    #define Surface_SetBackgroundDepth(surfaceData, depthMap) \
        surfaceData.backgroundDepth = ReconstructDepth(texture(depthMap, surfaceData.screenPos).r)
#else
    #define Surface_SetBackgroundDepth(surfaceData, depthMap)
#endif

#define Surface_SetBackground(surfaceData, colorMap, depthMap) \
{ \
    Surface_SetBackgroundColor(surfaceData, colorMap); \
    Surface_SetBackgroundDepth(surfaceData, depthMap); \
}

/// =================================== Surface soft fade out ===================================

/// @def Surface_ApplySoftFadeOut(surfaceData, worldDepth, fadeOffsetScale)
/// @brief Apply soft fade out to the surface alpha.
/// @param[in] worldDepth Depth of the surface in world space units.
/// @param[in] fadeOffsetScale Constant offset and scale of the fade out.
/// @param[in] surfaceData.backgroundDepth
/// @param[in,out] surfaceData.albedo.a

half GetSoftParticleFade(half fragmentDepth, half backgroundDepth, half2 fadeOffsetScale)
{
    half depthDelta = backgroundDepth - fragmentDepth - fadeOffsetScale.x;
    return clamp(depthDelta * fadeOffsetScale.y, 0.0, 1.0);
}

#ifdef URHO3D_SOFT_PARTICLES
    #define Surface_ApplySoftFadeOut(surfaceData, worldDepth, fadeOffsetScale) \
        surfaceData.albedo.a *= GetSoftParticleFade(worldDepth, surfaceData.backgroundDepth, fadeOffsetScale)
#else
    #define Surface_ApplySoftFadeOut(surfaceData, worldDepth, fadeOffsetScale)
#endif
