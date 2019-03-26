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

#include "dgStdafx.h"
#include "dgTypes.h"
#include "dgDebug.h"
#include "dgVector.h"
#include "dgMemory.h"
#include "dgStack.h"

dgUnsigned64 dgGetTimeInMicrosenconds()
{
#ifdef _MSC_VER
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER baseCount;
	if (!frequency.QuadPart) {
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter (&baseCount);
	}

	LARGE_INTEGER count;
	QueryPerformanceCounter (&count);
	count.QuadPart -= baseCount.QuadPart;
	dgUnsigned64 ticks = dgUnsigned64 (count.QuadPart * LONGLONG (1000000) / frequency.QuadPart);
	return ticks;

#elif (defined (_POSIX_VER) || defined (_POSIX_VER_64))

	timespec ts;
	static dgUnsigned64 baseCount = 0;
	if (!baseCount) {
		clock_gettime(CLOCK_REALTIME, &ts);
		baseCount = dgUnsigned64 (ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
	}
	clock_gettime(CLOCK_REALTIME, &ts); // Works on Linux
	return dgUnsigned64 (ts.tv_sec) * 1000000 + ts.tv_nsec / 1000 - baseCount;

#elif defined (_MACOSX_VER)
	timeval tp;
	static dgUnsigned64 baseCount = 0;
	if (!baseCount) {
		gettimeofday(&tp, NULL);
		baseCount = dgUnsigned64 (tp.tv_sec) * 1000000 + tp.tv_usec;
	}

	gettimeofday(&tp, NULL);
	dgUnsigned64 microsecunds = dgUnsigned64 (tp.tv_sec) * 1000000 + tp.tv_usec;
	return microsecunds - baseCount;
#else 
	#error "dgGetTimeInMicrosenconds implementation required"
	return 0;
#endif
}


dgFloat64 dgRoundToFloat(dgFloat64 val)
{
	dgInt32 exp;
	dgFloat64 mantissa = frexp(val, &exp);

	const dgFloat64 power = 1 << 23;
	const dgFloat64 invPower = dgFloat64(1.0f) / power;
	mantissa = floor(mantissa * power) * invPower;

	dgFloat64 val1 = ldexp(mantissa, exp);
	return val1;
}


void dgGetMinMax (dgBigVector &minOut, dgBigVector &maxOut, const dgFloat64* const vertexArray, dgInt32 vCount, dgInt32 strideInBytes)
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));
	const dgFloat64* vArray = vertexArray + stride;

	dgAssert (stride >= 3);
	minOut = dgBigVector (vertexArray[0], vertexArray[1], vertexArray[2], dgFloat64 (0.0f)); 
	maxOut = dgBigVector (vertexArray[0], vertexArray[1], vertexArray[2], dgFloat64 (0.0f)); 

	for (dgInt32 i = 1; i < vCount; i ++) {
		minOut.m_x = dgMin (minOut.m_x, vArray[0]);
		minOut.m_y = dgMin (minOut.m_y, vArray[1]);
		minOut.m_z = dgMin (minOut.m_z, vArray[2]);

		maxOut.m_x = dgMax (maxOut.m_x, vArray[0]);
		maxOut.m_y = dgMax (maxOut.m_y, vArray[1]);
		maxOut.m_z = dgMax (maxOut.m_z, vArray[2]);

		vArray += stride;
	}
}




static inline dgInt32 cmp_vertex (const dgFloat64* const v1, const dgFloat64* const v2, dgInt32 firstSortAxis)
{
	if (v1[firstSortAxis] < v2[firstSortAxis]) {
		return -1;
	}

	if (v1[firstSortAxis] > v2[firstSortAxis]){
		return 1;
	}

	return 0;
}

