/// _Material_Pixel_Fill.glsl
/// Don't include!
/// Fill SurfaceData structure.

/// =================================== Non-customizable properties ===================================

/// @def FillSurfaceFogFactor(surfaceData)
/// @brief Fill fog factor in SurfaceData.
/// @param[out] surfaceData.fogFactor

/// @def FillSurfaceEyeVector(surfaceData)
/// @brief Fill eye vector in SurfaceData.
/// @param[out,optional] surfaceData.eyeVec

/// @def FillSurfaceScreenPosition(surfaceData)
/// @brief Fill screen position in SurfaceData.
/// @param[out,optional] surfaceData.screenPos

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
/// @param[in,optional] lightMap Lightmap texture. Ignored if URHO3D_HAS_LIGHTMAP is not defined.
/// @param[in,optional] texCoord Texture coordinate for lightmap lookup. Ignored if URHO3D_HAS_LIGHTMAP is not defined.
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
/// @param[in,optional] normalMap Normal map texture. Ignored if URHO3D_NORMAL_MAPPING is not defined.
/// @param[in,optional] texCoord Texture coordinate for normal map lookup.
/// @param[in,optional] vertexTangent Vertex tangent.
/// @param[in,optional] vertexBitangentXY Vertex bitangent XY.
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
/// @param[in,optional] occlusionMap Occlusion map texture for non-PBR renderer.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined, URHO3D_PHYSICAL_MATERIAL is defined,
///     AO is not defined or URHO3D_MATERIAL_HAS_EMISSIVE is not defined.
/// @param[in,optional] occlusionTexCoord Texture coordinate for occlusion map lookup.
///     Ignored if URHO3D_HAS_LIGHTMAP is defined, URHO3D_PHYSICAL_MATERIAL is defined,
///     AO is not defined or URHO3D_MATERIAL_HAS_EMISSIVE is not defined.
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

/// Fill surface albedo and specular.
/// out: SurfaceData.albedo
/// out: SurfaceData.specular
void _GetFragmentAlbedoSpecular(half oneMinusReflectivity, out half4 albedo, out half3 specular)
{
#ifdef URHO3D_MATERIAL_HAS_DIFFUSE
    half4 albedoInput = texture(sDiffMap, vTexCoord);
    #ifdef ALPHAMASK
        if (albedoInput.a < cAlphaCutoff)
            discard;
    #endif

    albedo = GammaToLightSpaceAlpha(cMatDiffColor) * DiffMap_ToLight(albedoInput);
#else
    albedo = GammaToLightSpaceAlpha(cMatDiffColor);
#endif

#ifdef URHO3D_PIXEL_NEED_VERTEX_COLOR
    albedo *= LinearToLightSpaceAlpha(vColor);
#endif

#ifdef URHO3D_PHYSICAL_MATERIAL
    specular = albedo.rgb * (1.0 - oneMinusReflectivity);
    albedo.rgb *= oneMinusReflectivity;
#else
    #ifdef URHO3D_MATERIAL_HAS_SPECULAR
        specular = GammaToLightSpace(cMatSpecColor.rgb * texture(sSpecMap, vTexCoord).rgb);
    #else
        specular = GammaToLightSpace(cMatSpecColor.rgb);
    #endif
#endif

#ifdef URHO3D_PREMULTIPLY_ALPHA
    albedo.rgb *= albedo.a;
    #ifdef URHO3D_PHYSICAL_MATERIAL
        albedo.a = 1.0 - oneMinusReflectivity + albedo.a * oneMinusReflectivity;
    #endif
#endif
}

#define FillSurfaceAlbedoSpecular(surfaceData) \
    _GetFragmentAlbedoSpecular(surfaceData.oneMinusReflectivity, surfaceData.albedo, surfaceData.specular)

/// Fill surface emission.
/// out: SurfaceData.emission
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    #ifndef URHO3D_HAS_LIGHTMAP
        #if defined(URHO3D_MATERIAL_HAS_EMISSIVE) && !defined(AO)
            #define FillSurfaceEmission(surfaceData) \
                surfaceData.emission = GammaToLightSpace(cMatEmissiveColor) * EmissiveMap_ToLight(texture(sEmissiveMap, vTexCoord).rgb)
        #else
            #define FillSurfaceEmission(surfaceData) \
                surfaceData.emission = GammaToLightSpace(cMatEmissiveColor)
        #endif
    #else
        #define FillSurfaceEmission(surfaceData) \
            surfaceData.emission = vec3(0.0)
    #endif
#else
    #define FillSurfaceEmission(surfaceData)
#endif

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
#endif

