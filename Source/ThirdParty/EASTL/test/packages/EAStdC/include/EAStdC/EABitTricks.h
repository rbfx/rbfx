///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This is a list of C/C++ bit manipulation tricks. For example, it is 
// well-known that (x * 2) can be also accomplished with (x << 1). 
// And while this example may not be useful in practice, there are many 
// more such tricks which have real use, particularly for speeding up code.
// 
// Notes:
//     * Twos-complement integer storage is assumed. Nearly all modern 
//       processors use twos-complement storage.
//     * Some tricks assume that right shifts of signed values preserve 
//       the sign. While nearly all modern processors and C/C++ compilers 
//       support this, some primitive processors don't. By preserving sign, 
//       we mean that signed binary (10000000 >> 1) gives (11000000).
//     * Only 'tricky' efficient solutions are provided. Obvious brute force loops 
//       to do useful things aren't included. We attempt to use branchless and  
//       loopless logic where possible.
//     * We don't cover magic number tricks for simplifying multiplication and 
//       division by constants. For example (x * 17) can also be quickly accomplished 
//       with ((x << 4) + x). Optimized integer multiplication and division tricks 
//       such as this is something for a separate library.
//     * We don't cover floating point tricks. That is something for a separate library. 
//     * Implementations here are written as standalone functions for readability. 
//       However, the user may find that it's better in some cases to copy the implementation 
//       to a macro or to simply copy the implementation directly inline into source 
//       code. EABitTricks is meant to be a reference for copy and paste as much as 
//       it is meant to be used as-is.
//     * Many of these functions are templated instead of taking a given integer 
//       type such as uint32_t. The reason for this is that we want 64 bit support 
//       and that can be had automatically in most cases by the use of templates. The 
//       generated code will be exactly as fast as the case when templates are not used.
// 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EABITTRICKS_H
#define EASTDC_EABITTRICKS_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
#include <limits.h>



#if defined(EA_COMPILER_MSVC) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
	// We can use assembly intrinsics for some of these functions.
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <math.h>       // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#include <intrin.h>
	EA_RESTORE_ALL_VC_WARNINGS()
	#pragma intrinsic(_BitScanForward)
	#pragma intrinsic(_BitScanReverse)
	#if defined(EA_PROCESSOR_X86_64)
		#pragma intrinsic(_BitScanForward64)
		#pragma intrinsic(_BitScanReverse64)
	#endif
#elif (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
    #include <x86intrin.h>
#endif

EA_DISABLE_VC_WARNING(6326 4146) // C6326: Potential comparison of a constant with another constant
								 // C4146: unary minus operator applied to unsigned type, result still unsigned



namespace EA
{
namespace StdC
{
	namespace helper
	{
		///////////////////////////////////////////////////////////////////////
		// is_signed
		///////////////////////////////////////////////////////////////////////

		struct false_type { static const bool value = false; };
		struct true_type  { static const bool value = true;  };

		template <typename T> struct is_signed : public false_type{};

		template <> struct is_signed<signed char>              : public true_type{};
		template <> struct is_signed<const signed char>        : public true_type{};
		template <> struct is_signed<signed short>             : public true_type{};
		template <> struct is_signed<const signed short>       : public true_type{};
		template <> struct is_signed<signed int>               : public true_type{};
		template <> struct is_signed<const signed int>         : public true_type{};
		template <> struct is_signed<signed long>              : public true_type{};
		template <> struct is_signed<const signed long>        : public true_type{};
		template <> struct is_signed<signed long long>         : public true_type{};
		template <> struct is_signed<const signed long long>   : public true_type{};

		#if (CHAR_MAX == SCHAR_MAX)
			template <> struct is_signed<char>            : public true_type{};
			template <> struct is_signed<const char>      : public true_type{};
		#endif
		#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
			#if defined(__WCHAR_MAX__) && ((__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 32767)) // GCC defines __WCHAR_MAX__ for most platforms.
				template <> struct is_signed<wchar_t>         : public true_type{};
				template <> struct is_signed<const wchar_t>   : public true_type{};
			#endif
		#endif
	
		#define bt_is_signed helper::is_signed<T>::value
		

		//////////////////////////////////////////////
		// add_signed
		//////////////////////////////////////////////
		
		template<typename T>
		struct add_signed
		{ typedef T type; };

		template<>
		struct add_signed<unsigned char>
		{ typedef signed char type; };

		#if (defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)) // If char is unsigned (which is usually not the case)...
			template<>
			struct add_signed<char>
			{ typedef signed char type; };
		#endif

		template<>
		struct add_signed<unsigned short>
		{ typedef short type; };

		template<>
		struct add_signed<unsigned int>
		{ typedef int type; };

		template<>
		struct add_signed<unsigned long>
		{ typedef long type; };

		template<>
		struct add_signed<unsigned long long>
		{ typedef long long type; };

		#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
			#if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 4294967295U)) // If wchar_t is a 32 bit unsigned value...
				template<>
				struct add_signed<wchar_t>
				{ typedef int32_t type; };
			#elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 65535)) // If wchar_t is a 16 bit unsigned value...
				template<>
				struct add_signed<wchar_t>
				{ typedef int16_t type; };
			#endif
		#endif

		#define bt_signed typename helper::add_signed<T>::type // Must be used as part of a cast, as in (bt_signed) or static_cast<bt_signed>()
	}
	

	////////////////////////////////////////////////////////////////////////////
	// Bit manipulation
	////////////////////////////////////////////////////////////////////////////

	/// TurnOffLowestBit
	/// How to turn off the lowest 1 bit in an integer.
	/// Returns 0 for an input of 0.
	/// Works with signed and unsigned integers.
	/// Example:
	///     01011000 -> 01010000
	template <typename T>
	inline T TurnOffLowestBit(T x){
		return (T)(x & (x - 1));
	}

	/// IsolateLowestBit
	/// How to isolate the lowest 1 bit.
	/// Returns 0 for an input of 0.
	/// Example:
	///     01011000 -> 00001000
	template <typename T>
	inline T IsolateLowestBit(T x){
		return (T)(x & (0 - x));
	}

	/// IsolateLowest0Bit
	/// How to isolate the lowest 0 bit.
	/// Returns 0 for an input of all bits set.
	/// Example:
	///     10100111 -> 00001000
	template <typename T>
	inline T IsolateLowest0Bit(T x){
		return (T)(~x & (x + 1));
	}

	/// GetTrailing0Bits
	/// How to produce a mask of all low zeroes.
	/// Returns 0 for an input of all bits set.
	/// Example:
	///     01011000 -> 00000111
	template <typename T>
	inline T GetTrailing0Bits(T x){
		return (T)(~x & (x - 1));
	}

	/// GetTrailing1And0Bits
	/// How to produce a mask of lowest 1 bit and all lower zeroes.
	/// Returns all bits set for an input of 0.
	/// Returns 1 for an input of all bits set.
	/// Example:
	///     01011000 -> 00001111
	template <typename T>
	inline T GetTrailing1And0Bits(T x){
		return (T)(x ^ (x - 1));
	}

