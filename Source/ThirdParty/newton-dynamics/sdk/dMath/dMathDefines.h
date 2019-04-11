/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#ifndef __dMathDefined__
#define __dMathDefined__

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
	#include <windows.h>
	#ifdef _DEBUG
		#include <stdarg.h>
		inline void dExpandTraceMessage (const char* const fmt, ...)
		{
			va_list v_args;
			char text[4096];

			text[0] = 0;
			va_start (v_args, fmt);     
			vsprintf_s(text, fmt, v_args);
			va_end (v_args);            

			OutputDebugStringA (text);
		}

		#define dTrace(x)										\
		{														\
			dExpandTraceMessage x;								\
		}																	
	#else
		#define dTrace(x)
	#endif
#else
	#define dTrace(x)
#endif

#if ( defined (_MSC_VER) || defined (_MINGW_32_VER) || defined (_MINGW_64_VER) )
	#include <crtdbg.h>
	#define dAssert(x) _ASSERTE(x)
#else 
	#define dAssert(x) assert(x)
#endif



#ifndef dFloat
	#ifdef _NEWTON_USE_DOUBLE
		typedef double dFloat;
	#else 
		typedef float dFloat;
	#endif
#endif 

#ifndef dFloat64
	typedef double dFloat64;
#endif 

// some constants
#define	dPi			  3.141592f
#define	dRadToDegree (180.0f / dPi)
#define	dDegreeToRad (1.0f / dRadToDegree)

// transcendental functions
#define	dSqrt(x)	dFloat (sqrt (dFloat(x))) 
#define	dCiel(x)	dFloat (ceil (dFloat(x))) 
#define	dFloor(x)	dFloat (floor (dFloat(x))) 
#define	dLog(x)		dFloat (log (dFloat(x))) 
#define	dMod(x,y)	dFloat (fmod (dFloat(x), dFloat(y))) 
#define	dPow(x,y)	dFloat (pow (dFloat(x), dFloat(y))) 

#define dSin(x)		dFloat (sin (dFloat(x)))
#define dCos(x)		dFloat (cos (dFloat(x)))
#define dTan(x)		dFloat (tan (dFloat(x)))
#define dAsin(x)	dFloat (asin (dFloat(x)))
#define dAcos(x)	dFloat (acos (dFloat(x)))
#define	dAtan(x)	dFloat (atan (dFloat(x)))
#define	dAtan2(x,y) dFloat (atan2 (dFloat(x), dFloat(y)))



#ifdef D_PROFILER
	#include <dProfiler.h>

	#define D_TRACKTIME() dProfilerZoneScoped(__FUNCTION__)
	#define D_SET_TRACK_NAME(trackName) dProfilerSetTrackName(trackName)
#else
	#define D_TRACKTIME() 
	#define D_SET_TRACK_NAME(trackName)
#endif


#define	D_MSC_VECTOR_ALIGMENT

enum dEulerAngleOrder
{
	m_pitchYawRoll,
	m_pitchRollYaw,
	PYR = (0 << 8) + (1 << 4) + (2 << 0),
	PRY = (0 << 8) + (2 << 4) + (1 << 0),
	YPR = (1 << 8) + (0 << 4) + (2 << 0),
	YRP = (1 << 8) + (2 << 4) + (0 << 0),
	RYP = (2 << 8) + (1 << 4) + (0 << 0),
	RPY = (2 << 8) + (0 << 4) + (1 << 0),
};

#define dAlloca(type,size) (type*) alloca ((size) * sizeof (type))

template <class T>
T dAbs(T A)
{
	// this is far faster than the standard function (does not mess with cpu rounding mode)
	return (A >= T(0.0f)) ? A : -A;
}

template <class T>
T dSign (T A)
{
	return (A >= T(0)) ? T(1) : T(-1);
}

template <class T> 
void dSwap(T& A, T& B)
{
	T tmp (A);
	A = B;
	B = tmp;
}	

template <class T>
T dMax(T A, T B)
{
	return (A > B) ? A : B; 
}

template <class T>
T dMin(T A, T B)
{
	return (A < B) ? A : B; 
}

template <class T>
T dClamp(T val, T min, T max)
{
	return dMax (min, dMin (max, val));
}

