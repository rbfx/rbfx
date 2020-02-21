///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTDC_INTERNAL_SPRINTFCORE_H
#define EASTDC_INTERNAL_SPRINTFCORE_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/stdioEA.h>
#include <EAStdC/EASprintf.h>

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif



/////////////////////////////////////////////////////////////////////////////
// IsNeg
//
// bool IsNeg(double x); 
//
#if defined(EA_PLATFORM_MICROSOFT)
	#define IsNeg(x) ((*((uint64_t*)&(x)) & UINT64_C(0x8000000000000000)) != 0) // This assumes that double is a 64 bit value.
#elif defined(__APPLE__) || defined(EA_PLATFORM_ANDROID)
	inline  bool IsNeg(double x) { union { double f; uint64_t i; } u = { x }; return (u.i & UINT64_C(0x8000000000000000)) != 0; }
#elif defined(__CYGWIN__) && defined(__GNUC__)
	inline  bool IsNeg(double x) { union { double f; uint64_t i; } u = { x }; return (u.i & UINT64_C(0x8000000000000000)) != 0; }
#else
	inline  bool IsNeg(double x) { union { double f; uint64_t i; } u = { x }; return (u.i & UINT64_C(0x8000000000000000)) != 0; }
#endif




/////////////////////////////////////////////////////////////////////////////
// EASPRINTF_MIN / EASPRINTF_MAX
//
#ifndef EASPRINTF_MIN
	#define EASPRINTF_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef EASPRINTF_MAX
	#define EASPRINTF_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif



namespace EA
{
namespace StdC
{


namespace SprintfLocal
{

/////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////

const int      kConversionBufferSize = EASPRINTF_FIELD_MAX + 8; // This is the size for a single field's representation, and not the entire formatted string representation. Multiple references say that this value must be at least 509, but I can't find that in the C99 standard.
const int      kMaxWidth             = kConversionBufferSize - 8; 
const int      kMaxPrecision         = kConversionBufferSize - 8; 
const int      kNoPrecision          = INT_MAX;
const int      kFormatError          = 0;
const char  kStringNull8[]        = { '(', 'n', 'u', 'l', 'l', ')', '\0' }; // Used if the user uses "%s" but passes NULL as the string pointer.
const char16_t kStringNull16[]       = { '(', 'n', 'u', 'l', 'l', ')', '\0' };
const char32_t kStringNull32[]       = { '(', 'n', 'u', 'l', 'l', ')', '\0' };


/////////////////////////////////////////////////////////////////////////////
// Enumerations
/////////////////////////////////////////////////////////////////////////////

enum Alignment          // The C99 standard incorrectly uses the term "justification" when it means to use "alignment". There is no such thing as left or right justification, but thinking so is a common mistake for typography newbies.
{                       // 
	kAlignmentLeft,     // 
	kAlignmentRight,    // 
	kAlignmentZeroFill  // 
};

enum Sign
{
	kSignNone,          // Never show any sign.
	kSignMinus,         // Only show sign if minus (this is default).
	kSignMinusPlus,     // Show sign if plus or minus.
	kSignSpace          // Show space in place of a plus sign.
};

enum Modifier
{
	kModifierNone,       // No modifier, use the type as-is.
	kModifierChar,       // Use char instead of int. Specified by hh in front of d, i, o, u, x, or X.
	kModifierShort,      // Use short instead of int. Specified by h in front of d, i, o, u, x, or X.
	kModifierInt,        // This is a placeholder, as integer is the default format for integral types.
	kModifierLong,       // Use long instead of int. Specified by l in front of d, i, o, u, x, or X.
	kModifierLongLong,   // Use long long instead of int. Specified by ll in front of d, i, o, u, x, or X.
	kModifierMax_t,      // Use intmax_t argument. Specified by 'j' in front of d, i, o, u, x, or X.
	kModifierSize_t,     // Use size_t argument. Specified by 'z' in front of d, i, o, u, x, or X.
	kModifierPtrdiff_t,  // Use ptrdiff_t argument. Specified by 't' in front of d, i, o, u, x, or X.
	kModifierDouble,     // This is a placeholder, as double is the default format for floating point types.
	kModifierLongDouble, // Use long double instead of double. Specified by l in front of e, E, f, g, G.
	kModifierWChar,      // Use wide char instead of char. Specified by l (in front of c).
	kModifierInt8,       // Use int8_t or uint8_t.     Specified by I8 in front of d, i, o, u.
	kModifierInt16,      // Use int16_t or uint16_t.   Specified by I16 in front of d, i, o, u.
	kModifierInt32,      // Use int32_t or uint32_t.   Specified by I32 in front of d, i, o, u.
	kModifierInt64,      // Use int64_t or uint64_t.   Specified by I64 in front of d, i, o, u.
	kModifierInt128      // Use int128_t or uint128_t. Specified by I128 in front of d, i, o, u.
};


/////////////////////////////////////////////////////////////////////////////
// Structs
/////////////////////////////////////////////////////////////////////////////

union AllTypes
{
	char        mChar;
	short       mShort;
	int         mInt;
	long        mLong;
	long long   mLongLong;
	intmax_t    mMax;
	size_t      mSize;
	ptrdiff_t   mPtrDiff;
	double      mDouble;
	long double mLongDouble;
	wchar_t     mWChar;
	int8_t      mInt8;
	int16_t     mInt16;
	int32_t     mInt32;
	int64_t     mInt64;
  //int128_t    mInt128; // Disabled until we support it fully. We have the EAStdC int128_t type we can use.
};

struct FormatData
{
	Alignment mAlignment;           // One of enum Alignment.
	Sign      mSign;                // One of enum Sign.
	bool      mbAlternativeForm;    // See the C99 standard, section 7.19.6.1.6.
	int       mnWidth;              // Field width in characters.
	int       mnPrecision;          // A value of kNoPrecision means that precision was not specified.
	Modifier  mModifier;            // One of enum Modifier. Example is 'h' for 'short'.
	int       mnType;               // 'c', 'C', 'b', 'd', 'i', 'u', 'e', 'E', 'f', 'g', 'G', 'o', 's', 'S', 'x', 'X', 'p', 'n', '%', or 0 (for error).
	int       mDecimalPoint;        // Typically equal to '.', but could be ',' for some locales.
	bool      mbDisplayThousands;   // Non-standard extension, though found in a number of Unix compiler C library printf implementations.
	int       mThousandsSeparator;  // Typically equal to ',' but could be '.' or ' ' for some locales.

