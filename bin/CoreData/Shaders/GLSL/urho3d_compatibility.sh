#ifndef __URHO3D_COMPATIBILITY_SH__
#define __URHO3D_COMPATIBILITY_SH__

#if !defined(BILLBOARD) && !defined(TRAILFACECAM) && !defined(BASIC)
#define _NORMAL , a_normal
#elif defined(BASIC) && defined(DIRBILLBOARD) || defined(TRAILBONE)
#define _NORMAL , a_normal
#else
#define _NORMAL
#endif

#if !defined(NOUV) && !defined(BASIC)
#define _TEXCOORD0 , a_texcoord0
#define _VTEXCOORD , vTexCoord
#elif defined(BASIC) && defined(DIFFMAP) || defined(ALPHAMAP)
#define _TEXCOORD0 , a_texcoord0
#define _VTEXCOORD , vTexCoord
#else
#define _TEXCOORD0
#define _VTEXCOORD
#endif

#ifdef VERTEXCOLOR
#define _COLOR0 , a_color0
#define _VCOLOR , vColor
#else
#define _ACOLOR0
#define _VCOLOR
#endif

#if (defined(NORMALMAP) || defined(TRAILFACECAM) || defined(TRAILBONE)) && !defined(BILLBOARD) && !defined(DIRBILLBOARD)
#define _ATANGENT , a_tangent
#else
#define _ATANGENT
#endif

#if defined(LIGHTMAP) || defined(AO) || defined(BILLBOARD) || defined(DIRBILLBOARD)
#define _TEXCOORD1 , a_texcoord1
#else
#define _TEXCOORD1
#endif

#ifdef SKINNED
#define _SKINNED , a_weight, a_indices
#else
#define _SKINNED
#endif

#ifdef INSTANCED
#define _INSTANCED , i_data0, i_data1, i_data2, i_data3
#else
#define _INSTANCED
#endif

#if defined(DIRLIGHT) && (!defined(GL_ES) || defined(WEBGL))
#define _NUMCASCADES 4
#else
#define _NUMCASCADES 1
#endif

#ifdef NORMALMAP
#define _VTANGENT , vTangent
#else
#define _VTANGENT
#endif

#ifdef SHADOW
#define _VSHADOWPOS , vShadowPos[_NUMCASCADES]
#else
#define _VSHADOWPOS
#endif

#ifdef SPOTLIGHT
#define _VSPOTPOS , vSpotPos
#else
#define _VSPOTPOS
#endif

#ifdef POINTLIGHT
#define _VCUBEMASKVEC , vCubeMaskVec
#else
#define _VCUBEMASKVEC
#endif

#ifdef ENVCUBEMAP
#define _VREFLECTIONVEC , vReflectionVec
#else
#define _VREFLECTIONVEC
#endif

#if defined(LIGHTMAP) || defined(AO)
#define _VTEXCOORD2 , vTexCoord2
#else
#define _VTEXCOORD2
#endif

#ifdef ORTHO
#define _VNEARRAY , vNearRay
#else
#define _VNEARRAY
#endif

#ifdef SOFTPARTICLES
#define _VSCREENPOS , vScreenPos
#else
#define _VSCREENPOS
#endif

#ifdef CLIPPLANE
#define _VCLIP //, vClip
#else
#define _VCLIP
#endif

#define iPos a_position
#define iNormal a_normal
#define iTexCoord a_texcoord0
#define iColor a_color0
#define iTexCoord2 a_texcoord1
#define iTangent a_tangent
#define iBlendWeights a_weight
#define iBlendIndices a_indices
#define iSize a_texcoord1
#define iTexCoord4 i_data0
#define iTexCoord5 i_data1
#define iTexCoord6 i_data2
#define iObjectIndex i_data3

#endif