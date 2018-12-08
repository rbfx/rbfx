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

#include "dContainersStdAfx.h"
#include "dContainersAlloc.h"


#pragma warning (disable: 4100) //warning C4100: 'lpReserved' : unreferenced formal parameter

#ifdef _DCONTAINERS_DLL
	void* operator new (size_t size)
	{ 
		void* const ptr = malloc (size);
		dAssert (ptr);
		return ptr;
	}

	void operator delete (void* ptr) 
	{ 
		free (ptr);
	}

	BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
			case DLL_PROCESS_ATTACH:
			case DLL_THREAD_ATTACH:
				// check for memory leaks
				#if defined(_DEBUG) && defined(_MSC_VER)
					// Track all memory leaks at the operating system level.
					// make sure no Newton tool or utility leaves leaks behind.
					_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF));
				#endif

			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				break;
		}
		return TRUE;
	}
#endif


void* dContainersAlloc::operator new (size_t size)
{
	//return ::new char[size];
	return Alloc (size);
}

void dContainersAlloc::operator delete (void* ptr)
{
//	delete[] (char*) ptr;
	Free(ptr);
}

void* dContainersAlloc::Alloc (size_t size)
{
	char* const ptr = ::new char[size];
	return ptr;
}

void dContainersAlloc::Free(void* const ptr)
{
	delete[] (char*) ptr;
}




dContainerFixSizeAllocator::dContainerFixSizeAllocator(int size, int poolSize)
	:m_freeListNode(NULL)
	,m_size(size)
	,m_poolSize(poolSize)
{
	Prefetch ();
}


dContainerFixSizeAllocator::~dContainerFixSizeAllocator()
{
	Flush();
}


dContainerFixSizeAllocator* dContainerFixSizeAllocator::Create (int size, int poolSize)
{
	class AllocatorsFactory
	{
		public:
		AllocatorsFactory()
			:m_count(0)
			,m_maxSize(0)
			,m_pool (NULL)
		{
			m_maxSize = 1;
			m_pool = new dContainerFixSizeAllocator*[1];
		}

		~AllocatorsFactory()
		{
			for (int i = 0; i < m_count; i ++) {
				delete m_pool[i];
			}
			delete[] m_pool; 
		}

		dContainerFixSizeAllocator* FindCreate (int size, int poolSize)
		{
			int i0 = 0;
			int i2 = m_count -1;
			while ((i2 - i0) > 4) {
				int i1 = (i0 + i2) >> 1;
				if (size < m_pool[i1]->m_size) {
					i2 = i1;
				} else {
					i0 = i1;
				}
			}

			for (int i = i0; (i < m_count) && (m_pool[i]->m_size <= size); i ++) {
				if (m_pool[i]->m_size == size) {
					return m_pool[i];
				}
			}

			if (m_count == m_maxSize) {
				m_maxSize *= 2;
				dContainerFixSizeAllocator** pool = new dContainerFixSizeAllocator*[m_maxSize];
				memcpy (pool, m_pool, m_count * sizeof (dContainerFixSizeAllocator*));
				delete[] m_pool;
				m_pool = pool;
			}

			dContainerFixSizeAllocator* const allocator = new dContainerFixSizeAllocator(size, poolSize);

			int entry = m_count;
			for (; entry && (m_pool[entry - 1]->m_size > size); entry --) {
				m_pool[entry] = m_pool[entry - 1];
			}
			m_pool[entry] = allocator;
			m_count ++;
			return allocator;
		}

		int m_count;
		int m_maxSize;
		dContainerFixSizeAllocator** m_pool;
	};

	static AllocatorsFactory factories;
	return factories.FindCreate(size, poolSize);
}


void dContainerFixSizeAllocator::Prefetch ()
{
	for (int i = 0; i < m_poolSize; i ++) {
		dFreeListNode* const data = (dFreeListNode*) dContainersAlloc::Alloc (m_size);
		data->m_count = i + 1; 
		data->m_next = m_freeListNode; 
		m_freeListNode = data;
	}
}


void dContainerFixSizeAllocator::Flush ()
{
	for (int i = 0; m_freeListNode && (i < m_poolSize); i ++) {
		dFreeListNode* const ptr = m_freeListNode;
		m_freeListNode = m_freeListNode->m_next;
		dContainersAlloc::Free (ptr);
	}
}


void* dContainerFixSizeAllocator::Alloc() 
{
	if (!m_freeListNode) {
		Prefetch ();
	}
	dFreeListNode* const data = m_freeListNode;
	m_freeListNode = m_freeListNode->m_next;
	return data;
}


void dContainerFixSizeAllocator::Free(void* const ptr) 
{
	dFreeListNode* const data = (dFreeListNode*) ptr;
	data->m_count = m_freeListNode ? m_freeListNode->m_count + 1 : 1;
	data->m_next = m_freeListNode;
	m_freeListNode = data;
	if (data->m_count >= 2 * m_poolSize) {
		Flush();
	}
}
