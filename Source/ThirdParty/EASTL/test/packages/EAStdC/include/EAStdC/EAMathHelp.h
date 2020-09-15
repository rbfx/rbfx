///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements fast, specialized scalar math primitives. This is not a general
// purpose vector math library, but rather simply a portable version of some
// basic FPU primitives.
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAMATHHELP_H
#define EASTDC_EAMATHHELP_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>


namespace EA
{
namespace StdC
{
	typedef float float32_t;
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

	///////////////////////////////////////////////////////////////////////////////
	// Constants
	//
	const uint32_t     kFloat32SignMask                = UINT32_C(0x80000000);
	const uint32_t     kFloat32ExponentMask            = UINT32_C(0x7F800000);
	const uint32_t     kFloat32MantissaMask            = UINT32_C(0x007FFFFF);
	const uint32_t     kFloat32SignAndExponentMask     = UINT32_C(0xFF800000);
	const uint32_t     kFloat32SignAndMantissaMask     = UINT32_C(0x807FFFFF);
	const uint32_t     kFloat32ExponentAndMantissaMask = UINT32_C(0x7FFFFFFF);
	const uint32_t     kFloat32PositiveInfinityBits    = UINT32_C(0x7F800000);
	const unsigned     kFloat32SignBits                = 1;
	const unsigned     kFloat32ExponentBits            = 8;
	const unsigned     kFloat32MantissaBits            = 23;
	const unsigned     kFloat32BiasValue               = 127;

	const uint64_t     kFloat64SignMask                = UINT64_C(0x8000000000000000);
	const uint64_t     kFloat64ExponentMask            = UINT64_C(0x7FF0000000000000);
	const uint64_t     kFloat64MantissaMask            = UINT64_C(0x000FFFFFFFFFFFFF);
	const uint64_t     kFloat64SignAndExponentMask     = UINT64_C(0xFFF0000000000000);
	const uint64_t     kFloat64SignAndMantissaMask     = UINT64_C(0x800FFFFFFFFFFFFF);
	const uint64_t     kFloat64ExponentAndMantissaMask = UINT64_C(0x7FFFFFFFFFFFFFFF);
	const uint64_t     kFloat64PositiveInfinityBits    = UINT64_C(0x7FF0000000000000);
	const unsigned     kFloat64SignBits                = 1;
	const unsigned     kFloat64ExponentBits            = 11;
	const unsigned     kFloat64MantissaBits            = 52;
	const unsigned     kFloat64BiasValue               = 1023;

	const FloatUint32  kInfinityUnion32                = { kFloat32PositiveInfinityBits };
	const DoubleUint64 kInfinityUnion64                = { kFloat64PositiveInfinityBits };
	#define            kFloat32Infinity                  kInfinityUnion32.f
	#define            kFloat64Infinity                  kInfinityUnion64.f


	// bias to integer
	const float32_t kFToIBiasF32 = (float32_t)(uint32_t(3) << 22);
	const int32_t   kFToIBiasS32 = 0x4B400000;           // Same as ((int32_t&) kFToIBiasF32), but known to optimizer at compile time.
	const float64_t kFToIBiasF64 = (float64_t)(uint64_t(3) << 52);

	// bias to 8-bit fraction
	const float32_t kFToI8BiasF32 = (float32_t)(uint32_t(3) << 14);
	const int32_t   kFToI8BiasS32 = 0x47400000;          // Same as ((int32_t&) kFToI8BiasF32), but known to optimizer at compile time.

	// bias to 16-bit fraction
	const float32_t kFToI16BiasF32 = (float32_t)(uint32_t(3) << 6);
	const int32_t   kFToI16BiasS32 = 0x43400000;         // Same as ((int32_t&) kFToI16BiasF32), but known to optimizer at compile time.


