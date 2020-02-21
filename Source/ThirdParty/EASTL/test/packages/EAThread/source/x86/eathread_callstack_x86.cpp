///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a manual callstack reader for the x86 platform.
// It is an alternative to EACallstack_Win32.cpp, which implements 
// callstack reading by standard Microsoft debug functions. This file
// on the other hand avoids the use of Microsoft calls and instead
// directly walks the callstack via reading the registers and x86 
// instructions. This file can also be used to read x86 callstacks on 
// non-Microsoft x86 platforms such as Linux.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>

#if EATHREAD_GLIBC_BACKTRACE_AVAILABLE
	#include <signal.h>
	#include <execinfo.h>
#endif

#if defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86_64)
	#pragma warning(push, 0)
	#include <math.h>   // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#include <intrin.h>
	#pragma warning(pop)
#endif

#if defined(EA_COMPILER_CLANG)
	#include <unwind.h>
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
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EATHREADLIB_API void ShutdownCallstack()
{
}


#if defined(_MSC_VER)


///////////////////////////////////////////////////////////////////////////////
// ReturnAddressToCallingAddress
//
// Convert the return address to the calling address.
//
void* ReturnAddressToCallingAddress(unsigned char* return_address)
{
	// While negative array indices can be considered non-idiomatic it
	// was felt that they are semantically appropriate as this code bases
	// its comparisons from the return address and that it would be cleaner
	// than using *(return_address - index).

	// Three op-codes are used for the call instruction, 9A, E8, and FF. 
	// For a reference on the IA32 instruction format, see:
	// http://www.cs.princeton.edu/courses/archive/spr06/cos217/reading/ia32vol2.pdf

	// 9A cp - CALL ptr16:32 (7-byte)
	if(return_address[-7] == 0x9A)
	{
		return (return_address - 7);
	}
	// E8 cd - CALL rel32 (5-byte)        
	else if(return_address[-5] == 0xE8)
	{
		return (return_address - 5);
	}
	else
	{
		// The third opcode to specify "call" instructions is FF. 
		// Unfortunately this instruction also needs the succeeding ModR/M
		// byte to fully determine instruction length. The SIB value is
		// another byte used for extending the range of addressing modes
		// supported by the ModR/M byte. The values of this ModR/M byte
		// used in conjunction with the call instruction are as follows:
		//
		// 7-byte call:
		// FF [ModR/M] [SIB] [4-byte displacement]
		//    * ModR/M is either 0x94 or 0x9C
		//
		// 6-byte call:
		// FF [ModR/M] [4-byte displacement]
		//    * ModR/M can be:
		//      * 0x90 - 0x9F EXCLUDING 0x94 or 0x9C
		//      * 0x15 or 0x1D
		//
		// 4-byte call:
		// FF [ModR/M] [SIB] [1-byte displacement]
		//    * ModR/M is either 0x54 or 0x5C
		//
		// 3-byte call:
		// FF [ModR/M] [1-byte displacement]
		//    * ModR/M can be:
		//      * 0x50 - 0x5F EXCLUDING 0x54 or 0x5C
		// FF [ModR/M] [SIB]
		//    * ModR/M is either 0x14 or 0x1C
		//
		// 2-byte call:
		// FF [ModR/M]
		//    * ModR/M can be:
		//      * 0xD0 - 0xDF
		//      * 0x10 - 0x1F EXCEPT 0x14, 0x15, 0x1C, or 0x1D

		// The mask of F8 is used because we want to mask out the bottom 
		// three bits (which are most often used for register selection).
		const unsigned char rm_mask = 0xF8;
		
		// 7-byte format:
		if(return_address[-7] == 0xFF && 
		  (return_address[-6] == 0x94 || return_address[-6] == 0x9C))
		{
			return (return_address - 7);
		}
		// 6-byte format:
		// FF [ModR/M] [4-byte displacement] 
		else if(return_address[-6] == 0xFF &&
			  ((return_address[-5] & rm_mask) == 0x90 || (return_address[-5] & rm_mask) == 0x98) &&
			   (return_address[-5] != 0x94 && return_address[-5] != 0x9C))
		{
			return (return_address - 6);
		}
		// Alternate 6-byte format:
		else if(return_address[-6] == 0xFF &&
			   (return_address[-5] == 0x15 || return_address[-5] == 0x1D))
		{
			return (return_address - 6);
		}
		// 4-byte format:
		// FF [ModR/M] [SIB] [1-byte displacement] 
		else if(return_address[-4] == 0xFF &&
				(return_address[-3] == 0x54 || return_address[-3] == 0x5C))
		{
			return (return_address - 4);
		}
		// 3-byte format:
		// FF [ModR/M] [1-byte displacement] 
		else if(return_address[-3] == 0xFF && 
			  ((return_address[-2] & rm_mask) == 0x50 || (return_address[-2] & rm_mask) == 0x58) &&
			   (return_address[-2] != 0x54 && return_address[-2] != 0x5C))
		{
			return (return_address - 3);
		}
		// Alternate 3-byte format:
		// FF [ModR/M] [SIB]
		else if(return_address[-3] == 0xFF &&
			   (return_address[-2] == 0x14 || return_address[-2] == 0x1C))
		{
			return (return_address - 3);
		}
		// 2-byte calling format:
		// FF [ModR/M]
		else if(return_address[-2] == 0xFF &&
			  ((return_address[-1] & rm_mask) == 0xD0 || (return_address[-1] & rm_mask) == 0xD8))
		{
			return (return_address - 2);
		}
		// Alternate 2-byte calling format:
		// FF [ModR/M]
		else if(return_address[-2] == 0xFF && 
			  ((return_address[-1] & rm_mask) == 0x10 || (return_address[-1] & rm_mask) == 0x18) &&
			   (return_address[-1] != 0x14 && return_address[-1] != 0x15 &&
				return_address[-1] != 0x1C && return_address[-1] != 0x1D))
		{
			return (return_address - 2);
		}
		else
		{
			EA_FAIL_MSG("EACallstack: Unable to determine calling address!");
		}
	}
	
	EA_FAIL_MSG("EACallstack: Unable to determine calling address!");
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// Capture up to nReturnAddressArrayCapacity elements of the call stack, 
// or the whole callstack, whichever is smaller. 
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	size_t index = 0;

	// The pContext option is not currently supported.
	unsigned char* local_ebp;

	if(pContext)
		local_ebp = (unsigned char*)pContext->mEBP;
	else
	{
		__asm mov local_ebp, ebp

		// Remove the top return address because that's the one for this function
		// which we most likely don't want in our pReturnAddressArray.
		local_ebp = *(reinterpret_cast<unsigned char**>(local_ebp));
	}

	for(index = 0; index < nReturnAddressArrayCapacity; ++index)
	{
		unsigned char* return_address_ptr = *(reinterpret_cast<unsigned char**>(local_ebp + 4));

		if(return_address_ptr == 0)
		{
			pReturnAddressArray[index] = 0;
			break;
		}
		else
		{
			pReturnAddressArray[index] = ReturnAddressToCallingAddress(return_address_ptr);
			local_ebp = *(reinterpret_cast<unsigned char**>(local_ebp));
		}
	}

	return index;
}

