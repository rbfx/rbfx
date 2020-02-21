///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>

#if EATHREAD_DEBUG_DETAIL_ENABLED
	#include <EAStdC/EASprintf.h>
#endif

#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	#pragma warning(push, 0)
	#include <Windows.h>
	#include <winternl.h>
	#pragma warning(pop)
#endif

#if defined(EA_PLATFORM_UNIX)
	#include <pthread.h>
	#include <eathread/eathread.h>
#endif

#if defined(EA_COMPILER_CLANG)
	#include <unwind.h>
#endif

namespace EA
{
namespace Thread
{


#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	EATHREADLIB_API void GetInstructionPointer(void*& pInstruction)
	{
		CONTEXT context;

		// Apparently there is no need to memset the context struct.
		context.ContextFlags = CONTEXT_ALL;
		RtlCaptureContext(&context);

		// Possibly use the __emit intrinsic. http://msdn.microsoft.com/en-us/library/ms933778.aspx
		pInstruction = (void*)(uintptr_t)context.___; // To do.
	}
#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
	EATHREADLIB_API void GetInstructionPointer(void*& pInstruction)
	{
		// __builtin_return_address returns the address with the Thumb bit set
		// if it's a return to Thumb code. We intentionally preserve this and 
		// don't try to mask it away.
		pInstruction = (void*)(uintptr_t)__builtin_return_address(0);
	}
#else
	EATHREADLIB_API void GetInstructionPointer(void*& /*pInstruction*/)
	{
		//Un-implemented
	}
#endif


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


#if defined(EA_PLATFORM_APPLE)

