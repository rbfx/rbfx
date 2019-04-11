/* Copyright (c) <2018-2018> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __D_PROFILER_H__
#define __D_PROFILER_H__


#ifdef D_PROFILER_EXPORTS
#define D_PROFILER_API __declspec(dllexport)
#else
#define D_PROFILER_API __declspec(dllimport)
#endif

//#define D_PROFILER

struct dProfilerSourceLocation
{
	const char* name;
	const char* function;
	const char* file;
	long long line;
	long long color;
};

D_PROFILER_API void dProfilerEnableProlingLow(int mode);
D_PROFILER_API long long dProfilerStartTraceLow(const dProfilerSourceLocation* const sourceLocation);
D_PROFILER_API void dProfilerEndTraceLow(long long id);
D_PROFILER_API void dProfilerSetTrackNameLow(const char* const trackName);


#ifdef D_PROFILER

class dgProfile
{
	public:
	dgProfile(const dProfilerSourceLocation* const location)
		:m_thread(dProfilerStartTraceLow(location))
	{
	}

	~dgProfile()
	{
		dProfilerEndTraceLow(m_thread);
	}

	private:
	long long m_thread;
};

#define dProfilerEnableProling(mode) dProfilerEnableProlingLow(mode);
#define dProfilerZoneScoped(name)					\
static const dProfilerSourceLocation __dprofiler_source_location { NULL, __FUNCTION__,  __FILE__, (long long)__LINE__, 0 }; \
dgProfile ___dgprofile_scoped_zone( &__dprofiler_source_location );

#define dProfilerSetTrackName(trackName) dProfilerSetTrackNameLow(trackName) 

#else

#define dProfilerEnableProling(mode);
#define dProfilerZoneScoped(name)
#define dProfilerSetTrackName(trackName)
#endif

#endif