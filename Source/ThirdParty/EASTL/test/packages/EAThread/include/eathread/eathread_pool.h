///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a classic thread pool.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_POOL_H
#define EATHREAD_EATHREAD_POOL_H


#ifndef EATHREAD_EATHREAD_THREAD_H
	#include <eathread/eathread_thread.h>
#endif
#ifndef EATHREAD_EATHREAD_CONDITION_H
	#include <eathread/eathread_condition.h>
#endif
#ifndef EATHREAD_EATHREAD_ATOMIC_H
	#include <eathread/eathread_atomic.h>
#endif
#ifndef EATHREAD_EATHREAD_LIST_H
	#include <eathread/eathread_list.h>
#endif
#include <stddef.h>


#if defined(EA_DLL) && defined(_MSC_VER)
	// Suppress warning about class 'EA::Thread::simple_list<T>' needs to have
	// dll-interface to be used by clients of class which have a templated member.
	// 
	// These templates cannot be instantiated outside of the DLL. If you try, a
	// link error will result. This compiler warning is intended to notify users
	// of this.
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



/////////////////////////////////////////////////////////////////////////////
// EA_THREAD_POOL_MAX_SIZE
//
// Defines the maximum number of threads the pool can have.
// Currently we have a limit of at most N threads in a pool, in order to 
// simplify memory management issues.
//
#ifndef EA_THREAD_POOL_MAX_SIZE
	#define EA_THREAD_POOL_MAX_SIZE 16
#endif



namespace EA
{
	namespace Thread
	{
		/// ThreadPoolParameters
		/// Specifies how a thread pool is initialized
		struct EATHREADLIB_API ThreadPoolParameters
		{
			unsigned         mnMinCount;                /// Default is kDefaultMinCount.
			unsigned         mnMaxCount;                /// Default is kDefaultMaxCount.
			unsigned         mnInitialCount;            /// Default is kDefaultInitialCount
			ThreadTime       mnIdleTimeoutMilliseconds; /// Default is kDefaultIdleTimeout. This is a relative time, not an absolute time. Can be a millisecond value or Thread::kTimeoutNone or Thread::kTimeoutImmediate.
			unsigned         mnProcessorMask;           /// Default is 0xffffffff. Controls which processors we are allowed to create threads on. Default is all processors.
			ThreadParameters mDefaultThreadParameters;  /// Currently only the mnStackSize, mnPriority, and mpName fields from ThreadParameters are used.

			ThreadPoolParameters();

		private:
			// Prevent default generation of these functions by not defining them
			ThreadPoolParameters(const ThreadPoolParameters& rhs);               // copy constructor
			ThreadPoolParameters& operator=(const ThreadPoolParameters& rhs);    // assignment operator
		};


		/// class ThreadPool
		/// 
		/// Implements a conventional thread pool. Thread pools are useful for situations where
		/// thread creation and destruction is common and the application speed would improve
		/// by using pre-made threads that are ready to execute. 
		class EATHREADLIB_API ThreadPool
		{
		public:
			enum Default
			{
				kDefaultMinCount      = 0,
				kDefaultMaxCount      = 4,
				kDefaultInitialCount  = 0,
				kDefaultIdleTimeout   = 60000, // Milliseconds
				kDefaultProcessorMask = 0xffffffff
			};

			enum Result
			{
				kResultOK       =  0,
				kResultError    = -1,
				kResultTimeout  = -2,
				kResultDeferred = -3
			};

			enum JobWait
			{
				kJobWaitNone,    /// Wait for no jobs to complete, including those currently running.
				kJobWaitCurrent, /// Wait for currently proceeding jobs to complete but not those that haven't started.
				kJobWaitAll      /// Wait for all jobs to complete, including those that haven't yet begun.
			};

