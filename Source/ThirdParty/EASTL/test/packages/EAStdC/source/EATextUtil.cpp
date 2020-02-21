///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EATextUtil.h>
#include <EAStdC/EAString.h>



/////////////////////////////////////////////////////////////////////////////
// EATEXTUTIL_MIN / EATEXTUTIL_MAX
//
#define EATEXTUTIL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define EATEXTUTIL_MAX(a, b) ((a) > (b) ? (a) : (b))


namespace EA
{
namespace StdC
{


extern uint8_t utf8lengthTable[256];


///////////////////////////////////////////////////////////////////////////////
// UTF8Validate
//
// There are multiple definitions of what a valid UTF8 string is. UTF8 allows
// the ability to encode the same UTF16 character in multiple ways. This in
// one sense is a legal UTF8 array. However, for some security reasons it is
// sometimes considered that a UTF8 array is illegal (or at least 'unsafe')
// if it encodes some character with more bytes than needed. Actually the
// Unicode standard v3.0 says that these 'insecure' UTF8 sequences are
// formally illegal to generate but not illegal to interpret.
// See "http://www.unicode.org/unicode/uni2errata/UTF-8_Corrigendum.html"
//
// We take the high-security approach here, though it is slower. We could write
// a simpler function that does a non-security check with the simple table
// of info here:
//    0x00-0x7f are single standalone bytes.
//    0xc2-0xFD are first byte of a multi-byte sequence.
//    0xc2-0xdf are first byte of a pair.
//    0xe0-0xef are first byte of a triplet.
//    0x00-0xf7 are first byte of a quadruplet.
//    0xf8-0xfb are first byte of a 5-tuplet.
//    0xfc-0xfd are first byte of a 6-tuplet.
//    0xfe-0xff are invalid bytes anywhere in a UTF8 string.
//    0x80-0xbf are the second-sixth byte of a multi-byte sequence, though not all values are valid for all such bytes.
//
// See 'http://www.cl.cam.ac.uk/~mgk25/unicode.html' or search for "UTF8 FAQ"
// on the Internet for more details on UTF8 and Unicode.
//
EASTDC_API bool UTF8Validate(const char* pText, size_t nLength)
{
	const uint8_t*       pSource8    = (const uint8_t*)pText;
	const uint8_t* const pSource8End = pSource8 + nLength;

	while(pSource8 < pSource8End)
	{
		if(pSource8[0] < 0x80)
			++pSource8;
		else if(pSource8[0] < 0xC2)
			break; // The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
		else if(pSource8[0] < 0xE0) // If 2 input chars result in 1 output char...
		{
			if(pSource8End - pSource8 >= 2)
			{
				if(!((pSource8[1] ^ 0x80) < 0x40))
					break; //The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
				pSource8 += 2;
			}
			else
				break; //The input string is not long enough to finish reading the current character.
		}
		else if(pSource8[0] < 0xF0) // If 3 input chars result in 1 output char...
		{
			if((pSource8End - pSource8) >= 3)
			{
				if(!(((pSource8[1] ^ 0x80) < 0x40) &&
					 ((pSource8[2] ^ 0x80) < 0x40) &&
					  (pSource8[0] >= 0xE1 || pSource8[1] >= 0xA0)))
					break; //The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
				pSource8 += 3;
			}
			else
				break; //The input string is not long enough to finish reading the current character.
		}
		else if(pSource8[0] < 0xF8) // If 4 input chars result in 1 output char...
		{
			if((pSource8End - pSource8) >= 4)
			{
				if(!(((pSource8[1] ^ 0x80) < 0x40) &&
					 ((pSource8[2] ^ 0x80) < 0x40) &&
					 ((pSource8[3] ^ 0x80) < 0x40) &&
					  (pSource8[0] >= 0xF1 || pSource8[1] >= 0x90)))
					break; // The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
				pSource8 += 4;
			}
			else
				break; //The input string is not long enough to finish reading the current character.
		}
		else if(pSource8[0] < 0xFC) // If 5 input chars result in 1 output char...
		{
			if((pSource8End - pSource8) >= 5)
			{
				if(!(((pSource8[1] ^ 0x80) < 0x40) &&
					 ((pSource8[2] ^ 0x80) < 0x40) &&
					 ((pSource8[3] ^ 0x80) < 0x40) &&
					 ((pSource8[4] ^ 0x80) < 0x40) &&
					  (pSource8[0] >= 0xf9 || pSource8[1] >= 0x88)))
					break; //The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
				pSource8 += 5;
			}
			else
				break; //The input string is not long enough to finish reading the current character.
		}
		else if(pSource8[0] < 0xFE) // If 6 input chars result in 1 output char...
		{
			if((pSource8End - pSource8) >= 6)
			{
				if(!(((pSource8[1] ^ 0x80) < 0x40) &&
					 ((pSource8[2] ^ 0x80) < 0x40) &&
					 ((pSource8[3] ^ 0x80) < 0x40) &&
					 ((pSource8[4] ^ 0x80) < 0x40) &&
					 ((pSource8[5] ^ 0x80) < 0x40) &&
					  (pSource8[0] >= 0xfd || pSource8[1] >= 0x84)))
					break; //The character is invalid. It is important that we check for this because various security issues potentially arise if we don't.
				pSource8 += 6;
			}
			else
				break; //The input string is not long enough to finish reading the current character.
		}
		else //Else the current input char is invalid.
			break;
	}

	return (pSource8 == pSource8End); // The return value is OK if we successfully processed all characters.
}


// Returns the pointer p incremented by n multibyte characters.
// The string must be a valid UTF8 string or else the behavior is undefined.
// If the string is not known to be valid, then it should be first validated independently
// or a validating version of this function should be used instead.
EASTDC_API char* UTF8Increment(const char* p, size_t n)
{
	while(n--)
	{
		// To do: Change this code to instead use the utf8lengthTable fropm EAString.cpp

		const int c = (uint8_t)*p;

		if     (c <= 0xc1)    // Actually, any value greater than 0x80 and less than 0xc2 is an invalid leading UTF8 char.
			p += 1;
		else if(c <= 0xdf) 
			p += 2;
		else if(c <= 0xef)
			p += 3;
		else if(c <= 0xf7)
			p += 4;
		else if(c <= 0xfb)
			p += 5;
		else if(c <= 0xfd)
			p += 6;
		else
			p += 1;           // Error. We return 1 instead of 0 or -1 because the user is probably iterating a string and so this is safer.
	}

	return (char*)p;
}


// Returns the pointer p decremented by n multibyte characters.
// The string must be decrementable by the given number of characters or else 
// the behavior becomes undefined.
// The string must be a valid UTF8 string or else the behavior is undefined.
// If the string is not known to be valid, then it should be first validated independently
// or a validating version of this function should be used instead.
EASTDC_API char* UTF8Decrement(const char* p, size_t n)
{
	while(n)
	{
		if(!UTF8IsFollowByte(*--p))
			--n;
	}

	return (char*)p;
}


// Returns number of Unicode characters are in the UTF8-encoded string.
// Return value will be <= Strlen(pString).
// The string p must be 0-terminated or the behavior of this function is undefined.
// The string must be a valid UTF8 string or else the behavior is undefined.
// If the string is not known to be valid, then it should be first validated independently
// or a validating version of this function should be used instead.
EASTDC_API size_t UTF8Length(const char* p)
{
	size_t n = 0;

	while(*p)
	{
		if((*p & 0xc0) != 0x80) // If this is a leading char...
			++n;
		++p;
	}

	return n;
}


// Returns number of characters that would be in a UTF8-encoded string.
// Return value will be >= Strlen(pString).
// The string p must be 0-terminated or the behavior of this function is undefined.
EASTDC_API size_t UTF8Length(const char16_t* p)
{
	size_t   n = 0;
	uint32_t c;

	while((c = *p++) != 0)
	{
		if(c < 0x00000080)
			n += 1;
		else if(c < 0x00000800)
			n += 2;
		else // if(c < 0x00010000) 
			n += 3;
	}

	return n;
}


// Returns number of characters that would be in a UTF8-encoded string.
// Return value will be >= Strlen(pString).
// The string p must be 0-terminated or the behavior of this function is undefined.
// Assumes the input values are valid, else the return value will be wrong.
EASTDC_API size_t UTF8Length(const char32_t* p)
{
	size_t   n = 0;
	uint32_t c;

	while((c = (uint32_t)*p++) != 0)
	{
		if(c < 0x00000080)
			n += 1;
		else if(c < 0x00000800)
			n += 2;
		else if(c < 0x00010000) 
			n += 3;
		else if(c < 0x00200000)
			n += 4;
		else if(c < 0x04000000)
			n += 5;
		else if(c <= 0x7fffffff)
			n += 6;
		else
			n += 1;  // Error
	}

	return n;
}

///////////////////////////////////////////////////////////////////////////////
// UTF8CharSize
//
// Returns the byte length of the UTF8 multibyte char pointed to by p.
// The input p must point to the beginning of a UTF8 multibyte sequence, 
// else the return value is 1.
//
// 0x00-0x80 are single bytes.
// 0x81-0xc1 are invalid values for a leading UTF8 char.
// 0xc2-0xdf are first byte of a pair.
// 0xe0-0xef are first byte of a triplet.
// 0xf0-0xf7 are first byte of a quadruplet.
// 0xf8-0xfb are first byte of a 5-tuplet.
// 0xfc-0xfd are first byte of a 6-tuplet.
// 0xfe-0xff are invalid values for a leading UTF8 char.
//
EASTDC_API size_t UTF8CharSize(const char* p)
{
	// To do: Change this code to instead use the utf8lengthTable fropm EAString.cpp

	const int c = (uint8_t)*p;

	if     (c <= 0xc1)    // Any value greater than 0x80 and less than 0xc2 is an invalid leading UTF8 char.
		return 1;
	else if(c <= 0xdf)
		return 2;
	else if(c <= 0xef)
		return 3;
	else if(c <= 0xf7)    // This refers to a unicode point > char16_t
		return 4;
	else if(c <= 0xfb)    // This refers to a unicode point > char16_t
		return 5;
	else if(c <= 0xfd)    // This refers to a unicode point > char16_t
		return 6;

	return 1; // Error. We return 1 instead of 0 or -1 because the user is probably iterating a string and so this is safer.
}


EASTDC_API size_t UTF8CharSize(char16_t c)
{
	if(c < 0x00000080)
		return 1;
	else if(c < 0x00000800)
		return 2;
	else // if(c < 0x00010000) 
		return 3;

	// The following would be used if the input was 32 bit instead of 16 bit.
	//else if(c < 0x00010000)
	//    return 3;
	//else if(c < 0x00200000)
	//    return 4;
	//else if(c < 0x04000000)
	//    return 5;
	//else if(c <= 0x7fffffff)
	//    return 6;
	//
	//return 1; // Error
}


EASTDC_API size_t UTF8CharSize(char32_t c)
{
	if((uint32_t)c < 0x00000080)
		return 1;
	else if((uint32_t)c < 0x00000800)
		return 2;
	else if((uint32_t)c < 0x00010000)
		return 3;
	else if((uint32_t)c < 0x00200000)
		return 4;
	else if((uint32_t)c < 0x04000000)
		return 5;
	else if((uint32_t)c < 0x80000000)
		return 6;
	
	return 1; // Error
}


EASTDC_API char16_t UTF8ReadChar(const char* p, const char** ppEnd)
{
	char16_t        c = 0;
	const char*  pCurrent;
	uint8_t         cChar0((uint8_t)*p), cChar1, cChar2, cChar3;

	//assert((cChar0 != 0xFE) && (cChar0 != 0xFF));     //  No byte can contain 0xFE or 0xFF

	if(cChar0 < 0x80)
	{
		c = cChar0;
		pCurrent = p + 1;
	}
	else
	{
		//assert((cChar0 & 0xC0) == 0xC0);              //  The top two bits need to be equal to 1

		if((cChar0 & 0xE0) == 0xC0)
		{
			c = (char16_t)((cChar0 & 0x1F) << 6);

			cChar1 = static_cast<uint8_t>(p[1]);
			//assert((cChar1 & 0xC0) == 0x80);          //  All subsequent code should be b10xxxxxx
			c |= cChar1 & 0x3F;

			//assert(c >= 0x0080 && c < 0x0800);        //  Check that we have the smallest coding

			pCurrent = p + 2;
		}
		else if((cChar0 & 0xF0) == 0xE0)
		{
			c = (char16_t)((cChar0 & 0xF) << 12);

			cChar1 = static_cast<uint8_t>(p[1]);
			//assert((cChar1 & 0xC0) == 0x80);           //  All subsequent code should be b10xxxxxx
			c |= (cChar1 & 0x3F) << 6;

			cChar2 = static_cast<uint8_t>(p[2]);
			//assert((cChar2 & 0xC0) == 0x80);           //  All subsequent code should be b10xxxxxx
			c |= cChar2 & 0x3F;

			//assert(c >= 0x00000800 && c <  0x00010000); //  Check that we have the smallest coding

			pCurrent = p + 3;
		}
		else
		{
			//assert((cChar0 & 0xf8) == 0xf0);          //  We handle the unicode but not UCS-4
			c = (char16_t)((cChar0 & 0x7) << 18);

			cChar1 = static_cast<uint8_t>(p[1]);
			//assert((cChar1 & 0xC0) == 0x80);          //  All subsequent code should be b10xxxxxx
			c |= (char16_t)((cChar1 & 0x3F) << 12);

			cChar2 = static_cast<uint8_t>(p[2]);
			//assert((cChar2 & 0xC0) == 0x80);          //  All subsequent code should be b10xxxxxx
			c |= (cChar2 & 0x3F) << 6;

			cChar3 = static_cast<uint8_t>(p[3]);
			//assert((cChar3 & 0xC0) == 0x80);          //  All subsequent code should be b10xxxxxx
			c |= cChar3 & 0x3F;

			//assert(c >= 0x00010000 && c <= 0x0010FFFF); //  Check that we have the smallest coding, Unicode and not ucs-4

			pCurrent = p + 4;
		}
	}

	if(ppEnd)
		*ppEnd = pCurrent;

	return c;
}


// This function assumes that there is enough space at p to write the char.
// At most three bytes are needed to write a char16_t value and 6 bytes are
// needed to write a char32_t value.
EASTDC_API char* UTF8WriteChar(char* p, char16_t c)
{
	if(c < 0x80)
	{
		*p++ = (char)(uint8_t)c;
	}
	else if(c < 0x0800)
	{
		*p++ = (char)(uint8_t)((c >> 6) | 0xC0);
		*p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	}
	else // if(c < 0x00010000)
	{
		*p++ = (char)(uint8_t)((c >> 12) | 0xE0);
		*p++ = (char)(uint8_t)(((c >> 6) & 0x3F) | 0x80);
		*p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	}
	//else
	//{
	//    *p++ = (char)(uint8_t)((c >> 18) | 0xF0);
	//    *p++ = (char)(uint8_t)(((c >> 12) & 0x3F) | 0x80);
	//    *p++ = (char)(uint8_t)(((c >> 6) & 0x3F) | 0x80);
	//    *p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	//}

	return p;
}

// This function assumes that there is enough space at p to write the char.
// At most three bytes are needed to write a char32_t value and 6 bytes are
// needed to write a char32_t value.
EASTDC_API char* UTF8WriteChar(char* p, char32_t c)
{
	if((uint32_t)c < 0x80)
	{
		*p++ = (char)(uint8_t)c;
	}
	else if((uint32_t)c < 0x0800)
	{
		*p++ = (char)(uint8_t)((c >> 6) | 0xC0);
		*p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	}
	else if((uint32_t)c < 0x00010000)
	{
		*p++ = (char)(uint8_t)((c >> 12) | 0xE0);
		*p++ = (char)(uint8_t)(((c >> 6) & 0x3F) | 0x80);
		*p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	}
	else
	{
		*p++ = (char)(uint8_t)((c >> 18) | 0xF0);
		*p++ = (char)(uint8_t)(((c >> 12) & 0x3F) | 0x80);
		*p++ = (char)(uint8_t)(((c >> 6) & 0x3F) | 0x80);
		*p++ = (char)(uint8_t)((c & 0x3F) | 0x80);
	}

	return p;
}


/// UTF8TrimPartialChar
///
/// Trim the string to the last valid UTF8 character. This function has no effect on a UTF8 string that has 
/// entirely valid UTF8 content. It only trims the string if there is an incomplete UTF8 sequence at the
/// end. The resulting string will always be a valid UTF8 string, whereas the input string may not be.
/// Returns the strlen of the trimmed string.
size_t UTF8TrimPartialChar(char* pString, size_t nLength)
{
	size_t validPos = 0;

	while(validPos < nLength)
	{
		uint8_t ch = (uint8_t)pString[validPos];
		size_t length = utf8lengthTable[ch];
		
		// length = 0 means invalid UTF8 marker
		if((length == 0) || ((validPos + length) > nLength))
			break;
		else
			validPos += length;
	}

	pString[validPos] = 0;
	return validPos;
}


///////////////////////////////////////////////////////////////////////////////
// UTF8ReplaceInvalidChar
// 
// This function replaces all invalidate UTF8 characters with the user provided
// 8-bit replacement. The returned character array is guaranteed null-terminated.
//
EASTDC_API char* UTF8ReplaceInvalidChar(const char* pIn, size_t nLength, char* pOut, char replaceWith)
{
	size_t validPos = 0;

	while(validPos < nLength)
	{
		uint8_t ch = (uint8_t)pIn[validPos];
		size_t length = utf8lengthTable[ch];
		
		// length = 0 means invalid UTF8 marker
		if((length == 0) || ((validPos + length) > nLength))
		{
			pOut[validPos++] = replaceWith;
		}
		else
		{
			for(auto i = validPos; i < validPos + length; i++)
				pOut[i] = pIn[i];

			validPos += length;
		}
	}

	pOut[validPos] = 0;
	return pOut + validPos;
}



///////////////////////////////////////////////////////////////////////////////
// MatchPattern
//
// This function is recursively called on substrings. 
// Used by the WildcardMatch function.
//
template <class CharT>
bool MatchPattern(const CharT* pElement, const CharT* pPattern)
{
	if((*pPattern == (CharT)'*') && !pPattern[1])
		return true;                                       // The pattern is set to match everything, so return true.
	else if(!*pElement && *pPattern)
		return false;                                      // The element is empty but the pattern is not, so return false.
	else if(!*pElement)
		return true;                                       // The element and pattern are both empty, so we are done. Return true.
	else
	{
		if(*pPattern == (CharT)'*')
		{
			if(MatchPattern(pElement, pPattern+1))          // What this section does is try to match source segments to
				return true;                                // the '*' portion of the pattern. As many parts of the source that
			else                                            // can be assigned to the '*' portion of the pattern are done. If
				return MatchPattern(pElement+1, pPattern);  // not possible, we pop out of the whole thing.
		}
		else if(*pPattern == (CharT)'?')
			return MatchPattern(pElement+1, pPattern+1);    // The pattern accepts any character here, so move onto the next character.
		else
		{
			if(*pElement == *pPattern)
				return MatchPattern(pElement+1, pPattern+1); // The current element and pattern chars match, so move onto next character.
			else
				return false;                                // The current element char simply doesn't match the pattern char, so return false.
		}
	}
	// return true;   // This should never get executed, but some compilers might not be smart enough to realize it.
}


///////////////////////////////////////////////////////////////////////////////
// WildcardMatch
//
// We go through extra effort below to avoid doing memory allocation in most cases.
//
EASTDC_API bool WildcardMatch(const char* pString, const char* pPattern, bool bCaseSensitive)
{
	if(bCaseSensitive)
		return MatchPattern(pString, pPattern);
	else
	{
		// Do efficient string conversion to lower case...
		char  pStringLBuffer[384];
		char* pStringL;
		char* pStringLAllocated;
		size_t   nStringLLength = Strlen(pString);

		if(nStringLLength >= (sizeof(pStringLBuffer) / sizeof(pStringLBuffer[0]) - 1))
		{
			pStringLAllocated = EASTDC_NEW("EATextUtil/StringAllocated/char[]") char[nStringLLength + 1];
			pStringL          = pStringLAllocated;
		}
		else
		{
			pStringLAllocated = NULL;
			pStringL          = pStringLBuffer;
		}
		Strcpy(pStringL, pString);
		Strlwr(pStringL);

		// Do efficient pattern conversion to lower case...
		char  pPatternLBuffer[32];
		char* pPatternL;
		char* pPatternLAllocated;
		size_t   nPatternLLength = Strlen(pPattern);

		if(nPatternLLength >= (sizeof(pPatternLBuffer) / sizeof(pPatternLBuffer[0]) - 1))
		{
			pPatternLAllocated = EASTDC_NEW("EATextUtil/PatternAllocated/char[]") char[nPatternLLength + 1];
			pPatternL          = pPatternLAllocated;
		}
		else
		{
			pPatternLAllocated = NULL;
			pPatternL          = pPatternLBuffer;
		}
		Strcpy(pPatternL, pPattern);
		Strlwr(pPatternL);

		const bool bResult = MatchPattern(pStringL, pPatternL);

		delete[] pStringLAllocated; // In most cases, this will be NULL and there will be no effect.
		delete[] pPatternLAllocated;

		return bResult;
   }
}

///////////////////////////////////////////////////////////////////////////////
// WildcardMatch
//
// We go through extra effort below to avoid doing memory allocation in most cases.
//
EASTDC_API bool WildcardMatch(const char16_t* pString, const char16_t* pPattern, bool bCaseSensitive)
{
	if(bCaseSensitive)
		return MatchPattern(pString, pPattern);
	else
	{
		// Do efficient string conversion to lower case...
		char16_t  pStringLBuffer[384];
		char16_t* pStringL;
		char16_t* pStringLAllocated;
		size_t    nStringLLength = Strlen(pString);

		if(nStringLLength >= (sizeof(pStringLBuffer) / sizeof(pStringLBuffer[0]) - 1))
		{
			pStringLAllocated = EASTDC_NEW("EATextUtil/StringAllocated/char16[]") char16_t[nStringLLength + 1];
			pStringL          = pStringLAllocated;
		}
		else
		{
			pStringLAllocated = NULL;
			pStringL          = pStringLBuffer;
		}
		Strcpy(pStringL, pString);
		Strlwr(pStringL);

		// Do efficient pattern conversion to lower case...
		char16_t  pPatternLBuffer[32];
		char16_t* pPatternL;
		char16_t* pPatternLAllocated;
		size_t    nPatternLLength = Strlen(pPattern);

		if(nPatternLLength >= (sizeof(pPatternLBuffer) / sizeof(pPatternLBuffer[0]) - 1))
		{
			pPatternLAllocated = EASTDC_NEW("EATextUtil/PatternAllocated/char16[]") char16_t[nPatternLLength + 1];
			pPatternL          = pPatternLAllocated;
		}
		else
		{
			pPatternLAllocated = NULL;
			pPatternL          = pPatternLBuffer;
		}
		Strcpy(pPatternL, pPattern);
		Strlwr(pPatternL);

		const bool bResult = MatchPattern(pStringL, pPatternL);

		delete[] pStringLAllocated; // In most cases, this will be NULL and there will be no effect.
		delete[] pPatternLAllocated;

		return bResult;
	}
}

///////////////////////////////////////////////////////////////////////////////
// WildcardMatch
//
// We go through extra effort below to avoid doing memory allocation in most cases.
//
EASTDC_API bool WildcardMatch(const char32_t* pString, const char32_t* pPattern, bool bCaseSensitive)
{
	if(bCaseSensitive)
		return MatchPattern(pString, pPattern);
	else
	{
		// Do efficient string conversion to lower case...
		char32_t  pStringLBuffer[384];
		char32_t* pStringL;
		char32_t* pStringLAllocated;
		size_t    nStringLLength = Strlen(pString);

		if(nStringLLength >= (sizeof(pStringLBuffer) / sizeof(pStringLBuffer[0]) - 1))
		{
			pStringLAllocated = EASTDC_NEW("EATextUtil/StringAllocated/char32[]") char32_t[nStringLLength + 1];
			pStringL          = pStringLAllocated;
		}
		else
		{
			pStringLAllocated = NULL;
			pStringL          = pStringLBuffer;
		}
		Strcpy(pStringL, pString);
		Strlwr(pStringL);

		// Do efficient pattern conversion to lower case...
		char32_t  pPatternLBuffer[32];
		char32_t* pPatternL;
		char32_t* pPatternLAllocated;
		size_t    nPatternLLength = Strlen(pPattern);

		if(nPatternLLength >= (sizeof(pPatternLBuffer) / sizeof(pPatternLBuffer[0]) - 1))
		{
			pPatternLAllocated = EASTDC_NEW("EATextUtil/PatternAllocated/char32[]") char32_t[nPatternLLength + 1];
			pPatternL          = pPatternLAllocated;
		}
		else
		{
			pPatternLAllocated = NULL;
			pPatternL          = pPatternLBuffer;
		}
		Strcpy(pPatternL, pPattern);
		Strlwr(pPatternL);

		const bool bResult = MatchPattern(pStringL, pPatternL);

		delete[] pStringLAllocated; // In most cases, this will be NULL and there will be no effect.
		delete[] pPatternLAllocated;

		return bResult;
	}
}



//////////////////////////////////////////////////////////////////////////
// GetTextLine
//
EASTDC_API const char* GetTextLine(const char* pText, const char* pTextEnd, const char** ppNewText)
{
	if(pText < pTextEnd)
	{
		while((pText < pTextEnd) && (*pText != '\r') && (*pText != '\n'))
			++pText;

		if(ppNewText)
		{
			*ppNewText = pText;

			if(*ppNewText < pTextEnd)
			{
				if((++*ppNewText < pTextEnd) && (**ppNewText ^ *pText) == ('\r' ^ '\n'))
					++*ppNewText;
			}
		}
	}
	else if(ppNewText)
		*ppNewText = pTextEnd;

	return pText;
}

//////////////////////////////////////////////////////////////////////////
// GetTextLine
//
EASTDC_API const char16_t* GetTextLine(const char16_t* pText, const char16_t* pTextEnd, const char16_t** ppNewText)
{
	if(pText < pTextEnd)
	{
		while((pText < pTextEnd) && (*pText != '\r') && (*pText != '\n'))
			++pText;

		if(ppNewText)
		{
			*ppNewText = pText;

			if(*ppNewText < pTextEnd)
			{
				if((++*ppNewText < pTextEnd) && (**ppNewText ^ *pText) == ('\r' ^ '\n'))
					++*ppNewText;
			}
		}
	}
	else if(ppNewText)
		*ppNewText = pTextEnd;

	return pText;
}

//////////////////////////////////////////////////////////////////////////
// GetTextLine
//
EASTDC_API const char32_t* GetTextLine(const char32_t* pText, const char32_t* pTextEnd, const char32_t** ppNewText)
{
	if(pText < pTextEnd)
	{
		while((pText < pTextEnd) && (*pText != '\r') && (*pText != '\n'))
			++pText;

		if(ppNewText)
		{
			*ppNewText = pText;

			if(*ppNewText < pTextEnd)
			{
				if((++*ppNewText < pTextEnd) && (**ppNewText ^ *pText) == ('\r' ^ '\n'))
					++*ppNewText;
			}
		}
	}
	else if(ppNewText)
		*ppNewText = pTextEnd;

	return pText;
}




EASTDC_API bool ParseDelimitedText(const char* pText, const char* pTextEnd, char cDelimiter, 
								   const char*& pToken, const char*& pTokenEnd, const char** ppNewText)
{
	int  nQuoteLevel     = 0;
	bool bDelimiterFound = false;

	// We remove leading spaces.
	for(pToken = pText; pToken < pTextEnd; ++pToken)
	{
		if((*pToken != ' ') && (*pToken != '\t'))
			break;
	}

	for(pTokenEnd = pToken; pTokenEnd < pTextEnd; ++pTokenEnd)
	{
		const bool bLastCharacter = ((pTokenEnd + 1) == pTextEnd);

		if(cDelimiter == ' ')  // The space char delimiter is a special case that means delimit by whitespace.
			bDelimiterFound = ((*pTokenEnd == ' ') || (*pTokenEnd == '\t'));
		else
			bDelimiterFound = (*pTokenEnd == cDelimiter);

		if(bDelimiterFound || bLastCharacter) // If we found a delimiter or if we are on the last character...
		{
			if(!bDelimiterFound)
				++pTokenEnd;

			const bool bInQuotes = ((nQuoteLevel & 1) != 0);

			if(!bInQuotes || bLastCharacter) // If not within a quoted section...
			{
				if(ppNewText)
					*ppNewText = pTokenEnd;

				if((cDelimiter != ' ') && (pTokenEnd != pTextEnd))
				{
					// Eliminate spaces before the trailing delimiter.
					while((pTokenEnd != pToken) && ((pTokenEnd[-1] == ' ') || (pTokenEnd[-1] == '\t')))
						pTokenEnd--;
				}

				if((pToken != pTextEnd) && (*pToken == '"') && (pTokenEnd[-1] == '"'))
				{
					pToken++;
					pTokenEnd--;
				}

				return true;
			}
		}
		else if(*pTokenEnd == '"')
			nQuoteLevel++;
	}

	if(ppNewText)
		*ppNewText = pTokenEnd;

	return false;
}

//////////////////////////////////////////////////////////////////////////
// ParseDelimitedText
//
// This function takes a line text that has fields separated by delimiters
// and parses the line into the component fields. It is common to read
// command lines like this or to parse ini file settings like this.
//
EASTDC_API bool ParseDelimitedText(const char16_t* pText, const char16_t* pTextEnd, char16_t cDelimiter, 
								   const char16_t*& pToken, const char16_t*& pTokenEnd, const char16_t** ppNewText)
{
	int  nQuoteLevel     = 0;
	bool bDelimiterFound = false;

	// We remove leading spaces.
	for(pToken = pText; pToken < pTextEnd; ++pToken)
	{
		if((*pToken != ' ') && (*pToken != '\t'))
			break;
	}

	for(pTokenEnd = pToken; pTokenEnd < pTextEnd; ++pTokenEnd)
	{
		const bool bLastCharacter = ((pTokenEnd + 1) == pTextEnd);

		if(cDelimiter == ' ')  // The space char delimiter is a special case that means delimit by whitespace.
			bDelimiterFound = ((*pTokenEnd == ' ') || (*pTokenEnd == '\t'));
		else
			bDelimiterFound = (*pTokenEnd == cDelimiter);

		if(bDelimiterFound || bLastCharacter) // If we found a delimiter or if we are on the last character...
		{
			if(!bDelimiterFound)
				++pTokenEnd;

			const bool bInQuotes = ((nQuoteLevel & 1) != 0);

			if(!bInQuotes || bLastCharacter) // If not within a quoted section...
			{
				if(ppNewText)
					*ppNewText = pTokenEnd;

				if((cDelimiter != ' ') && (pTokenEnd != pTextEnd))
				{
					// Eliminate spaces before the trailing delimiter.
					while((pTokenEnd != pToken) && ((pTokenEnd[-1] == ' ') || (pTokenEnd[-1] == '\t')))
						pTokenEnd--;
				}

				if((pToken != pTextEnd) && (*pToken == '"') && (pTokenEnd[-1] == '"'))
				{
					pToken++;
					pTokenEnd--;
				}

				return true;
			}
		}
		else if(*pTokenEnd == '"')
			nQuoteLevel++;
	}

	if(ppNewText)
		*ppNewText = pTokenEnd;

	return false;
}

//////////////////////////////////////////////////////////////////////////
// ParseDelimitedText
//
// This function takes a line text that has fields separated by delimiters
// and parses the line into the component fields. It is common to read
// command lines like this or to parse ini file settings like this.
//
EASTDC_API bool ParseDelimitedText(const char32_t* pText, const char32_t* pTextEnd, char32_t cDelimiter, 
								   const char32_t*& pToken, const char32_t*& pTokenEnd, const char32_t** ppNewText)
{
	int  nQuoteLevel     = 0;
	bool bDelimiterFound = false;

	// We remove leading spaces.
	for(pToken = pText; pToken < pTextEnd; ++pToken)
	{
		if((*pToken != ' ') && (*pToken != '\t'))
			break;
	}

	for(pTokenEnd = pToken; pTokenEnd < pTextEnd; ++pTokenEnd)
	{
		const bool bLastCharacter = ((pTokenEnd + 1) == pTextEnd);

		if(cDelimiter == ' ')  // The space char delimiter is a special case that means delimit by whitespace.
			bDelimiterFound = ((*pTokenEnd == ' ') || (*pTokenEnd == '\t'));
		else
			bDelimiterFound = (*pTokenEnd == cDelimiter);

		if(bDelimiterFound || bLastCharacter) // If we found a delimiter or if we are on the last character...
		{
			if(!bDelimiterFound)
				++pTokenEnd;

			const bool bInQuotes = ((nQuoteLevel & 1) != 0);

			if(!bInQuotes || bLastCharacter) // If not within a quoted section...
			{
				if(ppNewText)
					*ppNewText = pTokenEnd;

				if((cDelimiter != ' ') && (pTokenEnd != pTextEnd))
				{
					// Eliminate spaces before the trailing delimiter.
					while((pTokenEnd != pToken) && ((pTokenEnd[-1] == ' ') || (pTokenEnd[-1] == '\t')))
						pTokenEnd--;
				}

				if((pToken != pTextEnd) && (*pToken == '"') && (pTokenEnd[-1] == '"'))
				{
					pToken++;
					pTokenEnd--;
				}

				return true;
			}
		}
		else if(*pTokenEnd == '"')
			nQuoteLevel++;
	}

	if(ppNewText)
		*ppNewText = pTokenEnd;

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// ConvertBinaryDataToASCIIArray
//
// Since every binary byte converts to exactly 2 ascii bytes, the ASCII
// array  must have space for at least twice the amount of bytes
// as 'nBinaryDataLength' + 1.
//
EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData_, size_t nBinaryDataLength, char* pASCIIArray)
{
	const uint8_t* pBinaryData = (uint8_t*)pBinaryData_;
	const uint8_t* pEnd = pBinaryData + nBinaryDataLength;

	while(pBinaryData < pEnd)
	{
		*pASCIIArray = (char)('0' + ((*pBinaryData & 0xf0) >> 4));  // Convert the high byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		*pASCIIArray = (char)('0' + (*pBinaryData & 0x0f));         // Convert the low byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		pBinaryData++;
	}

	*pASCIIArray = '\0';
}

EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData_, size_t nBinaryDataLength, char16_t* pASCIIArray)
{
	const uint8_t* pBinaryData = (uint8_t*)pBinaryData_;
	const uint8_t* pEnd = pBinaryData + nBinaryDataLength;

	while(pBinaryData < pEnd)
	{
		*pASCIIArray = (char16_t)('0' + ((*pBinaryData & 0xf0) >> 4));  // Convert the high byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		*pASCIIArray = (char16_t)('0' + (*pBinaryData & 0x0f));         // Convert the low byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		pBinaryData++;
	}

	*pASCIIArray = '\0';
}

EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData_, size_t nBinaryDataLength, char32_t* pASCIIArray)
{
	const uint8_t* pBinaryData = (uint8_t*)pBinaryData_;
	const uint8_t* pEnd = pBinaryData + nBinaryDataLength;

	while(pBinaryData < pEnd)
	{
		*pASCIIArray = (char32_t)('0' + ((*pBinaryData & 0xf0) >> 4));  // Convert the high byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		*pASCIIArray = (char32_t)('0' + (*pBinaryData & 0x0f));         // Convert the low byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; // Convert the ':' to 'A', for example.
		pASCIIArray++;
		pBinaryData++;
	}

	*pASCIIArray = '\0';
}