	/// PropogateLowestBitDownward
	/// How to propogate the lowest 1 bit downward.
	/// Returns all bits set for an input of 0.
	/// Example:
	///     01011000 -> 01011111
	template <typename T>
	inline T PropogateLowestBitDownward(T x){
		return (T)(x | (x - 1));
	}

	/// TurnOffLowestContiguousBits
	/// How to turn off the lowest contiguous string of 1 bits.
	/// Returns 0 for an input of 0.
	/// Returns 0 for an input of all bits set.
	/// Example:
	///     01011000 -> 01000000
	template <typename T>
	inline T TurnOffLowestContiguousBits(T x){
		return (T)(((x | (x - 1)) + 1) & x);
	}

	/// TurnOnLowest0Bit
	/// How to turn off the lowest 0 bit in an integer.
	/// Returns all bits set for an input of all bits set.
	/// Example:
	///     10100111 -> 10101111
	template <typename T>
	inline T TurnOnLowest0Bit(T x){
		return (T)(x | (x + 1));
	}

	/// GetNextWithEqualBitCount
	/// How to get the next higher integer with the same number of bits set.
	/// This function is supposed (claim by original author) to be able to 
	/// wrap around and continue properly, but testing indicates otherwise.
	/// Do not call this with x = 0; as that causes a divide by 0.
	/// Doesn't work for an input of all bits set.
	/// Do not assign the result of this to a type of less size than the input argument.
	/// This function has certain real-world applications.
	/// This function has been called 'snoob' by some. 
	/// Example:
	///     01010110 -> 01011001
	template <typename T>
	inline T GetNextWithEqualBitCount(T x){
		T smallest, ripple, ones;

		smallest = x & -(bt_signed)x;
		ripple   = x + smallest;
		ones     = x ^ ripple;
		ones     = (ones >> 2) / smallest;
		return ripple | ones;
	}


	/// IsolateSingleBits
	/// How to isolate single bits in an integer.
	/// Example:
	///        10101011 -> 10101000
	template <typename T>
	inline T IsolateSingleBits(T x){
		return x & ~((x << 1) | (x >> 1));
	}

	template <typename T>
	inline T IsolateSingle0Bits(T x){
		return IsolateSingleBits(~x);
	}

	template <typename T>
	inline T IsolateSingle0And1Bits(T x){
		return (x ^ (x << 1)) & (x ^ (x >> 1));
	}

	/// ShiftRightSigned
	/// How to do a signed right shift portably.
	/// Some platform/compiler combinations don't support sign extension 
	/// with right shifts of signed values. The C language standard doesn't 
	/// guarantee such functionality. Most advanced CPUs and nearly all C 
	/// compilers support this. Weak embedded processors may possibly not.
	/// Example:
	///     10000000 - by 2-> 11100000
	///     00000000 - by 2-> 00000000
	inline int32_t ShiftRightSigned(int32_t x, uint32_t n){
		return int32_t((((uint32_t)x + UINT32_C(0x80000000)) >> n) - (UINT32_C(0x80000000) >> n));
	}

	inline int64_t ShiftRightSigned(int64_t x, uint64_t n){
		return int64_t((((uint64_t)x + UINT64_C(0x8000000000000000)) >> n) - (UINT64_C(0x8000000000000000) >> n));
	}

	/// CountTrailing0Bits
	/// How to count the number of trailing zeroes in an unsigned 32 bit integer.
	/// Example:
	///     ...10101000 -> 3
	///     ...11111111 -> 0
	///     ...00000000 -> 32
	inline int CountTrailing0Bits(uint32_t x){
		#if defined(EA_COMPILER_MSVC) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
			// This has been benchmarked as significantly faster than the generic code below.
			unsigned char isNonZero;
			unsigned long index;
			isNonZero = _BitScanForward(&index, x);
			return isNonZero ? (int)index : 32;
		#elif defined(EA_COMPILER_GNUC) && !defined(EA_COMPILER_EDG)
			if(x)
				return __builtin_ctz(x);
			return 32;
		#else
			if(x){
				int n = 1;
				if((x & 0x0000FFFF) == 0) {n += 16; x >>= 16;}
				if((x & 0x000000FF) == 0) {n +=  8; x >>=  8;}
				if((x & 0x0000000F) == 0) {n +=  4; x >>=  4;}
				if((x & 0x00000003) == 0) {n +=  2; x >>=  2;}
				return n - int(x & 1);
			}
			return 32;
		#endif
	}

