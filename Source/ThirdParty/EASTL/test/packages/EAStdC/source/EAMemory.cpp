///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAMemory.h>
#include <EAAssert/eaassert.h>



// In optimized non-debug builds, we inline various functions.
// We don't inline these functions in debug builds because in debug builds they
// contain diagnostic code that can't be exposed in headers because that would the 
// user of this header to #include all the debug functionality headers, which isn't 
// feasible.
#if !EASTDC_MEMORY_INLINE_ENABLED
	#include <EAStdC/internal/EAMemory.inl>
#endif



namespace EA
{
namespace StdC
{

///////////////////////////////////////////////////////////////////////////
// Deprecated functions
//
#if EASTDC_MEMCPY16_ENABLED
	// This function is deprecated. It was mistakenly created during a code migration.
	// It is scheduled for removal in a future version of this package.
	EASTDC_API char16_t* Memcpy(char16_t* pDestination, const char16_t* pSource, size_t nCharCount)
	{
		return (char16_t*)memcpy(pDestination, pSource, nCharCount * sizeof(char16_t));
	}
#endif

#if EASTDC_MEMCPY16_ENABLED
	// This function is deprecated. It was mistakenly created during a code migration.
	// It is scheduled for removal in a future version of this package.
	EASTDC_API char16_t* Memmove(char16_t* pDestination, const char16_t* pSource, size_t nCharCount)
	{
		return (char16_t*)memmove(pDestination, pSource, nCharCount * sizeof(char16_t));
	}
#endif


///////////////////////////////////////////////////////////////////////////
// rwstdc compatibility
// These functions implement the same named function and argument types
// as the corresponding functions from the rwstdc package.
EASTDC_API void MemFill16(void* pDestination, uint16_t c, unsigned int byteCount)
{
	Memfill16(pDestination, c, (size_t)byteCount);
}

EASTDC_API void MemFill32(void* pDestination, unsigned int c, unsigned int byteCount)
{
	Memfill32(pDestination, (uint32_t)c, (size_t)byteCount);
}

EASTDC_API void MemFillSpecific(void* pDestination, const void* pSource, unsigned int destByteCount, unsigned int sourceByteCount)
{
	MemfillSpecific(pDestination, pSource, (size_t)destByteCount, (size_t)sourceByteCount);
}







EASTDC_API uint16_t* Memset16(void* pDest, uint16_t c, size_t count)
{
	// Instead of casting between types, we just create a union.
	union PointerUnion
	{
		void*     mpVoid;
		uint16_t* mp16;
		uint32_t* mp32;
		uintptr_t mU;
	};

	PointerUnion p;
	p.mpVoid = pDest;

	EA_ASSERT((p.mU & 1) == 0);

	const uint16_t* const pEnd = (p.mp16 + count);

	if(count <= 32)                 // For small sizes, we simply do a little loop.
	{
		while(p.mp16 < pEnd)
			*p.mp16++ = c;
	}
	else
	{
		if(p.mU & 3)                // If the address is not aligned on a 32 bit boundary.
		{
			*p.mp16++ = c;          // Align it on a 32 bit boundary.
			count--;
		}

		// From here on we copy in 32 bit chunks for speed.
		count /= 2;
		const uint32_t c32 = (uint32_t)(c | (c << 16));

		while(count--)
			*p.mp32++ = c32;

		if(p.mp16 < pEnd)
			*p.mp16 = c;
	}

	return (uint16_t*)pDest;
}


EASTDC_API uint32_t* Memset32(void* pDest, uint32_t c, size_t count)
{
	EA_ASSERT(((uintptr_t)pDest & 3) == 0);

	#if (EA_PLATFORM_WORD_SIZE >= 8) || (EA_PLATFORM_PTR_SIZE >= 8) // If we are using a 64 bit system...
		const uint32_t* const pEnd = (uint32_t*)pDest+count;
		uint32_t* pDest32 = (uint32_t*)pDest;
		uint64_t  c64;

		if(count <= 16)               // For small sizes, we simply do a little loop.
		{
			while(pDest32 < pEnd)
				*pDest32++ = c;
		}
		else
		{
			if(((uintptr_t)pDest32) & 7)    // If the address is not aligned on a 64 bit boundary.
			{
				*pDest32++ = c;             // Align it on a 64 bit boundary.
				count--;
			}

			uint64_t* pDest64 = (uint64_t*)pDest32; // From here on we copy in 64 bit chunks for speed.
			count /= 2;
			c64 = ((uint64_t)c | ((uint64_t)c << 32));

			while(count)
			{
				*pDest64++ = c64;
				count--;
			}

			if((uint32_t*)pDest64 < pEnd)
				*((uint32_t*)pDest64) = (uint32_t)c64;
		}
	#else
		uint32_t*             cur = (uint32_t*)pDest;
		const uint32_t* const end = (uint32_t*)pDest + count;
		while(cur < end)
			*cur++ = c;
	#endif

	return (uint32_t*)pDest;
}


EASTDC_API uint64_t* Memset64(void* pDest, uint64_t c, size_t count)
{
	EA_ASSERT(((uintptr_t)pDest & 7) == 0);

	uint64_t*             cur = (uint64_t*)pDest;
	const uint64_t* const end = (uint64_t*)pDest + count;

	while(cur < end)
		*cur++ = c;

	return (uint64_t*)pDest;
}


EASTDC_API void* MemsetN(void* pDestination, const void* pSource, size_t sourceBytes, size_t count)
{
	// This is a generic implementation. Pathways optimized for 24 bits and/or 128 bits might be desired.

	uint8_t*       pDestination8 = (uint8_t*)pDestination;
	const uint8_t* pSource8      = (const uint8_t*)pSource;
	const uint8_t* pSource8Temp  = pSource8;

	if(((sourceBytes & 3) == 0) && (((uintptr_t)pDestination & 3) == 0) && (((uintptr_t)pSource & 3) == 0))
	{
		// Pathway for 32-bit aligned copy
		size_t i = 0;

		while(count >= 4)
		{
			 pSource8Temp = pSource8;

			 for(i = 0; (i < sourceBytes) && (count >= 4); i += 4, count -= 4)
			 {
				 *((uint32_t*)pDestination8) = *(const uint32_t*)(pSource8Temp);
				 pDestination8 += 4;
				 pSource8Temp  += 4;
			 }
		}

		if(i == sourceBytes)
			i = 0;

		pSource8Temp = pSource8 + i;
		while(count-- >= 1)
			 *pDestination8++ = *pSource8Temp++;
	}
	else // ((sourceBytes & 3) != 0)
	{
		// Pathway for non 32-bit aligned copy
		while(count >= 1)
		{
			 pSource8Temp = pSource8;

			 for(size_t i = 0; (i < sourceBytes) && (count >= 1); i++, count--)
				 *pDestination8++ = *pSource8Temp++;
		}
	}

	return pDestination;
}


EASTDC_API const void* Memcheck8(const void* p, uint8_t c, size_t byteCount)
{
	for(const uint8_t* p8 = (const uint8_t*)p; byteCount > 0; ++p8, --byteCount)
	{
		if(*p8 != c)
			return p8;
	}

	return NULL;
}


EASTDC_API const void* Memcheck16(const void* p, uint16_t c, size_t byteCount)
{
	union U16 {
		uint16_t c16;
		uint8_t  c8[2];
	};
	const U16 u = { c };
	size_t    i = (size_t)((uintptr_t)p % 2);

	for(const uint8_t* p8 = (const uint8_t*)p, *p8End = (const uint8_t*)p + byteCount; p8 != p8End; ++p8, i ^= 1)
	{
		if(*p8 != u.c8[i])
			return p8;
	}

	return NULL;
}


EASTDC_API const void* Memcheck32(const void* p, uint32_t c, size_t byteCount)
{
	union U32 {
		uint32_t c32;
		uint8_t  c8[4];
	};
	const U32 u = { c };
	size_t    i = (size_t)((uintptr_t)p % 4);

	// This code could be a little faster if it could work with an aligned 
	// destination and do word compares. There are some pitfalls to be careful
	// of which may make the effort not worth it in practice for typical uses 
	// of this code. In particular we need to make sure that word compares are 
	// done with word-aligned memory, and that may mean using a version of 
	// the c argument which has bytes rotated from their current position.
	for(const uint8_t* p8 = (const uint8_t*)p, *p8End = (const uint8_t*)p + byteCount; p8 != p8End; ++p8, i = (i + 1) % 4)
	{
		if(*p8 != u.c8[i])
			return p8;
	}

	return NULL;
}


EASTDC_API const void* Memcheck64(const void* p, uint64_t c, size_t byteCount)
{
	union U64 {
		uint64_t c64;
		uint8_t  c8[8];
	};
	const U64 u = { c };
	size_t    i = (size_t)((uintptr_t)p % 8);

	for(const uint8_t* p8 = (const uint8_t*)p, *p8End = (const uint8_t*)p + byteCount; p8 != p8End; ++p8, i = (i + 1) % 8)
	{
		if(*p8 != u.c8[i])
			return p8;
	}

	return NULL;
}




EASTDC_API const char* Memchr(const char* p, char c, size_t nCharCount)
{
	for(const char* p8 = (const char*)p; nCharCount > 0; ++p8, --nCharCount)
	{
		if(*p8 == c)
			return p8;
	}

	return NULL;
}


EASTDC_API const char16_t* Memchr16(const char16_t* pString, char16_t c, size_t nCharCount)
{
	for(; nCharCount > 0; ++pString, --nCharCount)
	{
		if(*pString == c)
			return pString;
	}

	return NULL;
}


EASTDC_API const char32_t* Memchr32(const char32_t* pString, char32_t c, size_t nCharCount)
{
	for(; nCharCount > 0; ++pString, --nCharCount)
	{
		if(*pString == c)
			return pString;
	}

	return NULL;
}


#if EASTDC_MEMCHR16_ENABLED
	EASTDC_API const char16_t* Memchr(const char16_t* pString, char16_t c, size_t nCharCount)
	{
		return Memchr16(pString, c, nCharCount);
	}
#endif


EASTDC_API int Memcmp(const void* pString1, const void* pString2, size_t nCharCount)
{
	const char* p1 = (const char*)pString1;
	const char* p2 = (const char*)pString2;

	for(; nCharCount > 0; ++p1, ++p2, --nCharCount)
	{
		if(*p1 != *p2)
			return (*p1 < *p2) ? -1 : 1;
	}

	return 0;
}


#if EASTDC_MEMCPY16_ENABLED
	EASTDC_API int Memcmp(const char16_t* pString1, const char16_t* pString2, size_t nCharCount)
	{
		for(; nCharCount > 0; ++pString1, ++pString2, --nCharCount)
		{
			if(*pString1 != *pString2)
				return (*pString1 < *pString2) ? -1 : 1;
		}

		return 0;
	}
#endif


// Search for pFind/findSize within pMemory/memorySize.
EASTDC_API void* Memmem(const void* pMemory, size_t memorySize, const void* pFind, size_t findSize)
{
	EA_ASSERT((pMemory || !memorySize) && (pFind || !findSize)); // Verify that if pMemory or pFind is NULL, their respective size must be 0.

	const uint8_t* const pMemory8 = static_cast<const uint8_t*>(pMemory);
	const uint8_t* const pFind8   = static_cast<const uint8_t*>(pFind);
	const uint8_t* const pEnd8    = (pMemory8 + memorySize) - findSize;

	if(memorySize && (findSize <= memorySize))
	{
		if(findSize) // An empty pFind results in success, return pMemory.
		{
			for(const uint8_t* pCurrent8 = pMemory8; pCurrent8 <= pEnd8; ++pCurrent8)       // This looping algorithm is not the fastest possible way to 
			{                                                                               // implement this function. A faster, but much more complex, algorithm
				if(EA_UNLIKELY(pCurrent8[0] == pFind8[0])) // Do a quick first char check.  // might involve a two-way memory search (http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260).
				{                                                                           // Another algorithm might be to start by searching for words instead of bytes, then use Memcmp.
					if(Memcmp(pCurrent8 + 1, pFind8 + 1, findSize - 1) == 0)
						return const_cast<uint8_t*>(pCurrent8);
				}
			}
		}
		else
			return const_cast<void*>(pMemory);
	}

	return NULL;
}


// This is a local function called by MemfillSpecific.
static void Memfill24(void* pD, const void* pS, size_t byteCount)
{
	unsigned char* pDestination = static_cast<unsigned char*>(pD);
	const unsigned char* pSource = static_cast<const unsigned char*>(pS);
	// Optimization wise, this function will assume that pDestination is already aligned 32-bit
	// Construct the 3 32-bit values
	unsigned int val8a = *(static_cast<const unsigned char*>(pSource));
	unsigned int val8b = *(static_cast<const unsigned char*>(pSource+1));
	unsigned int val8c = *(static_cast<const unsigned char*>(pSource+2));
	unsigned int val32a,val32b,val32c;

	#if defined(EA_SYSTEM_BIG_ENDIAN)
		val32a=(val8a*256*256*256)+(val8b*256*256)+(val8c*256)+val8a;
		val32b=(val8b*256*256*256)+(val8c*256*256)+(val8a*256)+val8b;
		val32c=(val8c*256*256*256)+(val8a*256*256)+(val8b*256)+val8c;
	#else
		val32a=val8a+(val8b*256)+(val8c*256*256)+(val8a*256*256*256);
		val32b=val8b+(val8c*256)+(val8a*256*256)+(val8b*256*256*256);
		val32c=val8c+(val8a*256)+(val8b*256*256)+(val8c*256*256*256);
	#endif

	// time to copy
	// we have to align the address to 32-bit, otherwise it's going to crash on the ps2!
	while (((reinterpret_cast<uintptr_t>(pDestination) & 0x03)!=0) && (byteCount>0))
	{
		byteCount--;
		// rotate the values over
		#if defined(EA_SYSTEM_BIG_ENDIAN)
			*(pDestination++)=static_cast<uint8_t>(val32a >> 24);
			unsigned int tmp = val32a;
			val32a=(val32a << 8) + (val32b >> 24);
			val32b=(val32b << 8) + (val32c >> 24);
			val32c=(val32c << 8) + (tmp >> 24);
		#else
			*(pDestination++)=static_cast<uint8_t>(val32a);
			unsigned int tmp = val32a;
			val32a=(val32a >> 8) + (val32b << 24);
			val32b=(val32b >> 8) + (val32c << 24);
			val32c=(val32c >> 8) + (tmp << 24);
		#endif
	}

	while (byteCount >= 12)
	{
		*(reinterpret_cast<unsigned int*>(pDestination)) = val32a;
		*(reinterpret_cast<unsigned int*>(pDestination+4)) = val32b;
		*(reinterpret_cast<unsigned int*>(pDestination+8)) = val32c;
		pDestination+=12;
		byteCount-=12;
	}
	while (byteCount >= 4)
	{
		*(reinterpret_cast<unsigned int*>(pDestination)) = val32a;
		pDestination+=4;
		byteCount-=4;
		val32a=val32b;
		val32b=val32c;
	}
	while (byteCount >= 1)
	{
		#if defined(EA_SYSTEM_BIG_ENDIAN)
			*pDestination = static_cast<uint8_t>(val32a >> 24);
			val32a = val32a << 8;
		#else
			*pDestination = static_cast<uint8_t>(val32a);
			val32a = val32a >> 8;
		#endif
		pDestination++;
		byteCount--;
	}
}

// This is a local function called by MemfillSpecific.
static void MemfillAny(void* pD, const void* pS, size_t destByteCount, size_t sourceByteCount)
{
	union Memory // Use a union to avoid memory aliasing problems in the compiler.
	{
		void*     mpVoid;
		uint8_t*  mp8;
		uint32_t* mp32;
		uint32_t  m32;
	};

	Memory d;
	d.mpVoid = pD;

	Memory s;
	s.mpVoid = const_cast<void*>(pS);

	if (((sourceByteCount & 0x03) == 0) && ((d.m32 & 0x03) == 0) && ((s.m32 & 0x03) == 0))
	{
		// Routine for 32-bit aligned copy
		size_t i = 0;
		
		while (destByteCount >= 4)
		{
			s.mpVoid = const_cast<void*>(pS);

			for (i = 0; (i < sourceByteCount) && (destByteCount >= 4); i += 4, destByteCount -= 4)
				*d.mp32++ = *s.mp32++;
		}

		if (i == sourceByteCount)
			i = 0;

		s.mpVoid = const_cast<void*>(pS);
		s.mp8   += i;

		while (destByteCount >= 1)
		{
			*d.mp8++ = *s.mp8++;
			destByteCount--;
		}
	}
	else
	{
		// Routine for non 32-bit aligned copy
		while (destByteCount)
		{
			s.mpVoid = const_cast<void*>(pS);

			for (size_t i = 0; (i < sourceByteCount) && destByteCount; i++)
			{
				*d.mp8++ = *s.mp8++;
				destByteCount--;
			}
		}
	}
}


// This is a local function called by MemfillSpecific.
static void Memfill128(void* pD, const void* pS, size_t byteCount)
{
	unsigned char* pDestination = static_cast<unsigned char*>(pD);
	const unsigned char* pSource = static_cast<const unsigned char*>(pS);
	unsigned int v1;
	unsigned int v2;
	unsigned int v3;
	unsigned int v4;

	if ((reinterpret_cast<uintptr_t>(pSource) & 0x3) != 0)
	{
		// If the source is not aligned, we need to retrieve the values on a byte by byte basis
		#if defined(EA_SYSTEM_BIG_ENDIAN)
			v1 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource)))*256*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+1)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+2)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+3))));
			v2 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+4)))*256*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+5)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+6)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+7))));
			v3 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+8)))*256*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+9)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+10)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+11))));
			v4 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+12)))*256*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+13)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+14)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+15))));
		#else
			v1 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource)))) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+1)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+2)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+3)))*256*256*256);
			v2 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+4)))) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+5)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+6)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+7)))*256*256*256);
			v3 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+8)))) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+9)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+10)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+11)))*256*256*256);
			v4 =(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+12)))) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+13)))*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+14)))*256*256) +
				(static_cast<uint32_t>(*(static_cast<const uint8_t*>(pSource+15)))*256*256*256);
		#endif
	}
	else
	{
		v1 = *(reinterpret_cast<const uint32_t*>(pSource));
		v2 = *(reinterpret_cast<const uint32_t*>(pSource+4));
		v3 = *(reinterpret_cast<const uint32_t*>(pSource+8));
		v4 = *(reinterpret_cast<const uint32_t*>(pSource+12));
	}

	// Alignment correction
	if ((reinterpret_cast<uintptr_t>(pDestination) & 0xF) != 0)
	{
		// Perform 32-bit alignment (this is required for ps2, since it crashes when it writes a 32-bit
		//   value to an unaligned 32-bit memory address)
		while (((reinterpret_cast<uintptr_t>(pDestination) & 0x03) != 0) && (byteCount>0))
		{
			byteCount--;
			// rotate the values over
			#if defined(EA_SYSTEM_BIG_ENDIAN)
				*(pDestination++)=static_cast<uint8_t>(v1 >> 24);
				unsigned int tmp = v1;
				v1=(v1 << 8) + (v2 >> 24);
				v2=(v2 << 8) + (v3 >> 24);
				v3=(v3 << 8) + (v4 >> 24);
				v4=(v4 << 8) + (tmp >> 24);
			#else
				*(pDestination++)=static_cast<uint8_t>(v1);
				unsigned int tmp = v1;
				v1=(v1 >> 8) + (v2 << 24);
				v2=(v2 >> 8) + (v3 << 24);
				v3=(v3 >> 8) + (v4 << 24);
				v4=(v4 >> 8) + (tmp << 24);
			#endif
		}

		if (byteCount >=256)
		{
			// not really worth performing all these functions if byteCount isn't large
			// Perform 128-bit alignment on 32-bit boundary
			unsigned int tempval,tempval2;
			switch (reinterpret_cast<uintptr_t>(pDestination)&0xC)
			{
			case 0xC:
				*reinterpret_cast<uint32_t*>(pDestination) = v1;
				pDestination+=4;
				byteCount-=4;
				tempval = v1;
				v1 = v2;
				v2 = v3;
				v3 = v4;
				v4 = tempval;
				break;
			case 0x8:
				*reinterpret_cast<uint32_t*>(pDestination) = v1;
				*reinterpret_cast<uint32_t*>(pDestination+4) = v2;
				pDestination+=8;
				byteCount-=8;
				tempval = v1;
				tempval2 = v3;
				v1 = tempval2;
				v3 = tempval;
				tempval = v2;
				tempval2 = v4;
				v2 = tempval2;
				v4 = tempval;
				break;
			case 0x4:
				*reinterpret_cast<uint32_t*>(pDestination) = v1;
				*reinterpret_cast<uint32_t*>(pDestination+4) = v2;
				*reinterpret_cast<uint32_t*>(pDestination+8) = v3;
				pDestination+=12;
				byteCount-=12;
				tempval = v4;
				v4 = v3;
				v3 = v2;
				v2 = v1;
				v1 = tempval;
				break;
			default:
				break;
			}
		}
	}

	// Start copying the stuff
	while (byteCount >= 16)
	{
		*(reinterpret_cast<uint32_t*>(pDestination)) = v1;
		*(reinterpret_cast<uint32_t*>(pDestination+4)) = v2;
		*(reinterpret_cast<uint32_t*>(pDestination+8)) = v3;
		*(reinterpret_cast<uint32_t*>(pDestination+12)) = v4;
		byteCount-=16;
		pDestination+=16;
	}
	if (byteCount > 0)
	{
		// end of destination not aligned to 128-bit
		unsigned int i = 0;
		while (byteCount >= 4)
		{
			*(reinterpret_cast<unsigned int*>(pDestination)) = v1;
			pDestination+=4;
			byteCount-=4;
			v1=v2;
			v2=v3;
			v3=v4;
		}
		#if defined(EA_SYSTEM_BIG_ENDIAN)
			for (i=0;(i<4) && (byteCount);i++)
			{
				*pDestination++ = static_cast<uint8_t>(v1 >> 24);
				v1 = v1 << 8;
				byteCount--;
			}
		#else
			// write the remainder for low-endian as long as the byteCount value allows it
			for (i=0;(i<4) && (byteCount!=0);i++)
			{
				*pDestination++ = static_cast<uint8_t>(v1);
				v1 = v1 >> 8;
				byteCount--;
			}
		#endif
	}
}


