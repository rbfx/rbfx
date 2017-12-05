#ifndef __TRANSFORM_SH__
#define __TRANSFORM_SH__

#if COMPILEVS

#ifdef SKINNED
mat4 GetSkinMatrix(vec4 blendWeights, vec4 blendIndices)
{
    ivec4 idx = ivec4(blendIndices) * 3;
    const vec4 lastColumn = vec4(0.0, 0.0, 0.0, 1.0);
    return mat4(cSkinMatrices[idx.x], cSkinMatrices[idx.x + 1], cSkinMatrices[idx.x + 2], lastColumn) * blendWeights.x +
        mat4(cSkinMatrices[idx.y], cSkinMatrices[idx.y + 1], cSkinMatrices[idx.y + 2], lastColumn) * blendWeights.y +
        mat4(cSkinMatrices[idx.z], cSkinMatrices[idx.z + 1], cSkinMatrices[idx.z + 2], lastColumn) * blendWeights.z +
        mat4(cSkinMatrices[idx.w], cSkinMatrices[idx.w + 1], cSkinMatrices[idx.w + 2], lastColumn) * blendWeights.w;
}
#endif

#ifdef INSTANCED
mat4 GetInstanceMatrix()
{
    const vec4 lastColumn = vec4(0.0, 0.0, 0.0, 1.0);
    return mat4(i_data0, i_data1, i_data2, lastColumn);
}
#endif

mat3 GetNormalMatrix(mat4 modelMatrix)
{
    return mat3(modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz);
}

vec2 GetTexCoord(vec2 texCoord)
{
    return vec2(dot(texCoord, cUOffset.xy) + cUOffset.w, dot(texCoord, cVOffset.xy) + cVOffset.w);
}

vec4 GetClipPos(vec3 worldPos)
{
    vec4 ret = mul(vec4(worldPos, 1.0), cViewProj);
    // While getting the clip coordinate, also automatically set gl_ClipVertex for user clip planes
    #ifdef CLIPPLANE
    #if !defined(GL_ES) && !defined(GL3) && !defined(D3D11)
        gl_ClipVertex = ret;
    #elif defined(GL3)
        gl_ClipDistance[0] = dot(cClipPlane, ret);
    #elif defined(D3D11)
        //vClip = dot(ret, cClipPlane);
    #endif
    #endif
    return ret;
}

float GetZonePos(vec3 worldPos)
{
    return clamp(mul(vec4(worldPos, 1.0), cZone).z, 0.0, 1.0);
}

float GetDepth(vec4 clipPos)
{
    return dot(clipPos.zw, cDepthMode.zw);
}

#ifdef BILLBOARD
vec3 GetBillboardPos(vec4 iPos, vec2 iSize, mat4 modelMatrix)
{
    return mul(iPos, modelMatrix).xyz + mul(vec3(iSize.x, iSize.y, 0.0), cBillboardRot);
}

vec3 GetBillboardNormal()
{
    return vec3(-cBillboardRot[0][2], -cBillboardRot[1][2], -cBillboardRot[2][2]);
}
#endif

#ifdef DIRBILLBOARD
mat3 GetFaceCameraRotation(vec3 position, vec3 direction)
{
    vec3 cameraDir = normalize(position - cCameraPos);
    vec3 front = normalize(direction);
    vec3 right = normalize(cross(front, cameraDir));
    vec3 up = normalize(cross(front, right));

    return mat3(
        right.x, up.x, front.x,
        right.y, up.y, front.y,
        right.z, up.z, front.z
    );
}

vec3 GetBillboardPos(vec4 iPos, vec3 iDirection, mat4 modelMatrix)
{
    vec3 worldPos = mul(iPos, modelMatrix).xyz;
    return worldPos + mul(vec3(iTexCoord1.x, 0.0, iTexCoord1.y), GetFaceCameraRotation(worldPos, iDirection));
}

vec3 GetBillboardNormal(vec4 iPos, vec3 iDirection, mat4 modelMatrix)
{
    vec3 worldPos = mul(iPos, modelMatrix).xyz;
    return mul(vec3(0.0, 1.0, 0.0), GetFaceCameraRotation(worldPos, iDirection));
}
#endif

#ifdef TRAILFACECAM
vec3 GetTrailPos(vec4 iPos, vec3 iFront, float iScale, mat4 modelMatrix)
{
    vec3 up = normalize(cCameraPos - iPos.xyz);
    vec3 right = normalize(cross(iFront, up));
    return (mul(vec4((iPos.xyz + right * iScale), 1.0), modelMatrix)).xyz;
}

vec3 GetTrailNormal(vec4 iPos)
{
    return normalize(cCameraPos - iPos.xyz);
}
#endif

#ifdef TRAILBONE
vec3 GetTrailPos(vec4 iPos, vec3 iParentPos, float iScale, mat4 modelMatrix)
{
    vec3 right = iParentPos - iPos.xyz;
    return (mul(vec4((iPos.xyz + right * iScale), 1.0), modelMatrix)).xyz;
}

vec3 GetTrailNormal(vec4 iPos, vec3 iParentPos, vec3 iForward)
{
    vec3 left = normalize(iPos.xyz - iParentPos);
    vec3 up = normalize(cross(normalize(iForward), left));
    return up;
}
#endif

#if defined(SKINNED)
    #define iModelMatrix GetSkinMatrix(iBlendWeights, iBlendIndices)
#elif defined(INSTANCED)
    #define iModelMatrix GetInstanceMatrix()
#else
    #define iModelMatrix cModel
#endif

#if defined(BILLBOARD)
    #define GetWorldPos(modelMatrix) GetBillboardPos(iPos, iSize, modelMatrix)
#elif defined(DIRBILLBOARD)
    #define GetWorldPos(modelMatrix) GetBillboardPos(iPos, iSize, iNormal, modelMatrix)
#elif defined(TRAILFACECAM)
    #define GetWorldPos(modelMatrix) GetTrailPos(iPos, iTangent.xyz, iTangent.w, modelMatrix)
#elif defined(TRAILBONE)
    #define GetWorldPos(modelMatrix) GetTrailPos(iPos, iTangent.xyz, iTangent.w, modelMatrix)
#else
    #define GetWorldPos(modelMatrix) mul(iPos, modelMatrix).xyz
#endif

#if defined(BILLBOARD)
    #define GetWorldNormal(modelMatrix) GetBillboardNormal()
#elif defined(DIRBILLBOARD)
    #define GetWorldNormal(modelMatrix) GetBillboardNormal(iPos, iNormal, modelMatrix)
#elif defined(TRAILFACECAM)
    #define GetWorldNormal(modelMatrix) GetTrailNormal(iPos)
#elif defined(TRAILBONE)
    #define GetWorldNormal(modelMatrix) GetTrailNormal(iPos, iTangent.xyz, iNormal)
#else
    #define GetWorldNormal(modelMatrix) normalize(mul(iNormal, GetNormalMatrix(modelMatrix)))
#endif

#if defined(BILLBOARD)
    #define GetWorldTangent(modelMatrix) vec4(normalize(mul(vec3(1.0, 0.0, 0.0), cBillboardRot)), 1.0)
#elif defined(DIRBILLBOARD)
    #define GetWorldTangent(modelMatrix) vec4(normalize(mul(vec3(1.0, 0.0, 0.0), GetNormalMatrix(modelMatrix))), 1.0)
#else
    #define GetWorldTangent(modelMatrix) vec4(normalize(mul(iTangent.xyz, GetNormalMatrix(modelMatrix))), iTangent.w)
#endif

#endif

#endif