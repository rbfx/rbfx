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

#ifndef __DG_PROFILER_H__
#define __DG_PROFILER_H__

// uncomment out _DG_USE_PROFILER to enable detail profile captures
// alternatively the end application can use a command line option to enable this define
//#define _DG_USE_PROFILER

#ifdef D_PROFILER
	#include <dProfiler.h>

	#define D_TRACKTIME() dProfilerZoneScoped(__FUNCTION__)
	#define D_SET_TRACK_NAME(trackName) dProfilerSetTrackName(trackName)
#else
	#define D_TRACKTIME() 
	#define D_SET_TRACK_NAME(trackName)
#endif

	
#ifdef _DG_USE_PROFILER
	#define DG_TRACKTIME() D_TRACKTIME()
#else
	#define DG_TRACKTIME()
#endif

#endif
