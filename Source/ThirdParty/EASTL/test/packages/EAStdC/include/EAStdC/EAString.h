///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This module implements the following functions.
// Some of these are direct analogues of the Standard C Library, whereas others
// are extension functions that have uses not provided by the Standard C Library.
//
//    char_t*  Strcat(char_t* pDestination, const char_t* pSource);
//    char_t*  Strncat(char_t* pDestination, const char_t* pSource, size_t n);
//    size_t   Strlcat(char_t* pDestination, const char_t* pSource, size_t n);
//    char_t*  Strcpy(char_t* pDestination, const char_t* pSource);
//    char_t*  Strncpy(char_t* pDestination, const char_t* pSource, size_t n);
//    size_t   Strlcpy(char_t* pDestination, const char_t* pSource, size_t n);
//    int      Strlcpy(char16_t* pDestination, const char*  pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
//    int      Strlcpy(char*  pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
//    size_t   Strlen(const char_t* pString);
//    size_t   StrlenUTF8Decoded(const char* pString);
//    size_t   StrlenUTF8Encoded(const char16_t* pString);
//    char_t*  Strend(const char_t* pString);
//    size_t   Strxfrm(char_t* pDest, const char_t* pSource, size_t n);
//    char_t*  Strdup(const char_t* pString);
//
//    char_t*  Strlwr(char_t* pString);
//    char_t*  Strupr(char_t* pString);
//    char_t*  Strmix(char_t* pDestination, const char_t* pSource, const char_t* pDelimiters);
//    char_t*  Strchr(const char_t* pString, char_t c);
//    char_t*  Strnchr(const char_t* pString, char_t c, size_t n);
//    size_t   Strcspn(const char_t* pString1, const char_t* pString2);
//    char_t*  Strpbrk(const char_t* pString1, const char_t* pString2);
//    char_t*  Strrchr(const char_t* pString, char_t c);
//    size_t   Strspn(const char_t* pString, const char_t* pSubString);
//    char_t*  Strstr(const char_t* pString, const char_t* pSubString);
//    char_t*  Stristr(const char_t* pString, const char_t* pSubString);
//    bool     Strstart(const char_t* pString, const char_t* pPrefix);
//    bool     Stristart(const char_t* pString, const char_t* pPrefix);
//    bool     Strend(const char_t* pString, const char_t* pSuffix, size_t stringLength = kSizeTypeUnset);
//    bool     Striend(const char_t* pString, const char_t* pSuffix, size_t stringLength = kSizeTypeUnset);
//    char_t*  Strtok(char_t* pString, const char_t*  pTokens, char_t** pContext);
//    char_t*  Strset(char_t* pString, char_t c);
//    char_t*  Strnset(char_t* pString, char_t c, size_t n);
//    char_t*  Strrev(char_t* pString);
//    char_t*  Strstrip(char_t* pString)
//    int      Strcmp(const char_t* pString1, const char_t* pString2);
//    int      Strncmp(const char_t* pString1, const char_t* pString2, size_t n);
//    int      Stricmp(const char_t*  pString1, const char_t* pString2);
//    int      Strnicmp(const char_t* pString1, const char_t* pString2, size_t n);
//    int      StrcmpAlnum(const char_t* pString1, const char_t* pString2);
//    int      Strcoll(const char_t*  pString1, const char_t* pString2);
//    int      Strncoll(const char_t* pString1, const char_t* pString2, size_t n);
//    int      Stricoll(const char_t* pString1, const char_t* pString2);
//    int      Strnicoll(const char_t* pString1, const char_t* pString1, size_t n);
//
//    template <typename Source, typename Dest>
//    inline bool ConvertString(const Source& s, Dest& d);
//
//    template <typename Source, typename Dest>
//    inline Dest ConvertString(const Source& s);
//
//    char_t*  EcvtBuf(double dValue, int nDigitCount, int* decimalPos, int* sign, char_t* buffer);
//    char_t*  FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* decimalPos, int* sign, char_t* buffer);
//
//    char_t*  I32toa(int32_t nValue, char_t* pResult, int nBase);
//    char_t*  U32toa(uint32_t nValue, char_t* pResult, int nBase);
//    char_t*  I64toa(int64_t nValue, char_t* pBuffer, int nBase);
//    char_t*  U64toa(uint64_t nValue, char_t* pBuffer, int nBase);
//    double   Strtod(const char_t* pString, char_t** ppStringEnd);
//    double   StrtodEnglish(const char_t* pString, char_t** ppStringEnd);
//    int64_t  StrtoI64(const char_t* pString, char_t** ppStringEnd, int nBase);
//    uint64_t StrtoU64(const char_t* pString, char_t** ppStringEnd, int nBase);
//    int32_t  StrtoI32(const char_t* pString, char_t** ppStringEnd, int nBase);
//    uint32_t StrtoU32(const char_t* pString, char_t** ppStringEnd, int nBase);
//    int32_t  AtoI32(const char_t* pString);
//    uint32_t AtoU32(const char_t* pString);
//    int64_t  AtoI64(const char_t* pString);
//    uint64_t AtoU64(const char_t* pString);
//    double   Atof(const char_t* pString);
//    double   AtofEnglish(const char_t* pString);
//
//    char_t*  Ftoa(double dValue, char_t* pResult, int nInputLength, int nPrecision, bool bExponentEnabled);
//    char_t*  FtoaEnglish(double dValue, char_t* pResult, int nInputLength, int nPrecision, bool bExponentEnabled);
//
//    size_t   ReduceFloatString(char_t* pString, size_t length);
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EASTRING_H
#define EASTDC_EASTRING_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>
EA_RESTORE_ALL_VC_WARNINGS()


// EA_WCHAR_UNIQUE is defined by recent versions of EABase. If it's not defined then 
// we just define it to 0 here.
#ifndef EA_WCHAR_UNIQUE
	#define EA_WCHAR_UNIQUE 0
#endif


