///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// eathread.h
//
// Created by Paul Pedriana, Maxis
//
// Provides some base global definitions for the EA::Thread library.
//
// Design
// Many of the design criteria for EA::Thread is based on the design of the 
// Posix threading standard. The Posix threading standard is designed to 
// work portably on a wide range of operating systems and hardware, including
// embedded systems and realtime environments. As such, Posix threads generally
// represent a competent model to follow where possible. Windows and various
// other platforms have independent multi-threading systems which are taken
// into account here as well. If something exists in Windows but doesn't 
// exist here (e.g. Thread suspend/resume), there is a decent chance that it 
// is by design and for some good reason.
// 
// C++
// There are a number of C++ libraries devoted to multithreading. Usually the 
// goal of these libraries is provide a platform independent interface which
// simplifies the most common usage patterns and helps prevent common errors.
// Some of these libraries are basic wrappers around existing C APIs while 
// others provide a new and different paradigm. We take the former approach
// here, as it is provides more or less the same functionality but provides 
// it in a straightforward way that is easily approached by those familiar 
// with platform-specific APIs. This approach has been referred to as the 
// "Wrapper Facade Pattern".
//
// Condition Variables
// Posix condition variables are implemented via the Condition class. Condition 
// is essentially the Java and C# name for Posix' condition variables. For some
// people, a condition variable may seem similar to a Win32 Signal. In actuality
// they are similar but there is one critical difference: a Signal does not 
// atomically unlock a mutex as part of the signaling process. This results in
// problematic race conditions that make reliable producer/consumer systems
// impossible to implement.
//
// Signals
// As of this writing, there isn't a Win32-like Signal class. The reason for this
// is that Semaphore does most or all the duty that Signal does and is a little
// more portable, given that Signals exist only on Win32 and not elsewhere.
//
// Timeouts
// Timeouts are specified as absolute times and not relative times. This may
// not be how Win32 threading works but it is what's proper and is how Posix
// threading works. From the OpenGroup online pthread documentation on this:
//     An absolute time measure was chosen for specifying the 
//     timeout parameter for two reasons. First, a relative time 
//     measure can be easily implemented on top of a function 
//     that specifies absolute time, but there is a race 
//     condition associated with specifying an absolute timeout 
//     on top of a function that specifies relative timeouts. 
//     For example, assume that clock_gettime() returns the 
//     current time and cond_relative_timed_wait() uses relative 
//     timeouts:
//            clock_gettime(CLOCK_REALTIME, &now);
//            reltime = sleep_til_this_absolute_time - now;
//            cond_relative_timed_wait(c, m, &reltime);
//     If the thread is preempted between the first statement and 
//     the last statement, the thread blocks for too long. Blocking, 
//     however, is irrelevant if an absolute timeout is used. 
//     An absolute timeout also need not be recomputed if it is used 
//     multiple times in a loop, such as that enclosing a condition wait.
//     For cases when the system clock is advanced discontinuously by 
//     an operator, it is expected that implementations process any 
//     timed wait expiring at an intervening time as if that time had 
//     actually occurred.
// 
// General Threads
// For detailed information about threads, it is recommended that you read
// various competent sources of information about multithreading and 
// multiprocessing.
//    Programming with POSIX(R) Threads, by David R. Butenhof
//    http://www.opengroup.org/onlinepubs/007904975/basedefs/pthread.h.html
//    usenet: comp.programming.threads
//    http://www.openmp.org/index.cgi?faq
//    http://www.lambdacs.com/cpt/MFAQ.html
//    http://www.lambdacs.com/cpt/FAQ.html
//    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/processes_and_threads.asp
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_H
#define EATHREAD_EATHREAD_H

#include <eathread/internal/config.h>

#if !EA_THREADS_AVAILABLE
	// Do nothing
#elif EA_USE_CPP11_CONCURRENCY
	EA_DISABLE_VC_WARNING(4265 4365 4836 4571 4625 4626 4628 4193 4127 4548 4574 4946 4350)
	#include <chrono>
	#include <thread>
	EA_RESTORE_VC_WARNING()
#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>
	#if defined(_YVALS)         // Dinkumware doesn't usually provide gettimeofday or <sys/types.h>
		#include <time.h>       // clock_gettime
	#elif defined(EA_PLATFORM_UNIX)
		#include <sys/time.h>   // gettimeofday
	#endif
#endif
#if defined(EA_PLATFORM_APPLE)
	#include <mach/mach_types.h>
#endif
#if defined(EA_PLATFORM_SONY) 
	#include "sdk_version.h"
	#include <kernel.h>
#endif
#include <limits.h>
#include <float.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_THREAD_PREEMPTIVE / EA_THREAD_COOPERATIVE
//
// Defined or not defined.
//
// EA_THREAD_COOPERATIVE means that threads are not time-sliced by the 
// operating system. If there exist multiple threads of the same priority 
// then they will need to wait, sleep, or yield in order for the others 
// to get time. See enum Scheduling and EATHREAD_SCHED for more info.
//
// EA_THREAD_PREEMPTIVE means that threads are time-sliced by the operating 
// system at runtime. If there exist multiple threads of the same priority 
// then the operating system will split execution time between them.
// See enum Scheduling and EATHREAD_SCHED for more info.
//
#if !EA_THREADS_AVAILABLE 
	#define EA_THREAD_COOPERATIVE
