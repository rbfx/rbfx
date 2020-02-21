///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <eathread/eathread_barrier.h>
#include <eathread/eathread.h>


#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>
	#include <time.h>
	#include <errno.h>
	#include <string.h>
	#include <new>
	#ifdef EA_PLATFORM_WINDOWS
		#pragma warning(push, 0)
		#include <Windows.h> // Presumably we are using pthreads-win32.
		#pragma warning(pop)
	#endif


	EABarrierData::EABarrierData()
		:  mCV(), mMutex(), mnHeight(0), mnCurrent(0), mnCycle(0), mbValid(false)
	{}


	EA::Thread::BarrierParameters::BarrierParameters(int height, bool bIntraProcess, const char* pName)
		: mHeight(height), mbIntraProcess(bIntraProcess)
	{
		if(pName)
			strncpy(mName, pName, sizeof(mName)-1); 
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
		if(mBarrierData.mbValid){
			EAT_ASSERT(mBarrierData.mnCurrent == mBarrierData.mnHeight);
			int result = pthread_mutex_destroy(&mBarrierData.mMutex);
			EA_UNUSED(result);
			EAT_ASSERT(result == 0);
			result = pthread_cond_destroy(&mBarrierData.mCV);
			EAT_ASSERT(result == 0);
			EA_UNUSED( result ); //if compiling without asserts
		}
	}


	bool EA::Thread::Barrier::Init(const BarrierParameters* pBarrierParameters)
	{
		if(pBarrierParameters && !mBarrierData.mbValid){
			mBarrierData.mbValid   = false;
			mBarrierData.mnHeight  = pBarrierParameters->mHeight;
			mBarrierData.mnCurrent = pBarrierParameters->mHeight;
			mBarrierData.mnCycle   = 0;

			int result = pthread_mutex_init(&mBarrierData.mMutex, NULL);
			if(result == 0){
				result = pthread_cond_init(&mBarrierData.mCV, NULL);
				if(result == 0)
					mBarrierData.mbValid = true;
				else
					pthread_mutex_destroy(&mBarrierData.mMutex);
			}
			return mBarrierData.mbValid;
		}
		return false;
	}


	EA::Thread::Barrier::Result EA::Thread::Barrier::Wait(const ThreadTime& timeoutAbsolute)
	{
		if(!mBarrierData.mbValid){
			EAT_ASSERT(false);
			return kResultError;
		}

		int result = pthread_mutex_lock(&mBarrierData.mMutex);
		if(result != 0){
			EAT_ASSERT(false);
			return kResultError;
		}

		const unsigned long nCurrentCycle = (unsigned)mBarrierData.mnCycle;
		bool bPrimary = false;

		if(--mBarrierData.mnCurrent == 0){ // This is not an atomic operation. We are within a mutex lock.
			// The last barrier can never time out, as its action is always immediate.
			mBarrierData.mnCycle++;
			mBarrierData.mnCurrent = mBarrierData.mnHeight;
			result = pthread_cond_broadcast(&mBarrierData.mCV);

			// The last thread into the barrier will return a result of
			// kResultPrimary rather than kResultSecondary.
			if(result == 0)
				bPrimary = true;
			//else leave result as an error value.
		}
		else{
			// timeoutMilliseconds
			// Wait with cancellation disabled, because pthreads barrier_wait
			// should not be a cancellation point.
			#if defined(PTHREAD_CANCEL_DISABLE)
				int cancel;
				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
			#endif

			// Wait until the barrier's cycle changes, which means that 
			// it has been broadcast, and we don't want to wait anymore.
			while(nCurrentCycle == mBarrierData.mnCycle){
				do{
					// Under SMP systems, pthread_cond_wait can return the success value 'spuriously'. 
					// This is by design and we must retest the predicate condition and if it has
					// not true, we must go back to waiting. 
					result = pthread_cond_timedwait(&mBarrierData.mCV, &mBarrierData.mMutex, &timeoutAbsolute);
				} while((result == 0) && (nCurrentCycle == mBarrierData.mnCycle));
				if(result != 0)
					break;
			}

			#if defined(PTHREAD_CANCEL_DISABLE)
				int cancelTemp;
				pthread_setcancelstate(cancel, &cancelTemp);
			#endif
		}

		// We declare a new result2 value because the old one 
		// might have a special value from above in it.
		const int result2 = pthread_mutex_unlock(&mBarrierData.mMutex); (void)result2;
		EAT_ASSERT(result2 == 0);

		if(result == 0)
			return bPrimary ? kResultPrimary : kResultSecondary;
		else if(result == ETIMEDOUT)
			return kResultTimeout;
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


#endif // EA_PLATFORM_XXX