namespace EA
{
namespace StdC
{


/// kSizeTypeUnset
///
/// Used to specify that a size_t length is not specified, and should be 
/// determined by the function (e.g. via Strlen).
///
static const size_t kSizeTypeUnset = (size_t)~0;


/// Strlen
///
/// Returns the length of pString, not including the terminating 0 char.
/// This function acts the same as strlen.
///
EASTDC_API size_t Strlen(const char*  pString);
EASTDC_API size_t Strlen(const char16_t* pString);
EASTDC_API size_t Strlen(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t Strlen(const wchar_t* pString);
#endif
#if EA_CHAR8_UNIQUE
	inline size_t Strlen(const char8_t* pString) { return Strlen((const char*)pString); }
#endif

/// StrlenUTF8Decoded
///
/// Returns the Unicode character length of pString, not including the terminating 0 char.
/// For ASCII text, this function returns the same as Strlen. For text with UTF8 multi-byte
/// sequences, this will return a value less than Strlen.
/// This function assumes that pString represents a valid UT8-encoded string.
///
/// Note: this function is a duplicate of the EATextUtil.h UTF8Length(const char16_t*) function and is deprecated in favor of the EATextUtil.h version.
///
EASTDC_API size_t StrlenUTF8Decoded(const char* pString);


/// StrlenUTF8Encoded
///
/// Returns number of characters that would be in a UTF8-encoded equivalent string, 
/// not including the terminating 0 char. For ASCII text, this function returns 
/// the same as Strlen. For text with Unicode values > 0x7f, this function 
/// will return a value greater that Strlen.
///
/// Note: this function is a duplicate of the EATextUtil.h UTF8Length(const char*) function  and is deprecated in favor of the EATextUtil.h version.
///
EASTDC_API size_t StrlenUTF8Encoded(const char16_t* pString);
EASTDC_API size_t StrlenUTF8Encoded(const char32_t* pString);


/// Strend
///
/// Returns the end of the string, which is the same thing as (pStr + Strlen(pStr)). 
///
EASTDC_API char*  Strend(const char*  pString);
EASTDC_API char16_t* Strend(const char16_t* pString);
EASTDC_API char32_t* Strend(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strend(const wchar_t* pString);
#endif

/// Strcpy
///
/// Copies pSource to pDestination with a terminating 0 char.
/// Returns pDestination.
/// Considering using Strlcpy as a safe alternative to Strcat.
/// This function acts the same as strcpy.
///
EASTDC_API char*  Strcpy(char*  pDestination, const char*  pSource);
EASTDC_API char16_t* Strcpy(char16_t* pDestination, const char16_t* pSource);
EASTDC_API char32_t* Strcpy(char32_t* pDestination, const char32_t* pSource);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strcpy(wchar_t* pDestination, const wchar_t* pSource);
#endif


/// Strncpy
///
/// Copies the first n characters of pSource to pDestination. If the end of the 
/// pSource string is found before n characters have been copied, pDestination 
/// is padded with trailing 0 chars until a sum of n chars have been written to it.
/// Note that n denotes the maximum number of characters copied from the source.
/// 
/// pDestination will be 0-terminated only if the length of pSource is less than n.
/// This characteristic is a common source of software bugs, and should be taken
/// into account as per the example code here. 
/// Considering using Strlcpy as a safe alternative to Strncpy.
/// This function acts the same as strncpy.
///
/// Example safe usage:
///     char buffer[32];
///     Strncpy(buffer, pSomeString, 32);
///     buffer[31] = 0;
///
EASTDC_API char*  Strncpy(char*  pDestination, const char*  pSource, size_t n);
EASTDC_API char16_t* Strncpy(char16_t* pDestination, const char16_t* pSource, size_t n);
EASTDC_API char32_t* Strncpy(char32_t* pDestination, const char32_t* pSource, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strncpy(wchar_t* pDestination, const wchar_t* pSource, size_t n);
#endif


/// StringnCopy
///
/// This function is identical to the rwstdc package StringnCopy function and is 
/// provided for compatibility with it. This equivalence extends to any unintended
/// bugs that may be in the StringnCopy function.
/// Users are advised to use Strlcat instead of Strncat or StringnCat.
///
EASTDC_API char*  StringnCopy(char*  pDestination, const char*  pSource, size_t n);
EASTDC_API char16_t* StringnCopy(char16_t* pDestination, const char16_t* pSource, size_t n);
EASTDC_API char32_t* StringnCopy(char32_t* pDestination, const char32_t* pSource, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* StringnCopy(wchar_t* pDestination, const wchar_t* pSource, size_t n);
#endif

/// Strlcpy
///
/// The strlcpy and strlcat functions copy and concatenate strings
/// respectively. They are designed to be safer, more consistent, 
/// and less error prone replacements for strncpy and strncat. Unlike those
/// functions, strlcpy and strlcat take the full capacity of the buffer (not
/// just the length) and guarantee to 0-terminate the result (as long as
/// nDestCapacity is larger than 0 or, in the case of strlcat, as long as there is
/// at least one byte free in dst). Note that you should include a byte for
/// the 0 in the input nDestCapacity. Also note that strlcpy and strlcat only operate 
/// on true C strings. This means that for strlcpy pSource must be 0-terminated
/// and for strlcat both pSource and pDest must be 0-terminated.
/// 
/// The strlcpy function copies up to (nDestCapacity - 1) characters from the 
/// 0-terminated string pSource to pDest, 0-terminating the result.
/// 
/// The strlcat function appends the 0-terminated string src to the end
/// of dst. It will append at most (nDestCapacity - strlen(dst) - 1) characters, 
/// 0-terminating the result.
///
/// Users must be careful because Strlcpy and Strlcat may copy less than the required
/// characters. Unless you check the Strlcat return value, you could be trading one
/// kind of error (buffer overflow) with another (truncated string). In some cases
/// the truncated string may be significant to the point of creating security errors.
///
/// Return value
/// The strlcpy and strlcat functions return the strlen of the string they 
/// tried to create. For strlcpy that means the strlen of pSource.
/// For strlcat that means the initial strlen of pDest plus the strlen of pSource. 
/// While this may seem somewhat confusing, it makes truncation detection simple.
/// 
/// Note however, that if Strlcat traverses nDestSize characters without finding
/// a 0, the length of the string is considered to be nDestCapacity and the destination
/// string will not be 0-terminated (since there was no space for the 0). This keeps 
/// strlcat from running off the end of a string. In practice this should not happen 
/// (as it means that either size is incorrect or that dst is not a proper C string). 
/// The check exists to prevent potential security problems in incorrect code.
///
/// The versions that implement 8 <->16 or 8<->32 conversions assume the 8 bit text
/// is UTF8 and they convert to and from UCS2 and UCS4 encodings respectivly. 
///
/// Example usage:
///  The following code fragment illustrates the simple case:
///      char *s, *p, buffer[BUFFERSIZE];
///      Strlcpy(buffer, s, EAArrayCount(buffer));
///      Strlcat(buffer, p, EAArrayCount(buffer));
///      
///  To detect truncation, perhaps while building a path, the following might be used:
///      char *dir, *file, path[_MAX_PATH];
///      if(Strlcpy(path, dir, EAArrayCount(path)) >= EAArrayCount(path))
///          goto toolong;
///      if(Strlcat(path, file, EAArrayCount(path)) >= EAArrayCount(path))
///          goto toolong;
/// 
EASTDC_API size_t Strlcpy(char*     pDestination, const char*     pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcpy(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcpy(char32_t* pDestination, const char32_t* pSource, size_t nDestCapacity);

#if EA_WCHAR_UNIQUE
	EASTDC_API size_t Strlcpy(wchar_t* pDestination, const wchar_t* pSource, size_t nDestCapacity);
#endif

/// The following versions of Strlcpy do UTF8 to UTF16 conversions while copying.
/// These can thus be considered as encoding conversion functions.
/// Returns the required Strlen of pDestination.
/// Returns -1 if the source is a malformed or illegal UTF8 string and thus that 
/// the conversion cannot be completed.
/// If nSourceLength is kSizeTypeUnset then the source is copied until a 0 byte is encountered.
/// The destination needs to have nSourceLength + 1 bytes of capacity. Recall that 
/// nSourceLength is the strlen of the source and thus doesn't include the source's
/// own trailing 0 byte.
///
EASTDC_API int Strlcpy(char16_t* pDestination, const char*  pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
EASTDC_API int Strlcpy(char*  pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);

EASTDC_API int Strlcpy(char32_t* pDestination, const char*  pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
EASTDC_API int Strlcpy(char*  pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);

EASTDC_API int Strlcpy(char32_t* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
EASTDC_API int Strlcpy(char16_t* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);

#if EA_CHAR8_UNIQUE
	inline int Strlcpy(char8_t* pDestination, const char* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy((char*)pDestination, pSource, nDestCapacity);
	}
	inline int Strlcpy(char* pDestination, const char8_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy(pDestination, (const char*)pSource, nDestCapacity);
	}

	inline int Strlcpy(char8_t* pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy((char*)pDestination, pSource, nDestCapacity, nSourceLength);
	}
	inline int Strlcpy(char16_t* pDestination, const char8_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy(pDestination, (const char*)pSource, nDestCapacity, nSourceLength);
	}

	inline int Strlcpy(char8_t* pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy((char*)pDestination, pSource, nDestCapacity, nSourceLength);
	}
	inline int Strlcpy(char32_t* pDestination, const char8_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset)
	{
		return (int)Strlcpy(pDestination, (const char*)pSource, nDestCapacity, nSourceLength);
	}
#endif

#if EA_WCHAR_UNIQUE
	EASTDC_API int Strlcpy(char* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
	EASTDC_API int Strlcpy(char16_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
	EASTDC_API int Strlcpy(char32_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
	EASTDC_API int Strlcpy(wchar_t* pDestination, const char* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
	EASTDC_API int Strlcpy(wchar_t* pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
	EASTDC_API int Strlcpy(wchar_t* pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength = kSizeTypeUnset);
#endif

// To consider: Enable this for completeness with the above:
//EASTDC_API int Strlcpy(char*  pDestination, const char*  pSource, size_t nDestCapacity, size_t nSourceLength); // nSourceLength = kSizeTypeUnset
//EASTDC_API int Strlcpy(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength); // nSourceLength = kSizeTypeUnset. We can't define the default parameter because if we did then Strlcpy would conflict with the other version of Strlcpy.
//EASTDC_API int Strlcpy(char32_t* pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength); // nSourceLength = kSizeTypeUnset. We can't define the default parameter because if we did then Strlcpy would conflict with the other version of Strlcpy.

/// The following versions of Strlcpy supports partial conversion of a string.  Unlike the other Strlcpy,
/// pDest must not be nullptr.  The number of code units consumed is returned in nSourceUsed.  The number 
/// of code units written is returned in nDestUsed.
///
/// If there is space available in the destination, then a null code unit is added to the end of the destination.
///
/// If the return value is true, then:
///
///		a) If nSourceUsed == nSourceLength, then the entire source buffer was copied.
///		b) If nSourceUsed < nSourceLength, then there wasn't enough room in the destination
///		   buffer and the routine should be called again.
///
/// If the return value if false, then there was an error encoding or decoding a code point. 
/// pSource + nSourceUsed will point to the start of the code point that had issues.
///
/// kSizeTypeUnset can be specified as nSourceLength.  Regardless of the setting for nSourceLength,
/// the string copy will stop after a null is found.  In that case, nSourceUsed will have the
/// value of nSourceLength.
///
EASTDC_API bool Strlcpy(char*     pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
EASTDC_API bool Strlcpy(char*     pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
EASTDC_API bool Strlcpy(char16_t* pDest, const char*     pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
EASTDC_API bool Strlcpy(char16_t* pDest, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
EASTDC_API bool Strlcpy(char32_t* pDest, const char*     pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
EASTDC_API bool Strlcpy(char32_t* pDest, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
#if EA_WCHAR_UNIQUE
	EASTDC_API bool Strlcpy(char*     pDestination, const wchar_t*  pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
	EASTDC_API bool Strlcpy(char16_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
	EASTDC_API bool Strlcpy(char32_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
	EASTDC_API bool Strlcpy(wchar_t*  pDestination, const char*     pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
	EASTDC_API bool Strlcpy(wchar_t*  pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
	EASTDC_API bool Strlcpy(wchar_t*  pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed);
#endif

/// Strlcpy
///
/// Implements a generic STL or EASTL 8 <--> 16 conversion via Strlcpy.
///
/// Example usage:
///    eastl::string8  s8("hello world");
///    eastl::string16 s16;
///    EA::StdC::Strlcpy(s16, s8);
///
namespace Internal
{
	// We don't have visibility to EASTL for its type traits, and std <type_traits> isn't necessarily available.
	template <typename T> struct is_pointer                    { static const bool value = false; };
	template <typename T> struct is_pointer<T*>                { static const bool value = true;  };
	template <typename T> struct is_pointer<T* const>          { static const bool value = true;  };
	template <typename T> struct is_pointer<T* volatile>       { static const bool value = true;  };
	template <typename T> struct is_pointer<T* const volatile> { static const bool value = true;  };

	template<typename T>           struct is_array       { static const bool value = false; };
	template<typename T>           struct is_array<T[]>  { static const bool value = true; };
	template<typename T, size_t N> struct is_array<T[N]> { static const bool value = true; };



	template <typename Dest, typename Source, bool isPointer>
	struct Strlcpy1Class;

	template <typename Dest, typename Source>
	struct Strlcpy1Class<Dest, Source, false>
	{
		static bool Strlcpy1Impl(Dest& d, const Source& s)
		{
			const int requiredStrlen = EA::StdC::Strlcpy(&d[0], s.data(), 0, s.length());

			if(requiredStrlen >= 0)
			{
				d.resize((typename Dest::size_type)requiredStrlen);
				EA::StdC::Strlcpy(&d[0], s.data(), d.length() + 1, s.length());

				return true;
			}

			d.clear();
			return false;
		}
	};

	template <typename T> // Specialization for the case that source and dest are equal.
	struct Strlcpy1Class<T, T, false>
	{
		static bool Strlcpy1Impl(T& d, const T& s)
			{ d = s; return true; }
	};

	template <typename Dest, typename Source> // Specialization for the case that Source is a pointer and not a class
	struct Strlcpy1Class<Dest, Source, true>
	{
		static bool Strlcpy1Impl(Dest& d, const Source& pSource)
		{
			const size_t sourceLength   = EA::StdC::Strlen(pSource);
			const int    requiredStrlen = EA::StdC::Strlcpy(&d[0], pSource, 0, sourceLength);

			if(requiredStrlen >= 0)
			{
				d.resize((typename Dest::size_type)requiredStrlen);
				EA::StdC::Strlcpy(&d[0], pSource, d.length() + 1, sourceLength);

				return true;
			}

			d.clear();
			return false;
		}
	};
}

template <typename Dest, typename Source>
inline bool Strlcpy(Dest& d, const Source& s)
{
	return Internal::Strlcpy1Class<Dest, Source, Internal::is_pointer<Source>::value || Internal::is_array<Source>::value>::Strlcpy1Impl(d, s);
}


/// Strlcpy
///
/// Implements a generic STL/EASTL 8 <--> 16 conversion via Strlcpy.
///
/// Example usage:
///    eastl::string16 s16;
///    EA::StdC::Strlcpy(s16, "hello world");
///
namespace Internal
{
	template <typename Dest, typename DestCharType, typename Source>
	struct Strlcpy2Class
	{
		static bool Strlcpy2Impl(Dest& d, const Source* pSource, size_t sourceLength)
		{
			const int requiredStrlen = EA::StdC::Strlcpy(&d[0], pSource, 0, sourceLength);

			if(requiredStrlen >= 0)
			{
				d.resize((typename Dest::size_type)requiredStrlen);
				EA::StdC::Strlcpy(&d[0], pSource, d.length() + 1, sourceLength);

				return true;
			}

			d.clear();
			return false;
		}
	};

	template <typename Dest, typename CharType> // Specialization for the case that source and dest are equal.
	struct Strlcpy2Class<Dest, CharType, CharType>
	{
		static bool Strlcpy2Impl(Dest& d, const CharType* pSource, size_t sourceLength)
			{ d.assign(pSource, (typename Dest::size_type)sourceLength); return true; }
	};
}

template <typename Dest, typename Source>
inline bool Strlcpy(Dest& d, const Source* pSource, size_t sourceLength = kSizeTypeUnset)
{
	if(sourceLength == kSizeTypeUnset)
		sourceLength = Strlen(pSource);

	return Internal::Strlcpy2Class<Dest, typename Dest::value_type, Source>::Strlcpy2Impl(d, pSource, sourceLength);
}



/// Strlcpy
///
/// Implements a generic STL/EASTL 8 <--> 16 conversion via Strlcpy.
///
/// Example usage:
///    eastl::string8  s8("hello world");
///    eastl::string16 s16;
///
///    s16 = EA::StdC::Strlcpy<eastl::string16, eastl::string8>(s8);   // 8  -> 16
///    s8  = EA::StdC::Strlcpy<eastl::string8,  eastl::string16>(s16); // 16 ->  8
///    s16 = EA::StdC::Strlcpy<eastl::string16, eastl::string16>(s16); // 16 -> 16  (merely returns the source)
///    s8  = EA::StdC::Strlcpy<eastl::string8,  eastl::string8>(s8);   //  8 ->  8  (merely returns the source)
///
namespace Internal
{
	template <class Dest, class Source>
	struct Strlcpy3Class
	{
		static Dest Strlcpy3Impl(const Source& s)
		{
			Dest d;

			const int requiredStrlen = EA::StdC::Strlcpy(&d[0], s.data(), 0, s.length());

			if(requiredStrlen >= 0)
			{
				d.resize((typename Dest::size_type)requiredStrlen);
				EA::StdC::Strlcpy(&d[0], s.data(), d.length() + 1, s.length());
			}

			return d;
		}
	};

	template <class T> // Specialization for the case that source and dest are equal.
	struct Strlcpy3Class<T, T>
	{
		static T Strlcpy3Impl(const T& s)
			{ return s; }
	};
}

template <typename Dest, typename Source>
inline Dest Strlcpy(const Source& s)
{
	return Internal::Strlcpy3Class<Dest, Source>::Strlcpy3Impl(s);
}


/// Strlcpy
///
/// Implements a generic STL/EASTL 8 <--> 16 conversion via Strlcpy.
///
/// Example usage:
///    eastl::string16 s16 = Strlcpy<eastl::string16, char>("hello world");
///
namespace Internal
{
	template <typename Dest, typename DestCharType, typename Source>
	struct Strlcpy4Class
	{
		static Dest Strlcpy4Impl(const Source* pSource, size_t sourceLength)
		{
			Dest d;

			const int requiredStrlen = EA::StdC::Strlcpy(&d[0], pSource, 0, sourceLength);

			if(requiredStrlen >= 0)
			{
				d.resize((typename Dest::size_type)requiredStrlen);
				EA::StdC::Strlcpy(&d[0], pSource, d.length() + 1, sourceLength);
			}

			return d;
		}
	};

	template <typename Dest, typename CharType> // Specialization for the case that source and dest are equal.
	struct Strlcpy4Class<Dest, CharType, CharType>
	{
		static Dest Strlcpy4Impl(const CharType* pSource, size_t sourceLength)
			{ return Dest(pSource, (typename Dest::size_type)sourceLength); }
	};
}

template <typename Dest, typename Source>
inline Dest Strlcpy(const Source* pSource, size_t sourceLength = kSizeTypeUnset)
{
	if(sourceLength == kSizeTypeUnset)
		sourceLength = Strlen(pSource);

	return Internal::Strlcpy4Class<Dest, typename Dest::value_type, Source>::Strlcpy4Impl(pSource, sourceLength);
}



/// Strcat
///
/// Appends pSource to the end of the string at pDestination.
/// Returns pDestination.
/// The terminating 0 char of destination is overwritten by the first 
/// character of source, and a new null-character is appended at the 
/// new end of the destination. The required capacity of the destination
/// is (Strlen(pSource) + Strlen(pDestination) + 1).
/// Considering using Strlcat as a safer alternative to Strcat.
///
EASTDC_API char*  Strcat(char*  pDestination, const char*  pSource);
EASTDC_API char16_t* Strcat(char16_t* pDestination, const char16_t* pSource);
EASTDC_API char32_t* Strcat(char32_t* pDestination, const char32_t* pSource);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strcat(wchar_t* pDestination, const wchar_t* pSource);
#endif


/// Strncat
///
/// Appends the first n chars from pSource to pDestination and 0-terminates pDestination. 
/// If the Strlen of pSource is less than n, only characters preceding the 0 char are copied.
/// Considering using Strlcat as a safer alternative to Strncat.
/// This function acts the same as strncat
///
EASTDC_API char*  Strncat(char*  pDestination, const char*  pSource, size_t n);
EASTDC_API char16_t* Strncat(char16_t* pDestination, const char16_t* pSource, size_t n);
EASTDC_API char32_t* Strncat(char32_t* pDestination, const char32_t* pSource, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strncat(wchar_t* pDestination, const wchar_t* pSource, size_t n);
#endif


/// StringnCat
///
/// This function is identical to the rwstdc package StringnCat function and is 
/// provided for compatibility with it. This equivalence extends to any unintended
/// bugs that may be in the StringnCopy function.
/// Users are advised to use Strlcat instead of Strncat or StringnCat.
///
EASTDC_API char*  StringnCat(char*  pDestination, const char*  pSource, size_t n);
EASTDC_API char16_t* StringnCat(char16_t* pDestination, const char16_t* pSource, size_t n);
EASTDC_API char32_t* StringnCat(char32_t* pDestination, const char32_t* pSource, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* StringnCat(wchar_t* pDestination, const wchar_t* pSource, size_t n);
#endif


/// Strlcat
///
/// The strlcpy and strlcat functions copy and concatenate strings
/// respectively. They are designed to be safer, more consistent, 
/// and less error prone replacements for strncpy and strncat. Unlike those
/// functions, strlcpy and strlcat take the full capacity of the buffer (not
/// just the length) and guarantee to 0-terminate the result (as long as
/// nDestCapacity is larger than 0 or, in the case of strlcat, as long as there is
/// at least one byte free in dst). Note that you should include a byte for
/// the 0 in the input nDestCapacity. Also note that strlcpy and strlcat only operate 
/// on true C strings. This means that for strlcpy pSource must be 0-terminated
/// and for strlcat both pSource and pDest must be 0-terminated.
/// 
/// The strlcpy function copies up to (nDestCapacity - 1) characters from the 
/// 0-terminated string pSource to pDest, 0-terminating the result.
/// 
/// The strlcat function appends the 0-terminated string src to the end
/// of dst. It will append at most (nDestCapacity - strlen(dst) - 1) characters, 
/// 0-terminating the result.
///
/// Users must be careful because Strlcpy and Strlcat may copy less than the required
/// characters. Unless you check the Strlcat return value, you could be trading one
/// kind of error (buffer overflow) with another (truncated string). In some cases
/// the truncated string may be significant to the point of creating security errors.
///
/// Return value
/// The strlcpy and strlcat functions return the strlen of the string they 
/// tried to create. For strlcpy that means the strlen of pSource.
/// For strlcat that means the initial strlen of pDest plus the strlen of pSource. 
/// While this may seem somewhat confusing, it makes truncation detection simple.
/// 
/// Note however, that if strlcat traverses nDestSize characters without finding
/// a 0, the length of the string is considered to be nDestCapacity and the destination
/// string will not be 0-terminated (since there was no space for the 0). This keeps 
/// strlcat from running off the end of a string. In practice this should not happen 
/// (as it means that either size is incorrect or that dst is not a proper C string). 
/// The check exists to prevent potential security problems in incorrect code.
///
/// The versions that implement 8 <->16 or 8<->32 conversions assume the 8 bit text
/// is UTF8 and they convert to and from UCS2 and UCS4 encodings respectivly. 
///
/// Example usage:
///  The following code fragment illustrates the simple case:
///      char *s, *p, buffer[BUFFERSIZE];
///      Strlcpy(buffer, s, EAArrayCount(buffer));
///      Strlcat(buffer, p, EAArrayCount(buffer));
///      
///  To detect truncation, perhaps while building a path, the following might be used:
///      char *dir, *file, path[_MAX_PATH];
///      if(Strlcpy(path, dir, EAArrayCount(path)) >= EAArrayCount(path))
///          goto toolong;
///      if(Strlcat(path, file, EAArrayCount(path)) >= EAArrayCount(path))
///          goto toolong;
///
EASTDC_API size_t Strlcat(char*     pDestination, const char*     pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcat(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcat(char32_t* pDestination, const char32_t* pSource, size_t nDestCapacity);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t Strlcat(wchar_t*  pDestination, const wchar_t*  pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(wchar_t*  pDestination, const char*     pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(wchar_t*  pDestination, const char16_t* pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(wchar_t*  pDestination, const char32_t* pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(char*     pDestination, const wchar_t*  pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(char16_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity);
	EASTDC_API size_t Strlcat(char32_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity);
#endif

// UTF8 <-> UTF16 <-> UTF32 encoding conversion 
EASTDC_API size_t Strlcat(char16_t* pDestination, const char*  pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcat(char*  pDestination, const char16_t* pSource, size_t nDestCapacity);

EASTDC_API size_t Strlcat(char32_t* pDestination, const char*  pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcat(char*  pDestination, const char32_t* pSource, size_t nDestCapacity);

EASTDC_API size_t Strlcat(char16_t* pDestination, const char32_t*  pSource, size_t nDestCapacity);
EASTDC_API size_t Strlcat(char32_t* pDestination, const char16_t*  pSource, size_t nDestCapacity);


/// Strlcat
///
/// Implements a generic STL/EASTL 8 <--> 16/32 conversion via Strlcpy.
///
/// Example usage:
///    eastl::wstring sW(L"hello ");
///    eastl::string8 s8("world");
///
///    EA::StdC::Strlcat(sW, s8);  // char -> wchar_t. Becomes L"hello world."
///
namespace Internal
{
	template <typename Dest, typename Source>
	struct Strlcat5Class
	{
		static bool Strlcat5Impl(Dest& d, const Source& s)
		{
			const int additionalLength = EA::StdC::Strlcpy(&d[0], s.data(), 0, s.length());

			if(additionalLength >= 0) // If there wasn't an encoding error in the source string...
			{
				const typename Dest::size_type originalLength = d.length();
				d.resize(originalLength + (typename Dest::size_type)additionalLength);
				EA::StdC::Strlcpy(&d[originalLength], s.data(), d.length() + 1, s.length());

				return true;
			}

			return false;
		}
	};

	template <typename T> // Specialization for the case that source and dest are equal.
	struct Strlcat5Class<T, T>
	{
		static bool Strlcat5Impl(T& d, const T& s)
			{ d += s; return true; }
	};
}

template <typename Dest, typename Source>
inline bool Strlcat(Dest& d, const Source& s)
{
	return Internal::Strlcat5Class<Dest, Source>::Strlcat5Impl(d, s);
}


/// Strlcat
///
/// Implements a generic STL/EASTL 8 <--> 16/32 conversion via Strlcpy.
///
/// Example usage:
///    eastl::wstring sW(L"hello ");
///    EA::StdC::Strlcat(sW, "world");  // char -> wchar_t. Becomes L"hello world."
///
namespace Internal
{
	template <typename Dest, typename DestCharType, typename Source>
	struct Strlcat6Class
	{
		static bool Strlcat6Impl(Dest& d, const Source* pSource, size_t sourceLength)
		{
			const int additionalLength = EA::StdC::Strlcpy(&d[0], pSource, 0, sourceLength);

			if(additionalLength >= 0) // If there wasn't an encoding error in the source string...
			{
				const typename Dest::size_type originalLength = d.length();
				d.resize(originalLength + (typename Dest::size_type)additionalLength);
				EA::StdC::Strlcpy(&d[originalLength], pSource, d.length() + 1, sourceLength);

				return true;
			}

			return false;
		}
	};

	template <typename Dest, typename CharType> // Specialization for the case that source and dest are equal.
	struct Strlcat6Class<Dest, CharType, CharType>
	{
		static bool Strlcat6Impl(Dest& d, const CharType* pSource, size_t sourceLength)
			{ d.append(pSource, (typename Dest::size_type)sourceLength); return true; }
	};
}

template <typename Dest, typename Source>
inline bool Strlcat(Dest& d, const Source* pSource, size_t sourceLength = kSizeTypeUnset)
{
	if(sourceLength == kSizeTypeUnset)
		sourceLength = Strlen(pSource);

	return Internal::Strlcat6Class<Dest, typename Dest::value_type, Source>::Strlcat6Impl(d, pSource, sourceLength);
}



/// Strxfrm
///
/// Copies the first n characters of pSource to pDest performing the 
/// appropiate transformations for the current locale settings if needed.
/// Returns the new Strlen of pDest. 
/// If pDest is NULL or n is zero, nothing is written to pDest and the return 
/// value is the required Strlen of pDest.
/// Since localization is not currently supported by this module, this function
/// doesn't do any transformation other than simply copy.
/// This is similar to the strxfrm C function.
///
EASTDC_API size_t Strxfrm(char*     pDest, const char*     pSource, size_t n);
EASTDC_API size_t Strxfrm(char16_t* pDest, const char16_t* pSource, size_t n);
EASTDC_API size_t Strxfrm(char32_t* pDest, const char32_t* pSource, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t Strxfrm(wchar_t* pDest, const wchar_t* pSource, size_t n);
#endif


/// Strdup
///
/// Duplicates a string, returning a pointer allocated by operator new[] and 
/// which needs to be deallocated by the user with delete[] or by Strdel.
/// If Strdup resides in a DLL or similar kind of independent library, Strldel is required.
/// This is similar to the strdup C function.
///
EASTDC_API char*  Strdup(const char*  pString);
EASTDC_API char16_t* Strdup(const char16_t* pString);
EASTDC_API char32_t* Strdup(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strdup(const wchar_t* pString);
#endif


/// Strdel
///
/// Deletes a string returned by Strdup.
///
EASTDC_API void Strdel(char*  pString);
EASTDC_API void Strdel(char16_t* pString);
EASTDC_API void Strdel(char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API void Strdel(wchar_t* pString);
#endif


/// Strupr
///
/// Converts an ASCII string to upper-case. Any characters that are outside
/// the ASCII set are not left as they are.
/// If you want Unicode-correct character and string functionality for all 
/// Unicode characters, use the EATextUnicode module from the EAText package. 
/// The Unicode module may be split off into a  unique package by the time you read this.
/// This is similar to the sometimes seen _strupr C function.
///
EASTDC_API char*  Strupr(char*  pString);
EASTDC_API char16_t* Strupr(char16_t* pString);
EASTDC_API char32_t* Strupr(char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strupr(wchar_t* pString);
#endif


/// Strlwr
///
/// Converts an ASCII string to lower-case. Any characters that are outside
/// the ASCII set are not left as they are.
/// If you want Unicode-correct character and string functionality for all 
/// Unicode characters, use the EATextUnicode module from the EAText package. 
/// The Unicode module may be split off into a  unique package by the time you read this.
/// This is similar to the sometimes seen _strlwr C function.
///
EASTDC_API char*  Strlwr(char*  pString);
EASTDC_API char16_t* Strlwr(char16_t* pString);
EASTDC_API char32_t* Strlwr(char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strlwr(wchar_t* pString);
#endif


/// Strmix
///
/// Copies the string from source to destination while converting it to mixed 
/// upper and lower case. The first letter to appear at the beginning of the 
/// string or after a character that is contained in separatorsPtr is encountered 
/// is capitalized while the rest is lower-cased. For example, if pSource is 
/// "aBcDe_fGhI*jKl+m", and if pDelimiters is "_+", pDestination becomes "Abcde_Fghi*jkl+M".
/// pDelimiters is a string containing characters acting as separators to mark that 
/// the next character is to be capitalized
/// Returns pDestination.
///
/// This function is currently provided for compatibility with the rwstdc package. 
///
EASTDC_API char*  Strmix(char*  pDestination, const char*  pSource, const char*  pDelimiters);
EASTDC_API char16_t* Strmix(char16_t* pDestination, const char16_t* pSource, const char16_t* pDelimiters);
EASTDC_API char32_t* Strmix(char32_t* pDestination, const char32_t* pSource, const char32_t* pDelimiters);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strmix(wchar_t* pDestination, const wchar_t* pSource, const wchar_t* pDelimiters);
#endif


/// Strchr
///
/// Returns a pointer to the first occurrence of c in pString, or NULL if the c is not found.
/// The null-terminating character is included as part of the string and can also be searched.
/// This is similar to the strchr C function.
///
EASTDC_API char*  Strchr(const char*  pString, int      c);
EASTDC_API char16_t* Strchr(const char16_t* pString, char16_t c);
EASTDC_API char32_t* Strchr(const char32_t* pString, char32_t c);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strchr(const wchar_t* pString, wchar_t c);
#endif

/// Strnchr
///
/// Returns a pointer to the first occurrence of c in pString, or NULL if the c is not found within the first n characters.
/// The search stops after n characters or at the end of the string, whichever comes first.
/// The null-terminating character is included as part of the string and can also be searched.
/// This differs from some implementations that let you search past the null-terminator of pString. If you'd like to do that, use EA::StdC::Memchr
///  Some other C library strnchr functions have the last two arguments switched relative to this version.
///
EASTDC_API char*  Strnchr(const char*  pString, int      c, size_t n);
EASTDC_API char16_t* Strnchr(const char16_t* pString, char16_t c, size_t n);
EASTDC_API char32_t* Strnchr(const char32_t* pString, char32_t c, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strnchr(const wchar_t* pString, wchar_t c);
#endif


/// Strcspn
///
/// Scans pString1 character by character, returning the number of chars read 
/// until the first occurrence of any character included in pString2. 
/// The search includes terminating null-characters, so the function will return 
/// the length of pString1 if none of the characters included in pString2 is in pString1.
/// This is similar to the strcspn C function.
///
EASTDC_API size_t  Strcspn(const char*  pString1, const char*  pString2);
EASTDC_API size_t  Strcspn(const char16_t* pString1, const char16_t* pString2);
EASTDC_API size_t  Strcspn(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t  Strcspn(const wchar_t* pString1, const wchar_t* pString2);
#endif


/// Strpbrk
///
/// Scans pString1 character by character, returning a pointer to the first 
/// character that matches with any of the characters in pString2. The search 
/// does not includes the terminating null characters.
/// This is similar to the strpbrk C function.
///
EASTDC_API char*  Strpbrk(const char*  pString1, const char*  pString2);
EASTDC_API char16_t* Strpbrk(const char16_t* pString1, const char16_t* pString2);
EASTDC_API char32_t* Strpbrk(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strpbrk(const wchar_t* pString1, const wchar_t* pString2);
#endif


/// Strrchr
///
/// Returns the last occurrence of c in pString, or NULL if the c is not found.
/// The null-terminating character is included as part of the string and can also be searched.
/// This is similar to the strrchr C function.
///
EASTDC_API char*  Strrchr(const char*  pString, int      c);
EASTDC_API char16_t* Strrchr(const char16_t* pString, char16_t c);
EASTDC_API char32_t* Strrchr(const char32_t* pString, char32_t c);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strrchr(const wchar_t* pString, wchar_t c);
#endif


/// Strspn
///
/// This is similar to the strspn C function.
///
EASTDC_API size_t Strspn(const char*  pString, const char*  pSubString);
EASTDC_API size_t Strspn(const char16_t* pString, const char16_t* pSubString);
EASTDC_API size_t Strspn(const char32_t* pString, const char32_t* pSubString);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t Strspn(const wchar_t* pString, const wchar_t* pSubString);
#endif


/// Strstr
///
/// Finds the first occurrence of pSubString within pString, exclusive of the 
/// terminating null character.
/// This is similar to the strstr C function.
/// Note: Like the C version this takes const char parameters but returns
/// non-const values. In C++ there are two versions: an all const version and 
/// an all non-const version. The C++ design is preferable, but switching to 
/// it could break users who expect a non-const return value from the const 
/// argument version.
/// 
EASTDC_API char*  Strstr(const char*  pString, const char*  pSubString);
EASTDC_API char16_t* Strstr(const char16_t* pString, const char16_t* pSubString);
EASTDC_API char32_t* Strstr(const char32_t* pString, const char32_t* pSubString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strstr(const wchar_t* pString, const wchar_t* pSubString);
#endif


/// Stristr
///
/// This is a case-insensitive version of Strstr.
/// Scans pString for the first occurrence of pSubString with case-insensitive 
/// comparison. The search does not include terminating null-characters.
/// The return value is char16_t* and not const char16_t* because that's how 
/// standard wcsstr works.
/// This is similar to the stristr and wcsstr C functions. See the notes for
/// those functions for some additional info.
///
EASTDC_API char*  Stristr(const char*  pString, const char*  pSubString);
EASTDC_API char16_t* Stristr(const char16_t* pString, const char16_t* pSubString);
EASTDC_API char32_t* Stristr(const char32_t* pString, const char32_t* pSubString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Stristr(const wchar_t* pString, const wchar_t* pSubString);
#endif


/// Strrstr
///
/// Finds the last occurrence of pSubString within pString, exclusive of the 
/// terminating null character.
/// This is similar to the strrstr C function. See the notes for
/// Strstr for some additional info.
///
EASTDC_API char*  Strrstr(const char*  pString, const char*  pSubString);
EASTDC_API char16_t* Strrstr(const char16_t* pString, const char16_t* pSubString);
EASTDC_API char32_t* Strrstr(const char32_t* pString, const char32_t* pSubString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strrstr(const wchar_t* pString, const wchar_t* pSubString);
#endif


/// Strirstr
///
/// Finds the last case insensitive occurrence of pSubString within pString, 
/// exclusive of the terminating null character.
/// This is similar to the strirstr C function. See the notes for
/// Strstr for some additional info.
///
EASTDC_API char*  Strirstr(const char*  pString, const char*  pSubString);
EASTDC_API char16_t* Strirstr(const char16_t* pString, const char16_t* pSubString);
EASTDC_API char32_t* Strirstr(const char32_t* pString, const char32_t* pSubString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strirstr(const wchar_t* pString, const wchar_t* pSubString);
#endif


/// Strstart
///
/// Returns true if pString begins with pPrefix.
/// Both strings must be non-null, but either can be empty.
/// An empty pPrefix results in success, regardless of what pString is.
///
EASTDC_API bool Strstart(const char* pString, const char* pPrefix);
EASTDC_API bool Strstart(const char16_t* pString, const char16_t* pPrefix);
EASTDC_API bool Strstart(const char32_t* pString, const char32_t* pPrefix);
#if EA_WCHAR_UNIQUE
	EASTDC_API bool Strstart(const wchar_t* pString, const wchar_t* pPrefix);
#endif


/// Stristart
///
/// Returns true if pString begins with pPrefix, compared case-insensitively.
/// Both strings must be non-null, but either can be empty.
/// An empty pPrefix results in success, regardless of what pString is.
/// Supports ASCII case comparisons only.
///
EASTDC_API bool Stristart(const char* pString, const char* pPrefix);
EASTDC_API bool Stristart(const char16_t* pString, const char16_t* pPrefix);
EASTDC_API bool Stristart(const char32_t* pString, const char32_t* pPrefix);
#if EA_WCHAR_UNIQUE
	EASTDC_API bool Stristart(const wchar_t* pString, const wchar_t* pPrefix);
#endif


/// Strend
///
/// Returns true if pString's contents ends with pSuffix's contents.
/// Both strings must be non-null, but either can be empty.
/// An empty pSuffix results in success, regardless of what pString is.
/// This Strend differs from Strend(const char_t*) in that this one searches
/// for a substring as opposed to simply searching for the end of the string.
/// If stringLength is the default value then it will be calculated, resulting in 
/// slower execution than otherwise.
///
EASTDC_API bool Strend(const char* pString, const char* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);
EASTDC_API bool Strend(const char16_t* pString, const char16_t* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);
EASTDC_API bool Strend(const char32_t* pString, const char32_t* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);


/// Striend
///
/// Returns true if pString's contents ends with pSuffix's contents, compared case-insensitively.
/// Both strings must be non-null, but either can be empty.
/// An empty pSuffix results in success, regardless of what pString is.
/// If stringLength is the default value then it will be calculated, resulting in 
/// slower execution than otherwise.
/// Supports ASCII case comparisons only.
///
EASTDC_API bool Striend(const char* pString, const char* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);
EASTDC_API bool Striend(const char16_t* pString, const char16_t* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);
EASTDC_API bool Striend(const char32_t* pString, const char32_t* pSuffix, size_t stringLength = kSizeTypeUnset, size_t suffixLength = kSizeTypeUnset);


/// Strtok
///
/// Parses pString into tokens separated by characters in pDelimiters.
/// If pString is NULL, the saved pointer in pContext is used as
/// the next starting point. 
///
/// Note that this function is slightly different from strtok in that
/// it requires a user supplied context pointer. This context pointer
/// requirement is part of the C99 standard wcstok function and is
/// there for good reason: strtok is neither thread-safe nor re-entrant. 
/// The sometimes-seen strtok_r function is provided which rectifies
/// this problem. Strtok here acts like strtok_r or wcstok.
///
/// Note that this function modifies pString in place, as per the C strtok
/// convention. To avoid this use the Strtok2 function.
/// 
/// This is similar to wcstok and to the sometimes seen strtok_r 
/// function (as opposed to the strtok function).
///
/// Detailed specification:
/// The first call in the sequence has pString as its first argument, and is followed 
/// by calls with a null pointer as their first argument. The separator string pointed 
/// to by pDelimiters may be different from call to call. The first call in the 
/// sequence shall search the string pointed to by pString for the first character code 
/// that is not contained in the current separator string pointed to by pDelimiters. 
/// If no such character code is found, then there are no tokens in the string 
/// pointed to by pString and Strtok shall return a null pointer. If such a 
/// character code is found, it shall be the start of the first token.
/// The Strtok function shall then search from there for a character code that is 
/// contained in the current separator string. If no such character code is found, 
/// the current token extends to the end of the string pointed to by pString, and 
/// subsequent searches for a token shall return a null pointer. If such a character 
/// code is found, it shall be overwritten by a null character, which terminates the 
/// current token. The Strtok function shall save a pointer to the following character  
/// code, from which the next search for a token shall start. Each subsequent call, 
/// with a null pointer as the value of the first argument, shall start searching 
/// from pContext and behave as described above.
/// 
/// Example:
///     char* pContext = NULL;
///     char* pString  = "000\t000\n000\t000";
///     char* pResult;
///
///     while((pResult = Strtok(pContext ? NULL : pString, "\t\n", &pContext)) != NULL)
///         printf("%s\n", pResult);
///     
/// Example:
///    char* p = "-abc-=-def";
///    char* p1;
///    char* r;
///
///    r = Strtok8(p,    "-",  &p1);    // r = "abc", p1 = "=-def", p = "abc\0=-def"
///    r = Strtok8(NULL, "-=", &p1);    // r = "def", p1 = NULL,    p = "abc\0=-def"
///    r = Strtok8(NULL, "=",  &p1);    // r = NULL,  p1 = NULL,    p = "abc\0=-def"
/// 
EASTDC_API char*  Strtok(char*  pString, const char*  pDelimiters, char**  pContext);
EASTDC_API char16_t* Strtok(char16_t* pString, const char16_t* pDelimiters, char16_t** pContext);
EASTDC_API char32_t* Strtok(char32_t* pString, const char32_t* pDelimiters, char32_t** pContext);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strtok(wchar_t* pString, const wchar_t* pDelimiters, wchar_t** pContext);
#endif


/// Strtok2
///
/// Searches through pString while skipping any leading delimiters and returns
/// the pointer to the first token. The string length (which is possibly zero) 
/// of the resulting delimited string is returned in pResultLength. 
/// Returns NULL if pString is empty.
///
/// Unlike strtok, this function does not replace the following delimiter with 
/// a null character and thus leaves pString unmodified.
///
/// This function is identical to the rwstdc package GetFirstTokenInString function
/// when bFirst is true and identical to GetNextTokenInString when bFirst is false. 
/// This function has the potentially undesirable effect of skipping empty fields 
/// in the string; you may want to try using the parsing functions in EATextUtil.h 
/// in order to avoid this.
/// 
EASTDC_API const char*  Strtok2(const char*  pString, const char*  pDelimiters, size_t* pResultLength, bool bFirst);
EASTDC_API const char16_t* Strtok2(const char16_t* pString, const char16_t* pDelimiters, size_t* pResultLength, bool bFirst);
EASTDC_API const char32_t* Strtok2(const char32_t* pString, const char32_t* pDelimiters, size_t* pResultLength, bool bFirst);
#if EA_WCHAR_UNIQUE
	EASTDC_API const wchar_t* Strtok2(const wchar_t* pString, const wchar_t* pDelimiters, size_t* pResultLength, bool bFirst);
#endif


/// Strset
///
/// Sets all characters up to but not including the terminating 0 char in 
/// pString with the value c. The char version uses int instead of char
/// in order to be compatible with the C strset function.
/// Returns pString.
/// This is similar to the sometimes seen _strset C function.
///
EASTDC_API char*  Strset(char*  pString, int      c);
EASTDC_API char16_t* Strset(char16_t* pString, char16_t c);
EASTDC_API char32_t* Strset(char32_t* pString, char32_t c);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strset(wchar_t* pString, wchar_t c);
#endif


/// Strnset
///
/// Sets up to the first n bytes of pString to c. If n is greater than the 
/// Strlen of pString, the Strlen(pString) is used instead of n. 
/// This is similar to the sometimes seen _strnset C function.
///
EASTDC_API char*  Strnset(char*  pString, int c,      size_t n);
EASTDC_API char16_t* Strnset(char16_t* pString, char16_t c, size_t n);
EASTDC_API char32_t* Strnset(char32_t* pString, char32_t c, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strnset(wchar_t* pString, wchar_t c, size_t n);
#endif


/// Strrev
///
/// This is similar to the sometimes seen _strrev C function.
///
EASTDC_API char*  Strrev(char*  pString);
EASTDC_API char16_t* Strrev(char16_t* pString);
EASTDC_API char32_t* Strrev(char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strrev(wchar_t* pString);
#endif


/// Strstrip
///
/// Removes leading and trailing space, defined by the Isspace function.
/// The trailing space is removed by potentially modifying the end position 
/// of the string via writing '\0'. The leading space isn't removed, but rather
/// the first non-whitespace char in pString is returned. Thus beware that 
/// this function may return a pointer >= pString, and you cannot free that 
/// that pointer instead of pString.
/// This is similar to the sometimes seen strstrip C function.
///
EASTDC_API char*  Strstrip(char*  pString);
EASTDC_API char16_t* Strstrip(char16_t* pString);
EASTDC_API char32_t* Strstrip(char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Strstrip(wchar_t* pString);
#endif


/// Strcmp
///
/// This is similar to the strcmp C function.
///
EASTDC_API int Strcmp(const char*  pString1, const char*  pString2);
EASTDC_API int Strcmp(const char16_t* pString1, const char16_t* pString2);
EASTDC_API int Strcmp(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strcmp(const wchar_t* pString1, const wchar_t* pString2);
#endif
#if EA_CHAR8_UNIQUE
	inline int Strcmp(const char8_t* pString1, const char8_t* pString2)
		{ return Strcmp((const char*)pString1, (const char*)pString2); }
#endif


/// Strncmp
///
/// This is similar to the strncmp C function.
///
EASTDC_API int Strncmp(const char*  pString1, const char*  pString2, size_t n);
EASTDC_API int Strncmp(const char16_t* pString1, const char16_t* pString2, size_t n);
EASTDC_API int Strncmp(const char32_t* pString1, const char32_t* pString2, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strncmp(const wchar_t* pString1, const wchar_t* pString2, size_t n);
#endif


/// Stricmp
///
/// This is similar to the sometimes seen _stricmp or strcasecmp C function.
///
EASTDC_API int Stricmp(const char*  pString1, const char*  pString2);
EASTDC_API int Stricmp(const char16_t* pString1, const char16_t* pString2);
EASTDC_API int Stricmp(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Stricmp(const wchar_t* pString1, const wchar_t* pString2);
#endif


/// Strnicmp
///
/// This is similar to the sometimes seen _strnicmp or strncasecmp C function.
///
EASTDC_API int Strnicmp(const char*  pString1, const char*  pString2, size_t n);
EASTDC_API int Strnicmp(const char16_t* pString1, const char16_t* pString2, size_t n);
EASTDC_API int Strnicmp(const char32_t* pString1, const char32_t* pString2, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strnicmp(const wchar_t* pString1, const wchar_t* pString2, size_t n);
#endif


/// StrcmpAlnum
///
/// *** This function is deprecated. ***
///
/// This function is fairly crippled and may not be as useful as StrcmpNumeric.
/// Its sole purpose is for backward compatibility with older EA code.
///
/// Implements an alphanumeric string compare. Strings of integral decimal digits
/// are treated as unsigned integers instead of individual ascii characters. 
/// Thus a string of "abc100" is greater than a string of "abc20", whereas strcmp 
/// would say that it is less. Dashes in front of characters are not treated as 
/// sign characters. Leading zeroes in front of numbers are treated as part of 
/// the number. Numbers are always treated as base 10.
///
/// Example behaviour:
///    EATEST_VERIFY(StrcmpAlnum("",       "")        == 0);
///    EATEST_VERIFY(StrcmpAlnum("abc",    "abc")     == 0);
///    EATEST_VERIFY(StrcmpAlnum("a",      "b")        < 0);
///    EATEST_VERIFY(StrcmpAlnum("abc",    "abcd")     < 0);
///    EATEST_VERIFY(StrcmpAlnum("abcd",   "abc")      > 0);
///    EATEST_VERIFY(StrcmpAlnum("a",      "")         > 0);
///    EATEST_VERIFY(StrcmpAlnum("",       "1")        < 0);       // Verify numerics evaluate highest.
///    EATEST_VERIFY(StrcmpAlnum("abc",    "abd")      < 0);
///    EATEST_VERIFY(StrcmpAlnum("a",      "1")        < 0);
///    EATEST_VERIFY(StrcmpAlnum("%",      "1")        < 0);       // Verify numerics evaluate highest.
///    EATEST_VERIFY(StrcmpAlnum("103",    "12")       > 0);       // Verify that numeric compare is occuring.
///    EATEST_VERIFY(StrcmpAlnum("abc12a", "abc12b")   < 0);
///    EATEST_VERIFY(StrcmpAlnum("abc123", "abc12a")   > 0);
///    EATEST_VERIFY(StrcmpAlnum("abc-2",  "abc-1")    > 0);       // Verify that '-' is not treated as a minus sign.
///    EATEST_VERIFY(StrcmpAlnum("abc1.1", "abc1.02")  < 0);       // Verify that '.' is not treated as a decimal point.
///    EATEST_VERIFY(StrcmpAlnum("44",     "044")     == 0);       // Verify that digit spans are treated as base 10 numbers.
///
EASTDC_API int StrcmpAlnum(const char*  pString1, const char*  pString2);
EASTDC_API int StrcmpAlnum(const char16_t* pString1, const char16_t* pString2);
// No 32 bit version because this function is deprecated.

EASTDC_API int StricmpAlnum(const char*  pString1, const char*  pString2);
EASTDC_API int StricmpAlnum(const char16_t* pString1, const char16_t* pString2);
// No 32 bit version because this function is deprecated.


/// StrcmpNumeric
///
/// Compares number strings, including both integer and floating point numbers.
/// This is different from StrcmpAlnum because StrcmpAlnum just looks at digits
/// as integers and doesn't understand decimal points or exponents. A primary use
/// of this function is to save CPU cycles when comparing numeric strings, since
/// it would be significantly slower to convert strings to floating point values
/// and then do a floating point compare. Another use of this function is that it
/// avoids using the FPU and so can handle precisions that might not be available
/// of the user has the FPU precision set to a lower value. Numbers are always
/// treated as base-ten, even if they have leading zeroes.
/// If the string has a specified length and the string has a 0 char prior to that
/// length, the 0 char is not treated as a string terminator but is treated like
/// any other control character value.
/// For efficiency reasons, this code doesn't rigorously check to make sure numbers
/// are valid.
/// Virtual number strings such as "NaN" are not treated as numeric NaNs but are
/// treated as string characters. Normally NaN compared to a non-NaN is already 
/// unequal. But the situation where this function differs from numeric NaN
/// comparison is that numeric NaNs always compare against self as non-equal.
/// That doesn't work well in the world of integer and string logic, as it makes 
/// it difficult to have sensible hashed containers of such things.
///
/// Example behaviour:
///    EATEST_VERIFY(StrcmpNumeric("1",       "1")      == 0);
///    EATEST_VERIFY(StrcmpNumeric("12",      "13")      > 0);
///    EATEST_VERIFY(StrcmpNumeric("-1",      "1")       < 0);
///    EATEST_VERIFY(StrcmpNumeric("10.01",   "10.02")   < 0);
///    EATEST_VERIFY(StrcmpNumeric("-1.010",  "-1.001")  < 0);       // -1.010 is more negative than -1.010, and thus is lesser.
///
/// Basic alphabetic values are handled as well, like Strcmp.
///    EATEST_VERIFY(StrcmpNumeric("",       "")        == 0);
///    EATEST_VERIFY(StrcmpNumeric("abc",    "abc")     == 0);
///    EATEST_VERIFY(StrcmpNumeric("a",      "b")        < 0);
///    EATEST_VERIFY(StrcmpNumeric("abc",    "abcd")     < 0);
///    EATEST_VERIFY(StrcmpNumeric("abcd",   "abc")      > 0);
///    EATEST_VERIFY(StrcmpNumeric("a",      "")         > 0);
///    EATEST_VERIFY(StrcmpNumeric("",       "1")        < 0);       // Verify numerics evaluate highest.
///    EATEST_VERIFY(StrcmpNumeric("abc",    "abd")      < 0);
///    EATEST_VERIFY(StrcmpNumeric("a",      "1")        < 0);
///    EATEST_VERIFY(StrcmpNumeric("%",      "1")        < 0);       // Verify numerics evaluate highest.
///    EATEST_VERIFY(StrcmpNumeric("103",    "12")       > 0);       // Verify that numeric compare is occuring.
///    EATEST_VERIFY(StrcmpNumeric("abc12a", "abc12b")   < 0);
///    EATEST_VERIFY(StrcmpNumeric("abc123", "abc12a")   > 0);
///    EATEST_VERIFY(StrcmpNumeric("abc-2",  "abc-1")    < 0);
///    EATEST_VERIFY(StrcmpNumeric("abc1.1", "abc1.02")  > 0);
///    EATEST_VERIFY(StrcmpNumeric("44",     "044")     == 0);
///
EASTDC_API int StrcmpNumeric (const char*  pString1, const char*  pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char  decimal = '.', char  thousandsSeparator = ',');
EASTDC_API int StrcmpNumeric (const char16_t* pString1, const char16_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char16_t decimal = '.', char16_t thousandsSeparator = ',');
EASTDC_API int StrcmpNumeric (const char32_t* pString1, const char32_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char32_t decimal = '.', char32_t thousandsSeparator = ',');
#if EA_WCHAR_UNIQUE
	EASTDC_API int StrcmpNumeric (const wchar_t* pString1, const wchar_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, wchar_t decimal = '.', wchar_t thousandsSeparator = ',');
#endif

EASTDC_API int StricmpNumeric(const char*  pString1, const char*  pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char  decimal = '.', char  thousandsSeparator = ',');
EASTDC_API int StricmpNumeric(const char16_t* pString1, const char16_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char16_t decimal = '.', char16_t thousandsSeparator = ',');
EASTDC_API int StricmpNumeric(const char32_t* pString1, const char32_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, char32_t decimal = '.', char32_t thousandsSeparator = ',');
#if EA_WCHAR_UNIQUE
	EASTDC_API int StricmpNumeric(const wchar_t* pString1, const wchar_t* pString2, size_t length1 = kSizeTypeUnset, size_t length2 = kSizeTypeUnset, wchar_t decimal = '.', wchar_t thousandsSeparator = ',');
#endif


/// Strcoll
///
/// Compare two strings using the locale LC_COLLATE information.
/// In the C locale, Strcmp() is used to make the comparison.
/// However, this function is not localized and always uses the C locale. 
/// If you want Unicode-correct character and string functionality for all 
/// Unicode characters, use the EATextUnicode module from the EAText package. 
/// The Unicode module may be split off into a  unique package by the time you read this.
/// This is similar to the strcoll C function.
///
EASTDC_API int Strcoll(const char*  pString1, const char*  pString2);
EASTDC_API int Strcoll(const char16_t* pString1, const char16_t* pString2);
EASTDC_API int Strcoll(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strcoll(const wchar_t* pString1, const wchar_t* pString2);
#endif


/// Strncoll
///
/// If you want Unicode-correct character and string functionality for all 
/// Unicode characters, use the EATextUnicode module from the EAText package. 
/// The Unicode module may be split off into a  unique package by the time you read this.
/// This is similar to the sometimes seen _strncoll C function.
///
EASTDC_API int Strncoll(const char*  pString1, const char*  pString2, size_t n);
EASTDC_API int Strncoll(const char16_t* pString1, const char16_t* pString2, size_t n);
EASTDC_API int Strncoll(const char32_t* pString1, const char32_t* pString2, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strncoll(const wchar_t* pString1, const wchar_t* pString2, size_t n);
#endif


/// Stricoll
///
/// This is similar to the sometimes seen _stricoll C function.
///
EASTDC_API int Stricoll(const char*  pString1, const char*  pString2);
EASTDC_API int Stricoll(const char16_t* pString1, const char16_t* pString2);
EASTDC_API int Stricoll(const char32_t* pString1, const char32_t* pString2);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Stricoll(const wchar_t* pString1, const wchar_t* pString2);
#endif


/// Strnicoll
///
/// If you want Unicode-correct character and string functionality for all 
/// Unicode characters, use the EATextUnicode module from the EAText package. 
/// The Unicode module may be split off into a  unique package by the time you read this.
/// This is similar to the sometimes seen _strnicoll C function.
///
EASTDC_API int Strnicoll(const char*  pString1, const char*  pString2, size_t n);
EASTDC_API int Strnicoll(const char16_t* pString1, const char16_t* pString2, size_t n);
EASTDC_API int Strnicoll(const char32_t* pString1, const char32_t* pString2, size_t n);
#if EA_WCHAR_UNIQUE
	EASTDC_API int Strnicoll(const wchar_t* pString1, const wchar_t* pString2, size_t n);
#endif



/// EASTDC_NATIVE_FCVT / EASTDC_NATIVE_FCVT_SHORT
///
/// EASTDC_NATIVE_FCVT and EASTDC_NATIVE_FCVT_SHORT are defined as 0 or 1.
/// This is useful for defining if the provided standard library implements
/// a version of the fcvt and ecvt functions. A standard library version of
/// fcvt and ecvt may be faster than the one provided here.
/// EASTDC_NATIVE_FCVT is 1 if the compiler's C library provides ecvt/fcvt/gcvt.
/// EASTDC_NATIVE_FCVT_SHORT is 1 if the implementation limits the output string
/// to some internally defined limit as opposed to using the user-specified ndigits 
/// output string precision. 
///
#ifndef EASTDC_NATIVE_FCVT
	#if defined(EA_PLATFORM_MICROSOFT)
		#define EASTDC_NATIVE_FCVT       1
		#define EASTDC_NATIVE_FCVT_SHORT 0
	#elif defined(EA_COMPILER_GNUC) && (defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_WINDOWS)) && !defined(_BSD_SIZE_T_DEFINED_) && !defined(__APPLE__) // BSD-based (as opposed to glibc-based) standard libraries don't support fcvt. Apple has a fcvt, but it is broken and our unit tests fail when using it.
		#define EASTDC_NATIVE_FCVT       1
		#define EASTDC_NATIVE_FCVT_SHORT 1 // GCC's standard library will only write 16 chars, even if you pass a higher ndigits value.
	#else
		#define EASTDC_NATIVE_FCVT       0
		#define EASTDC_NATIVE_FCVT_SHORT 0
	#endif
#endif



/// EcvtBuf
///
/// These functions convert a floating point value to a string and decimal point information.
/// They are most useful for constucting localized formatted numbers. These are similar to 
/// the ecvt() and fcvt() functions but are thread-safe due to having the user specify the buffer.
/// The supplied buffer must have at least 350 characters of capacity, which could hold any 
/// double represented as a decimal string.
///
/// nDigitCount Must be > 0, pDecimalPos must be valid, pSign must be valid, pBuffer must be valid.
///
const int kEcvtBufMaxSize = 350;

EASTDC_API char*  EcvtBuf(double dValue, int nDigitCount, int* pDecimalPos, int* pSign, char*  pBuffer);
EASTDC_API char16_t* EcvtBuf(double dValue, int nDigitCount, int* pDecimalPos, int* pSign, char16_t* pBuffer);
EASTDC_API char32_t* EcvtBuf(double dValue, int nDigitCount, int* pDecimalPos, int* pSign, char32_t* pBuffer);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* EcvtBuf(double dValue, int nDigitCount, int* pDecimalPos, int* pSign, wchar_t* pBuffer);
#endif


/// FcvtBuf
///
/// These functions convert a floating point value to a string and decimal point information.
/// They are most useful for constucting localized formatted numbers. These are similar to 
/// the ecvt() and fcvt() functions but are thread-safe due to having the user specify the buffer.
/// The supplied buffer must have at least 350 characters of capacity, which could hold any 
/// double represented as a decimal string.
///
/// nDigitCountAfterDecimal Must be > 0, pDecimalPos must be valid, pSign must be valid, pBuffer must be valid.
///
const int kFcvtBufMaxSize = 350;

EASTDC_API char*  FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* pDecimalPos, int* pSign, char*  pBuffer);
EASTDC_API char16_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* pDecimalPos, int* pSign, char16_t* pBuffer);
EASTDC_API char32_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* pDecimalPos, int* pSign, char32_t* pBuffer);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* pDecimalPos, int* pSign, wchar_t* pBuffer);
#endif



//const int kInt8MinCapacity   =  5;
//const int kUint8MinCapacity  =  4;
//const int kInt16MinCapacity  =  7;
//const int kUint16MinCapacity =  6;
const int kInt32MinCapacity  = 12;
const int kUint32MinCapacity = 11;
const int kInt64MinCapacity  = 21;
const int kUint64MinCapacity = 21;


/// I32toa
///
/// Buffer must hold at least kInt32MinCapacity (12) chars.
///
EASTDC_API char*  I32toa(int32_t nValue, char*  pBuffer, int nBase);
EASTDC_API char16_t* I32toa(int32_t nValue, char16_t* pBuffer, int nBase);
EASTDC_API char32_t* I32toa(int32_t nValue, char32_t* pBuffer, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* I32toa(int32_t nValue, wchar_t* pBuffer, int nBase);
#endif


/// U32toa
///
/// Buffer must hold at least kUint32MinCapacity (11) chars.
///
EASTDC_API char*  U32toa(uint32_t nValue, char*  pBuffer, int nBase);
EASTDC_API char16_t* U32toa(uint32_t nValue, char16_t* pBuffer, int nBase);
EASTDC_API char32_t* U32toa(uint32_t nValue, char32_t* pBuffer, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* U32toa(uint32_t nValue, wchar_t* pBuffer, int nBase);
#endif


/// I64toa
///
/// Buffer must hold at least kInt64MinCapacity (21) chars.
///
EASTDC_API char*  I64toa(int64_t nValue, char*  pBuffer, int nBase);
EASTDC_API char16_t* I64toa(int64_t nValue, char16_t* pBuffer, int nBase);
EASTDC_API char32_t* I64toa(int64_t nValue, char32_t* pBuffer, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* I64toa(int64_t nValue, wchar_t* pBuffer, int nBase);
#endif


/// U64toa
///
/// Buffer must hold at least kUint64MinCapacity (21) chars.
///
EASTDC_API char*  U64toa(uint64_t nValue, char*  pBuffer, int nBase);
EASTDC_API char16_t* U64toa(uint64_t nValue, char16_t* pBuffer, int nBase);
EASTDC_API char32_t* U64toa(uint64_t nValue, char32_t* pBuffer, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* U64toa(uint64_t nValue, wchar_t* pBuffer, int nBase);
#endif


/// Strtod
///
/// Converts pString to a floating point value.
/// The function first discards leading whitespace chars until the first 
/// non-whitespace char is found. It then takes as many chars as possible 
/// that are valid in the representation of a floating point literal and 
/// interprets them as a floating point double. Any remaining chars
/// are ignored. 
/// 
/// A valid floating point value is formed by a succession of:
///    - An minus sign or optional plus sign.
///    - A sequence of decimal digits, optionally containing a decimal-point character.
///    - An optional exponent which consists of an 'e' or 'E' char followed 
///      by a minus sign or optional plus sign and a sequence of decimal digits.
///
/// If successful, the function returns the resulting floating point double.
/// If no valid conversion could be performed or if the correct value would 
/// be too small to be representable, a value of zero is returned.
/// If the correct value is positively or negatively beyond of the range of 
/// representable values, a positive or negative HUGE_VAL is returned.
///
/// If the first sequence of non-whitespace chars in pString does not 
/// form a valid floating-point number or is empty or only whitespace
/// no conversion is done and a value of zero is returned.
///
/// This function has the same effect as Strtod(pString, NULL);
///
EASTDC_API double Strtod(const char*  pString, char**  ppStringEnd);
EASTDC_API double Strtod(const char16_t* pString, char16_t** ppStringEnd);
EASTDC_API double Strtod(const char32_t* pString, char32_t** ppStringEnd);
#if EA_WCHAR_UNIQUE
	EASTDC_API double Strtod(const wchar_t* pString, wchar_t** ppStringEnd);
#endif

EASTDC_API float StrtoF32(const char*  pString, char**  ppStringEnd);
EASTDC_API float StrtoF32(const char16_t* pString, char16_t** ppStringEnd);
EASTDC_API float StrtoF32(const char32_t* pString, char32_t** ppStringEnd);
#if EA_WCHAR_UNIQUE
	EASTDC_API float StrtoF32(const wchar_t* pString, wchar_t** ppStringEnd);
#endif

/// StrtodEnglish
///
/// Implements a version of Strtod that interprets numbers 
/// as English regardless of what the C runtime library language
/// setting is. This is useful for being able to interpret numbers
/// in a constant way with respect to decimal point usage, etc.
///
EASTDC_API double StrtodEnglish(const char*  pString, char**  ppStringEnd);
EASTDC_API double StrtodEnglish(const char16_t* pString, char16_t** ppStringEnd);
EASTDC_API double StrtodEnglish(const char32_t* pString, char32_t** ppStringEnd);
#if EA_WCHAR_UNIQUE
	EASTDC_API double StrtodEnglish(const wchar_t* pString, wchar_t** ppStringEnd);
#endif

EASTDC_API float  StrtoF32English(const char*  pString, char**  ppStringEnd);
EASTDC_API float  StrtoF32English(const char16_t* pString, char16_t** ppStringEnd);
EASTDC_API float  StrtoF32English(const char32_t* pString, char32_t** ppStringEnd);
#if EA_WCHAR_UNIQUE
	EASTDC_API float  StrtoF32English(const wchar_t* pString, wchar_t** ppStringEnd);
#endif


/// StrtoI32
///
/// It is similar to the C strtol function.
///
EASTDC_API int32_t StrtoI32(const char*  pString, char**  ppStringEnd, int nBase);
EASTDC_API int32_t StrtoI32(const char16_t* pString, char16_t** ppStringEnd, int nBase);
EASTDC_API int32_t StrtoI32(const char32_t* pString, char32_t** ppStringEnd, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API int32_t StrtoI32(const wchar_t* pString, wchar_t** ppStringEnd, int nBase);
#endif


/// StrtoU32
///
/// It is similar to the C strtoul function.
///
EASTDC_API uint32_t StrtoU32(const char*  pString, char**  ppStringEnd, int nBase);
EASTDC_API uint32_t StrtoU32(const char16_t* pString, char16_t** ppStringEnd, int nBase);
EASTDC_API uint32_t StrtoU32(const char32_t* pString, char32_t** ppStringEnd, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API uint32_t StrtoU32(const wchar_t* pString, wchar_t** ppStringEnd, int nBase);
#endif


/// StrtoI64
///
/// It is similar to the C strtoll function.
///
/// StrtoI64 expects pString to point to a string of the following form:
///    [whitespace] [{+ | -}] [0 [{ x | X }]] [digits]
///
/// A whitespace may consist of space and tab characters, which are ignored; digits 
/// are one or more decimal digits. The first character that does not fit this form 
/// stops the scan. If base is between 2 and 36, then it is used as the base of the number. 
/// If nBase is 0, the initial characters of the string pointed to by pString are used 
/// to determine the base. If the first character is 0 and the second character 
/// is not 'x' or 'X', the string is interpreted as an octal integer; otherwise, 
/// it is interpreted as a decimal number. If the first character is '0' and the 
/// second character is 'x' or 'X', the string is interpreted as a hexadecimal integer. 
/// If the first character is '1' through '9', the string is interpreted as a decimal integer. 
/// The letters 'a' through 'z' (or 'A' through 'Z') are assigned the values 10 
/// through 35; only letters whose assigned values are less than base are permitted. 
/// StrtoI64 allows a plus (+) or minus (-) sign prefix; a leading minus sign indicates 
/// that the return value is negated.
///
EASTDC_API int64_t StrtoI64(const char*  pString, char**  ppStringEnd, int nBase);
EASTDC_API int64_t StrtoI64(const char16_t* pString, char16_t** ppStringEnd, int nBase);
EASTDC_API int64_t StrtoI64(const char32_t* pString, char32_t** ppStringEnd, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API int64_t StrtoI64(const wchar_t* pString, wchar_t** ppStringEnd, int nBase);
#endif


/// StrtoU64
///
/// It is similar to the C strtoull function.
///
EASTDC_API uint64_t StrtoU64(const char*  pString, char**  ppStringEnd, int nBase);
EASTDC_API uint64_t StrtoU64(const char16_t* pString, char16_t** ppStringEnd, int nBase);
EASTDC_API uint64_t StrtoU64(const char32_t* pString, char32_t** ppStringEnd, int nBase);
#if EA_WCHAR_UNIQUE
	EASTDC_API uint64_t StrtoU64(const wchar_t* pString, wchar_t** ppStringEnd, int nBase);
#endif


/// AtoI32
///
/// This function has the same effect as StrtoI32(pString, NULL, 10);
/// It is similar to the C atoll function.
/// 
EASTDC_API int32_t AtoI32(const char*  pString);
EASTDC_API int32_t AtoI32(const char16_t* pString);
EASTDC_API int32_t AtoI32(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API int32_t AtoI32(const wchar_t* pString);
#endif


/// AtoU32
///
/// This function has the same effect as StrtoU32(pString, NULL, 10);
/// It is similar to the C atoul function.
/// 
EASTDC_API uint32_t AtoU32(const char*  pString);
EASTDC_API uint32_t AtoU32(const char16_t* pString);
EASTDC_API uint32_t AtoU32(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API uint32_t AtoU32(const wchar_t* pString);
#endif

/// AtoI64
///
/// This function has the same effect as StrtoI64(pString, NULL, 10);
/// It is similar to the C atoll function.
/// 
EASTDC_API int64_t AtoI64(const char*  pString);
EASTDC_API int64_t AtoI64(const char16_t* pString);
EASTDC_API int64_t AtoI64(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API int64_t AtoI64(const wchar_t* pString);
#endif


/// AtoU64
///
/// This function has the same effect as StrtoU64(pString, NULL, 10);
/// It is similar to the C atoull function.
/// 
EASTDC_API uint64_t AtoU64(const char*  pString);
EASTDC_API uint64_t AtoU64(const char16_t* pString);
EASTDC_API uint64_t AtoU64(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API uint64_t AtoU64(const wchar_t* pString);
#endif



/// Atof
///
/// This function has the same effect as Strtod(pString, NULL);
/// It is similar to the C strtod function.
/// 
/// Converts pString to a floating point value.
/// The function first discards leading whitespace chars until the first 
/// non-whitespace char is found. It then takes as many chars as possible 
/// that are valid in the representation of a floating point literal and 
/// interprets them as a floating point double. Any remaining chars
/// are ignored. Use the Strtod function if you want to find out where 
/// the remaining chars begin.
/// 
/// A valid floating point value is formed by a succession of:
///    - An minus sign or optional plus sign.
///    - A sequence of decimal digits, optionally containing a decimal-point character.
///    - An optional exponent which consists of an 'e' or 'E' char followed 
///      by a minus sign or optional plus sign and a sequence of decimal digits.
///
/// If successful, the function returns the resulting floating point double.
/// If no valid conversion could be performed or if the correct value would 
/// be too small to be representable, a value of zero is returned.
/// If the correct value is positively or negatively beyond of the range of 
/// representable values, a positive or negative HUGE_VAL is returned.
///
/// If the first sequence of non-whitespace chars in pString does not 
/// form a valid floating-point number or is empty or only whitespace
/// no conversion is done and a value of zero is returned.
///
/// This function has the same effect as Strtod(pString, NULL);
///
EASTDC_API double Atof(const char*  pString);
EASTDC_API double Atof(const char16_t* pString);
EASTDC_API double Atof(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API double Atof(const wchar_t* pString);
#endif

EASTDC_API float  AtoF32(const char*  pString);
EASTDC_API float  AtoF32(const char16_t* pString);
EASTDC_API float  AtoF32(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API float  AtoF32(const wchar_t* pString);
#endif


/// AtofEnglish
///
/// This function has the same effect as Strtod(pString, NULL);
/// It is similar to the C strtod function.
/// 
/// Implements a version of Atof that interprets numbers as English regardless of 
/// what the C runtime library language setting is. This is useful for being able to 
/// interpret numbers in a constant way with respect to decimal point usage, etc.
/// 
EASTDC_API double AtofEnglish(const char*  pString);
EASTDC_API double AtofEnglish(const char16_t* pString);
EASTDC_API double AtofEnglish(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API double AtofEnglish(const wchar_t* pString);
#endif

EASTDC_API float  AtoF32English(const char*  pString);
EASTDC_API float  AtoF32English(const char16_t* pString);
EASTDC_API float  AtoF32English(const char32_t* pString);
#if EA_WCHAR_UNIQUE
	EASTDC_API float  AtoF32English(const wchar_t* pString);
#endif


/// Ftoa
///
/// Same as FtoaEnglish.
/// Similar but not identical to the non-standard but sometimes seen C ftoa function 
/// except it assumes English language numeric representation (e.g. decimal point 
/// represented by a period).
/// 
/// The nResultCapacity argument is number of characters in the destination, 
/// including the char that will hold the terminating 0 char.
///
/// The precision argument is the number of digits to return after the 
/// decimal point, if there is one. Trailing zeroes after a decimal point 
/// aren't added to the string, even if you request them.
/// 
/// pResult should generally have at least 32 characters to deal with all 
/// floating point values.
///
EASTDC_API char*  Ftoa(double dValue, char*  pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
EASTDC_API char16_t* Ftoa(double dValue, char16_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
EASTDC_API char32_t* Ftoa(double dValue, char32_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* Ftoa(double dValue, wchar_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
#endif


/// FtoaEnglish
///
/// Implements a version of ftoa which interprets numbers as English regardless of 
/// what the C runtime library language setting is. This is useful for being able to 
/// interpret numbers in a constant way with respect to decimal point usage, etc.
/// This function will often be significantly faster than the C ftoa function.
/// 
/// The nResultCapacity argument is number of characters in the destination, 
/// including the char that will hold the terminating 0 char.
///
/// The precision argument is the number of digits to return after the 
/// decimal point, if there is one. Trailing zeroes after a decimal point 
/// aren't added to the string, even if you request them.
///
/// Returns pResult if there was enough space in the input buffer to hold the 
/// resulting string. If the return value is NULL, pResult will hold the
/// numerical string value built left-to-right before space was exhausted. 
/// 
/// pResult should generally have at least 32 characters to deal with all 
/// floating point values.
///
/// Examples:
///     FtoaEnglish(10000, pResult, 1, 0, false);
///     returns NULL
///
///     FtoaEnglish(10000, pResult, 10, 0, false);
///     returns pResult and "10000"
///
///     FtoaEnglish(10000.003, pResult, 10, 0, false);
///     returns pResult and "10000"
///
///     FtoaEnglish(10000.003, pResult, 6, 10, false);
///     returns NULL
///
///     FtoaEnglish(10000.12345, pResult, 20, 3, false);
///     returns pResult and "10000.123"
///
///     FtoaEnglish(10000.12348, pResult, 20, 8, false);
///     returns pResult and "10000.12348"
///
///     FtoaEnglish(.12345, pResult, 20, 2, false);
///     returns pResult and "0.12"
///
///     FtoaEnglish(.123450000, pResult, 20, 20, false);
///     returns pResult and "0.12345"
///
EASTDC_API char*  FtoaEnglish(double dValue, char*  pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
EASTDC_API char16_t* FtoaEnglish(double dValue, char16_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
EASTDC_API char32_t* FtoaEnglish(double dValue, char32_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
#if EA_WCHAR_UNIQUE
	EASTDC_API wchar_t* FtoaEnglish(double dValue, wchar_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled);
#endif


/// ReduceFloatString
///
/// This function removes unnecessary floating point digits from a string.
/// It is useful for the creation of minumum size floating point strings for  
/// the case of reducing memory requirements of floating point representation. 
///
/// The input 'nLength' is the length of the string as would be returned by Strlen, 
/// but it can be less. If nLength is (size_t)-1, then pString must be 0-terminated.
/// The return value is the new Strlen of the string. 
///
/// Given a floating point number string like this:
///    "3.000000"
/// There is no reason this can't simply be represented as this:
///    "3"
///
/// This function determines of such a shortening can take place, and if so, 
/// it modifies the buffer (if needed) and returns the appropriate length. 
/// There are other conditions that we need to test for, such as numbers like this:
///    "2.3400"          -> "2.34"
///    "00000"           -> "0"
///    ".0000"           -> "0"
///    "000.0000"        -> "0"
///    "-0.0000"         -> "-0"
///    "23.430000e10"    -> "23.43e10"
///
/// We also deal with valid numbers like this:
///    "2.34001"
///    "0.00001"
///    "-0.00001"
///    "23.43e10"
///    "15e-0"
///
EASTDC_API size_t ReduceFloatString(char*  pString, size_t nLength = kSizeTypeUnset);
EASTDC_API size_t ReduceFloatString(char16_t* pString, size_t nLength = kSizeTypeUnset);
EASTDC_API size_t ReduceFloatString(char32_t* pString, size_t nLength = kSizeTypeUnset);
#if EA_WCHAR_UNIQUE
	EASTDC_API size_t ReduceFloatString(wchar_t* pString, size_t nLength = kSizeTypeUnset);
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

	inline EASTDC_API int32_t AtoI32(const char* pString)
	{
		return StrtoI32(pString, NULL, 10);
	}

	inline EASTDC_API int32_t AtoI32(const char16_t* pString)
	{
		return StrtoI32(pString, NULL, 10);
	}

	inline EASTDC_API int32_t AtoI32(const char32_t* pString)
	{
		return StrtoI32(pString, NULL, 10);
	}



	inline EASTDC_API uint32_t AtoU32(const char* pString)
	{
		return StrtoU32(pString, NULL, 10);
	}

	inline EASTDC_API uint32_t AtoU32(const char16_t* pString)
	{
		return StrtoU32(pString, NULL, 10);
	}

	inline EASTDC_API uint32_t AtoU32(const char32_t* pString)
	{
		return StrtoU32(pString, NULL, 10);
	}



	inline EASTDC_API int64_t AtoI64(const char* pString)
	{
		return StrtoI64(pString, NULL, 10);
	}

	inline EASTDC_API int64_t AtoI64(const char16_t* pString)
	{
		return StrtoI64(pString, NULL, 10);
	}

	inline EASTDC_API int64_t AtoI64(const char32_t* pString)
	{
		return StrtoI64(pString, NULL, 10);
	}



	inline EASTDC_API uint64_t AtoU64(const char*  pString)
	{
		return StrtoU64(pString, NULL, 10);
	}

	inline EASTDC_API uint64_t AtoU64(const char16_t* pString)
	{
		return StrtoU64(pString, NULL, 10);
	}

	inline EASTDC_API uint64_t AtoU64(const char32_t* pString)
	{
		return StrtoU64(pString, NULL, 10);
	}



	inline EASTDC_API double Strtod(const char* pString, char** ppStringEnd)
	{
		return strtod(pString, ppStringEnd); // strtod is well-implemented by most standard libraries.
	}

	inline EASTDC_API double Strtod(const char16_t* pString, char16_t** ppStringEnd)
	{
		#if (EA_WCHAR_SIZE == 2) && defined(EA_HAVE_WCHAR_IMPL)
			return wcstod(reinterpret_cast<const wchar_t*>(pString), reinterpret_cast<wchar_t**>(reinterpret_cast<void*>(ppStringEnd)));
		#else
			// EA_PLATFORM_PLAYSTATION2, __CYGWIN__
			// These platforms don't have wide string support.

			// EA_PLATFORM_UNIX
			// Unix defines wchar_t to be 32 bit. We compile under GCC/Unix with the -fshort-wchar argument
			// which makes wchar_t be 16 bit. But the GCC standard library linked in is still the one with 
			// 32 bit wchar_t. So we can't call wcstod here as it will treat our char16_t string as a 32 bit string.
			// It's possible that the user could recompile libc to use 16 bit strings, but that's a pain for most users.
			char  buffer8[64];
			char* p    = buffer8;
			char* pEnd = buffer8 + 63; 

			while(p != pEnd)
			{   //             '+'                  'z'
				if((*pString < 0x2b) || (*pString > 0x7a)) // This includes a '\0' check.
					break;
				*p++ = (char)(int16_t)(uint16_t)*pString++;
			}
			*p = 0;

			double d = strtod(buffer8, &p);
			if(ppStringEnd)
				*ppStringEnd = (char16_t*)pString + (uintptr_t)(p - buffer8); // This math should be safe in the presence of multi-byte UTF8 strings because the only chars that can be part of a floating point string are always single bytes.

			return d;
		#endif
	}

	inline EASTDC_API double Strtod(const char32_t* pString, char32_t** ppStringEnd)
	{
		#if (EA_WCHAR_SIZE == 4) && defined(EA_HAVE_WCHAR_IMPL)
			return wcstod(reinterpret_cast<const wchar_t*>(pString), reinterpret_cast<wchar_t**>(ppStringEnd));
		#else
			// We convert from char32_t to char and convert that.
			char  buffer8[64]; buffer8[0] = 0;
			char* p    = buffer8;
			char* pEnd = buffer8 + 63; 

			while(p != pEnd)
			{   //             '+'                  'z'
				if((*pString < 0x2b) || (*pString > 0x7a)) // This includes a '\0' check.
					break;
				*p++ = (char)(int32_t)(uint32_t)*pString++;
			}
			*p = 0;

			double d = strtod(buffer8, &p);
			if(ppStringEnd)
				*ppStringEnd = (char32_t*)pString + (uintptr_t)(p - buffer8); // This math should be safe in the presence of multi-byte UTF8 strings because the only chars that can be part of a floating point string are always single bytes.

			return d;
		#endif
	}



	inline EASTDC_API float StrtoF32(const char* pString, char** ppStringEnd)
	{
		return (float)strtod(pString, ppStringEnd); // strtod is well-implemented by most standard libraries.
	}

	inline EASTDC_API float StrtoF32(const char16_t* pString, char16_t** ppStringEnd)
	{
		return (float)Strtod(pString, ppStringEnd);
	}


	inline EASTDC_API float StrtoF32(const char32_t* pString, char32_t** ppStringEnd)
	{
		return (float)Strtod(pString, ppStringEnd);
	}



	inline EASTDC_API float StrtoF32English(const char*  pString, char** ppStringEnd)
	{
		return (float)StrtodEnglish(pString, ppStringEnd);
	}

	inline EASTDC_API float StrtoF32English(const char16_t* pString, char16_t** ppStringEnd)
	{
		return (float)StrtodEnglish(pString, ppStringEnd);
	}

	inline EASTDC_API float StrtoF32English(const char32_t* pString, char32_t** ppStringEnd)
	{
		return (float)StrtodEnglish(pString, ppStringEnd);
	}



	inline EASTDC_API double Atof(const char* pString)
	{
		return atof(pString); // atof is well-implemented by most standard libraries.
	}

	inline EASTDC_API double Atof(const char16_t* pString)
	{
		return Strtod(pString, NULL);
	}

	inline EASTDC_API double Atof(const char32_t* pString)
	{
		return Strtod(pString, NULL);
	}



	inline EASTDC_API float AtoF32(const char* pString)
	{
		return (float)atof(pString); // atof is well-implemented by most standard libraries.
	}

	inline EASTDC_API float AtoF32(const char16_t* pString)
	{
		return (float)Strtod(pString, NULL);
	}

	inline EASTDC_API float AtoF32(const char32_t* pString)
	{
		return (float)Strtod(pString, NULL);
	}



	inline EASTDC_API double AtofEnglish(const char* pString)
	{
		return StrtodEnglish(pString, NULL);
	}

	inline EASTDC_API double AtofEnglish(const char16_t* pString)
	{
		return StrtodEnglish(pString, NULL);
	}

	inline EASTDC_API double AtofEnglish(const char32_t* pString)
	{
		return StrtodEnglish(pString, NULL);
	}



	inline EASTDC_API float AtoF32English(const char*  pString)
	{
		return (float)StrtodEnglish(pString, NULL);
	}

	inline EASTDC_API float AtoF32English(const char16_t* pString)
	{
		return (float)StrtodEnglish(pString, NULL);
	}

	inline EASTDC_API float AtoF32English(const char32_t* pString)
	{
		return (float)StrtodEnglish(pString, NULL);
	}



	inline EASTDC_API char* Ftoa(double dValue, char* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
	{
		// An implementation which calls sprintf might be a better idea.
		return FtoaEnglish(dValue, pResult, nResultCapacity, nPrecision, bExponentEnabled);
	}

	inline EASTDC_API char16_t* Ftoa(double dValue, char16_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
	{
		// An implementation which calls sprintf might be a better idea.
		return FtoaEnglish(dValue, pResult, nResultCapacity, nPrecision, bExponentEnabled);
	}

	inline EASTDC_API char32_t* Ftoa(double dValue, char32_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
	{
		// An implementation which calls sprintf might be a better idea.
		return FtoaEnglish(dValue, pResult, nResultCapacity, nPrecision, bExponentEnabled);
	}



#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE

	#ifdef EASTDC_UNICODE_CHAR_PTR_PTR_CAST
	#undef EASTDC_UNICODE_CHAR_PTR_PTR_CAST
	#endif

	#ifdef EASTDC_UNICODE_CONST_CHAR_PTR_CAST
	#undef EASTDC_UNICODE_CONST_CHAR_PTR_CAST
	#endif

	#ifdef EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST
	#undef EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST
	#endif

	#ifdef EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST
	#undef EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST
	#endif

	#ifdef EASTDC_UNICODE_CHAR_PTR_CAST
	#undef EASTDC_UNICODE_CHAR_PTR_CAST
	#endif

	#ifdef EASTDC_UNICODE_CHAR_CAST
	#undef EASTDC_UNICODE_CHAR_CAST
	#endif

	#if (EA_WCHAR_SIZE == 2)
		#define EASTDC_UNICODE_CHAR_PTR_PTR_CAST(x)       reinterpret_cast<char16_t **>(reinterpret_cast<void*>(x))
		#define EASTDC_UNICODE_CONST_CHAR_PTR_CAST(x)     reinterpret_cast<const char16_t *>(x)
		#define EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(x) reinterpret_cast<const char16_t **>(x)
		#define EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST(x) reinterpret_cast<const char16_t *&>(x)
		#define EASTDC_UNICODE_CHAR_PTR_CAST(x)           reinterpret_cast<char16_t *>(x)
		#define EASTDC_UNICODE_CHAR_CAST(x)               char16_t(x)
	#else
		#define EASTDC_UNICODE_CHAR_PTR_PTR_CAST(x)       reinterpret_cast<char32_t **>(x)
		#define EASTDC_UNICODE_CONST_CHAR_PTR_CAST(x)     reinterpret_cast<const char32_t *>(x)
		#define EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(x) reinterpret_cast<const char32_t **>(x)
		#define EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST(x) reinterpret_cast<const char32_t *&>(x)
		#define EASTDC_UNICODE_CHAR_PTR_CAST(x)           reinterpret_cast<char32_t *>(x)
		#define EASTDC_UNICODE_CHAR_CAST(x)               char32_t(x)
	#endif



	inline size_t Strlen(const wchar_t* pString)
	{
		return Strlen(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}

	inline wchar_t* Strend(const wchar_t* pString)
	{
		return reinterpret_cast<wchar_t *>(Strend(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString)));
	}

	inline wchar_t* Strcpy(wchar_t* pDestination, const wchar_t* pSource)
	{
		return reinterpret_cast<wchar_t *>(Strcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource)));
	}

	inline wchar_t* Strncpy(wchar_t* pDestination, const wchar_t* pSource, size_t n)
	{
		return reinterpret_cast<wchar_t *>(Strncpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), n));
	}

	inline wchar_t* StringnCopy(wchar_t* pDestination, const wchar_t* pSource, size_t n)
	{
		return reinterpret_cast<wchar_t *>(StringnCopy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), n));
	}

	inline size_t Strlcpy(wchar_t* pDestination, const wchar_t* pSource, size_t nDestCapacity)
	{
		return Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity);
	}

	inline int Strlcpy(wchar_t* pDestination, const char* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		return static_cast<int>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength));
	}

	inline int Strlcpy(wchar_t* pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		#if (EA_WCHAR_SIZE == 2)
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			return static_cast<int>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity));
		#else
			return static_cast<int>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength));
		#endif
	}

	inline int Strlcpy(wchar_t* pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		#if (EA_WCHAR_SIZE == 2)
			return static_cast<int>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength));
		#else
			EA_UNUSED(nSourceLength);
			return static_cast<int>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity));
		#endif
	}
	
	inline int Strlcpy(char* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		return static_cast<int>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength));
	}

	inline int Strlcpy(char16_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		#if (EA_WCHAR_SIZE == 2)
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			return static_cast<int>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity));
		#else
			return static_cast<int>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength));
		#endif
	}

	inline int Strlcpy(char32_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength)
	{
		#if (EA_WCHAR_SIZE == 2)
			return static_cast<int>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength));
		#else
			EA_UNUSED(nSourceLength);
			return static_cast<int>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity));
		#endif
	}

	inline bool Strlcpy(char* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		return Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
	}

	inline bool Strlcpy(char16_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		#if (EA_WCHAR_SIZE == 2)
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			nDestUsed = nSourceUsed = static_cast<size_t>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity));
			return true;
		#else
			return Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
		#endif
	}

	inline bool Strlcpy(char32_t* pDestination, const wchar_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
		#else
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			nDestUsed = nSourceUsed = static_cast<size_t>(Strlcpy(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity));
			return true;
		#endif
	}

	inline bool Strlcpy(wchar_t* pDestination, const char* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		return Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
	}

	inline bool Strlcpy(wchar_t* pDestination, const char16_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		#if (EA_WCHAR_SIZE == 2)
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			nDestUsed = nSourceUsed = static_cast<size_t>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity));
			return true;
		#else
			return Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
		#endif
	}

	inline bool Strlcpy(wchar_t* pDestination, const char32_t* pSource, size_t nDestCapacity, size_t nSourceLength, size_t& nDestUsed, size_t& nSourceUsed)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity, nSourceLength, nDestUsed, nSourceUsed);
		#else
			EA_UNUSED(nSourceLength); // To do: This pathway is broken in that this version of Strlcpy allows a source length to be specified, whereas this pathway doesn't use it.
			nDestUsed = nSourceUsed = static_cast<size_t>(Strlcpy(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity));
			return true;
		#endif
	}

	inline wchar_t* Strcat(wchar_t* pDestination, const wchar_t* pSource)
	{
		return reinterpret_cast<wchar_t *>(Strcat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource)));
	}

	inline wchar_t* Strncat(wchar_t* pDestination, const wchar_t* pSource, size_t n)
	{
		return reinterpret_cast<wchar_t *>(Strncat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), n));
	}

	inline wchar_t* StringnCat(wchar_t* pDestination, const wchar_t* pSource, size_t n)
	{
		return reinterpret_cast<wchar_t *>(StringnCat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), n));
	}

	inline size_t Strlcat(wchar_t* pDestination, const wchar_t* pSource, size_t nDestCapacity)
	{
		return Strlcat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity);
	}

	inline size_t Strlcat(wchar_t* pDestination, const char*  pSource, size_t nDestCapacity)
	{
		return Strlcat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity);
	}

	inline size_t Strlcat(wchar_t* pDestination, const char16_t*  pSource, size_t nDestCapacity)
	{
		return Strlcat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity);
	}

	inline size_t Strlcat(wchar_t* pDestination, const char32_t*  pSource, size_t nDestCapacity)
	{
		return Strlcat(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), pSource, nDestCapacity);
	}

	inline size_t Strlcat(char* pDestination, const wchar_t*  pSource, size_t nDestCapacity)
	{
		return Strlcat(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity);
	}

	inline size_t Strlcat(char16_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity)
	{
		return Strlcat(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity);
	}
		
	inline size_t Strlcat(char32_t* pDestination, const wchar_t*  pSource, size_t nDestCapacity)
	{
		return Strlcat(pDestination, EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nDestCapacity);
	}

	inline size_t Strxfrm(wchar_t* pDest, const wchar_t* pSource, size_t n)
	{
		return Strxfrm(EASTDC_UNICODE_CHAR_PTR_CAST(pDest), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), n);
	}

	inline wchar_t* Strdup(const wchar_t* pString)
	{
		return reinterpret_cast<wchar_t *>(Strdup(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString)));
	}

	inline void Strdel(wchar_t* pString)
	{
		Strdel(EASTDC_UNICODE_CHAR_PTR_CAST(pString));
	}

	inline wchar_t* Strupr(wchar_t* pString)
	{
		return reinterpret_cast<wchar_t *>(Strupr(EASTDC_UNICODE_CHAR_PTR_CAST(pString)));
	}

	inline wchar_t* Strlwr(wchar_t* pString)
	{
		return reinterpret_cast<wchar_t *>(Strlwr(EASTDC_UNICODE_CHAR_PTR_CAST(pString)));
	}

	inline wchar_t* Strmix(wchar_t* pDestination, const wchar_t* pSource, const wchar_t* pDelimiters)
	{
		return reinterpret_cast<wchar_t *>(Strmix(EASTDC_UNICODE_CHAR_PTR_CAST(pDestination), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pDelimiters)));
	}

	inline wchar_t* Strchr(const wchar_t* pString, wchar_t c)
	{
		return reinterpret_cast<wchar_t *>(Strchr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_CAST(c)));
	}

	inline wchar_t* Strnchr(const wchar_t* pString, wchar_t c, size_t n)
	{
		return reinterpret_cast<wchar_t *>(Strnchr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_CAST(c), n));
	}

	inline size_t  Strcspn(const wchar_t* pString1, const wchar_t* pString2)
	{
		return Strcspn(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2));
	}

	inline wchar_t* Strpbrk(const wchar_t* pString1, const wchar_t* pString2)
	{
		return reinterpret_cast<wchar_t *>(Strpbrk(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2)));
	}

	inline wchar_t* Strrchr(const wchar_t* pString, wchar_t c)
	{
		return reinterpret_cast<wchar_t *>(Strrchr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_CAST(c)));
	}

	inline size_t Strspn(const wchar_t* pString, const wchar_t* pSubString)
	{
		return Strspn(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSubString));
	}

	inline wchar_t* Strstr(const wchar_t* pString, const wchar_t* pSubString)
	{
		return reinterpret_cast<wchar_t *>(Strstr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSubString)));
	}

	inline wchar_t* Stristr(const wchar_t* pString, const wchar_t* pSubString)
	{
		return reinterpret_cast<wchar_t *>(Stristr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSubString)));
	}

	inline wchar_t* Strrstr(const wchar_t* pString, const wchar_t* pSubString)
	{
		return reinterpret_cast<wchar_t *>(Strrstr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSubString)));
	}

	inline wchar_t* Strirstr(const wchar_t* pString, const wchar_t* pSubString)
	{
		return reinterpret_cast<wchar_t *>(Strirstr(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSubString)));
	}

	inline bool Strstart(const wchar_t* pString, const wchar_t* pPrefix)
	{
		return Strstart(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pPrefix));
	}

	inline bool Stristart(const wchar_t* pString, const wchar_t* pPrefix)
	{
		return Stristart(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pPrefix));
	}

	inline bool Strend(const wchar_t* pString, const wchar_t* pSuffix, size_t stringLength = kSizeTypeUnset)
	{
		return Strend(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSuffix), stringLength);
	}

	inline bool Striend(const wchar_t* pString, const wchar_t* pSuffix, size_t stringLength = kSizeTypeUnset)
	{
		return Striend(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSuffix), stringLength);
	}

	inline wchar_t* Strtok(wchar_t* pString, const wchar_t* pDelimiters, wchar_t** pContext)
	{
		return reinterpret_cast<wchar_t *>(Strtok(EASTDC_UNICODE_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pDelimiters), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(pContext)));
	}

	inline const wchar_t* Strtok2(const wchar_t* pString, const wchar_t* pDelimiters, size_t* pResultLength, bool bFirst)
	{
		return reinterpret_cast<const wchar_t *>(Strtok2(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pDelimiters), pResultLength, bFirst));
	}

	inline wchar_t* Strset(wchar_t* pString, wchar_t c)
	{
		return reinterpret_cast<wchar_t *>(Strset(EASTDC_UNICODE_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_CAST(c)));
	}

	inline wchar_t* Strnset(wchar_t* pString, wchar_t c, size_t n)
	{
		return reinterpret_cast<wchar_t *>(Strnset(EASTDC_UNICODE_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_CAST(c), n));
	}

	inline wchar_t* Strrev(wchar_t* pString)
	{
		return reinterpret_cast<wchar_t *>(Strrev(EASTDC_UNICODE_CHAR_PTR_CAST(pString)));
	}

	inline int Strcmp(const wchar_t* pString1, const wchar_t* pString2)
	{
		return Strcmp(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2));
	}

	inline int Strncmp(const wchar_t* pString1, const wchar_t* pString2, size_t n)
	{
		return Strncmp(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), n);
	}

	inline int Stricmp(const wchar_t* pString1, const wchar_t* pString2)
	{
		return Stricmp(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2));
	}

	inline int Strnicmp(const wchar_t* pString1, const wchar_t* pString2, size_t n)
	{
		return Strnicmp(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), n);
	}

	inline int StrcmpNumeric (const wchar_t* pString1, const wchar_t* pString2, size_t length1, size_t length2, wchar_t decimal, wchar_t thousandsSeparator)
	{
		return StrcmpNumeric(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), length1, length2, EASTDC_UNICODE_CHAR_CAST(decimal), EASTDC_UNICODE_CHAR_CAST(thousandsSeparator));
	}

	inline int StricmpNumeric (const wchar_t* pString1, const wchar_t* pString2, size_t length1, size_t length2, wchar_t decimal, wchar_t thousandsSeparator)
	{
		return StricmpNumeric(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), length1, length2, EASTDC_UNICODE_CHAR_CAST(decimal), EASTDC_UNICODE_CHAR_CAST(thousandsSeparator));
	}

	inline int Strcoll(const wchar_t* pString1, const wchar_t* pString2)
	{
		return Strcoll(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2));
	}

	inline int Strncoll(const wchar_t* pString1, const wchar_t* pString2, size_t n)
	{
		return Strncoll(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), n);
	}

	inline int Stricoll(const wchar_t* pString1, const wchar_t* pString2)
	{
		return Stricoll(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2));
	}

	inline int Strnicoll(const wchar_t* pString1, const wchar_t* pString2, size_t n)
	{
		return Strnicoll(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString1), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString2), n);
	}

	inline wchar_t* EcvtBuf(double dValue, int nDigitCount, int* pDecimalPos, int* pSign, wchar_t* pBuffer)
	{
		return reinterpret_cast<wchar_t *>(EcvtBuf(dValue, nDigitCount, pDecimalPos, pSign, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer)));
	}