	///////////////////////////////////////////////////////////////////////
	// MulDiv
	//
	// Returns the result of the expression a * b / divisor without loss of
	// precision. This is useful because any operation of two (e.g.) 32 bit
	// values can generate a 64 bit result.
	// divisor cannot be 0.
	//
	// Example usage:
	//     int64_t result = MulDiv(INT64_C(0x8000000000000000), 
	//                             INT64_C(0x8000000000000000), 
	//                             INT64_C(0x4000000000000000));
	//     // result -> 0x10000000000000000
	//
	int32_t  MulDiv(int32_t  a, int32_t  b, int32_t  divisor);
	uint32_t MulDiv(uint32_t a, uint32_t b, uint32_t divisor);
	int64_t  MulDiv(int64_t  a, int64_t  b, int64_t  divisor);
	uint64_t MulDiv(uint64_t a, uint64_t b, uint64_t divisor);    
 
	
	///////////////////////////////////////////////////////////////////////
	// DivMod
	//
	// Returns the integer result of dividend / divisor and dividend % divisor 
	// in a single operation.
	// This is useful because division is slow on some platforms and performing
	// a div and mod at the same time will be faster than performing both 
	// separately. ARM processors don't have an integer divide function even
	// in 32 bit mode, and most processors don't have a 64 bit integer divide
	// function in 32 bit mode. In those cases this function is a potential 2x win.
	// This function is similar to the C Standard div, ldiv, and lldiv functions
	// except with unsigned versions and portable to all compilers/platforms.
	// Returns the result of the division. The modulus (remainder of the division) 
	// is written to modResult.
	// divisor cannot be 0, and modResult must be valid.
	//
	// Example usage:
	//     int64_t result, remainder;
	//     result = DivMod(14, 3, &remainder); 
	//     // result -> 4, remainder-> 2.
	//
	int32_t  DivMod(int32_t  dividend, int32_t  divisor, int32_t*  modResult);
	uint32_t DivMod(uint32_t dividend, uint32_t divisor, uint32_t* modResult);
	int64_t  DivMod(int64_t  dividend, int64_t  divisor, int64_t*  modResult);
	uint64_t DivMod(uint64_t dividend, uint64_t divisor, uint64_t* modResult);    


	///////////////////////////////////////////////////////////////////////
	// Full range conversion functions.
	//
	// These are good for floats within the full range of a float. Remember
	// that a single-precision float only has a 24-bit significand so
	// most integers |x| > 2^24 cannot be represented exactly.
	//
	// The result of converting an out-of-range number, infinity, or NaN
	// is undefined.
	//
	inline uint32_t RoundToUint32(float fValue);
	inline int32_t RoundToInt32(float fValue);
	inline int32_t FloorToInt32(float fValue);
	inline int32_t CeilToInt32(float fValue);
	inline int32_t TruncateToInt32(float fValue);

	///////////////////////////////////////////////////////////////////////
	// Partial range conversion functions.
	//
	// These are only good for |x| <= 2^23. The result of converting an
	// out-of-range number, infinity, or NaN is undefined.
	//
	inline int32_t FastRoundToInt23(float fValue);

	///////////////////////////////////////////////////////////////////////
	// Unit-to-byte functions.
	//
	// Converts real values in the range |x| <= 1 to unsigned 8-bit values
	// [0, 255]. The result of calling UnitFloatToUint8() with |x|>1 is
	// undefined.
	//
	inline uint8_t UnitFloatToUint8(float fValue);
	inline uint8_t ClampUnitFloatToUint8(float fValue);

	///////////////////////////////////////////////////////////////////////
	// IsInvalid
	//
	// Returns true if a value does not obey normal arithmetic rules;
	// specifically, x != x. In the case of Visual C++ 2003, this is true
	// for NaNs and indefinites, and not for normalized finite values,
	// denormals, and infinities. Other compilers may return different
	// results or even false for all values.
	//
	// IsInvalid() is useful as a fast assert check that floats are
	// sane and won't poison computations as NaNs can with masked
	// exceptions.
	//
	inline bool IsInvalid(float32_t fValue);
	inline bool IsInvalid(float64_t fValue);

	///////////////////////////////////////////////////////////////////////////////
	// IsNormal
	//
	// Returns true if the value is a normalized finite number. That is, it is neither
	// an infinite, nor a NaN (including indefinite NaN), nor a denormalized number.
	// You generally want to write math operation checking code that asserts for 
	// IsNormal() as opposed to checking specifically for IsNaN, etc.
	//
	// Normal values are defined as any floating point value with an exponent in 
	// the range of [1, 254], as 0 is reserved for denormalized (underflow) values 
	// and 255 is reserved for infinite (overflow) and NaN values. 
	//
	inline bool IsNormal(float32_t fValue);
	inline bool IsNormal(float64_t fValue);

