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
#include "dgList.h"
#include "dgDebug.h"
#include "dgMemory.h"

dgInt32 dgMemoryAllocator::m_lock = 0;
#if 0
class dgAllocDebugCheck
{
	public:
	dgAllocDebugCheck()
	{
		dgAssert(!dgMemoryAllocator::m_lock);
		dgAtomicExchangeAndAdd(&dgMemoryAllocator::m_lock, 1);
	}
	~dgAllocDebugCheck()
	{
		dgAtomicExchangeAndAdd(&dgMemoryAllocator::m_lock, -1);
	}
};
#define DG_MEMORY_LOCK() dgAllocDebugCheck m_lock;
#else 
#define DG_MEMORY_LOCK() dgScopeSpinPause lock (&dgMemoryAllocator::m_lock);
#endif

class dgMemoryAllocator::dgMemoryBin
{
	public:
	class dgMemoryBinInfo
	{
		public:
		dgInt32 m_count;
		dgInt32 m_totalCount;
		dgInt32 m_stepInBites;
		dgMemoryBin* m_next;
		dgMemoryBin* m_prev;
	};
	char m_pool[DG_MEMORY_BIN_SIZE - sizeof (dgMemoryBinInfo)-DG_MEMORY_GRANULARITY * 2];
	dgMemoryBinInfo m_info;
};

class dgMemoryAllocator::dgMemoryCacheEntry
{
	public:
	dgMemoryCacheEntry* m_next;
	dgMemoryCacheEntry* m_prev;
};

class dgMemoryAllocator::dgMemoryInfo
{
	public:
	void *m_ptr;
	dgMemoryAllocator* m_allocator;
	dgInt32 m_size;
	dgInt32 m_enum;

	#ifdef _DEBUG
	dgInt32 m_workingSize;
	#endif

	DG_INLINE void SaveInfo(dgMemoryAllocator* const allocator, void* const ptr, dgInt32 size, dgInt32& enumerator, dgInt32 workingSize = 0)
	{
		m_ptr = ptr;
		m_size = size;
		m_enum = enumerator;
		enumerator++;
		m_allocator = allocator;
#ifdef _DEBUG
		m_workingSize = workingSize;
#endif
	}
};


class dgGlobalAllocator: public dgMemoryAllocator, public dgList<dgMemoryAllocator*>
{
	public:
	dgGlobalAllocator ()
		:dgMemoryAllocator (__malloc__, __free__), dgList<dgMemoryAllocator*> (NULL)
	{
		SetAllocator (this);
	}

	~dgGlobalAllocator ()
	{
		dgAssert (GetCount() == 0);
	}

	static void* dgApi __malloc__ (dgUnsigned32 size) 
	{
		return malloc (size);
	}

	static void dgApi __free__ (void* const ptr, dgUnsigned32 size)
	{
		free (ptr);
	}

	void operator delete (void* const ptr)
	{
		dgAssert (0);
		free (ptr);
	}

	dgInt32 GetMemoryUsed () const
	{
		dgInt32 mem = m_memoryUsed;
		for (dgList<dgMemoryAllocator*>::dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			mem += node->GetInfo()->GetMemoryUsed();
		}
		return mem;
	}

	static dgGlobalAllocator& GetGlobalAllocator()
	{
		static dgGlobalAllocator m_globalAllocator;
		return m_globalAllocator;
	}
};

dgMemoryAllocator::dgMemoryAllocator ()
	:m_free(NULL)
	,m_malloc(NULL)
	,m_enumerator(0)
	,m_memoryUsed(0)
	,m_isInList(1)
{
	SetAllocatorsCallback (dgGlobalAllocator::GetGlobalAllocator().m_malloc, dgGlobalAllocator::GetGlobalAllocator().m_free);
	memset (m_memoryDirectory, 0, sizeof (m_memoryDirectory));
	dgGlobalAllocator::GetGlobalAllocator().Append(this);
}

dgMemoryAllocator::dgMemoryAllocator (dgMemAlloc memAlloc, dgMemFree memFree)
	:m_free(NULL)
	,m_malloc(NULL)
	,m_enumerator(0)
	,m_memoryUsed(0)
	,m_isInList(0)
{
	SetAllocatorsCallback (memAlloc, memFree);
	memset (m_memoryDirectory, 0, sizeof (m_memoryDirectory));
}

dgMemoryAllocator::~dgMemoryAllocator  ()
{
	if (m_isInList) {
		dgGlobalAllocator::GetGlobalAllocator().Remove(this);
	}
	dgAssert (m_memoryUsed == 0);
}


void *dgMemoryAllocator::operator new (size_t size) 
{ 
	return dgMallocStack(size);
}

void dgMemoryAllocator::operator delete (void* const ptr) 
{ 
	dgFreeStack(ptr); 
}

dgInt32 dgMemoryAllocator::GetMemoryUsed() const
{
	return m_memoryUsed;
}

void dgMemoryAllocator::SetAllocatorsCallback (dgMemAlloc memAlloc, dgMemFree memFree)
{
	m_free = memFree;
	m_malloc = memAlloc;
}

void *dgMemoryAllocator::MallocLow (dgInt32 workingSize, dgInt32 alignment)
{
	dgAssert (alignment >= DG_MEMORY_GRANULARITY);
	dgAssert (((-alignment) & (alignment - 1)) == 0);
	dgInt32 size = workingSize + alignment * 2;
	void* const ptr = m_malloc(dgUnsigned32 (size));
	dgAssert (ptr);
#ifdef _DEBUG
	memset(ptr, 99, size_t(size));
#endif

	dgUnsigned64 val = dgUnsigned64 (PointerToInt(ptr));
	val = (val & dgUnsigned64(-alignment)) + alignment * 2;
	void* const retPtr = IntToPointer (val);

	dgMemoryInfo* const info = ((dgMemoryInfo*) (retPtr)) - 1;
	info->SaveInfo(this, ptr, size, m_enumerator, workingSize);

	dgAtomicExchangeAndAdd (&m_memoryUsed, size);
	return retPtr;
}

void dgMemoryAllocator::FreeLow (void* const retPtr)
{
	dgMemoryInfo* const info = ((dgMemoryInfo*) (retPtr)) - 1;
	dgAssert (info->m_allocator == this);

	dgAtomicExchangeAndAdd (&m_memoryUsed, -info->m_size);

#ifdef _DEBUG
	memset (retPtr, 0, size_t(info->m_workingSize));
#endif

	m_free (info->m_ptr, dgUnsigned32 (info->m_size));
}

// alloca memory on pool that are quantized to DG_MEMORY_GRANULARITY
// if memory size is larger than DG_MEMORY_BIN_ENTRIES then the memory is not placed into a pool
void *dgMemoryAllocator::Malloc (dgInt32 memsize)
{
	dgAssert (dgInt32 (sizeof (dgMemoryCacheEntry) + sizeof (dgInt32) + sizeof(dgInt32)) <= DG_MEMORY_GRANULARITY);

	dgInt32 size = memsize + DG_MEMORY_GRANULARITY - 1;
	size &= (-DG_MEMORY_GRANULARITY);

	dgInt32 paddedSize = size + DG_MEMORY_GRANULARITY; 
	dgInt32 entry = paddedSize >> DG_MEMORY_GRANULARITY_BITS;	

	void* ptr;
	if (entry >= DG_MEMORY_BIN_ENTRIES) {
		ptr = MallocLow (size);
	} else {
		if (!m_memoryDirectory[entry].m_cache) {
			dgMemoryBin* const bin = (dgMemoryBin*) MallocLow (sizeof (dgMemoryBin));

			dgInt32 count = dgInt32 (sizeof (bin->m_pool) / paddedSize);
			bin->m_info.m_count = 0;
			bin->m_info.m_totalCount = count;
			bin->m_info.m_stepInBites = paddedSize;
			bin->m_info.m_next = m_memoryDirectory[entry].m_first;
			bin->m_info.m_prev = NULL;
			if (bin->m_info.m_next) {
				bin->m_info.m_next->m_info.m_prev = bin;
			}

			m_memoryDirectory[entry].m_first = bin;

			dgInt8* charPtr = reinterpret_cast<dgInt8*>(bin->m_pool);
			m_memoryDirectory[entry].m_cache = (dgMemoryCacheEntry*)charPtr;

			for (dgInt32 i = 0; i < count; i ++) {
				dgMemoryCacheEntry* const cashe = (dgMemoryCacheEntry*) charPtr;
				cashe->m_next = (dgMemoryCacheEntry*) (charPtr + paddedSize);
				cashe->m_prev = (dgMemoryCacheEntry*) (charPtr - paddedSize);
				dgMemoryInfo* const info = ((dgMemoryInfo*) (charPtr + DG_MEMORY_GRANULARITY)) - 1;						
				info->SaveInfo(this, bin, entry, m_enumerator, memsize);
				charPtr += paddedSize;
			}
			dgMemoryCacheEntry* const cashe = (dgMemoryCacheEntry*) (charPtr - paddedSize);
			cashe->m_next = NULL;
			m_memoryDirectory[entry].m_cache->m_prev = NULL;
		}


		dgAssert (m_memoryDirectory[entry].m_cache);

		dgMemoryCacheEntry* const cashe = m_memoryDirectory[entry].m_cache;
		m_memoryDirectory[entry].m_cache = cashe->m_next;
		if (cashe->m_next) {
			cashe->m_next->m_prev = NULL;
		}

		ptr = ((dgInt8*)cashe) + DG_MEMORY_GRANULARITY;

		dgMemoryInfo* info;
		info = ((dgMemoryInfo*) (ptr)) - 1;
		dgAssert (info->m_allocator == this);

		dgMemoryBin* const bin = (dgMemoryBin*) info->m_ptr;
		bin->m_info.m_count ++;
	}
	return ptr;
}

