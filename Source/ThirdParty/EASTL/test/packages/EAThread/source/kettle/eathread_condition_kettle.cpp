///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/internal/config.h>
#include <eathread/eathread_condition.h>
#include <kernel.h>
#include <time.h>
#include <errno.h>
#include <string.h>


EAConditionData::EAConditionData()
{
	memset(&mCV, 0, sizeof(mCV));
}


EA::Thread::ConditionParameters::ConditionParameters(bool bIntraProcess, const char* pName)
	: mbIntraProcess(bIntraProcess)
{
	if (pName)
	{
		strncpy(mName, pName, sizeof(mName) - 1);
		mName[sizeof(mName) - 1] = 0;
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
	scePthreadCondDestroy(&mConditionData.mCV);
}


bool EA::Thread::Condition::Init(const ConditionParameters* pConditionParameters)
{
	if(pConditionParameters)
	{
		ScePthreadCondattr cattr;
		scePthreadCondattrInit(&cattr);
		const int result = scePthreadCondInit(&mConditionData.mCV, &cattr, pConditionParameters->mName);
		EAT_ASSERT(result == 0);

		scePthreadCondattrDestroy(&cattr);
		return (result == 0);
	}

	return false;
}


EA::Thread::Condition::Result EA::Thread::Condition::Wait(Mutex* pMutex, const ThreadTime& timeoutAbsolute)
{
	int result;
	ScePthreadMutex* pMutex_t;
	EAMutexData* pMutexData;

	EAT_ASSERT(pMutex);

	// We have a small problem here in that if we are using the pMutex argument, 
	// the pthread_cond_wait call will unlock the mutex via the internal mutex data and
	// not without calling the Mutex::Lock function. The result is that the Mutex doesn't
	// have its lock count value reduced by one and so other threads will see the lock
	// count as being 1 when in fact it should be zero. So we account for that here
	// by manually maintaining the lock count, which we can do because we have the lock.
	EAT_ASSERT(pMutex->GetLockCount() == 1);
	pMutexData = (EAMutexData*)pMutex->GetPlatformData();
	pMutexData->SimulateLock(false);
	pMutex_t = &pMutexData->mMutex;

	if(timeoutAbsolute == kTimeoutNone)
		result = scePthreadCondWait(&mConditionData.mCV, pMutex_t);
	else
		result = scePthreadCondTimedwait(&mConditionData.mCV, pMutex_t, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));    

	pMutexData->SimulateLock(true);
	EAT_ASSERT(!pMutex || (pMutex->GetLockCount() == 1));

	if(result != 0)
	{
		if(result == SCE_KERNEL_ERROR_ETIMEDOUT)
			return kResultTimeout;
		EAT_ASSERT(false);
		return kResultError;
	}
	return kResultOK;
}


bool EA::Thread::Condition::Signal(bool bBroadcast)
{
	if(bBroadcast)
		return (scePthreadCondBroadcast(&mConditionData.mCV) == 0);
	return (scePthreadCondSignal(&mConditionData.mCV) == 0);
}










