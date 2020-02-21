///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_callstack.h>
#include <new>
#include <kernel.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sceerror.h>


#define EA_ALLOW_POSIX_THREADS_PRIORITIES 1


namespace
{
	// We convert a an EAThread priority (higher value implies higher priority) to a native priority 
	// value, as some implementations of pthread_disableds use lower values to indicate higher priority.
	void ConvertToNativePriority(int eathreadPriority, sched_param& param, int& policy)
	{
		using namespace EA::Thread;

		policy = SCE_KERNEL_SCHED_RR;

		const int nMin = SCE_KERNEL_PRIO_FIFO_HIGHEST;
		const int nMax = SCE_KERNEL_PRIO_FIFO_LOWEST;

		// Kettle pthread_disableds uses a reversed interpretation of sched_get_priority_min and sched_get_priority_max.
		param.sched_priority = (SCE_KERNEL_PRIO_FIFO_DEFAULT + (-1 * eathreadPriority));

		if(param.sched_priority < nMin)
			param.sched_priority = nMin;
		else if(param.sched_priority > nMax)
			param.sched_priority = nMax;
	}


	// We convert a native priority value to an EAThread priority (higher value implies higher 
	// priority), as some implementations of pthread_disableds use lower values to indicate higher priority.
	int ConvertFromNativePriority(const sched_param& param, int policy)
	{
		using namespace EA::Thread;

		// Some implementations of pthreads associate higher priorities with smaller
		// integer values. We hide this. To the user, a higher value must always
		// indicate higher priority.

		// Kettle pthread_disableds uses a reversed interpretation of sched_get_priority_min and sched_get_priority_max.
		return -1 * (param.sched_priority - SCE_KERNEL_PRIO_FIFO_DEFAULT);
	}


	// Setup stack and/or priority of a new thread
	void SetupThreadAttributes(ScePthreadAttr& creationAttribs, const EA::Thread::ThreadParameters* pTP)
	{
		int result = 0;
		EA_UNUSED( result ); //only used for assertions

		// We create the thread as attached, and we'll call either pthread_disabled_join or pthread_disabled_detach, 
		// depending on whether WaitForEnd (pthread_disabled_join) is called or not (pthread_disabled_detach).

		if(pTP)
		{
			// Set thread stack address and/or size
			if(pTP->mpStack)
			{
				EAT_ASSERT(pTP->mnStackSize != 0);
				result = scePthreadAttrSetstack(&creationAttribs, (void*)pTP->mpStack, pTP->mnStackSize);
				EAT_ASSERT(result == 0);
			}
			else if(pTP->mnStackSize)
			{
				result = scePthreadAttrSetstacksize(&creationAttribs, pTP->mnStackSize);
				EAT_ASSERT(result == 0);
			}

			// Set initial non-zero priority
			// Even if pTP->mnPriority == kThreadPriorityDefault, we need to run this on some platforms, as the thread priority for new threads on them isn't the same as the thread priority for the main thread.
			int         policy = SCHED_OTHER;
			sched_param param;

			ConvertToNativePriority(pTP->mnPriority, param, policy);
			result = scePthreadAttrSetschedpolicy(&creationAttribs, policy);
			EAT_ASSERT(result == 0);
			result = scePthreadAttrSetschedparam(&creationAttribs, &param);
			EAT_ASSERT(result == 0);

			// Unix doesn't let you specify thread CPU affinity via pthread_disabled attributes.
			// Instead you need to call sched_setaffinity or pthread_setaffinity_np.
		}
		else
		{
			result = scePthreadAttrSetschedpolicy(&creationAttribs, SCE_KERNEL_SCHED_RR);
			EAT_ASSERT(result == 0);
		}
	}

// This function is not currently used if the thread name can be set from any other thread
#if !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED

	void SetCurrentThreadName(const char8_t* pName)
	{
		EAT_COMPILETIME_ASSERT(EATHREAD_NAME_SIZE == 32);  // New name (up to 32 bytes including the NULL terminator), or NULL  
		scePthreadRename(scePthreadSelf(), pName);
	}

#endif

