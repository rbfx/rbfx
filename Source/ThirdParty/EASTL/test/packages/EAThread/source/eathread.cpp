///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/internal/config.h>
#include <eathread/eathread.h>
#include <stdarg.h>
#include <stdio.h>


namespace EA
{
	namespace Thread
	{
		EA::Thread::Allocator* gpAllocator = NULL;

		EATHREADLIB_API void SetAllocator(Allocator* pEAThreadAllocator)
		{
			gpAllocator = pEAThreadAllocator;
		}

		EATHREADLIB_API Allocator* GetAllocator()
		{
			return gpAllocator;
		}



		// Currently we take advantage of the fact that ICoreAllocator
		// is a binary mapping to EA::Thread::Allocator.
		// To do: We need to come up with a better solution that this, 
		//        as it is not future-safe and not even guaranteed to
		//        be portable. The problem is that we can't make this
		//        package dependent on the CoreAllocator package without
		//        breaking users who aren't using it.

		EATHREADLIB_API void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
		{
			gpAllocator = (EA::Thread::Allocator*)(uintptr_t)pCoreAllocator;
		}

		EATHREADLIB_API void SetThreadAffinityMask(ThreadAffinityMask nAffinityMask)
		{ 
			EA::Thread::SetThreadAffinityMask(GetThreadId(), nAffinityMask);
		}

		EATHREADLIB_API ThreadAffinityMask GetThreadAffinityMask()
		{
			return GetThreadAffinityMask(GetThreadId());
		}
	}
}

#if !EA_THREADS_AVAILABLE
	//  Do nothing
#elif EA_USE_CPP11_CONCURRENCY
	#include "cpp11/eathread_cpp11.cpp"
#elif defined(EA_PLATFORM_SONY)
   #include "kettle/eathread_kettle.cpp"
#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
   #include "unix/eathread_unix.cpp"
#elif defined(EA_PLATFORM_MICROSOFT)
   #include "pc/eathread_pc.cpp"
#endif

namespace EA
{
	namespace Thread
	{
		namespace detail
		{
			#if !defined(EAThreadIdToString_CUSTOM_IMPLEMENTATION)
				ThreadIdToStringBuffer::ThreadIdToStringBuffer(EA::Thread::ThreadId threadId)
				{
					sprintf(mBuf, "%d", (int)(intptr_t)threadId);
				}

				SysThreadIdToStringBuffer::SysThreadIdToStringBuffer(EA::Thread::SysThreadId sysThreadId)
				{
					sprintf(mBuf, "%d", (int)(intptr_t)sysThreadId);
				}
			#endif
		}
	}
}

#if defined(EA_PLATFORM_ANDROID)
	#if EATHREAD_C11_ATOMICS_AVAILABLE == 0
		#include "android/eathread_fake_atomic_64.cpp"
	#endif
#endif

#if !defined(EAT_ASSERT_SNPRINTF)
	#if defined(EA_PLATFORM_MICROSOFT)
		#define EAT_ASSERT_SNPRINTF _vsnprintf
	#else
		#define EAT_ASSERT_SNPRINTF snprintf
	#endif
#endif

	void EA::Thread::AssertionFailureV(const char* pFormat, ...)
	{
		const size_t kBufferSize = 512;
		char buffer[kBufferSize];

		va_list arguments;
		va_start(arguments, pFormat);
		const int nReturnValue = EAT_ASSERT_SNPRINTF(buffer, kBufferSize, pFormat, arguments);
		va_end(arguments);

		if(nReturnValue > 0)
		{
			buffer[kBufferSize - 1] = 0;
			AssertionFailure(buffer);
		}
	}

///////////////////////////////////////////////////////////////////////////////
// non-threaded implementation
///////////////////////////////////////////////////////////////////////////////

#if !EA_THREADS_AVAILABLE

	#include <stdio.h>
	#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
		#include <sched.h>
		#include <sys/time.h>
	#elif defined(EA_PLATFORM_WINDOWS)
		extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
	#endif


	namespace EA
	{
		namespace Thread
		{
			// Assertion variables.
			EA::Thread::AssertionFailureFunction gpAssertionFailureFunction = NULL;
			void*                                gpAssertionFailureContext  = NULL;
		}
	}

	EA::Thread::ThreadId EA::Thread::GetThreadId()
	{
		 return 1;
	}


	int EA::Thread::GetThreadPriority()
	{
		return kThreadPriorityDefault;
	}


	bool EA::Thread::SetThreadPriority(int nPriority)
	{
		return true;
	}


	void* EA::Thread::GetThreadStackBase()
	{
		return NULL;
	}


	void EA::Thread::SetThreadProcessor(int /*nProcessor*/)
	{
	}


	int EA::Thread::GetThreadProcessor()
	{
		return 0;
	}


	int EA::Thread::GetProcessorCount()
	{
		return 1;
	}


	void EA::Thread::ThreadSleep(const ThreadTime& timeRelative)
	{
		#if defined(EA_PLATFORM_WINDOWS)

			// There is no nanosleep on Windows, but there is Sleep.
			if(timeRelative == kTimeoutImmediate)
				Sleep(0);
			else
				Sleep((unsigned)((timeRelative.tv_sec * 1000) + (((timeRelative.tv_nsec % 1000) * 1000000))));

		#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE

			if(timeRelative == kTimeoutImmediate)
				sched_yield();
			else
				nanosleep(&timeRelative, 0);

		#endif
	}


	void EA::Thread::ThreadEnd(intptr_t /*threadReturnValue*/)
	{
		// We could possibly call exit here.
	}


	EA::Thread::ThreadTime EA::Thread::GetThreadTime()
	{
		#if defined(EA_PLATFORM_WINDOWS)

			return (ThreadTime)GetTickCount();

		#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE

			#if defined(EA_PLATFORM_LINUX) || defined(__CYGWIN__) || (_POSIX_TIMERS > 0)
				ThreadTime threadTime;
				clock_gettime(CLOCK_REALTIME, &threadTime);  // If you get a linker error about clock_getttime, you need to link librt.a (specify -lrt to the linker).
				return threadTime;
			#else
				timeval temp;
				gettimeofday(&temp, NULL);
				return ThreadTime(temp.tv_sec, temp.tv_usec * 1000);    
			#endif

		#endif
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
				printf("EA::Thread::AssertionFailure: %s\n", pExpression);
			#endif
		}
	}


#endif // EA_THREADS_AVAILABLE