	inline wchar_t* FcvtBuf(double dValue, int nDigitCountAfterDecimal, int* pDecimalPos, int* pSign, wchar_t* pBuffer)
	{
		return reinterpret_cast<wchar_t *>(FcvtBuf(dValue, nDigitCountAfterDecimal, pDecimalPos, pSign, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer)));
	}

	inline wchar_t* I32toa(int32_t nValue, wchar_t* pBuffer, int nBase)
	{
		return reinterpret_cast<wchar_t *>(I32toa(nValue, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer), nBase));
	}

	inline wchar_t* U32toa(uint32_t nValue, wchar_t* pBuffer, int nBase)
	{
		return reinterpret_cast<wchar_t *>(U32toa(nValue, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer), nBase));
	}

	inline wchar_t* I64toa(int64_t nValue, wchar_t* pBuffer, int nBase)
	{
		return reinterpret_cast<wchar_t *>(I64toa(nValue, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer), nBase));
	}

	inline wchar_t* U64toa(uint64_t nValue, wchar_t* pBuffer, int nBase)
	{
		return reinterpret_cast<wchar_t *>(U64toa(nValue, EASTDC_UNICODE_CHAR_PTR_CAST(pBuffer), nBase));
	}

	inline double Strtod(const wchar_t* pString, wchar_t** ppStringEnd)
	{
		return Strtod(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd));
	}

	inline float StrtoF32(const wchar_t* pString, wchar_t** ppStringEnd)
	{
		return StrtoF32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd));
	}

	inline double StrtodEnglish(const wchar_t* pString, wchar_t** ppStringEnd)
	{
		return StrtodEnglish(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd));
	}

	inline float  StrtoF32English(const wchar_t* pString, wchar_t** ppStringEnd)
	{
		return StrtoF32English(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd));
	}

	inline int32_t StrtoI32(const wchar_t* pString, wchar_t** ppStringEnd, int nBase)
	{
		return StrtoI32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd), nBase);
	}


	inline uint32_t StrtoU32(const wchar_t* pString, wchar_t** ppStringEnd, int nBase)
	{
		return StrtoU32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd), nBase);
	}


	inline int64_t StrtoI64(const wchar_t* pString, wchar_t** ppStringEnd, int nBase)
	{
		return StrtoI64(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd), nBase);
	}


	inline uint64_t StrtoU64(const wchar_t* pString, wchar_t** ppStringEnd, int nBase)
	{
		return StrtoU64(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CHAR_PTR_PTR_CAST(ppStringEnd), nBase);
	}


	inline int32_t AtoI32(const wchar_t* pString)
	{
		return AtoI32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline uint32_t AtoU32(const wchar_t* pString)
	{
		return AtoU32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline int64_t AtoI64(const wchar_t* pString)
	{
		return AtoI64(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline uint64_t AtoU64(const wchar_t* pString)
	{
		return AtoU64(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline double Atof(const wchar_t* pString)
	{
		return Atof(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline float  AtoF32(const wchar_t* pString)
	{
		return AtoF32(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline double AtofEnglish(const wchar_t* pString)
	{
		return AtofEnglish(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline float  AtoF32English(const wchar_t* pString)
	{
		return AtoF32English(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString));
	}


	inline wchar_t* Ftoa(double dValue, wchar_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
	{
		return reinterpret_cast<wchar_t *>(Ftoa(dValue, EASTDC_UNICODE_CHAR_PTR_CAST(pResult), nResultCapacity, nPrecision, bExponentEnabled));
	}


	inline wchar_t* FtoaEnglish(double dValue, wchar_t* pResult, int nResultCapacity, int nPrecision, bool bExponentEnabled)
	{
		return reinterpret_cast<wchar_t *>(FtoaEnglish(dValue, EASTDC_UNICODE_CHAR_PTR_CAST(pResult), nResultCapacity, nPrecision, bExponentEnabled));
	}


	inline size_t ReduceFloatString(wchar_t* pString, size_t nLength)
	{
		return ReduceFloatString(EASTDC_UNICODE_CHAR_PTR_CAST(pString), nLength);
	}

#endif


#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE

#endif


	///////////////////////////////////////////////////////////////////////////
	// Deprecated functionality
	///////////////////////////////////////////////////////////////////////////

	template <typename Source, typename Dest>  // ConvertString has been renamed to Strlcpy and the template arguments reversed.
	inline Dest ConvertString(const Source& s){ return Strlcpy<Dest, Source>(s); }

	template <typename Source, typename Dest> // ConvertString has been renamed to Strlcpy and the template arguments reversed.
	inline bool ConvertString(const Source& s, Dest& d){ return Strlcpy<Dest, Source>(d, s); }


} // namespace StdC
} // namespace EA




#endif // Header include guard