#else
	#define EA_THREAD_PREEMPTIVE
#endif


/// namespace EA
///
/// This is the standard Electronic Arts C++ namespace.
///
namespace EA
{
	namespace Allocator
	{
		class ICoreAllocator;
	}

	/// namespace Thread
	///
	/// This is the standard Electronic Arts Thread C++ namespace.
	///
	namespace Thread
	{
		/// Scheduling 
		/// Defines scheduling types supported by the given platform.
		/// These are defined in detail by the Posix standard, with the 
		/// exception of Coop, which is added here. FIFO scheduling
		/// is the most classic for game development, as it allows for 
		/// thread priorities and well-behaved synchronization primitives,
		/// but it doesn't do time-slicing. The problem with time slicing
		/// is that threads are pre-empted in the middle of work and this
		/// hurts execution performance and cache performance. 
		///
		enum Scheduling
		{
			kSchedulingFIFO     =  1,    /// There is no automatic time-slicing; thread priorities control execution and context switches.
			kSchedulingRR       =  2,    /// Same as FIFO but is periodic time-slicing.
			kSchedulingSporadic =  4,    /// Complex scheduling control. See the Posix standard.
			kSchedulingTS       =  8,    /// a.k.a. SCHED_OTHER. Usually same as FIFO or RR except that thread priorities and execution can be temporarily modified.
			kSchedulingCoop     = 16     /// The user must control thread scheduling beyond the use of synchronization primitives.
		};
		 
		#if defined(EA_PLATFORM_UNIX)
			#define EATHREAD_SCHED    kSchedulingFIFO

		#elif defined(EA_PLATFORM_MICROSOFT)
			#define EATHREAD_SCHED    kSchedulingRR

		#else
			#define EATHREAD_SCHED    kSchedulingFIFO

		#endif


		// EATHREAD_MULTIPROCESSING_OS
		//
		// Defined as 0 or 1. 
		// Indicates whether the OS supports multiple concurrent processes, which may be in 
		// addition to supporting multiple threads within a process.
		// Some platforms support multiple concurrently loaded processes but don't support
		// running these processes concurrently. We don't currently count this as a
		// multiprocessing OS.
		#ifndef EATHREAD_MULTIPROCESSING_OS
			#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX)
				#define EATHREAD_MULTIPROCESSING_OS 1
			#else
				#define EATHREAD_MULTIPROCESSING_OS 0
			#endif
		#endif
		
		// EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
		// 
		// Defined as 0 or 1. 
		// Indicates whether the OS supports setting the thread name from a different
		// thread (set to 1) or if the name can be set only from the curren thread (set to 0)
		#ifndef EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
			#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_APPLE)
				#define EATHREAD_OTHER_THREAD_NAMING_SUPPORTED 0
			#else
				#define EATHREAD_OTHER_THREAD_NAMING_SUPPORTED 1
			#endif
		#endif

		// Uint / Int
		// Defines a machine-word sized integer, useful for operations that are as efficient
		// as possible on the given machine. Note that the C99 intfastNN_t types aren't sufficient,
		// as they are defined by compilers in an undesirable way for the processors we work with.
		#if !defined(EA_PLATFORM_WORD_SIZE) || (EA_PLATFORM_WORD_SIZE == 4)
			typedef uint32_t Uint;
			typedef int32_t  Int;
		#else
			typedef uint64_t Uint;
			typedef int64_t  Int;
		#endif


		/// ThreadId
		/// Uniquely identifies a thread throughout the system and is used by the EAThread API
		/// to identify threads in a way equal to system provided thread ids. A ThreadId is the 
		/// same as a system thread id and can be used in direct system threading API calls.
		#if !EA_THREADS_AVAILABLE
			typedef int ThreadId;
		#elif EA_USE_CPP11_CONCURRENCY
			typedef std::thread::id ThreadId;
		#elif defined(EA_PLATFORM_SONY)
			typedef uint64_t ThreadId;
		#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
			typedef pthread_t ThreadId;
		#elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
			typedef void* ThreadId; // This is really HANDLE, but HANDLE is the same as void* and we can avoid an expensive #include here.
		#else
			typedef int ThreadId;
		#endif


		// ThreadId constants
		#if EA_USE_CPP11_CONCURRENCY
			const ThreadId kThreadIdInvalid = ThreadId(); /// Special ThreadId indicating an invalid thread identifier.
		#else
			const ThreadId kThreadIdInvalid  = ThreadId(0);            /// Special ThreadId indicating an invalid thread identifier.
			const ThreadId kThreadIdCurrent  = ThreadId(INT_MAX);      /// Special ThreadId indicating the current thread.
			const ThreadId kThreadIdAny      = ThreadId(INT_MAX - 1);  /// Special ThreadId indicating no thread in particular.
		#endif

