///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_INTERNAL_SCANFCORE_H
#define EASTDC_INTERNAL_SCANFCORE_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/stdioEA.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <math.h>


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4127)  // conditional expression is constant.
#endif



namespace EA
{
namespace StdC
{
namespace ScanfLocal
{

/////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////

typedef float  float32_t;
typedef double float64_t;

union FloatUint32
{
	uint32_t i;
	float    f;
};

union DoubleUint64
{
	uint64_t i;
	double   f;
};

const int          kConversionBufferSize           = EASCANF_FIELD_MAX + 8; // This is the size for a single field's representation, and not the entire formatted string representation. Multiple references say that this value must be at least 509, but I can't find that in the C99 standard.
const int          kMaxWidth                       = kConversionBufferSize - 8; 
const int          kMaxPrecision                   = kConversionBufferSize - 8; 
const int          kNoPrecisionLimit               = INT_MAX;
const int          kNoWidthLimit                   = INT_MAX;
const int          kMinDoubleExponent              = DBL_MIN_10_EXP;   // From <float.h>
const int          kMaxDoubleExponent              = DBL_MAX_10_EXP;   // From <float.h>
const int          kFormatError                    = 0;
const int          kMaxSignificandDigits           = 24;
const uint32_t     kFloat32PositiveInfinityBits    = UINT32_C(0x7F800000);
const uint64_t     kFloat64PositiveInfinityBits    = UINT64_C(0x7FF0000000000000);
const FloatUint32  kInfinityUnion32                = { kFloat32PositiveInfinityBits };
const DoubleUint64 kInfinityUnion64                = { kFloat64PositiveInfinityBits };
//const float32_t  kFloat32Infinity                = kInfinityUnion32.f;
//const float64_t  kFloat64Infinity                = kInfinityUnion64.f;
const uint32_t     kFloat32NANBits                 = UINT32_C(0x7FFFFFFF);
const uint64_t     kFloat64NANBits                 = UINT64_C(0x7FFFFFFFFFFFFFFF);
const FloatUint32  kNANUnion32                     = { kFloat32NANBits };
const DoubleUint64 kNANUnion64                     = { kFloat64NANBits };
//const float32_t  kFloat32NAN                     = kNANUnion32.f;
const float64_t    kFloat64NAN                     = kNANUnion64.f;


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
	kModifierInt,        // This is a placeholder, as integer is the default fd for integral types.
	kModifierLong,       // Use long instead of int. Specified by l in front of d, i, o, u, x, or X.
	kModifierLongLong,   // Use long long instead of int. Specified by ll in front of d, i, o, u, x, or X.
	kModifierMax_t,      // Use intmax_t argument. Specified by 'j' in front of d, i, o, u, x, or X.
	kModifierSize_t,     // Use size_t argument. Specified by 'z' in front of d, i, o, u, x, or X.
	kModifierPtrdiff_t,  // Use ptrdiff_t argument. Specified by 't' in front of d, i, o, u, x, or X.
	kModifierDouble,     // Use double instead of float. Specified by nothing in front of e, E, f, F, g, G for printf and l for scanf.
	kModifierLongDouble, // Use long double instead of double. Specified by l in front of e, E, f, F, g, G for printf and L for scanf.
	kModifierWChar,      // Use wide char instead of char. Specified by l (in front of c).
	kModifierInt8,       // Use int8_t or uint8_t.   Specified by I8 in front of d, i, o, u.
	kModifierInt16,      // Use int16_t or uint16_t. Specified by I16 in front of d, i, o, u.
	kModifierInt32,      // Use int32_t or uint32_t. Specified by I32 in front of d, i, o, u.
	kModifierInt64,      // Use int64_t or uint64_t. Specified by I64 in front of d, i, o, u.
	kModifierInt128      // Use int128_t or uint128_t. Specified by I128 in front of d, i, o, u.
};


enum ReadIntegerState                       // The ^ chars below indicate what part of the string the state refers to.
{                                           // "   -00123456"
	kRISLeadingSpace        = 0x0001,       //  ^
	kRISZeroTest            = 0x0002,       //     ^
	kRISAfterZero           = 0x0004,       //        ^
	kRISReadFirstDigit      = 0x0008,       //        ^
	kRISReadDigits          = 0x0010,       //         ^ 
	kRISEnd                 = 0x0020,       //              ^
	kRISError               = 0x0040        //              ^
};

enum ReadDoubleState                        // The ^ chars below indicate what part of the string the state refers to.
{                                           // "   -123.345e-0023"
	kRDSLeadingSpace        = 0x0001,       //  ^
	kRDSSignificandBegin    = 0x0002,       //     ^
	kRDSSignificandLeading  = 0x0004,       //     ^
	kRDSIntegerDigits       = 0x0008,       //      ^
	kRDSFractionBegin       = 0x0010,       //          ^
	kRDSFractionLeading     = 0x0020,       //          ^
	kRDSFractionDigits      = 0x0040,       //           ^
	kRDSSignificandEnd      = 0x0080,       //             ^
	kRDSExponentBegin       = 0x0100,       //              ^
	kRDSExponentBeginDigits = 0x0200,       //              ^
	kRDSExponentLeading     = 0x0400,       //               ^
	kRDSExponentDigits      = 0x0800,       //                ^
	kRDSInfinity            = 0x1000,       //                   ^
	kRDSNAN                 = 0x2000,       //                   ^
	kRDSEnd                 = 0x4000,       //                   ^
	kRDSError               = 0x8000        //                   ^
};



/////////////////////////////////////////////////////////////////////////////
// CharBitmap
//
// Used to do fast char set tests.
/////////////////////////////////////////////////////////////////////////////

struct CharBitmap
{
	uint32_t mBits[8];  // 32 bits per uint32_t times 8 values => 256 bits.

