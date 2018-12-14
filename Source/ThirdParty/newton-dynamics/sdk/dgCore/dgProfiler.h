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

// uncomment out _DG_USE_PROFILER to enable profiler frame capture profiler traces
// alternatively the end application can use a command line option to enable this define
//#define _DG_USE_PROFILER

#ifdef _DG_USE_PROFILER

void ttDeleteTrack();
void ttStopRecording();
void ttCloseRecord(int recordIndex);
int ttOpenRecord(const char* const name);
void ttSetTrackName(const char* const threadName);
void ttStartRecording(const char* const fileName);

inline void dProfilerSetTrackName(const char* const name)
{
	ttSetTrackName(name);
}

inline void dProfilerDeleteTrack()
{
	ttDeleteTrack();
}

inline void dProfilerStartRecording(const char* const fileName)
{
	ttStartRecording(fileName);
}

inline void dProfilerStopRecording()
{
	ttStopRecording();
}


class dgProfile
{
	public:
	dgProfile(const char* const functionName)
		:m_entry(ttOpenRecord(functionName))
		,m_name(functionName)
	{
	}

	~dgProfile()
	{
		ttCloseRecord(m_entry);
	}

	private:
	dgInt32 m_entry;
	const char* m_name;
};

#define DG_START_RECORDING(fileName) dProfilerStartRecording(fileName)
#define DG_STOP_RECORDING() dProfilerStopRecording()

#define DG_DELETE_TRACK() dProfilerDeleteTrack()
#define DG_SET_TRACK_NAME(trackName) dProfilerSetTrackName(trackName)

#define DG_TRACKTIME(name) dgProfile _profile##name(name);
#define DG_TRACKTIME_NAMED(name) dgProfile _profile(name);

#else

#define DG_START_RECORDING(fileName);
#define DG_STOP_RECORDING();
#define DG_TRACKTIME(name);
#define DG_TRACKTIME_NAMED(name);
#define DG_SET_TRACK_NAME(trackName);
#define DG_DELETE_TRACK();

#endif

#endif
