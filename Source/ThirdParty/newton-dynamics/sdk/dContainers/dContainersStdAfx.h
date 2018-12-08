/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef __D_CONTAINERS_STDAFX__
#define __D_CONTAINERS_STDAFX__

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dMathDefines.h>
#include <dVector.h>
#include <dMatrix.h>


#ifdef _DCONTAINERS_DLL
	#ifdef _DCONTAINERS_EXPORT
		#define DCONTAINERS_API __declspec (dllexport)
	#else
		#define DCONTAINERS_API __declspec (dllimport)
	#endif
#else
	#define DCONTAINERS_API
#endif


#ifdef _MSC_VER
	#include <windows.h>
	#include <crtdbg.h>
#endif

#ifdef _MACOSX_VER
	#include <unistd.h>
	#include <libkern/OSAtomic.h>
	#include <sys/sysctl.h>
#endif

#include <dMathDefines.h>

#endif