	// Apple defines a different ABI than the ARM eabi used by Linux and the ABI used
	// by Microsoft. It implements a predictable stack frame system using r7 as the 
	// frame pointer. Documentation:
	//     http://developer.apple.com/library/ios/#documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARMv6FunctionCallingConventions.html
	//
	// Apple ARM stack frame:
	//     struct StackFrame {
	//         StackFrame* mpParentStackFrame;
	//         void* mpReturnPC;
	//     }
	//
	// Basically, Apple uses r7 as a frame pointer. So for any function you are
	// executing, r7 + 4 is the LR passed to us by the caller and is the PC of 
	// the parent. And r7 + 0 is a pointer to the parent's r7. 
	//
	static size_t GetCallstackARMApple(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
	{
		struct StackFrame {
			StackFrame* mpParentStackFrame;
			void*       mpReturnPC;
		};
		
		size_t index = 0;

		if(nReturnAddressArrayCapacity && pContext->mFP) // To consider: Do some basic validation of mFP if it refers to this same thread.
		{
			StackFrame* pStackFrame = static_cast<StackFrame*>((void*)pContext->mFP); // Points to the GetCallstack frame pointer.

			pReturnAddressArray[index++] = pStackFrame->mpReturnPC;  // Should happen to be equal to pContext->mLR.

			while(pStackFrame && pStackFrame->mpReturnPC && (index < nReturnAddressArrayCapacity)) // To consider: do some validation of the PC. We can validate it by making sure it's with 20 MB of our PC and also verify that the instruction before it (be it Thumb or ARM) is a BL or BLX function call instruction.
			{
				pStackFrame = pStackFrame->mpParentStackFrame;

				if(pStackFrame && pStackFrame->mpReturnPC)
					pReturnAddressArray[index++] = pStackFrame->mpReturnPC;
			}
		}

		return index;
	}

#endif

#if defined(EA_COMPILER_CLANG)
	struct CallstackState
	{
		void** current;
		void** end;
	};

	static _Unwind_Reason_Code UnwindCallback(struct _Unwind_Context* context, void* arg)
	{
		CallstackState* state = static_cast<CallstackState*>(arg);
		uintptr_t pc = _Unwind_GetIP(context);
		if (pc)
		{
			if (state->current == state->end)
			{
				return _URC_END_OF_STACK;
			}
			else
			{
				*state->current++ = reinterpret_cast<void*>(pc);
			}
		}
		return _URC_NO_REASON;
	}

#endif

///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// Capture up to nReturnAddressArrayCapacity elements of the call stack, 
// or the whole callstack, whichever is smaller. 
///////////////////////////////////////////////////////////////////////////////

	EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
	{
		void* p;
		CallstackContext context;
		size_t entryCount = 0;

		if(pContext)
			context = *pContext;
		else
		{
			#if defined(__ARMCC_VERSION)
				context.mFP = 0; // We don't currently have a simple way to read fp (which is r7 (Thumb) or r11 (ARM)).
				context.mSP = (uintptr_t)__current_sp();
				context.mLR = (uintptr_t)__return_address();
				GetInstructionPointer(p); // Intentionally don't call __current_pc() or EAGetInstructionPointer, because these won't set the Thumb bit it this is Thumb code.
				context.mPC = (uintptr_t)p;

			#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG) // Including Apple iOS.
				void* spAddress = &context.mSP;
				void* sp;
				asm volatile(
					"add %0, sp, #0\n"
					"str %0, [%1, #0]\n"
						 : "=r"(sp), "+r"(spAddress) :: "memory");

				context.mFP = (uintptr_t)__builtin_frame_address(0);
				context.mLR = (uintptr_t)__builtin_return_address(0);
				GetInstructionPointer(p); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit it this is Thumb code.
				context.mPC = (uintptr_t)p;

			#elif defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
				// Possibly use the __emit intrinsic. Do this by making a __declspec(naked) function that 
				// does nothing but return r14 (move r14 to r0). Need to know the opcode for that.
				// http://msdn.microsoft.com/en-us/library/ms933778.aspx
				#error Need to complete this somehow.
				context.mFP = 0; 
				context.mLR = 0;
				context.mSP = 0;
				GetInstructionPointer(p); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit it this is Thumb code.
				context.mPC = (uintptr_t)p;
			#endif
		}

		#if defined(__APPLE__)
			// We have reason to believe that the following should be reliable. But if it's not then we should
			// just call the code below.
			entryCount = GetCallstackARMApple(pReturnAddressArray, nReturnAddressArrayCapacity, &context);

			if(entryCount >= 3) // If GetCallstackARMApple seems to have been successful, use it. Else fall through to the more complicated code below. 
				return entryCount;
		#elif defined(EA_COMPILER_CLANG)
			CallstackState state = { pReturnAddressArray, pReturnAddressArray + nReturnAddressArrayCapacity };
			_Unwind_Backtrace(UnwindCallback, &state);

			entryCount = state.current - pReturnAddressArray;
		#else
			EA_UNUSED(pReturnAddressArray);
			EA_UNUSED(nReturnAddressArrayCapacity);
			EA_UNUSED(context);
		#endif

		EA_UNUSED(p);
		return entryCount;
	}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
	context.mSP = pContext->mGpr[13];
	context.mLR = pContext->mGpr[14];
	context.mPC = pContext->mGpr[15];
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* /*address*/, char* pModuleName, size_t /*moduleNameCapacity*/)
{
	pModuleName[0] = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Under Windows, the threadId parameter is expected to be a thread HANDLE, 
// which is different from a windows integer thread id.
// On Unix the threadId parameter is expected to be a pthread id.
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	memset(&context, 0, sizeof(context));

	// True Linux-based ARM platforms (usually tablets and phones) can use pthread_attr_getstack.
	#if defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_IPHONE)
		if((threadId == (intptr_t)kThreadIdInvalid) || 
		   (threadId == (intptr_t)kThreadIdCurrent) || 
		   (threadId == (intptr_t)EA::Thread::GetThreadId()))
		{
			void* p;

			// TODO: make defines of this so that the implementation between us and GetCallstack remains the same
			#if defined(__ARMCC_VERSION)
				context.mSP = (uint32_t)__current_sp();
				context.mLR = (uint32_t)__return_address();
				context.mPC = (uint32_t)__current_pc();

			#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
				// register uintptr_t current_sp asm ("sp");
				p = __builtin_frame_address(0);
				context.mSP = (uintptr_t)p;

				p = __builtin_return_address(0);
				context.mLR = (uint32_t)p;

				EAGetInstructionPointer(p);
				context.mPC = reinterpret_cast<uintptr_t>(p);

			#elif defined(_MSC_VER)
				context.mSP = 0;

				#error EACallstack::GetCallstack: Need a way to get the return address (register 14)
				// Possibly use the __emit intrinsic. Do this by making a __declspec(naked) function that 
				// does nothing but return r14 (move r14 to r0). Need to know the opcode for that.
				// http://msdn.microsoft.com/en-us/library/ms933778.aspx
				context.mLR = 0;

				EAGetInstructionPointer(p);
				context.mPC = reinterpret_cast<uintptr_t>(p);
			#endif

			context.mStackBase    = (uintptr_t)GetStackBase();
			context.mStackLimit   = (uintptr_t)GetStackLimit();
			context.mStackPointer = context.mSP;

			return true;
		}
		// Else haven't implemented getting the stack info for other threads

	#else
		// Not currently implemented for the given platform.
		EA_UNUSED(threadId);

	#endif

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
// as can be seen from the logic below. For example iPhone probably doesn't need it.
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
		// Can't call GetStackLimit() because doing so would disturb the stack. 
		// As of this writing, we don't have an EAGetStackTop macro which could do this.
		// So we implement it inline here.
		#if   defined(__ARMCC_VERSION)
			pStackBase = (void*)__current_sp();
		#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
			pStackBase = __builtin_frame_address(0);
		#endif

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

	#if   defined(__ARMCC_VERSION)
		void* pStack = (void*)__current_sp();
	#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
		void* pStack = __builtin_frame_address(0);
	#else
		void* pStack = NULL;  // TODO:  determine fix.
		pStack = &pStack;
	#endif

	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.
}



} // namespace Thread
} // namespace EA





