///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/internal/config.h>
#include <eathread/eathread_pool.h>
#include <eathread/eathread_sync.h>
#include <string.h>
#include <new>

#if   defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 6011) // Dereferencing NULL pointer 'gpAllocator'
	#pragma warning(disable: 6211) // Leaking memory 'pThreadInfo' due to an exception.
	#pragma warning(disable: 6326) // Potential comparison of a constant with another constant
#endif


namespace EA
{
	namespace Thread
	{
		extern Allocator* gpAllocator;
	}
}


EA::Thread::ThreadPoolParameters::ThreadPoolParameters()
  : mnMinCount(EA::Thread::ThreadPool::kDefaultMinCount),
	mnMaxCount(EA::Thread::ThreadPool::kDefaultMaxCount),
	mnInitialCount(EA::Thread::ThreadPool::kDefaultInitialCount),
	mnIdleTimeoutMilliseconds(EA::Thread::ThreadPool::kDefaultIdleTimeout), // This is a relative time, not an absolute time. Can be a millisecond value or Thread::kTimeoutNone or Thread::kTimeoutImmediate.
	mnProcessorMask(0xffffffff),
	mDefaultThreadParameters()
{
	// Empty
}


EA::Thread::ThreadPool::Job::Job()
  : mpRunnable(NULL), mpFunction(NULL), mpContext(NULL)
{
	// Empty
}


EA::Thread::ThreadPool::ThreadInfo::ThreadInfo()
  : mbActive(false),
	mbQuit(false),
  //mbPersistent(false),
	mpThread(NULL),
	mpThreadPool(NULL),
	mCurrentJob()
{
	// Empty
}


EA::Thread::ThreadPool::ThreadPool(const ThreadPoolParameters* pThreadPoolParameters, bool bDefaultParameters)
  : mbInitialized(false),
	mnMinCount(kDefaultMinCount), 
	mnMaxCount(kDefaultMaxCount), 
	mnCurrentCount(0),
	mnActiveCount(0),
	mnIdleTimeoutMilliseconds(kDefaultIdleTimeout),
	mnProcessorMask((unsigned)kDefaultProcessorMask),
	mnProcessorCount(0),
	mnNextProcessor(0),
	mnPauseCount(0),
	mnLastJobID(0),
	mDefaultThreadParameters(),
	mThreadCondition(NULL, false),  // Explicitly don't initialize.
	mThreadMutex(NULL, false),      // Explicitly don't initialize.
	mThreadInfoList(),
	mJobList()
{
	if(!pThreadPoolParameters && bDefaultParameters)
	{
		ThreadPoolParameters parameters;
		Init(&parameters);
	}
	else
		Init(pThreadPoolParameters);
}


EA::Thread::ThreadPool::~ThreadPool()
{
	Shutdown(kJobWaitAll, kTimeoutNone);
	EAT_ASSERT(mJobList.empty() && mThreadInfoList.empty() && (mnCurrentCount == 0) && (mnActiveCount == 0) && (mThreadMutex.GetLockCount() == 0));
}


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4296 4706) // '>=' : expression is always true and assignment within conditional expression (in the assert)
#endif


#if EAT_ASSERT_ENABLED
	template <class T>
	inline bool EATIsUnsigned(T)
	{ return (((T)(-1) >> 1) != (T)(-1)); }
#endif


// If mDefaultThreadParameters.mnProcessor is set to kThreadPoolParametersProcessorDefault,  
// then the ThreadPool controls what processors the thread executes on. Otherwise ThreadPool 
// doesn't set the thread affinity itself.
static const int kThreadPoolParametersProcessorDefault = -1;