//////////////////////////////////////////////////////////////////////////////
// ConvertASCIIArrayToBinaryData (8 bit version)
//
// We have a boolean return value because it is possible that the ascii data is
// corrupt. We check for this corruption and return false if so, while converting
// all corrupt bytes to valid ones.
//
EASTDC_API bool ConvertASCIIArrayToBinaryData(const char* pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData)
{
	uint8_t*        pBinaryData8 = (uint8_t*)pBinaryData;
	const char*  pEnd = pASCIIArray + nASCIIArrayLength;
	char         cTemp;
	bool            bReturnValue(true);

	while(pASCIIArray < pEnd)
	{
		*pBinaryData8 = 0;

		for(int j = 4; j >= 0; j -= 4)
		{
			cTemp = *pASCIIArray;

			if(cTemp < '0') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp > 'F') // Do some bounds checking.
			{
				if(cTemp >= 'a' && cTemp <= 'f')
					cTemp -= 39; // Convert 'a' to ':'.
				else
				{
					cTemp = '0';
					bReturnValue = false;
				}
			}
			else if(cTemp > '9' && cTemp < 'A') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp >= 'A')
				cTemp -= 7;

			*pBinaryData8 = (uint8_t)(*pBinaryData8 + ((cTemp - '0') << j));
			pASCIIArray++;
		}

		pBinaryData8++;
	}

	return bReturnValue;
}

//////////////////////////////////////////////////////////////////////////////
// ConvertASCIIArrayToBinaryData (16 bit version)
//
// We have a boolean return value because it is possible that the ascii data is
// corrupt. We check for this corruption and return false if so, while converting
// all corrupt bytes to valid ones.
//
EASTDC_API bool ConvertASCIIArrayToBinaryData(const char16_t* pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData)
{
	uint8_t*        pBinaryData8 = (uint8_t*)pBinaryData;
	const char16_t* pEnd = pASCIIArray + nASCIIArrayLength;
	char16_t        cTemp;
	bool            bReturnValue(true);

	while(pASCIIArray < pEnd)
	{
		*pBinaryData8 = 0;

		for(int j = 4; j >= 0; j -= 4)
		{
			cTemp = *pASCIIArray;

			if(cTemp < '0') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp > 'F') // Do some bounds checking.
			{
				if(cTemp >= 'a' && cTemp <= 'f')
					cTemp -= 39; // Convert 'a' to ':'.
				else
				{
					cTemp = '0';
					bReturnValue = false;
				}
			}
			else if(cTemp > '9' && cTemp < 'A') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp >= 'A')
				cTemp -= 7;

			*pBinaryData8 = (uint8_t)(*pBinaryData8 + ((cTemp - '0') << j));
			pASCIIArray++;
		}

		pBinaryData8++;
	}

	return bReturnValue;
}

//////////////////////////////////////////////////////////////////////////////
// ConvertASCIIArrayToBinaryData (32 bit version)
//
// We have a boolean return value because it is possible that the ascii data is
// corrupt. We check for this corruption and return false if so, while converting
// all corrupt bytes to valid ones.
//
EASTDC_API bool ConvertASCIIArrayToBinaryData(const char32_t* pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData)
{
	uint8_t*        pBinaryData8 = (uint8_t*)pBinaryData;
	const char32_t* pEnd = pASCIIArray + nASCIIArrayLength;
	char32_t        cTemp;
	bool            bReturnValue(true);

	while(pASCIIArray < pEnd)
	{
		*pBinaryData8 = 0;

		for(int j = 4; j >= 0; j -= 4)
		{
			cTemp = *pASCIIArray;

			if(cTemp < '0') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp > 'F') // Do some bounds checking.
			{
				if(cTemp >= 'a' && cTemp <= 'f')
					cTemp -= 39; // Convert 'a' to ':'.
				else
				{
					cTemp = '0';
					bReturnValue = false;
				}
			}
			else if(cTemp > '9' && cTemp < 'A') // Do some bounds checking.
			{
				cTemp = '0';
				bReturnValue = false;
			}
			else if(cTemp >= 'A')
				cTemp -= 7;

			*pBinaryData8 = (uint8_t)(*pBinaryData8 + ((cTemp - '0') << j));
			pASCIIArray++;
		}

		pBinaryData8++;
	}

	return bReturnValue;
}