	/// CountTrailing0Bits
	/// How to count the number of trailing zeroes in an unsigned 64 bit integer.
	/// Example:
	///     ...10101000 -> 3
	///     ...11111111 -> 0
	///     ...00000000 -> 64
	inline int CountTrailing0Bits(uint64_t x){
		#if defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86_64)
			// This has been benchmarked as significantly faster than the generic code below.
			unsigned char isNonZero;
			unsigned long index;
			isNonZero = _BitScanForward64(&index, x);
			return isNonZero ? (int)index : 64;
		#elif defined(EA_COMPILER_GNUC) && !defined(EA_COMPILER_EDG)
			if(x)
				return __builtin_ctzll(x);
			return 64;
		#else
			if(x){
				int n = 1;
				if((x & 0xFFFFFFFF) == 0) { n += 32; x >>= 32; }
				if((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
				if((x & 0x000000FF) == 0) { n +=  8; x >>=  8; }
				if((x & 0x0000000F) == 0) { n +=  4; x >>=  4; }
				if((x & 0x00000003) == 0) { n +=  2; x >>=  2; }
				return n - (int)(unsigned)(x & 1);
			}
			return 64;
		#endif
	}

	/// CountLeading0Bits
	/// How to count the number of leading zeroes in an unsigned integer.
	/// Example:
	///   ..00000000 -> 32
	///     00110111 ->  2
	///     11111111 ->  0
	///
	inline int CountLeading0Bits(uint32_t x){
		#if defined(EA_COMPILER_MSVC) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
			// This has been benchmarked as significantly faster than the generic code below.
			unsigned char isNonZero;
			unsigned long index;
			isNonZero = _BitScanReverse(&index, x);
			return isNonZero ? (31 - (int)index) : 32;
		#elif defined(EA_COMPILER_GNUC) && !defined(EA_COMPILER_EDG)
			if(x)
				return __builtin_clz(x);
			return 32;
		#else
			/// This implementation isn't highly compact, but would likely be 
			/// faster than a brute force solution.
			/// x86 function which is claimed to be faster:
			///    double ff = (double)(v|1);
			///    return ((*(1+(unsigned long *)&ff))>>20)-1023;
			if(x){
				int n = 0;
				if(x <= 0x0000FFFF) { n += 16; x <<= 16; }
				if(x <= 0x00FFFFFF) { n +=  8; x <<=  8; }
				if(x <= 0x0FFFFFFF) { n +=  4; x <<=  4; }
				if(x <= 0x3FFFFFFF) { n +=  2; x <<=  2; }
				if(x <= 0x7FFFFFFF) { n +=  1;           }
				return n;
			}
			return 32;
		#endif
	}

	/// CountLeading0Bits
	/// How to count the number of leading zeroes in an unsigned integer.
	/// Example:
	///   ..00000000 -> 64
	///   ..00110111 ->  2
	///   ..11111111 ->  0
	///
	inline int CountLeading0Bits(uint64_t x){
		#if defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86_64)
			// This has been benchmarked as significantly faster than the generic code below.
			unsigned char isNonZero;
			unsigned long index;
			isNonZero = _BitScanReverse64(&index, x);
			return isNonZero ? (63 - (int)index) : 64;
		#elif defined(EA_COMPILER_GNUC) && !defined(EA_COMPILER_EDG)
			if(x)
				return __builtin_clzll(x);
			return 64;
		#else
			if(x){ // We use a slightly different algorithm than the 32 bit version above because this version avoids slow large 64 bit constants, especially on RISC processors.
				int n = 0;
				if(x & UINT64_C(0xFFFFFFFF00000000)) { n += 32; x >>= 32; }
				if(x & 0xFFFF0000)                   { n += 16; x >>= 16; }
				if(x & 0xFFFFFF00)                   { n +=  8; x >>=  8; }
				if(x & 0xFFFFFFF0)                   { n +=  4; x >>=  4; }
				if(x & 0xFFFFFFFC)                   { n +=  2; x >>=  2; }
				if(x & 0xFFFFFFFE)                   { n +=  1;           }
				return 63 - n;
			}
			return 64;
		#endif
	}


	/// CountBits
	/// How to count the number of bits in an unsigned integer.
	/// There are multiple variations of this function available, 
	/// each tuned to the expected bit counts and locations. 
	/// The version presented here has a large number of logical 
	/// operations but has no looping or branching.
	/// This implementation is taken from the AMD x86 optimization guide.
	/// Example:
	///     11001010 -> 4
	inline int CountBits(uint32_t x) // Branchless version
	{ 
	#if defined(EASTDC_SSE_POPCNT)
		return _mm_popcnt_u32(x);
	#else
		x = x - ((x >> 1) & 0x55555555);
		x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
		x = (x + (x >> 4)) & 0x0F0F0F0F;
		return (int)((x * 0x01010101) >> 24);
	#endif
	}

	inline int CountBits64(uint64_t x) // Branchless version
	{ 
	#if defined(EASTDC_SSE_POPCNT) && defined(EA_PROCESSOR_X86_64)
		return (int)_mm_popcnt_u64(x);
	#else
		#if (EA_PLATFORM_WORD_SIZE < 8)
			return CountBits((uint32_t)(x >> 32)) + CountBits((uint32_t)x);
		#else
			x = x - ((x >> 1) & UINT64_C(0x5555555555555555));
			x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2) & UINT64_C(0x3333333333333333));
			x = (x + (x >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
			return (int)((x * UINT64_C(0x0101010101010101)) >> 56);
		#endif
	#endif
	}

	/// RotateLeft
	/// How to rotate an integer left.
	/// Bit rotations can often be accomplished with inline assembly 
	/// that uses the processor's native bit rotation capabilities.
	/// Example:
	///     11110000 -> left 2 -> 11000011
	inline uint32_t RotateLeft(uint32_t x, uint32_t n){
		return (uint32_t)((x << n) | (x >> (32 - n)));
	}

	inline uint64_t RotateLeft(uint64_t x, uint64_t n){
		return (uint64_t)((x << n) | (x >> (64 - n)));
	}

	inline uint32_t RotateRight(uint32_t x, uint32_t n){
		return (uint32_t)((x >> n) | (x << (32 - n)));
	}

	inline uint64_t RotateRight(uint64_t x, uint64_t n){
		return (uint64_t)((x >> n) | (x << (64 - n)));
	}


	/// ReverseBits
	/// How to reverse the bits in an integer.
	/// There are other mechansims for accomplishing this. 
	/// The version presented here has no branches, looping, nor table lookups.
	/// Table lookups are the fastest when done on large amounts of data, but slower
	/// when just done once due to the initial lookup cost.
	/// Some ARM results: http://corner.squareup.com/2013/07/reversing-bits-on-arm.html
	/// http://stackoverflow.com/questions/746171/best-algorithm-for-bit-reversal-from-msb-lsb-to-lsb-msb-in-c
	/// Example:
	///    11100001 -> 10000111
	inline uint32_t ReverseBits(uint32_t x){
		x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);
		x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);
		x = ((x & 0x0F0F0F0F) << 4) | ((x >> 4) & 0x0F0F0F0F);
		x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
		return x;
	}

	// This is surely not the fastest way to do this, and is here for completeness only.
	// To do: Make a better version of this.
	inline uint64_t ReverseBits(uint64_t x){
		uint32_t x1 = (uint32_t)(x & 0xffffffff);
		uint32_t x2 = (uint32_t)(x >> 32);
		return ((uint64_t)ReverseBits(x1) << 32) | ReverseBits(x2);
	}


	/// IsolateHighestBit
	/// How to isolate the highest 1 bit.
	/// Returns 0 for an input of 0.
	/// Example:
	///    01000100 -> 01000000
	///    00000000 -> 00000000
	inline uint32_t IsolateHighestBit(uint32_t x){
		x |= x >> 1; 
		x |= x >> 2; 
		x |= x >> 4; 
		x |= x >> 8; 
		x |= x >> 16;
		return x ^ (x >> 1);
	}

	inline uint64_t IsolateHighestBit(uint64_t x){
		x |= x >> 1; 
		x |= x >> 2; 
		x |= x >> 4; 
		x |= x >> 8; 
		x |= x >> 16;
		x |= x >> 32;
		return x ^ (x >> 1);
	}

	inline uint32_t IsolateHighest0Bit(uint32_t x){
		return IsolateHighestBit(~x);
	}

	inline uint64_t IsolateHighest0Bit(uint64_t x){
		return IsolateHighestBit(~x);
	}

	/// PropogateHighestBitDownward
	/// How to set all bits from the highest 1 bit downward.
	/// Returns 0 for an input of 0.
	/// Example:
	///    01001000 -> 01111111
	///    00000000 -> 00000000
	inline uint32_t PropogateHighestBitDownward(uint32_t x){
		x |= (x >>  1);
		x |= (x >>  2);
		x |= (x >>  4);
		x |= (x >>  8);
		x |= (x >> 16);
		return  x;
	}

	inline uint64_t PropogateHighestBitDownward(uint64_t x){
		x |= (x >>  1);
		x |= (x >>  2);
		x |= (x >>  4);
		x |= (x >>  8);
		x |= (x >> 16);
		x |= (x >> 32);
		return  x;
	}

	/// GetHighestContiguous0Bits
	/// How to set the highest contiguous 0 bits.
	/// Returns 0 for an input of all bits set.
	/// Example:
	///    00011001 -> 11100000
	///    11111111 -> 00000000
	inline uint32_t GetHighestContiguous0Bits(uint32_t x){
		x |= (x >>  1);
		x |= (x >>  2);
		x |= (x >>  4);
		x |= (x >>  8);
		x |= (x >> 16);
		return  ~x;
	}

	inline uint64_t GetHighestContiguous0Bits(uint64_t x){
		x |= (x >>  1);
		x |= (x >>  2);
		x |= (x >>  4);
		x |= (x >>  8);
		x |= (x >> 16);
		x |= (x >> 32);
		return  ~x;
	}

