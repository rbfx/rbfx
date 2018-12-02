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

#ifndef __DG_THREAD_API_H__
#define __DG_THREAD_API_H__

// by default newton is run on a separate thread, optionally concurrent with the calling thread, it also uses a thread job pool for multi core systems.
// define DG_USE_THREAD_EMULATION on the command line for platform that do not support hardware multi threading or if multi threading is not stable 
//#define DG_USE_THREAD_EMULATION


class dgThread
{
	public:
	class dgSemaphore
	{
		public:
		dgSemaphore ();
		~dgSemaphore ();
		void Wait();
		void Release();

		private:
		#ifdef DG_USE_THREAD_EMULATION
			dgInt32 m_sem;
		#else 
			std::condition_variable m_sem;
			std::mutex m_mutex;
			dgInt32 m_count;
		#endif
	};

	dgThread ();
	dgThread (const char* const name, dgInt32 id);
	virtual ~dgThread ();

	virtual void Execute (dgInt32 threadId) = 0;
	
	bool IsThreadActive() const;
	void SuspendExecution (dgSemaphore& mutex);
	void SuspendExecution (dgInt32 count, dgSemaphore* const mutexes);

	protected:
	void Init ();
	void Init (const char* const name, dgInt32 id);
	void Close ();
	static void* dgThreadSystemCallback(void* threadData);

	#ifndef DG_USE_THREAD_EMULATION
		std::thread m_handle;
	#endif
	dgInt32 m_id;
	dgInt32 m_terminate;
	dgInt32 m_threadRunning;
	
	char m_name[32];
};

#endif
