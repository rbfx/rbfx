///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EAMathHelp.h>
#include <EAStdC/EAStdC.h>
#include <EAStdC/EAMemory.h>
#include <EAStdC/EAAlignment.h>
#include <EAStdC/EABitTricks.h>
#include <EAAssert/eaassert.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
EA_RESTORE_ALL_VC_WARNINGS()

EA_DISABLE_VC_WARNING(4996 4127 6385 4146) // 'ecvt' was declared deprecated
									       // conditional expression is constant.
									       // Invalid data: accessing 'comp1', the readable size is '20' bytes, but '4194248' bytes might be read: Lines: 504, 505, 507, 508, 510, 512, 514
										   //  warning C4146: unary minus operator applied to unsigned type, result still unsigned
#if defined(EA_COMPILER_GNUC) && (EA_COMPILER_VERSION >= 4007)
	EA_DISABLE_GCC_WARNING(-Wmaybe-uninitialized) // GCC 4.7+ mistakenly warns about this.
#endif



/////////////////////////////////////////////////////////////////////////////
// EASTDC_MIN / EASTDC_MAX
//
#define EASTDC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define EASTDC_MAX(a, b) ((a) > (b) ? (a) : (b))


namespace EA
{
namespace StdC
{


// Define word_type to match machine word type. This is not the same 
// thing as size_t or uintptr_t, as there are machines with 64 bit
// words but 32 bit pointers (e.g. XBox 360, PS3).

#if (EA_PLATFORM_WORD_SIZE == 8) // From EABase
	typedef uint64_t word_type;
#else
	typedef uint32_t word_type;
#endif


EASTDC_API char* Strcpy(char* pDestination, const char* pSource)
{
	const char* s = pSource;
	char*       d = pDestination;

	while((*d++ = *s++) != 0)
		{} // Do nothing.
	
	return pDestination;
}

EASTDC_API char16_t* Strcpy(char16_t* pDestination, const char16_t* pSource)
{
	const char16_t* s = pSource;
	char16_t*       d = pDestination;

	while((*d++ = *s++) != 0)
		{} // Do nothing.
	
	return pDestination;
}

EASTDC_API char32_t* Strcpy(char32_t* pDestination, const char32_t* pSource)
{
	const char32_t* s = pSource;
	char32_t*       d = pDestination;

	while((*d++ = *s++) != 0)
		{} // Do nothing.
	
	return pDestination;
}




EASTDC_API char* Strncpy(char* pDestination, const char* pSource, size_t n)
{
	const char* s = pSource;
	char*       d = pDestination;

	n++;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			while(--n)
				*d++ = 0;
			break;
		}
	}
	
	return pDestination;
}

EASTDC_API char16_t* Strncpy(char16_t* pDestination, const char16_t* pSource, size_t n)
{
	const char16_t* s = pSource;
	char16_t*       d = pDestination;

	n++;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			while(--n)
				*d++ = 0;
			break;
		}
	}
	
	return pDestination;
}

EASTDC_API char32_t* Strncpy(char32_t* pDestination, const char32_t* pSource, size_t n)
{
	const char32_t* s = pSource;
	char32_t*       d = pDestination;

	n++;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			while(--n)
				*d++ = 0;
			break;
		}
	}
	
	return pDestination;
}




EASTDC_API char* StringnCopy(char* pDestination, const char* pSource, size_t n)
{
	char* pOriginalDest = pDestination;

	if(n)
	{
		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		if(n != static_cast<size_t>(-1)) // Is this portable?
			*pDestination = 0;
	}

	return pOriginalDest;
}

EASTDC_API char16_t* StringnCopy(char16_t* pDestination, const char16_t* pSource, size_t n)
{
	char16_t* pOriginalDest = pDestination;

	if(n)
	{
		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		if(n != static_cast<size_t>(-1))
			*pDestination = 0;
	}

	return pOriginalDest;
}

EASTDC_API char32_t* StringnCopy(char32_t* pDestination, const char32_t* pSource, size_t n)
{
	char32_t* pOriginalDest = pDestination;

	if(n)
	{
		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		if(n != static_cast<size_t>(-1))
			*pDestination = 0;
	}

	return pOriginalDest;
}




/* Reference implementation which ought to be a little slower than our more optimized implementation.
EASTDC_API size_t Strlcpy(char* pDestination, const char* pSource, size_t nDestCapacity)
{
	const size_t n = Strlen(pSource);

	if(n < nDestCapacity)
		memcpy(pDestination, pSource, (n + 1) * sizeof(*pSource));
	else if(nDestCapacity)
	{
		memcpy(pDestination, pSource, (nDestCapacity - 1) * sizeof(*pSource));
		pDestination[nDestCapacity - 1] = 0;
	}

	return n;
}
*/

EASTDC_API size_t Strlcpy(char* pDestination, const char* pSource, size_t nDestCapacity)
{
	const char* s = pSource;
	size_t         n = nDestCapacity;

	if(n && --n)
	{
		do{
			if((*pDestination++ = *s++) == 0)
				break;
		} while(--n);
	}

	if(!n)
	{
		if(nDestCapacity)
			*pDestination = 0;
		while(*s++)
			{ }
	}

	return (size_t)(s - pSource - 1);
}

/* Reference implementation which ought to be a little slower than our more optimized implementation.
EASTDC_API size_t Strlcpy(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity)
{
	const size_t n = Strlen(pSource);

	if(n < nDestCapacity)
		memcpy(pDestination, pSource, (n + 1) * sizeof(*pSource));
	else if(nDestCapacity)
	{
		memcpy(pDestination, pSource, (nDestCapacity - 1) * sizeof(*pSource));
		pDestination[nDestCapacity - 1] = 0;
	}

	return n;
}
*/

EASTDC_API size_t Strlcpy(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity)
{
	const char16_t* s = pSource;
	size_t          n = nDestCapacity;

	if(n && --n)
	{
		do{
			if((*pDestination++ = *s++) == 0)
				break;
		} while(--n);
	}

	if(!n)
	{
		if(nDestCapacity)
			*pDestination = 0;
		while(*s++)
			{ }
	}

	return (size_t)(s - pSource - 1);
}

EASTDC_API size_t Strlcpy(char32_t* pDestination, const char32_t* pSource, size_t nDestCapacity)
{
	const char32_t* s = pSource;
	size_t          n = nDestCapacity;

	if(n && --n)
	{
		do{
			if((*pDestination++ = *s++) == 0)
				break;
		} while(--n);
	}

	if(!n)
	{
		if(nDestCapacity)
			*pDestination = 0;
		while(*s++)
			{ }
	}

	return (size_t)(s - pSource - 1);
}



/*
// To consider: Enable this for completeness with the above:

EASTDC_API int Strlcpy(char* pDest, const char* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	if(nSourceLength == kSizeTypeUnset)
		nSourceLength = Strlen(pSource);

	if(nSourceLength < nDestCapacity)
	{
		memcpy(pDest, pSource, nSourceLength * sizeof(*pSource));
		pDest[nSourceLength] = 0;
	}
	else if(nDestCapacity)
	{
		memcpy(pDest, pSource, (nDestCapacity - 1) * sizeof(*pSource));
		pDest[nDestCapacity - 1] = 0;
	}

	return (int)(unsigned)nSourceLength;
}

EASTDC_API int Strlcpy(char16_t* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	if(nSourceLength == kSizeTypeUnset)
		nSourceLength = Strlen(pSource);

	if(nSourceLength < nDestCapacity)
	{
		memcpy(pDest, pSource, nSourceLength * sizeof(*pSource));
		pDest[nSourceLength] = 0;
	}
	else if(nDestCapacity)
	{
		memcpy(pDest, pSource, (nDestCapacity - 1) * sizeof(*pSource));
		pDest[nDestCapacity - 1] = 0;
	}

	return (int)(unsigned)nSourceLength;
}

EASTDC_API int Strlcpy(char32_t* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	if(nSourceLength == kSizeTypeUnset)
		nSourceLength = Strlen(pSource);

	if(nSourceLength < nDestCapacity)
	{
		memcpy(pDest, pSource, nSourceLength * sizeof(*pSource));
		pDest[nSourceLength] = 0;
	}
	else if(nDestCapacity)
	{
		memcpy(pDest, pSource, (nDestCapacity - 1) * sizeof(*pSource));
		pDest[nDestCapacity - 1] = 0;
	}

	return (int)(unsigned)nSourceLength;
}
*/



//static const int32_t kLeadingSurrogateStart  = 0x0000d800;
//static const int32_t kTrailingSurrogateStart = 0x0000dc00;
//static const int32_t kLeadingSurrogateEnd    = 0x0000dbff;
//static const int32_t kTrailingSurrogateEnd   = 0x0000dfff;
//static const int32_t kSurrogateOffset        = 0x00010000 - (kLeadingSurrogateStart << 10) - kTrailingSurrogateStart;

// This is not static because it is used elsewhere.
uint8_t utf8lengthTable[256] = 
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // For full UTF8 support, this last row should be: 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
};

