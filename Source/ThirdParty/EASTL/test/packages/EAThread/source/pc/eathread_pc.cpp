///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include "eathread/eathread.h"
#include "eathread/eathread_thread.h"
#include "eathread/eathread_storage.h"

#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	#include <process.h>

	EA_DISABLE_ALL_VC_WARNINGS()
	#include <Windows.h>
	#include <stdlib.h> // for mbstowcs
	#include <setjmp.h>
	EA_RESTORE_ALL_VC_WARNINGS()

	#include "eathread/eathread_futex.h"

	extern "C" WINBASEAPI DWORD WINAPI SetThreadIdealProcessor(_In_ HANDLE hThread, _In_ DWORD dwIdealProcessor);
	#if defined(EA_PLATFORM_WIN64)
		extern "C" WINBASEAPI DWORD WINAPI GetThreadId(_In_ HANDLE hThread);
		extern "C" WINBASEAPI ULONGLONG GetTickCount64(VOID); // Will not run on pre-Vista OS so 64 bit XP not supported
	#endif

	// We set this module to initialize early. We want to do this because it 
	// allows statically initialized objects to call these functions safely.
	EA_DISABLE_VC_WARNING(4074)  // warning C4074: initializers put in compiler reserved initialization area
	#pragma init_seg(compiler)
	EA_RESTORE_VC_WARNING()
	#ifndef EATHREAD_INIT_SEG_DEFINED 
		#define EATHREAD_INIT_SEG_DEFINED
	#endif 

	namespace EA
	{
		namespace Thread
		{
			// Note by Paul Pedriana:
			// There is a bit of code here which implements "dynamic thread array maintenance". 
			// The reason for this is that we are trying to present to the user a consistently
			// behaving GetThreadId function. The Windows threading API has a number of design
			// characteristics that make it less than ideal for applications. One of these 
			// designs is that an application cannot ask the system what its thread id is and 
			// get a consistent answer; in fact you always get a different answer. 

			// To consider: Use the VC++ undocumented __tlregdtor function to detect thread exits.
			// __tlregdtor is a VC++ CRT function which detects the exiting of any threads created 
			// with the CRT beginthread family of functions. It cannot detect the exit of any threads 
			// that are begun via direct OS thread creation functions, nor can it detect the exit of 
			// threads that are exited by direct OS thread exit functions. This is may not be a major 
			// problem, because C/C++ programs should virtually always be calling the CRT thread begin 
			// and end functions so that the CRT can be maintained properly for the thread.
			//
			//    typedef void (*_PVFV)();
			//    void __tlregdtor(_PVFV func);
			//    void ThreadExit(){ Do something. May need to be careful about what APIs are called. }

			// Assertion variables.
			EA::Thread::AssertionFailureFunction gpAssertionFailureFunction = NULL;
			void*                                gpAssertionFailureContext  = NULL;

			// Dynamic thread array maintenance.
			// If the user calls GetThreadId from a thread that was created by some third 
			// party, then we don't have a thread handle for it. The only current way to get 
			// such a thread handle is to call OpenThread(GetCurrentThreadId()) or 
			// DuplicateHandle(GetCurrentThread()). In either case the return value is a 
			// handle which must be disposed of via CloseHandle. Additionally, since the 
			// thread was created by a thrid party, it's entirely possible that the thread 
			// will be exited without us ever finding about it. But we still need to call 
			// CloseHandle on the handle. So we maintain an array of handles and check their
			// status periodically and upon process exit.

			const size_t kMaxThreadDynamicArrayCount = 128;

			struct DynamicThreadArray
			{
				static HANDLE            mhDynamicThreadArray[kMaxThreadDynamicArrayCount];
				static CRITICAL_SECTION  mCriticalSection;
				static bool              mbDynamicThreadArrayInitialized;

				static void Initialize();
				static void CheckDynamicThreadArray(bool bCloseAll);
				static void AddDynamicThreadHandle(HANDLE hThread, bool bAdd);
			};

			HANDLE DynamicThreadArray::mhDynamicThreadArray[kMaxThreadDynamicArrayCount];
			CRITICAL_SECTION DynamicThreadArray::mCriticalSection;
			bool DynamicThreadArray::mbDynamicThreadArrayInitialized;

			// DynamicThreadArray ctor/dtor were removed to because memory tracking systems that are required to run 
			// pre-main and post-main.  In order to support memory tracking of allocations that occur post-main we
			// intentially "leak" a operating system critical section and leave it to be cleaned up by the operating
			// system at process shutdown.
			//
			// DynamicThreadArray::DynamicThreadArray()
			// {
			//     Initialize();
			// }

			// DynamicThreadArray::~DynamicThreadArray()
			// {
			//     CheckDynamicThreadArray(true);
			//     DeleteCriticalSection(&mCriticalSection);
			// }

			void DynamicThreadArray::Initialize()
			{
				static EA::Thread::Futex m;

				const bool done = mbDynamicThreadArrayInitialized;

				// ensure that if we've seen previous writes to mbDynamicThreadArrayInitialized, we also
				// see the writes to mCriticalSection, to avoid the case where another thread sees the flag
				// before it sees the initialization
				EAReadBarrier();

				if(!done)
				{
					EA::Thread::AutoFutex _(m);

					if (!mbDynamicThreadArrayInitialized)
					{
						memset(mhDynamicThreadArray, 0, sizeof(mhDynamicThreadArray));
						InitializeCriticalSection(&mCriticalSection);

						// ensure writes to mCriticalSection and mhDynamicThreadArray are visible before writes
						// to mbDynamicThreadArrayInitialized, to avoid the case where another thread sees the
						// flag before it sees the initialization
						EAWriteBarrier();

						mbDynamicThreadArrayInitialized = true;
					}
				}
			}

			// This function looks at the existing set of thread ids and see if any of them 
			// were quit. If so then this function removes their entry from our array of 
			// thread handles, and most importantly, calls CloseHandle on the thread handle.
			void DynamicThreadArray::CheckDynamicThreadArray(bool bCloseAll)
			{
				Initialize();

				EnterCriticalSection(&mCriticalSection);

				for(size_t i(0); i < sizeof(mhDynamicThreadArray)/sizeof(mhDynamicThreadArray[0]); i++)
				{
					if(mhDynamicThreadArray[i])
					{
						DWORD dwExitCode(0);

						// Note that GetExitCodeThread is a hazard if the user of a thread exits 
						// with a return value that is equal to the value of STILL_ACTIVE (i.e. 259).
						// We can document that users shouldn't do this, or we can change the code 
						// here to use WaitForSingleObject(hThread, 0) and assume the thread is 
						// still active if the return value is WAIT_TIMEOUT.
						if(bCloseAll || !GetExitCodeThread(mhDynamicThreadArray[i], &dwExitCode) || (dwExitCode != STILL_ACTIVE)) // If the thread id is invalid or it has exited...
						{
							CloseHandle(mhDynamicThreadArray[i]); // This matches the DuplicateHandle call we made below.
							mhDynamicThreadArray[i] = 0;
						}
					}
				}

				LeaveCriticalSection(&mCriticalSection);
			}

			void DynamicThreadArray::AddDynamicThreadHandle(HANDLE hThread, bool bAdd)
			{
				Initialize();

				if(hThread)
				{
					EnterCriticalSection(&mCriticalSection);

					if(bAdd)
					{
						for(size_t i(0); i < sizeof(mhDynamicThreadArray)/sizeof(mhDynamicThreadArray[0]); i++)
						{
							if(mhDynamicThreadArray[i] == kThreadIdInvalid)
							{
								mhDynamicThreadArray[i] = hThread;
								hThread = kThreadIdInvalid;         // This tells us that we succeeded, and we'll use this result below.
								break;
							}
						}

						EAT_ASSERT(hThread == kThreadIdInvalid);    // Assert that there was enough room (that the above loop found a spot).
						if(hThread != kThreadIdInvalid)             // If not, then we need to free the handle.
							CloseHandle(hThread);                   // This matches the DuplicateHandle call we made below.
					}
					else
					{
						for(size_t i(0); i < sizeof(mhDynamicThreadArray)/sizeof(mhDynamicThreadArray[0]); i++)
						{
							if(mhDynamicThreadArray[i] == hThread)
							{
								CloseHandle(hThread);           // This matches the DuplicateHandle call we made below.
								mhDynamicThreadArray[i] = kThreadIdInvalid;
								break;
							}
						}
						// By design, we don't consider a non-found handle an error. It may simply be the 
						// case that the given handle was not a dynamnic thread handle. Due to the way 
						// Windows works, there's just no way for us to tell.
					}

					LeaveCriticalSection(&mCriticalSection);
				}
			}

			// Thread handle local storage.
			// We have this code here in order to cache the thread handles for 
			// threads, so that the user gets a consistent return value from the 
			// GetThreadId function for each unique thread.

			static DWORD dwThreadHandleTLS = TLS_OUT_OF_INDEXES; // We intentionally make this an independent variable so that it is initialized unilaterally on segment load.

			struct TLSAlloc
			{
				TLSAlloc()
				{
					if(dwThreadHandleTLS == TLS_OUT_OF_INDEXES) // It turns out that the user might have set this to a 
						dwThreadHandleTLS = TlsAlloc();         // value before this constructor has run. So we check.
				}

				#if EATHREAD_TLSALLOC_DTOR_ENABLED
				// Since this class is used only as a static variable, this destructor would
				// only get called during module destruction: app quit or DLL unload. 
				// In the case of DLL unload, we may have a problem if the DLL was unloaded 
				// before threads created by it were destroyed. Whether the problem is significant
				// depends on the application. In most cases it won't be significant.
				//
				// We want to call TlsFree because not doing so results in a memory leak and eventual
				// exhaustion of TLS ids by the system.
				~TLSAlloc()
				{
					if(dwThreadHandleTLS != TLS_OUT_OF_INDEXES)
					{
						// We don't read the hThread stored at dwThreadHandleTLS and call CloseHandle
						// on it, as the DynamicThreadArray destructor will deal with closing any
						// thread handles this module knows about. 

						TlsFree(dwThreadHandleTLS);
						dwThreadHandleTLS = TLS_OUT_OF_INDEXES;
					}
				}
				#endif
			};
			static TLSAlloc sTLSAlloc;

			void SetCurrentThreadHandle(HANDLE hThread, bool bDynamic)
			{
				// EAT_ASSERT(hThread != kThreadIdInvalid); We can't do this, as we can be intentionally called with an hThread of kThreadIdInvalid.
				if(dwThreadHandleTLS == TLS_OUT_OF_INDEXES) // This should generally always evaluate to true because we init dwThreadHandleTLS on startup.
					dwThreadHandleTLS = TlsAlloc();
				EAT_ASSERT(dwThreadHandleTLS != TLS_OUT_OF_INDEXES);
				if(dwThreadHandleTLS != TLS_OUT_OF_INDEXES)
				{
					DynamicThreadArray::CheckDynamicThreadArray(false);
					if(bDynamic)
					{
						if(hThread != kThreadIdInvalid) // If adding the hThread...
							DynamicThreadArray::AddDynamicThreadHandle(hThread, true);
						else // Else removing the existing current thread handle...
						{
							HANDLE hThreadOld = TlsGetValue(dwThreadHandleTLS);
							if(hThreadOld != kThreadIdInvalid) // This should always evaluate to true in practice.
								DynamicThreadArray::AddDynamicThreadHandle(hThreadOld, false); // Will Close the dynamic thread handle if it is one.
						}
					}
					TlsSetValue(dwThreadHandleTLS, hThread);
				}
			}
		} // namespace Thread
	} // namespace EA