#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)

EATHREADLIB_API void GetInstructionPointer(void *&p)
{
	p = __builtin_return_address(0);
}

#if !EATHREAD_GLIBC_BACKTRACE_AVAILABLE && defined(EA_COMPILER_CLANG)
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
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	#if EATHREAD_GLIBC_BACKTRACE_AVAILABLE
		size_t count = 0;

		// The pContext option is not currently supported.
		if(pContext == NULL)
		{
			count = (size_t)backtrace(pReturnAddressArray, nReturnAddressArrayCapacity);
			if(count > 0)
			{
				--count; // Remove the first entry, because it refers to this function and by design we don't include this function.
				memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
			}
		}
		
		return count;
	#elif defined(EA_COMPILER_CLANG)
		size_t count = 0;

		// The pContext option is not currently supported.
		if (pContext == NULL)
		{
			CallstackState state = { pReturnAddressArray, pReturnAddressArray + nReturnAddressArrayCapacity };
			_Unwind_Backtrace(UnwindCallback, &state);

			count = state.current - pReturnAddressArray;
			if (count > 0)
			{
				--count; // Remove the first entry, because it refers to this function and by design we don't include this function.
				memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
			}
		}
		return count;
	#else
		size_t index = 0;

		// The pContext option is not currently supported.
		if(pContext == NULL)
		{
			void** sp;
			void** new_sp;

			#if defined(__i386__)
				// Stack frame format:
				//    sp[0]   pointer to previous frame
				//    sp[1]   caller address
				//    sp[2]   first argument
				//    ...
				sp = (void**)&pReturnAddressArray - 2;
			#elif defined(__x86_64__)
				// Arguments are passed in registers on x86-64, so we can't just offset from &pReturnAddressArray.
				sp = (void**) __builtin_frame_address(0);
			#endif

			for(int count = 0; sp && (index < nReturnAddressArrayCapacity); sp = new_sp, ++count)
			{
				if(count > 0) // We skip the current frame.
					pReturnAddressArray[index++] = *(sp + 1);

				new_sp = (void**)*sp;

				if((new_sp < sp) || (new_sp > (sp + 100000)))
					break;
			}
		}

		return index;
	#endif
}

