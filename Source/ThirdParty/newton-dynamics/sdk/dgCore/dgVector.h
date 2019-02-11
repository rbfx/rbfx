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

#ifndef __dgVector__
#define __dgVector__

#include "dgStdafx.h"
#include "dgTypes.h"
#include "dgDebug.h"
#include "dgMemory.h"

#define dgCheckVector(x) (dgCheckFloat(x[0]) && dgCheckFloat(x[1]) && dgCheckFloat(x[2]) && dgCheckFloat(x[3]))

#ifdef DG_SCALAR_VECTOR_CLASS
#include "dgVectorScalar.h"
#else
#include "dgVectorSimd.h"
#endif

template<class T>
class dgTemplateVector
{
	public:
	DG_CLASS_ALLOCATOR(allocator)

	DG_INLINE dgTemplateVector () 
	{
	}
	
	DG_INLINE dgTemplateVector (const T* const ptr)
//		:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (T(0.0f))
		:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (ptr[3])
	{
		dgAssert (0);
		//dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgTemplateVector (const dgTemplateVector<T>& copy)
		:m_x(copy.m_x), m_y(copy.m_y), m_z(copy.m_z), m_w (copy.m_w)
	{
		//	dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgTemplateVector (T x, T y, T z, T w) 
		:m_x(x), m_y(y), m_z(z), m_w (w)
	{
	}
	
	DG_INLINE T& operator[] (dgInt32 i)
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}	

	DG_INLINE const T& operator[] (dgInt32 i) const
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}

	DG_INLINE T GetScalar() const
	{
		return m_x;
	}

	DG_INLINE dgTemplateVector<T> Scale (T scale) const
	{
		return dgTemplateVector<T> (m_x * scale, m_y * scale, m_z * scale, m_w * scale);
	}

	DG_INLINE dgTemplateVector<T> operator+ (const dgTemplateVector<T>& B) const
	{
		return dgTemplateVector<T> (m_x + B.m_x, m_y + B.m_y, m_z + B.m_z, m_w + B.m_w);
	}

	DG_INLINE dgTemplateVector<T>& operator+= (const dgTemplateVector<T>& A) 
	{
		return (*this = dgTemplateVector<T> (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w));
	}

	DG_INLINE dgTemplateVector<T> operator- (const dgTemplateVector<T>& A) const
	{
		return dgTemplateVector<T> (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w);
	}

	DG_INLINE dgTemplateVector<T>& operator-= (const dgTemplateVector<T>& A) 
	{
		return (*this = dgTemplateVector<T> (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w));
	}

	DG_INLINE dgTemplateVector<T> operator* (const dgTemplateVector<T>& B) const
	{
		return dgTemplateVector<T>(m_x * B.m_x, m_y * B.m_y, m_z * B.m_z, m_w * B.m_w);
	}

	DG_INLINE dgTemplateVector<T> operator*= (const dgTemplateVector<T>& B) const
	{
		return (*this = dgTemplateVector<T>(m_x * B.m_x, m_y * B.m_y, m_z * B.m_z, m_w * B.m_w));
	}

	DG_INLINE dgTemplateVector<T> AddHorizontal() const
	{
		T val(m_x + m_y + m_z + m_w);
		return dgTemplateVector<T>(val, val, val, val);
	}

	DG_INLINE dgTemplateVector<T> MulAdd(const dgTemplateVector<T>& A, const dgTemplateVector<T>& B) const
	{
		return *this + A * B;
	}

	DG_INLINE dgTemplateVector<T> MulSub(const dgTemplateVector<T>& A, const dgTemplateVector<T>& B) const
	{
		return *this - A * B;
	}

	// return dot product
	DG_INLINE T DotProduct3 (const dgTemplateVector<T>& A) const
	{
		return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
	}

	// return cross product
	DG_INLINE dgTemplateVector<T> CrossProduct (const dgTemplateVector<T>& B) const
	{
		return dgTemplateVector<T> (m_y * B.m_z - m_z * B.m_y,
									m_z * B.m_x - m_x * B.m_z,
									m_x * B.m_y - m_y * B.m_x, m_w);
	}

	DG_INLINE dgTemplateVector<T> CrossProduct(const dgTemplateVector &A, const dgTemplateVector &B) const
	{
		T cofactor[3][3];
		T array[4][4];

		const dgTemplateVector<T>& me = *this;
		for (dgInt32 i = 0; i < 4; i++) {
			array[0][i] = me[i];
			array[1][i] = A[i];
			array[2][i] = B[i];
			array[3][i] = T(1.0f);
		}

		dgTemplateVector<T> normal;
		T sign = T(-1.0f);
		for (dgInt32 i = 0; i < 4; i++) {

			for (dgInt32 j = 0; j < 3; j++) {
				dgInt32 k0 = 0;
				for (dgInt32 k = 0; k < 4; k++) {
					if (k != i) {
						cofactor[j][k0] = array[j][k];
						k0++;
					}
				}
			}
			T x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
			T y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
			T z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
			T det = x + y + z;

			normal[i] = sign * det;
			sign *= T(-1.0f);
		}

		return normal;
	}

	// return dot 4d dot product
	DG_INLINE dgTemplateVector<T> DotProduct (const dgTemplateVector &A) const
	{
		T val (m_x * A.m_x + m_y * A.m_y + m_z * A.m_z + m_w * A.m_w);
		return dgTemplateVector<T> (val, val, val, val);
	}


	T GetMax () const
	{
		return dgMax(dgMax(m_x, m_y), dgMax(m_z, m_w));
	}

	DG_INLINE dgTemplateVector<T> GetMax(const dgTemplateVector<T>& data) const
	{
		return dgTemplateVector<T>((m_x > data.m_x) ? m_x : data.m_x, (m_y > data.m_y) ? m_y : data.m_y, (m_z > data.m_z) ? m_z : data.m_z,	(m_w > data.m_w) ? m_w : data.m_w);
	}

	DG_INLINE dgTemplateVector<T> GetMin(const dgTemplateVector<T>& data) const
	{
		return dgTemplateVector<T>((m_x < data.m_x) ? m_x : data.m_x, (m_y < data.m_y) ? m_y : data.m_y, (m_z < data.m_z) ? m_z : data.m_z,	(m_w < data.m_w) ? m_w : data.m_w);
	}

	// check validity of floats
#ifdef _DEBUG
	void Trace (char* const name) const
	{
		dgTrace (("%s %f %f %f %f\n", name, m_x, m_y, m_z, m_w));
	}
#endif

	T m_x;
	T m_y;
	T m_z;
	T m_w;
};



#endif