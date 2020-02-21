///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
	#pragma warning(disable: 4985)  // 'ceil': attributes not present on previous declaration.1>    C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\INCLUDE\intrin.h(142) : see declaration of 'ceil'
#endif


#include <eathread/internal/config.h>
#include <eathread/eathread_rwmutex.h>
#include <eathread/eathread.h>
#include <new> // include new for placement new operator
#include <string.h>

#ifdef _MSC_VER
	#pragma warning(disable : 4996) // This function or variable may be unsafe / deprecated.
#endif


	EARWMutexData::EARWMutexData()
	  : mnReadWaiters(0), 
		mnWriteWaiters(0), 
		mnReaders(0),
		mThreadIdWriter(EA::Thread::kThreadIdInvalid), 
		mMutex(NULL, false),
		mReadCondition(NULL, false),
		mWriteCondition(NULL, false)
	{
		// Empty
	}
	
	
	EA::Thread::RWMutexParameters::RWMutexParameters(bool bIntraProcess, const char* pName)
		: mbIntraProcess(bIntraProcess)
	{
		(void)pName; // Suppress possible warnings.

		#ifdef EA_PLATFORM_WINDOWS
			if(pName)
			{
				strncpy(mName, pName, sizeof(mName)-1);
				mName[sizeof(mName)-1] = 0;
			}
			else
				mName[0] = 0;
		#endif
	}
	
	
	EA::Thread::RWMutex::RWMutex(const RWMutexParameters* pRWMutexParameters, bool bDefaultParameters)
	{
		if(!pRWMutexParameters && bDefaultParameters)
		{
			RWMutexParameters parameters;
			Init(&parameters);
		}
		else
			Init(pRWMutexParameters);
	}
	
	
	EA::Thread::RWMutex::~RWMutex()
	{
		// Possibly do asserts here.
	}
	
	
	bool EA::Thread::RWMutex::Init(const RWMutexParameters* pRWMutexParameters)
	{
		if(pRWMutexParameters)
		{
			#if EATHREAD_MULTIPROCESSING_OS
				EAT_ASSERT(pRWMutexParameters->mbIntraProcess); // We don't currently have support for intra-process RWMutex on these platforms (and any multi-process platform). 
			#endif

			MutexParameters mup(pRWMutexParameters->mbIntraProcess);
			mRWMutexData.mMutex.Init(&mup);

			ConditionParameters mop(pRWMutexParameters->mbIntraProcess);
			mRWMutexData.mReadCondition.Init(&mop);
			mRWMutexData.mWriteCondition.Init(&mop);
			return true;
		}

		return false;
	}
	
	
	int EA::Thread::RWMutex::Lock(LockType lockType, const ThreadTime& timeoutAbsolute)
	{
		int result = 0;
	
		mRWMutexData.mMutex.Lock(); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.
		EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
		// We cannot obtain a write lock recursively, else we will deadlock.
		// Alternatively, we can build a bunch of extra logic to deal with this.
		EAT_ASSERT(mRWMutexData.mThreadIdWriter != GetThreadId());
	
		// Assert that there aren't both readers and writers at the same time.
		EAT_ASSERT(!((mRWMutexData.mThreadIdWriter != kThreadIdInvalid) && mRWMutexData.mnReaders));
	
		if(lockType == kLockTypeRead)
		{
			while(mRWMutexData.mThreadIdWriter != kThreadIdInvalid)
			{
				EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
				mRWMutexData.mnReadWaiters++;
				const Condition::Result mresult = mRWMutexData.mReadCondition.Wait(&mRWMutexData.mMutex, timeoutAbsolute);
				mRWMutexData.mnReadWaiters--;
	
				EAT_ASSERT(mresult != EA::Thread::Condition::kResultError);
				EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
				if(mresult == Condition::kResultTimeout)
				{
					mRWMutexData.mMutex.Unlock();
					return kResultTimeout;
				}
			}
	
			result = ++mRWMutexData.mnReaders; // This is not an atomic operation. We are within a mutex lock.
		}
		else if(lockType == kLockTypeWrite)
		{
			while((mRWMutexData.mnReaders > 0) || (mRWMutexData.mThreadIdWriter != kThreadIdInvalid))
			{
				EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
				mRWMutexData.mnWriteWaiters++;
				const Condition::Result mresult = mRWMutexData.mWriteCondition.Wait(&mRWMutexData.mMutex, timeoutAbsolute);
				mRWMutexData.mnWriteWaiters--;
	
				EAT_ASSERT(mresult != EA::Thread::Condition::kResultError);
				EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
				if(mresult == Condition::kResultTimeout)
				{
					mRWMutexData.mMutex.Unlock();
					return kResultTimeout;
				}
			}
	
			result = 1;
			mRWMutexData.mThreadIdWriter = GetThreadId();
		}
	
		EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
		mRWMutexData.mMutex.Unlock();
	
		return result;
	}
	
	
	int EA::Thread::RWMutex::Unlock()
	{
		mRWMutexData.mMutex.Lock(); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.
		EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
	
		if(mRWMutexData.mThreadIdWriter != kThreadIdInvalid)
		{
			EAT_ASSERT(mRWMutexData.mThreadIdWriter == GetThreadId());
	
			//Possibly enable this if we want some runtime error checking at some cost.
			//if(mRWMutexData.mThreadIdWriter == GetThreadId()){
			//    mRWMutexData.mMutex.Unlock();
			//    return kResultError;
			//}
	
			mRWMutexData.mThreadIdWriter = kThreadIdInvalid;
		}
		else
		{
			EAT_ASSERT(mRWMutexData.mnReaders >= 1);
	
			//Possibly enable this if we want some runtime error checking at some cost.
			//if(mRWMutexData.mnReaders < 1){
			//    mRWMutexData.mMutex.Unlock();
			//    return kResultError;
			//}
	
			const int nNewReaders = --mRWMutexData.mnReaders; // This is not an atomic operation. We are within a mutex lock.
			if(nNewReaders > 0)
			{
				EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
				mRWMutexData.mMutex.Unlock();
				return nNewReaders;
			}
		}
	
		if(mRWMutexData.mnWriteWaiters > 0)
			mRWMutexData.mWriteCondition.Signal(false);
		else if(mRWMutexData.mnReadWaiters > 0)
			mRWMutexData.mReadCondition.Signal(true);
	
		EAT_ASSERT(mRWMutexData.mMutex.GetLockCount() == 1);
		mRWMutexData.mMutex.Unlock();
	
		return 0;
	}
	
	
	int EA::Thread::RWMutex::GetLockCount(LockType lockType)
	{
		if(lockType == kLockTypeRead)
			return mRWMutexData.mnReaders;
		else if((lockType == kLockTypeWrite) && (mRWMutexData.mThreadIdWriter != kThreadIdInvalid))
			return 1;
		return 0;
	}




namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::RWMutex* EA::Thread::RWMutexFactory::CreateRWMutex()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::RWMutex))) EA::Thread::RWMutex;
	else
		return new EA::Thread::RWMutex;
}

void EA::Thread::RWMutexFactory::DestroyRWMutex(EA::Thread::RWMutex* pRWMutex)
{
	if(gpAllocator)
	{
		pRWMutex->~RWMutex();
		gpAllocator->Free(pRWMutex);
	}
	else
		delete pRWMutex;
}

size_t EA::Thread::RWMutexFactory::GetRWMutexSize()
{
	return sizeof(EA::Thread::RWMutex);
}

EA::Thread::RWMutex* EA::Thread::RWMutexFactory::ConstructRWMutex(void* pMemory)
{
	return new(pMemory) EA::Thread::RWMutex;
}

void EA::Thread::RWMutexFactory::DestructRWMutex(EA::Thread::RWMutex* pRWMutex)
{
	pRWMutex->~RWMutex();
}