		/// SysThreadId
		/// It turns out that Microsoft operating systems (Windows, XBox, XBox 360)
		/// have two different ways to identify a thread: HANDLE and DWORD. Some API
		/// functions take thread HANDLES, while others take what Microsoft calls
		/// thread ids (DWORDs). EAThread ThreadId is a HANDLE, as that is used for 
		/// more of the core threading APIs. However, some OS-level APIs accept instead   
		/// the DWORD thread id. 
		#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE && !EA_USE_CPP11_CONCURRENCY
			typedef uint32_t SysThreadId;
			const SysThreadId kSysThreadIdInvalid = SysThreadId(0); /// Special SysThreadId indicating an invalid thread identifier.
		#elif defined(EA_PLATFORM_SONY)
			typedef ScePthread SysThreadId;
			const SysThreadId kSysThreadIdInvalid = { 0 }; /// Special SysThreadId indicating an invalid thread identifier.
		#elif defined(EA_PLATFORM_APPLE)
			typedef thread_act_t SysThreadId; // thread_act_t is useful for calling mach APIs such as thread_policy_set() with. 
			const SysThreadId kSysThreadIdInvalid = SysThreadId(0); /// Special SysThreadId indicating an invalid thread identifier.
		#elif EA_USE_CPP11_CONCURRENCY
			typedef std::thread::native_handle_type SysThreadId;
			const SysThreadId kSysThreadIdInvalid = { 0 }; /// Special SysThreadId indicating an invalid thread identifier.
			
			// For MSVC, native_handle_type is not a primitive type so we define operator== and operator!= for convenience.
			// We use an auto converting proxy type for comparisons to avoid errors when native_handle_type is a built in type.
			bool Equals(const SysThreadId& a, const SysThreadId& b);
			struct SysThreadIdProxy 
			{ 
				SysThreadIdProxy(const SysThreadId& id_) : id(id_) {}
				SysThreadId id;
			};
			inline bool operator==(const SysThreadId& lhs, const SysThreadIdProxy& rhs) { return Equals(lhs, rhs.id); }
			inline bool operator!=(const SysThreadId& lhs, const SysThreadIdProxy& rhs) { return !Equals(lhs, rhs.id); }
		#else
			typedef ThreadId SysThreadId;
			const SysThreadId kSysThreadIdInvalid = SysThreadId(0); /// Special SysThreadId indicating an invalid thread identifier.
		#endif

		/// ThreadUniqueId
		/// Uniquely identifies a thread throughout the system, but in a way that is not 
		/// necessarily compatible with system thread id identification. Sometimes it is 
		/// costly to work with system thread ids whereas all you want is some integer 
		/// that is unique between threads and you don't need to use it for system calls.
		/// See the EAThreadGetUniqueId macro/function for usage.
		typedef Uint ThreadUniqueId;

		// ThreadUniqueId constants
		const ThreadUniqueId kThreadUniqueIdInvalid = 0; /// Special ThreadUniqueId indicating an invalid thread identifier.


		// Time constants
		// Milliseconds are the units of time in EAThread. While every generation of computers
		// results in faster computers and thus milliseconds become an increasingly large number
		// compared to the computer speed, computer multithreading is still largely done at the 
		// millisecond level, due to it still being a small value relative to human perception.
		// We may reconsider this some time in the future and provide an option to have ThreadTime
		// be specified in finer units, such as microseconds.
		#if EA_USE_CPP11_CONCURRENCY
			typedef std::chrono::milliseconds::rep ThreadTime;                               /// Current storage mechanism for time used by thread timeout functions. Units are milliseconds.
			const   ThreadTime kTimeoutImmediate = std::chrono::milliseconds::zero().count();/// Used to specify to functions to return immediately if the operation could not be done.
			const   ThreadTime kTimeoutNone = std::chrono::milliseconds::max().count();      /// Used to specify to functions to block without a timeout (i.e. block forever).
			const   ThreadTime kTimeoutYield = std::chrono::milliseconds::zero().count();    /// This is used with ThreadSleep to minimally yield to threads of equivalent priority.

			#define EA_THREADTIME_AS_INT64(t)  ((int64_t)(t))
			#define EA_THREADTIME_AS_DOUBLE(t) ((double)(t))

		#elif defined(EA_PLATFORM_SONY) && EA_THREADS_AVAILABLE
			typedef double ThreadTime;  // SceKernelUseconds maps to unsigned int 
			static_assert(sizeof(ThreadTime) >= sizeof(unsigned int), "ThreadTime not large enough for uint32_t representation of milliseconds for platform portablity");

			const ThreadTime kTimeoutImmediate = 0;
			const ThreadTime kTimeoutNone = DBL_MAX;
			const ThreadTime kTimeoutYield = 0.000001; // 1 nanosecond in terms of a millisecond

			#define EA_THREADTIME_AS_UINT_MICROSECONDS(t)  ((unsigned int)((t) * 1000.0))                           /// Returns the milliseconds time as uint in microseconds.           
			#define EA_THREADTIME_AS_INT64(t)  ((int64_t)(t))                                                       /// Returns the unconverted milliseconds time as a int64_t.
			#define EA_THREADTIME_AS_DOUBLE(t) (t)                                                                  /// Returns the time as double milliseconds. May include a fraction component.
			#define EA_TIMESPEC_AS_UINT(t)  ((unsigned int)(((t).tv_sec * 1000) + ((t).tv_nsec / 1000000)))         /// Returns the time as uint in milliseconds.            
			#define EA_TIMESPEC_AS_DOUBLE_IN_MS(t)  ( (((t).tv_sec * 1000000000ull) + ((t).tv_nsec))/1000000.0)     /// Returns the time as uint in milliseconds.            

