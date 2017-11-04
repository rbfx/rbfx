#ifdef COMPILEPS

    float2 hammersley(uint i, uint N)
    {
        return float2(float(i) / float(N), float(reversebits(i)) * 2.3283064365386963e-10);
    }

     float3 ImportanceSampleSimple(in float2 Xi, in float roughness, in float3 T, in float3 B, in float3 N)
    {
        const float a = roughness * roughness;
        const float3x3 tbn = float3x3(T, B, N);
     //   #if defined(PBRFAST)
         //   const float blurFactor = 0.0;
       // #else
            const float blurFactor = 5.0;
        //#endif
        const float3 Xi3 = lerp(float3(0,0,1), normalize(float3(Xi.xy * blurFactor , 1)), a);
        const float3 XiWS = mul(Xi3, tbn);
        return normalize(N + XiWS);
    }

    // Karis '13
    float3 ImportanceSampleGGX(in float2 Xi, in float roughness, in float3 N)
    {
         float r_square = roughness * roughness;
        float phi = M_PI * Xi.x;
        float cos_theta_sq = (1 - Xi.y) / max(1e-3, 1 + (r_square * r_square - 1) * Xi.y);
        float cos_theta = sqrt(cos_theta_sq);
        float sin_theta = sqrt(max(0.0, 1.0 - cos_theta_sq));
        return float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
    }

    #define IMPORTANCE_SAMPLES 64

    float GetMipFromRougness(float roughness)
    {
        const float smoothness = 1.0 - roughness;
        return (1.0 - smoothness * smoothness) * 10.0;
    }

    /// Perform importance sampling
    ///     reflectVec: calculated vector of reflection
    ///     wsNormal: world-space normal of the surface
    ///     toCamera: direction from the pixel to the camera
    ///     specular: specular color
    ///     roughness: surface roughness
    ///     reflectionCubeColor: output color for diffuse

    // Implementation based on Epics 2013 course notes
    float3 ImportanceSampling(in float3 reflectVec, in float3 tangent, in float3 bitangent, in float3 wsNormal, in float3 toCamera,  in float3 diffColor, in float3 specColor, in float roughness, inout float3 reflectionCubeColor)
    {
        reflectionCubeColor = 1.0;

        const float3 reflectSpec = normalize(GetSpecularDominantDir(wsNormal, reflectVec, roughness));

        const float3 V = normalize(-toCamera);
        const float3 N = normalize(wsNormal);
        const float ndv = saturate(abs(dot(N, V)));

        const float specMipLevel = GetMipFromRougness(roughness);

        float3 accumulatedColor = float3(0,0,0);
        for (int i = 0; i < IMPORTANCE_SAMPLES; ++i)
        {
            float3 kd = 1.0;
            float3 diffuseFactor = 0.0;
            float3 specularFactor = 0.0;

            {
                // Diffuse IBL
                const float rough = 1.0;
                const float mipLevel = 9.0;

                const float3 H = ImportanceSampleSimple(hammersley(i, IMPORTANCE_SAMPLES), rough, tangent, bitangent, N);
                const float3 L = 2.0 * dot( V, H ) * H - V;

                const float vdh = saturate(abs(dot(V, H)));
                const float ndh = saturate(abs(dot(N, H)));
                const float ndl = saturate(abs(dot(N, L)));

                if (ndl > 0.0)
                {
                    const float3 sampledColor = SampleCubeLOD(ZoneCubeMap, float4(L, mipLevel));

                    const float3 diffuseTerm = Diffuse(diffColor, IMPORTANCE_SAMPLES, ndv, ndl, vdh);
                    const float3 lightTerm = sampledColor;

                    diffuseFactor = lightTerm * diffuseTerm;
                }
            }

            {
                // Specular IBL
                const float rough = roughness;
                const float mipLevel = specMipLevel;

                const float3 H = ImportanceSampleSimple(hammersley(i, IMPORTANCE_SAMPLES), rough, tangent, bitangent, N);
                const float3 L = 2.0 * dot( V, H ) * H - V;
                const float3 sampledColor = SampleCubeLOD(ZoneCubeMap, float4(L, mipLevel));

                const float vdh = saturate(abs(dot(V, H)));
                const float ldh = saturate(abs(dot(L, H)));
                const float ndh = saturate(abs(dot(N, H)));
                const float ndl = saturate(abs(dot(N, L)));

                float3 specularTerm = 0.0;
                float3 lightTerm = 0.0;

                if (ndl > 0.05)
                {
                    const float3 fresnelTerm = Fresnel(specColor, vdh, ldh);
                    const float distTerm = 1.0; // Optimization, this term is mathematically cancelled out  -- Distribution(ndh, roughness);
                    const float visTerm = Visibility(ndl, ndv, rough);

                    lightTerm = sampledColor * ndl;
                    specularTerm = distTerm * visTerm * fresnelTerm * ndl/ M_PI;
                }
                else // reduce artifacts at extreme grazing angles
                {
                    const float3 fresnelTerm = Fresnel(specColor, vdh, ldh);
                    const float distTerm = 1.0;//Distribution(ndh_, roughness);
                    const float visTerm = Visibility(ndl, ndv, rough);

                    lightTerm = sampledColor * ndl;
                    specularTerm = distTerm * visTerm * fresnelTerm * ndl/ M_PI;
                }

                // energy conservation:
                // Specular conservation:
                specularFactor = lightTerm * specularTerm / M_PI;
                specularFactor = max(saturate(normalize(specularFactor) * (length(sampledColor * specColor))), specularFactor);

                // Diffuse conservation:
                //kd = (sampledColor * specColor)/specularFactor; //energy conservation
                kd = 1.0 - specularFactor;
            }

            accumulatedColor += specularFactor + diffuseFactor * kd;
        }

        return (accumulatedColor / IMPORTANCE_SAMPLES);
    }

    /// Calculate IBL contributation
    ///     reflectVec: reflection vector for cube sampling
    ///     wsNormal: surface normal in word space
    ///     toCamera: normalized direction from surface point to camera
    ///     roughness: surface roughness
    ///     ambientOcclusion: ambient occlusion
    float3 ImageBasedLighting(in float3 reflectVec, in float3 tangent, in float3 bitangent, in float3 wsNormal, in float3 toCamera, in float3 diffColor, in float3 specColor, in float roughness, inout float3 reflectionCubeColor)
    {
        return ImportanceSampling(reflectVec, tangent, bitangent, wsNormal, toCamera, diffColor, specColor, roughness, reflectionCubeColor);
    }
#endif
    