///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>

#include "eathread/eathread.h"
#include "eathread/eathread_thread.h"

#include <cstring>
#include <sstream>
#include <type_traits>

namespace EA
{
	namespace Thread
	{
		EA::Thread::AssertionFailureFunction gpAssertionFailureFunction = NULL;
		void*                                gpAssertionFailureContext  = NULL;

		EATHREADLIB_API ThreadId EA::Thread::GetThreadId()
		{
			return std::this_thread::get_id();
		}

		EATHREADLIB_API ThreadId EA::Thread::GetThreadId(EA::Thread::SysThreadId id)
		{
			EAThreadDynamicData* const pTDD = EA::Thread::FindThreadDynamicData(id);
			if(pTDD)
			{   
				return pTDD->mpComp->mThread.get_id();
			}

			return EA::Thread::kThreadIdInvalid;
		}

		EATHREADLIB_API SysThreadId EA::Thread::GetSysThreadId(ThreadId threadId)
		{
			EAThreadDynamicData* tdd = EA::Thread::FindThreadDynamicData(threadId);
			if (tdd && tdd->mpComp)
				return tdd->mpComp->mThread.native_handle();

			ThreadId threadIdCurrent = GetThreadId();
			if(threadId == threadIdCurrent)
			{
				#if defined(EA_PLATFORM_MICROSOFT)
					std::thread::id stdId = std::this_thread::get_id();
					EAT_COMPILETIME_ASSERT(sizeof(_Thrd_t) == sizeof(std::thread::id));
					return ((_Thrd_t&)stdId)._Hnd;
				#elif EA_POSIX_THREADS_AVAILABLE && defined(_YVALS)
					std::thread::id stdId = std::this_thread::get_id();
					EAT_COMPILETIME_ASSERT(sizeof(_Thrd_t) == sizeof(std::thread::id));
					return reinterpret_cast<_Thrd_t>(stdId);
				#else
					#error Platform not supported yet.
				#endif
			}

			EAT_ASSERT_MSG(false, "Failed to find associated EAThreadDynamicData for this thread.\n");
			return SysThreadId();
		}

		EATHREADLIB_API SysThreadId EA::Thread::GetSysThreadId()
		{
			// There currently isn't a means to directly get the current SysThreadId, so we do it indirectly:
			return GetSysThreadId(std::this_thread::get_id());
		}

		EATHREADLIB_API ThreadTime EA::Thread::GetThreadTime()
		{
			using namespace std::chrono;
			auto nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
			return nowMs.count();
		}

		EATHREADLIB_API int GetThreadPriority()
		{
			// No way to query or set thread priority through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs
			return kThreadPriorityDefault;
		}

		EATHREADLIB_API bool SetThreadPriority(int nPriority)
		{
			// No way to query or set thread priority through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs
			return false;
		}

		EATHREADLIB_API void SetThreadProcessor(int nProcessor)
		{
			// No way to query or set thread processor through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs
		}

		EATHREADLIB_API int GetThreadProcessor()
		{
			// No way to query or set thread processor through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs
			return 0;
		}

		EATHREADLIB_API int GetProcessorCount()
		{
			return static_cast<int>(std::thread::hardware_concurrency());
		}

		EATHREADLIB_API void ThreadSleep(const ThreadTime& timeRelative)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(timeRelative));
		}

		void ThreadEnd(intptr_t threadReturnValue)
		{
			// No way to end a thread through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs
			EAT_ASSERT_MSG(false, "ThreadEnd is not implemented for C++11 threads.\n");
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
				// Call the Windows library function.
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

		EATHREADLIB_API void SetAssertionFailureFunction(AssertionFailureFunction pAssertionFailureFunction, void* pContext)
		{
			gpAssertionFailureFunction = pAssertionFailureFunction;
			gpAssertionFailureContext  = pContext;
		}

		EATHREADLIB_API void AssertionFailure(const char* pExpression)
		{
			if(gpAssertionFailureFunction)
				gpAssertionFailureFunction(pExpression, gpAssertionFailureContext);
		}

		void* GetThreadStackBase()
		{
			return nullptr;
		}

		// This can be removed once all remaining synchronization primitives are implemented in terms of C++11 APIs
		uint32_t EA::Thread::RelativeTimeoutFromAbsoluteTimeout(ThreadTime timeoutAbsolute)
		{
			EAT_ASSERT((timeoutAbsolute == kTimeoutImmediate) || (timeoutAbsolute > EATHREAD_MIN_ABSOLUTE_TIME)); // Assert that the user didn't make the mistake of treating time as relative instead of absolute.

			DWORD timeoutRelative = 0;

			if (timeoutAbsolute == kTimeoutNone)
			{
				timeoutRelative = 0xffffffff;
			}
			else if (timeoutAbsolute == kTimeoutImmediate)
			{
				timeoutRelative = 0;
			}
			else
			{
				ThreadTime timeCurrent(GetThreadTime());
				timeoutRelative = (timeoutAbsolute > timeCurrent) ? static_cast<DWORD>(timeoutAbsolute - timeCurrent) : 0;
			}

			EAT_ASSERT((timeoutRelative == 0xffffffff) || (timeoutRelative < 100000000)); // Assert that the timeout is a sane value and didn't wrap around.

			return timeoutRelative;
		}

		// Implement native_handle_type comparison as a memcmp() - may need platform specific implementations on some future platforms.
		bool Equals(const SysThreadId& a, const SysThreadId& b)
		{
			static_assert((std::is_fundamental<SysThreadId>::value || std::is_pointer<SysThreadId>::value || std::is_pod<SysThreadId>::value), 
				"SysThreadId should be comparable using memcmp()");
			return memcmp(&a, &b, sizeof(SysThreadId)) == 0;
		}

		namespace detail
		{
			// Override the default EAThreadToString implementation
			#define EAThreadIdToString_CUSTOM_IMPLEMENTATION
			ThreadIdToStringBuffer::ThreadIdToStringBuffer(EA::Thread::ThreadId threadId)
			{
				std::stringstream formatStream;
				formatStream << threadId;
				strncpy(mBuf, formatStream.str().c_str(), BufSize - 1);
				mBuf[BufSize - 1] = '\0';
			}

			SysThreadIdToStringBuffer::SysThreadIdToStringBuffer(EA::Thread::SysThreadId sysThreadId)
			{
				strncpy(mBuf, "Unknown", BufSize - 1);
				mBuf[BufSize - 1] = '\0';
			}
		}
	}
}

