/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __DGTYPES_H__
#define __DGTYPES_H__

#ifdef _MSC_VER 
	#ifdef _M_X64
		#ifndef _WIN_64_VER
			#define _WIN_64_VER
		#endif
	#else 		
		#ifndef _WIN_32_VER
			#define _WIN_32_VER
		#endif
	#endif

#if _MSC_VER >= 1400
	#define HAVE_STRUCT_TIMESPEC
#endif
#endif


#ifdef _MSC_VER 
	#if !(defined (_WIN_32_VER) || defined (_WIN_64_VER))
		#error "most define _WIN_32_VER or _WIN_64_VER for a correct CPU target"
	#endif
#endif

#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#pragma warning (disable: 4100) //unreferenced formal parameter
	#pragma warning (disable: 4201) //nonstandard extension used : nameless struct/union
	#pragma warning (disable: 4324) //structure was padded due to __declspec(align())
	#pragma warning (disable: 4514) //unreferenced inline function has been removed
	#pragma warning (disable: 4530) //C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning (disable: 4625) //copy constructor could not be generated because a base class copy constructor is inaccessible or deleted
	#pragma warning (disable: 4626) //assignment operator could not be generated because a base class assignment operator is inaccessible or deleted
	#pragma warning (disable: 4640) //construction of local static object is not thread-safe
	#pragma warning (disable: 4820) //bytes padding added after data member
	#pragma warning (disable: 4005) //'__useHeader': macro redefinition
	//#pragma warning (disable: 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
    
	#include <io.h> 
	#include <direct.h> 
	#include <malloc.h>
	#include <float.h>
	#include <stdarg.h>
	#include <process.h>

	#ifdef _DEBUG
		#pragma warning (disable: 4127)	//conditional expression is constant
	#endif

	#pragma warning (push, 3) 
		#include <windows.h>
		#include <crtdbg.h>
		#ifndef _DURANGO
			#include <tlhelp32.h>
		#endif
	#pragma warning (pop) 
#endif

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
//#include <atomic>

#if (defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
	#include <io.h> 
	#include <direct.h> 
	#include <malloc.h>
	#include <float.h>
	#include <windows.h>
	#include <process.h>
#endif

#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#include <intrin.h>
	#include <emmintrin.h> 
	#include <pmmintrin.h>
#endif

#if (defined (_POSIX_VER) || defined (_POSIX_VER_64) || defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
  /* CMake defines NDEBUG for _not_ debug builds. Therefore, set
     Newton's _DEBUG flag only when NDEBUG is not defined.
  */
  #ifndef NDEBUG
    #define _DEBUG 1
  #endif

	#include <unistd.h>
	#include <assert.h>
	#ifndef __ARMCC_VERSION
		extern "C" 
		{ 
			// for SSE3 and up
			#include <pmmintrin.h> 
			#include <emmintrin.h> 
			#include <mmintrin.h> 
		} 
	#endif
#endif

#ifdef _MACOSX_VER
	#include <unistd.h>
	#include <sys/sysctl.h>
    #include <assert.h> 
    #if (defined __i386__ || defined __x86_64__)
		#include <fenv.h>
		#include <pmmintrin.h> 
		#include <emmintrin.h>  //sse3
        #include <mmintrin.h> 
    #endif
#endif

//#define DG_SCALAR_VECTOR_CLASS


// by default newton run on a separate thread and 
// optionally concurrent with the calling thread,
// it also uses a thread job pool for multi core systems.
// define DG_USE_THREAD_EMULATION on the command line for 
// platform that do not support hardware multi threading or 
// if the and application want to control threading at the application level 
//#define DG_USE_THREAD_EMULATION

#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#if _MSC_VER < 1700
		#ifndef DG_USE_THREAD_EMULATION
			#define DG_USE_THREAD_EMULATION
		#endif
	#endif
#endif

#ifndef DG_USE_THREAD_EMULATION
	#include <mutex>
	#include <thread>
	#include <condition_variable>
