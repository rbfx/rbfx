///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/internal/config.h>
#include <eathread/eathread_rwmutex_ip.h>
#include <new> // include new for placement new operator
#include <string.h>


#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

	///////////////////////////////////////////////////////////////////////////
	// EARWMutexIPData
	///////////////////////////////////////////////////////////////////////////

	EA::Thread::EARWMutexIPData::EARWMutexIPData()
	  : mSharedData(),  // This still needs to be Init-ed.
		mMutex(NULL),
		mReadSemaphore(NULL),
		mWriteSemaphore(NULL)
	{
	}

	EA::Thread::EARWMutexIPData::~EARWMutexIPData()
	{
		// mSharedData.Shutdown(); // This shouldn't be necessary, as the SharedData dtor will do this itself.
	}

	bool EA::Thread::EARWMutexIPData::Init(const char* pName)
	{
		char mutexName[256];
		mutexName[0] = '\0';
		if(pName)
			strcpy(mutexName, pName);
		strcat(mutexName, ".Mutex");
		mMutex = CreateMutexA(NULL, FALSE, mutexName);

		char readSemaphoreName[256];
		readSemaphoreName[0] = '\0';
		if(pName)
			strcpy(readSemaphoreName, pName);
		strcat(readSemaphoreName, ".SemR");
		mReadSemaphore = CreateSemaphoreA(NULL, 0, 9999, readSemaphoreName);

		char writeSemaphoreName[256];
		writeSemaphoreName[0] = '\0';
		if(pName)
			strcpy(writeSemaphoreName, pName);
		strcat(writeSemaphoreName, ".SemW");
		mWriteSemaphore = CreateSemaphoreA(NULL, 0, 9999, writeSemaphoreName);

		return mSharedData.Init(pName);
	}

	void EA::Thread::EARWMutexIPData::Shutdown()
	{
		if(mMutex)
		{
			CloseHandle(mMutex);
			mMutex = NULL;
		}

		if(mReadSemaphore)
		{
			CloseHandle(mReadSemaphore);
			mReadSemaphore = NULL;
		}

		if(mWriteSemaphore)
		{
			CloseHandle(mWriteSemaphore);
			mWriteSemaphore = NULL;
		}

		mSharedData.Shutdown();
	}



	///////////////////////////////////////////////////////////////////////////
	// RWMutexIPParameters
	///////////////////////////////////////////////////////////////////////////

	EA::Thread::RWMutexIPParameters::RWMutexIPParameters(bool bIntraProcess, const char* pName)
		: mbIntraProcess(bIntraProcess)
	{
		#ifdef EA_PLATFORM_WINDOWS
			if(pName)
			{
				strncpy(mName, pName, sizeof(mName)-1);
				mName[sizeof(mName)-1] = 0;
			}
			else
				mName[0] = 0;
		#else
			(void)pName; // Suppress possible warnings.
		#endif
	}



	///////////////////////////////////////////////////////////////////////////
	// RWMutexIP
	///////////////////////////////////////////////////////////////////////////

	EA::Thread::RWMutexIP::RWMutexIP(const RWMutexIPParameters* pRWMutexIPParameters, bool bDefaultParameters)
	{
		if(!pRWMutexIPParameters && bDefaultParameters)
		{
			RWMutexIPParameters parameters;
			Init(&parameters);
		}
		else
			Init(pRWMutexIPParameters);
	}
	
	
	EA::Thread::RWMutexIP::~RWMutexIP()
	{
	}
	

	bool EA::Thread::RWMutexIP::Init(const RWMutexIPParameters* pRWMutexIPParameters)
	{
		if(pRWMutexIPParameters)
		{
			// Must provide a valid name for inter-process RWMutex.
			EAT_ASSERT(pRWMutexIPParameters->mbIntraProcess || pRWMutexIPParameters->mName[0]);

			return mRWMutexIPData.Init(pRWMutexIPParameters->mName);
		}

		return false;
	}


	int EA::Thread::RWMutexIP::Lock(LockType lockType, const ThreadTime& /*timeoutAbsolute*/)
	{
		int result = 0;

		WaitForSingleObject(mRWMutexIPData.mMutex, INFINITE); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.
		//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
	
		// We cannot obtain a write lock recursively, else we will deadlock.
		// Alternatively, we can build a bunch of extra logic to deal with this.
		EAT_ASSERT(mRWMutexIPData.mSharedData->mThreadIdWriter != ::GetCurrentThreadId());
	
		// Assert that there aren't both readers and writers at the same time.
		EAT_ASSERT(!((mRWMutexIPData.mSharedData->mThreadIdWriter != kSysThreadIdInvalid) && mRWMutexIPData.mSharedData->mnReaders));

		if(lockType == kLockTypeRead)
		{
			while(mRWMutexIPData.mSharedData->mThreadIdWriter != kSysThreadIdInvalid)
			{
				//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
	
				mRWMutexIPData.mSharedData->mnReadWaiters++;
				ReleaseMutex(mRWMutexIPData.mMutex);
				DWORD dwResult = WaitForSingleObject(mRWMutexIPData.mReadSemaphore, INFINITE); // To do: support timeoutAbsolute
				WaitForSingleObject(mRWMutexIPData.mMutex, INFINITE);
				mRWMutexIPData.mSharedData->mnReadWaiters--;
	
				EAT_ASSERT(dwResult != WAIT_FAILED);
				//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
	
				if(dwResult == WAIT_TIMEOUT)
				{
					ReleaseMutex(mRWMutexIPData.mMutex);
					return kResultTimeout;
				}
			}
	
			result = ++mRWMutexIPData.mSharedData->mnReaders; // This is not an atomic operation. We are within a mutex lock.
		}
		else if(lockType == kLockTypeWrite)
		{
			while((mRWMutexIPData.mSharedData->mnReaders > 0) ||                            // While somebody has the read lock or
				  (mRWMutexIPData.mSharedData->mThreadIdWriter != kSysThreadIdInvalid))     // somebody has the write lock... go back to waiting.
			{
				//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
	
				mRWMutexIPData.mSharedData->mnWriteWaiters++;
				ReleaseMutex(mRWMutexIPData.mMutex);
				DWORD dwResult = WaitForSingleObject(mRWMutexIPData.mWriteSemaphore, INFINITE); // To do: support timeoutAbsolute
				WaitForSingleObject(mRWMutexIPData.mMutex, INFINITE);
				mRWMutexIPData.mSharedData->mnWriteWaiters--;
	
				EAT_ASSERT(dwResult != WAIT_FAILED);
				//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
	
				if(dwResult == WAIT_TIMEOUT)
				{
					ReleaseMutex(mRWMutexIPData.mMutex);
					return kResultTimeout;
				}
			}
	
			result = 1;
			mRWMutexIPData.mSharedData->mThreadIdWriter = ::GetCurrentThreadId();
		}
	
		//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
		ReleaseMutex(mRWMutexIPData.mMutex);
	
		return result;
	}


	int EA::Thread::RWMutexIP::Unlock()
	{
		WaitForSingleObject(mRWMutexIPData.mMutex, INFINITE); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.
		//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);

		if(mRWMutexIPData.mSharedData->mThreadIdWriter != kSysThreadIdInvalid) // If we have a write lock...
		{
			EAT_ASSERT(mRWMutexIPData.mSharedData->mThreadIdWriter == ::GetCurrentThreadId());
			mRWMutexIPData.mSharedData->mThreadIdWriter = kSysThreadIdInvalid;
		}
		else // Else we have a read lock...
		{
			EAT_ASSERT(mRWMutexIPData.mSharedData->mnReaders >= 1);

			const int nNewReaders = --mRWMutexIPData.mSharedData->mnReaders; // This is not an atomic operation. We are within a mutex lock.
			if(nNewReaders > 0)
			{
				//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
				ReleaseMutex(mRWMutexIPData.mMutex);
				return nNewReaders;
			}
		}

		if(mRWMutexIPData.mSharedData->mnWriteWaiters > 0) // We ignore the possibility that 
		{
			ReleaseSemaphore(mRWMutexIPData.mWriteSemaphore, 1, NULL);
			// We rely on the released write waiter to decrement mnWriteWaiters.
			// If the released write waiter doesn't wake up for a while, it's possible that the ReleaseMutex below 
			// will be called and another read unlocker will execute this code and release the semaphore again and
			// we will have two writers that are released. But this isn't a problem because the released writers 
			// must still lock our mMutex and contend for the write lock, and one of the two will fail and go back
			// to waiting on the semaphore.
		}
		else if(mRWMutexIPData.mSharedData->mnReadWaiters > 0)
		{
			// I'm a little concerned about this signal here. We release mnReadWaiters, though it's possible
			// that a reader could timeout before this function completes and not all the semaphore count
			// will be claimed by waiters. However, the read wait code in the Lock function above does 
			// seem to be able to handle this case, as it does do a check to make sure it can hold the read
			// lock before it claims it.
			ReleaseSemaphore(mRWMutexIPData.mReadSemaphore, mRWMutexIPData.mSharedData->mnReadWaiters, NULL);
		}

		//EAT_ASSERT(mRWMutexIPData.mMutex.GetLockCount() == 1);
		ReleaseMutex(mRWMutexIPData.mMutex);

		return 0;
	}


	int EA::Thread::RWMutexIP::GetLockCount(LockType lockType)
	{
		if(lockType == kLockTypeRead)
			return mRWMutexIPData.mSharedData->mnReaders;
		else if((lockType == kLockTypeWrite) && (mRWMutexIPData.mSharedData->mThreadIdWriter != kSysThreadIdInvalid))
			return 1;
		return 0;
	}


