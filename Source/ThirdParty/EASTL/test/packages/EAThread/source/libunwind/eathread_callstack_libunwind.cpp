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
#include <pthread.h>
#include <unistd.h>
#include <unwind.h>

#if defined(EA_PLATFORM_BSD)
	#include <sys/signal.h>
	#include <machine/signal.h>
#elif defined(EA_PLATFORM_LINUX)
	#include <signal.h>
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
	// Currently all platforms that have <unwind.h> have __builtin_return_address().
	p = __builtin_return_address(0);
}



// This is a callback function which libunwind calls, once per callstack entry.
/*
Linux for ARM:
	enum _Unwind_Reason_Code {
		_URC_OK = 0,                        // operation completed successfully
		_URC_FOREIGN_EXCEPTION_CAUGHT = 1,
		_URC_END_OF_STACK = 5,
		_URC_HANDLER_FOUND = 6,
		_URC_INSTALL_CONTEXT = 7,
		_URC_CONTINUE_UNWIND = 8,
		_URC_FAILURE = 9                    // unspecified failure of some kind
	};

	#define _URC_NO_REASON _URC_OK

BSD (and I think also Linux for x86/x64): 
	enum _Unwind_Reason_Code {
		_URC_NO_REASON = 0,
		_URC_FOREIGN_EXCEPTION_CAUGHT = 1,
		_URC_FATAL_PHASE2_ERROR = 2,
		_URC_FATAL_PHASE1_ERROR = 3,
		_URC_NORMAL_STOP = 4,
		_URC_END_OF_STACK = 5,
		_URC_HANDLER_FOUND = 6,
		_URC_INSTALL_CONTEXT = 7,
		_URC_CONTINUE_UNWIND = 8
	};
*/

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

	#if defined(EA_PLATFORM_LINUX)
		return _URC_NO_REASON; // Is there a way to tell the caller that we want to stop?
	#else
		return _URC_NORMAL_STOP;
	#endif
}





/*

The following commented-out code is for reading the callstack of a thread other than the current one.
The code below is originally for BSD Unix, and probably needs to be tweaked to support Linux.

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
		pthread_t               mPthread;            // Output param

		ThreadBacktraceState() : mState(EATHREAD_BACKTRACE_STATE_NONE), mCallstackCapacity(0), mCallstackCount(0), mPthread(NULL){}
	};


	static pthread_mutex_t      gThreadBacktraceMutex = PTHREAD_MUTEX_INITIALIZER;
	static ThreadBacktraceState gThreadBacktraceState; // Protected by gThreadBacktraceMutex.


	static void gThreadBacktraceSignalHandler(int sigNum, siginfo_t* pSigInfo, void* pSigContextVoid)
	{
		int32_t lwpSelf = *(int32_t*)pthread_self();

		if(gThreadBacktraceState.mState.SetValueConditional(EATHREAD_BACKTRACE_STATE_DUMPING, lwpSelf))
		{
			gThreadBacktraceState.mPthread = pthread_self();

			if(gThreadBacktraceState.mCallstackCapacity)
			{
				gThreadBacktraceState.mCallstackCount = GetCallstack(gThreadBacktraceState.mCallstack, gThreadBacktraceState.mCallstackCapacity, (const CallstackContext*)NULL);

				// At this point we need to remove the top 6 entries and insert an entry for where the thread's instruction pointer is.
				if(gThreadBacktraceState.mCallstackCount >= 6) // This should always be true.
				{
					gThreadBacktraceState.mCallstackCount -= 5;
					memmove(&gThreadBacktraceState.mCallstack[1], &gThreadBacktraceState.mCallstack[6], (gThreadBacktraceState.mCallstackCount - 1) * sizeof(void*));
				}
				else
					gThreadBacktraceState.mCallstackCount = 1;

				gThreadBacktraceState.mCallstack[0] = pSigContextVoid ? reinterpret_cast<void*>(reinterpret_cast<sigcontext*>((uintptr_t)pSigContextVoid + 48)->sc_rip) : NULL;
			}
			else
				gThreadBacktraceState.mCallstackCount = 0;

			gThreadBacktraceState.mState.SetValue(EATHREAD_BACKTRACE_STATE_DONE);
		}
		// else this thread received an unexpected SIGURG. This can happen if it was so delayed that 
		// we timed out waiting for it to happen and moved on.
	}
}


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
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, pthread_t& pthread)
{
	using namespace Local;

	size_t callstackCount = 0;

	if(pthread)
	{
		pthread_t pthreadSelf    = pthread_self();
		int32_t   lwp            = *(int32_t*)pthread;
		int32_t   lwpSelf        = *(int32_t*)pthreadSelf;

		if(lwp == lwpSelf) // This function can be called only for a thread other than self.
			callstackCount = GetCallstack(pReturnAddressArray, nReturnAddressArrayCapacity, (const CallstackContext*)NULL);
		else
		{
			struct sigaction act;   memset(&act, 0, sizeof(act));
			struct sigaction oact;  memset(&oact, 0, sizeof(oact));
	
			act.sa_sigaction = gThreadBacktraceSignalHandler;
			act.sa_flags     = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

			pthread_mutex_lock(&gThreadBacktraceMutex);

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

			pthread_mutex_unlock(&gThreadBacktraceMutex);
		}
	}

	return callstackCount;
}
*/



