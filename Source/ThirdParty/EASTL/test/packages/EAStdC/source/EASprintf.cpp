///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/SprintfCore.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EACType.h>
#include <EAAssert/eaassert.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <math.h>
#include <float.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
EA_RESTORE_ALL_VC_WARNINGS()
EA_DISABLE_VC_WARNING(4127) // conditional expression is constant.


namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// char 
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Vcprintf(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(pWriteFunction8, pContext, pFormat, arguments);
}

EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, va_list arguments)
{
	#if EASTDC_PRINTF_DEBUG_ENABLED
		if((pFile == stdout) || (pFile == stderr))
		{
			SprintfLocal::PlatformLogWriterContext8 context;
			return SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);
		}
	#endif

	return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter8, pFile, pFormat, arguments);
}

EASTDC_API int Vprintf(const char* EA_RESTRICT pFormat, va_list arguments)
{
	#if EASTDC_PRINTF_DEBUG_ENABLED
		SprintfLocal::PlatformLogWriterContext8 context;
		return SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);
	#else
		return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter8, stdout, pFormat, arguments);
	#endif
}

EASTDC_API int Vsprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return Vsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int Vsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext8 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::VprintfCore(SprintfLocal::StringWriter8, &sc, pFormat, arguments);

	#if EASPRINTF_SNPRINTF_C99_RETURN
		if(pDestination && (nRequiredLength >= 0))
		{
			if((size_t)nRequiredLength < n) // If there was enough space...
				pDestination[nRequiredLength] = 0;
			else if(n > 0)
				pDestination[n - 1] = 0;
		} // Else an encoding error has occurred and we can do nothing.

		return nRequiredLength;
	#else
		if((size_t)nRequiredLength < n)
		{
			if(pDestination)
				pDestination[nRequiredLength] = 0;
			return nRequiredLength;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		else if(n == 0)
			return nRequiredLength;
		return -1;
	#endif
}

EASTDC_API int Vscprintf(const char* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return Vsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int Vdprintf(const char* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::PlatformLogWriterContext8 context;
	return SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);
}




EASTDC_API int Cprintf(WriteFunction8 pWriteFunction, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(pWriteFunction, pContext, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, ...)
{
	int result;

	va_list arguments;
	va_start(arguments, pFormat);

	#if EASTDC_PRINTF_DEBUG_ENABLED
		if((pFile == stdout) || (pFile == stderr))
		{
			SprintfLocal::PlatformLogWriterContext8 context;
			result = SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);
		}
		else
	#endif

	result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter8, pFile, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Printf(const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	#if EASTDC_PRINTF_DEBUG_ENABLED
		SprintfLocal::PlatformLogWriterContext8 context;
		int result = SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);
	#else
		int result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter8, stdout, pFormat, arguments);
	#endif

	va_end(arguments);

	return result;
}

EASTDC_API int Sprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Snprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, n, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Dprintf(const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	SprintfLocal::PlatformLogWriterContext8 context;
	int result = SprintfLocal::VprintfCore(SprintfLocal::PlatformLogWriter8, &context, pFormat, arguments);

	va_end(arguments);

	return result;
}



///////////////////////////////////////////////////////////////////////////////
// char16_t 
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Vcprintf(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(pWriteFunction16, pContext, pFormat, arguments);
}

EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter16, pFile, pFormat, arguments);
}

EASTDC_API int Vprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter16, stdout, pFormat, arguments);
}

EASTDC_API int Vsprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return Vsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int Vsnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext16 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::VprintfCore(SprintfLocal::StringWriter16, &sc, pFormat, arguments);

	#if EASPRINTF_SNPRINTF_C99_RETURN
		if(pDestination && (nRequiredLength >= 0))
		{
			if((size_t)nRequiredLength < n) // If there was enough space...
				pDestination[nRequiredLength] = 0;
			else if(n > 0)
				pDestination[n - 1] = 0;
		} // Else an encoding error has occurred and we can do nothing.

		return nRequiredLength;
	#else
		if((size_t)nRequiredLength < n)
		{
			if(pDestination)
				pDestination[nRequiredLength] = 0;
			return nRequiredLength;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		else if(n == 0)
			return nRequiredLength;
		return -1;
	#endif
}