// Used to subtract out the control bits in multi-byte sequence.
static const uint32_t utf8DecodingOffsetTable[5] = 
{
	0,                                                  // 0x00000000
	0,                                                  // 0x00000000
	(0xC0 << 6) + 0x80,                                 // 0x00003080
	(0xE0 << 12) + (0x80 << 6) + 0x80,                  // 0x000e2080
	(0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80    // 0x03c82080
  // to do
  // to do
};

// The minimum value that can be encoded in a particular number
// of bytes. Used to disallow non-canonical encoded sequences.
// It turns out that this is not a fully proper way to check for 
// valid sequences. See the Unicode Standard http://unicode.org/versions/corrigendum1.html
static const uint32_t utf8MinimumValueTable[] = 
{
	0x00000000,     // This slot is unused
	0x00000000,     // 1 byte per char
	0x00000080,     // 2 bytes per char
	0x00000800,     // 3 bytes per char
	0x00010000      // 4 bytes per char
  //0x00200000,     // 5 bytes per char
  //0x04000000,     // 6 bytes per char
};

// One past the maximum value that can be encoded in a particular number
// of bytes. Used to disallow non-canonical encoded sequences.
static const uint32_t utf8MaximumValueTable[] = 
{
	0x00000000,     // This slot is unused
	0x00000080,     // 1 byte per char
	0x00000800,     // 2 bytes per char
	0x00010000,     // 3 bytes per char
	0x00110000      // 4 bytes per char      *** Should be: 0x00200000
  //0x04000000
  //0x80000000 	
};

const uint32_t kUnicodeReplacementChar	= 0x0000fffd;
const uint32_t kUnicodeInvalidDecode = 0xffffffffu;

EA_FORCE_INLINE uint32_t DecodeCodePoint(const char*& pSourceStart, const char* pSourceEnd)
{
	const char* pSource = pSourceStart;
	uint32_t c = (uint8_t)*(pSource++);

	if(c < 128)
	{
		// Valid character, do nothing
	}
	else
	{
		uint32_t nLength = utf8lengthTable[c]; // nLength may be zero, in which case we'll return 'IncorrectEncoding'.

											   // Do we have an incomplete or invalid string?
		if((pSourceStart + nLength > pSourceEnd) || (nLength == 0))
		{
			if(EA::StdC::GetAssertionsEnabled())
			{
				EA_FAIL_MSG("Incomplete Unicode character in buffer");
			}
			return kUnicodeInvalidDecode;
		}

		// Now decode the remaining ("following") bytes.
		for(uint32_t i = 0; i < nLength - 1; ++i)
		{
			uint8_t nByte = (uint8_t)*(pSource++);

			if((nByte < 0x80u) || (nByte > 0xbfu))   // Syntax check
			{
				if(EA::StdC::GetAssertionsEnabled())
				{
					EA_FAIL_MSG("Invalid following byte");
				}
				return kUnicodeInvalidDecode;
			}

			c = (c << 6) + nByte;   // Preserve control bits (don't OR)
		}

		c -= utf8DecodingOffsetTable[nLength];    // Subtract accumulated control bits just once

												  // Check for canonical encoding.
		if((c < utf8MinimumValueTable[nLength]) || (c >= utf8MaximumValueTable[nLength]))
		{
			return kUnicodeInvalidDecode;
		}
	}

	pSourceStart = pSource;
	return c;
}

EA_FORCE_INLINE bool EncodeCodePoint(uint32_t c, char*& pDestStart, char* pDestEnd)
{
	// Encode as UTF-8
	if(c < 0x00000080u)
	{
		*(pDestStart++) = static_cast<char>(c);
		return true;
	}
	else if(c < 0x00000800u)
	{
		if (pDestStart + 2 <= pDestEnd)
		{
			char* pDest = pDestStart;
			*(pDest++) = static_cast<char>((c >> 6) | 0xc0);
			*(pDest++) = static_cast<char>((c | 0x80) & 0xbf);
			pDestStart = pDest;
			return true;
		}
		return false;
	}
	else if(c < 0x00010000u)
	{
		if (pDestStart + 3 <= pDestEnd)
		{
			char* pDest = pDestStart;
			*(pDest++) = static_cast<char>((c >> 12) | 0xe0);
			*(pDest++) = static_cast<char>(((c >> 6) | 0x80) & 0xbf);
			*(pDest++) = static_cast<char>((c | 0x80) & 0xbf);
			pDestStart = pDest;
			return true;
		}
		return false;
	}
	else if(c < 0x00200000u)
	{
		if(pDestStart + 4 <= pDestEnd)
		{
			char* pDest = pDestStart;
			*(pDest++) = static_cast<char>((c >> 18) | 0xf0);
			*(pDest++) = static_cast<char>(((c >> 12) | 0x80) & 0xbf);
			*(pDest++) = static_cast<char>(((c >> 6) | 0x80) & 0xbf);
			*(pDest++) = static_cast<char>((c | 0x80) & 0xbf);
			pDestStart = pDest;
			return true;
		}
		return false;
	}
	else
	{
		// We don't currently support Unicode beyond "Plane 0", as game software has never needed it.
		// If you have a need for this support, feel free to submit a patch or make a specific request.
		c = kUnicodeReplacementChar;
		if(pDestStart + 3 <= pDestEnd)
		{
			char* pDest = pDestStart;
			*(pDest++) = static_cast<char>((c >> 12) | 0xe0);
			*(pDest++) = static_cast<char>(((c >> 6) | 0x80) & 0xbf);
			*(pDest++) = static_cast<char>(((c >> 0) | 0x80) & 0xbf);
			pDestStart = pDest;
			return true;
		}
		return false;
	}
}

EA_FORCE_INLINE uint32_t DecodeCodePoint(const char16_t*& pSourceStart, const char16_t* pSourceEnd)
{
	return (uint32_t)*(pSourceStart++);
}

EA_FORCE_INLINE bool EncodeCodePoint(uint32_t c, char16_t*& pDestStart, char16_t* pDestEnd)
{
	*(pDestStart++) = static_cast<char16_t>(c);
	return true;
}

EA_FORCE_INLINE uint32_t DecodeCodePoint(const char32_t*& pSourceStart, const char32_t* pSourceEnd)
{
	return (uint32_t)*(pSourceStart++);
}

EA_FORCE_INLINE bool EncodeCodePoint(uint32_t c, char32_t*& pDestStart, char32_t* pDestEnd)
{
	*(pDestStart++) = static_cast<char32_t>(c);
	return true;
}

template <typename InCharT, typename OutCharT>
bool StrlcpyInternal(OutCharT* pDest, const InCharT* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	EA_ASSERT(pDest != nullptr && pSource != nullptr);

	// If we have no capacity, then just return nothing
	if(nDestCapacity == 0)
	{
		nDestUsed = 0;
		nSourceUsed = 0;
		return true;
	}

	const InCharT* pSourceStart = pSource;
	const InCharT* pSourceEnd = pSource + nSourceLength;
	if (pSourceEnd < pSourceStart)
		pSourceEnd = (const InCharT*)(uintptr_t)-1;
	OutCharT* pDestStart = pDest;
	OutCharT* pDestEnd = pDest + nDestCapacity - 1;
	bool bGood = true;

	while(bGood && (pSource < pSourceEnd) && (pDest < pDestEnd))
	{
		uint32_t c = DecodeCodePoint(pSource, pSourceEnd);
		if (c == 0)
		{
			pSource = pSourceEnd;
			break;
		}
		bGood = (c != kUnicodeInvalidDecode) && EncodeCodePoint(c, pDest, pDestEnd);
	}

	*pDest = 0;

	nDestUsed = pDest - pDestStart;
	nSourceUsed = pSource - pSourceStart;
	return bGood;
}

bool Strlcpy(char* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

bool Strlcpy(char* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

bool Strlcpy(char16_t* pDest, const char* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

bool Strlcpy(char16_t* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

bool Strlcpy(char32_t* pDest, const char* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

bool Strlcpy(char32_t* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
{
	return StrlcpyInternal(pDest, pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
}

EASTDC_API int Strlcpy(char* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint16_t)*pSource++;   // Deal with surrogate characters

		// Encode as UTF-8
		if (c < 0x00000080u)
		{
			if(c == 0) // Break on NULL char, even if explicit length was set
				break;

			if(pDest && ((destCount + 1) < nDestCapacity))
				*pDest++ = static_cast<char>(c);

			destCount += 1;
		}
		else if(c < 0x00000800u)
		{
			if(pDest && ((destCount + 2) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 6) | 0xc0);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 2;
		}
		else if(c < 0x00010000u)
		{
			if(pDest && ((destCount + 3) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 12) | 0xe0);
				*pDest++ = static_cast<char>(((c >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 3;
		}
		else if(c < 0x00200000u)
		{
			if(pDest && ((destCount + 4) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 18) | 0xf0);
				*pDest++ = static_cast<char>(((c >> 12) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>(((c >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 4;
		}
		else
		{
			const uint32_t kIllegalUnicodeChar = 0x0000fffd;

			if(pDest && ((destCount + 3) < nDestCapacity))
			{
				*pDest++ = static_cast<char>( (kIllegalUnicodeChar >> 12) | 0xe0);
				*pDest++ = static_cast<char>(((kIllegalUnicodeChar >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>(((kIllegalUnicodeChar >>  0) | 0x80) & 0xbf);
			}

			destCount += 3;
		}
	}

	if(pDest && nDestCapacity != 0)
		*pDest = 0;

	return (int)(unsigned)destCount;
}

EASTDC_API int Strlcpy(char* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint32_t)*pSource++;   // Deal with surrogate characters

		// Encode as UTF-8
		if (c < 0x00000080u)
		{
			if(c == 0) // Break on NULL char, even if explicit length was set
				break;

			if(pDest && ((destCount + 1) < nDestCapacity))
				*pDest++ = static_cast<char>(c);

			destCount += 1;
		}
		else if(c < 0x00000800u)
		{
			if(pDest && ((destCount + 2) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 6) | 0xc0);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 2;
		}
		else if(c < 0x00010000u)
		{
			if(pDest && ((destCount + 3) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 12) | 0xe0);
				*pDest++ = static_cast<char>(((c >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 3;
		}
		else if(c < 0x00200000u)
		{
			if(pDest && ((destCount + 4) < nDestCapacity))
			{
				*pDest++ = static_cast<char>((c >> 18) | 0xf0);
				*pDest++ = static_cast<char>(((c >> 12) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>(((c >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>((c | 0x80) & 0xbf);
			}

			destCount += 4;
		}
		else
		{
			// We don't currently support Unicode beyond "Plane 0", as game software has never needed it.
			// If you have a need for this support, feel free to submit a patch or make a specific request.
			const uint32_t kIllegalUnicodeChar = 0x0000fffd;

			if(pDest && ((destCount + 3) < nDestCapacity))
			{
				*pDest++ = static_cast<char>( (kIllegalUnicodeChar >> 12) | 0xe0);
				*pDest++ = static_cast<char>(((kIllegalUnicodeChar >>  6) | 0x80) & 0xbf);
				*pDest++ = static_cast<char>(((kIllegalUnicodeChar >>  0) | 0x80) & 0xbf);
			}

			destCount += 3;
		}
	}

	if(pDest && nDestCapacity != 0)
		*pDest = 0;

	return (int)(unsigned)destCount;
}



EASTDC_API int Strlcpy(char16_t* pDest, const char* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint8_t)*pSource++;

		if(c < 128)
		{
			if(c == 0) // Break on NULL char, even if explicit length was set
				break;

			if(pDest && ((destCount + 1) < nDestCapacity)) // +1 because we want to append c to pDest but also append '\0'.
				*pDest++ = static_cast<char16_t>(c);

			destCount++;
		}
		else
		{
			uint32_t nLength = utf8lengthTable[c]; // nLength may be zero, in which case we'll return 'IncorrectEncoding'.

			// Do we have an incomplete or invalid string?
			if((nLength > (nSourceLength + 1)) || (nLength == 0))
			{
				if(EA::StdC::GetAssertionsEnabled())
					{ EA_FAIL_MSG("Incomplete Unicode character in buffer"); }
				if(pDest && (destCount < nDestCapacity))
					*pDest++ = 0; // Even though we are returning an error, 0-terminate anyway for safety.
				return -1;
			}

			// Now decode the remaining ("following") bytes.
			for(uint32_t i = 0; i < nLength - 1; ++i) 
			{
				uint8_t nByte = (uint8_t)*pSource++;

				if((nByte < 0x80u) || (nByte > 0xbfu))   // Syntax check
				{
					if(EA::StdC::GetAssertionsEnabled())
						{ EA_FAIL_MSG("Invalid following byte"); }
					if(pDest && (destCount < nDestCapacity))
						*pDest++ = 0; // Even though we are returning an error, 0-terminate anyway for safety.
					return -1;
				}

				c = (c << 6) + nByte;   // Preserve control bits (don't OR)
			}

			nSourceLength -= (nLength - 1);           // We've just processed all remaining bytes for this multi-byte character
			c -= utf8DecodingOffsetTable[nLength];    // Subtract accumulated control bits just once

			// Check for canonical encoding.
			if((c >= utf8MinimumValueTable[nLength]) && (c < utf8MaximumValueTable[nLength]))
			{
				if(pDest && ((destCount + 1) < nDestCapacity))
					*pDest++ = static_cast<char16_t>(c);

				destCount++;
			}
			else
				break;
		}
	}

	if(pDest && (nDestCapacity != 0))
		*pDest = 0;

	return (int)(unsigned)destCount;
}

EASTDC_API int Strlcpy(char32_t* pDest, const char* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint8_t)*pSource++;

		if(c < 128)
		{
			if(c == 0) // Break on NULL char, even if explicit length was set
				break;

			if(pDest && ((destCount + 1) < nDestCapacity)) // +1 because we want to append c to pDest but also append '\0'.
				*pDest++ = static_cast<char32_t>(c);

			destCount++;
		}
		else
		{
			uint32_t nLength = utf8lengthTable[c]; // nLength may be zero, in which case we'll return 'IncorrectEncoding'.

			// Do we have an incomplete or invalid string?
			if((nLength > (nSourceLength + 1)) || (nLength == 0))
			{
				if(EA::StdC::GetAssertionsEnabled())
					{ EA_FAIL_MSG("Incomplete Unicode character in buffer"); }
				if(pDest && (destCount < nDestCapacity))
					*pDest++ = 0; // Even though we are returning an error, 0-terminate anyway for safety.
				return -1;
			}

			// Now decode the remaining ("following") bytes.
			for(uint32_t i = 0; i < nLength - 1; ++i) 
			{
				uint8_t nByte = (uint8_t)*pSource++;

				if((nByte < 0x80u) || (nByte > 0xbfu))   // Syntax check
				{
					if(EA::StdC::GetAssertionsEnabled())
						{ EA_FAIL_MSG("Invalid following byte"); }
					if(pDest && (destCount < nDestCapacity))
						*pDest++ = 0; // Even though we are returning an error, 0-terminate anyway for safety.
					return -1;
				}

				c = (c << 6) + nByte;   // Preserve control bits (don't OR)
			}

			nSourceLength -= (nLength - 1);           // We've just processed all remaining bytes for this multi-byte character
			c -= utf8DecodingOffsetTable[nLength];    // Subtract accumulated control bits just once

			// Check for canonical encoding.
			if((c >= utf8MinimumValueTable[nLength]) && (c < utf8MaximumValueTable[nLength]))
			{
				if(pDest && ((destCount + 1) < nDestCapacity))
					*pDest++ = static_cast<char32_t>(c);

				destCount++;
			}
			else
				break;
		}
	}

	if(pDest && (nDestCapacity != 0))
		*pDest = 0;

	return (int)(unsigned)destCount;
}


EASTDC_API int Strlcpy(char32_t* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint32_t)*pSource++;

		if(c == 0) // Break on NULL char, even if explicit length was set
			break;

		if(pDest && ((destCount + 1) < nDestCapacity)) // +1 because we want to append c to pDest but also append '\0'.
			*pDest++ = static_cast<char32_t>(c);

		destCount += 1;
	}

	if(pDest && nDestCapacity != 0)
		*pDest = 0;

	return (int)(unsigned)destCount;
}


EASTDC_API int Strlcpy(char16_t* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength)
{
	size_t destCount = 0;

	while(nSourceLength-- > 0)
	{
		uint32_t c = (uint32_t)*pSource++;

		if(c == 0) // Break on NULL char, even if explicit length was set
			break;

		if(pDest && ((destCount + 1) < nDestCapacity)) // +1 because we want to append c to pDest but also append '\0'.
			*pDest++ = static_cast<char16_t>(c);

		destCount += 1;
	}

	if(pDest && nDestCapacity != 0)
		*pDest = 0;

	return (int)(unsigned)destCount;
}



EASTDC_API char* Strcat(char* pDestination, const char* pSource)
{
	const char* s = pSource;
	char*       d = pDestination;

	while(*d++){}          // Do nothing.
		--d;
	while((*d++ = *s++) != 0)
		{} // Do nothing.
	
	return pDestination;
}

EASTDC_API char16_t* Strcat(char16_t* pDestination, const char16_t* pSource)
{
	const char16_t* s = pSource;
	char16_t*       d = pDestination;

	while(*d++){}          // Do nothing.
		--d;
	while((*d++ = *s++) != 0)
		{} // Do nothing.
	
	return pDestination;
}

EASTDC_API char32_t* Strcat(char32_t* pDestination, const char32_t* pSource)
{
	const char32_t* s = pSource;
	char32_t*       d = pDestination;

	while(*d++){}          // Do nothing.
		--d;
	while((*d++ = *s++) != 0)
		{} // Do nothing.

	return pDestination;
}




EASTDC_API char* Strncat(char* pDestination, const char* pSource, size_t n)
{
	const char* s = pSource;
	char*       d = pDestination;
	
	while(*d++){} // Do nothing.
	--d;
	++n;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			--d;
			break;
		}
	}
	*d = 0;
	
	return pDestination;
}

EASTDC_API char16_t* Strncat(char16_t* pDestination, const char16_t* pSource, size_t n)
{
	const char16_t* s = pSource;
	char16_t*       d = pDestination;
	
	while(*d++){} // Do nothing.
	--d;
	++n;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			--d;
			break;
		}
	}
	*d = 0;
	
	return pDestination;
}

EASTDC_API char32_t* Strncat(char32_t* pDestination, const char32_t* pSource, size_t n)
{
	const char32_t* s = pSource;
	char32_t*       d = pDestination;
	
	while(*d++){} // Do nothing.
	--d;
	++n;
	while(--n)
	{
		if((*d++ = *s++) == 0)
		{
			--d;
			break;
		}
	}
	*d = 0;
	
	return pDestination;
}



EASTDC_API char* StringnCat(char* pDestination, const char* pSource, size_t n)
{
	char* const pOriginalDest = pDestination;

	if(n)
	{
		while(*pDestination)
			++pDestination;

		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		*pDestination = 0;
	}

	return pOriginalDest;
}

EASTDC_API char16_t* StringnCat(char16_t* pDestination, const char16_t* pSource, size_t n)
{
	char16_t* const pOriginalDest = pDestination;

	if(n)
	{
		while(*pDestination)
			++pDestination;

		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		*pDestination = 0;
	}

	return pOriginalDest;
}

EASTDC_API char32_t* StringnCat(char32_t* pDestination, const char32_t* pSource, size_t n)
{
	char32_t* const pOriginalDest = pDestination;

	if(n)
	{
		while(*pDestination)
			++pDestination;

		while(n-- && *pSource)
			*pDestination++ = *pSource++;

		*pDestination = 0;
	}

	return pOriginalDest;
}




EASTDC_API size_t Strlcat(char* pDestination, const char* pSource, size_t nDestCapacity)
{
	const size_t d = nDestCapacity ? Strlen(pDestination) : 0;
	const size_t s = Strlen(pSource);
	const size_t t = s + d;

	EA_ASSERT_MSG((nDestCapacity == 0) || (d < nDestCapacity), "Destination string is longer than the specified capacity! "
		"Either an out of bounds write has occurred previous to this call or the specified capacity is incorrect.");

	if(t < nDestCapacity)
		memcpy(pDestination + d, pSource, (s + 1) * sizeof(*pSource));
	else
	{
		if(nDestCapacity)
		{
			memcpy(pDestination + d, pSource, ((nDestCapacity - d) - 1) * sizeof(*pSource));
			pDestination[nDestCapacity - 1] = 0;
		}
	}

	return t;
}

EASTDC_API size_t Strlcat(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity)
{
	const size_t d = nDestCapacity ? Strlen(pDestination) : 0;
	const size_t s = Strlen(pSource);
	const size_t t = s + d;

	EA_ASSERT_MSG((nDestCapacity == 0) || (d < nDestCapacity), "Destination string is longer than the specified capacity! "
		"Either an out of bounds write has occurred previous to this call or the specified capacity is incorrect.");

	if(t < nDestCapacity)
		memcpy(pDestination + d, pSource, (s + 1) * sizeof(*pSource));
	else
	{
		if(nDestCapacity)
		{
			memcpy(pDestination + d, pSource, ((nDestCapacity - d) - 1) * sizeof(*pSource));
			pDestination[nDestCapacity - 1] = 0;
		}
	}

	return t;
}

EASTDC_API size_t Strlcat(char32_t* pDestination, const char32_t* pSource, size_t nDestCapacity)
{
	const size_t d = nDestCapacity ? Strlen(pDestination) : 0;
	const size_t s = Strlen(pSource);
	const size_t t = s + d;

	EA_ASSERT_MSG((nDestCapacity == 0) || (d < nDestCapacity), "Destination string is longer than the specified capacity! "
		"Either an out of bounds write has occurred previous to this call or the specified capacity is incorrect.");

	if(t < nDestCapacity)
		memcpy(pDestination + d, pSource, (s + 1) * sizeof(*pSource));
	else
	{
		if(nDestCapacity)
		{
			memcpy(pDestination + d, pSource, ((nDestCapacity - d) - 1) * sizeof(*pSource));
			pDestination[nDestCapacity - 1] = 0;
		}
	}

	return t;
}




EASTDC_API size_t Strlcat(char16_t* pDestination, const char* pSource, size_t nDestCapacity)
{
	size_t sourceLen = StrlenUTF8Decoded(pSource);
	size_t destLen   = Strlen(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}

EASTDC_API size_t Strlcat(char32_t* pDestination, const char* pSource, size_t nDestCapacity)
{
	size_t sourceLen = StrlenUTF8Decoded(pSource);
	size_t destLen   = Strlen(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}




EASTDC_API size_t Strlcat(char* pDestination, const char16_t* pSource, size_t nDestCapacity)
{
	size_t sourceLen = Strlen(pSource);
	size_t destLen   = StrlenUTF8Decoded(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}

EASTDC_API size_t Strlcat(char* pDestination, const char32_t* pSource, size_t nDestCapacity)
{
	size_t sourceLen = Strlen(pSource);
	size_t destLen   = StrlenUTF8Decoded(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}

EASTDC_API size_t Strlcat(char16_t* pDestination, const char32_t* pSource, size_t nDestCapacity)
{
	size_t sourceLen = Strlen(pSource);
	size_t destLen   = Strlen(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}

EASTDC_API size_t Strlcat(char32_t* pDestination, const char16_t* pSource, size_t nDestCapacity)
{
	size_t sourceLen = Strlen(pSource);
	size_t destLen   = Strlen(pDestination);

	if(nDestCapacity > destLen)
		Strlcpy(pDestination + destLen, pSource, nDestCapacity - destLen);
	
	return sourceLen + destLen;
}



// Optimized Strlen
//
// This function assumes that we can read the last size_t-sized word at
// the end of the string, even if as many as three of the word bytes are
// beyond the end of the string.  This is typically a valid assumption
// because valid memory is always aligned to big power-of-2 sizes.
//
// Tests of this Strlen show that it outperforms the basic strlen 
// implementation by 2x-6x on lengths ranging from 128 bytes to 4096 bytes.
// At lengths under 10 bytes this strlen performs similarly to strlen.
// These observations apply to x86, x64 and PowerPC32 platforms.
//
// There could be faster strlen implementations with some additional
// tricks, asm, SSE, etc. But this version works well while being simple.

#if EASTDC_STATIC_ANALYSIS_ENABLED
	#define EASTDC_ENABLE_OPTIMIZED_STRLEN 0 // Disabled because the optimized strlen reads words and the string may have some uninitialized chars at the end past the trailing 0 char. Valgrind reports this as an error, but it's not actually an error in practice.
#else
	#define EASTDC_ENABLE_OPTIMIZED_STRLEN 1
#endif

#if EASTDC_ENABLE_OPTIMIZED_STRLEN

	EASTDC_API size_t Strlen(const char* pString)
	{
		#if EA_COMPILER_HAS_BUILTIN(__builtin_strlen)
			return __builtin_strlen(pString);
		#else
			// Instead of casting between types, we just create a union.
			union PointerUnion
			{
				const char*   mp8;
				const word_type* mpW;
				uintptr_t        mU;
			} pu;

			// Leading unaligned bytes
			for(pu.mp8 = pString; pu.mU & (sizeof(word_type) - 1); pu.mp8++)
			{
				if(*pu.mp8 == 0)
					return (size_t)(pu.mp8 - pString);
			}

			for(; ; pu.mpW++)
			{
				#if defined(__GNUC__) && (__GNUC__ >= 3) && !defined(__EDG_VERSION__)
					__builtin_prefetch(pu.mpW + 64, 0, 0);
				#endif

				// Quit if there are any zero chars.
				const word_type kOneBytes  = ((word_type)-1 / 0xff); // 0x01010101
				const word_type kHighBytes = (kOneBytes * 0x80);     // 0x80808080

				const word_type u = *pu.mpW;

				if((u - kOneBytes) & ~u & kHighBytes)
					break;
			}

			// Trailing unaligned bytes
			while(*pu.mp8)
				++pu.mp8;

			return (size_t)(pu.mp8 - pString);
		#endif
	}

#else

	EASTDC_API size_t Strlen(const char* pString)
	{
		ssize_t nLength = (size_t)-1; // EABase 1.0.14 and later recognize ssize_t for all platforms.

		do
		{
			++nLength;
		} while (*pString++);

		return (size_t)nLength;
	}

#endif


#if EASTDC_ENABLE_OPTIMIZED_STRLEN

	EASTDC_API size_t Strlen(const char16_t* pString)
	{
		// Instead of casting between types, we just create a union.
		union PointerUnion
		{
			const char16_t*  mp16;
			const word_type* mpW;
			uintptr_t        mU;
		} pu;

		// Leading unaligned bytes
		for(pu.mp16 = pString; pu.mU & (sizeof(word_type) - 1); pu.mp16++)
		{
			if(*pu.mp16 == 0)
				return (size_t)(pu.mp16 - pString);
		}

		for(; ; pu.mpW++)
		{
			#if defined(__GNUC__) && (__GNUC__ >= 3) && !defined(__EDG_VERSION__)
				__builtin_prefetch(pu.mpW + 64, 0, 0);
			#endif

			// Quit if there are any zero char16_ts.
			const word_type kOneBytes  = ((word_type)-1 / 0xffff); // 0x00010001
			const word_type kHighBytes = (kOneBytes * 0x8000);     // 0x80008000

			const word_type u = *pu.mpW;

			if((u - kOneBytes) & ~u & kHighBytes)
				break;
		}

		// Trailing unaligned bytes
		while(*pu.mp16)
			++pu.mp16;

		return (size_t)(pu.mp16 - pString);
	}

#else

	EASTDC_API size_t Strlen(const char16_t* pString)
	{
		size_t nLength = (size_t)-1;

		do
		{
			++nLength;
		} while (*pString++);

		return nLength;
	}

#endif

// To consider: This might benefit from an optimized implementation on machines withi 64 bit registers.
EASTDC_API size_t Strlen(const char32_t* pString)
{
	size_t nLength = (size_t)-1;
	
	do{
		++nLength;
	}while(*pString++);
	
	return nLength;
}





// Returns number of Unicode characters are in the UTF8-encoded string.
// Return value will be <= Strlen(pString).
EASTDC_API size_t StrlenUTF8Decoded(const char* pString)
{
	size_t nLength = 0;

	while(*pString)
	{
		if((*pString & 0xc0) != 0x80)
			++nLength;
		++pString;
	}

	return nLength;
}


// Returns number of characters that would be in a UTF8-encoded string.
// Return value will be >= Strlen(pString).
EASTDC_API size_t StrlenUTF8Encoded(const char16_t* pString)
{
	size_t   nLength = 0;
	uint32_t c;

	while((c = *pString++) != 0)
	{
		if(c < 0x00000080)
			nLength += 1;
		else if(c < 0x00000800)
			nLength += 2;
		else // if(c < 0x00010000) 
			nLength += 3;

		// The following would be used if the input string was 32 bit instead of 16 bit.
		//else if(c < 0x00200000)
		//    destCount += 4;
		//else                    // Else we use the error char 0xfffd
		//    destCount += 3;
	}

	return nLength;
}

// Returns number of characters that would be in a UTF8-encoded string.
// Return value will be >= Strlen(pString).
EASTDC_API size_t StrlenUTF8Encoded(const char32_t* pString)
{
	size_t   nLength = 0;
	uint32_t c;

	while((c = *pString++) != 0)
	{
		if(c < 0x00000080)
			nLength += 1;
		else if(c < 0x00000800)
			nLength += 2;
		else // if(c < 0x00010000) 
			nLength += 3;

		// The following would be used if the input string was 32 bit instead of 32 bit.
		//else if(c < 0x00200000)
		//    destCount += 4;
		//else                    // Else we use the error char 0xfffd
		//    destCount += 3;
	}

	return nLength;
}



EASTDC_API char* Strend(const char* pString)
{
	while (*pString)
		++pString;

	return (char*)pString;
}

EASTDC_API char16_t* Strend(const char16_t* pString)
{
	while (*pString)
		++pString;

	return (char16_t*)pString;
}

EASTDC_API char32_t* Strend(const char32_t* pString)
{
	while(*pString)
		++pString;

	return (char32_t*)pString;
}



EASTDC_API size_t Strxfrm(char* pDest, const char* pSource, size_t n)
{
	const size_t nLength = Strlen(pSource);

	if(n > 0)
	{
		Strncpy(pDest, pSource, n - 1);
		if(n < nLength)
			pDest[n - 1] = 0;
	}

	return nLength;
}

EASTDC_API size_t Strxfrm(char16_t* pDest, const char16_t* pSource, size_t n)
{
	const size_t nLength = Strlen(pSource);

	if(n > 0)
	{
		Strncpy(pDest, pSource, n - 1);
		if(n < nLength)
			pDest[n - 1] = 0;
	}

	return nLength;
}

EASTDC_API size_t Strxfrm(char32_t* pDest, const char32_t* pSource, size_t n)
{
	const size_t nLength = Strlen(pSource);

	if(n > 0)
	{
		Strncpy(pDest, pSource, n - 1);
		if(n < nLength)
			pDest[n - 1] = 0;
	}

	return nLength;
}



EASTDC_API char* Strdup(const char* pString)
{
	if(pString)
	{
		const size_t nLength = Strlen(pString);
		char* const p = EASTDC_NEW(EASTDC_ALLOC_PREFIX "Strdup") char[nLength + 1]; // '+ 1' to include terminating zero.

		Strcpy(p, pString);
		return p;
	}

	return NULL;
}

EASTDC_API char16_t* Strdup(const char16_t* pString)
{
	if(pString)
	{
		const size_t nLength = Strlen(pString);
		char16_t* const p = EASTDC_NEW(EASTDC_ALLOC_PREFIX "Strdup") char16_t[nLength + 1]; // '+ 1' to include terminating zero.

		Strcpy(p, pString);
		return p;
	}

	return NULL;
}

EASTDC_API char32_t* Strdup(const char32_t* pString)
{
	if(pString)
	{
		const size_t nLength = Strlen(pString);
		char32_t* const p = EASTDC_NEW(EASTDC_ALLOC_PREFIX "Strdup") char32_t[nLength + 1]; // '+ 1' to include terminating zero.

		Strcpy(p, pString);
		return p;
	}

	return NULL;
}



EASTDC_API void Strdel(char* pString)
{
	EASTDC_DELETE[] pString;
}

EASTDC_API void Strdel(char16_t* pString)
{
	EASTDC_DELETE[] pString;
}

EASTDC_API void Strdel(char32_t* pString)
{
	EASTDC_DELETE[] pString;
}




EASTDC_API char* Strupr(char* pString)
{
	// This implementation converts only 7 bit ASCII characters.
	// As such it is safe to use with 7-bit-safe multibyte encodings
	// such as UTF8 but may yield incorrect results with such text.
	char* pStringTemp = pString;

	while(*pStringTemp)
	{
		if((uint8_t)*pStringTemp <= 127)
			*pStringTemp = (char)Toupper(*pStringTemp);
		++pStringTemp;
	}

	return pString;
}

EASTDC_API char16_t* Strupr(char16_t* pString)
{
	char16_t* pStringTemp = pString;

	while(*pStringTemp)
	{
		const char16_t c = *pStringTemp;
		*pStringTemp++ = Toupper(c);
	}

	return pString;
}

EASTDC_API char32_t* Strupr(char32_t* pString)
{
	char32_t* pStringTemp = pString;

	while(*pStringTemp)
	{
		const char32_t c = *pStringTemp;
		*pStringTemp++ = Toupper(c);
	}

	return pString;
}




EASTDC_API char* Strlwr(char* pString)
{
	// This implementation converts only 7 bit ASCII characters.
	// As such it is safe to use with 7-bit-safe multibyte encodings
	// such as UTF8 but may yield incorrect results with such text.
	char* pStringTemp = pString;

	while(*pStringTemp)
	{
		if((uint8_t)*pStringTemp <= 127)
			*pStringTemp = (char)Tolower(*pStringTemp);
		++pStringTemp;
	}

	return pString;
}

EASTDC_API char16_t* Strlwr(char16_t* pString)
{
	char16_t* pStringTemp = pString;

	while(*pStringTemp)
	{
		const char16_t c = *pStringTemp;
		*pStringTemp++ = Tolower(c);
	}

	return pString;
}

EASTDC_API char32_t* Strlwr(char32_t* pString)
{
	char32_t* pStringTemp = pString;

	while(*pStringTemp)
	{
		const char32_t c = *pStringTemp;
		*pStringTemp++ = Tolower(c);
	}

	return pString;
}




EASTDC_API char* Strmix(char* pDestination, const char* pSource, const char* pDelimiters)
{
	bool bCapitalize = true;
	char* const pOriginalDest = pDestination;

	while(*pSource)
	{
		char c = *pSource++;

		// This character is upper-cased if bCapitalize flag is true, else lower-cased
		if(bCapitalize)
		{
			if(Islower(c))
			{
				c = Toupper(c);
				bCapitalize = false;
			}
			else if(Isupper(c))
				bCapitalize = false;
		}
		else
		{
			if(Isupper(c))
				c = Tolower(c);
		}

		// Check whether this character is a separator character. If so, set the bCapitalize flag.
		for(const char* pCheck = pDelimiters; *pCheck; ++pCheck)
		{
			if(c == *pCheck)
				bCapitalize = true;
		}

		*pDestination++ = c;
	}

	*pDestination = 0;
	return pOriginalDest;
}

EASTDC_API char16_t* Strmix(char16_t* pDestination, const char16_t* pSource, const char16_t* pDelimiters)
{
	bool bCapitalize = true;
	char16_t* const pOriginalDest = pDestination;

	while(*pSource)
	{
		char16_t c = *pSource++;

		// This character is upper-cased if bCapitalize flag is true, else lower-cased
		if(bCapitalize)
		{
			if(Islower(c))
			{
				c = Toupper(c);
				bCapitalize = false;
			}
			else if(Isupper(c))
				bCapitalize = false;
		}
		else
		{
			if(Isupper(c))
				c = Tolower(c);
		}

		// Check whether this character is a separator character. If so, set the bCapitalize flag.
		for(const char16_t* pCheck = pDelimiters; *pCheck; ++pCheck)
		{
			if(c == *pCheck)
				bCapitalize = true;
		}

		*pDestination++ = c;
	}

	*pDestination = 0;
	return pOriginalDest;
}

EASTDC_API char32_t* Strmix(char32_t* pDestination, const char32_t* pSource, const char32_t* pDelimiters)
{
	bool bCapitalize = true;
	char32_t* const pOriginalDest = pDestination;

	while(*pSource)
	{
		char32_t c = *pSource++;

		// This character is upper-cased if bCapitalize flag is true, else lower-cased
		if(bCapitalize)
		{
			if(Islower(c))
			{
				c = Toupper(c);
				bCapitalize = false;
			}
			else if(Isupper(c))
				bCapitalize = false;
		}
		else
		{
			if(Isupper(c))
				c = Tolower(c);
		}

		// Check whether this character is a separator character. If so, set the bCapitalize flag.
		for(const char32_t* pCheck = pDelimiters; *pCheck; ++pCheck)
		{
			if(c == *pCheck)
				bCapitalize = true;
		}

		*pDestination++ = c;
	}

	*pDestination = 0;
	return pOriginalDest;
}




EASTDC_API char* Strchr(const char* pString, int c)
{
	do
	{
		if (*pString == c)
			return (char*)pString;
	} while (*pString++);

	return NULL;
}

EASTDC_API char16_t* Strchr(const char16_t* pString, char16_t c)
{
	do
	{
		if (*pString == c)
			return (char16_t*)pString;
	} while (*pString++);

	return NULL;
}

EASTDC_API char32_t* Strchr(const char32_t* pString, char32_t c)
{
	do {
		if(*pString == c)
			return (char32_t*)pString;
	} while (*pString++);

	return NULL;
}




EASTDC_API char* Strnchr(const char* pString, int c, size_t n)
{
	while(n-- > 0)
	{
		if(*pString == c)
		{
			return (char*)pString;
		}
		if(*pString == '\0')
		{
			return NULL;
		}
		pString++;
	}
	return NULL;
}

EASTDC_API char16_t* Strnchr(const char16_t* pString, char16_t c, size_t n)
{
	while(n-- > 0)
	{
		if(*pString == c)
		{
			return (char16_t*)pString;
		}
		if(*pString == '\0')
		{
			return NULL;
		}
		pString++;
	}
	return NULL;
}

EASTDC_API char32_t* Strnchr(const char32_t* pString, char32_t c, size_t n)
{
	while(n-- > 0)
	{
		if(*pString == c)
		{
			return (char32_t*)pString;
		}
		if(*pString == '\0')
		{
			return NULL;
		}
		pString++;
	}
	return NULL;
}




EASTDC_API size_t Strcspn(const char* pString1, const char* pString2)
{
	const char* pStringCurrent = pString1;

	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2.
	while(*pStringCurrent)
	{
		for(const char* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pStringCurrent)
				return (size_t)(pStringCurrent - pString1);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString1);
}

EASTDC_API size_t Strcspn(const char16_t* pString1, const char16_t* pString2)
{
	const char16_t* pStringCurrent = pString1;

	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char16_t
	// means that the map would have to be 65536 bits (8192 bytes) in size.
	while(*pStringCurrent)
	{
		for(const char16_t* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pStringCurrent)
				return (size_t)(pStringCurrent - pString1);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString1);
}

EASTDC_API size_t Strcspn(const char32_t* pString1, const char32_t* pString2)
{
	const char32_t* pStringCurrent = pString1;

	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char32_t
	// means that the map would have to be huge in size.
	while(*pStringCurrent)
	{
		for(const char32_t* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pStringCurrent)
				return (size_t)(pStringCurrent - pString1);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString1);
}




EASTDC_API char* Strpbrk(const char* pString1, const char* pString2)
{
	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2.
	while(*pString1)
	{
		for(const char* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pString1)
				return (char*)pString1;
		}

		++pString1;
	}

	return NULL;
}

EASTDC_API char16_t* Strpbrk(const char16_t* pString1, const char16_t* pString2)
{
	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char16_t
	// means that the map would have to be 65536 bits (8192 bytes) in size.
	while(*pString1)
	{
		for(const char16_t* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pString1)
				return (char16_t*)pString1;
		}

		++pString1;
	}

	return NULL;
}

EASTDC_API char32_t* Strpbrk(const char32_t* pString1, const char32_t* pString2)
{
	// This implementation does a double loop. As such, it can get slow for
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char32_t
	// means that the map would have to be huge in size.
	while(*pString1)
	{
		for(const char32_t* pCharSet = pString2; *pCharSet; ++pCharSet)
		{
			if(*pCharSet == *pString1)
				return (char32_t*)pString1;
		}

		++pString1;
	}

	return NULL;
}


EASTDC_API char* Strrchr(const char* pString, int c)
{
	const char* pFound = NULL;
	char cCurrent;

	while ((cCurrent = *pString++) != 0)
	{
		if (cCurrent == c)
			pFound = (pString - 1);
	}

	if (pFound)
		return (char*)pFound;

	return c ? NULL : (char*)(pString - 1);
}

EASTDC_API char16_t* Strrchr(const char16_t* pString, char16_t c)
{
	const char16_t* pFound = NULL;
	char16_t cCurrent;

	while ((cCurrent = *pString++) != 0)
	{
		if (cCurrent == c)
			pFound = (pString - 1);
	}

	if (pFound)
		return (char16_t*)pFound;

	return c ? NULL : (char16_t*)(pString - 1);
}

EASTDC_API char32_t* Strrchr(const char32_t* pString, char32_t c)
{
	const char32_t* pFound = NULL;
	char32_t        cCurrent;

	while ((cCurrent = *pString++) != 0)
	{
		if (cCurrent == c)
			pFound = (pString - 1);
	}

	if (pFound)
		return (char32_t*)pFound;

	return c ? NULL : (char32_t*)(pString - 1);
}


EASTDC_API size_t Strspn(const char* pString, const char* pSubString)
{
	// This implementation does a double loop. As such, it can get slow for 
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. 
	const char* pStringCurrent = pString;

	while(*pStringCurrent)
	{
		for(const char* pSubStringCurrent = pSubString; *pSubStringCurrent != *pStringCurrent; ++pSubStringCurrent)
		{
			if(*pSubStringCurrent == 0)
				return (size_t)(pStringCurrent - pString);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString);
}

EASTDC_API size_t Strspn(const char16_t* pString, const char16_t* pSubString)
{
	// This implementation does a double loop. As such, it can get slow for 
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char16_t 
	// means that the map would have to be 65536 bits (8192 bytes) in size.
	const char16_t* pStringCurrent = pString;

	while(*pStringCurrent)
	{
		for(const char16_t* pSubStringCurrent = pSubString; *pSubStringCurrent != *pStringCurrent; ++pSubStringCurrent)
		{
			if(*pSubStringCurrent == 0)
				return (size_t)(pStringCurrent - pString);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString);
}

EASTDC_API size_t Strspn(const char32_t* pString, const char32_t* pSubString)
{
	// This implementation does a double loop. As such, it can get slow for 
	// very long strings. An alternative implementation is to use a hash
	// table or to create a bytemap of the chars in pString2. But char32_t 
	// means that the map would have to be huge in size.
	const char32_t* pStringCurrent = pString;

	while(*pStringCurrent)
	{
		for(const char32_t* pSubStringCurrent = pSubString; *pSubStringCurrent != *pStringCurrent; ++pSubStringCurrent)
		{
			if(*pSubStringCurrent == 0)
				return (size_t)(pStringCurrent - pString);
		}

		++pStringCurrent;
	}

	return (size_t)(pStringCurrent - pString);
}



EASTDC_API char* Strstr(const char* pString, const char* pSubString)
{
	char* s1 = (char*)pString - 1;
	char* p1 = (char*)pSubString - 1;
	char  c0, c1, c2;

	if((c0 = *++p1) == 0)   // An empty pSubString results in success, return pString.
		return (char*)pString;

	while((c1 = *++s1) != 0)
	{
		if(c1 == c0)
		{
			const char* s2 = (s1 - 1);
			const char* p2 = (p1 - 1);
			
			while((c1 = *++s2) == (c2 = *++p2) && c1){} // Do nothing

			if(!c2)
				return (char*)s1;
		}
	}
	return NULL;
}

EASTDC_API char16_t* Strstr(const char16_t* pString, const char16_t* pSubString)
{
	char16_t* s1 = (char16_t*)pString - 1;
	char16_t* p1 = (char16_t*)pSubString - 1;
	char16_t  c0, c1, c2;

	if((c0 = *++p1) == 0)   // An empty pSubString results in success, return pString.
		return (char16_t*)pString;

	while((c1 = *++s1) != 0)
	{
		if(c1 == c0)
		{
			const char16_t* s2 = (s1 - 1);
			const char16_t* p2 = (p1 - 1);
			
			while((c1 = *++s2) == (c2 = *++p2) && c1){} // Do nothing

			if(!c2)
				return (char16_t*)s1;
		}
	}
	return NULL;
}

EASTDC_API char32_t* Strstr(const char32_t* pString, const char32_t* pSubString)
{
	char32_t* s1 = (char32_t*)pString - 1;
	char32_t* p1 = (char32_t*)pSubString - 1;
	char32_t  c0, c1, c2;

	if((c0 = *++p1) == 0)   // An empty pSubString results in success, return pString.
		return (char32_t*)pString;

	while((c1 = *++s1) != 0)
	{
		if(c1 == c0)
		{
			const char32_t* s2 = (s1 - 1);
			const char32_t* p2 = (p1 - 1);
			
			while((c1 = *++s2) == (c2 = *++p2) && c1){} // Do nothing

			if(!c2)
				return (char32_t*)s1;
		}
	}
	return NULL;
}



EASTDC_API char* Stristr(const char* s1, const char* s2)
{
	const char* cp = s1;

	if(!*s2)
		return (char*)s1;

	while(*cp)
	{
		const char* s = cp;
		const char* t = s2;

		while(*s && *t && (Tolower(*s) == Tolower(*t)))
			++s, ++t;

		if(*t == 0)
			return (char*)cp;
		++cp;
	}

	return 0;
}

EASTDC_API char16_t* Stristr(const char16_t* s1, const char16_t* s2)
{
	const char16_t* cp = s1;

	if(!*s2)
		return (char16_t*)s1;

	while(*cp)
	{
		const char16_t* s = cp;
		const char16_t* t = s2;

		while(*s && *t && (Tolower(*s) == Tolower(*t)))
			++s, ++t;

		if(*t == 0)
			return (char16_t*)cp;
		++cp;
	}

	return 0;
}

EASTDC_API char32_t* Stristr(const char32_t* s1, const char32_t* s2)
{
	const char32_t* cp = s1;

	if(!*s2)
		return (char32_t*)s1;

	while(*cp)
	{
		const char32_t* s = cp;
		const char32_t* t = s2;

		while(*s && *t && (Tolower(*s) == Tolower(*t)))
			++s, ++t;

		if(*t == 0)
			return (char32_t*)cp;
		++cp;
	}

	return 0;
}




EASTDC_API char* Strrstr(const char* s1, const char* s2) 
{
	if(!*s2)
		return (char*)s1;

	const char* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char* psc1 = --ps1;
		const char* sc2  = s2;

		for(;;)
		{
			if(*psc1++ != *sc2++)
				break;
			else if(!*sc2)
				return (char*)ps1;
		}
	}

	return 0;
}

EASTDC_API char16_t* Strrstr(const char16_t* s1, const char16_t* s2) 
{
	if(!*s2)
		return (char16_t*)s1;

	const char16_t* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char16_t* psc1 = --ps1;
		const char16_t* sc2  = s2;

		for(;;)
		{
			if(*psc1++ != *sc2++)
				break;
			else if(!*sc2)
				return (char16_t*)ps1;
		}
	}

	return 0;
}

EASTDC_API char32_t* Strrstr(const char32_t* s1, const char32_t* s2) 
{
	if(!*s2)
		return (char32_t*)s1;

	const char32_t* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char32_t* psc1 = --ps1;
		const char32_t* sc2  = s2;

		for(;;)
		{
			if(*psc1++ != *sc2++)
				break;
			else if(!*sc2)
				return (char32_t*)ps1;
		}
	}

	return 0;
}




EASTDC_API char* Strirstr(const char* s1, const char* s2)
{
	if(!*s2)
		return (char*)s1;

	const char* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char* psc1 = --ps1;
		const char* sc2  = s2;

		for(;;)
		{
			if(Tolower(*psc1++) != Tolower(*sc2++))
				break;
			else if(!*sc2)
				return (char*)ps1;
		}
	}

	return 0;
}

EASTDC_API char16_t* Strirstr(const char16_t* s1, const char16_t* s2)
{
	if(!*s2)
		return (char16_t*)s1;

	const char16_t* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char16_t* psc1 = --ps1;
		const char16_t* sc2  = s2;

		for(;;)
		{
			if(Tolower(*psc1++) != Tolower(*sc2++))
				break;
			else if(!*sc2)
				return (char16_t*)ps1;
		}
	}

	return 0;
}

EASTDC_API char32_t* Strirstr(const char32_t* s1, const char32_t* s2)
{
	if(!*s2)
		return (char32_t*)s1;

	const char32_t* ps1 = s1 + Strlen(s1);

	while(ps1 != s1) 
	{
		const char32_t* psc1 = --ps1;
		const char32_t* sc2  = s2;

		for(;;)
		{
			if(Tolower(*psc1++) != Tolower(*sc2++))
				break;
			else if(!*sc2)
				return (char32_t*)ps1;
		}
	}

	return 0;
}



EASTDC_API bool Strstart(const char* pString, const char* pPrefix)
{
	while(*pPrefix)
	{
		if(*pString++ != *pPrefix++)
			return false;
	}

	return true;
}


EASTDC_API bool Strstart(const char16_t* pString, const char16_t* pPrefix)
{
	while(*pPrefix)
	{
		if(*pString++ != *pPrefix++)
			return false;
	}

	return true;
}


EASTDC_API bool Strstart(const char32_t* pString, const char32_t* pPrefix)
{
	while(*pPrefix)
	{
		if(*pString++ != *pPrefix++)
			return false;
	}

	return true;
}



EASTDC_API bool Stristart(const char* pString, const char* pPrefix)
{
	while(*pPrefix)
	{
		if(Tolower(*pString++) != Tolower(*pPrefix++))
			return false;
	}

	return true;
}


EASTDC_API bool Stristart(const char16_t* pString, const char16_t* pPrefix)
{
	while(*pPrefix)
	{
		if(Tolower(*pString++) != Tolower(*pPrefix++))
			return false;
	}

	return true;
}


EASTDC_API bool Stristart(const char32_t* pString, const char32_t* pPrefix)
{
	while(*pPrefix)
	{
		if(Tolower(*pString++) != Tolower(*pPrefix++))
			return false;
	}

	return true;
}




EASTDC_API bool Strend(const char* pString, const char* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Memcmp(pString + stringLength - suffixLength, pSuffix, suffixLength * sizeof(char)) == 0;

	return false;
}


EASTDC_API bool Strend(const char16_t* pString, const char16_t* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Memcmp(pString + stringLength - suffixLength, pSuffix, suffixLength * sizeof(char16_t)) == 0;

	return false;
}


EASTDC_API bool Strend(const char32_t* pString, const char32_t* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Memcmp(pString + stringLength - suffixLength, pSuffix, suffixLength * sizeof(char32_t)) == 0;

	return false;
}


EASTDC_API bool Striend(const char* pString, const char* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Stricmp(pString + stringLength - suffixLength, pSuffix) == 0;

	return false;
}


EASTDC_API bool Striend(const char16_t* pString, const char16_t* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Stricmp(pString + stringLength - suffixLength, pSuffix) == 0;

	return false;
}


EASTDC_API bool Striend(const char32_t* pString, const char32_t* pSuffix, size_t stringLength, size_t suffixLength)
{
	if(stringLength == kSizeTypeUnset)
		stringLength = Strlen(pString);

	if(suffixLength == kSizeTypeUnset)
		suffixLength = Strlen(pSuffix);

	if(stringLength >= suffixLength)
		return Stricmp(pString + stringLength - suffixLength, pSuffix) == 0;

	return false;
}


///////////////////////////////////////////////////////////////////
// This function was implemented by Avery Lee.
//
EASTDC_API char* Strtok(char* pString, const char* pDelimiters, char** pContext)
{
	// find point on string to resume
	char* s = pString;

	if(!s)
	{
		s = *pContext;
		if(!s)
			return NULL;
	}

	// Compute bit hash based on lower 5 bits of delimiter characters
	const char* d = pDelimiters;
	int32_t        hash = 0;
	uint32_t       delimiterCount = 0;

	while(const char c = *d++)
	{
		hash |= (int32_t)(0x80000000 >> (c & 31));
		++delimiterCount;
	}

	// Skip delimiters
	for(;;)
	{
		const char c = *s;

		// If we hit the end of the string, it ends solely with delimiters
		// and there are no more tokens to get.
		if(!c)
		{
			*pContext = NULL;
			return NULL;
		}

		// Fast rejection against hash set
		if(int32_t(uint64_t(hash) << (c & 31)) >= 0)
			break;

		// brute-force search against delimiter list
		for(uint32_t i=0; i<delimiterCount; ++i)
		{
			if (pDelimiters[i] == c)    // Is it a delimiter? ...
				goto still_delimiters;  // yes, continue the loop
		}

		// Not a token, so exit
		break;

	still_delimiters:
		++s;
	}

	// Mark beginning of token
	char* const pToken = s;

	// Search for end of token
	while(const char c = *s)
	{
		// Fast rejection against hash set
		if(int32_t(int64_t(hash) << (c & 31)) < 0)
		{
			// Brute-force search against delimiter list
			for(uint32_t i=0; i<delimiterCount; ++i)
			{
				if(pDelimiters[i] == c)
				{
					// This token ends with a delimiter.
					*s = 0;                 // null-term substring
					*pContext = (s + 1);    // restart on next byte
					return pToken;          // return found token
				}
			}
		}

		++s;
	}

	// We found a token but it was at the end of the string, 
	// so we null out the context and return the last token.
	*pContext = NULL;           // no more tokens
	return pToken;              // return found token
}

EASTDC_API char16_t* Strtok(char16_t* pString, const char16_t* pDelimiters, char16_t** pContext)
{
	// Find point on string to resume
	char16_t* s = pString;

	if(!s)
	{
		s = *pContext;
		if(!s)
			return NULL;
	}

	// compute bit hash based on lower 5 bits of delimiter characters
	const char16_t* d = pDelimiters;
	int32_t         hash = 0;
	uint32_t        delimiterCount = 0;

	while(const char16_t c = *d++)
	{
		hash |= (int32_t)(0x80000000 >> (c & 31));
		++delimiterCount;
	}

	// Skip delimiters
	for(;;)
	{
		const char16_t c = *s;

		// If we hit the end of the string, it ends solely with delimiters
		// and there are no more tokens to get.
		if(!c)
		{
			*pContext = NULL;
			return NULL;
		}

		// Fast rejection against hash set
		if(int32_t(int64_t(hash) << (c & 31)) >= 0)
			break;

		// Brute-force search against delimiter list
		for(uint32_t i=0; i<delimiterCount; ++i)
		{
			if(pDelimiters[i] == (char16_t)c)    // Is it a delimiter? ...
				goto still_delimiters;           // yes, continue the loop
		}

		// Not a token, so exit
		break;

	still_delimiters:
		++s;
	}

	// Mark beginning of token
	char16_t* const pToken = s;

	// Search for end of token
	while(const char16_t c = *s)
	{
		// Fast rejection against hash set
		if(int32_t(int64_t(hash) << (c & 31)) < 0)
		{
			// Brute-force search against delimiter list
			for(uint32_t i=0; i<delimiterCount; ++i)
			{
				if(pDelimiters[i] == c)
				{
					// This token ends with a delimiter.
					*s = 0;                 // null-term substring
					*pContext = (s + 1);    // restart on next byte
					return pToken;          // return found token
				}
			}
		}

		++s;
	}

	// We found a token but it was at the end of the string, 
	// so we null out the context and return the last token.
	*pContext = NULL;           // no more tokens
	return pToken;              // return found token
}

EASTDC_API char32_t* Strtok(char32_t* pString, const char32_t* pDelimiters, char32_t** pContext)
{
	// Find point on string to resume
	char32_t* s = pString;

	if(!s)
	{
		s = *pContext;
		if(!s)
			return NULL;
	}

	// compute bit hash based on lower 5 bits of delimiter characters
	const char32_t* d = pDelimiters;
	int32_t         hash = 0;
	uint32_t        delimiterCount = 0;

	while(const uint32_t c = (uint32_t)*d++)
	{
		hash |= (int32_t)(0x80000000 >> (c & 31));
		++delimiterCount;
	}

	// Skip delimiters
	for(;;)
	{
		const char32_t c = *s;

		// If we hit the end of the string, it ends solely with delimiters
		// and there are no more tokens to get.
		if(!c)
		{
			*pContext = NULL;
			return NULL;
		}

		// Fast rejection against hash set
		if(int32_t(int64_t(hash) << (c & 31)) >= 0)
			break;

		// Brute-force search against delimiter list
		for(uint32_t i=0; i<delimiterCount; ++i)
		{
			if(pDelimiters[i] == c)    // Is it a delimiter? ...
				goto still_delimiters;  // yes, continue the loop
		}

		// Not a token, so exit
		break;

	still_delimiters:
		++s;
	}

	// Mark beginning of token
	char32_t* const pToken = s;

	// Search for end of token
	while(const uint32_t c = (uint32_t)*s)
	{
		// Fast rejection against hash set
		if(int32_t(int64_t(hash) << (c & 31)) < 0)
		{
			// Brute-force search against delimiter list
			for(uint32_t i=0; i<delimiterCount; ++i)
			{
				if(pDelimiters[i] == (char32_t)c)
				{
					// This token ends with a delimiter.
					*s = 0;                 // null-term substring
					*pContext = (s + 1);    // restart on next byte
					return pToken;          // return found token
				}
			}
		}

		++s;
	}

	// We found a token but it was at the end of the string, 
	// so we null out the context and return the last token.
	*pContext = NULL;           // no more tokens
	return pToken;              // return found token
}



EASTDC_API const char* Strtok2(const char* pString, const char* pDelimiters, 
								  size_t* pResultLength, bool bFirst)
{
	// Skip any non-delimiters
	if(!bFirst)
	{
		while(*pString && !Strchr(pDelimiters, *pString))
			++pString;
	}

	// Skip any delimiters
	while(*pString && Strchr(pDelimiters, *pString))
		++pString;

	const char* const pBegin = pString;

	// Calculate the length of the string
	while(*pString && !Strchr(pDelimiters, *pString))
		++pString;

	if(pBegin != pString)
	{
		*pResultLength = static_cast<size_t>(pString - pBegin);
		return pBegin;
	}

	*pResultLength = 0;
	return NULL;
}

EASTDC_API const char16_t* Strtok2(const char16_t* pString, const char16_t* pDelimiters, size_t* pResultLength, bool bFirst)
{
	// Skip any non-delimiters
	if(!bFirst)
	{
		while(*pString && !Strchr(pDelimiters, *pString))
			++pString;
	}

	// Skip any delimiters
	while(*pString && Strchr(pDelimiters, *pString))
		++pString;

	const char16_t* const pBegin = pString;

	// Calculate the length of the string
	while(*pString && !Strchr(pDelimiters, *pString))
		++pString;

	if(pBegin != pString)
	{
		*pResultLength = static_cast<size_t>(pString - pBegin);
		return pBegin;
	}

	*pResultLength = 0;
	return NULL;
}

EASTDC_API const char32_t* Strtok2(const char32_t* pString, const char32_t* pDelimiters, size_t* pResultLength, bool bFirst)
{
	// Skip any non-delimiters
	if(!bFirst)
	{
		while(*pString && !Strchr(pDelimiters, *pString))
			++pString;
	}

	// Skip any delimiters
	while(*pString && Strchr(pDelimiters, *pString))
		++pString;

	const char32_t* const pBegin = pString;

	// Calculate the length of the string
	while(*pString && !Strchr(pDelimiters, *pString))
		++pString;

	if(pBegin != pString)
	{
		*pResultLength = static_cast<size_t>(pString - pBegin);
		return pBegin;
	}

	*pResultLength = 0;
	return NULL;
}



EASTDC_API char* Strset(char* pString, int c)
{
	char* pStringTemp = pString;

	while(*pStringTemp)
		*pStringTemp++ = (char)c;

	return pString;
}

EASTDC_API char16_t* Strset(char16_t* pString, char16_t c)
{
	char16_t* pStringTemp = pString;

	while(*pStringTemp)
		*pStringTemp++ = c;

	return pString;
}

EASTDC_API char32_t* Strset(char32_t* pString, char32_t c)
{
	char32_t* pStringTemp = pString;

	while(*pStringTemp)
		*pStringTemp++ = c;

	return pString;
}



EASTDC_API char* Strnset(char* pString, int c, size_t n)
{
	char* pSaved = pString;

	for(size_t i = 0; *pString && (i < n); ++i)
		*pString++ = (char)c;

	return pSaved;
}

EASTDC_API char16_t* Strnset(char16_t* pString, char16_t c, size_t n)
{
	char16_t* pSaved = pString;

	for(size_t i = 0; *pString && (i < n); ++i)
		*pString++ = c;

	return pSaved;
}

EASTDC_API char32_t* Strnset(char32_t* pString, char32_t c, size_t n)
{
	char32_t* pSaved = pString;

	for(size_t i = 0; *pString && (i < n); ++i)
		*pString++ = c;

	return pSaved;
}



EASTDC_API char* Strrev(char* pString)
{
	for(char* p1 = pString, *p2 = (pString + Strlen(pString)) - 1; p1 < p2; ++p1, --p2)
	{
		char c = *p2;
		*p2 = *p1;
		*p1 = c;
	}

	return pString;
}

EASTDC_API char16_t* Strrev(char16_t* pString)
{
	for(char16_t* p1 = pString, *p2 = (pString + Strlen(pString)) - 1; p1 < p2; ++p1, --p2)
	{
		char16_t c = *p2;
		*p2 = *p1;
		*p1 = c;
	}

	return pString;
}

EASTDC_API char32_t* Strrev(char32_t* pString)
{
	for(char32_t* p1 = pString, *p2 = (pString + Strlen(pString)) - 1; p1 < p2; ++p1, --p2)
	{
		char32_t c = *p2;
		*p2 = *p1;
		*p1 = c;
	}

	return pString;
}


EASTDC_API char* Strstrip(char* pString)
{
	// Walk forward from the beginning and find the first non-whitespace.
	while(EA::StdC::Isspace(*pString)) // Isspace returns false for *pString == '\0'.
		++pString;

	if(*pString)
	{
		// Walk backward from the end and find the last whitespace.
		size_t   length = EA::StdC::Strlen(pString);
		char* pEnd   = (pString + length) - 1;

		while((pEnd > pString) && EA::StdC::Isspace(*pEnd))
			pEnd--;

		pEnd[1] = '\0';
	}

	return pString;
}


EASTDC_API char16_t* Strstrip(char16_t* pString)
{
	// Walk forward from the beginning and find the first non-whitespace.
	while(EA::StdC::Isspace(*pString)) // Isspace returns false for *pString == '\0'.
		++pString;

	if(*pString)
	{
		// Walk backward from the end and find the last whitespace.
		size_t    length = EA::StdC::Strlen(pString);
		char16_t* pEnd   = (pString + length) - 1;

		while((pEnd > pString) && EA::StdC::Isspace(*pEnd))
			pEnd--;

		pEnd[1] = '\0';
	}

	return pString;
}


EASTDC_API char32_t* Strstrip(char32_t* pString)
{
	// Walk forward from the beginning and find the first non-whitespace.
	while(EA::StdC::Isspace(*pString)) // Isspace returns false for *pString == '\0'.
		++pString;

	if(*pString)
	{
		// Walk backward from the end and find the last whitespace.
		size_t    length = EA::StdC::Strlen(pString);
		char32_t* pEnd   = (pString + length) - 1;

		while((pEnd > pString) && EA::StdC::Isspace(*pEnd))
			pEnd--;

		pEnd[1] = '\0';
	}

	return pString;
}



// Optimized Strcmp
//
// This function assumes that we can read the last size_t-sized word at
// the end of the string, even if as many as three of the word bytes are
// beyond the end of the string.  This is typically a valid assumption
// because valid memory is always aligned to big power-of-2 sizes.
//
// There could be faster strcmp implementations with some additional
// tricks, asm, SSE, etc. But this version works well while being simple.
// To do: Implement a vector-specific version for at least x64-based platforms.

#if EASTDC_STATIC_ANALYSIS_ENABLED
	#define EASTDC_ENABLE_OPTIMIZED_STRCMP 0 // Disabled because the optimized strcmp reads words and the string may have some uninitialized chars at the 
#else                                        // end past the trailing 0 char. Valgrind reports this as an error, but it's not actually an error in practice.
	#define EASTDC_ENABLE_OPTIMIZED_STRCMP 1
#endif

#if EASTDC_ENABLE_OPTIMIZED_STRCMP
	#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_OSX)
		// Some platforms have an optimized vector implementation of strcmp which is fast and which provides
		// identical return value behavior to our Strcmp (which is to return the byte difference and not just
		// -1, 0, +1). And so until we have our own vectored version we use the built-in version.
		EASTDC_API int Strcmp(const char* pString1, const char* pString2)
		{
			return strcmp(pString1, pString2);
		}
	#else
		// To do: Implement an x86/x64 vectorized version of Strcmp, which can work on 16 byte chunks and thus be faster than below.

		EASTDC_API int Strcmp(const char* pString1, const char* pString2)
		{
			if(IsAligned<const char, sizeof(word_type)>(pString1) && // If pString1 and pString2 are word-aligned... compare in word-sized chunks.
			   IsAligned<const char, sizeof(word_type)>(pString2))
			{
				const word_type* pWord1 = (word_type*)pString1;
				const word_type* pWord2 = (word_type*)pString2;

				while(*pWord1 == *pWord2)
				{
					if(ZeroPresent8(*pWord1++))
						return 0;
					++pWord2;
				}

				// At this point, the strings differ somewhere in the bytes pointed to by pWord1/pWord2.
				pString1 = reinterpret_cast<const char*>(pWord1); // Fall through and do byte comparisons for the rest of the string.
				pString2 = reinterpret_cast<const char*>(pWord2);
			}

			while(*pString1 && (*pString1 == *pString2))
			{
				++pString1;
				++pString2;
			}

			return ((uint8_t)*pString1 - (uint8_t)*pString2);
		}
	#endif
#else
	EASTDC_API int Strcmp(const char* pString1, const char* pString2)
	{
		char c1, c2;

		while((c1 = *pString1++) == (c2 = *pString2++))
		{
			if(c1 == 0)
				return 0;
		}

		return ((uint8_t)c1 - (uint8_t)c2);
	}
#endif


#if EASTDC_ENABLE_OPTIMIZED_STRCMP
	// To do: Implement an x86/x64 vectorized version of Strcmp, which can work on 16 byte chunks and thus be faster than below.

	EASTDC_API int Strcmp(const char16_t* pString1, const char16_t* pString2)
	{
		if(IsAligned<const char16_t, sizeof(word_type)>(pString1) && // If pString1 and pString2 are word-aligned... compare in word-sized chunks.
		   IsAligned<const char16_t, sizeof(word_type)>(pString2))
		{
			const word_type* pWord1 = (word_type*)pString1;
			const word_type* pWord2 = (word_type*)pString2;

			while(*pWord1 == *pWord2)
			{
				if(ZeroPresent16(*pWord1++))
					return 0;
				++pWord2;
			}

			// At this point, the strings differ somewhere in the bytes pointed to by pWord1/pWord2.
			pString1 = reinterpret_cast<const char16_t*>(pWord1); // Fall through and do byte comparisons for the rest of the string.
			pString2 = reinterpret_cast<const char16_t*>(pWord2);
		}

		while(*pString1 && (*pString1 == *pString2))
		{
			++pString1;
			++pString2;
		}

		return ((uint16_t)*pString1 - (uint16_t)*pString2);
	}
#else
	EASTDC_API int Strcmp(const char16_t* pString1, const char16_t* pString2)
	{
		char16_t c1, c2;

		while((c1 = *pString1++) == (c2 = *pString2++))
		{
			if(c1 == 0) // If we've reached the end of the string with no difference...
				return 0;
		}

		EA_COMPILETIME_ASSERT(sizeof(int) > sizeof(uint16_t));
		return ((uint16_t)c1 - (uint16_t)c2);
	}
#endif


EASTDC_API int Strcmp(const char32_t* pString1, const char32_t* pString2)
{
	char32_t c1, c2;

	while((c1 = *pString1++) == (c2 = *pString2++))
	{
		if(c1 == 0) // If we've reached the end of the string with no difference...
			return 0;
	}

	// We can't just return c1 - c2, because the difference might be greater than INT_MAX.
	return ((uint32_t)c1 > (uint32_t)c2) ? 1 : -1;
}



#if EASTDC_ENABLE_OPTIMIZED_STRCMP
	// Some platforms have an optimized vector implementation of strncmp which is fast and which provides
	// identical return value behavior to our Strncmp (which is to return the byte difference and not just
	// -1, 0, +1). And so until we have our own vectored version we use the built-in version.
	EASTDC_API int Strncmp(const char* pString1, const char* pString2, size_t n)
	{
		return strncmp(pString1, pString2, n);
	}

	// To do: Implement a general portable version of a more optimized Strncmp.

#else
	EASTDC_API int Strncmp(const char* pString1, const char* pString2, size_t n)
	{
		char c1, c2;

		++n;
		while(--n)
		{
			if((c1 = *pString1++) != (c2 = *pString2++))
				return ((uint8_t)c1 - (uint8_t)c2);
			else if(c1 == 0)
				break;
		}
		
		return 0;
	}
#endif

EASTDC_API int Strncmp(const char16_t* pString1, const char16_t* pString2, size_t n)
{
	char16_t c1, c2;

	// Code below which uses (c1 - c2) assumes this.
	EA_COMPILETIME_ASSERT(sizeof(int) > sizeof(uint16_t));

	++n;
	while(--n)
	{
		if((c1 = *pString1++) != (c2 = *pString2++))
			return ((uint16_t)c1 - (uint16_t)c2);
		else if(c1 == 0)
			break;
	}
		
	return 0;
}

EASTDC_API int Strncmp(const char32_t* pString1, const char32_t* pString2, size_t n)
{
	char32_t c1, c2;

	++n;
	while(--n)
	{
		if((c1 = *pString1++) != (c2 = *pString2++))
		{
			// We can't just return c1 - c2, because the difference might be greater than INT_MAX.
			return ((uint32_t)c1 > (uint32_t)c2) ? 1 : -1;
		}
		else if(c1 == 0)
			break;
	}

	return 0;
}



#if EASTDC_ENABLE_OPTIMIZED_STRCMP && (defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_OSX))
	// Some platforms have an optimized vector implementation of stricmp/strcasecmp which is fast and which provides
	// identical return value behavior to our Stricmp (which is to return the byte difference and not just
	// -1, 0, +1). And so until we have our own vectored version we use the built-in version.
	EASTDC_API int Stricmp(const char* pString1, const char* pString2)
	{
		return strcasecmp(pString1, pString2);
	}

	// To do: Implement a general portable version of a more optimized Stricmp.

#else
	EASTDC_API int Stricmp(const char* pString1, const char* pString2)
	{
		char c1, c2;

		while((c1 = Tolower(*pString1++)) == (c2 = Tolower(*pString2++)))
		{
			if(c1 == 0)
				return 0;
		}

		return ((uint8_t)c1 - (uint8_t)c2);
	}
#endif


EASTDC_API int Stricmp(const char16_t* pString1, const char16_t* pString2)
{
	char16_t c1, c2;

	while((c1 = Tolower(*pString1++)) == (c2 = Tolower(*pString2++)))
	{
		if(c1 == 0)
			return 0;
	}

	// Code below which uses (c1 - c2) assumes this.
	EA_COMPILETIME_ASSERT(sizeof(int) > sizeof(uint16_t));
	return ((uint16_t)c1 - (uint16_t)c2);
}

EASTDC_API int Stricmp(const char32_t* pString1, const char32_t* pString2)
{
	char32_t c1, c2;

	while((c1 = Tolower(*pString1++)) == (c2 = Tolower(*pString2++)))
	{
		if(c1 == 0)
			return 0;
	}

	// We can't just return c1 - c2, because the difference might be greater than INT_MAX.
	return ((uint32_t)c1 > (uint32_t)c2) ? 1 : -1;
}



EASTDC_API int Strnicmp(const char* pString1, const char* pString2, size_t n)
{
	char c1, c2;

	++n;
	while(--n)
	{
		if((c1 = Tolower(*pString1++)) != (c2 = Tolower(*pString2++)))
			return ((uint8_t)c1 - (uint8_t)c2);
		else if(c1 == 0)
			break;
	}
		
	return 0;
}

EASTDC_API int Strnicmp(const char16_t* pString1, const char16_t* pString2, size_t n)
{
	char16_t c1, c2;

	// Code below which uses (c1 - c2) assumes this.
	EA_COMPILETIME_ASSERT(sizeof(int) > sizeof(uint16_t));

	++n;
	while(--n)
	{
		if((c1 = Tolower(*pString1++)) != (c2 = Tolower(*pString2++)))
			return ((uint16_t)c1 - (uint16_t)c2);
		else if(c1 == 0)
			break;
	}
		
	return 0;
}

EASTDC_API int Strnicmp(const char32_t* pString1, const char32_t* pString2, size_t n)
{
	char32_t c1, c2;

	++n;
	while(--n)
	{
		if((c1 = Tolower(*pString1++)) != (c2 = Tolower(*pString2++)))
		{
			// We can't just return c1 - c2, because the difference might be greater than INT_MAX.
			return ((uint32_t)c1 > (uint32_t)c2) ? 1 : -1;
		}
		else if(c1 == 0)
			break;
	}
		
	return 0;
}




// *** this function is deprecated. ***
EASTDC_API int StrcmpAlnum(const char* pString1, const char* pString2)
{
	char c1, c2;
	const char* pStart1      = pString1;
	const char* pStart2      = pString2;
	const char* pDigitStart1 = pString1;

	while(((c1 = *pString1++) == (c2 = *pString2++)) && c1)
	{
		if(!Isdigit(c1))
			pDigitStart1 = pString1;
	}

	const int c1d = Isdigit(c1);
	const int c2d = Isdigit(c2);

	if(c1d && c2d)
		return (int)StrtoI32(pDigitStart1, NULL, 10) - (int)StrtoI32(pStart2 + (pDigitStart1 - pStart1), NULL, 10);

	if(c1d != c2d)  // If one char is decimal and the other is not..
		return c1d ? 1 : -1;

	return ((uint8_t)c1 - (uint8_t)c2);
}


// *** this function is deprecated. ***
EASTDC_API int StrcmpAlnum(const char16_t* pString1, const char16_t* pString2)
{
	char16_t c1, c2;
	const char16_t* pStart1      = pString1;
	const char16_t* pStart2      = pString2;
	const char16_t* pDigitStart1 = pString1;

	while(((c1 = *pString1++) == (c2 = *pString2++)) && c1)
	{
		if(!Isdigit(c1))
			pDigitStart1 = pString1;
	}

	const int c1d = Isdigit(c1);
	const int c2d = Isdigit(c2);

	if(c1d && c2d)
		return (int)StrtoI32(pDigitStart1, NULL, 10) - (int)StrtoI32(pStart2 + (pDigitStart1 - pStart1), NULL, 10);

	if(c1d != c2d)  // If one char is decimal and the other is not..
		return c1d ? 1 : -1;

	return ((uint16_t)c1 - (uint16_t)c2);
}


// *** this function is deprecated. ***
EASTDC_API int StricmpAlnum(const char* pString1, const char* pString2)
{
	char c1, c2;
	const char* pStart1      = pString1;
	const char* pStart2      = pString2;
	const char* pDigitStart1 = pString1;

	while(((c1 = Tolower(*pString1++)) == (c2 = Tolower(*pString2++))) && c1)
	{
		if(!Isdigit(c1))
			pDigitStart1 = pString1;
	}

	const int c1d = Isdigit(c1);
	const int c2d = Isdigit(c2);

	if(c1d && c2d)
		return (int)StrtoI32(pDigitStart1, NULL, 10) - (int)StrtoI32(pStart2 + (pDigitStart1 - pStart1), NULL, 10);

	if(c1d != c2d)  // If one char is decimal and the other is not..
		return c1d ? 1 : -1;

	return ((uint8_t)c1 - (uint8_t)c2);
}


// *** this function is deprecated. ***
EASTDC_API int StricmpAlnum(const char16_t* pString1, const char16_t* pString2)
{
	char16_t c1, c2;
	const char16_t* pStart1      = pString1;
	const char16_t* pStart2      = pString2;
	const char16_t* pDigitStart1 = pString1;

	while(((c1 = Tolower(*pString1++)) == (c2 = Tolower(*pString2++))) && c1)
	{
		if(!Isdigit(c1))
			pDigitStart1 = pString1;
	}

	const int c1d = Isdigit(c1);
	const int c2d = Isdigit(c2);

	if(c1d && c2d)
		return (int)StrtoI32(pDigitStart1, NULL, 10) - (int)StrtoI32(pStart2 + (pDigitStart1 - pStart1), NULL, 10);

	if(c1d != c2d)  // If one char is decimal and the other is not..
		return c1d ? 1 : -1;

	return ((uint16_t)c1 - (uint16_t)c2);
}



EASTDC_API int StrcmpNumeric(const char* pString1, const char* pString2, 
							 size_t length1, size_t length2, 
							 char decimal, char thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}

EASTDC_API int StrcmpNumeric(const char16_t* pString1, const char16_t* pString2, 
							 size_t length1, size_t length2, 
							 char16_t decimal, char16_t thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}

EASTDC_API int StrcmpNumeric(const char32_t* pString1, const char32_t* pString2, 
							 size_t length1, size_t length2, 
							 char32_t decimal, char32_t thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}




EASTDC_API int StricmpNumeric(const char* pString1, const char* pString2, 
							  size_t length1, size_t length2, 
							  char decimal, char thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}

EASTDC_API int StricmpNumeric(const char16_t* pString1, const char16_t* pString2, 
							  size_t length1, size_t length2, 
							  char16_t decimal, char16_t thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}

EASTDC_API int StricmpNumeric(const char32_t* pString1, const char32_t* pString2, 
							  size_t length1, size_t length2, 
							  char32_t decimal, char32_t thousandsSeparator)
{
	// To do: Implement this function. Ask Paul Pedriana to implement this if you need it.
	EA_UNUSED(pString1);
	EA_UNUSED(pString2);
	EA_UNUSED(length1);
	EA_UNUSED(length2);
	EA_UNUSED(decimal);
	EA_UNUSED(thousandsSeparator);

	return 0;
}





EASTDC_API int Strcoll(const char* pString1, const char* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strcmp(pString1, pString2);
}

EASTDC_API int Strcoll(const char16_t* pString1, const char16_t* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strcmp(pString1, pString2);
}

EASTDC_API int Strcoll(const char32_t* pString1, const char32_t* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strcmp(pString1, pString2);
}




EASTDC_API int Strncoll(const char* pString1, const char* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strncmp(pString1, pString2, n);
}

EASTDC_API int Strncoll(const char16_t* pString1, const char16_t* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strncmp(pString1, pString2, n);
}

EASTDC_API int Strncoll(const char32_t* pString1, const char32_t* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strncmp(pString1, pString2, n);
}




EASTDC_API int Stricoll(const char* pString1, const char* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Stricmp(pString1, pString2);
}

EASTDC_API int Stricoll(const char16_t* pString1, const char16_t* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Stricmp(pString1, pString2);
}

EASTDC_API int Stricoll(const char32_t* pString1, const char32_t* pString2)
{
	// The user needs to use a localization package to get proper localized collation.
	return Stricmp(pString1, pString2);
}




EASTDC_API int Strnicoll(const char* pString1, const char* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strnicmp(pString1, pString2, n);
}

EASTDC_API int Strnicoll(const char16_t* pString1, const char16_t* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strnicmp(pString1, pString2, n);
}

EASTDC_API int Strnicoll(const char32_t* pString1, const char32_t* pString2, size_t n)
{
	// The user needs to use a localization package to get proper localized collation.
	return Strnicmp(pString1, pString2, n);
}





///////////////////////////////////////////////////////////////////////////////
// EcvtBuf / FcvtBuf
//
#if EASTDC_NATIVE_FCVT    
	EASTDC_API char* EcvtBuf(double dValue, int nDigitCount, int* decimalPos, int* sign, char* buffer)
	{
		#ifdef __GNUC__
			const char* const pResult =  ecvt(dValue, nDigitCount, decimalPos, sign);
		#else
			const char* const pResult = _ecvt(dValue, nDigitCount, decimalPos, sign);
		#endif

		strcpy(buffer, pResult);

		#if EASTDC_NATIVE_FCVT_SHORT
			// For ecvt, nDigitCount is the resulting length of the buffer of digits, regardless of the decimal point location.
			if(nDigitCount > 15) // The '> 15' part is a quick check to avoid the rest of the code for most cases.
			{
				int len = (int)strlen(buffer);

				while(len < nDigitCount)
					buffer[len++] = '0';
				buffer[len] = 0;
			}
		#endif

		return buffer;
	}

	EASTDC_API char* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* decimalPos, int* sign, char* buffer)
	{
		#ifdef __GNUC__
			const char* const pResult =  fcvt(dValue, nDigitCountAfterDecimal, decimalPos, sign);
		#else
			char pResult[_CVTBUFSIZE+1];
			_fcvt_s(pResult, sizeof(pResult), dValue, nDigitCountAfterDecimal, decimalPos, sign);
		#endif

		strcpy(buffer, pResult);

		#if EASTDC_NATIVE_FCVT_SHORT
			// For fcvt, nDigitCount is the resulting length of the buffer of digits after the decimal point location.
			nDigitCountAfterDecimal += *decimalPos;

			if(nDigitCountAfterDecimal > 15) // The '> 15' part is a quick check to avoid the rest of the code for most cases.
			{
				int len = (int)strlen(buffer);

				while(len < nDigitCountAfterDecimal)
					buffer[len++] = '0';
				buffer[len] = 0;
			}
		#endif

		return buffer;
	}

#else

	#if defined(EA_COMPILER_MSVC)
		#include <float.h>
		#define isnan(x)  _isnan(x)
	  //#define isinf(x) !_finite(x)
	#endif

	#if !defined(isnan)

		inline bool isnan(double fValue)
		{
			const union {
				double  f;
				int64_t i;
			} converter = { fValue };

			// An IEEE real value is a NaN if all exponent bits are one and
			// the mantissa is not zero.
			return (converter.i & ~kFloat64SignMask) > kFloat64ExponentMask;
		}
	#endif

	union DoubleShape
	{
		double   mValue;
		uint32_t mUint64;

		#if defined(EA_SYSTEM_LITTLE_ENDIAN)
			struct numberStruct
			{
				unsigned int fraction1 : 32;
				unsigned int fraction0 : 20;
				unsigned int exponent  : 11;
				unsigned int sign      :  1;
			} mNumber;
		#else
			struct numberStruct
			{
				unsigned int sign      :  1;
				unsigned int exponent  : 11;
				unsigned int fraction0 : 20;
				unsigned int fraction1 : 32;
			} mNumber;
		#endif
	};

	union FloatShape
	{
		float    mValue;
		uint32_t mUint32;

		#if defined(EA_SYSTEM_LITTLE_ENDIAN)
			struct numberStruct
			{
				unsigned int fraction : 23;
				unsigned int exponent :  8;
				unsigned int sign     :  1;
			} mNumber;
		#else
			struct numberStruct
			{
				unsigned int sign     :  1;
				unsigned int exponent :  8;
				unsigned int fraction : 23;
			} mNumber;
		#endif
	};


	EASTDC_API char* EcvtBuf(double dValue, int nDigitCount, int* decimalPos, int* sign, char* buffer)
	{
		int      nDigitCountAfterDecimal;
		double   fract;
		double   integer;
		double   tmp;
		int      neg = 0;
		int      expcnt = 0;
		char* buf = buffer;
		char* t = buf;
		char* p = buf + kEcvtBufMaxSize - 1;
		char* pbuf = p;

		// We follow the same preconditions as Microsoft does with its _ecvt function.
		EA_ASSERT((nDigitCount >= 0) && (decimalPos != NULL) && (sign != NULL) && (buffer != NULL));

		// assume decimal to left of digits in string
		*decimalPos = 0;

		// To consider: Enable the following.
		//if(nDigitCount > 16) // It turns out that we can't get any more precision than this.
		//   nDigitCount = 16; // Any digits beyond 16 would be nearly meaningless.

		if(sizeof(double) == sizeof(float)) // If the user has the compiler set to use doubles that are smaller...
		{
			FloatShape floatShape;
			floatShape.mValue = (float)dValue; // This should be a lossless conversion.

			if(floatShape.mNumber.exponent == 0xff) // If not finite...
			{
				if(floatShape.mUint32 & 0x007fffff) // If is a NAN...
				{
					*t++ = 'N';
					*t++ = 'A';
					*t++ = 'N';
				}
				else
				{
					*t++ = 'I';
					*t++ = 'N';
					*t++ = 'F';
				}
				*t = 0;
				return buffer;
			}
		}
		else
		{
			DoubleShape doubleShape;
			doubleShape.mValue = dValue;

			if(doubleShape.mNumber.exponent == 0x7ff) // If not finite...
			{
				if(isnan(dValue)) // If is a NAN...
				{
					*t++ = 'N';
					*t++ = 'A';
					*t++ = 'N';
				}
				else
				{
					*t++ = 'I';
					*t++ = 'N';
					*t++ = 'F';
				}
				*t = 0;
				return buffer;
			}
		}

		if(dValue < 0)
		{
			neg  = 1;
			dValue = -dValue;
		}

		fract = modf(dValue, &integer);

		if(dValue >= 1.0f)
		{
			for(; integer; ++expcnt)
			{
				tmp = modf(integer / 10.0f, &integer);
				*p-- = (char)((int)((tmp + 0.01f) * 10.0f) + '0');
				EA_ASSERT(p >= buffer);
			}
		}

		*t++ = 0;   // Extra slot for rounding
		buf += 1;   // Point return value to beginning of string.

		int tempExp = expcnt;
		nDigitCountAfterDecimal = nDigitCount - expcnt;
		
		if(expcnt)
		{
			//if expcnt > nDigitCount, need to round the integer part, and reset expcnt
			if(expcnt > nDigitCount)
			{
				pbuf = p + nDigitCount + 1;

				if(*pbuf >= '5')
				{
					do
					{
						pbuf--;
						if(++*pbuf <= '9')
							break;
						*pbuf = '0';
					}
					while(pbuf >= p+1);
				}

				expcnt = nDigitCount;
				fract = 0.0;//no more rounding will be needed down below!
			}

			for(++p; expcnt--;)
				*t++ = *p++;
		}

		if(nDigitCountAfterDecimal >= 0)
		{
			// Per spec, don't actually put decimal in string, just let caller know where it should be...
			*decimalPos = (int)(ptrdiff_t)(t - buf); // Count of chars into string when to place decimal point
		}
		else
			*decimalPos = (int)tempExp;

		bool leading = dValue < 1.0f ? true : false;//for Ecvt, leading zeros need to be omitted and decimalPos needs to be readjusted
		while((nDigitCountAfterDecimal > 0) && fract)
		{
			fract = modf(fract * 10.0f, &tmp);
			
			if(leading && (int)tmp == 0)
			{
				(*decimalPos)--;
				continue;
			}
			else
			{
				leading = false;
				*t++  = (char)((int)tmp + '0');
				nDigitCountAfterDecimal -= 1;
			}
		} 
		
		if(fract)
		{
			char* scan = (t - 1);

			// round off the number
			modf(fract * 10.0f, &tmp);

			if(tmp > 4)
			{
				for(; ; --scan)
				{
					if(*scan == '.')
						scan -= 1;
					if(++*scan <= '9')
						break;
					*scan = '0';
					if(scan == buf)
					{
						*--scan = '1';
						buf -= 1;      // Rounded into holding spot
						++*decimalPos;  // This line added by Paul Pedriana, May 8 2008, in order to fix a bug where ("%.1f", 0.952) gave "0.1" instead of "1.0". I need to investigate this more to verify the fix.
						break;
					}
				}
			} 
			else if(neg)
			{
				// fix ("%.3f", -0.0004) giving -0.000
				for( ; ; scan -= 1)
				{
					if(scan <= buf)
						break;
					if(*scan == '.')
						scan -= 1;
					if(*scan != '0')
						break;
					if(scan == buf)
						neg = 0;
				}
			}
		}
		
		if(nDigitCountAfterDecimal<0)//this means the digitcount is smaller than integre part and need to round the integer part
			nDigitCountAfterDecimal = 0;
		
		while(nDigitCountAfterDecimal--)
			*t++ = '0';
		*t++ = 0; // Always terminate the string of digits

		if(*buffer == 0) // If the above rounding place wasn't necessary...
			memmove(buffer, buffer + 1, (size_t)(t - (buffer + 1)));

		*sign = neg ? 1 : 0;

		return buffer;

	}

	EASTDC_API char* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* decimalPos, int* sign, char* buffer)
	{
		double   fract;
		double   integer;
		double   tmp;
		int      neg = 0;
		int      expcnt = 0;
		char* buf = buffer;
		char* t = buf;
		char* p = buf + kFcvtBufMaxSize - 1;

		// We follow the same preconditions as Microsoft does with its _fcvt function.
		EA_ASSERT((nDigitCountAfterDecimal >= 0) && (decimalPos != NULL) && (sign != NULL) && (buffer != NULL));

		// assume decimal to left of digits in string
		*decimalPos = 0;

		if(sizeof(double) == sizeof(float)) // If the user has the compiler set to use doubles that are smaller...
		{
			FloatShape floatShape;
			floatShape.mValue = (float)dValue; // This should be a lossless conversion.

			if(floatShape.mNumber.exponent == 0xff) // If not finite...
			{
				if(floatShape.mUint32 & 0x007fffff) // If is a NAN...
				{
					*t++ = 'N';
					*t++ = 'A';
					*t++ = 'N';
				}
				else
				{
					*t++ = 'I';
					*t++ = 'N';
					*t++ = 'F';
				}
				*t = 0;
				return buffer;
			}
		}
		else
		{
			DoubleShape doubleShape;
			doubleShape.mValue = dValue;

			if(doubleShape.mNumber.exponent == 0x7ff) // If not finite...
			{
				if(isnan(dValue)) // If is a NAN...
				{
					*t++ = 'N';
					*t++ = 'A';
					*t++ = 'N';
				}
				else
				{
					*t++ = 'I';
					*t++ = 'N';
					*t++ = 'F';
				}
				*t = 0;
				return buffer;
			}
		}

		if(dValue < 0)
		{
			neg  = 1;
			dValue = -dValue;
		}

		fract = modf(dValue, &integer);

		if(dValue >= 1.0f)
		{
			for(; integer; ++expcnt)
			{
				tmp = modf(integer / 10.0f, &integer);
				*p-- = (char)((int)((tmp + 0.01f) * 10.0f) + '0');
				EA_ASSERT(p >= buffer);
			}
		}

		*t++ = 0;   // Extra slot for rounding
		buf += 1;   // Point return value to beginning of string.
		
		if(expcnt)
		{
			for(++p; expcnt--;)
				*t++ = *p++;
		}

		// Per spec, don't actually put decimal in string, just let caller know where it should be...
		*decimalPos = (int)(ptrdiff_t)(t - buf); // Count of chars into string when to place decimal point.

		// We give up trying to calculate fractions beyond 16 digits, which is the maximum possible precision with a double.
		int count = (nDigitCountAfterDecimal <= 16) ? nDigitCountAfterDecimal : 16;
		while(count && fract)
		{
			fract = modf(fract * 10.0f, &tmp);
			*t++  = (char)((int)tmp + '0');
			nDigitCountAfterDecimal--;
			count--;
		}

		if(fract)
		{
			char* scan = (t - 1);

			// round off the number
			modf(fract * 10.0f, &tmp);

			if(tmp > 4)
			{
				for(; ; --scan)
				{
					if(*scan == '.')
						scan -= 1;
					if(++*scan <= '9')
						break;
					*scan = '0';
					if(scan == buf)
					{
						*--scan = '1';
						buf -= 1;       // Rounded into holding spot
						++*decimalPos;  // This line added by Paul Pedriana, May 8 2008, in order to fix a bug where ("%.1f", 0.952) gave "0.1" instead of "1.0". I need to investigate this more to verify the fix.
						break;
					}
				}
			} 
			else if(neg)
			{
				// fix ("%.3f", -0.0004) giving -0.000
				for( ; ; --scan)
				{
					if(scan <= buf)
						break;
					if(*scan == '.')
						scan -= 1;
					if(*scan != '0')
						break;
					if(scan == buf)
						neg = 0;
				}
			}
		}

		while(nDigitCountAfterDecimal--)
			*t++ = '0';
		*t++ = 0; // Always terminate the string of digits

		if(*buffer == 0) // If the above rounding place wasn't necessary...
			memmove(buffer, buffer + 1, (size_t)(t - (buffer + 1)));

		*sign = neg ? 1 : 0;

		return buffer;
	}

	// Matching #undef for each #define above for unity build friendliness.
	#if defined(EA_COMPILER_MSVC)
		#undef isnan
	  //#undef isinf
	#endif

#endif // Compiler support



EASTDC_API char16_t* EcvtBuf(double dValue, int nDigitCount, int* decimalPos, int* sign, char16_t* buffer)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBufferCvt8[kEcvtBufMaxSize];
	char16_t* pCurrent16 = buffer;

	EcvtBuf(dValue, nDigitCount, decimalPos, sign, pBufferCvt8);

	for(char* pCurrent8 = pBufferCvt8; *pCurrent8; ) // Do a 8 bit to 16 bit strcpy.
		*pCurrent16++ = (char16_t)(unsigned char)*pCurrent8++;

	*pCurrent16 = 0;

	return buffer;
}

EASTDC_API char32_t* EcvtBuf(double dValue, int nDigitCount, int* decimalPos, int* sign, char32_t* buffer)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBufferCvt8[kEcvtBufMaxSize];
	char32_t* pCurrent32 = buffer;

	EcvtBuf(dValue, nDigitCount, decimalPos, sign, pBufferCvt8);

	for(char* pCurrent8 = pBufferCvt8; *pCurrent8; ) // Do a 8 bit to 32 bit strcpy.
		*pCurrent32++ = (char32_t)(unsigned char)*pCurrent8++;

	*pCurrent32 = 0;

	return buffer;
}




EASTDC_API char16_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* decimalPos, int* sign, char16_t* buffer)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBufferCvt8[kEcvtBufMaxSize];
	char16_t* pCurrent16 = buffer;

	FcvtBuf(dValue, nDigitCountAfterDecimal, decimalPos, sign, pBufferCvt8);

	for(char* pCurrent8 = pBufferCvt8; *pCurrent8; ) // Do a 8 bit to 16 bit strcpy.
		*pCurrent16++ = (char16_t)(unsigned char)*pCurrent8++;

	*pCurrent16 = 0;

	return buffer;
}

EASTDC_API char32_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* decimalPos, int* sign, char32_t* buffer)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBufferCvt8[kEcvtBufMaxSize];
	char32_t* pCurrent32 = buffer;

	FcvtBuf(dValue, nDigitCountAfterDecimal, decimalPos, sign, pBufferCvt8);

	for(char* pCurrent8 = pBufferCvt8; *pCurrent8; ) // Do a 8 bit to 32 bit strcpy.
		*pCurrent32++ = (char32_t)(unsigned char)*pCurrent8++;

	*pCurrent32 = 0;

	return buffer;
}

// end of EcvtBuf / FcvtBuf
////////////////////////////////////////////////////////////////////////////////////



// Optimization technique:
//     https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
// This results in performance improvements of 2x to 5x depending on the input value. Our general test in 
// TestString.cpp showed a 3.1x performance gain on VC++/x64.
//
static uint32_t digits10(uint64_t v)
{
	if(v < 10)
		return 1;
	if(v < 100)
		return 2;
	if(v < 1000)
		return 3;
	if(v < UINT64_C(1000000000000))
	{
		if(v < UINT64_C(100000000))
		{
			if(v < 1000000)
			{
				if (v < 10000)
					return 4;
				return (uint32_t)(5 + (v >= 100000));
			}

			return (uint32_t)(7 + (v >= 10000000));
		}

		if(v < UINT64_C(10000000000))
			return (uint32_t)(9 + (v >= UINT64_C(1000000000)));

		return (uint32_t)(11 + (v >= UINT64_C(100000000000)));
	}

	return 12 + digits10(v / UINT64_C(1000000000000));
}

char* X64toaCommon10(uint64_t nValue, char* pBuffer)
{
	static const char digits[201] =
		"0001020304050607080910111213141516171819202122232425262728293031323334353637383940414243444546474849"
		"5051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";

	uint32_t length = digits10(nValue);
	uint32_t next   = length - 1;

	pBuffer[length] = '\0';

	while(nValue >= 100)
	{
		const uint64_t i = (nValue % 100) * 2;
		nValue /= 100;
		pBuffer[next] = digits[i + 1];
		pBuffer[next - 1] = digits[i];
		next -= 2;
	}

	if (nValue < 10)
		pBuffer[next] = (char)('0' + (uint32_t)nValue);
	else
	{
		const uint32_t i  = (uint32_t)nValue * 2;
		pBuffer[next]     = digits[i + 1];
		pBuffer[next - 1] = digits[i];
	}

	return pBuffer;
}


static char* X64toaCommon(uint64_t nValue, char* pBuffer, int nBase, bool bNegative)
{
	char* pCurrent = pBuffer;

	if(bNegative)
		*pCurrent++ = '-';

	if(nBase == 10)
		X64toaCommon10(nValue, pCurrent);
	else
	{
		char* pFirstDigit = pCurrent;

		do{
			const unsigned nDigit = (unsigned)(nValue % nBase);
			nValue /= nBase;

			if(nDigit > 9)
				*pCurrent++ = (char)(nDigit - 10 + 'a');
			else
				*pCurrent++ = (char)(nDigit + '0');
		} while(nValue > 0);

		// Need to reverse the string.
		*pCurrent-- = 0;

		do{
			const char cTemp = *pCurrent;
			*pCurrent--         = *pFirstDigit;
			*pFirstDigit++      =  cTemp;
		}while(pFirstDigit < pCurrent);
	}

	return pBuffer;
}


static char16_t* X64toaCommon(uint64_t nValue, char16_t* pBuffer, int nBase, bool bNegative)
{
	char16_t* pCurrent = pBuffer;

	if(bNegative)
		*pCurrent++ = '-';

	char16_t* pFirstDigit = pCurrent;

	do{
		const unsigned nDigit = (unsigned)(nValue % nBase);
		nValue /= nBase;

		if(nDigit > 9)
			*pCurrent++ = (char16_t)(nDigit - 10 + 'a');
		else
			*pCurrent++ = (char16_t)(nDigit + '0');
	} while(nValue > 0);

	// Need to reverse the string.
	*pCurrent-- = 0;

	do{
		const char16_t cTemp = *pCurrent;
		*pCurrent--          = *pFirstDigit;
		*pFirstDigit++       =  cTemp;
	}while(pFirstDigit < pCurrent);

	return pBuffer;
}

static char32_t* X64toaCommon(uint64_t nValue, char32_t* pBuffer, int nBase, bool bNegative)
{
	char32_t* pCurrent = pBuffer;

	if(bNegative)
		*pCurrent++ = '-';

	char32_t* pFirstDigit = pCurrent;

	do{
		const unsigned nDigit = (unsigned)(nValue % nBase);
		nValue /= nBase;

		if(nDigit > 9)
			*pCurrent++ = (char32_t)(nDigit - 10 + 'a');
		else
			*pCurrent++ = (char32_t)(nDigit + '0');
	} while(nValue > 0);

	// Need to reverse the string.
	*pCurrent-- = 0;

	do{
		const char32_t cTemp = *pCurrent;
		*pCurrent--          = *pFirstDigit;
		*pFirstDigit++       =  cTemp;
	}while(pFirstDigit < pCurrent);

	return pBuffer;
}



EASTDC_API char* I32toa(int32_t nValue, char* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
	{
		#if defined(__GNUC__)   // -INT32_MIN => INT32_MIN, but with GCC on Android it's acting differently.
		if(nValue != INT32_MIN)
		#endif
		nValue = -nValue;
	}

	return X64toaCommon((uint64_t)(uint32_t)nValue, pBuffer, nBase, bNegative);
}

EASTDC_API char16_t* I32toa(int32_t nValue, char16_t* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
	{
		#if defined(__GNUC__)   // -INT32_MIN => INT32_MIN, but with GCC on Android it's acting differently.
		if(nValue != INT32_MIN)
		#endif
		nValue = -nValue;
	}

	return X64toaCommon((uint64_t)(uint32_t)nValue, pBuffer, nBase, bNegative);
}

EASTDC_API char32_t* I32toa(int32_t nValue, char32_t* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
	{
		#if defined(__GNUC__)   // -INT32_MIN => INT32_MIN, but with GCC on Android it's acting differently.
		if(nValue != INT32_MIN)
		#endif
		nValue = -nValue;
	}

	return X64toaCommon((uint64_t)(uint32_t)nValue, pBuffer, nBase, bNegative);
}



EASTDC_API char* U32toa(uint32_t nValue, char* pBuffer, int nBase)
{
	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, 0);
}

EASTDC_API char16_t* U32toa(uint32_t nValue, char16_t* pBuffer, int nBase)
{
	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, 0);
}

EASTDC_API char32_t* U32toa(uint32_t nValue, char32_t* pBuffer, int nBase)
{
	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, 0);
}




EASTDC_API char* I64toa(int64_t nValue, char* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
		nValue = -(uint64_t)nValue;

	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, bNegative);
}

EASTDC_API char16_t* I64toa(int64_t nValue, char16_t* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
		nValue = -(uint64_t)nValue;

	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, bNegative);
}

