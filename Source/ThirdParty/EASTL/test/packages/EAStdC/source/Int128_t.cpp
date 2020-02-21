///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/Int128_t.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <EAAssert/eaassert.h>


#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4723) // potential divide by 0
	#pragma warning(disable: 4365) // 'argument' : conversion from 'int' to 'uint32_t', signed/unsigned mismatch
	#pragma warning(disable: 4146) // unary minus operator applied to unsigned type, result still unsigned
#endif


namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// Constants

// EASTDC_INT128_MIN is equal to: -170141183460469231731687303715884105728;
const int128_t EASTDC_INT128_MIN(0x00000000, 0x00000000, 0x00000000, 0x80000000);

// EASTDC_INT128_MAX is equal to:  170141183460469231731687303715884105727;
const int128_t EASTDC_INT128_MAX(0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff);

// EASTDC_UINT128_MIN is equal to:  0;
const uint128_t EASTDC_UINT128_MIN(0x00000000, 0x00000000, 0x00000000, 0x00000000);

// EASTDC_UINT128_MAX is equal to:  340282366920938463463374607431768211455;
const uint128_t EASTDC_UINT128_MAX(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);





///////////////////////////////////////////////////////////////////////////////
// int128_t
///////////////////////////////////////////////////////////////////////////////

int128_t_base::int128_t_base()
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = 0;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = 0;
	#endif
}

int128_t_base::int128_t_base(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3)
{
	#if EA_INT128_USE_INT64
		mPart1 = ((uint64_t)nPart3 << 32) + nPart2;
		mPart0 = ((uint64_t)nPart1 << 32) + nPart0;
	#else
		mPart3 = nPart3;
		mPart2 = nPart2;
		mPart1 = nPart1;
		mPart0 = nPart0;
	#endif
}

int128_t_base::int128_t_base(uint64_t nPart0, uint64_t nPart1)
{
	#if EA_INT128_USE_INT64
		mPart1 = nPart1;
		mPart0 = nPart0;
	#else
		mPart3 = (uint32_t)(nPart1 >> 32);
		mPart2 = (uint32_t) nPart1;
		mPart1 = (uint32_t)(nPart0 >> 32);
		mPart0 = (uint32_t) nPart0;
	#endif
}

int128_t_base::int128_t_base(uint8_t value)
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = value;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = value;
	#endif
}

int128_t_base::int128_t_base(uint16_t value)
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = value;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = value;
	#endif
}

int128_t_base::int128_t_base(uint32_t value)
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = value;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = value;
	#endif
}


#if defined(INT128_UINT_TYPE)

int128_t_base::int128_t_base(INT128_UINT_TYPE value)
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = value;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = value;
	#endif
}

#endif


int128_t_base::int128_t_base(uint64_t value)
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = value;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = (uint32_t) ((value >> 32) & 0xffffffff);
		mPart0 = (uint32_t) ((value >>  0) & 0xffffffff);
	#endif
}


int128_t_base::int128_t_base(const int128_t_base& value)
{
	#if EA_INT128_USE_INT64
		mPart1 = value.mPart1;
		mPart0 = value.mPart0;
	#else
		mPart3 = value.mPart3;
		mPart2 = value.mPart2;
		mPart1 = value.mPart1;
		mPart0 = value.mPart0;
	#endif
}


int128_t_base& int128_t_base::operator=(const int128_t_base& value)
{
	#if EA_INT128_USE_INT64
		mPart1 = value.mPart1;
		mPart0 = value.mPart0;
	#else
		mPart3 = value.mPart3;
		mPart2 = value.mPart2;
		mPart1 = value.mPart1;
		mPart0 = value.mPart0;
	#endif
	return *this;
}



///////////////////////////////////////////////////////////////////////////////
// operatorPlus
//
// Returns: (value1 + value2) into result.
// The output 'result' *is* allowed to point to the same memory as one of the inputs.
// To consider: Fix 'defect' of this function whereby it doesn't implement overflow wraparound.
//
void int128_t_base::operatorPlus(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result)
{
	#if defined(EA_ASM_STYLE_INTEL) && defined(EA_PROCESSOR_X86)
		__asm
		{
			mov ebx, value1
			mov ecx, value2
			mov edx, result

			mov eax, [ebx]
			add eax, [ecx]      ;(nCarry, tmp) = value1.mPart0 + value2.mPart0
			mov [edx], eax      ;result.mPart0 = value1.mPart0 + value2.mPart0

			mov eax, [ebx+4]
			adc eax, [ecx+4]    ;(nCarry, tmp) = value1.mPart1 + value2.mPart1
			mov [edx+4], eax    ;result.mPart1 = value1.mPart1 + value2.mPart1 + nCarry

			mov eax, [ebx+8]
			adc eax, [ecx+8]    ;(nCarry, tmp) = value1.mPart2 + value2.mPart2
			mov [edx+8], eax    ;result.mPart2 = value1.mPart2 + value2.mPart2 + nCarry

			mov eax, [ebx+12]
			adc eax, [ecx+12]   ;(nCarry, tmp) = value1.mPart3 + value2.mPart3
			mov [edx+12], eax   ;result.mPart3 = value1.mPart3 + value2.mPart3 + nCarry
		}
	#elif EA_INT128_USE_INT64
		uint64_t t      = value1.mPart0 + value2.mPart0;
		uint64_t nCarry = (t < value1.mPart0) && (t < value2.mPart0);
		result.mPart0   = t;
		result.mPart1   = value1.mPart1 + value2.mPart1 + nCarry;
	#else
		uint64_t t      = ((uint64_t)value1.mPart0) + ((uint64_t)value2.mPart0);
		uint32_t nCarry = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart0   = (uint32_t) t;

		t               = ((uint64_t)value1.mPart1) + ((uint64_t)value2.mPart1) + nCarry;
		nCarry          = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart1   = (uint32_t) t;

		t               = ((uint64_t)value1.mPart2) + ((uint64_t)value2.mPart2) + nCarry;
		nCarry          = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart2   = (uint32_t) t;

		t               = ((uint64_t)value1.mPart3) + ((uint64_t)value2.mPart3) + nCarry;
		//nCarry        = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart3   = (uint32_t) t;
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// operatorMinus
//
// Returns: (value1 - value2) into result.
// The output 'result' *is* allowed to point to the same memory as one of the inputs.
// To consider: Fix 'defect' of this function whereby it doesn't implement overflow wraparound.
//
void int128_t_base::operatorMinus(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		uint64_t t      = (value1.mPart0 - value2.mPart0);
		uint64_t nCarry = (value1.mPart0 < value2.mPart0) ? 1 : 0;
		result.mPart0   = t;
		result.mPart1   = (value1.mPart1 - value2.mPart1) - nCarry;
	#else
		uint64_t t      = ((uint64_t)value1.mPart0) - ((uint64_t)value2.mPart0);
		uint32_t nCarry = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart0   = (uint32_t) t;

		t               = (((uint64_t)value1.mPart1) - ((uint64_t)value2.mPart1)) - nCarry;
		nCarry          = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart1   = (uint32_t) t;

		t               = (((uint64_t)value1.mPart2) - ((uint64_t)value2.mPart2)) - nCarry;
		nCarry          = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart2   = (uint32_t) t;

		t               = (((uint64_t)value1.mPart3) - ((uint64_t)value2.mPart3)) - nCarry;
		//nCarry        = (uint32_t)((t > 0xffffffff) ? 1 : 0);
		result.mPart3   = (uint32_t) t;
	#endif
}



///////////////////////////////////////////////////////////////////////////////
// operatorMul
//
// 32 bit systems:
//    The way this works is like decimal multiplication by hand with a pencil and
//    paper. The difference is that we work with blocks of 32 bits intead of blocks
//    of ten. Here is a multiplication of 0x00000008000000040000000200000001 x 
//    the same value done like you do with pencil and paper:
//     
//                                          Part 3         2         1         0
//                                        00000008  00000004  00000002  00000001
//                                     x  00000008  00000004  00000002  00000001
//                                   -------------------------------------------
//                                      | 00000008  00000004  00000002  00000001
//                             00000010 | 00000008  00000004  00000002 (00000000)
//                   00000020  00000010 | 00000008  00000004 (00000000)(00000000)
//      +  00000040  00000020  00000010 | 00000008 (00000000)(00000000)(00000000)
//     -------------------------------------------------------------------------
//
//    That the numbers above have columns each with the same values is a coincidence
//    of the choice of the two multiplying numbers and in reality numbers would 
//    likely be much more complicated. But the above is easy to show. Note that 
//    the numbers to the left of the column with 00000008 are outside the range 
//    of 128 bits. As a result, in our implementation below, we skip the steps that 
//    create these values, as they would just get lost anyway.
//
// 64 bit systems:
//    This is how it would be able to work if we could get a 128 bit result from
//    two 64 bit values. None of the 64 bit systems that we are currently working
//    with have C language support for multiplying two 64 bit numbers and retrieving
//    the 128 bit result. However, many 64 bit platforms have support at the asm
//    level for doing such a thing.
//                                                      Part 1            Part 0
//                                            0000000000000002  0000000000000001
//                                         x  0000000000000002  0000000000000001
//                                   -------------------------------------------
//                                          | 0000000000000002  0000000000000001
//                      +  0000000000000004 | 0000000000000002 (0000000000000000)
//     -------------------------------------------------------------------------
//
void int128_t_base::operatorMul(const int128_t_base& a, const int128_t_base& b, int128_t_base& result)
{
	// To consider: Use compiler or OS-provided custom functionality here, such as
	//              Windows UnsignedMultiply128 and GCC's built-in int128_t.

	#if EA_INT128_USE_INT64
		#if   defined(DISABLED_PLATFORM_WIN64)
			// To do: Implement x86-64 asm here.

		#else
			// Else we are stuck doing something less efficient. In this case we 
			// fall back to doing 32 bit multiplies as with 32 bit platforms.
			result       = (a.mPart0 & 0xffffffff) *  (b.mPart0 & 0xffffffff);
			int128_t v01 = (a.mPart0 & 0xffffffff) * ((b.mPart0 >> 32) & 0xffffffff);
			int128_t v02 = (a.mPart0 & 0xffffffff) *  (b.mPart1 & 0xffffffff);
			int128_t v03 = (a.mPart0 & 0xffffffff) * ((b.mPart1 >> 32) & 0xffffffff);

			int128_t v10 = ((a.mPart0 >> 32) & 0xffffffff) *  (b.mPart0 & 0xffffffff);
			int128_t v11 = ((a.mPart0 >> 32) & 0xffffffff) * ((b.mPart0 >> 32) & 0xffffffff);
			int128_t v12 = ((a.mPart0 >> 32) & 0xffffffff) *  (b.mPart1 & 0xffffffff);

			int128_t v20 = (a.mPart1 & 0xffffffff) *  (b.mPart0 & 0xffffffff);
			int128_t v21 = (a.mPart1 & 0xffffffff) * ((b.mPart0 >> 32) & 0xffffffff);

			int128_t v30 = ((a.mPart1 >> 32) & 0xffffffff) * (b.mPart0 & 0xffffffff);

			// Do row addition, shifting as needed. 
			operatorPlus(result, v01 << 32, result);
			operatorPlus(result, v02 << 64, result);
			operatorPlus(result, v03 << 96, result);

			operatorPlus(result, v10 << 32, result);
			operatorPlus(result, v11 << 64, result);
			operatorPlus(result, v12 << 96, result);

			operatorPlus(result, v20 << 64, result);
			operatorPlus(result, v21 << 96, result);

			operatorPlus(result, v30 << 96, result);
		#endif
	#else
		// Do part-by-part multiplication, skipping overflowing combinations.
		result        = ((uint64_t)a.mPart0) * ((uint64_t)b.mPart0);
		uint128_t v01 = ((uint64_t)a.mPart0) * ((uint64_t)b.mPart1);
		uint128_t v02 = ((uint64_t)a.mPart0) * ((uint64_t)b.mPart2);
		uint128_t v03 = ((uint64_t)a.mPart0) * ((uint64_t)b.mPart3);

		uint128_t v10 = ((uint64_t)a.mPart1) * ((uint64_t)b.mPart0);
		uint128_t v11 = ((uint64_t)a.mPart1) * ((uint64_t)b.mPart1);
		uint128_t v12 = ((uint64_t)a.mPart1) * ((uint64_t)b.mPart2);

		uint128_t v20 = ((uint64_t)a.mPart2) * ((uint64_t)b.mPart0);
		uint128_t v21 = ((uint64_t)a.mPart2) * ((uint64_t)b.mPart1);

		uint128_t v30 = ((uint64_t)a.mPart3) * ((uint64_t)b.mPart0);

		// Do row addition, shifting as needed. 
		operatorPlus(result, v01 << 32, result);
		operatorPlus(result, v02 << 64, result);
		operatorPlus(result, v03 << 96, result);

		operatorPlus(result, v10 << 32, result);
		operatorPlus(result, v11 << 64, result);
		operatorPlus(result, v12 << 96, result);

		operatorPlus(result, v20 << 64, result);
		operatorPlus(result, v21 << 96, result);

		operatorPlus(result, v30 << 96, result);
	#endif
}



///////////////////////////////////////////////////////////////////////////////
// operatorShiftRight
//
// Returns: value >> nShift into result
// The output 'result' may *not* be the same as one the input.
// With rightward shifts of negative numbers, shift in zero from the left side.
//
void int128_t_base::operatorShiftRight(const int128_t_base& value, int nShift, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		if(nShift >= 0)
		{
			if(nShift < 64)
			{   // 0 - 63
				result.mPart1 = (value.mPart1 >> nShift);

				if(nShift == 0)
					result.mPart0 = (value.mPart0 >> nShift);
				else
					result.mPart0 = (value.mPart0 >> nShift) | (value.mPart1 << (64 - nShift));
			}
			else
			{   // 64+
				result.mPart1 = 0;
				result.mPart0 = (value.mPart1 >> (nShift - 64));
			}
		}
		else // (nShift < 0)
			operatorShiftLeft(value, -nShift, result);
	#else
		if(nShift >= 0)
		{
			if(nShift <= 32)
			{
				if(nShift == 32)
				{   // We can't use the code further below for 0-31 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = value.mPart1;
					result.mPart1 = value.mPart2;
					result.mPart2 = value.mPart3;
					result.mPart3 = 0;
				}
				else
				{   // 0 - 31
					result.mPart3 = (value.mPart3 >> nShift);
					result.mPart2 = (value.mPart2 >> nShift) | (value.mPart3 << (32 - nShift));
					result.mPart1 = (value.mPart1 >> nShift) | (value.mPart2 << (32 - nShift));
					result.mPart0 = (value.mPart0 >> nShift) | (value.mPart1 << (32 - nShift));
				}
			}
			else if(nShift <= 64)
			{
				if(nShift == 64)
				{   // We can't use the code further below for 0-31 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = value.mPart2;
					result.mPart1 = value.mPart3;
					result.mPart2 = 0;
					result.mPart3 = 0;
				}
				else
				{   // 33 - 63
					result.mPart3 = 0;
					result.mPart2 = (value.mPart3 >> (nShift - 32));
					result.mPart1 = (value.mPart2 >> (nShift - 32)) | (value.mPart3 << (64 - nShift));
					result.mPart0 = (value.mPart1 >> (nShift - 32)) | (value.mPart2 << (64 - nShift));
				}
			}
			else if(nShift <= 96)
			{
				if(nShift == 96)
				{   // We can't use the code further below for 0-31 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = value.mPart3;
					result.mPart1 = 0;
					result.mPart2 = 0;
					result.mPart3 = 0;
				}
				else
				{   // 65 - 95
					result.mPart3 = 0;
					result.mPart2 = 0;
					result.mPart1 = (value.mPart3 >> (nShift - 64));
					result.mPart0 = (value.mPart2 >> (nShift - 64)) | (value.mPart3 << (96 - nShift));
				}
			}
			else if(nShift < 128)
			{   // 96 - 127
				result.mPart3 = 0;
				result.mPart2 = 0;
				result.mPart1 = 0;
				result.mPart0 = (value.mPart3 >> (nShift - 96));
			}
			else
			{   // 128+
				result.mPart3 = 0;
				result.mPart2 = 0;
				result.mPart1 = 0;
				result.mPart0 = 0;
			}
		}
		else // (nShift < 0)
			operatorShiftLeft(value, -nShift, result);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// operatorShiftRight
//
// Returns: value << nShift into result
// The output 'result' may *not* be the same as one the input.
// With rightward shifts of negative numbers, shift in zero from the left side.
//
void int128_t_base::operatorShiftLeft(const int128_t_base& value, int nShift, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		if(nShift >= 0)
		{
			if(nShift < 64)
			{
				if(nShift) // We need to have a special case because CPUs convert a shift by 64 to a no-op.
				{
					// 1 - 63
					result.mPart0 = (value.mPart0 << nShift);
					result.mPart1 = (value.mPart1 << nShift) | (value.mPart0 >> (64 - nShift));
				}
				else
				{
					result.mPart0 = value.mPart0;
					result.mPart1 = value.mPart1;
				}
			}
			else
			{   // 64+
				result.mPart0 = 0;
				result.mPart1 = (value.mPart0 << (nShift - 64));
			}
		}
		else // (nShift < 0)
			operatorShiftRight(value, -nShift, result);
	#else
		if(nShift >= 0)
		{
			if(nShift <= 32)
			{
				if(nShift == 32)
				{   // We can't use the code further below for 32 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = 0;
					result.mPart1 = value.mPart0;
					result.mPart2 = value.mPart1;
					result.mPart3 = value.mPart2;
				}
				else if(nShift)
				{   // 1 - 31
					result.mPart0 = (value.mPart0 << nShift);
					result.mPart1 = (value.mPart1 << nShift) | (value.mPart0 >> (32 - nShift));
					result.mPart2 = (value.mPart2 << nShift) | (value.mPart1 >> (32 - nShift));
					result.mPart3 = (value.mPart3 << nShift) | (value.mPart2 >> (32 - nShift));
				}
				else
				{
					result.mPart0 = value.mPart0;
					result.mPart1 = value.mPart1;
					result.mPart2 = value.mPart2;
					result.mPart3 = value.mPart3;
				}
			}
			else if(nShift <= 64)
			{
				if(nShift == 64)
				{   // We can't use the code further below for 0-31 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = 0;
					result.mPart1 = 0;
					result.mPart2 = value.mPart0;
					result.mPart3 = value.mPart1;
				}
				else
				{   // 33 - 63
					result.mPart0 = 0;
					result.mPart1 = (value.mPart0 << (nShift - 32));
					result.mPart2 = (value.mPart1 << (nShift - 32)) | (value.mPart0 >> (64 - nShift));
					result.mPart3 = (value.mPart2 << (nShift - 32)) | (value.mPart1 >> (64 - nShift));
				}
			}
			else if(nShift <= 96)
			{
				if(nShift == 96)
				{   // We can't use the code further below for 0-31 because 32 bit 
					// processors (e.g. Intel) often implement a shift of 32 as a no-op.
					result.mPart0 = 0;
					result.mPart1 = 0;
					result.mPart2 = 0;
					result.mPart3 = value.mPart0;
				}
				else
				{   // 65 - 95
					result.mPart0 = 0;
					result.mPart1 = 0;
					result.mPart2 = (value.mPart0 << (nShift - 64));
					result.mPart3 = (value.mPart1 << (nShift - 64)) | (value.mPart0 >> (96 - nShift));
				}
			}
			else if(nShift < 128)
			{   // 96 - 127
				result.mPart0 = 0;
				result.mPart1 = 0;
				result.mPart2 = 0;
				result.mPart3 = (value.mPart0 << (nShift - 96));
			}
			else
			{   // 128+
				result.mPart3 = 0;
				result.mPart2 = 0;
				result.mPart1 = 0;
				result.mPart0 = 0;
			}
		}
		else // (nShift < 0)
			operatorShiftRight(value, -nShift, result);
	#endif
}


bool int128_t_base::operator!() const
{
	#if EA_INT128_USE_INT64
		return (mPart0 == 0) && (mPart1 == 0);
	#else
		return (mPart0 == 0) && (mPart1 == 0) && (mPart2 == 0) && (mPart3 == 0);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// operatorXOR
//
// Returns: value1 ^ value2 into result
// The output 'result' may be the same as one the input.
//
void int128_t_base::operatorXOR(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		result.mPart0 = (value1.mPart0 ^ value2.mPart0);
		result.mPart1 = (value1.mPart1 ^ value2.mPart1);
	#else
		result.mPart0 = (value1.mPart0 ^ value2.mPart0);
		result.mPart1 = (value1.mPart1 ^ value2.mPart1);
		result.mPart2 = (value1.mPart2 ^ value2.mPart2);
		result.mPart3 = (value1.mPart3 ^ value2.mPart3);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// operatorOR
//
// Returns: value1 | value2 into result
// The output 'result' may be the same as one the input.
//
void int128_t_base::operatorOR(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		result.mPart0 = (value1.mPart0 | value2.mPart0);
		result.mPart1 = (value1.mPart1 | value2.mPart1);
	#else
		result.mPart0 = (value1.mPart0 | value2.mPart0);
		result.mPart1 = (value1.mPart1 | value2.mPart1);
		result.mPart2 = (value1.mPart2 | value2.mPart2);
		result.mPart3 = (value1.mPart3 | value2.mPart3);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// operatorAND
//
// Returns: value1 & value2 into result
// The output 'result' may be the same as one the input.
//
void int128_t_base::operatorAND(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result)
{
	#if EA_INT128_USE_INT64
		result.mPart0 = (value1.mPart0 & value2.mPart0);
		result.mPart1 = (value1.mPart1 & value2.mPart1);
	#else
		result.mPart0 = (value1.mPart0 & value2.mPart0);
		result.mPart1 = (value1.mPart1 & value2.mPart1);
		result.mPart2 = (value1.mPart2 & value2.mPart2);
		result.mPart3 = (value1.mPart3 & value2.mPart3);
	#endif
}


bool int128_t_base::AsBool() const
{
	#if EA_INT128_USE_INT64
		return (mPart0 || mPart1);
	#else
		return (mPart0 || mPart1 || mPart2 || mPart3);
	#endif
}


uint8_t int128_t_base::AsUint8() const
{
	// OK for EA_INT128_USE_INT64
	return (uint8_t) mPart0;
}


uint16_t int128_t_base::AsUint16() const
{
	// OK for EA_INT128_USE_INT64
	return (uint16_t) mPart0;
}


uint32_t int128_t_base::AsUint32() const
{
	// OK for EA_INT128_USE_INT64
	return (uint32_t) mPart0;
}


uint64_t int128_t_base::AsUint64() const
{
	#if EA_INT128_USE_INT64
		return mPart0;
	#else
		return (((uint64_t) mPart1) << 32) + mPart0;
	#endif
}


int int128_t_base::GetBit(int nIndex) const
{
	// EA_ASSERT((nIndex >= 0) && (nIndex < 128));

	#if EA_INT128_USE_INT64
		const uint64_t nBitMask = ((uint64_t)1 << (nIndex % 64));

		if(nIndex < 64)
			return ((mPart0 & nBitMask) ? 1 : 0);
		else if(nIndex < 128)
			return ((mPart1 & nBitMask) ? 1 : 0);
		return 0;
	#else
		const uint32_t nBitMask = ((uint32_t)1 << (nIndex % 32));

		if(nIndex < 32)
			return ((mPart0 & nBitMask) ? 1 : 0);
		else if(nIndex < 64)
			return ((mPart1 & nBitMask) ? 1 : 0);
		else if(nIndex < 96)
			return ((mPart2 & nBitMask) ? 1 : 0);
		else if(nIndex < 128)
			return ((mPart3 & nBitMask) ? 1 : 0);
		return 0;
	#endif
}


void int128_t_base::SetBit(int nIndex, int value)
{
	// EA_ASSERT((nIndex >= 0) && (nIndex < 128));

	#if EA_INT128_USE_INT64
		const uint64_t nBitMask = ((uint64_t)1 << (nIndex % 64));

		if(nIndex < 64)
		{
			if(value)
				mPart0 = mPart0 |  nBitMask;
			else
				mPart0 = mPart0 & ~nBitMask;
		}
		else if(nIndex < 128)
		{
			if(value)
				mPart1 = mPart1 |  nBitMask;
			else
				mPart1 = mPart1 & ~nBitMask;
		}
	#else
		const uint32_t nBitMask = ((uint32_t)1 << (nIndex % 32));

		if(nIndex < 32)
		{
			if(value)
				mPart0 = mPart0 |  nBitMask;
			else
				mPart0 = mPart0 & ~nBitMask;
		}
		else if(nIndex < 64)
		{
			if(value)
				mPart1 = mPart1 |  nBitMask;
			else
				mPart1 = mPart1 & ~nBitMask;
		}
		else if(nIndex < 96)
		{
			if(value)
				mPart2 = mPart2 |  nBitMask;
			else
				mPart2 = mPart2 & ~nBitMask;
		}
		else if(nIndex < 128)
		{
			if(value)
				mPart3 = mPart3 |  nBitMask;
			else
				mPart3 = mPart3 & ~nBitMask;
		}
	#endif
}


// part is in the range of [0,15]
uint8_t int128_t_base::GetPartUint8(int nIndex) const
{
	#if EA_INT128_USE_INT64
		uint64_t value(0);

		switch (nIndex / 8) 
		{
			case 0:
				value = mPart0;
				break;

			case 1:
				value = mPart1;
				break;
		}

		nIndex = ((nIndex % 8) * 8);
		return (uint8_t)((value & ((uint64_t)0xff << nIndex)) >> nIndex);
	#else
		uint32_t value(0);

		switch (nIndex / 4) 
		{
			case 0:
				value = mPart0;
				break;

			case 1:
				value = mPart1;
				break;

			case 2:
				value = mPart2;
				break;

			case 3:
				value = mPart3;
				break;
		}

		nIndex = ((nIndex % 4) * 8);
		return (uint8_t)(((value & ((uint32_t)0xff << nIndex))) >> nIndex);
	#endif
}


// part is in the range of [0,7]
uint16_t int128_t_base::GetPartUint16(int nIndex) const
{
	#if EA_INT128_USE_INT64
		uint64_t value(0);

		switch (nIndex / 4) 
		{
			case 0:
				value = mPart0;
				break;

			case 1:
				value = mPart1;
				break;
		}

		nIndex = ((nIndex % 4) * 16);
		return (uint16_t)(((value & ((uint64_t)0xffff << nIndex))) >> nIndex);
	#else
		uint32_t value(0);

		switch (nIndex / 2) 
		{
			case 0:
				value = mPart0;
				break;

			case 1:
				value = mPart1;
				break;

			case 2:
				value = mPart2;
				break;

			case 3:
				value = mPart3;
				break;
		}

		if(nIndex % 2)
			return (uint16_t)(value >> 16);
		else
			return (uint16_t)(value);
	#endif
}


// part is in the range of [0,3]
uint32_t int128_t_base::GetPartUint32(int nIndex) const
{
	#if EA_INT128_USE_INT64
		switch (nIndex) 
		{
			case 0:
				return (uint32_t) mPart0;
			case 1:
				return (uint32_t)(mPart0 >> 32);
			case 2:
				return (uint32_t) mPart1;
			case 3:
				return (uint32_t)(mPart1 >> 32);
		}
		return 0;
	#else
		switch (nIndex) 
		{
			case 0:
				return mPart0;
			case 1:
				return mPart1;
			case 2:
				return mPart2;
			case 3:
				return mPart3;
		}
		return 0;
	#endif
}


// part is in the range of [0,1]
uint64_t int128_t_base::GetPartUint64(int nIndex) const
{
	#if EA_INT128_USE_INT64
		if(nIndex == 0)
			return mPart0;
		else if(nIndex == 1)
			return mPart1;
		return 0;
	#else
		if(nIndex == 0)
			return uint64_t((uint64_t(mPart1) << 32) + mPart0);
		else if(nIndex == 1)
			return uint64_t((uint64_t(mPart3) << 32) + mPart2);
		return 0;
	#endif
}


void int128_t_base::SetPartUint8(int nIndex, uint8_t value)
{
	#if EA_INT128_USE_INT64
		uint64_t* pValue;

		switch (nIndex / 8) 
		{
			case 0:
				pValue = &mPart0;
				break;

			case 1:
				pValue = &mPart1;
				break;

			default:
				return;
		}

		nIndex %= 8;
		*pValue = ((*pValue & ~(UINT64_C(0xff) << (nIndex * 8))) + ((uint64_t)value << (nIndex * 8)));

	#else
		uint32_t* pValue;

		switch (nIndex / 4) 
		{
			case 0:
				pValue = &mPart0;
				break;

			case 1:
				pValue = &mPart1;
				break;

			case 2:
				pValue = &mPart2;
				break;

			case 3:
				pValue = &mPart3;
				break;

			default:
				return;
		}

		switch (nIndex % 4)
		{
			case 0:
				*pValue = ((*pValue & 0xffffff00) + (value <<  0));
				break;

			case 1:
				*pValue = ((*pValue & 0xffff00ff) + (value <<  8));
				break;

			case 2:
				*pValue = ((*pValue & 0xff00ffff) + (value << 16));
				break;

			case 3:
				*pValue = ((*pValue & 0x00ffffff) + (value << 24));
				break;
		}
	#endif
}


void int128_t_base::SetPartUint16(int nIndex, uint16_t value)
{
	#if EA_INT128_USE_INT64
		uint64_t* pValue;

		switch (nIndex / 4) 
		{
			case 0:
				pValue = &mPart0;
				break;

			case 1:
				pValue = &mPart1;
				break;

			default:
				return;
		}

		nIndex %= 4;
		*pValue = ((*pValue & ~(UINT64_C(0xffff) << (nIndex * 16))) + ((uint64_t)value << (nIndex * 16)));

	#else
		uint32_t* pValue;

		switch (nIndex / 2) 
		{
			case 0:
				pValue = &mPart0;
				break;

			case 1:
				pValue = &mPart1;
				break;

			case 2:
				pValue = &mPart2;
				break;

			case 3:
				pValue = &mPart3;
				break;

			default:
				return;
		}

		if(nIndex % 2)
			*pValue = ((*pValue & 0x0000ffff) + (value << 16));
		else
			*pValue = ((*pValue & 0xffff0000) + (value));
	#endif
}


void int128_t_base::SetPartUint32(int nIndex, uint32_t value)
{
	#if EA_INT128_USE_INT64
		switch (nIndex) 
		{
			case 0:
				mPart0 = (mPart0 & UINT64_C(0xffffffff00000000)) + value;
				break;
			case 1:
				mPart0 = (mPart0 & UINT64_C(0x00000000ffffffff)) + ((uint64_t)value << 32);
				break;
			case 2:
				mPart1 = (mPart1 & UINT64_C(0xffffffff00000000)) + value;
				break;
			case 3:
				mPart1 = (mPart1 & UINT64_C(0x00000000ffffffff)) + ((uint64_t)value << 32);
				break;
		}
	#else
		switch (nIndex) 
		{
			case 0:
				mPart0 = value;
				break;
			case 1:
				mPart1 = value;
				break;
			case 2:
				mPart2 = value;
				break;
			case 3:
				mPart3 = value;
				break;
		}
	#endif
}


void int128_t_base::SetPartUint64(int nIndex, uint64_t value)
{
	#if EA_INT128_USE_INT64
		if(nIndex == 0)
			mPart0 = value;
		else if(nIndex == 1)
			mPart1 = value;
	#else
		if(nIndex == 0)
		{
			mPart0 = (uint32_t)(value);
			mPart1 = (uint32_t)(value >> 32);
		}
		else if(nIndex == 1)
		{
			mPart2 = (uint32_t)(value);
			mPart3 = (uint32_t)(value >> 32);
		}
	#endif
}


bool int128_t_base::IsZero() const
{
	#if EA_INT128_USE_INT64
		return (mPart0 == 0) && // Check mPart0 first as this will likely yield faster execution.
			   (mPart1 == 0);
	#else
		return (mPart0 == 0) && // Check mPart0 first as this will likely yield faster execution.
			   (mPart1 == 0) && 
			   (mPart2 == 0) && 
			   (mPart3 == 0);
	#endif
}


void int128_t_base::SetZero()
{
	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = 0;
	#else
		mPart3 = 0;
		mPart2 = 0;
		mPart1 = 0;
		mPart0 = 0;
	#endif
}


void int128_t_base::TwosComplement()
{
	#if EA_INT128_USE_INT64
		mPart1 = ~mPart1;
		mPart0 = ~mPart0;
	#else
		mPart3 = ~mPart3;
		mPart2 = ~mPart2;
		mPart1 = ~mPart1;
		mPart0 = ~mPart0;
	#endif

	// What we want to do, but isn't available at this level:
	// operator++();
	// Alternative:
	int128_t_base one((uint32_t)1);
	operatorPlus(*this, one, *this);
}


void int128_t_base::InverseTwosComplement()
{
	// What we want to do, but isn't available at this level:
	// operator--();
	// Alternative:
	int128_t_base one((uint32_t)1);
	operatorMinus(*this, one, *this);

	#if EA_INT128_USE_INT64
		mPart1 = ~mPart1;
		mPart0 = ~mPart0;
	#else
		mPart3 = ~mPart3;
		mPart2 = ~mPart2;
		mPart1 = ~mPart1;
		mPart0 = ~mPart0;
	#endif
}


void int128_t_base::DoubleToUint128(double value)
{
	// Currently this function is limited to 64 bits of integer input.
	// We need to make a better version of this function. Perhaps we should implement 
	// it via dissecting the IEEE floating point format (sign, exponent, matissa).
	// EA_ASSERT(fabs(value) < 18446744073709551616.0); // Assert that the input is <= 64 bits of integer.

	#if EA_INT128_USE_INT64
		mPart1 = 0;
		mPart0 = (value >= 0 ? (uint64_t)value : (uint64_t)-value);
	#else
		const uint64_t value64 = (value >= 0 ? (uint64_t)value : (uint64_t)-value);

		mPart3 = 0;
		mPart2 = 0;
		mPart1 = (uint32_t) (value64 >> 32);
		mPart0 = (uint32_t)((value64 >>  0) & 0xffffffff);

		// Below is a version I have been working on a version that works up to the full 128 bits.
		// The implementation below has a roundoff problem for some cases and would have to be reworked.
		/*
		double valueTemp(value);

		if(value < 0)
			valueTemp = -valueTemp;

		//Get part3
		mPart3      = (uint32_t)(valueTemp / 79228162514264337593543950336.0); // 79228162514264337593543950336.0 is the same as 0xffffffffffffffffffffffff + 1, or 0x1000000000000000000000000.
		valueTemp -= (mPart3 * 79228162514264337593543950336.0);

		//Get part2
		mPart2      = (uint32_t)(valueTemp / 18446744073709551616.0); // 18446744073709551616.0 is the same as 0xffffffffffffffff + 1, or 0x10000000000000000.
		valueTemp -= (mPart2 * 18446744073709551616.0);

		//Get part1
		mPart1      = (uint32_t)(valueTemp / 4294967296.0); // 4294967296.0 is the same as 0xffffffff + 1, or 0x100000000.
		valueTemp -= (mPart1 * 4294967296.0);

		//Get part0
		mPart0      = (uint32_t)(valueTemp);
		*/
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// int128_t
///////////////////////////////////////////////////////////////////////////////

int128_t::int128_t()
	#if EA_INT128_USE_INT64
	  : int128_t_base(0, 0)
	#else
	  : int128_t_base(0, 0, 0, 0)
	#endif
{
}


int128_t::int128_t(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3)
	: int128_t_base(nPart0, nPart1, nPart2, nPart3) // OK for EA_INT128_USE_INT64
{
}


int128_t::int128_t(uint64_t nPart0, uint64_t nPart1)
	: int128_t_base(nPart0, nPart1) // OK for EA_INT128_USE_INT64
{
}


int128_t::int128_t(uint8_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


int128_t::int128_t(uint16_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


int128_t::int128_t(uint32_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}

#if defined(INT128_UINT_TYPE)

int128_t::int128_t(INT128_UINT_TYPE value)
	: int128_t_base((uint64_t)value) // OK for EA_INT128_USE_INT64
{
}

#endif


int128_t::int128_t(uint64_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


int128_t::int128_t(int8_t value)
{
	if(value < 0)
	{
		*this = int128_t((uint8_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = value;
		#endif
	}
}


int128_t::int128_t(int16_t value)
{
	if(value < 0)
	{
		*this = int128_t((uint16_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = value;
		#endif
	}
}


int128_t::int128_t(int32_t value)
{
	if(value < 0)
	{
		*this = int128_t((uint32_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = (uint32_t)value;
		#endif
	}
}


#if defined(INT128_INT_TYPE)

int128_t::int128_t(INT128_INT_TYPE value)
{
	operator=(int128_t((int64_t)value));
}

#endif


int128_t::int128_t(int64_t value)
{
	if(value < 0)
	{
		*this = int128_t((int64_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = (uint64_t) (value);
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = (uint32_t) ((value >> 32) & 0xffffffff);
			mPart0 = (uint32_t) (value & 0xffffffff);
		#endif
	}
}


int128_t::int128_t(const int128_t& value)
	:  int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


// Not defined because doing so would make the compiler unable to 
// decide how to choose binary functions involving int128/uint128.
//int128_t::int128_t(const uint128_t& value)
//    :  int128_t_base(value) // OK for EA_INT128_USE_INT64
//{
//}


int128_t::int128_t(const float value)
{
	// OK for EA_INT128_USE_INT64
	DoubleToUint128(value);
	if(value < 0)
		Negate();
}


int128_t::int128_t(const double value)
{
	// OK for EA_INT128_USE_INT64
	DoubleToUint128(value);
	if(value < 0)
		Negate();
}


int128_t::int128_t(const char* pValue, int nBase){
	// OK for EA_INT128_USE_INT64
	const int128_t value(StrToInt128(pValue, NULL, nBase));
	operator=(value);
}


int128_t::int128_t(const wchar_t* pValue, int nBase){
	// OK for EA_INT128_USE_INT64
	wchar_t* pTextEnd(NULL);
	const int128_t value(StrToInt128(pValue, &pTextEnd, nBase));
	operator=(value);
}


int128_t& int128_t::operator=(const int128_t_base& value)
{
	// C++ requires operator= to be subclassed, even if the subclassed 
	// implementation is identical to the base implementation.
	// OK for EA_INT128_USE_INT64
	int128_t_base::operator=(value);
	return *this;
}


int128_t int128_t::operator-() const
{
	// OK for EA_INT128_USE_INT64
	int128_t returnValue(*this);
	returnValue.Negate();
	return returnValue;
}


int128_t& int128_t::operator++()
{
	// OK for EA_INT128_USE_INT64
	int128_t_base one((uint32_t)1);
	operatorPlus(*this, one, *this);
	return *this;
}


int128_t& int128_t::operator--()
{
	// OK for EA_INT128_USE_INT64
	int128_t_base one((uint32_t)1);
	operatorMinus(*this, one, *this);
	return *this;
}


int128_t int128_t::operator++(int)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp((uint32_t)1);
	operatorPlus(*this, temp, temp);
	return temp;
}


int128_t int128_t::operator--(int)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp((uint32_t)1);
	operatorMinus(*this, temp, temp);
	return temp;
}


int128_t int128_t::operator+() const
{
	// OK for EA_INT128_USE_INT64
	return *this;
}


int128_t int128_t::operator~() const
{
	#if EA_INT128_USE_INT64
		return int128_t(~mPart0, ~mPart1);
	#else
		return int128_t(~mPart0, ~mPart1, ~mPart2, ~mPart3);
	#endif
}

int128_t operator+(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	int128_t::operatorPlus(value1, value2, temp);
	return temp;
}


int128_t operator-(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	int128_t::operatorMinus(value1, value2, temp);
	return temp;
}


///////////////////////////////////////////////////////////////////////////////
// operator *
//
int128_t operator*(const int128_t& value1, const int128_t& value2)
{
	int128_t a(value1);
	int128_t b(value2);
	int128_t returnValue;

	// Correctly handle negative values
	bool bANegative(false);
	bool bBNegative(false);

	if(a.IsNegative())
	{
		bANegative = true;
		a.Negate();
	}

	if(b.IsNegative())
	{
		bBNegative = true;
		b.Negate();
	}

	int128_t_base::operatorMul(a, b, returnValue);

	// Do negation as needed.
	if(bANegative != bBNegative)
		returnValue.Negate();

	return returnValue;
}


int128_t operator/(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t remainder;
	int128_t quotient;
	value1.Modulus(value2, quotient, remainder);
	return quotient;
}


int128_t operator%(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t remainder;
	int128_t quotient;
	value1.Modulus(value2, quotient, remainder);
	return remainder;
}


int128_t& int128_t::operator+=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorPlus(*this, value, *this);
	return *this;
}


int128_t& int128_t::operator-=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorMinus(*this, value, *this);
	return *this;
}


int128_t& int128_t::operator*=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this * value;
	return *this;
}


int128_t& int128_t::operator/=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this / value;
	return *this;
}


int128_t& int128_t::operator%=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this % value;
	return *this;
}



// With rightward shifts of negative numbers, shift in zero from the left side.
int128_t int128_t::operator>>(int nShift) const
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	operatorShiftRight(*this, nShift, temp);
	return temp;
}


// With rightward shifts of negative numbers, shift in zero from the left side.
int128_t int128_t::operator<<(int nShift) const
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	operatorShiftLeft(*this, nShift, temp);
	return temp;
}


int128_t& int128_t::operator>>=(int nShift)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	operatorShiftRight(*this, nShift, temp);
	*this = temp;
	return *this;
}


int128_t& int128_t::operator<<=(int nShift)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	operatorShiftLeft(*this, nShift, temp);
	*this = temp;
	return *this;
}


int128_t operator^(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	int128_t::operatorXOR(value1, value2, temp);
	return temp;
}


int128_t operator|(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	int128_t::operatorOR(value1, value2, temp);
	return temp;
}


int128_t operator&(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	int128_t temp;
	int128_t::operatorAND(value1, value2, temp);
	return temp;
}


int128_t& int128_t::operator^=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorXOR(*this, value, *this);
	return *this;
}


int128_t& int128_t::operator|=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorOR(*this, value, *this);
	return *this;
}


int128_t& int128_t::operator&=(const int128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorAND(*this, value, *this);
	return *this;
}


// This function forms the basis of all logical comparison functions.
// If value1 <  value2, the return value is -1.
// If value1 == value2, the return value is 0.
// If value1 >  value2, the return value is 1.
int compare(const int128_t& value1, const int128_t& value2)
{
	// Cache some values. Positive means >= 0. Negative means < 0 and thus means '!positive'.
	const bool bValue1IsPositive(value1.IsPositive());
	const bool bValue2IsPositive(value2.IsPositive());

	// Do positive/negative tests.
	if(bValue1IsPositive != bValue2IsPositive)
		return bValue1IsPositive ? 1 : -1;

	// Compare individual parts. At this point, the two numbers have the same sign.
	#if EA_INT128_USE_INT64
		if(value1.mPart1 == value2.mPart1)
		{
			if(value1.mPart0 == value2.mPart0)
				return 0;
			else if(value1.mPart0 > value2.mPart0)
				return 1;
			// return -1; //Just fall through to the end.
		}
		else if(value1.mPart1 > value2.mPart1)
			return 1;
		return -1;
	#else
		if(value1.mPart3 == value2.mPart3)
		{
			if(value1.mPart2 == value2.mPart2)
			{
				if(value1.mPart1 == value2.mPart1)
				{
					if(value1.mPart0 == value2.mPart0)
						return 0;
					else if(value1.mPart0 > value2.mPart0)
						return 1;
					// return -1; //Just fall through to the end.
				}
				else if(value1.mPart1 > value2.mPart1)
					return 1;
				// return -1; //Just fall through to the end.
			}
			else if(value1.mPart2 > value2.mPart2)
				return 1;
			// return -1; //Just fall through to the end.
		}
		else if(value1.mPart3 > value2.mPart3)
			return 1;
		return -1;
	#endif
}


bool operator==(const int128_t& value1, const int128_t& value2)
{
	#if EA_INT128_USE_INT64
		return (value1.mPart0 == value2.mPart0) && // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 == value2.mPart1);
	#else
		return (value1.mPart0 == value2.mPart0) && // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 == value2.mPart1) && 
			   (value1.mPart2 == value2.mPart2) && 
			   (value1.mPart3 == value2.mPart3);
	#endif
}

bool operator!=(const int128_t& value1, const int128_t& value2)
{
	#if EA_INT128_USE_INT64
		return (value1.mPart0 != value2.mPart0) ||  // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 != value2.mPart1);
	#else
		return (value1.mPart0 != value2.mPart0) ||  // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 != value2.mPart1) || 
			   (value1.mPart2 != value2.mPart2) || 
			   (value1.mPart3 != value2.mPart3);
	#endif
}


bool operator>(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) > 0);
}


bool operator>=(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) >= 0);
}


bool operator<(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) < 0);
}


bool operator<=(const int128_t& value1, const int128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) <= 0);
}


int8_t int128_t::AsInt8() const
{
	// OK for EA_INT128_USE_INT64
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return (int8_t)-t.AsInt8();
	}

	return (int8_t) mPart0;
}


int16_t int128_t::AsInt16() const
{
	// OK for EA_INT128_USE_INT64
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return (int16_t)-t.AsInt16();
	}

	return (int16_t) mPart0;
}


int32_t int128_t::AsInt32() const
{
	// OK for EA_INT128_USE_INT64
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return -t.AsInt32();
	}

	return (int32_t) mPart0;
}


int64_t int128_t::AsInt64() const
{
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return -t.AsUint64(); // ensure mod2 behaviour
	}

	#if EA_INT128_USE_INT64
		return (int64_t) mPart0;
	#else
		return (((int64_t) mPart1) << 32) + mPart0;
	#endif
}


// I am not convinced that this is a reliable method of conversion.
float int128_t::AsFloat() const
{
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return -t.AsFloat();
	}

	float fReturnValue(0);

	#if EA_INT128_USE_INT64
		if(mPart1)
			fReturnValue += (mPart1 * 18446744073709551616.f);
		if(mPart0)
			fReturnValue +=  (float)mPart0;
	#else
		if(mPart3)
			fReturnValue += (mPart3 * 79228162514264337593543950336.f);
		if(mPart2)
			fReturnValue += (mPart2 * 18446744073709551616.f);
		if(mPart1)
			fReturnValue += (mPart1 * 4294967296.f);
		if(mPart0)
			fReturnValue +=  (float)mPart0;
	#endif

	return fReturnValue;
}