EASTDC_API int Vscprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return Vsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int Cprintf(WriteFunction16 pWriteFunction, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(pWriteFunction, pContext, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter16, pFile, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Printf(const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter16, stdout, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Sprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Snprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, n, pFormat, arguments);

	va_end(arguments);

	return result;
}



///////////////////////////////////////////////////////////////////////////////
// char32_t 
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Vcprintf(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(pWriteFunction32, pContext, pFormat, arguments);
}

EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter32, pFile, pFormat, arguments);
}

EASTDC_API int Vprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::VprintfCore(SprintfLocal::FILEWriter32, stdout, pFormat, arguments);
}

EASTDC_API int Vsprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return Vsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int Vsnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext32 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::VprintfCore(SprintfLocal::StringWriter32, &sc, pFormat, arguments);

	#if EASPRINTF_SNPRINTF_C99_RETURN
		if(pDestination && (nRequiredLength >= 0))
		{
			if((size_t)nRequiredLength < n) // If there was enough space...
				pDestination[nRequiredLength] = 0;
			else if(n > 0)
				pDestination[n - 1] = 0;
		} // Else an encoding error has occurred and we can do nothing.

		return nRequiredLength;
	#else
		if((size_t)nRequiredLength < n)
		{
			if(pDestination)
				pDestination[nRequiredLength] = 0;
			return nRequiredLength;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		else if(n == 0)
			return nRequiredLength;
		return -1;
	#endif
}

EASTDC_API int Vscprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return Vsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int Cprintf(WriteFunction32 pWriteFunction, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(pWriteFunction, pContext, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter32, pFile, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Printf(const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = SprintfLocal::VprintfCore(SprintfLocal::FILEWriter32, stdout, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Sprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);

	va_end(arguments);

	return result;
}

EASTDC_API int Snprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	int result = Vsnprintf(pDestination, n, pFormat, arguments);

	va_end(arguments);

	return result;
}






///////////////////////////////////////////////////////////////////////////
// Deprecated functionality
///////////////////////////////////////////////////////////////////////////

#if EASTDC_VSNPRINTF8_ENABLED
	EASTDC_API int Vsnprintf8(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments)
	{
		return Vsnprintf(pDestination, n, pFormat, arguments);
	}

	EASTDC_API int Vsnprintf16(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments)
	{
		return Vsnprintf(pDestination, n, pFormat, arguments);
	}

	EASTDC_API int Vsnprintf32(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments)
	{
		return Vsnprintf(pDestination, n, pFormat, arguments);
	}

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		EASTDC_API int VsnprintfW(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, va_list arguments)
		{
			return Vsnprintf(pDestination, n, pFormat, arguments);
		}
	#endif

#endif

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
	EASTDC_API int Vcprintf(WriteFunctionW pWriteFunctionW, void* EA_RESTRICT pContext, const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vcprintf(reinterpret_cast<WriteFunction16>(pWriteFunctionW), pContext, reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vcprintf(reinterpret_cast<WriteFunction32>(pWriteFunctionW), pContext, reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vfprintf(FILE* EA_RESTRICT pFile, const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vfprintf(pFile, reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vfprintf(pFile, reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vprintf(const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vprintf(reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vprintf(reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vsprintf(wchar_t* EA_RESTRICT pDestination, const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vsprintf(reinterpret_cast<char16_t *>(pDestination), reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vsprintf(reinterpret_cast<char32_t *>(pDestination), reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vsnprintf(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vsnprintf(reinterpret_cast<char16_t *>(pDestination), n, reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vsnprintf(reinterpret_cast<char32_t *>(pDestination), n, reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vscprintf(const wchar_t* EA_RESTRICT pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vscprintf(reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vscprintf(reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Cprintf(WriteFunctionW pWriteFunction, void* EA_RESTRICT pContext, const wchar_t* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vcprintf(pWriteFunction, pContext, pFormat, arguments);

		va_end(arguments);

		return result;
	}

	EASTDC_API int Fprintf(FILE* EA_RESTRICT pFile, const wchar_t* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vfprintf(pFile, pFormat, arguments);

		va_end(arguments);

		return result;
	}

	EASTDC_API int Printf(const wchar_t* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vprintf(pFormat, arguments);

		va_end(arguments);

		return result;
	}

	EASTDC_API int Sprintf(wchar_t* EA_RESTRICT pDestination, const wchar_t* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vsprintf(pDestination, pFormat, arguments);

		va_end(arguments);

		return result;
	}

	EASTDC_API int Snprintf(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vsnprintf(pDestination, n, pFormat, arguments);

		va_end(arguments);

		return result;
	}

#endif

} // namespace StdC
} // namespace EA


EA_RESTORE_VC_WARNING()