/// Fill surface reflection color(s).
/// out: SurfaceData.reflectionColor[]
#ifdef URHO3D_SURFACE_NEED_REFLECTION_COLOR
    #if (defined(URHO3D_REFLECTION_MAPPING) || defined(URHO3D_PHYSICAL_MATERIAL)) && defined(URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT)

        /// Planar reflections don't support reflection blending
        void FillSurfaceReflectionColorPlanar(out half4 reflectionColor[URHO3D_NUM_REFLECTIONS],
            vec2 screenPos, half3 normal)
        {
            reflectionColor[0] = texture(sEnvMap, GetPlanarReflectionUV(screenPos, vec4(normal, 1.0)));
        }

        #define FillSurfaceReflectionColor(surfaceData) \
            FillSurfaceReflectionColorPlanar(surfaceData.reflectionColor, surfaceData.screenPos, surfaceData.normal)

    #elif defined(URHO3D_VERTEX_REFLECTION)

        /// Vertex reflection must use reflection vector from vertex shader
        void FillSurfaceReflectionColorCubeSimple(out half4 reflectionColor[URHO3D_NUM_REFLECTIONS])
        {
            #ifdef URHO3D_BLEND_REFLECTIONS
                reflectionColor[1] = texture(sZoneCubeMap, vReflectionVec);
            #endif
            reflectionColor[0] = texture(sEnvMap, vReflectionVec);
        }

        #define FillSurfaceReflectionColor(surfaceData) \
            FillSurfaceReflectionColorCubeSimple(surfaceData.reflectionColor)

    #else

        /// Best quality reflections with optional LOD sampling
        half4 SampleCubeReflectionColor(in samplerCube source, \
            half3 reflectionVec, half roughness, half roughnessFactor)
        {
        #ifdef URHO3D_BLUR_REFLECTION
            return textureLod(source, reflectionVec, roughness * roughnessFactor);
        #else
            return texture(source, reflectionVec);
        #endif
        }

        void FillSurfaceReflectionColorCube(out half4 reflectionColor[URHO3D_NUM_REFLECTIONS],
            half3 normal, half3 eyeVec, half roughness)
        {
            half3 reflectionVec0 = reflect(-eyeVec, normal);

            #ifdef URHO3D_BLEND_REFLECTIONS
                half3 reflectionVec1 = reflectionVec0;

                #ifdef URHO3D_BOX_PROJECTION
                    reflectionVec1 = ApplyBoxProjection(reflectionVec1, vWorldPos,
                        cCubemapCenter1, cProjectionBoxMin1.xyz, cProjectionBoxMax1.xyz);
                #endif
                reflectionColor[1] = SampleCubeReflectionColor(sZoneCubeMap, reflectionVec1, roughness, cRoughnessToLODFactor1);
            #endif

            #ifdef URHO3D_BOX_PROJECTION
                reflectionVec0 = ApplyBoxProjection(reflectionVec0, vWorldPos,
                    cCubemapCenter0, cProjectionBoxMin0.xyz, cProjectionBoxMax0.xyz);
            #endif
            reflectionColor[0] = SampleCubeReflectionColor(sEnvMap, reflectionVec0, roughness, cRoughnessToLODFactor0);
        }

        #define FillSurfaceReflectionColor(surfaceData) \
            FillSurfaceReflectionColorCube(surfaceData.reflectionColor, \
                surfaceData.normal, surfaceData.eyeVec, surfaceData.roughness)

    #endif
#else
    #define FillSurfaceReflectionColor(surfaceData)
#endif

/// Fill surface background color.
/// out: SurfaceData.backgroundColor
#ifdef URHO3D_SURFACE_NEED_BACKGROUND_COLOR
    #define FillSurfaceBackgroundColor(surfaceData) \
        surfaceData.backgroundColor = texture(sEmissiveMap, surfaceData.screenPos).rgb
#else
    #define FillSurfaceBackgroundColor(surfaceData)
#endif

/// Fill surface background depth.
/// out: SurfaceData.backgroundDepth
#ifdef URHO3D_SURFACE_NEED_BACKGROUND_DEPTH
    #define FillSurfaceBackgroundDepth(surfaceData) \
        surfaceData.backgroundDepth = ReconstructDepth(texture(sDepthBuffer, surfaceData.screenPos).r)
#else
    #define FillSurfaceBackgroundDepth(surfaceData)
#endif

/// Fill surface background from samplers.
#define FillSurfaceBackground(surfaceData) \
    FillSurfaceBackgroundColor(surfaceData); \
    FillSurfaceBackgroundDepth(surfaceData)