// I am not convinced that this is a reliable method of conversion.
double int128_t::AsDouble() const
{
	if(IsNegative())
	{
		int128_t t(*this);
		t.Negate();
		return -t.AsDouble();
	}

	double fReturnValue(0);

	#if EA_INT128_USE_INT64
		if(mPart1)
			fReturnValue += (mPart1 * 18446744073709551616.0);
		if(mPart0)
			fReturnValue +=  (double)mPart0;
	#else
		if(mPart3)
			fReturnValue += (mPart3 * 79228162514264337593543950336.0);
		if(mPart2)
			fReturnValue += (mPart2 * 18446744073709551616.0);
		if(mPart1)
			fReturnValue += (mPart1 * 4294967296.0);
		if(mPart0)
			fReturnValue +=  (double)mPart0;
	#endif

	return fReturnValue;
}


void int128_t::Negate()
{
	// OK for EA_INT128_USE_INT64
	if(IsPositive())
		TwosComplement();
	else
		InverseTwosComplement();
}


bool int128_t::IsNegative() const
{   // True if value < 0
	#if EA_INT128_USE_INT64
		return ((mPart1 & UINT64_C(0x8000000000000000)) != 0);
	#else
		return ((mPart3 & 0x80000000) != 0);
	#endif
}


bool int128_t::IsPositive() const
{   // True of value >= 0
	#if EA_INT128_USE_INT64
		return ((mPart1 & UINT64_C(0x8000000000000000)) == 0);
	#else
		return ((mPart3 & 0x80000000) == 0);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// Modulus
//
// This is a generic function that does both division modulus calculations.
//
void int128_t::Modulus(const int128_t& divisor, int128_t& quotient, int128_t& remainder) const
{
	// OK for EA_INT128_USE_INT64
	int128_t tempDividend(*this);
	int128_t tempDivisor(divisor);

	bool bDividendNegative = false;
	bool bDivisorNegative = false;

	if(tempDividend.IsNegative())
	{
		bDividendNegative = true;
		tempDividend.Negate();
	}
	if(tempDivisor.IsNegative())
	{
		bDivisorNegative = true;
		tempDivisor.Negate();
	}

	// Handle the special cases
	if(tempDivisor.IsZero())
	{
		// Force a divide by zero exception. 
		// We know that tempDivisor.mPart0 is zero.
		quotient.mPart0 /= tempDivisor.mPart0;
	}
	else if(tempDividend.IsZero())
	{
		quotient  = int128_t((uint32_t)0);
		remainder = int128_t((uint32_t)0);
	}
	else
	{
		remainder.SetZero();

		for(int i(0); i < 128; i++)
		{
			remainder += (uint32_t)tempDividend.GetBit(127 - i);
			const bool bBit(remainder >= tempDivisor);
			quotient.SetBit(127 - i, bBit);

			if(bBit)
				remainder -= tempDivisor;
		 
			if((i != 127) && !remainder.IsZero())
				remainder <<= 1;
		}
	}

	if((bDividendNegative && !bDivisorNegative) || (!bDividendNegative && bDivisorNegative))
	{
		// Ensure the following formula applies for negative dividends
		// dividend = divisor * quotient + remainder
		quotient.Negate();
	}
}


///////////////////////////////////////////////////////////////////////////////
// StrToInt128
//
// Same as C runtime strtol function but for int128_t. 
// This is probably the most general and useful of the C atoi family of functions.
//
int128_t int128_t::StrToInt128(const char* pValue, char** ppEnd, int nBase)
{
	int128_t    value((uint32_t)0); // Current value
	const char* p = pValue;         // Current position
	const char* pBegin = NULL;      // Where the digits start.
	const char* pEnd = NULL;        // Where the digits end. One-past the last digit.
	char        chSign('+');        // One of either '+' or '-'

	// Skip leading whitespace
	while(isspace((unsigned char)*p))
		++p;

	// Check for sign.
	if((*p == '-') || (*p == '+'))
		chSign = *p++;

	// Do checks on 'nBase'.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (char*)pValue;
		return value;
	}
	else if(nBase == 0){
		// Auto detect one of base 2, 8, 10, or 16. 
		if(*p != '0')
			nBase = 10;
		else if((p[1] == 'x') || (p[1] == 'X')) // It's safe to read p[1] because p[0] is known to be '0'.
			nBase = 16;
		else if((p[1] == 'b') || (p[1] == 'B'))
			nBase = 2;
		else
			nBase = 8;
	}

	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((*p == '0') && ((p[1] == 'x') || (p[1] == 'X')))
			p += 2;
	}
	else if(nBase == 2){
		// If there is a leading '0b', then skip past it.
		if((*p == '0') && ((p[1] == 'b') || (p[1] == 'B')))
			p += 2;
	}

	// Save the position where the digits start.
	pBegin = p;

	if(nBase == 2)  // Binary
	{
		while((*p == '0') || (*p == '1'))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 128) // There can be at most 128 binary digits in the string.
		{
			pEnd = pBegin + 128;
			p    = pEnd;
		}

		for(int i(0); p > pBegin; ++i)
		{
			--p;
			if(*p == '1')
				value.SetBit(i, true);
		}
	}
	else if(nBase == 10) // Decimal
	{
		while(isdigit((unsigned char)*p))
			++p;
		pEnd = p;

		if(pEnd > pBegin + 39) // With base 10, it is not enough to simply check against 39 digits, 
		{                      // as you can have 39 '9's and overflow. But 39 is the most you could have.
			pEnd = pBegin + 39;
			p    = pEnd;
		}

		int128_t multiplier((uint32_t)1);

		for(int i(0); p > pBegin; ++i)
		{
			const uint32_t c = (uint32_t)(*(--p) - '0');

			if(c)
			{
				// This can be optimized for faster speed by doing the smaller orders
				// of ten on value.mPart0 with an int multiplier instead of on value 
				// and a int128_t multiplier.
				value += (multiplier * c);
			}

			multiplier *= (uint32_t)10;
		}
	}
	else if(nBase == 16) // Hexadecimal
	{
		while(isxdigit((unsigned char)*p))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 32) // There can be at most 32 hexadecimal digits in the string.
		{
			pEnd = pBegin + 32;
			p    = pEnd;
		}

		// There can be as many as 32 characters.
		for(int i(0); p > pBegin; i++)
		{
			#if EA_INT128_USE_INT64
				const int nPart = (int)((pEnd - p) / 16);
				uint64_t c = *(--p); // c is an integer in the range of [0,15].
			#else
				const int nPart = (int)((pEnd - p) / 8);
				uint32_t c = *(--p); // c is an integer in the range of [0,15].
			#endif

			if(c >= '0' && c <= '9')
				c = (c - '0');
			else if(c >= 'a' && c <= 'f')
				c = 10 + (c - 'a');
			else
				c = 10 + (c - 'A');

			if(c)
			{
				#if EA_INT128_USE_INT64
					c <<= ((i % 16) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
				#else
					c <<= ((i % 8) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
					else if(nPart == 2)
						value.mPart2 |= c;
					else if(nPart == 3)
						value.mPart3 |= c;
				#endif
			}
		}
	}
	else
	{
		// EA_ASSERT(false); // For the time being, we handle only the above bases. But that's all that's required by the standard.
	}

	if(chSign == '-')
		value.Negate();

	if(ppEnd)
		*ppEnd = (char*)pEnd;

	return value;
}