EASTDC_API char32_t* I64toa(int64_t nValue, char32_t* pBuffer, int nBase)
{
	const bool bNegative = (nValue < 0) && (nBase == 10);

	if(bNegative)
		nValue = -(uint64_t)nValue;

	return X64toaCommon((uint64_t)nValue, pBuffer, nBase, bNegative);
}




EASTDC_API char* U64toa(uint64_t nValue, char* pBuffer, int nBase)
{
	return X64toaCommon(nValue, pBuffer, nBase, 0);
}

EASTDC_API char16_t* U64toa(uint64_t nValue, char16_t* pBuffer, int nBase)
{
	return X64toaCommon(nValue, pBuffer, nBase, 0);
}

EASTDC_API char32_t* U64toa(uint64_t nValue, char32_t* pBuffer, int nBase)
{
	return X64toaCommon(nValue, pBuffer, nBase, 0);
}





EASTDC_API double StrtodEnglish(const char* pValue, char** ppEnd)
{
	// This implementation is an exact copy of StrtodEnglish but 
	// with char in place of char16_t. For the time being, if 
	// you do maintenance on either of these functions, you need to 
	// copy the result to the other version.
	int            c;
	double         dTotal(0.0);
	char        chSign('+');
	const char* pEnd = pValue;

	while(Isspace(*pValue))
		++pValue;    //Remove leading spaces.

	pEnd =  pValue;
	c    = *pValue++;
	if(c == '-' || c == '+'){
		chSign = (char)c;
		pEnd   = pValue;
		c      = *pValue++;
	}

	while((c >= '0') && (c <= '9')){
		dTotal = (10 * dTotal) + (c - '0');
		pEnd = pValue;
		c = *pValue++;
	}

	if(c == '.'){
		double dMultiplier(1); //Possibly some BCD variable would be more accurate.

		pEnd = pValue;
		c    = *pValue++;
		while((c >= '0') && (c <= '9')){
			dMultiplier *= 0.1;
			dTotal += (c - '0') * dMultiplier;
			pEnd = pValue;
			c    = *pValue++;
		}
	}

	if(c == 'e' || c == 'E'){
		int     nExponentValue(0);
		double  dExponentTotal;
		char chExponentSign('+');

		pEnd = pValue;
		c    = *pValue++; //Move past the exponent.

		if(c == '-' || c == '+'){
			chExponentSign = (char)c;
			pEnd = pValue;
			c    = *pValue++; //Move past the '+' or '-' sign.
		}

		while((c >= '0') && (c <= '9')){
			nExponentValue = (10 * nExponentValue) + (c - '0');
			pEnd = pValue;
			c    = *pValue++;
		}

		dExponentTotal = ::pow(10.0, (double)nExponentValue); // The CRT pow function is actually somewhat slow and weak.
															  // It would be very nice to change this to at least implement
		if(chExponentSign == '-')                             // the low exponents with a lookup table.
			dExponentTotal = 1/dExponentTotal;
		dTotal *= dExponentTotal;
	}

	if(ppEnd)
		*ppEnd = (char*)pEnd;

	if(chSign == '-')
		return -dTotal;
	return dTotal;
}