bool EA::Thread::ThreadPool::Init(const ThreadPoolParameters* pThreadPoolParameters)
{
	if(!mbInitialized)
	{
		if(pThreadPoolParameters && (mnCurrentCount == 0))
		{
			mbInitialized = true;

			mnMinCount                = pThreadPoolParameters->mnMinCount;
			mnMaxCount                = pThreadPoolParameters->mnMaxCount;
			mnCurrentCount            = (int)pThreadPoolParameters->mnInitialCount;
			mnIdleTimeoutMilliseconds = pThreadPoolParameters->mnIdleTimeoutMilliseconds;
			mnProcessorMask           = pThreadPoolParameters->mnProcessorMask;
			mDefaultThreadParameters  = pThreadPoolParameters->mDefaultThreadParameters;
			mnProcessorCount          = (uint32_t)EA::Thread::GetProcessorCount();  // We currently assume this value is constant at runtime.

			// Do bounds checking. 
			//if(mnMinCount < 0)  // This check is unnecessary because mnMinCount is of an 
			//    mnMinCount = 0; // unsigned data type. We assert for this unsigned-ness below.
			EAT_ASSERT(EATIsUnsigned(mnMinCount));

			if(mnMaxCount > EA_THREAD_POOL_MAX_SIZE)
				mnMaxCount = EA_THREAD_POOL_MAX_SIZE;

			if(mnCurrentCount < (int)mnMinCount)
				mnCurrentCount = (int)mnMinCount;

			if(mnCurrentCount > (int)mnMaxCount)
				mnCurrentCount = (int)mnMaxCount;

			// Make sure the processor mask refers to existing processors.
			const int processorMask  = (1 << mnProcessorCount) - 1;       // So for a processor count of 8 we have a mask of 11111111 (255)

			if((mnProcessorMask & processorMask) == 0) 
				mnProcessorMask = 0xffffffff;

			mDefaultThreadParameters.mpStack = NULL;  // You can't specify a default stack location, as every thread needs a unique one.
			if(mDefaultThreadParameters.mnProcessor != EA::Thread::kProcessorAny)               // If the user hasn't set threads to execute on any processor chosen by the OS...
				mDefaultThreadParameters.mnProcessor = kThreadPoolParametersProcessorDefault;   //   then use our default processing, which is for us to currently round-robin the processor used.

			ConditionParameters mnp;
			mThreadCondition.Init(&mnp);

			MutexParameters mtp;
			mThreadMutex.Init(&mtp);

			mThreadMutex.Lock();
			const int nDesiredCount((int)mnCurrentCount);
			mnCurrentCount = 0;
			AdjustThreadCount((unsigned int)nDesiredCount);
			mThreadMutex.Unlock();

			return true;
		}
	}
	return false;
}


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


bool EA::Thread::ThreadPool::Shutdown(JobWait jobWait, const ThreadTime& timeoutAbsolute)
{
	int nResult;

	if(mbInitialized)
	{
		mbInitialized = false;

		nResult = WaitForJobCompletion(-1, jobWait, timeoutAbsolute);

		mThreadMutex.Lock();

		// If jobWait is kJobWaitNone, then we nuke all existing jobs.
		if(jobWait == kJobWaitNone)
			mJobList.clear();

		// Leave a message to tell the thread to quit.
		for(ThreadInfoList::iterator it(mThreadInfoList.begin()), itEnd(mThreadInfoList.end()); it != itEnd; )
		{
			ThreadInfo* const pThreadInfo = *it;

			pThreadInfo->mbQuit       = true;
		  //pThreadInfo->mbPersistent = false;

			// If somehow the thread isn't running (possibly because it never started), manually remove it.
			if(pThreadInfo->mpThread->GetStatus() != EA::Thread::Thread::kStatusRunning)
				it = mThreadInfoList.erase(it);
			else
				++it;
		}

		// Wake up any threads that may be blocked on a condition variable wait.
		mThreadCondition.Signal(true);

		// Make sure we unlock after we signal, lest there be a certain kind of race condition.
		mThreadMutex.Unlock();

		// Wait for any existing threads to quit.
		// Todo: Replace this poor polling loop with Thread::Wait calls.
		//         Doing so requires a little finessing with the thread 
		//         objects in the list. Possibly make ThreadInfo ref-counted.
		while(!mThreadInfoList.empty())
		{
			ThreadSleep(1);
			EAReadBarrier();
		}

		mThreadMutex.Lock();
		mnPauseCount = 0;
		mThreadMutex.Unlock();
	}
	else
		nResult = kResultOK;

	return (nResult == kResultOK);
}