///////////////////////////////////////////////////////////////////////////////
// StrToInt128
//
// Same as C runtime strtol function but for int128_t. 
// This is probably the most general and useful of the C atoi family of functions.
//
int128_t int128_t::StrToInt128(const wchar_t* pValue, wchar_t** ppEnd, int nBase)
{
	// This is simply a copy and paste of the char version of StrToInt128, with minor
	// modifications for wchar_t. 
	// To consider: Make an alternative implementation of this which converts the wchar_t
	// buffer to char and uses the char version. Doing this properly would involve more
	// than a trivial number of lines of code, and so for the time being we do the copy/paste.

	int128_t       value((uint32_t)0); // Current value
	const wchar_t* p = pValue;         // Current position
	const wchar_t* pBegin = NULL;      // Where the digits start.
	const wchar_t* pEnd = NULL;        // Where the digits end. One-past the last digit.
	wchar_t        chSign('+');        // One of either '+' or '-'

	// Skip leading whitespace
	while((*p > 0) && (*p < 127) && isspace((uint8_t)*p)) // Compare to < 127 because ctype functions will crash for higher values.
		++p;

	// Check for sign.
	if((*p == '-') || (*p == '+'))
		chSign = *p++;

	// Do checks on 'nBase'.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (wchar_t*)pValue;
		return value;
	}
	else if(nBase == 0){
		// Auto detect one of base 2, 8, 10, or 16. 
		if(*p != '0')
			nBase = 10;
		else if((p[1] == 'x') || (p[1] == 'X'))
			nBase = 16;
		else if((p[1] == 'b') || (p[1] == 'B'))
			nBase = 2;
		else
			nBase = 8;
	}

	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((*p == '0') && ((p[1] == 'x') || (p[1] == 'X')))
			p += 2;
	}
	else if(nBase == 2){
		// If there is a leading '0b', then skip past it.
		if((*p == '0') && ((p[1] == 'b') || (p[1] == 'B')))
			p += 2;
	}

	// Save the position where the digits start.
	pBegin = p;

	if(nBase == 2)  // Binary
	{
		while((*p == '0') || (*p == '1'))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 128) // There can be at most 128 binary digits in the string.
		{
			pEnd = pBegin + 128;
			p    = pEnd;
		}

		for(int i(0); p > pBegin; ++i)
		{
			--p;
			if(*p == '1')
				value.SetBit(i, true);
		}
	}
	else if(nBase == 10) // Decimal
	{
		while((*p > 0) && (*p < 127) && isdigit((uint8_t)*p)) // Compare to < 127 because ctype functions will crash for higher values.
			++p;
		pEnd = p;

		if(pEnd > pBegin + 39) // With base 10, it is not enough to simply check against 39 digits, 
		{                      // as you can have 39 '9's and overflow. But 39 is the most you could have.
			pEnd = pBegin + 39;
			p    = pEnd;
		}

		int128_t multiplier((uint32_t)1);

		for(int i(0); p > pBegin; ++i)
		{
			const uint32_t c = (uint32_t)(*(--p) - '0');

			if(c)
			{
				// This can be optimized for faster speed by doing the smaller orders
				// of ten on value.mPart0 with an int multiplier instead of on value 
				// and a int128_t multiplier.
				value += (multiplier * c);
			}

			multiplier *= (uint32_t)10;
		}
	}
	else if(nBase == 16) // Hexadecimal
	{
		while((*p > 0) && (*p < 127) && isxdigit(*p)) // Compare to < 127 because ctype functions will crash for higher values.
			p++;
		pEnd = p;

		if(pEnd > pBegin + 32) // There can be at most 32 hexadecimal digits in the string.
		{
			pEnd = pBegin + 32;
			p    = pEnd;
		}

		// There can be as many as 32 characters.
		for(int i(0); p > pBegin; i++)
		{
			#if EA_INT128_USE_INT64
				const int nPart = (int)((pEnd - p) / 16);
				uint64_t c = *(--p); // c is an integer in the range of [0,15].
			#else
				const int nPart = (int)((pEnd - p) / 8);
				uint32_t c = *(--p); // c is an integer in the range of [0,15].
			#endif

			if(c >= '0' && c <= '9')
				c = (c - '0');
			else if(c >= 'a' && c <= 'f')
				c = 10 + (c - 'a');
			else
				c = 10 + (c - 'A');

			if(c)
			{
				#if EA_INT128_USE_INT64
					c <<= ((i % 16) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
				#else
					c <<= ((i % 8) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
					else if(nPart == 2)
						value.mPart2 |= c;
					else if(nPart == 3)
						value.mPart3 |= c;
				#endif
			}
		}
	}
	else
	{
		// EA_ASSERT(false); // For the time being, we handle only the above bases. But that's all that's required by the standard.
	}

	if(chSign == '-')
		value.Negate();

	if(ppEnd)
		*ppEnd = (wchar_t*)pEnd;

	return value;
}


///////////////////////////////////////////////////////////////////////////////
// Int128ToStr
//
// Returned string has a NULL appended to it.
// Upon return, ppEnd points to the terminating NULL.
// Thus, ppEnd - pValue => string length.
//
// bPrefix applies only to base 2 (0b) and base 16 (0x).
//
void int128_t::Int128ToStr(char* pValue, char** ppEnd, int nBase, LeadingZeroes lz, Prefix prefix) const
{
	if(nBase == 2)
	{
		bool bLeadingZeros = (lz == kLZEnable);         // By default leading zeroes are disabled.
		bool bPrefix       = (prefix == kPrefixEnable); // By default prefix is disabled.

		if(bPrefix)
		{
			*pValue++ = '0'; 
			*pValue++ = 'b';
		}

		if(IsZero())
		{
			if(bLeadingZeros)
			{
				for(int i(0); i < 128; i++)
					*pValue++ = '0';
			}
			else
				*pValue++ = '0'; // This is all we need to write.
		}
		else
		{
			// Print out the text.
			bool bNonZeroFound(false);

			for(int i(127); i >= 0; --i)
			{
				const int bBitIsSet(GetBit(i));
				if(bBitIsSet)
					bNonZeroFound = true;
				if(bLeadingZeros || bNonZeroFound)
					*pValue++ = (bBitIsSet ? '1' : '0');
			}
		}
	}
	else if(nBase == 10)
	{
		// To do: Support leading zeroes and prefix for base 10.

		if(*this == EASTDC_INT128_MIN)
		{
			// This code has a special pathway because negating EASTDC_INT128_MIN results 
			// in EASTDC_INT128_MIN and thus the code below can't work.
			static const char* pINT128_MIN = "-170141183460469231731687303715884105728";
			for(const char* pCurrent = pINT128_MIN; *pCurrent; ++pCurrent, ++pValue)
				*pValue = *pCurrent;
		}
		else
		{
			int128_t   value(*this);
			char*      pValueInitial = pValue;
			const bool bNegative(IsNegative());

			if(bNegative)
			{
				value.Negate();
				*pValue++ = '-';
			}

			// This part here isn't particularly fast.
			const int128_t ten((uint32_t)10);

			while (value >= ten)
			{
				const int128_t remainder = (value % ten);
				*pValue++ = (char)('0' + remainder.mPart0);
				value /= (uint32_t)10;
			}

			*pValue++ = (char)('0' + value.mPart0);

			// Reverse the string.
			char* pEnd = pValue - 1;

			if(bNegative)
				++pValueInitial;

			while(pValueInitial < pEnd)
			{
				char temp      = *pValueInitial;
				*pValueInitial = *pEnd;
				*pEnd          = temp;
				++pValueInitial;
				--pEnd;
			}
		}
	}
	else if(nBase == 16)
	{
		bool bLeadingZeros = (lz != kLZDisable);         // By default leading zeroes are enabled.
		bool bPrefix       = (prefix != kPrefixDisable); // By default prefix is enabled.

		static const char* const pHexCharTable = "0123456789abcdef";

		if(bPrefix)
		{
			*pValue++ = '0'; 
			*pValue++ = 'x';
		}

		if(IsZero())
		{
			if(bLeadingZeros)
			{
				for(int i(0); i < 32; i++) // 32 is equal to (128 / 16)
					*pValue++ = '0';
			}
			else
				*pValue++ = '0'; // This is all we need to write.
		}
		else
		{
			// Print out the text.
			bool bNonZeroFound(false);

			// Work on each part in turn, starting with the high part.
			#if EA_INT128_USE_INT64
				for(int i(1); i >= 0; --i)
				{
					const uint64_t* pCurrent;

					if(i == 1)
						pCurrent = &mPart1;
					else
						pCurrent = &mPart0;

					// Work on each sub-part (4 bits) or the current part (64 bits), starting with the high sub-part.
					for(int j(60); j >= 0; j -= 4)
					{
						const char c = pHexCharTable[(*pCurrent >> j) & 0x0F];

						if(c != '0')
							bNonZeroFound = true;
						if(bLeadingZeros || bNonZeroFound)
							*pValue++ = c;
					}
				}
			#else
				for(int i(3); i >= 0; --i)
				{
					const uint32_t* pCurrent;

					if(i == 3)
						pCurrent = &mPart3;
					else if(i == 2)
						pCurrent = &mPart2;
					else if(i == 1)
						pCurrent = &mPart1;
					else
						pCurrent = &mPart0;

					// Work on each sub-part (4 bits) or the current part (32 bits), starting with the high sub-part.
					for(int j(28); j >= 0; j -= 4)
					{
						const char c = pHexCharTable[(*pCurrent >> j) & 0x0F];

						if(c != '0')
							bNonZeroFound = true;
						if(bLeadingZeros || bNonZeroFound)
							*pValue++ = c;
					}
				}
			#endif
		}
	}
	else
	{
		// To do: Implement this in a generic way.
		EA_FAIL(); // Base not supported.
	}

	if(ppEnd)
		*ppEnd = pValue;

	*pValue = 0;
}


void int128_t::Int128ToStr(wchar_t* pValue, wchar_t** ppEnd, int nBase, LeadingZeroes lz, Prefix prefix) const
{
	char  str8[130];
	char* pEnd = str8;

	Int128ToStr(str8, &pEnd, nBase, lz, prefix);

	for(char* p = str8; p < pEnd;)
		*pValue++ = (wchar_t)(uint8_t)*p++;

	if(ppEnd)
		*ppEnd = pValue;

	*pValue = 0;
}



///////////////////////////////////////////////////////////////////////////////
// uint128_t
///////////////////////////////////////////////////////////////////////////////

uint128_t::uint128_t()
	#if EA_INT128_USE_INT64
	  : int128_t_base(0, 0)
	#else
	  : int128_t_base(0, 0, 0, 0)
	#endif
{
}


uint128_t::uint128_t(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3)
	: int128_t_base(nPart0, nPart1, nPart2, nPart3) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(uint64_t nPart0, uint64_t nPart1)
	: int128_t_base(nPart0, nPart1) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(uint8_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(uint16_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(uint32_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


#if defined(INT128_UINT_TYPE)

uint128_t::uint128_t(INT128_UINT_TYPE value)
	: int128_t_base((uint64_t)value) // OK for EA_INT128_USE_INT64
{
}

#endif


uint128_t::uint128_t(uint64_t value)
	: int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(int8_t value)
{
	if(value < 0)
	{
		*this = uint128_t((uint8_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = value;
		#endif
	}
}

uint128_t::uint128_t(int16_t value)
{
	if(value < 0)
	{
		*this = uint128_t((uint16_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = value;
		#endif
	}
}

uint128_t::uint128_t(int32_t value)
{
	if(value < 0)
	{
		*this = uint128_t((uint32_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = value;
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = 0;
			mPart0 = (uint32_t)value;
		#endif
	}
}


#if defined(INT128_INT_TYPE)

uint128_t::uint128_t(INT128_INT_TYPE value)
{
	operator=(uint128_t((int64_t)value));
}

#endif


uint128_t::uint128_t(int64_t value)
{
	if(value < 0)
	{
		*this = uint128_t((uint64_t)-value);
		TwosComplement();
	}
	else
	{
		#if EA_INT128_USE_INT64
			mPart1 = 0;
			mPart0 = (uint64_t) (value);
		#else
			mPart3 = 0;
			mPart2 = 0;
			mPart1 = (uint32_t) ((value >> 32) & 0xffffffff);
			mPart0 = (uint32_t) (value & 0xffffffff);
		#endif
	}
}


uint128_t::uint128_t(const float value)
{
	DoubleToUint128(value); // OK for EA_INT128_USE_INT64
}


uint128_t::uint128_t(const double value)
{
	DoubleToUint128(value); // OK for EA_INT128_USE_INT64
}


uint128_t::uint128_t(const int128_t& value)
	:  int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(const uint128_t& value)
	:  int128_t_base(value) // OK for EA_INT128_USE_INT64
{
}


uint128_t::uint128_t(const char* pValue, int nBase){
	// OK for EA_INT128_USE_INT64
	const uint128_t value(StrToInt128(pValue, NULL, nBase));
	operator=(value);
}


uint128_t::uint128_t(const wchar_t* pValue, int nBase){
	// OK for EA_INT128_USE_INT64
	wchar_t* pTextEnd(NULL);
	const uint128_t value(StrToInt128(pValue, &pTextEnd, nBase));
	operator=(value);
}


uint128_t& uint128_t::operator=(const int128_t_base& value)
{
	// C++ requires operator= to be subclassed, even if the subclassed 
	// implementation is identical to the base implementation.
	// OK for EA_INT128_USE_INT64
	int128_t_base::operator=(value);
	return *this;
}


uint128_t uint128_t::operator-() const
{
	// OK for EA_INT128_USE_INT64
	uint128_t returnValue(*this);
	returnValue.Negate();
	return returnValue;
}


uint128_t& uint128_t::operator++()
{
	// OK for EA_INT128_USE_INT64
	int128_t_base one((uint32_t)1);
	operatorPlus(*this, one, *this);
	return *this;
}


uint128_t& uint128_t::operator--()
{
	// OK for EA_INT128_USE_INT64
	int128_t_base one((uint32_t)1);
	operatorMinus(*this, one, *this);
	return *this;
}


uint128_t uint128_t::operator++(int)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp((uint32_t)1);
	operatorPlus(*this, temp, temp);
	return temp;
}


uint128_t uint128_t::operator--(int)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp((uint32_t)1);
	operatorMinus(*this, temp, temp);
	return temp;
}


uint128_t uint128_t::operator+() const
{
	// OK for EA_INT128_USE_INT64
	return *this;
}


uint128_t uint128_t::operator~() const
{
	#if EA_INT128_USE_INT64
		return uint128_t(~mPart0, ~mPart1);
	#else
		return uint128_t(~mPart0, ~mPart1, ~mPart2, ~mPart3);
	#endif
}


uint128_t operator+(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	uint128_t::operatorPlus(value1, value2, temp);
	return temp;
}


uint128_t operator-(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	uint128_t::operatorMinus(value1, value2, temp);
	return temp;
}


///////////////////////////////////////////////////////////////////////////////
// operator *
//
uint128_t operator*(const uint128_t& value1, const uint128_t& value2)
{
	uint128_t returnValue;
	int128_t_base::operatorMul(value1, value2, returnValue);
	return returnValue;
}


uint128_t operator/(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t remainder;
	uint128_t quotient;
	value1.Modulus(value2, quotient, remainder);
	return quotient;
}


uint128_t operator%(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t remainder;
	uint128_t quotient;
	value1.Modulus(value2, quotient, remainder);
	return remainder;
}


uint128_t& uint128_t::operator+=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorPlus(*this, value, *this);
	return *this;
}


uint128_t& uint128_t::operator-=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorMinus(*this, value, *this);
	return *this;
}


uint128_t& uint128_t::operator*=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this * value;
	return *this;
}


uint128_t& uint128_t::operator/=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this / value;
	return *this;
}


uint128_t& uint128_t::operator%=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	*this = *this % value;
	return *this;
}



// With rightward shifts of negative numbers, shift in zero from the left side.
uint128_t uint128_t::operator>>(int nShift) const
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	operatorShiftRight(*this, nShift, temp);
	return temp;
}


// With rightward shifts of negative numbers, shift in zero from the left side.
uint128_t uint128_t::operator<<(int nShift) const
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	operatorShiftLeft(*this, nShift, temp);
	return temp;
}


uint128_t& uint128_t::operator>>=(int nShift)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	operatorShiftRight(*this, nShift, temp);
	*this = temp;
	return *this;
}


