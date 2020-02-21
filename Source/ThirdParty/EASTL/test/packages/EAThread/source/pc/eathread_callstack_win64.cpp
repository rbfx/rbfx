///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <stdio.h>
#include <string.h>
#include <eathread/eathread_storage.h>

#if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0500)
	#undef  _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#ifdef _MSC_VER
	#pragma warning(push, 0)
	#include <Windows.h>
	#include <math.h>       // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#include <intrin.h>
	#pragma intrinsic(_ReturnAddress)
	#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		#include <winternl.h>
	#else
		// Temporary while waiting for formal support:
		extern "C" NTSYSAPI PEXCEPTION_ROUTINE NTAPI RtlVirtualUnwind(DWORD, DWORD64, DWORD64, PRUNTIME_FUNCTION, PCONTEXT, PVOID*, PDWORD64, PKNONVOLATILE_CONTEXT_POINTERS);
		extern "C" WINBASEAPI DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);
	#endif
	#pragma warning(pop)
#else
	#include <Windows.h>
	#include <winternl.h>
#endif


// Disable optimization of this code under VC++ for x64.
// This is due to some as-yet undetermined crash that happens  
// when compiler optimizations are enabled for this code.
// This function is not performance-sensitive and so disabling 
// optimizations shouldn't matter.
#if defined(_MSC_VER) && (defined(_M_AMD64) || defined(_WIN64))
	#pragma optimize("", off) 
#endif


///////////////////////////////////////////////////////////////////////////////
// Stuff that is supposed to be in windows.h and/or winternl.h but isn't
// consistently present in all versions.
//
#ifndef UNW_FLAG_NHANDLER
	#define UNW_FLAG_NHANDLER 0
#endif

#ifndef UNWIND_HISTORY_TABLE_SIZE
	extern "C"
	{
		#define UNWIND_HISTORY_TABLE_SIZE    12
		#define UNWIND_HISTORY_TABLE_NONE     0
		#define UNWIND_HISTORY_TABLE_GLOBAL   1
		#define UNWIND_HISTORY_TABLE_LOCAL    2

		typedef struct _UNWIND_HISTORY_TABLE_ENTRY
		{
			ULONG64 ImageBase;
			PRUNTIME_FUNCTION FunctionEntry;
		} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;


		typedef struct _UNWIND_HISTORY_TABLE
		{
			ULONG Count;
			UCHAR Search;
			ULONG64 LowAddress;
			ULONG64 HighAddress;
			UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
		} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;


		PVOID WINAPI RtlLookupFunctionEntry(ULONG64 ControlPC, PULONG64 ImageBase, PUNWIND_HISTORY_TABLE HistoryTable);

		#if !defined(_MSC_VER) || (_MSC_VER < 1500) // if earlier than VS2008...
			typedef struct _KNONVOLATILE_CONTEXT_POINTERS
			{ 
				PULONGLONG dummy; 
			} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS; 

			typedef struct _FRAME_POINTERS
			{
				ULONGLONG MemoryStackFp;
				ULONGLONG BackingStoreFp;
			} FRAME_POINTERS, *PFRAME_POINTERS;

			ULONGLONG WINAPI RtlVirtualUnwind(ULONG HandlerType, ULONGLONG ImageBase, ULONGLONG ControlPC, 
													  PRUNTIME_FUNCTION FunctionEntry, PCONTEXT ContextRecord, PBOOLEAN InFunction, 
													  PFRAME_POINTERS EstablisherFrame, PKNONVOLATILE_CONTEXT_POINTERS ContextPointers);
		#endif
	}
#endif

extern "C" WINBASEAPI DWORD WINAPI GetThreadId(_In_ HANDLE hThread);

///////////////////////////////////////////////////////////////////////////////





