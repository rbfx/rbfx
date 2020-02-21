///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_storage.h>

#include <sched.h>
#include <unistd.h>
#if defined(_YVALS)
	#include <time.h>
#else
	#include <sys/time.h>
#endif

#include <kernel.h>
#include <sceerror.h>
#include <sdk_version.h>
#include <cpuid.h>
#include <new>
#include <string.h>

namespace EA
{
	namespace Thread
	{
		// Assertion variables.
		EA::Thread::AssertionFailureFunction gpAssertionFailureFunction = NULL;
		void*                                gpAssertionFailureContext  = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Variables required for ThreadSleep
//
// TLS var for quicker lookups to our thread's data so we may grab the thread local EAThreadTimerQueue
static EA_THREAD_LOCAL EAThreadDynamicData* tpThreadDynamicData = nullptr;
// In the event a non-EAThread requires a timer queue we may supply the global instance
static EAThreadTimerQueue gThreadTimerQueue;
////////////////////////////////////////////////////////////////////////////////////////////////////////

EA::Thread::ThreadId EA::Thread::GetThreadId()
{
	// https://ps4.scedev.net/forums/thread/12697/
	// https://ps4.scedev.net/forums/thread/53323/
	// 
	// ScePthread scePthreadSelf() does not return a integral thread id value. Instead it returns a ScePthread structure
	// with is actually a pointer to a pthread structure (eg. pthread*).  On other Sony platforms, an API like
	// scePthreadGetthreadid was available for this use case but this isn't the case on the PS4.  The above scedev.net
	// threads indicate that the request for an additiona API to retrieve the kernel threadid has been submitted to
	// Sony.  Until this feature is shipped in a future SDK update we use the following technique to get a scalar thread
	// id value that matches the threadid presented in the PS4 debugger. 

	const EA::Thread::ThreadId currentThreadId = *reinterpret_cast<EA::Thread::ThreadId*>(scePthreadSelf());
	return currentThreadId;
}

EA::Thread::ThreadId EA::Thread::GetThreadId(EA::Thread::SysThreadId id)
{
	EAThreadDynamicData* const pTDD = EA::Thread::FindThreadDynamicData(id);
	if(pTDD)
	{   
		return pTDD->mThreadId;
	}

	return EA::Thread::kThreadIdInvalid;
}

int EA::Thread::GetThreadPriority()
{
	int         policy;
	sched_param param;
	SysThreadId currentThreadId = scePthreadSelf();

	int result = scePthreadGetschedparam(currentThreadId, &policy, &param);
	if(result == SCE_OK)
	{
		// Kettle pthreads uses a reversed interpretation of sched_get_priority_min and sched_get_priority_max.
		return -1 * (param.sched_priority - SCE_KERNEL_PRIO_FIFO_DEFAULT);
	}

	return kThreadPriorityDefault;
}


bool EA::Thread::SetThreadPriority(int nPriority)
{
	SysThreadId             currentThreadId = scePthreadSelf();
	int                     policy;
	SceKernelSchedParam     param;
	int                     result = -1;

	EAT_ASSERT(nPriority != kThreadPriorityUnknown);
	 
	result = scePthreadGetschedparam(currentThreadId, &policy, &param);
	if(result == SCE_OK)
	{
		// Kettle pthreads uses a reversed interpretation of sched_get_priority_min and sched_get_priority_max.
		const int nMin = SCE_KERNEL_PRIO_FIFO_HIGHEST;
		const int nMax = SCE_KERNEL_PRIO_FIFO_LOWEST;

		param.sched_priority = (SCE_KERNEL_PRIO_FIFO_DEFAULT + (-1 * nPriority));

		// Clamp to min/max as appropriate for current scheduling policy
		if(param.sched_priority < nMin)
			param.sched_priority = nMin;
		else if(param.sched_priority > nMax)
			param.sched_priority = nMax;

		result = scePthreadSetprio(currentThreadId, param.sched_priority);
	}
		 
	return (result == SCE_OK);
}


void* EA::Thread::GetThreadStackBase()
{
	void* pStackAddr = NULL;
	int result;

	ScePthreadAttr attr;
	result = scePthreadAttrInit(&attr); 
	EAT_ASSERT(SCE_OK == result);
	result = scePthreadAttrGet(scePthreadSelf(), &attr);
	EAT_ASSERT(SCE_OK == result);
	result = scePthreadAttrGetstackaddr(&attr, &pStackAddr); 
	EAT_ASSERT(SCE_OK == result);
	result = scePthreadAttrDestroy(&attr); 
	EAT_ASSERT(SCE_OK == result);
	EA_UNUSED(result);

	return pStackAddr; 
}

namespace
{
	SceKernelCpumask GetSceKernelAllCpuMask()
	{
	#if (SCE_ORBIS_SDK_VERSION >= 0x03000000u)
		return (EA::Thread::GetProcessorCount() == 6) ? SCE_KERNEL_CPUMASK_6CPU_ALL : SCE_KERNEL_CPUMASK_7CPU_ALL;
	#else
		nAffinityMask &= 0x3f;
	#endif
	}
}


void EA::Thread::SetThreadProcessor(int nProcessor)
{
	SceKernelCpumask mask = GetSceKernelAllCpuMask();
	if (nProcessor >= 0)
		mask = (SceKernelCpumask)(1 << nProcessor);

	int result = scePthreadSetaffinity(scePthreadSelf(), mask);
	EA_UNUSED(result);
	EAT_ASSERT(SCE_OK == result);
}

int EA::Thread::GetThreadProcessor()
{
	return sceKernelGetCurrentCpu();
}

EATHREADLIB_API void EA::Thread::SetThreadAffinityMask(const EA::Thread::ThreadId& id, ThreadAffinityMask nAffinityMask)
{
	// Update the affinity mask in the thread dynamic data cache.
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	if(pTDD)
	{
		pTDD->mnThreadAffinityMask = nAffinityMask;
	}

#if EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
	nAffinityMask &= GetSceKernelAllCpuMask();
	int res = scePthreadSetaffinity(GetSysThreadId(id), static_cast<SceKernelCpumask>(nAffinityMask));
	EAT_ASSERT(SCE_OK == res);
	EA_UNUSED(res);
#endif
}

EATHREADLIB_API EA::Thread::ThreadAffinityMask EA::Thread::GetThreadAffinityMask(const EA::Thread::ThreadId& id)
{ 
	// Update the affinity mask in the thread dynamic data cache.
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	if(pTDD)
	{
		return pTDD->mnThreadAffinityMask;
	}

	return kThreadAffinityMaskAny;
}

namespace Internal
{
	void SetThreadName(EAThreadDynamicData* pTDD)
	{
		if(pTDD)
		{
			EAT_COMPILETIME_ASSERT(EATHREAD_NAME_SIZE == 32); // New name (up to 32 bytes including the NULL terminator), or NULL due to Sony OS constraint
			char buf[EATHREAD_NAME_SIZE];
			snprintf(buf, sizeof(buf), "%s", pTDD->mName);
			buf[EATHREAD_NAME_SIZE - 1] = 0;

			auto sceResult = scePthreadRename(pTDD->mSysThreadId, buf);
			EA_UNUSED(sceResult);
			EAT_ASSERT(SCE_OK == sceResult);
		}
	}
};

EATHREADLIB_API void EA::Thread::SetThreadName(const char* pName) { SetThreadName(GetThreadId(), pName); }
EATHREADLIB_API const char* EA::Thread::GetThreadName() { return GetThreadName(GetThreadId()); }

EATHREADLIB_API void EA::Thread::SetThreadName(const EA::Thread::ThreadId& id, const char* pName)
{
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	if (pTDD)
	{
		strncpy(pTDD->mName, pName, EATHREAD_NAME_SIZE);
		pTDD->mName[EATHREAD_NAME_SIZE - 1] = 0;

		Internal::SetThreadName(pTDD);
	}
}

EATHREADLIB_API const char* EA::Thread::GetThreadName(const EA::Thread::ThreadId& id)
{ 
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	return pTDD ? pTDD->mName : "";
}

int EA::Thread::GetProcessorCount() 
{ 
#if (SCE_ORBIS_SDK_VERSION >= 0x03000000u)
	return sceKernelGetCpumode() == SCE_KERNEL_CPUMODE_6CPU ? 6 : 7; 
#else
	return 6;
#endif
}

void EA::Thread::ThreadSleep(const ThreadTime& timeRelative)
{
	if(timeRelative == kTimeoutImmediate)
	{
		scePthreadYield();
	}
	else
	{
		SceKernelTimespec ts;
		static const double MILLISECONDS_TO_NANOSECONDS = 1000000.0;
		static const uint64_t SECONDS_TO_NANOSECONDS = 1000000000;

		// make sure we compute this with doubles then uint64_t or we will run out of bits precision
		uint64_t timeNanoSeconds = (uint64_t)(MILLISECONDS_TO_NANOSECONDS * timeRelative);
		ts.tv_sec = timeNanoSeconds / SECONDS_TO_NANOSECONDS;
		ts.tv_nsec = static_cast<long>(timeNanoSeconds % SECONDS_TO_NANOSECONDS);  // converting from milliseconds to nanoseconds.


		// Determine which TimerQueue to use. Timer Queues are used to allow for higher resolution sleeps
		EAThreadTimerQueue* pThreadTimerQueue = nullptr;
		if (EA_UNLIKELY(tpThreadDynamicData == nullptr))
		{
			// This is either the first time an EAThread thread has called ThreadSleep or we are calling ThreadSleep
			// from a non-eathread function. Find the ThreadDynamicData which houses the TimerQueue and if not present (we are 
			// using a non-eathread) grab the global instance instead.
			tpThreadDynamicData = EA::Thread::FindThreadDynamicData(EA::Thread::GetThreadId());
			if (tpThreadDynamicData)
			{
				pThreadTimerQueue = &tpThreadDynamicData->mThreadTimerQueue;
			}
			else
			{
				pThreadTimerQueue = &gThreadTimerQueue;
			}
		}
		else
		{
			pThreadTimerQueue = &tpThreadDynamicData->mThreadTimerQueue;
		}
		
		// Timer queues may only accept sleep values between 100 microseconds and while we guarantee pThreadTimerQueue will
		// not be null, we must ensure it has been enabled since it may fail in two uncommon ways:
		// 1. The underlying Sony Queue failed to initialize (such as too many queues currently being created) 
		// 2. This function (ThreadSleep) is called during static initialization and due to static initialization order
		//    we haven't had a chance to initialize the global static EAThreadTimerQueue instance
		if (EA_LIKELY((timeNanoSeconds < (SECONDS_TO_NANOSECONDS * 100)) && pThreadTimerQueue->mbEnabled))
		{
			const long kMinTimeForTimerEventNanoSeconds = 100000; // 100 microseconds represented in nanoseconds
			ts.tv_nsec = EA_UNLIKELY((ts.tv_nsec < kMinTimeForTimerEventNanoSeconds) && (ts.tv_sec != 0)) ?
				kMinTimeForTimerEventNanoSeconds : ts.tv_nsec;

			// it's ok to submit negative ids to the queue in the event that mCurrentId wraps around
			int result = sceKernelAddHRTimerEvent(pThreadTimerQueue->mTimerEventQueue, (int)pThreadTimerQueue->mCurrentId++, &ts, nullptr);
			EA_UNUSED(result);
			EAT_ASSERT_FORMATTED(result == SCE_OK, "sceKernelAddHRTimerEvent returned an error (0x%08x)", result);

			int out;
			SceKernelEvent ev;
			result = sceKernelWaitEqueue(pThreadTimerQueue->mTimerEventQueue, &ev, 1, &out, nullptr);
			EAT_ASSERT_FORMATTED(result == SCE_OK, "sceKernelWaitEqueue returned an error (0x%08x)", result);
		}
		else
		{
			int result = sceKernelNanosleep(&ts, 0);
			EA_UNUSED(result);
			EAT_ASSERT_MSG(result == SCE_OK, "sceKernelNanosleep returned an error");
		}
	}
}


namespace EA 
{ 
	namespace Thread 
	{ 
		EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId);     
	}
}

void EA::Thread::ThreadEnd(intptr_t threadReturnValue)
{
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(GetThreadId());

	if(pTDD)
	{
		pTDD->mnStatus = Thread::kStatusEnded;
		pTDD->mnReturnValue = threadReturnValue;
		pTDD->mRunMutex.Unlock();
		pTDD->Release();
	}

	scePthreadExit((void*)threadReturnValue);
}

EA::Thread::ThreadTime EA::Thread::GetThreadTime()
{    
	SceKernelTimespec ts;
	sceKernelClockGettime(SCE_KERNEL_CLOCK_MONOTONIC, &ts);
	ThreadTime ret = EA_TIMESPEC_AS_DOUBLE_IN_MS(ts);
	return ret;
}


void EA::Thread::SetAssertionFailureFunction(EA::Thread::AssertionFailureFunction pAssertionFailureFunction, void* pContext)
{
	gpAssertionFailureFunction = pAssertionFailureFunction;
	gpAssertionFailureContext  = pContext;
}


void EA::Thread::AssertionFailure(const char* pExpression)
{
	if(gpAssertionFailureFunction)
		gpAssertionFailureFunction(pExpression, gpAssertionFailureContext);
	else
	{
		#if EAT_ASSERT_ENABLED
			// Todo.
		#endif
	}
}


EA::Thread::SysThreadId EA::Thread::GetSysThreadId(EA::Thread::ThreadId id)
{
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	if (pTDD)
	{
		return pTDD->mSysThreadId;
	}

	return kSysThreadIdInvalid;
}

EA::Thread::SysThreadId EA::Thread::GetSysThreadId()
{
	return scePthreadSelf();
}