intptr_t EA::Thread::ThreadPool::ThreadFunction(void* pContext)
{
	ThreadInfo* const pThreadInfo = reinterpret_cast<ThreadInfo*>(pContext);
	ThreadPool* const pThreadPool = pThreadInfo->mpThreadPool;
	Condition*  const pCondition  = &pThreadPool->mThreadCondition;
	Mutex*      const pMutex      = &pThreadPool->mThreadMutex;

	pMutex->Lock();

	while(!pThreadInfo->mbQuit)
	{
		if(!pThreadPool->mJobList.empty())
		{
			pThreadInfo->mCurrentJob = pThreadPool->mJobList.front();
			pThreadPool->mJobList.pop_front();
			pThreadInfo->mbActive = true;
			++pThreadPool->mnActiveCount; // Atomic integer operation.
			pMutex->Unlock();

			// Do the job here. It's important that we keep the mutex unlocked while doing the job.
			if(pThreadInfo->mCurrentJob.mpRunnable)
				pThreadInfo->mCurrentJob.mpRunnable->Run(pThreadInfo->mCurrentJob.mpContext);
			else if(pThreadInfo->mCurrentJob.mpFunction)
				pThreadInfo->mCurrentJob.mpFunction(pThreadInfo->mCurrentJob.mpContext);
			else
				pThreadInfo->mbQuit = true;  // Tell ourself to quit.

			// Problem: We are not paying attention to the pThreadInfo->mbPersistent variable. 
			// We don't have an easy way of dealing with it because we don't have a means for
			// the ThreadPool to direct quit commands to individual threads. For now we don't
			// pay attention to mbPersistent and require that persistence be controlled by 
			// the min/max thread count settings. 

			pMutex->Lock();

			--pThreadPool->mnActiveCount; // Atomic integer operation.
			pThreadInfo->mbActive = false;
		}
		else
		{
			// The wait call here will unlock the condition variable and will re-lock it upon return.
			EA::Thread::ThreadTime timeoutAbsolute = (GetThreadTime() + pThreadPool->mnIdleTimeoutMilliseconds);

			if (pThreadPool->mnIdleTimeoutMilliseconds == kTimeoutNone) 
				timeoutAbsolute = kTimeoutNone;
			else if(pThreadPool->mnIdleTimeoutMilliseconds == kTimeoutImmediate)
				timeoutAbsolute = kTimeoutImmediate;
			else if(timeoutAbsolute == kTimeoutNone) // If it coincidentally is the magic kTimeoutNone value...
				timeoutAbsolute -= 1;

			const Condition::Result result = pCondition->Wait(pMutex, timeoutAbsolute);

			if(result != Condition::kResultOK) // If result is an error then what do we do? Is there a 
				pThreadInfo->mbQuit = true;    // specific reason to quit? There's no good solution here,
		}                                      // but on the other hand this should never happen in practice.
	}

	pThreadPool->RemoveThread(pThreadInfo);

	pMutex->Unlock();

	return 0;
}


EA::Thread::ThreadPool::Result EA::Thread::ThreadPool::QueueJob(const Job& job, Thread** ppThread, bool /*bEnableDeferred*/)
{
	if(mbInitialized){
		mThreadMutex.Lock();

		// If there are other threads busy with jobs or other threads soon to be busy with jobs and if the thread count is less than the maximum allowable, bump up the thread count by one.
		EAT_ASSERT(mnActiveCount <= mnCurrentCount);
		if((((int)mnActiveCount >= mnCurrentCount) || !mJobList.empty()) && (mnCurrentCount < (int)mnMaxCount))
			AdjustThreadCount((unsigned)(mnCurrentCount + 1));

		mJobList.push_back(job);
		FixThreads();

		if(mnPauseCount == 0)
			mThreadCondition.Signal(false); // Wake up one thread to work on this.

		mThreadMutex.Unlock();

		if(ppThread){
			// In this case the caller wants to know what thread got the job. 
			// So we wait until we know what caller got the job.
			// Todo: Complete this.
			*ppThread = NULL;
		}

		return kResultDeferred;
	}

	return kResultError;
}