#endif

#if defined (_DG_USE_PROFILER) && (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#include <dTimeTracker.h>
#endif


//************************************************************
#ifdef DG_DISABLE_ASSERT
	#define dgAssert(x)
#else 
	#if defined (_WIN_32_VER) || defined (_WIN_64_VER)
		#define dgAssert(x) _ASSERTE(x)
	#else 
		#ifdef _DEBUG
			#define dgAssert(x) assert(x)
		#else 
			#define dgAssert(x)
		#endif
	#endif
#endif


#define	DG_MAX_THREADS_HIVE_COUNT		16

#ifdef _DEBUG
//#define __ENABLE_DG_CONTAINERS_SANITY_CHECK 
#endif


#ifdef DLL_DECLSPEC
#undef DLL_DECLSPEC
#endif

#ifdef _DEBUG
	#define DG_INLINE inline
#else 
	#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
		#define DG_INLINE __forceinline 
	#else 
		#define DG_INLINE	inline
		//#define DG_INLINE	 __attribute__((always_inline))
	#endif
#endif


#define DG_VECTOR_SIMD_SIZE		16
#define DG_VECTOR_AVX2_SIZE		32

#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#define	DG_GCC_VECTOR_ALIGMENT	
	#define	DG_MSC_VECTOR_ALIGMENT			__declspec(align(DG_VECTOR_SIMD_SIZE))
#else
	#define	DG_GCC_VECTOR_ALIGMENT			__attribute__ ((aligned (DG_VECTOR_SIMD_SIZE)))
	#define	DG_MSC_VECTOR_ALIGMENT			
#endif

#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#define	DG_GCC_AVX_ALIGMENT	
	#define	DG_MSC_AVX_ALIGMENT			__declspec(align(DG_VECTOR_AVX2_SIZE))
#else
	#define	DG_GCC_AVX_ALIGMENT			__attribute__ ((aligned (DG_VECTOR_AVX2_SIZE)))
	#define	DG_MSC_AVX_ALIGMENT			
#endif



#if ((defined (_WIN_32_VER) || defined (_WIN_64_VER)) && (_MSC_VER  >= 1600))
	#include <stdint.h>
	typedef int8_t dgInt8;
	typedef uint8_t dgUnsigned8;

	typedef int16_t dgInt16;
	typedef uint16_t dgUnsigned16;

	typedef int32_t dgInt32;
	typedef uint32_t dgUnsigned32;

	typedef int64_t dgInt64;
	typedef uint64_t dgUnsigned64;
#else
	typedef char dgInt8;
	typedef unsigned char dgUnsigned8;

	typedef short dgInt16;
	typedef unsigned short dgUnsigned16;

	typedef int dgInt32;
	typedef unsigned dgUnsigned32;
	typedef unsigned int dgUnsigned32;

	typedef long long dgInt64;
	typedef unsigned long long dgUnsigned64;
	typedef double dgFloat64;
#endif


typedef double dgFloat64;

#ifdef _NEWTON_USE_DOUBLE
	typedef double dgFloat32;
#else
	typedef float dgFloat32;
#endif


class dgTriplex
{
	public:
	dgFloat32 m_x;
	dgFloat32 m_y;
	dgFloat32 m_z;
};


#define dgPI			 	dgFloat32 (3.141592f)
#define dgPI2			 	dgFloat32 (dgPI * 2.0f)
#define dgEXP			 	dgFloat32 (2.71828f)
#define dgEPSILON	  	 	dgFloat32 (1.0e-5f)
#define dgGRAVITY	  	 	dgFloat32 (9.8f)
#define dgDEG2RAD	  	 	dgFloat32 (dgPI / 180.0f)
#define dgRAD2DEG	  	 	dgFloat32 (180.0f / dgPI)
#define dgKMH2MPSEC		 	dgFloat32 (0.278f)