static dgInt32 SortVertices (dgFloat64* const vertexList,  dgInt32 stride, dgInt32 compareCount, dgInt32 vertexCount, dgFloat64 tolerance)
{
	dgBigVector xc(dgFloat64(0.0f));
	dgBigVector x2c(dgFloat64(0.0f));
	dgBigVector minP (dgFloat64 (1.0e10f), dgFloat64 (1.0e10f), dgFloat64 (1.0e10f), dgFloat64 (0.0f));
	dgBigVector maxP (dgFloat64 (-1.0e10f), dgFloat64 (-1.0e10f), dgFloat64 (-1.0e10f), dgFloat64 (0.0f));

	for (dgInt32 k = 0, i = 0; i < vertexCount; i ++) {

		dgBigVector x(vertexList[k + 2], vertexList[k + 3], vertexList[k + 4], dgFloat64(0.0f));
		xc += x;
		x2c += x * x;
		minP = minP.GetMin(x);
		maxP = maxP.GetMax(x);
		k += stride;
	}

	dgBigVector del (maxP - minP);
	dgFloat64 minDist = dgMin (del.m_x, del.m_y, del.m_z);
	if (minDist < dgFloat64 (1.0e-3f)) {
		minDist = dgFloat64 (1.0e-3f);
	}

	dgFloat64 tol = tolerance * minDist + dgFloat64 (1.0e-12f);
	dgFloat64 sweptWindow = dgFloat64 (2.0f) * tol;
	sweptWindow += dgFloat64 (1.0e-4f);

	x2c = x2c.Scale (dgFloat32 (vertexCount)) - xc * xc;

	dgInt32 firstSortAxis = 2;
	if ((x2c.m_y >= x2c.m_x) && (x2c.m_y >= x2c.m_z)) {
		firstSortAxis = 3;
	} else if ((x2c.m_z >= x2c.m_x) && (x2c.m_z >= x2c.m_y)) {
		firstSortAxis = 4;
	}

	dgInt32 stack[1024][2];
	stack[0][0] = 0;
	stack[0][1] = vertexCount - 1;
	dgInt32 stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		dgInt32 lo = stack[stackIndex][0];
		dgInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > 8) {
			dgInt32 i = lo;
			dgInt32 j = hi;
			dgFloat64 val[64]; 
			memcpy (val, &vertexList[((lo + hi) >> 1) * stride], stride * sizeof (dgFloat64));
			do {    
				while (cmp_vertex (&vertexList[i * stride], val, firstSortAxis) < 0) i ++;
				while (cmp_vertex (&vertexList[j * stride], val, firstSortAxis) > 0) j --;

				if (i <= j)	{
					if (i < j) {
						dgFloat64 tmp[64]; 
						memcpy (tmp, &vertexList[i * stride], stride * sizeof (dgFloat64));
						memcpy (&vertexList[i * stride], &vertexList[j * stride], stride * sizeof (dgFloat64)); 
						memcpy (&vertexList[j * stride], tmp, stride * sizeof (dgFloat64)); 
					}
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
		} else {
			for (dgInt32 i = lo + 1; i <= hi ; i++) {
				dgFloat64 tmp[64]; 
				memcpy (tmp, &vertexList[i * stride], stride * sizeof (dgFloat64));

				dgInt32 j = i;
				for (; j && (cmp_vertex (&vertexList[(j - 1) * stride], tmp, firstSortAxis) > 0); j --) {
					memcpy (&vertexList[j * stride], &vertexList[(j - 1)* stride], stride * sizeof (dgFloat64));
				}
				memcpy (&vertexList[j * stride], tmp, stride * sizeof (dgFloat64)); 
			}
		}
	}


#ifdef _DEBUG
	for (dgInt32 i = 0; i < (vertexCount - 1); i ++) {
		dgAssert (cmp_vertex (&vertexList[i * stride], &vertexList[(i + 1) * stride], firstSortAxis) <= 0);
	}
#endif

	dgInt32 count = 0;
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		dgInt32 m = i * stride;
		dgInt32 index = dgInt32 (vertexList[m + 0]);
		if (index == dgInt32 (0xffffffff)) {
			dgFloat64 swept = vertexList[m + firstSortAxis] + sweptWindow;
			dgInt32 k = i * stride + stride;
			for (dgInt32 i1 = i + 1; i1 < vertexCount; i1 ++) {

				index = dgInt32 (vertexList[k + 0]);
				if (index == dgInt32 (0xffffffff)) {
					dgFloat64 val = vertexList[k + firstSortAxis];
					if (val >= swept) {
						break;
					}
					bool test = true;
					for (dgInt32 t = 0; test && (t < compareCount); t ++) {
						val = fabs (vertexList[m + t + 2] - vertexList[k + t + 2]);
						test = test && (val <= tol);
					}
					if (test) {
						vertexList[k + 0] = dgFloat64 (count);
					}
				}
				k += stride;
			}

			dgAssert (&vertexList[count * stride + 2] <= &vertexList[m + 2]);
			if (&vertexList[count * stride + 2] < &vertexList[m + 2]) {
				memcpy (&vertexList[count * stride + 2], &vertexList[m + 2], (stride - 2) * sizeof (dgFloat64));
			}
			vertexList[m + 0] = dgFloat64 (count);
			count ++;
		}
	}

	return count;
}


