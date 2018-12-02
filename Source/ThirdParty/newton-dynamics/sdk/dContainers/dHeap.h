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

#ifndef __dHeapBase__
#define __dHeapBase__


#include "dContainersStdAfx.h"
#include "dContainersAlloc.h"


template <class OBJECT, class KEY>
class dHeapBase: public dContainersAlloc
{
	protected:
	struct dHeapRecord 
	{
		dHeapRecord ()
            :m_key(), m_obj()
		{
		}

		dHeapRecord (KEY key, const OBJECT& obj)
			:m_key(key), m_obj(obj)
		{
		}

		~dHeapRecord ()
		{
		}

		KEY m_key;
		OBJECT m_obj;
	};

	dHeapBase (int maxElements);
	virtual ~dHeapBase ();
	

	public:
	void Flush (); 
	KEY MaxValue() const; 
	KEY Value(int i = 0) const;
	int GetCount() const;
	int GetMaxCount() const;
	const OBJECT& operator[] (int i) const;
	int Find (OBJECT &obj);
	int Find (KEY key);

	int m_curCount;
	int m_maxCount;
	bool m_allocated;
	dHeapRecord* m_pool;
};

template <class OBJECT, class KEY>
class dDownHeap: public dHeapBase<OBJECT, KEY>
{
	public:
	dDownHeap (int maxElements);
	//dDownHeap (const void* const buffer, int sizeInBytes);

	void Pop ();
	void Push (OBJECT &obj, KEY key);
	void Sort ();
	void Remove (int Index);

};

template <class OBJECT, class KEY>
class dUpHeap: public dHeapBase<OBJECT, KEY>
{
	public:
	dUpHeap (int maxElements);
	//dUpHeap (const void* const buffer, int sizeInBytes);

	void Pop ();
	void Push (OBJECT &obj, KEY key);
	void Sort ();
	void Remove (int Index);

};



template <class OBJECT, class KEY>
dHeapBase<OBJECT,KEY>::dHeapBase (int maxElements)
	:m_curCount(0)
	,m_maxCount(maxElements)
	,m_allocated(true)
	,m_pool(new dHeapRecord[maxElements])
{
	Flush();
}


template <class OBJECT, class KEY>
dHeapBase<OBJECT,KEY>::~dHeapBase ()
{   
	if (m_allocated == true) {
		delete[] m_pool;
	}
}


template <class OBJECT, class KEY>
KEY dHeapBase<OBJECT,KEY>::Value(int i) const
{
	return m_pool[i].m_key;
}


template <class OBJECT, class KEY>
int dHeapBase<OBJECT,KEY>::GetCount() const
{ 
	return m_curCount;
}


template <class OBJECT, class KEY>
void dHeapBase<OBJECT,KEY>::Flush () 
{
	m_curCount = 0;

	#ifdef _DEBUG
//	dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key = KEY (0);
	#endif
}


template <class OBJECT, class KEY>
KEY dHeapBase<OBJECT,KEY>::MaxValue() const 
{
	return m_pool[0].m_key;
}


template <class OBJECT, class KEY>
int dHeapBase<OBJECT,KEY>::GetMaxCount() const
{ 
	return m_maxCount;
}


template <class OBJECT, class KEY>
int dHeapBase<OBJECT,KEY>::Find (OBJECT &obj)
{
	// For now let perform a linear search
	// this is efficient if the size of the heap is small
	// ex: m_curCount < 32
	// this will be change to a binary search in the heap should the 
	// the size of the heap get larger than 32
	//	dAssert (m_curCount <= 32);
	for (int i = 0; i < m_curCount; i ++) {
		if (m_pool[i].obj == obj) {
			return i;
		}
	}
	return - 1;
}


template <class OBJECT, class KEY>
int dHeapBase<OBJECT,KEY>::Find (KEY key)
{
	// ex: m_curCount < 32
	// this will be change to a binary search in the heap shoud the 
	// the size of the heap get larger than 32
	dAssert (m_curCount <= 32);
	for (int i = 0; i < m_curCount; i ++)	{
		if (m_pool[i].m_key == key) {
			return i;
		}
	}
	return - 1;
}


template <class OBJECT, class KEY>
const OBJECT& dHeapBase<OBJECT,KEY>::operator[] (int i) const
{ 
	dAssert (i<= m_curCount);
	return m_pool[i].m_obj;
}


// **************************************************************************
//
// down Heap
//
// **************************************************************************
template <class OBJECT, class KEY>
dDownHeap<OBJECT,KEY>::dDownHeap (int maxElements)
	:dHeapBase<OBJECT, KEY> (maxElements)
{
}
/*
template <class OBJECT, class KEY>
dDownHeap<OBJECT,KEY>::dDownHeap (const void* const buffer, int sizeInBytes)
	:dHeapBase<OBJECT, KEY> (buffer, sizeInBytes)
{
}
*/

