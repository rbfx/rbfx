///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/internal/config.h>
#include <eathread/eathread_semaphore.h>

EA_DISABLE_VC_WARNING(4574)
#include <string.h>
#include <new>
EA_RESTORE_VC_WARNING()

#if !EA_THREADS_AVAILABLE
	#include <eathread/eathread_semaphore.h>
#elif EATHREAD_USE_SYNTHESIZED_SEMAPHORE
	// Fall through.
#elif 0 //EA_USE_CPP11_CONCURRENCY
	#include "cpp11/eathread_semaphore_cpp11.cpp"
#elif defined(__APPLE__)
	#include "apple/eathread_semaphore_apple.cpp"
#elif defined(EA_PLATFORM_ANDROID)
	#include "android/eathread_semaphore_android.cpp"
#elif defined(EA_PLATFORM_SONY)
	#include "kettle/eathread_semaphore_kettle.cpp"
#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include "unix/eathread_semaphore_unix.cpp"
#elif defined(EA_PLATFORM_MICROSOFT)
	#include "pc/eathread_semaphore_pc.cpp"
#endif


namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::Semaphore* EA::Thread::SemaphoreFactory::CreateSemaphore()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::Semaphore))) EA::Thread::Semaphore;
	else
		return new EA::Thread::Semaphore;
}

void EA::Thread::SemaphoreFactory::DestroySemaphore(EA::Thread::Semaphore* pSemaphore)
{
	if(gpAllocator)
	{
		pSemaphore->~Semaphore();
		gpAllocator->Free(pSemaphore);
	}
	else
		delete pSemaphore;
}

size_t EA::Thread::SemaphoreFactory::GetSemaphoreSize()
{
	return sizeof(EA::Thread::Semaphore);
}

EA::Thread::Semaphore* EA::Thread::SemaphoreFactory::ConstructSemaphore(void* pMemory)
{
	return new(pMemory) EA::Thread::Semaphore;
}

void EA::Thread::SemaphoreFactory::DestructSemaphore(EA::Thread::Semaphore* pSemaphore)
{
	pSemaphore->~Semaphore();
}



#if EATHREAD_USE_SYNTHESIZED_SEMAPHORE

	EASemaphoreData::EASemaphoreData()
	  : mCV(), 
		mMutex(),      
		mnCount(0),
		mnMaxCount(INT_MAX),
		mbValid(false)
	{
		// Empty
	}


	EA::Thread::SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char* pName)
	  : mInitialCount(initialCount),
		mMaxCount(INT_MAX),
		mbIntraProcess(bIntraProcess)
	{
		if(pName)
		{
			strncpy(mName, pName, sizeof(mName)-1);
			mName[sizeof(mName)-1] = 0;
		}
		else
			mName[0] = 0;
	}


	EA::Thread::Semaphore::Semaphore(const SemaphoreParameters* pSemaphoreParameters, bool bDefaultParameters)
	{
		if(!pSemaphoreParameters && bDefaultParameters)
		{
			SemaphoreParameters parameters;
			Init(&parameters);
		}
		else
			Init(pSemaphoreParameters);
	}


	EA::Thread::Semaphore::Semaphore(int initialCount)
	{
		SemaphoreParameters parameters(initialCount);
		Init(&parameters);
	}


	EA::Thread::Semaphore::~Semaphore()
	{
		EAT_ASSERT(!mSemaphoreData.mMutex.HasLock()); // The mMutex destructor will also assert this, but here it makes it more obvious this mutex is ours.
	}


	bool EA::Thread::Semaphore::Init(const SemaphoreParameters* pSemaphoreParameters)
	{
		if(pSemaphoreParameters && (!mSemaphoreData.mbValid))
		{
			mSemaphoreData.mbValid    = true; // It's not really true unless our member mCV and mMutex init OK. To do: Added functions to our classes that verify they are OK.
			mSemaphoreData.mnCount    = pSemaphoreParameters->mInitialCount;
			mSemaphoreData.mnMaxCount = pSemaphoreParameters->mMaxCount;

			if(mSemaphoreData.mnCount < 0)
				mSemaphoreData.mnCount = 0;

			return mSemaphoreData.mbValid;
		}

		return false;
	}


	int EA::Thread::Semaphore::Wait(const ThreadTime& timeoutAbsolute)
	{
		int nReturnValue = kResultError;
		int result       = mSemaphoreData.mMutex.Lock(); // This mutex is owned by us and will be unlocked immediately in the mCV.Wait call, so we don't apply timeoutAbsolute. To consider: Maybe we should do so, though it's less efficient.

		if(result > 0) // If success...
		{
			if(timeoutAbsolute == kTimeoutImmediate)
			{
				if(mSemaphoreData.mnCount.GetValue() >= 1)
					nReturnValue = mSemaphoreData.mnCount.Decrement();
				else
					nReturnValue = kResultTimeout;
			}
			else
			{
				if(mSemaphoreData.mnCount.GetValue() >= 1) // If we can decrement it immediately...
					nReturnValue = mSemaphoreData.mnCount.Decrement();
				else // Else we need to wait.
				{
					Condition::Result cResult;

					do{
						cResult = mSemaphoreData.mCV.Wait(&mSemaphoreData.mMutex, timeoutAbsolute);
					} while((cResult == Condition::kResultOK) && (mSemaphoreData.mnCount.GetValue() < 1)); // Always need to check the condition and retry if not matched. In rare cases two threads could return from Wait.

					if(cResult == Condition::kResultOK) // If apparent success...
						nReturnValue = mSemaphoreData.mnCount.Decrement();
					else if(cResult == Condition::kResultTimeout)
						nReturnValue = kResultTimeout;
					else
					{
						// We return immediately here because mCV.Wait has not locked the mutex for 
						// us and so we don't want to fall through and unlock it below. Also, it would
						// be inefficient for us to lock here and fall through only to unlock it below.
						return nReturnValue;
					}
				}
			}

			result = mSemaphoreData.mMutex.Unlock();
			EAT_ASSERT(result >= 0);
			if(result < 0)
				nReturnValue = kResultError; // This Semaphore is now considered dead and unusable.
		}

		return nReturnValue;
	}


	int EA::Thread::Semaphore::Post(int count)
	{
		EAT_ASSERT(mSemaphoreData.mnCount >= 0);

		int newValue = kResultError;
		int result   = mSemaphoreData.mMutex.Lock();

		if(result > 0)
		{
			// Set the new value to be whatever the current value is. 
			newValue = mSemaphoreData.mnCount.GetValue();

			if((mSemaphoreData.mnMaxCount - count) < newValue)  // If count would cause an overflow...
				return kResultError; // We do what most OS implementations of max-count do. count = (mSemaphoreData.mnMaxCount - newValue);

			newValue = mSemaphoreData.mnCount.Add(count);

			bool bResult = mSemaphoreData.mCV.Signal(true); // Signal broadcast (the true arg) because semaphores could have multiple counts and multiple threads waiting for them. There's a potential "thundering herd" problem here.
			EAT_ASSERT(bResult);
			EA_UNUSED(bResult);

			result = mSemaphoreData.mMutex.Unlock(); // Important that we lock after the mCV.Signal.
			EAT_ASSERT(result >= 0);
			if(result < 0)
				newValue = kResultError; // This Semaphore is now considered dead and unusable.
		}

		return newValue;
	}


	int EA::Thread::Semaphore::GetCount() const
	{
		return mSemaphoreData.mnCount.GetValue();
	}

