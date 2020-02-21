///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/internal/config.h>

EA_DISABLE_VC_WARNING(4574)
#include <new>
EA_RESTORE_VC_WARNING()

#if defined(EA_PLATFORM_SONY)
	// Posix already defines a Condition (via condition variables).
	#include "kettle/eathread_condition_kettle.cpp"
#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EA_THREADS_AVAILABLE
	// Posix already defines a Condition (via condition variables).
	#include "unix/eathread_condition_unix.cpp"

#else // All other platforms

	#include <eathread/eathread_condition.h>
	#include <string.h>

	EAConditionData::EAConditionData()
	   : mnWaitersBlocked(0), mnWaitersToUnblock(0), mnWaitersDone(0),
		 mSemaphoreBlockQueue(NULL, false), // We will be initializing these ourselves specifically below.
		 mSemaphoreBlockLock(NULL, false),
		 mUnblockLock(NULL, false)
	{
		// Empty
	}


	EA::Thread::ConditionParameters::ConditionParameters(bool bIntraProcess, const char* pName)
		: mbIntraProcess(bIntraProcess)
	{
		if(pName)
		{
			strncpy(mName, pName, sizeof(mName)-1);
			mName[sizeof(mName)-1] = 0;
		}
		else
			mName[0] = 0;
	}


	EA::Thread::Condition::Condition(const ConditionParameters* pConditionParameters, bool bDefaultParameters)
	{
		if(!pConditionParameters && bDefaultParameters)
		{
			ConditionParameters parameters;
			Init(&parameters);
		}
		else
			Init(pConditionParameters);
	}


	EA::Thread::Condition::~Condition()
	{
		// Empty
	}


	bool EA::Thread::Condition::Init(const ConditionParameters* pConditionParameters)
	{
		if(pConditionParameters)
		{
			// We have a problem with naming here. We implement our Condition variable with two semaphores and a mutex.
			// It's not possible to have them all have the same name, since the OS will think you want them to be
			// shared instances. What we really need is an explicit debug name that is separate from the OS name. 
			// And the ConditionParameters::mName should be that debug name only and not be applied to the child primitives.

			const SemaphoreParameters sp1(0, pConditionParameters->mbIntraProcess, NULL);   // Set the name to NULL, regardless of what pConditionParameters->mName is. 
			const SemaphoreParameters sp2(1, pConditionParameters->mbIntraProcess, NULL);
			const MutexParameters     mp(pConditionParameters->mbIntraProcess,     NULL);

			if(mConditionData.mSemaphoreBlockQueue.Init(&sp1) && 
			   mConditionData.mSemaphoreBlockLock .Init(&sp2) && 
			   mConditionData.mUnblockLock.Init(&mp))
			{
				return true;
			}
		}

		return false;
	}


	EA::Thread::Condition::Result EA::Thread::Condition::Wait(Mutex* pMutex, const ThreadTime& timeoutAbsolute)
	{
		int lockResult, result;

		EAT_ASSERT(pMutex); // The user is required to pass a valid Mutex pointer.

		++mConditionData.mnWaitersBlocked; // Note that this is an atomic operation.

		EAT_ASSERT(pMutex->GetLockCount() == 1);
		lockResult = pMutex->Unlock();
		if(lockResult < 0)
			return (Result)lockResult;

		result = mConditionData.mSemaphoreBlockQueue.Wait(timeoutAbsolute);
		EAT_ASSERT(result != EA::Thread::Semaphore::kResultError);
		// Regardless of the result of the above error, we must press on with the code below.

		mConditionData.mUnblockLock.Lock();
		
		const int nWaitersToUnblock = mConditionData.mnWaitersToUnblock;

		if(nWaitersToUnblock != 0)
			--mConditionData.mnWaitersToUnblock;
		else if(++mConditionData.mnWaitersDone == (INT_MAX / 2)) // This is not an atomic operation. We are within a mutex lock.
		{ 
			// Normally this doesn't happen, but can happen under very 
			// unusual circumstances, such as spurious semaphore signals
			// or cases whereby many many threads are timing out.
			EAT_ASSERT(false);
			mConditionData.mSemaphoreBlockLock.Wait();
			mConditionData.mnWaitersBlocked -= mConditionData.mnWaitersDone;
			mConditionData.mSemaphoreBlockLock.Post();
			mConditionData.mnWaitersDone = 0;
		}

		mConditionData.mUnblockLock.Unlock();

		if(nWaitersToUnblock == 1) // If we were the last...
			mConditionData.mSemaphoreBlockLock.Post();

		// We cannot apply a timeout here. The caller always expects to have the 
		// lock upon return, even in the case of a wait timeout. Similarly, we 
		// may or may not want the result of the lock attempt to be propogated
		// back to the caller. In this case, we do if it is an error.
		lockResult = pMutex->Lock();

		if(lockResult == Mutex::kResultError)
			return kResultError;
		else if(result >= 0)
			return kResultOK;

		return (Result)result; // This is the result of the wait call above.
	}


	bool EA::Thread::Condition::Signal(bool bBroadcast)
	{
		int result;
		int nSignalsToIssue;

		result = mConditionData.mUnblockLock.Lock();

		if(result < 0)
			return false;

		if(mConditionData.mnWaitersToUnblock)
		{
			if(mConditionData.mnWaitersBlocked == 0)
			{
				mConditionData.mUnblockLock.Unlock();
				return true;
			}

			if(bBroadcast)
			{
				nSignalsToIssue = (int)mConditionData.mnWaitersBlocked.SetValue(0);
				mConditionData.mnWaitersToUnblock += nSignalsToIssue;
			}
			else
			{
				nSignalsToIssue = 1;
				mConditionData.mnWaitersToUnblock++;
				mConditionData.mnWaitersBlocked--;
			}
		}
		else if(mConditionData.mnWaitersBlocked > mConditionData.mnWaitersDone)
		{
			if(mConditionData.mSemaphoreBlockLock.Wait() == EA::Thread::Semaphore::kResultError)
			{
				mConditionData.mUnblockLock.Unlock();
				return false;
			}

			if(mConditionData.mnWaitersDone != 0)
			{
				mConditionData.mnWaitersBlocked -= mConditionData.mnWaitersDone;
				mConditionData.mnWaitersDone     = 0;
			}

			if(bBroadcast)
			{
				nSignalsToIssue = mConditionData.mnWaitersToUnblock = (int)mConditionData.mnWaitersBlocked.SetValue(0);
			}
			else
			{
				nSignalsToIssue = mConditionData.mnWaitersToUnblock = 1;
				mConditionData.mnWaitersBlocked--;
			}
		}
		else
		{
			mConditionData.mUnblockLock.Unlock();
			return true;
		}

		mConditionData.mUnblockLock.Unlock();
		mConditionData.mSemaphoreBlockQueue.Post(nSignalsToIssue);

		return true;
	}

#endif // EA_PLATFORM_XXX




EA::Thread::Condition* EA::Thread::ConditionFactory::CreateCondition()
{
	Allocator* pAllocator = GetAllocator();

	if(pAllocator)
		return new(pAllocator->Alloc(sizeof(EA::Thread::Condition))) EA::Thread::Condition;
	else
		return new EA::Thread::Condition;
}

void EA::Thread::ConditionFactory::DestroyCondition(EA::Thread::Condition* pCondition)
{
	Allocator* pAllocator = GetAllocator();

	if(pAllocator)
	{
		pCondition->~Condition();
		pAllocator->Free(pCondition);
	}
	else
		delete pCondition;
}

size_t EA::Thread::ConditionFactory::GetConditionSize()
{
	return sizeof(EA::Thread::Condition);
}

EA::Thread::Condition* EA::Thread::ConditionFactory::ConstructCondition(void* pMemory)
{
	return new(pMemory) EA::Thread::Condition;
}

void EA::Thread::ConditionFactory::DestructCondition(EA::Thread::Condition* pCondition)
{
	pCondition->~Condition();
}













