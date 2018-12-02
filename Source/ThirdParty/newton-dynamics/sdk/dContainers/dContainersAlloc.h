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

#ifndef _D_CONTAINERS_ALLOC_H_
#define _D_CONTAINERS_ALLOC_H_
#include "dContainersStdAfx.h"

#define D_MAX_ENTRIES_IN_FREELIST	32

class dContainersAlloc  
{
	public:
	DCONTAINERS_API void *operator new (size_t size);
	DCONTAINERS_API void operator delete (void* ptr);

	dContainersAlloc()
	{
	}

	virtual ~dContainersAlloc() 
	{
	}

	static DCONTAINERS_API void* Alloc (size_t size);
	static DCONTAINERS_API void Free (void* const ptr);
};


class dContainerFixSizeAllocator
{
	public:
	static DCONTAINERS_API dContainerFixSizeAllocator* Create (int size, int poolSize);
	DCONTAINERS_API ~dContainerFixSizeAllocator();
	DCONTAINERS_API void* Alloc();
	DCONTAINERS_API void Free(void* const ptr);
//	DCONTAINERS_API bool IsAlive() const;
	DCONTAINERS_API void Flush ();

	private:
	DCONTAINERS_API dContainerFixSizeAllocator(int size, int poolSize);

	class dFreeListNode
	{
		public:
		int m_count;
		dFreeListNode* m_next;
	};

	DCONTAINERS_API void Prefetch ();

	dFreeListNode* m_freeListNode;
	int m_size;
	int m_poolSize;
};


#endif