uint128_t& uint128_t::operator<<=(int nShift)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	operatorShiftLeft(*this, nShift, temp);
	*this = temp;
	return *this;
}


uint128_t operator^(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	uint128_t::operatorXOR(value1, value2, temp);
	return temp;
}


uint128_t operator|(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	uint128_t::operatorOR(value1, value2, temp);
	return temp;
}


uint128_t operator&(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	uint128_t temp;
	uint128_t::operatorAND(value1, value2, temp);
	return temp;
}


uint128_t& uint128_t::operator^=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorXOR(*this, value, *this);
	return *this;
}


uint128_t& uint128_t::operator|=(const uint128_t& value)
{
	// EA_INT128_USE_INT64
	operatorOR(*this, value, *this);
	return *this;
}


uint128_t& uint128_t::operator&=(const uint128_t& value)
{
	// OK for EA_INT128_USE_INT64
	operatorAND(*this, value, *this);
	return *this;
}


// This function forms the basis of all logical comparison functions.
// If value1 <  value2, the return value is -1.
// If value1 == value2, the return value is 0.
// If value1 >  value2, the return value is 1.
int compare(const uint128_t& value1, const uint128_t& value2)
{
	// Compare individual parts. At this point, the two numbers have the same sign.
	#if EA_INT128_USE_INT64
		if(value1.mPart1 == value2.mPart1)
		{
			if(value1.mPart0 == value2.mPart0)
				return 0;
			else if(value1.mPart0 > value2.mPart0)
				return 1;
			// return -1; //Just fall through to the end.
		}
		else if(value1.mPart1 > value2.mPart1)
			return 1;
		return -1;
	#else
		if(value1.mPart3 == value2.mPart3)
		{
			if(value1.mPart2 == value2.mPart2)
			{
				if(value1.mPart1 == value2.mPart1)
				{
					if(value1.mPart0 == value2.mPart0)
						return 0;
					else if(value1.mPart0 > value2.mPart0)
						return 1;
					// return -1; //Just fall through to the end.
				}
				else if(value1.mPart1 > value2.mPart1)
					return 1;
				// return -1; //Just fall through to the end.
			}
			else if(value1.mPart2 > value2.mPart2)
				return 1;
			// return -1; //Just fall through to the end.
		}
		else if(value1.mPart3 > value2.mPart3)
			return 1;
		return -1;
	#endif
}


