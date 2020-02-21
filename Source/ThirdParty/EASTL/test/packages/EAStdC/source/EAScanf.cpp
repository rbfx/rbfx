///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/internal/ScanfCore.h>


namespace EA
{
namespace StdC
{

///////////////////////////////////////////////////////////////////////////////
// char functions
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Cscanf(ReadFunction8 pReadFunction8, void* pContext, const char* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(pReadFunction8, pContext, pFormat, arguments); 
}

EASTDC_API int Fscanf(FILE* pFile, const char* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader8, pFile, pFormat, arguments); 
}

EASTDC_API int Scanf(const char* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader8, stdin, pFormat, arguments); 
}

EASTDC_API int Sscanf(const char* pDestination, const char* pFormat, ...)
{
	ScanfLocal::SscanfContext8 sc(pDestination);

	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader8, &sc, pFormat, arguments); 
}


EASTDC_API int Vcscanf(ReadFunction8 pReadFunction8, void* pContext, const char* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(pReadFunction8, pContext, pFormat, arguments); 
}

EASTDC_API int Vfscanf(FILE* pFile, const char* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader8, pFile, pFormat, arguments); 
}

EASTDC_API int Vscanf(const char* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader8, stdin, pFormat, arguments); 
}

EASTDC_API int Vsscanf(const char* pDestination, const char* pFormat, va_list arguments)
{
	ScanfLocal::SscanfContext8 sc(pDestination);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader8, &sc, pFormat, arguments); 
}



///////////////////////////////////////////////////////////////////////////////
// char16_t functions
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Cscanf(ReadFunction16 pReadFunction16, void* pContext, const char16_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(pReadFunction16, pContext, pFormat, arguments);
}

EASTDC_API int Fscanf(FILE* pFile, const char16_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader16, pFile, pFormat, arguments); 
}

EASTDC_API int Scanf(const char16_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader16, stdin, pFormat, arguments); 
}

EASTDC_API int Sscanf(const char16_t* pDestination, const char16_t* pFormat, ...)
{
	ScanfLocal::SscanfContext16 sc(pDestination);

	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader16, &sc, pFormat, arguments); 
}


EASTDC_API int Vcscanf(ReadFunction16 pReadFunction16, void* pContext, const char16_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(pReadFunction16, pContext, pFormat, arguments); 
}

EASTDC_API int Vfscanf(FILE* pFile, const char16_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader16, pFile, pFormat, arguments); 
}

EASTDC_API int Vscanf(const char16_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader16, stdin, pFormat, arguments); 
}

EASTDC_API int Vsscanf(const char16_t* pDestination, const char16_t* pFormat, va_list arguments)
{
	ScanfLocal::SscanfContext16 sc(pDestination);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader16, &sc, pFormat, arguments); 
}


///////////////////////////////////////////////////////////////////////////////
// char32_t functions
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int Cscanf(ReadFunction32 pReadFunction32, void* pContext, const char32_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(pReadFunction32, pContext, pFormat, arguments);
}

EASTDC_API int Fscanf(FILE* pFile, const char32_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader32, pFile, pFormat, arguments); 
}

EASTDC_API int Scanf(const char32_t* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader32, stdin, pFormat, arguments); 
}

EASTDC_API int Sscanf(const char32_t* pDestination, const char32_t* pFormat, ...)
{
	ScanfLocal::SscanfContext32 sc(pDestination);

	va_list arguments;
	va_start(arguments, pFormat);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader32, &sc, pFormat, arguments); 
}


EASTDC_API int Vcscanf(ReadFunction32 pReadFunction32, void* pContext, const char32_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(pReadFunction32, pContext, pFormat, arguments); 
}

EASTDC_API int Vfscanf(FILE* pFile, const char32_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader32, pFile, pFormat, arguments); 
}

EASTDC_API int Vscanf(const char32_t* pFormat, va_list arguments)
{
	return ScanfLocal::VscanfCore(ScanfLocal::FILEReader32, stdin, pFormat, arguments); 
}

EASTDC_API int Vsscanf(const char32_t* pDestination, const char32_t* pFormat, va_list arguments)
{
	ScanfLocal::SscanfContext32 sc(pDestination);

	return ScanfLocal::VscanfCore(ScanfLocal::StringReader32, &sc, pFormat, arguments); 
}

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
	EASTDC_API int Cscanf(ReadFunctionW pReadFunctionW, void* pContext, const wchar_t* pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vcscanf(pReadFunctionW, pContext, pFormat, arguments);

		va_end(arguments);
		return result;
	}

	EASTDC_API int Fscanf(FILE* pFile, const wchar_t* pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vfscanf(pFile, pFormat, arguments);

		va_end(arguments);
		return result;
	}

	EASTDC_API int Scanf(const wchar_t* pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vscanf(pFormat, arguments);

		va_end(arguments);
		return result;
	}

	EASTDC_API int Sscanf(const wchar_t* pTextBuffer, const wchar_t* pFormat, ...)
	{
		va_list arguments;
		va_start(arguments, pFormat);

		int result = Vsscanf(pTextBuffer, pFormat, arguments);

		va_end(arguments);
		return result;
	}

	EASTDC_API int Vcscanf(ReadFunctionW pReadFunctionW, void* pContext, const wchar_t* pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vcscanf(reinterpret_cast<ReadFunction16>(pReadFunctionW), pContext, reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vcscanf(reinterpret_cast<ReadFunction32>(pReadFunctionW), pContext, reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vfscanf(FILE* pFile, const wchar_t* pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vfscanf(pFile, reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vfscanf(pFile, reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vscanf(const wchar_t* pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vscanf(reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vscanf(reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

	EASTDC_API int Vsscanf(const wchar_t* pTextBuffer, const wchar_t* pFormat, va_list arguments)
	{
		#if (EA_WCHAR_SIZE == 2)
			return Vsscanf(reinterpret_cast<const char16_t *>(pTextBuffer), reinterpret_cast<const char16_t *>(pFormat), arguments);
		#else
			return Vsscanf(reinterpret_cast<const char32_t *>(pTextBuffer), reinterpret_cast<const char32_t *>(pFormat), arguments);
		#endif
	}

#endif


} // namespace StdC
} // namespace EA





