///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EABase/eahave.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>


#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>
	#include <sched.h>
	#include <string.h>
	#ifdef EA_PLATFORM_WINDOWS
		#pragma warning(push, 0)
		#include <Windows.h> // Presumably we are using pthreads-win32.
		#pragma warning(pop)
		#include <time.h>
	#else
		#include <unistd.h>
		#if defined(_YVALS)
			#include <time.h>
		#else
			#include <sys/time.h>
		#endif

		#if defined(EA_PLATFORM_OSX) || defined(EA_PLATFORM_BSD)
			#include <sys/sysctl.h>
			#include <sys/param.h>
		#endif

		#if defined(EA_PLATFORM_LINUX)
			#include <sys/prctl.h>
		#endif

		#if defined(EA_PLATFORM_APPLE) || defined(__APPLE__)
			#include <dlfcn.h>
		#endif
	
		#if defined(EA_PLATFORM_BSD) || defined(EA_PLATFORM_CONSOLE_BSD) || defined(__FreeBSD__)
			#include <sys/param.h>
			#include <pthread_np.h>
		#endif

		#if defined(EA_PLATFORM_ANDROID)
			#include <sys/syscall.h>
		#endif
		
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
		return pthread_self();
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
		ThreadId    currentThreadId = pthread_self();

		int result = pthread_getschedparam(currentThreadId, &policy, &param);

		if(result == 0)
		{
			#if defined(EA_PLATFORM_LINUX) && !defined(__CYGWIN__)
				return kThreadPriorityDefault + param.sched_priority; // This works for both SCHED_OTHER, SCHED_RR, and SCHED_FIFO.
			#else
				#if defined(EA_PLATFORM_WINDOWS)
					if(param.sched_priority == THREAD_PRIORITY_NORMAL)
						return kThreadPriorityDefault;
				#elif !(defined(__CYGWIN__) || defined(CS_UNDEFINED_STRING))
					if(policy == SCHED_OTHER)
						return 0; // 0 is the only native priority permitted with the SCHED_OTHER scheduling scheme.
				#endif

				// The following needs to be tested on a Unix-by-Unix case. 
				const int nMin = sched_get_priority_min(policy);
				const int nMax = sched_get_priority_max(policy);

				// Some implementations of Pthreads associate higher priorities with smaller
				// integer values. We hide this. To the user, a higher value must always
				// indicate higher priority.
				const int adjustDir          = (nMin < nMax) ? 1 : -1;
				const int nativeBasePriority = (nMin + nMax) / 2;

				// EAThread_user_priority = +/-(native_priority - EAThread_native_priority_default)
				return adjustDir * (param.sched_priority - nativeBasePriority);
			#endif
		}

		return kThreadPriorityDefault;
	}


	bool EA::Thread::SetThreadPriority(int nPriority)
	{
		ThreadId    currentThreadId = pthread_self();
		int         policy;
		sched_param param;
		int         result = -1;

		EAT_ASSERT(nPriority != kThreadPriorityUnknown);

		#if defined(EA_PLATFORM_LINUX) && !defined(__CYGWIN__)
			// We are assuming Kernel 2.6 and later behavior, but perhaps we should dynamically detect.
			// Linux supports three scheduling policies SCHED_OTHER, SCHED_RR, and SCHED_FIFO. 
			// The process needs to be run with superuser privileges to use SCHED_RR or SCHED_FIFO.
			// Thread priorities for SCHED_OTHER do not exist; there is only one allowed thread priority: 0.
			// Thread priorities for SCHED_RR and SCHED_FIFO are limited to the range of [1, 99] (verified with Linux 2.6.17), 
			// despite documentation on the Internet that refers to ranges of 0-99, 1-100, 1-140, etc.
			// Higher values in this range mean higher priority.
			// All of the SCHED_RR and SCHED_FIFO privileges are higher than anything running at SCHED_OTHER,
			// as they are considered to be real-time scheduling. A result of this is that there is no
			// such thing as having a thread of lower priority than normal; there are only higher real-time priorities.
			if(nPriority <= kThreadPriorityDefault)
			{
				policy = SCHED_OTHER;
				param.sched_priority = 0;
			}
			else
			{
				policy = SCHED_RR;
				param.sched_priority = (nPriority - kThreadPriorityDefault);
			}

			result = pthread_setschedparam(currentThreadId, policy, &param);
		#else
			// The following needs to be tested on a Unix-by-Unix case. 
			result = pthread_getschedparam(currentThreadId, &policy, &param);

			if(result == 0)
			{
				// Cygwin does not support any scheduling policy other than SCHED_OTHER.
				#if !defined(__CYGWIN__)
					if(policy == SCHED_OTHER)
						policy = SCHED_FIFO;
				#endif

				int nMin      = sched_get_priority_min(policy);
				int nMax      = sched_get_priority_max(policy);
				int adjustDir = 1;

				// Some implementations of pthreads associate higher priorities with smaller integer values.
				// To the EAThread user, a higher value indicates a higher priority.
				if (nMin > nMax)
				{
					adjustDir = nMax;
					nMax      = nMin;
					nMin      = adjustDir;
					adjustDir = -1; // Translate user's desire for higher priority into a native lower value.
				}

				// native_priority = EAThread_native_priority_default +/- EAThread_user_priority.
				// This calculation sets the default to be in the middle of low and high, which might not be so for all platforms in practice.
				param.sched_priority = ((nMin + nMax) / 2) + (adjustDir * nPriority);

				// Clamp to min/max as appropriate for current scheduling policy
				if(param.sched_priority < nMin)
					param.sched_priority = nMin;
				else if(param.sched_priority > nMax)
					param.sched_priority = nMax;

				result = pthread_setschedparam(currentThreadId, policy, &param);
			}
		#endif

		return (result == 0);
	}


	void* EA::Thread::GetThreadStackBase()
	{
		#if defined(EA_PLATFORM_APPLE)
			pthread_t threadId = pthread_self();
			return pthread_get_stackaddr_np(threadId);

		#elif (EA_PLATFORM_SOLARIS)
			stack_t s;
			thr_stksegment(&s);
			return s.ss_sp;  // Note that this is not the sp pointer (which would refer to the a location low in the stack address space). When returned by thr_stksegment(), ss_sp refers to the top (base) of the stack.

		#elif defined(__CYGWIN__)
			// Cygwin reserves pthread_attr_getstackaddr and pthread_attr_getstacksize for future use.
			// The solution here is probably to use the Windows implementation of this here.
			return 0;

		#else // Other Unix
			void*     stackLow = NULL;
			size_t    stackSize = 0;
			pthread_t threadId = pthread_self();

			pthread_attr_t sattr;
			pthread_attr_init(&sattr);

			#if defined(EA_PLATFORM_BSD) || defined(EA_PLATFORM_CONSOLE_BSD) || defined(__FreeBSD__)
				pthread_attr_get_np(threadId, &sattr);
			#elif defined(EA_HAVE_pthread_getattr_np_DECL)
				// Note: this function is non-portable; various Unix systems may have different np alternatives
				pthread_getattr_np(threadId, &sattr);
			#else
				EA_UNUSED(threadId);
				// What to do?
			#endif

			// See http://www.opengroup.org/onlinepubs/009695399/functions/pthread_attr_getstack.html
			// stackLow is a constant. It is not the current low location but rather is the lowest allowed location.
			
			pthread_attr_getstack(&sattr, &stackLow, &stackSize);
			pthread_attr_destroy(&sattr);

			return (char*)stackLow + stackSize;
		#endif
	}


	void EA::Thread::SetThreadProcessor(int nProcessor)
	{
		// Posix threading doesn't have the ability to set the processor.
		#if defined(EA_PLATFORM_WINDOWS)
			static int nProcessorCount = 0; // This doesn't really need to be an atomic integer.

			if(nProcessorCount == 0)
			{
				SYSTEM_INFO systemInfo;
				memset(&systemInfo, 0, sizeof(systemInfo));
				GetSystemInfo(&systemInfo);
				nProcessorCount = (int)systemInfo.dwNumberOfProcessors;
			}

			DWORD dwThreadAffinityMask;

			if((nProcessor < 0) || (nProcessor >= nProcessorCount))
				dwThreadAffinityMask = 0xffffffff;
			else
				dwThreadAffinityMask = 1 << nProcessor;
			SetThreadAffinityMask(GetCurrentThread(), dwThreadAffinityMask);

		#elif (defined(EA_PLATFORM_LINUX) && !defined(EA_PLATFORM_ANDROID)) || defined(CS_UNDEFINED_STRING)
			cpu_set_t cpus;
			CPU_ZERO(&cpus);
			CPU_SET(nProcessor, &cpus);

			for (int c = 0; c < EA::Thread::GetProcessorCount(); c++, nProcessor >>= 1)
			{
				if (nProcessor & 1)
				{
					CPU_SET(c, &cpus);
				}
			}

			// To consider: Make it so we return a value.
			/*int result =*/ pthread_setaffinity_np(pthread_self(), sizeof(cpus), &cpus);
			// We don't assert on the success, as that could be very noisy for some users.

		#else
			// Other Unix platforms don't provide a means to specify what processor a thread runs on. 
			// You have no choice but to let the OS schedule threads for you.
			EA_UNUSED(nProcessor);
		#endif
	}


	#if defined(EA_PLATFORM_WINDOWS) && defined(EA_PROCESSOR_X86) && defined(MSC_VER) && (MSC_VER >= 1400)
		int GetCurrentProcessorNumberXP()
		{
			_asm { mov eax, 1   }
			_asm { cpuid        }
			_asm { shr ebx, 24  }
			_asm { mov eax, ebx }
		}
	#endif


	int EA::Thread::GetThreadProcessor()
	{
		#if defined(EA_PLATFORM_WINDOWS)
			// We are using Posix threading on Windows. It happens to be mapped to Windows threading and
			// so we can use Windows facilities to tell what processor the thread is running on.
			// Only Windows Vista and later provides GetCurrentProcessorNumber.
			// So we must dynamically link to this function.
			static EA_THREAD_LOCAL  bool           bInitialized = false;
			static EA_THREAD_LOCAL  DWORD (WINAPI *pfnGetCurrentProcessorNumber)() = NULL;

			if(!bInitialized)
			{
				HMODULE hKernel32 = GetModuleHandle("KERNEL32.DLL");
				if(hKernel32)
					pfnGetCurrentProcessorNumber = (DWORD (WINAPI*)())GetProcAddress(hKernel32, "GetCurrentProcessorNumber");
				bInitialized = true;
			}

			if(pfnGetCurrentProcessorNumber)
				return (int)(unsigned)pfnGetCurrentProcessorNumber();

			#if defined(EA_PLATFORM_WINDOWS) && defined(EA_PROCESSOR_X86) && defined(MSC_VER) && (MSC_VER >= 1400)
				return GetCurrentProcessorNumberXP();
			#else
				return 0;
			#endif

		#elif defined(EA_PLATFORM_ANDROID)
				// return zero until Google provides a alternative to smp_processor_id() 
				return 0;

		#elif EA_VALGRIND_ENABLED
			// Valgrind does not support the sched_getcpu() vsyscall.  It causes it to detect a segfault in the program and stop it.
			// https://bugs.kde.org/show_bug.cgi?id=187043
			// http://git.dorsal.polymtl.ca/?p=ust.git;a=commitdiff_plain;h=8f09cb9340387a52b483752c5d2d6c36035b26bc
			return 0;

		#elif (defined(EA_PLATFORM_LINUX) && (defined(EATHREAD_GLIBC_VERSION) && (EATHREAD_GLIBC_VERSION > 2005)))
			// http://www.kernel.org/doc/man-pages/online/pages/man3/sched_getcpu.3.html
			// http://www.kernel.org/doc/man-pages/online/pages/man2/getcpu.2.html
			// Another solution is to use the cpuid instruction like we do for Windows.
			int cpu = sched_getcpu();
			if(cpu < 0)
				cpu = 0;

			if(cpu >= 0)
				return cpu;

			// Ideally we would never need to execute the following code:
			cpu_set_t cpus;
			CPU_ZERO(&cpus);
			pthread_getaffinity_np(pthread_self(), sizeof(cpus), &cpus);
			
			for(int i = 0; i < CPU_SETSIZE; i++)
			{
				if(CPU_ISSET(i, &cpus))
					return i;
			}
			
			return 0;
		#else
			return 0;
		#endif
	}

	EATHREADLIB_API void EA::Thread::SetThreadAffinityMask(const EA::Thread::ThreadId& id, ThreadAffinityMask nAffinityMask)
	{
		EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
		if(pTDD)
		{
			pTDD->mnThreadAffinityMask = nAffinityMask;
	
			#if EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
				cpu_set_t cpuSetMask;
				memset(&cpuSetMask, 0, sizeof(cpu_set_t));

				for (int c = 0; c < EA::Thread::GetProcessorCount(); c++, nAffinityMask >>= 1)
				{
					if (nAffinityMask & 1)
					{
						CPU_SET(c, &cpuSetMask);
					}
				}
				
					sched_setaffinity(pTDD->mThreadPid, sizeof(cpu_set_t), &cpuSetMask);
			#endif
		}
	}
	
	
	EATHREADLIB_API EA::Thread::ThreadAffinityMask EA::Thread::GetThreadAffinityMask(const EA::Thread::ThreadId& id)
	{ 
		EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
		if(pTDD)
		{
			return pTDD->mnThreadAffinityMask;
		}
	
		return kThreadAffinityMaskAny;
	}

	// Internal SetThreadName API's so we don't repeat the implementations
	namespace Internal
	{
		// This function is not currently used if the thread name can be set from any other thread
		#if !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED

			void SetCurrentThreadName(const char8_t* pName)
			{
				#if defined(EA_PLATFORM_LINUX)
					// http://manpages.courier-mta.org/htmlman2/prctl.2.html
					// The Linux documentation says PR_SET_NAME sets the process name, but that 
					// documentation is wrong and instead it sets the current thread name.

					// Also: http://0pointer.de/blog/projects/name-your-threads.html
					// Stefan Kost recently pointed me to the fact that the Linux system call prctl(PR_SET_NAME) 
					// does not in fact change the process name, but the task name (comm field) -- in contrast 
					// to what the man page suggests. That makes it very useful for naming threads, since you 
					// can read back the name you set with PR_SET_NAME earlier from the /proc file system 
					// (/proc/$PID/task/$TID/comm on newer kernels, /proc/$PID/task/$TID/stat's second field 
					// on older kernels), and hence distinguish which thread might be responsible for the high 
					// CPU load or similar problems.
					char8_t nameBuf[16]; // Limited to 16 bytes, null terminated if < 16 bytes
					strncpy(nameBuf, pName, sizeof(nameBuf));
					nameBuf[15] = 0;
					prctl(PR_SET_NAME, (unsigned long)nameBuf, 0, 0, 0);

				#elif defined(EA_PLATFORM_APPLE) || defined(__APPLE__)
					// http://src.chromium.org/viewvc/chrome/trunk/src/base/platform_thread_mac.mm?revision=49465&view=markup&pathrev=49465
					// "There's a non-portable function for doing this: pthread_setname_np.
					// It's supported by OS X >= 10.6 and the Xcode debugger will show the thread
					// names if they're provided."
					// On OSX the return value is always -1 on error; use errno to tell the error value.
					typedef int (*pthread_setname_np_type)(const char*);
					pthread_setname_np_type pthread_setname_np_ptr = (pthread_setname_np_type)(uintptr_t)dlsym(RTLD_DEFAULT, "pthread_setname_np");

					if(pthread_setname_np_ptr)
					{
						// Mac OS X does not expose the length limit of the name, so hardcode it.
						char8_t nameBuf[63]; // It is not clear what the size limit actually is, though 63 is known to work because it was seen on the Internet.
						strncpy(nameBuf, pName, sizeof(nameBuf));
						nameBuf[62] = 0;
						pthread_setname_np_ptr(nameBuf);
					}

				#elif defined(EA_PLATFORM_BSD) || defined(EA_PLATFORM_CONSOLE_BSD) || defined(__FreeBSD__)
					// http://www.unix.com/man-page/freebsd/3/PTHREAD_SET_NAME_NP/
					pthread_set_name_np(pthread_self(), pName);
				#endif
			}

		#endif

		EA::Thread::ThreadId GetId(EAThreadDynamicData* pTDD) 
		{
			if(pTDD)
				return pTDD->mThreadId;

			return EA::Thread::kThreadIdInvalid;
		}

		void SetThreadName(EAThreadDynamicData* pTDD)
		{
			#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_APPLE)
				EAT_COMPILETIME_ASSERT(EATHREAD_OTHER_THREAD_NAMING_SUPPORTED == 0);
				// http://stackoverflow.com/questions/2369738/can-i-set-the-name-of-a-thread-in-pthreads-linux
				// Under some Unixes you can name only the current thread, so we apply the naming
				// only if the currently executing thread is the one that is associated with
				// this class object.
				if(GetId(pTDD) == EA::Thread::GetThreadId())
					SetCurrentThreadName(pTDD->mName);

			#elif defined(EA_PLATFORM_BSD) 
				EAT_COMPILETIME_ASSERT(EATHREAD_OTHER_THREAD_NAMING_SUPPORTED == 1);
				// http://www.unix.com/man-page/freebsd/3/PTHREAD_SET_NAME_NP/
				if(GetId(pTDD) != EA::Thread::kThreadIdInvalid)
					pthread_set_name_np(GetId(pTDD), pTDD->mName);
					
			#endif
		}
	} // namespace Internal 



	EATHREADLIB_API void EA::Thread::SetThreadName(const char* pName) { SetThreadName(GetThreadId(), pName); }
	EATHREADLIB_API const char* EA::Thread::GetThreadName() { return GetThreadName(GetThreadId()); }

	EATHREADLIB_API void EA::Thread::SetThreadName(const EA::Thread::ThreadId& id, const char* pName)
	{
		EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
		if(pTDD)
		{
			if(pTDD->mName != pName) // self-assignment check
			{
				strncpy(pTDD->mName, pName, EATHREAD_NAME_SIZE);
				pTDD->mName[EATHREAD_NAME_SIZE - 1] = 0;
			}

			Internal::SetThreadName(pTDD);
		}
	}

	EATHREADLIB_API const char* EA::Thread::GetThreadName(const EA::Thread::ThreadId& id)
	{ 
		EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
		return pTDD ?  pTDD->mName : "";
	}


	int EA::Thread::GetProcessorCount()
	{
		#if defined(EA_PLATFORM_WINDOWS)
			static int nProcessorCount = 0; // This doesn't really need to be an atomic integer.

			if(nProcessorCount == 0)
			{
				// A better function to use would possibly be KeQueryActiveProcessorCount 
				// (NTKERNELAPI ULONG KeQueryActiveProcessorCount(PKAFFINITY ActiveProcessors))

				SYSTEM_INFO systemInfo;
				memset(&systemInfo, 0, sizeof(systemInfo));
				GetSystemInfo(&systemInfo);
				nProcessorCount = (int)systemInfo.dwNumberOfProcessors;
			}

			return nProcessorCount;

		#elif defined(EA_PLATFORM_OSX) || defined(EA_PLATFORM_BSD)
			// http://developer.apple.com/mac/library/documentation/Darwin/Reference/ManPages/man3/sysctlbyname.3.html
			// We can use:
			//     int sysctl(int* name, u_int namelen, void* oldp, size_t* oldlenp, void* newp, size_t newlen);
			//     int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

			#ifdef EA_PLATFORM_BSD 
			  int  mib[4] = { CTL_HW, HW_NCPU, 0, 0 };
			#else
			  int  mib[4] = { CTL_HW, HW_AVAILCPU, 0, 0 };
			#endif
			int    cpuCount = 0;    // Unfortunately, Apple's documentation fails to clarify if this needs to be 'int' or 'long'.
			size_t len = sizeof(cpuCount);

			sysctl(mib, 2, &cpuCount, &len, NULL, 0);

			if(cpuCount < 1) 
			{
				mib[1] = HW_NCPU;
				sysctl(mib, 2, &cpuCount, &len, NULL, 0);

				if(cpuCount < 1)
					cpuCount = 1;
			}

			return cpuCount;

			// Maybe simpler, should try it out to make sure it works:
			//
			// int cpuCount = 0;
			// size_t len = sizeof(cpuCount);
			// if(sysctlbyname("hw.ncpu", &cpuCount, &len, NULL, 0) != 0) 
			//     cpuCount = 1;
			// return cpuCount;
		#else
			// Posix doesn't provide a means to get this information.
			// Some Unixes provide sysconf() with the _SC_NPROCESSORS_ONLN or _SC_NPROCESSORS_CONF option.
			// Another option is to count the number of entries in /proc/cpuinfo
			#ifdef _SC_NPROCESSORS_ONLN
				return (int)sysconf(_SC_NPROCESSORS_ONLN);
			#else
				return 1;
			#endif
		#endif
	}


	#if defined(EA_PLATFORM_WINDOWS)
		extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
	#endif

	void EA::Thread::ThreadSleep(const ThreadTime& timeRelative)
	{
		#if defined(EA_PLATFORM_WINDOWS)
			// There is no nanosleep on Windows, but there is Sleep.
			if(timeRelative == kTimeoutImmediate)
				Sleep(0);
			else
				Sleep((unsigned)((timeRelative.tv_sec * 1000) + (((timeRelative.tv_nsec % 1000) * 1000000))));
		#else
			if(timeRelative == kTimeoutImmediate)
			{
				sched_yield();
			}
			else
			{
				#if defined(EA_HAVE_nanosleep_DECL)
					nanosleep(&timeRelative, 0);
				#else
					// What to do?
				#endif
			}
		#endif
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

		pthread_exit((void*)threadReturnValue);
	}


	#if defined(EA_PLATFORM_APPLE)
		EA::Thread::SysThreadId EA::Thread::GetSysThreadId(ThreadId id)
		{
			return pthread_mach_thread_np(id);
		}

		EA::Thread::SysThreadId EA::Thread::GetSysThreadId()
		{
			return pthread_mach_thread_np(pthread_self()); // There isn't a self-specific version of pthread_mach_thread_np.
		}
	#endif
	
	
	EA::Thread::ThreadTime EA::Thread::GetThreadTime()
	{
		#if defined(EA_PLATFORM_WINDOWS) && !defined(__CYGWIN__)
			// We use this code instead of GetTickCount or similar because pthreads under
			// Win32 uses the 'system file time' definition (e.g. GetSystemTimeAsFileTime()) 
			// for current time. The implementation here is just like that in the 
			// pthreads-Win32 ptw32_timespec.c file.
			int64_t ft;
			ThreadTime threadTime;
			GetSystemTimeAsFileTime((FILETIME*)&ft); // nTime64 is in intervals of 100ns.
			#define PTW32_TIMESPEC_TO_FILETIME_OFFSET (((int64_t)27111902 << 32) + (int64_t)3577643008)
			threadTime.tv_sec  = (int)((ft - PTW32_TIMESPEC_TO_FILETIME_OFFSET) / 10000000);
			threadTime.tv_nsec = (int)((ft - PTW32_TIMESPEC_TO_FILETIME_OFFSET - ((int64_t)threadTime.tv_sec * (int64_t)10000000)) * 100);
			return threadTime;

			// Alternative which will likely be slower:
			//#include <sys/timeb.h>
			//ThreadTime threadTime;
			//_timeb fTime; _ftime(&fTime);
			//threadTime.tv_sec  = (long)fTime.time;
			//threadTime.tv_nsec = fTime.millitm * 1000000;
			//return threadTime;
		#else
			// For some systems we may need to use gettimeofday() instead of clock_gettime().
			#if defined(EA_PLATFORM_LINUX) || defined(__CYGWIN__) || (_POSIX_TIMERS > 0)
				ThreadTime threadTime;
				clock_gettime(CLOCK_REALTIME, &threadTime);  // If you get a linker error about clock_getttime, you need to link librt.a (specify -lrt to the linker).
				return threadTime;
			#else
				timeval temp;
				gettimeofday(&temp, NULL);
				return ThreadTime((ThreadTime::seconds_t)temp.tv_sec, (ThreadTime::nseconds_t)temp.tv_usec * 1000);    
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
				#ifdef EA_PLATFORM_WINDOWS
					OutputDebugStringA("EA::Thread::AssertionFailure: ");
					OutputDebugStringA(pExpression);
					OutputDebugStringA("\n");
				#else
					printf("EA::Thread::AssertionFailure: ");
					printf("%s", pExpression);
					printf("\n");
					fflush(stdout);
					fflush(stderr);
				#endif

				EATHREAD_DEBUG_BREAK();
			#endif
		}
	}


#endif // defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE








