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


#ifndef __DG_SORT_H__
#define __DG_SORT_H__
#include "dgStdafx.h"
#include "dgHeap.h"
#include "dgThreadHive.h"

#define DG_PARALLET_SORT_BATCH_SIZE	1024

template <class T>
dgInt32 dgBinarySearch(T const* array, dgInt32 elements, const T& entry, dgInt32(*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	dgInt32 index0 = 0;
	dgInt32 index2 = elements - 1;

	while ((index2 - index0) > 4) {
		dgInt32 index1 = (index0 + index2) >> 1;
		dgInt32 test = compare(&array[index1], &entry, context);
		if (test < 0) {
			index0 = index1;
		}
		else {
			index2 = index1;
		}
	}

	index0 = (index0 > 0) ? index0 - 1 : 0;
	index2 = ((index2 + 1) < elements) ? index2 + 1 : elements;
	dgInt32 index = index0 - 1;
	for (dgInt32 i = index0; i < index2; i++) {
		dgInt32 test = compare(&array[i], &entry, context);
		if (!test) {
			return i;
		}
		else if (test > 0) {
			break;
		}
		index = i;
	}
	return index;
}

template <class T>
dgInt32 dgBinarySearchIndirect(T** const array, dgInt32 elements, const T& entry, dgInt32(*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	dgInt32 index0 = 0;
	dgInt32 index2 = elements - 1;

	while ((index2 - index0) > 4) {
		dgInt32 index1 = (index0 + index2) >> 1;
		dgInt32 test = compare(array[index1], &entry, context);
		if (test < 0) {
			index0 = index1;
		}
		else {
			index2 = index1;
		}
	}

	index0 = (index0 > 0) ? index0 - 1 : 0;
	index2 = ((index2 + 1) < elements) ? index2 + 1 : elements;
	dgInt32 index = index0 - 1;
	for (dgInt32 i = index0; i < index2; i++) {
		dgInt32 test = compare(array[i], &entry, context);
		if (!test) {
			return i;
		}
		else if (test > 0) {
			break;
		}
		index = i;
	}
	return index;
}

template <class T>
void dgRadixSort(T* const array, T* const tmpArray, dgInt32 elements, dgInt32 radixPass, dgInt32(*getRadixKey) (const T* const  A, void* const context), void* const context = NULL)
{
	dgInt32 scanCount[256];
	dgInt32 histogram[256][4];

	dgAssert(radixPass >= 1);
	dgAssert(radixPass <= 4);

	memset(histogram, 0, sizeof(histogram));
	for (dgInt32 i = 0; i < elements; i++) {
		dgInt32 key = getRadixKey(&array[i], context);
		for (dgInt32 j = 0; j < radixPass; j++) {
			dgInt32 radix = (key >> (j << 3)) & 0xff;
			histogram[radix][j] = histogram[radix][j] + 1;
		}
	}

	for (dgInt32 radix = 0; radix < radixPass; radix += 2) {
		scanCount[0] = 0;
		for (dgInt32 i = 1; i < 256; i++) {
			scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix];
		}
		dgInt32 radixShift = radix << 3;
		for (dgInt32 i = 0; i < elements; i++) {
			dgInt32 key = (getRadixKey(&array[i], context) >> radixShift) & 0xff;
			dgInt32 index = scanCount[key];
			tmpArray[index] = array[i];
			scanCount[key] = index + 1;
		}

		if ((radix + 1) < radixPass) {
			scanCount[0] = 0;
			for (dgInt32 i = 1; i < 256; i++) {
				scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix + 1];
			}

			dgInt32 radixShift = (radix + 1) << 3;
			for (dgInt32 i = 0; i < elements; i++) {
				dgInt32 key = (getRadixKey(&array[i], context) >> radixShift) & 0xff;
				dgInt32 index = scanCount[key];
				array[index] = tmpArray[i];
				scanCount[key] = index + 1;
			}
		}
		else {
			memcpy(array, tmpArray, elements * sizeof(T));
		}
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i++) {
		dgAssert(getRadixKey(&array[i], context) <= getRadixKey(&array[i + 1], context));
	}
#endif
}

