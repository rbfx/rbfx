///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EABase/eabase.h"
#include "eathread/eathread_semaphore.h"
#include "eathread/eathread_sync.h"
#include <limits.h>

#if defined(EA_PLATFORM_MICROSOFT)
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <Windows.h>
	EA_RESTORE_ALL_VC_WARNINGS()
	#if defined(EA_PLATFORM_WIN64)
		#if !defined _Ret_maybenull_
			#define _Ret_maybenull_
		#endif

		#if !defined _Reserved_
			#define _Reserved_
		#endif
		extern "C" WINBASEAPI _Ret_maybenull_ HANDLE WINAPI CreateSemaphoreExW(_In_opt_ LPSECURITY_ATTRIBUTES, _In_ LONG, _In_ LONG, _In_opt_ LPCWSTR, _Reserved_ DWORD, _In_ DWORD);    
	#endif
#endif
#ifdef CreateSemaphore
	#undef CreateSemaphore
#endif



#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE

	// Helper function to abstract away differences between APIs for different versions of Windows
	static DWORD EASemaphoreWaitForSingleObject(HANDLE handle, DWORD milliseconds)
	{
		#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			return WaitForSingleObject(handle, milliseconds);
		#else
			return WaitForSingleObjectEx(handle, milliseconds, TRUE);
		#endif
	}

	EASemaphoreData::EASemaphoreData()
		: mhSemaphore(0), mnCount(0), mnCancelCount(0), mnMaxCount(INT_MAX), mbIntraProcess(true)
	{
		EAWriteBarrier();
		static_assert(sizeof(int32_t) == sizeof(LONG), "We use int32_t and LONG interchangably. Windows (including Win64) uses 32 bit longs.");
	}


	void EASemaphoreData::UpdateCancelCount(int32_t cancelCount)
	{
		// This is used by the fast semaphore path. This function actually isn't called very often -- only under uncommon circumstances.
		// This is based on an algorithm discussed on usenet in 2004.
		// We safely increment count by min(cancelCount, -count) if count < 0.
		int32_t oldCount, newCount, cmpCount;

		if(cancelCount > 0)
		{
			oldCount = mnCount;

			while(oldCount < 0)
			{
				// Increment old count by the number of cancels
				if((newCount = oldCount + cancelCount) > 0)
					newCount = 0; // ...but not greater then zero.

				cmpCount = oldCount;
				oldCount = InterlockedCompareExchange((LONG*)&mnCount, newCount, cmpCount);

				if(oldCount == cmpCount)
				{
					cancelCount -= (newCount - oldCount);
					break;
				}
			}

			if(cancelCount > 0)
				InterlockedExchangeAdd((LONG*)&mnCancelCount, cancelCount);
		} 
	}


	EA::Thread::SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char* pName)
		: mInitialCount(initialCount), mMaxCount(INT_MAX), mbIntraProcess(bIntraProcess)
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
		if(mSemaphoreData.mhSemaphore)
			CloseHandle(mSemaphoreData.mhSemaphore);
	}


	bool EA::Thread::Semaphore::Init(const SemaphoreParameters* pSemaphoreParameters)
	{
		if(pSemaphoreParameters && !mSemaphoreData.mhSemaphore)
		{
			mSemaphoreData.mnCount    = pSemaphoreParameters->mInitialCount;
			mSemaphoreData.mnMaxCount = pSemaphoreParameters->mMaxCount;

			if(mSemaphoreData.mnCount < 0)
				mSemaphoreData.mnCount = 0;

			mSemaphoreData.mbIntraProcess = pSemaphoreParameters->mbIntraProcess;

			// If the fast semaphore is disabled, then we always act like inter-process as opposed to intra-process.
			#if EATHREAD_FAST_MS_SEMAPHORE_ENABLED
				const bool bIntraProcess = mSemaphoreData.mbIntraProcess;
			#else
				const bool bIntraProcess = false;
			#endif

			if(bIntraProcess)
			{
				// Note that we do things rather differently for intra-process, as we are 
				// implementing a fast semaphore. This semaphore will be at least 10 times 
				// faster than the OS semaphore for all Microsoft platforms for the case of
				// successful immediate acquire of a semaphore. Semaphore posts (or releases
				// will also be much faster than the OS version.
				#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
					mSemaphoreData.mhSemaphore = CreateSemaphoreA(NULL, 0, INT_MAX/2, NULL); // Intentionally ignore mnCount and mnMaxCount here.
				#else
					mSemaphoreData.mhSemaphore = CreateSemaphoreExW(NULL, 0, INT_MAX/2, NULL, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE); // Intentionally ignore mnCount and mnMaxCount here.
				#endif
			}
			else // Else we create a conventional Win32-style semaphore.
			{
				#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
					mSemaphoreData.mhSemaphore = CreateSemaphoreA(NULL, (LONG)mSemaphoreData.mnCount, (LONG)mSemaphoreData.mnMaxCount, pSemaphoreParameters->mName[0] ? pSemaphoreParameters->mName : NULL);
				#else
					wchar_t wName[EAArrayCount(pSemaphoreParameters->mName)]; // We do an ASCII conversion.
					for(size_t c = 0; c < EAArrayCount(wName); c++)
						wName[c] = (wchar_t)(uint8_t)pSemaphoreParameters->mName[c];
					mSemaphoreData.mhSemaphore = CreateSemaphoreExW(NULL, (LONG)mSemaphoreData.mnCount, (LONG)mSemaphoreData.mnMaxCount, wName[0] ? wName : NULL, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
				#endif
			}

			EAWriteBarrier();
			EAT_ASSERT(mSemaphoreData.mhSemaphore != 0);
			return (mSemaphoreData.mhSemaphore != 0);
		}
		return false;
	}


	int EA::Thread::Semaphore::Wait(const ThreadTime& timeoutAbsolute)
	{
		EAT_ASSERT(mSemaphoreData.mhSemaphore != 0);

		// If the fast semaphore is disabled, then we always act like inter-process as opposed to intra-process.
		#if EATHREAD_FAST_MS_SEMAPHORE_ENABLED
			const bool bIntraProcess = mSemaphoreData.mbIntraProcess;
		#else
			const bool bIntraProcess = false;
		#endif

		if(bIntraProcess) // If this is true, we are using the fast semaphore pathway.
		{
			if(InterlockedDecrement((LONG*)&mSemaphoreData.mnCount) < 0) // InterlockedDecrement returns the new value. If the mnCount was > 0 before this decrement, then this Wait function will return very quickly.
			{
				const DWORD dw = EASemaphoreWaitForSingleObject(mSemaphoreData.mhSemaphore, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));

				if(dw != WAIT_OBJECT_0) // If there was a timeout...
				{
					mSemaphoreData.UpdateCancelCount(1); // or InterlockedIncrement(&mSemaphoreData.mnCancelCount); // The latter has a bug whereby mnCancelCount can increment indefinitely.

					EAT_ASSERT(dw == WAIT_TIMEOUT); // Otherwise it was probably a timeout.
					if(dw == WAIT_TIMEOUT)
						return kResultTimeout;
					return kResultError; // WAIT_FAILED
				}
			}

			// It is by design that a semaphore post does a full memory barrier. 
			// We don't need such a barrier for this pathway to work, but rather
			// it is expected by the user that such a barrier is executed. Investigation
			// into the choice of a full vs. just read or write barrier has concluded 
			// (based on the Posix standard) that a full read-write barrier is expected.
			EAReadWriteBarrier();

			const int count = (int)mSemaphoreData.mnCount; // Make temporary to avoid race condition in ternary operator below.
			return (count > 0 ? count : 0);
		}
		else
		{
			const DWORD dw = EASemaphoreWaitForSingleObject(mSemaphoreData.mhSemaphore, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));

			if(dw == WAIT_OBJECT_0)
				return (int)InterlockedDecrement((LONG*)&mSemaphoreData.mnCount);
			else if(dw == WAIT_TIMEOUT)
				return kResultTimeout;
			return kResultError;
		}
	}


	int EA::Thread::Semaphore::Post(int count)
	{
		EAT_ASSERT((mSemaphoreData.mhSemaphore != 0) && (count >= 0));

		if(count > 0)
		{
			// If the fast semaphore is disabled, then we always act like inter-process as opposed to intra-process.
			#if EATHREAD_FAST_MS_SEMAPHORE_ENABLED
				const bool bIntraProcess = mSemaphoreData.mbIntraProcess;
			#else
				const bool bIntraProcess = false;
			#endif

			if(bIntraProcess) // If this is true, we are using the fast semaphore pathway.
			{
				// It is by design that a semaphore post does a full memory barrier. 
				// We don't need such a barrier for this pathway to work, but rather
				// it is expected by the user that such a barrier is executed. Investigation
				// into the choice of a full vs. just read or write barrier has concluded 
				// (based on the Posix standard) that a full read-write barrier is expected.
				EAReadWriteBarrier();

				if((mSemaphoreData.mnCancelCount > 0) && (mSemaphoreData.mnCount < 0)) // Much of the time this will evaluate to false due to the first condition.
					mSemaphoreData.UpdateCancelCount(InterlockedExchange((LONG*)&mSemaphoreData.mnCancelCount, 0)); // It's possible that mnCancelCount may have decremented down to zero between the previous line of code and this line of code.

				const int currentCount = mSemaphoreData.mnCount;
				if((mSemaphoreData.mnMaxCount - count) < currentCount)  // If count would cause an overflow...
					return kResultError; // We do what most OS implementations of max-count do. count = (mSemaphoreData.mnMaxCount - currentCount);

				const int32_t nWaiterCount = -InterlockedExchangeAdd((LONG*)&mSemaphoreData.mnCount, count); // InterlockedExchangeAdd returns the initial value of mnCount. If it's below zero, then it's a count of waiters.
				const int     nNewCount    = count - nWaiterCount;

				if(nWaiterCount > 0)  // If there were waiters blocking...
				{
					const int32_t nReleaseCount = (count < nWaiterCount) ? count : nWaiterCount; // Call ReleaseSemaphore for as many waiters as possible.
					ReleaseSemaphore(mSemaphoreData.mhSemaphore, nReleaseCount, NULL); // Note that by the time this executes, nReleaseCount might be > than the actual number of waiting threads, due to timeouts.
				}

				return (nNewCount > 0 ? nNewCount : 0);
			}
			else
			{
				const int32_t nPreviousCount = InterlockedExchangeAdd((LONG*)&mSemaphoreData.mnCount, count);
				const int     nNewCount      = nPreviousCount + count;

				const BOOL result = ReleaseSemaphore(mSemaphoreData.mhSemaphore, count, NULL);

				if(!result)
				{
					InterlockedExchangeAdd((LONG*)&mSemaphoreData.mnCount, -count);
					EAT_ASSERT(result);
					return kResultError;
				}

				return nNewCount;
			}
		}

		return (int)mSemaphoreData.mnCount; // We don't worry if this value is changing. There is nothing that you can rely upon about this value anyway.
	}


	int EA::Thread::Semaphore::GetCount() const
	{
		// Under our fast pathway, mnCount can go below zero.
		// Under the fast pathway, we need to add mnCancelCount to mnCount because mnCount is negative by the number of waiters and mnCancelCount is the number of waiters that have abandoned waiting but the value hasn't been rolled back into mnCount yet.
		EAReadBarrier();
		const int count = (int)mSemaphoreData.mnCount + (int)mSemaphoreData.mnCancelCount; // Make temporary to avoid race condition in ternary operator below.
		return (count > 0 ? count : 0);
	}

#endif // EA_PLATFORM_XXX