EASTDC_API double StrtodEnglish(const char16_t* pValue, char16_t** ppEnd)
{
	// This implementation is an exact copy of StrtodEnglish8 but 
	// with char16_t in place of char. For the time being, if you
	// do maintenance on either of these functions, you need to 
	// copy the result to the other version.
	char16_t        c;
	double          dTotal(0.0);
	char16_t        chSign('+');
	const char16_t* pEnd = pValue;

	while(Isspace(*pValue))
		++pValue;    // Remove leading spaces.

	pEnd =  pValue;
	c    = *pValue++;
	if(c == '-' || c == '+'){
		chSign = (char16_t)c;
		pEnd   = pValue;
		c      = *pValue++;
	}

	while((c >= '0') && (c <= '9')){
		dTotal = (10 * dTotal) + (c - '0');
		pEnd = pValue;
		c = *pValue++;
	}

	if(c == '.'){
		double dMultiplier(1); // Possibly some BCD variable would be more accurate.

		pEnd = pValue;
		c    = *pValue++;
		while((c >= '0') && (c <= '9')){
			dMultiplier *= 0.1;
			dTotal += (c - '0') * dMultiplier;
			pEnd = pValue;
			c    = *pValue++;
		}
	}

	if(c == 'e' || c == 'E'){
		int      nExponentValue(0);
		double   dExponentTotal;
		char16_t chExponentSign('+');

		pEnd = pValue;
		c    = *pValue++; //Move past the exponent.

		if(c == '-' || c == '+'){
			chExponentSign = (char16_t)c;
			pEnd = pValue;
			c    = *pValue++; // Move past the '+' or '-' sign.
		}

		while((c >= '0') && (c <= '9')){
			nExponentValue = (int)((10 * nExponentValue) + (c - '0'));
			pEnd = pValue;
			c    = *pValue++;
		}

		dExponentTotal = ::pow(10.0, (double)nExponentValue);  // The CRT pow function is actually somewhat slow and weak.
															   // It would be very nice to change this to at least implement
		if(chExponentSign == '-')                              // the low exponents with a lookup table.
			dExponentTotal = 1/dExponentTotal;
		dTotal *= dExponentTotal;
	}

	if(ppEnd)
		*ppEnd = (char16_t*)pEnd;

	if(chSign == '-')
		return -dTotal;
	return dTotal;
}

