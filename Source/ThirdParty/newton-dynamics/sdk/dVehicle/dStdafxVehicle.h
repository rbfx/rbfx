/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/


#ifndef __D_VEHICLE_H__
#define __D_VEHICLE_H__


#ifdef _DVEHICLE_STATIC_LIB
	#define DVEHICLE_API
#else 
	#ifdef _DVEHICLE_BUILD_DLL
        #ifdef WIN32
            #define DVEHICLE_API __declspec (dllexport)
        #elif defined(__GNUC__)
            #define DVEHICLE_API __attribute__((visibility("default")))
        #endif
	#else
        #ifdef WIN32
            #define DVEHICLE_API __declspec (dllimport)
        #else
            #define DVEHICLE_API
        #endif
	#endif
#endif


#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#endif
	#include <windows.h>
	#include <crtdbg.h>
#endif

#if ( defined (_MINGW_32_VER) || defined (_MINGW_64_VER) )
	#include <crtdbg.h>
#endif

#include <stdio.h>
#include <assert.h>
#endif

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <Newton.h>

#include <dStdAfxMath.h>
#include <dMathDefines.h>
#include <dVector.h>
#include <dMatrix.h>
#include <dLinearAlgebra.h>

#include <dCustomJointLibraryStdAfx.h>
#include <dCustomAlloc.h>
#include <dCustomJoint.h>
#include <dCustomControllerManager.h>

#include <dAnimationStdAfx.h>
//#include <dAnimAcyclicJoint.h>
//#include <dAnimAcyclicSolver.h>
//#include <dAnimIDRigKinematicLoopJoint.h>