	/// GetBitwiseEquivalence
	/// How to calculate bitwise equivalence.
	/// Bitwise equivalence is the opposite of xor. It sets all bits that 
	/// are the same to 1, whereas xor sets all bits that are different to 1. 
	/// Thus, you can simply use ~ to get bitwise equivalence from xor.
	/// Example:
	///    11001100, 11110000 -> 11000011
	template <typename T>
	inline T GetBitwiseEquivalence(T x, T y){
		return (T)(~(x ^ y));
	}

	/// AreLessThan2BitsSet
	/// How to tell if at least two bits are set in an integer.
	/// Example:
	///    00001000 -> true
	///    01001110 -> false
	template <typename T>
	inline bool AreLessThan2BitsSet(T x){
		return (x & (x - 1)) == 0;
	}

#if defined(EASTDC_SSE_POPCNT)
	template <>
	inline bool AreLessThan2BitsSet<uint32_t>(uint32_t x){
		uint32_t rv = (uint32_t)_mm_popcnt_u32(x);
		return (rv < 2);
	}

#if defined(EA_PROCESSOR_X86_64)
	template <>
	inline bool AreLessThan2BitsSet<uint64_t>(uint64_t x){
		uint64_t rv = (uint64_t)_mm_popcnt_u64(x);
		return (rv < 2);
	}
#endif
#endif


	/// GetHighestBit
	/// How to refer to the high bit of any integer data type where the 
	/// data type size is not known.
	/// There are cases where you may want to refer to the higest bit in 
	/// an integer in a portable way. If you can't know the size of the 
	/// destination platform's int type, you can use this to refer to such a bit.
	/// Example:
	///    GetHighestBit(uint32_t(0)) -> 0x80000000
	///    GetHighestBit(uint16_t(0)) ->     0x8000
	template <typename T>
	inline T GetHighestBit(T /*t*/){
		return (T)((T)1 << (T)((sizeof(T) * 8) - 1));
	}



	////////////////////////////////////////////////////////////////////////////
	// Alignment / Power of 2
	////////////////////////////////////////////////////////////////////////////

	/// IsPowerOf2
	/// How to tell if an unsigned integer is a power of 2.
	/// Works with unsigned integers only.
	/// Returns true for x == 0.
	/// Example:
	///    01000010 (66) -> false
	///    00001100 (12) -> false
	///    00000100  (4) -> true
	///    00000000  (0) -> true
	template <typename T>
	inline bool IsPowerOf2(T x){
		return (x & (x - 1)) == 0;
	}

#if defined(EASTDC_SSE_POPCNT)
	template <>
	inline bool IsPowerOf2<uint32_t>(uint32_t x){
		uint32_t rv = (uint32_t)_mm_popcnt_u32(x);
		return (rv < 2);
	}

#if defined(EA_PROCESSOR_X86_64)
	template <>
	inline bool IsPowerOf2<uint64_t>(uint64_t x){
		uint64_t rv = (uint64_t)_mm_popcnt_u64(x);
		return (rv < 2);
	}
#endif
#endif


	/// RoundUpToPowerOf2
	/// Rounds an integer up to the nearest power of 2.
	/// Returns 16 for x == 16, 32 for x == 32, etc.
	/// Returns 0 for x == 0.
	/// Example:
	///    01000010 (66) -> 10000000 (128)
	///    00001100 (12) -> 00010000 (16)
	///    00000100  (4) -> 00000100 (4)
	///    00000000  (0) -> 00000000 (0)
	inline uint32_t RoundUpToPowerOf2(uint32_t x){
		--x;
		x |= (x >> 16);
		x |= (x >>  8);
		x |= (x >>  4);
		x |= (x >>  2);
		x |= (x >>  1);
		return ++x;
	}

	inline uint64_t RoundUpToPowerOf2(uint64_t x){
		--x;
		x |= (x >> 32);
		x |= (x >> 16);
		x |= (x >>  8);
		x |= (x >>  4);
		x |= (x >>  2);
		x |= (x >>  1);
		return ++x;
	}


	/// IsPowerOf2Multiple
	/// How to tell if an unsigned integer is a multiple of some specific power of 2.
	/// Template constant n is required to be a power of 2.
	/// Works with an input unsigned integers only. 'k' must be an even power of 2, such as 1, 2, 4, 8, etc.
	/// Returns true for x == 0.
	/// Example:
	///     IsPowerOf2Multiple<4>(3)   -> false
	///     IsPowerOf2Multiple<16>(32) -> true
	///
	template <typename T, int n>
	inline bool IsPowerOf2Multiple(T x){
		return (x & (n - 1)) == 0;
	}
	// *** The following is deprecated, as it doesn't use "power of 2" in its name ***
	template <typename T, int n>
	inline bool IsMultipleOf(T x){
		return (x & (n - 1)) == 0;
	}


	/// IsPowerOf2Minus1
	/// How to tell if an unsigned integer is of the form 2n-1 (e.g. 0x0fff, 0x000fffff).
	/// Returns true for an input of 0.
	/// Example:
	///    00001111 (15) -> true
	///    00001011 (11) -> false
	///    00000000  (0) -> true
	template <typename T>
	inline bool IsPowerOf2Minus1(T x){
		return (x & (x + 1)) == 0;
	}

	/// CrossesPowerOf2
	/// How to detect any power of two crossing between two integers.
	/// Useful for telling if an increasing value is about to go over a power of two boundary.
	/// Example:
	///    4, 5, 8 -> false
	///    5, 9, 8 -> true
	template <typename T>
	inline bool CrossesPowerOf2(T x, T y, T n){
		return (n - (x & (n - 1))) < (y - x);
	}