EASTDC_API double StrtodEnglish(const char32_t* pValue, char32_t** ppEnd)
{
	// This implementation is an exact copy of StrtodEnglish8 but 
	// with char32_t in place of char. For the time being, if you
	// do maintenance on either of these functions, you need to 
	// copy the result to the other version.
	char32_t        c;
	double          dTotal(0.0);
	char32_t        chSign('+');
	const char32_t* pEnd = pValue;

	while(Isspace(*pValue))
		++pValue;    // Remove leading spaces.

	pEnd =  pValue;
	c    = *pValue++;
	if(c == '-' || c == '+'){
		chSign = (char32_t)c;
		pEnd   = pValue;
		c      = *pValue++;
	}

	while((c >= '0') && (c <= '9')){
		dTotal = (10 * dTotal) + (c - '0');
		pEnd = pValue;
		c = *pValue++;
	}

	if(c == '.'){
		double dMultiplier(1); // Possibly some BCD variable would be more accurate.

		pEnd = pValue;
		c    = *pValue++;
		while((c >= '0') && (c <= '9')){
			dMultiplier *= 0.1;
			dTotal += (c - '0') * dMultiplier;
			pEnd = pValue;
			c    = *pValue++;
		}
	}

	if(c == 'e' || c == 'E'){
		int      nExponentValue(0);
		double   dExponentTotal;
		char32_t chExponentSign('+');

		pEnd = pValue;
		c     = *pValue++; //Move past the exponent.

		if(c == '-' || c == '+'){
			chExponentSign = (char32_t)c;
			pEnd = pValue;
			c    = *pValue++; // Move past the '+' or '-' sign.
		}

		while((c >= '0') && (c <= '9')){
			nExponentValue = (int)((10 * nExponentValue) + (c - '0'));
			pEnd = pValue;
			c    = *pValue++;
		}

		dExponentTotal = ::pow(10.0, (double)nExponentValue);  // The CRT pow function is actually somewhat slow and weak.
															   // It would be very nice to change this to at least implement
		if(chExponentSign == '-')                              // the low exponents with a lookup table.
			dExponentTotal = 1/dExponentTotal;
		dTotal *= dExponentTotal;
	}

	if(ppEnd)
		*ppEnd = (char32_t*)pEnd;

	if(chSign == '-')
		return -dTotal;
	return dTotal;
}




static uint64_t StrtoU64Common(const char* pValue, char** ppEnd, int nBase, bool bUnsigned)
{
	uint64_t       nValue(0);                 // Current value
	const char* p = pValue;                // Current position
	char        c;                         // Temp value
	char        chSign('+');               // One of either '+' or '-'
	bool           bDigitWasRead(false);      // True if any digits were read.
	bool           bOverflowOccurred(false);  // True if integer overflow occurred.

	// Skip leading whitespace
	c = *p++;
	while(Isspace(c))
		c = *p++;

	// Check for sign.
	if((c == '-') || (c == '+')){
		chSign = c;
		c = *p++;
	}

	// Do checks on nBase.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (char*)pValue;
		return 0;
	}
	else if(nBase == 0){
		// Auto detect one of base 8, 10, or 16. 
		if(c != '0')
			nBase = 10;
		else if(*p == 'x' || *p == 'X')
			nBase = 16;
		else
			nBase = 8;
	}
	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((c == '0') && ((*p == 'x') || (*p == 'X'))) {
			++p;
			c = *p++;
		}
	}

	// If nValue exceeds this, an integer overflow is reported.
	#if (EA_PLATFORM_WORD_SIZE >= 8)
		const uint64_t nMaxValue(UINT64_MAX / nBase);
		const uint64_t nModValue(UINT64_MAX % nBase);
	#else
		// 32 bit platforms are very slow at doing 64 bit div and mod operations.
		uint64_t nMaxValue;
		uint64_t nModValue;

		switch(nBase)
		{
			case 2:
				nMaxValue = UINT64_C(9223372036854775807);
				nModValue = 1;
				break;
			case 8:
				nMaxValue = UINT64_C(2305843009213693951);
				nModValue = 7;
				break;
			case 10:
				nMaxValue = UINT64_C(1844674407370955161);
				nModValue = 5;
				break;
			case 16:
				nMaxValue = UINT64_C(1152921504606846975);
				nModValue = 15;
				break;
			default:
				nMaxValue = (UINT64_MAX / nBase);
				nModValue = (UINT64_MAX % nBase);
				break;
		}
	#endif

	for(unsigned nCurrentDigit; ; ){
		if(Isdigit(c))
			nCurrentDigit = (unsigned)(c - '0');
		else if(Isalpha(c))
			nCurrentDigit = (unsigned)(Toupper(c) - 'A' + 10);
		else
			break; // The digit is invalid.

		if(nCurrentDigit >= (unsigned)nBase)
			break; // The digit is invalid.

		bDigitWasRead = true;

		// Check for overflow.
		if((nValue < nMaxValue) || ((nValue == nMaxValue) && ((uint64_t)nCurrentDigit <= nModValue)))
			nValue = (nValue * nBase) + nCurrentDigit;
		else
			bOverflowOccurred = true; // Set the flag, but continue processing.

		c = *p++;
	}

	--p; // Go back to the last character

	if(!bDigitWasRead){
		if(ppEnd)
			p = pValue; // We'll assign 'ppEnd' below.
	}
	else if(bOverflowOccurred || (!bUnsigned && (((chSign == '-') && (nValue > ((uint64_t)INT64_MAX + 1))) || ((chSign == '+') && (nValue > (uint64_t)INT64_MAX))))){
		// Integer overflow occurred.
		if(bUnsigned)
			nValue = UINT64_MAX;
		else if(chSign == '-')
			nValue = (uint64_t)INT64_MAX + 1; // INT64_MAX + 1 is the same thing as -INT64_MIN with most compilers.
		else
			nValue = INT64_MAX;

		errno = ERANGE; // The standard specifies that we set this value.
	}

	if(ppEnd)
		*ppEnd = (char*)p;

	if(chSign == '-')
		nValue = -nValue;

	return nValue;
}

