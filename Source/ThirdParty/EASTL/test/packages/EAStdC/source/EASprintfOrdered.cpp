///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/SprintfCore.h>
#include <EAStdC/EASprintfOrdered.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EABitTricks.h>
#include <EAAssert/eaassert.h>
#include <stdarg.h>
#include <string.h>



// GCC has va_copy, but VC++ does not. However, VC++ implements
// va_list in such a way that it can simply be memcpyd.
#ifndef va_copy
	#ifdef __va_copy
		#define va_copy(dest, src) __va_copy((dest), (src))
	#else
		#define va_copy(dest, src) memcpy(&(dest), &(src), sizeof(va_list))
	#endif
#endif


namespace EA
{
namespace StdC
{
namespace SprintfLocal
{

static const int kSpanFormatCapacity = 16;

template<typename CharT>
struct Span
{
	const CharT*   mpBegin;                         // The first char in the span.
	const CharT*   mpEnd;                           // One-past the last used char.
	Modifier       mType;                           // This tells us what the type of the argument is (e.g. kModifierInt).
	AllTypes       mValue;                          // This stores the value, which is of type mType.
	CharT          mFormat[kSpanFormatCapacity];    // The format to use (e.g. %5.3f). If empty then this is a string span.
	CharT          mFormatChar;                     // The last char in the mFormat string.
	int            mUserIndex;                      // The index the user assigned to this format. Negative value if this is a string span.
	bool           mbEscapePresent;                 // True if the span is a string and it has a %% sequence. We can optimize writes if we know that it doesn't.

