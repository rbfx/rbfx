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
#include "dgThread.h"
#include "dgProfiler.h"

dgThread::dgThread ()
	:m_id(0)
	,m_terminate(0)
	,m_threadRunning(0)
{
	m_name[0] = 0;
}

dgThread::dgThread (const char* const name, dgInt32 id)
	:m_id(id)
	,m_terminate(0)
	,m_threadRunning(0)
{
	strncpy (m_name, name, sizeof (m_name) - 1);
}


void dgThread::Init (const char* const name, dgInt32 id)
{
	m_id = id;
	strncpy (m_name, name, sizeof (m_name) - 1);
	Init ();
}


bool dgThread::IsThreadActive() const
{
	return m_threadRunning ? true : false;
}



#ifdef DG_USE_THREAD_EMULATION

dgThread::dgSemaphore::dgSemaphore ()
{
	m_sem = 0;
}

dgThread::dgSemaphore::~dgSemaphore ()
{
}

void dgThread::dgSemaphore::Release ()
{
}


dgThread::~dgThread ()
{
}

void dgThread::Init ()
{
}

void dgThread::Close ()
{
}

void dgThread::SuspendExecution (dgSemaphore& mutex)
{
}


void dgThread::SuspendExecution (dgInt32 count, dgSemaphore* const semArray)
{
}


void* dgThread::dgThreadSystemCallback(void* threadData)
{
	return 0;
}

#else  

dgThread::dgSemaphore::dgSemaphore ()
	:m_count(0)
{
}

dgThread::dgSemaphore::~dgSemaphore ()
{
}

void dgThread::dgSemaphore::Release ()
{
	std::unique_lock <std::mutex> lck(m_mutex);
	m_count ++;
	m_sem.notify_one();
}

void dgThread::dgSemaphore::Wait()
{
	std::unique_lock <std::mutex> lck(m_mutex);
	while (m_count == 0)
	{
		m_sem.wait(lck);
	}
	m_count --;
}

dgThread::~dgThread ()
{
}

void dgThread::Init ()
{
	// This must be set now because otherwise if this thread is
	// immediately closed the Terminate method won't detect that the
	// thread is running
	dgInterlockedExchange(&m_threadRunning, 1);
	m_handle = std::thread(dgThreadSystemCallback, this);
	dgThreadYield();
}

void dgThread::Close ()
{
	m_handle.join();
}

void dgThread::SuspendExecution (dgSemaphore& mutex)
{
	mutex.Wait();
}

void dgThread::SuspendExecution (dgInt32 count, dgSemaphore* const semArray)
{
	for (dgInt32 i = 0; i < count; i ++) {
		SuspendExecution (semArray[i]);
	}
}


void* dgThread::dgThreadSystemCallback(void* threadData)
{
	dgFloatExceptions exception;
	dgSetPrecisionDouble precision;

	dgThread* const me = (dgThread*) threadData;

	D_SET_TRACK_NAME(me->m_name);

	me->Execute(me->m_id);
	dgInterlockedExchange(&me->m_threadRunning, 0);

	return 0;
}


#endif