EATHREADLIB_API EA::Thread::ThreadId EA::Thread::GetThreadId()
{
	// We have some non-trivial code here because Windows doesn't provide a means for a
	// thread to read its own thread id (thread handle) in a consistent way. 

	// If we have allocated thread-local storage for this module...
	if(dwThreadHandleTLS != TLS_OUT_OF_INDEXES)
	{
		void* const pValue = TlsGetValue(dwThreadHandleTLS);

		if(pValue)          // If the current thread's ThreadId has been previously saved...
			return pValue;  // Under Win32, type ThreadId should be the same as HANDLE which should be the same as void*.

		// Else fall through and get the current thread handle and cache it so that next time the above code will succeed.
	}

	// In this case the thread was not created by EAThread. So we give 
	// the thread a new Id, based on GetCurrentThread and DuplicateHandle.
	// GetCurrentThread returns a "pseudo handle" which isn't actually the 
	// thread handle but is a hard-coded constant which means "current thread".
	// If you want to get a real thread handle then you need to call DuplicateHandle
	// on the pseudo handle. Every time you call DuplicateHandle you get a different
	// result, yet we want this GetThreadId function to return a consistent value
	// to the user, as that's what a rational user would expect. So after calling
	// DuplicateHandle we save the value for the next time the user calls this 
	// function. We save the value in thread-local storage, so each unique thread
	// sees a unique view of GetThreadId.
	HANDLE hThread, hThreadPseudo = GetCurrentThread();
	BOOL bResult = DuplicateHandle(GetCurrentProcess(), hThreadPseudo, GetCurrentProcess(), &hThread, 0, true, DUPLICATE_SAME_ACCESS);
	EAT_ASSERT(bResult && (hThread != kThreadIdInvalid));
	if(bResult)
		EA::Thread::SetCurrentThreadHandle(hThread, true); // Need to eventually call CloseHandle on hThread, so we store it.

	return hThread;        
}