template <class T> 
void dSort (T* const array, int elements, int (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	int stride = 8;
	int stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	int stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		int lo = stack[stackIndex][0];
		int hi = stack[stackIndex][1];
		if ((hi - lo) > stride) {
			int i = lo;
			int j = hi;
			T val (array[(lo + hi) >> 1]);
			do {    
				while (compare (&array[i], &val, context) < 0) i ++;
				while (compare (&array[j], &val, context) > 0) j --;

				if (i <= j)	{
					dSwap(array[i], array[j]);
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
			dAssert (stackIndex < int (sizeof (stack) / (2 * sizeof (stack[0][0]))));
		}
	}

	stride = stride * 2;
	if (elements < stride) {
		stride = elements;
	}
	for (int i = 1; i < stride; i ++) {
		if (compare (&array[0], &array[i], context) > 0) {
			dSwap(array[0], array[i]);
		}
	}

	for (int i = 1; i < elements; i ++) {
		int j = i;
		T tmp (array[i]);
		for (; compare (&array[j - 1], &tmp, context) > 0; j --) {
			dAssert (j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (int i = 0; i < (elements - 1); i ++) {
		dAssert (compare (&array[i], &array[i + 1], context) <= 0);
	}
#endif
}


// return dot product
template<class T>
T dDotProduct(int size, const T* const A, const T* const B)
{
	T val(0.0f);
	for (int i = 0; i < size; i++) {
		val = val + A[i] * B[i];
	}
	return val;
}


template<class T>
void dMatrixTimeVector(int size, const T* const matrix, const T* const v, T* const out)
{
	int stride = 0;
	for (int i = 0; i < size; i++) {
		out[i] = dDotProduct(size, &matrix[stride], v);
		stride += size;
	}
}

template<class T>
void dCholeskySolve(int size, int n, const T* const choleskyMatrix, T* const x)
{
	int stride = 0;
	for (int i = 0; i < n; i++) {
		T acc(0.0f);
		const T* const row = &choleskyMatrix[stride];
		for (int j = 0; j < i; j++) {
			acc = acc + row[j] * x[j];
		}
		x[i] = (x[i] - acc) / row[i];
		stride += size;
	}

	for (int i = n - 1; i >= 0; i--) {
		T acc = 0.0f;
		for (int j = i + 1; j < n; j++) {
			acc = acc + choleskyMatrix[size * j + i] * x[j];
		}
		x[i] = (x[i] - acc) / choleskyMatrix[size * i + i];
	}
}

template<class T>
bool dCholeskyFactorization(int size, int block, T* const matrix)
{
	for (int i = 0; i < block; i++) {
		T* const rowN = &matrix[size * i];

		int stride = 0;
		for (int j = 0; j <= i; j++) {
			T s(0.0f);
			T* const rowJ = &matrix[stride];
			for (int k = 0; k < j; k++) {
				s += rowN[k] * rowJ[k];
			}

			if (i == j) {
				T diag = rowN[i] - s;
				if (diag < T(1.0e-8f)) {
					return false;
				}
				rowN[i] = T(sqrt(diag));
			} else {
				rowN[j] = ((T(1.0f) / rowJ[j]) * (rowN[j] - s));
			}
			stride += size;
		}
	}
	return true;
}

template<class T>
bool dCholeskyFactorization(int size, T* const matrix)
{
	return dCholeskyFactorization(size, size, matrix);
}

template<class T>
void dPermuteRows(int size, int i, int j, T* const matrix, T* const choleskyMatrix, T* const x, T* const r, T* const low, T* const high, int* const permute)
{
	if (i != j) {
		T* const matrixRowA = &matrix[size * i];
		T* const matrixRowB = &matrix[size * j];
		T* const choleskyRowA = &choleskyMatrix[size * i];
		T* const choleskyRowB = &choleskyMatrix[size * j];
		for (int k = 0; k < size; k++) {
			dSwap(matrixRowA[k], matrixRowB[k]);
			dSwap(choleskyRowA[k], choleskyRowB[k]);
		}

		int stride = 0;
		for (int k = 0; k < size; k++) {
			dSwap(matrix[stride + i], matrix[stride + j]);
			stride += size;
		}

		dSwap(x[i], x[j]);
		dSwap(r[i], r[j]);
		dSwap(low[i], low[j]);
		dSwap(high[i], high[j]);
		dSwap(permute[i], permute[j]);
	}
}

template<class T>
void dCholeskyUpdate(int size, int row, int colum, T* const choleskyMatrix, T* const tmp, T* const reflexion)
{
	if (row != colum) {
		dAssert(row < colum);

		for (int i = row; i < size; i++) {
			T* const rowI = &choleskyMatrix[size * i];
			T mag(T(dFloat(0.0f)));
			for (int j = i + 1; j < size; j++) {
				mag += rowI[j] * rowI[j];
				reflexion[j] = rowI[j];
			}
			if (mag > T(dFloat(1.0e-9f))) {
				reflexion[i] = rowI[i] - T(sqrt(mag + rowI[i] * rowI[i]));

				T vMag2(mag + reflexion[i] * reflexion[i]);
				T den(dFloat(1.0f) / T(sqrt(vMag2)));
				for (int j = i; j < size; j++) {
					reflexion[j] *= den;
				}

				for (int j = i; j < size; j++) {
					T acc(0.0f);
					T* const rowJ = &choleskyMatrix[size * j];
					for (int k = i; k < size; k++) {
						acc += rowJ[k] * reflexion[k];
					}
					tmp[j] = acc * T(dFloat(2.0f));
				}

				for (int j = i + 1; j < size; j++) {
					T* const rowJ = &choleskyMatrix[size * j];
					for (int k = i; k < size; k++) {
						rowJ[k] -= tmp[j] * reflexion[k];
					}
				}
				rowI[i] -= tmp[i] * reflexion[i];
			}

			for (int k = i + 1; k < size; k++) {
				rowI[k] = T(dFloat(0.0f));
			}

			if (rowI[i] < T(dFloat(0.0f))) {
				for (int k = i; k < size; k++) {
					choleskyMatrix[size * k + i] = -choleskyMatrix[size * k + i];
				}
			}
		}
		for (int i = row; i < size; i++) {
			choleskyMatrix[size * i + i] = dMax(choleskyMatrix[size * i + i], T(dFloat(1.0e-6f)));
		}
	}
}

template<class T>
void dCalculateDelta_x(int size, T dir, int n, const T* const matrix, const T* const choleskyMatrix, T* const delta_x)
{
	const T* const row = &matrix[size * n];
	for (int i = 0; i < n; i++) {
		delta_x[i] = -row[i] * dir;
	}
	dCholeskySolve(size, n, choleskyMatrix, delta_x);
	delta_x[n] = dir;
}

// calculate delta_r = A * delta_x
template<class T>
void dCalculateDelta_r(int size, int n, const T* const matrix, const T* const delta_x, T* const delta_r)
{
	int stride = n * size;
	for (int i = n; i < size; i++) {
		delta_r[i] = dDotProduct(size, &matrix[stride], delta_x);
		stride += size;
	}
}


// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solve is far more general than a strict LCP
// to solve a strict LCP set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constrain: x(i) * r(i) = 0
template <class T>
bool dSolveDantzigLCP(int size, T* const matrix, T* const x, T* const b, T* const low, T* const high, T regularizer = T(1.e-4f))
{
	T* const choleskyMatrix = dAlloca(T, size * size);
	T* const x0 = dAlloca(T, size);
	T* const r0 = dAlloca(T, size);
	T* const delta_r = dAlloca(T, size);
	T* const delta_x = dAlloca(T, size);
	T* const tmp0 = dAlloca(T, size);
	T* const tmp1 = dAlloca(T, size);
	int* const permute = dAlloca(int, size);

	memcpy(choleskyMatrix, matrix, sizeof(T) * size * size);
	bool pass = dCholeskyFactorization(size, choleskyMatrix);
	while (!pass) {
		int stride = 0;
		for (int i = 0; i < size; i ++) {
			matrix[stride] += matrix[stride] * regularizer;
			stride += size + 1;
		}
		memcpy(choleskyMatrix, matrix, sizeof(T) * size * size);
		pass = dCholeskyFactorization(size, choleskyMatrix);
	}

	for (int i = 0; i < size; i++) {
		T* const row = &choleskyMatrix[i * size];
		for (int j = i + 1; j < size; j++) {
			row[j] = T(0.0f);
		}
	}

	int index = 0;
	int count = size;
	int clampedIndex = size;
	for (int i = 0; i < size; i++) {
		permute[i] = short(i);
		r0[i] = -b[i];
		x0[i] = dFloat(0.0f);
		delta_x[i] = T(dFloat(0.0f));
		delta_r[i] = T(dFloat(0.0f));
	}

	bool findInitialGuess = size >= 6;
	if (findInitialGuess) {
		int initialGuessCount = size;
		for (int j = 0; (j < 5) && findInitialGuess; j++) {

			findInitialGuess = false;
			for (int i = 0; i < initialGuessCount; i++) {
				x0[i] = -r0[i];
			}
			dCholeskySolve(size, initialGuessCount, choleskyMatrix, x0);
			int permuteStart = initialGuessCount;
			for (int i = 0; i < initialGuessCount; i++) {
				T f = x0[i];
				if ((f < low[i]) || (f > high[i])) {
					findInitialGuess = true;
					x0[i] = T(dFloat(0.0f));
					dPermuteRows(size, i, initialGuessCount - 1, matrix, choleskyMatrix, x0, r0, low, high, permute);
					permuteStart = dMin(permuteStart, i);
					i--;
					initialGuessCount--;
				}
			}
			if (findInitialGuess) {
				dCholeskyUpdate(size, permuteStart, size - 1, choleskyMatrix, tmp0, tmp1);
			}
		}

		if (initialGuessCount == size) {
			for (int i = 0; i < size; i++) {
				x[i] = x0[i];
				b[i] = T(dFloat(0.0f));
			}
			return true;
		} else {
			if (!findInitialGuess) {
				for (int i = 0; i < initialGuessCount; i++) {
					r0[i] = T(dFloat(0.0f));
				}
				for (int i = initialGuessCount; i < size; i++) {
					r0[i] += dDotProduct(size, &matrix[i * size], x0);
				}
				index = initialGuessCount;
				count = size - initialGuessCount;
			} else {
				//dAssert(0);
				for (int i = 0; i < size; i++) {
					x0[i] = dFloat(0.0f);
				}
			}
		}
	}

	while (count) {
		bool loop = true;
		bool calculateDelta_x = true;

		while (loop) {
			loop = false;
			T clamp_x(0.0f);
			int swapIndex = -1;

			if (dAbs(r0[index]) > T(1.0e-12f)) {
				if (calculateDelta_x) {
					T dir(dFloat(1.0f));
					dCalculateDelta_x(size, dir, index, matrix, choleskyMatrix, delta_x);
				}

				calculateDelta_x = true;
				dCalculateDelta_r(size, index, matrix, delta_x, delta_r);
				dAssert(delta_r[index] != T(dFloat(0.0f)));
				dAssert(dAbs(delta_x[index]) == T(1.0f));
				delta_r[index] = (delta_r[index] == T(dFloat(0.0f))) ? T(dFloat(1.0e-12f)) : delta_r[index];

				T s = -r0[index] / delta_r[index];
				dAssert(dAbs(s) >= T(dFloat(0.0f)));

				for (int i = 0; i <= index; i++) {
					T x1 = x0[i] + s * delta_x[i];
					if (x1 > high[i]) {
						swapIndex = i;
						clamp_x = high[i];
						s = (high[i] - x0[i]) / delta_x[i];
					} else if (x1 < low[i]) {
						swapIndex = i;
						clamp_x = low[i];
						s = (low[i] - x0[i]) / delta_x[i];
					}
				}
				dAssert(dAbs(s) >= T(dFloat(0.0f)));

				for (int i = clampedIndex; (i < size) && (s > T(1.0e-12f)); i++) {
					T r1 = r0[i] + s * delta_r[i];
					if ((r1 * r0[i]) < T(dFloat(0.0f))) {
						dAssert(dAbs(delta_r[i]) > T(dFloat(0.0f)));
						T s1 = -r0[i] / delta_r[i];
						dAssert(dAbs(s1) >= T(dFloat(0.0f)));
						dAssert(dAbs(s1) <= dAbs(s));
						if (dAbs(s1) < dAbs(s)) {
							s = s1;
							swapIndex = i;
						}
					}
				}

				for (int i = 0; i < size; i++) {
					dAssert((x0[i] + dAbs(x0[i]) * T(dFloat(1.0e-4f))) >= low[i]);
					dAssert((x0[i] - dAbs(x0[i]) * T(dFloat(1.0e-4f))) <= high[i]);

					x0[i] += s * delta_x[i];
					r0[i] += s * delta_r[i];

					dAssert((x0[i] + dFloat(1.0f)) >= low[i]);
					dAssert((x0[i] - dFloat(1.0f)) <= high[i]);
				}
			}

			if (swapIndex == -1) {
				r0[index] = T(dFloat(0.0f));
				delta_r[index] = T(dFloat(0.0f));
				index++;
				count--;
				loop = false;
			} else if (swapIndex == index) {
				count--;
				clampedIndex--;
				x0[index] = clamp_x;
				dPermuteRows(size, index, clampedIndex, matrix, choleskyMatrix, x0, r0, low, high, permute);
				dCholeskyUpdate(size, index, clampedIndex, choleskyMatrix, tmp0, tmp1);

				loop = count ? true : false;
			} else if (swapIndex > index) {
				loop = true;
				r0[swapIndex] = T(dFloat(0.0f));
				dAssert(swapIndex < size);
				dAssert(clampedIndex <= size);
				if (swapIndex < clampedIndex) {
					count--;
					clampedIndex--;
					dPermuteRows(size, clampedIndex, swapIndex, matrix, choleskyMatrix, x0, r0, low, high, permute);
					dCholeskyUpdate(size, swapIndex, clampedIndex, choleskyMatrix, tmp0, tmp1);
					dAssert(clampedIndex >= index);
				} else {
					count++;
					dAssert(clampedIndex < size);
					dPermuteRows(size, clampedIndex, swapIndex, matrix, choleskyMatrix, x0, r0, low, high, permute);
					dCholeskyUpdate(size, clampedIndex, swapIndex, choleskyMatrix, tmp0, tmp1);
					clampedIndex++;
					dAssert(clampedIndex <= size);
					dAssert(clampedIndex >= index);
				}
				calculateDelta_x = false;
			} else {
				dAssert(index > 0);
				x0[swapIndex] = clamp_x;
				delta_x[index] = T(dFloat(0.0f));

				dAssert(swapIndex < index);
				dPermuteRows(size, swapIndex, index - 1, matrix, choleskyMatrix, x0, r0, low, high, permute);
				dPermuteRows(size, index - 1, index, matrix, choleskyMatrix, x0, r0, low, high, permute);
				dPermuteRows(size, clampedIndex - 1, index, matrix, choleskyMatrix, x0, r0, low, high, permute);
				dCholeskyUpdate(size, swapIndex, clampedIndex - 1, choleskyMatrix, tmp0, tmp1);

				clampedIndex--;
				index--;
				loop = true;
			}
		}
	}

	for (int i = 0; i < size; i++) {
		int j = permute[i];
		x[j] = x0[i];
		b[j] = r0[i];
	}
	return true;
}


template <class T>
bool dCholeskyWithRegularizer(int size, int block, T* const matrix, T regularizer)
{
	T* const copy = dAlloca(T, size * size);
	memcpy(copy, matrix, size * size * sizeof (T));
	bool pass = dCholeskyFactorization(size, block, matrix);

	int count = 0;
	while (!pass && (count < 10)) {
		int stride = 0;
		for (int i = 0; i < block; i++) {
			copy[stride] += copy[stride] * regularizer;
			stride += size + 1;
		}
		memcpy(matrix, copy, sizeof(T)* size * size);
		pass = dCholeskyFactorization(size, block, matrix);
	}
	return pass;
}


// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// b is zero
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this is the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
bool dSolvePartitionDantzigLCP(int size, T* const symmetricMatrixPSD, T* const x, T* const b, T* const low, T* const high, int unboundedSize, T regularizer = T(1.e-4f))
{
	bool ret = false;
	if (unboundedSize > 0) {

		ret = dCholeskyWithRegularizer(size, unboundedSize, symmetricMatrixPSD, regularizer);
		if (ret) {
			memcpy (x, b, unboundedSize * sizeof (T));
			dCholeskySolve(size, unboundedSize, symmetricMatrixPSD, x);
			int base = unboundedSize * size;
			for (int i = unboundedSize; i < size; i++) {
				b[i] -= dDotProduct(unboundedSize, &symmetricMatrixPSD[base], x);
				base += size;
			}

			const int boundedSize = size - unboundedSize;
			T* const l = dAlloca(T, boundedSize);
			T* const h = dAlloca(T, boundedSize);
			T* const c = dAlloca(T, boundedSize);
			T* const u = dAlloca(T, boundedSize);
			T* const a11 = dAlloca(T, boundedSize * boundedSize);
			T* const a10 = dAlloca(T, boundedSize * unboundedSize);

			for (int i = 0; i < boundedSize; i++) {
				T* const g = &a10[i * unboundedSize];
				const T* const row = &symmetricMatrixPSD[(unboundedSize + i) * size];
				for (int j = 0; j < unboundedSize; j++) {
					g[j] = -row[j];
				}
				dCholeskySolve(size, unboundedSize, symmetricMatrixPSD, g);

				T* const arow = &a11[i * boundedSize];
				const T* const row2 = &symmetricMatrixPSD[(unboundedSize + i) * size];
				arow[i] = row2[unboundedSize + i] + dDotProduct(unboundedSize, g, row2);
				for (int j = i + 1; j < boundedSize; j++) {
					const T* const row1 = &symmetricMatrixPSD[(unboundedSize + j) * size];
					T elem = row1[unboundedSize + i] + dDotProduct(unboundedSize, g, row1);
					arow[j] = elem;
					a11[j * boundedSize + i] = elem;
				}
				u[i] = T(0.0f);
				c[i] = b[i + unboundedSize];
				l[i] = low[i + unboundedSize];
				h[i] = high[i + unboundedSize];
			}

			if (dSolveDantzigLCP(boundedSize, a11, u, c, l, h, regularizer)) {
				for (int i = 0; i < boundedSize; i++) {
					const T s = u[i];
					x[unboundedSize + i] = s;
					const T* const g = &a10[i * unboundedSize];
					for (int j = 0; j < unboundedSize; j++) {
						x[j] += g[j] * s;
					}
				}
				ret = true;
			}
		}
	} else {
		ret = dSolveDantzigLCP(size, symmetricMatrixPSD, x, b, low, high, regularizer);
	}

	return ret;
}


// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
void dGaussSeidelLcpSor(const int size, const T* const matrix, T* const x, const T* const b, const int* const normalIndex, const T* const low, const T* const high, T tol2, int maxIterCount, T sor)
{
	const T* const me = matrix;
	T* const invDiag1 = dAlloca(T, size);

	int stride = 0;
	for (int j = 0; j < size; j++) {
		const int index = normalIndex[j];
		const T val = index ? x[j + index] : 1.0f;
		const T l = low[j] * val;
		const T h = high[j] * val;
		x[j] = dClamp(x[j], l, h);
		invDiag1[j] = T(1.0f) / me[stride + j];
		stride += size;
	}

	T tolerance(tol2 * 2.0f);
	const T* const invDiag = invDiag1;
	const int maxCount = dMax (8, size);
	for (int i = 0; (i < maxCount) && (tolerance > T(1.0e-8f)); i++) {
		int base = 0;
		tolerance = T(0.0f);
		for (int j = 0; j < size; j++) {
			const T* const row = &me[base];
			T r(b[j] - dDotProduct(size, row, x));
			T f((r + row[j] * x[j]) * invDiag[j]);

			const int index = normalIndex[j];
			const T val = index ? x[j + index] : 1.0f;
			const T l = low[j] * val;
			const T h = high[j] * val;
			if (f > h) {
				x[j] = h;
			} else if (f < l) {
				x[j] = l;
			} else {
				tolerance += r * r;
				x[j] = f;
			}
			base += size;
		}
	}

#ifdef _DEBUG 
	int passes = 0;
#endif
	for (int i = 0; (i < maxIterCount) && (tolerance > tol2); i++) {
		int base = 0;
		tolerance = T(0.0f);
#ifdef _DEBUG 
		passes++;
#endif
		for (int j = 0; j < size; j++) {
			const T* const row = &me[base];
			T r(b[j] - dDotProduct(size, row, x));
			T f((r + row[j] * x[j]) * invDiag[j]);
			f = x[j] + (f - x[j]) * sor;

			const int index = normalIndex[j];
			const T val = index ? x[j + index] : 1.0f;
			const T l = low[j] * val;
			const T h = high[j] * val;
			if (f > h) {
				x[j] = h;
			} else if (f < l) {
				x[j] = l;
			} else {
				tolerance += r * r;
				x[j] = f;
			}
			base += size;
		}
	}
}

#endif

