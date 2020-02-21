///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>
#include <sys/signal.h>
#include <machine/signal.h>
#include <sdk_version.h>
#include <unistd.h>


// EATHREAD_PTHREAD_SIGACTION_SUPPORTED
//
// Defined as 0 or 1.
//
#if !defined(EATHREAD_PTHREAD_SIGACTION_SUPPORTED)
	//#if EATHREAD_SCEDBG_ENABLED || defined(EA_DEBUG)
	//    #define EATHREAD_PTHREAD_SIGACTION_SUPPORTED 1
	//#else
	//    #define EATHREAD_PTHREAD_SIGACTION_SUPPORTED 0
	//#endif

	// Disabling due to syscall crashing on SDK 1.6.
	#define EATHREAD_PTHREAD_SIGACTION_SUPPORTED 0
#endif


#if EATHREAD_PTHREAD_SIGACTION_SUPPORTED 
	// Until Sony provides a declaration for this or an alternative scheme, we declare this ourselves.
	__BEGIN_DECLS

	// User-level applications use as integer registers for passing the sequence %rdi, %rsi, %rdx, %rcx, %r8 and %r9. 
	// The kernel interface uses %rdi, %rsi, %rdx, %r10, %r8 and %r9, which is what matters to us below.
	// http://www.ibm.com/developerworks/library/l-ia/index.html
	// A system-call is done via the syscall instruction. The kernel destroys registers %rcx and %r11.
	// The number of the syscall has to be passed in register %rax.
	// System-calls are limited to six arguments, no argument is passed directly on the stack.
	// Returning from the syscall, register %rax contains the result of the system-call. A value in the range between -4095 and -1 indicates an error, it is -errno.
	// Only values of class INTEGER or class MEMORY are passed to the kernel.
	// Relevant BSD source code: https://bitbucket.org/freebsd/freebsd-head/src/36b017c6a0f817439d40abfd790238dfa13e2be3/lib/libthr/thread?at=default
	// The BSD pthread struct: https://bitbucket.org/freebsd/freebsd-head/src/36b017c6a0f817439d40abfd790238dfa13e2be3/lib/libthr/thread/thr_private.h?at=default
	// Some NetBSD pthread source: http://cvsweb.netbsd.org/bsdweb.cgi/src/lib/libpthread/pthread.c?rev=1.134&content-type=text/x-cvsweb-markup&only_with_tag=MAIN

	static int sigaction(int sig, const struct sigaction * __restrict act, struct sigaction * __restrict oact)
	{
			int result;
			__asm__ __volatile__( 
					"mov %%rcx, %%r10\n\t"
					"syscall\n\t"
					: "=a"(result) : "a"(416), "D"(sig), "S"(act), "d"(oact));
			return result;
	}


	// #define SYS_thr_kill 433
	// typedef long thread_t 

	// pthread_t is an opaque typedef for struct pthread. struct pthread looks like so: 
	//    struct pthread {
	//        long tid; // Kernel thread id. 
	//        . . .     // Many other members.
	//    }
	// Thus you can directly reinterpret_cast pthread to a pointer to a kernel thread id.
	#if !defined(GetTidFromPthread)
		#define GetTidFromPthread(pthreadId) *reinterpret_cast<long*>(pthreadId)
	#endif

	static int thr_kill(long thread, int sig)
	{
		int result;
		__asm__ __volatile__( 
				"mov %%rcx, %%r10\n\t"
				"syscall\n\t"
				: "=a"(result) : "a"(433), "D"(thread), "S"(sig));
		return result;
	}

	static int pthread_kill(pthread_t pthreadId, int sig)
	{
		long tid = GetTidFromPthread(pthreadId);
		thr_kill(tid, sig);
		return 0;
	}

	const size_t kBacktraceSignalHandlerIgnoreCount = 2; // It's unclear what this value should be. On one machine it was 4, but on another it was 2. Going with a lower number is more conservative. Possibly a debug/opt thing?

	__END_DECLS
#endif


// Sony may remove this header in the future, so we use the clang __has_include feature to detect if and when that occurs.

// NOTE:  Use of unwind.h is disabled on PS4 due to syscall hangs in the kernel
// experienced by Frostbite when overloadiing user_malloc to generate a
// callstack.  In addition, Sony recommends the use of __builtin_frame_address
// / __builtin_return_address over _Unwind_Backtrace as it is more performant
// due to the frame pointers being included by default in all builds.

// Thread that stats performance of __builtin_frame_pointer is better.
// https://ps4.scedev.net/forums/thread/2267/

// Open support ticket for syscall hang:
// https://ps4.scedev.net/forums/thread/52687/

#if __has_include(<unwind.h>) && !defined(EA_PLATFORM_SONY)
	#include <unwind.h>

	#if !defined(EA_HAVE_UNWIND_H)
		#define EA_HAVE_UNWIND_H 1
	#endif
