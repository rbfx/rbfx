///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/internal/config.h>
#include "eathread/internal/eathread_global.h"
#include <eathread/eathread_thread.h>
#include <eathread/eathread_mutex.h>
#include <new> // include new for placement new operator

#if !EA_THREADS_AVAILABLE
	// Do nothing
#elif EA_USE_CPP11_CONCURRENCY
	#include "cpp11/eathread_thread_cpp11.cpp"
#elif defined(EA_PLATFORM_SONY)
	#include "kettle/eathread_thread_kettle.cpp"
#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include "unix/eathread_thread_unix.cpp"
#elif defined(EA_PLATFORM_MICROSOFT)
	#include "pc/eathread_thread_pc.cpp"
#endif



namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::Thread* EA::Thread::ThreadFactory::CreateThread()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::Thread))) EA::Thread::Thread;
	else
		return new EA::Thread::Thread;
}

void EA::Thread::ThreadFactory::DestroyThread(EA::Thread::Thread* pThread)
{
	if(gpAllocator)
	{
		pThread->~Thread();
		gpAllocator->Free(pThread);
	}
	else
		delete pThread;
}

size_t EA::Thread::ThreadFactory::GetThreadSize()
{
	return sizeof(EA::Thread::Thread);
}

EA::Thread::Thread* EA::Thread::ThreadFactory::ConstructThread(void* pMemory)
{
	return new(pMemory) EA::Thread::Thread;
}

void EA::Thread::ThreadFactory::DestructThread(EA::Thread::Thread* pThread)
{
	pThread->~Thread();
}

EA::Thread::ThreadEnumData::ThreadEnumData()
: mpThreadDynamicData(NULL)
{
}

EA::Thread::ThreadEnumData::~ThreadEnumData()
{
	Release();
}

void EA::Thread::ThreadEnumData::Release()
{
	if(mpThreadDynamicData)
	{
		mpThreadDynamicData->Release();
		mpThreadDynamicData = NULL;
	}
}

extern const size_t kMaxThreadDynamicDataCount;
EATHREAD_GLOBALVARS_EXTERN_INSTANCE;
///////////////////////////////////////////////////////////////////////////////
//
size_t EA::Thread::EnumerateThreads(ThreadEnumData* pDataArray, size_t dataArrayCapacity)
{
	size_t requiredCount = 0;

	if(dataArrayCapacity > EA::Thread::kMaxThreadDynamicDataCount)
	   dataArrayCapacity = EA::Thread::kMaxThreadDynamicDataCount;  
	
	EATHREAD_GLOBALVARS.gThreadDynamicMutex.Lock();
	for(size_t i(0); i < EA::Thread::kMaxThreadDynamicDataCount; i++)
	{
		if(EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[i].GetValue() != 0)
		{
			if(requiredCount < dataArrayCapacity)
			{
				pDataArray[requiredCount].mpThreadDynamicData = (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
				pDataArray[requiredCount].mpThreadDynamicData->AddRef(); 
			}
			requiredCount++;
		}
	} 
	EATHREAD_GLOBALVARS.gThreadDynamicMutex.Unlock();

	return requiredCount;
}

///////////////////////////////////////////////////////////////////////////////
// non-threaded implementation
///////////////////////////////////////////////////////////////////////////////

#if !EA_THREADS_AVAILABLE

	// If mulitithreading support is not available, we can't implement anything
	// here that works. All we do is define a null implementation that links
	// but fails all operations.


	EA::Thread::ThreadParameters::ThreadParameters()
	  : mpStack(NULL),
		mnStackSize(0),
		mnPriority(kThreadPriorityDefault),
		mnProcessor(kProcessorDefault),
		mpName(""),
		mbDisablePriorityBoost(false)
	{
	}


	EA::Thread::Thread::Thread()
	{
		mThreadData.mpData = NULL;
	}


	EA::Thread::Thread::Thread(const Thread& /*t*/)
	{
	}


	EA::Thread::Thread& EA::Thread::Thread::operator=(const Thread& /*t*/)
	{
		return *this;
	}


	EA::Thread::Thread::~Thread()
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
		if (sGlobalRunnableFunctionUserWrapper != NULL)
		{
			// Can only be set once in entire game. 
			EAT_ASSERT(false);
		}
		else
			sGlobalRunnableFunctionUserWrapper = pUserWrapper;
	}

	EA::Thread::RunnableClassUserWrapper EA::Thread::Thread::GetGlobalRunnableClassUserWrapper()
	{
		return sGlobalRunnableClassUserWrapper;
	}

	void EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(EA::Thread::RunnableClassUserWrapper pUserWrapper)
	{
		if (sGlobalRunnableClassUserWrapper != NULL)
		{
			// Can only be set once in entire game. 
			EAT_ASSERT(false);
		}
		else
			sGlobalRunnableClassUserWrapper = pUserWrapper;
	}


	EA::Thread::ThreadId EA::Thread::Thread::Begin(RunnableFunction /*pFunction*/, void* /*pContext*/, const ThreadParameters* /*pTP*/, RunnableFunctionUserWrapper /*pUserWrapper*/)
	{
		return kThreadIdInvalid;
	}


	EA::Thread::ThreadId EA::Thread::Thread::Begin(IRunnable* /*pRunnable*/, void* /*pContext*/, const ThreadParameters* /*pTP*/, RunnableClassUserWrapper /*pUserWrapper*/)
	{
		return kThreadIdInvalid;
	}


	EA::Thread::Thread::Status EA::Thread::Thread::WaitForEnd(const ThreadTime& /*timeoutAbsolute*/, intptr_t* /*pThreadReturnValue*/)
	{
		return kStatusNone; 
	}


	EA::Thread::Thread::Status EA::Thread::Thread::GetStatus(intptr_t* /*pThreadReturnValue*/) const
	{
		return kStatusNone;
	}


	EA::Thread::ThreadId EA::Thread::Thread::GetId() const
	{
		return (ThreadId)kThreadIdInvalid;
	}


	int EA::Thread::Thread::GetPriority() const
	{
		return kThreadPriorityUnknown;
	}


	bool EA::Thread::Thread::SetPriority(int /*nPriority*/)
	{
		return false;
	}


	void EA::Thread::Thread::SetProcessor(int /*nProcessor*/)
	{
	}


	void EA::Thread::Thread::Wake()
	{
	}


	const char* EA::Thread::Thread::GetName() const
	{
		return "";
	}

 
	void EA::Thread::Thread::SetName(const char* /*pName*/)
	{
	}

#endif // !EA_THREADS_AVAILABLE

