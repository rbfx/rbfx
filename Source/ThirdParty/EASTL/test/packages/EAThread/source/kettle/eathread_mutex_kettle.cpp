///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/internal/config.h>
#include <eathread/eathread_mutex.h>
#include <errno.h>
#include <string.h>
#include <sceerror.h>

EAMutexData::EAMutexData()
	: mMutex(), mnLockCount(0)
{
	#if EAT_ASSERT_ENABLED
		mThreadId = EA::Thread::kThreadIdInvalid; 
	#endif

	::memset(&mMutex, 0, sizeof(mMutex));
}

void EAMutexData::SimulateLock(bool bLock)
{
	if(bLock)
	{
		++mnLockCount;
		EAT_ASSERT((mThreadId = EA::Thread::GetThreadId()) || true); // Intentionally '=' here and not '=='.
	}
	else
	{
		--mnLockCount;
		EAT_ASSERT((mThreadId = EA::Thread::kThreadIdInvalid) || true); // Intentionally '=' here and not '=='.
	}
}


EA::Thread::MutexParameters::MutexParameters(bool bIntraProcess, const char* pName)
	: mbIntraProcess(bIntraProcess)
{
	mName[0] = '\0';

	if (pName != nullptr)
	{
		strncpy(mName, pName, sizeof(mName) - 1);
		mName[sizeof(mName) - 1] = '\0';
	}
}


EA::Thread::Mutex::Mutex(const MutexParameters* pMutexParameters, bool bDefaultParameters)
{
	if(!pMutexParameters && bDefaultParameters)
	{
		MutexParameters parameters;
		Init(&parameters);
	}
	else
		Init(pMutexParameters);
}


EA::Thread::Mutex::~Mutex()
{
	EAT_ASSERT(mMutexData.mnLockCount == 0);
	scePthreadMutexDestroy(&mMutexData.mMutex);
}


bool EA::Thread::Mutex::Init(const MutexParameters* pMutexParameters)
{
	if(pMutexParameters)
	{
		mMutexData.mnLockCount = 0;

		ScePthreadMutexattr attr;
		scePthreadMutexattrInit(&attr);

		scePthreadMutexattrSettype(&attr, SCE_PTHREAD_MUTEX_RECURSIVE);

		#if defined(SCE_PTHREAD_PROCESS_PRIVATE)    // Some pthread_disabled implementations don't recognize this.
			if(pMutexParameters->mbIntraProcess)                
				scePthreadMutexattrSettype(&attr, SCE_PTHREAD_PROCESS_PRIVATE); 
			else                
				scePthreadMutexattrSettype(&attr, SCE_PTHREAD_PROCESS_PRIVATE); 
		#endif

		// kettle mutex name is restricted to 32 bytes INCLUDING null character. See "scePthreadMutexInit"
		char mutexNameCopy[32];
		strncpy(mutexNameCopy, pMutexParameters->mName, sizeof(mutexNameCopy) - 1);
		mutexNameCopy[sizeof(mutexNameCopy)-1] = '\0';

		// Sony allocates memory for any length string which reduces the amount of active mutex allowed by the operating
		// system.  We only provide a string if it is non-zero in length.
		int result = SCE_KERNEL_ERROR_EAGAIN;
		if (pMutexParameters->mName[0] != '\0')
		{
			result = scePthreadMutexInit(&mMutexData.mMutex, &attr, mutexNameCopy);
		}

		if (result == SCE_KERNEL_ERROR_EAGAIN)
		{
			// We've hit the limit for named mutexes on PS4, so fallback to an unnamed mutex which has a much higher limit
			result = scePthreadMutexInit(&mMutexData.mMutex, &attr, NULL);
		}
		scePthreadMutexattrDestroy(&attr);

		EAT_ASSERT(SCE_OK == result);
		return (SCE_OK == result);
	}

	return false;
}


int EA::Thread::Mutex::Lock(const ThreadTime& timeoutAbsolute)
{
	int result;

	EAT_ASSERT(mMutexData.mnLockCount < 100000);

	if(timeoutAbsolute == kTimeoutNone)
	{
		result = scePthreadMutexLock(&mMutexData.mMutex);

		if(result != 0)
		{
			EAT_ASSERT(false);
			return kResultError;
		}
	}
	else if(timeoutAbsolute == kTimeoutImmediate)
	{
		result = scePthreadMutexTrylock(&mMutexData.mMutex);

		if(result != 0)
		{
			if(result == SCE_KERNEL_ERROR_EBUSY)
				return kResultTimeout;

			EAT_ASSERT(false);
			return kResultError;
		}
	}
	else
	{        
		result = scePthreadMutexTimedlock(&mMutexData.mMutex, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));

		if(result != 0)
		{
			if(result == SCE_KERNEL_ERROR_ETIMEDOUT)
				return kResultTimeout;

			EAT_ASSERT(false);
			return kResultError;
		}
	}

	EAT_ASSERT(mMutexData.mThreadId = EA::Thread::GetThreadId()); // Intentionally '=' here and not '=='.
	EAT_ASSERT(mMutexData.mnLockCount >= 0);
	return ++mMutexData.mnLockCount; // This is safe to do because we have the lock.
}


int EA::Thread::Mutex::Unlock()
{
	EAT_ASSERT(mMutexData.mThreadId == EA::Thread::GetThreadId());
	EAT_ASSERT(mMutexData.mnLockCount > 0);

	const int nReturnValue(--mMutexData.mnLockCount); // This is safe to do because we have the lock.

	if(scePthreadMutexUnlock(&mMutexData.mMutex) != 0)
	{
		EAT_ASSERT(false);
		return nReturnValue + 1;
	}

	return nReturnValue;
}


int EA::Thread::Mutex::GetLockCount() const
{
	return mMutexData.mnLockCount;
}


bool EA::Thread::Mutex::HasLock() const
{
	#if EAT_ASSERT_ENABLED
		return (mMutexData.mnLockCount > 0) && (mMutexData.mThreadId == GetThreadId());
	#else
		return (mMutexData.mnLockCount > 0); // This is the best we can do.
	#endif
}





