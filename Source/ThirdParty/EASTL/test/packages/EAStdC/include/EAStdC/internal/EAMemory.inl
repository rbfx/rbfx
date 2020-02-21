///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


// No include guards should be necessary since this is an internal file which
// is only ever #included in one place.


#include <EAAssert/eaassert.h>


// If EASTDC_MEMORY_INLINE_ENABLED is 1 (usually optimized builds) then this .inl file is 
// #included from EAMemory.h and thus we want the functions to be declared inline and don't 
// want them ever declared with DLL-export tags like EASTDC_API does. But otherwise (usually
// debug builds) this .inl file is #included from EAMemory.cpp and we don't want 'inline' 
// but do want EASTDC_API.
#if EASTDC_MEMORY_INLINE_ENABLED
	#define EASTDC_EAMEMORY_DECL inline
#else
	#define EASTDC_EAMEMORY_DECL EASTDC_API  // Maps to __declspec(dllexport) in DLL-builds (with VC++).
#endif


namespace EA
{
namespace StdC
{

	EASTDC_EAMEMORY_DECL void Memclear(void* pDestination, size_t n)
	{
		memset(pDestination, 0, n);
	}


	EASTDC_EAMEMORY_DECL void MemclearC(void* pDestination, size_t n)
	{
		memset(pDestination, 0, n);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8(void* pDestination, uint8_t c, size_t uint8Count)
	{
		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8C(void* pDestination, uint8_t c, size_t uint8Count)
	{
		if(c == 0)
		{
			Memclear(pDestination, uint8Count);
			return (uint8_t*)pDestination;
		}

		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8_128(void* pDestination, uint8_t c, size_t uint8Count)
	{
		// To do: Make an optimized version of this.
		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8_128C(void* pDestination, uint8_t c, size_t uint8Count)
	{
		if(c == 0)
		{
			Memclear(pDestination, uint8Count);
			return (uint8_t*)pDestination;
		}

		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL void* MemsetPointer(void* pDestination, const void* const pValue, size_t ptrCount)
	{
		#if (EA_PLATFORM_PTR_SIZE == 8)
			return Memset64(pDestination, (uint64_t)(uintptr_t)pValue, ptrCount);
		#else
			return Memset32(pDestination, (uint32_t)(uintptr_t)pValue, ptrCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char* Memcpy(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

			// Some compilers offer __builtin_memcpy, but we haven't found it to be faster than memcpy for any platforms
			// and it's significantly slower than memcpy for some platform/compiler combinations (e.g. SN compiler on PS3).
			return (char*)memcpy(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* MemcpyC(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		return (char*)memcpy(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* MemcpyS(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));


		return (char*)memcpy(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* Memcpy128(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		// This is expected to work with both cacheable and uncacheable memory, 
		// thus we can't use all alternative optimized functions that exist for memcpy.
		return (char*)memcpy(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* Memcpy128C(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		return (char*)memcpy(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* Memmove(void* pDestination, const void* pSource, size_t nByteCount)
	{
		// Some compilers offer __builtin_memmove, but we haven't found it to be faster than memcpy for any platforms
		// and it's significantly slower than memcpy for some platform/compiler combinations (e.g. SN compiler on PS3).
		return (char*)memmove(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char* MemmoveC(void* pDestination, const void* pSource, size_t nByteCount)
	{
		return (char*)memmove(pDestination, pSource, nByteCount);
	}

} // namespace StdC
} // namespace EA


// See the top of this file for #define EASTDC_API and an explanation of it.
#undef EASTDC_EAMEMORY_DECL