		#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EA_THREADS_AVAILABLE
			struct ThreadTime : public timespec
			{
				typedef int seconds_t;  // To consider: change to uint64_t or maybe long.
				typedef int nseconds_t;
				
				ThreadTime()                                            { tv_sec  = 0;                         tv_nsec  = 0; }
				ThreadTime(const timespec& ts)                          { tv_sec  = ts.tv_sec;                 tv_nsec  = ts.tv_nsec; }
				ThreadTime(seconds_t nSeconds, nseconds_t nNanoseconds) { tv_sec  = (long)nSeconds;            tv_nsec  = (long)nNanoseconds; }
				ThreadTime(const int64_t& nMilliseconds)             { tv_sec  = (long)(nMilliseconds / 1000); tv_nsec  = (long)((nMilliseconds - (tv_sec * 1000)) * 1000000); }
				ThreadTime& operator+=(const int64_t& nMilliseconds) { long lTemp((long)nMilliseconds / 1000); tv_sec  += lTemp; tv_nsec += (long)((nMilliseconds - (lTemp * 1000)) * 1000000); if(tv_nsec >= 1000000000){ tv_sec++; tv_nsec -= 1000000000; } return *this; }
				ThreadTime& operator-=(const int64_t& nMilliseconds) { long lTemp((long)nMilliseconds / 1000); tv_sec  -= lTemp; tv_nsec -= (long)((nMilliseconds - (lTemp * 1000)) * 1000000); if(tv_nsec < 0)          { tv_sec--; tv_nsec += 1000000000; } return *this; }
				ThreadTime& operator+=(const ThreadTime& tt)         { tv_sec += tt.tv_sec;                    tv_nsec += tt.tv_nsec; if(tv_nsec >= 1000000000){ tv_sec++; tv_nsec -= 1000000000; } return *this; }
				ThreadTime& operator-=(const ThreadTime& tt)         { tv_sec -= tt.tv_sec;                    tv_nsec -= tt.tv_nsec; if(tv_nsec < 0)          { tv_sec--; tv_nsec += 1000000000; } return *this; }
			};
			inline ThreadTime operator+ (const ThreadTime& tt1, const ThreadTime& tt2)       { ThreadTime ttR(tt1); ttR += tt2;           return ttR; }
			inline ThreadTime operator+ (const ThreadTime& tt,  const int64_t& nMilliseconds){ ThreadTime ttR(tt);  ttR += nMilliseconds; return ttR; }
			inline ThreadTime operator- (const ThreadTime& tt1, const ThreadTime& tt2)       { ThreadTime ttR(tt1); ttR -= tt2;           return ttR; }
			inline ThreadTime operator- (const ThreadTime& tt,  const int64_t& nMilliseconds){ ThreadTime ttR(tt);  ttR -= nMilliseconds; return ttR; }
			inline bool       operator==(const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_nsec == tt2.tv_nsec) && (tt1.tv_sec == tt2.tv_sec); } // These comparisons assume that the nsec value is normalized (always between 0 && 1000000000).
			inline bool       operator!=(const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_nsec != tt2.tv_nsec) || (tt1.tv_sec != tt2.tv_sec); }
			inline bool       operator< (const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_sec == tt2.tv_sec) ? (tt1.tv_nsec <  tt2.tv_nsec) : (tt1.tv_sec <  tt2.tv_sec); }
			inline bool       operator> (const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_sec == tt2.tv_sec) ? (tt1.tv_nsec >  tt2.tv_nsec) : (tt1.tv_sec >  tt2.tv_sec); }
			inline bool       operator<=(const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_sec == tt2.tv_sec) ? (tt1.tv_nsec <= tt2.tv_nsec) : (tt1.tv_sec <= tt2.tv_sec); }
			inline bool       operator>=(const ThreadTime& tt1, const ThreadTime& tt2) { return (tt1.tv_sec == tt2.tv_sec) ? (tt1.tv_nsec >= tt2.tv_nsec) : (tt1.tv_sec >= tt2.tv_sec); }

			const  ThreadTime kTimeoutImmediate(0, 0);            /// Used to specify to functions to return immediately if the operation could not be done.
			const  ThreadTime kTimeoutNone(INT_MAX, INT_MAX);     /// Used to specify to functions to block without a timeout (i.e. block forever).
			const  ThreadTime kTimeoutYield(0, 0);                /// Used to specify to ThreadSleep to yield to threads of equivalent priority.

			#define EA_THREADTIME_AS_INT64(t)  ((int64_t)(((t).tv_sec * 1000) + ((t).tv_nsec / 1000000)))                   /// Returns the time as int64_t milliseconds.
			#define EA_THREADTIME_AS_INT64_MICROSECONDS(t)  ((int64_t)(((t).tv_sec * 1000000) + (((t).tv_nsec / 1000))))    /// Returns the time as int64_t microseconds.
			#define EA_THREADTIME_AS_DOUBLE(t) (((t).tv_sec * 1000.0) + ((t).tv_nsec / 1000000.0))                          /// Returns the time as double milliseconds.

		#elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64)
			typedef uint64_t   ThreadTime;                        /// Current storage mechanism for time used by thread timeout functions. Units are milliseconds.
			const   ThreadTime kTimeoutImmediate = 0;             /// Used to specify to functions to return immediately if the operation could not be done.
			const   ThreadTime kTimeoutNone      = UINT64_MAX;    /// Used to specify to functions to block without a timeout (i.e. block forever).
			const   ThreadTime kTimeoutYield     = 0;             /// This is used with ThreadSleep to minimally yield to threads of equivalent priority.

			#define EA_THREADTIME_AS_INT64(t)  ((int64_t)(t))
			#define EA_THREADTIME_AS_DOUBLE(t) ((double)(t))

		#else
			typedef unsigned   ThreadTime;                        /// Current storage mechanism for time used by thread timeout functions. Units are milliseconds.
			const   ThreadTime kTimeoutImmediate = 0;             /// Used to specify to functions to return immediately if the operation could not be done.
			const   ThreadTime kTimeoutNone      = UINT_MAX;      /// Used to specify to functions to block without a timeout (i.e. block forever).
			const   ThreadTime kTimeoutYield     = 0;             /// This is used with ThreadSleep to minimally yield to threads of equivalent priority.

			#define EA_THREADTIME_AS_INT64(t)  ((int64_t)(t))
			#define EA_THREADTIME_AS_DOUBLE(t) ((double)(t))

		#endif

		#if defined(EA_PLATFORM_MICROSOFT)                        /// Can be removed from C++11 Concurrency builds once full C++11 implementation is completed
			uint32_t RelativeTimeoutFromAbsoluteTimeout(ThreadTime absoluteTimeout);
		#endif

		// Thread priority constants
		// There is a standardized mechanism to convert system-specific thread
		// priorities to these platform-independent priorities and back without 
		// loss of precision or behaviour. The convention is that kThreadPriorityDefault 
		// equates to the system-specific normal thread priority. Thus for Microsoft
		// APIs a thread with priority kThreadPriorityDefault will be of Microsoft
		// priority THREAD_PRIORITY_NORMAL. A thread with an EAThread priority 
		// of kThreadPriorityDefault + 1 will have a Microsoft priority of THREAD_PRIORITY_NORMAL + 1.
		// The only difference is that with EAThread all platforms are standardized on 
		// kThreadPriorityDefault as the normal value and that higher EAThread priority
		// integral values mean higher thread priorities for running threads. This last
		// item is of significance because Sony platforms natively define lower integers
		// to mean higher thread priorities. With EAThread you get consistent behaviour
		// across platforms and thus kThreadPriorityDefault + 1 always results in a
		// thread that runs at priority of one level higher. On Sony platforms, this + 1
		// gets translated to a - 1 when calling the Sony native thread priority API.
		// EAThread priorities have no mandated integral bounds, though
		// kThreadPriorityMin and kThreadPriorityMax are defined as convenient practical
		// endpoints for users.  Users should not generally use hard-coded constants to
		// refer to EAThread priorities much like it's best not to use hard-coded
		// constants to refer to platform-specific native thread priorities. Also, users
		// generally want to avoid manipulating thread priorities to the extent possible
		// and use conventional synchronization primitives to control execution.
		// Similarly, wildly varying thread priorities such as +100 are not likely to
		// achieve much and are not likely to be very portable.
		//
		const int kThreadPriorityUnknown = INT_MIN;      /// Invalid or unknown priority.
		const int kThreadPriorityMin     =    -128;      /// Minimum thread priority enumerated by EAThread. In practice, a valid thread priority can be anything other than kThreadPriorityUnknown.
		const int kThreadPriorityDefault =       0;      /// Default (a.k.a. normal) thread priority.
		const int kThreadPriorityMax     =     127;      /// Maximum thread priority enumerated by EAThread. In practice, a valid thread priority can be anything other than kThreadPriorityUnknown.



		/// kSysThreadPriorityDefault
		/// Defines the platform-specific default thread priority.
		#if defined(EA_PLATFORM_SONY)
			const int kSysThreadPriorityDefault = 700;
		#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
			const int kSysThreadPriorityDefault = 0; // Some Unix variants use values other than zero, but these are not relevant.
		#elif defined(EA_PLATFORM_MICROSOFT)
			const int kSysThreadPriorityDefault = 0; // Same as THREAD_PRIORITY_NORMAL
		#else
			const int kSysThreadPriorityDefault = 0;
		#endif


		// The following functions are standalone and not static members of the thread class 
		// because they are potentially used by multiple threading primitives and we don't 
		// want to create a dependency of threading primitives on class Thread.

		/// GetThreadId
		/// Gets the thread ID for the current thread. This thread ID should 
		/// be unique throughout the system.
		EATHREADLIB_API ThreadId GetThreadId();


		/// GetSysThreadId
		/// Gets the operating system thread id associated with the given ThreadId.
		/// It turns out that Microsoft operating systems (Windows, XBox, XBox 360)
		/// have two different ways to identify a thread: HANDLE and DWORD. Some API
		/// functions take thread HANDLES, while others take what Microsoft calls
		/// thread ids (DWORDs). EAThread ThreadId is a HANDLE, as that is used for 
		/// more of the core threading APIs. However, some OS-level APIs accept instead   
		/// the DWORD thread id. This function returns the OS thread id for a given 
		/// EAThread ThreadId. In the case of Microsoft OSs, this returns a DWORD from
		/// a HANDLE and with other OSs this function simply returns the ThreadId.
		/// Returns a valid SysThreadId or kSysThreadIdInvalid if the input id is invalid.
		EATHREADLIB_API SysThreadId GetSysThreadId(ThreadId id);


		/// GetThreadId
		///
		/// This is a portable function to convert between ThreadId's and SysThreadId's.
		/// For platforms that do not differentiate between these two types no conversion is attempted. 
		EATHREADLIB_API ThreadId GetThreadId(SysThreadId id);


		/// GetSysThreadId
		/// Gets the SysThreadId for the current thread. This thread ID should 
		/// be unique throughout the system.
		EATHREADLIB_API SysThreadId GetSysThreadId();


		/// GetThreadPriority
		/// Gets the priority of the current thread.
		/// This function can return any int except for kThreadPriorityUnknown, as the 
		/// current thread's priority will always be knowable. A return value of kThreadPriorityDefault
		/// means that this thread is of normal (a.k.a. default) priority.
		/// See the documentation for thread priority constants (e.g. kThreadPriorityDefault) 
		/// for more information about thread priority values and behaviour.
		EATHREADLIB_API int GetThreadPriority();


		/// SetThreadPriority
		/// Sets the priority of the current thread.
		/// Accepts any integer priority value except kThreadPriorityUnknown.
		/// On some platforms, this function will automatically convert any invalid 
		/// priority for that particular platform to a valid one.  A normal (a.k.a. default) thread 
		/// priority is identified by kThreadPriorityDefault.
		/// See the documentation for thread priority constants (e.g. kThreadPriorityDefault) 
		/// for more information about thread priority values and behaviour.
		EATHREADLIB_API bool SetThreadPriority(int nPriority);


		/// GetThreadStackBase
		/// Returns the base address of the current thread's stack.
		/// Recall that on all supported platforms that the stack grows downward
		/// and thus that the stack base address is of a higher value than the 
		/// stack's contents.
		EATHREADLIB_API void* GetThreadStackBase();


		// Thread processor constants
		const int kProcessorDefault = -1;    /// Use the default processor for the platform. On many platforms, the default is to not be tied to any specific processor, but other threads can only ever be bound to a single processor.
		const int kProcessorAny     = -2;    /// Run the thread on any processor. Many platforms will switch threads between processors dynamically.


		/// SetThreadProcessor  
		/// Sets the processor the current thread should run on. Valid values 
		/// are kThreadProcessorDefault, kThreadProcessorAny, or a processor
		/// index in the range of [0, processor count). If the input value
		/// is >= the processor count, it will be reduced to be a modulo of
		/// the processor count. Any other invalid value will cause the processor
		/// to be set to zero.
		/// This function isn't guaranteed to restrict the thread from executing 
		/// on the given processor for all platforms. Some platforms don't support
		/// assigning thread processor affinity, while with others (e.g. Windows using 
		/// SetThreadIdealProcessor) the OS tries to comply but will use a different
		/// processor when the assigned one is unavailable.
		EATHREADLIB_API void SetThreadProcessor(int nProcessor);
		

		/// GetThreadProcessor
		/// Returns the (possibly virtual) CPU index that the thread is currently
		/// running on. Different systems may have differing definitions of what
		/// a unique processor is. Some CPUs have multiple sub-CPUs (e.g. "cores")
		/// which are treated as unique processors by the system. 
		/// Many systems switch threads between processors dynamically; thus it's 
		/// possible that the thread may be on a different CPU by the time this 
		/// function returns. 
		/// Lastly, some systems don't provide the ability to detect what processor
		/// the current thread is running on; in these cases this function returns 0.
		EATHREADLIB_API int GetThreadProcessor();
		

		/// GetProcessorCount
		/// Returns the (possibly virtual) CPU count that the current system has.
		/// Some systems (e.g. Posix, Unix) don't expose an ability to tell how 
		/// many processors there are; in these cases this function returns 1.
		/// This function returns the number of currently active processors. 
		/// Some systems can modify the number of active processors dynamically.
		EATHREADLIB_API int GetProcessorCount();


		/// kThreadAffinityMaskAny
		/// Defines the thread affinity mask that enables the thread 
		/// to float on all available processors.
		typedef uint64_t ThreadAffinityMask;
		const ThreadAffinityMask kThreadAffinityMaskAny = ~0U;


		/// SetThreadAffinityMask
		/// 
		/// The nAffinityMask is a bit field where each bit designates a processor.
		///  
		/// This function isn't guaranteed to restrict the thread from executing 
		/// on the given processor for all platforms. Some platforms don't support
		/// assigning thread processor affinity, while with others (e.g. Windows using 
		/// SetThreadIdealProcessor) the OS tries to comply but will use a different
		/// processor when the assigned one is unavailable.
		EATHREADLIB_API void SetThreadAffinityMask(ThreadAffinityMask nAffinityMask);
		EATHREADLIB_API void SetThreadAffinityMask(const EA::Thread::ThreadId& id, ThreadAffinityMask nAffinityMask);
	

		/// GetThreadAffinityMask
		///   
		/// Returns the current thread affinity mask specified by the user.
		EATHREADLIB_API ThreadAffinityMask GetThreadAffinityMask();
		EATHREADLIB_API ThreadAffinityMask GetThreadAffinityMask(const EA::Thread::ThreadId& id);


		/// GetName
		/// Returns the name of the thread assigned by the SetName function.
		/// If the thread was not named by the SetName function, then the name is empty ("").
		EATHREADLIB_API const char* GetThreadName();
		EATHREADLIB_API const char* GetThreadName(const EA::Thread::ThreadId& id);


		/// SetThreadName
		///
		/// Sets a descriptive name or the thread. On some platforms this name is passed on
		/// to the debugging tools so they can see this name. The name length, including a
		/// terminating 0 char, is limited to EATHREAD_NAME_SIZE characters. Any characters
		/// beyond that are ignored.
		/// 
		/// You can set the name of a Thread object only if it has already begun.  You can
		/// also set the name with the Begin function via the ThreadParameters argument to
		/// Begin. This design is in order to simplify the implementation, but being able
		/// to set ThreadParameters before Begin is something that can be considered in the
		/// future.
		///
		/// Some platforms (e.g. Linux) have the restriction that this function works
		/// properly only when called by the same thread that you want to name. Given this
		/// situation, the most portable way to use this SetName function is to either
		/// always call it from the thread to be named or to use the ThreadParameters to
		/// give the thread a name before it is started and let the started thread name
		/// itself.
		//
		// 
		//
		EATHREADLIB_API void SetThreadName(const char* pName);
		EATHREADLIB_API void SetThreadName(const EA::Thread::ThreadId& id, const char* pName);


		/// ThreadSleep
		/// Puts the current thread to sleep for an amount of time hinted at 
		/// by the time argument. The timeout is merely a hint and the system 
		/// thread scheduler might return well before the sleep time has elapsed.
		/// The input 'timeRelative' refers to a relative time and not an
		/// absolute time such as used by EAThread mutexes, semaphores, etc. 
		/// This is for consistency with other threading systems such as Posix and Win32.
		/// A sleep time of zero has the same effect as simply yielding to other
		/// available threads.
		///
		EATHREADLIB_API void ThreadSleep(const ThreadTime& timeRelative = kTimeoutImmediate);


		/// ThreadCooperativeYield
		/// On platforms that use cooperative multithreading instead of 
		/// pre-emptive multithreading, this function maps to ThreadSleep(0).
		/// On pre-emptive platforms, this function is a no-op. The intention
		/// is to allow cooperative multithreaded systems to yield manually
		/// in order for other threads to run, but also not to penalize 
		/// pre-emptive systems that don't need such manual yielding. If you 
		/// want to forcefully yield on a pre-emptive system, call ThreadSleep(0).
		#ifdef EA_THREAD_COOPERATIVE
			#define ThreadCooperativeYield() EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield)
		#else
			#define ThreadCooperativeYield()
		#endif


		/// End
		/// This function provides a way for a thread to end itself.
		EATHREADLIB_API void ThreadEnd(intptr_t threadReturnValue);


		/// GetThreadTime
		/// Gets the current absolute time in milliseconds.
		/// This is required for working with absolute timeouts, for example.
		/// To specify a timeout that is relative to the current time, simply
		/// add time (in milliseconds) to the return value of GetThreadTime.
		/// Alternatively, you can use ConvertRelativeTime to calculate an absolute time.
		EATHREADLIB_API ThreadTime GetThreadTime();


		/// ConvertRelativeTime
		/// Given a relative time (in milliseconds), this function returns an 
		/// absolute time (in milliseconds).
		/// Example usage:
		///     mutex.Lock(ConvertRelativeTime(1000));
		EATHREADLIB_API inline ThreadTime ConvertRelativeTime(const ThreadTime& timeRelative)
		{
			return GetThreadTime() + timeRelative;
		}

		/// SetAssertionFailureFunction
		/// Allows the user to specify a callback function to trap assertion failures.
		/// You can use this to glue your own assertion system into this system.
		typedef void (*AssertionFailureFunction)(const char* pExpression, void* pContext);
		EATHREADLIB_API void SetAssertionFailureFunction(AssertionFailureFunction pAssertionFailureFunction, void* pContext);


		/// AssertionFailure
		/// Triggers an assertion failure. This function is generally intended for internal
		/// use but is available so that related code can use the same system.
		EATHREADLIB_API void AssertionFailure(const char* pExpression);
		EATHREADLIB_API void AssertionFailureV(const char* pFormat, ...);




		/// Allocator
		/// This is the same as (the first four functions of) ICoreAllocator.
		/// If the allocator is set via SetAllocator, then it must be done before
		/// any other thread operations which might allocate memory are done. 
		/// Typically this includes creating objects via factory functions and 
		/// creating threads whereby you specify that thread resources be allocated for you..
		class Allocator
		{
		public:
			virtual ~Allocator() {}
			virtual void* Alloc(size_t size, const char* name = 0, unsigned int flags = 0) = 0;
			virtual void* Alloc(size_t size, const char* name, unsigned int flags,
									unsigned int align, unsigned int alignOffset = 0) = 0;
			virtual void Free(void* block, size_t size=0) = 0;
		};

		EATHREADLIB_API void       SetAllocator(Allocator* pAllocator);
		EATHREADLIB_API Allocator* GetAllocator();

		EATHREADLIB_API void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

	} // namespace Thread

} // namespace EA



