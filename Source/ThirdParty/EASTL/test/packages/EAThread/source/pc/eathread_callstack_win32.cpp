///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>

#if defined(EA_PLATFORM_WIN32) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // The following only works on Win32 and not Win64.

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <Windows.h>
#include <DbgHelp.h>
#include <stdio.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


#ifdef _MSC_VER
	#pragma warning(disable: 4740)      // flow in or out of inline asm code suppresses global optimization
	#pragma comment(lib, "dbghelp.lib")
	#pragma comment(lib, "psapi.lib")
#endif


typedef BOOL   (__stdcall *SYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL   (__stdcall *SYMCLEANUP)(HANDLE);
typedef BOOL   (__stdcall *STACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE,PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESS)(HANDLE, DWORD);
typedef DWORD  (__stdcall *SYMGETMODULEBASE)(HANDLE, DWORD);
typedef BOOL   (__stdcall *SYMGETSYMFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL   (__stdcall *SYMGETLINEFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);


namespace // We construct an anonymous namespace because doing so keeps the definitions within it local to this module.
{  
	struct Win32DbgHelp
	{
		HMODULE                mhDbgHelp;
		bool                   mbSymInitialized;
		SYMINITIALIZE          mpSymInitialize;
		SYMCLEANUP             mpSymCleanup;
		STACKWALK              mpStackWalk;
		SYMFUNCTIONTABLEACCESS mpSymFunctionTableAccess;
		SYMGETMODULEBASE       mpSymGetModuleBase;
		SYMGETSYMFROMADDR      mpSymGetSymFromAddr;
		SYMGETLINEFROMADDR     mpSymGetLineFromAddr;

		Win32DbgHelp() : mhDbgHelp(0), mbSymInitialized(false), mpSymInitialize(0), 
						 mpSymCleanup(0), mpStackWalk(0), mpSymFunctionTableAccess(0), 
						 mpSymGetModuleBase(0), mpSymGetSymFromAddr(0), mpSymGetLineFromAddr(0)
		{
			// Empty. The initialization is done externally, due to tricky startup/shutdown ordering issues.
		}

		~Win32DbgHelp()
		{
			// Empty. The shutdown is done externally, due to tricky startup/shutdown ordering issues.
		}

		void Init()
		{
			if(!mhDbgHelp)
			{
				mhDbgHelp = ::LoadLibraryA("DbgHelp.dll");
				if(mhDbgHelp)
				{
					mpSymInitialize          = (SYMINITIALIZE)(uintptr_t)         ::GetProcAddress(mhDbgHelp, "SymInitialize");
					mpSymCleanup             = (SYMCLEANUP)(uintptr_t)            ::GetProcAddress(mhDbgHelp, "SymCleanup");
					mpStackWalk              = (STACKWALK)(uintptr_t)             ::GetProcAddress(mhDbgHelp, "StackWalk");
					mpSymFunctionTableAccess = (SYMFUNCTIONTABLEACCESS)(uintptr_t)::GetProcAddress(mhDbgHelp, "SymFunctionTableAccess");
					mpSymGetModuleBase       = (SYMGETMODULEBASE)(uintptr_t)      ::GetProcAddress(mhDbgHelp, "SymGetModuleBase");
					mpSymGetSymFromAddr      = (SYMGETSYMFROMADDR)(uintptr_t)     ::GetProcAddress(mhDbgHelp, "SymGetSymFromAddr");
					mpSymGetLineFromAddr     = (SYMGETLINEFROMADDR)(uintptr_t)    ::GetProcAddress(mhDbgHelp, "SymGetLineFromAddr");
				}
			}
		}

		void Shutdown()
		{
			if(mhDbgHelp)
			{
				if(mbSymInitialized && mpSymCleanup)
					mpSymCleanup(::GetCurrentProcess());
				::FreeLibrary(mhDbgHelp);
			}
		}
	};

	static int          sInitCount = 0;
	static Win32DbgHelp sWin32DbgHelp;
}





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


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EATHREADLIB_API void InitCallstack()
{
	if(++sInitCount == 1)
		sWin32DbgHelp.Init();
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EATHREADLIB_API void ShutdownCallstack()
{
	if(--sInitCount == 0)
		sWin32DbgHelp.Shutdown();
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	size_t nEntryIndex(0);

	if(!sWin32DbgHelp.mhDbgHelp)
		sWin32DbgHelp.Init();

	if(sWin32DbgHelp.mpStackWalk)
	{
		CONTEXT context;
		memset(&context, 0, sizeof(context));
		context.ContextFlags = CONTEXT_CONTROL;

		if(pContext)
		{
			context.Eip = pContext->mEIP;
			context.Esp = pContext->mESP;
			context.Ebp = pContext->mEBP;
		}
		else
		{
		// RtlCaptureStackBackTrace can only generate stack traces on Win32 when the stack frame contains frame
		// pointers.  This only a limitation on 32-bit Windows and is controlled by the following compilers switches.
		//
		// /Oy  : removes frame-pointers
		// /Oy- : emits frame-pointers
		// 
		// The language is wierd here because Microsoft refers it as enabling/disabling an performance optimization.
		// https://docs.microsoft.com/en-us/cpp/build/reference/oy-frame-pointer-omission?view=vs-2017
		// 
		// EATHREAD_WIN32_FRAME_POINTER_OPTIMIZATION_DISABLED is enabled/disabled based on if the user has requested eaconfig to disable
		// frame-pointer optimizations (enable frame-pointers).  See property: 'eaconfig.disable_framepointer_optimization'.
		//
		#ifdef EATHREAD_WIN32_FRAME_POINTER_OPTIMIZATION_DISABLED
			return RtlCaptureStackBackTrace(1, (ULONG)nReturnAddressArrayCapacity, pReturnAddressArray, NULL);
		#else
			// With VC++, EIP is not accessible directly, but we can use an assembly trick to get it.
			// VC++ and Intel C++ compile this fine, but Metrowerks 7 has a bug and fails.
			__asm{
				mov context.Ebp, EBP
					mov context.Esp, ESP
					call GetEIP
					GetEIP:
					pop context.Eip
			}
		#endif
		}

		// Initialize the STACKFRAME structure for the first call. This is only
		// necessary for Intel CPUs, and isn't mentioned in the documentation.
		STACKFRAME sf;
		memset(&sf, 0, sizeof(sf));
		sf.AddrPC.Offset     = context.Eip;
		sf.AddrPC.Mode       = AddrModeFlat;
		sf.AddrStack.Offset  = context.Esp;
		sf.AddrStack.Mode    = AddrModeFlat;
		sf.AddrFrame.Offset  = context.Ebp;
		sf.AddrFrame.Mode    = AddrModeFlat;

		const HANDLE hCurrentProcess = ::GetCurrentProcess();
		const HANDLE hCurrentThread  = ::GetCurrentThread();

		// To consider: We have had some other code which can read the stack with better success
		// than the DbgHelp stack walk function that we use here. In particular, the DbgHelp 
		// stack walking function doesn't do well unless x86 stack frames are used.
		for(int nStackIndex = 0; nEntryIndex < (nReturnAddressArrayCapacity - 1); ++nStackIndex)
		{
			if(!sWin32DbgHelp.mpStackWalk(IMAGE_FILE_MACHINE_I386, hCurrentProcess, hCurrentThread, 
											&sf, &context, NULL, sWin32DbgHelp.mpSymFunctionTableAccess, 
											sWin32DbgHelp.mpSymGetModuleBase, NULL))
			{
				break;
			}

			if(sf.AddrFrame.Offset == 0)  // Basic sanity check to make sure the frame is OK. Bail if not.
				break;

			// If using the current execution context, then we ignore the first 
			// one because it is the one that is our stack walk function itself.
			if(pContext || (nStackIndex > 0)) 
				pReturnAddressArray[nEntryIndex++] = ((void*)(uintptr_t)sf.AddrPC.Offset);
		}
	}

	pReturnAddressArray[nEntryIndex] = 0;
	return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
	#if defined(EA_PLATFORM_WIN32)
		EAT_COMPILETIME_ASSERT(offsetof(EA::Thread::Context, Eip) == offsetof(CONTEXT, Eip));
		EAT_COMPILETIME_ASSERT(offsetof(EA::Thread::Context, SegSs) == offsetof(CONTEXT, SegSs));
	#endif

	context.mEIP = pContext->Eip;
	context.mESP = pContext->Esp;
	context.mEBP = pContext->Ebp;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* address, char* pModuleName, size_t moduleNameCapacity)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery(address, &mbi, sizeof(mbi)))
	{
		HMODULE hModule = (HMODULE)mbi.AllocationBase;

		if(hModule)
			return GetModuleFileNameA(hModule, pModuleName, (DWORD)moduleNameCapacity);
	}

	pModuleName[0] = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery(pAddress, &mbi, sizeof(mbi)))
		return (ModuleHandle)mbi.AllocationBase;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetThreadIdFromThreadHandle
//
// This implementation is the same as the one in EAThread.
//
EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId)
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
			return pGetThreadIdFunc((HANDLE)threadId);
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

			if(pNtQueryInformationThread((HANDLE)threadId, 0, &tbi, sizeof(tbi), NULL) == 0)
				return tbi.UniqueThreadId;
		}
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// The threadId is the same thing as the Windows' HANDLE GetCurrentThread() function
// and not the same thing as Windows' GetCurrentThreadId function. See the 
// GetCallstackContextSysThreadId for the latter.
// 
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	if((threadId == (intptr_t)kThreadIdInvalid) || (threadId == (intptr_t)kThreadIdCurrent))
		threadId = (intptr_t)::GetCurrentThread(); // GetCurrentThread returns a thread 'pseudohandle' and not a real thread handle.

	const DWORD sysThreadId        = EA::Thread::GetThreadIdFromThreadHandle(threadId);
	const DWORD sysThreadIdCurrent = ::GetCurrentThreadId();
	CONTEXT     win32CONTEXT;
	NT_TIB*     pTib;

	if(sysThreadIdCurrent == sysThreadId)
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

		// Offset 0x18 from the FS segment register gives a pointer to
		// the thread information block for the current thread
		__asm {
			mov eax, fs:[18h]
			mov pTib, eax
		}
	}
	else
	{
		// In this case we are working with a separate thread, so we suspend it
		// and read information about it and then resume it.
		::SuspendThread((HANDLE)threadId);
		win32CONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;
		::GetThreadContext((HANDLE)threadId, &win32CONTEXT);
		// TODO: This has not been tested!
		pTib = *((NT_TIB**)(win32CONTEXT.SegFs * 16 + 18));
		::ResumeThread((HANDLE)threadId);
	}

	context.mEBP = (uint32_t)win32CONTEXT.Ebp;
	context.mESP = (uint32_t)win32CONTEXT.Esp;
	context.mEIP = (uint32_t)win32CONTEXT.Eip;
	context.mStackBase    = (uintptr_t)pTib->StackBase;
	context.mStackLimit   = (uintptr_t)pTib->StackLimit;
	context.mStackPointer = (uintptr_t)win32CONTEXT.Esp;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
