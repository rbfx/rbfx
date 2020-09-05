#version 410

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Fog.glsl"

// Tessellation control params
//      X: maximum power, used as is if TESS_FIXED is defed
//      Y: blend range distance
//      Z: blend range offset, moves the closest point
//      W: phong smoothing power, only used if TESS_PHONG is defed
uniform vec4 cTessParams;

// Vertex shader data

#if !defined(COMPILEPS) && !defined(COMPILEDS)

    #ifdef COMPILEHS
        #define VSDATA_TAIL []
    #else
        #define VSDATA_TAIL
    #endif

    #ifdef COMPILEHS
        in
    #else
        out
    #endif
VSDataOut
{
    vec2 oTexCoord;
    vec4 oWorldPos;
    #ifdef TESS_PHONG
        vec3 oNormal;
    #endif
    #ifdef VERTEXCOLOR
        vec4 oColor;
    #endif
} vs_out VSDATA_TAIL;

#endif

#ifdef COMPILEVS

#define vTexCoord vs_out.oTexCoord
#define vWorldPos vs_out.oWorldPos
#define vColor vs_out.oColor
#define vNormal vs_out.oNormal

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetTexCoord(iTexCoord);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));

    #ifdef TESS_PHONG
        vNormal = GetWorldNormal(modelMatrix);
    #endif

    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif
}

#endif

#ifdef COMPILEHS

float PointLineDistance(vec3 p, in vec3 a, in vec3 b)
{
    vec3 ab = b - a;
	float t = clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0);
	return abs(length((ab*t + a) - p)) * 0.5;
}

bool EdgeCull(vec4 a, vec4 b, vec3 aN, vec3 bN)
{
    vec3 norm = normalize((aN + bN) * 0.5);
    vec3 midPoint = (a.xyz + b.xyz) * 0.5;
    float edgeDot = dot(normalize(cCameraPos - midPoint), norm);
    return edgeDot < -0.2;
}

out HSDataOut
{
    vec4 oWorldPos;
    vec2 oTexCoord;
    #ifdef TESS_PHONG
        vec3 oNormal;
    #endif
    #ifdef VERTEXCOLOR
        vec4 oColor;
    #endif
} hs_out[];

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

layout(vertices = 3) out;
void HS()
{
    // only calculate once
    if (gl_InvocationID == 0)
    {
    #if !defined(TESS_FIXED)
        // Tessellate based on the distance from the camera

        float distA = max(0, PointLineDistance(cCameraPos, vs_out[0].oWorldPos.xyz, vs_out[1].oWorldPos.xyz) - cTessParams.z);
        float distB = max(0, PointLineDistance(cCameraPos, vs_out[1].oWorldPos.xyz, vs_out[2].oWorldPos.xyz) - cTessParams.z);
        float distC = max(0, PointLineDistance(cCameraPos, vs_out[2].oWorldPos.xyz, vs_out[0].oWorldPos.xyz) - cTessParams.z);

        float tessAdaptDist = max(0.01, cTessParams.y - cTessParams.z);
        float maxTessLevel = max(1, min(cTessParams.x, 16));

        float minDist = min(distA, min(distB, distC));
        float tessLevelA = max(1, (1.0 - saturate(distA / tessAdaptDist)) * maxTessLevel);
        float tessLevelB = max(1, (1.0 - saturate(distB / tessAdaptDist)) * maxTessLevel);
        float tessLevelC = max(1, (1.0 - saturate(distC / tessAdaptDist)) * maxTessLevel);

        // note edge order
        gl_TessLevelOuter[2] = tessLevelA;
        gl_TessLevelOuter[0] = tessLevelB;
        gl_TessLevelOuter[1] = tessLevelC;
        gl_TessLevelInner[0]  = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3.0;

    #else
        // clip tess level to prevent accidental entry of excessive values
        float tessLevel = max(1, min(cTessParams.x, 16));
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;
        gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3.0;
    #endif

    #ifdef TESS_PHONG

        // Perform basic culling
        bool aCull = EdgeCull(vs_out[0].oWorldPos, vs_out[1].oWorldPos, vs_out[0].oNormal, vs_out[1].oNormal);
        bool bCull = EdgeCull(vs_out[1].oWorldPos, vs_out[2].oWorldPos, vs_out[1].oNormal, vs_out[2].oNormal);
        bool cCull = EdgeCull(vs_out[2].oWorldPos, vs_out[0].oWorldPos, vs_out[2].oNormal, vs_out[0].oNormal);
        if (aCull && bCull && cCull)
            gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0;

    #endif

    }

    hs_out[gl_InvocationID].oWorldPos = vs_out[gl_InvocationID].oWorldPos;
    hs_out[gl_InvocationID].oTexCoord = vs_out[gl_InvocationID].oTexCoord;
    #ifdef TESS_PHONG
        hs_out[gl_InvocationID].oNormal = vs_out[gl_InvocationID].oNormal;
    #endif
    #ifdef VERTEXCOLOR
        hs_out[gl_InvocationID].oColor = vs_out[gl_InvocationID].oColor;
    #endif
}

