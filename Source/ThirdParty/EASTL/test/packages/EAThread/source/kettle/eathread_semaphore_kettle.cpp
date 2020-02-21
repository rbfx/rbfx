///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread_semaphore.h>

#if defined(EA_PLATFORM_SONY)

	#include <kernel/semaphore.h>
	#include <sceerror.h>

	EASemaphoreData::EASemaphoreData()
	: mSemaphore(NULL), mnMaxCount(INT_MAX), mnCount(0)
	{
	}

	EA::Thread::SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char* pName)
		: mInitialCount(initialCount), mMaxCount(INT_MAX), mbIntraProcess(bIntraProcess)
	{
		// Maximum lenght for the semaphore name on Kettle is 32 (including NULL terminator)
		EAT_COMPILETIME_ASSERT(sizeof(mName) <= 32);

		if (pName)
		{
			strncpy(mName, pName, sizeof(mName)-1);
			mName[sizeof(mName)-1] = 0;
		}
		else
		{
			mName[0] = 0;
		}
	}


	EA::Thread::Semaphore::Semaphore(const SemaphoreParameters* pSemaphoreParameters, bool bDefaultParameters)
	{
		if (!pSemaphoreParameters && bDefaultParameters)
		{
			SemaphoreParameters parameters;
			Init(&parameters);
		}
		else
		{
			Init(pSemaphoreParameters);
		}
	}


	EA::Thread::Semaphore::Semaphore(int initialCount)
	{
		SemaphoreParameters parameters(initialCount);
		Init(&parameters);
	}


	EA::Thread::Semaphore::~Semaphore()
	{
		int result = sceKernelDeleteSema(mSemaphoreData.mSemaphore);
		EAT_ASSERT(result == SCE_OK); EA_UNUSED(result);
	}


	bool EA::Thread::Semaphore::Init(const SemaphoreParameters* pSemaphoreParameters)
	{
		if (pSemaphoreParameters 
			&& pSemaphoreParameters->mInitialCount >= 0
			&& pSemaphoreParameters->mMaxCount >= 0)
		{
			mSemaphoreData.mnMaxCount = pSemaphoreParameters->mMaxCount;
			mSemaphoreData.mnCount = pSemaphoreParameters->mInitialCount;

			int result = sceKernelCreateSema(
				&mSemaphoreData.mSemaphore,
				pSemaphoreParameters->mName,
				SCE_KERNEL_SEMA_ATTR_TH_FIFO,
				mSemaphoreData.mnCount,
				mSemaphoreData.mnMaxCount,
				NULL);

			if (result == SCE_OK)
				return true;
		}

		// Failure: could not create semaphore
		return false;
	}


	int EA::Thread::Semaphore::Wait(const ThreadTime& timeoutAbsolute)
	{
		int result = 0;

		// Convert timeout from absolute to relative (possibly losing some capacity)        
		SceKernelUseconds timeoutRelativeUs = static_cast<SceKernelUseconds>(RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));
		do
		{
			if (timeoutAbsolute == kTimeoutImmediate)
			{
				result = sceKernelPollSema(mSemaphoreData.mSemaphore, 1);
			}
			else
			{
				result = sceKernelWaitSema(mSemaphoreData.mSemaphore, 1, &timeoutRelativeUs);
			}

			if (result != SCE_OK)
			{
				// SCE_KERNEL_ERROR_ETIMEDOUT is the failure case for 'sceKernelWaitSema'
				// SCE_KERNEL_ERROR_EBUSY is the failure case for 'sceKernelPollSema'
				// We want to consume the SCE_KERNEL_ERROR_EBUSY error code from the polling interface
				// users have a consistent error code to check against.
				if (result == SCE_KERNEL_ERROR_ETIMEDOUT || result == SCE_KERNEL_ERROR_EBUSY) 
				{
					if (timeoutAbsolute != kTimeoutNone)
						return kResultTimeout;
				}
				else
				{
					EAT_FAIL_MSG("Semaphore::Wait: sceKernelWaitSema failure.");
					return kResultError;
				}
			}
		} while (result != SCE_OK);

		// Success
		EAT_ASSERT(mSemaphoreData.mnCount.GetValue() > 0);
		return static_cast<int>(mSemaphoreData.mnCount.Decrement());
	}


	int EA::Thread::Semaphore::Post(int count)
	{
		EAT_ASSERT(count >= 0);

		const int currentCount = mSemaphoreData.mnCount;

		if (count > 0)
		{
			// If count would cause an overflow exit early
			if ((mSemaphoreData.mnMaxCount - count) < currentCount)
				return kResultError;

			// We increment the count before we signal the semaphore so that any waken up
			// thread will have the right count immediately
			mSemaphoreData.mnCount.Add(count);

			int result = sceKernelSignalSema(mSemaphoreData.mSemaphore, count);

			if (result != SCE_OK)
			{
				// If not successful set the count back
				mSemaphoreData.mnCount.Add(-count);
				return kResultError;
			}
		}

		return currentCount + count; // It's possible that another thread may have modified this value since we changed it, but that's not important.
	}


	int EA::Thread::Semaphore::GetCount() const
	{
		// There is no way to query the semaphore for the resource count on Kettle, 
		// we need to rely on our external atomic counter
		return mSemaphoreData.mnCount.GetValue();
	}

#endif // EA_PLATFORM_SONY








