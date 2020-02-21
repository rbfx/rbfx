///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This module implements the functions listed below. 
//
// Variable argument versions:
//   int Cprintf(WriteFunction pFunction, void* pContext, const char_t* pFormat, ...);
//   int Fprintf(FILE* pFile, const char_t* pFormat, ...);
//   int Printf(cconst char_t* pFormat, ...);
//   int Sprintf(char_t* pDestination, const char_t* pFormat, ...);
//   int Snprintf(char_t* pDestination, size_t n, const char_t* pFormat, ...);
//   int Dprintf(char_t* pDestination, size_t n, const char_t* pFormat, ...);
//   int StringPrintf(String& s, const typename String::value_type* EA_RESTRICT pFormat, ...);
//
// Vararg versions:
//   int Vcprintf(WriteFunction pFunction, void* pContext, const char_t* pFormat, va_list arguments);
//   int Vfprintf(FILE* pFile, const char_t* pFormat, va_list arguments);
//   int Vprintf(const char_t* pFormat, va_list arguments);
//   int Vsprintf(char_t* pDestination, const char_t* pFormat, va_list arguments);
//   int Vsnprintf(char_t* pDestination, size_t n, const char_t* pFormat, va_list arguments);
//   int Vscprintf(const char* pFormat, va_list arguments);
//   int Vdprintf(const char* pFormat, va_list arguments);
//   int StringVcprintf(String& s, const typename String::value_type* EA_RESTRICT pFormat, va_list arguments)
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EASPRINTF_H
#define EASTDC_EASPRINTF_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/internal/stdioEA.h>

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif


namespace EA
{
namespace StdC
{

	////////////////////////////////////////////////////////////////////////////
	// WriteFunctionState
	//
	// This is used to help the WriteFunction know what state its being called
	// in. Our sprintf family of functions work by repeatedly calling the 
	// WriteFunction until all of the string is complete. But due to platform
	// quirks and limitations, some platforms require the WriteFunction to know
	// if this is the last write, because they use it to complete a write that
	// needs to do the complete string all at once. 
	//
	enum WriteFunctionState
	{
		kWFSBegin,          /// Called once before any data is written, including if there will be no data written.
		kWFSIntermediate,   /// Called zero or more times, with partial data. UTF8 sequences will always be whole and not split between calls.
		kWFSEnd             /// Called once after any data is written, including if there was no data written.
	};

	/////////////////////////////////////////////////////////////////////////////
	// WriteFunction8
	//
	// The input string for WriteFunction8 is assumed to be UTF8 or ASCII-encoded.
	// Returns number of chars actually written to the destination.
	// Returns -1 upon error.
	//
	// UTF8 sequences will always be whole and not split between calls.
	// The input pData is *not* guaranteed to be 0-terminated and won't be so for 
	// some cases, as it's impractical to make it always be so due to the fact 
	// that users can print truncated read-only string data memory. However, you are
	// guaranteed to be able to read the character at pData[nCount], as it will 
	// always be valid readable memory. This may be useful to do because you can
	// write an optimized pathway for the case that pData is in fact 0-terminated,
	// which will often be the case.
	//
	typedef int (*WriteFunction8)(const char* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs);

	/////////////////////////////////////////////////////////////////////////////
	// WriteFunction16
	//
	// The input string is UCS2 encoded.
	// Otherwise this is the same as WriteFunction8.
	//
	typedef int (*WriteFunction16)(const char16_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs);

	/////////////////////////////////////////////////////////////////////////////
	// WriteFunction32
	//
	// The input string is UCS4 encoded.
	// Otherwise this is the same as WriteFunction8.
	//
	typedef int (*WriteFunction32)(const char32_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs);


	/////////////////////////////////////////////////////////////////////////////
	// WriteFunctionW
	//
	// The input string is wchar_t, with wchar_t being UCS2 or UCS4-encoded, depending on its size.
	// Otherwise this is the same as WriteFunction8.
	//
	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		typedef int (*WriteFunctionW)(const wchar_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs);
	#endif


