///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread_barrier.h>
#include <eathread/eathread.h>
#include <kernel.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <new>

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
		int result = scePthreadMutexDestroy(&mBarrierData.mMutex);
		EA_UNUSED(result);
		EAT_ASSERT(result == 0);
		result = scePthreadCondDestroy(&mBarrierData.mCV);
		EAT_ASSERT(result == 0);
		EA_UNUSED( result ); //if compiling without asserts
	}
}


bool EA::Thread::Barrier::Init(const BarrierParameters* pBarrierParameters)
{
	if(pBarrierParameters && !mBarrierData.mbValid)
	{
		mBarrierData.mbValid   = false;
		mBarrierData.mnHeight  = pBarrierParameters->mHeight;
		mBarrierData.mnCurrent = pBarrierParameters->mHeight;
		mBarrierData.mnCycle   = 0;

		int result = scePthreadMutexInit(&mBarrierData.mMutex, NULL, pBarrierParameters->mName);
		if(result == 0){
			result = scePthreadCondInit(&mBarrierData.mCV, NULL, pBarrierParameters->mName);
			if(result == 0)
				mBarrierData.mbValid = true;
			else
				scePthreadMutexDestroy(&mBarrierData.mMutex);
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

	int result = scePthreadMutexLock(&mBarrierData.mMutex);
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
		result = scePthreadCondBroadcast(&mBarrierData.mCV);

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
		#if defined(SCE_PTHREAD_CANCEL_DISABLE)
			int cancel;
			scePthreadSetcancelstate(SCE_PTHREAD_CANCEL_DISABLE, &cancel);
		#endif

		// Wait until the barrier's cycle changes, which means that 
		// it has been broadcast, and we don't want to wait anymore.
		while(nCurrentCycle == mBarrierData.mnCycle){
			do{
				// Under SMP systems, pthread_cond_wait can return the success value 'spuriously'. 
				// This is by design and we must retest the predicate condition and if it has
				// not true, we must go back to waiting. 
				result = scePthreadCondTimedwait(&mBarrierData.mCV, &mBarrierData.mMutex, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));
			} while((result == 0) && (nCurrentCycle == mBarrierData.mnCycle));
			if(result != 0)
				break;
		}

		#if defined(SCE_PTHREAD_CANCEL_DISABLE)
			int cancelTemp;
			scePthreadSetcancelstate(cancel, &cancelTemp);
		#endif
	}

	// We declare a new result2 value because the old one 
	// might have a special value from above in it.
	const int result2 = scePthreadMutexUnlock(&mBarrierData.mMutex); (void)result2;
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



