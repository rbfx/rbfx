/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/


#ifndef __DG_THREAD_HIVE_H__
#define __DG_THREAD_HIVE_H__

#include "dgThread.h"
#include "dgMemory.h"
#include "dgFastQueue.h"

#define DG_THREAD_POOL_JOB_SIZE (256)
typedef void (*dgWorkerThreadTaskCallback) (void* const context0, void* const context1, dgInt32 threadID);

#ifndef WIN32
#define USE_UNIX_THREAD_POOL 
#endif

#ifdef USE_UNIX_THREAD_POOL 
	class dgThreadHive  
	{
		public:
		class dgThreadJob
		{
			public:
			dgThreadJob()
			{
			}

			dgThreadJob (void* const context0, void* const context1, dgWorkerThreadTaskCallback callback, const char* const jobName)
				:m_context0(context0)
				,m_context1(context1)
				,m_callback(callback)
				,m_jobName(jobName)
			{
			}

			void* m_context0;
			void* m_context1;
			const char* m_jobName;
			dgWorkerThreadTaskCallback m_callback;
		};

		class dgWorkerThread: public dgThread
		{
			public:
			DG_CLASS_ALLOCATOR(allocator)

			dgWorkerThread();
			~dgWorkerThread();

			bool IsBusy() const;
			void SetUp(dgMemoryAllocator* const allocator, const char* const name, dgInt32 id, dgThreadHive* const hive);
			virtual void Execute (dgInt32 threadId);

			dgInt32 PushJob(const dgThreadJob& job);
			void RunNextJobInQueue(dgInt32 threadId);

			dgThreadHive* m_hive;
			dgMemoryAllocator* m_allocator; 
			dgInt32 m_isBusy;
			dgInt32 m_jobsCount;
			dgSemaphore m_workerSemaphore;
			dgThreadJob m_jobPool[DG_THREAD_POOL_JOB_SIZE];
		};

		dgThreadHive(dgMemoryAllocator* const allocator);
		virtual ~dgThreadHive();

		virtual void OnBeginWorkerThread (dgInt32 threadId);
		virtual void OnEndWorkerThread (dgInt32 threadId);

		void BeginSection() {}
		void EndSection() {}

		void SetParentThread (dgThread* const mastertThread);

		void GlobalLock() const;
		void GlobalUnlock() const;

		void GetIndirectLock (dgInt32* const criticalSectionLock) const;
		void ReleaseIndirectLock (dgInt32* const criticalSectionLock) const;

		dgInt32 GetThreadCount() const;
		dgInt32 GetMaxThreadCount() const;
		void SetThreadsCount (dgInt32 count);

		virtual void QueueJob (dgWorkerThreadTaskCallback callback, void* const context0, void* const context1, const char* const functionName);
		virtual void SynchronizationBarrier ();

		private:
		void DestroyThreads();

		dgThread* m_parentThread;
		dgWorkerThread* m_workerThreads;
		dgMemoryAllocator* m_allocator;
		dgInt32 m_jobsCount;
		dgInt32 m_workerThreadsCount;
		mutable dgInt32 m_globalCriticalSection;
		dgThread::dgSemaphore m_beginSectionSemaphores[DG_MAX_THREADS_HIVE_COUNT];
	};

	DG_INLINE dgInt32 dgThreadHive::GetThreadCount() const
	{
		return m_workerThreadsCount ? m_workerThreadsCount : 1;
	}

	DG_INLINE dgInt32 dgThreadHive::GetMaxThreadCount() const
	{
		return DG_MAX_THREADS_HIVE_COUNT;
	}


	DG_INLINE void dgThreadHive::GlobalLock() const
	{
		GetIndirectLock(&m_globalCriticalSection);
	}

	DG_INLINE void dgThreadHive::GlobalUnlock() const
	{
		ReleaseIndirectLock(&m_globalCriticalSection);
	}

	DG_INLINE void dgThreadHive::GetIndirectLock (dgInt32* const criticalSectionLock) const
	{
		if (m_workerThreadsCount) {	
			dgSpinLock(criticalSectionLock);
		}
	}

	DG_INLINE void dgThreadHive::ReleaseIndirectLock (dgInt32* const criticalSectionLock) const
	{
		if (m_workerThreadsCount) {	
			dgSpinUnlock(criticalSectionLock);
		}
	}