static uint64_t StrtoU64Common(const char16_t* pValue, char16_t** ppEnd, int nBase, bool bUnsigned)
{
	uint64_t        nValue(0);                 // Current value
	const char16_t* p = pValue;                // Current position
	char16_t        c;                         // Temp value
	char16_t        chSign('+');               // One of either '+' or '-'
	bool            bDigitWasRead(false);      // True if any digits were read.
	bool            bOverflowOccurred(false);  // True if integer overflow occurred.

	// Skip leading whitespace
	c = *p++;
	while(Isspace(c))
		c = *p++;

	// Check for sign.
	if((c == '-') || (c == '+')){
		chSign = c;
		c = *p++;
	}

	// Do checks on nBase.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (char16_t*)pValue;
		return 0;
	}
	else if(nBase == 0){
		// Auto detect one of base 8, 10, or 16. 
		if(c != '0')
			nBase = 10;
		else if(*p == 'x' || *p == 'X')
			nBase = 16;
		else
			nBase = 8;
	}
	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((c == '0') && ((*p == 'x') || (*p == 'X'))) {
			++p;
			c = *p++;
		}
	}

	// If nValue exceeds this, an integer overflow is reported.
	#if (EA_PLATFORM_WORD_SIZE >= 8)
		const uint64_t nMaxValue(UINT64_MAX / nBase);
		const uint64_t nModValue(UINT64_MAX % nBase);
	#else
		// 32 bit platforms are very slow at doing 64 bit div and mod operations.
		uint64_t nMaxValue;
		uint64_t nModValue;

		switch(nBase)
		{
			case 2:
				nMaxValue = UINT64_C(9223372036854775807);
				nModValue = 1;
				break;
			case 8:
				nMaxValue = UINT64_C(2305843009213693951);
				nModValue = 7;
				break;
			case 10:
				nMaxValue = UINT64_C(1844674407370955161);
				nModValue = 5;
				break;
			case 16:
				nMaxValue = UINT64_C(1152921504606846975);
				nModValue = 15;
				break;
			default:
				nMaxValue = (UINT64_MAX / nBase);
				nModValue = (UINT64_MAX % nBase);
				break;
		}
	#endif

	for(unsigned nCurrentDigit; ;){
		if(Isdigit(c))
			nCurrentDigit = (unsigned)(c - '0');
		else if(Isalpha(c))
			nCurrentDigit = (unsigned)(Toupper(c) - 'A' + 10);
		else
			break; // The digit is invalid.

		if(nCurrentDigit >= (unsigned)nBase)
			break; // The digit is invalid.

		bDigitWasRead = true;

		// Check for overflow.
		if((nValue < nMaxValue) || ((nValue == nMaxValue) && ((uint64_t)nCurrentDigit <= nModValue)))
			nValue = (nValue * nBase) + nCurrentDigit;
		else
			bOverflowOccurred = true; // Set the flag, but continue processing.

		c = *p++;
	}

	--p; // Go back to the last character

	if(!bDigitWasRead){
		if(ppEnd)
			p = pValue; // We'll assign 'ppEnd' below.
	}                                                                                  // INT64_MAX + 1 is the same thing as -INT64_MIN with most compilers.
	else if(bOverflowOccurred || (!bUnsigned && (((chSign == '-') && (nValue > ((uint64_t)INT64_MAX + 1))) || ((chSign == '+') && (nValue > (uint64_t)INT64_MAX))))){
		// Integer overflow occurred.
		if(bUnsigned)
			nValue = UINT64_MAX;
		else if(chSign == '-')
			nValue = (uint64_t)INT64_MAX + 1; // INT64_MAX + 1 is the same thing as -INT64_MIN with most compilers.
		else
			nValue = INT64_MAX;

		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoU64Common: Range underflow or overflow.");}
		errno = ERANGE; // The standard specifies that we set this value.
	}

	if(ppEnd)
		*ppEnd = (char16_t*)p;

	if(chSign == '-')
		nValue = -nValue;

	return nValue;
}

static uint64_t StrtoU64Common(const char32_t* pValue, char32_t** ppEnd, int nBase, bool bUnsigned)
{
	uint64_t        nValue(0);                 // Current value
	const char32_t* p = pValue;                // Current position
	char32_t        c;                         // Temp value
	char32_t        chSign('+');               // One of either '+' or '-'
	bool            bDigitWasRead(false);      // True if any digits were read.
	bool            bOverflowOccurred(false);  // True if integer overflow occurred.

	// Skip leading whitespace
	c = *p++;
	while(Isspace(c))
		c = *p++;

	// Check for sign.
	if((c == '-') || (c == '+')){
		chSign = c;
		c = *p++;
	}

	// Do checks on nBase.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (char32_t*)pValue;
		return 0;
	}
	else if(nBase == 0){
		// Auto detect one of base 8, 10, or 32. 
		if(c != '0')
			nBase = 10;
		else if(*p == 'x' || *p == 'X')
			nBase = 32;
		else
			nBase = 8;
	}
	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((c == '0') && ((*p == 'x') || (*p == 'X'))) {
			++p;
			c = *p++;
		}
	}

	// If nValue exceeds this, an integer overflow is reported.
	#if (EA_PLATFORM_WORD_SIZE >= 8)
		const uint64_t nMaxValue(UINT64_MAX / nBase);
		const uint64_t nModValue(UINT64_MAX % nBase);
	#else
		// 32 bit platforms are very slow at doing 64 bit div and mod operations.
		uint64_t nMaxValue;
		uint64_t nModValue;

		switch(nBase)
		{
			case 2:
				nMaxValue = UINT64_C(9223372036854775807);
				nModValue = 1;
				break;
			case 8:
				nMaxValue = UINT64_C(2305843009213693951);
				nModValue = 7;
				break;
			case 10:
				nMaxValue = UINT64_C(1844674407370955161);
				nModValue = 5;
				break;
			case 16:
				nMaxValue = UINT64_C(1152921504606846975);
				nModValue = 15;
				break;
			default:
				nMaxValue = (UINT64_MAX / nBase);
				nModValue = (UINT64_MAX % nBase);
				break;
		}
	#endif

	for(unsigned nCurrentDigit; ;){
		if(Isdigit(c))
			nCurrentDigit = (unsigned)(c - '0');
		else if(Isalpha(c))
			nCurrentDigit = (unsigned)(Toupper(c) - 'A' + 10);
		else
			break; // The digit is invalid.

		if(nCurrentDigit >= (unsigned)nBase)
			break; // The digit is invalid.

		bDigitWasRead = true;

		// Check for overflow.
		if((nValue < nMaxValue) || ((nValue == nMaxValue) && ((uint64_t)nCurrentDigit <= nModValue)))
			nValue = (nValue * nBase) + nCurrentDigit;
		else
			bOverflowOccurred = true; // Set the flag, but continue processing.

		c = *p++;
	}

	--p; // Go back to the last character

	if(!bDigitWasRead){
		if(ppEnd)
			p = pValue; // We'll assign 'ppEnd' below.
	}                                                                                  // INT64_MAX + 1 is the same thing as -INT64_MIN with most compilers.
	else if(bOverflowOccurred || (!bUnsigned && (((chSign == '-') && (nValue > ((uint64_t)INT64_MAX + 1))) || ((chSign == '+') && (nValue > (uint64_t)INT64_MAX))))){
		// Integer overflow occurred.
		if(bUnsigned)
			nValue = UINT64_MAX;
		else if(chSign == '-')
			nValue = (uint64_t)INT64_MAX + 1; // INT64_MAX + 1 is the same thing as -INT64_MIN with most compilers.
		else
			nValue = INT64_MAX;

		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoU64Common: Range underflow or overflow.");}
		errno = ERANGE; // The standard specifies that we set this value.
	}

	if(ppEnd)
		*ppEnd = (char32_t*)p;

	if(chSign == '-')
		nValue = -nValue;

	return nValue;
}



EASTDC_API int32_t StrtoI32(const char* pValue, char** ppEnd, int nBase)
{
	int64_t val = (int64_t) StrtoU64Common(pValue, ppEnd, nBase, false);

	if(val < INT32_MIN)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range underflow. You may need to use StrtoI64 instead."); }
		errno = ERANGE;
		return (int32_t)INT32_MIN;
	}

	if(val > INT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range overflow. You may need to use StrtoU32 or StrtoU64 instead."); }
		errno = ERANGE;
		return INT32_MAX;
	}
	
	return (int32_t) val;
}

EASTDC_API int32_t StrtoI32(const char16_t* pValue, char16_t** ppEnd, int nBase)
{
	int64_t val = (int64_t) StrtoU64Common(pValue, ppEnd, nBase, false);

	if(val < INT32_MIN)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range underflow. You may need to use StrtoI64 instead."); }
		errno = ERANGE;
		return (int32_t)INT32_MIN;
	}

	if(val > INT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range overflow. You may need to use StrtoU32 or StrtoU64 instead."); }
		errno = ERANGE;
		return INT32_MAX;
	}
	
	return (int32_t) val;
}

EASTDC_API int32_t StrtoI32(const char32_t* pValue, char32_t** ppEnd, int nBase)
{
	int64_t val = (int64_t) StrtoU64Common(pValue, ppEnd, nBase, false);

	if(val < INT32_MIN)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range underflow. You may need to use StrtoI64 instead."); }
		errno = ERANGE;
		return (int32_t)INT32_MIN;
	}

	if(val > INT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoI32: Range overflow. You may need to use StrtoU32 or StrtoU64 instead."); }
		errno = ERANGE;
		return INT32_MAX;
	}
	
	return (int32_t) val;
}




EASTDC_API uint32_t StrtoU32(const char* pValue, char** ppEnd, int nBase)
{
	uint64_t val = StrtoU64Common(pValue, ppEnd, nBase, true);

	if(val > UINT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoU32: Range overflow. You may need to use StrtoU64 instead."); }
		errno = ERANGE;
		return UINT32_MAX;
	}

	return (uint32_t)val;
}

EASTDC_API uint32_t StrtoU32(const char16_t* pValue, char16_t** ppEnd, int nBase)
{
	uint64_t val = StrtoU64Common(pValue, ppEnd, nBase, true);

	if(val > UINT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoU32: Range overflow. You may need to use StrtoU64 instead."); }
		errno = ERANGE;
		return UINT32_MAX;
	}

	return (uint32_t)val;
}

EASTDC_API uint32_t StrtoU32(const char32_t* pValue, char32_t** ppEnd, int nBase)
{
	uint64_t val = StrtoU64Common(pValue, ppEnd, nBase, true);

	if(val > UINT32_MAX)
	{
		if(EA::StdC::GetAssertionsEnabled())
			{ EA_FAIL_MSG("StrtoU32: Range overflow. You may need to use StrtoU64 instead."); }
		errno = ERANGE;
		return UINT32_MAX;
	}

	return (uint32_t)val;
}




EASTDC_API int64_t StrtoI64(const char* pString, char** ppStringEnd, int nBase)
{
	return (int64_t)StrtoU64Common(pString, ppStringEnd, nBase, false);
}

EASTDC_API int64_t StrtoI64(const char16_t* pString, char16_t** ppStringEnd, int nBase)
{
	return (int64_t)StrtoU64Common(pString, ppStringEnd, nBase, false);
}

EASTDC_API int64_t StrtoI64(const char32_t* pString, char32_t** ppStringEnd, int nBase)
{
	return (int64_t)StrtoU64Common(pString, ppStringEnd, nBase, false);
}



EASTDC_API uint64_t StrtoU64(const char* pString, char** ppStringEnd, int nBase)
{
	return StrtoU64Common(pString, ppStringEnd, nBase, true);
}

EASTDC_API uint64_t StrtoU64(const char16_t* pString, char16_t** ppStringEnd, int nBase)
{
	return StrtoU64Common(pString, ppStringEnd, nBase, true);
}

EASTDC_API uint64_t StrtoU64(const char32_t* pString, char32_t** ppStringEnd, int nBase)
{
	return StrtoU64Common(pString, ppStringEnd, nBase, true);
}