#endif


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
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* address, char* pModuleName, size_t moduleNameCapacity)
{
	#ifdef EA_PLATFORM_WINDOWS
		MEMORY_BASIC_INFORMATION mbi;

		if(VirtualQuery(address, &mbi, sizeof(mbi)))
		{
			HMODULE hModule = (HMODULE)mbi.AllocationBase;

			if(hModule)
				return GetModuleFileNameA(hModule, pModuleName, (DWORD)moduleNameCapacity);
		}
	#else
		// Not currently implemented for the given platform.
		EA_UNUSED(address);
		EA_UNUSED(moduleNameCapacity);
	#endif

	pModuleName[0] = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
	#ifdef EA_PLATFORM_WINDOWS
		MEMORY_BASIC_INFORMATION mbi;

		if(VirtualQuery(pAddress, &mbi, sizeof(mbi)))
			return (ModuleHandle)mbi.AllocationBase;
	#else
		// Not currently implemented for the given platform.
		EA_UNUSED(pAddress);
	#endif

	return 0;
}



#ifdef EA_PLATFORM_WINDOWS
	///////////////////////////////////////////////////////////////////////////////
	// GetThreadIdFromThreadHandleWin32
	//
	static DWORD GetThreadIdFromThreadHandleWin32(HANDLE hThread)
	{
		struct THREAD_BASIC_INFORMATION_WIN32
		{
			BOOL  ExitStatus;
			PVOID TebBaseAddress;
			DWORD UniqueProcessId;
			DWORD UniqueThreadId;
			DWORD AffinityMask;
			DWORD Priority;
			DWORD BasePriority;
		};

		static HMODULE hKernel32 = NULL;
		if(!hKernel32)
			hKernel32 = LoadLibraryA("kernel32.dll");

		if(hKernel32)
		{
			typedef DWORD (WINAPI *GetThreadIdFunc)(HANDLE);

			static GetThreadIdFunc pGetThreadIdFunc = NULL;
			if(!pGetThreadIdFunc)
			   pGetThreadIdFunc = (GetThreadIdFunc)(uintptr_t)GetProcAddress(hKernel32, "GetThreadId");

			if(pGetThreadIdFunc)
				return pGetThreadIdFunc(hThread);
		}


		static HMODULE hNTDLL = NULL; 
		if(!hNTDLL)
			hNTDLL = LoadLibraryA("ntdll.dll");

		if(hNTDLL)
		{
			typedef LONG (WINAPI *NtQueryInformationThreadFunc)(HANDLE, int, PVOID, ULONG, PULONG);

			static NtQueryInformationThreadFunc pNtQueryInformationThread = NULL;
			if(!pNtQueryInformationThread)
			   pNtQueryInformationThread = (NtQueryInformationThreadFunc)(uintptr_t)GetProcAddress(hNTDLL, "NtQueryInformationThread");

			if(pNtQueryInformationThread)
			{
				THREAD_BASIC_INFORMATION_WIN32 tbi;

				if(pNtQueryInformationThread(hThread, 0, &tbi, sizeof(tbi), NULL) == 0)
					return tbi.UniqueThreadId;
			}
		}

		return 0;
	}