#else
	class dgThreadHive
	{

		class dgThreadJob
		{
			public:
			dgThreadJob()
			{
			}

			dgThreadJob(void* const context0, void* const context1, dgWorkerThreadTaskCallback callback, const char* const jobName)
				:m_context0(context0)
				,m_context1(context1)
				,m_callback(callback)
				,m_jobName(jobName)
			{
			}

			void* m_context0;
			void* m_context1;
			const char* m_jobName;
			dgWorkerThreadTaskCallback m_callback;
		};


		class dgWorkerThread: public dgThread
		{
			public:
			DG_CLASS_ALLOCATOR(allocator)

			dgWorkerThread();
			~dgWorkerThread();

			void SetUp(dgMemoryAllocator* const allocator, const char* const name, dgInt32 id, dgThreadHive* const hive);
			virtual void Execute(dgInt32 threadId);

			dgInt32 PushJob(const dgThreadJob& job);
			void RunNextJobInQueue(dgInt32 threadId);
			void ConcurrentWork(dgInt32 threadId);

	//		bool IsBusy() const;
	//		dgInt32 m_isBusy;

			dgSemaphore m_workerSemaphore;
			dgThreadHive* m_hive;
			dgMemoryAllocator* m_allocator;
			dgInt32 m_concurrentWork;
			dgInt32 m_pendingWork;
			dgInt32 m_jobsCount;
			dgThreadJob m_jobPool[DG_THREAD_POOL_JOB_SIZE];
		};

		public:
		dgThreadHive(dgMemoryAllocator* const allocator);
		virtual ~dgThreadHive();

		virtual void OnBeginWorkerThread(dgInt32 threadId);
		virtual void OnEndWorkerThread(dgInt32 threadId);

		void BeginSection();
		void EndSection();

		void SetParentThread(dgThread* const mastertThread);

		void GlobalLock() const;
		void GlobalUnlock() const;

		void GetIndirectLock(dgInt32* const criticalSectionLock) const;
		void ReleaseIndirectLock(dgInt32* const criticalSectionLock) const;

		dgInt32 GetThreadCount() const;
		dgInt32 GetMaxThreadCount() const;
		void SetThreadsCount(dgInt32 count);

		virtual void QueueJob(dgWorkerThreadTaskCallback callback, void* const context0, void* const context1, const char* const functionName);
		virtual void SynchronizationBarrier();

		private:
		void DestroyThreads();

		dgThread* m_parentThread;
		dgWorkerThread* m_workerThreads;
		dgMemoryAllocator* m_allocator;
		dgInt32 m_syncLock;
		dgInt32 m_jobsCount;
		dgInt32 m_workerThreadsCount;
		mutable dgInt32 m_globalCriticalSection;
		dgThread::dgSemaphore m_endSectionSemaphores[DG_MAX_THREADS_HIVE_COUNT];
		dgThread::dgSemaphore m_beginSectionSemaphores[DG_MAX_THREADS_HIVE_COUNT];
	};

	DG_INLINE dgInt32 dgThreadHive::GetThreadCount() const
	{
		return m_workerThreadsCount ? m_workerThreadsCount : 1;
	}

	DG_INLINE dgInt32 dgThreadHive::GetMaxThreadCount() const
	{
		return DG_MAX_THREADS_HIVE_COUNT;
	}

	DG_INLINE void dgThreadHive::GlobalLock() const
	{
		GetIndirectLock(&m_globalCriticalSection);
	}

	DG_INLINE void dgThreadHive::GlobalUnlock() const
	{
		ReleaseIndirectLock(&m_globalCriticalSection);
	}

	DG_INLINE void dgThreadHive::GetIndirectLock(dgInt32* const criticalSectionLock) const
	{
		if (m_workerThreadsCount) {
			dgSpinLock(criticalSectionLock);
		}
	}

	DG_INLINE void dgThreadHive::ReleaseIndirectLock(dgInt32* const criticalSectionLock) const
	{
		if (m_workerThreadsCount) {
			dgSpinUnlock(criticalSectionLock);
		}
	}
#endif

#endif
