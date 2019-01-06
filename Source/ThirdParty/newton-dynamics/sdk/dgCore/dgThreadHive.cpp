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

#include "dgStdafx.h"
#include "dgTypes.h"
#include "dgMemory.h"
#include "dgProfiler.h"
#include "dgThreadHive.h"

#ifdef USE_OLD_THREAD_POOL 
dgThreadHive::dgWorkerThread::dgWorkerThread()
	:dgThread()
	,m_hive(NULL)
	,m_allocator(NULL)
	,m_isBusy(0)
	,m_jobsCount(0)
	,m_workerSemaphore()
{
}

dgThreadHive::dgWorkerThread::~dgWorkerThread()
{
	while (IsBusy());

	dgInterlockedExchange(&m_terminate, 1);
	m_workerSemaphore.Release();
	Close();
}

void dgThreadHive::dgWorkerThread::SetUp(dgMemoryAllocator* const allocator, const char* const name, dgInt32 id, dgThreadHive* const hive)
{
	m_hive = hive;
	m_allocator = allocator;
	Init (name, id);

	#ifndef DG_USE_THREAD_EMULATION
		#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
			SetThreadPriority(m_handle.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
		#endif
	#endif
}

bool dgThreadHive::dgWorkerThread::IsBusy() const
{
	return m_isBusy ? true : false;
}

void dgThreadHive::dgWorkerThread::Execute (dgInt32 threadId)
{
	m_hive->OnBeginWorkerThread (threadId);

	while (!m_terminate) {
		dgInterlockedExchange(&m_isBusy, 0);
		SuspendExecution(m_workerSemaphore);
		dgInterlockedExchange(&m_isBusy, 1);
		if (!m_terminate) {
			RunNextJobInQueue(threadId);
			m_hive->m_beginSectionSemaphores[threadId].Release();
		}
	}

	dgInterlockedExchange(&m_isBusy, 0);

	m_hive->OnEndWorkerThread (threadId);
}

dgInt32 dgThreadHive::dgWorkerThread::PushJob(const dgThreadJob& job)
{
	dgAssert (m_jobsCount < sizeof (m_jobPool)/ sizeof (m_jobPool[0]));
	m_jobPool[m_jobsCount] = job;
	m_jobsCount ++;
	return m_jobsCount;
}

void dgThreadHive::dgWorkerThread::RunNextJobInQueue(dgInt32 threadId)
{
	for (dgInt32 i = 0; i < m_jobsCount; i ++) {
		const dgThreadJob& job = m_jobPool[i];
		DG_TRACKTIME_NAMED(job.m_jobName);
		job.m_callback (job.m_context0, job.m_context1, m_id);
	}
	m_jobsCount = 0;
}

dgThreadHive::dgThreadHive(dgMemoryAllocator* const allocator)
	:m_parentThread(NULL)
	,m_workerThreads(NULL)
	,m_allocator(allocator)
	,m_jobsCount(0)
	,m_workerThreadsCount(0)
	,m_globalCriticalSection(0)
{
}

dgThreadHive::~dgThreadHive()
{
	DestroyThreads();
}

void dgThreadHive::SetParentThread (dgThread* const parentThread)
{
	m_parentThread = parentThread;
}

void dgThreadHive::DestroyThreads()
{
	if (m_workerThreadsCount) {
		delete[] m_workerThreads;
		m_workerThreads = NULL;
		m_workerThreadsCount = 0;
	}
}

void dgThreadHive::SetThreadsCount (dgInt32 threads)
{
	DestroyThreads();

	m_workerThreadsCount = dgMin (threads, DG_MAX_THREADS_HIVE_COUNT);
	if (m_workerThreadsCount == 1) {
		m_workerThreadsCount = 0;
	}

	if (m_workerThreadsCount) {
		m_workerThreads = new (m_allocator) dgWorkerThread[dgUnsigned32 (m_workerThreadsCount)];

		for (dgInt32 i = 0; i < m_workerThreadsCount; i ++) {
			char name[256];
			sprintf (name, "dgWorkerThread%d", i);
			m_workerThreads[i].SetUp(m_allocator, name, i, this);
		}
	}
}

void dgThreadHive::QueueJob (dgWorkerThreadTaskCallback callback, void* const context0, void* const context1, const char* const functionName)
{
	if (!m_workerThreadsCount) {
		DG_TRACKTIME(functionName);
		callback (context0, context1, 0);
	} else {
		dgInt32 workerTreadEntry = m_jobsCount % m_workerThreadsCount;
		#ifdef DG_USE_THREAD_EMULATION
			DG_TRACKTIME(functionName);
			callback (context0, context1, workerTreadEntry);
		#else 
			dgInt32 index = m_workerThreads[workerTreadEntry].PushJob(dgThreadJob(context0, context1, callback, functionName));
			if (index >= DG_THREAD_POOL_JOB_SIZE) {
				dgAssert (0);
				SynchronizationBarrier ();
			}
		#endif
	}

	m_jobsCount ++;
}

void dgThreadHive::OnBeginWorkerThread (dgInt32 threadId)
{
}

void dgThreadHive::OnEndWorkerThread (dgInt32 threadId)
{
}

void dgThreadHive::SynchronizationBarrier ()
{
	if (m_workerThreadsCount) {
		DG_TRACKTIME(__FUNCTION__);
		for (dgInt32 i = 0; i < m_workerThreadsCount; i ++) {
			m_workerThreads[i].m_workerSemaphore.Release();
		}
		m_parentThread->SuspendExecution(m_workerThreadsCount, m_beginSectionSemaphores);
	}
	m_jobsCount = 0;
}

#else

dgThreadHive::dgWorkerThread::dgWorkerThread()
	:dgThread()
	,m_workerSemaphore()
	,m_hive(NULL)
	,m_allocator(NULL)
	,m_concurrentWork(0)
	,m_pendingWork(0)
	,m_jobsCount(0)
{
}

dgThreadHive::dgWorkerThread::~dgWorkerThread()
{
	dgInterlockedExchange(&m_terminate, 1);
	m_workerSemaphore.Release();
	Close();
}

void dgThreadHive::dgWorkerThread::SetUp(dgMemoryAllocator* const allocator, const char* const name, dgInt32 id, dgThreadHive* const hive)
{
	m_hive = hive;
	m_allocator = allocator;
	Init(name, id);
}

void dgThreadHive::dgWorkerThread::RunNextJobInQueue(dgInt32 threadId)
{
	for (dgInt32 i = 0; i < m_jobsCount; i++) {
		const dgThreadJob& job = m_jobPool[i];
		DG_TRACKTIME_NAMED(job.m_jobName);
		job.m_callback(job.m_context0, job.m_context1, m_id);
	}
}

dgInt32 dgThreadHive::dgWorkerThread::PushJob(const dgThreadJob& job)
{
	dgAssert(m_jobsCount < sizeof (m_jobPool) / sizeof (m_jobPool[0]));
	m_jobPool[m_jobsCount] = job;
	m_jobsCount++;
	return m_jobsCount;
}


void dgThreadHive::dgWorkerThread::ConcurrentWork(dgInt32 threadId)
{
	while (dgInterlockedTest(&m_concurrentWork, 1)) {
		if (dgInterlockedExchange(&m_pendingWork, 0)) {
			//DG_TRACKTIME(__FUNCTION__);
			RunNextJobInQueue(threadId);
			m_jobsCount = 0;
			dgAtomicExchangeAndAdd(&m_hive->m_syncLock, -1);
		}
		dgThreadYield();
	}
}

void dgThreadHive::dgWorkerThread::Execute(dgInt32 threadId)
{
	m_hive->OnBeginWorkerThread(threadId);

	while (!m_terminate) {
//		dgInterlockedExchange(&m_isBusy, 0);
		SuspendExecution(m_workerSemaphore);
//		dgInterlockedExchange(&m_isBusy, 1);
		if (!m_terminate) {
			m_concurrentWork = 1;
			m_hive->m_beginSectionSemaphores[threadId].Release();
			ConcurrentWork(threadId);
			m_hive->m_endSectionSemaphores[threadId].Release();
		}
	}

//	dgInterlockedExchange(&m_isBusy, 0);
	m_hive->OnEndWorkerThread(threadId);
}

dgThreadHive::dgThreadHive(dgMemoryAllocator* const allocator)
	:m_parentThread(NULL)
	,m_workerThreads(NULL)
	,m_allocator(allocator)
	,m_syncLock(0)
	,m_jobsCount(0)
	,m_workerThreadsCount(0)
	,m_globalCriticalSection(0)
{
}

dgThreadHive::~dgThreadHive()
{
	DestroyThreads();
}

void dgThreadHive::SetParentThread(dgThread* const mastertThread)
{
	m_parentThread = mastertThread;
}

void dgThreadHive::OnBeginWorkerThread(dgInt32 threadId)
{
}

void dgThreadHive::OnEndWorkerThread(dgInt32 threadId)
{
}

void dgThreadHive::BeginSection()
{
	if (m_workerThreadsCount) {
		DG_TRACKTIME(__FUNCTION__);
		for (dgInt32 i = 0; i < m_workerThreadsCount; i++) {
			m_workerThreads[i].m_workerSemaphore.Release();
		}
		m_parentThread->SuspendExecution(m_workerThreadsCount, m_beginSectionSemaphores);
	}
}

void dgThreadHive::EndSection()
{
	if (m_workerThreadsCount) {
		DG_TRACKTIME(__FUNCTION__);
		for (dgInt32 i = 0; i < m_workerThreadsCount; i++) {
			dgInterlockedExchange(&m_workerThreads[i].m_concurrentWork, 0);
		}
		m_parentThread->SuspendExecution(m_workerThreadsCount, m_endSectionSemaphores);
	}
}

void dgThreadHive::QueueJob(dgWorkerThreadTaskCallback callback, void* const context0, void* const context1, const char* const functionName)
{
	if (!m_workerThreadsCount) {
		DG_TRACKTIME(functionName);
		callback(context0, context1, 0);
	} else {
		dgInt32 workerTreadEntry = m_jobsCount % m_workerThreadsCount;
#ifdef DG_USE_THREAD_EMULATION
		DG_TRACKTIME(functionName);
		callback(context0, context1, workerTreadEntry);
#else 
		dgInt32 index = m_workerThreads[workerTreadEntry].PushJob(dgThreadJob(context0, context1, callback, functionName));
		if (index >= DG_THREAD_POOL_JOB_SIZE) {
			dgAssert(0);
			SynchronizationBarrier();
		}
#endif
	}
	m_jobsCount++;
}

void dgThreadHive::SetThreadsCount(dgInt32 threads)
{
	DestroyThreads();

	m_workerThreadsCount = dgMin(threads, DG_MAX_THREADS_HIVE_COUNT);
	if (m_workerThreadsCount == 1) {
		m_workerThreadsCount = 0;
	}

	if (m_workerThreadsCount) {
		m_workerThreads = new (m_allocator) dgWorkerThread[dgUnsigned32(m_workerThreadsCount)];

		for (dgInt32 i = 0; i < m_workerThreadsCount; i++) {
			char name[256];
			sprintf(name, "dgWorkerThread%d", i);
			m_workerThreads[i].SetUp(m_allocator, name, i, this);
		}
	}
}

void dgThreadHive::SynchronizationBarrier()
{
	if (m_workerThreadsCount) {
		DG_TRACKTIME(__FUNCTION__);

		#ifndef DG_USE_THREAD_EMULATION
		m_syncLock = m_workerThreadsCount;
		for (dgInt32 i = 0; i < m_workerThreadsCount; i++) {
			dgInterlockedExchange(&m_workerThreads[i].m_pendingWork, 1);
		}
		while (dgInterlockedTest(&m_syncLock, 0)) {
			dgThreadYield();
		}
		#endif
	}
	m_jobsCount = 0;
}

void dgThreadHive::DestroyThreads()
{
	if (m_workerThreadsCount) {
		delete[] m_workerThreads;
		m_workerThreads = NULL;
		m_workerThreadsCount = 0;
	}
}

#endif