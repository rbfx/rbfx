#version 430

#define PI 3.141596

layout(binding = 0)
uniform samplerCube srcTex;

layout(binding = 1, rgba8)
uniform image2DArray outputTexture;

vec2 Hammersley(uint seed, uint ct)
{
    uint bits = seed;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float radInv = float(bits) * 2.3283064365386963e-10;
	return vec2(float(seed) / float(ct), radInv);
}

mat3x3 CalcTangentSpace(const vec3 n)
{
    vec3 alignmentCheck = abs(n.x) > 0.99f ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(n, alignmentCheck));
    vec3 binormal = normalize(cross(n, tangent));
    return mat3x3(tangent, binormal, n);
}

vec3 ImportanceSample(vec2 interval, float roughness)
{
	float roughFactor = roughness * roughness;
    roughFactor *= roughFactor;

	const float phi = PI * 2.0 * interval.x;
	const float cosTheta = sqrt((1.0 - interval.y) / (1.0 + (roughFactor - 1.0) * interval.y));
	const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 lobeVec;
	lobeVec.x = sinTheta * cos(phi);
	lobeVec.y = sinTheta * sin(phi);
	lobeVec.z = cosTheta;
	return lobeVec;
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    uvec3 dispatchThreadId = gl_GlobalInvocationID.xyz;

    if (dispatchThreadId.x < FILTER_RES && dispatchThreadId.y < FILTER_RES)
    {
        // squash down to 0 - 1.0 range
        vec2 uvCoords = vec2(dispatchThreadId.x, dispatchThreadId.y) * FILTER_INV_RES;

        // then scale to domain of -1 to 1 and flip Y for sample correctness
        uvCoords = uvCoords * 2.0 - 1.0;
        uvCoords.y *= -1.0;

        vec3 sampleDir;
        switch (dispatchThreadId.z)
        {
        case 0: // +X
            sampleDir = vec3(1.0, uvCoords.y, -uvCoords.x);
            break;
        case 1: // -X
            sampleDir = vec3(-1.0, uvCoords.yx);
            break;
        case 2: // +Y
            sampleDir = vec3(uvCoords.x, 1.0, -uvCoords.y);
            break;
        case 3: // -Y
            sampleDir = vec3(uvCoords.x, -1.0, uvCoords.y);
            break;
        case 4: // +Z
            sampleDir = vec3(uvCoords, 1.0);
            break;
        case 5: // -Z
            sampleDir = vec3(-uvCoords.x, uvCoords.y, -1.0);
            break;
        }

        const mat3x3 tanFrame = CalcTangentSpace(sampleDir);

        vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

        // gather samples on the dome, could instead use importance sampling
        for (uint i = 0; i < RAY_COUNT; ++i)
        {
            vec2 h = Hammersley(i, RAY_COUNT);
            vec3 hemisphere = ImportanceSample(h, ROUGHNESS);
			vec3 finalDir = tanFrame * hemisphere;

            color.rgb += textureLod(srcTex, finalDir, 0).rgb;
        }
        color.rgb /= RAY_COUNT;

        imageStore(outputTexture, ivec3(dispatchThreadId), color);
    }
}
