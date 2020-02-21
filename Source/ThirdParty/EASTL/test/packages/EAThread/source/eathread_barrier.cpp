///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/internal/config.h>

EA_DISABLE_VC_WARNING(4574)
#include <new>
EA_RESTORE_VC_WARNING()

#if defined(EA_PLATFORM_SONY)
	#include "kettle/eathread_barrier_kettle.cpp"

#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EA_THREADS_AVAILABLE
	// Posix already defines a barrier (via condition variables or directly with pthread_barrier).
	#include "unix/eathread_barrier_unix.cpp"

#else // All other platforms

	#include <eathread/eathread_barrier.h>
	#include <string.h>


	EABarrierData::EABarrierData() 
		: mnCurrent(0), mnHeight(0), mnIndex(0), mSemaphore0(NULL, false), mSemaphore1(NULL, false) 
	{
		// Leave mSemaphores alone for now. We leave them constructed but not initialized.
	}


	EA::Thread::BarrierParameters::BarrierParameters(int height, bool bIntraProcess, const char* pName)
		: mHeight(height), mbIntraProcess(bIntraProcess)
	{
		if(pName)
		{
			EA_DISABLE_VC_WARNING(4996); // This function or variable may be unsafe / deprecated. 
			strncpy(mName, pName, sizeof(mName)-1);
			EA_RESTORE_VC_WARNING();
			mName[sizeof(mName)-1] = 0;
		}
		else
			mName[0] = 0;
	}


	EA::Thread::Barrier::Barrier(const BarrierParameters* pBarrierParameters, bool bDefaultParameters)
	{
		if(!pBarrierParameters && bDefaultParameters)
		{
			BarrierParameters parameters;
			Init(&parameters);
		}
		else
			Init(pBarrierParameters);
	}


	EA::Thread::Barrier::Barrier(int height)
	{
		BarrierParameters parameters(height);
		Init(&parameters);
	}


	EA::Thread::Barrier::~Barrier()
	{
		// Nothing to do.
	}


	bool EA::Thread::Barrier::Init(const BarrierParameters* pBarrierParameters)
	{
		// You cannot set the height after it's already been set.
		EAT_ASSERT((mBarrierData.mnHeight == 0) && (mBarrierData.mnCurrent == 0));

		if(pBarrierParameters && (mBarrierData.mnHeight == 0))
		{
			mBarrierData.mnHeight  = pBarrierParameters->mHeight; // We don't put mutex lock around this as it is only to be ever set once, before use.
			mBarrierData.mnCurrent = pBarrierParameters->mHeight;

			SemaphoreParameters sp(0, pBarrierParameters->mbIntraProcess);
			mBarrierData.mSemaphore0.Init(&sp);
			mBarrierData.mSemaphore1.Init(&sp);

			return true;
		}

		return false;
	}


	EA::Thread::Barrier::Result EA::Thread::Barrier::Wait(const ThreadTime& timeoutAbsolute)
	{
		int result;
		const int nCurrentIndex = (int)mBarrierData.mnIndex;

		// Question: What do we do if a fifth thread calls Wait on a barrier with height 
		// of four after the fourth thread has decremented the current count below?

		EAT_ASSERT(mBarrierData.mnCurrent > 0); // If this assert fails then it means that more threads are waiting on the barrier than the barrier height.

		const int32_t nCurrent = mBarrierData.mnCurrent.Decrement(); // atomic integer operation.

		if(nCurrent == 0) // If the barrier has been breached... 
		{
			mBarrierData.mnCurrent = mBarrierData.mnHeight;

			if(mBarrierData.mnHeight > 1) // If there are threads other than us...
			{
				// We don't have a potential race condition here because we use alternating
				// semaphores and since we are here, all other threads are waiting on the 
				// current semaphore below. And if they haven't started waiting on the 
				// semaphore yet, they'll succeed anyway because we Post all directly below.
				Semaphore* const pSemaphore = (nCurrentIndex == 0 ? &mBarrierData.mSemaphore0 : &mBarrierData.mSemaphore1);

				result = pSemaphore->Post(mBarrierData.mnHeight - 1); // Upon success, the return value will in practice be >= 1, but semaphore defines success as >= 0.
			}
			else // Else we are the only thead.
				result = 0;
		}
		else
		{
			Semaphore* const pSemaphore = (nCurrentIndex == 0 ? &mBarrierData.mSemaphore0 : &mBarrierData.mSemaphore1);

			result = pSemaphore->Wait(timeoutAbsolute);

			if(result == Semaphore::kResultTimeout)
				return kResultTimeout;
		}

		if(result >= 0) // If the result wasn't an error such as Semaphore::kResultError or Semaphore::kResultTimeout.
		{
			// Use an atomic operation to change the index, which conveniently gives us a thread to designate as primary.
			EAT_ASSERT((unsigned)nCurrentIndex <= 1);

			if(mBarrierData.mnIndex.SetValueConditional(1 - nCurrentIndex, nCurrentIndex))  // Toggle value between 0 and 1.
				return kResultPrimary;

			return kResultSecondary;
		}

		return kResultError;
	}


	EA::Thread::Barrier* EA::Thread::BarrierFactory::CreateBarrier()
	{
		EA::Thread::Allocator* pAllocator = EA::Thread::GetAllocator();

		if(pAllocator)
			return new(pAllocator->Alloc(sizeof(EA::Thread::Barrier))) EA::Thread::Barrier;
		else
			return new EA::Thread::Barrier;
	}

	void EA::Thread::BarrierFactory::DestroyBarrier(EA::Thread::Barrier* pBarrier)
	{
		EA::Thread::Allocator* pAllocator = EA::Thread::GetAllocator();

		if(pAllocator)
		{
			pBarrier->~Barrier();
			pAllocator->Free(pBarrier);
		}
		else
			delete pBarrier;
	}

	size_t EA::Thread::BarrierFactory::GetBarrierSize()
	{
		return sizeof(EA::Thread::Barrier);
	}

	EA::Thread::Barrier* EA::Thread::BarrierFactory::ConstructBarrier(void* pMemory)
	{
		return new(pMemory) EA::Thread::Barrier;
	}

	void EA::Thread::BarrierFactory::DestructBarrier(EA::Thread::Barrier* pBarrier)
	{
		pBarrier->~Barrier();
	}


#endif // EA_PLATFORM_XXX