class dgBigVector;
#ifndef _NEWTON_USE_DOUBLE
class dgVector;
#endif 


#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#define dgApi __cdecl 	
	#define dgStdApi __stdcall 	
#else
	#define dgApi 	
	#define dgStdApi
#endif



#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
	#define dgCheckFloat(x) (_finite(x) && !_isnan(x))
//	#define dgCheckFloat(x) 1
#else
	#define dgCheckFloat(x) (isfinite(x) && !isnan(x))
//		#define dgCheckFloat(x) 1
#endif

// adding frame capture profiler macros
#include "dgProfiler.h"

typedef void (dgApi *dgDeserialize) (void* const userData, void* buffer, dgInt32 size);
typedef void (dgApi *dgSerialize) (void* const userData, const void* const buffer, dgInt32 size);
typedef bool (dgApi *dgReportProgress) (dgFloat32 progressNormalzedPercent, void* const userData);

// assume this function returns memory aligned to 16 bytes
#define dgAlloca(type, count) (type*) alloca (sizeof (type) * (count))

//#define dgCheckAligment(x) dgAssert (!(dgUnsigned64 (x) & 0xf))
#define dgCheckAligment(x) 

DG_INLINE dgInt32 dgExp2 (dgInt32 x)
{
	dgInt32 exp;
	for (exp = -1; x; x >>= 1) {
		exp ++;
	}
	return exp;
}

DG_INLINE dgInt32 dgBitReversal(dgInt32 v, dgInt32 base)
{
	dgInt32 x = 0;
	dgInt32 power = dgExp2 (base) - 1;
	do {
		x += (v & 1) << power;
		v >>= 1;
		power--;
	} while (v);
	dgAssert(x < base);
	return x;
}


template <class T> 
DG_INLINE T dgMin(T A, T B)
{
	return (A < B) ? A : B; 
}

template <class T> 
DG_INLINE T dgMax(T A, T B)
{
	return (A > B) ? A : B; 
}

template <class T>
DG_INLINE T dgMin(T A, T B, T C)
{
	return dgMin(dgMin (A, B), C);
}

template <class T>
DG_INLINE T dgMax(T A, T B, T C)
{
	return dgMax(dgMax (A, B), C);
}

template <class T>
DG_INLINE T dgClamp(T val, T min, T max)
{
	return dgMax (min, dgMin (max, val));
}

template <class T> 
DG_INLINE void dgSwap(T& A, T& B)
{
	T tmp (A);
	A = B;
	B = tmp;
}	

template <class T>
DG_INLINE T dgAbs(T A)
{
	// according to Intel this is better because is does not read after write
	return (A >= T(0)) ? A : -A;
}

template <class T>
DG_INLINE T dgSign(T A)
{
	return (A >= T(0)) ? T(1) : T(-1);
}

template <class T> 
DG_INLINE bool dgAreEqual(T A, T B, T tol)
{
	if ((dgAbs(A) < tol) && (dgAbs(B) < tol)) {
		return true;
	}
/*
	dgInt32 exp0;
	dgFloat64 mantissa0 = frexp(dgFloat64 (A), &exp0);

	dgInt32 exp1;
	dgFloat64 mantissa1 = frexp(dgFloat64(B), &exp1);

	if ((exp0 < -12) && (exp1 < -12)) {
		return true;
	}

	if (exp0 != exp1) {
		return false;
	}
	return dgAbs(mantissa0 - mantissa1) < tol;
*/	
	T den = dgMax(dgAbs(A), dgAbs(B)) + tol;
	A /= den;
	B /= den;
	return dgAbs(A - B) < tol;
}