namespace EA
{
namespace Thread
{


/* To consider: Enable usage of this below.
///////////////////////////////////////////////////////////////////////////////
// IsAddressReadable
//
static bool IsAddressReadable(const void* pAddress)
{
	bool bPageReadable;
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery(pAddress, &mbi, sizeof(mbi)))
	{
		const DWORD flags = (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE);
		bPageReadable = (mbi.State == MEM_COMMIT) && ((mbi.Protect & flags) != 0);
	}
	else
		bPageReadable = false;

	return bPageReadable;
}
*/


#if !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	// GetRSP
	//
	// Returns the RSP of the caller.
	// 
	// We could also solve this with the following asm function.
	// .CODE
	//      GetRSP PROC
	//      mov rax, rsp
	//      add rax, 8
	//      ret
	//      GetRSP ENDP
	//      END
	//
	static EA_NO_INLINE void* GetRSP()
	{
		#if defined(_MSC_VER)
			uintptr_t ara = (uintptr_t)_AddressOfReturnAddress();
		#else
			uintptr_t ara = (uintptr_t)__builtin_frame_address();
		#endif
		return (void*)(ara + 8);
	}
#endif


///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
EATHREADLIB_API EA_NO_INLINE void GetInstructionPointer(void*& pInstruction)
{
	#if defined(_MSC_VER)
		pInstruction = _ReturnAddress();
	#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
		pInstruction = __builtin_return_address(0);
	#else
		void* pReturnAddressArray[2] = { 0, 0 };

		GetCallstack(pReturnAddressArray, 2, NULL);
		pInstruction = pReturnAddressArray[1]; // This is the address of the caller.
	#endif
}


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


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// With the x64 (a.k.a. x86-64) platform, the CPU supports call stack tracing
// natively, by design. This is as opposed to the x86 platform, in which call
// stack tracing (a.k.a. unwinding) is a crap-shoot. The Win64 OS provides
// two functions in particular that take care of the primary work of stack
// tracing: RtlLookupFunctionEntry and RtlVirtualUnwind/RtlUnwindEx.
//
// On x64 each non-leaf function must have an info struct (unwind metadata) in 
// static memory associated with it. That info struct describes the prologue and  
// epilogue of the function in such a way as tell identify where its return address
// is stored and how to restore non-volatile registers of the caller so that
// an unwind can happen during an exception and C++ object destructors can 
// be called, etc. In order to implement a stack unwinding function for 
// Microsoft x64, you can go the old x86 route of requiring the compiler to
// emit stack frames and reading the stack frame values. But that would work 
// only where the frames were enabled (maybe just debug builds) and wouldn't
// work with third party code that didn't use the frames. But the Microsoft
// x64 ABI -requires- that all non-leaf functions have the info struct 
// described above. And Microsoft provides the Rtl functions mentioned above 
// to read the info struct (RtlLookupFunctionEntry) and use it to unwind a 
// frame (RtlVirtualUnwind/RtlUnwindEx), whether you are in an exception or not. 
// 
// RtlVirtualUnwind implements a virtual (pretend) unwind of a stack and is 
// useful for reading a call stack and its unwind info without necessarily 
// executing an unwind (like in an exception handler). RtlVirtualUnwind provides 
// the infrastructure upon which higher level exception and unwind handling 
// support is implemented. It doesn't exist on x86, as x86 exception unwinding
// is entirely done by generated C++ code and isn't in the ABI. The Virtual in 
// RtlVirtualUnwind has nothing to do with virtual memory, virtual functions, 
// or virtual disks.
//
// RtlUnwindEx (replaces RtlUnwind) implements an actual unwind and thus is 
// mostly useful only in the implementation of an exception handler and not 
// for doing an ad-hoc stack trace.
//
// You can't use RtlLookupFunctionEntry on the IP (instruction pointer) of a 
// leaf function, as the compiler isn't guaranteed to generate this info for 
// such functions. But if a leaf function calls RtlLookupFunctionEntry on its
// own IP then it's no longer a leaf function and by virtue of calling RtlLookupFunctionEntry
// the info will necessarily be generated by the compiler. If you want to read
// the info associated with an IP of another function which may be a leaf 
// function, it's best to read the return address of that associated with that
// function's callstack context, which is that that function's rsp register's
// value as a uintptr_t* dereferenced (i.e. rsp holds the address of the 
// return address).
//
// UNWIND_HISTORY_TABLE "is used as a cache to speed up repeated exception handling lookups, 
// and is typically optional as far as usage with RtlUnwindEx goes – though certainly 
// recommended from a performance perspective." This may be useful to us, though we'd need
// to make it a thread-safe static variable or similar and not a local variable.
// History table declaration and preparation for use, which needs to be done per-thread:
//     UNWIND_HISTORY_TABLE unwindHistoryTable;
//     RtlZeroMemory(&unwindHistoryTable, sizeof(UNWIND_HISTORY_TABLE));
//     unwindHistoryTable.Unwind = TRUE;
// To do: Implement usage of the history table for faster callstack tracing.
//
// Reading for anybody wanting to understand this:
//     http://www.nynaeve.net/?p=105
//     http://www.nynaeve.net/?p=106
//     http://blogs.msdn.com/b/freik/archive/2005/03/17/398200.aspx
//     http://www.codemachine.com/article_x64deepdive.html
//     http://blogs.msdn.com/b/ntdebugging/archive/2010/05/12/x64-manual-stack-reconstruction-and-stack-walking.aspx
//     http://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64/
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	CONTEXT           context;
	PRUNTIME_FUNCTION pRuntimeFunction;
	ULONG64           nImageBase = 0;
	ULONG64           nPrevImageBase = 0;
	size_t            nFrameIndex = 0;