EATHREADLIB_API EA::Thread::ThreadId EA::Thread::GetThreadId(EA::Thread::SysThreadId id)
{
	EAThreadDynamicData* const pTDD = EA::Thread::FindThreadDynamicData(id);
	if(pTDD)
	{   
		return pTDD->mhThread;
	}

	return EA::Thread::kThreadIdInvalid;
}

EATHREADLIB_API EA::Thread::SysThreadId EA::Thread::GetSysThreadId(ThreadId id)
{
	#if defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64)
		// Win64 has this function natively.
		return ::GetThreadId(id);

		// Fast implementation of this, which has been verified:
		// uintptr_t pTIB = __readgsqword(0x30);
		// uint32_t threadId = *((uint32_t*)(((uint8_t*)pTIB) + 0x48));
		// return (EA::Thread::SysThreadId)threadId;

	#elif defined(EA_PLATFORM_WIN32)

		// What we do here is first try to use the GetThreadId function, which is 
		// available on some later versions of WinXP and later OSs. If that doesn't
		// work then we are using an earlier OS and we use the NtQueryInformationThread
		// kernel function to read thread info.

		typedef DWORD (WINAPI *GetThreadIdFunc)(HANDLE);
		typedef BOOL (WINAPI *NtQueryInformationThreadFunc)(HANDLE, int, PVOID, ULONG, PULONG);

		// We implement our own manual version of static variables here. We do this because 
		// the static variable mechanism the compiler provides wouldn't provide thread 
		// safety for us. 
		static volatile bool                sInitialized = false;
		static GetThreadIdFunc              spGetThreadIdFunc = NULL;
		static NtQueryInformationThreadFunc spNtQueryInformationThread = NULL;

		if(!sInitialized)
		{
			HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
			if(hKernel32)
				spGetThreadIdFunc = (GetThreadIdFunc)(uintptr_t)GetProcAddress(hKernel32, "GetThreadId");

			if(!spGetThreadIdFunc)
			{
				HMODULE hNTDLL = GetModuleHandleA("ntdll.dll");

				if(hNTDLL)
					spNtQueryInformationThread = (NtQueryInformationThreadFunc)(uintptr_t)GetProcAddress(hNTDLL, "NtQueryInformationThread");
			}

			sInitialized = true;
		}

		if(spGetThreadIdFunc)
			return (SysThreadId)spGetThreadIdFunc(id);

		if(spNtQueryInformationThread)
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

			THREAD_BASIC_INFORMATION_WIN32 tbi;

			if(spNtQueryInformationThread(id, 0, &tbi, sizeof(tbi), NULL) == 0)
			   return (SysThreadId)tbi.UniqueThreadId;
		}

		return kSysThreadIdInvalid;

	#endif
}