static dgInt32 QuickSortVertices (dgFloat64* const vertList, dgInt32 stride, dgInt32 compareCount, dgInt32 vertexCount, dgFloat64 tolerance)
{
	dgInt32 count = 0;
	if (vertexCount > (1024 * 256)) {
		dgFloat64 x = dgFloat32 (0.0f);
		dgFloat64 y = dgFloat32 (0.0f);
		dgFloat64 z = dgFloat32 (0.0f);
		dgFloat64 xd = dgFloat32 (0.0f);
		dgFloat64 yd = dgFloat32 (0.0f);
		dgFloat64 zd = dgFloat32 (0.0f);

		for (dgInt32 i = 0; i < vertexCount; i ++) {
			dgFloat64 x0 = vertList[i * stride + 2];
			dgFloat64 y0 = vertList[i * stride + 3];
			dgFloat64 z0 = vertList[i * stride + 4];
			x += x0;
			y += y0;
			z += z0;
			xd += x0 * x0;
			yd += y0 * y0;
			zd += z0 * z0;
		}

		xd = vertexCount * xd - x * x;
		yd = vertexCount * yd - y * y;
		zd = vertexCount * zd - z * z;

		dgInt32 axis = 2;
		dgFloat64 axisVal = x / vertexCount;
		if ((yd > xd) && (yd > zd)) {
			axis = 3;
			axisVal = y / vertexCount;
		}
		if ((zd > xd) && (zd > yd)) {
			axis = 4;
			axisVal = z / vertexCount;
		}

		dgInt32 i0 = 0;
		dgInt32 i1 = vertexCount - 1;
		do {    
			for ( ;vertList[i0 * stride + axis] < axisVal; i0 ++); 
			for ( ;vertList[i1 * stride + axis] > axisVal; i1 --);
			if (i0 <= i1) {
				for (dgInt32 i = 0; i < stride; i ++) {
					dgSwap (vertList[i0 * stride + i], vertList[i1 * stride + i]);
				}
				i0 ++; 
				i1 --;
			}
		} while (i0 <= i1);
		dgAssert (i0 < vertexCount);

		dgInt32 count0 = QuickSortVertices (&vertList[ 0 * stride], stride, compareCount, i0, tolerance);
		dgInt32 count1 = QuickSortVertices (&vertList[i0 * stride], stride, compareCount, vertexCount - i0, tolerance);

		count = count0 + count1;

		for (dgInt32 i = 0; i < count1; i ++) {
			memcpy (&vertList[(count0 + i) * stride + 2], &vertList[(i0 + i) * stride + 2], (stride - 2) * sizeof (dgFloat64));
		}


		//		dgFloat64* const indexPtr = (dgInt64*)vertList;
		for (dgInt32 i = i0; i < vertexCount; i ++) {
			//			indexPtr[i * stride] += count0;
			vertList[i * stride] += dgFloat64 (count0);
		}

	} else {
		count = SortVertices (vertList, stride, compareCount, vertexCount, tolerance);
	}

	return count;
}


dgInt32 dgVertexListToIndexList (dgFloat64* const vertList, dgInt32 strideInBytes, dgInt32 compareCount, dgInt32 vertexCount, dgInt32* const indexListOut, dgFloat64 tolerance)
{
	dgSetPrecisionDouble precision;

	if (strideInBytes < 3 * dgInt32 (sizeof (dgFloat64))) {
		return 0;
	}
	if (compareCount < 3) {
		return 0;
	}
	dgAssert (compareCount <= dgInt32 (strideInBytes / sizeof (dgFloat64)));
	dgAssert (strideInBytes == dgInt32 (sizeof (dgFloat64) * (strideInBytes / sizeof (dgFloat64))));

	dgInt32 stride = strideInBytes / dgInt32 (sizeof (dgFloat64));
	dgInt32 stride2 = stride + 2;

	dgStack<dgFloat64>pool (stride2  * vertexCount);
	dgFloat64* const tmpVertexList = &pool[0];

	dgInt32 k = 0;
	dgInt32 m = 0;
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		memcpy (&tmpVertexList[m + 2], &vertList[k], stride * sizeof (dgFloat64));
		tmpVertexList[m + 0] = dgFloat64 (- 1.0f);
		tmpVertexList[m + 1] = dgFloat64 (i);
		k += stride;
		m += stride2;
	}
	
	dgInt32 count = QuickSortVertices (tmpVertexList, stride2, compareCount, vertexCount, tolerance);

	k = 0;
	m = 0;
	for (dgInt32 i = 0; i < count; i ++) {
		k = i * stride;
		m = i * stride2;
		memcpy (&vertList[k], &tmpVertexList[m + 2], stride * sizeof (dgFloat64));
		k += stride;
		m += stride2;
	}

	m = 0;
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		dgInt32 i1 = dgInt32 (tmpVertexList [m + 1]);
		dgInt32 index = dgInt32 (tmpVertexList [m + 0]);
		indexListOut[i1] = index;
		m += stride2;
	}

	return count;
}