template <class T> 
dgInt32 dgBinarySearch (T const* array, dgInt32 elements, const T& entry, dgInt32 (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	dgInt32 index0 = 0;
	dgInt32 index2 = elements - 1;

	while ((index2 - index0) > 4) {
		dgInt32 index1 = (index0 + index2) >> 1;
		dgInt32 test = compare (&array[index1], &entry, context);
		if (test < 0) {
			index0 = index1;
		} else {
			index2 = index1;
		}
	}
	
	index0 = (index0 > 0) ? index0 - 1 : 0;
	index2 = ((index2 + 1) < elements) ? index2 + 1 : elements;
	dgInt32 index = index0 - 1;
	for (dgInt32 i = index0; i < index2; i ++) {
		dgInt32 test = compare (&array[i], &entry, context);
		if (!test) {
			return i;
		} else if (test > 0) {
			break;
		}
		index = i;
	}
	return index;
}


template <class T>
dgInt32 dgBinarySearchIndirect(T** const array, dgInt32 elements, const T& entry, dgInt32(*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	dgInt32 index0 = 0;
	dgInt32 index2 = elements - 1;

	while ((index2 - index0) > 4) {
		dgInt32 index1 = (index0 + index2) >> 1;
		dgInt32 test = compare(array[index1], &entry, context);
		if (test < 0) {
			index0 = index1;
		} else {
			index2 = index1;
		}
	}

	index0 = (index0 > 0) ? index0 - 1 : 0;
	index2 = ((index2 + 1) < elements) ? index2 + 1 : elements;
	dgInt32 index = index0 - 1;
	for (dgInt32 i = index0; i < index2; i++) {
		dgInt32 test = compare(array[i], &entry, context);
		if (!test) {
			return i;
		} else if (test > 0) {
			break;
		}
		index = i;
	}
	return index;
}


template <class T> 
void dgRadixSort (T* const array, T* const tmpArray, dgInt32 elements, dgInt32 radixPass,  dgInt32 (*getRadixKey) (const T* const  A, void* const context), void* const context = NULL)
{
	dgInt32 scanCount[256]; 
	dgInt32 histogram[256][4];

	dgAssert (radixPass >= 1);
	dgAssert (radixPass <= 4);
	
	memset (histogram, 0, sizeof (histogram));
	for (dgInt32 i = 0; i < elements; i ++) {
		dgInt32 key = getRadixKey (&array[i], context);
		for (dgInt32 j = 0; j < radixPass; j ++) {
			dgInt32 radix = (key >> (j << 3)) & 0xff;
			histogram[radix][j] = histogram[radix][j] + 1;
		}
	}

	for (dgInt32 radix = 0; radix < radixPass; radix += 2) {
		scanCount[0] = 0;
		for (dgInt32 i = 1; i < 256; i ++) {
			scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix];
		}
		dgInt32 radixShift = radix << 3;
		for (dgInt32 i = 0; i < elements; i ++) {
			dgInt32 key = (getRadixKey (&array[i], context) >> radixShift) & 0xff;
			dgInt32 index = scanCount[key];
			tmpArray[index] = array[i];
			scanCount[key] = index + 1;
		}

		if ((radix + 1) < radixPass) { 
			scanCount[0] = 0;
			for (dgInt32 i = 1; i < 256; i ++) {
				scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix + 1];
			}
			
			dgInt32 radixShift = (radix + 1) << 3;
			for (dgInt32 i = 0; i < elements; i ++) {
				dgInt32 key = (getRadixKey (&array[i], context) >> radixShift) & 0xff;
				dgInt32 index = scanCount[key];
				array[index] = tmpArray[i];
				scanCount[key] = index + 1;
			}
		} else {
			memcpy (array, tmpArray, elements * sizeof (T)); 
		}
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i ++) {
		dgAssert (getRadixKey (&array[i], context) <= getRadixKey (&array[i + 1], context));
	}
#endif
}