bool operator==(const uint128_t& value1, const uint128_t& value2)
{
	#if EA_INT128_USE_INT64
		return (value1.mPart0 == value2.mPart0) && // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 == value2.mPart1);
	#else
		return (value1.mPart0 == value2.mPart0) && // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 == value2.mPart1) && 
			   (value1.mPart2 == value2.mPart2) && 
			   (value1.mPart3 == value2.mPart3);
	#endif
}


bool operator!=(const uint128_t& value1, const uint128_t& value2)
{
	#if EA_INT128_USE_INT64
		return (value1.mPart0 != value2.mPart0) ||  // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 != value2.mPart1);
	#else
		return (value1.mPart0 != value2.mPart0) ||  // Check mPart0 first as this will likely yield faster execution.
			   (value1.mPart1 != value2.mPart1) || 
			   (value1.mPart2 != value2.mPart2) || 
			   (value1.mPart3 != value2.mPart3);
	#endif
}


bool operator>(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) > 0);
}


bool operator>=(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) >= 0);
}


bool operator<(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) < 0);
}


bool operator<=(const uint128_t& value1, const uint128_t& value2)
{
	// OK for EA_INT128_USE_INT64
	return (compare(value1, value2) <= 0);
}


int8_t uint128_t::AsInt8() const
{
	// OK for EA_INT128_USE_INT64
	// The C++ Standard, section 4.7, paragraph 3 states that the results of 
	// conversion of an unsigned type to a signed type that cannot represent 
	// the unsigned type are implementation-defined.
	return (int8_t)mPart0;
}


int16_t uint128_t::AsInt16() const
{
	// OK for EA_INT128_USE_INT64
	// The C++ Standard, section 4.7, paragraph 3 states that the results of 
	// conversion of an unsigned type to a signed type that cannot represent 
	// the unsigned type are implementation-defined.
	return (int16_t)mPart0;
}


int32_t uint128_t::AsInt32() const
{
	// OK for EA_INT128_USE_INT64
	// The C++ Standard, section 4.7, paragraph 3 states that the results of 
	// conversion of an unsigned type to a signed type that cannot represent 
	// the unsigned type are implementation-defined.
	return (int32_t)mPart0;
}


int64_t uint128_t::AsInt64() const
{
	#if EA_INT128_USE_INT64
		return (int64_t)mPart0;
	#else
		return (((int64_t) mPart1) << 32) + mPart0;
	#endif
}


// I am not convinced that this is a reliable method of conversion.
float uint128_t::AsFloat() const
{
	float fReturnValue(0);

	#if EA_INT128_USE_INT64
		if(mPart1)
			fReturnValue += (mPart1 * 18446744073709551616.f);
		if(mPart0)
			fReturnValue +=  (float)mPart0;
	#else
		if(mPart3)
			fReturnValue += (mPart3 * 79228162514264337593543950336.f);
		if(mPart2)
			fReturnValue += (mPart2 * 18446744073709551616.f);
		if(mPart1)
			fReturnValue += (mPart1 * 4294967296.f);
		if(mPart0)
			fReturnValue += (float)mPart0;
	#endif

	return fReturnValue;
}


// I am not convinced that this is a reliable method of conversion.
double uint128_t::AsDouble() const
{
	double fReturnValue(0);

	#if EA_INT128_USE_INT64
		if(mPart1)
			fReturnValue += (mPart1 * 18446744073709551616.0);
		if(mPart0)
			fReturnValue +=  (double)mPart0;
	#else
		if(mPart3)
			fReturnValue += (mPart3 * 79228162514264337593543950336.0);
		if(mPart2)
			fReturnValue += (mPart2 * 18446744073709551616.0);
		if(mPart1)
			fReturnValue += (mPart1 * 4294967296.0);
		if(mPart0)
			fReturnValue += (double)mPart0;
	#endif

	return fReturnValue;
}


void uint128_t::Negate()
{
	// OK for EA_INT128_USE_INT64
	TwosComplement();
}


bool uint128_t::IsNegative() const
{   // True if value < 0
	// OK for EA_INT128_USE_INT64
	return false;
}


