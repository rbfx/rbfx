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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#ifndef __dgFastQueue__
#define __dgFastQueue__

#include "dgStdafx.h"

template<class T, dgInt32 sizeInPowerOfTwo>
class dgFastQueue
{
	public:
    DG_CLASS_ALLOCATOR_NEW(allocator)

	dgFastQueue (dgMemoryAllocator* const allocator);
	~dgFastQueue ();

	bool IsEmpty() const;
	bool IsFull() const;
	const T& GetHead() const;
	void Pop();
	void Push(T& object);

	private:
	T* m_pool;
    dgMemoryAllocator* m_allocator;
	dgInt32 m_head;
	dgInt32 m_tail;
};



template<class T, dgInt32 sizeInPowerOfTwo>
dgFastQueue<T, sizeInPowerOfTwo>::dgFastQueue (dgMemoryAllocator* const allocator)
    :m_allocator(allocator)
	,m_head(0)
	,m_tail(0)
{
	dgAssert (((sizeInPowerOfTwo -1) & (-sizeInPowerOfTwo)) == 0);
    m_pool = (T*) m_allocator->MallocLow(sizeInPowerOfTwo * sizeof (T));
}


template<class T, dgInt32 sizeInPowerOfTwo>
dgFastQueue<T, sizeInPowerOfTwo>::~dgFastQueue ()
{
    m_allocator->FreeLow(m_pool); 
}

template<class T, dgInt32 sizeInPowerOfTwo>
bool dgFastQueue<T, sizeInPowerOfTwo>::IsEmpty() const
{
	return (m_head == m_tail);
}

template<class T, dgInt32 sizeInPowerOfTwo>
bool dgFastQueue<T, sizeInPowerOfTwo>::IsFull() const
{
	return (((m_tail + 1) & (sizeInPowerOfTwo - 1)) == m_head);
}

template<class T, dgInt32 sizeInPowerOfTwo>
const T& dgFastQueue<T, sizeInPowerOfTwo>::GetHead() const
{
	dgAssert (!IsEmpty());
	return m_pool[m_head];
}

template<class T, dgInt32 sizeInPowerOfTwo>
void dgFastQueue<T, sizeInPowerOfTwo>::Pop()
{
	dgAssert (!IsEmpty());
	m_head = (m_head + 1) & (sizeInPowerOfTwo - 1);
}

template<class T, dgInt32 sizeInPowerOfTwo>
void dgFastQueue<T, sizeInPowerOfTwo>::Push(T& object)
{
	dgAssert (!IsFull());
	m_pool[m_tail] = object; 
	m_tail = (m_tail + 1) & (sizeInPowerOfTwo - 1);
}



#endif