#else
	#if !defined(EA_NO_HAVE_UNWIND_H)
		#define EA_NO_HAVE_UNWIND_H 1
	#endif
#endif



namespace EA
{
namespace Thread
{


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EATHREADLIB_API void InitCallstack()
{
	// Nothing needed.
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EATHREADLIB_API void ShutdownCallstack()
{
	// Nothing needed.
}


EATHREADLIB_API void GetInstructionPointer(void*& p)
{
	p = __builtin_return_address(0);
}



#if defined(EA_HAVE_UNWIND_H)
	// This is a callback function which libunwind calls, once per callstack entry.
	struct UnwindCallbackContext
	{
		void** mpReturnAddressArray;
		size_t mReturnAddressArrayCapacity;
		size_t mReturnAddressArrayIndex;
	};

	static _Unwind_Reason_Code UnwindCallback(_Unwind_Context* pUnwindContext, void* pUnwindCallbackContextVoid)
	{
		UnwindCallbackContext* pUnwindCallbackContext = (UnwindCallbackContext*)pUnwindCallbackContextVoid;

		if(pUnwindCallbackContext->mReturnAddressArrayIndex < pUnwindCallbackContext->mReturnAddressArrayCapacity)
		{
			uintptr_t ip = _Unwind_GetIP(pUnwindContext);
			pUnwindCallbackContext->mpReturnAddressArray[pUnwindCallbackContext->mReturnAddressArrayIndex++] = (void*)ip;
			return _URC_NO_REASON;
		}

		return _URC_NORMAL_STOP;
	}
#endif




#if EATHREAD_PTHREAD_SIGACTION_SUPPORTED 
	namespace Local
	{
		enum EAThreadBacktraceState
		{
			// Positive thread lwp ids are here implicitly.
			EATHREAD_BACKTRACE_STATE_NONE    = -1,
			EATHREAD_BACKTRACE_STATE_DUMPING = -2,
			EATHREAD_BACKTRACE_STATE_DONE    = -3,
			EATHREAD_BACKTRACE_STATE_CANCEL  = -4
		};

		struct ThreadBacktraceState
		{
			EA::Thread::AtomicInt32 mState;              // One of enum EAThreadBacktraceState or (initially) the thread id of the thread we are targeting.
			void**                  mCallstack;          // Output param
			size_t                  mCallstackCapacity;  // Input param, refers to array capacity of mCallstack.
			size_t                  mCallstackCount;     // Output param
			ScePthread              mPthread;   	     // Output param

			ThreadBacktraceState() : mState(EATHREAD_BACKTRACE_STATE_NONE), mCallstackCapacity(0), mCallstackCount(0), mPthread(NULL){}
		};


		static ScePthreadMutex      gThreadBacktraceMutex = SCE_PTHREAD_MUTEX_INITIALIZER;
		static ThreadBacktraceState gThreadBacktraceState; // Protected by gThreadBacktraceMutex.


		static void gThreadBacktraceSignalHandler(int /*sigNum*/, siginfo_t* /*pSigInfo*/, void* pSigContextVoid)
		{
			int32_t lwpSelf = *(int32_t*)scePthreadSelf();

			if(gThreadBacktraceState.mState.SetValueConditional(EATHREAD_BACKTRACE_STATE_DUMPING, lwpSelf))
			{
				gThreadBacktraceState.mPthread = scePthreadSelf();

				if(gThreadBacktraceState.mCallstackCapacity)
				{
					gThreadBacktraceState.mCallstackCount = GetCallstack(gThreadBacktraceState.mCallstack, gThreadBacktraceState.mCallstackCapacity, (const CallstackContext*)NULL);

					// At this point we need to remove the top N entries and insert an entry for where the thread's instruction pointer is.

					// We originally had code like the following, but it's returning a signal 
					// handling address now that  we are using our own pthread_kill function:
					//if(gThreadBacktraceState.mCallstackCount >= kBacktraceSignalHandlerIgnoreCount) // This should always be true.
					//{
					//    gThreadBacktraceState.mCallstackCount -= (kBacktraceSignalHandlerIgnoreCount - 1);
					//    memmove(&gThreadBacktraceState.mCallstack[1], &gThreadBacktraceState.mCallstack[kBacktraceSignalHandlerIgnoreCount], (gThreadBacktraceState.mCallstackCount - 1) * sizeof(void*));
					//}
					//else
					//    gThreadBacktraceState.mCallstackCount = 1;
					//gThreadBacktraceState.mCallstack[0] = pSigContextVoid ? reinterpret_cast<void*>(reinterpret_cast<sigcontext*>((uintptr_t)pSigContextVoid + 48)->sc_rip) : NULL;

					// New code that's working for our own pthread_kill function usage:
					if(gThreadBacktraceState.mCallstackCount >= kBacktraceSignalHandlerIgnoreCount) // This should always be true.
					{
						gThreadBacktraceState.mCallstackCount -= kBacktraceSignalHandlerIgnoreCount;
						memmove(&gThreadBacktraceState.mCallstack[0], &gThreadBacktraceState.mCallstack[kBacktraceSignalHandlerIgnoreCount], gThreadBacktraceState.mCallstackCount * sizeof(void*));
					}
				}
				else
					gThreadBacktraceState.mCallstackCount = 0;

				gThreadBacktraceState.mState.SetValue(EATHREAD_BACKTRACE_STATE_DONE);
			}
			// else this thread received an unexpected SIGURG. This can happen if it was so delayed that 
			// we timed out waiting for it to happen and moved on.
		}
	}
#endif


/// GetCallstack
///
/// This is a version of GetCallstack which gets the callstack of a thread based on its thread id as opposed to 
/// its register state. It works by injecting a signal handler into the given thread and reading the self callstack
/// then exiting from the signal handler. The GetCallstack function sets this up, generates the signal for the 
/// other thread, then waits for it to complete. It uses the SIGURG signal for this.
///
/// Primary causes of failure:
///     The target thread has SIGURG explicitly ignored.
///     The target thread somehow is getting too little CPU time to respond to the signal.
///
/// To do: Change this function to take a ThreadInfo as a last parameter instead of pthread_t. And have the 
/// ThreadInfo return additional basic thread information. Or maybe even change this function to be a 
/// GetThreadInfo function instead of GetCallstack.
///
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, EA::Thread::ThreadId& pthread)
{
	size_t callstackCount = 0;

	#if EATHREAD_PTHREAD_SIGACTION_SUPPORTED
		using namespace Local;

		if(pthread)
		{
			ScePthread pthreadSelf = scePthreadSelf();
			int32_t    lwp         = *(int32_t*)pthread;
			int32_t    lwpSelf     = *(int32_t*)pthreadSelf;

			if(lwp == lwpSelf) // This function can be called only for a thread other than self.
				callstackCount = GetCallstack(pReturnAddressArray, nReturnAddressArrayCapacity, (const CallstackContext*)NULL);
			else
			{
				struct sigaction act;   memset(&act, 0, sizeof(act));
				struct sigaction oact;  memset(&oact, 0, sizeof(oact));
	
				act.sa_sigaction = gThreadBacktraceSignalHandler;
				act.sa_flags     = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

				scePthreadMutexLock(&gThreadBacktraceMutex);

				if(sigaction(SIGURG, &act, &oact) == 0)
				{
					gThreadBacktraceState.mCallstack         = pReturnAddressArray;
					gThreadBacktraceState.mCallstackCapacity = nReturnAddressArrayCapacity;
					gThreadBacktraceState.mState.SetValue(lwp);

					// Signal the specific thread that we want to dump.
					int32_t stateTemp = lwp;

					if(pthread_kill(pthread, SIGURG) == 0)
					{
						// Wait for the other thread to start dumping the stack, or time out.
						for(int waitMS = 200; waitMS; waitMS--)
						{
							stateTemp = gThreadBacktraceState.mState.GetValue();

							if(stateTemp != lwp)
								break;

							usleep(1000); // This sleep gives the OS the opportunity to execute the target thread, even if it's of a lower priority than this thread.
						}
					} 
					// else apparently failed to send SIGURG to the thread, or the thread was paused in a way that it couldn't receive it.

					if(stateTemp == lwp) // If the operation timed out or seemingly never started...
					{
						if(gThreadBacktraceState.mState.SetValueConditional(EATHREAD_BACKTRACE_STATE_CANCEL, lwp)) // If the backtrace still didn't start, and we were able to stop it by setting the state to cancel...
							stateTemp = EATHREAD_BACKTRACE_STATE_CANCEL;
						else
							stateTemp = gThreadBacktraceState.mState.GetValue();    // It looks like the backtrace thread did in fact get a late start and is now executing
					}

					// Wait indefinitely for the dump to finish or be canceled.
					// We cannot apply a timeout here because the other thread is accessing state that
					// is owned by this thread.
					for(int waitMS = 100; (stateTemp == EATHREAD_BACKTRACE_STATE_DUMPING) && waitMS; waitMS--) // If the thread is (still) busy writing it out its callstack...
					{
						usleep(1000);
						stateTemp = gThreadBacktraceState.mState.GetValue();
					}

					if(stateTemp == EATHREAD_BACKTRACE_STATE_DONE)
						callstackCount = gThreadBacktraceState.mCallstackCount;
					// Else give up on it. It's OK to just fall through.

					// Restore the original SIGURG handler.
					sigaction(SIGURG, &oact, NULL);
				}

				scePthreadMutexUnlock(&gThreadBacktraceMutex);
			}
		}
	#endif

	return callstackCount;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	#if defined(EA_HAVE_UNWIND_H)
		// libunwind can only read the stack from the current thread.
		// However, we can accomplish this for another thread by injecting a signal handler into that thread.
		// See the EAThreadBacktrace() function source code above.

		if(pContext == NULL) // If reading the current thread's context...
		{
			UnwindCallbackContext context = { pReturnAddressArray, nReturnAddressArrayCapacity, 0 };
			_Unwind_Backtrace(&UnwindCallback, &context);
			return context.mReturnAddressArrayIndex;
		}

		// We don't yet have a means to read another thread's context.
		return 0;
	#else
		// This platform doesn't use glibc and so the backtrace() function isn't available.
		// For debug builds we can follow the stack frame manually, as stack frames are usually available in debug builds.
		EA_UNUSED(pReturnAddressArray);
		EA_UNUSED(nReturnAddressArrayCapacity);

		size_t index = 0;
		void** sp = nullptr;
		void** new_sp = nullptr;
		const uintptr_t kPtrSanityCheckLimit = 1*1024*1024;

		if (pContext == NULL)
		{
			// Arguments are passed in registers on x86-64, so we can't just offset from &pReturnAddressArray.
			sp = (void**)__builtin_frame_address(0);
		}
		else
		{
			// On kettle it's not recommended to omit the frame pointer so we check that RBP is sane before use since 
			// it could have been omitted. From Sony Docs:
			// "[omit frame pointer] will inhibit unwinding and ... the option may also increase code size since the 
			// encoding for stack-based addressing is often 1 byte longer then RBP-based (frame pointer) addressing. 
			// With PlayStation®4 Clang, frame pointer omission may not lead to improved performance. 
			// Performance analysis and code  profiling are recommended before using this option"
			sp = (void**)((pContext->mRBP - pContext->mRSP) > kPtrSanityCheckLimit ? pContext->mRSP : pContext->mRBP);
			pReturnAddressArray[index++] = (void*)pContext->mRIP;
		}

		for(int count = 0; sp && (index < nReturnAddressArrayCapacity); sp = new_sp, ++count)
		{
			if(count > 0 || index != 0) // We skip the current frame if we haven't set it already above
				pReturnAddressArray[index++] = *(sp + 1);

			new_sp = (void**)*sp;

			if((new_sp < sp) || (new_sp > (sp + kPtrSanityCheckLimit)))
				break;
		}         

		return index;
	#endif
}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	ScePthread self         = scePthreadSelf();
	ScePthread pthread_Id   = (ScePthread)threadId; // Requires that ScePthread is a pointer or integral type.