	/////////////////////////////////////////////////////////////////////////////
	// WriteFunctionString
	//
	// This allows the implementation of a Sprintf that writes directly into 
	// a C++ string class instance. This approach may be better because otherwise
	// you'd have to do an approach where you call Sprintf once to find the length,
	// resize your string, then call Sprintf again with the string address.
	// See StringSprintf below for a generic implementation of direct string writing.
	//
	template <typename String>
	int WriteFunctionString(const typename String::value_type* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState /*wfs*/)
	{
		String* pString = static_cast<String*>(pContext);
		pString->append(pData, (uint32_t)nCount);
		return (int)nCount;
	}


	///////////////////////////////////////////////////////////////////////////////
	/// Vprintf
	///
	/// Note that vsnprintf was not added to the C standard until C99.
	/// Here we follow the C99 standard and have the return value of vsnprintf 
	/// return the number of required characters to complete the formatted string.
	/// This is more useful than the way some libraries implement vsnprintf by 
	/// returning -1 if there isn't enough space. The return value is -1 if there 
	/// was an "encoding error" or the implementation was unable to return a 
	/// value > n upon lack of sufficient space as per the C99 standard.
	///
	/// Official specification:
	/// The vsnprintf function is equivalent to snprintf, with the variable 
	/// argument list replaced by arguments, which shall have been initialized 
	/// by the va_start macro (and possibly subsequent va_arg calls). 
	/// The vsnprintf function does not invoke the va_end macro. If copying 
	/// takes place between objects that overlap, the behavior is undefined.
	///
	/// The vsnprintf function returns the number of characters that would 
	/// have been written had n been sufficiently large, not counting the 
	/// terminating null character, or a neg ative value if an encoding error 
	/// occurred. Thus, the null-terminated output has been completely written 
	/// if and only if the returned value is nonnegative and less than n.
	///
	/// The vscprintf function returns the number of chars that are needed for 
	/// a printf operation. It writes nothing to any destination.
	///
	/// See also http://www.cplusplus.com/reference/clibrary/cstdio/printf.html
	///
	/// Description:
	///     Vcprintf:  Print to a user-supplied WriteFunction. Can form the foundation for other printfs.
	///     Vfprintf:  Print to a specific FILE, same as the C vfprintf function. Acts like Vdprintf for platforms where stdout doesn't exist (e.g. consoles) when the FILE is stdout.
	///     Vprintf:   Print to the stdout FILE, same as the C vfprintf function. Acts like Vdprintf for platforms where stdout doesn't exist (e.g. consoles).
	///     Vsprintf:  Print to a string, with no capacity specified, same as the C vsprintf function.
	///     Vsnprintf: Print to a string, with capacity specified, same as the C vsnprintf function.
	///     Vdprintf:  Print to a debug output destination (e.g. OutputDebugString on Microsoft platforms).
	///
	EASTDC_API int Vcprintf(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vprintf(const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vscprintf(const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vdprintf(const char* EA_RESTRICT pFormat, va_list arguments);

	EASTDC_API int Vcprintf(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vscprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments);

	EASTDC_API int Vcprintf(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vsnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int Vscprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		EASTDC_API int Vcprintf(WriteFunctionW pWriteFunctionW, void* EA_RESTRICT pContext, const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vprintf(const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vsprintf(wchar_t* EA_RESTRICT pDestination, const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vsnprintf(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vscprintf(const wchar_t* EA_RESTRICT pFormat, va_list arguments);
	#endif

	// StringVcprintf
	// Writes directly into a C++ string object. e.g. StringVcprintf(strObject, "%d", arguments);
	// Will be faster than eastl::string::printf() as long as the supplied string has a capacity that
	// doesn't need to keep increasing while being written into.
	template <typename String>
	int StringVcprintf(String& s, const typename String::value_type* EA_RESTRICT pFormat, va_list arguments)
	{
		return Vcprintf(WriteFunctionString<String>, &s, pFormat, arguments);
	}


	///////////////////////////////////////////////////////////////////////////////
	/// Printf
	///
	/// The printf family implements the printf family of functions strictly to 
	/// the C99 standard. As of this writing, all C99 functionality is supported
	/// except the unusual %a field type, which is treated as %g for the time being.
	/// Additionally, extra functionality such as the following field types and 
	/// modifiers are supported:
	///     b      Binary output field type (joins d, i, x, o, etc.). Example: printf("%b", 255) prints "11111111"
	///     I8     8 bit integer field modifier. Example: printf("%I8d", 0xff) prints "-1"
	///     I16    16 bit integer field modifier. Example: printf("%I16d", 0xffff) prints "-1"
	///     I32    32 bit integer field modifier. Example: printf("%I32d", 0xffffffff) prints "-1"
	///     I64    64 bit integer field modifier. Example: printf("%I64d", 0xffffffffffffffff) prints "-1"
	///     I128   128 bit integer field modifier. Example: printf("%I128d", 0xffffffffffffffffffffffffffffffff) prints "-1"
	///     '      Display a thousands separator. Example: printf("%'I16u", 0xffff); prints 65,535
	///
	/// See also http://www.cplusplus.com/reference/clibrary/cstdio/printf.html
	///
	/// Description:
	///     Cprintf:  Print to a user-supplied WriteFunction. Can form the foundation for other printfs.
	///     Fprintf:  Print to a specific FILE, same as the C fprintf function. Acts like Dprintf for platforms where stdout doesn't exist (e.g. consoles) when the FILE is stdout.
	///     Printf:   Print to the stdout FILE, same as the C fprintf function. Acts like Dprintf for platforms where stdout doesn't exist (e.g. consoles).
	///     Sprintf:  Print to a string, with no capacity specified, same as the C sprintf function.
	///     Snprintf: Print to a string, with capacity specified, same as the C snprintf function.
	///     Dprintf:  Print to a debug output destination (e.g. OutputDebugString on Microsoft platforms).
	///
	EASTDC_API int Cprintf(WriteFunction8 pWriteFunction, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int Printf(const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int Sprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int Snprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int Dprintf(const char* EA_RESTRICT pFormat, ...);

	EASTDC_API int Cprintf(WriteFunction16 pWriteFunction, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Printf(const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Sprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Snprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, ...);

	EASTDC_API int Cprintf(WriteFunction32 pWriteFunction, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Printf(const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Sprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int Snprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, ...);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		EASTDC_API int Cprintf(WriteFunctionW pWriteFunction, void* EA_RESTRICT pContext, const wchar_t* EA_RESTRICT pFormat, ...);
		EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const wchar_t* EA_RESTRICT pFormat, ...);
		EASTDC_API int Printf(const wchar_t* EA_RESTRICT pFormat, ...);
		EASTDC_API int Sprintf(wchar_t* EA_RESTRICT pDestination, const wchar_t* EA_RESTRICT pFormat, ...);
		EASTDC_API int Snprintf(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, ...);
	#endif

	// StringPrintf
	// Writes directly into a C++ string object. e.g. StringPrintf(strObject, "%d", 37);
	// Will be faster than eastl::string::printf() as long as the supplied string has a capacity that
	// doesn't need to keep increasing while being written into.
	template <typename String> 
	int StringPrintf(String& s, const typename String::value_type* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);
		int result = Vcprintf(WriteFunctionString<String>, &s, pFormat, arguments);
		va_end(arguments);
		return result;
	}



	///////////////////////////////////////////////////////////////////////////
	// Deprecated functionality
	///////////////////////////////////////////////////////////////////////////

	#if EASTDC_VSNPRINTF8_ENABLED
		EASTDC_API int Vsnprintf8(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vsnprintf16(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments);
		EASTDC_API int Vsnprintf32(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			EASTDC_API int VsnprintfW(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, va_list arguments);
		#endif
	#endif

} // namespace StdC
} // namespace EA


#endif // Header include guard













