///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "eathread/eathread_mutex.h"

EAMutexData::EAMutexData() : mnLockCount(0) {}

EA::Thread::MutexParameters::MutexParameters(bool /*bIntraProcess*/, const char* pName)
{
	if(pName)
	{
		strncpy(mName, pName, sizeof(mName)-1);
		mName[sizeof(mName)-1] = 0;
	}
	else
	{
		mName[0] = 0;
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
	{
		Init(pMutexParameters);
	}
}

EA::Thread::Mutex::~Mutex()
{
	EAT_ASSERT(mMutexData.mnLockCount == 0);
}

bool EA::Thread::Mutex::Init(const MutexParameters* pMutexParameters)
{
	if (pMutexParameters)
	{
		mMutexData.mnLockCount = 0;
		return true;
	}
	return false;
}

int EA::Thread::Mutex::Lock(const ThreadTime& timeoutAbsolute)
{
	if (timeoutAbsolute == kTimeoutNone)
	{
		mMutexData.mMutex.lock();
	}
	else
	{
		std::chrono::milliseconds timeoutAbsoluteMs(timeoutAbsolute);
		std::chrono::time_point<std::chrono::system_clock> timeout_time(timeoutAbsoluteMs);
		if (!mMutexData.mMutex.try_lock_until(timeout_time))
		{
			return kResultTimeout;
		}
	}

	EAT_ASSERT((mMutexData.mThreadId = EA::Thread::GetThreadId()) != kThreadIdInvalid);
	EAT_ASSERT(mMutexData.mnLockCount >= 0);

	return ++mMutexData.mnLockCount; // This is safe to do because we have the lock.
}

int EA::Thread::Mutex::Unlock()
{
	EAT_ASSERT(mMutexData.mThreadId == EA::Thread::GetThreadId());
	EAT_ASSERT(mMutexData.mnLockCount > 0);

	const int nReturnValue(--mMutexData.mnLockCount); // This is safe to do because we have the lock.
	mMutexData.mMutex.unlock();
	return nReturnValue;
}

int EA::Thread::Mutex::GetLockCount() const
{
	return mMutexData.mnLockCount;
}

bool EA::Thread::Mutex::HasLock() const
{
#if EAT_ASSERT_ENABLED
	return (mMutexData.mnLockCount > 0) && (mMutexData.mThreadId == EA::Thread::GetThreadId());
#else
	return (mMutexData.mnLockCount > 0); // This is the best we can do, though it is of limited use, since it doesn't tell you if you are the thread with the lock.
#endif
}