/// EAThreadGetUniqueId
///
/// Gets a value that is unique per thread but isn't necessarily the system-recognized
/// thread id. This function is at least as fast as GetThreadId, and on some platforms
/// is potentially significantly faster due to being implemented in inline asm which 
/// avoids a system function call which may cause an instruction cache miss penalty.
/// This function is useful for creating very fast implementations of some kinds of 
/// threading constructs. It's implemented as a macro instead of a function in order
/// to optimizing inlining success across all platforms and compilers.
///
/// This function is guaranteed to yield a valid value; there are no error conditions.
///
/// This macro acts as if it were declared as a function like this:
///     void EAThreadGetUniqueId(ThreadUniqueId& result);
///
/// Example usage:
///     ThreadUniqueId x;
///     EAThreadGetUniqueId(x);
///
#if EA_USE_CPP11_CONCURRENCY
	#define EAThreadGetUniqueId(dest) (dest = static_cast<uintptr_t>(std::hash<std::thread::id>()(std::this_thread::get_id())))

#elif defined(EA_PLATFORM_WINDOWS) && defined(_MSC_VER) && !defined(_WIN64)

	// Reference implementation:
	//extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
	//#define EAThreadGetUniqueId(dest) dest = (ThreadUniqueId)(uintptr_t)GetCurrentThreadId()

	// Fast implementation:
	extern "C" unsigned long __readfsdword(unsigned long offset);
	#pragma intrinsic(__readfsdword)
	#define EAThreadGetUniqueId(dest) dest = (EA::Thread::ThreadUniqueId)(uintptr_t)__readfsdword(0x18)

