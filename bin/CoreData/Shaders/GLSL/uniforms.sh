#ifndef __UNIFORMS_SH__
#define __UNIFORMS_SH__

#if BGFX_SHADER_TYPE_VERTEX == 1

// Vertex shader uniforms
uniform vec3 cAmbientStartColor;
uniform vec3 cAmbientEndColor;
uniform mat3 cBillboardRot;
uniform vec3 cCameraPos;
uniform float cNearClip;
uniform float cFarClip;
uniform vec4 cDepthMode;
uniform vec3 cFrustumSize;
uniform float cDeltaTime;
uniform float cElapsedTime;
uniform vec4 cGBufferOffsets;
uniform vec4 cLightPos;
uniform vec3 cLightDir;
uniform vec4 cNormalOffsetScale;
uniform mat4 cModel;
//uniform mat4 cView; u_view
//uniform mat4 cViewInv; u_invProj
//uniform mat4 cViewProj; u_viewProj
uniform vec4 cUOffset;
uniform vec4 cVOffset;
uniform mat4 cZone;
#if !defined(GL_ES) || defined(WEBGL)
    uniform mat4 cLightMatrices[4];
#else
    uniform highp mat4 cLightMatrices[2];
#endif
#ifdef SKINNED
    uniform vec4 cSkinMatrices[MAXBONES*3];
#endif
#ifdef NUMVERTEXLIGHTS
    uniform vec4 cVertexLights[4*3];
#endif
#ifdef GL3
    uniform vec4 cClipPlane;
#endif
#endif

#if BGFX_SHADER_TYPE_FRAGMENT == 1

// Fragment shader uniforms
#ifdef GL_ES
    precision mediump float;
#endif

uniform vec4 cAmbientColor;
uniform vec3 cCameraPosPS;
uniform float cDeltaTimePS;
uniform vec4 cDepthReconstruct;
uniform float cElapsedTimePS;
uniform vec4 cFogParams;
uniform vec3 cFogColor;
uniform vec2 cGBufferInvSize;
uniform vec4 cLightColor;
uniform vec4 cLightPosPS;
uniform vec3 cLightDirPS;
uniform vec4 cNormalOffsetScalePS;
uniform vec4 cMatDiffColor;
uniform vec3 cMatEmissiveColor;
uniform vec3 cMatEnvMapColor;
uniform vec4 cMatSpecColor;
#ifdef PBR
    uniform float cRoughness;
    uniform float cMetallic;
    uniform float cLightRad;
    uniform float cLightLength;
#endif
uniform vec3 cZoneMin;
uniform vec3 cZoneMax;
uniform float cNearClipPS;
uniform float cFarClipPS;
uniform vec4 cShadowCubeAdjust;
uniform vec4 cShadowDepthFade;
uniform vec2 cShadowIntensity;
uniform vec2 cShadowMapInvSize;
uniform vec4 cShadowSplits;
uniform mat4 cLightMatricesPS[4];
#ifdef VSM_SHADOW
uniform vec2 cVSMShadowParams;
#endif
#endif

#endif