template <class T>
void dgSort(T* const array, dgInt32 elements, dgInt32(*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	DG_TRACKTIME(__FUNCTION__);
	const dgInt32 batchSize = 8;
	dgInt32 stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	dgInt32 stackIndex = 1;
	while (stackIndex) {
		stackIndex--;
		dgInt32 lo = stack[stackIndex][0];
		dgInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > batchSize) {
			dgInt32 mid = (lo + hi) >> 1;
			if (compare(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			if (compare(&array[mid], &array[hi], context) > 0) {
				dgSwap(array[mid], array[hi]);
			}
			if (compare(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			dgInt32 i = lo + 1;
			dgInt32 j = hi - 1;
			T pivot(array[mid]);
			do {
				while (compare(&array[i], &pivot, context) < 0) i++;
				while (compare(&array[j], &pivot, context) > 0) j--;

				if (i <= j) {
					dgSwap(array[i], array[j]);
					i++;
					j--;
				}
			} while (i <= j);

			if (i < hi) {
				stack[stackIndex][0] = i;
				stack[stackIndex][1] = hi;
				stackIndex++;
			}
			if (lo < j) {
				stack[stackIndex][0] = lo;
				stack[stackIndex][1] = j;
				stackIndex++;
			}
			dgAssert(stackIndex < dgInt32(sizeof(stack) / (2 * sizeof(stack[0][0]))));
		}
	}

	dgInt32 stride = batchSize + 1;
	if (elements < stride) {
		stride = elements;
	}
	for (dgInt32 i = 1; i < stride; i++) {
		if (compare(&array[0], &array[i], context) > 0) {
			dgSwap(array[0], array[i]);
		}
	}

	for (dgInt32 i = 1; i < elements; i++) {
		dgInt32 j = i;
		T tmp(array[i]);
		for (; compare(&array[j - 1], &tmp, context) > 0; j--) {
			dgAssert(j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i++) {
		dgAssert(compare(&array[i], &array[i + 1], context) <= 0);
	}
#endif
}

template <class T>
void dgSortIndirect(T** const array, dgInt32 elements, dgInt32(*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	DG_TRACKTIME(__FUNCTION__);
	const dgInt32 batchSize = 8;
	dgInt32 stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	dgInt32 stackIndex = 1;
	while (stackIndex) {
		stackIndex--;
		dgInt32 lo = stack[stackIndex][0];
		dgInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > batchSize) {
			dgInt32 mid = (lo + hi) >> 1;
			if (compare(array[lo], array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			if (compare(array[mid], array[hi], context) > 0) {
				dgSwap(array[mid], array[hi]);
			}
			if (compare(array[lo], array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			dgInt32 i = lo + 1;
			dgInt32 j = hi - 1;
			T* val(array[mid]);
			do {
				while (compare(array[i], val, context) < 0) i++;
				while (compare(array[j], val, context) > 0) j--;

				if (i <= j) {
					dgSwap(array[i], array[j]);
					i++;
					j--;
				}
			} while (i <= j);

			if (i < hi) {
				stack[stackIndex][0] = i;
				stack[stackIndex][1] = hi;
				stackIndex++;
			}
			if (lo < j) {
				stack[stackIndex][0] = lo;
				stack[stackIndex][1] = j;
				stackIndex++;
			}
			dgAssert(stackIndex < dgInt32(sizeof(stack) / (2 * sizeof(stack[0][0]))));
		}
	}

	dgInt32 stride = batchSize + 1;
	if (elements < stride) {
		stride = elements;
	}
	for (dgInt32 i = 1; i < stride; i++) {
		if (compare(array[0], array[i], context) > 0) {
			dgSwap(array[0], array[i]);
		}
	}

	for (dgInt32 i = 1; i < elements; i++) {
		dgInt32 j = i;
		T* tmp(array[i]);
		for (; compare(array[j - 1], tmp, context) > 0; j--) {
			dgAssert(j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i++) {
		dgAssert(compare(array[i], array[i + 1], context) <= 0);
	}
#endif
}


class dgParallelSortRange
{
	public:
	dgParallelSortRange() {}
	dgParallelSortRange(dgInt32 i0, dgInt32 i1)
		:m_i0(i0)
		, m_i1(i1)
	{
	}
	dgInt32 m_i0;
	dgInt32 m_i1;
};

template <class T>
class dgParallelSourtDesc
{
	public:
	typedef dgInt32(*CompareFunction) (const T* const A, const T* const B, void* const context);

	dgParallelSourtDesc(dgThreadHive& threadPool, T* const array, dgInt32 elements, CompareFunction compareFunct, void* const context)
		:m_data(array)
		,m_callback(compareFunct)
		,m_context(context)
		,m_threadCount(threadPool.GetThreadCount())
	{
		dgDownHeap<dgParallelSortRange, dgInt32> rangeMerge(m_buffer, sizeof(m_buffer));

		dgParallelSortRange range(0, elements - 1);
		rangeMerge.Push(range, elements);

		const dgInt32 batchSize = DG_PARALLET_SORT_BATCH_SIZE;
		const dgInt32 rangesCount = m_threadCount;

		while ((rangeMerge.GetCount() < rangesCount) && (rangeMerge.Value() > batchSize)) {
			dgParallelSortRange splitRange(rangeMerge[0]);
			rangeMerge.Pop();

			const dgInt32 lo = splitRange.m_i0;
			const dgInt32 hi = splitRange.m_i1;
			const dgInt32 mid = (lo + hi) >> 1;
			if (m_callback(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			if (m_callback(&array[mid], &array[hi], context) > 0) {
				dgSwap(array[mid], array[hi]);
			}
			if (m_callback(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			dgInt32 i = lo;
			dgInt32 j = hi;
			T pivot(array[mid]);
			for (;;) {
				do {
					i++;
				} while (m_callback(&array[i], &pivot, context) < 0);
				do {
					j--;
				} while (m_callback(&array[j], &pivot, context) > 0);

				if (i >= j) {
					break;
				}
				dgSwap(array[i], array[j]);
			}

			dgParallelSortRange newRange0(lo, j);
			dgParallelSortRange newRange1(j + 1, hi);
			rangeMerge.Push(newRange0, j - lo + 1);
			rangeMerge.Push(newRange1, hi - j);
		}

		m_rangeMerge = &rangeMerge;
		for (dgInt32 i = 0; i < m_threadCount; i++) {
			threadPool.QueueJob(dgParallelKernel, this, NULL, __FUNCTION__);
		}
		threadPool.SynchronizationBarrier();

		#ifdef _DEBUG
		for (dgInt32 i = 0; i < (elements - 1); i++) {
			dgAssert(m_callback(&m_data[i], &m_data[i + 1], context) <= 0);
		}
		#endif
	}

	static void dgParallelKernel(void* const context, void* const worldContext, dgInt32 threadID)
	{
		dgParallelSourtDesc<T>* const me = (dgParallelSourtDesc<T>*) context;
		me->dgParallelKernel(threadID);
	}

	void dgParallelKernel(dgInt32 threadID) 
	{
		dgDownHeap<dgParallelSortRange, dgInt32>& rangeMerge = *((dgDownHeap<dgParallelSortRange, dgInt32>*)m_rangeMerge);
		const dgInt32 count = rangeMerge.GetCount();
		for (dgInt32 i = threadID; i < count; i += m_threadCount) {
			dgParallelSortRange range(rangeMerge[i]);
			T* const data = &m_data[range.m_i0];
			dgSort(data, range.m_i1 - range.m_i0 + 1, m_callback, m_context);
		}
	}

	T* m_data;
	void* m_rangeMerge;
	CompareFunction m_callback;
	void* m_context;
	int m_threadCount;
	dgInt8 m_buffer[256 * sizeof (dgParallelSortRange)];
};

template <class T>
void dgParallelSort(dgThreadHive& threadPool, T* const array, dgInt32 elements, dgInt32(*compare) (const T* const A, const T* const B, void* const context), void* const context = NULL)
{
	DG_TRACKTIME(__FUNCTION__);
	if ((threadPool.GetThreadCount() <= 1) || (elements < DG_PARALLET_SORT_BATCH_SIZE)) {
//	if (1) {
		dgSort(array, elements, compare, context);
	} else {
		dgParallelSourtDesc<T> sort(threadPool, array, elements, compare, context);
	}
}

#endif