#elif !EA_THREADS_AVAILABLE

	///////////////////////////////////////////////////////////////////////////////
	// non-threaded implementation
	///////////////////////////////////////////////////////////////////////////////

	EASemaphoreData::EASemaphoreData()
	  : mnCount(0), 
		mnMaxCount(INT_MAX)
	{
		// Empty
	}


	EA::Thread::SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char* pName)
	  : mInitialCount(initialCount),
		mMaxCount(INT_MAX),
		mbIntraProcess(bIntraProcess)
	{
		if(pName)
		{
			strncpy(mName, pName, sizeof(mName)-1);
			mName[sizeof(mName)-1] = 0;
		}
		else
			mName[0] = 0;
	}


	EA::Thread::Semaphore::Semaphore(const SemaphoreParameters* pSemaphoreParameters, bool bDefaultParameters)
	{
		if(!pSemaphoreParameters && bDefaultParameters)
		{
			SemaphoreParameters parameters;
			Init(&parameters);
		}
		else
			Init(pSemaphoreParameters);
	}


	EA::Thread::Semaphore::Semaphore(int initialCount)
	{
		SemaphoreParameters parameters(initialCount);
		Init(&parameters);
	}


	EA::Thread::Semaphore::~Semaphore()
	{
	}


	bool EA::Thread::Semaphore::Init(const SemaphoreParameters* pSemaphoreParameters)
	{
		if(pSemaphoreParameters)
		{
			mSemaphoreData.mnCount    = pSemaphoreParameters->mInitialCount;
			mSemaphoreData.mnMaxCount = pSemaphoreParameters->mMaxCount;
			return true;
		}

		return false;
	}


	int EA::Thread::Semaphore::Wait(const ThreadTime& timeoutAbsolute)
	{
		if(timeoutAbsolute == kTimeoutNone)
		{
			while(mSemaphoreData.mnCount <= 0)
				ThreadSleep(1);

			--mSemaphoreData.mnCount;
		}
		else if(timeoutAbsolute == 0)
		{
			if(mSemaphoreData.mnCount)
				--mSemaphoreData.mnCount;
			else
				return kResultTimeout;
		}
		else
		{
			while((mSemaphoreData.mnCount <= 0) && (GetThreadTime() < timeoutAbsolute))
				ThreadSleep(1);

			if(mSemaphoreData.mnCount <= 0)
				return kResultTimeout;
		}

		return mSemaphoreData.mnCount;
	}


	int EA::Thread::Semaphore::Post(int count)
	{
		EAT_ASSERT(mSemaphoreData.mnCount >= 0);

		// Ideally, what we would do is account for the number of waiters in 
		// this overflow calculation. If max-count = 4, count = 6, waiters = 8, 
		// we would release 6 waiters and leave the semaphore at 2.
		// The problem is that some of those 6 waiters might time out while we 
		// are doing this and leave ourselves with count greater than max-count.
		if((mSemaphoreData.mnMaxCount - count) < mSemaphoreData.mnCount)  // If count would cause an overflow...
			return kResultError; // We do what most OS implementations of max-count do. // count = (mSemaphoreData.mnMaxCount - nLastCount);

		return (mSemaphoreData.mnCount += count);
	}


	int EA::Thread::Semaphore::GetCount() const
	{
		return mSemaphoreData.mnCount;
	}

#endif // !EA_THREADS_AVAILABLE