	///////////////////////////////////////////////////////////////////////////////
	// IsNAN
	//
	// A NaN is a special kind of number that is neither finite nor infinite. 
	// It is the result of doing things like the following:
	//    float x = 1 * NaN;
	//    float x = NaN + NaN;
	//    float x = 0 / 0;
	//    float x = 0 / infinite;
	//    float x = infinite - infinite
	//    float x = sqrt(-1);
	//    float x = cos(infinite);
	// Under the VC++ debugger, x will be displayed as 1.#QNAN00 or 1.#IND00 and 
	// the bit representation of x will be 0x7fc00001 (in the case of 1 * NaN). 
	// The 'Q' in front of NAN stands for "quiet" and means that use of that value 
	// in expressions won't generate exceptions. A signaling NaN (SNAN) means that 
	// use of the value would generate exceptions.
	// 
	// NaNs are frequently generated in physics simulations and similar mathematical 
	// situations when you are simulating an object moving or turning over time but 
	// the time or distance differential in the calculation is very small. 
	// Also, floating point roundoff error can generate NaNs if you do things 
	// like call acos(x) where you didn't take care to clamp x to <= 1. You can 
	// also get a NaN when memory used to store a floating point value is written 
	// with random data.
	//
	// A curious property of NaNs is that all comparisons between NaNs return 
	// false except the expression: NaN != NaN. This is so even if the bit 
	// representation of the two compared NaNs are identical. Thus, with NaNs, 
	// the following holds:
	//    x == x is always false
	//    x < y is always false
	//    x > y is always false
	//
	// As a result, one simple way to test for a NaN without fiddling with bits is 
	// to simply test for x == x. If this returns false, then you have a NaN. 
	// Unfortunately, many C and C++ compilers don't obey this, so you are usually 
	// stuck fiddling with bits.
	//
	// With a NaN, all exponent bits are 1 and the mantissa is not zero.
	// If the highest fraction bit is 1, the NAN is "quiet" -- it represents
	// and indeterminant operation rather than an invalid one.
	//
	inline bool IsNAN(float32_t fValue);
	inline bool IsNAN(float64_t fValue);

	///////////////////////////////////////////////////////////////////////////////
	// IsInfinite
	//
	// A value is infinity if the exponent bits are all 1 and all the bits of the 
	// mantissa (significand) are 0. The sign bit indicates positive or negative
	// infinity. Thus, for Float32, 0x7f800000 is positive infinity and 0xff800000
	// is negative infinity.
	//
	inline bool IsInfinite(float32_t fValue);
	inline bool IsInfinite(float64_t fValue);

	///////////////////////////////////////////////////////////////////////////////
	// IsIndefinite
	//
	// An indefinite is a special kind of NaN that is used to signify that an 
	// operation between non-NaNs generated a NaN. Other than that, it really is 
	// simply another NaN.
	//
	inline bool IsIndefinite(float32_t fValue);
	inline bool IsIndefinite(float64_t fValue);

	///////////////////////////////////////////////////////////////////////////////
	// IsDenormalized
	//
	// Much in the same way that infinite numbers represent an overflow, 
	// denormalized numbers represent an underflow. A denormalized number is 
	// indicated by an exponent with a value of zero. You get a denormalized 
	// number when you do operations such as this:
	//    float x = 1e-10 / 1e35;
	// Under the VC++ debugger, x will be displayed as 1.4e-045#DEN and the 
	// bit representation of x will be 0x00000001. Unlike infinites and NaNs, 
	// you can still do math with denormalized numbers. However, the results 
	// of your math will likely have a lot of imprecision. You can also get a 
	// denormalized value when memory used to store a floating point value is 
	// written with random data.
	//
	inline bool IsDenormalized(float32_t fValue);
	inline bool IsDenormalized(float64_t fValue);

} // namespace StdC
} // namespace EA




// Common implementations.
namespace EA
{
namespace StdC
{
	const int32_t kFloat32LowestExponentBit = kFloat32ExponentMask & (~kFloat32ExponentMask << 1);
	const int64_t kFloat64LowestExponentBit = kFloat64ExponentMask & (~kFloat64ExponentMask << 1);

	inline bool IsInvalid(float32_t fValue) {
		return fValue != fValue;
	}

	inline bool IsInvalid(float64_t fValue) {
		return fValue != fValue;
	}

	inline bool IsNormal(float32_t fValue) {
		const union {
			float32_t f;
			int32_t i;
		} converter = { fValue };

		// An IEEE real value is normalized finite if its exponent field
		// is not either all zeroes or ones, or if the number is zero;
		return !(converter.i & ~kFloat32SignMask) || ((converter.i - (uint32_t)kFloat32LowestExponentBit) & kFloat32ExponentMask) < (kFloat32ExponentMask - kFloat32LowestExponentBit);
	}

	inline bool IsNormal(float64_t fValue) {
		const union {
			float64_t f;
			int64_t i;
		} converter = { fValue };

		// An IEEE real value is normalized finite if its exponent field
		// is not either all zeroes or ones.
		return !(converter.i & ~kFloat64SignMask) || ((converter.i - (uint64_t)kFloat64LowestExponentBit) & kFloat64ExponentMask) < (kFloat64ExponentMask - kFloat64LowestExponentBit);
	}