EATHREADLIB_API EA::Thread::SysThreadId EA::Thread::GetSysThreadId()
{
	return ::GetCurrentThreadId();
}


EATHREADLIB_API int EA::Thread::GetThreadPriority()
{
	const int nPriority = ::GetThreadPriority(GetCurrentThread());
	return kThreadPriorityDefault + (nPriority - THREAD_PRIORITY_NORMAL);
}


EATHREADLIB_API bool EA::Thread::SetThreadPriority(int nPriority)
{
	EAT_ASSERT(nPriority != kThreadPriorityUnknown);
	int nNewPriority = THREAD_PRIORITY_NORMAL + (nPriority - kThreadPriorityDefault);
	bool result = ::SetThreadPriority(GetCurrentThread(), nNewPriority) != 0;

	// Windows process running in NORMAL_PRIORITY_CLASS is picky about the priority passed in.
	// So we need to set the priority to the next priority supported
	#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE)
		HANDLE thread = GetCurrentThread();

		while(!result)
		{
			if (nNewPriority >= THREAD_PRIORITY_TIME_CRITICAL) 
				return ::SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL) != 0;

			if (nNewPriority <= THREAD_PRIORITY_IDLE) 
				return ::SetThreadPriority(thread, THREAD_PRIORITY_IDLE) != 0;

			result = ::SetThreadPriority(thread, nNewPriority) != 0;
			nNewPriority++;
		}
	#endif

	return result;
}