int EA::Thread::ThreadPool::Begin(IRunnable* pRunnable, void* pContext, Thread** ppThread, bool bEnableDeferred)
{
	Job job;
	job.mnJobID    = mnLastJobID.Increment();
	job.mpRunnable = pRunnable;
	job.mpFunction = NULL;
	job.mpContext  = pContext;

	if(QueueJob(job, ppThread, bEnableDeferred) != kResultError)
		return job.mnJobID;
	return kResultError;
}


int EA::Thread::ThreadPool::Begin(RunnableFunction pFunction, void* pContext, Thread** ppThread, bool bEnableDeferred)
{
	Job job;
	job.mnJobID     = mnLastJobID.Increment();
	job.mpRunnable = NULL;
	job.mpFunction = pFunction;
	job.mpContext  = pContext;

	if(QueueJob(job, ppThread, bEnableDeferred) != kResultError)
		return job.mnJobID;
	return kResultError;
}


int EA::Thread::ThreadPool::WaitForJobCompletion(int nJob, JobWait jobWait, const ThreadTime& timeoutAbsolute)
{
	int nResult = kResultError;

	if(nJob == -1){
		// We have a problem here in that we need to wait for all threads to finish
		// but the only way to wait for them to finish is to use the Thread::WaitForEnd
		// function. But when the thread exits, it destroys the Thread object rendering
		// it unsafe for us to use that object in any safe way here. We can rearrange
		// things to allow this to work more cleanly, but in the meantime we spin and 
		// sleep, which is not a good solution if the worker threads are of a lower
		// priority than this sleeping thread, as this thread will steal their time.

		if(jobWait == kJobWaitNone){
			// Do nothing.
			nResult = kResultOK;
		}
		else if(jobWait == kJobWaitCurrent){
			// Wait for currently running jobs to complete.
			while((mnActiveCount != 0) && (GetThreadTime() < timeoutAbsolute))
				ThreadSleep(10);
			if(mnActiveCount == 0)
				nResult = kResultOK;
			else
				nResult = kResultTimeout;
		}
		else{ // jobWait == kJobWaitAll
			// Wait for all current and queued jobs to complete.
			bool shouldContinue = true;

			while(shouldContinue)
			{
				mThreadMutex.Lock();
				shouldContinue = (((mnActiveCount != 0) || !mJobList.empty()) && (GetThreadTime() < timeoutAbsolute));
				mThreadMutex.Unlock();
				if(shouldContinue)
					ThreadSleep(10);
			}

			mThreadMutex.Lock();

			if((mnActiveCount == 0) && mJobList.empty())
				nResult = kResultOK;
			else
				nResult = kResultTimeout;

			mThreadMutex.Unlock();
		}
	}
	else{
		// Like above we do the wait via polling. Ideally we want to set up a 
		// mechanism whereby we sleep until an alarm wakes us. This can perhaps
		// be done by setting a flag in the job which causes the job to signal
		// the alarm when complete. In the meantime we will follow the simpler
		// behaviour we have here.

		bool bJobExists;

		for(;;){
			bJobExists = false;
			mThreadMutex.Lock();
			
			// Search the list of jobs yet to become active to see if the job exists in there.
			for(JobList::iterator it(mJobList.begin()); it != mJobList.end(); ++it){
				const Job& job = *it;

				if(job.mnJobID == nJob){ // If the user's job was found...
					bJobExists = true;
					nResult = kResultTimeout;
				}
			}

			// Search the list of jobs actively executing as well.
			for(ThreadInfoList::iterator it(mThreadInfoList.begin()); it != mThreadInfoList.end(); ++it){
				const ThreadInfo* const pThreadInfo = *it;
				const Job& job = pThreadInfo->mCurrentJob;

				// Note the thread must be active for the Job assigned to it be valid.
				if(pThreadInfo->mbActive && job.mnJobID == nJob){ // If the user's job was found...
					bJobExists = true;
					nResult = kResultTimeout;
				}
			}
	  
			mThreadMutex.Unlock();
			if(!bJobExists || (GetThreadTime() >= timeoutAbsolute))
				break;
			ThreadSleep(10);
		}

		if(!bJobExists)
			nResult = kResultOK;
	}

	return nResult;
}


