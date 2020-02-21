///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EABase/eabase.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread.h"

#if defined(EA_PLATFORM_MICROSOFT)
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <Windows.h>
	EA_RESTORE_ALL_VC_WARNINGS()
#endif
#ifdef CreateMutex
	#undef CreateMutex // Windows #defines CreateMutex to CreateMutexA or CreateMutexW.
#endif


#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	#if defined(EA_PLATFORM_WINDOWS)
		extern "C" WINBASEAPI BOOL WINAPI TryEnterCriticalSection(_Inout_ LPCRITICAL_SECTION lpCriticalSection);
	#endif

	EAMutexData::EAMutexData()
		: mnLockCount(0), mbIntraProcess(true)
	{
		#if EAT_ASSERT_ENABLED
			mThreadId = EA::Thread::kThreadIdInvalid; 
			mSysThreadId = EA::Thread::kSysThreadIdInvalid;
		#endif

		::memset(&mData, 0, sizeof(mData));
	}


	EA::Thread::MutexParameters::MutexParameters(bool bIntraProcess, const char* pName)
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

		// Consider doing something to verify the mutex object has been initialized.
		#if defined(EA_PLATFORM_WINDOWS)
			if(mMutexData.mbIntraProcess)
				DeleteCriticalSection((CRITICAL_SECTION*)mMutexData.mData);
			else
				CloseHandle(*(HANDLE*)mMutexData.mData);
		#else
			DeleteCriticalSection((CRITICAL_SECTION*)mMutexData.mData);
		#endif
	}


	bool EA::Thread::Mutex::Init(const MutexParameters* pMutexParameters)
	{
		// Make sure that internal structure is big enough to hold critical section data.
		// If this assert fires, please adjust MUTEX_PLATFORM_DATA_SIZE in eathread_mutex.h accordingly.
		EAT_COMPILETIME_ASSERT(sizeof(CRITICAL_SECTION) <= (MUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t) * sizeof(uint64_t)));
		EAT_COMPILETIME_ASSERT(sizeof(HANDLE) <= MUTEX_PLATFORM_DATA_SIZE);

		if(pMutexParameters)
		{
			mMutexData.mnLockCount = 0;

			#if defined(EA_PLATFORM_WINDOWS) 
				mMutexData.mbIntraProcess = pMutexParameters->mbIntraProcess;

				if(mMutexData.mbIntraProcess)
				{
					// We use InitializeCriticalSectionAndSpinCount, as that has resulted in improved performance in practice on multiprocessors systems.
					int rv = InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION*)mMutexData.mData, 256);
					EAT_ASSERT(rv != 0);
					EA_UNUSED(rv);

					return true;
				}
				else
				{
					EAT_COMPILETIME_ASSERT(sizeof(pMutexParameters->mName) <= MAX_PATH);
					*(HANDLE*)mMutexData.mData = ::CreateMutexA(NULL, false, pMutexParameters->mName[0] ? pMutexParameters->mName : NULL);
					EAT_ASSERT(*(HANDLE*)mMutexData.mData != 0);
					return *(HANDLE*)mMutexData.mData != 0;
				}
			#else
				// We use InitializeCriticalSectionAndSpinCount, as that has resulted in improved performance in practice on multiprocessors systems.
				InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION*)mMutexData.mData, 256);
				return true;
			#endif
		}
		return false;
	}



	#pragma warning(push)
	#pragma warning(disable: 4706) // disable warning about assignment within a conditional expression

	int EA::Thread::Mutex::Lock(const ThreadTime& timeoutAbsolute)
	{
		EAT_ASSERT(mMutexData.mnLockCount < 100000);

		#if defined(EA_PLATFORM_WINDOWS) // Non-Windows is always assumed to be intra-process.
			if(mMutexData.mbIntraProcess)
			{
		#endif
				if(timeoutAbsolute == kTimeoutNone)
					EnterCriticalSection((CRITICAL_SECTION*)mMutexData.mData);
				else
				{
					// To consider: Have a pathway for kTimeoutImmediate which doesn't check the current time.
					while(!TryEnterCriticalSection((CRITICAL_SECTION*)mMutexData.mData))
					{
						if(GetThreadTime() >= timeoutAbsolute)
							return kResultTimeout;
						Sleep(1);
					}
				}
		#if defined(EA_PLATFORM_WINDOWS)
			}
			else
			{
				EAT_ASSERT(*(HANDLE*)mMutexData.mData != 0);

				const DWORD dw = ::WaitForSingleObject(*(HANDLE*)mMutexData.mData, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));

				if(dw == WAIT_TIMEOUT)
					return kResultTimeout;

				if(dw != WAIT_OBJECT_0)
				{
					EAT_ASSERT(false);
					return kResultError;
				}
			}
		#endif

		EAT_ASSERT((mMutexData.mSysThreadId = EA::Thread::GetSysThreadId()) != kSysThreadIdInvalid);
		EAT_ASSERT(mMutexData.mnLockCount >= 0);
		return ++mMutexData.mnLockCount; // This is safe to do because we have the lock.
	}

	#pragma warning(pop)



	int EA::Thread::Mutex::Unlock()
	{
		EAT_ASSERT(mMutexData.mSysThreadId == EA::Thread::GetSysThreadId());
		EAT_ASSERT(mMutexData.mnLockCount > 0);

		const int nReturnValue(--mMutexData.mnLockCount); // This is safe to do because we have the lock.

		#if defined(EA_PLATFORM_WINDOWS)
			if(mMutexData.mbIntraProcess)
				LeaveCriticalSection((CRITICAL_SECTION*)mMutexData.mData);
			else
			{
				EAT_ASSERT(*(HANDLE*)mMutexData.mData != 0);
				ReleaseMutex(*(HANDLE*)mMutexData.mData);
			}
		#else
			LeaveCriticalSection((CRITICAL_SECTION*)mMutexData.mData);
		#endif

		return nReturnValue;
	}


	int EA::Thread::Mutex::GetLockCount() const
	{
		return mMutexData.mnLockCount;
	}


	bool EA::Thread::Mutex::HasLock() const
	{
		#if EAT_ASSERT_ENABLED
			return (mMutexData.mnLockCount > 0) && (mMutexData.mSysThreadId == EA::Thread::GetSysThreadId());
		#else
			return (mMutexData.mnLockCount > 0); // This is the best we can do, though it is of limited use, since it doesn't tell you if you are the thread with the lock.
		#endif
	}


#endif // EA_PLATFORM_XXX







