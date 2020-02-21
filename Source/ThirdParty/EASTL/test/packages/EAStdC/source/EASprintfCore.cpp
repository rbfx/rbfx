///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/SprintfCore.h>
#include <EAStdC/EAMathHelp.h>
#include <EAStdC/EAString.h>
#include <EAAssert/eaassert.h>
#include <string.h>
#include <float.h>
#include <math.h>
#if defined(EA_PLATFORM_ANDROID)
	#include <android/log.h>
#endif

//include for OutputDebugStringA
#if defined(EASTDC_OUTPUTDEBUGSTRING_ENABLED) && EASTDC_OUTPUTDEBUGSTRING_ENABLED
		EA_DISABLE_ALL_VC_WARNINGS();
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
			#define EASTDC_WIN32_LEAN_AND_MEAN_TEMP_DEF
		#endif
		#include <windows.h>
		#if defined(EASTDC_WIN32_LEAN_AND_MEAN_TEMP_DEF)
			#undef WIN32_LEAN_AND_MEAN
			#undef EASTDC_WIN32_LEAN_AND_MEAN_TEMP_DEF
		#endif
		EA_RESTORE_ALL_VC_WARNINGS();
#endif

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4127)  // conditional expression is constant.
#endif

// To do: switch this to instead use <EABase/eastdarg.h>'s va_list_reference at about Summer of 2015:
#if (defined(__GNUC__) && (defined(__x86__) || defined(__x86_64__)))
	#define VA_LIST_REFERENCE(arguments) ((va_list*) arguments)
#else
	#define VA_LIST_REFERENCE(arguments) ((va_list*)&arguments)
#endif