			/// ThreadPool
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use ThreadPool(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to ThreadPool(NULL, false).
			ThreadPool(const ThreadPoolParameters* pThreadPoolParameters = NULL, bool bDefaultParameters = true);

			/// ~ThreadPool
			/// Destroys the thread pool. Waits for any busy threads to complete.
		   ~ThreadPool();

			/// Init
			/// Initializes the thread pool with given characteristics. If the thread pool is 
			/// already initialized, this updates the settings.
			bool Init(const ThreadPoolParameters* pThreadPoolParameters);

			/// Shutdown
			/// Disables the thread pool, waits for busy threads to complete, destroys all threads.
			///
			/// If bWaitForAllJobs is true, then Shutdown will wait until all jobs, including
			/// jobs that haven't been started yet, to complete. Otherwise, only currently 
			/// proceeding jobs will be completed. 
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			bool Shutdown(JobWait jobWait = kJobWaitAll, const ThreadTime& timeoutAbsolute = kTimeoutNone);

			/// Begin
			/// Starts a thread from the pool with the given parameters. 
			/// Returns kResultError or a job id of >= kResultOK. A return of kResultDeferred is 
			/// possible if the number of active threads is greater or equal to the max count.
			/// If input ppThread is non-NULL and return value is >= kResultOK, the returned thread
			/// will be the thread used for the job. Else the returned thread pointer will be NULL.
			/// If input bEnabledDeferred is false but the max count of active theads has been 
			/// reached, a new thread is nevertheless created.
			int Begin(IRunnable*       pRunnable, void* pContext = NULL, Thread** ppThread = NULL, bool bEnableDeferred = false);
			int Begin(RunnableFunction pFunction, void* pContext = NULL, Thread** ppThread = NULL, bool bEnableDeferred = false);

			/// WaitForJobCompletion
			/// Waits for an individual job or for all jobs (job id of -1) to complete. 
			/// If a job id is given which doesn't correspond to any existing job, 
			/// the job is assumed to have been completed and the wait completes immediately.
			/// If new jobs are added while the wait is occurring, this function will wait
			/// for those jobs to complete as well. jobWait is valid only if nJob is -1.
			/// Note that the timeout is specified in absolute time and not relative time.
			/// Returns one of enum Result.
			int WaitForJobCompletion(int nJob = -1, JobWait jobWait = kJobWaitAll, const ThreadTime& timeoutAbsolute = kTimeoutNone);

			/// Pause
			/// Enables or disables the activation of threads from the pool. 
			/// When paused, calls to Begin will return kResultDeferred instead of kResultOK.
			void Pause(bool bPause);

			/// Locks the thread pool thread list.
			void Lock();
			void Unlock();

			struct Job
			{
				int              mnJobID;       /// Unique job id.
				IRunnable*       mpRunnable;    /// User-supplied IRunnable. This is an alternative to mpFunction.
				RunnableFunction mpFunction;    /// User-supplied function. This is an alternative to mpRunnable.
				void*            mpContext;     /// User-supplied context.

				Job();
			};

			struct ThreadInfo
			{
				volatile bool mbActive;         /// True if the thread is currently busy working on a job.
				volatile bool mbQuit;           /// If set to true then this thread should quit at the next opportunity.
			  //bool          mbPersistent;     /// If true then this thread is never quit at runtime. False by default.
				Thread*       mpThread;         /// The Thread itself.
				ThreadPool*   mpThreadPool;     /// The ThreadPool that owns this thread.
				Job           mCurrentJob;      /// The most recent job a thread is or was working on.

				ThreadInfo();
			};

			/// AddThread
			/// Adds a new thread with the given ThreadParameters.
			/// The return value is not safe to use unless this function is called
			/// and the result used within a Lock/Unlock pair.
			/// It's the user's responsibility to supply ThreadParameters that are sane.
			/// If bBeginThread is true, then the Thread is started via a call to 
			/// pThreadInfo->mpThread->Begin(ThreadFunction, pThreadInfo, &tp);
			/// Otherwise the user is expected to manually start the thread.
			ThreadInfo* AddThread(const ThreadParameters& tp, bool bBeginThread);

			// Gets the ThreadInfo for the nth Thread identified by index. 
			// You must call this function and use the info within a Lock/Unlock pair 
			// on the thread pool.
			ThreadInfo* GetThreadInfo(int index);

			// Unless you call this function while the Pool is locked (via Lock), the return
			// value may be out of date by the time you read it. 
			int GetThreadCount();

		protected:
			typedef EA::Thread::simple_list<Job>         JobList;
			typedef EA::Thread::simple_list<ThreadInfo*> ThreadInfoList;

			// Member functions
			static intptr_t ThreadFunction(void* pContext);
			ThreadInfo*     CreateThreadInfo();
			void            SetupThreadParameters(ThreadParameters& tp);
			void            AdjustThreadCount(unsigned nCount);
			Result          QueueJob(const Job& job, Thread** ppThread, bool bEnableDeferred);
			void            AddThread(ThreadInfo* pThreadInfo);
			void            RemoveThread(ThreadInfo* pThreadInfo);
			void            FixThreads();

			// Member data
			bool                mbInitialized;              // 
			uint32_t            mnMinCount;                 // Min number of threads to have available.
			uint32_t            mnMaxCount;                 // Max number of threads to have available.
			AtomicInt32         mnCurrentCount;             // Current number of threads available.
			AtomicInt32         mnActiveCount;              // Current number of threads busy with jobs.
			ThreadTime          mnIdleTimeoutMilliseconds;  // Timeout before quitting threads that have had no jobs.
			uint32_t            mnProcessorMask;            // If mask is not 0xffffffff then we manually round-robin assign processors.
			uint32_t            mnProcessorCount;           // The number of processors currently present.
			uint32_t            mnNextProcessor;            // Used if we are manually round-robin assigning processors. 
			AtomicInt32         mnPauseCount;               // A positive value means we pause working on jobs.
			AtomicInt32         mnLastJobID;                // 
			ThreadParameters    mDefaultThreadParameters;   // 
			Condition           mThreadCondition;           // Manages signalling mJobList.
			Mutex               mThreadMutex;               // Guards manipulation of mThreadInfoList and mJobList.
			ThreadInfoList      mThreadInfoList;            // List of threads in our pool.
			JobList             mJobList;                   // List of waiting jobs.

		private:
			// Prevent default generation of these functions by not defining them
			ThreadPool(const ThreadPool& rhs);               // copy constructor
			ThreadPool& operator=(const ThreadPool& rhs);    // assignment operator
		};



		/// ThreadPoolFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class ThreadPool.
		/// A primary use of this would be to allow the ThreadPool implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API ThreadPoolFactory
		{
		public:
			static ThreadPool*  CreateThreadPool();                          // Internally implemented as: return new ThreadPool;
			static void         DestroyThreadPool(ThreadPool* pThreadPool);  // Internally implemented as: delete pThreadPool;

			static size_t       GetThreadPoolSize();                         // Internally implemented as: return sizeof(ThreadPool);
			static ThreadPool*  ConstructThreadPool(void* pMemory);          // Internally implemented as: return new(pMemory) ThreadPool;
			static void         DestructThreadPool(ThreadPool* pThreadPool); // Internally implemented as: pThreadPool->~ThreadPool();
		};

	} // namespace Thread

} // namespace EA



#if defined(EA_DLL) && defined(_MSC_VER)
	// re-enable warning 4251 (it's a level-1 warning and should not be suppressed globally)
	#pragma warning(pop)
#endif


#endif // EATHREAD_EATHREAD_POOL_H