bool uint128_t::IsPositive() const
{
	// True of value >= 0
	// OK for EA_INT128_USE_INT64
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Modulus
//
// This is a generic function that does both division modulus calculations.
//
void uint128_t::Modulus(const uint128_t& divisor, uint128_t& quotient, uint128_t& remainder) const
{
	// OK for EA_INT128_USE_INT64
	uint128_t tempDividend(*this);
	uint128_t tempDivisor(divisor);

	if(tempDivisor.IsZero())
	{
		// Force a divide by zero exception. 
		// We know that tempDivisor.mPart0 is zero.
		quotient.mPart0 /= tempDivisor.mPart0;
	}
	else if(tempDividend.IsZero())
	{
		quotient  = uint128_t((uint32_t)0);
		remainder = uint128_t((uint32_t)0);
	}
	else
	{
		remainder.SetZero();

		for(int i(0); i < 128; i++)
		{
			remainder += (uint32_t)tempDividend.GetBit(127 - i);
			const bool bBit(remainder >= tempDivisor);
			quotient.SetBit(127 - i, bBit);

			if(bBit)
				remainder -= tempDivisor;
		 
			if((i != 127) && !remainder.IsZero())
				remainder <<= 1;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// StrToInt128
//
// Same as C runtime strtol function but for uint128_t. 
// This is probably the most general and useful of the C atoi family of functions.
//
uint128_t uint128_t::StrToInt128(const char* pValue, char** ppEnd, int nBase)
{
	uint128_t   value((uint32_t)0); // Current value
	const char* p = pValue;         // Current position
	const char* pBegin = NULL;      // Where the digits start.
	const char* pEnd = NULL;        // Where the digits end. One-past the last digit.
	char        chSign('+');        // One of either '+' or '-'

	// Skip leading whitespace
	while(isspace((unsigned char)*p))
		++p;

	// Check for sign.
	if((*p == '-') || (*p == '+'))
		chSign = *p++;

	// Do checks on 'nBase'.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (char*)pValue;
		return value;
	}
	else if(nBase == 0){
		// Auto detect one of base 2, 8, 10, or 16. 
		if(*p != '0')
			nBase = 10;
		else if((p[1] == 'x') || (p[1] == 'X'))
			nBase = 16;
		else if((p[1] == 'b') || (p[1] == 'B'))
			nBase = 2;
		else
			nBase = 8;
	}

	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((*p == '0') && ((p[1] == 'x') || (p[1] == 'X')))
			p += 2;
	}
	else if(nBase == 2){
		// If there is a leading '0b', then skip past it.
		if((*p == '0') && ((p[1] == 'b') || (p[1] == 'B')))
			p += 2;
	}

	// Save the position where the digits start.
	pBegin = p;

	if(nBase == 2)  // Binary
	{
		while((*p == '0') || (*p == '1'))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 128) // There can be at most 128 binary digits in the string.
		{
			pEnd = pBegin + 128;
			p    = pEnd;
		}

		for(int i(0); p > pBegin; ++i)
		{
			--p;
			if(*p == '1')
				value.SetBit(i, true);
		}
	}
	else if(nBase == 10) // Decimal
	{
		while(isdigit((unsigned char)*p))
			++p;
		pEnd = p;

		if(pEnd > pBegin + 39) // With base 10, it is not enough to simply check against 39 digits, 
		{                      // as you can have 39 '9's and overflow. But 39 is the most you could have.
			pEnd = pBegin + 39;
			p    = pEnd;
		}

		uint128_t multiplier((uint32_t)1);

		for(int i(0); p > pBegin; ++i)
		{
			const uint32_t c = *(--p) - (uint32_t)'0';

			if(c)
			{
				// This can be optimized for faster speed by doing the smaller orders
				// of ten on value.mPart0 with an int multiplier instead of on value 
				// and a uint128_t multiplier.
				value += (multiplier * c);
			}

			multiplier *= (uint32_t)10;
		}
	}
	else if(nBase == 16) // Hexadecimal
	{
		while(isxdigit((unsigned char)*p))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 32) // There can be at most 32 hexadecimal digits in the string.
		{
			pEnd = pBegin + 32;
			p    = pEnd;
		}

		// There can be as many as 32 characters.
		for(int i(0); p > pBegin; i++)
		{
			#if EA_INT128_USE_INT64
				const int nPart = (int)((pEnd - p) / 16);
				uint64_t c = *(--p);
			#else
				const int nPart = (int)((pEnd - p) / 8);
				uint32_t c = *(--p);
			#endif
			
			if(c >= '0' && c <= '9')
				c = (c - '0');
			else if(c >= 'a' && c <= 'f')
				c = 10 + (c - 'a');
			else
				c = 10 + (c - 'A');

			if(c)
			{
				#if EA_INT128_USE_INT64
					c <<= ((i % 16) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
				#else
					c <<= ((i % 8) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
					else if(nPart == 2)
						value.mPart2 |= c;
					else if(nPart == 3)
						value.mPart3 |= c;
				#endif
			}
		}
	}
	else
	{
		// EA_ASSERT(false); // For the time being, we handle only the above bases.
	}

	if(chSign == '-')
		value.Negate();

	if(ppEnd)
		*ppEnd = (char*)pEnd;

	return value;
}


///////////////////////////////////////////////////////////////////////////////
// StrToInt128
//
// Same as C runtime strtol function but for uint128_t. 
// This is probably the most general and useful of the C atoi family of functions.
//
uint128_t uint128_t::StrToInt128(const wchar_t* pValue, wchar_t** ppEnd, int nBase)
{
	// This is simply a copy and paste of the char version of StrToInt128, with minor
	// modifications for wchar_t. 
	uint128_t      value((uint32_t)0); // Current value
	const wchar_t* p = pValue;         // Current position
	const wchar_t* pBegin = NULL;      // Where the digits start.
	const wchar_t* pEnd = NULL;        // Where the digits end. One-past the last digit.
	wchar_t        chSign('+');        // One of either '+' or '-'

	// Skip leading whitespace
	while((*p > 0) && (*p < 127) && isspace((uint8_t)*p)) // Compare to < 127 because ctype functions will crash for higher values.
		++p;

	// Check for sign.
	if((*p == '-') || (*p == '+'))
		chSign = *p++;

	// Do checks on 'nBase'.
	if((nBase < 0) || (nBase == 1) || (nBase > 36)){
		if(ppEnd)
			*ppEnd = (wchar_t*)pValue;
		return value;
	}
	else if(nBase == 0){
		// Auto detect one of base 2, 8, 10, or 16. 
		if(*p != '0')
			nBase = 10;
		else if((p[1] == 'x') || (p[1] == 'X'))
			nBase = 16;
		else if((p[1] == 'b') || (p[1] == 'B'))
			nBase = 2;
		else
			nBase = 8;
	}

	if(nBase == 16){
		// If there is a leading '0x', then skip past it.
		if((*p == '0') && ((p[1] == 'x') || (p[1] == 'X')))
			p += 2;
	}
	else if(nBase == 2){
		// If there is a leading '0b', then skip past it.
		if((*p == '0') && ((p[1] == 'b') || (p[1] == 'B')))
			p += 2;
	}

	// Save the position where the digits start.
	pBegin = p;

	if(nBase == 2)  // Binary
	{
		while((*p == '0') || (*p == '1'))
			p++;
		pEnd = p;

		if(pEnd > pBegin + 128) // There can be at most 128 binary digits in the string.
		{
			pEnd = pBegin + 128;
			p    = pEnd;
		}

		for(int i(0); p > pBegin; ++i)
		{
			--p;
			if(*p == '1')
				value.SetBit(i, true);
		}
	}
	else if(nBase == 10) // Decimal
	{
		while((*p > 0) && (*p < 127) && isdigit((uint8_t)*p)) // Compare to < 127 because ctype functions will crash for higher values.
			++p;
		pEnd = p;

		if(pEnd > pBegin + 39) // With base 10, it is not enough to simply check against 39 digits, 
		{                      // as you can have 39 '9's and overflow. But 39 is the most you could have.
			pEnd = pBegin + 39;
			p    = pEnd;
		}

		uint128_t multiplier((uint32_t)1);

		for(int i(0); p > pBegin; ++i)
		{
			const uint32_t c = *(--p) - (uint32_t)'0';

			if(c)
			{
				// This can be optimized for faster speed by doing the smaller orders
				// of ten on value.mPart0 with an int multiplier instead of on value 
				// and a uint128_t multiplier.
				value += (multiplier * c);
			}

			multiplier *= (uint32_t)10;
		}
	}
	else if(nBase == 16) // Hexadecimal
	{
		while((*p > 0) && (*p < 127) && isxdigit((uint8_t)*p)) // Compare to < 127 because ctype functions will crash for higher values.
			p++;
		pEnd = p;

		if(pEnd > pBegin + 32) // There can be at most 32 hexadecimal digits in the string.
		{
			pEnd = pBegin + 32;
			p    = pEnd;
		}

		// There can be as many as 32 characters.
		for(int i(0); p > pBegin; i++)
		{
			#if EA_INT128_USE_INT64
				const int nPart = (int)((pEnd - p) / 16);
				uint64_t c = *(--p);
			#else
				const int nPart = (int)((pEnd - p) / 8);
				uint32_t c = *(--p);
			#endif
			
			if(c >= '0' && c <= '9')
				c = (c - '0');
			else if(c >= 'a' && c <= 'f')
				c = 10 + (c - 'a');
			else
				c = 10 + (c - 'A');

			if(c)
			{
				#if EA_INT128_USE_INT64
					c <<= ((i % 16) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
				#else
					c <<= ((i % 8) * 4);

					if(nPart == 0)
						value.mPart0 |= c;
					else if(nPart == 1)
						value.mPart1 |= c;
					else if(nPart == 2)
						value.mPart2 |= c;
					else if(nPart == 3)
						value.mPart3 |= c;
				#endif
			}
		}
	}
	else
	{
		// EA_ASSERT(false); // For the time being, we handle only the above bases.
	}

	if(chSign == '-')
		value.Negate();

	if(ppEnd)
		*ppEnd = (wchar_t*)pEnd;

	return value;
}


///////////////////////////////////////////////////////////////////////////////
// Int128ToStr
//
// Returned string has a NULL appended to it.
// Upon return, ppEnd points to the terminating NULL.
// Thus, ppEnd - pValue => string length.
//
// bPrefix applies only to base 2 (0b) and base 16 (0x).
//
void uint128_t::Int128ToStr(char* pValue, char** ppEnd, int nBase, LeadingZeroes lz, Prefix prefix) const
{
	if(nBase == 2)
	{
		bool bLeadingZeros = (lz == kLZEnable);         // By default leading zeroes are disabled.
		bool bPrefix       = (prefix == kPrefixEnable); // By default prefix is disabled.

		if(bPrefix)
		{
			*pValue++ = '0'; 
			*pValue++ = 'b';
		}

		if(IsZero())
		{
			if(bLeadingZeros)
			{
				for(int i(0); i < 128; i++)
					*pValue++ = '0';
			}
			else
				*pValue++ = '0'; // This is all we need to write.
		}
		else
		{
			// Print out the text.
			bool bNonZeroFound(false);

			for(int i(127); i >= 0; --i)
			{
				const int bBitIsSet(GetBit(i));
				if(bBitIsSet)
					bNonZeroFound = true;
				if(bLeadingZeros || bNonZeroFound)
					*pValue++ = (bBitIsSet ? '1' : '0');
			}
		}
	}
	else if(nBase == 10)
	{
		// To do: Support leading zeroes and prefix for base 10.

		uint128_t  value(*this);
		char*      pValueInitial = pValue;

		// This part here isn't particularly fast.
		const uint128_t ten((uint32_t)10);

		while (value >= ten)
		{
			const uint128_t remainder = (value % ten);
			*pValue++ = (char)('0' + remainder.mPart0);
			value /= (uint32_t)10;
		}

		*pValue++ = (char)('0' + value.mPart0);

		// Reverse the string.
		char* pEnd = pValue - 1;

		while(pValueInitial < pEnd)
		{
			char temp      = *pValueInitial;
			*pValueInitial = *pEnd;
			*pEnd          = temp;
			++pValueInitial;
			--pEnd;
		}
	}
	else if(nBase == 16)
	{
		bool bLeadingZeros = (lz != kLZDisable);         // By default leading zeroes are enabled.
		bool bPrefix       = (prefix != kPrefixDisable); // By default prefix is enabled.

		static const char* const pHexCharTable = "0123456789abcdef";

		if(bPrefix)
		{
			*pValue++ = '0'; 
			*pValue++ = 'x';
		}

		if(IsZero())
		{
			if(bLeadingZeros)
			{
				for(int i(0); i < 32; i++) // 32 is equal to (128 / 16)
					*pValue++ = '0';
			}
			else
				*pValue++ = '0'; // This is all we need to write.
		}
		else
		{
			// Print out the text.
			bool bNonZeroFound(false);

			// Work on each part in turn, starting with the high part.
			#if EA_INT128_USE_INT64
				for(int i(1); i >= 0; --i)
				{
					const uint64_t* pCurrent;

					if(i == 1)
						pCurrent = &mPart1;
					else
						pCurrent = &mPart0;

					// Work on each sub-part (4 bits) or the current part (64 bits), starting with the high sub-part.
					for(int j(60); j >= 0; j -= 4)
					{
						const char c = pHexCharTable[(*pCurrent >> j) & 0x0F];

						if(c != '0')
							bNonZeroFound = true;
						if(bLeadingZeros || bNonZeroFound)
							*pValue++ = c;
					}
				}
			#else
				for(int i(3); i >= 0; --i)
				{
					const uint32_t* pCurrent;

					if(i == 3)
						pCurrent = &mPart3;
					else if(i == 2)
						pCurrent = &mPart2;
					else if(i == 1)
						pCurrent = &mPart1;
					else
						pCurrent = &mPart0;

					// Work on each sub-part (4 bits) or the current part (32 bits), starting with the high sub-part.
					for(int j(28); j >= 0; j -= 4)
					{
						const char c = pHexCharTable[(*pCurrent >> j) & 0x0F];

						if(c != '0')
							bNonZeroFound = true;
						if(bLeadingZeros || bNonZeroFound)
							*pValue++ = c;
					}
				}
			#endif
		}
	}
	else
	{
		// To do: Implement this in a generic way.
		EA_FAIL(); // Base not supported.
	}

	if(ppEnd)
		*ppEnd = pValue;

	*pValue++ = 0;
}


void uint128_t::Int128ToStr(wchar_t* pValue, wchar_t** ppEnd, int nBase, LeadingZeroes lz, Prefix prefix) const
{
	char  str8[130];
	char* pEnd = str8;

	Int128ToStr(str8, &pEnd, nBase, lz, prefix);

	for(char* p = str8; p < pEnd;)
		*pValue++ = (wchar_t)(uint8_t)*p++;

	if(ppEnd)
		*ppEnd = pValue;

	*pValue = 0;
}


} // namespace StdC
} // namespace EA


#ifdef _MSC_VER
	#pragma warning(pop)
#endif