#else

	EA::Thread::RWMutexIPParameters::RWMutexIPParameters(bool /*bIntraProcess*/, const char* /*pName*/)
	{
	}


	EA::Thread::RWMutexIP::RWMutexIP(const RWMutexIPParameters* /*pRWMutexIPParameters*/, bool /*bDefaultParameters*/)
	{
	}
	
	
	EA::Thread::RWMutexIP::~RWMutexIP()
	{
	}
	

	bool EA::Thread::RWMutexIP::Init(const RWMutexIPParameters* /*pRWMutexIPParameters*/)
	{
		return false;
	}


	int EA::Thread::RWMutexIP::Lock(LockType /*lockType*/, const ThreadTime& /*timeoutAbsolute*/)
	{
		return 0;
	}


	int EA::Thread::RWMutexIP::Unlock()
	{
		return 0;
	}


	int EA::Thread::RWMutexIP::GetLockCount(LockType /*lockType*/)
	{
		return 0;
	}

#endif // EA_PLATFORM_XXX




namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::RWMutexIP* EA::Thread::RWMutexIPFactory::CreateRWMutexIP()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::RWMutexIP))) EA::Thread::RWMutexIP;
	else
		return new EA::Thread::RWMutexIP;
}

void EA::Thread::RWMutexIPFactory::DestroyRWMutexIP(EA::Thread::RWMutexIP* pRWMutexIP)
{
	if(gpAllocator)
	{
		pRWMutexIP->~RWMutexIP();
		gpAllocator->Free(pRWMutexIP);
	}
	else
		delete pRWMutexIP;
}

size_t EA::Thread::RWMutexIPFactory::GetRWMutexIPSize()
{
	return sizeof(EA::Thread::RWMutexIP);
}

EA::Thread::RWMutexIP* EA::Thread::RWMutexIPFactory::ConstructRWMutexIP(void* pMemory)
{
	return new(pMemory) EA::Thread::RWMutexIP;
}

void EA::Thread::RWMutexIPFactory::DestructRWMutexIP(EA::Thread::RWMutexIP* pRWMutexIP)
{
	pRWMutexIP->~RWMutexIP();
}