///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	// libunwind can only read the stack from the current thread.
	// However, we can accomplish this for another thread by injecting a signal handler into that thread.
	// See the EAThreadBacktrace() function source code above.

	if(pContext == NULL) // If reading the current thread's context...
	{
		UnwindCallbackContext context = { pReturnAddressArray, nReturnAddressArrayCapacity, 0 };
		_Unwind_Backtrace(&UnwindCallback, &context);

		if (context.mReturnAddressArrayIndex > 0)
		{
			--context.mReturnAddressArrayIndex; // Remove the first entry, because it refers to this function and by design we don't include this function.
			memmove(pReturnAddressArray, pReturnAddressArray + 1, context.mReturnAddressArrayIndex * sizeof(void*));
		}

		return context.mReturnAddressArrayIndex;
	}

	// We don't yet have a means to read another thread's callstack via only the CallstackContext.
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	// True Linux-based ARM platforms (usually tablets and phones) can use pthread_attr_getstack.
	#if defined(EA_PLATFORM_ANDROID)
		if((threadId == (intptr_t)kThreadIdInvalid) || 
		   (threadId == (intptr_t)kThreadIdCurrent) || 
		   (threadId == (intptr_t)EA::Thread::GetThreadId()))
		{
			// Note: the behavior below is inconsistent between platforms and needs to be made so.
			#if defined(__ARMCC_VERSION) // If using the ARM Compiler...
				context.mSP           = (uint32_t)__current_sp();
				context.mLR           = (uint32_t)__return_address();
				context.mPC           = (uint32_t)__current_pc();
				context.mStackPointer = context.mSP;

			#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)                
				#if defined(EA_PROCESSOR_X86_64)
					context.mRIP          = (uint64_t)__builtin_return_address(0);
					context.mRSP          = (uint64_t)__builtin_frame_address(1);
					context.mRBP          = 0;
					context.mStackPointer = context.mRSP;

				#elif defined(EA_PROCESSOR_X86)
					context.mEIP          = (uint32_t)__builtin_return_address(0);
					context.mESP          = (uint32_t)__builtin_frame_address(1);
					context.mEBP          = 0;
					context.mStackPointer = context.mESP;

				#elif defined(EA_PROCESSOR_ARM32)
					// register uintptr_t current_sp asm ("sp");
					context.mSP = (uint32_t)__builtin_frame_address(0);
					context.mLR = (uint32_t)__builtin_return_address(0);

					void* pInstruction;
					EAGetInstructionPointer(pInstruction);
					context.mPC = reinterpret_cast<uintptr_t>(pInstruction);

					context.mStackPointer = context.mSP;

				#elif defined(EA_PROCESSOR_ARM64)
					// register uintptr_t current_sp asm ("sp");
					context.mSP = (uint64_t)__builtin_frame_address(0);
					context.mLR = (uint64_t)__builtin_return_address(0);

					void* pInstruction;
					EAGetInstructionPointer(pInstruction);
					context.mPC = reinterpret_cast<uintptr_t>(pInstruction);

					context.mStackPointer = context.mSP;
				#endif
			#endif

			context.mStackBase  = (uintptr_t)GetStackBase();
			context.mStackLimit = (uintptr_t)GetStackLimit();

			return true;
		}
		else
		{
			// Else haven't implemented getting the stack info for other threads
			memset(&context, 0, sizeof(context));
			return false;
		}        

	#else
		pthread_t self      = pthread_self();
		pthread_t pthreadId = (typeof(pthread_t))threadId; // Requires that pthread_t is a pointer or integral type.

		if(pthread_equal(pthreadId, self))
		{
			void* pInstruction;

			// This is some crazy GCC code that happens to work:
			pInstruction = ({ __label__ label; label: &&label; });

			// Note: the behavior below is inconsistent between platforms and needs to be made so.
			#if defined(EA_PROCESSOR_X86_64)
				context.mRIP = (uint64_t)pInstruction;
				context.mRSP = (uint64_t)__builtin_frame_address(1);
				context.mRBP = 0;

			#elif defined(EA_PROCESSOR_X86)
				context.mEIP = (uint32_t)__builtin_return_address(0);
				context.mESP = (uint32_t)__builtin_frame_address(1);
				context.mEBP = 0;
			#endif

			return true;
		}
		else
		{
			// There is currently no way to do this.
			memset(&context, 0, sizeof(context));
			return false;
		}
	#endif
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
	#if defined(EA_PROCESSOR_X86_64)
		context.mRIP = pContext->Rip;
		context.mRSP = pContext->Rsp;
		context.mRBP = pContext->Rbp;
	#elif defined(EA_PROCESSOR_X86)
		context.mEIP = pContext->Eip;
		context.mESP = pContext->Esp;
		context.mEBP = pContext->Ebp;
	#elif defined(EA_PROCESSOR_ARM32)
		context.mSP  = pContext->mGpr[13];
		context.mLR  = pContext->mGpr[14];
		context.mPC  = pContext->mGpr[15];
	#elif defined(EA_PROCESSOR_ARM64)
		context.mSP  = pContext->mGpr[31];
		context.mLR  = pContext->mGpr[30];
		context.mPC  = pContext->mPC;
	#else
		// To do.
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
// Returns the required strlen of pModuleName.
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* address, char* pModuleName, size_t moduleNameCapacity)
{
	#if 0 // Disabled until testable: defined(EA_PLATFORM_LINUX)
		// The output of reading /proc/self/maps is like the following (there's no leading space on each line).
		// We look for entries that have r-x as the first three flags, as they are executable modules.
		// The format is (http://linux.die.net/man/5/proc):
		// <begin address>-<end address> <flags> <offset> <device major>:<device minor> <inode> <path>
		//
		// 00400000-0040b000 r-xp 00000000 08:01 655382                             /bin/cat
		// 0060a000-0060b000 r--p 0000a000 08:01 655382                             /bin/cat
		// 0060b000-0060c000 rw-p 0000b000 08:01 655382                             /bin/cat
		// 0060c000-0062d000 rw-p 00000000 00:00 0                                  [heap]
		// 7ffff77b5000-7ffff7a59000 r--p 00000000 08:01 395618                     /usr/lib/locale/locale-archive
		// 7ffff7a59000-7ffff7bd3000 r-xp 00000000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7bd3000-7ffff7dd2000 ---p 0017a000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd2000-7ffff7dd6000 r--p 00179000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd6000-7ffff7dd7000 rw-p 0017d000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd7000-7ffff7ddc000 rw-p 00000000 00:00 0 
		// 7ffff7ddc000-7ffff7dfc000 r-xp 00000000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7fd9000-7ffff7fdc000 rw-p 00000000 00:00 0 
		// 7ffff7ff9000-7ffff7ffb000 rw-p 00000000 00:00 0 
		// 7ffff7ffb000-7ffff7ffc000 r-xp 00000000 00:00 0                          [vdso]
		// 7ffff7ffc000-7ffff7ffd000 r--p 00020000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7ffd000-7ffff7ffe000 rw-p 00021000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7ffe000-7ffff7fff000 rw-p 00000000 00:00 0 
		// 7ffffffde000-7ffffffff000 rw-p 00000000 00:00 0                          [stack]
		// ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

		FILE* file = fopen("/proc/self/maps", "rt"); 

		if(file)
		{
			uint64_t address64 = (uint64_t)reinterpret_cast<uintptr_t>(address);
			char     lineBuffer[1024]; 

			while(fgets(lineBuffer, sizeof(lineBuffer), file) != NULL) 
			{ 
				size_t lineLength = strlen(lineBuffer); 

				if((lineLength > 0) && (lineBuffer[lineLength - 1] == '\n'))
					lineBuffer[--lineLength] = '\0'; 

				uint64_t start, end, offset, devMajor, devMinor, inode;
				char     flags[4]; 
				char     path[512 + 1]; 

				// 7ffff7ddc000-7ffff7dfc000 r-xp 00000000 08:01 1062651 /lib/ld-2.12.1.so
				int fieldCount = EA::StdC::Sscanf(lineBuffer, "%I64x-%I64x %c%c%c%c %I64x %I64d:%I64d %I64x %512s", 
									&start, &end, &flags[0], &flags[1], &flags[2], &flags[3], &offset,
									&devMajor, &devMinor, &inode, path);
				if(fieldCount == 11) 
				{ 
					if((flags[0] == 'r') && (flags[1] == '-') && (flags[2] == 'x')) // If this looks like an executable module...
					{
						if((address64 >= start) && (address64 < end)) // If this is the module that corresponds to the input address
						{
							// We can't strcpy path as-is because it might be truncated due to spaces in the file name.
							// So we get the location path is in the original lineBuffer and strcpy everything till the end.
							char* pPathBegin = EA::StdC::Strstr(lineBuffer, path);

							return EA::StdC::Strlcpy(pModuleName, pPathBegin, moduleNameCapacity);
						}
					}
				}
			}

			fclose(file);
		}
	#else
		EA_UNUSED(address);

		// Probably also doable for BSD.
		// http://freebsd.1045724.n5.nabble.com/How-to-get-stack-bounds-of-current-process-td4053477.html

	#endif

	if(moduleNameCapacity > 0)
		pModuleName[0] = 0;

	return 0;
}


