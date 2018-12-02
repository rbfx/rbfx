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

#ifndef __D_TRACKER_RECORD__
#define __D_TRACKER_RECORD__

#define DG_TIME_TRACKER_ENTRIES_POWER	13


enum dTrackerChunkType
{
	m_traceSamples,
	m_traceLabel,
	m_traceEnd = 0x7fffffff
};

class dTimeTrackerRecord
{
	public:
	unsigned m_start;
	unsigned m_duration;
	unsigned m_nameHash;
};


#endif