	static void SetPlatformThreadAffinity(EAThreadDynamicData* pTDD)
	{
		if(pTDD->mThreadId != EA::Thread::kThreadIdInvalid) // If the thread has been created...
		{
			SceKernelCpumask mask;
			mask = (1 << pTDD->mStartupProcessor) & 0xFF;
			int nResult = scePthreadSetaffinity(pTDD->mSysThreadId, mask);
			EAT_ASSERT(nResult == SCE_OK); EA_UNUSED(nResult);
		}
		// Else the thread hasn't started yet, or has already exited. Let the thread set its own 
		// affinity when it starts.
	}

} // namespace




namespace EA
{ 
	namespace Thread
	{
		extern Allocator* gpAllocator;

		const size_t kMaxThreadDynamicDataCount = 128;

		struct EAThreadGlobalVars
		{
			EA_PREFIX_ALIGN(8)
			char        gThreadDynamicData[kMaxThreadDynamicDataCount][sizeof(EAThreadDynamicData)] EA_POSTFIX_ALIGN(8);
			AtomicInt32 gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];
			Mutex gThreadDynamicMutex;
		};
		EATHREAD_GLOBALVARS_CREATE_INSTANCE;       

		EAThreadDynamicData* AllocateThreadDynamicData()
		{
			for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
			{
				if(EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[i].SetValueConditional(1, 0))
					return (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
			}

			// This is a safety fallback mechanism. In practice it won't be used in almost all situations.
			if(gpAllocator)
				return (EAThreadDynamicData*)gpAllocator->Alloc(sizeof(EAThreadDynamicData));
			else
				return (EAThreadDynamicData*)new char[sizeof(EAThreadDynamicData)]; // We assume the returned alignment is sufficient.
		}

		void FreeThreadDynamicData(EAThreadDynamicData* pEAThreadDynamicData)
		{
			if((pEAThreadDynamicData >= (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData) && (pEAThreadDynamicData < ((EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData + kMaxThreadDynamicDataCount)))
			{
				pEAThreadDynamicData->~EAThreadDynamicData();
				EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[pEAThreadDynamicData - (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData].SetValue(0);
			}
			else
			{
				// Assume the data was allocated via the fallback mechanism.
				pEAThreadDynamicData->~EAThreadDynamicData();
				if(gpAllocator)
					gpAllocator->Free(pEAThreadDynamicData);
				else
					delete[] (char*)pEAThreadDynamicData;
			}
		}

		// This is a public function.
		EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId)
		{
			for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
			{
				EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];

				if(pTDD->mThreadId == threadId)
					return pTDD;
			}

			return NULL; // This is no practical way we can find the data unless thread-specific storage was involved.
		}

		EAThreadDynamicData* FindThreadDynamicData(SysThreadId sysThreadId)
		{
			for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
			{
				EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];

				if(pTDD->mSysThreadId == sysThreadId)
					return pTDD;
			}

			return NULL; // This is no practical way we can find the data unless thread-specific storage was involved.
		}
	}
}


EAThreadDynamicData::EAThreadDynamicData()
  : mThreadId(EA::Thread::kThreadIdInvalid),
	mSysThreadId(0),
	mThreadPid(0),
	mnStatus(EA::Thread::Thread::kStatusNone),
	mnReturnValue(0),
  //mpStartContext[],
	mpBeginThreadUserWrapper(NULL),
	mnRefCount(0),
  //mName[],
	mStartupProcessor(EA::Thread::kProcessorDefault),
	mRunMutex(),
	mStartedSemaphore(),
	mnThreadAffinityMask(EA::Thread::kThreadAffinityMaskAny)
{
	memset(mpStartContext, 0, sizeof(mpStartContext));
	memset(mName, 0, sizeof(mName));
}