	CharBitmap()
		{ memset(mBits, 0, sizeof(mBits)); }

	int Get(char c) const
		{ return (int)mBits[(uint8_t)(unsigned)c >> 5] & (1 << (c & 31)); }

	// This isn't correct. To do this completely right for all uses of scanf, 
	// we need to make a 16 bit bitmap which handles 65536 bits. However, this
	// would entail rather obscure scanf %[...]s usage for Unicode text.
	int Get(char16_t c) const // If c >= 256, we return whatever the first bit is, since it will be equal to what bits 256 - 65536 are meant to be.
		{ if(c < 256) return (int)mBits[(uint8_t)(unsigned)c >> 5] & (1 << (c & 31)); else return (int)(mBits[0] & 0x00000001); }

	int Get(char32_t c) const // If c >= 256, we return whatever the first bit is, since it will be equal to what bits 256 - 2^32 are meant to be.
		{ if(c < 256) return (int)mBits[(uint8_t)(unsigned)c >> 5] & (1 << (c & 31)); else return (int)(mBits[0] & 0x00000001); }

	void Set(char c)
		{ mBits[(uint8_t)(unsigned)c >> 5] |= (1 << (c & 31)); }

	void Set(char16_t c)
		{ if((uint16_t)c < 256) mBits[(uint8_t)(uint16_t)c >> 5] |= (1 << (c & 31)); }

	void Set(char32_t c)
		{ if((uint32_t)c < 256) mBits[(uint8_t)(uint32_t)c >> 5] |= (1 << (c & 31)); }

	void SetAll()
		{ memset(mBits, 0xffffffff, sizeof(mBits)); }

	void NegateAll()
		{ for(size_t i = 0; i < 8; i++) mBits[i] = ~mBits[i]; }

	void ClearAll()
		{ memset(mBits, 0, sizeof(mBits)); }

	void Clear(int c)
		{ mBits[(uint8_t)(unsigned)c >> 5] &= ~(1 << (c & 31)); }
};


/////////////////////////////////////////////////////////////////////////////
// DoubleValue
//
// Used as the lowest level string representation of a double.
/////////////////////////////////////////////////////////////////////////////

struct DoubleValue
{
	char mSigStr[kMaxSignificandDigits + 1];  // String
	int16_t mSigLen;                             // Length of string
	int16_t mExponent;                           // Exponent value.

	DoubleValue()
	  : mSigLen(0), mExponent(0)
	{ mSigStr[0] = 0; }

	double ToDouble() const;
};


/////////////////////////////////////////////////////////////////////////////
// FormatData
//
// Used by Scanf's state machine.
/////////////////////////////////////////////////////////////////////////////

struct FormatData
{
	int         mnWidth;            // Field width in characters.
	Modifier    mModifier;          // One of enum Modifier. Example is 'h' for 'short'.
	int         mnType;             // 'c', 'C', 'b', 'd', 'i', 'u', 'e', 'E', 'f', 'g', 'G', 'o', 's', 'S', 'x', 'X', 'p', 'n', '%', or 0 (for error).
	bool        mbWidthSpecified;   // True if the field width was specified by the user.
	bool        mbSkipAssignment;   // True if the user used * in the format, which means to eat the input field without assigning it to anything.
	CharBitmap  mCharBitmap;        // Allows fast character inclusion/exclusion tests.
	int         mDecimalPoint;      // Typically equal to '.', but could be ',' for some locales.

	FormatData()
	  : mnWidth(kNoWidthLimit),
		mModifier(kModifierNone),
		mnType(kFormatError),
		mbWidthSpecified(false),
		mbSkipAssignment(false),
		mCharBitmap(),
		mDecimalPoint('.')
	{}
};


/////////////////////////////////////////////////////////////////////////////
// SscanfContext8 / SscanfContext16
//
// Used by StringReader8 / StringReader16
/////////////////////////////////////////////////////////////////////////////

struct SscanfContext8
{
	const char* mpSource;
	int            mbEndFound;

	SscanfContext8(const char* pSource = NULL)
	  : mpSource(pSource),
		mbEndFound(0)
	{}
};

struct SscanfContext16
{
	const char16_t* mpSource;
	int             mbEndFound;

	SscanfContext16(const char16_t* pSource = NULL)
	  : mpSource(pSource),
		mbEndFound(0)
	{}
};

struct SscanfContext32
{
	const char32_t* mpSource;
	int             mbEndFound;

	SscanfContext32(const char32_t* pSource = NULL)
	  : mpSource(pSource),
		mbEndFound(0)
	{}
};


/////////////////////////////////////////////////////////////////////////////
// Reader functions, of type ReadFunction8
/////////////////////////////////////////////////////////////////////////////

int FILEReader8  (ReadAction readAction, int value, void* pContext);
int FILEReader16 (ReadAction readAction, int value, void* pContext);
int FILEReader32 (ReadAction readAction, int value, void* pContext);

int StringReader8 (ReadAction readAction, int value, void* pContext);
int StringReader16(ReadAction readAction, int value, void* pContext);
int StringReader32(ReadAction readAction, int value, void* pContext);


///////////////////////////////////////////////////////////////////////////////
// VscanfCore
//
int VscanfCore(ReadFunction8  pReadFunction8,  void* pReadFunction8Context, const char* pFormat,  va_list arguments);
int VscanfCore(ReadFunction16 pReadFunction16, void* pReadFunction8Context, const char16_t* pFormat, va_list arguments);
int VscanfCore(ReadFunction32 pReadFunction32, void* pReadFunction8Context, const char32_t* pFormat, va_list arguments);


} // namespace ScanfLocal
} // namespace StdC
} // namespace EA



#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard


