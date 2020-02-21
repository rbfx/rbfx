///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This module implements the following functions.
//
//  int     Isalnum(char_t c);
//  int     Isalpha(char_t c);
//  int     Isdigit(char_t c);
//  int     Isxdigit(char_t c);
//  int     Isgraph(char_t c);
//  int     Islower(char_t c);
//  char_t  Tolower(char_t c);
//  int     Isupper(char_t c);
//  char_t  Toupper(char_t c);
//  int     Isprint(char_t c);
//  int     Ispunct(char_t c);
//  int     Isspace(char_t c);
//  int     Iscntrl(char_t c);
//  int     Isascii(char_t c);
//
// By design, the 16 bit versions of these functions work only for chars 
// up to 255. Characters above that always yield a return value of zero.
// If you want Unicode-correct character and string classification 
// functionality for all Unicode characters, use the EATextUnicode module 
// from the EAText package. The Unicode module may be split off into a  
// unique package by the time you read this.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EACTYPE_H
#define EASTDC_EACTYPE_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>



namespace EA
{
namespace StdC
{

	EASTDC_API int      Isalnum(char c);
	EASTDC_API int      Isalnum(char16_t c);
	EASTDC_API int      Isalnum(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isalnum(wchar_t c);
	#endif

	EASTDC_API int      Isalpha(char c);
	EASTDC_API int      Isalpha(char16_t c);
	EASTDC_API int      Isalpha(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isalpha(wchar_t c);
	#endif

	EASTDC_API int      Isdigit(char c);
	EASTDC_API int      Isdigit(char16_t c);
	EASTDC_API int      Isdigit(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isdigit(wchar_t c);
	#endif

	EASTDC_API int      Isxdigit(char c);
	EASTDC_API int      Isxdigit(char16_t c);
	EASTDC_API int      Isxdigit(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isxdigit(wchar_t c);
	#endif

	EASTDC_API int      Isgraph(char c);
	EASTDC_API int      Isgraph(char16_t c);
	EASTDC_API int      Isgraph(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isgraph(wchar_t c);
	#endif

	EASTDC_API int      Islower(char c);
	EASTDC_API int      Islower(char16_t c);
	EASTDC_API int      Islower(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Islower(wchar_t c);
	#endif

	EASTDC_API char  Tolower(char c);
	EASTDC_API char16_t Tolower(char16_t c);
	EASTDC_API char32_t Tolower(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API wchar_t  Tolower(wchar_t c);
	#endif

	EASTDC_API int      Isupper(char c);
	EASTDC_API int      Isupper(char16_t c);
	EASTDC_API int      Isupper(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isupper(wchar_t c);
	#endif

	EASTDC_API char  Toupper(char c);
	EASTDC_API char16_t Toupper(char16_t c);
	EASTDC_API char32_t Toupper(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API wchar_t  Toupper(wchar_t c);
	#endif

	EASTDC_API int      Isprint(char c);
	EASTDC_API int      Isprint(char16_t c);
	EASTDC_API int      Isprint(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isprint(wchar_t c);
	#endif

	EASTDC_API int      Ispunct(char c);
	EASTDC_API int      Ispunct(char16_t c);
	EASTDC_API int      Ispunct(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Ispunct(wchar_t c);
	#endif

	EASTDC_API int      Isspace(char c);
	EASTDC_API int      Isspace(char16_t c);
	EASTDC_API int      Isspace(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isspace(wchar_t c);
	#endif

	EASTDC_API int      Iscntrl(char c);
	EASTDC_API int      Iscntrl(char16_t c);
	EASTDC_API int      Iscntrl(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Iscntrl(wchar_t c);
	#endif

	EASTDC_API int      Isascii(char c);
	EASTDC_API int      Isascii(char16_t c);
	EASTDC_API int      Isascii(char32_t c);
	#if EA_WCHAR_UNIQUE
		EASTDC_API int  Isascii(wchar_t c);
	#endif

} // namespace StdC
} // namespace EA




///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
namespace StdC
{
	const size_t EASTDC_WCMAP_SIZE = 256;

	extern EASTDC_API uint8_t EASTDC_WCTYPE_MAP[EASTDC_WCMAP_SIZE];
	extern EASTDC_API uint8_t EASTDC_WLOWER_MAP[EASTDC_WCMAP_SIZE];
	extern EASTDC_API uint8_t EASTDC_WUPPER_MAP[EASTDC_WCMAP_SIZE];

	#define EASTDC_WCTYPE_CONTROL_1    0x01
	#define EASTDC_WCTYPE_MOTION       0x02
	#define EASTDC_WCTYPE_SPACE_1      0x04
	#define EASTDC_WCTYPE_PUNCT        0x08
	#define EASTDC_WCTYPE_DIGIT        0x10
	#define EASTDC_WCTYPE_XDIGIT       0x20
	#define EASTDC_WCTYPE_LOWER        0x40
	#define EASTDC_WCTYPE_UPPER        0x80
	#define EASTDC_WCTYPE_ALPHA        (EASTDC_WCTYPE_LOWER     | EASTDC_WCTYPE_UPPER)
	#define EASTDC_WCTYPE_ALNUM        (EASTDC_WCTYPE_ALPHA     | EASTDC_WCTYPE_DIGIT)
	#define EASTDC_WCTYPE_GRAPH        (EASTDC_WCTYPE_ALNUM     | EASTDC_WCTYPE_PUNCT)
	#define EASTDC_WCTYPE_PRINT        (EASTDC_WCTYPE_GRAPH     | EASTDC_WCTYPE_SPACE)
	#define EASTDC_WCTYPE_SPACE        (EASTDC_WCTYPE_SPACE_1   | EASTDC_WCTYPE_MOTION)
	#define EASTDC_WCTYPE_CONTROL      (EASTDC_WCTYPE_CONTROL_1 | EASTDC_WCTYPE_MOTION)



	inline int Isalnum(char c) // char is the same as char -- it is a signed or unsigned 8 bit value
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_ALNUM;
	}

	inline int Isalnum(char16_t c) // char16_t is an unsigned 16 bit value (the same as wchar_t when wchar_t is 16 bit). 
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALNUM) : 0);
	}

	inline int Isalnum(char32_t c) // char16_t is an unsigned 16 bit value (the same as wchar_t when wchar_t is 16 bit). 
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALNUM) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isalnum(wchar_t c) // char16_t is an unsigned 16 bit value (the same as wchar_t when wchar_t is 16 bit). 
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALNUM) : 0);
		}
	#endif



	inline int Isalpha(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_ALPHA;
	}

	inline int Isalpha(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALPHA) : 0);
	}

	inline int Isalpha(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALPHA) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isalpha(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_ALPHA) : 0);
		}
	#endif


	inline int Isdigit(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_DIGIT;

		// Alternative which may be faster due to avoiding a memory hit:
		// return (((unsigned)(int)c - '0') < 10) ? 1 : 0;
	}

	inline int Isdigit(char16_t c)
	{
		// return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_DIGIT) : 0);
		// Since the above has a conditional, we can use a similar conditional here but avoid a memory hit.
		return (((unsigned)c - '0') < 10) ? 1 : 0;
	}

	inline int Isdigit(char32_t c)
	{
		// return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_DIGIT) : 0);
		// Since the above has a conditional, we can use a similar conditional here but avoid a memory hit.
		return (((uint32_t)c - '0') < 10) ? 1 : 0;
	}

	#if EA_WCHAR_UNIQUE
		inline int Isdigit(wchar_t c)
		{
			// return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_DIGIT) : 0);
			// Since the above has a conditional, we can use a similar conditional here but avoid a memory hit.
			return (((uint32_t)c - '0') < 10) ? 1 : 0;
		}
	#endif



	inline int Isxdigit(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_XDIGIT;
	}

	inline int Isxdigit(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_XDIGIT) : 0);
	}

	inline int Isxdigit(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_XDIGIT) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isxdigit(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_XDIGIT) : 0);
		}
	#endif



	inline int Isgraph(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_GRAPH;
	}

	inline int Isgraph(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_GRAPH) : 0);
	}

	inline int Isgraph(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_GRAPH) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isgraph(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_GRAPH) : 0);
		}
	#endif


	inline int Islower(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_LOWER;
	}

	inline int Islower(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_LOWER) : 0);
	}

	inline int Islower(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_LOWER) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Islower(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_LOWER) : 0);
		}
	#endif



	inline char Tolower(char c)
	{
		return (char)EASTDC_WLOWER_MAP[(uint8_t)c];
	}

	inline char16_t Tolower(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (char16_t)EASTDC_WLOWER_MAP[(char16_t)c] : c);
	}

	inline char32_t Tolower(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (char32_t)EASTDC_WLOWER_MAP[(char16_t)c] : c);
	}

	#if EA_WCHAR_UNIQUE
		inline wchar_t Tolower(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (wchar_t)EASTDC_WLOWER_MAP[(char16_t)c] : c);
		}
	#endif


	inline int Isupper(char c)
	{
		return (char)EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_UPPER;
	}

	inline int Isupper(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_UPPER) : 0);
	}

	inline int Isupper(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_UPPER) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isupper(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_UPPER) : 0);
		}
	#endif


	inline char Toupper(char c)
	{
		return (char)EASTDC_WUPPER_MAP[(uint8_t)c];
	}

	inline char16_t Toupper(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (char16_t)EASTDC_WUPPER_MAP[(char16_t)c] : c);
	}

	inline char32_t Toupper(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (char32_t)EASTDC_WUPPER_MAP[(char16_t)c] : c);
	}

	#if EA_WCHAR_UNIQUE
		inline wchar_t Toupper(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (wchar_t)EASTDC_WUPPER_MAP[(char16_t)c] : c);
		}
	#endif


	inline int Isprint(char c) 
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_PRINT;
	}

	inline int Isprint(char16_t c) 
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PRINT) : 0);
	}

	inline int Isprint(char32_t c) 
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PRINT) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isprint(wchar_t c) 
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PRINT) : 0);
		}
	#endif


	inline int Ispunct(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_PUNCT;
	}

	inline int Ispunct(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PUNCT) : 0);
	}

	inline int Ispunct(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PUNCT) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Ispunct(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_PUNCT) : 0);
		}
	#endif


	inline int Isspace(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_SPACE;
	}

	inline int Isspace(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_SPACE) : 0);
	}

	inline int Isspace(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_SPACE) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Isspace(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_SPACE) : 0);
		}
	#endif


	inline int Iscntrl(char c)
	{
		return EASTDC_WCTYPE_MAP[(uint8_t)c] & EASTDC_WCTYPE_CONTROL;
	}

	inline int Iscntrl(char16_t c)
	{
		return (((uint16_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_CONTROL) : 0);
	}

	inline int Iscntrl(char32_t c)
	{
		return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_CONTROL) : 0);
	}

	#if EA_WCHAR_UNIQUE
		inline int Iscntrl(wchar_t c)
		{
			return (((uint32_t)c < EASTDC_WCMAP_SIZE) ? (EASTDC_WCTYPE_MAP[c] & EASTDC_WCTYPE_CONTROL) : 0);
		}
	#endif


	inline int Isascii(char c)
	{
		return (uint8_t)c < 0x80;
	}

	inline int Isascii(char16_t c)
	{
		return (uint8_t)c < 0x80;
	}

	inline int Isascii(char32_t c)
	{
		return (uint8_t)c < 0x80;
	}

	#if EA_WCHAR_UNIQUE
		inline int Isascii(wchar_t c)
		{
			return (uint8_t)c < 0x80;
		}
	#endif


} // namespace StdC
} // namespace EA


#endif // Header include guard











