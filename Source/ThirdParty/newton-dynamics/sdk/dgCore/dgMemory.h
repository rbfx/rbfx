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

#ifndef __dgMemory__
#define __dgMemory__

#include "dgStdafx.h"

class dgMemoryAllocator;

void* dgApi dgMalloc (size_t size, dgMemoryAllocator* const allocator);
void  dgApi dgFree (void* const ptr);

void* dgApi dgMallocStack (size_t size);
void* dgApi dgMallocAligned (size_t size, dgInt32 alignmentInBytes);
void  dgApi dgFreeStack (void* const ptr);

typedef void* (dgApi *dgMemAlloc) (dgUnsigned32 size);
typedef void (dgApi *dgMemFree) (void* const ptr, dgUnsigned32 size);


#define DG_CLASS_ALLOCATOR_NEW(allocator)			DG_INLINE void *operator new (size_t size, dgMemoryAllocator* const allocator) { return dgMalloc(size, allocator);}
#define DG_CLASS_ALLOCATOR_NEW_ARRAY(allocator)		DG_INLINE void *operator new[] (size_t size, dgMemoryAllocator* const allocator) { return dgMalloc(size, allocator);}
#define DG_CLASS_ALLOCATOR_DELETE(allocator)		DG_INLINE void operator delete (void* const ptr, dgMemoryAllocator* const allocator) { dgFree(ptr); }
#define DG_CLASS_ALLOCATOR_DELETE_ARRAY(allocator)	DG_INLINE void operator delete[] (void* const ptr, dgMemoryAllocator* const allocator) { dgFree(ptr); }
#define DG_CLASS_ALLOCATOR_NEW_DUMMY				DG_INLINE void *operator new (size_t size) { dgAssert (0); return dgMalloc(size, NULL);}
#define DG_CLASS_ALLOCATOR_NEW_ARRAY_DUMMY			DG_INLINE void *operator new[] (size_t size) { dgAssert (0); return dgMalloc(size, NULL);}
#define DG_CLASS_ALLOCATOR_DELETE_DUMMY				DG_INLINE void operator delete (void* const ptr) { dgFree(ptr); }
#define DG_CLASS_ALLOCATOR_DELETE_ARRAY_DUMMY		DG_INLINE void operator delete[] (void* const ptr) { dgFree(ptr); }


#define DG_CLASS_ALLOCATOR(allocator)				\
	DG_CLASS_ALLOCATOR_DELETE(allocator)			\
	DG_CLASS_ALLOCATOR_DELETE_ARRAY(allocator)		\
	DG_CLASS_ALLOCATOR_NEW(allocator)				\
	DG_CLASS_ALLOCATOR_NEW_ARRAY(allocator)			\
	DG_CLASS_ALLOCATOR_NEW_DUMMY					\
	DG_CLASS_ALLOCATOR_NEW_ARRAY_DUMMY				\
	DG_CLASS_ALLOCATOR_DELETE_DUMMY					\
	DG_CLASS_ALLOCATOR_DELETE_ARRAY_DUMMY

class dgMemoryAllocator
{
	#if (defined (__LP64__) || defined (_WIN_64_VER) || defined (_MINGW_64_VER) || defined (_POSIX_VER_64) || defined (_MACOSX_VER))
		#define DG_MEMORY_GRANULARITY_BITS		6	
	#else
		#define DG_MEMORY_GRANULARITY_BITS		5	
	#endif
	#define DG_MEMORY_GRANULARITY				(1 << DG_MEMORY_GRANULARITY_BITS)	
	#define DG_MEMORY_SIZE						(1024 - 64)
	#define DG_MEMORY_BIN_SIZE					(1024 * 16)
	#define DG_MEMORY_BIN_ENTRIES				(DG_MEMORY_SIZE / DG_MEMORY_GRANULARITY)

	public: 
	class dgMemoryBin;
	class dgMemoryInfo;
	class dgMemoryCacheEntry;

	class dgMemDirectory
	{
		public: 
		dgMemoryBin* m_first;
		dgMemoryCacheEntry* m_cache;
	};

	dgMemoryAllocator ();
	virtual ~dgMemoryAllocator ();

	void *operator new (size_t size);
	void operator delete (void* const ptr);
	dgInt32 GetMemoryUsed() const;

	void SetAllocatorsCallback (dgMemAlloc memAlloc, dgMemFree memFree);
	virtual void *MallocLow (dgInt32 size, dgInt32 alignment = DG_MEMORY_GRANULARITY);
	virtual void FreeLow (void* const retPtr);
	virtual void *Malloc (dgInt32 memsize);
	virtual void Free (void* const retPtr);

	static dgInt32 GetGlobalMemoryUsed ();
	static void SetGlobalAllocators (dgMemAlloc alloc, dgMemFree free);

	protected:
	dgMemoryAllocator (bool init)
		:m_free(NULL)
		,m_malloc(NULL)
		,m_enumerator(0)
		,m_memoryUsed(0)
		,m_isInList(0)
	{	
	}

	dgMemoryAllocator (dgMemAlloc memAlloc, dgMemFree memFree);

	dgMemFree m_free;
	dgMemAlloc m_malloc;
	dgMemDirectory m_memoryDirectory[DG_MEMORY_BIN_ENTRIES + 1]; 
	dgInt32 m_enumerator;
	dgInt32 m_memoryUsed;
	dgInt32 m_isInList;

	public:
	static dgInt32 m_lock;
};

class dgStackMemoryAllocator: public dgMemoryAllocator 
{
	public:
	DG_INLINE dgStackMemoryAllocator(void* const pool, dgInt32 size)
		:dgMemoryAllocator (false)
		,m_pool((dgInt8*) pool)
		,m_index(0)
		,m_size(size)
	{
	}

	DG_INLINE ~dgStackMemoryAllocator()
	{
	}

	DG_INLINE void* Alloc(dgInt32 size)
	{
		dgInt8* const ptr = (dgInt8*) (reinterpret_cast<uintptr_t>(m_pool + m_index + 15) & -0x10);
		m_index = dgInt32 (ptr - m_pool) + size;
		dgAssert (m_index < m_size);
		return ptr;
	}

	DG_INLINE void* Malloc(dgInt32 size)
	{
		return Alloc(size);
	}

	DG_INLINE void* MallocLow(dgInt32 size, dgInt32 alignment)
	{
		return Alloc(size);
	}

	DG_INLINE void Free(void* const retPtr)
	{
	}

	DG_INLINE void FreeLow(void* const retPtr)
	{
	}

	dgInt8* m_pool;
	dgInt32 m_index;
	dgInt32 m_size;
};

#endif