// A sysThreadId is a Microsoft DWORD thread id, which can be obtained from 
// the currently running thread via GetCurrentThreadId. It can be obtained from
// a Microsoft thread HANDLE via EA::Thread::GetThreadIdFromThreadHandle();
// A DWORD thread id can be converted to a thread HANDLE via the Microsoft OpenThread
// system function.
//
EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	bool        bReturnValue       = true;
	const DWORD sysThreadIdCurrent = ::GetCurrentThreadId();
	CONTEXT     win32CONTEXT;

	if(sysThreadIdCurrent == (DWORD)sysThreadId)
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
		// and read information about it and then resume it.
		HANDLE threadId = ::OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, TRUE, (DWORD)sysThreadId);

		if(threadId)
		{
			::SuspendThread(threadId);
			win32CONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
			::GetThreadContext(threadId, &win32CONTEXT);
			::ResumeThread(threadId);

			::CloseHandle(threadId);
		}
		else
		{
			memset(&win32CONTEXT, 0, sizeof(win32CONTEXT));
			bReturnValue = false;
		}
	}

	context.mEBP = (uint32_t)win32CONTEXT.Ebp;
	context.mESP = (uint32_t)win32CONTEXT.Esp;
	context.mEIP = (uint32_t)win32CONTEXT.Eip;
  //context.mStackBase    = (uintptr_t)pTib->StackBase;     // To do. (Whoever added mStackBase to CallstackContext forgot to add this code)
  //context.mStackLimit   = (uintptr_t)pTib->StackLimit;
  //context.mStackPointer = (uintptr_t)win32CONTEXT.Esp;

	return bReturnValue;
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
	CallstackContext context;

	GetCallstackContext(context, 0);
	return (void*)context.mStackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	CallstackContext context;

	GetCallstackContext(context, 0);
	return (void*)context.mStackLimit;

	// Alternative which returns a slightly different answer:
	// We return our stack pointer, which is a good approximation of the stack limit of the caller.
	// void* pStack = NULL;
	// __asm { mov pStack, ESP};
	// return pStack;
}


} // namespace Thread
} // namespace EA

#else // Stub out function for WinRT / Windows Phone 8

namespace EA
{
namespace Thread
{

EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	EA_UNUSED(pContext);
	EA_UNUSED(pReturnAddressArray);
	EA_UNUSED(nReturnAddressArrayCapacity);

	return 0;
}

} // namespace Thread
} // namespace EA

#endif // defined(EA_PLATFORM_WIN32)