	if(scePthreadEqual(pthread_Id, self))
	{
		void* pInstruction;

		// This is some crazy GCC code that happens to work:
		pInstruction = ({ __label__ label; label: &&label; });

		context.mRIP = (uint64_t)pInstruction;
		context.mRSP = (uint64_t)__builtin_frame_address(1);
		context.mRBP = 0;
	}
	else
	{
		// There is currently no way to do this.
		memset(&context, 0, sizeof(context));
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	// Assuming we are using pthreads, sysThreadId == threadId.
	return GetCallstackContext(context, sysThreadId);
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
	context.mRIP = pContext->Rip;
	context.mRSP = pContext->Rsp;
	context.mRBP = pContext->Rbp;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* /*address*/, char* pModuleName, size_t /*moduleNameCapacity*/)
{
	// Not currently implemented for the given platform.
	pModuleName[0] = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
	// Not currently implemented for the given platform.
	return 0;
}



EA::Thread::ThreadLocalStorage sStackBase;

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* pStackBase)
{
	if(pStackBase)
		sStackBase.SetValue(pStackBase);
	else
	{
		pStackBase = __builtin_frame_address(0);

		if(pStackBase)
			SetStackBase(pStackBase);
		// Else failure; do nothing.
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	void* pBase;

	if(GetPthreadStackInfo(&pBase, NULL))
		return pBase;

	// Else we require the user to have set this previously, usually via a call 
	// to SetStackBase() in the start function of this currently executing 
	// thread (or main for the main thread).
	pBase = sStackBase.GetValue();

	if(pBase == NULL)
		pBase = (void*)(((uintptr_t)&pBase + 4095) & ~4095); // Make a guess, round up to next 4096.

	return pBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	void* pLimit;

	if(GetPthreadStackInfo(NULL, &pLimit))
		return pLimit;

	pLimit = __builtin_frame_address(0);

	return (void*)((uintptr_t)pLimit & ~4095); // Round down to nearest page.
}



} // namespace Thread
} // namespace EA







