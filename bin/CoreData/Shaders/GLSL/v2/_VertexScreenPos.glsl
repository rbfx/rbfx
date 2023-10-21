#ifndef _VERTEX_SCREEN_POS_GLSL_
#define _VERTEX_SCREEN_POS_GLSL_

#ifdef URHO3D_VERTEX_SHADER

mediump mat3 GetCameraRot()
{
#ifdef URHO3D_XR
    mat4 viewInv = cViewInv[gl_InstanceID & 1];
    return mat3(viewInv[0][0], viewInv[0][1], viewInv[0][2],
        viewInv[1][0], viewInv[1][1], viewInv[1][2],
        viewInv[2][0], viewInv[2][1], viewInv[2][2]);
#else
    return mat3(cViewInv[0][0], cViewInv[0][1], cViewInv[0][2],
        cViewInv[1][0], cViewInv[1][1], cViewInv[1][2],
        cViewInv[2][0], cViewInv[2][1], cViewInv[2][2]);
#endif
}

vec4 GetScreenPos(vec4 clipPos)
{
    return vec4(
        clipPos.x * cGBufferOffsets.z + cGBufferOffsets.x * clipPos.w,
#ifdef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
        -clipPos.y * cGBufferOffsets.w + cGBufferOffsets.y * clipPos.w,
#else
        clipPos.y * cGBufferOffsets.w + cGBufferOffsets.y * clipPos.w,
#endif
        0.0,
        clipPos.w);
}

vec2 GetScreenPosPreDiv(vec4 clipPos)
{
    return vec2(
        clipPos.x / clipPos.w * cGBufferOffsets.z + cGBufferOffsets.x,
#ifdef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
        -clipPos.y / clipPos.w * cGBufferOffsets.w + cGBufferOffsets.y);
#else
        clipPos.y / clipPos.w * cGBufferOffsets.w + cGBufferOffsets.y);
#endif
}

vec2 GetQuadTexCoord(vec4 clipPos)
{
    return vec2(
        clipPos.x / clipPos.w * 0.5 + 0.5,
#ifdef URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
        -clipPos.y / clipPos.w * 0.5 + 0.5);
#else
        clipPos.y / clipPos.w * 0.5 + 0.5);
#endif
}

vec2 GetQuadTexCoordNoFlip(vec3 worldPos)
{
    return vec2(
        worldPos.x * 0.5 + 0.5,
        -worldPos.y * 0.5 + 0.5);
}

vec3 GetFarRay(vec4 clipPos)
{
    vec3 frustumSize = STEREO_VAR(cFrustumSize).xyz;
    vec3 viewRay = vec3(
        clipPos.x / clipPos.w * frustumSize.x,
        clipPos.y / clipPos.w * frustumSize.y,
        frustumSize.z);

    return viewRay * GetCameraRot();
}

vec3 GetNearRay(vec4 clipPos)
{
    vec3 frustumSize = STEREO_VAR(cFrustumSize).xyz;
    vec3 viewRay = vec3(
        clipPos.x / clipPos.w * frustumSize.x,
        clipPos.y / clipPos.w * frustumSize.y,
        0.0);

    return (viewRay * GetCameraRot()) * STEREO_VAR(cDepthMode).x;
}

#endif // URHO3D_VERTEX_SHADER
#endif // _VERTEX_SCREEN_POS_GLSL_
