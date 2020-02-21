///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_futex.h>
#include <new>

#if defined(EA_THREAD_NONTHREADED_FUTEX) && EA_THREAD_NONTHREADED_FUTEX

	void EA::Thread::Futex::CreateFSemaphore()
	{
		mSemaphore.mnCount = 0;
	}

	void EA::Thread::Futex::DestroyFSemaphore()
	{
		// Do nothing;
	}

	void EA::Thread::Futex::SignalFSemaphore()
	{
		mSemaphore.mnCount++;
	}

	void EA::Thread::Futex::WaitFSemaphore()
	{
		while(mSemaphore.mnCount <= 0)
			EA_THREAD_DO_SPIN();
		mSemaphore.mnCount--;
	}

	bool EA::Thread::Futex::WaitFSemaphore(const ThreadTime&)
	{
		WaitFSemaphore();
		return true;
	}

#elif defined(__APPLE__) && EATHREAD_MANUAL_FUTEX_ENABLED
	#include <semaphore.h>
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <libkern/OSAtomic.h>

	void EA::Thread::Futex::CreateFSemaphore()
	{   
		mSemaphore.Init(0);
	}

	void EA::Thread::Futex::DestroyFSemaphore()
	{
		// Do nothing;
	}

	void EA::Thread::Futex::SignalFSemaphore()
	{
		mSemaphore.Post();
	}

	void EA::Thread::Futex::WaitFSemaphore()
	{
		mSemaphore.Wait();
	}

	bool EA::Thread::Futex::WaitFSemaphore(const ThreadTime& timeoutAbsolute)
	{
		return (mSemaphore.Wait(timeoutAbsolute) >= 0);
	}

#elif defined(EA_PLATFORM_SONY) && !EATHREAD_MANUAL_FUTEX_ENABLED
	#include <kernel.h>	
	#include <eathread/eathread_atomic.h>

	EA::Thread::Futex::Futex()
	: mSpinCount(EATHREAD_FUTEX_SPIN_COUNT)
	{
	}

	EA::Thread::Futex::~Futex()
	{
	}

	void EA::Thread::Futex::Lock()
	{
		Uint spinCount(mSpinCount);
		while(--spinCount)
		{
			if(TryLock())
				return;
		}

		mMutex.Lock();
	}

	void EA::Thread::Futex::Unlock()
	{
		mMutex.Unlock();
	}

	bool EA::Thread::Futex::TryLock()
	{
		if(mMutex.Lock(EA::Thread::kTimeoutImmediate) > 0)  // This calls scePthreadMutexTrylock
			return true;

		return false;
	}

	int EA::Thread::Futex::Lock(const ThreadTime& timeoutAbsolute)
	{ 
		return mMutex.Lock(timeoutAbsolute); 
	}

	int EA::Thread::Futex::GetLockCount() const
	{
		return mMutex.GetLockCount();
	}  

	bool EA::Thread::Futex::HasLock() const
	{
		return mMutex.HasLock();
	}

	void EA::Thread::Futex::SetSpinCount(Uint spinCount)
	{ 
		mSpinCount = spinCount;
	}

#elif defined(EA_PLATFORM_SONY) && EATHREAD_MANUAL_FUTEX_ENABLED
	#include <kernel/semaphore.h>
	#include <sceerror.h>

	void EA::Thread::Futex::CreateFSemaphore()
	{
		// To consider: Copy the Futex name into this semaphore name.
		int result = sceKernelCreateSema(&mSemaphore, "Futex", SCE_KERNEL_SEMA_ATTR_TH_FIFO, 0, 100000, NULL);
		EA_UNUSED(result);
		EAT_ASSERT(result == SCE_OK);
	}

	void EA::Thread::Futex::DestroyFSemaphore()
	{
		int result = sceKernelDeleteSema(mSemaphore);
		EA_UNUSED(result);
		EAT_ASSERT(result == SCE_OK);
	}

	void EA::Thread::Futex::SignalFSemaphore()
	{
	   int result = sceKernelSignalSema(mSemaphore, 1);
		EA_UNUSED(result);
		EAT_ASSERT(result == SCE_OK);
	}

	void EA::Thread::Futex::WaitFSemaphore()
	{
		int result = sceKernelWaitSema(mSemaphore, 1, NULL);
		EA_UNUSED(result);
		EAT_ASSERT(result == SCE_OK);
	}

	bool EA::Thread::Futex::WaitFSemaphore(const ThreadTime& timeoutAbsolute)
	{
		SceKernelUseconds timeoutRelativeUs = static_cast<SceKernelUseconds>(RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));        
		if(timeoutRelativeUs < 1)
			timeoutRelativeUs = 1;

		return (sceKernelWaitSema(mSemaphore, 1, &timeoutRelativeUs) == SCE_OK);
	}