EAThreadDynamicData::~EAThreadDynamicData()
{
	if(mThreadId != EA::Thread::kThreadIdInvalid)
		scePthreadDetach(mSysThreadId);

	mThreadId = EA::Thread::kThreadIdInvalid;
	mThreadPid = 0;
	mSysThreadId = 0;
}


void EAThreadDynamicData::AddRef()
{
	mnRefCount.Increment();    // Note that mnRefCount is an AtomicInt32.
}


void EAThreadDynamicData::Release()
{
	if(mnRefCount.Decrement() == 0)   // Note that mnRefCount is an AtomicInt32.
		EA::Thread::FreeThreadDynamicData(this);
}


EA::Thread::ThreadParameters::ThreadParameters()
  : mpStack(NULL),
	mnStackSize(0),
	mnPriority(kThreadPriorityDefault),  
	mnProcessor(kProcessorDefault),
	mpName(""),
	mnAffinityMask(kThreadAffinityMaskAny), 
	mbDisablePriorityBoost(false)
{
	// Empty
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
	if(sGlobalRunnableFunctionUserWrapper)
		EAT_FAIL_MSG("Thread::SetGlobalRunnableFunctionUserWrapper already set."); // Can only be set once for the application. 
	else
		sGlobalRunnableFunctionUserWrapper = pUserWrapper;
}


EA::Thread::RunnableClassUserWrapper EA::Thread::Thread::GetGlobalRunnableClassUserWrapper()
{
	return sGlobalRunnableClassUserWrapper;
}


void EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(EA::Thread::RunnableClassUserWrapper pUserWrapper)
{
	if(sGlobalRunnableClassUserWrapper)
		EAT_FAIL_MSG("EAThread::SetGlobalRunnableClassUserWrapper already set."); // Can only be set once for the application. 
	else
		sGlobalRunnableClassUserWrapper = pUserWrapper;
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


static void* RunnableFunctionInternal(void* pContext)
{
	// The parent thread is sharing memory with us and we need to 
	// make sure our view of it is synchronized with the parent.
	EAReadWriteBarrier();

	EAThreadDynamicData* const pTDD        = (EAThreadDynamicData*)pContext; 
	EA::Thread::RunnableFunction pFunction = (EA::Thread::RunnableFunction)pTDD->mpStartContext[0];
	void* pCallContext                     = pTDD->mpStartContext[1];

	pTDD->mThreadPid = 0;

	// Lock the runtime mutex which is used to allow other threads to wait on this thread with a timeout.
	pTDD->mRunMutex.Lock();         // Important that this be before the semaphore post.
	pTDD->mStartedSemaphore.Post(); // Announce that the thread has started.
	pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;
	pTDD->mpStackBase = EA::Thread::GetStackBase();

#if !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
	// Under Unix we need to set the thread name from the thread that is being named and not from an outside thread.
	if(pTDD->mName[0])
		SetCurrentThreadName(pTDD->mName);
#endif

	#ifdef EA_PLATFORM_ANDROID
		AttachJavaThread();
	#endif

	if(pTDD->mpBeginThreadUserWrapper)
	{
		// If user wrapper is specified, call user wrapper and pass the pFunction and pContext.
		EA::Thread::RunnableFunctionUserWrapper pWrapperFunction = (EA::Thread::RunnableFunctionUserWrapper)pTDD->mpBeginThreadUserWrapper;
		pTDD->mnReturnValue = pWrapperFunction(pFunction, pCallContext);
	}
	else
		pTDD->mnReturnValue = pFunction(pCallContext);

	#ifdef EA_PLATFORM_ANDROID
		DetachJavaThread();
	#endif

	void* pReturnValue = (void*)pTDD->mnReturnValue;
	pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
	pTDD->mRunMutex.Unlock();
	pTDD->Release();

	return pReturnValue;
}

static void* RunnableObjectInternal(void* pContext)
{
	EAThreadDynamicData* const pTDD  = (EAThreadDynamicData*)pContext; 
	EA::Thread::IRunnable* pRunnable = (EA::Thread::IRunnable*)pTDD->mpStartContext[0];
	void* pCallContext               = pTDD->mpStartContext[1];

	pTDD->mThreadPid = 0;

	pTDD->mRunMutex.Lock();         // Important that this be before the semaphore post.
	pTDD->mStartedSemaphore.Post();

	pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

#if !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
	// Under Unix we need to set the thread name from the thread that is being named and not from an outside thread.
	if(pTDD->mName[0])
		SetCurrentThreadName(pTDD->mName);
#endif

	#ifdef EA_PLATFORM_ANDROID
		AttachJavaThread();
	#endif

	if(pTDD->mpBeginThreadUserWrapper)
	{
		// If user wrapper is specified, call user wrapper and pass the pFunction and pContext.
		EA::Thread::RunnableClassUserWrapper pWrapperClass = (EA::Thread::RunnableClassUserWrapper)pTDD->mpBeginThreadUserWrapper;
		pTDD->mnReturnValue = pWrapperClass(pRunnable, pCallContext);
	}
	else
		pTDD->mnReturnValue = pRunnable->Run(pCallContext);

	#ifdef EA_PLATFORM_ANDROID
		DetachJavaThread();
	#endif

	void* const pReturnValue = (void*)pTDD->mnReturnValue;
	pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
	pTDD->mRunMutex.Unlock();
	pTDD->Release();

	return pReturnValue;
}

void EA::Thread::Thread::SetAffinityMask(EA::Thread::ThreadAffinityMask nAffinityMask)
{
	if(mThreadData.mpData && mThreadData.mpData->mThreadId)
	{
		EA::Thread::SetThreadAffinityMask(mThreadData.mpData->mThreadId, nAffinityMask);
	}
}

EA::Thread::ThreadAffinityMask EA::Thread::Thread::GetAffinityMask()
{
	if(mThreadData.mpData->mThreadId)
	{
		return mThreadData.mpData->mnThreadAffinityMask;
	}

	return kThreadAffinityMaskAny;
}

/// BeginThreadInternal
/// Extraction of both RunnableFunction and RunnableObject EA::Thread::Begin in order to have thread initialization
/// in one place
static EA::Thread::ThreadId BeginThreadInternal(EAThreadData& mThreadData, void* pRunnableOrFunction, void* pContext, const EA::Thread::ThreadParameters* pTP,
												void* pUserWrapper, void* (*InternalThreadFunction)(void*))
{
	using namespace EA::Thread;

	// The parent thread is sharing memory with us and we need to
	// make sure our view of it is synchronized with the parent.
	EAReadWriteBarrier();

	// Check there is an entry for the current thread context in our ThreadDynamicData array.
	EA::Thread::ThreadId thisThreadId = EA::Thread::GetThreadId();
	if(!FindThreadDynamicData(thisThreadId))
	{
		EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData;
		if(pData)
		{
			pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
							 // Do no AddRef for thread execution because this is not an EAThread managed thread.
			pData->AddRef(); // AddRef for this function, to be released upon this function's exit.                
			pData->mThreadId = thisThreadId;
			pData->mSysThreadId = GetSysThreadId();
			strncpy(pData->mName, "external", EATHREAD_NAME_SIZE);
			pData->mName[EATHREAD_NAME_SIZE - 1] = 0;
			pData->mpStackBase = EA::Thread::GetStackBase();
		}
	}
	
	if(mThreadData.mpData)
		mThreadData.mpData->Release(); // Matches the "AddRef for ourselves" below.

	// We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
	// modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
	// during execution.
	EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData; // Note that we use a special new here which doesn't use the heap.
	EAT_ASSERT(pData);

	if(pData)
	{
		mThreadData.mpData = pData;

		pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
		pData->AddRef(); // AddRef for the thread, to be released upon the thread exiting.
		pData->AddRef(); // AddRef for this function, to be released upon this function's exit.
		pData->mThreadId = kThreadIdInvalid;
		pData->mSysThreadId = kSysThreadIdInvalid;
		pData->mThreadPid = 0;
		pData->mnStatus = Thread::kStatusNone;
		pData->mpStartContext[0] = pRunnableOrFunction;
		pData->mpStartContext[1] = pContext;
		pData->mpBeginThreadUserWrapper = pUserWrapper;
		pData->mStartupProcessor = pTP ? pTP->mnProcessor % EA::Thread::GetProcessorCount() : kProcessorDefault;
		pData->mnThreadAffinityMask = pTP ? pTP->mnAffinityMask : kThreadAffinityMaskAny;
		strncpy(pData->mName, (pTP && pTP->mpName) ? pTP->mpName : "", EATHREAD_NAME_SIZE);
		pData->mName[EATHREAD_NAME_SIZE - 1] = 0;

		// Pass NULL attribute pointer if there are no special setup steps
		ScePthreadAttr* pCreationAttribs = NULL;
		int result(0);

		ScePthreadAttr creationAttribs;

		scePthreadAttrInit(&creationAttribs);

		// Sony has stated that we should call scePthreadAttrSetinheritsched, otherwise the 
		// thread priority set up in pthread_attr_t gets ignored by the newly created thread.
		scePthreadAttrSetinheritsched(&creationAttribs, SCE_PTHREAD_EXPLICIT_SCHED);

		if(pData->mStartupProcessor == EA::Thread::kProcessorAny)
		{
			if(pData->mnThreadAffinityMask == kThreadAffinityMaskAny)
			// Unless you specifically set the thread affinity to SCE_KERNEL_CPUMASK_USER_ALL,
			// Sony apparently assigns your thread to a single CPU.
				scePthreadAttrSetaffinity(&creationAttribs, SCE_KERNEL_CPUMASK_USER_ALL);
			else
				scePthreadAttrSetaffinity(&creationAttribs, pData->mnThreadAffinityMask);
		}
		else if(pData->mStartupProcessor != kProcessorDefault)
		{
			SceKernelCpumask mask = (1 << pData->mStartupProcessor) & 0xFF;
			scePthreadAttrSetaffinity(&creationAttribs, mask);
		}

		SetupThreadAttributes(creationAttribs, pTP);
		pCreationAttribs = &creationAttribs;
		
		result = scePthreadCreate(&pData->mSysThreadId, pCreationAttribs, InternalThreadFunction, pData, mThreadData.mpData->mName);
		
		if(result == 0) // If success...
		{
			// NOTE:  This cast must match the caset that is done in EA::Thread::GetThreadId.
			pData->mThreadId = *reinterpret_cast<EA::Thread::ThreadId*>(pData->mSysThreadId);

			ThreadId threadIdTemp = pData->mThreadId; // Temp value because Release below might delete pData.

			// If additional attributes were used, free initialization data.
			if(pCreationAttribs)
			{
				result = scePthreadAttrDestroy(pCreationAttribs);
				EAT_ASSERT(result == 0);
			}

			pData->Release(); // Matches AddRef for this function.
			return threadIdTemp;
		}

		// If additional attributes were used, free initialization data
		if(pCreationAttribs)
		{
			result = scePthreadAttrDestroy(pCreationAttribs);
			EAT_ASSERT(result == 0);
		}

		pData->Release(); // Matches AddRef for "cleanup" above.
		pData->Release(); // Matches AddRef for this Thread class above.
		pData->Release(); // Matches AddRef for thread above.
		mThreadData.mpData = NULL; // mThreadData.mpData == pData
	}

	return (ThreadId)kThreadIdInvalid;
}


EA::Thread::ThreadId EA::Thread::Thread::Begin(RunnableFunction pFunction, void* pContext,
											   const ThreadParameters* pTP, RunnableFunctionUserWrapper pUserWrapper)
{
	ThreadId threadId = BeginThreadInternal(mThreadData, reinterpret_cast<void*>((uintptr_t)pFunction), pContext, pTP,
											reinterpret_cast<void*>((uintptr_t)pUserWrapper), RunnableFunctionInternal);
	return threadId;
}


EA::Thread::ThreadId EA::Thread::Thread::Begin(IRunnable* pRunnable, void* pContext,
											   const ThreadParameters* pTP, RunnableClassUserWrapper pUserWrapper)
{
	ThreadId threadId = BeginThreadInternal(mThreadData, reinterpret_cast<void*>((uintptr_t)pRunnable), pContext, pTP,
											reinterpret_cast<void*>((uintptr_t)pUserWrapper), RunnableObjectInternal);
	return threadId;
}


EA::Thread::Thread::Status EA::Thread::Thread::WaitForEnd(const ThreadTime& timeoutAbsolute, intptr_t* pThreadReturnValue)
{
	// In order to support timeoutAbsolute, we don't just call pthread_disabled_join, as that's an infinitely blocking call.
	// Instead we wait on a Mutex (with a timeout) which the running thread locked, and will unlock as it is exiting.
	// Only after the successful Mutex lock do we call pthread_disabled_join, as we know that it won't block for an indeterminate
	// amount of time (barring a thread priority inversion problem). If the user never calls WaitForEnd, then we 
	// will eventually call pthread_disabled_detach in the EAThreadDynamicData destructor.

	// The mThreadData memory is shared between threads and when 
	// reading it we must be synchronized.
	EAReadWriteBarrier();

	// A mutex lock around mpData is not needed below because mpData is never allowed to go from non-NULL to NULL. 
	// However, there is an argument that can be made for placing a memory read barrier before reading it.

	if(mThreadData.mpData) // If this is non-zero then we must have created the thread.
	{
		// We must not call WaitForEnd from the thread we are waiting to end. 
		// That would result in a deadlock, at least if the timeout was infinite.
		EAT_ASSERT(mThreadData.mpData->mThreadId != EA::Thread::GetThreadId());

		Status currentStatus = GetStatus();

		if(currentStatus == kStatusNone) // If the thread hasn't started yet...
		{
			// The thread has not been started yet. Wait on the semaphore (which is posted when the thread actually starts executing).
			Semaphore::Result result = (Semaphore::Result)mThreadData.mpData->mStartedSemaphore.Wait(timeoutAbsolute);
			EAT_ASSERT(result != Semaphore::kResultError);

			if(result >= 0) // If the Wait succeeded, as opposed to timing out...
			{
				// We know for sure that the thread status is running now.
				currentStatus = kStatusRunning;
				mThreadData.mpData->mStartedSemaphore.Post(); // Re-post the semaphore so that any other callers of WaitForEnd don't block on the Wait above.
			}
		} // fall through.

		if(currentStatus == kStatusRunning) // If the thread has started but not yet exited...
		{
			// Lock on the mutex (which is available when the thread is exiting)
			Mutex::Result result = (Mutex::Result)mThreadData.mpData->mRunMutex.Lock(timeoutAbsolute);
			EAT_ASSERT(result != Mutex::kResultError);

			if(result > 0) // If the Lock succeeded, as opposed to timing out... then the thread has exited or is in the process of exiting.
			{
				// Do a pthread_disabled join. This is a blocking call, but we know that it will end very soon, 
				// as the mutex unlock the thread did is done right before the thread returns to the OS.
				// The return value of pthread_disabled_join has information that isn't currently useful to us.
				scePthreadJoin(mThreadData.mpData->mSysThreadId, NULL);
				mThreadData.mpData->mThreadId = kThreadIdInvalid;

				// We know for sure that the thread status is ended now.
				currentStatus = kStatusEnded;
				mThreadData.mpData->mRunMutex.Unlock();
			}
			// Else the Lock timed out, which means that the thread didn't exit before we ran out of time.
			// In this case we need to return to the user that the status is kStatusRunning.
		}
		else
		{
			// Else currentStatus == kStatusEnded.
			scePthreadJoin(mThreadData.mpData->mSysThreadId, NULL);
			mThreadData.mpData->mThreadId = kThreadIdInvalid;
		}

		if(currentStatus == kStatusEnded)
		{
			// Call GetStatus again to get the thread return value.
			currentStatus = GetStatus(pThreadReturnValue);
		}

		return currentStatus;
	}
	else
	{
		// Else the user hasn't started the thread yet, so we wait until the user starts it.
		// Ideally, what we really want to do here is wait for some kind of signal. 
		// Instead for the time being we do a polling loop. 
		while((!mThreadData.mpData || (mThreadData.mpData->mThreadId == kThreadIdInvalid)) && (GetThreadTime() < timeoutAbsolute))
		{
			ThreadSleep(1);
			EAReadWriteBarrier();
			EACompilerMemoryBarrier();
		}

		if(mThreadData.mpData)
			return WaitForEnd(timeoutAbsolute);
	}

	return kStatusNone; 
}


EA::Thread::Thread::Status EA::Thread::Thread::GetStatus(intptr_t* pThreadReturnValue) const
{
	if(mThreadData.mpData)
	{
		EAReadBarrier();
		Status status = (Status)mThreadData.mpData->mnStatus;

		if(pThreadReturnValue && (status == kStatusEnded))
			*pThreadReturnValue = mThreadData.mpData->mnReturnValue;

		return status;
	}

	return kStatusNone;
}


EA::Thread::ThreadId EA::Thread::Thread::GetId() const
{
	// A mutex lock around mpData is not needed below because 
	// mpData is never allowed to go from non-NULL to NULL. 
	if(mThreadData.mpData)
		return mThreadData.mpData->mThreadId;

	return kThreadIdInvalid;
}


int EA::Thread::Thread::GetPriority() const
{
	// A mutex lock around mpData is not needed below because 
	// mpData is never allowed to go from non-NULL to NULL. 
	if(mThreadData.mpData)
	{
		int         policy;
		sched_param param;

		int result = scePthreadGetschedparam(mThreadData.mpData->mSysThreadId, &policy, &param);

		if(result == 0)
			return ConvertFromNativePriority(param, policy);

		return kThreadPriorityDefault;
	}

	return kThreadPriorityUnknown;
}


bool EA::Thread::Thread::SetPriority(int nPriority)
{
	// A mutex lock around mpData is not needed below because 
	// mpData is never allowed to go from non-NULL to NULL. 
	EAT_ASSERT(nPriority != kThreadPriorityUnknown);

	if(mThreadData.mpData)
	{
		int         policy;
		sched_param param;

		int result = scePthreadGetschedparam(mThreadData.mpData->mSysThreadId, &policy, &param);

		if(result == 0) // If success...
		{
			ConvertToNativePriority(nPriority, param, policy);

			result = scePthreadSetschedparam(mThreadData.mpData->mSysThreadId, policy, &param);
		}

		return (result == 0);
	}

	return false;
}


// To consider: Make it so we return a value.
void EA::Thread::Thread::SetProcessor(int nProcessor)
{
	if(mThreadData.mpData)
	{
		mThreadData.mpData->mStartupProcessor = nProcessor; // Assign this in case the thread hasn't started yet and thus we are leaving it a message to set it when it has started.
		SetPlatformThreadAffinity(mThreadData.mpData);
	}
}


void EA::Thread::Thread::Wake()
{
	// Todo: implement this. The solution is to use a signal to wake the sleeping thread via an EINTR.
	// Possibly use the SIGCONT signal. Have to look into this to tell what the best approach is.
}


const char* EA::Thread::Thread::GetName() const 
{ 
	return mThreadData.mpData ? mThreadData.mpData->mName : ""; 
}


void EA::Thread::Thread::SetName(const char* pName)
{
	if(mThreadData.mpData && pName)
		SetThreadName(mThreadData.mpData->mThreadId, pName);
}