EASTDC_API void Memfill16(void* pDestination, uint16_t c, size_t byteCount)
{
	Memfill32(pDestination, (uint32_t)((c << 16) + c), byteCount);
}


EASTDC_API void Memfill24(void* pDestination, uint32_t c, size_t byteCount)
{
	const uint8_t c24[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
	Memfill24(pDestination, c24, byteCount);
}


#if defined(EA_PROCESSOR_X86) &&  defined(_MSC_VER)

	EASTDC_API __declspec(naked) void Memfill32(void* /*pDestination*/, uint32_t /*c*/, size_t /*byteCount*/)
	{
		__asm
		{
			mov     eax,dword ptr [esp+4]   ; pDestination
			mov     edx,dword ptr [esp+8]   ; c
			mov     ecx,dword ptr [esp+12]  ; byteCount

			sub     ecx,32
			jns     b32a
			jmp     b32b

			align   16

		; 32 byte filler
		b32a:
			sub     ecx,32
			mov     [eax],edx
			mov     [eax+4],edx
			mov     [eax+8],edx
			mov     [eax+12],edx
			mov     [eax+16],edx
			mov     [eax+20],edx
			mov     [eax+24],edx
			mov     [eax+28],edx
			lea     eax,[eax+32]
			jns     b32a

		b32b:
			add     ecx,32-8
			js      b8b

		; 8 byte filler
		b8a:
			mov     [eax],edx
			mov     [eax+4],edx
			add     eax,8
			sub     ecx,8
			jns     b8a

		b8b:
			add     ecx,8
			jne     bend
			ret

		; tail cleanup 4,2,1
		bend:
			cmp     ecx,4
			jb      be4
			mov     [eax],edx
			add     eax,4
			sub     ecx,4
		be4:
			cmp     ecx,2
			jb      be2
			mov     [eax],dx
			ror     edx,16
			add     eax,2
			sub     ecx,2
		be2:
			cmp     ecx,1
			jb      be1
			mov     [eax],dl
			inc     eax
			dec     ecx
		be1:
			ret
		}
	}

#else

	EASTDC_API void Memfill32(void* pDestination, uint32_t c, size_t byteCount)
	{
		while (((reinterpret_cast<intptr_t>(pDestination) & 3) != 0) && (byteCount > 0))
		{
			#if defined(EA_SYSTEM_BIG_ENDIAN)
				*static_cast<uint8_t*>(pDestination) = static_cast<uint8_t>(c >> 24);
				pDestination = static_cast<void*>(static_cast<char *>(pDestination) + 1);
				c = (c << 8) + (c >> 24);        // rotate the value
			#else
				*static_cast<uint8_t*>(pDestination) = static_cast<uint8_t>(c);
				pDestination = static_cast<void*>(static_cast<char *>(pDestination) + 1);
				c = (c << 24) + (c >> 8);        // rotate the value
			#endif

			--byteCount;
		}

		if ((byteCount >= 4) && ((reinterpret_cast<intptr_t>(pDestination) & 4) != 0))
		{
			*static_cast<uint32_t*>(pDestination) = static_cast<uint32_t>(c);
			pDestination = static_cast<void*>(static_cast<char*>(pDestination) + 4);
			byteCount -= 4;
		}

		if (byteCount >= 64)
		{
			uint64_t c64 = (static_cast<uint64_t>(c) << static_cast<uint64_t>(32)) | static_cast<uint64_t>(c);

			do
			{
				(static_cast<uint64_t*>(pDestination))[0] = c64;
				(static_cast<uint64_t*>(pDestination))[1] = c64;
				(static_cast<uint64_t*>(pDestination))[2] = c64;
				(static_cast<uint64_t*>(pDestination))[3] = c64;
				(static_cast<uint64_t*>(pDestination))[4] = c64;
				(static_cast<uint64_t*>(pDestination))[5] = c64;
				(static_cast<uint64_t*>(pDestination))[6] = c64;
				(static_cast<uint64_t*>(pDestination))[7] = c64;

				pDestination = static_cast<void*> (static_cast<char*>(pDestination) + 64);
				byteCount -= 64;
			}
			while (byteCount >= 64);
		}

		if (byteCount >= 16)
		{
			do
			{
				(reinterpret_cast<uint32_t*>(pDestination))[0] = c;
				(reinterpret_cast<uint32_t*>(pDestination))[1] = c;
				(reinterpret_cast<uint32_t*>(pDestination))[2] = c;
				(reinterpret_cast<uint32_t*>(pDestination))[3] = c;

				pDestination = static_cast<void*> (static_cast<char*>(pDestination) + 16);
				byteCount -= 16;
			}
			while (byteCount >= 16);
		}

		if (byteCount >= 4)
		{
			do
			{
				*static_cast<uint32_t*>(pDestination) = static_cast<uint32_t> (c);
				pDestination = static_cast<void*>(static_cast<char*>(pDestination) + 4);
				byteCount -= 4;
			}
			while (byteCount >= 4);
		}

		while (byteCount >= 1)
		{
			#if defined(EA_SYSTEM_BIG_ENDIAN)
				*static_cast<uint8_t*>(pDestination) = static_cast<uint8_t> (c >> 24);
				pDestination = static_cast<void*>(static_cast<char*>(pDestination) + 1);
				c = c << 8;
			#else
				*static_cast<uint8_t*>(pDestination) = static_cast<uint8_t> (c);
				pDestination = static_cast<void*>(static_cast<char*>(pDestination) + 1);
				c = c >> 8;
			#endif

			--byteCount;
		}
	}

#endif



EASTDC_API void Memfill64(void* pDestination, uint64_t c, size_t byteCount)
{
	MemfillAny(pDestination, &c, byteCount, sizeof(c));
}

EASTDC_API void Memfill8(void* pDestination, uint8_t c, size_t byteCount)
{
	Memset8(pDestination, c, byteCount);
}


EASTDC_API void MemfillSpecific(void* pDestination, const void* pSource, size_t destByteCount, size_t sourceByteCount)
{
	switch (sourceByteCount)
	{
		case 1:
		{
			const uint8_t c = *static_cast<const uint8_t*>(pSource);
			Memset8(pDestination, c, destByteCount);
			break;
		}

		case 2:
		{
			const uint16_t c = *static_cast<const uint16_t*>(pSource);
			Memfill16(pDestination, c, destByteCount);
			break;
		}

		case 3:
			Memfill24(pDestination, pSource, destByteCount);
			break;

		case 4:
		{
			uint32_t c = *static_cast<const uint32_t*>(pSource);
			Memfill32(pDestination, c, destByteCount);
			break;
		}

		case 8:
		default:
			MemfillAny(pDestination, pSource, destByteCount, sourceByteCount);
			break;

		case 16:
			Memfill128(pDestination, pSource, destByteCount);
			break;
	}
}


// Has similar behavior to the Unix bcmp function but executes the same instructions every 
// time and thus executes in the same amount of time for a given byteCount. Assumes that the 
// CPU executes the instructions below equivalently for all input byte combinations,
// as is usually the case for logical integer operations.
EASTDC_API bool TimingSafeMemEqual(const void* pMem1, const void* pMem2, size_t byteCount)
{
	const char* p1 = (const char*)pMem1;
	const char* p2 = (const char*)pMem2;
	char mask = 0;

	for(; byteCount > 0; ++p1, ++p2, --byteCount)
		mask |= (*p1 ^ *p2);    // Accumulate any differences between the memory.

	return (mask == 0);  // Concern: If the compiler sees the contents of pMem1 and pMem2 then it may optimize away the code above. In practice the compiler won't be able to see that in the use cases that matter to users.
}


// Has the same behavior as memcmp, but executes the same instructions every time and
// thus executes in the same amount of time for a given byteCount. Assumes that the 
// CPU executes the instructions below equivalently for all input byte combinations,
// as is usually the case for logical integer operations.
EASTDC_API int TimingSafeMemcmp(const void* pMem1, const void* pMem2, size_t byteCount)
{
	const uint8_t* p1 = static_cast<const uint8_t*>(pMem1);
	const uint8_t* p2 = static_cast<const uint8_t*>(pMem2);
	int result = 0;

	while(byteCount--) // Walk through the bytes from back to front and recalculate the difference if one is encountered.
	{
		const int c1   = p1[byteCount];
		const int c2   = p2[byteCount];
		const int mask = (((c1 ^ c2) - 1) >> 8); // The result of the following is that mask is -1 (*p1 == *p2) or 0 (*p1 != *p2).
		result &= mask;                          // If (*p1 == *p2) then mask is 0xffffffff and result is unchanged. Else result will is reset to 0 (to be updated on the next line).
		result += (c1 - c2);                     // If (*p1 == *p2) then this adds 0 and result is unchanged. Else result will be (*p1 - *p2).
	}                                            // The return value will be equal to the difference of the first unequal bytes.

	return result;
}


EASTDC_API bool TimingSafeMemIsClear(const void* pMem, size_t byteCount)
{
	uint32_t mask = 0;
	const uint8_t* p = static_cast<const uint8_t*>(pMem);

	while(byteCount--)
		mask |= *p++;

	return (mask == 0);   // Concern: If the compiler sees the contents of pMem then it may optimize away the code above. In practice the compiler won't be able to see that in the use cases that matter to users.
}





} // namespace StdC
} // namespace EA








