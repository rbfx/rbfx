///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Win32 implementation of fast, specialized scalar math primitives
///////////////////////////////////////////////////////////////////////////////


// EAMathHelp.h (or possibly the build file) would have set none or one of the 
// following (usually none). If none were defined then we auto-detect.
#if !defined(EAMATHHELP_MODE_SSE) && !defined(EAMATHHELP_MODE_X86ASM) && !defined(EAMATHHELP_MODE_REFERENCE)
	#if !defined(EAMATHHELP_MODE_SSE) && defined(EA_PROCESSOR_X86_64) && defined(EA_COMPILER_MSVC)
		#define EAMATHHELP_MODE_SSE 1
	#elif !defined(EAMATHHELP_MODE_X86ASM) && defined(EA_PROCESSOR_X86) && defined(EA_ASM_STYLE_INTEL)
		#define EAMATHHELP_MODE_X86ASM 1
	#else
		#define EAMATHHELP_MODE_REFERENCE 1
	#endif
#endif

#if EAMATHHELP_MODE_SSE
	#pragma warning(push, 0)
	#include <xmmintrin.h>
	#pragma warning(pop)
#endif



namespace EA
{
namespace StdC
{

	#if EAMATHHELP_MODE_SSE

		inline uint32_t RoundToUint32(float32_t fValue) {
			// We accomplish unsigned 32-bit conversion here by wrapping the
			// value to [-2^31, 2^31] before doing a signed 32-bit conversion.
			// This is necessary because SSE cannot do unsigned 32-bit itof
			// conversion and cannot store 64-bit values like x87 can.

			const __m128 fValue128 = _mm_set_ss(fValue);
			return (uint32_t)_mm_cvtss_si32(
								_mm_add_ss(fValue128,
									_mm_and_ps(
										_mm_cmpgt_ss(
											fValue128,
											_mm_set_ss(2147483648.0f)
										),
										_mm_set_ss(-4294967296.0f)
									)
								)
							);
		}

		inline int32_t RoundToInt32(float32_t fValue) {
			return _mm_cvtss_si32(_mm_set_ss(fValue));
		}

		inline int32_t FloorToInt32(float32_t fValue) {
			__m128 fValue128 = _mm_set_ss(fValue);
			int32_t iValue = _mm_cvtss_si32(fValue128);
			int32_t correction = _mm_cvtss_si32(            // correction = iValue > fValue128 ? -1.0f : 0.0f
									_mm_and_ps(
										_mm_cmplt_ss(
											fValue128,
											_mm_cvtsi32_ss(_mm_setzero_ps(), iValue)
										),
										_mm_set_ss(-1.0f)
									)
								);

			return iValue + correction;
		}

		inline int32_t CeilToInt32(float32_t fValue) {
			__m128 fValue128 = _mm_set_ss(fValue);
			int32_t iValue = _mm_cvtss_si32(fValue128);
			int32_t correction = _mm_cvtss_si32(            // correction = iValue < fValue128 ? +1.0f : 0.0f
									_mm_and_ps(
										_mm_cmplt_ss(
											_mm_cvtsi32_ss(_mm_setzero_ps(), iValue),
											fValue128
										),
										_mm_set_ss(+1.0f)
									)
								);

			return iValue + correction;
		}

		// This shouldn't be necessary with /arch:SSE, but if VC7.1 is doing calcs
		// in x87 -- which it will sometimes do if the SSE ops or data types are
		// too awkward for the expression -- it will resort to good old slow-as-@&$
		// fldcw + fistp.
		inline int32_t TruncateToInt32(float32_t fValue) {
			#if (defined(_MSC_VER) && (_MSC_VER < 1500)) || !EA_SSE2  // If using VC++ < VS2008 or if SSE2+ is not available...
				return _mm_cvttss_si32(_mm_set_ss(fValue));
			#else
				return (int32_t)fValue;
			#endif
		}

		// This function is deprecated, as it's not very useful any more.
		inline int32_t FastRoundToInt23(float32_t fValue) {
			return _mm_cvtss_si32(_mm_set_ss(fValue));
		}

		inline uint8_t UnitFloatToUint8(float fValue) {
			return (uint8_t)_mm_cvtss_si32(_mm_set_ss(fValue * 255.0f));
		}