#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EATHREAD_MANUAL_FUTEX_ENABLED
	#include <semaphore.h>
	#include <errno.h>

	void EA::Thread::Futex::CreateFSemaphore()
	{   
		const int result = sem_init(&mSemaphore, 0, 0);
		(void)result;
		EAT_ASSERT(result != -1);
	}

	void EA::Thread::Futex::DestroyFSemaphore()
	{
		#if defined (__APPLE__)
			sem_close(&mSemaphore);
		#elif defined(EA_PLATFORM_ANDROID)
			sem_destroy(&mSemaphore);   // Android's sem_destroy is broken. http://code.google.com/p/android/issues/detail?id=3106
		#else
			int result = -1;
	
			for(;;)
			{
				result = sem_destroy(&mSemaphore);

				if((result == -1) && (errno == EBUSY)) // If another thread or process is blocked on this semaphore...
					ThreadSleep(kTimeoutYield);        // Yield. If we don't yield, it's possible we could block other other threads or processes from running, on some systems.
				else
					break;
			}

			EAT_ASSERT(result != -1);
		#endif
	}

	void EA::Thread::Futex::SignalFSemaphore()
	{
		sem_post(&mSemaphore);
	}

	void EA::Thread::Futex::WaitFSemaphore()
	{
		// We don't have much choice but to retry interrupted waits,
		// as there is no lock failure return value.
		while((sem_wait(&mSemaphore) == -1) && (errno == EINTR))
			continue;
	}

	bool EA::Thread::Futex::WaitFSemaphore(const ThreadTime&)
	{
		WaitFSemaphore();
		return true;
	}

#elif defined(EA_PLATFORM_MICROSOFT) && !EA_USE_CPP11_CONCURRENCY && !EATHREAD_MANUAL_FUTEX_ENABLED

	#pragma warning(push, 0)
	#include <Windows.h>
	#pragma warning(pop)

	// Validate what we assume to be invariants.
	EAT_COMPILETIME_ASSERT(sizeof(CRITICAL_SECTION) <= (EA::Thread::FUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t) * sizeof(uint64_t)));

	#if defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64)
		EAT_COMPILETIME_ASSERT(offsetof(CRITICAL_SECTION, RecursionCount) == (3 * sizeof(int)));
		EAT_COMPILETIME_ASSERT(offsetof(CRITICAL_SECTION, OwningThread)   == (4 * sizeof(int)));
	#elif defined(EA_PLATFORM_WIN32)
		EAT_COMPILETIME_ASSERT(offsetof(CRITICAL_SECTION, RecursionCount) == (2 * sizeof(int)));
		EAT_COMPILETIME_ASSERT(offsetof(CRITICAL_SECTION, OwningThread)   == (3 * sizeof(int)));
	#else
		EAT_FAIL_MSG("Need to verify offsetof.");
	#endif


#elif defined(EA_PLATFORM_MICROSOFT) && EATHREAD_MANUAL_FUTEX_ENABLED

	#if defined(EA_PLATFORM_WINDOWS)
		#pragma warning(push, 0)
		#include <Windows.h>
		#pragma warning(pop)
	#endif

	void EA::Thread::Futex::CreateFSemaphore()
	{
		mSemaphore = CreateSemaphoreA(NULL, 0, INT_MAX / 2, NULL);
		EAT_ASSERT(mSemaphore != 0);
	}

	void EA::Thread::Futex::DestroyFSemaphore()
	{
		if(mSemaphore)
			CloseHandle(mSemaphore);
	}

	void EA::Thread::Futex::SignalFSemaphore()
	{
		ReleaseSemaphore(mSemaphore, 1, NULL);
	}

	void EA::Thread::Futex::WaitFSemaphore()
	{
		WaitForSingleObject(mSemaphore, INFINITE);
	}

	bool EA::Thread::Futex::WaitFSemaphore(const ThreadTime& timeoutAbsolute)
	{
		int64_t timeoutRelativeMS = (int64_t)(timeoutAbsolute - GetThreadTime());
		if(timeoutRelativeMS < 1)
			timeoutRelativeMS = 1;
		return WaitForSingleObject(mSemaphore, (DWORD)timeoutRelativeMS) == WAIT_OBJECT_0;
	}

#endif





namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::Futex* EA::Thread::FutexFactory::CreateFutex()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::Futex))) EA::Thread::Futex;
	else
		return new EA::Thread::Futex;
}

void EA::Thread::FutexFactory::DestroyFutex(EA::Thread::Futex* pFutex)
{
	if(gpAllocator)
	{
		pFutex->~Futex();
		gpAllocator->Free(pFutex);
	}
	else
		delete pFutex;
}

size_t EA::Thread::FutexFactory::GetFutexSize()
{
	return sizeof(EA::Thread::Futex);
}

EA::Thread::Futex* EA::Thread::FutexFactory::ConstructFutex(void* pMemory)
{
	return new(pMemory) EA::Thread::Futex;
}

void EA::Thread::FutexFactory::DestructFutex(EA::Thread::Futex* pFutex)
{
	pFutex->~Futex();
}







