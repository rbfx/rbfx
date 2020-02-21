///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <eathread/internal/config.h>
#include <eathread/eathread_mutex.h>


#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <errno.h>
	#include <string.h>
	#ifdef EA_PLATFORM_WINDOWS
	  #pragma warning(push, 0)
	  #include <Windows.h> // Presumably we are using pthreads-win32.
	  #pragma warning(pop)
		#ifdef CreateMutex
			#undef CreateMutex // Windows #defines CreateMutex to CreateMutexA or CreateMutexW.
		#endif
	#endif


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

#if EAT_ASSERT_ENABLED 
			mThreadId = EA::Thread::GetThreadId();
#endif
		}
		else
		{
			--mnLockCount;

#if EAT_ASSERT_ENABLED 
			mThreadId = EA::Thread::kThreadIdInvalid;
#endif
		}
	}


	EA::Thread::MutexParameters::MutexParameters(bool bIntraProcess, const char* /*pName*/)
		: mbIntraProcess(bIntraProcess)
	{
		// Empty
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
		pthread_mutex_destroy(&mMutexData.mMutex);
	}


	bool EA::Thread::Mutex::Init(const MutexParameters* pMutexParameters)
	{
		if(pMutexParameters)
		{
			mMutexData.mnLockCount = 0;

			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);

			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

				#if defined(PTHREAD_PROCESS_PRIVATE)    // Some pthread implementations don't recognize this.
					#if defined(PTHREAD_PROCESS_SHARED)
						if (pMutexParameters->mbIntraProcess)
							pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
						else
							pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
					#else
						pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
					#endif
				#endif

			const int result = pthread_mutex_init(&mMutexData.mMutex, &attr);
			pthread_mutexattr_destroy(&attr);

			EAT_ASSERT(result != -1);
			return (result != -1);
		}

		return false;
	}


	int EA::Thread::Mutex::Lock(const ThreadTime& timeoutAbsolute)
	{
		int result;

		EAT_ASSERT(mMutexData.mnLockCount < 100000);

		if(timeoutAbsolute == kTimeoutNone)
		{
			result = pthread_mutex_lock(&mMutexData.mMutex);

			if(result != 0)
			{
				EAT_ASSERT(false);
				return kResultError;
			}
		}
		else if(timeoutAbsolute == kTimeoutImmediate)
		{
			result = pthread_mutex_trylock(&mMutexData.mMutex);

			if(result != 0)
			{
				if(result == EBUSY)
					return kResultTimeout;

				EAT_ASSERT(false);
				return kResultError;
			}
		}
		else
		{
			#if (defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_WINDOWS)) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID)
				const timespec* pTimeSpec = &timeoutAbsolute;
				result = pthread_mutex_timedlock(&mMutexData.mMutex, const_cast<timespec*>(pTimeSpec)); // Some pthread implementations use non-const timespec, so cast for them.

				if(result != 0)
				{
					if(result == ETIMEDOUT)
						return kResultTimeout;
					EAT_ASSERT(false);
					return kResultError;
				}
			#else // OSX, BSD
				// Some Posix systems don't have pthread_mutex_timedlock. In these
				// cases we fall back to a polling mechanism. However, polling really
				// isn't proper because the polling thread might be at a greater 
				// priority level than the lock-owning thread and thus this code
				// might not work as well as desired.
				while(((result = pthread_mutex_trylock(&mMutexData.mMutex)) != 0) && (GetThreadTime() < timeoutAbsolute))
					ThreadSleep(1);

				if(result != 0)
				{
					if(result == EBUSY)
						return kResultTimeout;
					EAT_ASSERT(false);
					return kResultError;
				}
			#endif
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

		if(pthread_mutex_unlock(&mMutexData.mMutex) != 0)
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


#endif // EA_PLATFORM_XXX







