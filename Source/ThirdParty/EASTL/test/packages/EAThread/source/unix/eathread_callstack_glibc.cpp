///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <string.h>
#include <pthread.h>

#include <eathread/eathread_storage.h>

#if EATHREAD_GLIBC_BACKTRACE_AVAILABLE
	#include <signal.h>
	#include <execinfo.h>
#endif


namespace EA
{
namespace Thread
{


///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
EATHREADLIB_API void GetInstructionPointer(void*& pInstruction)
{
	pInstruction = __builtin_return_address(0);
}


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EATHREADLIB_API void InitCallstack()
{
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EATHREADLIB_API void ShutdownCallstack()
{
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// Capture up to nReturnAddressArrayCapacity elements of the call stack, 
// or the whole callstack, whichever is smaller. 
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	EA_UNUSED(pContext);

	#if EATHREAD_GLIBC_BACKTRACE_AVAILABLE
		size_t count = 0;

		// The pContext option is not currently supported.
		if(pContext == NULL)
		{
			count = (size_t)backtrace(pReturnAddressArray, (int)nReturnAddressArrayCapacity);
			if(count > 0)
			{
				--count; // Remove the first entry, because it refers to this function and by design we don't include this function.
				memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
			}
		}
		
		return count;

	#elif defined(__APPLE__) && defined(EA_PROCESSOR_X86)
		// Apple's ABI defines a callstack frame system.
		// http://developer.apple.com/library/mac/#documentation/DeveloperTools/Conceptual/LowLevelABI/100-32-bit_PowerPC_Function_Calling_Conventions/32bitPowerPC.html#//apple_ref/doc/uid/TP40002438-SW20
		// Apple x86 stack frame:
		//     struct StackFrame {
		//         StackFrame* mpParentStackFrame;
		//         void*       mpParentCR;
		//         void*       mpReturnPC;
		//     }
		//
		//struct StackFrame {                        To do: Re-write this in terms of StackFrame and pContext.
		//    StackFrame* mpParentStackFrame;
		//    void*       mpParentCR;
		//    void*       mpReturnPC;
		//};

		size_t index = 0;

		if(nReturnAddressArrayCapacity)
		{
			void*  fpParent = __builtin_frame_address(1);  // frame pointer of caller.
			void** fp;

			pReturnAddressArray[index++] = __builtin_return_address(0);
			fp = (void**)fpParent;

			while(fp && *fp && (index < nReturnAddressArrayCapacity))
			{
				fpParent = *fp;
				fp = fpParent;

				if(*fp)
					pReturnAddressArray[index++] = *(fp + 2);
			}
		}

		return index;

	#else
		EA_UNUSED(pReturnAddressArray);
		EA_UNUSED(nReturnAddressArrayCapacity);

		return 0;
	#endif
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
	#elif defined(EA_PROCESSOR_ARM)
		context.mSP  = pContext->mGpr[13];
		context.mLR  = pContext->mGpr[14];
		context.mPC  = pContext->mGpr[15];
	#else
		// To do.
	#endif
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


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Under Windows, the threadId parameter is expected to be a thread HANDLE, 
// which is different from a windows integer thread id.
// On Unix the threadId parameter is expected to be a pthread id.
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t /*threadId*/)
{
	//if(threadId == pthread_self()) // Note that at least for MacOS, it's possible to get other threads' info.
	{
		#if defined(EA_PROCESSOR_X86)
			context.mEIP = (uint32_t)__builtin_return_address(0);
			context.mESP = (uint32_t)__builtin_frame_address(1);
			context.mEBP = 0;
		#else
			// To do.
		#endif
		
		return true;
	}
	
	// Not currently implemented for the given platform.
	memset(&context, 0, sizeof(context));
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	return GetCallstackContext(context, sysThreadId);
}


// To do: Remove the usage of sStackBase for the platforms that it's not needed,
// as can be seen from the logic below. For example Mac OSX probably doesn't need it.
static EA::Thread::ThreadLocalStorage sStackBase;

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
	#if defined(EA_PLATFORM_UNIX)
		void* pBase;
		if(GetPthreadStackInfo(&pBase, NULL))
			return pBase;
	#endif

	// Else we require the user to have set this previously, usually via a call 
	// to SetStackBase() in the start function of this currently executing 
	// thread (or main for the main thread).
	return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	#if defined(EA_PLATFORM_UNIX)
		void* pLimit;
		if(GetPthreadStackInfo(NULL, &pLimit))
			return pLimit;
	#endif

	// If this fails then we might have an issue where you are using GCC but not 
	// using the GCC standard library glibc. Or maybe glibc doesn't support 
	// __builtin_frame_address on this platform. Or maybe you aren't using GCC but
	// rather a compiler that masquerades as GCC (common situation).
	void* pStack = __builtin_frame_address(0);
	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.

}


} // namespace Thread
} // namespace EA