	if(pContext)
	{
		RtlZeroMemory(&context, sizeof(context));
		context.Rip          = pContext->mRIP;
		context.Rsp          = pContext->mRSP;
		context.Rbp          = pContext->mRBP;
		context.ContextFlags = CONTEXT_CONTROL; // CONTEXT_CONTROL actually specifies SegSs, Rsp, SegCs, Rip, and EFlags. But for callstack tracing and unwinding, all that matters is Rip and Rsp.

		// In the case where we are calling 0, we might be able to unwind one frame and see if we are now in a valid stack frame for 
		// callstack generation. If not abort, otherwise we continue one frame past where the exception (calling 0) was performed
		if (context.Rip == 0 && context.Rsp != 0)
		{ 
			context.Rip = (ULONG64)(*(PULONG64)context.Rsp); // To consider: Use IsAddressReadable(pFrame) before dereferencing this pointer.
			context.Rsp += 8; // reset the stack pointer (+8 since we know there has been no prologue run requiring a larger number since RIP == 0)
		} 

		if(context.Rip && (nFrameIndex < nReturnAddressArrayCapacity))
			pReturnAddressArray[nFrameIndex++] = (void*)(uintptr_t)context.Rip;
	}
	else // Else we are reading the current thread's callstack.
	{
		// To consider: Don't call the RtlCaptureContext function for EA_WINAPI_PARTITION_DESKTOP and 
		// instead use the simpler version below it which writes Rip/Rsp/Rbp. RtlCaptureContext is much 
		// slower. We need to verify that the 'quality' and extent of returned callstacks is good for 
		// the simpler version before using it exclusively.
		#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			// Apparently there is no need to memset the context struct.
			context.ContextFlags = CONTEXT_ALL; // Actually we should need only CONTEXT_INTEGER, so let's test that next chance we get.
			RtlCaptureContext(&context);

		#elif defined(EA_PLATFORM_XBOXONE) // This probably isn't limited to just this platform, but until we can test any other platforms we'll leave it at just this.
			return RtlCaptureStackBackTrace(1, (ULONG)nReturnAddressArrayCapacity, pReturnAddressArray, NULL);

		#else
			void* ip = NULL;
			EAGetInstructionPointer(ip);
			context.Rip          = (uintptr_t)ip;
			context.Rsp          = (uintptr_t)GetRSP();
			context.Rbp          = 0; // RBP isn't actually needed for stack unwinding on x64, and don't typically need to use it in generated code, as the instruction set provides an efficient way to read/write via rsp offsets. Also, when frame pointers are omitted in the compiler settings then ebp won't be used.
			context.ContextFlags = CONTEXT_CONTROL;
		#endif
	}

	// The following loop intentionally skips the first call stack frame because 
	// that frame corresponds this function (GetCallstack).
	while(context.Rip && (nFrameIndex < nReturnAddressArrayCapacity))
	{
		// Try to look up unwind metadata for the current function.
		nPrevImageBase = nImageBase;
		__try
		{
			pRuntimeFunction = (PRUNTIME_FUNCTION)RtlLookupFunctionEntry(context.Rip, &nImageBase, NULL /*&unwindHistoryTable*/);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// Something went wrong in RtlLookupFunctionEntry, and it is unknown
			// if it is recoverable; so just get out.
			return nFrameIndex;
		}

		if(pRuntimeFunction)
		{
			// RtlVirtualUnwind is not declared in the SDK headers for non-desktop apps, 
			// but for 64 bit targets it's always present and appears to be needed by the
			// existing RtlUnwindEx function. If in the end we can't use RtlVirtualUnwind
			// and Microsoft doesn't provide an alternative, we can implement RtlVirtualUnwind
			// ourselves manually (not trivial, but has the best results) or we can use
			// the old style stack frame following, which works only when stack frames are 
			// enabled in the build, which usually isn't so for optimized builds and for
			// third party code. 

			__try // Under at least the XBox One platform, RtlVirtualUnwind can crash here. It may possibly be due to the context being incomplete.
			{
				VOID*          handlerData = NULL;
				ULONG64        establisherFramePointers[2] = { 0, 0 };
				RtlVirtualUnwind(UNW_FLAG_NHANDLER, nImageBase, context.Rip, pRuntimeFunction, &context, &handlerData,  establisherFramePointers, NULL);                        
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				context.Rip          = NULL;
				context.ContextFlags = 0;
			}
		}
		else
		{
			// If we don't have a RUNTIME_FUNCTION, then we've encountered an error of some sort (mostly likely only for cases of corruption) or leaf function (which doesn't make sense, given that we are moving up in the call sequence). Adjust the stack appropriately.
			context.Rip  = (ULONG64)(*(PULONG64)context.Rsp); // To consider: Use IsAddressReadable(pFrame) before dereferencing this pointer.
			context.Rsp += 8;
		}

		if(context.Rip)
		{
			if(nFrameIndex < nReturnAddressArrayCapacity)
				pReturnAddressArray[nFrameIndex++] = (void*)(uintptr_t)context.Rip;
		}
	}

