///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This module implements the following functions.
//   int Cscanf(ReadFunction pReadFunction, void* pContext, const char_t* pFormat, ...);
//   int Fscanf(FILE* pFile, const char_t* pFormat, ...);
//   int Scanf(const char_t* pFormat, ...);
//   int Sscanf(char_t* pDestination, const char_t* pFormat, ...);
//
//   int Vcscanf(ReadFunction pReadFunction, void* pContext, const char_t* pFormat, va_list arguments);
//   int Vfscanf(FILE* pFile, const char_t* pFormat, va_list arguments);
//   int Vscanf(const char_t* pFormat, va_list arguments);
//   int Vsscanf(char_t* pDestination, const char_t* pFormat, va_list arguments);
//
// Limitations
// As of this writing (September 2007), the %[] modifier supports only single-byte
// characters in the 8 bit version and supports only the first 256 characters in 
// the 16/32 bit versions. If you need support for additional characters, consult the 
// maintainer of this package and you should be able to get it within 48 hours.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EASCANF_H
#define EASTDC_EASCANF_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
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

	/////////////////////////////////////////////////////////////////////////////
	// ReadAction
	//
	enum ReadAction
	{
		kReadActionBegin,           // Called upon first entering scanf. The value param will be 1 for UTF8 and 2 for UCS2. This allows the ReadFunction to take any setup actions.
		kReadActionEnd,             // Called upon just exiting scanf. This allows the ReadFunction to take any cleanup actions. The value param is unused.
		kReadActionRead,            // Read and return a single UCS2 Unicode character, very much like the fgetc function. Return -1 upon error or EOF; GetLastError will be used to distinguish between the two. The read function may need to convert the data source to Unicode if the data source is not UCS-encoded. The value param is unused.
		kReadActionUnread,          // Push back the given UCS2 Unicode value. Return -1 upon error, 0 on success. The read function may need to convert the data source from Unicode if the data source is not UCS-encoded. The value param is unused.
		kReadActionGetAtEnd,        // Return 1 if at end of data, 0 if not. The value param is unused.
		kReadActionGetLastError     // Return the last file read error value. Zero means no error. The value param is unused.
	};


	/////////////////////////////////////////////////////////////////////////////
	// kReadError
	//
	const int kReadError = -1;


	/////////////////////////////////////////////////////////////////////////////
	// ReadFunction8
	//
	// This a multi-purpose callback function that's provided by the user for 
	// the scanf function and similar data reading functions. It is called with
	// one of the ReadAction enumerations, and the expected behaviour depends 
	// on the enumeration.
	//
	// The meaning of the value parameter depends on the ReadAction. See ReadAction
	// for documentation of each case.
	// 
	// The pContext parameter is the value the user originally provided to 
	// scanf or similar data reading function.
	//
	// The function is expected to convert the actual source data into individual
	// Unicode code points during kReadActionRead. For ASCII text there is no 
	// conversion involved, but for multi-byte text the ReadFunction will need to 
	// convert any such text to UCS Unicode.
	//
	// Returns the number of chars read on success. Returns 0 when at end of buffer.
	// Upon error, returns -1 (kReadError, same as EOF).
	//
	// The scanf functions will not use kReadActionUnread multiple times in
	// succession; only at most unread action will be outstanding. Due to the 
	// specification for scanf, there is no practical way to avoid the requirement 
	// of being able to unread characters.
	//
	// See the source code to the scanf function in order to see some examples
	// of ReadFunction implementations.
	//
	// UTF8 multi-byte characters should be returned as their unsigned value.
	// Thus even though char is signed, all characters are read and written
	// as if they are uint8_t. The only time a negative value should be used is
	// in the case of a -1 return value. This situation exists because it 
	// exists as such with the C Standard Library, for better or worse.
	//
	typedef int (*ReadFunction8)(ReadAction readAction, int value, void* pContext);

	/////////////////////////////////////////////////////////////////////////////
	// ReadFunction16
	//
	// This function is currently identical to ReadFunction8 with the exception
	// that kReadActionBegin will specify char16_t characters instead of char. 
	//
	typedef int (*ReadFunction16)(ReadAction readAction, int value, void* pContext);

	/////////////////////////////////////////////////////////////////////////////
	// ReadFunction32
	//
	// This function is currently identical to ReadFunction8 with the exception
	// that kReadActionBegin will specify char32_t characters instead of char. 
	//
	typedef int (*ReadFunction32)(ReadAction readAction, int value, void* pContext);

	/////////////////////////////////////////////////////////////////////////////
	// ReadFunctionW
	//
	// This function is currently identical to ReadFunction8 with the exception
	// that kReadActionBegin will specify wchar_t characters instead of char. 
	//
	typedef int (*ReadFunctionW)(ReadAction readAction, int value, void* pContext);


	///////////////////////////////////////////////////////////////////////////////
	// EAScanf configuration parameters
	//
	#ifndef EASCANF_FIELD_MAX               // Defines the maximum supported length of a field, 
		#define EASCANF_FIELD_MAX 1024      // except string fields, which have no size limit.
	#endif                                  // This value relates to the size of buffers used in the stack space.

	#ifndef EASCANF_MS_STYLE_S_FORMAT       // Microsoft uses a non-standard interpretation of the %s field type. 
		#define EASCANF_MS_STYLE_S_FORMAT 1 // For wsprintf MSVC interprets %s as a wchar_t string and %S as a char string.
	#endif                                  // You can make your code portable by using %hs and %ls to force the type.



	///////////////////////////////////////////////////////////////////////////////
	/// Scanf
	///
	EASTDC_API int Cscanf(ReadFunction8 pReadFunction8, void* pContext, const char* pFormat, ...);
	EASTDC_API int Fscanf(FILE* pFile, const char* pFormat, ...);
	EASTDC_API int Scanf(const char* pFormat, ...);
	EASTDC_API int Sscanf(const char*  pTextBuffer, const char* pFormat, ...);

	EASTDC_API int Cscanf(ReadFunction16 pReadFunction16, void* pContext, const char16_t* pFormat, ...);
	EASTDC_API int Fscanf(FILE* pFile, const char16_t* pFormat, ...);
	EASTDC_API int Scanf(const char16_t* pFormat, ...);
	EASTDC_API int Sscanf(const char16_t* pTextBuffer, const char16_t* pFormat, ...);

	EASTDC_API int Cscanf(ReadFunction32 pReadFunction32, void* pContext, const char32_t* pFormat, ...);
	EASTDC_API int Fscanf(FILE* pFile, const char32_t* pFormat, ...);
	EASTDC_API int Scanf(const char32_t* pFormat, ...);
	EASTDC_API int Sscanf(const char32_t* pTextBuffer, const char32_t* pFormat, ...);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		EASTDC_API int Cscanf(ReadFunctionW pReadFunctionW, void* pContext, const wchar_t* pFormat, ...);
		EASTDC_API int Fscanf(FILE* pFile, const wchar_t* pFormat, ...);
		EASTDC_API int Scanf(const wchar_t* pFormat, ...);
		EASTDC_API int Sscanf(const wchar_t* pTextBuffer, const wchar_t* pFormat, ...);
	#endif


	///////////////////////////////////////////////////////////////////////////////
	/// Vscanf
	///
	EASTDC_API int Vcscanf(ReadFunction8 pReadFunction8, void* pContext, const char* pFormat, va_list arguments);
	EASTDC_API int Vfscanf(FILE* pFile, const char* pFormat, va_list arguments);
	EASTDC_API int Vscanf(const char* pFormat, va_list arguments);
	EASTDC_API int Vsscanf(const char* pTextBuffer, const char* pFormat, va_list arguments);

	EASTDC_API int Vcscanf(ReadFunction16 pReadFunction16, void* pContext, const char16_t* pFormat, va_list arguments);
	EASTDC_API int Vfscanf(FILE* pFile, const char16_t* pFormat, va_list arguments);
	EASTDC_API int Vscanf(const char16_t* pFormat, va_list arguments);
	EASTDC_API int Vsscanf(const char16_t* pTextBuffer, const char16_t* pFormat, va_list arguments);

	EASTDC_API int Vcscanf(ReadFunction32 pReadFunction32, void* pContext, const char32_t* pFormat, va_list arguments);
	EASTDC_API int Vfscanf(FILE* pFile, const char32_t* pFormat, va_list arguments);
	EASTDC_API int Vscanf(const char32_t* pFormat, va_list arguments);
	EASTDC_API int Vsscanf(const char32_t* pTextBuffer, const char32_t* pFormat, va_list arguments);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		EASTDC_API int Vcscanf(ReadFunctionW pReadFunctionW, void* pContext, const wchar_t* pFormat, va_list arguments);
		EASTDC_API int Vfscanf(FILE* pFile, const wchar_t* pFormat, va_list arguments);
		EASTDC_API int Vscanf(const wchar_t* pFormat, va_list arguments);
		EASTDC_API int Vsscanf(const wchar_t* pTextBuffer, const wchar_t* pFormat, va_list arguments);
	#endif


} // namespace StdC
} // namespace EA


#endif // Header include guard













