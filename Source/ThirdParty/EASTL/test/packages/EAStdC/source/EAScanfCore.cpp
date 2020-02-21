///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/internal/ScanfCore.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EAMathHelp.h>
#include <EAAssert/eaassert.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <math.h>
#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
EA_RESTORE_ALL_VC_WARNINGS()
EA_DISABLE_VC_WARNING(4127 4146) // C4127: conditional expression is constant.
								 // C4146: unary minus operator applied to unsigned type, result still unsigned


#if defined(EA_PLATFORM_WINDOWS)
	// Users sometimes get link problems when mixing DLL and non-DLL builds under Windows
	// due to HUGE_VAL. This is due to how they are building the app, though we can help
	// avoid the problem by using a different source for HUGE_VAL.
	#define EASTDC_HUGE_VAL EA::StdC::kFloat64Infinity
#else
	#define EASTDC_HUGE_VAL HUGE_VAL
#endif


// EA_ENABLE_PRECISE_FP / EA_RESTORE_PRECISE_FP
//
// Allows you to force the usage of precise floating point support by the compiler, whereas the 
// compiler may have been set to default to another form which is faster but doesn't strictly
// follow conventions. For example, see:
//     http://connect.microsoft.com/VisualStudio/feedback/details/754839/signed-double-0-0-vs-0-0-msvs2012-visual-c-bug-in-x64-mode-when-compiled-with-fp-fast-o2
// Reference:
//     http://msdn.microsoft.com/en-us/library/45ec64h6.aspx
//
// Example usage:
//      EA_ENABLE_PRECISE_FP()
//      void SomeFunction(){ ... }
//      EA_RESTORE_PRECISE_FP()
//
#if !defined(EA_ENABLE_PRECISE_FP)
	#if defined(EA_COMPILER_MSVC)
		#define EA_ENABLE_PRECISE_FP() __pragma(float_control(precise, on, push))
	#else
		#define EA_ENABLE_PRECISE_FP()
	#endif
#endif
#if !defined(EA_RESTORE_PRECISE_FP)
	#if defined(EA_COMPILER_MSVC)
		#define EA_RESTORE_PRECISE_FP() __pragma(float_control(pop))
	#else
		#define EA_RESTORE_PRECISE_FP()
	#endif
#endif



