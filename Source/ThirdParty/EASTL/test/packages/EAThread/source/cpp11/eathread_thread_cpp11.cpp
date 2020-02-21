///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "eathread/eathread_thread.h"
#include "eathread/eathread.h"
#include "eathread/eathread_sync.h"
#include "eathread/eathread_callstack.h"
#include "eathread/internal/eathread_global.h"

namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;

		static AtomicInt32 nLastProcessor = 0;
		const size_t kMaxThreadDynamicDataCount = 128;

		struct EAThreadGlobalVars
		{
			char gThreadDynamicData[kMaxThreadDynamicDataCount][sizeof(EAThreadDynamicData)];
			AtomicInt32 gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];
			Mutex gThreadDynamicMutex;
		};
		EATHREAD_GLOBALVARS_CREATE_INSTANCE;

		EAThreadDynamicData* AllocateThreadDynamicData()
		{
			for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
			{
				if (EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[i].SetValueConditional(1, 0))
					return (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
			}

			// This is a safety fallback mechanism. In practice it won't be used in almost all situations.
			if (gpAllocator)
				return (EAThreadDynamicData*)gpAllocator->Alloc(sizeof(EAThreadDynamicData));

			return nullptr;
		}

		void FreeThreadDynamicData(EAThreadDynamicData* pEAThreadDynamicData)
		{
			pEAThreadDynamicData->~EAThreadDynamicData();
			if ((pEAThreadDynamicData >= (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData) && (pEAThreadDynamicData < ((EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData + kMaxThreadDynamicDataCount)))
			{
				EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[pEAThreadDynamicData - (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData].SetValue(0);
			}
			else
			{
				// Assume the data was allocated via the fallback mechanism.
				if (gpAllocator)
				{
					gpAllocator->Free(pEAThreadDynamicData);
				}
			}
		}

		EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId)
		{
			for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
			{
				EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
				if (pTDD->mpComp && pTDD->mpComp->mThread.get_id() == threadId)
					return pTDD;
			}
			return nullptr; // This is no practical way we can find the data unless thread-specific storage was involved.
		}

		EAThreadDynamicData* FindThreadDynamicData(EA::Thread::ThreadUniqueId threadId)
		{
			for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
			{
				EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
				if (pTDD->mUniqueThreadId == threadId)
					return pTDD;
			}
			return nullptr; // This is no practical way we can find the data unless thread-specific storage was involved.
		}

		EAThreadDynamicData* FindThreadDynamicData(EA::Thread::SysThreadId sysThreadId)
		{
			for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
			{
				EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
				if (pTDD->mpComp && pTDD->mpComp->mThread.native_handle() == sysThreadId)
					return pTDD;
			}

			// NOTE:  This function does not support finding externally created threads due to limitations in the CPP11 std::thread API.
			//        At the time of writing, it is not possible to retrieve the thread object of a thread not created by the CPP11 API.
			return nullptr; // This is no practical way we can find the data unless thread-specific storage was involved.
		}
	}
}

EA_DISABLE_VC_WARNING(4355) // this used in base member initializer list - should be safe in this context
EAThreadDynamicData::EAThreadDynamicData(void* userFunc, void* userContext, void* userWrapperFunc, ThreadFunc threadFunc) : 
	mnRefCount(2), // Init ref count to 2, one corresponding release happens on threadFunc exit and the other when Thread class is destroyed or Begin is called again
	mStatus(EA::Thread::Thread::kStatusNone),
	mpComp(nullptr)
{
	mpComp = new EAThreadComposite();

	if(mpComp)
		mpComp->mThread = std::thread(threadFunc, this, userFunc, userContext, userWrapperFunc);  // This doesn't spawn CPP11 threads when created within the EAThreadComposite constructor.
}


EAThreadDynamicData::EAThreadDynamicData(EA::Thread::ThreadUniqueId uniqueThreadId, const char* pThreadName) : 
	mnRefCount(2), // Init ref count to 2, one corresponding release happens on threadFunc exit and the other when Thread class is destroyed or Begin is called again
	mStatus(EA::Thread::Thread::kStatusNone),
	mpComp(nullptr),
	mUniqueThreadId(uniqueThreadId)
{
	strncpy(mName, pThreadName, EATHREAD_NAME_SIZE);
	mName[EATHREAD_NAME_SIZE - 1] = 0;
}

EA_RESTORE_VC_WARNING()


EAThreadDynamicData::~EAThreadDynamicData()
{
	if (mpComp->mThread.joinable())
		mpComp->mThread.detach();

	if(mpComp)
		delete mpComp;

	mpComp = nullptr;

	// the threads, promises, and futures in this class will 
	// allocate memory with the Concurrency runtime new/delete operators.
	// If you're crashing in here with access violations on process exit,
	// then you likely have a static instance of EA::Thread::Thread somewhere
	// that's being destructed after your memory system is uninitialized
	// leaving dangling pointers to bad memory.  Attempt to change
	// these static instances to be constructed/destructed with the scope
	// of normal app operation.
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

namespace EA
{
	namespace Thread
	{
		ThreadParameters::ThreadParameters() : 
			mpStack(NULL), 
			mnStackSize(0), 
			mnPriority(kThreadPriorityDefault), 
			mnProcessor(kProcessorDefault), 
			mpName(""), 
			mbDisablePriorityBoost(false)
		{
		}

		RunnableFunctionUserWrapper  Thread::sGlobalRunnableFunctionUserWrapper = NULL;
		RunnableClassUserWrapper     Thread::sGlobalRunnableClassUserWrapper    = NULL;
		AtomicInt32      			 Thread::sDefaultProcessor                  = kProcessorAny;

		RunnableFunctionUserWrapper Thread::GetGlobalRunnableFunctionUserWrapper()
		{
			return sGlobalRunnableFunctionUserWrapper;
		}

		void Thread::SetGlobalRunnableFunctionUserWrapper(RunnableFunctionUserWrapper pUserWrapper)
		{
			if (sGlobalRunnableFunctionUserWrapper != NULL)
			{
				// Can only be set once in entire game. 
				EAT_ASSERT(false);
			}
			else
			{
				sGlobalRunnableFunctionUserWrapper = pUserWrapper;
			}
		}

		RunnableClassUserWrapper Thread::GetGlobalRunnableClassUserWrapper()
		{
			return sGlobalRunnableClassUserWrapper;
		}

		void Thread::SetGlobalRunnableClassUserWrapper(RunnableClassUserWrapper pUserWrapper)
		{
			if (sGlobalRunnableClassUserWrapper != NULL)
			{
				// Can only be set once in entire game. 
				EAT_ASSERT(false);
			}
			else
			{
				sGlobalRunnableClassUserWrapper = pUserWrapper;
			}
		}

		Thread::Thread()
		{
			mThreadData.mpData = NULL;
		}


		Thread::Thread(const Thread& t) : 
			mThreadData(t.mThreadData)
		{
			if (mThreadData.mpData)
				mThreadData.mpData->AddRef();
		}


		Thread& Thread::operator=(const Thread& t)
		{
			// We don't synchronize access to mpData; we assume that the user 
			// synchronizes it or this Thread instances is used from a single thread.
			if (t.mThreadData.mpData)
				t.mThreadData.mpData->AddRef();

			if (mThreadData.mpData)
				mThreadData.mpData->Release();

			mThreadData = t.mThreadData;

			return *this;
		}


		Thread::~Thread()
		{
			// We don't synchronize access to mpData; we assume that the user 
			// synchronizes it or this Thread instances is used from a single thread.
			if (mThreadData.mpData)
				mThreadData.mpData->Release();
		}

		static void RunnableFunctionInternal(EAThreadDynamicData* tdd, void* userFunc, void* userContext, void* userWrapperFunc)
		{
			tdd->mStatus = Thread::kStatusRunning;
			tdd->mpStackBase = EA::Thread::GetStackBase();
			RunnableFunction pFunction = (RunnableFunction)userFunc;

			if (userWrapperFunc)
			{
				RunnableFunctionUserWrapper pWrapperFunction = (RunnableFunctionUserWrapper)userWrapperFunc;
				// if user wrapper is specified, call user wrapper and pass down the pFunction and pContext
				tdd->mpComp->mReturnPromise.set_value(pWrapperFunction(pFunction, userContext));
			}
			else
			{
				tdd->mpComp->mReturnPromise.set_value(pFunction(userContext));
			}

			tdd->mStatus = Thread::kStatusEnded;
			tdd->Release(); // Matches an implicit AddRef in EAThreadDynamicData constructor
		}


		ThreadId Thread::Begin(RunnableFunction pFunction, void* pContext, const ThreadParameters* pTP, RunnableFunctionUserWrapper pUserWrapper)
		{
			// Check there is an entry for the current thread context in our ThreadDynamicData array.            
			ThreadUniqueId threadUniqueId;
			EAThreadGetUniqueId(threadUniqueId);
			if(!FindThreadDynamicData(threadUniqueId))
			{
				EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData(threadUniqueId, "external");
				if(pData)
				{
					pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
									 // Do no AddRef for thread execution because this is not an EAThread managed thread.
				}
			}

			if (mThreadData.mpData)
				mThreadData.mpData->Release(); // Matches an implicit AddRef in EAThreadDynamicData constructor

			// C++11 Threads don't support user-supplied stacks. A user-supplied stack pointer 
			// here would be a waste of user memory, and so we assert that mpStack == NULL.
			EAT_ASSERT(!pTP || (pTP->mpStack == NULL));

			// We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
			// modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
			// during execution.
			EAThreadDynamicData* pDataAddr = AllocateThreadDynamicData();
			EAT_ASSERT(pDataAddr != nullptr);
			EAThreadDynamicData* pData = new(pDataAddr) EAThreadDynamicData(pFunction, pContext, pUserWrapper, RunnableFunctionInternal); // Note that we use a special new here which doesn't use the heap.
			EAT_ASSERT(pData != nullptr);
			mThreadData.mpData = pData;
			if (pTP)
				SetName(pTP->mpName);

			return pData->mpComp->mThread.get_id();
		}

		static void RunnableObjectInternal(EAThreadDynamicData* tdd, void* userFunc, void* userContext, void* userWrapperFunc)
		{
			tdd->mStatus = Thread::kStatusRunning;
			IRunnable* pRunnable = (IRunnable*)userFunc;

			if (userWrapperFunc)
			{
				RunnableClassUserWrapper pWrapperFunction = (RunnableClassUserWrapper)userWrapperFunc;
				// if user wrapper is specified, call user wrapper and pass down the pFunction and pContext
				tdd->mpComp->mReturnPromise.set_value(pWrapperFunction(pRunnable, userContext));
			}
			else
			{
				tdd->mpComp->mReturnPromise.set_value(pRunnable->Run(userContext));
			}

			tdd->mStatus = Thread::kStatusEnded;
			tdd->Release(); // Matches implicit AddRef in EAThreadDynamicData constructor
		}


		ThreadId Thread::Begin(IRunnable* pRunnable, void* pContext, const ThreadParameters* pTP, RunnableClassUserWrapper pUserWrapper)
		{
			if (mThreadData.mpData)
				mThreadData.mpData->Release(); // Matches an implicit AddRef in EAThreadDynamicData constructor

			// C++11 Threads don't support user-supplied stacks. A user-supplied stack pointer 
			// here would be a waste of user memory, and so we assert that mpStack == NULL.
			EAT_ASSERT(!pTP || (pTP->mpStack == NULL));

			// We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
			// modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
			// during execution.
			EAThreadDynamicData* pDataAddr = AllocateThreadDynamicData();
			EAT_ASSERT(pDataAddr != nullptr);
			EAThreadDynamicData* pData = new(pDataAddr) EAThreadDynamicData(pRunnable, pContext, pUserWrapper, RunnableObjectInternal); // Note that we use a special new here which doesn't use the heap.
			EAT_ASSERT(pData != nullptr);
			mThreadData.mpData = pData;
			if (pTP)
				SetName(pTP->mpName);

			EAT_ASSERT(pData && pData->mpComp);
			return pData->mpComp->mThread.get_id();
		}

		Thread::Status Thread::WaitForEnd(const ThreadTime& timeoutAbsolute, intptr_t* pThreadReturnValue)
		{
			// The mThreadData memory is shared between threads and when 
			// reading it we must be synchronized.
			EAReadWriteBarrier();

			// A mutex lock around mpData is not needed below because 
			// mpData is never allowed to go from non-NULL to NULL. 
			// Todo: Consider that there may be a subtle race condition here if 
			// the user immediately calls WaitForEnd right after calling Begin.
			if (mThreadData.mpData && mThreadData.mpData->mpComp)
			{
				// We must not call WaitForEnd from the thread we are waiting to end. That would result in a deadlock.
				EAT_ASSERT(mThreadData.mpData->mpComp->mThread.get_id() != GetThreadId());

				std::chrono::milliseconds timeoutAbsoluteMs(timeoutAbsolute);
				std::chrono::time_point<std::chrono::system_clock> timeoutTime(timeoutAbsoluteMs);
				if (mThreadData.mpData->mpComp->mReturnFuture.wait_until(timeoutTime) == std::future_status::timeout)
				{
					return kStatusRunning;
				}

				if (pThreadReturnValue)
				{
					mThreadData.mpData->mReturnValue = mThreadData.mpData->mpComp->mReturnFuture.get();
					*pThreadReturnValue = mThreadData.mpData->mReturnValue;
				}

				mThreadData.mpData->mpComp->mThread.join();

				return kStatusEnded; // A thread was created, so it must have ended.
			}
			else
			{
				// Else the user hasn't started the thread yet, so we wait until the user starts it.
				// Ideally, what we really want to do here is wait for some kind of signal. 
				// Instead for the time being we do a polling loop. 
				while ((!mThreadData.mpData) && (GetThreadTime() < timeoutAbsolute))
				{
					ThreadSleep(1);
				}
				if (mThreadData.mpData)
					return WaitForEnd(timeoutAbsolute);
			}
			return kStatusNone; // No thread has been started.
		}

		Thread::Status Thread::GetStatus(intptr_t* pThreadReturnValue) const
		{
			if (mThreadData.mpData && mThreadData.mpData->mpComp)
			{
				auto status = static_cast<Thread::Status>(mThreadData.mpData->mStatus.GetValue());
				if (pThreadReturnValue && status == kStatusEnded)
				{
					if (mThreadData.mpData->mpComp->mGetStatusFuture.valid())
						mThreadData.mpData->mReturnValue = mThreadData.mpData->mpComp->mGetStatusFuture.get();
					*pThreadReturnValue = mThreadData.mpData->mReturnValue;
				}
				return status;
			}
			return kStatusNone;
		}

		int Thread::GetPriority() const
		{
			// No way to query or set thread priority through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs using native_handle()
			return kThreadPriorityDefault;
		}


		bool Thread::SetPriority(int nPriority)
		{
			// No way to query or set thread priority through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs using native_handle()
			return false;
		}


		void Thread::SetProcessor(int nProcessor)
		{
			// No way to query or set thread priority through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs using native_handle()
		}

		void EA::Thread::Thread::SetAffinityMask(EA::Thread::ThreadAffinityMask nAffinityMask)
		{
			if(mThreadData.mpData)
			{
				EA::Thread::SetThreadAffinityMask(nAffinityMask);
			}
		}

		EA::Thread::ThreadAffinityMask EA::Thread::Thread::GetAffinityMask()
		{
			if(mThreadData.mpData)
			{
				return mThreadData.mpData->mnThreadAffinityMask;
			}

			return kThreadAffinityMaskAny;
		}

		void Thread::Wake()
		{
			// No way to wake a thread through standard C++11 thread library.
			// On some platforms this could be implemented through platform specific APIs using native_handle()
		}


		const char* Thread::GetName() const
		{
			if (mThreadData.mpData)
				return mThreadData.mpData->mName;
			return "";
		}


		void Thread::SetName(const char* pName)
		{
			if (mThreadData.mpData && pName)
			{
				strncpy(mThreadData.mpData->mName, pName, EATHREAD_NAME_SIZE);
				mThreadData.mpData->mName[EATHREAD_NAME_SIZE - 1] = 0;
			}
		}

		ThreadId Thread::GetId() const
		{
			if (mThreadData.mpData && mThreadData.mpData->mpComp)
				return mThreadData.mpData->mpComp->mThread.get_id();
			return kThreadIdInvalid;
		}

	}
}