//////////////////////////////////////////////////////////////////////////////
// SplitTokenDelimited (8 bit version)
//  
EASTDC_API bool SplitTokenDelimited(const char* pSource, size_t nSourceLength, char cDelimiter, 
									char* pToken, size_t nTokenLength, const char** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)
	if(pToken && nTokenLength)
		*pToken = 0;

	if(pSource && nSourceLength && *pSource)
	{
		// look for the delimiter
		for(size_t i = 0; i < nSourceLength && *pSource; i++)
		{
			const char cTemp(*pSource);

			// update new source pointer if present
			if(ppNewSource)
				(*ppNewSource)++;

			if(cTemp == cDelimiter) // If there is a delimiter match...
				break; // We are done.
			else
			{
				// keep moving characters into the token until we find the delimiter or reached the end of the token string
				if(pToken && ((i + 1) < nTokenLength)) // we need an extra character for terminating null
				{
					*pToken = cTemp;    // add the character
					 pToken++;          // increment the token pointer
					*pToken = 0;        // insert terminating null character
				}

				pSource++; // increment source pointer
			}
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// SplitTokenDelimited (16 bit version)
//
// Implemented by Blazej Stompel and Paul Pedriana
//
EASTDC_API bool SplitTokenDelimited(const char16_t* pSource, size_t nSourceLength, char16_t cDelimiter, 
									char16_t* pToken, size_t nTokenLength, const char16_t** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)
	if(pToken && nTokenLength)
		*pToken = 0;

	if(pSource && nSourceLength && *pSource)
	{
		// look for the delimiter
		for(size_t i = 0; i < nSourceLength && *pSource; i++)
		{
			const char16_t cTemp(*pSource);

			// update new source pointer if present
			if(ppNewSource)
				(*ppNewSource)++;

			if(cTemp == cDelimiter) // If there is a delimiter match...
				break; // We are done.
			else
			{
				// keep moving characters into the token until we find the delimiter or reached the end of the token string
				if(pToken && ((i + 1) < nTokenLength)) // we need an extra character for terminating null
				{
					*pToken = cTemp;    // add the character
					 pToken++;          // increment the token pointer
					*pToken = 0;        // insert terminating null character
				}

				pSource++; // increment source pointer
			}
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// SplitTokenDelimited (32 bit version)
//
// Implemented by Blazej Stompel and Paul Pedriana
//
EASTDC_API bool SplitTokenDelimited(const char32_t* pSource, size_t nSourceLength, char32_t cDelimiter, 
									char32_t* pToken, size_t nTokenLength, const char32_t** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)
	if(pToken && nTokenLength)
		*pToken = 0;

	if(pSource && nSourceLength && *pSource)
	{
		// look for the delimiter
		for(size_t i = 0; i < nSourceLength && *pSource; i++)
		{
			const char32_t cTemp(*pSource);

			// update new source pointer if present
			if(ppNewSource)
				(*ppNewSource)++;

			if(cTemp == cDelimiter) // If there is a delimiter match...
				break; // We are done.
			else
			{
				// keep moving characters into the token until we find the delimiter or reached the end of the token string
				if(pToken && ((i + 1) < nTokenLength)) // we need an extra character for terminating null
				{
					*pToken = cTemp;    // add the character
					 pToken++;          // increment the token pointer
					*pToken = 0;        // insert terminating null character
				}

				pSource++; // increment source pointer
			}
		}

		return true;
	}

	return false;
}




//////////////////////////////////////////////////////////////////////////////
// SplitTokenSeparated (8 bit version)
//
EASTDC_API bool SplitTokenSeparated(const char* pSource, size_t nSourceLength, char c, 
									char* pToken, size_t nTokenLength, const char** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)

	if(pToken && nTokenLength)
		*pToken = '\0';

	if(pSource)
	{
		// keep track of how many characters we have written to the token buffer
		size_t nTokenIndex = 0;

		// keep track whether we found the token and if we are done reading it
		bool bFoundToken = false;
		bool bReadToken  = false;

		// look for the separators
		for(size_t i = 0; i < nSourceLength; i++)
		{
			// get the character
			const char cTemp(*pSource);

			// quit if we found the terminating null character
			if(cTemp != '\0')
			{
				// is the character not a separator ?
				if(cTemp != c)
				{
					// we have a token
					bFoundToken = true;

					// were we done reading the token ?
					if(bReadToken)
						return true;
					else
					{
						// add the character to the token
						if(pToken && (nTokenIndex + 1) < nTokenLength) // we need an extra character for terminating null
						{
							// add the character
							*pToken = cTemp;

							// increment the token pointer
							pToken++;

							// and index
							nTokenIndex++;

							// insert terminating null character
							*pToken = '\0';
						}
					}
				}
				else
				{
					// the character is a separator - if we found our token then we are done reading it
					if(bFoundToken)
						bReadToken = true;
				}

				// update new source pointer if present
				if(ppNewSource)
					(*ppNewSource)++;

				// increment source pointer
				pSource++;
			}
			else
			{
				// we have reached the end of the string
				break;
			}
		}

		return bFoundToken;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// SplitTokenSeparated (16 bit version)
//
// Implemented by Blazej Stompel
//
// Unit test can be found in Foundation\Test\UnitTests
//
EASTDC_API bool SplitTokenSeparated(const char16_t* pSource, size_t nSourceLength, char16_t c, 
									char16_t* pToken, size_t nTokenLength, const char16_t** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)
	if(pToken && nTokenLength)
		*pToken = '\0';

	if(pSource)
	{
		// keep track of how many characters we have written to the token buffer
		size_t nTokenIndex = 0;

		// keep track whether we found the token and if we are done reading it
		bool bFoundToken = false;
		bool bReadToken  = false;

		// look for the separators
		for(size_t i = 0; i < nSourceLength; i++)
		{
			// get the character
			const char16_t cTemp(*pSource);

			// quit if we found the terminating null character
			if(cTemp != '\0')
			{
				// is the character not a separator ?
				if(cTemp != c)
				{
					// we have a token
					bFoundToken = true;

					// were we done reading the token ?
					if(bReadToken)
						return true;
					else
					{
						// add the character to the token
						if(pToken && (nTokenIndex + 1) < nTokenLength) // we need an extra character for terminating null
						{
							// add the character
							*pToken = cTemp;

							// increment the token pointer
							pToken++;

							// and index
							nTokenIndex++;

							// insert terminating null character
							*pToken = '\0';
						}
					}
				}
				else
				{
					// the character is a separator - if we found our token then we are done reading it
					if(bFoundToken)
						bReadToken = true;
				}

				// update new source pointer if present
				if(ppNewSource)
					(*ppNewSource)++;

				// increment source pointer
				pSource++;
			}
			else
			{
				// we have reached the end of the string
				break;
			}
		}

		return bFoundToken;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// SplitTokenSeparated (32 bit version)
//
// Implemented by Blazej Stompel
//
// Unit test can be found in Foundation\Test\UnitTests
//
EASTDC_API bool SplitTokenSeparated(const char32_t* pSource, size_t nSourceLength, char32_t c, 
									char32_t* pToken, size_t nTokenLength, const char32_t** ppNewSource)
{
	// terminate the token (so it appears empty if we don't find anything)
	if(pToken && nTokenLength)
		*pToken = '\0';

	if(pSource)
	{
		// keep track of how many characters we have written to the token buffer
		size_t nTokenIndex = 0;

		// keep track whether we found the token and if we are done reading it
		bool bFoundToken = false;
		bool bReadToken  = false;

		// look for the separators
		for(size_t i = 0; i < nSourceLength; i++)
		{
			// get the character
			const char32_t cTemp(*pSource);

			// quit if we found the terminating null character
			if(cTemp != '\0')
			{
				// is the character not a separator ?
				if(cTemp != c)
				{
					// we have a token
					bFoundToken = true;

					// were we done reading the token ?
					if(bReadToken)
						return true;
					else
					{
						// add the character to the token
						if(pToken && (nTokenIndex + 1) < nTokenLength) // we need an extra character for terminating null
						{
							// add the character
							*pToken = cTemp;

							// increment the token pointer
							pToken++;

							// and index
							nTokenIndex++;

							// insert terminating null character
							*pToken = '\0';
						}
					}
				}
				else
				{
					// the character is a separator - if we found our token then we are done reading it
					if(bFoundToken)
						bReadToken = true;
				}

				// update new source pointer if present
				if(ppNewSource)
					(*ppNewSource)++;

				// increment source pointer
				pSource++;
			}
			else
			{
				// we have reached the end of the string
				break;
			}
		}

		return bFoundToken;
	}

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// Boyer-Moore string search
//
// This is the "turbo" implementation defined at http://www-igm.univ-mlv.fr/~lecroq/string/node14.html#SECTION00140.
// Boyer-Moore is a very fast string search compared to most others, including
// those in the STL. However, you need to be searching a string of at least 100
// chars and have a search pattern of at least 3 characters for the speed to show,
// as Boyer-Moore has a startup precalculation that costs some cycles. 
// This startup precalculation is proportional to the size of your search pattern
// and the size of the alphabet in use. Thus, doing Boyer-Moore searches on the 
// entire Unicode alphabet is going to incur a fairly expensive precalculation cost.
//

// This is a private function used by BoyerMooreSearch.
// 
static void BoyerMooreBadCharacterCalc(const char* pPattern, int nPatternLength, 
									   int* pAlphabetBuffer, int nAlphabetBufferSize)
{
	int i;
	 
	for(i = 0; i < nAlphabetBufferSize; ++i)
		pAlphabetBuffer[i] = nPatternLength;

	for(i = 0; i < (nPatternLength - 1); ++i)
		pAlphabetBuffer[(int)pPattern[i]] = (nPatternLength - i) - 1;
}


// This is a private function used by BoyerMooreSearch.
// 
static void BoyerMooreGoodSuffixCalc(const char* pPattern, int nPatternLength, 
									 int* pPatternBuffer1, int* pPatternBuffer2)
{
	int i;
	int j = 0;
	int f = 0;
	int g = nPatternLength - 1;

	pPatternBuffer2[nPatternLength - 1] = nPatternLength;

	for(i = nPatternLength - 2; i >= 0; --i)
	{
		if((i > g) && pPatternBuffer2[((i + nPatternLength) - 1) - f] < (i - g))
			pPatternBuffer2[i] = pPatternBuffer2[((i + nPatternLength) - 1) - f];
		else 
		{
			if(i < g)
				g = i;

			f = i;

			while((g >= 0) && (pPattern[g] == pPattern[((g + nPatternLength) - 1) - f]))
				--g;

			pPatternBuffer2[i] = f - g;
		}
	}

	for(i = 0; i < nPatternLength; ++i)
		pPatternBuffer1[i] = nPatternLength;

	for(i = nPatternLength - 1; i >= -1; --i)
	{
		if((i == -1) || (pPatternBuffer2[i] == (i + 1)))
		{
			for(; j < (nPatternLength - 1) - i; ++j)
			{
				if(pPatternBuffer1[j] == nPatternLength)
					pPatternBuffer1[j] = (nPatternLength - 1) - i;
			}
		}
	}

	for(i = 0; i <= nPatternLength - 2; ++i)
		pPatternBuffer1[(nPatternLength - 1) - pPatternBuffer2[i]] = (nPatternLength - 1) - i;
}



// Argument specification.
//
// patternBuffer1 is a user-supplied buffer and must be at least as long as the search pattern.
// patternBuffer2 is a user-supplied buffer and must be at least as long as the search pattern.
// alphabetBuffer is a user-supplied buffer and must be at least as long as the highest character value used in the searched string and search pattern.
//
EASTDC_API int BoyerMooreSearch(const char* pPattern, int nPatternLength, const char* pSearchString, int nSearchStringLength, 
								int* pPatternBuffer1, int* pPatternBuffer2, int* pAlphabetBuffer, int nAlphabetBufferSize)
{
	// Do precalculations
	BoyerMooreGoodSuffixCalc(pPattern, nPatternLength, pPatternBuffer1, pPatternBuffer2);
	BoyerMooreBadCharacterCalc(pPattern, nPatternLength, pAlphabetBuffer, nAlphabetBufferSize);

	// Do search
	for(int j = 0, shift = nPatternLength, u = 0; j <= (nSearchStringLength - nPatternLength); j += shift)
	{
		int i = nPatternLength - 1;

		while((i >= 0) && (pPattern[i] == pSearchString[i + j]))
		{
			--i;

			if((u != 0) && (i == (nPatternLength - 1) - shift))
				i -= u;
		}

		if(i < 0)
		{
			return j;

			// Only used if we were iterating multiple found items:
			//shift = pPatternBuffer1[0];
			//u     = nPatternLength - shift;
		}
		else
		{
			const int v          = nPatternLength - 1 - i;
			const int turboShift = u - v;
			const int bcShift    = pAlphabetBuffer[(int)pSearchString[i + j]] - nPatternLength + 1 + i;
			shift                = EATEXTUTIL_MAX(turboShift, bcShift);
			shift                = EATEXTUTIL_MAX(shift, pPatternBuffer1[i]);

			if(shift == pPatternBuffer1[i])
				u = EATEXTUTIL_MIN(nPatternLength - shift, v);
			else
			{
				if(turboShift < bcShift)
					shift = EATEXTUTIL_MAX(shift, u + 1);
				u = 0;
			}
		}
	}

	return nPatternLength;
}


#undef EATEXTUTIL_MIN
#undef EATEXTUTIL_MAX


} // namespace StdC
} // namespace EA





