template <class OBJECT, class KEY>
void dDownHeap<OBJECT,KEY>::Push (OBJECT &obj, KEY key)
{
	dAssert ((dHeapBase<OBJECT,KEY>::m_curCount < dHeapBase<OBJECT,KEY>::m_maxCount));
	dHeapBase<OBJECT,KEY>::m_curCount ++;

	int j;
	int i = dHeapBase<OBJECT,KEY>::m_curCount;
	for (; i; i = j) {
		j = i >> 1;
		if (!j || (dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key > key)) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dAssert (i);
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = obj;
}


template <class OBJECT, class KEY>
void dDownHeap<OBJECT,KEY>::Remove (int index)
{
	dHeapBase<OBJECT,KEY>::m_curCount--;
	KEY key (dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key);

	int j;
	int i = index + 1;
	for (; i <= (dHeapBase<OBJECT,KEY>::m_curCount>>1); i = j) {
		j = i + i;
		if ((j < dHeapBase<OBJECT,KEY>::m_curCount) && 
			(dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key < dHeapBase<OBJECT,KEY>::m_pool[j].m_key)) {
				j ++;
		}
		if (key >= dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj;
}

template <class OBJECT, class KEY>
void dDownHeap<OBJECT,KEY>::Pop ()
{
	dHeapBase<OBJECT,KEY>::m_curCount--;
	KEY key (dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key);
	int j;
	int i = 1;
	for (; i <= (dHeapBase<OBJECT,KEY>::m_curCount>>1); i = j) {
		j = i + i;
		if ((j < dHeapBase<OBJECT,KEY>::m_curCount) && 
			(dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key < dHeapBase<OBJECT,KEY>::m_pool[j].m_key)) {
			j ++;
		}
		if (key >= dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj;
}


template <class OBJECT, class KEY>
void dDownHeap<OBJECT,KEY>::Sort ()
{
	int count = dHeapBase<OBJECT,KEY>::m_curCount;
	for (int i = 1; i < count; i ++) {
		KEY key (dHeapBase<OBJECT,KEY>::m_pool[0].m_key);
		OBJECT obj (dHeapBase<OBJECT,KEY>::m_pool[0].m_obj);

		Pop();

		dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key = key;
		dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj = obj;
	}

	dHeapBase<OBJECT,KEY>::m_curCount = count;
	for (int i = 0; i < count / 2; i ++) {
		KEY key (dHeapBase<OBJECT,KEY>::m_pool[i].m_key);
		OBJECT obj (dHeapBase<OBJECT,KEY>::m_pool[i].m_obj);

		dHeapBase<OBJECT,KEY>::m_pool[i].m_key = dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_key;
		dHeapBase<OBJECT,KEY>::m_pool[i].m_obj = dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_obj;

		dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_key = key;
		dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_obj = obj;
	}
}


// **************************************************************************
//
// down Heap
//
// **************************************************************************
template <class OBJECT, class KEY>
dUpHeap<OBJECT,KEY>::dUpHeap (int maxElements)
	:dHeapBase<OBJECT, KEY> (maxElements)
{
}
/*
template <class OBJECT, class KEY>
dUpHeap<OBJECT,KEY>::dUpHeap (const void* const buffer, int sizeInBytes)
	:dHeapBase<OBJECT, KEY> (buffer, sizeInBytes)
{
}
*/

template <class OBJECT, class KEY>
void dUpHeap<OBJECT,KEY>::Push (OBJECT &obj, KEY key)
{
	dAssert ((dHeapBase<OBJECT,KEY>::m_curCount < dHeapBase<OBJECT,KEY>::m_maxCount));
	dHeapBase<OBJECT,KEY>::m_curCount ++;
	
	int j;
	int i = dHeapBase<OBJECT,KEY>::m_curCount;
	for (; i; i = j) {
		j = i >> 1;
		if (!j || (dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key < key)) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dAssert (i);
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = obj;
}


template <class OBJECT, class KEY>
void dUpHeap<OBJECT,KEY>::Sort ()
{
	int count = dHeapBase<OBJECT,KEY>::m_curCount;
	for (int i = 1; i < count; i ++) {
		KEY key (dHeapBase<OBJECT,KEY>::m_pool[0].m_key);
		OBJECT obj (dHeapBase<OBJECT,KEY>::m_pool[0].m_obj);

		Pop();

		dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key = key;
		dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj = obj;
	}

	dHeapBase<OBJECT,KEY>::m_curCount = count;
	for (int i = 0; i < count / 2; i ++) {
		KEY key (dHeapBase<OBJECT,KEY>::m_pool[i].m_key);
		OBJECT obj (dHeapBase<OBJECT,KEY>::m_pool[i].m_obj);

		dHeapBase<OBJECT,KEY>::m_pool[i].m_key = dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_key;
		dHeapBase<OBJECT,KEY>::m_pool[i].m_obj = dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_obj;

		dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_key = key;
		dHeapBase<OBJECT,KEY>::m_pool[count - i - 1].m_obj = obj;
	}
}


template <class OBJECT, class KEY>
void dUpHeap<OBJECT,KEY>::Remove (int index)
{
	dHeapBase<OBJECT,KEY>::m_curCount--;
	KEY key (dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key);

	int j;
	int i = index + 1;
	for (; i <= (dHeapBase<OBJECT,KEY>::m_curCount>>1); i = j) {
		j = i + i;
		if ((j < dHeapBase<OBJECT,KEY>::m_curCount) && 
			(dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key > dHeapBase<OBJECT,KEY>::m_pool[j].m_key)) {
				j ++;
		}
		if (key <= dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj;
}


template <class OBJECT, class KEY>
void dUpHeap<OBJECT,KEY>::Pop ()
{
	dHeapBase<OBJECT,KEY>::m_curCount--;
	KEY key (dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_key);

	int j;
	int i = 1;
	for (; i <= (dHeapBase<OBJECT,KEY>::m_curCount>>1); i = j) {
		j = i + i;
		if ((j < dHeapBase<OBJECT,KEY>::m_curCount) && 
			(dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key > dHeapBase<OBJECT,KEY>::m_pool[j].m_key)) {
				j ++;
		}
		if (key <= dHeapBase<OBJECT,KEY>::m_pool[j - 1].m_key) {
			break;
		}
		dHeapBase<OBJECT,KEY>::m_pool[i - 1] = dHeapBase<OBJECT,KEY>::m_pool[j - 1];
	}
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_key = key;
	dHeapBase<OBJECT,KEY>::m_pool[i - 1].m_obj = dHeapBase<OBJECT,KEY>::m_pool[dHeapBase<OBJECT,KEY>::m_curCount].m_obj;
}


#endif