	inline bool IsNAN(float32_t fValue) {
		const union {
			float32_t f;
			int32_t i;
		} converter = { fValue };

		// An IEEE real value is a NaN if all exponent bits are one and
		// the mantissa is not zero.
		return (converter.i & ~kFloat32SignMask) > kFloat32ExponentMask;
	}

	inline bool IsNAN(float64_t fValue) {
		const union {
			float64_t f;
			int64_t i;
		} converter = { fValue };

		// An IEEE real value is a NaN if all exponent bits are one and
		// the mantissa is not zero.
		return (converter.i & ~kFloat64SignMask) > kFloat64ExponentMask;
	}

	inline bool IsInfinite(float32_t fValue) {
		const union {
			float32_t f;
			int32_t i;
		} converter = { fValue };

		// An IEEE real value is infinite if all exponent bits are one
		// and the mantissa is zero.
		return (converter.i & ~kFloat32SignMask) == kFloat32PositiveInfinityBits;
	}

	inline bool IsInfinite(float64_t fValue) {
		const union {
			float64_t f;
			int64_t i;
		} converter = { fValue };

		// An IEEE real value is infinite if all exponent bits are one
		// and the mantissa is zero.
		return (converter.i & ~kFloat64SignMask) == kFloat64PositiveInfinityBits;
	}

	inline bool IsIndefinite(float32_t fValue) {
		const union {
			float32_t f;
			int32_t i;
		} converter = { fValue };

		return converter.i == (int32_t)UINT32_C(0xFFC00000);
	}

	inline bool IsIndefinite(float64_t fValue) {
		const union {
			float64_t f;
			int64_t i;
		} converter = { fValue };

		return converter.i == (int64_t)UINT64_C(0xFFF8000000000000);
	}

	inline bool IsDenormalized(float32_t fValue) {
		const union {
			float32_t f;
			int32_t i;
		} converter = { fValue };

		// An IEEE real value is denormalized if its exponent is zero and the
		// mantissa is non-zero.
		return (uint32_t)((converter.i & ~kFloat32SignMask) - 1) < kFloat32MantissaMask;
	}

	inline bool IsDenormalized(float64_t fValue) {
		const union {
			float64_t f;
			int64_t i;
		} converter = { fValue };

		// An IEEE real value is denormalized if its exponent is zero and the
		// mantissa is non-zero.
		return (uint64_t)((converter.i & ~kFloat64SignMask) - 1) < kFloat64MantissaMask;
	}

} // namespace StdC
} // namespace EA



#if defined(EA_PLATFORM_MICROSOFT)
	#include <EAStdC/Win32/EAMathHelpWin32.inl>
#else
	#define EAMATHHELP_MODE_REFERENCE 1 // Use the reference implementation, which has no optimizations.
#endif

#if defined(EAMATHHELP_MODE_REFERENCE) && EAMATHHELP_MODE_REFERENCE
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <math.h>
	EA_RESTORE_ALL_VC_WARNINGS()

	namespace EA
	{
	namespace StdC
	{
		EA_NO_UBSAN inline uint32_t RoundToUint32(float32_t fValue) {
			return (uint32_t)floorf(fValue + 0.5f);
		}

		inline int32_t RoundToInt32(float32_t fValue) {
			return (int32_t)floorf(fValue + 0.5f);
		}

		inline int32_t FloorToInt32(float32_t fValue) {
			return (int32_t)floorf(fValue);
		}

		inline int32_t CeilToInt32(float32_t fValue) {
			return (int32_t)ceilf(fValue);
		}

		inline int32_t TruncateToInt32(float32_t fValue) {
			return (int32_t)fValue;
		}

		EA_NO_UBSAN inline uint8_t UnitFloatToUint8(float fValue) {
			return (uint8_t)floorf((fValue * 255.0f) + 0.5f);
		}

		inline uint8_t ClampUnitFloatToUint8(float fValue) {
			if (fValue < 0.0f)
				fValue = 0.0f;
			else if (fValue > 1.0f)
				fValue = 1.0f;

			return (uint8_t)floorf((fValue * 255.0f) + 0.5f);
		}

		// This function is deprecated, as it's not very useful any more.
		inline int32_t FastRoundToInt23(float32_t fValue) {
			return (int32_t)floorf(fValue + 0.5f);
		}

	} // namespace StdC
	} // namespace EA


#endif // EAMATHHELP_MODE_REFERENCE


#endif // HEader include guard