template <class T> 
void dgSort (T* const array, dgInt32 elements, dgInt32 (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
//	DG_TRACKTIME_NAMED("dgSort");
	DG_TRACKTIME(__FUNCTION__);
	const dgInt32 batchSize = 8;
	dgInt32 stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	dgInt32 stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		dgInt32 lo = stack[stackIndex][0];
		dgInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > batchSize) {
			dgInt32 mid = (lo + hi) >> 1;
			if (compare(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			if (compare(&array[mid], &array[hi], context) > 0) {
				dgSwap(array[mid], array[hi]);
			}
			if (compare(&array[lo], &array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			dgInt32 i = lo + 1;
			dgInt32 j = hi - 1;
			T val (array[mid]);
			do {    
				while (compare (&array[i], &val, context) < 0) i ++;
				while (compare (&array[j], &val, context) > 0) j --;

				if (i <= j)	{
					dgSwap(array[i], array[j]);
					i++; 
					j--;
				}
			} while (i <= j);

			if (i < hi) {
				stack[stackIndex][0] = i;
				stack[stackIndex][1] = hi;
				stackIndex ++;
			}
			if (lo < j) {
				stack[stackIndex][0] = lo;
				stack[stackIndex][1] = j;
				stackIndex ++;
			}
			dgAssert (stackIndex < dgInt32 (sizeof (stack) / (2 * sizeof (stack[0][0]))));
		}
	}

	dgInt32 stride = batchSize + 1;
	if (elements < stride) {
		stride = elements;
	}
	for (dgInt32 i = 1; i < stride; i ++) {
		if (compare (&array[0], &array[i], context) > 0) {
			dgSwap(array[0], array[i]);
		}
	}

	for (dgInt32 i = 1; i < elements; i ++) {
		dgInt32 j = i;
		T tmp (array[i]);
		for (; compare (&array[j - 1], &tmp, context) > 0; j --) {
			dgAssert (j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i ++) {
		dgAssert (compare (&array[i], &array[i + 1], context) <= 0);
	}
#endif
}


template <class T> 
void dgSortIndirect (T** const array, dgInt32 elements, dgInt32 (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	DG_TRACKTIME(__FUNCTION__);
	const dgInt32 batchSize = 8;
	dgInt32 stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	dgInt32 stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		dgInt32 lo = stack[stackIndex][0];
		dgInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > batchSize) {
			dgInt32 mid = (lo + hi) >> 1;
			if (compare(array[lo], array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			if (compare(array[mid], array[hi], context) > 0) {
				dgSwap(array[mid], array[hi]);
			}
			if (compare(array[lo], array[mid], context) > 0) {
				dgSwap(array[lo], array[mid]);
			}
			dgInt32 i = lo + 1;
			dgInt32 j = hi - 1;
			T* val (array[mid]);
			do {    
				while (compare (array[i], val, context) < 0) i ++;
				while (compare (array[j], val, context) > 0) j --;

				if (i <= j)	{
					dgSwap(array[i], array[j]);
					i++; 
					j--;
				}
			} while (i <= j);

			if (i < hi) {
				stack[stackIndex][0] = i;
				stack[stackIndex][1] = hi;
				stackIndex ++;
			}
			if (lo < j) {
				stack[stackIndex][0] = lo;
				stack[stackIndex][1] = j;
				stackIndex ++;
			}
			dgAssert (stackIndex < dgInt32 (sizeof (stack) / (2 * sizeof (stack[0][0]))));
		}
	}


	dgInt32 stride = batchSize + 1;
	if (elements < stride) {
		stride = elements;
	}
	for (dgInt32 i = 1; i < stride; i ++) {
		if (compare (array[0], array[i], context) > 0) {
			dgSwap(array[0], array[i]);
		}
	}

	for (dgInt32 i = 1; i < elements; i ++) {
		dgInt32 j = i;
		T* tmp (array[i]);
		for (; compare (array[j - 1], tmp, context) > 0; j --) {
			dgAssert (j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < (elements - 1); i ++) {
		dgAssert (compare (array[i], array[i + 1], context) <= 0);
	}
#endif
}


#ifdef _NEWTON_USE_DOUBLE
	union dgFloatSign
	{
		struct {
			dgInt32 m_dommy;
			dgInt32 m_iVal;
		} m_integer;
		dgFloat64 m_fVal;
	};
#else
	union dgFloatSign
	{
		struct {
			dgInt32 m_iVal;
		} m_integer;
		dgFloat32 m_fVal;
	};
#endif

union dgDoubleInt
{
	struct {
		dgInt32 m_intL;
		dgInt32 m_intH;
	};
	void* m_ptr;
	dgInt64 m_int;
	dgFloat64 m_float;
};


void dgGetMinMax (dgBigVector &Min, dgBigVector &Max, const dgFloat64* const vArray, dgInt32 vCount, dgInt32 strideInBytes);
dgInt32 dgVertexListToIndexList (dgFloat64* const vertexList, dgInt32 strideInBytes, dgInt32 compareCount,     dgInt32 vertexCount,         dgInt32* const indexListOut, dgFloat64 tolerance = dgEPSILON);
dgInt32 dgVertexListToIndexList (dgFloat32* const vertexList, dgInt32 strideInBytes, dgInt32 floatSizeInBytes, dgInt32 unsignedSizeInBytes, dgInt32 vertexCount, dgInt32* const indexListOut, dgFloat32 tolerance = dgEPSILON);

#define PointerToInt(x) ((size_t)x)
#define IntToPointer(x) ((void*)(size_t(x)))


#ifndef _MSC_VER 
	#define _stricmp(x,y) strcasecmp(x,y)
#endif

#define dgSqrt(x)			dgFloat32 (sqrt(x))	
#define dgSin(x)			dgFloat32 (sin(x))
#define dgCos(x)			dgFloat32 (cos(x))
#define dgAsin(x)			dgFloat32 (asin(x))
#define dgAcos(x)			dgFloat32 (acos(x))
#define dgLog(x)			dgFloat32 (log(x))
#define dgCeil(x)			dgFloat32 (ceil(x))
#define dgFloor(x)			dgFloat32 (floor(x))	
#define dgPow(x,y)			dgFloat32 (pow(x,y))
#define dgFmod(x,y)			dgFloat32 (fmod(x,y))
#define dgAtan2(x,y)		dgFloat32 (atan2(x,y))
#define dgRsqrt(x)			(dgFloat32 (1.0f) / dgSqrt(x))
#define dgClearFP()			_clearfp() 
#define dgControlFP(x,y)	_controlfp(x,y)

enum dgSerializeRevisionNumber
{
	m_firstRevision = 100,
	// add new serialization revision number here
	m_currentRevision 
};

dgUnsigned64 dgGetTimeInMicrosenconds();
dgFloat64 dgRoundToFloat(dgFloat64 val);
void dgSerializeMarker(dgSerialize serializeCallback, void* const userData);
dgInt32 dgDeserializeMarker(dgDeserialize serializeCallback, void* const userData);

class dgFloatExceptions
{
	public:
	#ifdef _MSC_VER
		#define DG_FLOAT_EXECTIONS_MASK	(EM_INVALID | EM_DENORMAL | EM_ZERODIVIDE)
	#else 
		#define DG_FLOAT_EXECTIONS_MASK	0
	#endif

	dgFloatExceptions(dgUnsigned32 mask = DG_FLOAT_EXECTIONS_MASK);
	~dgFloatExceptions();

	private:
	dgUnsigned32 m_mask;
};


class dgSetPrecisionDouble 
{
	public:
	dgSetPrecisionDouble();
	~dgSetPrecisionDouble();
	dgInt32 m_mask; 
};

DG_INLINE dgInt32 dgAtomicExchangeAndAdd (dgInt32* const addend, dgInt32 amount)
{
	#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
		return _InterlockedExchangeAdd((long*)addend, long(amount));
	#elif (defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
		return InterlockedExchangeAdd((long*)addend, long(amount));
	#elif (defined (_POSIX_VER) || defined (_POSIX_VER_64) ||defined (_MACOSX_VER)|| defined ANDROID)
		return __sync_fetch_and_add((int32_t*)addend, amount);
	#else
		#error "dgAtomicExchangeAndAdd implementation required"
	#endif
}

DG_INLINE dgInt32 dgInterlockedExchange(dgInt32* const ptr, dgInt32 value)
{
	#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
		return _InterlockedExchange((long*) ptr, value);
	#elif (defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
		return InterlockedExchange((long*) ptr, value);
	#elif (defined (_POSIX_VER) || defined (_POSIX_VER_64) ||defined (_MACOSX_VER))
		//__sync_synchronize();
		return __sync_lock_test_and_set((int32_t*)ptr, value);
	#else
		#error "dgInterlockedExchange implementation required"
	#endif
}

DG_INLINE dgInt32 dgInterlockedTest(dgInt32* const ptr, dgInt32 value)
{
	#if (defined (_WIN_32_VER) || defined (_WIN_64_VER))
		return _InterlockedCompareExchange((long*)ptr, value, value);
	#elif (defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
		return InterlockedCompareExchange((long*)ptr, value, value);
	#elif (defined (_POSIX_VER) || defined (_POSIX_VER_64) ||defined (_MACOSX_VER))
		//__sync_synchronize();
		return __sync_lock_test_and_set((int32_t*)ptr, value);
	#else
		#error "dgInterlockedTest implementation required"
	#endif
}

/*
class dgAtomic: protected std::atomic<dgInt32>
{
	public:
	dgAtomic()
		:std::atomic<dgInt32>()
	{
	}

	DG_INLINE operator dgInt32() const 
	{
		const std::atomic<dgInt32>& me = *this;
		return me;
	}

	DG_INLINE dgInt32 Exchange(dgInt32 value)
	{
		return exchange(value);
	}
};
*/

DG_INLINE void dgThreadYield()
{
#ifndef DG_USE_THREAD_EMULATION
	std::this_thread::yield();
#endif
}

DG_INLINE void dgThreadPause()
{
#ifndef DG_USE_THREAD_EMULATION
	#if defined (_WIN_32_VER) || defined (_WIN_64_VER) || defined (WIN32) || defined (i386_) || defined (x86_64_)
		_mm_pause();
	#else 
		std::this_thread::yield();
	#endif
#endif
}

DG_INLINE void dgSpinLock(dgInt32* const ptr)
{
#ifndef DG_USE_THREAD_EMULATION 
	while (dgInterlockedExchange(ptr, 1)) {
//		DG_TRACKTIME_NAMED("lock");
		dgThreadYield();
	}
#endif
}


DG_INLINE void dgSpinUnlock (dgInt32* const ptr)
{
	#ifndef DG_USE_THREAD_EMULATION 
		dgInterlockedExchange(ptr, 0);
	#else
		*ptr = 0;
	#endif
}


class dgScopeSpinLock
{
	public:
	DG_INLINE dgScopeSpinLock(dgInt32* const lock)
		:m_atomicLock(lock)
	{
		dgSpinLock(m_atomicLock);
	}

	DG_INLINE ~dgScopeSpinLock()
	{
		dgSpinUnlock(m_atomicLock);
	}

	dgInt32* m_atomicLock;
};

class dgScopeSpinPause
{
	public:
	DG_INLINE dgScopeSpinPause(dgInt32* const lock)
		:m_atomicLock(lock)
	{
		while (dgInterlockedExchange(m_atomicLock, 1)) {
			//DG_TRACKTIME_NAMED("pause");
			dgThreadPause();
		}
	}

	DG_INLINE ~dgScopeSpinPause()
	{
		dgSpinUnlock(m_atomicLock);
	}

	dgInt32* m_atomicLock;
};



#ifdef _MACOSX_VER
#include <sys/time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
//clock_gettime is not implemented on OSX
DG_INLINE int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

#endif