#elif defined(_MSC_VER) && defined(EA_PROCESSOR_X86_64)
	#pragma warning(push, 0)
	#include <intrin.h>
	#pragma warning(pop)
	#define EAThreadGetUniqueId(dest) dest = (EA::Thread::ThreadUniqueId)(uintptr_t)__readgsqword(0x30)
	// Could also use dest = (EA::Thread::ThreadUniqueId)NtCurrentTeb(), but that would require #including <windows.h>, which is very heavy.

#else

	// Reference implementation:
	#define EAThreadGetUniqueId(dest) dest = (EA::Thread::ThreadUniqueId)(uintptr_t)EA::Thread::GetThreadId()

#endif


// EAThreadIdToString
// Convert a thread id to a string suitable for use with printf like functions, e.g.:
//      printf("%s", EAThreadIdToString(myThreadId));
// This macro is intended for debugging purposes and makes no guarantees about performance 
// or how a thread id is mapped to a string.
namespace EA
{
	namespace Thread
	{
		namespace detail
		{
			struct EATHREADLIB_API ThreadIdToStringBuffer
			{
			public:
				enum { BufSize = 32 };
				explicit ThreadIdToStringBuffer(EA::Thread::ThreadId threadId);
				const char* c_str() const { return mBuf; }
			private:
				char mBuf[BufSize];
			};