// alloca memory on pool that are quantized to DG_MEMORY_GRANULARITY
// if memory size is larger than DG_MEMORY_BIN_ENTRIES then the memory is not placed into a pool
void dgMemoryAllocator::Free (void* const retPtr)
{
	dgMemoryInfo* const info = ((dgMemoryInfo*) (retPtr)) - 1;
	dgAssert (info->m_allocator == this);

	dgInt32 entry = info->m_size;

	if (entry >= DG_MEMORY_BIN_ENTRIES) {
		FreeLow (retPtr);
	} else {
		dgMemoryCacheEntry* const cashe = (dgMemoryCacheEntry*) (((char*)retPtr) - DG_MEMORY_GRANULARITY) ;

		dgMemoryCacheEntry* const tmpCashe = m_memoryDirectory[entry].m_cache;
		if (tmpCashe) {
			dgAssert (!tmpCashe->m_prev);
			tmpCashe->m_prev = cashe;
		}
		cashe->m_next = tmpCashe;
		cashe->m_prev = NULL;

		m_memoryDirectory[entry].m_cache = cashe;

		dgMemoryBin* const bin = (dgMemoryBin *) info->m_ptr;

		dgAssert (bin);
#ifdef _DEBUG
		dgAssert ((bin->m_info.m_stepInBites - DG_MEMORY_GRANULARITY) > 0);
		memset (retPtr, 0, size_t(bin->m_info.m_stepInBites - DG_MEMORY_GRANULARITY));
#endif

		bin->m_info.m_count --;
		if (bin->m_info.m_count == 0) {

			dgInt32 count = bin->m_info.m_totalCount;
			dgInt32 sizeInBytes = bin->m_info.m_stepInBites;
			char* charPtr = bin->m_pool;
			for (dgInt32 i = 0; i < count; i ++) {
				dgMemoryCacheEntry* const tmpCashe1 = (dgMemoryCacheEntry*)charPtr;
				charPtr += sizeInBytes;

				if (tmpCashe1 == m_memoryDirectory[entry].m_cache) {
					m_memoryDirectory[entry].m_cache = tmpCashe1->m_next;
				}

				if (tmpCashe1->m_prev) {
					tmpCashe1->m_prev->m_next = tmpCashe1->m_next;
				}

				if (tmpCashe1->m_next) {
					tmpCashe1->m_next->m_prev = tmpCashe1->m_prev;
				}
			}

			if (m_memoryDirectory[entry].m_first == bin) {
				m_memoryDirectory[entry].m_first = bin->m_info.m_next;
			}

			if (bin->m_info.m_next) {
				bin->m_info.m_next->m_info.m_prev = bin->m_info.m_prev;
			}
			if (bin->m_info.m_prev) {
				bin->m_info.m_prev->m_info.m_next = bin->m_info.m_next;
			}

			FreeLow (bin);
		}
	}
}

// Set the pointer of memory allocation functions
void dgMemoryAllocator::SetGlobalAllocators (dgMemAlloc malloc, dgMemFree free)
{
	dgGlobalAllocator::GetGlobalAllocator().SetAllocatorsCallback (malloc, free);
}

dgInt32 dgMemoryAllocator::GetGlobalMemoryUsed ()
{
	return dgGlobalAllocator::GetGlobalAllocator().GetMemoryUsed();
}

// this can be used by function that allocates large memory pools memory locally on the stack
// this by pases the pool allocation because this should only be used for very large memory blocks.
// this was using virtual memory on windows but 
// but because of many complaint I changed it to use malloc and free
void* dgApi dgMallocStack (size_t size)
{
	DG_MEMORY_LOCK();
	void * const ptr = dgGlobalAllocator::GetGlobalAllocator().MallocLow (dgInt32 (size));
	return ptr;
}

void* dgApi dgMallocAligned (size_t size, dgInt32 align)
{
	DG_MEMORY_LOCK();
	void * const ptr = dgGlobalAllocator::GetGlobalAllocator().MallocLow (dgInt32 (size), align);
	return ptr;
}

// this can be used by function that allocates large memory pools memory locally on the stack
// this by pases the pool allocation because this should only be used for very large memory blocks.
// this was using virtual memory on windows but 
// but because of many complaint I changed it to use malloc and free
void  dgApi dgFreeStack (void* const ptr)
{
	DG_MEMORY_LOCK();
	dgGlobalAllocator::GetGlobalAllocator().FreeLow (ptr);
}

// general memory allocation for all data in the library
void* dgApi dgMalloc (size_t size, dgMemoryAllocator* const allocator) 
{
	void* ptr = NULL;
	dgAssert (allocator);

	DG_MEMORY_LOCK();
	if (size) {
		ptr = allocator->Malloc (dgInt32 (size));
	}
	return ptr;
}

// general deletion allocation for all data in the library
void dgApi dgFree (void* const ptr)
{
	if (ptr) {
		DG_MEMORY_LOCK();
		dgMemoryAllocator::dgMemoryInfo* const info = ((dgMemoryAllocator::dgMemoryInfo*) ptr) - 1;
		dgAssert (info->m_allocator);
		info->m_allocator->Free (ptr);
	}
}