namespace EA
{
namespace StdC
{

extern uint8_t utf8lengthTable[256];

namespace ScanfLocal
{


int FILEReader8(ReadAction readAction, int value, void* pContext)
{
	// We verify that the FILE functions work as desired. If this fails to 
	// be so for some compiler/platform then we can modify this code.
	EA_COMPILETIME_ASSERT(EOF == kReadError);

	FILE* const pFile = (FILE*)pContext;

	switch(readAction)
	{
		case kReadActionBegin:
		{
			#if (!defined(__GNUC__) || (__GNUC__ >= 4)) // Older versions of GCC don't support fwide().
				// Question: Is this doing the right thing? Or do we need to be doing something else?
				if(value == 1) // "The value param will be 1 for UTF8 and 2 for UCS2."
					return (fwide(pFile, -1) < 0) ? 1 : 0; // Set the file to be interpreted as UTF8.
				else
				{
					// Problem: wide is 2 bytes for some platforms and 4 for others. The only way for us
					// to fix this problem properly is to handle 32->16 or 16->32 conversions on our 
					// side. But we don't have state information in this FileReader8 function. We would
					// need to revise pContext to provide some space for us to write state. 
					return (fwide(pFile,  1) > 0) ? 1 : 0; // Set the file to be interpreted as wide. 
				}
			#endif
		}

		case kReadActionEnd:
			// Currently we do nothing, but possibly we should restore the file byte/wide state.
			break;

		case kReadActionRead:
			return fgetc(pFile);

		case kReadActionUnread:
			return ungetc(value, pFile);

		case kReadActionGetAtEnd:
			return feof(pFile);

		case kReadActionGetLastError:
			return ferror(pFile);
	}

	return 0;
}

int FILEReader16(ReadAction readAction, int value, void* pContext)
{
	return FILEReader8(readAction, value, pContext);
}

int FILEReader32(ReadAction readAction, int value, void* pContext)
{
	return FILEReader8(readAction, value, pContext);
}




int StringReader8(ReadAction readAction, int /*value*/, void* pContext)
{
	SscanfContext8* const pSscanfContext8 = (SscanfContext8*)pContext;

	switch(readAction)
	{
		case kReadActionBegin:
		case kReadActionEnd:
		case kReadActionGetLastError:
			break;

		case kReadActionRead:
			if(*pSscanfContext8->mpSource)
				return (uint8_t)*pSscanfContext8->mpSource++;
			else
			{
				pSscanfContext8->mbEndFound = 1;
				return kReadError;
			}

		case kReadActionUnread:
			if(!pSscanfContext8->mbEndFound)                
				pSscanfContext8->mpSource--;    // We don't error-check this; we currently assume the caller is bug-free.
			else
				pSscanfContext8->mbEndFound = 0;
			break;

		case kReadActionGetAtEnd:
			return pSscanfContext8->mbEndFound;
	}

	return 0;
}

int StringReader16(ReadAction readAction, int /*value*/, void* pContext)
{
	SscanfContext16* const pSscanfContext16 = (SscanfContext16*)pContext;

	switch(readAction)
	{
		case kReadActionBegin:
		case kReadActionEnd:
		case kReadActionGetLastError:
			break;

		case kReadActionRead:
			if(*pSscanfContext16->mpSource)
			{
				EA_COMPILETIME_ASSERT(sizeof(int) >= sizeof(char16_t));
				return (int)(char16_t)*pSscanfContext16->mpSource++;
			}
			else
			{
				pSscanfContext16->mbEndFound = 1;
				return kReadError;
			}

		case kReadActionUnread:
			if(!pSscanfContext16->mbEndFound)                
				pSscanfContext16->mpSource--;    // We don't error-check this; we currently assume the caller is bug-free.
			else
				pSscanfContext16->mbEndFound = 0;
			break;

		case kReadActionGetAtEnd:
			return pSscanfContext16->mbEndFound;
	}

	return 0;
}

int StringReader32(ReadAction readAction, int /*value*/, void* pContext)
{
	SscanfContext32* const pSscanfContext32 = (SscanfContext32*)pContext;

	switch(readAction)
	{
		case kReadActionBegin:
		case kReadActionEnd:
		case kReadActionGetLastError:
			break;

		case kReadActionRead:
			if(*pSscanfContext32->mpSource)
			{
				EA_COMPILETIME_ASSERT(sizeof(int) >= sizeof(char32_t));
				return (int)(char32_t)*pSscanfContext32->mpSource++;
			}
			else
			{
				pSscanfContext32->mbEndFound = 1;
				return kReadError;
			}

		case kReadActionUnread:
			if(!pSscanfContext32->mbEndFound)                
				pSscanfContext32->mpSource--;    // We don't error-check this; we currently assume the caller is bug-free.
			else
				pSscanfContext32->mbEndFound = 0;
			break;

		case kReadActionGetAtEnd:
			return pSscanfContext32->mbEndFound;
	}

	return 0;
}



///////////////////////////////////////////////////////////////////////////////
// ToDouble
//
// We have a string of digits and an exponent. We need to convert them to 
// a double. 
//
// The proper conversion from string to double is not entirely trivial, 
// as documented in the papers:
//    What Every Computer Scientist Should Know About Floating Point Arithmetic
//    How to Read Floating Point Numbers Accurately
//    Correctly Rounded Binary-Decimal and Decimal-Binary Conversions
//
const double powerTable[18] = { 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11 };

double DoubleValue::ToDouble() const
{
	if(mExponent >= -6 && mExponent <= 11) // If we can handle the result quickly and accurately with a simple implementation...
	{
		// We could handle exponents bigger than this if we used a bigger table or (better) if we 
		// wrote code that allowed us to apply a multiplier to the existing table. On the other hand,
		// for our purposes it is uncommon to work with such huge numbers.
		double result = 0.0;

		for(int i = 0; i < mSigLen; ++i)
			result = (result * 10.0) + (float)(mSigStr[i] - '0');

		result *= powerTable[mExponent + 6];  // +6 because our smallest exponent above is 1e-6

		return result;
	}
	else
	{
		// Negative exponents mean the number has a fractional component. 
		// However, floating point hardware can't perfectly represent decimal
		// fractions. A result of this, as noted in the above papers, is that
		// a simple loop which multiplies by 10 won't yield an ideal floating 
		// point representation for all decimal values. In the absence of 
		// FPU hardware support for decimal to binary conversions, the only 
		// way to achieve the ideal solution is to use an iterative approximating
		// algorithm.
		//
		// Until we implement that algorithm we do a fallback to the system
		// provided strtod function, which in many cases will implement such 
		// an algorithm itself internally.

		char buffer[kMaxSignificandDigits + 12];
		int     i;

		for(i = 0; i < mSigLen; ++i)
			buffer[i] = mSigStr[i];

		if(mExponent)
		{
			int multiplier;
			int e = mExponent;

			buffer[i++] = 'e';

			if(e < 0)
			{
				buffer[i++] = '-';
				e = -e;
			}

			if(e >= 100)
				multiplier = 100;
			else if(e >= 10)
				multiplier = 10;
			else
				multiplier = 1;

			while(multiplier)
			{
				buffer[i++] = (char)('0' + (e / multiplier));
				e          %= multiplier;
				multiplier /= 10;
			}
		}

		buffer[i] = 0;

		return strtod(buffer, NULL);  // This is not a fast function. That's why we want to replace it.
	}
}


// Force precise floating support by the compiler. This is necessary under VC++ because otherwise
// it's possible that the /fp:fast compiler argument was used, which causes the -0.0 code below to
// be generated by the compiler in a non-conformant way. This generation is by design, but prevents
// us from having a conforming scanf implementation.
EA_ENABLE_PRECISE_FP()

template<typename ReadFunctionT, typename ReadFormatSpanFunctionT, typename CharT>
class VscanfUtil
{
public:
	int VscanfCore(ReadFunctionT pReadFunction, ReadFormatSpanFunctionT pReadFormatSpanFunction, void* pContext, const CharT* pFormat, va_list arguments)   
	{
		using namespace ScanfLocal;

		int                 nAssignmentCount = 0;       // Number of assigned fields. This is the return value of this function. -1 in case of error.
		int                 nConversionCount = 0;       // Number of processed fields. Will be >= the number of assigned fields.
		int                 nReadCount;                 // Temporary holder.
		int                 nReadCountSum = 0;          // Used to support the %n field.
		const CharT*        pFormatCurrent = pFormat;   // Current position within entire fd string.
		char*               pArgumentCurrent = NULL;    // Pointer to current va_list argument. 
		FormatData          fd;                         // 
		int                 c = 0;                      // Temporary character.
		uintmax_t           uintMaxValue = 0;           // 
		intmax_t            intMaxValue = 0;            // 
		long double         ldValue;                    // 
		int                 bNegative;                  // 
		int                 bIntegerOverflow;           // 
		int                 nBase;                      // 

		pReadFunction(kReadActionBegin, sizeof(*pFormat), pContext);

		while(*pFormatCurrent)
		{
			CharT cFormat = *pFormatCurrent;

			if(Isspace(cFormat))
			{
				do{
					cFormat = *++pFormatCurrent;
				}
				while(Isspace(cFormat));

				while(Isspace((CharT)(c = pReadFunction(kReadActionRead, 0, pContext))))
					++nReadCountSum;

				pReadFunction(kReadActionUnread, c, pContext);
				continue;
			}

			if(cFormat != '%')
			{
				if((c = pReadFunction(kReadActionRead, 0, pContext)) != (int)cFormat)
				{
					pReadFunction(kReadActionUnread, c, pContext);
					goto Done;
				}

				++nReadCountSum;
				++pFormatCurrent;
				continue;
			}

			pFormatCurrent = ReadFormat(pFormatCurrent, &fd);

			if((fd.mnType == '%') || fd.mbSkipAssignment)
				pArgumentCurrent = NULL;
			else
				pArgumentCurrent = (char*)va_arg(arguments, void*); // User arguments are always passed as pointers.

			if((fd.mnType != 'n') && (pReadFunction(kReadActionGetLastError, 0, pContext) || pReadFunction(kReadActionGetAtEnd, 0, pContext)))
				break;

			switch (fd.mnType)
			{
				case '%':
				{
					while(Isspace((CharT)(c = pReadFunction(kReadActionRead, 0, pContext))))
						++nReadCountSum;

					if(c != '%')
					{
						pReadFunction(kReadActionUnread, c, pContext);
						goto Done;
					}

					++nReadCountSum;
					break;
				}

				case 'n':
				{
					if(pArgumentCurrent) // If we should write the value to a user argument...
					{
						switch (fd.mModifier)
						{
							// There are some other modifier types we could support here.
							case kModifierMax_t:    *(intmax_t*) pArgumentCurrent = (intmax_t) nReadCountSum; break;
							case kModifierSize_t:   *(size_t*)   pArgumentCurrent = (size_t)   nReadCountSum; break;
							case kModifierPtrdiff_t:*(ptrdiff_t*)pArgumentCurrent = (ptrdiff_t)nReadCountSum; break;
							case kModifierInt64:    *(int64_t*)  pArgumentCurrent = (int64_t)  nReadCountSum; break;
							case kModifierLongLong: *(long long*)pArgumentCurrent = (long long)nReadCountSum; break;
							case kModifierInt32:    *(int32_t*)  pArgumentCurrent = (int32_t)  nReadCountSum; break;
							case kModifierLong:     *(long*)     pArgumentCurrent = (long)     nReadCountSum; break;
							case kModifierInt16: 
							case kModifierShort:    *(short*)    pArgumentCurrent = (short)    nReadCountSum; break;
							case kModifierInt8:  
							case kModifierChar:     *(char*)     pArgumentCurrent = (char)     nReadCountSum; break;
							case kModifierNone:     *(int*)      pArgumentCurrent = (int)      nReadCountSum; break;
							default:                                                                          break;
						}
					}

					continue;
				}

				case 'b': // 'b' means binary. This is a convenient extension that we provide.
				case 'o':
				case 'u':
				case 'i':
				case 'd':
				case 'x':
				case 'X':
				{
					// To consider: replace if/else usage below with a switch or something with less branching.
					if(fd.mnType == 'b')
						nBase = 2;
					else if(fd.mnType == 'o')
						nBase = 8;
					else if(fd.mnType == 'u' || fd.mnType == 'd')
						nBase = 10;
					else if(fd.mnType == 'i')
						nBase = 0;
					else
						nBase = 16;

					switch (fd.mModifier)
					{
						case kModifierMax_t:
							uintMaxValue = (uintmax_t)         ReadUint64(pReadFunction, pContext, UINTMAX_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierSize_t:
							uintMaxValue = (size_t)            ReadUint64(pReadFunction, pContext, SIZE_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierPtrdiff_t:
							uintMaxValue = (ptrdiff_t)         ReadUint64(pReadFunction, pContext, PTRDIFF_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierInt64:
						case kModifierLongLong:
							uintMaxValue = (unsigned long long)ReadUint64(pReadFunction, pContext, UINT64_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierInt32:
						case kModifierLong:
							uintMaxValue  = (unsigned long)    ReadUint64(pReadFunction, pContext, UINT32_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierInt16:
						case kModifierShort:
							uintMaxValue  = (unsigned long)    ReadUint64(pReadFunction, pContext, UINT16_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;

						case kModifierInt8:
						case kModifierChar:
						default:
							uintMaxValue  = (unsigned long)    ReadUint64(pReadFunction, pContext, UINT8_MAX, nBase, fd.mnWidth, nReadCount, bNegative, bIntegerOverflow);
							break;
					}

					if(!nReadCount)
						goto Done;

					if((fd.mnType == 'i' || fd.mnType == 'd')) // If using a signed type...
					{
						if(bNegative)
							intMaxValue = -uintMaxValue;
						else
							intMaxValue = (intmax_t)uintMaxValue;

						if(pArgumentCurrent) // If we should write the value to a user argument...
						{
							switch (fd.mModifier)
							{
								case kModifierMax_t:    *(intmax_t*)  pArgumentCurrent = (intmax_t)  intMaxValue; break;
								case kModifierSize_t:   *(size_t*)    pArgumentCurrent = (size_t)    intMaxValue; break;
								case kModifierPtrdiff_t:*(ptrdiff_t*) pArgumentCurrent = (ptrdiff_t) intMaxValue; break;
								case kModifierInt64:    *(int64_t*)   pArgumentCurrent = (int64_t)   intMaxValue; break;
								case kModifierLongLong: *(long long*) pArgumentCurrent =             intMaxValue; break;
								case kModifierInt32:    *(int32_t*)   pArgumentCurrent = (int32_t)   intMaxValue; break; // We assume sizeof long >= sizeof int32
								case kModifierLong:     *(long*)      pArgumentCurrent = (long)      intMaxValue; break;
								case kModifierInt16: 
								case kModifierShort:    *(short*)     pArgumentCurrent = (short)     intMaxValue; break;
								case kModifierInt8:  
								case kModifierChar:     *(char*)      pArgumentCurrent = (char)      intMaxValue; break;
								case kModifierNone:     *(int*)       pArgumentCurrent = (int)       intMaxValue; break;
								default: /* This should never occur. Possibly assert false */                     break;
							}

							nAssignmentCount++;
						}
					}
					else
					{
						if(bNegative)
						{
							// It's a little odd to use a negative sign in front of an unsigned value, but it's valid C.
							uintMaxValue = (uintmax_t)-(intmax_t)uintMaxValue;
						}
						// else leave as-is.

						if(pArgumentCurrent) // If we should write the value to a user argument...
						{
							switch (fd.mModifier)
							{
								case kModifierMax_t:    *(uintmax_t*)          pArgumentCurrent = (uintmax_t)          uintMaxValue; break;
								case kModifierSize_t:   *(size_t*)             pArgumentCurrent = (size_t)             uintMaxValue; break;
								case kModifierPtrdiff_t:*(ptrdiff_t*)          pArgumentCurrent = (ptrdiff_t)          uintMaxValue; break;
								case kModifierInt64:    *(uint64_t*)           pArgumentCurrent = (uint64_t)           uintMaxValue; break;
								case kModifierLongLong: *(unsigned long long*) pArgumentCurrent =                      uintMaxValue; break;
								case kModifierInt32:    *(uint32_t*)           pArgumentCurrent = (uint32_t)           uintMaxValue; break; // We assume sizeof ulong >= sizeof uint32
								case kModifierLong:     *(unsigned long*)      pArgumentCurrent = (unsigned long)      uintMaxValue; break;
								case kModifierInt16: 
								case kModifierShort:    *(unsigned short*)     pArgumentCurrent = (unsigned short)     uintMaxValue; break;
								case kModifierInt8:  
								case kModifierChar:     *(unsigned char*)      pArgumentCurrent = (unsigned char)      uintMaxValue; break;
								case kModifierNone:     *(unsigned int*)       pArgumentCurrent = (unsigned int)       uintMaxValue; break;
								default: /* This should never occur. Possibly assert false */                                        break;
							}

							nAssignmentCount++;
						}
					}

					nReadCountSum += nReadCount;
					nConversionCount++;
					break;
				}

				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
				{
					ldValue = ReadDouble(pReadFunction, pContext, fd.mnWidth, fd.mDecimalPoint, nReadCount, bIntegerOverflow);

					if(!nReadCount)
						goto Done;

					if(pArgumentCurrent) // If we should write the value to a user argument...
					{
						switch (fd.mModifier)
						{
							case kModifierLongDouble: *(long double*) pArgumentCurrent =         ldValue; break;
							case kModifierDouble:     *(double*)      pArgumentCurrent = (double)ldValue; break;
							case kModifierNone:       *(float*)       pArgumentCurrent = (float) ldValue; break;
							default: /* This should never occur. Possibly assert false */                 break;
						}

						nAssignmentCount++;
					}

					nReadCountSum += nReadCount;
					nConversionCount++;
					break;
				}

				case 's':
				case 'S':
				{
					c = pReadFunction(kReadActionRead, 0, pContext);

					// We eat leading whitespace and then fall through to reading characters.
					while(Isspace((CharT)c))
					{
						++nReadCountSum;
						c = pReadFunction(kReadActionRead, 0, pContext);
					}

					pReadFunction(kReadActionUnread, c, pContext);
					// Fall through, as %[] processing is the same as %s except %[] specifies a filter for what characters to accept or ignore.
				}

				case '[':
				{
					// The user can use %[ab ] to read chars 'a', 'b', ' ' into the string until some other char is encountered. 
					nReadCount = 0;

					if(pArgumentCurrent) // If we should write the value to a user argument...
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
								//nAssignmentCount = -1; Should we do this?
								goto Done;
						}

						if (!pReadFormatSpanFunction(fd, c, pReadFunction, pContext, stringTypeSize, pArgumentCurrent, nReadCount))
						{
							nAssignmentCount = -1;
							goto Done;
						}

						if(!nReadCount)
						{
							pReadFunction(kReadActionUnread, c, pContext);
							goto Done;
						}

						// 0-terminate the user's string.
						switch (stringTypeSize)
						{
							case 1:
								*((char*)pArgumentCurrent)  = 0;
								break;

							case 2:
								*((char16_t*)pArgumentCurrent) = 0;
								break;

							case 4:
								*((char32_t*)pArgumentCurrent) = 0;
								break;
						}

						nAssignmentCount++;
					}
					else
					{
						pReadFormatSpanFunction(fd, c, pReadFunction, pContext, -1, pArgumentCurrent, nReadCount);

						if(!nReadCount)
						{
							pReadFunction(kReadActionUnread, c, pContext);
							break;
						}
					}

					if(fd.mnWidth >= 0)
						pReadFunction(kReadActionUnread, c, pContext);

					nReadCountSum += nReadCount;
					nConversionCount++;
					break;
				}

				case 'c':
				case 'C': // Actually %C is not a standard scanf format.
				{
					// The user can specify %23c to read 23 chars (including spaces) into an array, with no 0-termination.
					if(!fd.mbWidthSpecified)
						fd.mnWidth = 1;

					nReadCount = 0;

					if(pArgumentCurrent) // If we should write the value to a user argument...
					{
						int charTypeSize;

						switch (fd.mModifier)
						{
							case kModifierInt8:         // If the user specified %I8c or %I8C
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
								//nAssignmentCount = -1; Should we do this?
								goto Done;
						}

						while(fd.mnWidth-- && ((c = pReadFunction(kReadActionRead, 0, pContext)) != -1))
						{
							switch (charTypeSize)
							{
								case 1:
									// Applicable for 16 and 32 bit character string variant
									#if EASTDC_SCANF_WARNINGS_ENABLED
										//if(c > 127)
										//    ReportScanfWarning(loss of information);
									#endif

									*((char*)pArgumentCurrent) = (char)(uint8_t)(unsigned)c;
									break;

								case 2:
									// Applicable for 8bit character string variant
									// To do: Support UTF8 sequences.
									// size_t charLength = UTF8CharSize(&c); Do this only for the first char.
									// if(charLength > 1)
									//    read into buffer and do a single char Strlcpy from 8 bit to 16 bit.

									// Applicable for 32bit character string variant
									#if EASTDC_SCANF_WARNINGS_ENABLED
										//if(c > 65535)
										//    ReportScanfWarning(loss of information);
									#endif
									*((char16_t*)pArgumentCurrent) = (char16_t)c;
									break;

								case 4:
									// Applicable for 8bit character string variant
									// To do: Support UTF8 sequences.

									*((char32_t*)pArgumentCurrent) = (char32_t)c;
									break;
							}                           

							++nReadCount;
						}

						if(!nReadCount)
							goto Done;

						nAssignmentCount++;
					}
					else // else ignore the field
					{
						while(fd.mnWidth-- && ((c = pReadFunction(kReadActionRead, 0, pContext)) != -1))
							++nReadCount;
					
						if(!nReadCount)
							goto Done;
					}

					nReadCountSum += nReadCount;
					nConversionCount++;
					break;
				}

				case kFormatError:
				default:
					goto Done;

			} // switch (fd.mnType)

		} // while(*pFormatCurrent)

		Done:
		if((nConversionCount == 0) && pReadFunction(kReadActionGetLastError, 0, pContext))
			nAssignmentCount = -1;

		pReadFunction(kReadActionEnd, 0, pContext);

		return nAssignmentCount;
	}

private:
	const CharT* ReadFormat(const CharT* pFormat, FormatData* pFormatData)
	{
		const CharT*    pFormatCurrent = pFormat;
		bool            bModifierPresent = true;  // True until proven false.
		FormatData      fd;
		CharT           c;

		c = *++pFormatCurrent;

		if(c == '%') // A %% sequence means to simply treat it as a literal '%'.
		{
			fd.mnType    = '%';
			*pFormatData = fd;

			return ++pFormatCurrent;
		}

		if(Isdigit(c)) // If the user is specifying a field width...
		{
			// The standard doesn't say anything special about a field width of zero, so we allow it.
			fd.mbWidthSpecified = 1;
			fd.mnWidth = 0;

			do{
				fd.mnWidth = (int)((fd.mnWidth * 10) + (c - '0'));
				c = *++pFormatCurrent;
			} while(Isdigit(c));
		}
		else if(c == '*') // A * char after % means to skip the assignment but eat it from the source data.
		{
			fd.mbSkipAssignment = true;
			c = *++pFormatCurrent;
		}

		// See if the user specified a modifier, such as h, hh, l, ll, or L.
		switch (c)
		{
			case 'h': // handle h and hh
			{
				if(pFormatCurrent[1] == 'h') // If the fd is hh ...
				{
					fd.mModifier = kModifierChar;   // Specifies that a following d, i, o, u, x, X, or n conversion specifier applies to an argument with type pointer to signed char or unsigned char.
					c = *++pFormatCurrent;
				}
				else
					fd.mModifier = kModifierShort;  // Specifies that a following d, i, o, u, x, X, or n conversion specifier applies to an argument with type pointer to short int or unsigned short int.

				break;
			}

			case 'l': // handle l and ll
			{
				if(pFormatCurrent[1] == 'l') // If the fd is ll ...
				{
					fd.mModifier = kModifierLongLong;   // Specifies that a following d, i, o, u, x, X, or n conversion specifier applies to an argument with type pointer to long long int or unsigned long long int.
					c = *++pFormatCurrent;
				}
				else
					fd.mModifier = kModifierLong;       // Specifies that a following d, i, o, u, x, X, or n conversion specifier applies to an argument with type pointer to long int or unsigned long int; that a following a, A, e, E, f, F, g, or G conversion specifier applies to an argument with type pointer to double; or that a following c, s, or [ conversion specifier applies to an argument with type pointer to wchar_t.

				break;
			}

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
				 // Specifies that a following a, A, e, E, f, F, g, or G conversion specifier applies to an argument with type pointer to long double.
				fd.mModifier = kModifierLongDouble;
				break;

			case 'I': // We support Microsoft's extension sized fd specifiers.
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
					fd.mnType    = kFormatError;
					*pFormatData = fd;
					EA_FAIL_MSG("Scanf: Invalid %I modifier");

					return ++pFormatCurrent;
				}
				break;

			default:
				bModifierPresent = false;
				break;
		}

		if(bModifierPresent)
			c = *++pFormatCurrent;

		fd.mnType = (int)c;

		switch (c)
		{
			case 'b': // 'b' means binary. This is a convenient extension that we provide.
			case 'd':
			case 'u':
			case 'i':
			case 'x':
			case 'X':
			case 'o':
				if(fd.mModifier == kModifierLongDouble)
				{
					fd.mnType = kFormatError;
					EA_FAIL_MSG("Scanf: Invalid %b/%d/%u/%i/%x/%o modifier");
				}
				break;

			case 'c': // We accept %hc, %c, %lc, %I8c, %I16c, %I32c (regular, regular, wide, char, char16_t, char32_t)
			case 'C': // We accept %hC, %C, %lC, %I8C, %I16C, %I32C (regular, wide,    wide, char, char16_t, char32_t)
			case 's': // We accept %hs, %s, %ls, %I8s, %I16s, %I32s (regular, regular, wide, char, char16_t, char32_t)
			case 'S': // We accept %hS, %S, %lS, %I8s, %I16s, %I32s (regular, wide,    wide, char, char16_t, char32_t)
			{
				// Microsoft's library goes against the C and C++ standard: %s is 
				// not interpreted to mean char string but instead is interpreted 
				// to be either char or wchar_t depending on what the output
				// text fd is. This is non-standard but has the convenience
				// of allowing users to migrate between char and wchar_t usage
				// more easily. So we allow EASCANF_MS_STYLE_S_FORMAT to control this.

				if(fd.mModifier == kModifierLong)
					fd.mModifier = kModifierWChar;
				else if(fd.mModifier == kModifierShort)
					fd.mModifier = kModifierChar;    
				else if(fd.mModifier == kModifierNone)
				{
					#if EASCANF_MS_STYLE_S_FORMAT
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
				else if((fd.mModifier < kModifierInt8) || (fd.mModifier > kModifierInt32)) // This expression assumes that Int8, Int16, Int32 are contiguous enum values.
				{
					fd.mnType = kFormatError;
					EA_FAIL_MSG("Scanf: Invalid %s/%c modifier");
				}

				if((c == 's') || (c == 'S'))
				{
					// We make %s be a special case of %[] whereby all non-space characters are accepted.
					// fd.mCharBitmap.SetAll();
					// fd.mCharBitmap.Clear(0x09); // Set tab (0x09),
					// fd.mCharBitmap.Clear(0x0a); //     LF (0x0a),
					// fd.mCharBitmap.Clear(0x0b); //     VT (0x0b),
					// fd.mCharBitmap.Clear(0x0c); //     FF (0x0c),
					// fd.mCharBitmap.Clear(0x0d); //     CR (0x0d),
					// fd.mCharBitmap.Clear(0x20); //     space (0x20) to zero.

					// Pre-calculated version of above:
					fd.mCharBitmap.mBits[0] = 0xffffc1ff;
					fd.mCharBitmap.mBits[1] = 0xfffffffe;
					fd.mCharBitmap.mBits[2] = 0xffffffff;
					fd.mCharBitmap.mBits[3] = 0xffffffff;
					fd.mCharBitmap.mBits[4] = 0xffffffff;
					fd.mCharBitmap.mBits[5] = 0xffffffff;
					fd.mCharBitmap.mBits[6] = 0xffffffff;
					fd.mCharBitmap.mBits[7] = 0xffffffff;
				}

				break;
			}

			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
				// The C99 Standard, section 7.24.2.2, specifies that %l and %L are the only modifiers that 
				// affect floating point types. %f = float, %lf = double, %Lf = long double. It doesn't say 
				// what the expected result is for using other modifiers with floating point types. We can 
				// choose to ignore these types or yield an error. We give an error. The VC++ Standard 
				// Library has inconsistent behaviour: it ignores the h in %hf, but in the case of %llf it 
				// reinterprets the format to be %lli. 

				if(fd.mModifier == kModifierLong)                // if %lf ...
					fd.mModifier = kModifierDouble;
				else if((fd.mModifier != kModifierLongDouble) && // if not %Lf ...
						(fd.mModifier != kModifierNone))
				{
					fd.mnType = kFormatError;
					EA_FAIL_MSG("Scanf: Invalid %e/%f/%g/%a modifier");
				}

				break;

			case 'p':
				if(sizeof(void*) == 2)
					fd.mModifier = kModifierInt16;
				else if(sizeof(void*) == 4)
					fd.mModifier = kModifierInt32;
				else
					fd.mModifier = kModifierInt64;

				fd.mnType = 'x';

				break;

			case '[':
			{
				// The C99 standard states (7.19.6.2 p12): 
				// Matches a non-empty sequence of bytes from a set of expected bytes (the scanset). The normal skip over 
				// white-space characters shall be suppressed in this case. The application shall ensure that the 
				// corresponding argument is a pointer to the initial byte of an array of char, signed char, or unsigned char
				// large enough to accept the sequence and a terminating null byte, which shall be added automatically.
				//
				// If an l (ell) qualifier is present, the input is a sequence of characters that begins in the initial shift state. 
				// Each character in the sequence shall be converted to a wide character as if by a call to the mbrtowc() function, 
				// with the conversion state described by an mbstate_t object initialized to zero before the first character is converted. 
				// The application shall ensure that the corresponding argument is a pointer to an array of wchar_t large enough to 
				// accept the sequence and the terminating null wide character, which shall be added automatically.
				//
				// The conversion specification includes all subsequent bytes in the fd string up to and including the matching 
				// right square bracket (']'). The bytes between the square brackets (the scanlist) comprise the scanset, 
				// unless the byte after the left square bracket is a circumflex ('^'), in which case the scanset contains all 
				// bytes that do not appear in the scanlist between the circumflex and the right square bracket. If the conversion 
				// specification begins with "[]" or "[^]", the right square bracket is included in the scanlist and the next right 
				// square bracket is the matching right square bracket that ends the conversion specification; otherwise, the first 
				// right square bracket is the one that ends the conversion specification. If a '-' is in the scanlist and is not 
				// the first character, nor the second where the first character is a '^', nor the last character, the behavior is 
				// implementation-defined.

				bool bInclusive = true;

				if(fd.mModifier == kModifierShort)
				   fd.mModifier =  kModifierChar;
				else if(fd.mModifier == kModifierLong)
					fd.mModifier = kModifierWChar;
				else if(fd.mModifier == kModifierNone)
				{
					#if EASCANF_MS_STYLE_S_FORMAT
						fd.mModifier = (sizeof(*pFormat) == sizeof(char)) ? kModifierChar : kModifierWChar;
					#else
						fd.mModifier = (sizeof(*pFormat) == sizeof(char16_t)) ? kModifierWChar : kModifierChar;  //TODO: This condition seems odd, needs review.
					#endif
				}
				else if((fd.mModifier < kModifierInt8) || (fd.mModifier > kModifierInt32)) // This expression assumes that Int8, Int16, Int32 are contiguous enum values.
				{
					fd.mnType = kFormatError;
					EA_FAIL_MSG("Scanf: Invalid %[ modifier");
				}

				c = *++pFormatCurrent;

				if(c == '^')
				{
					bInclusive = false;
					c = *++pFormatCurrent;
				}

				if(c == ']') // The C99 standard requires that if there is an excluded ']' char, then it is the first char after [ or [^.
				{
					fd.mCharBitmap.Set((CharT)']');
					c = *++pFormatCurrent;
				}

				// To do: We need to read UTF8 character sequences here instead of just ascii values.
				EA_ASSERT((sizeof(CharT) != sizeof(char)) || ((uint8_t)(char)c < 128)); // A c >= 128 refers to a UTF8 sequence, which we don't yet support.

				while(c && (c != ']'))  // Walk through the characters until we encounter a closing ']' char. Use '-' char to indicate character ranges, as in "a-d"
				{
					fd.mCharBitmap.Set(c);

					if((pFormatCurrent[1] == '-') && pFormatCurrent[2] && (pFormatCurrent[2] != ']')) // If we have a character range specifier...
					{
						while(++c <= pFormatCurrent[2])
							fd.mCharBitmap.Set(c);

						pFormatCurrent += 2;
					}

					c = *++pFormatCurrent;
				}

				if(c) // At this point, c should be ']'
				{
					if(!bInclusive)
						fd.mCharBitmap.NegateAll();
				}
				else
				{
					fd.mnType = kFormatError;
					EA_FAIL_MSG("Scanf: Missing format ] char");
				}

				break;
			}

			case 'n':
			{
				// The C99 standard states (7.19.6.2 p12): No input is consumed. The application shall ensure 
				// that the corresponding argument is a pointer to the integer into which shall be written the 
				// number of bytes read from the input so far by this call to the fscanf() functions. 
				// Execution of a %n conversion specification shall not increment the assignment nReadCount returned 
				// at the completion of execution of the function. No argument shall be converted, but one shall 
				// be consumed. If the conversion specification includes an assignment-suppressing character or 
				// a field width, the behavior is undefined.
				break;
			}

			default:
			{
				fd.mnType = kFormatError;
				EA_FAIL_MSG("Scanf: Invalid format.");
				break;
			}
		}

		*pFormatData = fd;

		return ++pFormatCurrent;
	}
	uint64_t ReadUint64(ReadFunctionT pReadFunction, void* pContext,
						uint64_t nMaxValue, int nBase, int nMaxFieldWidth,
						int& nReadCount, int& bNegative, int& bIntegerOverflow)
	{
		ReadIntegerState state       = kRISError;
		uint64_t         nValue      = 0;
		int              nSpaceCount = 0;
		const int        kRISDone    = kRISError | kRISEnd;
		const int        kRISSuccess = kRISAfterZero | kRISReadDigits | kRISEnd;

		nReadCount       = 0;
		bNegative        = 0;
		bIntegerOverflow = 0;

		if((nBase != 1) && (nBase <= 36) && (nMaxFieldWidth >= 1))
		{
			uint64_t nMaxValueCheck  = 0; // This is what we compare nValue to as we build nValue. It is always equal to nValue / nBase. We need to do this because otherwise we'd compare overflowed values.
			int      c;

			state = kRISLeadingSpace;

			c = pReadFunction(kReadActionRead, 0, pContext);
			nReadCount++;

			if(nBase)
				nMaxValueCheck = (nMaxValue / nBase);

			while((c != kReadError) && (nReadCount <= nMaxFieldWidth) && ((state & kRISDone) == 0)) 
			{
				switch ((int)state)
				{
					case kRISLeadingSpace:
					{
						if(Isspace((CharT)c))
						{
							c = pReadFunction(kReadActionRead, 0, pContext);
							nSpaceCount++;
							// Stay in this state and read another char.
						}
						else
						{
							if(c == '-')
							{
								c = pReadFunction(kReadActionRead, 0, pContext);
								nReadCount++;

								bNegative = 1;
							}
							else if(c == '+')
							{
								c = pReadFunction(kReadActionRead, 0, pContext);
								nReadCount++;
							}

							state = kRISZeroTest;
						}

						break;
					}

					case kRISZeroTest:
					{
						if(((nBase == 0) || (nBase == 16)) && (c == '0')) // If nBase == 0, then we should expect hex (0x1234) or octal (01234)
						{
							c = pReadFunction(kReadActionRead, 0, pContext);
							nReadCount++;

							state = kRISAfterZero;
						}
						else
						{
							if(nBase == 0) // If the base hasn't been determined yet (e.g. by leading "0x"), then it must be 10.
								nBase = 10;

							if(nMaxValueCheck == 0)  // If this hasn't been set yet...
								nMaxValueCheck = (nMaxValue / nBase);

							state = kRISReadFirstDigit;
						}

						break;
					}

					case kRISAfterZero:
					{
						if((c == 'x') || (c == 'X'))
						{
							c = pReadFunction(kReadActionRead, 0, pContext);
							nReadCount++;

							nBase = 16;
							state = kRISReadFirstDigit;
						}
						else
						{
							if(nBase == 0)
								nBase = 8;

							state = kRISReadDigits;
						}

						if(nMaxValueCheck == 0)  // If this hasn't been set yet...
							nMaxValueCheck = (nMaxValue / nBase);

						break;
					}

					case kRISReadFirstDigit:
					case kRISReadDigits:
					{
						const int cDigit = (c - '0');

						if((unsigned)cDigit < 10)  // If c is compatible with base 2, 8, 10 ...
						{
							if(cDigit >= nBase)
							{
								if(state == kRISReadDigits)
									state = kRISEnd;
								else
									state = kRISError;

								break;
							}

							c = cDigit;
						}
						else  //Else we may have a hex digit, but we only pay attention to it if it's compatible with nBase (e.g. base 16).
						{
							int      cHex; // Actually it might be something other than hex if we are using a screwy base such as 18.
							CharT cLower;

							// If the base is > 10 and if c is a char that can represent a digit in the base...
							if((nBase > 10) && ((cLower = Tolower((CharT)c)) >= 'a') && ((cHex = (10 + (int)cLower - 'a')) < nBase))
								c = cHex;
							else
							{
								if(state == kRISReadDigits)
									state = kRISEnd;
								else
									state = kRISError;

								break;
							}
						}

						if(nValue > nMaxValueCheck)
							bIntegerOverflow = 1;

						nValue *= nBase;

						EA_ASSERT(c >= 0);
						if((unsigned)c > (nMaxValue - nValue))
							bIntegerOverflow = 1;

						nValue += c;
						state = kRISReadDigits;

						c = pReadFunction(kReadActionRead, 0, pContext);
						nReadCount++;

						break;
					}

				} // switch()

			} // while()

			// Return the final char back to the stream. It will typically be a 0 (nul terminator) char.
			pReadFunction(kReadActionUnread, c, pContext);

		} // if()

		if(state & kRISSuccess)
			nReadCount += nSpaceCount - 1;  // -1 because we un-read the last char above.
		else
		{
			nValue     = 0;
			nReadCount = 0;
		}

		return nValue;
	}

	double ReadDouble(ReadFunctionT pReadFunction, void* pContext,
						int nMaxFieldWidth, int cDecimalPoint, int& nReadCount, int& bOverflow)
	{
		int             c;
		DoubleValue     doubleValue;                // The string representation of the value, to be converted to actual value.
		double          dValue            = 0.0;
		int             nSpaceCount       = 0;
		int             nSignCount        = 0;      // There's supposed to be just zero or one of these.
		int             nFieldCount       = 0;
		int             nExponent         = 0;
		int             nExponentAdd      = 0;
		bool            bNegative         = false;
		bool            bExponentNegative = false;
		ReadDoubleState state             = kRDSLeadingSpace;
		const int       kRDSDone          = kRDSError | kRDSEnd;
		const int       kRDSSuccess       = kRDSSignificandLeading | kRDSIntegerDigits  |
											kRDSFractionLeading    | kRDSFractionDigits |
											kRDSExponentLeading    | kRDSExponentDigits |
											kRDSEnd;
		nReadCount = 0;
		bOverflow  = 0;

		c = pReadFunction(kReadActionRead, 0, pContext);
		nFieldCount++;

		while((c != kReadError) && (nFieldCount <= nMaxFieldWidth) && !(state & kRDSDone)) 
		{
			switch((int)state)
			{
				case kRDSLeadingSpace:
				{
					if(Isspace((CharT)c))
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nSpaceCount++;
						break;
					}

					switch(c)
					{
						case '-':
							bNegative = true;
							// Fall through
						case '+':
							c = pReadFunction(kReadActionRead, 0, pContext);
							nFieldCount++;
							nSignCount++;
							break;

						case 'i':  // Start of an "INF" or "INFINITY" sequence.
						case 'I':
							c = pReadFunction(kReadActionRead, 0, pContext);
							nFieldCount++;
							state = kRDSInfinity;
							break;

						case 'n':  // Start of an "NAN" or "NAN(...)" sequence.
						case 'N':
							c = pReadFunction(kReadActionRead, 0, pContext);
							nFieldCount++;
							state = kRDSNAN;
							break;

						default:
							state = kRDSSignificandBegin;
							break;
					}

					break;
				}

				case kRDSSignificandBegin:
				{
					if(c == cDecimalPoint)  // If there is no significand but there is instead the fraction-starting '.' char...
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						state = kRDSFractionBegin;
					}
					else if(c == '0')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						state = kRDSSignificandLeading;  // We eat leading zeroes.
					}
					else if(Isdigit((CharT)c))
						state = kRDSIntegerDigits;
					else
						state = kRDSError;

					break;
				}

				case kRDSSignificandLeading:
				{
					if(c == '0')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else
						state = kRDSIntegerDigits;

					break;
				}

				case kRDSIntegerDigits:
				{
					if(Isdigit((CharT)c))
					{
						if(doubleValue.mSigLen < kMaxSignificandDigits)  // If we have any more room...
							doubleValue.mSigStr[doubleValue.mSigLen++] = (char)c;
						else
							nExponentAdd++; // Lose significant digits but increase the exponent multiplier, so that the final result is close intended value, though chopped.

						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else
					{
						if(c == cDecimalPoint)
						{
							c = pReadFunction(kReadActionRead, 0, pContext);
							nFieldCount++;
							state = kRDSFractionDigits;
						}
						else
							state = kRDSSignificandEnd;
					}

					break;
				}

				case kRDSFractionBegin:
				{
					if(Isdigit((CharT)c))
						state = kRDSFractionDigits;
					else
						state = kRDSError;

					break;
				}

				case kRDSFractionDigits:
				{
					if(Isdigit((CharT)c))
					{
						if(doubleValue.mSigLen < kMaxSignificandDigits)  // If we have any more room...
						{
							nExponentAdd--;  // Fractional digits reduce our multiplier.

							if((c != '0') || doubleValue.mSigLen)
								doubleValue.mSigStr[doubleValue.mSigLen++] = (char)c;
						} // Else lose the remaining fractional part.

						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else
						state = kRDSSignificandEnd;

					break;
				}

				case kRDSSignificandEnd:
				{
					if(Toupper((CharT)c) == 'E')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						state = kRDSExponentBegin;
						break;
					}

					state = kRDSEnd;
					break;
				}

				case kRDSExponentBegin:
				{
					if(c == '+')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else if(c == '-')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						bExponentNegative = 1;
					}

					state = kRDSExponentBeginDigits;

					break;
				}

				case kRDSExponentBeginDigits:
				{
					if(c == '0')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						state = kRDSExponentLeading;
					}
					else if(Isdigit((CharT)c))
						state = kRDSExponentDigits;
					else
						state = kRDSError;

					break;
				}

				case kRDSExponentLeading:
				{
					if(c == '0')
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else
						state = kRDSExponentDigits;

					break;
				}

				case kRDSExponentDigits:
				{
					if(Isdigit((CharT)c))
					{
						nExponent = (nExponent * 10) + (c - '0');

						if(nExponent > kMaxDoubleExponent)
							bOverflow = 1;

						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}
					else
						state = kRDSEnd;

					break;
				}

				case kRDSInfinity:
				{
					// The C99 Standard specifies that we accept "INF" or "INFINITY", ignoring case. 
					int i = 1; // We have already matched the first 'I' char.

					while((i < 8) && ((int)Toupper((CharT)c) == (int)"INFINITY"[i]))
					{
						i++;
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
					}

					if((i == 3) || (i == 8))
					{
						if(bNegative)
							dValue = -kFloat64Infinity;
						else
							dValue =  kFloat64Infinity;

						nReadCount = nSpaceCount + nSignCount + i;

						return dValue; // Question: Do some FPUs refuse to accept infinity as-is?
					}
					else
						state = kRDSError;

					break;
				}

				case kRDSNAN:
				{
					// The C99 Standard specifies that we accept "NAN" or "NAN(n-char-sequence)", ignoring case. The n-char-sequence is implementation-defined but is a string specifying a particular NAN.
					int      i = 1; // We have already matched the first 'N' char.
					int      j = 0;
					//CharT pNANString[24]; pNANString[0] = 0;

					while((i < 4) && ((int)Toupper((CharT)c) == (int)"NAN("[i]))
					{
						c = pReadFunction(kReadActionRead, 0, pContext);
						nFieldCount++;
						i++;
					}

					if((i == 3) || (i == 4))
					{
						if(i == 4)
						{
							while((j < 32) && (Isdigit((CharT)c) || Isalpha((CharT)c)))
							{
								//pNANString[j] = (char16_t)c;
								c = pReadFunction(kReadActionRead, 0, pContext);
								nFieldCount++;
								j++;
							}

							if(c != ')')
							{
								state = kRDSError;
								break;
							}
							else
								j++;
						}

						// We currently ignore pNANString. To consider: Support some explicit NAN types based on the content of pNANString.
						//pNANString[j] = 0;

						if(bNegative)
							dValue = -kFloat64NAN;
						else
							dValue =  kFloat64NAN;

						nReadCount = nSpaceCount + nSignCount + i + j;

						return dValue; // Question: Don't some FPUs refuse to accept NANs as-is?
					}
					else
						state = kRDSError;

					break;
				}

			} // switch()

		} // while()

		// Return the final char back to the stream. It will typically be a 0 (nul terminator) char.
		pReadFunction(kReadActionUnread, c, pContext);

		if(state & kRDSSuccess)
		{
			nFieldCount--;
			nReadCount = nFieldCount + nSpaceCount;
		}
		else
		{
			nFieldCount = 0;
			nReadCount  = 0;
		}

		if(bExponentNegative)
			nExponent = -nExponent;

		// We've got a mSigStr/mExponent that is something like "123"/0 for the case of "123"
		// We've got a mSigStr/mExponent that is something like "123456"/-3 for the case of "123.456"
		// We remove trailing zeroes until we have at most just one trailing zero.
		int i = doubleValue.mSigLen - 1;

		while((i > 0) && (doubleValue.mSigStr[i] == '0'))
		{
			nExponentAdd++;
			i--;
		}

		if(i >= 0)
			doubleValue.mSigLen  = (int16_t)(i + 1);
		else
		{
			// In this case we have no significand or a significand of all zeroes.
			// Thus we have zero with some exponent, and the result is always zero,
			// even if we had some kind of apparent exponent overflow.
			bOverflow = false;

			return bNegative ? -0.0 : 0.0; // Usage of -0.0 requires that precise floating point be enabled under VC++. We ensure that above via EA_ENABLE_PRECISE_FP().

			// Alternative processing:
			// doubleValue.mSigLen    = 1;
			// doubleValue.mSigStr[0] = '0';  // Leave nExponent as it is, which is probably zero.
		}

		doubleValue.mExponent = (int16_t)(nExponent + nExponentAdd);

		if((doubleValue.mExponent < kMinDoubleExponent) || (doubleValue.mExponent > kMaxDoubleExponent))
			bOverflow = 1;
		// Note that it's still possible to have overflow if the exponent is present and less than max.

		if(bOverflow)
		{
			if(bExponentNegative)       // If the value is very small, return zero.
				return 0.0;
			else
			{
				// We don't set errno here; instead we set it in the caller.
				// errno = ERANGE;              // C99 standard, section 7.20.1.3-10

				if(bNegative)                   // If the value is a very large negative value...
					return -EASTDC_HUGE_VAL;    //    The C99 Standard (7.20.1.3-10) specifies that we return HUGE_VAL in the case of overflow.
				else                            // Else the value is a very large positive value.
					return  EASTDC_HUGE_VAL;
			}
		}

		dValue = doubleValue.ToDouble();

		if(dValue > DBL_MAX)   // If the value is denormalized too big...
		{
		  // We don't set errno here; instead we set it in the caller.
		  //errno = ERANGE;         // C99 standard, section 7.20.1.3-10
			bOverflow = 1;
			dValue    = EASTDC_HUGE_VAL;
		}
		else if((dValue != 0.0) && (dValue < DBL_MIN))   // If the value is denormalized too small...
		{
		  // We don't set errno here; instead we set it in the caller.
		  //errno = ERANGE;         // C99 standard, section 7.20.1.3-10
			bOverflow = 1;
		  //dValue    = 0.0;        // "If the result underflows (7.12.1), the functions return a value whose magnitude is no greater than the smallest normalized positive number in the return type; whether errno acquires the value ERANGE is implementation-defined."
		}

		if(bNegative)
			dValue = -dValue;

		return dValue;
	}
};


bool ReadFormatSpan8(FormatData& fd, int& c, ReadFunction8 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount)
{
	while(fd.mnWidth-- && ((c = pReadFunction(kReadActionRead, 0, pContext)) != -1) && fd.mCharBitmap.Get((char)c)) 
	{
		uint8_t c8 = (uint8_t)c; // It's easier to work with uint8_t instead of char, which might be signed.

		switch (stringTypeSize)
		{
			case 1:
				*((uint8_t*)pArgumentCurrent) = c8;
				++pArgumentCurrent;
				break;

			case 2:
			case 4:
			{
				if(c8 < 128)
				{
					if(stringTypeSize == 2)
						*((char16_t*)pArgumentCurrent) = c8;
					else
						*((char32_t*)pArgumentCurrent) = c8;
				}
				else
				{
					// We need to convert from UTF8 to UCS here. However, this can be complicated because 
					// multiple UTF8 chars may correspond to a single UCS char. Luckily, the UTF8 format  
					// allows us to know how many chars are in a multi-byte sequence based on the char value.
					char      buffer[7];
					const size_t utf8Len = utf8lengthTable[c8];
					char16_t     c16[2];
					char32_t     c32[2];
					int          result;

					buffer[0] = (char)c8;

					for(size_t i = 1; i < utf8Len; ++i)
					{
						c = pReadFunction(kReadActionRead, 0, pContext);

						if(c < 0)
							return false;
	 
						++nReadCount;
						buffer[i] = (char)c;
					}

							
					if(stringTypeSize == 2)
						result = Strlcpy(c16, buffer, 2, utf8Len);
					else
						result = Strlcpy(c32, buffer, 2, utf8Len);

					if(result < 0) // If the UTF8 sequence was malformed...
						return false;

					if(stringTypeSize == 2)
						*((char16_t*)pArgumentCurrent) = c16[0];
					else
						*((char32_t*)pArgumentCurrent) = c32[0];
				}

				pArgumentCurrent += stringTypeSize;
				break;
			}
		}

		++nReadCount;
	}
	return true;
}

bool ReadFormatSpan16(FormatData& fd, int& c, ReadFunction16 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount)
{
	while (fd.mnWidth-- && ((c = pReadFunction(kReadActionRead, 0, pContext)) != -1) && fd.mCharBitmap.Get((char16_t)c))
	{
		char16_t c16 = (char16_t)(unsigned)c;

		switch (stringTypeSize)
		{
		case 1:
			// We need to convert from UCS2 to UTF8 here. One UCS2 char may convert to 
			// as many as six UTF8 chars (though usually no more than three). 
			// This Strlcpy (16 to 8) can never fail.
			pArgumentCurrent += Strlcpy((char*)pArgumentCurrent, &c16, 7, 1);
			break;

		case 2:
			*((char16_t*)pArgumentCurrent) = (char16_t)c16;
			pArgumentCurrent += sizeof(char16_t);
			break;

		case 4:
			*((char32_t*)pArgumentCurrent) = c16;
			pArgumentCurrent += sizeof(char32_t);
			break;
		}

		++nReadCount;
	}
	return true;
}

bool ReadFormatSpan32(FormatData& fd, int& c, ReadFunction32 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount)
{
	while (fd.mnWidth-- && ((c = pReadFunction(kReadActionRead, 0, pContext)) != -1) && fd.mCharBitmap.Get((char32_t)c))
	{
		char32_t c32 = (char32_t)(unsigned)c;

		switch (stringTypeSize)
		{
		case 1:
			// We need to convert from UCS4 to UTF8 here. One UCS4 char may convert to 
			// as many as six UTF8 chars (though usually no more than three). 
			// This Strlcpy (32 to 8) can never fail.
			pArgumentCurrent += Strlcpy((char*)pArgumentCurrent, &c32, 7, 1);
			break;

		case 2:
			*((char16_t*)pArgumentCurrent) = (char16_t)c32;
			pArgumentCurrent += sizeof(char16_t);
			break;

		case 4:
			*((char32_t*)pArgumentCurrent) = c32;
			pArgumentCurrent += sizeof(char32_t);
			break;
		}

		++nReadCount;
	}
	return true;
}

typedef bool(*ReadFormatSpanFunction8)(FormatData& fd, int& c, ReadFunction8 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount);
typedef bool(*ReadFormatSpanFunction16)(FormatData& fd, int& c, ReadFunction16 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount);
typedef bool(*ReadFormatSpanFunction32)(FormatData& fd, int& c, ReadFunction32 pReadFunction, void* pContext, int stringTypeSize, char*& pArgumentCurrent, int& nReadCount);

int VscanfCore(ReadFunction8 pReadFunction8, void* pContext, const char* pFormat, va_list arguments)
{
	VscanfUtil<ReadFunction8, ReadFormatSpanFunction8, char> scanner;
	return scanner.VscanfCore(pReadFunction8, ReadFormatSpan8, pContext, pFormat, arguments);
}

int VscanfCore(ReadFunction16 pReadFunction16, void* pContext, const char16_t* pFormat, va_list arguments)
{
	VscanfUtil<ReadFunction16, ReadFormatSpanFunction16, char16_t> scanner;
	return scanner.VscanfCore(pReadFunction16, ReadFormatSpan16, pContext, pFormat, arguments);
}

int VscanfCore(ReadFunction32 pReadFunction32, void* pContext, const char32_t* pFormat, va_list arguments)
{
	VscanfUtil<ReadFunction32, ReadFormatSpanFunction32, char32_t> scanner;
	return scanner.VscanfCore(pReadFunction32, ReadFormatSpan32, pContext, pFormat, arguments);
}

// Undo the floating point precision statement we made above with EA_ENABLE_PRECISE_FP.
EA_RESTORE_PRECISE_FP()

} // namespace ScanfLocal
} // namespace StdC
} // namespace EA

EA_RESTORE_VC_WARNING()