	FormatData() 
	  : mAlignment(kAlignmentRight), // The standard states (7.19.6.1.6) that right alignment is default.
		mSign(kSignMinus),           // The standard states (7.19.6.1.6) that the default mode is to display the minus sign only.
		mbAlternativeForm(false),
		mnWidth(0), 
		mnPrecision(kNoPrecision),
		mModifier(kModifierNone), 
		mnType(kFormatError),
		mDecimalPoint('.'),          // To do: Make this configurable.
		mbDisplayThousands(false),
		mThousandsSeparator(',')     // To do: Make this configurable.
	{}
};



struct SnprintfContext8
{
	char* mpDestination;     // Start of destination data. Ptr doesn't change once it has been initialized.
	size_t   mnCount;           // Count written to destination so far.
	size_t   mnMaxCount;        // The max count we can write to the destination.
	bool     mbMaxCountReached; // True if the max count has been reached. Used because multi-byte strings (e.g. UTF8) could end with mnCount < mnMaxCount.

	SnprintfContext8(char* pDestination = NULL, size_t nCount = 0, size_t nMaxCount = (size_t)-1)
	  : mpDestination(pDestination),
		mnCount(nCount),
		mnMaxCount(nMaxCount),
		mbMaxCountReached(false)
	{}
};

struct SnprintfContext16
{
	char16_t* mpDestination;
	size_t    mnCount;
	size_t    mnMaxCount;

	SnprintfContext16(char16_t* pDestination = NULL, size_t nCount = 0, size_t nMaxCount = (size_t)-1)
	  : mpDestination(pDestination),
		mnCount(nCount),
		mnMaxCount(nMaxCount)
	{}
};

struct SnprintfContext32
{
	char32_t* mpDestination;
	size_t    mnCount;
	size_t    mnMaxCount;

	SnprintfContext32(char32_t* pDestination = NULL, size_t nCount = 0, size_t nMaxCount = (size_t)-1)
	  : mpDestination(pDestination),
		mnCount(nCount),
		mnMaxCount(nMaxCount)
	{}
};

#ifdef EA_PLATFORM_ANDROID
	struct PlatformLogWriterContext8
	{
		char mBuffer[512];
		size_t  mPosition;

		PlatformLogWriterContext8() { mBuffer[0] = 0; mPosition = 0; }
	};
#else
	// No context is required on most platforms
	struct PlatformLogWriterContext8 {};
#endif

// Default output writers
int StringWriter8     (const char*  EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8,  WriteFunctionState wfs);
int FILEWriter8       (const char*  EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8,  WriteFunctionState wfs);
int PlatformLogWriter8(const char*  EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext8,  WriteFunctionState wfs);

int StringWriter16   (const char16_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext16, WriteFunctionState wfs);
int FILEWriter16     (const char16_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext16, WriteFunctionState wfs);

int StringWriter32   (const char32_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext32, WriteFunctionState wfs);
int FILEWriter32     (const char32_t* EA_RESTRICT pData, size_t nCount, void* EA_RESTRICT pContext32, WriteFunctionState wfs);


// VaListContainer
//
// This exists because some compilers (e.g. some versions of GCC, Green Hills) aren't complaint with the 
// C99 Standard in allowing the taking of the address of a va_list variable. The problem is that some 
// compilers implement va_list as an array[1], which is converted to a pointer upon passing to a function,
// and this breaks the ability of us to take its address as a va_list. We work around that by putting 
// va_list objects in a struct, which can be passed as an argument to a function without changing its type.
// This struct isn't yet in use while we investigate possible alternative workarounds for the compiler issues.
//
struct VaListContainer
{
	va_list value;
};


///////////////////////////////////////////////////////////////////////////////
// Internal init/shutdown functions. 
// The user doesn't need to call these, as these are already called when the 
// user calls the EAStdC::Init and EAStdC::Shutdown high level functions.
//
void EASprintfInit();
void EASprintfShutdown();


///////////////////////////////////////////////////////////////////////////////
// ReadFormat
//
// Reads the current format into FormatData. Return value is pointer to first
// char/char16_t after the format data.
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
const CharT* ReadFormat(const CharT* EA_RESTRICT pFormat, SprintfLocal::FormatData* EA_RESTRICT pFormatData, va_list* EA_RESTRICT pArguments);

///////////////////////////////////////////////////////////////////////////////
// VprintfCore
//
int VprintfCore(WriteFunction8  pWriteFunction8,  void* EA_RESTRICT pWriteFunctionContext8,  const char*  EA_RESTRICT pFormat, va_list arguments);
int VprintfCore(WriteFunction16 pWriteFunction16, void* EA_RESTRICT pWriteFunctionContext16, const char16_t* EA_RESTRICT pFormat, va_list arguments);
int VprintfCore(WriteFunction32 pWriteFunction32, void* EA_RESTRICT pWriteFunctionContext32, const char32_t* EA_RESTRICT pFormat, va_list arguments);



} // namespace SprintfLocal
} // namespace StdC
} // namespace EA


#endif // Header include guard





