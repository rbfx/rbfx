#ifndef __UNIFORMS_SH__
#define __UNIFORMS_SH__

#if COMPILEVS

// Vertex shader uniforms
uniform vec4 u_AmbientStartColor; //vec3
uniform vec4 u_AmbientEndColor; //vec3
uniform mat3 u_BillboardRot;
uniform vec4 u_CameraPos; //vec3
uniform vec4 u_NearClip; //float
uniform vec4 u_FarClip; //float
uniform vec4 u_DepthMode;
uniform vec4 u_FrustumSize; //vec3
uniform vec4 u_DeltaTime; //float
uniform vec4 u_ElapsedTime; //float
uniform vec4 u_GBufferOffsets;
uniform vec4 u_LightPos;
uniform vec4 u_LightDir; //vec3
uniform vec4 u_NormalOffsetScale;
//uniform mat4 cModel; u_model
//uniform mat4 cView; u_view
//uniform mat4 cViewInv; u_invProj
//uniform mat4 cViewProj; u_viewProj
uniform vec4 u_UOffset;
uniform vec4 u_VOffset;
uniform mat4 u_Zone;
#if !defined(GL_ES) || defined(WEBGL)
    uniform mat4 u_LightMatrices[4];
#else
    uniform highp mat4 u_LightMatrices[2];
#endif
//#ifdef SKINNED
//    uniform vec4 u_SkinMatrices[MAXBONES*3]; // u_model[BGFX_CONFIG_MAX_BONES]
//#endif
#ifdef NUMVERTEXLIGHTS
    uniform vec4 u_VertexLights[4*3];
#endif
#ifdef GL3
    uniform vec4 u_ClipPlane;
#endif

#define cAmbientStartColor vec3(u_AmbientStartColor.xyz)
#define cAmbientEndColor vec3(u_AmbientEndColor.xyz)
#define cBillboardRot u_BillboardRot
#define cCameraPos vec3(u_CameraPos.xyz)
#define cNearClip u_NearClip.x
#define cFarClip u_FarClip.x
#define cDepthMode u_DepthMode
#define cFrustumSize vec3(u_FrustumSize.xyz)
#define cDeltaTime u_DeltaTime.x
#define cElapsedTime u_ElapsedTime.x
#define cGBufferOffsets u_GBufferOffsets
#define cLightPos u_LightPos
#define cLightDir vec3(u_LightDir.xyz)
#define cNormalOffsetScale u_NormalOffsetScale
#define cModel u_model[0]
#define cView u_view
#define cViewInv u_invProj
#define cViewProj u_viewProj
#define cUOffset u_UOffset
#define cVOffset u_VOffset
#define cZone u_Zone
#define cLightMatrices u_LightMatrices
#define cSkinMatrices u_SkinMatrices
#define cVertexLights u_VertexLights

#define cClipPlane u_ClipPlane

#endif

#if COMPILEPS

uniform vec4 u_AmbientColor;
uniform vec4 u_CameraPosPS; //vec3
uniform vec4 u_DeltaTimePS; //float
uniform vec4 u_DepthReconstruct;
uniform vec4 u_ElapsedTimePS; //float
uniform vec4 u_FogParams;
uniform vec4 u_FogColor; //vec3
uniform vec4 u_GBufferInvSize; //vec2
uniform vec4 u_LightColor;
uniform vec4 u_LightPosPS;
uniform vec4 u_LightDirPS; //vec3
uniform vec4 u_NormalOffsetScalePS;
uniform vec4 u_MatDiffColor;
uniform vec4 u_MatEmissiveColor; //vec3
uniform vec4 u_MatEnvMapColor; //vec3
uniform vec4 u_MatSpecColor;
#ifdef PBR
    uniform vec4 u_Roughness; //float
    uniform vec4 u_Metallic; //float
    uniform vec4 u_LightRad; //float
    uniform vec4 u_LightLength; //float
    // TODO: combine these into one
    //uniform vec4 u_PBRParams
#endif
uniform vec4 u_ZoneMin; //vec3
uniform vec4 u_ZoneMax; //vec3
uniform vec4 u_NearClipPS; //float
uniform vec4 u_FarClipPS; //float
uniform vec4 u_ShadowCubeAdjust;
uniform vec4 u_ShadowDepthFade;
uniform vec4 u_ShadowIntensity; //vec2
uniform vec4 u_ShadowMapInvSize; //vec2
uniform vec4 u_ShadowSplits;
uniform mat4 u_LightMatricesPS[4];
#ifdef VSM_SHADOW
uniform vec4 u_VSMShadowParams; //vec2
#endif

#define cAmbientColor u_AmbientColor
#define cCameraPosPS vec3(u_CameraPosPS.xyz)
#define cDeltaTimePS u_DeltaTimePS.x
#define cDepthReconstruct u_DepthReconstruct
#define cElapsedTimePS u_ElapsedTimePS.x
#define cFogParams u_FogParams
#define cFogColor u_FogColor.xyz
#define cGBufferInvSize vec2(u_GBufferInvSize.xy)
#define cLightColor u_LightColor
#define cLightPosPS u_LightPosPS
#define cLightDirPS vec3(u_LightDirPS.xyz)
#define cNormalOffsetScalePS u_NormalOffsetScalePS
#define cMatDiffColor u_MatDiffColor
#define cMatEmissiveColor vec3(u_MatEmissiveColor.xyz)
#define cMatEnvMapColor vec3(u_MatEnvMapColor.xyz)
#define cMatSpecColor u_MatSpecColor
#define cRoughness u_Roughness.x
#define cMetallic u_Metallic.x
#define cLightRad u_LightRad.x
#define cLightLength u_LightLength.x
//#define cRoughness u_PBRParams.x
//#define cMetallic u_PBRParams.y
//#define cLightRad u_PBRParams.z
//#define cLightLength u_PBRParams.w
#define cZoneMin vec3(u_ZoneMin.xyz)
#define cZoneMax vec3(u_ZoneMax.xyz)
#define cNearClipPS u_NearClipPS.x
#define cFarClipPS u_FarClipPS.x
#define cShadowCubeAdjust u_ShadowCubeAdjust
#define cShadowDepthFade u_ShadowDepthFade
#define cShadowIntensity vec2(u_ShadowIntensity.xy)
#define cShadowMapInvSize vec2(u_ShadowMapInvSize.xy)
#define cShadowSplits u_ShadowSplits
#define cLightMatricesPS u_LightMatricesPS
#define cVSMShadowParams vec2(u_VSMShadowParams.xy)

#endif

#endif