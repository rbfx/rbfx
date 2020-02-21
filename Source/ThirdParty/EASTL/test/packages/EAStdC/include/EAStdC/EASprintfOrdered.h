///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Ordered printf
//
// The module provides ordered versions of the printf family of functions:
//   int OCprintf(WriteFunction pFunction, void* pContext, const char_t* pFormat, ...);
//   int OFprintf(FILE* pFile, const char_t* pFormat, ...);
//   int OPrintf(cconst char_t* pFormat, ...);
//   int OSprintf(char_t* pDestination, const char_t* pFormat, ...);
//   int OSnprintf(char_t* pDestination, size_t n, const char_t* pFormat, ...);
//
//   int OVcprintf(WriteFunction pFunction, void* pContext, const char_t* pFormat, va_list arguments);
//   int OVfprintf(FILE* pFile, const char_t* pFormat, va_list arguments);
//   int OVprintf(const char_t* pFormat, va_list arguments);
//   int OVsprintf(char_t* pDestination, const char_t* pFormat, va_list arguments);
//   int OVsnprintf(char_t* pDestination, size_t n, const char_t* pFormat, va_list arguments);
//   int OVscprintf(const char* pFormat, va_list arguments);
//
// Ordered printf is like printf except it works on "ordered" printf specifiers.
// This means that instead of using "%4d %f" we give an order to the arguments via 
// an index and colon, as with "%1:4d %2:f". The point, however, is that you can 
// reorder the indexes, as with "%2:f %1:4d". This is particularly useful for 
// formatted string localization, as different locales use subjects and verbs
// in different orders.
//
// User indexes can start at either 0 or 1. Oprintf detects which is in use as 
// it goes. So the following have identical effect:
//     OPrintf("%1:s" %0:f", 3.0, "hello");
//     OPrintf("%2:s" %1:f", 3.0, "hello");
//
// User indexes must be contiguous and the behaviour of Oprintf is undefined
// if the ordering is non-contiguous. There are debug checks to validate 
// contiguity, so debug tests should catch mistakes. The following is an 
// example of non-contiguous ordering, as it is missing a "3:" format:
//     OPrintf("%1:d" %0:d %3:d", 17, 18, 19, 20);
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EASPRINTFORDERED_H
#define EASTDC_EASPRINTFORDERED_H


#include <EAStdC/EASprintf.h>


namespace EA
{
namespace StdC
{

	///////////////////////////////////////////////////////////////////////////////
	/// Printf
	///
	/// See EASprintf.h for documentation on the Printf family.
	/// See also http://www.cplusplus.com/reference/clibrary/cstdio/printf.html
	///
	EASTDC_API int OCprintf(WriteFunction8 pWriteFunction, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int OPrintf(const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, ...);

	EASTDC_API int OCprintf(WriteFunction16 pWriteFunction, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OPrintf(const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, ...);

	EASTDC_API int OCprintf(WriteFunction32 pWriteFunction, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OPrintf(const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, ...);
	EASTDC_API int OSnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, ...);



	///////////////////////////////////////////////////////////////////////////////
	/// OVprintf
	///
	/// See EASprintf.h for documentation on the Vprintf family.
	/// See also http://www.cplusplus.com/reference/clibrary/cstdio/printf.html
	///
	EASTDC_API int OVcprintf(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVprintf(const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVscprintf(const char* EA_RESTRICT pFormat, va_list arguments);

	EASTDC_API int OVcprintf(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVscprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments);

	EASTDC_API int OVcprintf(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVsnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments);
	EASTDC_API int OVscprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments);


} // namespace StdC
} // namespace EA


#endif // Header include guard













