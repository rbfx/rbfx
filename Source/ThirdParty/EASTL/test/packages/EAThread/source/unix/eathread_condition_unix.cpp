///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <eathread/internal/config.h>
#include <eathread/eathread_condition.h>


#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>
	#include <time.h>
	#include <errno.h>
	#include <string.h>
	#ifdef EA_PLATFORM_WINDOWS
		#pragma warning(push, 0)
		#include <Windows.h> // Presumably we are using pthreads-win32.
		#pragma warning(pop)
	#endif


	EAConditionData::EAConditionData()
	{
		memset(&mCV, 0, sizeof(mCV));
	}


	EA::Thread::ConditionParameters::ConditionParameters(bool bIntraProcess, const char* /*pName*/)
		: mbIntraProcess(bIntraProcess)
	{
		// Empty
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
		pthread_cond_destroy(&mConditionData.mCV);
	}


	bool EA::Thread::Condition::Init(const ConditionParameters* pConditionParameters)
	{
		if(pConditionParameters)
		{
			#if defined(EA_PLATFORM_ANDROID)  // Some platforms don't provide pthread_condattr_init, and pthread_condattr_t is just an int.
				pthread_condattr_t cattr;
				memset(&cattr, 0, sizeof(cattr));

				const int result = pthread_cond_init(&mConditionData.mCV, &cattr);

				return (result == 0);
			#else
				pthread_condattr_t cattr;
				pthread_condattr_init(&cattr);

				#if defined(PTHREAD_PROCESS_PRIVATE) // Some pthread implementations don't recognize this. PTHREAD_PROCESS_SHARED bugged on iphone
					#if defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_OSX)
						EAT_ASSERT( pConditionParameters->mbIntraProcess == true ); // shared conditions bugged on apple hardware
					#else   
						if(pConditionParameters->mbIntraProcess)
							 pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_PRIVATE); 
						else
							pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
					#endif
				#endif

				const int result = pthread_cond_init(&mConditionData.mCV, &cattr);

				pthread_condattr_destroy(&cattr);

				return (result == 0);
			#endif
		}

		return false;
	}


	EA::Thread::Condition::Result EA::Thread::Condition::Wait(Mutex* pMutex, const ThreadTime& timeoutAbsolute)
	{
		int result;
		pthread_mutex_t* pMutex_t;
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
			result = pthread_cond_wait(&mConditionData.mCV, pMutex_t);
		else
			result = pthread_cond_timedwait(&mConditionData.mCV, pMutex_t, &timeoutAbsolute);

		pMutexData->SimulateLock(true);
		EAT_ASSERT(!pMutex || (pMutex->GetLockCount() == 1));

		if(result != 0)
		{
			if(result == ETIMEDOUT)
				return kResultTimeout;
			EAT_ASSERT(false);
			return kResultError;
		}
		return kResultOK;
	}


	bool EA::Thread::Condition::Signal(bool bBroadcast)
	{
		if(bBroadcast)
			return (pthread_cond_broadcast(&mConditionData.mCV) == 0);
		return (pthread_cond_signal(&mConditionData.mCV) == 0);
	}


#endif // EA_PLATFORM_XXX








