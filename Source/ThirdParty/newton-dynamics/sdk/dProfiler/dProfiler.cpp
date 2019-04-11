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

#include "dProfiler.h"

//#if 0
#if !defined (WIN32) || (_MSC_VER >= 1900)


	#include "Tracy.hpp"
	#include "common\TracySystem.hpp"
	#include "client\TracyProfiler.hpp"

	using namespace tracy;

	static bool profileOn = false;

	void dProfilerEnableProlingLow(int mode)
	{
		profileOn = mode ? true : false;
	}
	
	long long dProfilerStartTraceLow(const dProfilerSourceLocation* const srcloc)
	{
		if (profileOn) {
			const auto thread = GetThreadHandle();
			Magic magic;
			auto token = GetToken();
			auto& tail = token->get_tail_index();
			auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>(magic);
			MemWrite(&item->hdr.type, QueueType::ZoneBegin);
#ifdef TRACY_RDTSCP_OPT
			MemWrite(&item->zoneBegin.time, Profiler::GetTime(item->zoneBegin.cpu));
#else
			uint32_t cpu;
			MemWrite(&item->zoneBegin.time, Profiler::GetTime(cpu));
			MemWrite(&item->zoneBegin.cpu, cpu);
#endif
			MemWrite(&item->zoneBegin.thread, thread);
			MemWrite(&item->zoneBegin.srcloc, (uint64_t)srcloc);
			tail.store(magic + 1, std::memory_order_release);
			return long long(thread);
		} else {
			return 0;
		}
	}

	void dProfilerEndTraceLow(long long threadId)
	{
		if (profileOn) {
			Magic magic;
			std::thread::native_handle_type thread = (std::thread::native_handle_type) threadId;
			auto token = GetToken();
			auto& tail = token->get_tail_index();
			auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>(magic);
			MemWrite(&item->hdr.type, QueueType::ZoneEnd);
#ifdef TRACY_RDTSCP_OPT
			MemWrite(&item->zoneEnd.time, Profiler::GetTime(item->zoneEnd.cpu));
#else
			uint32_t cpu;
			MemWrite(&item->zoneEnd.time, Profiler::GetTime(cpu));
			MemWrite(&item->zoneEnd.cpu, cpu);
#endif
			MemWrite(&item->zoneEnd.thread, thread);
			tail.store(magic + 1, std::memory_order_release);
		}
	}

	void dProfilerSetTrackNameLow(const char* const trackName)
	{
		std::thread::native_handle_type handle = (std::thread::native_handle_type) GetThreadHandle();
		SetThreadName(handle, trackName);
	}

#else

	void dProfilerEnableProlingLow(int mode)
	{
		mode = 0;
	}

	long long dProfilerStartTraceLow(const dProfilerSourceLocation* const)
	{
		return 0;
	}

	void dProfilerEndTraceLow(int long long)
	{
	}

	void dProfilerSetTrackNameLow(const char* const)
	{
	}

#endif