			struct EATHREADLIB_API SysThreadIdToStringBuffer
			{
			public:
				enum { BufSize = 32 };
				explicit SysThreadIdToStringBuffer(EA::Thread::SysThreadId sysThreadId);
				const char* c_str() const { return mBuf; }
			private:
				char mBuf[BufSize];
			};
		}
	}
}

#if !defined(EAThreadThreadIdToString)
	#define EAThreadThreadIdToString(threadId)       (EA::Thread::detail::ThreadIdToStringBuffer(threadId).c_str())
#endif
#if !defined(EAThreadSysThreadIdToString)
	#define EAThreadSysThreadIdToString(sysThreadId) (EA::Thread::detail::SysThreadIdToStringBuffer(sysThreadId).c_str())
#endif


///////////////////////////////////////////////////////////////////////////////
// Inline functions
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	// We implement GetSysThreadId in our associated .cpp file.
#elif defined(EA_PLATFORM_SONY)
	// We implement GetSysThreadId in our associated .cpp file.
#elif defined(EA_PLATFORM_APPLE)
	// We implement GetSysThreadId in our associated .cpp file.
#elif EA_USE_CPP11_CONCURRENCY
	// We implement GetSysThreadId in our associated .cpp file.
#else
	inline EA::Thread::SysThreadId EA::Thread::GetSysThreadId(ThreadId id)
	{
		return id;
	}

	inline EA::Thread::SysThreadId EA::Thread::GetSysThreadId()
	{
		return GetThreadId(); // ThreadId == SysThreadId in this case
	}
#endif

#endif // EATHREAD_EATHREAD_H



