///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EABase/eabase.h"
#include "eathread/eathread.h"
#include "eathread/eathread_callstack.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_sync.h"
#include "eathread/eathread_thread.h"
#include "eathread/internal/eathread_global.h"

#if EA_COMPILER_VERSION >= 1900 // VS2015+
// required for windows.h that has mismatch that is included in this file
	EA_DISABLE_VC_WARNING(5031 5032)// #pragma warning(pop): likely mismatch, popping warning state pushed in different file / detected #pragma warning(push) with no corresponding
#endif


// Warning 6312 and 6322 are spurious, as we are not execution a case that could possibly loop.
// 6312: Possible infinite loop: use of the constant EXCEPTION_CONTINUE_EXECUTION in the exception-filter expression of a try-except. Execution restarts in the protected block
// 6322: Empty _except block
EA_DISABLE_VC_WARNING(6312 6322) 


#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	#include <new>
	#include <process.h>

		EA_DISABLE_ALL_VC_WARNINGS()
		#include <Windows.h>
		EA_RESTORE_ALL_VC_WARNINGS()

		#if defined(_MSC_VER)
			struct ThreadNameInfo{
				DWORD  dwType;
				LPCSTR lpName;
				DWORD  dwThreadId;
				DWORD  dwFlags;
			};
			extern "C" WINBASEAPI DWORD WINAPI SetThreadIdealProcessor(_In_ HANDLE hThread, _In_ DWORD dwIdealProcessor);
			extern "C" WINBASEAPI BOOL  WINAPI IsDebuggerPresent();
		#endif


	#ifdef _MSC_VER
		#ifndef EATHREAD_INIT_SEG_DEFINED
			#define EATHREAD_INIT_SEG_DEFINED 
		#endif

		// We are changing the initalization ordering here because in bulkbuild tool builds the initialization 
		// order of globals changes and causes a crash when we attempt to lock the Mutex guarding access
		// of the EAThreadDynamicData objects.  The code attempts to lock a mutex that has been destructed
		// and causes a crash within the WindowsSDK.  This ensures that global mutex object is not destructed
		// until all user code has destructed.
		//
		#ifndef EATHREAD_INIT_SEG_DEFINED
			#define EATHREAD_INIT_SEG_DEFINED 
			#pragma warning(disable: 4075) // warning C4075: initializers put in unrecognized initialization area
			#pragma warning(disable: 4073) //warning C4073: initializers put in library initialization area
			#pragma init_seg(lib)
		#endif
	#endif


	namespace EA { 
		namespace Thread {
			extern void SetCurrentThreadHandle(HANDLE hThread, bool bDynamic);
			namespace Internal { extern void SetThreadName(EAThreadDynamicData* pTDD, const char* pName); };
		}
	}

	
	namespace EA
	{ 
		namespace Thread
		{
			extern Allocator* gpAllocator;
			static AtomicInt32 nLastProcessor = 0;
			const size_t kMaxThreadDynamicDataCount = 128;

			struct EAThreadGlobalVars
			{
				EA_PREFIX_ALIGN(8)
				char        gThreadDynamicData[kMaxThreadDynamicDataCount][sizeof(EAThreadDynamicData)] EA_POSTFIX_ALIGN(8);
				AtomicInt32 gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];
				Mutex       gThreadDynamicMutex;

				EAThreadGlobalVars() {}
				EAThreadGlobalVars(const EAThreadGlobalVars&) {}
				EAThreadGlobalVars& operator=(const EAThreadGlobalVars&) {}
			};
			EATHREAD_GLOBALVARS_CREATE_INSTANCE;

			EAThreadDynamicData* AllocateThreadDynamicData()
			{
				AutoMutex am(EATHREAD_GLOBALVARS.gThreadDynamicMutex);
				for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
				{
					if(EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[i].SetValueConditional(1, 0))
						return (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
				}

				// This is a safety fallback mechanism. In practice it won't be used in almost all situations.
				if(gpAllocator)
					return (EAThreadDynamicData*)gpAllocator->Alloc(sizeof(EAThreadDynamicData));
				else
					return new EAThreadDynamicData; // This is a small problem, as this doesn't just allocate it but also constructs it.
			}

			void FreeThreadDynamicData(EAThreadDynamicData* pEAThreadDynamicData)
			{
				AutoMutex am(EATHREAD_GLOBALVARS.gThreadDynamicMutex);
				if((pEAThreadDynamicData >= (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData) && (pEAThreadDynamicData < ((EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData + kMaxThreadDynamicDataCount)))
				{
					pEAThreadDynamicData->~EAThreadDynamicData();
					EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[pEAThreadDynamicData - (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData].SetValue(0);
				}
				else
				{
					// Assume the data was allocated via the fallback mechanism.
					if(gpAllocator)
					{
						pEAThreadDynamicData->~EAThreadDynamicData();
						gpAllocator->Free(pEAThreadDynamicData);
					}
					else
						delete pEAThreadDynamicData;
				}
			}

			EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId)
			{
				for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
				{
					EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
					if(pTDD->mhThread == threadId)
						return pTDD;
				}
				return NULL; // This is no practical way we can find the data unless thread-specific storage was involved.
			}
			
			EAThreadDynamicData* FindThreadDynamicData(EA::Thread::SysThreadId sysThreadId)
			{
				for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
				{
					EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
					if (pTDD->mnThreadId == sysThreadId)
						return pTDD;
				}
				return nullptr; // This is no practical way we can find the data unless thread-specific storage was involved.
			}

			bool IsDebuggerPresent()
			{
			#if defined(EA_PLATFORM_MICROSOFT)
				return ::IsDebuggerPresent() != 0;
			#else
				return false;
			#endif
			}
		}

	}


	EAThreadDynamicData::EAThreadDynamicData()
	  : mhThread(EA::Thread::kThreadIdInvalid),
		mnThreadId(0), // Note that this is a Windows "thread id", wheras for us thread ids are what Windows calls a thread handle.
		mnStatus(EA::Thread::Thread::kStatusNone),
		mnReturnValue(0),
		mpBeginThreadUserWrapper(NULL),
		mnRefCount(0)
	{
		// Empty
	}


	EAThreadDynamicData::~EAThreadDynamicData()
	{
		if(mhThread)
			::CloseHandle(mhThread);

		mhThread = EA::Thread::kThreadIdInvalid;
		mnThreadId = 0;
	}


	void EAThreadDynamicData::AddRef()
	{
		mnRefCount.Increment();
	}


	void EAThreadDynamicData::Release()
	{
		if(mnRefCount.Decrement() == 0)
			EA::Thread::FreeThreadDynamicData(this);
	}

	EA::Thread::ThreadParameters::ThreadParameters()
		: mpStack(NULL)
		, mnStackSize(0)
		, mnPriority(kThreadPriorityDefault)
		, mnProcessor(kProcessorDefault)
		, mnAffinityMask(kThreadAffinityMaskAny)
		, mpName("")
		, mbDisablePriorityBoost(false)
	{
	}

	EA::Thread::RunnableFunctionUserWrapper  EA::Thread::Thread::sGlobalRunnableFunctionUserWrapper = NULL;
	EA::Thread::RunnableClassUserWrapper     EA::Thread::Thread::sGlobalRunnableClassUserWrapper    = NULL;
	EA::Thread::AtomicInt32                  EA::Thread::Thread::sDefaultProcessor                  = kProcessorAny;
	EA::Thread::AtomicUint64                 EA::Thread::Thread::sDefaultProcessorMask              = UINT64_C(0xffffffffffffffff);

	EA::Thread::RunnableFunctionUserWrapper EA::Thread::Thread::GetGlobalRunnableFunctionUserWrapper()
	{
		return sGlobalRunnableFunctionUserWrapper;
	}

	void EA::Thread::Thread::SetGlobalRunnableFunctionUserWrapper(EA::Thread::RunnableFunctionUserWrapper pUserWrapper)
	{
		if(sGlobalRunnableFunctionUserWrapper != NULL)
		{
			 // Can only be set once in entire game. 
			 EAT_ASSERT(false);
		}
		else
		{
			sGlobalRunnableFunctionUserWrapper = pUserWrapper;
		}
	}

	EA::Thread::RunnableClassUserWrapper EA::Thread::Thread::GetGlobalRunnableClassUserWrapper()
	{
		return sGlobalRunnableClassUserWrapper;
	}

	void EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(EA::Thread::RunnableClassUserWrapper pUserWrapper)
	{
		if(sGlobalRunnableClassUserWrapper != NULL)
		{
			// Can only be set once. 
			EAT_ASSERT(false);
		}
		else
			sGlobalRunnableClassUserWrapper = pUserWrapper;
	}

	// Helper that selects a target processor based on the provided ThreadParameters structure and the various
	// pieces of shared state that EAThread maintains to implement a 'round-robin' style processor selection.
	int SelectProcessor(const EA::Thread::ThreadParameters* pTP, EA::Thread::AtomicInt32& sDefaultProcessor, EA::Thread::AtomicUint64& sDefaultProcessorMask)
	{
		int nProcessor;

		if (pTP && (pTP->mnProcessor >= 0))
		{
			nProcessor = pTP->mnProcessor;

			// This is a small attempt to try to spread out threads between processors.  We don't
			// care much if another thread happens to be created here and races with this.
			if (nProcessor == EA::Thread::nLastProcessor)
				++EA::Thread::nLastProcessor;
		}
		else
		{
		#if defined(EA_PLATFORM_MICROSOFT)
			if (!pTP || pTP->mnProcessor == EA::Thread::kProcessorAny)
			{
				// If the processor is not specified, then allow the scheduler to 
				// run the thread on any available processor
				nProcessor = EA::Thread::kProcessorDefault;
			}
			else
		#endif

			if (sDefaultProcessor >= 0) // If the user has identified a specific processor...
				nProcessor = sDefaultProcessor;
			else if(sDefaultProcessor == EA::Thread::kProcessorDefault) // If the user explicitly asked for the default processor
				nProcessor = sDefaultProcessor;
			else
			{
				// NOTE(rparolin): The reason we have this round-robin code is that the
				// originally we used it on Xenon OS which required us to pick a CPU to run on.
				// After the Xenon was deprecated this code remained and is now a functional
				// requirement.  We should probably deprecate and remove in the future but
				// currently teams are dependent on it.
				const uint64_t processorMask = sDefaultProcessorMask.GetValue();

				do
				{
					nProcessor = EA::Thread::nLastProcessor.Increment();

					if (nProcessor == MAXIMUM_PROCESSORS)
					{
						EA::Thread::nLastProcessor.SetValueConditional(0, MAXIMUM_PROCESSORS);
						nProcessor = 0;
					}
				} while ((((uint64_t)1 << nProcessor) & processorMask) == 0);
			}
		}

		return nProcessor;
	}

	EA::Thread::Thread::Thread()
	{
		mThreadData.mpData = NULL;
	}

	EA::Thread::Thread::Thread(const Thread& t)
	  : mThreadData(t.mThreadData)
	{
		if(mThreadData.mpData)
			mThreadData.mpData->AddRef();
	}


	EA::Thread::Thread& EA::Thread::Thread::operator=(const Thread& t)
	{
		// We don't synchronize access to mpData; we assume that the user 
		// synchronizes it or this Thread instances is used from a single thread.
		if(t.mThreadData.mpData)
			t.mThreadData.mpData->AddRef();

		if(mThreadData.mpData)
			mThreadData.mpData->Release();

		mThreadData = t.mThreadData;

		return *this;
	}


	EA::Thread::Thread::~Thread()
	{
		// We don't synchronize access to mpData; we assume that the user 
		// synchronizes it or this Thread instances is used from a single thread.
		if(mThreadData.mpData)
			mThreadData.mpData->Release();
	}

  #if defined(EA_PLATFORM_XBOXONE)
	static DWORD WINAPI RunnableFunctionInternal(void* pContext)
  #else
	static unsigned int __stdcall RunnableFunctionInternal(void* pContext)
  #endif
	{
		// The parent thread is sharing memory with us and we need to 
		// make sure our view of it is synchronized with the parent.
		EAReadWriteBarrier();
		

		EAThreadDynamicData* const pTDD        = (EAThreadDynamicData*)pContext; 
		EA::Thread::RunnableFunction pFunction = (EA::Thread::RunnableFunction)pTDD->mpStartContext[0];
		void* pCallContext                     = pTDD->mpStartContext[1];

		EA::Thread::SetCurrentThreadHandle(pTDD->mpStartContext[2], false);
		pTDD->mpStackBase = EA::Thread::GetStackBase();
		pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

		EA::Thread::SetThreadName(pTDD->mhThread, pTDD->mName);

		if(pTDD->mpBeginThreadUserWrapper != NULL)
		{
			EA::Thread::RunnableFunctionUserWrapper pWrapperFunction = (EA::Thread::RunnableFunctionUserWrapper)pTDD->mpBeginThreadUserWrapper;
			// if user wrapper is specified, call user wrapper and pass down the pFunction and pContext
			pTDD->mnReturnValue = pWrapperFunction(pFunction, pCallContext);
		}
		else
		{
			pTDD->mnReturnValue = pFunction(pCallContext);
		}
		
		const unsigned int nReturnValue = (unsigned int)pTDD->mnReturnValue;
		EA::Thread::SetCurrentThreadHandle(0, false);
		pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
		pTDD->Release();
		return nReturnValue;
	}

	void EA::Thread::Thread::SetAffinityMask(EA::Thread::ThreadAffinityMask nAffinityMask)
	{
		if(mThreadData.mpData->mhThread)
		{
			EA::Thread::SetThreadAffinityMask(mThreadData.mpData->mhThread, nAffinityMask);
		}
	}

	EA::Thread::ThreadAffinityMask EA::Thread::Thread::GetAffinityMask()
	{
		if(mThreadData.mpData->mhThread)
		{
			return mThreadData.mpData->mnThreadAffinityMask;
		}

		return kThreadAffinityMaskAny;
	}

	EA::Thread::ThreadId EA::Thread::Thread::Begin(RunnableFunction pFunction, void* pContext, const ThreadParameters* pTP, RunnableFunctionUserWrapper pUserWrapper)
	{
		// Check there is an entry for the current thread context in our ThreadDynamicData array.        
		ThreadId thisThreadId = GetThreadId();
		if(!FindThreadDynamicData(thisThreadId))
		{
			EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData;
			if(pData)
			{
				pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
								 // Do no AddRef for thread execution because this is not an EAThread managed thread.
				pData->AddRef(); // AddRef for this function, to be released upon this function's exit.                
				pData->mhThread = thisThreadId;
				pData->mnThreadId = GetCurrentThreadId();
				strncpy(pData->mName, "external", EATHREAD_NAME_SIZE);
				pData->mName[EATHREAD_NAME_SIZE - 1] = 0;
				pData->mpStackBase = EA::Thread::GetStackBase();
			}
		}

		if(mThreadData.mpData)
			mThreadData.mpData->Release(); // Matches the "AddRef for ourselves" below.

		// Win32-like platforms don't support user-supplied stacks. A user-supplied stack pointer 
		// here would be a waste of user memory, and so we assert that mpStack == NULL.
		EAT_ASSERT(!pTP || (pTP->mpStack == NULL));

		// We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
		// modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
		// during execution.
		EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData; // Note that we use a special new here which doesn't use the heap.
		mThreadData.mpData = pData;

		pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
		pData->AddRef(); // AddRef for the thread, to be released upon the thread exiting.
		pData->AddRef(); // AddRef for this function, to be released upon this function's exit.
		pData->mhThread = kThreadIdInvalid;
		pData->mpStartContext[0] = pFunction;
		pData->mpStartContext[1] = pContext;
		pData->mpBeginThreadUserWrapper = pUserWrapper;
		pData->mnThreadAffinityMask = pTP ? pTP->mnAffinityMask : kThreadAffinityMaskAny;

		const unsigned nStackSize = pTP ? (unsigned)pTP->mnStackSize : 0;

		#if defined(EA_PLATFORM_XBOXONE)
			// Capilano no longer supports _beginthreadex. Using CreateThread instead may cause issues when using the MS CRT 
			// according to MSDN (memory leaks or possibly crashes) as it does not initialize the CRT. This a reasonable
			// workaround while we wait for clarification from MS on what the recommended threading APIs are for Capilano.
			HANDLE hThread = CreateThread(NULL, nStackSize, RunnableFunctionInternal, pData, CREATE_SUSPENDED, reinterpret_cast<LPDWORD>(&pData->mnThreadId));
		#else
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, nStackSize, RunnableFunctionInternal, pData, CREATE_SUSPENDED, &pData->mnThreadId);
		#endif

		if(hThread)
		{
			pData->mhThread = hThread;

			if(pTP)
				SetName(pTP->mpName);
			pData->mpStartContext[2] = hThread;

			if(pTP && (pTP->mnPriority != kThreadPriorityDefault))
				SetPriority(pTP->mnPriority);

			#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE)
			if (pTP)
			{
				auto result = SetThreadPriorityBoost(pData->mhThread, pTP->mbDisablePriorityBoost);
				EAT_ASSERT(result != 0);
				EA_UNUSED(result);
			}
			#endif

			#if defined(EA_PLATFORM_MICROSOFT)
				int nProcessor = SelectProcessor(pTP, sDefaultProcessor, sDefaultProcessorMask);
				if(pTP && pTP->mnProcessor == EA::Thread::kProcessorAny)
					SetAffinityMask(pTP->mnAffinityMask);
				else
					SetProcessor(nProcessor);
			#endif

			ResumeThread(hThread);
			pData->Release(); // Matches AddRef for this function.
			return hThread;
		}

		pData->Release(); // Matches AddRef for this function.
		pData->Release(); // Matches AddRef for this Thread class above.
		pData->Release(); // Matches AddRef for the thread above.
		mThreadData.mpData = NULL; // mThreadData.mpData == pData

		return (ThreadId)kThreadIdInvalid;
	}

  #if defined(EA_PLATFORM_XBOXONE)
	static DWORD WINAPI RunnableObjectInternal(void* pContext)
  #else
	static unsigned int __stdcall RunnableObjectInternal(void* pContext)
  #endif
	{
		// The parent thread is sharing memory with us and we need to 
		// make sure our view of it is synchronized with the parent.
		EAReadWriteBarrier();

		EAThreadDynamicData* const pTDD  = (EAThreadDynamicData*)pContext;
		EA::Thread::IRunnable* pRunnable = (EA::Thread::IRunnable*)pTDD->mpStartContext[0];
		void* pCallContext               = pTDD->mpStartContext[1];

		EA::Thread::SetCurrentThreadHandle(pTDD->mpStartContext[2], false);
		pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

		EA::Thread::SetThreadName(pTDD->mhThread, pTDD->mName);

		if(pTDD->mpBeginThreadUserWrapper)
		{
			 EA::Thread::RunnableClassUserWrapper pWrapperClass = (EA::Thread::RunnableClassUserWrapper)pTDD->mpBeginThreadUserWrapper;
			 // if user wrapper is specified, call user wrapper and pass down the pFunction and pContext
			 pTDD->mnReturnValue = pWrapperClass(pRunnable, pCallContext);
		}
		else
			 pTDD->mnReturnValue = pRunnable->Run(pCallContext);

		const unsigned int nReturnValue = (unsigned int)pTDD->mnReturnValue;
		EA::Thread::SetCurrentThreadHandle(0, false);
		pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
		pTDD->Release();
		return nReturnValue;
	}


	EA::Thread::ThreadId EA::Thread::Thread::Begin(IRunnable* pRunnable, void* pContext, const ThreadParameters* pTP, RunnableClassUserWrapper pUserWrapper)
	{
		if(mThreadData.mpData)
			mThreadData.mpData->Release(); // Matches the "AddRef for ourselves" below.

		// Win32-like platforms don't support user-supplied stacks. A user-supplied stack pointer 
		// here would be a waste of user memory, and so we assert that mpStack == NULL.
		EAT_ASSERT(!pTP || (pTP->mpStack == NULL));

		// We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
		// modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
		// during execution.
		EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData; // Note that we use a special new here which doesn't use the heap.
		mThreadData.mpData = pData;

		pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
		pData->AddRef(); // AddRef for the thread, to be released upon the thread exiting.
		pData->AddRef(); // AddRef for this function, to be released upon this function's exit.
		pData->mhThread = kThreadIdInvalid;
		pData->mpStartContext[0] = pRunnable;
		pData->mpStartContext[1] = pContext;
		pData->mpBeginThreadUserWrapper = pUserWrapper;
		pData->mnThreadAffinityMask = pTP ? pTP->mnAffinityMask : kThreadAffinityMaskAny;
		const unsigned nStackSize     = pTP ? (unsigned)pTP->mnStackSize : 0;

		#if defined(EA_PLATFORM_XBOXONE)
			// Capilano no longer supports _beginthreadex. Using CreateThread instead may cause issues when using the MS CRT 
			// according to MSDN (memory leaks or possibly crashes) as it does not initialize the CRT. This a reasonable
			// workaround while we wait for clarification from MS on what the recommended threading APIs are for Capilano.
			HANDLE hThread = CreateThread(NULL, nStackSize, RunnableObjectInternal, pData, CREATE_SUSPENDED, reinterpret_cast<LPDWORD>(&pData->mnThreadId));
		#else
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, nStackSize, RunnableObjectInternal, pData, CREATE_SUSPENDED, &pData->mnThreadId);
		#endif

		if(hThread)
		{
			pData->mhThread = hThread;

			if(pTP)
				SetName(pTP->mpName);

			pData->mpStartContext[2] = hThread;

			if(pTP && (pTP->mnPriority != kThreadPriorityDefault))
				SetPriority(pTP->mnPriority);

			#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE)
			if (pTP)
			{
				auto result = SetThreadPriorityBoost(pData->mhThread, pTP->mbDisablePriorityBoost);
				EAT_ASSERT(result != 0);
				EA_UNUSED(result);
			}
			#endif

			#if defined(EA_PLATFORM_MICROSOFT)
				int nProcessor = SelectProcessor(pTP, sDefaultProcessor, sDefaultProcessorMask);
				if(pTP && pTP->mnProcessor == EA::Thread::kProcessorAny)
					SetAffinityMask(pTP->mnAffinityMask);
				else
					SetProcessor(nProcessor);
			#endif

			ResumeThread(hThread); // This will unsuspend the thread.
			pData->Release(); // Matches AddRef for this function.
			return hThread;
		}

		pData->Release(); // Matches AddRef for this function.
		pData->Release(); // Matches AddRef for this Thread class above.
		pData->Release(); // Matches AddRef for the thread above.
		mThreadData.mpData = NULL;

		return (ThreadId)kThreadIdInvalid;
	}


	EA::Thread::Thread::Status EA::Thread::Thread::WaitForEnd(const ThreadTime& timeoutAbsolute, intptr_t* pThreadReturnValue)
	{
		// The mThreadData memory is shared between threads and when 
		// reading it we must be synchronized.
		EAReadWriteBarrier();

		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 
		// Todo: Consider that there may be a subtle race condition here if 
		// the user immediately calls WaitForEnd right after calling Begin.
		if(mThreadData.mpData)
		{
			if(mThreadData.mpData->mhThread) // If it was started...
			{
				// We must not call WaitForEnd from the thread we are waiting to end. That would result in a deadlock.
				EAT_ASSERT(mThreadData.mpData->mhThread != EA::Thread::GetThreadId());
				// dwResult normally should be 'WAIT_OBJECT_0', but can also be WAIT_ABANDONED or WAIT_FAILED.
				const DWORD dwResult = ::WaitForSingleObject(mThreadData.mpData->mhThread, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));
				if(dwResult == WAIT_TIMEOUT)
					return kStatusRunning;

				// Close the handle now so as to minimize handle proliferation.
				::CloseHandle(mThreadData.mpData->mhThread); 
				mThreadData.mpData->mhThread = 0;
				mThreadData.mpData->mnStatus = kStatusEnded;
			}

			if(pThreadReturnValue)
			{
				EAReadWriteBarrier();
				*pThreadReturnValue = mThreadData.mpData->mnReturnValue;
			}
			return kStatusEnded; // A thread was created, so it must have ended.
		}
		else
		{
			// Else the user hasn't started the thread yet, so we wait until the user starts it.
			// Ideally, what we really want to do here is wait for some kind of signal. 
			// Instead for the time being we do a polling loop. 
			while((!mThreadData.mpData || !mThreadData.mpData->mhThread) && (GetThreadTime() < timeoutAbsolute))
			{
				ThreadSleep(1);
				EAReadWriteBarrier();
				EACompilerMemoryBarrier();
			}
			if(mThreadData.mpData)
				return WaitForEnd(timeoutAbsolute);
		}
		return kStatusNone; // No thread has been started.
	}


	EA::Thread::Thread::Status EA::Thread::Thread::GetStatus(intptr_t* pThreadReturnValue) const
	{
		// The mThreadData memory is shared between threads and when 
		// reading it we must be synchronized.
		EAReadWriteBarrier();

		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 
		if(mThreadData.mpData)
		{
			if(mThreadData.mpData->mhThread) // If the thread has been started...
			{
				DWORD dwExitStatus;

				// Note that GetExitCodeThread is a hazard if the user of a thread exits 
				// with a return value that is equal to the value of STILL_ACTIVE (i.e. 259).
				// We can document that users shouldn't do this, or we can change the code 
				// here to use WaitForSingleObject(hThread, 0) and assume the thread is 
				// still active if the return value is WAIT_TIMEOUT.
				if(::GetExitCodeThread(mThreadData.mpData->mhThread, &dwExitStatus))
				{
					if(dwExitStatus == STILL_ACTIVE)
						return kStatusRunning; // Nothing has changed.
					::CloseHandle(mThreadData.mpData->mhThread); // Do this now so as to minimize handle proliferation.
					mThreadData.mpData->mhThread = 0;
				} // else fall through.
			} // else fall through.

			if(pThreadReturnValue)
				*pThreadReturnValue = mThreadData.mpData->mnReturnValue;
			mThreadData.mpData->mnStatus = kStatusEnded;
			return kStatusEnded;
		}
		return kStatusNone;
	}


	EA::Thread::ThreadId EA::Thread::Thread::GetId() const
	{
		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 
		if(mThreadData.mpData)
			return (ThreadId)mThreadData.mpData->mhThread;
		return kThreadIdInvalid;
	}


	int EA::Thread::Thread::GetPriority() const
	{
		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 
		if(mThreadData.mpData)
		{
			const int nPriority = ::GetThreadPriority(mThreadData.mpData->mhThread);
			return kThreadPriorityDefault + (nPriority - THREAD_PRIORITY_NORMAL);
		}
		return kThreadPriorityUnknown;
	}


	bool EA::Thread::Thread::SetPriority(int nPriority)
	{
		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 

		// For more information on how Windows handle thread priority based on process priority, see
		// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/scheduling_priorities.asp

		EAT_ASSERT(nPriority != kThreadPriorityUnknown);
		if(mThreadData.mpData)
		{
			int nNewPriority = THREAD_PRIORITY_NORMAL + (nPriority - kThreadPriorityDefault);
			bool result = ::SetThreadPriority(mThreadData.mpData->mhThread, nNewPriority) != 0;

			// Windows process running in NORMAL_PRIORITY_CLASS is picky about the priority passed in.
			// So we need to set the priority to the next priority supported
			#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE)
				while(!result)
				{
					if(nNewPriority >= THREAD_PRIORITY_TIME_CRITICAL) 
						return ::SetThreadPriority(mThreadData.mpData->mhThread, THREAD_PRIORITY_TIME_CRITICAL) != 0;

					if(nNewPriority <= THREAD_PRIORITY_IDLE) 
						return ::SetThreadPriority(mThreadData.mpData->mhThread, THREAD_PRIORITY_IDLE) != 0;

					result = ::SetThreadPriority(mThreadData.mpData->mhThread, nNewPriority) != 0;
					nNewPriority++;
				}

			#endif

			return result;
		}
		return false;
	}


	void EA::Thread::Thread::SetProcessor(int nProcessor)
	{
		if(mThreadData.mpData)
		{
			#if defined(EA_PLATFORM_XBOXONE)

				static int nProcessorCount = GetProcessorCount();
				if(nProcessor >= nProcessorCount)
					nProcessor %= nProcessorCount;

				ThreadAffinityMask mask = 0x7F; // default to all 7 available cores.
				if (nProcessor >= 0)
					mask = ((ThreadAffinityMask)1) << nProcessor;
				
				SetThreadAffinityMask(mThreadData.mpData->mhThread, mask);

			#else
				static int nProcessorCount = GetProcessorCount();

				if(nProcessor < 0)
					nProcessor = MAXIMUM_PROCESSORS; // This causes the SetThreadIdealProcessor to reset to 'no ideal processor'.
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
				SetThreadIdealProcessor(mThreadData.mpData->mhThread, (DWORD)nProcessor);

			#endif
		}
	}


	typedef VOID (APIENTRY *PAPCFUNC)(_In_ ULONG_PTR dwParam);
	extern "C" WINBASEAPI DWORD WINAPI QueueUserAPC(_In_ PAPCFUNC pfnAPC, _In_ HANDLE hThread, _In_ ULONG_PTR dwData);

	void EA::Thread::Thread::Wake()
	{
		// A mutex lock around mpData is not needed below because 
		// mpData is never allowed to go from non-NULL to NULL. 
		struct ThreadWake{ static void WINAPI Empty(ULONG_PTR){} };
		if(mThreadData.mpData && mThreadData.mpData->mhThread)
			::QueueUserAPC((PAPCFUNC)ThreadWake::Empty, mThreadData.mpData->mhThread, 0); 
	}


	const char* EA::Thread::Thread::GetName() const
	{
		if(mThreadData.mpData)
			return mThreadData.mpData->mName;

		return "";
	}


	void EA::Thread::Thread::SetName(const char* pName)
	{
		if (mThreadData.mpData && pName)
			EA::Thread::Internal::SetThreadName(mThreadData.mpData, pName);
	}


#endif // EA_PLATFORM_MICROSOFT

EA_RESTORE_VC_WARNING()

#if EA_COMPILER_VERSION >= 1900 // VS2015+
	EA_RESTORE_VC_WARNING()// #pragma warning(pop): likely mismatch, popping warning state pushed in different file / detected #pragma warning(push) with no corresponding
#endif



