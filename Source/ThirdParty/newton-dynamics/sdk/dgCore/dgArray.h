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
#ifndef __dgArray__
#define __dgArray__

#include "dgStdafx.h"

template<class T>
class dgArray
{
	public:
	dgArray ();
	dgArray (dgMemoryAllocator* const allocator, dgInt32 aligmentInBytes = DG_MEMORY_GRANULARITY);
	dgArray (const dgArray& source, dgInt32 itemsToCopy);
	dgArray(const dgArray& source);
	~dgArray ();
	DG_CLASS_ALLOCATOR(allocator)
	
	DG_INLINE T& operator[] (dgInt32 i);
	DG_INLINE const T& operator[] (dgInt32 i) const;
	
	void Clear () const;
	void Resize (dgInt32 size) const;
	DG_INLINE void ResizeIfNecessary  (dgInt32 index) const;

	dgInt32 GetElementSize() const;
	dgInt32 GetBytesCapacity () const;
	dgInt32 GetElementsCapacity () const; 
	dgMemoryAllocator* GetAllocator() const;
	void SetAllocator(dgMemoryAllocator* const allocator);

	protected:
	mutable T *m_array;
	private:
	mutable dgInt32 m_maxSize;
	dgInt32 m_aligmentInBytes;
	dgMemoryAllocator* m_allocator;
};

template<class T>
dgArray<T>::dgArray()
	:m_array(NULL)
	,m_maxSize(0)
	,m_aligmentInBytes(DG_MEMORY_GRANULARITY)
	,m_allocator(NULL)
{
	m_aligmentInBytes = 1 << dgExp2(m_aligmentInBytes);
}

template<class T>
dgArray<T>::dgArray (dgMemoryAllocator* const allocator, dgInt32 aligmentInBytes)
	:m_array(NULL)
	,m_maxSize(0)
	,m_aligmentInBytes(aligmentInBytes)
	,m_allocator(allocator)
{
	if (m_aligmentInBytes <= 0) {
		m_aligmentInBytes = DG_MEMORY_GRANULARITY;
	}
	m_aligmentInBytes = 1 << dgExp2(m_aligmentInBytes);
}

template<class T>
dgArray<T>::dgArray (const dgArray& source, dgInt32 itemsToCopy)
	:m_array(NULL)
	,m_maxSize(itemsToCopy)
	,m_aligmentInBytes(source.m_aligmentInBytes)
	,m_allocator(source.m_allocator)
{
	if (source.m_array) {
		m_array = (T*) m_allocator->MallocLow (sizeof (T) * itemsToCopy, m_aligmentInBytes);
		for (dgInt32 i = 0; i < itemsToCopy; i++) {
			m_array[i] = source.m_array[i];
		}
	}
}
template<class T>
dgArray<T>::dgArray(const dgArray& source)
	:m_array(NULL)
	,m_maxSize(source.m_maxSize)
	,m_aligmentInBytes(source.m_aligmentInBytes)
	,m_allocator(source.m_allocator)
{
	dgAssert(0);
	// use dgArray<T>::dgArray(const dgArray& source, dgInt32 itemsToCopy)
	if (source.m_array) {
		m_array = (T*)m_allocator->MallocLow(sizeof(T) * m_maxSize, m_aligmentInBytes);
		for (dgInt32 i = 0; i < m_maxSize; i++) {
			m_array[i] = source.m_array[i];
		}
	}
}


template<class T>
dgArray<T>::~dgArray ()
{
	if (m_array) {
		m_allocator->FreeLow (m_array);
	}
}

template<class T>
DG_INLINE const T& dgArray<T>::operator[] (dgInt32 i) const
{ 
	dgAssert (i >= 0);
	while (i >= m_maxSize) {
		Resize (i * 2);
	}
	return m_array[i];
}

template<class T>
DG_INLINE T& dgArray<T>::operator[] (dgInt32 i)
{
	dgAssert (i >= 0);
	while (i >= m_maxSize) {
		Resize (i * 2);
	}
	return m_array[i];
}

template<class T>
dgInt32 dgArray<T>::GetElementSize() const
{
	return sizeof (T);
}

template<class T>
dgInt32 dgArray<T>::GetElementsCapacity () const
{
	return m_maxSize;
}


template<class T>
dgInt32 dgArray<T>::GetBytesCapacity () const
{
	return  m_maxSize * GetElementSize();
}

template<class T>
void dgArray<T>::Clear () const
{
	if (m_array) {
		m_allocator->FreeLow (m_array);
		m_array = NULL;
	}
	m_maxSize = 0;
}

template<class T>
dgMemoryAllocator* dgArray<T>::GetAllocator() const
{
	return m_allocator;
}

template<class T>
void dgArray<T>::SetAllocator(dgMemoryAllocator* const allocator)
{
	dgAssert (!m_allocator);
	m_allocator = allocator;
}

template<class T>
void dgArray<T>::Resize (dgInt32 size) const
{
	if (size >= m_maxSize) {
		size = dgMax (size, 16);
		T* const newArray = (T*) m_allocator->MallocLow (dgInt32 (sizeof (T) * size), m_aligmentInBytes);
		if (m_array) {
			for (dgInt32 i = 0; i < m_maxSize; i ++) {
				newArray[i]	= m_array[i];
			}
			m_allocator->FreeLow (m_array);
		}
		m_array = newArray;
		m_maxSize = size;
	} else if (size < m_maxSize) {
		size = dgMax (size, 16);
		T* const newArray = (T*) m_allocator->MallocLow (dgInt32 (sizeof (T) * size), m_aligmentInBytes);
		if (m_array) {
			for (dgInt32 i = 0; i < size; i ++) {
				newArray[i]	= m_array[i];
			}
			m_allocator->FreeLow (m_array);
		}
		m_array = newArray;
		m_maxSize = size;
	}
}


template<class T>
DG_INLINE void dgArray<T>::ResizeIfNecessary  (dgInt32 size) const
{
	while (size >= m_maxSize) {
		Resize (m_maxSize * 2);
	}
}

#endif