	Span()  : mpBegin(nullptr), 
			  mpEnd(nullptr),
			  mType(kModifierNone),
			  mValue(),
			  mFormatChar(0),
			  mUserIndex(0),
			  mbEscapePresent(false)
			{ mFormat[0] = 0; }
};

using Span8  = Span<char>;
using Span16 = Span<char16_t>;
using Span32 = Span<char32_t>;



// This function exists for the sole purpose of passing an arbitrary argument to VprintfCore
// along with an existing WriteFunction8/pWriteFunctionContext8. 
static int CallVprintfCore(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pWriteFunctionContext8, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return VprintfCore(pWriteFunction8, pWriteFunctionContext8, pFormat, arguments);
}

static int CallVprintfCore(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pWriteFunctionContext16, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return VprintfCore(pWriteFunction16, pWriteFunctionContext16, pFormat, arguments);
}

static int CallVprintfCore(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pWriteFunctionContext32, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return VprintfCore(pWriteFunction32, pWriteFunctionContext32, pFormat, arguments);
}

namespace Internal 
{
	// The way this function works is as follows: We walk through the pFormat string and identify all
	// format spans and non-format spans (i.e. literal text) between format spans. Thus for the string
	// "   %1:f  %0:d   " we have five spans: three non-format spans, and two format spans. The we 
	// read the va_list arguments in the user-specified order into a union that can hold any type.
	// It's important that we read the arguments in user-specified order and not format string order.
	// So the %d would be read first as an int, and the %f would be read second as a double.
	// Finally we walk through the format in string order and write out each span to the output.
	// Non-format segments are simply copied to the output. Format segments are written to the output
	// by a call to VPrintfCore each. 
	//
	template<typename Span, typename WriteFunction, typename CharT>
	static int OVprintfCore(WriteFunction pWriteFunction, void* EA_RESTRICT pWriteFunctionContext, const CharT* EA_RESTRICT pFormat, va_list arguments)
	{
		const int       kArgCapacity = 10;           // Currently only single digit ('0'-'9') order values are supported.
		const int       kSpanCapacity = 21;          // Worst case scenario of 21 spans. For example: " %2:d %7:d %1:d %6:d %3:d %5:d %4:d %0:d %8:d %9:d " or "%0:d%1:d%2:d%3:d%4:d%5:d%6:d%7:d%8:d%9:d"
		Span            spans[kSpanCapacity];
		int             spanArgOrder[kArgCapacity] = { -1 }; // Each entry is the index into 'spans' of that argument. This allows us to quickly find formats in the order the user passed them to this function. For the example directly above, the contents would be: 5, 1, 9, 13, 11, 7, 3, 17, 15. We initialize it to -1 in order to avoid compiler warnings about it being used before set.
		int             spanIndex          = 0;
		int             formattedSpanCount = 0;
		bool            bInFormat          = false;  // State variable indicating if we are within a % format sequence.
		int             nFormatLength      = 0;
		int             nWriteCountSum     = 0;
		int             startIndex         = 1;      // This is 1 or 0, and it defaults to 1 (but may change below) in order to mean that user formats start at 1, as in "%1:d". However, we have a feature whereby we detect that the user is using %0 as the start index.
		const CharT*  p;
		const CharT*  pEnd;
		int           result;

		static_assert((EAArrayCount(spans) == kSpanCapacity) && (EAArrayCount(spanArgOrder) == kArgCapacity), "spans and spanArgOrder are not using constants for their array size.");

		#ifdef EA_DEBUG
			for(int s = 0; s < kArgCapacity; ++s)
				spanArgOrder[s] = -1;
		#else
			memset(spanArgOrder, 0, sizeof(spanArgOrder));
		#endif

		pWriteFunction(NULL, 0, pWriteFunctionContext, kWFSBegin);

		// Initialize the first span. We always have a beginning sequence that is 
		// a string, even if it is empty. Actually, there may be an empty string
		// span between any two format spans. We'll ignore them later.
		spans[0].mpBegin    = pFormat;
		spans[0].mUserIndex = -1;

		// Build the list of spans.
		// Read each format string character while maintaining a little state machine.
		for(p = pFormat; *p; ++p)
		{
			if(*p == '%')
			{
				// A % char within a format is invalid (though %% is valid), and any '%' char that 
				// begins a format must be followed by at least three more chars, two for the user 
				// index plus colon and at least one for the actual format (e.g. %4:d).
				EA_ASSERT(!bInFormat && p[1] && p[2] && p[3]);

				if(p[1] == '%')
				{
					spans[spanIndex].mbEscapePresent = true;
					p++;                                        // Skip past the second % char.
				}
				else // else we don't have a %% sequence and thus have the start of a format...
				{
					// Finalize the current span (the one before the % char), before starting a new span for this % sequence.
					spans[spanIndex].mpEnd = p;
					spans[spanIndex].mFormat[nFormatLength] = 0;
					spans[spanIndex].mFormatChar = 0;            // This is redundant.
					if(++spanIndex == kSpanCapacity)
						break;

					// Intialize the next span.
					if((p[1] < '0') || (p[1] > '9'))
						return -1; // Invalid format. User specified a format like "%X:d" or just "%"

					const int userIndex = (int)(p[1] - '0');

					if((userIndex == 0) && (startIndex != 0))  // If it appears that the user is using argument numbering that is 0-based (e.g. "%0:d") instead of 1-based...
					{
						startIndex = 0;
						for(int i = kArgCapacity - 1; i > 0; --i)
							spanArgOrder[i] = spanArgOrder[i - 1]; // Convert any existing indexes from what we originally assumed to be 1-based values 'up' to being 0-based values.
					}

					bInFormat = true;
					nFormatLength = 1;                                  // For the % char we write into mFormat.
					spans[spanIndex].mpBegin = p;
					spans[spanIndex].mFormat[0] = '%';
					spans[spanIndex].mUserIndex = userIndex;            // We don't write the user index or the colon into the format string, which is a standard C format specifier sequence.
					spanArgOrder[userIndex - startIndex] = spanIndex;   // startIndex is normally 1, because usually users specify argument indexes in a 1:based way.
					formattedSpanCount++;
					EA_ASSERT(p[2] == ':');                           // We expect formats to have a N: sequence after the % in order to indicate order (e.g. %4:3.1f means %3.1f as 4th argument)
					if(p[2] != ':')
						return -1; // Invalid format. User specified a format like "%00:d" or just "%0"
					p += 2;                                             // Skip past the user index and the colon. 
				}
			}
			else if(bInFormat)
			{
				EA_ASSERT(nFormatLength < kSpanFormatCapacity);
				if(nFormatLength < kSpanFormatCapacity)
					spans[spanIndex].mFormat[nFormatLength++] = *p;
				else
					return -1; // Invalid format. User specified a format like "%0:000000000000001d" (too long a printf format)

				switch(*p)
				{
					case 'b': case 'd': case 'i': case 'u': case 'o': case 'x': case 'X': // If this character is the last posible character of a format specifier...
					case 'g': case 'G': case 'e': case 'E': case 'f': case 'F': case 'a': 
					case 'A': case 'p': case 'c': case 'C': case 's': case 'S': case 'n':
					{
						// Finalize the current span.
						spans[spanIndex].mpEnd = p + 1;
						spans[spanIndex].mFormat[nFormatLength < kSpanFormatCapacity ? nFormatLength : kSpanFormatCapacity - 1] = 0;
						spans[spanIndex].mFormatChar = *p;
						if(++spanIndex == kSpanCapacity)
							break;

						// Intialize the next span.
						bInFormat = false;
						nFormatLength = 0;
						spans[spanIndex].mpBegin = p + 1;
						spans[spanIndex].mUserIndex = -1;  // This is a string span. If a format sequence or end-of-string immediately follows then this will be an empty string span.
						break;
					}

					default:
						// To do: Handle the case of an invalid format character.
						break;
				}
			}
			// Else we are in the middle of a string portion of the format and we need do nothing here.
		}

		// Finalize the last span, which by definition ends at the end of our input.
		EA_ASSERT(spanIndex < kSpanCapacity);
		if((spanIndex == kSpanCapacity) && *p) // If the user somehow specified more format spans than possible...
			return -1; // Invalid format. Too many format spans.

		spans[spanIndex].mpEnd = p;                  // p Should always point to the terminating 0 char of pFormat.
		spans[spanIndex].mFormat[nFormatLength] = 0;
		spanIndex++;

		// Now we read the arguments into span[s].mValue in the order they were passed by the caller.
		for(int i = 0; i < formattedSpanCount; ++i)
		{
			EA_ASSERT((spanArgOrder[i] >= 0) && (spanArgOrder[i] < kSpanCapacity));

			Span&     span = spans[spanArgOrder[i]];
			FormatData formatData;

			// We call ReadFormat in order to get the argument type. We don't need the other information from FormatData.
			// ReadFormat returns the pointer to the next char after the format sequence. If there is an error then it
			// returns a pointer to where it failed. We can use this to tell if ReadFormat failed by testing *pEnd == 0.
			pEnd = ReadFormat(span.mFormat, &formatData, (va_list*)&arguments);

			if(*pEnd != 0) // If ReadFormat bailed before processing the entire format...
				return -1;

			// Unfortunately, ReadFormat tells us the type only if it is a modified type (e.g. %ld as opposed to %d).
			// We could go and modify ReadFormat to store the full type all the time, but that would be messing with 
			// that function and its performance and we'd rather stay away from that. We can solve this with a simple
			// switch statement here.
			if(formatData.mModifier == kModifierNone)
			{
				switch (pEnd[-1])
				{
					case 'b': case 'd': case 'i': case 'u':
					case 'o': case 'x': case 'X':
						formatData.mModifier = kModifierInt;
						break;

					case 'g': case 'G': case 'e': case 'E':
					case 'f': case 'F': case 'a': case 'A':
						formatData.mModifier = kModifierDouble;
						break;

					case 'p': case 's': case 'S': case 'n':
						EA_COMPILETIME_ASSERT(sizeof(size_t) == sizeof(void*)); // If this fails then we need to modify this statement.
						formatData.mModifier = kModifierSize_t;
						break;

					case 'c':
						formatData.mModifier = kModifierChar;
						break;

					case 'C':
						formatData.mModifier = kModifierWChar;
						break;

					default:
						EA_FAIL_M("EAStdC OVprintfCore"); // This shouldn't occur unless ReadFormat started supporting some new format that we aren't yet aware of and it is being used here.
						break;
				}
			}
		
			span.mType = formatData.mModifier;

			switch (span.mType)
			{
				case kModifierChar:
					span.mValue.mChar = (char)va_arg(arguments, int);  // Recall that C++ promotes types less than int to int.
					break;
				case kModifierShort:
					span.mValue.mShort = (short)va_arg(arguments, int);
					break;
				case kModifierInt:
					span.mValue.mInt = va_arg(arguments, int);
					break;
				case kModifierLong:
					span.mValue.mLong = va_arg(arguments, long);
					break;
				case kModifierLongLong:
					span.mValue.mLongLong = va_arg(arguments, long long);
					break;
				case kModifierMax_t:
					span.mValue.mMax = va_arg(arguments, intmax_t);
					break;
				case kModifierSize_t:
					span.mValue.mSize = va_arg(arguments, size_t);
					break;
				case kModifierPtrdiff_t:
					span.mValue.mPtrDiff = va_arg(arguments, ptrdiff_t);
					break;
				case kModifierDouble:
					span.mValue.mDouble = va_arg(arguments, double);
					break;
				case kModifierLongDouble:
					span.mValue.mLongDouble = va_arg(arguments, long double);
					break;
				case kModifierWChar:
					span.mValue.mWChar = (wchar_t)va_arg(arguments, unsigned int);
					break;
				case kModifierInt8:
					span.mValue.mInt8 = (int8_t)va_arg(arguments, int);
					break;
				case kModifierInt16:
					span.mValue.mInt16 = (int16_t)va_arg(arguments, int);
					break;
				case kModifierInt32:
					span.mValue.mInt32 = va_arg(arguments, int32_t);
					break;
				case kModifierInt64:
					span.mValue.mInt64 = va_arg(arguments, int64_t);
					break;
				case kModifierInt128:
					span.mValue.mLongLong = 0;
					EA_FAIL_M("EAStdC OVprintfCore");  // We don't currently support 128 bit types in this function. We do have an int128_t class in this package which could be used, though.
					break;
				case kModifierNone:
				default:
					// If this occurs, then our ReadFormat function seems to have a bug. We already have an assertion failure for this case above.
					span.mValue.mLongLong = 0;
					break;
			}
		}

		// Now we have an array of spans. Now we print the spans one by one.
		for(int s = 0; s < spanIndex; ++s)
		{
			const Span& span = spans[s];

			if(span.mpEnd != span.mpBegin) // If non-empty...
			{
				if(span.mUserIndex >= 0) // If this is a format span as opposed to a string span...
				{
					switch (span.mType)
					{
						case kModifierChar:
							// We can't call VprintfCore directly because it expects a va_list, whereas we have just a single value.
							// We can't call Sprintf because we need to pass the current pWriteFunction, pWriteFunctionContext.
							// To do: We have a problem here: VprintfCore will call the write function  
							//        with kWFSBegin, kWFSIntermediate, and kWFSEnd. We need it to use 
							//        just kFSIntermediate. Currently this affects only writing on 
							//        some mobile platforms to their custom log formats and it's unlikely
							//        anybody will ever use ordered sprintf to such a destination.
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mChar);
							break;
						case kModifierShort:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mShort);
							break;
						case kModifierInt:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mInt);
							break;
						case kModifierLong:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mLong);
							break;
						case kModifierLongLong:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mLongLong);
							break;
						case kModifierMax_t:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mMax);
							break;
						case kModifierSize_t:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mSize);
							break;
						case kModifierPtrdiff_t:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mPtrDiff);
							break;
						case kModifierDouble:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mDouble);
							break;
						case kModifierLongDouble:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mLongDouble);
							break;
						case kModifierWChar:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mWChar);
							break;
						case kModifierInt8:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mInt8);
							break;
						case kModifierInt16:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mInt16);
							break;
						case kModifierInt32:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mInt32);
							break;
						case kModifierInt64:
							result = CallVprintfCore(pWriteFunction, pWriteFunctionContext, span.mFormat, span.mValue.mInt64);
							break;
						case kModifierInt128:
							result = -1; // We don't currently support 128 bit types in this function. We do have an int128_t class in this package which could be used, though.
							break;
						case kModifierNone:
						default:
							result = -1; // If this fails, then our ReadFormat function seems to have a bug.
							break;
					}

					if(result < 0)
						return -1;

					nWriteCountSum += result;
				}
				else
				{
					// We simply copy the span to the pWriteFunction, while taking care of %% escape sequences.
					p    = span.mpBegin;
					pEnd = span.mpEnd;

					if(span.mbEscapePresent)  // If somewhere in the string span there is at least one %% sequence, we must copy the slow way: one by one.
					{
						for(result = 1; p < pEnd; ++p)
						{
							if(pWriteFunction(p, 1, pWriteFunctionContext, kWFSIntermediate) < 0)
								return -1;
							nWriteCountSum += result;
						}
					}
					else
					{
						if(pWriteFunction(p, (size_t)(pEnd - p), pWriteFunctionContext, kWFSIntermediate) < 0)
							return -1;
						nWriteCountSum += (int)(pEnd - p);
					}
				}
			}
		}

		pWriteFunction(NULL, 0, pWriteFunctionContext, kWFSEnd);

		return nWriteCountSum;
	}

} // namespace Internal