#endif // #ifdef EA_PLATFORM_WINDOWS


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Under Windows, the threadId parameter is expected to be a thread HANDLE, 
// which is different from a windows integer thread id.
// On Unix the threadId parameter is expected to be a pthread id.
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	#ifdef EA_PLATFORM_WINDOWS
		if((threadId == kThreadIdInvalid) || (threadId == kThreadIdCurrent))
			threadId = (intptr_t)GetCurrentThread(); // GetCurrentThread returns a thread 'pseudohandle' and not a real thread handle.

		const DWORD dwThreadIdCurrent = GetCurrentThreadId();
		const DWORD dwThreadIdArg     = GetThreadIdFromThreadHandleWin32((HANDLE)threadId);
		CONTEXT win32CONTEXT;

		if(dwThreadIdCurrent == dwThreadIdArg)
		{
			// With VC++, EIP is not accessible directly, but we can use an assembly trick to get it.
			// VC++ and Intel C++ compile this fine, but Metrowerks 7 has a bug and fails.
				__asm{
					mov win32CONTEXT.Ebp, EBP
					mov win32CONTEXT.Esp, ESP
					call GetEIP
					GetEIP:
					pop win32CONTEXT.Eip
				}
		}
		else
		{
			// In this case we are working with a separate thread, so we suspend it
			// and read information about it and then resume it. Windows requires that 
			// GetThreadContext be called on a thread that is stopped. There is a small
			// problem when running under the VC++ debugger: It reportedly calls
			// ResumeThread on suspended threads itself and so the sequence below can
			// be affected when stepped through with a debugger.
			SuspendThread((HANDLE)threadId);
			win32CONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
			GetThreadContext((HANDLE)threadId, &win32CONTEXT);
			ResumeThread((HANDLE)threadId);
		}

		context.mEBP = (uint32_t)win32CONTEXT.Ebp;
		context.mESP = (uint32_t)win32CONTEXT.Esp;
		context.mEIP = (uint32_t)win32CONTEXT.Eip;

		return true;

	#else
		// Not currently implemented for the given platform.
		EA_UNUSED(context);
		EA_UNUSED(threadId);
		memset(&context, 0, sizeof(context));
		return false;
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	#ifdef EA_PLATFORM_WINDOWS
		// To do: Implement this if/when necessary.
		EA_UNUSED(context);
		EA_UNUSED(sysThreadId);
		return false;
	#else
		return GetCallstackContext(context, sysThreadId);
	#endif
}


// To do: Remove the usage of sStackBase for the platforms that it's not needed,
// as can be seen from the logic below. For example Linux in general doesn't need it.
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
		#if defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86)
			__asm mov pStackBase, ESP
		#elif defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86_64)
			pStackBase = _AddressOfReturnAddress();   // Need to #include <intrin.h> for this.
		#elif (defined(EA_COMPILER_GNUC) && defined(EA_PROCESSOR_X86)) || (defined(EA_COMPILER_CLANG) && defined(EA_PROCESSOR_X86))
			asm volatile("mov %%esp, %0" : "=g"(pStackBase));
		#elif (defined(EA_COMPILER_GNUC) && defined(EA_PROCESSOR_X86_64)) || (defined(EA_COMPILER_CLANG) && defined(EA_PROCESSOR_X86_64))
			asm volatile("mov %%rsp, %0" : "=g"(pStackBase));
		#else
			#error Unsupported compiler/processor combination.
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

	// We call strcat in order to create a stack frame here and to avoid a 
	// compiler warning about returning a local variable in the case that we 
	// simply returned &stackLocation itself.
	char stackLocation = 0;
	char* pStack = strcat(&stackLocation, "");
	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.
}


} // namespace Callstack
} // namespace EA