EATHREADLIB_API void EA::Thread::SetThreadProcessor(int nProcessor)
{
	#if   defined(EA_PLATFORM_XBOXONE)

		DWORD mask = 0xFF; //Default to all
		if (nProcessor >= 0)
			mask = (DWORD)(1 << nProcessor);

		SetThreadAffinityMask(GetCurrentThread(), mask);

	#else
		static const int nProcessorCount = GetProcessorCount();

		if(nProcessor < 0)
			nProcessor = MAXIMUM_PROCESSORS; // This cases the SetThreadIdealProcessor to reset to 'no ideal processor'.
		else
		{
			if(nProcessor >= nProcessorCount)
				nProcessor %= nProcessorCount; 
		}

		// SetThreadIdealProcessor differs from SetThreadAffinityMask in that SetThreadIdealProcessor is not 
		// a strict assignment, and it allows the OS to move the thread if the ideal processor is busy. 
		// SetThreadAffinityMask is a more rigid assignment, but it can result in slower performance and 
		// possibly hangs due to processor contention between threads. For Windows we use SetIdealThreadProcessor
		// in the name of safety and likely better overall performance.
		SetThreadIdealProcessor(GetCurrentThread(), (DWORD)nProcessor);

	#endif
}


void* EA::Thread::GetThreadStackBase()
{
	#if   defined(EA_PLATFORM_WIN32) && defined(EA_PROCESSOR_X86) && defined(EA_COMPILER_MSVC)
		// Offset 0x18 from the FS segment register gives a pointer to
		// the thread information block for the current thread
		// VC++ also offers the __readfsdword() intrinsic, which would be better to use here.
		NT_TIB* pTib;

		__asm {
			mov eax, fs:[18h]
			mov pTib, eax
		}

		return (void*)pTib->StackBase;

	#elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64) && defined(EA_COMPILER_MSVC)
		// VC++ also offers the __readgsdword() intrinsic, which is an alternative which could
		// retrieve the current thread TEB if the following proves unreliable.
		PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());

		return (void*)pTib->StackBase;

	#elif defined(EA_PLATFORM_WIN32) && defined(EA_PROCESSOR_X86) && defined(EA_COMPILER_GCC)
		NT_TIB* pTib;

		asm ( "movl %%fs:0x18, %0\n"
			  : "=r" (pTib)
			);

		return (void*)pTib->StackBase;
	#endif
}


#if defined(EA_PLATFORM_WIN32) && defined(EA_PROCESSOR_X86) && defined(_MSC_VER) && (_MSC_VER >= 1400)
	// People report on the Internet that this function can get you what CPU the current thread
	// is running on. But that's false, as this function has been seen to return values greater than
	// the number of physical or real CPUs present. For example, this function returns 6 for my 
	// Single CPU that's dual-hyperthreaded.
	static int GetCurrentProcessorNumberCPUID()
	{
		_asm { mov eax, 1   }
		_asm { cpuid        }
		_asm { shr ebx, 24  }
		_asm { mov eax, ebx }
	}

	int GetCurrentProcessorNumberXP()
	{
		int cpuNumber = GetCurrentProcessorNumberCPUID();
		int cpuCount  = EA::Thread::GetProcessorCount();

		return (cpuNumber % cpuCount); // I don't know if this is the right thing to do, but it's better than returning an impossible number and Windows XP is a fading OS as it is.
	}

#endif