EASTDC_API char* FtoaEnglish(double dValue, char* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
{
	// Note that this function is a duplicate of FtoaEnglish16 but 
	// with char instead of char16_t. Modifications to either of 
	// these functions should be replicated to the other.

	int nDecimalPosition, nSign;
	int nPositionResult(0);
	int nPositionTemp(0);
	int i;
	int nExponent;

	if(nResultCapacity <= 0)
		return NULL;

	if(bExponentEnabled){
		if(dValue == 0.0)
			nExponent = 0;
		else
		{
			const double dValueAbs = fabs(dValue);
			const double dValueLog = ::log10(dValueAbs);
			nExponent = (int)::floor(dValueLog);
		}

		if((nExponent >= nPrecision) || (nExponent < -4)){    // printf's %g switches to exponential whenever exp >= precision || exp < -4.
			// Compute how many digits we need for the exponent.
			int nDigits = 1;
			int nLimit  = 10;

			while(nLimit <= nExponent){
				nLimit *= 10;
				++nDigits;
			}

			const double dExpPow = ::pow(10.0, (double)-nExponent);

			if(FtoaEnglish(dValue * dExpPow, pResult, nResultCapacity - nDigits - 2, nPrecision, false)){
				char* p = pResult + Strlen(pResult);

				*p++ = (char)'e';
				*p++ = ((nExponent < 0) ? (char)'-' : (char)'+');
				I32toa(abs(nExponent), p, 10);
				return pResult;
			}
			return NULL;
		}
	}

	// fcvt is a function that converts a floating point value to its component
	// string, sign, and decimal position. It doesn't convert it to a fully
	// finished string because sign and decimal usage usually varies between
	// locales and this function is trying to be locale-independent. It is up
	// to the user of this function to present the final data in a form that 
	// is locale-savvy. Actually, not all compilers implement fcvt.
	#if EASTDC_NATIVE_FCVT
		#ifdef __GNUC__   // nPrecision refers to the number of digits after the decimal point.
		const char* const pResultTemp =  fcvt(dValue, nPrecision, &nDecimalPosition, &nSign);
		#else
		char pResultTemp[_CVTBUFSIZE+1];
		_fcvt_s(pResultTemp, sizeof(pResultTemp), dValue, nPrecision, &nDecimalPosition, &nSign);
		#endif
	#else
	   char bufferTemp[kFcvtBufMaxSize];
	   const char* const pResultTemp = FcvtBuf(dValue, nPrecision, &nDecimalPosition, &nSign, bufferTemp);
	#endif

	// If the value is negative, then add a leading '-' sign.
	if(nSign){
		if(nPositionResult >= nResultCapacity){
			pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
			return NULL;
		}
		pResult[nPositionResult] = '-';
		nPositionResult++;
	}

	// If the value is < 1, then add a leading '0' digit.
	if(fabs(dValue) < 1.0){ 
	  #if EASTDC_NATIVE_FCVT && defined(__GNUC__)
		// GCC's fcvt has a quirk: If the input dValue is 0 (but no other value, fractional or not),
		// it yields an output string with a leading "0." So we need to make a special case to 
		// detect this here.
		if(dValue != 0.0)
	  #endif
		{
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '0';
		}
	}

	// Read digits up to the decimal position and write them to the output string.
	if(nDecimalPosition > 0){      // If the input was something like 1000.0
		for(i = 0; (i < nDecimalPosition) && pResultTemp[nPositionTemp]; i++){
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = pResultTemp[nPositionTemp++];
		}
	}

	if(pResultTemp[nPositionTemp]){
		// Find the last of the zeroes in the pResultTemp string. We don't want
		// to add unnecessary trailing zeroes to the returned string and don't
		// want to return a decimal point in the string if it isn't necessary.
		int nFirstTrailingZeroPosition(nPositionTemp);
		int nLastPositionTemp(nPositionTemp);

		while(pResultTemp[nLastPositionTemp]){
			if(pResultTemp[nLastPositionTemp] != '0')
				nFirstTrailingZeroPosition = nLastPositionTemp + 1;
			nLastPositionTemp++;
		}

		// If there is any reason to write a decimal point, then we write 
		// it and write the data that comes after it.
		if((nFirstTrailingZeroPosition > nPositionTemp) && (nPrecision > 0)){
			// Add a decimal point.
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '.';

			if(nDecimalPosition < 0){ // If there are zeroes after the decimal...
				for(i = nDecimalPosition; i < 0; i++){
					if(nPositionResult >= nResultCapacity){
						pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
						return NULL;
					}
					pResult[nPositionResult++] = '0';
					--nPrecision; 
				}
			}

			// Read digits after the decimal position and write them to the output string.
			for(i = 0; (i < nPrecision) && (nPositionTemp < nFirstTrailingZeroPosition) && pResultTemp[nPositionTemp]; i++){
				if(nPositionResult >= nResultCapacity){
					//What we do here is possibly erase trailing zeroes that we've written after the decimal.
					int nEndPosition = EASTDC_MAX(nPositionResult - 1, 0);
					pResult[nEndPosition] = 0;
					while((--nEndPosition > 0) && (pResult[nEndPosition] == '0'))
						pResult[nEndPosition] = 0;
					return NULL;
				}
				pResult[nPositionResult++] = pResultTemp[nPositionTemp++];
			}
		}
	}

	// Write the final terminating zero.
	if(nPositionResult >= nResultCapacity){
		pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
		return NULL;
	}
	pResult[nPositionResult] = 0;
	return pResult;
}

EASTDC_API char16_t* FtoaEnglish(double dValue, char16_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
{
	// Note that this function is a duplicate of FtoaEnglish8 but 
	// with char16_t instead of char. Modifications to either of 
	// these functions should be replicated to the other.

	int nDecimalPosition, nSign;
	int nPositionResult(0);
	int nPositionTemp(0);
	int i;
	int nExponent;

	if(nResultCapacity <= 0)
		return NULL;

	if(bExponentEnabled){
		if(dValue == 0.0)
			nExponent = 0;
		else
		{
			const double dValueAbs = fabs(dValue);
			const double dValueLog = ::log10(dValueAbs);
			nExponent = (int)::floor(dValueLog);
		}

		if((nExponent >= nPrecision) || (nExponent < -4)){    // printf's %g switches to exponential whenever exp >= mnPrecisionUsed || exp < -4.
			// Compute how many digits we need for the exponent.
			int nDigits = 1;
			int nLimit  = 10;

			while(nLimit <= nExponent){
				nLimit *= 10;
				++nDigits;
			}

			const double dExpPow = ::pow(10.0, (double)-nExponent);

			if(FtoaEnglish(dValue * dExpPow, pResult, nResultCapacity - nDigits - 2, nPrecision, false)){
				char16_t* p = pResult + Strlen(pResult);

				*p++ = (char16_t)'e';
				*p++ = ((nExponent < 0) ? (char16_t)'-' : (char16_t)'+');
				I32toa(abs(nExponent), p, 10);
				return pResult;
			}
			return NULL;
		}
	}

	// fcvt is a function that converts a floating point value to its component
	// string, sign, and decimal position. It doesn't convert it to a fully
	// finished string because sign and decimal usage usually varies between
	// locales and this function is trying to be locale-independent. It is up
	// to the user of this function to present the final data in a form that 
	// is locale-savvy. Actually, not all compilers implement fcvt.
	#if EASTDC_NATIVE_FCVT
		#ifdef __GNUC__
			const char* const pResultTemp =  fcvt(dValue, nPrecision, &nDecimalPosition, &nSign);
		#else
			const char* const pResultTemp = _fcvt(dValue, nPrecision, &nDecimalPosition, &nSign);
		#endif
	#else
	   char bufferTemp[kFcvtBufMaxSize];
	   const char* const pResultTemp = FcvtBuf(dValue, nPrecision, &nDecimalPosition, &nSign, bufferTemp);
	#endif

	// If the value is negative, then add a leading '-' sign.
	if(nSign){
		if(nPositionResult >= nResultCapacity){
			pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
			return NULL;
		}
		pResult[nPositionResult] = '-';
		nPositionResult++;
	}

	// If the value is < 1, then add a leading '0' digit.
	if(fabs(dValue) < 1.0){ 
	  #if EASTDC_NATIVE_FCVT && defined(__GNUC__)
		// GCC's fcvt has a quirk: If the input dValue is 0 (but no other value, fractional or not),
		// it yields an output string with a leading "0." So we need to make a special case to 
		// detect this here.
		if(dValue != 0.0)
	  #endif
		{
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '0';
		}
	}

	// Read digits up to the decimal position and write them to the output string.
	if(nDecimalPosition > 0){      // If the input was something like 1000.0
		for(i = 0; (i < nDecimalPosition) && pResultTemp[nPositionTemp]; i++){
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = (char16_t)pResultTemp[nPositionTemp++];
		}
	}

	if(pResultTemp[nPositionTemp]){
		// Find the last of the zeroes in the pResultTemp string. We don't want
		// to add unnecessary trailing zeroes to the returned string and don't
		// want to return a decimal point in the string if it isn't necessary.
		int nFirstTrailingZeroPosition(nPositionTemp);
		int nLastPositionTemp(nPositionTemp);

		while(pResultTemp[nLastPositionTemp]){
			if(pResultTemp[nLastPositionTemp] != '0')
				nFirstTrailingZeroPosition = nLastPositionTemp + 1;
			nLastPositionTemp++;
		}

		// If there is any reason to write a decimal point, then we write 
		// it and write the data that comes after it.
		if((nFirstTrailingZeroPosition > nPositionTemp) && (nPrecision > 0)){
			// Add a decimal point.
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '.';

			if(nDecimalPosition < 0){ // If there are zeroes after the decimal...
				for(i = nDecimalPosition; i < 0; i++){
					if(nPositionResult >= nResultCapacity){
						pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
						return NULL;
					}
					pResult[nPositionResult++] = '0';
					--nPrecision; 
				}
			}

			// Read digits after the decimal position and write them to the output string.
			for(i = 0; (i < nPrecision) && (nPositionTemp < nFirstTrailingZeroPosition) && pResultTemp[nPositionTemp]; i++){
				if(nPositionResult >= nResultCapacity){
					//What we do here is possibly erase trailing zeroes that we've written after the decimal.
					int nEndPosition = EASTDC_MAX(nPositionResult - 1, 0);
					pResult[nEndPosition] = 0;
					while((--nEndPosition > 0) && (pResult[nEndPosition] == '0'))
						pResult[nEndPosition] = 0;
					return NULL;
				}
				pResult[nPositionResult++] = (char16_t)pResultTemp[nPositionTemp++];
			}
		}
	}

	// Write the final terminating zero.
	if(nPositionResult >= nResultCapacity){
		pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
		return NULL;
	}
	pResult[nPositionResult] = 0;
	return pResult;
}

EASTDC_API char32_t* FtoaEnglish(double dValue, char32_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
{
	// Note that this function is a duplicate of FtoaEnglish8 but 
	// with char32_t instead of char. Modifications to either of 
	// these functions should be replicated to the other.

	int nDecimalPosition, nSign;
	int nPositionResult(0);
	int nPositionTemp(0);
	int i;
	int nExponent;

	if(nResultCapacity <= 0)
		return NULL;

	if(bExponentEnabled){
		if(dValue == 0.0)
			nExponent = 0;
		else
		{
			const double dValueAbs = fabs(dValue);
			const double dValueLog = ::log10(dValueAbs);
			nExponent = (int)::floor(dValueLog);
		}

		if((nExponent >= nPrecision) || (nExponent < -4)){    // printf's %g switches to exponential whenever exp >= mnPrecisionUsed || exp < -4.
			// Compute how many digits we need for the exponent.
			int nDigits = 1;
			int nLimit  = 10;

			while(nLimit <= nExponent){
				nLimit *= 10;
				++nDigits;
			}

			const double dExpPow = ::pow(10.0, (double)-nExponent);

			if(FtoaEnglish(dValue * dExpPow, pResult, nResultCapacity - nDigits - 2, nPrecision, false)){
				char32_t* p = pResult + Strlen(pResult);

				*p++ = (char32_t)'e';
				*p++ = ((nExponent < 0) ? (char32_t)'-' : (char32_t)'+');
				I32toa(abs(nExponent), p, 10);
				return pResult;
			}
			return NULL;
		}
	}

	// fcvt is a function that converts a floating point value to its component
	// string, sign, and decimal position. It doesn't convert it to a fully
	// finished string because sign and decimal usage usually varies between
	// locales and this function is trying to be locale-independent. It is up
	// to the user of this function to present the final data in a form that 
	// is locale-savvy. Actually, not all compilers implement fcvt.
	#if EASTDC_NATIVE_FCVT
		#ifdef __GNUC__
			const char* const pResultTemp =  fcvt(dValue, nPrecision, &nDecimalPosition, &nSign);
		#else
			const char* const pResultTemp = _fcvt(dValue, nPrecision, &nDecimalPosition, &nSign);
		#endif
	#else
	   char bufferTemp[kFcvtBufMaxSize];
	   const char* const pResultTemp = FcvtBuf(dValue, nPrecision, &nDecimalPosition, &nSign, bufferTemp);
	#endif

	// If the value is negative, then add a leading '-' sign.
	if(nSign){
		if(nPositionResult >= nResultCapacity){
			pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
			return NULL;
		}
		pResult[nPositionResult] = '-';
		nPositionResult++;
	}

	// If the value is < 1, then add a leading '0' digit.
	if(fabs(dValue) < 1.0){ 
	  #if EASTDC_NATIVE_FCVT && defined(__GNUC__)
		// GCC's fcvt has a quirk: If the input dValue is 0 (but no other value, fractional or not),
		// it yields an output string with a leading "0." So we need to make a special case to 
		// detect this here.
		if(dValue != 0.0)
	  #endif
		{
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '0';
		}
	}

	// Read digits up to the decimal position and write them to the output string.
	if(nDecimalPosition > 0){      // If the input was something like 1000.0
		for(i = 0; (i < nDecimalPosition) && pResultTemp[nPositionTemp]; i++){
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = (char32_t)pResultTemp[nPositionTemp++];
		}
	}

	if(pResultTemp[nPositionTemp]){
		// Find the last of the zeroes in the pResultTemp string. We don't want
		// to add unnecessary trailing zeroes to the returned string and don't
		// want to return a decimal point in the string if it isn't necessary.
		int nFirstTrailingZeroPosition(nPositionTemp);
		int nLastPositionTemp(nPositionTemp);

		while(pResultTemp[nLastPositionTemp]){
			if(pResultTemp[nLastPositionTemp] != '0')
				nFirstTrailingZeroPosition = nLastPositionTemp + 1;
			nLastPositionTemp++;
		}

		// If there is any reason to write a decimal point, then we write 
		// it and write the data that comes after it.
		if((nFirstTrailingZeroPosition > nPositionTemp) && (nPrecision > 0)){
			// Add a decimal point.
			if(nPositionResult >= nResultCapacity){
				pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
				return NULL;
			}
			pResult[nPositionResult++] = '.';

			if(nDecimalPosition < 0){ // If there are zeroes after the decimal...
				for(i = nDecimalPosition; i < 0; i++){
					if(nPositionResult >= nResultCapacity){
						pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
						return NULL;
					}
					pResult[nPositionResult++] = '0';
					--nPrecision; 
				}
			}

			// Read digits after the decimal position and write them to the output string.
			for(i = 0; (i < nPrecision) && (nPositionTemp < nFirstTrailingZeroPosition) && pResultTemp[nPositionTemp]; i++){
				if(nPositionResult >= nResultCapacity){
					//What we do here is possibly erase trailing zeroes that we've written after the decimal.
					int nEndPosition = EASTDC_MAX(nPositionResult - 1, 0);
					pResult[nEndPosition] = 0;
					while((--nEndPosition > 0) && (pResult[nEndPosition] == '0'))
						pResult[nEndPosition] = 0;
					return NULL;
				}
				pResult[nPositionResult++] = (char32_t)pResultTemp[nPositionTemp++];
			}
		}
	}

	// Write the final terminating zero.
	if(nPositionResult >= nResultCapacity){
		pResult[EASTDC_MAX(nPositionResult - 1, 0)] = 0;
		return NULL;
	}
	pResult[nPositionResult] = 0;
	return pResult;
}




EASTDC_API size_t ReduceFloatString(char* pString, size_t nLength)
{
	if(nLength == (size_t)-1)
		nLength = strlen(pString);

	size_t nNewLength(nLength);

	if(nLength > 0)
	{
		// Get the decimal index and exponent index. We won't chop off any zeros 
		// unless they are after the decimal position and before an exponent position.
		int nDecimalIndex  = -1;
		int nExponentIndex = -1;
		int nCurrentIndex  =  0;

		while(nCurrentIndex < (int)nLength)
		{
			if(pString[nCurrentIndex] == '.')
				nDecimalIndex = nCurrentIndex;
			if((pString[nCurrentIndex] == 'e') || (pString[nCurrentIndex] == 'E'))
				nExponentIndex = nCurrentIndex;
			nCurrentIndex++;
		}

		// Now we need to go to the end of the string and walk backwards to 
		// find any contiguous zero digits after a decimal point.
		if(nDecimalIndex >= 0) // If there is any decimal point...
		{
			const int nFirstDigitToCheck(nDecimalIndex + 1);
			const int nLastDigitToCheck ((nExponentIndex >= 0) ? (nExponentIndex - 1) : (int)(nLength - 1));

			nCurrentIndex = nLastDigitToCheck;

			while(nCurrentIndex >= nFirstDigitToCheck)
			{
				// assert((pString[nCurrentIndex] >= '0') && (pString[nCurrentIndex] <= '9'));

				if(pString[nCurrentIndex] == '0')
				{
					// Copy the string downward. Note that we copy the trailing 
					// terminator of the string as well.
					for(int i = nCurrentIndex; i < (int)nNewLength; i++)
						pString[i] = pString[i + 1]; // Copy the string downward.
					nNewLength--;
				}
				else
					break;
				nCurrentIndex--;
			}
		}
		else
		{
			// If the string is all zeroes, convert it to just one zero.
			size_t i;

			for(i = 0; (i < nLength) && (pString[i] == '0'); i++)
				{ } // Do nothing.

			if(i == nLength)
				nLength = 0; // And fall through to the code below.
		}

		// It is possible that the input string was "000", in which case the above code would 
		// erase the entire string. Here we simply make a string of "0" and return it.
		if(nLength == 0)
		{
			pString[0] = '0';
			pString[1] =  0;
			nNewLength =  1;
		}
		else
		{
			// We may have a number such as "234.", in which case we remove the trailing decimal.
			if((nDecimalIndex >= 0) && (nDecimalIndex == ((int)(unsigned)nNewLength - 1)))
			{
				pString[nDecimalIndex] = 0;
				nNewLength--;
			}

			size_t i;

			// It is also posible that we now have a string like "0." or "000." or just ".".
			// In this case, we simply set the string to "0".
			for(i = 0; i < nNewLength; i++)
			{
				if((pString[i] != '0') && (pString[i] != '.'))
					break;
			}

			if(i == nNewLength) // If the string was all zeros...
			{
				pString[0] = '0';
				pString[1] = 0;
				nNewLength = 1;
			}

			if((nNewLength >= 3) && (pString[0] == '0') && (pString[1] == '.'))   // If we have "0.x"
			{
				memmove(pString, pString + 1, nNewLength * sizeof(char));
				nNewLength--;
			}
		}
	}

	return nNewLength;
}

EASTDC_API size_t ReduceFloatString(char16_t* pString, size_t nLength)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBuffer8[64];
	char*  pCurrent8;
	char16_t* pCurrent16;
	size_t    n = 0;

	if(nLength < 63)
		nLength = 63;

	for(pCurrent8 = pBuffer8, pCurrent16 = pString; *pCurrent16 && (n < nLength); ++n) // Do a 16 bit to 8 bit strcpy.
		*pCurrent8++ = (char)(unsigned char)*pCurrent16++;
	
	*pCurrent8 = 0;

	n = ReduceFloatString(pBuffer8, n);

	for(pCurrent8 = pBuffer8, pCurrent16 = pString; *pCurrent8; ) // Do a 8 bit to 16 bit strcpy.
		*pCurrent16++ = (char16_t)(unsigned char)*pCurrent8++;

	*pCurrent16 = 0;

	return n;
}

EASTDC_API size_t ReduceFloatString(char32_t* pString, size_t nLength)
{
	// We implement this by calling the 8 bit version and copying its data.
	char   pBuffer8[64];
	char*  pCurrent8;
	char32_t* pCurrent32;
	size_t    n = 0;

	if(nLength < 63)
		nLength = 63;

	for(pCurrent8 = pBuffer8, pCurrent32 = pString; *pCurrent32 && (n < nLength); ++n) // Do a 32 bit to 8 bit strcpy.
		*pCurrent8++ = (char)(unsigned char)*pCurrent32++;
	
	*pCurrent8 = 0;

	n = ReduceFloatString(pBuffer8, n);

	for(pCurrent8 = pBuffer8, pCurrent32 = pString; *pCurrent8; ) // Do a 8 bit to 32 bit strcpy.
		*pCurrent32++ = (char32_t)(unsigned char)*pCurrent8++;

	*pCurrent32 = 0;

	return n;
}


} // namespace StdC
} // namespace EA


#undef EASTDC_MIN
#undef EASTDC_MAX


#if defined(EA_COMPILER_GNUC) && (EA_COMPILER_VERSION >= 4007)
	EA_RESTORE_GCC_WARNING()
#endif
EA_RESTORE_VC_WARNING()