void EA::Thread::ThreadPool::Pause(bool bPause)
{
	if(bPause)
		++mnPauseCount;
	else{
		if(mnPauseCount.Decrement() == 0){
			mThreadMutex.Lock();
			if(!mJobList.empty())
				mThreadCondition.Signal(true);
			mThreadMutex.Unlock();
		}
	}
}


void EA::Thread::ThreadPool::Lock()
{
	mThreadMutex.Lock();
}


void EA::Thread::ThreadPool::Unlock()
{
	mThreadMutex.Unlock();
}


void EA::Thread::ThreadPool::SetupThreadParameters(EA::Thread::ThreadParameters& tp)
{
	if(tp.mnProcessor == kThreadPoolParametersProcessorDefault) // If we are to manipulate tp.mnProcessor...
	{
		if(mnProcessorMask != 0xffffffff) // If we are not using the default...
		{
			// We round-robin mnNextProcessor within our mnProcessorMask.
			while(((1 << mnNextProcessor) & mnProcessorMask) == 0)
				++mnNextProcessor;
				
			mnNextProcessor %= mnProcessorCount;
			tp.mnProcessor = (int)mnNextProcessor++;
		}
	}
}


EA::Thread::ThreadPool::ThreadInfo* EA::Thread::ThreadPool::AddThread(const EA::Thread::ThreadParameters& tp, bool bBeginThread)
{
	ThreadInfo* const pThreadInfo = CreateThreadInfo();
	EAT_ASSERT(pThreadInfo != NULL);

	if(pThreadInfo)
	{
		AddThread(pThreadInfo);

		if(bBeginThread)
		{
			ThreadParameters tpUsed(tp);
			SetupThreadParameters(tpUsed);  // This function sets tpUsed.mnProcessor

			pThreadInfo->mpThread->Begin(ThreadFunction, pThreadInfo, &tpUsed);
		}
	}

	return pThreadInfo;
}


// Gets the ThreadInfo for the nth Thread identified by index. 
// You must call this function within a Lock/Unlock pair on the thread pool.
EA::Thread::ThreadPool::ThreadInfo* EA::Thread::ThreadPool::GetThreadInfo(int index)
{
	EA::Thread::AutoMutex autoMutex(mThreadMutex);

	int i = 0;

	for(ThreadInfoList::iterator it = mThreadInfoList.begin(); it != mThreadInfoList.end(); ++it)
	{
		if(i == index)
		{
			ThreadInfo* pThreadInfo = *it;
			return pThreadInfo;
		}

		++i;
	}
		
	return NULL;
}


// Unless you call this function while the Pool is locked (via Lock), the return
// value may be out of date by the time you read it. 
int EA::Thread::ThreadPool::GetThreadCount()
{
	EA::Thread::AutoMutex autoMutex(mThreadMutex);

	return (int)mThreadInfoList.size();
}


EA::Thread::ThreadPool::ThreadInfo* EA::Thread::ThreadPool::CreateThreadInfo()
{
	// Currently we assume that allocation never fails.
	ThreadInfo* const pThreadInfo = gpAllocator ? new(gpAllocator->Alloc(sizeof(ThreadInfo))) ThreadInfo : new ThreadInfo;

	if(pThreadInfo)
	{
		pThreadInfo->mbActive     = false;
		pThreadInfo->mbQuit       = false;
		pThreadInfo->mpThreadPool = this;
		pThreadInfo->mpThread     = gpAllocator ? new(gpAllocator->Alloc(sizeof(Thread))) Thread : new Thread;
	}

	return pThreadInfo;
}