	return nFrameIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetThreadIdFromThreadHandle
//
// This implementation is the same as the one in EAThread.
// Converts a thread HANDLE (threadId) to a thread id DWORD (sysThreadId).
// Recall that Windows has two independent thread identifier types.
//
EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId)
{
	// Win64 has this function natively, unlike earlier versions of 32 bit Windows.
	return (uint32_t)::GetThreadId((HANDLE)threadId);
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// The threadId is the same thing as the Windows' HANDLE GetCurrentThread() function
// and not the same thing as Windows' GetCurrentThreadId function. See the 
// GetCallstackContextSysThreadId for the latter.
// 
#if EA_USE_CPP11_CONCURRENCY
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, EA::Thread::ThreadId threadId)
{
	// Retrieve the Windows thread identifier from the std::thread::id structure.
	// This is unavoidable because GetCallstackContextSysThreadId compares the value of 'sysThreadId' 
	// against data from the Windows API function 'GetCurrentThreadId' which returns a Windows thread identifier.
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms683183(v=vs.85).aspx
	static_assert(sizeof(_Thrd_t) == sizeof(threadId), "We expect the 'std::thread::id' to have a single member of type '_Thrd_t'.");

	_Thrd_t wThrd;
	memcpy(&wThrd, &threadId, sizeof(wThrd));  // we use memcpy to avoid strict aliasing issues caused by casting to access internal members.
	return GetCallstackContextSysThreadId(context, _Thr_val(wThrd));
}
#else
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	if((threadId == (intptr_t)kThreadIdInvalid) || (threadId == (intptr_t)kThreadIdCurrent))
		threadId = (intptr_t)::GetCurrentThread(); // GetCurrentThread returns a thread 'pseudohandle' and not a real thread handle.

	const DWORD sysThreadId = EA::Thread::GetThreadIdFromThreadHandle(threadId);

	return GetCallstackContextSysThreadId(context, sysThreadId);
}
#endif



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
// A sysThreadId is a Microsoft DWORD thread id, which can be obtained from 
// the currently running thread via GetCurrentThreadId. It can be obtained from
// a Microsoft thread HANDLE via EA::Thread::GetThreadIdFromThreadHandle();
// A DWORD thread id can be converted to a thread HANDLE via the Microsoft OpenThread
// system function.
//

EA_DISABLE_VC_WARNING(4701) // potentially uninitialized local variable 'win64CONTEXT' used

EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	EAT_COMPILETIME_ASSERT(offsetof(EA::Thread::ContextX86_64, Rip)                  == offsetof(CONTEXT, Rip));
	EAT_COMPILETIME_ASSERT(offsetof(EA::Thread::ContextX86_64, VectorRegister)       == offsetof(CONTEXT, VectorRegister));
	EAT_COMPILETIME_ASSERT(offsetof(EA::Thread::ContextX86_64, LastExceptionFromRip) == offsetof(CONTEXT, LastExceptionFromRip));

	const DWORD sysThreadIdCurrent = GetCurrentThreadId();
	CONTEXT     win64CONTEXT;

	if(sysThreadIdCurrent == (DWORD)sysThreadId) // If getting the context of the current thread...
	{
		#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			RtlCaptureContext(&win64CONTEXT); // This function has no return value.
		#else
			void* ip = NULL;
			EAGetInstructionPointer(ip);
			win64CONTEXT.Rip          = (uintptr_t)ip;
			win64CONTEXT.Rsp          = (uintptr_t)GetRSP();
			win64CONTEXT.Rbp          = 0; // RBP isn't actually needed for stack unwinding on x64, and don't typically need to use it in generated code, as the instruction set provides an efficient way to read/write via rsp offsets. Also, when frame pointers are omitted in the compiler settings then ebp won't be used.
			win64CONTEXT.ContextFlags = CONTEXT_CONTROL; // CONTEXT_CONTROL actually specifies SegSs, Rsp, SegCs, Rip, and EFlags. But for callstack tracing and unwinding, all that matters is Rip and Rsp.
		#endif
	}
	else
	{
		#if !defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			// In this case we are working with a separate thread, so we suspend it
			// and read information about it and then resume it. We cannot use this
			// technique to get the context of the current thread unless we do it by
			// spawing a new thread which suspends this thread and calls GetThreadContext.

			HANDLE threadId = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, TRUE, (DWORD)sysThreadId);
			BOOL   result = false;

			EAT_ASSERT(threadId != 0); // If this fails then maybe there's a process security restriction we are running into.
			if(threadId)
			{
				DWORD suspendResult = SuspendThread(threadId);

				if(suspendResult != (DWORD)-1)
				{
					win64CONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
					result = GetThreadContext(threadId, &win64CONTEXT);
					suspendResult = ResumeThread(threadId);
					EAT_ASSERT(suspendResult != (DWORD)-1); EA_UNUSED(suspendResult);
				}

				CloseHandle(threadId);
			}

			if(!result)
			{
				win64CONTEXT.Rip          = 0;
				win64CONTEXT.Rsp          = 0;
				win64CONTEXT.Rbp          = 0;
				win64CONTEXT.ContextFlags = 0;
			}
		#endif
	}

	context.mRIP = win64CONTEXT.Rip;
	context.mRSP = win64CONTEXT.Rsp;
	context.mRBP = win64CONTEXT.Rbp;

	return (context.mRIP != 0);
}

EA_RESTORE_VC_WARNING()



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
	context.mRIP = pContext->Rip;
	context.mRSP = pContext->Rsp;
	context.mRBP = pContext->Rbp;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
size_t GetModuleFromAddress(const void* address, char* pModuleName, size_t moduleNameCapacity)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery(address, &mbi, sizeof(mbi)))
	{
		HMODULE hModule = (HMODULE)mbi.AllocationBase;

		if(hModule)
		{
			#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // GetModuleFileName is desktop API-only.
				// As of the early Windows 8 SDKs, GetModuleFileName is not exposed to non-desktop 
				// apps, though it's apparently nevertheless present in the libraries.
				return GetModuleFileNameA(hModule, pModuleName, (DWORD)moduleNameCapacity);
			#else
				// If it turns out in the end that we really can't do this, then for non-shipping builds
				// we can likely implement a manual version of this via information found through the 
				// TEB structure for the process. 
				return GetModuleFileNameA(hModule, pModuleName, (DWORD)moduleNameCapacity);
			#endif
		}
	}

	pModuleName[0] = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
// The input pAddress must be an address of code and not data or stack space.
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery(pAddress, &mbi, sizeof(mbi)))
	{
		// In Microsoft platforms, the module handle is really just a pointer
		// to the code for the module. It corresponds directly to the information
		// in the map file, though the actual address may have been changed
		// from the value in the map file on loading into memory.
		return (ModuleHandle)mbi.AllocationBase;
	}

	return 0;
}



///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* /*pStackBase*/)
{
	// Nothing to do, as GetStackBase always works on its own.
}

///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	NT_TIB64* pTIB = (NT_TIB64*)NtCurrentTeb(); // NtCurrentTeb is defined in <WinNT.h> as an inline call to __readgsqword
	return (void*)pTIB->StackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	NT_TIB64* pTIB = (NT_TIB64*)NtCurrentTeb(); // NtCurrentTeb is defined in <WinNT.h> as an inline call to __readgsqword
	return (void*)pTIB->StackLimit;

	// The following is an alternative implementation that returns the extent 
	// of the current stack usage as opposed to the stack limit as seen by the OS. 
	// This value will be a higher address than Tib.StackLimit (recall that the 
	// stack grows downward). It's debatable which of these two approaches is
	// better, as one returns the thread's -usable- stack space while the
	// other returns how much the thread is -currently- using. The determination
	// of the usable stack space is complicated by the fact that Microsoft 
	// platforms auto-extend the stack if the process pushes beyond the current limit.
	// In the end the Tib.StackLimit solution is actually the most portable across
	// Microsoft OSs and compilers for those OSs (Microsoft or not).

	// Alternative implementation:
	// We return our stack pointer, which is a good approximation of the stack limit of the caller.
	// void* rsp = GetRSP();
	// return rsp;
}


} // namespace Thread
} // namespace EA


#if defined(_MSC_VER) && (defined(_M_AMD64) || defined(_WIN64))
	#pragma optimize("", on) // See comments above regarding this optimization change.
#endif