	/// CrossesPowerOf2
	/// How to detect a specific power of two crossing between two integers.
	/// Template parameter n must be an even power of 2, such as 1, 2, 4, 8, 16, etc.
	/// Example:
	///    CrossesPowerOf2<8>(3, 5)  -> false
	///    CrossesPowerOf2<8>(7, 30) -> true
	template <typename T, int n>
	inline bool CrossesPowerOf2(T x, T y){
		return (n - (x & (n - 1))) < (y - x);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// GetHighestBitPowerOf2 template
	//
	// There is a templated compile-time constant version of GetHighestBitPowerOf2
	// but it is called Log2Uint32 / Log2Int32 / Log2Uint64 / Log2Int64. See below
	// for that template definition.
	/////////////////////////////////////////////////////////////////////////////////


	/// GetHighestBitPowerOf2 (a.k.a. GetHighestBitIndex)
	/// How to detect the power of 2 of the highest bit set in a uint32_t.
	/// The power of 2 is the same as the bit index, so this could also be 
	/// called GetHighestBitIndex.
	/// Returns 0 for an input of 0.
	/// Returns a value in the range of [0, 31]
	/// Example:
	///    GetHighestBitPowerOf2(0)  -> 0
	///    GetHighestBitPowerOf2(1)  -> 0
	///    GetHighestBitPowerOf2(2)  -> 1
	///    GetHighestBitPowerOf2(3)  -> 1
	///    GetHighestBitPowerOf2(4)  -> 2
	///    GetHighestBitPowerOf2(7)  -> 2
	///    GetHighestBitPowerOf2(8)  -> 3
	inline uint32_t GetHighestBitPowerOf2(uint32_t x){
		uint32_t r = 0;

		if(x & 0xffff0000){
			r  += 16;
			x >>= 16;
		}
		if(x & 0xff00){
			r  += 8;
			x >>= 8;
		}
		if(x & 0xf0){
			r  += 4;
			x >>= 4;
		}
		if(x & 0x0c){
			r  += 2;
			x >>= 2;
		}
		if(x & 0x02){
			r  += 1;
		}

		return r;
	}


	/// GetHighestBitPowerOf2 (a.k.a. GetHighestBitIndex)
	/// How to detect the power of 2 of the highest bit set in a uint64_t.
	/// The power of 2 is the same as the bit index, so this could also be 
	/// called GetHighestBitIndex.
	/// Returns a value in the range of [0, 63]
	/// Returns 0 for an input of 0.
	/// Example:
	///    GetHighestBitPowerOf2(0)  -> 0
	///    GetHighestBitPowerOf2(1)  -> 0
	///    GetHighestBitPowerOf2(2)  -> 1
	///    GetHighestBitPowerOf2(3)  -> 1
	///    GetHighestBitPowerOf2(4)  -> 2
	///    GetHighestBitPowerOf2(7)  -> 2
	///    GetHighestBitPowerOf2(8)  -> 3
	inline uint32_t GetHighestBitPowerOf2(uint64_t x){
		uint32_t r = 0;

		if(x & UINT64_C(0xffffffff00000000)){
			r  += 32;
			x >>= 32;
		}

		return GetHighestBitPowerOf2((uint32_t)x) + r;
	}


	/// GetNextGreaterEven
	/// Returns the next higher even integer. 
	/// Returns an even number, not necessarily a power of 2.
	/// Example:
	///    GetNextGreaterEven<int>(-3)  -> -2
	///    GetNextGreaterEven<int>(-2)  ->  0
	///    GetNextGreaterEven<int>(-1)  ->  0
	///    GetNextGreaterEven<int>( 0)  ->  2
	///    GetNextGreaterEven<int>( 1)  ->  2
	///    GetNextGreaterEven<int>( 2)  ->  4
	///    GetNextGreaterEven<int>( 3)  ->  4
	template <typename T>
	inline T GetNextGreaterEven(T x)
	{
		return (x + 2) & -2;
	}

	/// GetNextGreaterOdd
	/// Returns the next higher odd integer. 
	/// Returns an even number, not necessarily a power of 2.
	/// Example:
	///    GetNextGreaterOdd<int>(-3)  -> -1
	///    GetNextGreaterOdd<int>(-2)  -> -1
	///    GetNextGreaterOdd<int>(-1)  ->  1
	///    GetNextGreaterOdd<int>( 0)  ->  1
	///    GetNextGreaterOdd<int>( 1)  ->  3
	///    GetNextGreaterOdd<int>( 2)  ->  3
	///    GetNextGreaterOdd<int>( 3)  ->  5
	template <typename T>
	inline T GetNextGreaterOdd(T x)
	{
		return ((x + 1) & -2) + 1;
	}

	/// RoundUpTo
	/// Template constant n is required to be a power of 2.
	/// Rounds values towards positive infinity.
	/// Returns 0 for an input of 0.  
	/// Example:
	///    RoundUpTo<int, 4>(3)  ->  4
	///    RoundUpTo<int, 4>(8)  ->  8
	///    RoundUpTo<int, 4>(0)  ->  0
	///    RoundUpTo<int, 4>(-7) -> -4
	template <typename T, int n>
	inline T RoundUpTo(T x){
		return (T)((x + (n - 1)) & (T)-n);
	}


	/// RoundUpToEx
	/// Template constant n is required to be a power of 2.
	/// Rounds values away from zero.
	/// Returns 0 for an input of 0.
	/// Example:
	///    RoundUpToEx<int, 4>(3)  ->  4
	///
	template <bool is_signed>
	struct RoundUpToExStruct
	{
		template <typename T, int n>
		static T RoundUpToEx(T x){
			if(x >= 0)
				return (T)((x + (n - 1)) & (T)-n);
			return (T)-(bt_signed)((-(bt_signed)x + (n - 1)) & (T)-n);
		}
	};

	template <>
	struct RoundUpToExStruct<false>
	{
		template <typename T, int n>
		static T RoundUpToEx(T x){
			return (T)((x + (n - 1)) & (T)-n);
		}
	};

	template <typename T, int n>
	inline T RoundUpToEx(T x){
		return RoundUpToExStruct<bt_is_signed>::template RoundUpToEx<T, n>(x);
	}
	

	/// RoundDownTo
	/// Template constant n is required to be a power of 2.
	/// Returns 0 for an input of 0.
	/// Rounds values towards negative infinity.
	/// Example:
	///    RoundDownTo<int, 4>(5)  ->  4
	///    RoundDownTo<int, 4>(4)  ->  4
	///    RoundDownTo<int, 4>(0)  ->  0
	///    RoundDownTo<int, 4>(-7) -> -8
	template <typename T, int n>
	inline T RoundDownTo(T x){
		return (T)(x & ~(n - 1));
	}

	/// RoundDownToEx
	/// Template constant n is required to be a power of 2.
	/// Returns 0 for an input of 0.
	/// Rounds values towards zero.
	/// Example:
	///    RoundDownTo<int, 4>(5)  ->  4
	///    RoundDownTo<int, 4>(4)  ->  4
	///    RoundDownTo<int, 4>(0)  ->  0
	///    RoundDownTo<int, 4>(-7) -> -4
	///
	template <bool is_signed>
	struct RoundDownToExStruct
	{
		template <typename T, int n>
		static T RoundDownToEx(T x){
			if(x >= 0)
				return (T)(x & ~(n - 1));
			return (T)-(bt_signed)(-(bt_signed)x & ~(n - 1));
		}
	};

	template <>
	struct RoundDownToExStruct<false>
	{
		template <typename T, int n>
		static T RoundDownToEx(T x){
			return (T)(x & ~(n - 1));
		}
	};

	template <typename T, int n>
	inline T RoundDownToEx(T x){
		return RoundDownToExStruct<bt_is_signed>::template RoundDownToEx<T, n>(x);
	}



	/// RoundUpToMultiple
	/// This is a non-power of 2 function; n may be any value > zero.
	/// Rounds values towards infinity.
	/// Example:
	///    RoundUpToMultiple<int, 6>(37)  ->  42
	///    RoundUpToMultiple<int, 6>(42)  ->  42
	template <typename T, int n>
	inline T RoundUpToMultiple(T x){
		return (T)(((x + (n - 1)) / n) * n);
	}

	/// RoundDownToMultiple
	/// This is a non-power of 2 function; n may be any value > zero.
	/// Rounds values towards zero.
	/// Example:
	///    RoundDownToMultiple<int, 6>(37)  ->  36
	///    RoundDownToMultiple<int, 6>(36)  ->  36
	template <typename T, int n>
	inline T RoundDownToMultiple(T x){
		return (T)((x / n) * n);
	}


	/// ZeroPresent8
	/// Returns true if a 0 byte is present in x.
	/// Example:
	///    ZeroPresent8(0xffffffff)  ->  false
	///    ZeroPresent8(0xffff00ff)  ->  true
	inline bool ZeroPresent8(uint32_t x)
	{
		return ((((x - 0x01010101) & ~x) & 0x80808080) != 0); // Works because 0 is the only value for which subtracting 1 results in its high bit going from 0 to 1.
	}

	inline bool ZeroPresent8(uint64_t x)
	{
		return ((((x - UINT64_C(0x0101010101010101)) & ~x) & UINT64_C(0x8080808080808080)) != 0);
	}


	/// ZeroPresent16
	/// Returns true if an aligned uint16_t of 0 is present in x.
	/// Example:
	///    ZeroPresent16(0xffffffff)  ->  false
	///    ZeroPresent16(0xff0000ff)  ->  false   (There are 16 contiguous 0 bits, but they are not uint16_t aligned)
	///    ZeroPresent16(0x0000ffff)  ->  true
	inline bool ZeroPresent16(uint32_t x)
	{
		return ((((x - 0x00010001) & ~x) & 0x80008000) != 0);
	}

	inline bool ZeroPresent16(uint64_t x)
	{
		return ((((x - UINT64_C(0x0001000100010001)) & ~x) & UINT64_C(0x8000800080008000)) != 0);
	}


	/// ZeroPresent32
	/// Returns true if an aligned uint32_t of 0 is present in x.
	/// Example:
	///    ZeroPresent16(0xffffffffffffffff)  ->  false
	///    ZeroPresent16(0xffffff00000000ff)  ->  false   (There are 32 contiguous 0 bits, but they are not uint32_t aligned)
	///    ZeroPresent16(0xffffffff00000000)  ->  true
	inline bool ZeroPresent32(uint64_t x)
	{
		return ((((x - UINT64_C(0x0000000100000001)) & ~x) & UINT64_C(0x8000000080000000)) != 0);
	  //return (((x & 0xffffffff) == 0) || (x >> 32) == 0); // Alternative implementation which is slower.
	}


	/// Log2
	/// How to find the integer base 2 log of an integer.
	/// It is frequently useful to tell what power of 2 a value corresponds to.
	/// This function rounds the input value down to the nearest power of 
	/// 2 before testing. Thus an input of 17 returns 4. The function can be 
	/// made to round up by doing a standard power of 2 round-up at the 
	/// beginning of the function.
	/// This function doesn't work for an input of 0.
	/// This function only works for unsigned integers.
	/// For example, given "2^y = x" and you want to solve for y, you say:
	///     y = Log2(x);
	/// Assumes 32 bit IEEE float.
	/// Note: This function yields incorrect answers for very large integers.
	/// Example:
	///     4 -> 2
	///     8 -> 3
	///    11 -> 3
	///    16 -> 4
	///     0 -> error
	///     
	inline uint32_t Log2(uint32_t x)
	{
		const union { float f; uint32_t i; } converter = { (float)x };
		return (converter.i >> 23) - 127;
	}

	inline uint64_t Log2(uint64_t x)
	{
		const union { double f; uint64_t i; } converter = { (double)x };
		return ((converter.i >> 52) & 0x7ff) - 1023;
	}


	/// CeilLog2
	/// Generates the ceiling of Log2.
	/// This function exists to deal with errors generated by Log2 under
	/// the case of very large numbers.
	inline uint32_t CeilLog2(uint32_t x){
		const union { float f; uint32_t i; } converter = { (float)x };
		return (converter.i - 0x3F000001) >> 23;
	}

	//inline uint64_t CeilLog2(uint64_t x){
	//    const union { double f; uint64_t i; } converter = { (double)x };
	//    return (converter.i - UINT64_C(0x________________)) >> 52;
	//}


	/// Log2Uint32 / Log2Int32 / Log2Uint64 / Log2Int64
	///
	/// Evaluates to floor(log2(N)) for a compile-time constant value of N, 
	/// with no runtime cost, for any N > 0. This is the equivalent of a 
	/// compile-time version of GetHighestBitPowerOf2, which gets the 
	/// highest bit set in a value.
	///
	/// Example usage of Log2Uint32:
	///   Log2Uint32<8>::value evaluates to 3
	///   Log2Uint32<0x7fffffff>::value evaluates to 30
	///   Log2Uint64<UINT64_C(0x1000000000000000)>::value evaluates to 60
	///
	template <uint32_t N> struct Log2Uint32    { static const uint32_t value = (1 + Log2Uint32<N/2>::value); };
	template <>           struct Log2Uint32<1> { static const uint32_t value =  0; };

	template <int32_t N>  struct Log2Int32     { static const int32_t value = (1 + Log2Int32<N/2>::value); };
	template <>           struct Log2Int32<1>  { static const int32_t value =  0; };

	template <uint64_t N> struct Log2Uint64    { static const uint64_t value = (1 + Log2Uint64<N/2>::value); };
	template <>           struct Log2Uint64<1> { static const uint64_t value =  0; };

	template <int64_t N>  struct Log2Int64     { static const int64_t value = (1 + Log2Int64<N/2>::value); };
	template <>           struct Log2Int64<1>  { static const int64_t value =  0; };


	/// DivideByPowerOf2Rounded
	/// Divides a number x by 2^n, and rounds the result instead of 
	/// chopping the result. This is useful for certain kinds of 
	/// bit manipulation.
	/// Example:
	///     DivideByPowerOf2Rounded<uint32_t, 8>(  0)   -> 0
	///     DivideByPowerOf2Rounded<uint32_t, 8>(127)   -> 0
	///     DivideByPowerOf2Rounded<uint32_t, 8>(128)   -> 1
	///     DivideByPowerOf2Rounded<uint32_t, 8>(256)   -> 1
	///
	// This is not yet tested.
	//template<typename T, int n>
	//inline T DivideByPowerOf2Rounded(T x)
	//{
	//    return ((x + (1 << (n - 1))) >> n);
	//}


	// EAGetAlignment
	// This functionality and other alignment assistance functions have been
	// moved to EAAlignment.h. The best way to portably and reliably detect
	// alignment in both C and C++ is to use a compiler intrinsic or use 
	// some fancy C++ templates. In either case, the solution is outside the
	// realm of bit tricks.



	////////////////////////////////////////////////////////////////////////////
	// Overflow
	////////////////////////////////////////////////////////////////////////////

	EA_DISABLE_VC_WARNING(4310) // cast truncates constant value

	/// SignedAdditionWouldOverflow
	/// How to detect that an addition or subtraction of signed values will overflow.
	/// This is a useful operation to test for when writing robust applications.
	/// Signed integer overflow of addition or subtraction occurs if and only if 
	/// the operands have the same sign and the result of the operation has the 
	/// opposite of this sign.
	/// Some platforms generate exceptions upon an overflow, and the functions 
	/// presented here work by possibly causing overflow. Alternative versions of 
	/// these functions exist which are slightly more complex but can work without 
	/// generating overflows themselves. However, no platform that you 
	/// are likely to be using generates such exceptions.
	/// Example:
	///    SignedAdditionWouldOverflow<int>(2, 3)                   -> false
	///    SignedAdditionWouldOverflow<int>(0x7fffffff, 0x7fffffff) -> true
	///
	/// EA_NO_UBSAN is included because this function is designed to overflow 
	/// and it UBSAN correctly warns here.
	template <typename T>
	EA_NO_UBSAN inline bool SignedAdditionWouldOverflow(T x, T y){
		const T temp = (T)(x + y);
		return (((temp ^ x) & (temp ^ y)) >> ((sizeof(T) * (T)8) - 1)) != 0;
	}

	template <typename T>
	inline bool SignedSubtractionWouldOverflow(T x, T y){
		const T tMin = (T)((T)1 << (T)((sizeof(T) * 8) - 1)); // This is not strictly portable.
		return (x >= 0) ? (y < (T)(x - (T)-(tMin + 1))) : (y > (T)(x - tMin));
	}

	template <typename T>
	inline bool UnsignedAdditionWouldOverflow(T x, T y){
		const T temp = (T)(x + y);
		return (temp < x) && (temp < y);
	}

	template <typename T>
	inline bool UnsignedSubtractionWouldOverflow(T x, T y){
		return y > x;
	}

	/// UnsignedMultiplyWouldOverflow
	/// How to detect that a multiplication will overflow.
	/// This is a useful operation to test for when writing robust applications.
	/// Some platforms generate exceptions upon an overflow, and the functions 
	/// presented here work by possibly causing overflow. Alternative versions of 
	/// these functions exist which are slightly more complex but can work without 
	/// generating overflows themselves. However, no platform that you 
	/// are likely to be using generates such exceptions.
	/// Example:
	///    4 * 5 -> false
	///    0xffffffff * 0xffffffff -> true
	template <typename T>
	inline bool UnsignedMultiplyWouldOverflow(T x, T y){
		if(y)
			return (((x * y) / y) != x);
		return false;
	}

	inline bool SignedMultiplyWouldOverflow(int32_t x, int32_t y){
		if((y < 0) && (x == (int32_t)INT32_C(0x80000000)))
			return true;
		if(y)
			return (((x * y) / y) != x);
		return false;
	}

	inline bool SignedMultiplyWouldOverflow(int64_t x, int64_t y){
		if((y < 0) && (x == (int64_t)INT64_C(0x8000000000000000)))
			return true;
		if(y)
			return (((x * y) / y) != x);
		return false;
	}

	/// UnsignedDivisionWouldOverflow
	/// How to detect that a division will overflow.
	/// This is a useful operation to test for when writing robust applications.
	/// Some platforms generate exceptions upon an overflow, and the functions 
	/// presented here work by possibly causing overflow. Alternative versions 
	/// of these functions exist which are slightly more complex but can work 
	/// without generating overflows themselves. However, no platform that you 
	/// are likely to be using generates such exceptions.
	/// Example:
	///    5 / 4 -> false
	///    3 / 0 -> true
	template <typename T>
	inline bool UnsignedDivisionWouldOverflow(T /*x*/, T y){
		return y == 0;
	}

	/// SignedDivisionWouldOverflow
	/// How to detect that a division will overflow.
	inline bool SignedDivisionWouldOverflow(int32_t x, int32_t y){
		return (y == 0) || ((x == (int32_t)INT32_C(0x80000000)) && (y == -1));
	}

	inline bool SignedDivisionWouldOverflow(int64_t x, int64_t y){
		return (y == 0) || ((x == (int64_t)INT64_C(0x8000000000000000)) && (y == -1));
	}


	/// GetAverage
	/// How to compute average of two integers without possible overflow errors.
	/// Returns floor of the average; Rounds to negative infinity.
	/// This is useful for averaging two integers whereby the standard mechanism 
	/// of returning (x + y) / 2  may overflow and yield an incorrect answer.
	/// Normally, you want the floor (as opposed to ceiling) of the average, 
	/// at least for positive values.
	/// Example:
	///     3,  4 ->  3
	///     3,  3 ->  3
	///    -3, -4 -> -4
	///    -4,  5 ->  0
	template <typename T>
	inline int GetAverage(T x, T y){
		return (x & y) + ((x ^ y) >> 1); // Need to use '>> 1' instead of '/ 2'
	}

	/// GetAverage_Ceiling
	/// Returns ceiling of the average; Rounds to positive infinity.
	/// Example:
	///     3,  4 ->  4
	///     3,  3 ->  3
	///    -3, -4 -> -3
	///    -4,  5 ->  1
	template <typename T>
	inline int GetAverage_Ceiling(T x, T y){
		return (x | y) - ((x ^ y) >> 1); // Need to use '>> 1' instead of '/ 2'
	}

	EA_RESTORE_VC_WARNING()  // 4310



	////////////////////////////////////////////////////////////////////////////
	// Miscellaneous
	////////////////////////////////////////////////////////////////////////////

	/// GetParity
	/// How to calculate parity of an integer.
	/// Parity is defined as the oddness or evenness of the count of 1 bits.
	/// There are other mechansims for accomplishing this. The version 
	/// presented here has no branches, looping, nor table lookups.
	/// Example:
	///    01100011 -> 0 (even)
	///    00101010 -> 1 (odd)
	inline int GetParity(uint32_t x){
		x ^= x >> 1;
		x ^= x >> 2;
		x = (x & UINT32_C(0x11111111)) * UINT32_C(0x11111111);
		return (int)((x >> 28) & 1);
	}

	inline int GetParity(uint64_t x){
		x ^= x >> 1;
		x ^= x >> 2;
		x = (x & UINT64_C(0x1111111111111111)) * UINT64_C(0x1111111111111111);
		return (int)((x >> 60) & 1);
	}


	/// GetIsBigEndian
	/// How to test (or verify) that the current machine is big-endian.
	/// It is not normally possible with C/C++ to tell at compile-time what 
	/// the machine endian-ness is. However, you might be able to check a 
	/// predefined value, such as EABase's EA_SYSTEM_BIG_ENDIAN.
	inline bool GetIsBigEndian(){
		const int temp = 1;
		return ((char*)&temp)[0] == 0;
	}

	/// ToggleBetween0And1
	/// How to toggle an integer between 0 and 1.
	/// Example:
	///    1 -> 0
	///    0 -> 1
	inline int ToggleBetween0And1(int x){
		return x ^ 1;
	}

	/// ToggleBetweenIntegers
	/// How to toggle between any two integers.     
	/// Toggling between three or more values is possible but requires 
	/// code that is a bit more complicated.
	/// You can do this in place with:
	///    x ^= a ^ b;
	/// Example:
	///    5, 4, 5 -> 4
	template <typename T>
	inline T ToggleBetweenIntegers(T x, T a, T b){
		return (T)(x ^ a ^ b);
	}

	/// IsBetween0AndValue
	/// How to quickly see if an int is >= 0 and < some value.
	/// The purpose of this is to save a comparison to zero and thus 
	/// execute faster because you get two comparisons for the price of one. 
	/// This method can be extended to any integer range by adding a 
	/// value to x before the comparison.
	/// Normally you wouldn't call a function as with this example, 
	/// but instead you would just say something like:
	///    if((unsigned)x < 4) ...
	/// Example:
	///    3, 4 -> true 
	///   -3, 4 -> false
	/// Example function:
	///    inline bool IsBetween8And12(int x){
	///        return ((unsigned)x + 8) < 12;
	///    }
	inline bool IsBetween0AndValue(int32_t x, int32_t a){
		return (uint32_t)x < (uint32_t)a;
	}

	inline bool IsBetween0AndValue(int64_t x, int64_t a){
		return (uint64_t)x < (uint64_t)a;
	}

	/// ExchangeValues
	/// How to exchange two values in place.
	/// It is well-known that xor-ing two values like this exchanges them in place.
	/// This trick is perhaps of little practical value and is instead more of a curiosity.
	/// Example:
	///    2, 3 -> 3, 2
	template <typename T>
	inline void ExchangeValues(T& x, T& y){
		x = (T)(x ^ y);
		y = (T)(x ^ y);
		x = (T)(x ^ y);
	}

	/// FloorMod
	/// The normal C modulo operator (%) operates on the assumption that
	/// division rounds towards zero, which means that the result of a
	/// modulus operation on a negative number is also negative. (The
	/// modulus operator in C is defined as the remainder after a division,
	/// so in the case where a negative number is rounded up, the remainder
	/// will be negative.) FloorMod instead returns the remainder of
	/// a division rounded towards negative infinity; the modulus will always 
	/// be a positive number.
	template<class T>
	T FloorMod(T n, T mod){
		const T v = n % mod;
		return v + ((v >> ((sizeof(v) * 8) - 1)) & mod);
	}

	/// GetSign
	/// How to get the sign of an integer.
	/// Negative inputs return -1, positive inputs return +1, a zero input returns 0.
	/// These functions are significantly faster than brute force if/else statements.
	/// Requires signed right shift support. Note that most compilers/platforms 
	/// support signed numerical right shifts.
	/// Example:
	///     -5  -> -1
	///      0  ->  0
	///     250 ->  1
	inline int GetSign(int32_t x){
		return (int)((x >> 31) | (-(uint32_t)x >> 31)); // negation of signed int is undefined behaviour
	}

	inline int GetSign(int64_t x){
		return (int)((x >> 63) | (-(uint64_t)x >> 63)); // negation of signed int is undefined behaviour
	}

	/// GetSignEx
	/// How to get the sign of an integer.
	/// Returns 1 for > 0, 0 for 0, and -1 for < 0.
	/// This version doesn't work for input of 0x80000000 / 0x8000000000000000. 
	inline int GetSignEx(int32_t x){
		return (x >> 31) - (-x >> 31);
	}

	inline int GetSignEx(int64_t x){
		return (x >> 63) - (-x >> 63);
	}

	/// SignExtend12
	/// How to do fast sign extension of a 12 bit value to 32 bits.
	/// Say you have a signed 12 bit value stored in an integer and you want to 
	/// make it be a proper 32 bit int. To do this, you want to take the high bit 
	/// of the 12 bit value and replicate it leftward.
	/// To do this in a generic way, create a mask of the high bits in the 
	/// larger operand plus the sign bit of the smaller, add, and xor.
	/// Example:
	///    0x00000fff -> 12 -> 0xffffffff
	///    0x00000aaa -> 12 -> 0x00000aaa
	///
	///    0x00ffffff -> 24 -> 0xffffffff
	///    0x00aaaaaa -> 24 -> 0x00aaaaaa
	inline int32_t SignExtend12(int32_t x){
		return (int32_t)((x + 0xfffff800) ^ 0xfffff800);
	}

	inline int32_t SignExtend24(int32_t x){
		return (int32_t)((x + 0xff800000) ^ 0xff800000);
	}

	/// IsUnsigned
	/// This version requires signed right shift support. It should work for most 
	/// modern computers and compilers, as they have a signed right shift.
	template <class T>
	bool IsUnsigned(T /*x*/){
		return (((T)(-1) >> 1) != (T)(-1));
	}

	/// EAIsUnsigned
	/// How to tell if an integer variable is an unsigned type at runtime.
	/// 
	/// Note that this isn't a test to see of a given variable is negative; 
	/// this is a test to see if a data type is signed or unsigned. 
	/// With some data types, such as wchar_t and size_t, you can't know 
	/// if the type is signed or unsigned at compile time.
	///
	/// This is a notoriously tricky problem, as commonly published solutions 
	/// don't work for integer types other than int or make assumptions about 
	/// how right shifting of signed values occur.
	///
	/// This could also be solved for C++ with templates. That would be a better
	/// solution that this and would effectively be a compile-time check as opposed 
	/// to a runtime check. See EASTL type traits for this.
	///
	/// Example usage:
	///     uint8_t u8  = 0;
	///     int64_t i64 = 0;
	///     assert( EAIsUnsigned(u8)));
	///     assert(!EAIsUnsigned(i64)));
	#ifndef EAIsUnsigned
		#define EAIsUnsigned(x) ((((x = ~x) >= 0) || (((x = ~x) != 0) && 0)) && ((x = ~x) >= 0))
	#endif


	// How to tell the integer representation of the computer.
	// Just about every computer you are likely to work with will be twos-complement.
	// You can look these terms up on the Internet to get detailed descriptions of them.
	inline bool IsTwosComplement(){
		return ((-2 | -3) == -1);
	}

	inline bool IsOnesComplement(){
		return ((-1 & -2) == -3);
	}

	inline bool IsSignMagnitude(){
		return ((-1 | -2) == -3);
	}

	inline bool IsOffsetBinary(){
		return ((1u << (sizeof(int) * 8 - 1)) == 0);
	}


	/// EAOffsetOf
	///
	/// The offsetof macro is guaranteed to only work with POD types. However we wish to use
	/// it for non-POD types but where we know that offsetof will still work for the cases 
	/// in which we use it. 
	#ifndef EAOffsetOf
		#define EAOffsetOf EA_OFFSETOF
	#endif


	/// EAOffsetOfBase
	///
	/// Returns the offset of the class Base within the subclass Class.
	/// Fails to compile for cases where Base is multiply inherited by Class.
	/// Example usage:
	///     struct A{ int a; };
	///     struct B{ int b; };
	///     struct C : public A, public B{ int c; };
	///     EAOffsetOfBase<C, B>() => 4
	/// 
	/// EAOffsetOfBase is not guaranteed to be a compile-time constant expression, 
	/// due to the use of reinterpret_cast of a pointer. 
	///
	#if !defined(EAOffsetOfBaseDefined)
		#define EAOffsetOfBaseDefined 1

		template <typename Class, typename Base>
		EA_NO_UBSAN size_t EAOffsetOfBase() // Disable UBSAN because we are using an address which doesn't respect type alignment.
		{    
			return (size_t)((Base*)reinterpret_cast<Class*>(1)) - 1; // Use 1 instead of 0 because compilers often treat 0 specially and warn.
		}
	#endif


} // namespace StdC
} // namespace EA


EA_RESTORE_VC_WARNING()


#endif // Header include guard



 