		inline uint8_t ClampUnitFloatToUint8(float fValue) {
			return (uint8_t)_mm_cvtss_si32(
								_mm_max_ss(
									_mm_min_ss(
										_mm_set_ss(fValue * 255.0f),
										_mm_set_ss(255.0f)
									),
									_mm_set_ss(0.0f)
								)
							);
		}

	#elif EAMATHHELP_MODE_X86ASM

		inline uint32_t RoundToUint32(float32_t fValue) {
			EA_PREFIX_ALIGN(8) int64_t v;
			__asm {
				fld fValue
				fistp v
			}
			return (uint32_t)v;
		}

		inline int32_t RoundToInt32(float32_t fValue) {
			int32_t iv;
			__asm {
				fld fValue
				fistp iv
			}
			return iv;
		}

		inline int32_t FloorToInt32(float32_t fValue) {
			int32_t iv, correct;

			__asm {
				fld     fValue              ;load fp value
				fist    iv                  ;store as integer (rounded to nearest even)
				fild    iv                  ;reload rounded value
				fsub                        ;compute v - round(v)
				fstp    correct             ;store v - round(v)
				cmp     correct, 80000001h  ;set carry if (v >= round(v))   (watch for -0!)
				mov     eax, iv             ;load rounded value
				adc     eax, -1             ;subtract 1 if v < round(v)
			}
		}

		inline int32_t CeilToInt32(float32_t fValue) {
			int32_t iv, correct;

			__asm {
				fld     fValue              ;load fp value
				fist    iv                  ;store as integer (rounded to nearest even)
				fild    iv                  ;reload rounded value
				fsubr                       ;compute round(v) - v
				fstp    correct             ;store round(v) - v
				cmp     correct, 80000001h  ;set carry if (round(v) >= v)   (watch for -0!)
				mov     eax, iv             ;load rounded value
				sbb     eax, -1             ;add 1 if round(v) < v
			}
		}

		// This function is not much faster than VC7.1's improved _ftol()
		// and is actually slower than fldcw on a Pentium 4, but is likely
		// to be faster on a PIII, which does not cache fpucw changes.
		inline int32_t TruncateToInt32(float32_t fValue) {
			int32_t iv, correct;

			__asm {
				fld     fValue              ;load fp value
				fabs                        ;compute |value|
				fist    iv                  ;store |value| as integer using round-to-nearest-even
				mov     eax, fValue         ;load fp value as integer
				cdq                         ;extract sign bit
				fild    iv                  ;load rounded fp value
				fsub                        ;compute |v| - round(|v|)
				fstp    correct             ;store |v| - round(|v|)
				mov     eax, iv             ;load rounded value
				cmp     correct, 80000001h  ;set carry if (v >= round(|v|))   (watch for -0!)
				adc     eax, -1             ;add 1 if |v| < round(|v|)
				xor     eax, edx            ;compute ~round(|v|) if v<0
				sub     eax, edx            ;compute ~round(|v|)+1 == -round(|v|) if v<0
			}
		}


		// The FastRoundToInt23() and UnitFloatToUint8() functions are somewhat fast
		// by themselves, but what really makes them shine is the integer addition
		// that can easily be folded into other additions done on the value.
		// In particular, a table lookup on the result is likely to have the bias
		// subtracted incorporated into the address displacement for free on x86.

		// This function is deprecated, as it's not very useful any more.
		inline int32_t FastRoundToInt23(float32_t fValue) {
			const union {
				float32_t   f;
				int32_t i;
			} converter = { fValue + kFToIBiasF32 };

			return converter.i - kFToIBiasS32;
		}

		inline uint8_t UnitFloatToUint8(float fValue) {
			const union {
				float32_t   f;
				int32_t i;
			} converter = { fValue * (255.0f/256.0f) + kFToI8BiasF32 };

			return (uint8_t)(int8_t)(converter.i - kFToI8BiasS32);
		}


		inline uint8_t ClampUnitFloatToUint8(float fValue) {
			const union {
				float32_t   f;
				int32_t     i;
			} converter = { kFToI8BiasF32 + fValue * (255.0f/256.0f) };

			const int32_t i  = converter.i - kFToI8BiasS32;

			// ~(i>>31)    == (i<0  ) ? 0 : 0xFFFFFFFF    (low clamp)
			// (255-i)>>31 == (i>255) ? 0xFFFFFFFF : 0    (high clamp)

			return uint8_t((i & ~(i>>31)) | ((255-i)>>31));
		}

	#endif


} // namespace StdC
} // namespace EA