/*
	uint64_t GetLibraryAddressLinux(const char* pModuleName) 
	{
		// The output of reading /proc/self/maps is like the following (there's no leading space on each line).
		// We look for entries that have r-x as the first three flags, as they are executable modules.
		// The format is (http://linux.die.net/man/5/proc):
		// <begin address>-<end address> <flags> <offset> <device major>:<device minor> <inode> <path>
		//
		// 00400000-0040b000 r-xp 00000000 08:01 655382                             /bin/cat
		// 0060a000-0060b000 r--p 0000a000 08:01 655382                             /bin/cat
		// 0060b000-0060c000 rw-p 0000b000 08:01 655382                             /bin/cat
		// 0060c000-0062d000 rw-p 00000000 00:00 0                                  [heap]
		// 7ffff77b5000-7ffff7a59000 r--p 00000000 08:01 395618                     /usr/lib/locale/locale-archive
		// 7ffff7a59000-7ffff7bd3000 r-xp 00000000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7bd3000-7ffff7dd2000 ---p 0017a000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd2000-7ffff7dd6000 r--p 00179000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd6000-7ffff7dd7000 rw-p 0017d000 08:01 1062643                    /lib/libc-2.12.1.so
		// 7ffff7dd7000-7ffff7ddc000 rw-p 00000000 00:00 0 
		// 7ffff7ddc000-7ffff7dfc000 r-xp 00000000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7fd9000-7ffff7fdc000 rw-p 00000000 00:00 0 
		// 7ffff7ff9000-7ffff7ffb000 rw-p 00000000 00:00 0 
		// 7ffff7ffb000-7ffff7ffc000 r-xp 00000000 00:00 0                          [vdso]
		// 7ffff7ffc000-7ffff7ffd000 r--p 00020000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7ffd000-7ffff7ffe000 rw-p 00021000 08:01 1062651                    /lib/ld-2.12.1.so
		// 7ffff7ffe000-7ffff7fff000 rw-p 00000000 00:00 0 
		// 7ffffffde000-7ffffffff000 rw-p 00000000 00:00 0                          [stack]
		// ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

		uint64_t baseAddress = 0;
		FILE*    file = fopen("/proc/self/maps", "rt"); 

		if(file)
		{
			size_t moduleNameLength = strlen(pModuleName); 
			char   lineBuffer[512]; 

			while(fgets(lineBuffer, sizeof lineBuffer, file) != NULL) 
			{ 
				size_t lineLength = strlen(lineBuffer); 

				if((lineLength > 0) && (lineBuffer[lineLength - 1] == '\n'))
					lineBuffer[--lineLength] = '\0'; 

				if((lineLength >= moduleNameLength) && 
					memcmp(lineBuffer + lineLength - moduleNameLength, pModuleName, moduleNameLength) == 0)
				{
					uint64_t start, end, offset; 
					char     flags[4]; 

					if(EA::StdC::Sscanf(lineBuffer, "%I64x-%I64x %c%c%c%c %I64x", &start, &end, 
								 &flags[0], &flags[1], &flags[2], &flags[3], &offset) == 7) 
					{ 
						if((flags[0] == 'r') && (flags[1] == '-') && (flags[2] == 'x')) // If this looks like an executable module...
						{ 
							// Note: I don't understand from the Linux documentation what the 'offset' value really means
							// and how we are supposed to use it. Example code shows it being subtracted from offset, though 
							// offset is usually 0.
							baseAddress = (start - offset); 
							break; 
						}
					}
				}
			}

			fclose(file);
		}

		return baseAddress; 
   } 
*/


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
	// This is doable for Linux-based platforms via fopen("/proc/self/maps")
	// Probably also doable for BSD.
	// http://freebsd.1045724.n5.nabble.com/How-to-get-stack-bounds-of-current-process-td4053477.html
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

	return (void*)((uintptr_t)pLimit & ~4095); // Round down to nearest page, as the stack grows downward.
}



} // namespace Thread
} // namespace EA