static int OVprintfCore(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pWriteFunctionContext8, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return Internal::OVprintfCore<Span8>(pWriteFunction8, pWriteFunctionContext8, pFormat, arguments);
}

static int OVprintfCore(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pWriteFunctionContext16, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return Internal::OVprintfCore<Span16>(pWriteFunction16, pWriteFunctionContext16, pFormat, arguments);
}

static int OVprintfCore(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pWriteFunctionContext32, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return Internal::OVprintfCore<Span32>(pWriteFunction32, pWriteFunctionContext32, pFormat, arguments);
}

} // SprintfLocal



///////////////////////////////////////////////////////////////////////////////
// char
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int OVcprintf(WriteFunction8 pWriteFunction8, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(pWriteFunction8, pContext, pFormat, arguments);
}

EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter8, pFile, pFormat, arguments);
}

EASTDC_API int OVprintf(const char* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter8, stdout, pFormat, arguments);
}

EASTDC_API int OVsprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, va_list arguments)
{
	return OVsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int OVsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext8 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::OVprintfCore(SprintfLocal::StringWriter8, &sc, pFormat, arguments);

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
			return n;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		return -1;
	#endif
}

EASTDC_API int OVscprintf(const char* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return OVsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int OCprintf(WriteFunction8 pWriteFunction, void* EA_RESTRICT pContext, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);
	return SprintfLocal::OVprintfCore(pWriteFunction, pContext, pFormat, arguments);
}

EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter8, pFile, pFormat, arguments);
}

EASTDC_API int OPrintf(const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter8, stdout, pFormat, arguments);
}

EASTDC_API int OSprintf(char* EA_RESTRICT pDestination, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);
}

EASTDC_API int OSnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, n, pFormat, arguments);
}




///////////////////////////////////////////////////////////////////////////////
// char16_t
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int OVcprintf(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(pWriteFunction16, pContext, pFormat, arguments);
}

EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter16, pFile, pFormat, arguments);
}

EASTDC_API int OVprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter16, stdout, pFormat, arguments);
}

EASTDC_API int OVsprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	return OVsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int OVsnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext16 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::OVprintfCore(SprintfLocal::StringWriter16, &sc, pFormat, arguments);

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
			return n;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		return -1;
	#endif
}

EASTDC_API int OVscprintf(const char16_t* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return OVsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int OCprintf(WriteFunction16 pWriteFunction, void* EA_RESTRICT pContext, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(pWriteFunction, pContext, pFormat, arguments);
}

EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter16, pFile, pFormat, arguments);
}

EASTDC_API int OPrintf(const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter16, stdout, pFormat, arguments);
}

EASTDC_API int OSprintf(char16_t* EA_RESTRICT pDestination, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);
}