EATHREADLIB_API int EA::Thread::GetThreadProcessor()
{
	#if defined(EA_PLATFORM_WIN32)
		// Only Windows Vista and later provides GetCurrentProcessorNumber.
		// So we must dynamically link to this function.
		static EA_THREAD_LOCAL bool           bInitialized = false;
		static EA_THREAD_LOCAL DWORD (WINAPI *pfnGetCurrentProcessorNumber)() = NULL;

		if(!bInitialized)
		{
			HMODULE hKernel32 = GetModuleHandleA("KERNEL32.DLL");
			if(hKernel32)
				pfnGetCurrentProcessorNumber = (DWORD (WINAPI*)())(uintptr_t)GetProcAddress(hKernel32, "GetCurrentProcessorNumber");
			bInitialized = true;
		}

		if(pfnGetCurrentProcessorNumber)
			return (int)(unsigned)pfnGetCurrentProcessorNumber();

		#if defined(EA_PLATFORM_WINDOWS) && defined(EA_PROCESSOR_X86) && defined(_MSC_VER) && (_MSC_VER >= 1400)
			return GetCurrentProcessorNumberXP();
		#else
			return 0;
		#endif

	#elif defined(EA_PLATFORM_WIN64)
		static EA_THREAD_LOCAL bool           bInitialized = false;
		static EA_THREAD_LOCAL DWORD (WINAPI *pfnGetCurrentProcessorNumber)() = NULL;

		if(!bInitialized)
		{
			HMODULE hKernel32 = GetModuleHandleA("KERNEL32.DLL"); // Yes, we want to use Kernel32.dll. There is no Kernel64.dll on Win64. 
			if(hKernel32)
				pfnGetCurrentProcessorNumber = (DWORD (WINAPI*)())(uintptr_t)GetProcAddress(hKernel32, "GetCurrentProcessorNumber");
			bInitialized = true;
		}

		if(pfnGetCurrentProcessorNumber)
			return (int)(unsigned)pfnGetCurrentProcessorNumber();

		return 0;

	#else
		return (int)(unsigned)GetCurrentProcessorNumber();

	#endif
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
	#if defined(EA_PLATFORM_XBOXONE)
		DWORD_PTR nProcessorCountMask = 0x7F;  // default to all 7 available cores.
	#else
		DWORD_PTR nProcessorCountMask = (DWORD_PTR)1 << GetProcessorCount(); 
	#endif

	// Call the Windows library function.
	DWORD_PTR nProcessAffinityMask, nSystemAffinityMask;
	if(EA_LIKELY(GetProcessAffinityMask(GetCurrentProcess(), &nProcessAffinityMask, &nSystemAffinityMask)))
		nProcessorCountMask = nProcessAffinityMask;
	
	nAffinityMask &= nProcessorCountMask;

	auto opResult = ::SetThreadAffinityMask(id, static_cast<DWORD_PTR>(nAffinityMask));
	EA_UNUSED(opResult);
	EAT_ASSERT_FORMATTED(opResult != 0, "The Windows platform SetThreadAffinityMask failed. GetLastError %x", GetLastError());
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


// Internal SetThreadName API's so we don't repeat the implementations
namespace EA {
namespace Thread {
namespace Internal {
	bool PixSetThreadName(EA::Thread::ThreadId threadId, const char* pName)
	{
		EA_UNUSED(threadId); EA_UNUSED(pName);

		bool result = true;

	#if (defined(EA_PLATFORM_XBOXONE) && EA_CAPILANO_DBG_ENABLED == 1)
		wchar_t wName[EATHREAD_NAME_SIZE];
		mbstowcs(wName, pName, EATHREAD_NAME_SIZE);
		result = (::SetThreadName(threadId, wName) == TRUE); // requires toolhelpx.lib
		EAT_ASSERT(result);
	#endif

		return result; 
	}

	bool WinSetThreadName(EA::Thread::ThreadId threadId, const char* pName)
	{
		bool result = true;

		typedef HRESULT(WINAPI *SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

		// Check if Windows Operating System has the 'SetThreadDescription" API.
		auto pSetThreadDescription = (SetThreadDescription)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");
		if (pSetThreadDescription)
		{
			wchar_t wName[EATHREAD_NAME_SIZE];
			mbstowcs(wName, pName, EATHREAD_NAME_SIZE);

			result = SUCCEEDED(pSetThreadDescription(threadId, wName));
			EAT_ASSERT(result);
		}

		return result;
	}
	
	void WinSetThreadNameByException(EA::Thread::SysThreadId threadId, const char* pName)
	{
		struct ThreadNameInfo
		{
			DWORD dwType;
			LPCSTR lpName;
			DWORD dwThreadId;
			DWORD dwFlags;
		};

		// This setjmp/longjmp weirdness is here to work around an apparent bug in the VS2013 debugger,
		// whereby EBX will be trashed on return from RaiseException, causing bad things to happen in code
		// which runs later. This only seems to happen when a debugger is attached and there's some managed
		// code in the process.

		jmp_buf jmpbuf;

		__pragma(warning(push))
		__pragma(warning(disable : 4611))
		if (!setjmp(jmpbuf))
		{
			ThreadNameInfo threadNameInfo = {0x1000, pName, threadId, 0};
			__try { RaiseException(0x406D1388, 0, sizeof(threadNameInfo) / sizeof(ULONG_PTR), (CONST ULONG_PTR*)(uintptr_t)&threadNameInfo); }
			__except (GetExceptionCode() == 0x406D1388 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { }
			longjmp(jmpbuf, 1);
		}
		__pragma(warning(pop))
	}

	void SetThreadName(EAThreadDynamicData* pTDD, const char* pName)
	{
		strncpy(pTDD->mName, pName, EATHREAD_NAME_SIZE);
		pTDD->mName[EATHREAD_NAME_SIZE - 1] = 0;

	#if defined(EA_PLATFORM_WINDOWS) && defined(_MSC_VER) || (defined(EA_PLATFORM_XBOXONE))
		if(pTDD->mName[0] && (pTDD->mhThread != EA::Thread::kThreadIdInvalid))
		{
			#if EATHREAD_NAMING == EATHREAD_NAMING_DISABLED
				bool namingEnabled = false;
			#elif EATHREAD_NAMING == EATHREAD_NAMING_ENABLED
				bool namingEnabled = true;
			#else
				bool namingEnabled = IsDebuggerPresent();
			#endif

			if(namingEnabled)
			{
				PixSetThreadName(pTDD->mhThread, pTDD->mName);  
				WinSetThreadName(pTDD->mhThread, pTDD->mName);  
				WinSetThreadNameByException(pTDD->mnThreadId, pTDD->mName);  
			}
		}
	#endif
	}
} // namespace Internal
} // namespace Thread
} // namespace EA

EATHREADLIB_API void EA::Thread::SetThreadName(const char* pName) { SetThreadName(GetThreadId(), pName); }
EATHREADLIB_API const char* EA::Thread::GetThreadName() { return GetThreadName(GetThreadId()); }

EATHREADLIB_API void EA::Thread::SetThreadName(const EA::Thread::ThreadId& id, const char* pName)
{
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	if(pTDD)
	{
		Internal::SetThreadName(pTDD, pName);
	}
}

EATHREADLIB_API const char* EA::Thread::GetThreadName(const EA::Thread::ThreadId& id)
{ 
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(id);
	return pTDD ?  pTDD->mName : "";
}

EATHREADLIB_API int EA::Thread::GetProcessorCount()
{
	#if defined(EA_PLATFORM_XBOXONE)
		// Capilano has 7-ish physical CPUs available to titles.  We can access 50 - 90% of the 7th Core.  
		// Check platform documentation for details.
	    DWORD_PTR ProcessAffinityMask;
		DWORD_PTR SystemAffinityMask;
		unsigned long nCoreCount = 6;

		if(EA_LIKELY(GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinityMask, &SystemAffinityMask)))
		{
			_BitScanForward(&nCoreCount, (unsigned long)~ProcessAffinityMask);
		}

		return (int) nCoreCount; 

	#elif defined(EA_PLATFORM_WINDOWS)
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

	#else
		static int nProcessorCount = 0; // This doesn't really need to be an atomic integer.

		if(nProcessorCount == 0)
		{
			// A better function to use would possibly be KeQueryActiveProcessorCount 
			// (NTKERNELAPI ULONG KeQueryActiveProcessorCount(PKAFFINITY ActiveProcessors))

			SYSTEM_INFO systemInfo;
			memset(&systemInfo, 0, sizeof(systemInfo));
			GetNativeSystemInfo(&systemInfo);
			nProcessorCount = (int)systemInfo.dwNumberOfProcessors;
		}

		return nProcessorCount;

	#endif
}


EATHREADLIB_API void EA::Thread::ThreadSleep(const ThreadTime& timeRelative)
{
	// Sleep(0) sleeps the current thread if any other thread of equal priority is ready to run.
	// Sleep(n) sleeps the current thread for up to n milliseconds if there is any other thread of any priority ready to run.
	// SwitchToThread() sleeps the current thread for one time slice if there is any other thread of any priority ready to run.

	if(timeRelative == 0)
		SwitchToThread();  // It's debateable whether we should do a SwitchToThread or a Sleep(0) here. 
	else                        // The only difference is that the former allows threads of lower priority to execute.
		SleepEx((unsigned)timeRelative, TRUE);
}


namespace EA { 
	namespace Thread {
		extern EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId);
		extern EAThreadDynamicData* FindThreadDynamicData(SysThreadId sysThreadId);
	}
}

void EA::Thread::ThreadEnd(intptr_t threadReturnValue)
{
	EAThreadDynamicData* const pTDD = FindThreadDynamicData(GetThreadId());
	if(pTDD)
	{
		pTDD->mnStatus = Thread::kStatusEnded;
		pTDD->mnReturnValue = threadReturnValue;
		pTDD->Release();
	}

	EA::Thread::SetCurrentThreadHandle(kThreadIdInvalid, true); // We use 'true' here just to be safe, as we don't know who is calling this function.

	#if defined(EA_PLATFORM_XBOXONE)
		// _endthreadex is not supported on Capilano because it's not compatible with C++/CX and /ZW.  Use of ExitThread could result in memory leaks
		// as ExitThread does not clean up memory allocated by the C runtime library.
		// https://forums.xboxlive.com/AnswerPage.aspx?qid=47c1607c-bb18-4bc4-a79a-a40c59444ff3&tgt=1        
		ExitThread(static_cast<DWORD>(threadReturnValue));
	#elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PLATFORM_CONSOLE) && !defined(EA_PLATFORM_XBOXONE)
		EAT_FAIL_MSG("EA::Thread::ThreadEnd: Not supported by this platform.");
	#else
		_endthreadex((unsigned int)threadReturnValue);
	#endif
}


EATHREADLIB_API EA::Thread::ThreadTime EA::Thread::GetThreadTime()
{
	// We choose to use GetTickCount because it low overhead and 
	// still yields values that have a precision in the same range
	// as the Win32 thread time slice time. In particular: 
	//     rdtsc takes ~5 cycles and has a nanosecond resolution. But it is unreliable
	//     GetTickCount() takes ~80 cycles and has ~15ms resolution.
	//     timeGetTime() takes ~350 cpu cycles and has 1ms resolution.
	//     QueryPerformanceCounter() takes ~3000 cpu cycles on most machines and has ~1us resolution.
	//     We add EATHREAD_MIN_ABSOLUTE_TIME to this absolute time to ensure this absolute time is never less than our min 
	//     (This fix was required because GetTickCount64 starts at 0x0 for titles on capilano)
	#if   defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64)
		return (ThreadTime)(GetTickCount64() + EATHREAD_MIN_ABSOLUTE_TIME);
	#else                                                      // Note that this value matches the value used by some runtime assertion code within EA::Thread. It would be best to define this as a shared constant between modules.
		return (ThreadTime)(GetTickCount() + EATHREAD_MIN_ABSOLUTE_TIME);
	#endif
}


EATHREADLIB_API void EA::Thread::SetAssertionFailureFunction(EA::Thread::AssertionFailureFunction pAssertionFailureFunction, void* pContext)
{
	gpAssertionFailureFunction = pAssertionFailureFunction;
	gpAssertionFailureContext  = pContext;
}


EATHREADLIB_API void EA::Thread::AssertionFailure(const char* pExpression)
{
	if(gpAssertionFailureFunction)
		gpAssertionFailureFunction(pExpression, gpAssertionFailureContext);
	else
	{
		#if EAT_ASSERT_ENABLED
			OutputDebugStringA("EA::Thread::AssertionFailure: ");
			OutputDebugStringA(pExpression);
			OutputDebugStringA("\n");
			#ifdef _MSC_VER
				__debugbreak();
			#endif
		#endif
	}
}

uint32_t EA::Thread::RelativeTimeoutFromAbsoluteTimeout(ThreadTime timeoutAbsolute)
{
	EAT_ASSERT((timeoutAbsolute == kTimeoutImmediate) || (timeoutAbsolute > EATHREAD_MIN_ABSOLUTE_TIME)); // Assert that the user didn't make the mistake of treating time as relative instead of absolute.

	DWORD timeoutRelative = 0;

	if (timeoutAbsolute == kTimeoutNone)
	{
		timeoutRelative = INFINITE;
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

	EAT_ASSERT((timeoutRelative == INFINITE) || (timeoutRelative < 100000000)); // Assert that the timeout is a sane value and didn't wrap around.

	return timeoutRelative;
}

#endif // EA_PLATFORM_XXX








