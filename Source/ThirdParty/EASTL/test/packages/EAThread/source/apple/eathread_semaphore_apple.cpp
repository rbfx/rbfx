///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <eathread/eathread_semaphore.h>

#if defined(EA_PLATFORM_APPLE)

#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/kern_return.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>


EASemaphoreData::EASemaphoreData()
: mSemaphore(), mnCount(0), mnMaxCount(INT_MAX)
{
}


EA::Thread::SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char* /*pName*/)
: mInitialCount(initialCount), mMaxCount(INT_MAX), mbIntraProcess(bIntraProcess)
{
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
	const kern_return_t result = semaphore_destroy(mach_task_self(), mSemaphoreData.mSemaphore); (void)result;
	EAT_ASSERT(KERN_SUCCESS == result);
}


bool EA::Thread::Semaphore::Init(const SemaphoreParameters* pSemaphoreParameters)
{
	if(pSemaphoreParameters)
	{
		mSemaphoreData.mnCount    = pSemaphoreParameters->mInitialCount;
		mSemaphoreData.mnMaxCount = pSemaphoreParameters->mMaxCount;

		if(mSemaphoreData.mnCount < 0)
			mSemaphoreData.mnCount = 0;

		// Todo, Jaap Suter, December 2009, do we care about actually supporting this?
		mSemaphoreData.mbIntraProcess = pSemaphoreParameters->mbIntraProcess;

		const kern_return_t result = semaphore_create(mach_task_self(), &mSemaphoreData.mSemaphore, SYNC_POLICY_FIFO, static_cast<int>(mSemaphoreData.mnCount)); (void)result;
		EAT_ASSERT(KERN_SUCCESS == result);

		return true;
	}

	return false;
}


int EA::Thread::Semaphore::Wait(const ThreadTime& timeoutAbsolute)
{
	kern_return_t result = KERN_SUCCESS;

	if(timeoutAbsolute == kTimeoutNone)
	{
		result = semaphore_wait(mSemaphoreData.mSemaphore);

		if(result != KERN_SUCCESS)
		{
			EAT_ASSERT(false); // This is an error condition.
			return kResultError;
		}
	}
	else
	{
		for (;;)
		{               
			ThreadTime timeoutRelative = kTimeoutImmediate;
			if (timeoutAbsolute != kTimeoutImmediate)
			{
				ThreadTime timeCurrent = GetThreadTime();
				timeoutRelative = (timeoutAbsolute > timeCurrent) ? (timeoutAbsolute - timeCurrent) : kTimeoutImmediate;
			}

			mach_timespec_t machTimeoutRelative = { (unsigned int)timeoutRelative.tv_sec, (clock_res_t)timeoutRelative.tv_nsec };
			result = semaphore_timedwait(mSemaphoreData.mSemaphore, machTimeoutRelative);

			if (result == KERN_SUCCESS)
				break;

			if (result == KERN_OPERATION_TIMED_OUT)
				return kResultTimeout;

			// printf("semaphore_timedwait other error: %d\n", result);
		}
	}

	EAT_ASSERT(mSemaphoreData.mnCount > 0);
	return (int)mSemaphoreData.mnCount.Decrement(); // AtomicInt32 operation. Note that the value of the semaphore count could change from the returned value by the time the caller reads it. This is fine but the user should understand this.
}


int EA::Thread::Semaphore::Post(int count)
{
	// Some systems have a sem_post_multiple which we could take advantage 
	// of here to atomically post multiple times.
	EAT_ASSERT(mSemaphoreData.mnCount >= 0);

	int currentCount = mSemaphoreData.mnCount;

	// If count would cause an overflow exit early
	if ((mSemaphoreData.mnMaxCount - count) < currentCount)
		return kResultError;

	currentCount += count;

	while(count-- > 0)
	{
		++mSemaphoreData.mnCount;     // AtomicInt32 operation.

		if(semaphore_signal(mSemaphoreData.mSemaphore) != KERN_SUCCESS)
		{
			--mSemaphoreData.mnCount; // AtomicInt32 operation.
			EAT_ASSERT(false);
			return kResultError;        
		}
	}

	// If all count posts occurred...
	return currentCount; // It's possible that another thread may have modified this value since we changed it, but that's not important.
}


int EA::Thread::Semaphore::GetCount() const
{
	return (int)mSemaphoreData.mnCount;
}


#endif // #if defined(EA_PLATFORM_APPLE) 
