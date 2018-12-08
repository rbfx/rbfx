/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneStdafx.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 07:45:05
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////



#ifndef __D_SCENE_STDAFX_H__
#define __D_SCENE_STDAFX_H__


#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#endif
	#include <windows.h>
	#include <crtdbg.h>
#endif



#include <stdlib.h>
#include <stdio.h>
#include <locale.h>


#if _MSC_VER
	#include <windows.h>
	#include <crtdbg.h> 

	// The following macros define the minimum required platform.  The minimum required platform
	// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
	// your application.  The macros work by enabling all features available on platform versions up to and 
	// including the version specified.

	// Modify the following defines if you have to target a platform prior to the ones specified below.
	// Refer to MSDN for the latest info on corresponding values for different platforms.
	#ifndef WINVER                          // Specifies that the minimum required platform is Windows Vista.
	#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
	#endif

	#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
	#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
	#endif

	#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
	#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
	#endif

	#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
	#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
	#endif

	#pragma warning (disable: 4996) // for 2005 users declared deprecated
	#pragma warning (disable: 4100) // unreferenced formal parameter
	#pragma warning (disable: 4505) // unreferenced local function has been removed

#endif
/*
#ifndef _MSC_VER

	#ifndef stricmp
		#define stricmp strcasecmp
	#endif

	#ifndef strlwr
		inline char *_strlwr_ (char *a) 
		{ 
			char *ret = a; 
			while (*a != '\0') 
			{ 
				//if (isupper (*a)) 
				*a = char (tolower (*a)); 
				++a; 
			} 
			return ret; 
		}

		#define strlwr(a) _strlwr_ (a) 
	#endif
#endif
*/

#include <dStdAfxMath.h>
#include <dAnimationStdAfx.h>
#include <dContainersStdAfx.h>

#include <dMathDefines.h>
#include <dVector.h>
#include <dMatrix.h>
#include <dQuaternion.h>

#include <dCRC.h>
#include <dHeap.h>
#include <dList.h>
#include <dTree.h>
#include <dRtti.h>
#include <dString.h>
#include <dClassInfo.h>
#include <dRefCounter.h>
#include <dBaseHierarchy.h>
#include <dBezierSpline.h>
#include <Newton.h>

class TiXmlElement;


#ifdef _DSCENE_DLL
	#ifdef _DSCENE_EXPORTS
		#define DSCENE_API __declspec (dllexport)
	#else
		#define DSCENE_API __declspec (dllimport)
	#endif
#else
	#define DSCENE_API
#endif


#ifndef _NEWTON_USE_DOUBLE
DSCENE_API void dStringToFloatArray(const char* const string, dFloat64* const array, int maxCount);
DSCENE_API void dFloatArrayToString (const dFloat64* const array, int count, char* const string, int maxSixeInBytes);
#endif

DSCENE_API void dStringToFloatArray (const char* const string, dFloat* const array, int maxCount);
DSCENE_API void dFloatArrayToString (const dFloat* const array, int count, char* const string, int maxSixeInBytes);

DSCENE_API void dStringToIntArray (const char* const string, int* const array, int maxCount);
DSCENE_API void dIntArrayToString (const int* const array, int count, char* const string, int maxSixeInBytes);
DSCENE_API const char* dGetNameFromPath (const char* const fullName);
DSCENE_API void dExtractPathFromFullName (const char* const fullName, char* const path);

DSCENE_API int dPackVertexArray (dFloat* const vertexList, int compareElements, int strideInBytes, int vertexCount, int* const indexListOut);

DSCENE_API dFloat dBoxRayCast (const dVector& line0, const dVector& line1, const dVector& boxP0, const dVector& boxP1);
DSCENE_API void dRayToRayCast (const dVector& ray_p0, const dVector& ray_p1, const dVector& ray_q0, const dVector& ray_q1, dVector& pOut, dVector& qOut);

DSCENE_API dBigVector dPolygonNormal (int indexCount, const dFloat64* const vertex, int strideInBytes, const int* const indices);
DSCENE_API dFloat dPolygonRayCast (const dVector& line0, const dVector& line1, int indexCount, const dFloat64* const vertex, int strideInBytes, const int* const indices);

void SerializeMesh (const NewtonMesh* const mesh, TiXmlElement* const rootNode);
bool DeserializeMesh (const NewtonMesh* const mesh, TiXmlElement* const rootNode); 

// TODO: reference additional headers your program requires here

#endif