namespace EA
{
namespace StdC
{
namespace SprintfLocal
{


int StringWriter8(const char* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8, WriteFunctionState /*wfs*/)
{
	using namespace SprintfLocal;

	SnprintfContext8* const pSnprintfContext8 = (SnprintfContext8*)pContext8;

	if(nCount && !pSnprintfContext8->mbMaxCountReached) // If there is anything to write and if we haven't already exhausted the limit for this context...
	{
		if(nCount > (pSnprintfContext8->mnMaxCount - pSnprintfContext8->mnCount)) // If nCount results in an exhausting of the limit...
		{
			pSnprintfContext8->mbMaxCountReached = true; // Note that it is possible due to non-breakable multi-byte sequences that mnCount will be < mnMaxCount.

			// We must check for (UTF8) MBCS sequences here. We cannot write a
			// partial multi-byte sequence, but can only write a contiguous sequence.
			const size_t nRoom = pSnprintfContext8->mnMaxCount - pSnprintfContext8->mnCount;
			size_t i = 0;

			while (i < nCount)
			{
				size_t nClusterSize;

				if((uint8_t)pData[i] < 0xc2)
					nClusterSize = 1;                
				else if((uint8_t)pData[i] < 0xe0)
					nClusterSize = 2;
				else if((uint8_t)pData[i] < 0xf0)
					nClusterSize = 3;
				else
					break; // Unknown size. Fail the cluster.

				if (i + nClusterSize > nRoom)
					break; // Out of room in our destination buffer.

				i += nClusterSize;
			}

			nCount = i;
		}

		memcpy(pSnprintfContext8->mpDestination + pSnprintfContext8->mnCount, pData, nCount * sizeof(*pData));
		pSnprintfContext8->mnCount += nCount;

		return (int)(unsigned)nCount;
	}

	return 0;
}

int StringWriter16(const char16_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext16, WriteFunctionState /*wfs*/)
{
	using namespace SprintfLocal;

	SnprintfContext16* const pSnprintfContext16 = (SnprintfContext16*)pContext16;

	if(nCount > (pSnprintfContext16->mnMaxCount - pSnprintfContext16->mnCount))  // If nCount results in an exhausting of the limit...
		nCount = pSnprintfContext16->mnMaxCount - pSnprintfContext16->mnCount;

	memcpy(pSnprintfContext16->mpDestination + pSnprintfContext16->mnCount, pData, nCount * sizeof(char16_t));
	pSnprintfContext16->mnCount += nCount;

	return (int)(unsigned)nCount;
}

int StringWriter32(const char32_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext32, WriteFunctionState /*wfs*/)
{
	using namespace SprintfLocal;

	SnprintfContext32* const pSnprintfContext32 = (SnprintfContext32*)pContext32;

	if(nCount > (pSnprintfContext32->mnMaxCount - pSnprintfContext32->mnCount))  // If nCount results in an exhausting of the limit...
		nCount = pSnprintfContext32->mnMaxCount - pSnprintfContext32->mnCount;

	memcpy(pSnprintfContext32->mpDestination + pSnprintfContext32->mnCount, pData, nCount * sizeof(char32_t));
	pSnprintfContext32->mnCount += nCount;

	return (int)(unsigned)nCount;
}




int FILEWriter8(const char* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8, WriteFunctionState /*wfs*/)
{
	FILE* const pFile = (FILE*)pContext8;
	const size_t nResult = fwrite(pData, sizeof(char), nCount, pFile); // It's OK if nCount == 0.
	if(nResult == nCount)
		return (int)(unsigned)nResult;
	return -1;
}

int FILEWriter16(const char16_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext16, WriteFunctionState /*wfs*/)
{
	FILE* const pFile = (FILE*)pContext16;
	const size_t nResult = fwrite(pData, sizeof(char16_t), nCount, pFile); // It's OK if nCount == 0.
	if(nResult == nCount)
		return (int)(unsigned)nResult;
	return -1;
}

int FILEWriter32(const char32_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext32, WriteFunctionState /*wfs*/)
{
	FILE* const pFile = (FILE*)pContext32;
	const size_t nResult = fwrite(pData, sizeof(char32_t), nCount, pFile); // It's OK if nCount == 0.
	if(nResult == nCount)
		return (int)(unsigned)nResult;
	return -1;
}



///////////////////////////////////////////////////////////////////////////////
// PlatformLogWriter8
//
int PlatformLogWriter8(const char* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8, WriteFunctionState)
{
	#if defined(EA_PLATFORM_ANDROID)
		// The __android_log_write function appends a \n to every call you make to it. This is a problem for 
		// us because during a sprintf of a single string we call our Writer multiple times. If we just called
		// __android_log_write for every Writer call, a single sprintf would be split across multiple trace lines,
		// and \n are also (I think) ignored.


		size_t countOriginal = nCount;

		if(nCount)
		{
			const size_t kBufferSize               = EAArrayCount(PlatformLogWriterContext8::mBuffer);
			const size_t kBufferSizeActual         = kBufferSize - 1;           // -1 because we need to save space for a terminating 0 char that __android_log_write wants.
			const size_t kPlatformBufferSize       = 512;                       // This is the size that's the max size the platform's log-writing function can handle.
			const size_t kPlatformBufferSizeActual = kPlatformBufferSize - 1;   // -1 because we need to save space for a terminating 0 char that __android_log_write wants.
			const size_t kMaxCountActual           = (kBufferSizeActual < kPlatformBufferSizeActual) ? kBufferSizeActual : kPlatformBufferSizeActual;
			PlatformLogWriterContext8 *pWriteInfo  = reinterpret_cast<PlatformLogWriterContext8*>(pContext8);

			// Pick the smaller of the two buffers.

			for(size_t i = 0; i < nCount; i++)
			{
				pWriteInfo->mBuffer[pWriteInfo->mPosition] = pData[i];

				if((pData[i] == '\n') || (pWriteInfo->mPosition == kMaxCountActual)) // If the user is requesting a newline or if we have exhausted the most we can write in a single line...
				{
					if(pWriteInfo->mPosition == kMaxCountActual)
						pWriteInfo->mPosition++;
					pWriteInfo->mBuffer[pWriteInfo->mPosition] = 0;
					__android_log_write(ANDROID_LOG_INFO, "EAStdC.Printf", pWriteInfo->mBuffer);
					pWriteInfo->mPosition  = 0;
					pWriteInfo->mBuffer[0] = 0;
				}
				else
					pWriteInfo->mPosition++;
			}
		}

		return (int)(unsigned)countOriginal;
	#else
		EA_UNUSED(pContext8);
		// To do: buffer debug writes and flush the buffer at WriteFunctionState == kWFSEnd, because otherwise a 
		// single Dprintf call could result in numerous calls to (for example) OutputDebugString instead of just one call.
		// A good way to do this is to have the buffer be part of the void* context; that way we don't have to worry about
		// having thread-local storage.
		if(nCount)
		{
			if(pData[nCount] == 0) // If already 0-terminated...
			{
				#if EASTDC_OUTPUTDEBUGSTRING_ENABLED
					OutputDebugStringA(pData);
				#else
					fputs(pData, stdout);

					#if defined(EA_PLATFORM_MOBILE)
						fflush(stdout); // Mobile platforms need this because otherwise you can easily lose output if the device crashes while not all the output has been written.
					#endif
				#endif
			}
			else
			{
				// Copy to a buffer first, taking into account that nCount may be larger than our buffer size.
				size_t i, iEnd, charIndex = 0;
				char   buffer[512];

				while(charIndex < nCount)
				{
					for(i = 0, iEnd = (EAArrayCount(buffer) - 1); (i < iEnd) && (charIndex < nCount); ++i, ++charIndex)
						buffer[i] = pData[charIndex];
					buffer[i] = 0;

					#if EASTDC_OUTPUTDEBUGSTRING_ENABLED
						OutputDebugStringA(buffer);
					#else
						fputs(buffer, stdout);

						#if defined(EA_PLATFORM_MOBILE)
							fflush(stdout); // Mobile platforms need this because otherwise you can easily lose output if the device crashes while not all the output has been written.
						#endif
					#endif
				}
			}
		}

		return static_cast<int>(nCount);

	#endif
}


///////////////////////////////////////////////////////////////////////////////
// EASprintfInit
//
void EASprintfInit()
{
}

///////////////////////////////////////////////////////////////////////////////
// EASprintfShutdown
//
void EASprintfShutdown()
{
}

///////////////////////////////////////////////////////////////////////////////
// Helper template to avoid including EASTL
//
template<typename T, typename U>
struct IsSameType
{
	enum { value = 0 };
};

template<typename T>
struct IsSameType<T, T>
{
	enum { value = 1 };
};

///////////////////////////////////////////////////////////////////////////////
// WriteLeftPadding 
// 
// If the formatted data is right aligned, then this function prefixes the 
// output with the appropriate data.
//
template <typename CharT>
static int WriteLeftPadding(int(*pWriteFunction)(const CharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
	void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, const CharT*& pBufferData, int nWriteCount)
{
	if(fd.mAlignment == kAlignmentLeft || fd.mnWidth <= nWriteCount)
		return 0;

	CharT nFill;

	if(fd.mAlignment == kAlignmentZeroFill)
	{
		nFill = '0';

		if(pBufferData && ((*pBufferData == '+') || (*pBufferData == '-') || (*pBufferData == ' ')))
		{    // Skip zero fill for leading sign character.
			if(pWriteFunction(pBufferData, 1, pWriteFunctionContext, kWFSIntermediate) == -1)
				return -1; // This is an error; not the same as running out of space.
			--nWriteCount;
			++pBufferData;
		}
	}
	else
		nFill = ' ';

	int nFillCount = fd.mnWidth - nWriteCount;
	for(int i = 0; i < nFillCount; ++i)
	{
		// Consider making an optimization which writes more than one fill character at a time.
		// We can do this by using the space at the beginning of pBuffer;
		if(pWriteFunction(&nFill, 1, pWriteFunctionContext, kWFSIntermediate) == -1)
			return -1; // This is an error; not the same as running out of space.
	}
	return nFillCount;
}

///////////////////////////////////////////////////////////////////////////////
// WriteRightPadding 
// 
// If the formatted data is left aligned, then this function suffixes the 
// output with the appropriate data.
//
template <typename CharT>
static int WriteRightPadding(int(*pWriteFunction)(const CharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
	void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, int nWriteCount)
{
	if(fd.mAlignment != kAlignmentLeft || fd.mnWidth <= nWriteCount)
		return 0;

	const CharT nSpace = ' ';
	int nFillCount = fd.mnWidth - nWriteCount;
	for(int i = 0; i < nFillCount; ++i)
	{
		if(pWriteFunction(&nSpace, 1, pWriteFunctionContext, kWFSIntermediate) == -1)
			return -1; // This is an error; not the same as running out of space.
	}
	return nFillCount;
}

///////////////////////////////////////////////////////////////////////////////
// WriteBuffer 
// 
// Write the given buffer with the required left and right padding
//
template <typename CharT>
static int WriteBuffer(int(*pWriteFunction)(const CharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
	void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, const CharT* pBufferData, int nWriteCount)
{
	const CharT* pBufferDataEnd = pBufferData + nWriteCount;
	int nWriteCountCurrent = nWriteCount; // We will write at least as much nWriteCount, possibly more if the format specified a specific width.

	int nFillCount = WriteLeftPadding(pWriteFunction, pWriteFunctionContext, fd, pBufferData, nWriteCount);
	if(nFillCount < 0)
		return -1; // This is an error; not the same as running out of space.
	nWriteCountCurrent += nFillCount;

	if(pBufferData != pBufferDataEnd && (pWriteFunction(pBufferData, pBufferDataEnd - pBufferData, pWriteFunctionContext, kWFSIntermediate) == -1))
		return -1; // This is an error; not the same as running out of space.

	nFillCount = WriteRightPadding(pWriteFunction, pWriteFunctionContext, fd, nWriteCountCurrent);
	if(nFillCount < 0)
		return -1; // This is an error; not the same as running out of space.
	nWriteCountCurrent += nFillCount;
	return nWriteCountCurrent;
}
	
///////////////////////////////////////////////////////////////////////////////
// StringNull 
// 
// Based on the character type, return the appropriate (null) string.
//
template <typename CharT>
const CharT* StringNull();

template <>
const char* StringNull<char>() { return kStringNull8; }

template <>
const char16_t* StringNull<char16_t>() { return kStringNull16; }

template <>
const char32_t* StringNull<char32_t>() { return kStringNull32; }


///////////////////////////////////////////////////////////////////////////////
// StringFormatLength
//
template <typename CharT>
int StringFormatLength(const SprintfLocal::FormatData& fd, const CharT* pInBufferData)
{

	// For strings, the precision modifier refers to the number of chars to display.
	if(fd.mnPrecision != kNoPrecision) // If the user specified some precision...
	{
		// Find which is shorter, the length or the precision.
		const CharT*       pBufferCurrent = pInBufferData;
		const CharT* const pBufferMax = pInBufferData + fd.mnPrecision;

		while((pBufferCurrent < pBufferMax) && *pBufferCurrent)
			++pBufferCurrent;
		return (int)(pBufferCurrent - pInBufferData);
	}
	else
	{
		// Set the write count to be the string length. 
		// Don't call strlen, as that would jump outside this module.
		const CharT* pBufferCurrent = pInBufferData;

		while(*pBufferCurrent) // Implement a small inline strlen.
			++pBufferCurrent;
		return (int)(pBufferCurrent - pInBufferData);
	}
}

///////////////////////////////////////////////////////////////////////////////
// StringFormatHelper
//
// This is a structure template since we want to do partial specialization and it could be the case that some 
// compilers don't support it.  There are two forms of the implementation.  The first is when the
// in and out types are the same.  In that case we can use the input buffer as the output buffer and do
// no conversions.  When they are different, we are required to convert from one encoding to another.
// 
template <bool IsSameType, typename InCharT, typename OutCharT>
struct StringFormatHelper
{};

template <typename InCharT, typename OutCharT>
struct StringFormatHelper<true, InCharT, OutCharT>
{
	int operator()(int(*pWriteFunction)(const OutCharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
		void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, OutCharT* pScratchBuffer, const InCharT* pInBufferData)
	{
		int nWriteCount = StringFormatLength(fd, pInBufferData);
		return WriteBuffer(pWriteFunction, pWriteFunctionContext, fd, pInBufferData, nWriteCount);
	}
};

template <typename InCharT, typename OutCharT>
struct StringFormatHelper<false, InCharT, OutCharT>
{
	int operator()(int(*pWriteFunction)(const OutCharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
		void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, OutCharT* pScratchBuffer, const InCharT* pInBufferData)
	{
		// We have a problem here. We are converting from one encoding to another.
		// The only encoding that are supported are UTF-8, UCS-2 and UCS-4.  Any 
		// other encoding can result in a loss of data or failure to reencode.

		// Compute the input number of code units
		int nInCodeUnits;
		{
			const InCharT* pInBufferCurrent = pInBufferData;
			while(*pInBufferCurrent) // Implement a small inline strlen.
				++pInBufferCurrent;
			nInCodeUnits = (int)(pInBufferCurrent - pInBufferData);
		}
		const InCharT* pInBufferDataEnd = pInBufferData + nInCodeUnits;
		
		int nPrecision = fd.mnPrecision == kNoPrecision ? INT_MAX : fd.mnPrecision; // Don't assume the value of kNoPrecision
																					
		// If the input is empty or precision is zero, then write an empty buffer and return
		if(nInCodeUnits == 0 || nPrecision == 0)
		{
			return WriteBuffer(pWriteFunction, pWriteFunctionContext, fd, pScratchBuffer, 0);
		}

		int nWriteCountSum = 0;
		bool bFirstTime = true;
		while(nPrecision != 0 && pInBufferData != pInBufferDataEnd)
		{

			// Compute the required output size.  Truncate if we have a precision
			size_t outSize = (size_t)(unsigned)kConversionBufferSize;
			if((size_t)nPrecision < outSize)
			{
				outSize = (size_t)(unsigned)fd.mnPrecision + 1;
				nPrecision = 0;
			}
			else
				nPrecision -= kConversionBufferSize - 1;

			// Convert the string
			size_t nOutUsed;
			size_t nInUsed;
			if(!Strlcpy(pScratchBuffer, pInBufferData, outSize, pInBufferDataEnd - pInBufferData, nOutUsed, nInUsed))
				break;

			// If we haven't done the left padding
			if(bFirstTime)
			{
				int nWriteCount = static_cast<int>(nOutUsed);
				if(nPrecision != 0 && nInUsed < (size_t)nInCodeUnits)
				{
					int nRemaining = Strlcpy((OutCharT*)nullptr, pInBufferData + nInUsed, 0, nInCodeUnits - nInUsed);
					if(nRemaining < 0)
						break;
					nWriteCount += nRemaining;
					if(fd.mnPrecision != kNoPrecision && fd.mnPrecision < nWriteCount)
						nWriteCount = fd.mnPrecision;
				}

				const OutCharT* pTemp = pScratchBuffer;
				int nFillCount = WriteLeftPadding(pWriteFunction, pWriteFunctionContext, fd, pTemp, nWriteCount);
				EA_ASSERT(pTemp == pScratchBuffer); // should not get adjusted
				if(nFillCount < 0)
					return -1;
				nWriteCountSum += nFillCount;

				bFirstTime = false;
			}

			// Write the converted buffer
			if(nOutUsed != 0 && (pWriteFunction(pScratchBuffer, nOutUsed, pWriteFunctionContext, kWFSIntermediate) == -1))
				return -1; // This is an error; not the same as running out of space.
			nWriteCountSum += static_cast<int>(nOutUsed);

			// Advance the input
			pInBufferData += nInUsed;
		}

		// If required, write the end
		if(!bFirstTime)
		{
			int nFillCount = WriteRightPadding(pWriteFunction, pWriteFunctionContext, fd, nWriteCountSum);
			if(nFillCount < 0)
				return -1;
			nWriteCountSum += nFillCount;
		}

		return nWriteCountSum;
	}
};


///////////////////////////////////////////////////////////////////////////////
// StringFormat
//
// Check for a null input and/or reencode the input as requred.  The output string
// length is returned and pointed to by ppOutBufferData.
// 
template <typename InCharT, typename OutCharT>
int StringFormat(int(*pWriteFunction)(const OutCharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs),
	void* EA_RESTRICT pWriteFunctionContext, const SprintfLocal::FormatData& fd, OutCharT* pScratchBuffer, const InCharT* pInBufferData)
{
	// I don't see the C99 standard specifying the behaviour for a NULL string pointer, 
	// but both GCC and MSVC use "(null)" when such a NULL pointer is encountered.
	if(pInBufferData == nullptr)
	{
		StringFormatHelper<true, OutCharT, OutCharT> helper;
		return helper(pWriteFunction, pWriteFunctionContext, fd, pScratchBuffer, StringNull<OutCharT>());
	}
	else
	{
		StringFormatHelper<IsSameType<InCharT, OutCharT>::value, InCharT, OutCharT> helper;
		return helper(pWriteFunction, pWriteFunctionContext, fd, pScratchBuffer, pInBufferData);
	}
}


///////////////////////////////////////////////////////////////////////////////
// ReadFormat
//
// Reads the current format into FormatData. Return value is pointer to first
// char after the format data.
//
// To know how printf truly needs to work, see the ISO C 1999 standard, section 7.19.6.1.
// See http://www.cplusplus.com/ref/cstdio/printf.html or http://www.opengroup.org/onlinepubs/007908799/xsh/fprintf.html 
// for decent basic online documentation about how printf is supposed to work.
// 
// Argument pFormat is a string pointing to a % format specification of the form:
//        %[flags][width][.precision][modifiers]type
//
///////////////////////////////////////////////////////////////////////////////
//
template <typename CharT>
const CharT* ReadFormat(const CharT* EA_RESTRICT pFormat, SprintfLocal::FormatData* EA_RESTRICT pFormatData, va_list* EA_RESTRICT pArguments)
{
	using namespace SprintfLocal;

	const CharT*   pFormatCurrent = pFormat;
	Alignment      alignmentNonZeroFill = kAlignmentLeft; // We have a chicken and egg problem in that the zero-fill alignment may or may not be ignored. So we save the value here for what the alignment would be if zero-fill needs to be ignored.
	FormatData     fd;
	CharT          c;

	// Check for "%%". This is a quick test for early exit.
	if((c = *++pFormatCurrent) == '%')    // If the user specified "%%"...
	{
		fd.mnType = '%';
		*pFormatData = fd;    // pFormatData->mnType = '%'; Consider instead using just this line of code.
		return pFormatCurrent + 1;
	}

	// Check for flags field
	for(; ; (c = *++pFormatCurrent)) // Check for one or more flags ('-', '+', ' ', '#', or '0')
	{
		switch(c)
		{
			case '-': // '-' controls alignment, not the +/- sign before numbers.
				fd.mAlignment = kAlignmentLeft;
				break;

			case '+':
				fd.mSign = kSignMinusPlus;
				break;

			case ' ':    // The C99 language states (7.19.6.1.6): "If the space and + flags both appear, the space flag is ignored."
				if(fd.mSign != kSignMinusPlus)
				   fd.mSign  = kSignSpace;
				break;

			case '#':    // The C99 standard states (7.19.6.1.6): The result is converted to an "alternative form."
				fd.mbAlternativeForm = true; 
				break;

			case '\'':   // Non-standard but common extension. e.g. http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/printf.3.html
				fd.mbDisplayThousands = true;
				break;

			case '0':    // The C99 standard states (7.19.6.1.6): If the 0 and - flags both appear, the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a precision is specified, the 0 flag is ignored. For other conversions, the behavior is undefined.
				if(fd.mAlignment != kAlignmentLeft)     // Note that '0' is a flag (which can appear in any order) and not part of the number. This is a common misconception.
				{
					if(fd.mAlignment != kAlignmentZeroFill) // The C99 standard says that for string fields, 0 fill means space fill.
						alignmentNonZeroFill = fd.mAlignment;
					fd.mAlignment  = kAlignmentZeroFill;
				}
				break;

			default:
				goto EndFlagCheck; // C/C++ unfortunately don't provide an efficient mechanism to break from multiple loops other than 'goto'. We could avoid the goto with alternative logic, but that would be less efficient.
		}
	}
	EndFlagCheck:

	// Check for width field. 
	// The C99 standard states (7.19.6.1.5): A field width, or precision, or both, may be 
	// indicated by an asterisk. In this case, an int argument supplies the field width or 
	// precision. The arguments specifying field width, or precision, or both, shall appear 
	// (in that order) before the argument (if any) to be converted. A negative field 
	// width argument is taken as a - flag followed by a positive field width. 
	// A negative precision argument is taken as if the precision were omitted.
	if(c == '*')
	{
		fd.mnWidth = va_arg(*pArguments, int);
		if(fd.mnWidth < 0)
		{
			fd.mAlignment = kAlignmentLeft; // Pretend that a '-' flag was applied, as specified by the standard.
			fd.mnWidth    = -fd.mnWidth;
		}
		c = *++pFormatCurrent;
	}
	else
	{
		// Read the width numerical value. We don't do error checking here as it 
		// would incur a performance penalty that just isn't worth it to us.
		while(((unsigned)(c - '0')) < 10)    // Don't call isdigit() here because that might cause a data cache miss.
		{
			fd.mnWidth = (int)((fd.mnWidth * 10) + (c - '0')); // Consider if there is any way to make this loop more 
			c = *++pFormatCurrent;                             // efficient by reducing multiplies, etc.
		}
	}

	if(fd.mnWidth > kMaxWidth)
	{
		// Note that we leave fd.mnType as zero, indicating an error.
		*pFormatData = fd; // pFormatData->mnType = 0; Consider instead using just this line of code.
		return pFormatCurrent + 1;
	}

	// Check for precision field.
	// An optional precision that gives the minimum number of digits to appear for the 
	// d, i, o, u, x, and X conversions, the number of digits to appear after the decimal-point
	// character for a, A, e, E, f, and F conversions, the maximum number of significant
	// digits for the g and G conversions, or the maximum number of bytes to be written for
	// s conversions. The precision takes the form of a period (.) followed either by an
	// asterisk * (described later) or by an optional decimal integer; if only the period 
	// is specified, the precision is taken as zero. If a precision appears with any other
	// conversion specifier, the behavior is undefined.
	if(c == (CharT)pFormatData->mDecimalPoint) // If precision is specified...
	{
		if((c = *++pFormatCurrent) == '*') // If the precision is set as a value passed in as an argument...
		{
			fd.mnPrecision = va_arg(*pArguments, int);
			if(fd.mnPrecision < 0)
				fd.mnPrecision = 0;
			c = *++pFormatCurrent;
		}
		else
		{
			fd.mnPrecision = 0;
			while(((unsigned)(c - '0')) < 10) // Don't call isdigit() here because that might cause a data cache miss.
			{
				// Consider doing error checking 
				fd.mnPrecision = (int)((fd.mnPrecision * 10) + (c - '0'));
				c = *++pFormatCurrent;
			}
		}
	}

	// Check for length modifier field. C99 standard section 7.19.6.1.7.
	// We support the following modifiers, which include non-standard integer size-specific modifiers.
	//     hh, h, l, ll, I8, I16, I32, I64, I128
	bool bModifierPresent = true; // True until proven false below.

	switch(c)
	{
		case 'h':
			if(pFormatCurrent[1] == 'h') // If the user specified %hh ...
			{
				// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a signed char or unsigned char argument (the argument will have been promoted according to the integer promotions, but its value shall be converted to signed char or unsigned char before printing); or that a following n conversion specifier applies to a pointer to a signed char argument.
				fd.mModifier = kModifierChar;
				c = *++pFormatCurrent;
			}
			else // Else the user specified just %h
			{
				// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a short int or unsigned short int argument (the argument will have been promoted according to the integer promotions, but its value shall be converted to short int or unsigned short int before printing); or that a following n conversion specifier applies to a pointer to a short int argument.
				fd.mModifier = kModifierShort;
			}
			break;

		case 'l': // Check for ell (not one).
			if(pFormatCurrent[1] == 'l') // If the user specified %ll ...
			{
				// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a long long int or unsigned long long int argument; or that a following n conversion specifier applies to a pointer to a long long int argument.
				fd.mModifier = kModifierLongLong;
				c = *++pFormatCurrent;
			}
			else // Else the user specified just %l
			{
				// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a long int or unsigned long int argument; that a following n conversion specifier applies to a pointer to a long int argument; that a following c conversion specifier applies to a wint_t argument; that a following s conversion specifier applies to a pointer to a wchar_t argument; or has no effect on a following a, A, e, E, f, F, g, or G conversion specifier.
				fd.mModifier = kModifierLong;
			}
			break;

		case 'q': 
			// BSD-based OS's use %q to indicate "quad int", which is the same as "long long". We need to support it because iPhone's C99 headers define PRId64 as "qd". 
			fd.mModifier = kModifierLongLong;
			break;

		case 'j':
			// Specifies that a following d, i, o, u, x, or X conversion specifier applies to an intmax_t or uintmax_t argument; or that a following n conversion specifier applies to a pointer to an intmax_t argument.
			fd.mModifier = kModifierMax_t;
			break;

		case 'z':
			// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a size_t or the corresponding signed integer type argument; or that a following n conversion specifier applies to a pointer to a signed integer type corresponding to size_t argument.
			fd.mModifier = kModifierSize_t;
			break;

		case 't':
			// Specifies that a following d, i, o, u, x, or X conversion specifier applies to a ptrdiff_t or the corresponding unsigned integer type argument; or that a following n conversion specifier applies to a pointer to a ptrdiff_t argument.
			fd.mModifier = kModifierPtrdiff_t;
			break;

		case 'L':
			// Specifies that a following a, A, e, E, f, F, g, or G conversion specifier applies to a long double argument.
			fd.mModifier = kModifierLongDouble;
			break;

		case 'I':
			if(pFormatCurrent[1] == '8') // If the user specified %I8 ...
			{
				fd.mModifier = kModifierInt8;
				c = *++pFormatCurrent; // Account for the '8' part of I8. We'll account for the 'I' part below for all formats.
			}
			else if((pFormatCurrent[1] == '1') && (pFormatCurrent[2] == '6'))
			{
				fd.mModifier = kModifierInt16;                    
				c = *(pFormatCurrent += 2);
			}
			else if((pFormatCurrent[1] == '3') && (pFormatCurrent[2] == '2'))
			{
				fd.mModifier = kModifierInt32;                    
				c = *(pFormatCurrent += 2);
			}
			else if((pFormatCurrent[1] == '6') && (pFormatCurrent[2] == '4'))
			{
				fd.mModifier = kModifierInt64;                    
				c = *(pFormatCurrent += 2); // Account for the '64' part of I64. We'll account for the 'I' part below for all formats.
			}
			else if((pFormatCurrent[1] == '1') && (pFormatCurrent[2] == '2') && (pFormatCurrent[3] == '8'))
			{
				fd.mModifier = kModifierInt128;                    
				c = *(pFormatCurrent += 3);
			}
			else // Else the specified modifier was invalid.
			{
				// Note that we leave fd.mnType as zero, indicating an error.
				*pFormatData = fd; // pFormatData->mnType = kFormatError; // Consider instead using just this line of code.
				return pFormatCurrent + 1;
			}
			break;

		default:
			bModifierPresent = false;
	}

	if(bModifierPresent)
		c = *++pFormatCurrent; // Move the pointer past the format (e.g. the 'f' in "%3.1f")

	// Read the conversion type. This must be present.
	fd.mnType = (int)c;

	switch(c)
	{
		case 'b': // unsigned binary. This is a convenient extension that we add.
		case 'd': // signed
		case 'i': // signed
		case 'u': // unsigned
		case 'o': // unsigned
		case 'x': // unsigned
		case 'X': // unsigned
			if(fd.mnPrecision == kNoPrecision)
				fd.mnPrecision = 1;
			else if(fd.mAlignment == kAlignmentZeroFill)
				fd.mAlignment = kAlignmentRight;
			break;

		case 'g':
		case 'G':
			if(fd.mnPrecision == 0) // For %g, if the precision is zero, it is taken as 1.
			   fd.mnPrecision = 1;
			// fall through
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'a':  // See the C99 standard, section 7.19.6.1.8, for details on 'a' formatting.
		case 'A':
			if(fd.mnPrecision == kNoPrecision)
			   fd.mnPrecision = 6;    // The C99 standard explicitly states that this defaults to 6.
			break;

		case 'p':
			if(sizeof(void*) == 2)
				fd.mModifier = kModifierInt16;
			else if(sizeof(void*) == 4)
				fd.mModifier = kModifierInt32;
			else
				fd.mModifier = kModifierInt64;
			fd.mnPrecision = sizeof(void*) / 4;  // This is 8 for a 32 bit pointer, 16 for a 64 bit pointer.
			fd.mnType      = 'x';

			// For the "alternative form" of x (or X) conversion, a nonzero result has 0x (or 0X) prefixed to it.
			// So if the user uses %#p, then the user gets something like 0x12345678, whereas otherwise the 
			// user gets just 12345678.

			break;

		case 'c': // We accept %hc, %c, %lc, %I8c, %I16c, %I32c (regular, regular, wide, char, char16_t, char32_t)
		case 'C': // We accept %hC, %C, %lC, %I8C, %I16C, %I32C (regular, wide,    wide, char, char16_t, char32_t)
		case 's': // We accept %hs, %s, %ls, %I8s, %I16s, %I32s (regular, regular, wide, char, char16_t, char32_t)
		case 'S': // We accept %hS, %S, %lS, %I8s, %I16s, %I32s (regular, wide,    wide, char, char16_t, char32_t)
			// If the user specified zero-fill above, then it is a mistake and we 
			// need to use spaces instead. So we restore the fallback alignment.
			if(fd.mAlignment == kAlignmentZeroFill)
				fd.mAlignment = alignmentNonZeroFill;

			// Microsoft's library goes against the standard: %s is 
			// interpreted to mean char string but instead is interpreted 
			// to be either char or wchar_t depending on what the output
			// text format is. This is non-standard but has the convenience
			// of allowing users to migrate between char and wchar_t usage
			// more easily. So we allow EASPRINTF_MS_STYLE_S_FORMAT to control this.
			if(fd.mModifier == kModifierShort)
				fd.mModifier = kModifierChar;
			else if(fd.mModifier == kModifierLong)
				fd.mModifier = kModifierWChar;
			else if(fd.mModifier == kModifierNone) // If the user hasn't already specified, for example %I16.
			{
				#if EASPRINTF_MS_STYLE_S_FORMAT
					if((c == 's') || (c == 'c'))
						fd.mModifier = (sizeof(*pFormat) == sizeof(char)) ? kModifierChar : kModifierWChar;

					else
						fd.mModifier = (sizeof(*pFormat) == sizeof(char)) ? kModifierWChar : kModifierChar;
				#else
					if((c == 's') || (c == 'c'))
						fd.mModifier = kModifierChar;
					else
						fd.mModifier = kModifierWChar;
				#endif
			}
			//else if((fd.mModifier < kModifierInt8) || (fd.mModifier > kModifierInt32)) // This expression assumes that Int8, Int16, Int32 are contiguous enum values.
			//{
			//    EA_FAIL_MSG("Printf: Invalid %s modifier");
			//}
			break;

		case 'n':
			// The argument shall be a pointer to signed integer into which is written the 
			// number of characters written to the output stream so far by this call to printf. 
			// No argument is converted, but one is consumed. If the conversion specification 
			// includes any flags, a field width, or a precision, the behavior is undefined.

			// We don't really have anything to do here, as this function is merely reading
			// the format and not acting on it. The caller will act on this appropriately.
			break;
	}

	// If the field width is too long and it's not a string field...
	if((fd.mnPrecision > kMaxPrecision) && (fd.mnPrecision != kNoPrecision) && ((fd.mnType != 's') && (fd.mnType != 'S')))
		fd.mnType = kFormatError;

	// Note that at his point we haven't read the argument corresponding to the formatted value. 
	// We save this for the parent function, as otherwise we'd need some kind of union here to
	// hold all value types.
	*pFormatData = fd;
	return pFormatCurrent + 1;
}


///////////////////////////////////////////////////////////////////////////////
// WriteLongHelper
//
// Writes the given lValue to the given buffer and returns the start of the 
// data or returns NULL for error. Note that the buffer is passed in as the 
// end of the buffer and not the beginning. This is a common trick used when
// converting integer values to strings, as the conversion algorithm needs
// to work backwards as it is and it's quicker to simply start with the end
// of the buffer and move backwards.
//
template <typename CharT, typename ValueT, typename UValueT>
CharT* WriteLongHelper(const SprintfLocal::FormatData& fd, ValueT lValue, CharT* EA_RESTRICT pBufferEnd)
{
	using namespace SprintfLocal;

	UValueT       ulValue = (UValueT)lValue;
	unsigned int  nBase;
	unsigned int  nShift = 0;
	unsigned int  nAnd = 0;
	Sign          sign = kSignNone;
	CharT*        pCurrent = pBufferEnd;
	int           nDigitCount = 0;
	int           nDigitCountSum = fd.mnPrecision;
	bool          bNegative = false;

	*--pCurrent = 0;

	if((lValue > 0) || (fd.mnPrecision > 0) || fd.mbAlternativeForm)
	{
		// Do initial setup. 
		switch(fd.mnType)
		{
			case 'b': // Binary (this is non-standard, though many would like it to be so)
				nBase  = 2;
				nShift = 0x01;
				nAnd   = 0x01;
				break;

			case 'o': // Octal
				nBase  = 8;
				nShift = 0x03;
				nAnd   = 0x07;
				break;

			case 'd': // Decimal (signed)
			case 'i': // i and d are defined by the standard to be the same.
			default:
				nBase = 10;
				sign  = fd.mSign;
				if(lValue < 0)
				{
					ulValue   = (UValueT)-lValue;
					bNegative = true;
				}
				break;

			case 'u': // Decimal (unsigned)
				nBase = 10;
				break;

			case 'x': // Hexidecimal
			case 'X':
				nBase  = 16;
				nShift = 0x04;
				nAnd   = 0x0f;
				break;
		}

		// Write the individual digits.
		// To do: Use the optimization in EAString.cpp's X64toaCommon10 function 
		// to significantly improve writing base 10 strings. 
		// Optimization: Replace the % and / below with >> and & for cases where
		// we can do this (i.e. nBase = even power of two).
		do
		{
			int nDigit;

			if(nBase == 10)
			{
				nDigit = (int)(ulValue % nBase);
				ulValue /= nBase;
			}
			else // Else take faster pathway.
			{
				nDigit = (int)(ulValue & nAnd);
				ulValue >>= nShift;
			}

			if(nDigit < 10)
				nDigit = '0' + nDigit;
			else
			{
				nDigit -= 10;
				if(fd.mnType == 'x')
					nDigit = 'a' + nDigit;
				else
					nDigit = 'A' + nDigit;
			}

			*--pCurrent = (CharT)nDigit;
			++nDigitCount;

			if((nBase == 10) && (fd.mbDisplayThousands) && (ulValue > 0) && (((nDigitCount + 1) % 4) == 0))
			{
				*--pCurrent = (CharT)fd.mThousandsSeparator;
				++nDigitCount; // Even though the thousands separator isn't strictly a digit, it counts towards the space used by the number, which is what matters here.
			}
		} while(ulValue > 0);

		// For octal mode, the standard specifies that when 'alternative form' is enabled, 
		// the number is prefixed with a zero. This is like how the C language interprets 
		// integers that begin with zero as octal. 
		if((nBase == 8) && fd.mbAlternativeForm && (*pCurrent != (CharT)'0'))
		{
			*--pCurrent = (CharT)'0';
			++nDigitCount;
		}

		// Calculate any leading zeroes required by the 'zero fill' alignment option.
		if(fd.mAlignment == kAlignmentZeroFill) // If we are to precede the number with zeroes...
		{
			if(bNegative || (sign != kSignNone))
				nDigitCountSum = fd.mnWidth - 1; // Take into account the leading sign that we'll need to write.
			else if(fd.mbAlternativeForm && ((nBase == 2) || (nBase == 16)))
				nDigitCountSum = fd.mnWidth - 2; // Take into account the leading "0x" that we'll need to write.
			else
				nDigitCountSum = fd.mnWidth;
		}

		// Write in any leading zeroes as required by the precision specifier(or the zero fill alignment option). 
		while(nDigitCount < nDigitCountSum)
		{
			*--pCurrent = (CharT)'0';
			++nDigitCount;
		}

		// Potentially add the sign prefix, which might be either nothing, '-', '+', or ' '.
		if(nBase == 10) // Signs can only apply to decimal types.
		{
			if((fd.mnType == 'd') || (fd.mnType == 'i')) // The standard seems to say that only signed decimal types can have signs.
			{
				if(bNegative)
					*--pCurrent = (CharT)'-';
				else if(fd.mSign == kSignMinusPlus)
					*--pCurrent = (CharT)'+';
				else if(fd.mSign == kSignSpace)
					*--pCurrent = (CharT)' ';
			}
		}
		else if(fd.mbAlternativeForm && ((nBase == 2) || (nBase == 16)))
		{
			// Add the leading 0x, 0X, 0b, or 0B (for our binary extension).
			*--pCurrent = (CharT)fd.mnType;
			*--pCurrent = (CharT)'0';
		}
	}

	return pCurrent;
}


///////////////////////////////////////////////////////////////////////////////
// WriteLong
//
// Writes the given lValue to the given buffer and returns the start of the 
// data or returns NULL for error. Note that the buffer is passed in as the 
// end of the buffer and not the beginning. This is a common trick used when
// converting integer values to strings, as the conversion algorithm needs
// to work backwards as it is and it's quicker to simply start with the end
// of the buffer and move backwards.
//
template <typename CharT>
CharT* WriteLong(const SprintfLocal::FormatData& fd, long lValue, CharT* EA_RESTRICT pBufferEnd)
{
	return WriteLongHelper<CharT, long, unsigned long>(fd, lValue, pBufferEnd);
}


///////////////////////////////////////////////////////////////////////////////
// WriteLongLong
//
// This function is identical to WriteLong except that it works with long long
// instead of long. It is a separate function because on many platforms the
// long long data type is larger than the (efficient) machine word size and is
// thus to be avoided if possible.
//
template <typename CharT>
CharT* WriteLongLong(const SprintfLocal::FormatData& fd, long long lValue, CharT* EA_RESTRICT pBufferEnd)
{
	return WriteLongHelper<CharT, long long, unsigned long long> (fd, lValue, pBufferEnd);
}


///////////////////////////////////////////////////////////////////////////////
// WriteDouble
//
template <typename CharT>
CharT* WriteDouble(const SprintfLocal::FormatData& fd, double dValue, CharT* EA_RESTRICT pBufferEnd)
{
	using namespace SprintfLocal;

	// Check for nan or inf strings.
	if(IsNAN(dValue))
	{
		*--pBufferEnd = 0;
		if(fd.mnType >= 'a') // If type is f, e, g, a, and not F, E, G, A.
		{
			*--pBufferEnd = 'n';
			*--pBufferEnd = 'a';
			*--pBufferEnd = 'n';
		}
		else
		{
			*--pBufferEnd = 'N';
			*--pBufferEnd = 'A';
			*--pBufferEnd = 'N';
		}
		if(IsNeg(dValue))
			*--pBufferEnd = '-';
		return pBufferEnd;
	}
	else if(IsInfinite(dValue))
	{
		*--pBufferEnd = 0;
		if(fd.mnType >= 'a') // If type is f, e, g, a, and not F, E, G, A.
		{
			*--pBufferEnd = 'f';
			*--pBufferEnd = 'n';
			*--pBufferEnd = 'i';
		}
		else
		{
			*--pBufferEnd = 'F';
			*--pBufferEnd = 'N';
			*--pBufferEnd = 'I';
		}
		if(IsNeg(dValue))
			*--pBufferEnd = '-';
		return pBufferEnd;
	}

	// Regular processing.
	int       nType = fd.mnType;
	int       nPrecision = fd.mnPrecision;
	bool      bStripTrailingZeroes = false;      // If true, then don't write useless trailing zeroes, a with the three at the end of: "1.23000"
	bool      bStripPointlessDecimal = false;    // If true, then don't write a decimal point that has no digits after it, as with: "13."
	CharT*    pResult = NULL;

	*--pBufferEnd = 0;

	if (nPrecision <= kConversionBufferSize) // If there is enough space for the data...
	{
		int      nDecimalPoint, nSign, nExponent;
		CharT    pBufferCvt[kFcvtBufMaxSize]; pBufferCvt[0] = 0;
		int      nBufferLength;
		CharT*   pCurrent = pBufferEnd;

		switch(nType)
		{
			default :
			case 'g':
			case 'G':
			{
				// From section 7.19.6.1.8:
				// A double argument representing a floating-point number is converted in
				// style f or e (or in style F or E in the case of a G conversion specifier), with
				// the precision specifying the number of significant digits. If the precision is
				// zero, it is taken as 1. The style used depends on the value converted; style e
				// (or E) is used only if the exponent resulting from such a conversion is less
				// than -4 or greater than or equal to the precision. Trailing zeros are removed
				// from the fractional portion of the result unless the # flag is specified; a
				// decimal-point character appears only if it is followed by a digit.
				// 
				// A double argument representing an infinity or NaN is converted in the style
				// of an f or F conversion specifier.

				// %g differs from %e how we pass nPrecision to EcvtBuf.
				EcvtBuf(dValue, nPrecision, &nDecimalPoint, &nSign, pBufferCvt);
				nExponent = nDecimalPoint - 1; // Exponent can be a positive, zero, or negative value.

				if(!fd.mbAlternativeForm)
					bStripTrailingZeroes = true;
				bStripPointlessDecimal = true;

				if(!((nExponent < -4) || (nExponent >= nPrecision)))
				{
					if(!IsSameType<CharT, char>::value)
					{
						if(nExponent >= 0) // If there are any digits to the left of the decimal...
							nPrecision -= (nExponent + 1);
					}
					goto FType;
				}

				if(nType == 'g')
					nType = 'e';
				else
					nType = 'E';
				goto EContinuation;
			} // case g:

			case 'e':
			case 'E':
			{
				// From section 7.19.6.1.8:
				// A double argument representing a floating-point number is converted in the
				// style [-]d.ddd edd, where there is one digit (which is nonzero if the
				// argument is nonzero) before the decimal-point character and the number of
				// digits after it is equal to the precision; if the precision is missing, it is 
				// taken as 6; if the precision is zero and the # flag is not specified, no decimal-point
				// character appears. The value is rounded to the appropriate number of digits.
				// The E conversion specifier produces a number with E instead of e
				// introducing the exponent. The exponent always contains at least two digits,
				// and only as many more digits as necessary to represent the exponent. If the
				// value is zero, the exponent is zero.
				// 
				// A double argument representing an infinity or NaN is converted in the style
				// of an f or F conversion specifier.

				EcvtBuf(dValue, nPrecision + 1, &nDecimalPoint, &nSign, pBufferCvt);
				nExponent = nDecimalPoint - 1; // Exponent can be a positive, zero, or negative value.
				if(dValue == 0) // Should we instead compare dValue to a very small value?
					nExponent = 0; // In this case we are working with the value 0, which is always displayed as 0.???e+00

			EContinuation:
				nBufferLength = (int)Strlen(pBufferCvt);

				// Write the exponent digits, at least two of them.
				int nExponentAbs = abs(nExponent);
				while(nExponentAbs > 0)
				{
					*--pCurrent = (CharT)('0' + (nExponentAbs % 10));
					nExponentAbs /= 10;
				}
				if(pCurrent >= (pBufferEnd - 1)) // The C99 standard states that at least two digits are present in the exponent, even if they are either or both zeroes.
					*--pCurrent = '0';
				if(pCurrent >= (pBufferEnd - 1))
					*--pCurrent = '0';

				// Write the decimal point sign, always + or -.
				if(nExponent >= 0)
					*--pCurrent = '+';
				else
					*--pCurrent = '-';

				// Write 'e' or 'E'.
				*--pCurrent = (CharT)nType;

				// Write all digits but the first one.
				for(CharT* pTemp = pBufferCvt + nBufferLength; pTemp > pBufferCvt + 1; )
				{
					const CharT c = *--pTemp;

					if(c != '0') // As we move right to left, as soon as we encounter a non-'0', we are done with trialing zeroes.
						bStripTrailingZeroes = false;
					if((c != '0') || !bStripTrailingZeroes)
						*--pCurrent = c;
				}

				// Write the decimal point.
				if((*pCurrent != (CharT)nType) || !bStripPointlessDecimal) // If bStripPointlessDecimal is true, then draw decimal only if there are digits after it.
				{
					if((nBufferLength > 1) || fd.mbAlternativeForm) // If the 'alternative form' is set, then always show a decimal.
						*--pCurrent = (CharT)fd.mDecimalPoint;
				}

				// Write the first digit.
				*--pCurrent = pBufferCvt[0];
				break;
			} // case e:

			case 'f':
			case 'F':
			FType:
			{
				// From section 7.19.6.1.8:
				// A double argument representing a floating-point number is converted to
				// decimal notation in the style [-]ddd.ddd, where the number of digits after
				// the decimal-point character is equal to the precision specification. 
				// If the precision is missing, it is taken as 6; if the precision is zero 
				// and the # flag is not specified, no decimal-point character appears. 
				// If a decimal-point character appears, at least one digit appears before it. 
				// The value is rounded to the appropriate number of digits.
				//
				// A double argument representing an infinity is converted in one of the 
				// styles [-]inf or [-]infinity  which style is implementation-defined. 
				// A double argument representing a NaN is converted in one of the styles
				// [-]nan or [-]nan(n-char-sequence)  which style, and the meaning of
				// any n-char-sequence, is implementation-defined. The F conversion specifier
				// produces INF, INFINITY, or NAN instead of inf, infinity, or nan,
				// respectively.)

				FcvtBuf(dValue, nPrecision, &nDecimalPoint, &nSign, pBufferCvt);
				nBufferLength = (int)Strlen(pBufferCvt);

				// If the 'alternative form' is set, then always show a decimal.
				if(fd.mbAlternativeForm && (nDecimalPoint >= nBufferLength) && !bStripPointlessDecimal)
					*--pCurrent = (CharT)fd.mDecimalPoint;

				// Write the values that are after the decimal point.
				const CharT* const pDecimalPoint = pBufferCvt + nDecimalPoint - 1;
				const CharT*       pCurrentSource = pBufferCvt + nBufferLength - 1;

				if((pCurrentSource - pDecimalPoint) > nPrecision) // If dValue is very small, this may be true.
					pCurrentSource = pDecimalPoint + nPrecision;

				while(pCurrentSource > pDecimalPoint)
				{
					CharT c;

					if((pCurrentSource >= pBufferCvt) && (pCurrentSource <= (pBufferCvt + nBufferLength)))
						c = *pCurrentSource;
					else
						c = '0';

					if(c != '0') // As we move right to left, as soon as we encounter a non-'0', we are done with trialing zeroes.
						bStripTrailingZeroes = false;

					if((c != '0') || !bStripTrailingZeroes)
						*--pCurrent = c;
					--pCurrentSource;
				}

				// Write the decimal point.
				if(*pCurrent || !bStripPointlessDecimal) // If bStripPointlessDecimal is true, then draw decimal only if there is something after it.
				{
					if(nDecimalPoint < nBufferLength) // The standard specifies to not write the decimal point if the precision is zero. 
						*--pCurrent = (CharT)fd.mDecimalPoint;
				}

				// Write the values that are before the decimal point.
				if(nDecimalPoint > 0) // If there is any integral part to this number...
				{
					int nDigitCount = 0;
					pCurrentSource = pBufferCvt + nDecimalPoint;

					while(pCurrentSource > pBufferCvt)
					{
						*--pCurrent = *--pCurrentSource;
						++nDigitCount;

						if((fd.mbDisplayThousands) && (pCurrentSource > pBufferCvt) && ((nDigitCount % 3) == 0))
							*--pCurrent = (CharT)fd.mThousandsSeparator;
					}
				}
				else
					*--pCurrent = '0'; // Write the "0" before the decimal point, as in "0.1234".
				break;
			} // case f:
		} // switch

		// Write a sign character.
		if(nSign)
			*--pCurrent = '-';
		else if(fd.mSign == kSignMinusPlus)
			*--pCurrent = '+';
		else if(fd.mSign == kSignSpace)
			*--pCurrent = ' ';

		// Write leading spaces.
		if(fd.mAlignment == kAlignmentRight)
		{
			// Write in any leading spaces as required by the width specifier (or the zero fill alignment option). 
			int nWidth = (int)(intptr_t)(pBufferEnd - pCurrent);

			while(nWidth < fd.mnWidth)
			{
				*--pCurrent = (CharT)' ';
				++nWidth;
			}
		}

		pResult = pCurrent;
	}

	return pResult;
}


///////////////////////////////////////////////////////////////////////////////
// VprintfCore
//

template <typename CharT>
int VprintfCoreInternal(int(*pWriteFunction)(const CharT* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext, WriteFunctionState wfs), void* EA_RESTRICT pWriteFunctionContext, const CharT* EA_RESTRICT pFormat, va_list arguments)
{
	using namespace SprintfLocal;

	const CharT*        pFormatCurrent = pFormat;   // Current position within entire format string.
	const CharT*        pFormatSpec;                // Current format specification (e.g. "%3.2f").
	FormatData          fd;
	int                 nWriteCount;
	int                 nWriteCountSum = 0;
	int                 nWriteCountCurrent;
	CharT               pBuffer[kConversionBufferSize + 1]; // The +1 isn't necessary but placates code analysis tools.
	CharT* const        pBufferEnd = pBuffer + kConversionBufferSize;
	const CharT*        pBufferData = NULL;       // Where within pBuffer8 the data we are interested in begins.
	long                lValue = 0;                // All known supported platforms have 'long' support in hardware. 'int' is always only 32 bits (even on 64 bit systems).
	unsigned long       ulValue = 0;               // 
	long long           llValue = 0;               // Most compilers support 'long long' at this point. VC++ v6 and earlier are notable exceptions.
	unsigned long long  ullValue = 0;              // 

	pWriteFunction(NULL, 0, pWriteFunctionContext, kWFSBegin);

	// We walk through the format string and echo characters to the output until we 
	// come across a % specifier, at which point we process it and then move on as before.
	while(*pFormatCurrent)
	{
		// Find the next format specification (or end of the string).
		pFormatSpec = pFormatCurrent;
		while(*pFormatSpec && (*pFormatSpec != '%'))
			++pFormatSpec;

		// Write out non-formatted text
		nWriteCount = (int)(pFormatSpec - pFormatCurrent);
		if(nWriteCount)
		{
			if(pWriteFunction(pFormatCurrent, (size_t)nWriteCount, pWriteFunctionContext, kWFSIntermediate) == -1)
				goto FunctionError; // This is an error; not the same as running out of space.
			nWriteCountSum += nWriteCount;
			pFormatCurrent = pFormatSpec;
		}

		if(*pFormatSpec)
		{
			pFormatCurrent = ReadFormat(pFormatSpec, &fd, VA_LIST_REFERENCE(arguments));

			switch(fd.mnType)
			{
				case 'd':    // These are signed values.
				case 'i':    // The standard specifies that 'd' and 'i' are identical.
				{
					if(fd.mModifier == kModifierLongLong)
						llValue = va_arg(arguments, long long); // If the user didn't pass a long long, unexpected behaviour will occur.
					else if((fd.mModifier == kModifierLong) || (fd.mModifier == kModifierLongDouble)) // It is questionable whether we should allow %Ld here as we do. The standard doesn't define this behaviour.
						lValue = va_arg(arguments, long);
					else if(fd.mModifier == kModifierInt64)
					{
						if(sizeof(int64_t) == sizeof(long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							lValue = va_arg(arguments, long);
						}
						else if(sizeof(int64_t) == sizeof(long long))
						{
							fd.mModifier = kModifierLongLong;
							llValue = va_arg(arguments, long long);
						}
					}
					else if (fd.mModifier == kModifierMax_t)
					{
						if (sizeof(intmax_t) == sizeof(long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							lValue = va_arg(arguments, long);
						}
						else if (sizeof(intmax_t) == sizeof(long long))
						{
							fd.mModifier = kModifierLongLong;
							llValue = va_arg(arguments, long long);
						}
					}
					else if (fd.mModifier == kModifierSize_t)
					{
						if (sizeof(size_t) == sizeof(long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							lValue = (long) va_arg(arguments, unsigned long);
						}
						else if (sizeof(size_t) == sizeof(long long))
						{
							fd.mModifier = kModifierLongLong;
							llValue = (long) va_arg(arguments, unsigned long long);
						}
					}
					else if (fd.mModifier == kModifierPtrdiff_t)
					{
						if (sizeof(ptrdiff_t) == sizeof(long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							lValue = va_arg(arguments, long);
						}
						else if (sizeof(ptrdiff_t) == sizeof(long long))
						{
							fd.mModifier = kModifierLongLong;
							llValue = va_arg(arguments, long long);
						}
					}
					else if(fd.mModifier == kModifierInt128)
					{
						if(sizeof(int64_t) < sizeof(long long)) // If long long is 128 bits... (we don't test sizeof(int128_t) because there may be no such thing. Hopefully there is no int96_t.
							llValue = va_arg(arguments, long long);
						else
						{
							// We have a problem here. The user wants to print a 128 bit value but 
							// there is no built-in type to support this. For the time being, we 
							// simply use only 64 bits of data. If we really need this, we can
							// add the functionality later. We have the EAStdC int128_t type we can use.
							// I don't think calling two 64 bit va_args is the same as what a single
							// 128 bit arg would be. If we are using EAStdC int128_t then we handle the 
							// value the same as passing a struct by value. And that's compiler/ABI-specific.
							llValue = va_arg(arguments, long long);
							llValue = va_arg(arguments, long long);
						}
					}
					else // else we have kModifierChar, kModifierShort, kModifierInt8, kModifierInt16, kModifierInt32.
					{
						// COMPILE_TIME_ASSERT(sizeof(int32_t) <= sizeof(int));
						lValue = va_arg(arguments, int);
						if((fd.mModifier == kModifierShort) || (fd.mModifier == kModifierInt16))
							lValue = (long)(signed short)lValue; // We carefully do our casting here in order to preserve intent.
						else if((fd.mModifier == kModifierChar) || (fd.mModifier == kModifierInt8))
							lValue = (long)(signed char)lValue;  // We carefully do our casting here in order to preserve intent.
					}

					if(fd.mModifier == kModifierLongLong)
					{
						pBufferData = WriteLongLong(fd, llValue, pBufferEnd);
						if(!pBufferData)
							goto FormatError;
					}
					else
					{
						pBufferData = WriteLong(fd, lValue, pBufferEnd);
						if(!pBufferData)
							goto FormatError;
					}

					nWriteCount = (int)((pBufferEnd - pBufferData) - 1); // -1 because the written string is 0-terminated and we don't want to write the final 0.
					break;
				}

				case 'b': // 'b' means binary. This is a convenient extension that we provide.
				case 'o': // These are unsigned values.
				case 'u':
				case 'x':
				case 'X':
				{
					if(fd.mModifier == kModifierLong)
						ulValue = va_arg(arguments, unsigned long);
					else if(fd.mModifier == kModifierLongLong)
						ullValue = va_arg(arguments, unsigned long long);
					else if(fd.mModifier == kModifierInt64)
					{
						if(sizeof(uint64_t) == sizeof(unsigned long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							ulValue = va_arg(arguments, unsigned long);
						}
						else if(sizeof(uint64_t) == sizeof(unsigned long long))
						{
							fd.mModifier = kModifierLongLong;
							ullValue = va_arg(arguments, unsigned long long);
						}
					}
					else if (fd.mModifier == kModifierMax_t)
					{
						if (sizeof(uintmax_t) == sizeof(unsigned long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							ulValue = va_arg(arguments, unsigned long);
						}
						else if (sizeof(uintmax_t) == sizeof(unsigned long long))
						{
							fd.mModifier = kModifierLongLong;
							ullValue = va_arg(arguments, unsigned long long);
						}
					}
					else if (fd.mModifier == kModifierSize_t)
					{
						if (sizeof(size_t) == sizeof(unsigned long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							ulValue = va_arg(arguments, unsigned long);
						}
						else if (sizeof(size_t) == sizeof(unsigned long long))
						{
							fd.mModifier = kModifierLongLong;
							ullValue = va_arg(arguments, unsigned long long);
						}
					}
					else if (fd.mModifier == kModifierPtrdiff_t)
					{
						if (sizeof(ptrdiff_t) == sizeof(unsigned long))
						{
							// fd.mModifier == kModifierLong; -- Not necessary, as the logic below doesn't need this.
							ulValue = (unsigned long long) va_arg(arguments, long);
						}
						else if (sizeof(ptrdiff_t) == sizeof(unsigned long long))
						{
							fd.mModifier = kModifierLongLong;
							ullValue = (unsigned long long) va_arg(arguments, long long);
						}
					}
					else if(fd.mModifier == kModifierInt128)
					{
						if(sizeof(uint64_t) < sizeof(unsigned long long)) // If long long is 128 bits... (we don't test sizeof(int128_t) because there may be no such thing. Hopefully there is no int96_t.
							ullValue = va_arg(arguments, unsigned long long);
						else
						{
							// We have a problem here. The user wants to print a 128 bit value but 
							// there is no built-in type to support this. For the time being, we 
							// simply use only 64 bits of data. If we really need this, we can
							// add the functionality later.
							#ifdef EA_SYSTEM_BIG_ENDIAN
									 (void)va_arg(arguments, unsigned long long);
								ullValue = va_arg(arguments, unsigned long long);
							#else
								ullValue = va_arg(arguments, unsigned long long);
									 (void)va_arg(arguments, unsigned long long);
							#endif
						}
					}
					else // else we have kModifierChar, kModifierShort, kModifierInt8, kModifierInt16, kModifierInt32.
					{
						ulValue = va_arg(arguments, unsigned int);
						if((fd.mModifier == kModifierShort) || (fd.mModifier == kModifierInt16))
							ulValue = (unsigned long)(unsigned short)ulValue; // We carefully do our casting here in order to preserve intent.
						else if((fd.mModifier == kModifierChar) || (fd.mModifier == kModifierInt8))
							ulValue = (unsigned long)(unsigned char)ulValue;  // We carefully do our casting here in order to preserve intent.
					}

					// Now do the actual writing of the data.
					if(fd.mModifier == kModifierLongLong)
					{
						pBufferData = WriteLongLong(fd, (long long)ullValue, pBufferEnd);
						if(!pBufferData)
							goto FormatError;
					}
					else
					{
						pBufferData = WriteLong(fd, (long)ulValue, pBufferEnd);
						if(!pBufferData)
							goto FormatError;
					}

					nWriteCount = (int)((pBufferEnd - pBufferData) - 1); // -1 because the written string is 0-terminated and we don't want to write the final 0.
					break;
				}

				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 'a': // See the C99 standard, section 7.19.6.1.8, for details on 'a' formatting.
				case 'A':
				{
					// Since on most systems long double is the same as double, it's really no big deal to just work
					// with long double, much like we do with long int instead of int above.
					if(fd.mModifier == kModifierLongDouble)
					{
						long double ldValue = va_arg(arguments, long double);
						pBufferData = WriteDouble(fd, static_cast<double>(ldValue), pBufferEnd);
					}
					else
					{
						double dValue = va_arg(arguments, double);
						pBufferData = WriteDouble(fd, dValue, pBufferEnd);
					}

					if(!pBufferData)
						goto FormatError;

					nWriteCount = (int)((pBufferEnd - pBufferData) - 1); // -1 because the written string is 0-terminated and we don't want to write the final 0.
					break;
				}

				case 's':
				case 'S':
				{
					int stringTypeSize;

					switch (fd.mModifier)
					{
						case kModifierInt8:         // If the user specified %I8s or %I8S
						case kModifierChar:         // If the user specified %hs or %hS or kModifierWChar was chosen implicitly for other reasons.
							stringTypeSize = 1;
							break;

						case kModifierInt16:        // If the user specified %I16s or %I16S
							stringTypeSize = 2;
							break;

						case kModifierInt32:        // If the user specified %I32s or %I32S
							stringTypeSize = 4;
							break;

						case kModifierWChar:        // If the user specified %ls or %lS or kModifierWChar was chosen implicitly for other reasons.
							stringTypeSize = sizeof(wchar_t);
							break;

						default:                    // If the user specified %I64s or %I64S or another invalid size.
							goto FormatError;
					}

					switch (stringTypeSize)
					{
						case 1:
						{
							const char* pBufferData8 = va_arg(arguments, char*);
							nWriteCount = StringFormat<char, CharT>(pWriteFunction, pWriteFunctionContext, fd, pBuffer, pBufferData8);
							if (nWriteCount < 0)
								goto FormatError;
							nWriteCountSum += nWriteCount;
							break;
						} // case 1

						case 2:
						{
							const char16_t* pBufferData16 = va_arg(arguments, char16_t*);
							nWriteCount = StringFormat<char16_t, CharT>(pWriteFunction, pWriteFunctionContext, fd, pBuffer, pBufferData16);
							if (nWriteCount < 0)
								goto FormatError;
							nWriteCountSum += nWriteCount;
							break;
						} // case 2

						case 4:
						{
							const char32_t* pBufferData32 = va_arg(arguments, char32_t*);
							nWriteCount = StringFormat<char32_t, CharT>(pWriteFunction, pWriteFunctionContext, fd, pBuffer, pBufferData32);
							if (nWriteCount < 0)
								goto FormatError;
							nWriteCountSum += nWriteCount;
							break;
						} // case 4

					} // switch (stringTypeSize)

					continue; // Skip the appending of the buffer.  This is being done inside the StringFormat routine
				} // case 's'

				case 'n': // %n %hn %ln %lln %I64n %I32n %I16n, %I8n, %jn, %tn, %zn etc.
				{
					// In this case, we write the number of chars written so far to the passed in argument.
					void* pCountBufferData = va_arg(arguments, void*);

					switch (fd.mModifier)
					{
						case kModifierInt8:
						case kModifierChar:
							*(char*)pCountBufferData = (char)nWriteCountSum;
							break;

						case kModifierInt16:
						case kModifierShort:
							*(int16_t*)pCountBufferData = (int16_t)nWriteCountSum;
							break;

						case kModifierInt32:
							*(int32_t*)pCountBufferData = (int32_t)nWriteCountSum;
							break;

						case kModifierInt64:
							*(int64_t*)pCountBufferData = (int32_t)nWriteCountSum;
							break;

						case kModifierLong:
							*(long*)pCountBufferData = (long)nWriteCountSum;
							break;

						case kModifierLongLong:
							*(long long*)pCountBufferData = (long long)nWriteCountSum;
							break;

						case kModifierPtrdiff_t:
							*(ptrdiff_t*)pCountBufferData = (ptrdiff_t)nWriteCountSum;
							break;

						case kModifierSize_t:
							*(size_t*)pCountBufferData = (size_t)nWriteCountSum;
							break;

						case kModifierMax_t:
							*(intmax_t*)pCountBufferData = (intmax_t)nWriteCountSum;
							break;

							//case kModifierInt128:       // We really should generate an error with this. It's nearly pointless to support it.
							//    // Fall through
							//
							//case kModifierWChar:        // This should be impossible to encounter.
							//case kModifierLongDouble:   // This should be impossible to encounter.
							//    // Fall through

						case kModifierNone:
						default:
							*(int*)pCountBufferData = (int)nWriteCountSum;
							break;
					}
					continue; // Intentionally continue instead break.
				} // case 'n'

				case 'c':
				case 'C':
				{
					int charTypeSize;

					switch (fd.mModifier)
					{
						case kModifierInt8:         // If the user specified %I8c or %I8c
						case kModifierChar:         // If the user specified %hc or %hC or kModifierWChar was chosen implicitly for other reasons.
							charTypeSize = 1;
							break;

						case kModifierInt16:        // If the user specified %I16c or %I16C
							charTypeSize = 2;
							break;

						case kModifierInt32:        // If the user specified %I32c or %I32C
							charTypeSize = 4;
							break;

						case kModifierWChar:        // If the user specified %lc or %lC or kModifierWChar was chosen implicitly for other reasons.
							charTypeSize = sizeof(wchar_t);
							break;

						default:                    // If the user specified %I64c or %I64C or another invalid size.
							goto FormatError;
					}

					// In some cases, we are losing data here or doing an improper or incomplete encoding conversion.
					switch (charTypeSize)
					{
					case 1:
					{
						const char c8 = (char)va_arg(arguments, int); // We make the assumption here that sizeof(char16_t) is <= sizeof(int) and thus that a char16_t argument was promoted to int.
						pBuffer[0] = (CharT)(uint32_t)c8;
						pBufferData = pBuffer;
						nWriteCount = 1;
						break;
					}
					case 2:
					{
						const char16_t c16 = (char16_t)va_arg(arguments, unsigned int);
						pBuffer[0] = (CharT)(uint32_t)c16;
						pBufferData = pBuffer;
						nWriteCount = 1;
						break;
					}
					case 4:
					{
						const char32_t c32 = (char32_t)va_arg(arguments, unsigned int);
						pBuffer[0] = (CharT)(uint32_t)c32; 
						pBufferData = pBuffer;
						nWriteCount = 1;
						break;
					}
					}

					break;
				} // case 'c'

				case '%':
				{
					// In this case we just write a single '%' char to the output.
					pBuffer[0] = '%';
					pBufferData = pBuffer;
					nWriteCount = 1;
					break;
				}

				case kFormatError:
				default:
				FormatError:
					// We do what many printf implementations do here and simply print out the text
					// as-is -- as if there wasn't any formatting specifier. This at least lets the
					// user see what was (incorrectly) specified.
					nWriteCount = (int)(pFormatCurrent - pFormatSpec);
					nWriteCountSum += nWriteCount;

					if(nWriteCount && (pWriteFunction(pFormatSpec, (size_t)nWriteCount, pWriteFunctionContext, kWFSIntermediate) == -1))
						goto FunctionError; // This is an error; not the same as running out of space.
					continue; // Try to continue displaying further data.
			}

			nWriteCountCurrent = WriteBuffer(pWriteFunction, pWriteFunctionContext, fd, pBufferData, nWriteCount);
			if (nWriteCountCurrent < 0)
				goto FunctionError; // This is an error; not the same as running out of space.

			nWriteCountSum += nWriteCountCurrent;

		} // if(*pFormatSpec)

	} // while(*pFormatCurrent)

	pWriteFunction(NULL, 0, pWriteFunctionContext, kWFSEnd);
	return nWriteCountSum;

FunctionError:
	pWriteFunction(NULL, 0, pWriteFunctionContext, kWFSEnd);
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// VprintfCore
//
int VprintfCore(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pWriteFunctionContext8, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return VprintfCoreInternal(pWriteFunction8, pWriteFunctionContext8, pFormat, arguments);
}

int VprintfCore(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pWriteFunctionContext16, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return VprintfCoreInternal(pWriteFunction16, pWriteFunctionContext16, pFormat, arguments);
}

int VprintfCore(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pWriteFunctionContext32, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return VprintfCoreInternal(pWriteFunction32, pWriteFunctionContext32, pFormat, arguments);
}


} // namespace SprintfLocal
} // namespace StdC
} // namespace EA


// For bulk build friendliness, undef all local #defines.
#undef IsNeg
#undef EASPRINTF_MIN
#undef EASPRINTF_MAX


#ifdef _MSC_VER
	#pragma warning(pop)
#endif