EASTDC_API int OSnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, n, pFormat, arguments);
}



///////////////////////////////////////////////////////////////////////////////
// char32_t
///////////////////////////////////////////////////////////////////////////////

EASTDC_API int OVcprintf(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(pWriteFunction32, pContext, pFormat, arguments);
}

EASTDC_API int OVfprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter32, pFile, pFormat, arguments);
}

EASTDC_API int OVprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter32, stdout, pFormat, arguments);
}

EASTDC_API int OVsprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	return OVsnprintf(pDestination, (size_t)-1, pFormat, arguments);
}

EASTDC_API int OVsnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	SprintfLocal::SnprintfContext32 sc(pDestination, 0, pDestination ? n : 0);

	const int nRequiredLength = SprintfLocal::OVprintfCore(SprintfLocal::StringWriter32, &sc, pFormat, arguments);

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
			return n;
		}
		else if((n > 0) && pDestination)
			pDestination[n - 1] = 0;
		return -1;
	#endif
}

EASTDC_API int OVscprintf(const char32_t* EA_RESTRICT pFormat, va_list arguments)
{
	// vscprintf returns the number of chars that are needed for a printf operation.
	return OVsnprintf(NULL, 0, pFormat, arguments);
}

EASTDC_API int OCprintf(WriteFunction32 pWriteFunction, void* EA_RESTRICT pContext, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(pWriteFunction, pContext, pFormat, arguments);
}

EASTDC_API int OFprintf(FILE* EA_RESTRICT pFile, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter32, pFile, pFormat, arguments);
}

EASTDC_API int OPrintf(const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return SprintfLocal::OVprintfCore(SprintfLocal::FILEWriter32, stdout, pFormat, arguments);
}

EASTDC_API int OSprintf(char32_t* EA_RESTRICT pDestination, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, (size_t)SprintfLocal::kNoPrecision, pFormat, arguments);
}

EASTDC_API int OSnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);

	return OVsnprintf(pDestination, n, pFormat, arguments);
}

} // namespace StdC
} // namespace EA