void EA::Thread::ThreadPool::AdjustThreadCount(unsigned nDesiredCount)
{
	// This function doesn't read mnMinCount/mnMaxCount, as it expects the caller to do so.
	// Assumes that condition variable is locked.
	int nAdjustment = (int)(nDesiredCount - mnCurrentCount);

	while(nAdjustment > 0) // If we are to create threads...
	{
		ThreadInfo* const pThreadInfo = CreateThreadInfo();
		EAT_ASSERT(pThreadInfo != NULL);

		AddThread(pThreadInfo);

		ThreadParameters tpUsed(mDefaultThreadParameters);
		SetupThreadParameters(tpUsed); // This function sets tpUsed.mnProcessor

		pThreadInfo->mpThread->Begin(ThreadFunction, pThreadInfo, &tpUsed);
		nAdjustment--;
	}

	while(nAdjustment < 0) // If we are to quit threads...
	{
		// An empty job is a signal for a thread to quit.
		QueueJob(Job(), NULL, true);
		nAdjustment++;
	}

	FixThreads(); // Makes sure that mnCurrentCount really does match the number of threads waiting for work.
}



void EA::Thread::ThreadPool::AddThread(ThreadInfo* pThreadInfo)
{
	// Assumes that condition variable is locked.
	mThreadInfoList.push_back(pThreadInfo);
	++mnCurrentCount;
}


void EA::Thread::ThreadPool::RemoveThread(ThreadInfo* pThreadInfo)
{
	// Assumes that condition variable is locked.
	ThreadInfoList::iterator it = mThreadInfoList.find(pThreadInfo);
	EAT_ASSERT(it != mThreadInfoList.end());

	if(it != mThreadInfoList.end())
	{
		if(gpAllocator)
		{
			pThreadInfo->mpThread->~Thread();
			gpAllocator->Free(pThreadInfo->mpThread);
		}
		else
			delete pThreadInfo->mpThread;

		pThreadInfo->mpThread = NULL;
		mThreadInfoList.erase(it);

		if(gpAllocator)
		{
			pThreadInfo->~ThreadInfo();
			gpAllocator->Free(pThreadInfo);
		}
		else
			delete pThreadInfo;

		--mnCurrentCount;
	}
}


// FixThreads
// We have a small in problem in that the system allows threads to explicitly exit at any time without
// returning to the caller. Many operating systems with thread support don't have a mechanism to enable
// you to tell you via a callback when a thread has exited. Due to this latter problem, it is possible
// that threads could exit without us ever finding out about it. So we poll the threads to catch up 
// to their state in such cases here.
void EA::Thread::ThreadPool::FixThreads()
{
	// Assumes that condition variable is locked.
	for(ThreadInfoList::iterator it(mThreadInfoList.begin()), itEnd(mThreadInfoList.end()); it != itEnd; ++it)
	{
		ThreadInfo* const pThreadInfo = *it;

		// Fix any threads which have exited via a thread exit and not by simply returning to the caller.
		const EA::Thread::Thread::Status status = pThreadInfo->mpThread->GetStatus();

		if(status == EA::Thread::Thread::kStatusEnded)
			pThreadInfo->mpThread->Begin(ThreadFunction, pThreadInfo, &mDefaultThreadParameters);
	}
}


EA::Thread::ThreadPool* EA::Thread::ThreadPoolFactory::CreateThreadPool()
{
	if(gpAllocator)
		return new(gpAllocator->Alloc(sizeof(EA::Thread::ThreadPool))) EA::Thread::ThreadPool;
	else
		return new EA::Thread::ThreadPool;
}


void EA::Thread::ThreadPoolFactory::DestroyThreadPool(EA::Thread::ThreadPool* pThreadPool)
{
	if(gpAllocator)
	{
		pThreadPool->~ThreadPool();
		gpAllocator->Free(pThreadPool);
	}
	else
		delete pThreadPool;
}


size_t EA::Thread::ThreadPoolFactory::GetThreadPoolSize()
{
	return sizeof(EA::Thread::ThreadPool);
}


EA::Thread::ThreadPool* EA::Thread::ThreadPoolFactory::ConstructThreadPool(void* pMemory)
{
	return new(pMemory) EA::Thread::ThreadPool;
}


void EA::Thread::ThreadPoolFactory::DestructThreadPool(EA::Thread::ThreadPool* pThreadPool)
{
	pThreadPool->~ThreadPool();
}


#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


