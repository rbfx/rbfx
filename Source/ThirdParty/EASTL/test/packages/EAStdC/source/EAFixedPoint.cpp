///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// See the header file for documentation and example usage.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAFixedPoint.h>
#include <EAAssert/eaassert.h>



namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedMul
//
// returns a*b
//
#ifdef FP_USE_INTEL_ASM
	EASTDC_API __declspec(naked)
	int32_t __stdcall SFixed16::FixedMul(const int32_t /*a*/, const int32_t /*b*/)
	{ 
		__asm{                   // VC++ compiler will choke if you put ";" comments with a templated inline assembly function.
			mov  eax, [esp+4]    // Move a into eax
			mov  ecx, [esp+8]    // Move b into ecx
			imul ecx             // Multiply by b, storing the result in edx:eax
			shrd eax, edx, 16    // Do shift and leave result in eax. 
			ret  8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedMul(const int32_t a, const int32_t b)
	{
		const int64_t c = (int64_t)a * b;
		return (int32_t)(c >> 16);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedMul
//
// returns a*b
//
#ifdef FP_USE_INTEL_ASM
	EASTDC_API __declspec(naked)
	uint32_t __stdcall UFixed16::FixedMul(const uint32_t /*a*/, const uint32_t /*b*/)
	{ 
		__asm{                  // VC++ compiler will choke if you put ";" comments with a templated inline assembly function.
			mov  eax, [esp+4]   // Move a into eax
			mov  ecx, [esp+8]   // Move b into ecx
			mul  ecx            // Multiply by b, storing the result in edx:eax
			shrd eax, edx, 16   // Do shift and leave result in eax. 
			ret  8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedMul(const uint32_t a, const uint32_t b)
	{
		const uint64_t c = (uint64_t)a * b;
		return (uint32_t)(c >> 16);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedDiv
//
// returns a/b
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t FP_PASCAL SFixed16::FixedDiv(const int32_t /*a*/, const int32_t /*b*/)
	{ 
		__asm{
			mov   eax, [esp+4]  // Prepare the number for division
			rol   eax, 16       // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax       // Put the integer part in edx
			xor   ax,  ax       // Clear the integer part from eax 
			mov   ecx, [esp+8]  // Move b into ecx
			idiv  ecx           // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedDiv(const int32_t a, const int32_t b)
	{
		const int64_t c = ((uint64_t)a) << 16;
		return (int32_t)(c / b);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedDiv
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedDiv(const uint32_t /*a*/, const uint32_t /*b*/)
	{ 
		__asm{
			mov   eax, [esp+4]  // Prepare the number for division
			rol   eax, 16       // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax       // Put the integer part in edx
			xor   ax,  ax       // Clear the integer part from eax 
			mov   ecx, [esp+8]  // Move b into ecx
			div   ecx           // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedDiv(const uint32_t a, const uint32_t b)
	{
		const uint64_t c = ((uint64_t)a) << 16;
		return (uint32_t)(c / b);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedDivSafe
//
// returns a/b. Checks for overflow and returns max value if so, instead of bombing.
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t __stdcall SFixed16::FixedDivSafe(const int32_t /*a*/, const int32_t /*b*/)
	{ 
		__asm{
			mov   ecx, [esp+8]      // Move b into ecx.
			or    ecx, ecx          // Test to see if equal to zero.
			je    NOT_OK            // if not OK
			mov   eax, [esp+4]      // Put a into eax.
			xor   edx, edx          // Clear out edx, since we'll be shifting into it.
			shld  edx,eax,16        // double fixed
			cmp   edx, ecx          // do a test for overflow.
			jge   NOT_OK            // If edx is greater than ecx, then we will overflow on the divide. 
			shl   eax,16            // double fixed
			idiv  ecx               // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			ret   8
			NOT_OK:
			mov   eax, 0x7FFFFFFF   // put in max value
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedDivSafe(const int32_t a, const int32_t b)
	{
		if(b)
		{
			const int64_t c = ((int64_t)a) << 16;
			return (int32_t)(c / b);
		}
		return INT32_MAX;
	}
#endif



///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedDivSafe
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedDivSafe(const uint32_t /*a*/, const uint32_t /*b*/)
	{ 
		__asm{
			mov   ecx, [esp+8]  // Move b into ecx.
			or    ecx, ecx      // Test to see if equal to zero.
			je    NOT_OK        // if not OK
			mov   eax, [esp+4]  // Put a into eax.
			xor   edx, edx      // Clear out edx, since we'll be shifting into it.
			shld  edx,eax,16    // double fixed
			cmp   edx, ecx      // do a test for overflow.
			jae   NOT_OK        // If edx is greater than ecx, then we will overflow on the divide. 
			shl   eax,16        // double fixed
			div   ecx           // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			ret   8             // return
			NOT_OK:
			mov   eax, 0xFFFFFFFF // put in max value
			ret   8
		}
		#ifdef EA_COMPILER_INTEL //Intel C++
			return 0; //Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedDivSafe(const uint32_t a, const uint32_t b)
	{
		if(b)
		{
			const uint64_t c = ((uint64_t)a) << 16;
			return (uint32_t)(c / b);
		}
		return UINT32_MAX;
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedMulDiv
//
// returns a*b/c Faster than separate mul and div.
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t __stdcall SFixed16::FixedMulDiv(const int32_t /*a*/, const int32_t /*b*/, const int32_t /*c*/)
	{
		__asm{
			mov  eax, [esp+4]  // First set up the multiply. Put a into eax.
			mov  ecx, [esp+8]  // Put b into ecx.
			imul ecx           // Multiply. The result is in edx:eax.
			mov  ecx, [esp+12] // Put the denominator into ecx.
			idiv ecx           // Divide, leaving the result in eax and remainder in edx.
			ret  12
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedMulDiv(const int32_t /*a*/, const int32_t /*b*/, const int32_t /*c*/)
	{
		// EA_ASSERT(false); // This is not yet finished.
		return 0;
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedMulDiv
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedMulDiv(const uint32_t /*a*/, const uint32_t /*b*/, const uint32_t /*c*/)
	{
		__asm{
			mov  eax, [esp+4]  // First set up the multiply. Put a into eax.
			mov  ecx, [esp+8]  // Put b into ecx.
			mul  ecx           // Multiply. The result is in edx:eax.
			mov  ecx, [esp+12] // Put the denominator into ecx.
			div  ecx           // Divide, leaving the result in eax and remainder in edx.
			ret  12
		}
		#ifdef EA_COMPILER_INTEL //Intel C++
			return 0; //Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedMulDiv(const uint32_t /*a*/, const uint32_t /*b*/, const uint32_t /*c*/)
	{
		// EA_ASSERT(false); // This is not yet finished.
		return 0;
	}
#endif



///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedMulDivSafe
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t __stdcall SFixed16::FixedMulDivSafe(const int32_t /*a*/, const int32_t /*b*/, const int32_t /*c*/)
	{
		__asm{
			mov  ecx, [esp+12]  // Move b into ecx.
			or   ecx, ecx       // Test to see if equal to zero.
			je   NOT_OK         // if not OK
			mov  eax, [esp+4]   // 
			mov  ebx, [esp+8]   // 
			imul ebx            // 
			cmp  edx, ecx       // do a test for overflow.
			jge  NOT_OK         // If edx is greater than ecx, then we will overflow on the divide. 
			idiv ecx            // Result is in eax.
			ret  12
			NOT_OK:
			mov  eax, 0x7FFFFFFF // put in max value
			ret  12
		}
		#ifdef EA_COMPILER_INTEL //Intel C++
			return 0; //Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedMulDivSafe(const int32_t /*a*/, const int32_t /*b*/, const int32_t /*c*/)
	{
		// EA_ASSERT(false); // This is not yet finished.
		return 0;
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedMulDivSafe
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedMulDivSafe(const uint32_t /*a*/, const uint32_t /*b*/, const uint32_t /*c*/)
	{
		__asm{
			mov  ecx, [esp+12]    // Move b into ecx.
			or   ecx, ecx         // Test to see if equal to zero.
			je   NOT_OK           // if not OK
			mov  eax, [esp+4]     // 
			mov  ebx, [esp+8]     // 
			mul  ebx              // 
			cmp  edx, ecx         // do a test for overflow.
			jae  NOT_OK           // If edx is greater than ecx, then we will overflow on the divide. 
			div  ecx              // Result is in eax.
			ret  12
			NOT_OK:
			mov  eax, 0xFFFFFFFF  // put in max value
			ret  12
		}
		#ifdef EA_COMPILER_INTEL //Intel C++
			return 0; //Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedMulDivSafe(const uint32_t /*a*/, const uint32_t /*b*/, const uint32_t /*c*/)
	{
		// EA_ASSERT(false); // This is not yet finished.
		return 0;
	}
#endif



///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedMod
//
// returns a%b
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t __stdcall SFixed16::FixedMod(const int32_t /*a*/, const int32_t /*b*/)
	{ 
		__asm{
			mov   eax, [esp+4]      // Prepare the number for division
			rol   eax, 16           // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax           // Put the integer part in edx
			xor   ax, ax            // Clear the integer part from eax 
			mov   ecx, [esp+8]      // Move b into ecx
			idiv  ecx               // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			mov   eax, edx          // The remainder after idiv is in edx. So we move it to eax before the return.
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedMod(const int32_t a, const int32_t b)
	{
		const uint64_t c = ((uint64_t)a) << 16;
		return (int32_t)(c % b);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedMod
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedMod(const uint32_t /*a*/, const uint32_t /*b*/)
	{ 
		__asm{
			mov   eax, [esp+4]      // Prepare the number for division
			rol   eax, 16           // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax           // Put the integer part in edx
			xor   ax,  ax           // Clear the integer part from eax 
			mov   ecx, [esp+8]      // Move b into ecx
			div   ecx               // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			mov   eax, edx          // The remainder after div is in edx. So we move it to eax before the return.
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedMod(const uint32_t a, const uint32_t b)
	{
		const uint64_t c = ((uint64_t)a) << 16;
		return (uint32_t)(c % b);
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// SFixed16::FixedModSafe
//
// returns a%b
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	int32_t __stdcall SFixed16::FixedModSafe(const int32_t /*a*/, const int32_t /*b*/)
	{ 
		__asm{
			mov   ecx, [esp+8]  // Move b into ecx
			or    ecx, ecx      // Test to see if equal to zero.
			je    NOT_OK        // if not OK
			mov   eax, [esp+4]  // Prepare the number for division
			rol   eax, 16       // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax       // Put the integer part in edx
			xor   ax,  ax       // Clear the integer part from eax 
			cmp   edx, ecx      // do a test for overflow.
			jge   NOT_OK        // If edx is greater than ecx, then we will overflow on the divide. 
			div   ecx           // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			mov   eax, edx      // The remainder after idiv is in edx. So we move it to eax before the return.
			ret   8
			NOT_OK:
			mov   eax, 0xFFFFFFFF // put in max value
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	int32_t FP_PASCAL SFixed16::FixedModSafe(const int32_t a, const int32_t b)
	{
		if(b)
		{
			const uint64_t c = ((uint64_t)a) << 16;
			return (int32_t)(c % b);
		}
		return INT32_MAX;
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// UFixed16::FixedModSafe
//
#ifdef FP_USE_INTEL_ASM
	__declspec(naked)
	uint32_t __stdcall UFixed16::FixedModSafe(const uint32_t /*a*/, const uint32_t /*b*/)
	{ 
		__asm{
			mov   ecx, [esp+8]      // Move b into ecx
			or    ecx, ecx          // Test to see if equal to zero.
			je    NOT_OK            // if not OK
			mov   eax, [esp+4]      // Prepare the number for division
			rol   eax, 16           // Put the fractional part of the number in the top half of eax, rotating the top part to the bottom part for later use
			movsx edx, ax           // Put the integer part in edx
			xor   ax,  ax           // Clear the integer part from eax 
			cmp   edx, ecx          // do a test for overflow.
			jae   NOT_OK            // If edx is greater than ecx, then we will overflow on the divide. 
			div   ecx               // Divide and store. Divide the signed 64 bit value in edx:eax by the 32 bit value in ecx.
			mov   eax, edx          // The remainder after div is in edx. So we move it to eax before the return.
			ret   8
			NOT_OK:
			mov   eax, 0xFFFFFFFF   // put in max value
			ret   8
		}
		#ifdef EA_COMPILER_INTEL // Intel C++
			return 0; // Just to get the compiler to shut up.
		#endif
	}
#else
	template<> EASTDC_API
	uint32_t FP_PASCAL UFixed16::FixedModSafe(const uint32_t a, const uint32_t b)
	{
		if(b)
		{
			const uint64_t c = ((uint64_t)a) << 16;
			return (uint32_t)(c % b);
		}
		return UINT32_MAX;
	}
#endif


} // namespace StdC
} // namespace EA


// For unity build friendliness, undef all local #defines.
#undef FP_USE_INTEL_ASM