dgInt32 dgVertexListToIndexList (dgFloat32* const vertList, dgInt32 strideInBytes, dgInt32 floatSizeInBytes, dgInt32 unsignedSizeInBytes, dgInt32 vertexCount, dgInt32* const indexList, dgFloat32 tolerance)
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));

	dgAssert (!unsignedSizeInBytes);
	dgStack<dgFloat64> pool(vertexCount * stride);

	dgInt32 floatCount = dgInt32 (floatSizeInBytes / sizeof (dgFloat32));

	dgFloat64* const data = &pool[0];
	for (dgInt32 i = 0; i < vertexCount; i ++) {

		dgFloat64* const dst = &data[i * stride];
		dgFloat32* const src = &vertList[i * stride];
		for (dgInt32 j = 0; j < stride; j ++) {
			dst[j] = src[j];
		}
	}

	dgInt32 count = dgVertexListToIndexList (data, dgInt32 (stride * sizeof (dgFloat64)), floatCount, vertexCount, indexList, dgFloat64 (tolerance));
	for (dgInt32 i = 0; i < count; i ++) {
		dgFloat64* const src = &data[i * stride];
		dgFloat32* const dst = &vertList[i * stride];
		for (dgInt32 j = 0; j < stride; j ++) {
			dst[j] = dgFloat32 (src[j]);
		}
	}
	
	return count;
}


//#define SERIALIZE_END	'dne '
#define SERIALIZE_END   0x646e6520

struct dgSerializeMarkerData
{
	dgSerializeMarkerData()
		:m_maker0(SERIALIZE_END)
		,m_maker1(SERIALIZE_END)
		,m_maker2(SERIALIZE_END)
		,m_revision(m_currentRevision)
	{
	}
	dgInt32 m_maker0;
	dgInt32 m_maker1;
	dgInt32 m_maker2;
	dgInt32 m_revision;
};

void dgSerializeMarker(dgSerialize serializeCallback, void* const userData)
{
	dgSerializeMarkerData marker;
	serializeCallback (userData, &marker, sizeof (dgSerializeMarkerData));
}

dgInt32 dgDeserializeMarker(dgDeserialize serializeCallback, void* const userData)
{
	dgInt32 state = 0;
	while (state != 3) {
		dgInt32 marker;
		serializeCallback (userData, &marker, sizeof (marker));
		switch (state) 
		{
			case 0:
			{
				if (marker == SERIALIZE_END) {
					state = 1;
					break;
				} else {
					state = 0;
				}
				break;
			}

			case 1:
			{
				if (marker == SERIALIZE_END) {
					state = 2;
					break;
				} else {
					state = 0;
				}
				break;
			}

			case 2:
			{
				if (marker == SERIALIZE_END) {
					state = 3;
					break;
				} else {
					state = 0;
				}
				break;
			}
		}
	} 

	dgInt32 revision;
	serializeCallback (userData, &revision, sizeof (revision));
	return revision;
}

dgSetPrecisionDouble::dgSetPrecisionDouble()
{
	#if (defined (_MSC_VER) && defined (_WIN_32_VER))
		dgClearFP();
		m_mask = dgInt32 (dgControlFP(0, 0));
		dgControlFP (_PC_53, _MCW_PC);
	#endif
}

dgSetPrecisionDouble::~dgSetPrecisionDouble()
{
	#if (defined (_MSC_VER) && defined (_WIN_32_VER))
		dgClearFP();
		dgControlFP (dgUnsigned32(m_mask), _MCW_PC);
	#endif
}


dgFloatExceptions::dgFloatExceptions(dgUnsigned32 mask)
{
	#if (defined (_MSC_VER) && defined (_WIN_32_VER))
		dgClearFP();
		m_mask = dgControlFP(0, 0);
		dgControlFP (m_mask & ~mask, _MCW_EM);
	#endif

	#ifdef _MACOSX_VER
		#ifndef IOS
			fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
		#endif
	#else
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	#endif

//	float a (1.0f);
//	float b (0.1f);
//	while (a != 0.0f)
//		a = a * b;
}

dgFloatExceptions::~dgFloatExceptions()
{
	#if (defined (_MSC_VER) && defined (_WIN_32_VER))
		dgClearFP();
		dgControlFP(m_mask, _MCW_EM);
	#endif
}