#endif

#ifdef COMPILEDS

in HSDataOut
{
    vec4 oWorldPos;
    vec2 oTexCoord;
    #ifdef TESS_PHONG
        vec3 oNormal;
    #endif
    #ifdef VERTEXCOLOR
        vec4 oColor;
    #endif
} hs_out[];

out PSDataIn
{
    vec4 oWorldPos;
    vec2 oTexCoord;
    #ifdef VERTEXCOLOR
        vec4 oColor;
    #endif
} ps_in;

layout(triangles, equal_spacing, ccw) in;
void DS()
{
    vec4 vertexPosition = gl_TessCoord.x * hs_out[0].oWorldPos + gl_TessCoord.y * hs_out[1].oWorldPos + gl_TessCoord.z * hs_out[2].oWorldPos;
    vec2 texCoord = gl_TessCoord.x * hs_out[0].oTexCoord + gl_TessCoord.y * hs_out[1].oTexCoord + gl_TessCoord.z * hs_out[2].oTexCoord;

    #ifdef TESS_PHONG
        vec3 vertNor = normalize(gl_TessCoord.x * hs_out[0].oNormal + gl_TessCoord.y * hs_out[1].oNormal + gl_TessCoord.z * hs_out[2].oNormal);

        vec3 curveProjectionA = dot(hs_out[0].oWorldPos.xyz - vertexPosition.xyz, hs_out[0].oNormal) * hs_out[0].oNormal;
        vec3 curveProjectionB = dot(hs_out[1].oWorldPos.xyz - vertexPosition.xyz, hs_out[1].oNormal) * hs_out[1].oNormal;
        vec3 curveProjectionC = dot(hs_out[2].oWorldPos.xyz - vertexPosition.xyz, hs_out[2].oNormal) * hs_out[2].oNormal;
        vertexPosition.xyz += (gl_TessCoord.x * curveProjectionA + gl_TessCoord.y * curveProjectionB + gl_TessCoord.z * curveProjectionC) * cTessParams.w;

    #endif

    ps_in.oWorldPos = vertexPosition;
    ps_in.oTexCoord = texCoord;

    #ifdef VERTEXCOLOR
        ps_in.oColor = gl_TessCoord.x * hs_out[0].oColor + gl_TessCoord.y * hs_out[1].oColor + gl_TessCoord.z * hs_out[2].oColor;
    #endif

    gl_Position = GetClipPos(vertexPosition.xyz);
}

#endif

#ifdef COMPILEPS

in PSDataIn
{
    vec4 oWorldPos;
    vec2 oTexCoord;
    #ifdef VERTEXCOLOR
        vec4 oColor;
    #endif
} ps_in;

#define vWorldPos ps_in.oWorldPos
#define vTexCoord ps_in.oTexCoord
#define vColor ps_in.oColor

void PS()
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        vec4 diffColor = cMatDiffColor * texture2D(sDiffMap, vTexCoord);
        #ifdef ALPHAMASK
            if (diffColor.a < 0.5)
                discard;
        #endif
    #else
        vec4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= vColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    #if defined(PREPASS)
        // Fill light pre-pass G-Buffer
        gl_FragData[0] = vec4(0.5, 0.5, 0.5, 1.0);
        gl_FragData[1] = vec4(EncodeDepth(vWorldPos.w), 0.0);
    #elif defined(DEFERRED)
        gl_FragData[0] = vec4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
        gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        gl_FragData[2] = vec4(0.5, 0.5, 0.5, 1.0);
        gl_FragData[3] = vec4(EncodeDepth(vWorldPos.w), 0.0);
    #else
        gl_FragColor = vec4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
    #endif
}

#endif
