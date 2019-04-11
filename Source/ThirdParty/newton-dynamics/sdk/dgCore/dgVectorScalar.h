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

#ifndef __dgVectorScalar__
#define __dgVectorScalar__


#ifdef DG_SCALAR_VECTOR_CLASS
// *****************************************************************************************
//
// 4 x 1 single precision vector class declaration
//
// *****************************************************************************************
#ifdef _NEWTON_USE_DOUBLE
	#define dgVector dgBigVector
#else

class dgBigVector;
DG_MSC_VECTOR_ALIGMENT
class dgVector
{
	public:
	DG_INLINE dgVector()
	{
	}

	DG_INLINE dgVector(dgFloat32 val)
		:m_x(val), m_y(val), m_z(val), m_w(val)
	{
	}

	DG_INLINE dgVector (const dgVector& v)
		:m_x(v.m_x), m_y(v.m_y), m_z(v.m_z), m_w(v.m_w)
	{
		//dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgVector (const dgFloat32* const ptr)
		:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (ptr[3])
	{
		dgAssert (dgCheckVector ((*this)));
	}

#ifndef	_NEWTON_USE_DOUBLE
	DG_INLINE dgVector(const dgFloat64* const ptr)
		:m_x(dgFloat32(ptr[0]))
		,m_y(dgFloat32(ptr[1]))
		,m_z(dgFloat32(ptr[2]))
		,m_w(dgFloat32(ptr[3]))
	{
	}
#endif


	DG_INLINE dgVector (dgFloat32 x, dgFloat32 y, dgFloat32 z, dgFloat32 w) 
		:m_x(x), m_y(y), m_z(z), m_w(w)
	{
		dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgVector (dgInt32 ix, dgInt32 iy, dgInt32 iz, dgInt32 iw)
		:m_x(*((dgFloat32*)&ix)), m_y(*((dgFloat32*)&iy)), m_z(*((dgFloat32*)&iz)), m_w(*((dgFloat32*)&iw))
	{
	}

#ifndef  _NEWTON_USE_DOUBLE 
	DG_INLINE dgVector (const dgBigVector& copy)
		:m_x(dgFloat32 (((dgFloat64*)&copy)[0])) 
		,m_y(dgFloat32 (((dgFloat64*)&copy)[1])) 
		,m_z(dgFloat32 (((dgFloat64*)&copy)[2])) 
		,m_w(dgFloat32 (((dgFloat64*)&copy)[3])) 
	{
		dgAssert (dgCheckVector ((*this)));
	}
#endif

	DG_INLINE dgFloat32 GetScalar () const
	{
		return m_x;
	}

	DG_INLINE void Store (dgFloat32* const dst) const
	{
		dst[0] = m_x;
		dst[1] = m_y;
		dst[2] = m_z;
		dst[3] = m_w;
	}

	DG_INLINE dgVector BroadcastX () const
	{
		return dgVector (m_x);
	}

	DG_INLINE dgVector BroadcastY () const
	{
		return dgVector (m_y);
	}

	DG_INLINE dgVector BroadcastZ () const
	{
		return dgVector (m_z);
	}

	DG_INLINE dgVector BroadcastW () const
	{
		return dgVector (m_w);
	}


	DG_INLINE dgFloat32& operator[] (dgInt32 i)
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}

	DG_INLINE const dgFloat32& operator[] (dgInt32 i) const
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}

	DG_INLINE dgVector operator+ (const dgVector& A) const
	{
		return dgVector (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w);
	}

	DG_INLINE dgVector operator- (const dgVector& A) const 
	{
		return dgVector (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w);
	}

	DG_INLINE dgVector operator* (const dgVector& A) const
	{
		return dgVector(m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, m_w * A.m_w);
	}

	DG_INLINE dgVector& operator+= (const dgVector& A)
	{
		return (*this = dgVector (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w));
	}

	DG_INLINE dgVector& operator-= (const dgVector& A)
	{
		return (*this = dgVector (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w));
	}

	DG_INLINE dgVector& operator*= (const dgVector& A)
	{
		return (*this = dgVector(m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, m_w * A.m_w));
	}

	DG_INLINE dgVector MulAdd(const dgVector& A, const dgVector& B) const
	{
		return *this + A * B;
	}

	DG_INLINE dgVector MulSub(const dgVector& A, const dgVector& B) const
	{
		return *this - A * B;
	}

	DG_INLINE dgVector AddHorizontal () const
	{
		return dgVector (m_x + m_y + m_z + m_w);
	}

	DG_INLINE dgVector Scale (dgFloat32 scale) const
	{
		return dgVector (m_x * scale, m_y * scale, m_z * scale, m_w * scale);
	}

	// return dot product
	DG_INLINE dgFloat32 DotProduct3 (const dgVector& A) const
	{
		return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
	}

	// return cross product
	DG_INLINE dgVector CrossProduct (const dgVector& B) const
	{
		return dgVector (m_y * B.m_z - m_z * B.m_y,
			m_z * B.m_x - m_x * B.m_z,
			m_x * B.m_y - m_y * B.m_x, m_w);
	}

	DG_INLINE dgVector CrossProduct (const dgVector& A, const dgVector& B) const
	{
		dgFloat32 cofactor[3][3];
		dgFloat32 array[4][4];

		const dgVector& me = *this;
		for (dgInt32 i = 0; i < 4; i ++) {
			array[0][i] = me[i];
			array[1][i] = A[i];
			array[2][i] = B[i];
			array[3][i] = dgFloat32 (1.0f);
		}

		dgVector normal;
		dgFloat32  sign = dgFloat32 (-1.0f);
		for (dgInt32 i = 0; i < 4; i ++)  {

			for (dgInt32 j = 0; j < 3; j ++) {
				dgInt32 k0 = 0;
				for (dgInt32 k = 0; k < 4; k ++) {
					if (k != i) {
						cofactor[j][k0] = array[j][k];
						k0 ++;
					}
				}
			}
			dgFloat32 x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
			dgFloat32 y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
			dgFloat32 z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
			dgFloat32 det = x + y + z;

			normal[i] = sign * det;
			sign *= dgFloat32 (-1.0f);
		}

		return normal;
	}

	DG_INLINE dgVector GetInt () const
	{
		return dgVector (dgInt32 (dgFloor (m_x)), dgInt32(dgFloor (m_y)), dgInt32(dgFloor (m_z)), dgInt32 (dgFloor (m_w)));
	}

	DG_INLINE dgVector TestZero() const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		return dgVector ((a[0] == 0) ? dgFloat32 (-1.0f) : dgFloat32 (1.0f),
						 (a[1] == 0) ? dgFloat32 (-1.0f) : dgFloat32 (1.0f),
						 (a[2] == 0) ? dgFloat32 (-1.0f) : dgFloat32 (1.0f),
						 (a[3] == 0) ? dgFloat32 (-1.0f) : dgFloat32 (1.0f));
	}


	DG_INLINE dgVector Floor () const
	{
		return dgVector (dgFloor (m_x), dgFloor (m_y), dgFloor (m_z), dgFloor (m_w));
	}

	DG_INLINE dgVector DotProduct (const dgVector &A) const
	{
		return dgVector (m_x * A.m_x + m_y * A.m_y + m_z * A.m_z + m_w * A.m_w);
	}

	DG_INLINE dgVector Reciproc () const
	{
		return dgVector (dgFloat32 (1.0f) / m_x, dgFloat32 (1.0f) / m_y, dgFloat32 (1.0f) / m_z, dgFloat32 (1.0f) / m_w);
	}

	DG_INLINE dgVector Sqrt () const
	{
		return dgVector (dgSqrt (m_x), dgSqrt (m_y), dgSqrt (m_z), dgSqrt (m_w));
	}

	DG_INLINE dgVector InvSqrt () const
	{
		return dgVector (dgRsqrt (m_x), dgRsqrt (m_y), dgRsqrt (m_z), dgRsqrt (m_w));
	}

	DG_INLINE dgVector InvMagSqrt () const
	{
		return dgVector (dgRsqrt (DotProduct(*this).m_x));
	}

	DG_INLINE dgVector Normalize () const
	{
		dgAssert (m_w == dgFloat32 (0.0f));
		//return *this * dgVector (dgRsqrt (DotProduct(*this).m_x));
		//return Scale (dgRsqrt (DotProduct(*this).GetScalar()));
		const dgVector& me = *this; 
		return me * InvMagSqrt();
	}

	dgVector Abs () const
	{
		return dgVector ((m_x > dgFloat32 (0.0f)) ? m_x : -m_x,
			(m_y > dgFloat32 (0.0f)) ? m_y : -m_y,
			(m_z > dgFloat32 (0.0f)) ? m_z : -m_z,
			(m_w > dgFloat32 (0.0f)) ? m_w : -m_w);
	}

	dgFloat32 GetMax () const
	{
		return dgMax(dgMax(m_x, m_y), dgMax(m_z, m_w));
	}

	dgVector GetMax (const dgVector& data) const
	{
		return dgVector ((m_x > data.m_x) ? m_x : data.m_x,
			(m_y > data.m_y) ? m_y : data.m_y,
			(m_z > data.m_z) ? m_z : data.m_z,
			(m_w > data.m_w) ? m_w : data.m_w);
	}

	dgVector GetMin (const dgVector& data) const
	{
		return dgVector ((m_x < data.m_x) ? m_x : data.m_x,
			(m_y < data.m_y) ? m_y : data.m_y,
			(m_z < data.m_z) ? m_z : data.m_z,
			(m_w < data.m_w) ? m_w : data.m_w);
	}


	// relational operators
	DG_INLINE dgVector operator== (const dgVector& data) const
	{
		return dgVector ((m_x == data.m_x) ? dgInt32 (0xffffffff) : 0,
			(m_y == data.m_y) ? dgInt32 (0xffffffff) : 0,
			(m_z == data.m_z) ? dgInt32 (0xffffffff) : 0,
			(m_w == data.m_w) ? dgInt32 (0xffffffff) : 0);
	}

	DG_INLINE dgVector operator> (const dgVector& data) const
	{
		return dgVector ((m_x > data.m_x) ? dgInt32 (0xffffffff) : 0,
			(m_y > data.m_y) ? dgInt32 (0xffffffff) : 0,
			(m_z > data.m_z) ? dgInt32 (0xffffffff) : 0,
			(m_w > data.m_w) ? dgInt32 (0xffffffff) : 0);
	}

	DG_INLINE dgVector operator< (const dgVector& data) const
	{
		return dgVector ((m_x < data.m_x) ? dgInt32 (0xffffffff) : 0,
			(m_y < data.m_y) ? dgInt32 (0xffffffff) : 0,
			(m_z < data.m_z) ? dgInt32 (0xffffffff) : 0,
			(m_w < data.m_w) ? dgInt32 (0xffffffff) : 0);
	}

	DG_INLINE dgVector operator>= (const dgVector& data) const
	{
		return dgVector ((m_x >= data.m_x) ? dgInt32 (0xffffffff) : 0, 
			(m_y >= data.m_y) ? dgInt32 (0xffffffff) : 0,
			(m_z >= data.m_z) ? dgInt32 (0xffffffff) : 0,
			(m_w >= data.m_w) ? dgInt32 (0xffffffff) : 0);
	}

	DG_INLINE dgVector operator<= (const dgVector& data) const
	{
		return dgVector ((m_x <= data.m_x) ? dgInt32 (0xffffffff) : 0,
			(m_y <= data.m_y) ? dgInt32 (0xffffffff) : 0,
			(m_z <= data.m_z) ? dgInt32 (0xffffffff) : 0,
			(m_w <= data.m_w) ? dgInt32 (0xffffffff) : 0);
	}


	// logical operations
	DG_INLINE dgVector operator& (const dgVector& data) const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		const dgInt32* const b = (dgInt32*)&data.m_x;
		return dgVector (a[0] & b[0], a[1] & b[1], a[2] & b[2], a[3] & b[3]); 
	}

	DG_INLINE dgVector operator| (const dgVector& data) const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		const dgInt32* const b = (dgInt32*)&data.m_x;
		return dgVector (a[0] | b[0], a[1] | b[1], a[2] | b[2], a[3] | b[3]); 
	}

	DG_INLINE dgVector operator^ (const dgVector& data) const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		const dgInt32* const b = (dgInt32*)&data.m_x;
		return dgVector (a[0] ^ b[0], a[1] ^ b[1], a[2] ^ b[2], a[3] ^ b[3]); 
	}

	DG_INLINE dgVector AndNot (const dgVector& data) const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		const dgInt32* const b = (dgInt32*)&data.m_x;
		return dgVector (a[0] & ~b[0], a[1] & ~b[1], a[2] & ~b[2], a[3] & ~b[3]); 
	}

	DG_INLINE dgInt32 GetSignMask() const
	{
		const dgInt32* const a = (dgInt32*)&m_x;
		return (((a[0] & 0x80000000) ? 1 : 0) | ((a[1] & 0x80000000) ? 2 : 0) | ((a[2] & 0x80000000) ? 4 : 0) | ((a[3] & 0x80000000) ? 8 : 0));
	} 

	DG_INLINE dgVector ShiftTripleRight () const
	{
		return dgVector (m_z, m_x, m_y, m_w); 
	}

	DG_INLINE dgVector ShiftTripleLeft () const
	{
		return dgVector (m_y, m_z, m_x, m_w); 
	}

	DG_INLINE dgVector ShiftRightLogical (int bits) const
	{
		return dgVector (dgInt32 (dgUnsigned32 (m_ix) >> bits), dgInt32 (dgUnsigned32 (m_iy) >> bits), dgInt32 (dgUnsigned32 (m_iz) >> bits), dgInt32 (dgUnsigned32 (m_iw) >> bits)); 
	}

	DG_INLINE static void Transpose4x4 (dgVector& dst0, dgVector& dst1, dgVector& dst2, dgVector& dst3, const dgVector& src0, const dgVector& src1, const dgVector& src2, const dgVector& src3)
	{
		dgVector tmp0 (src0);
		dgVector tmp1 (src1);
		dgVector tmp2 (src2);
		dgVector tmp3 (src3);

		dst0 = dgVector (tmp0.m_x, tmp1.m_x, tmp2.m_x, tmp3.m_x);
		dst1 = dgVector (tmp0.m_y, tmp1.m_y, tmp2.m_y, tmp3.m_y);
		dst2 = dgVector (tmp0.m_z, tmp1.m_z, tmp2.m_z, tmp3.m_z);
		dst3 = dgVector (tmp0.m_w, tmp1.m_w, tmp2.m_w, tmp3.m_w);
	}

	DG_CLASS_ALLOCATOR(allocator)

	union {
		dgInt32 m_i[4];
		struct {
			dgFloat32 m_x;
			dgFloat32 m_y;
			dgFloat32 m_z;
			dgFloat32 m_w;
		};
		struct {
			dgInt32 m_ix;
			dgInt32 m_iy;
			dgInt32 m_iz;
			dgInt32 m_iw;
		};
	};

	static dgVector m_zero;
	static dgVector m_one;
	static dgVector m_wOne;
	static dgVector m_half;
	static dgVector m_two;
	static dgVector m_three;
	static dgVector m_negOne;
	static dgVector m_xMask;
	static dgVector m_yMask;
	static dgVector m_zMask;
	static dgVector m_wMask;
	static dgVector m_signMask;
	static dgVector m_triplexMask;
} DG_GCC_VECTOR_ALIGMENT;

#endif

DG_MSC_VECTOR_ALIGMENT
class dgBigVector
{
	public:
	DG_INLINE dgBigVector()
	{
	}

	DG_INLINE dgBigVector(dgFloat64 val)
		:m_x(val), m_y(val), m_z(val), m_w(val)
	{
	}

	DG_INLINE dgBigVector (const dgBigVector& v)
		:m_x(v.m_x), m_y(v.m_y), m_z(v.m_z), m_w(v.m_w)
	{
	}

#ifndef _NEWTON_USE_DOUBLE
	DG_INLINE dgBigVector (const dgVector& v)
		:m_x(v.m_x), m_y(v.m_y), m_z(v.m_z), m_w(v.m_w)
	{
	}

	DG_INLINE dgBigVector (const dgFloat32* const ptr)
		:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (dgFloat32 (0.0f))
	{
		dgAssert (dgCheckVector ((*this)));
	}
#endif

	DG_INLINE dgBigVector (const dgFloat64* const ptr)
		:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (ptr[3])
	{
		dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgBigVector (dgFloat64 x, dgFloat64 y, dgFloat64 z, dgFloat64 w) 
		:m_x(x), m_y(y), m_z(z), m_w(w)
	{
		dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgBigVector (dgInt32 ix, dgInt32 iy, dgInt32 iz, dgInt32 iw)
		:m_ix(ix), m_iy(iy), m_iz(iz), m_iw(iw)
	{
	}

	DG_INLINE dgBigVector (dgInt64 ix, dgInt64 iy, dgInt64 iz, dgInt64 iw)
		:m_ix(ix), m_iy(iy), m_iz(iz), m_iw(iw)
	{
	}

	DG_INLINE dgFloat64 GetScalar () const
	{
		return m_x;
	}

	DG_INLINE void Store (dgFloat64* const dst) const
	{
		dst[0] = m_x;
		dst[1] = m_y;
		dst[2] = m_z;
		dst[3] = m_w;
	}

	DG_INLINE dgBigVector BroadcastX () const
	{
		return dgBigVector (m_x);
	}

	DG_INLINE dgBigVector BroadcastY () const
	{
		return dgBigVector (m_y);
	}

	DG_INLINE dgBigVector BroadcastZ () const
	{
		return dgBigVector (m_z);
	}

	DG_INLINE dgBigVector BroadcastW () const
	{
		return dgBigVector (m_w);
	}


	DG_INLINE dgFloat64& operator[] (dgInt32 i)
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}

	DG_INLINE const dgFloat64& operator[] (dgInt32 i) const
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return (&m_x)[i];
	}

	DG_INLINE dgBigVector operator+ (const dgBigVector& A) const
	{
		return dgBigVector (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w);
	}

	DG_INLINE dgBigVector operator- (const dgBigVector& A) const 
	{
		return dgBigVector (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w);
	}

	DG_INLINE dgBigVector operator* (const dgBigVector& A) const
	{
		return dgBigVector(m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, m_w * A.m_w);
	}

	DG_INLINE dgBigVector& operator+= (const dgBigVector& A)
	{
		return (*this = dgBigVector (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w));
	}

	DG_INLINE dgBigVector& operator-= (const dgBigVector& A)
	{
		return (*this = dgBigVector (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w));
	}

	DG_INLINE dgBigVector& operator*= (const dgBigVector& A)
	{
		return (*this = dgBigVector(m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, m_w * A.m_w));
	}

	DG_INLINE dgBigVector MulAdd(const dgBigVector& A, const dgBigVector& B) const
	{
		return *this + A * B;
	}

	DG_INLINE dgBigVector MulSub(const dgVector& A, const dgBigVector& B) const
	{
		return *this - A * B;
	}


	DG_INLINE dgBigVector AddHorizontal () const
	{
		return dgBigVector (m_x + m_y + m_z + m_w);
	}

	DG_INLINE dgBigVector Scale (dgFloat64 scale) const
	{
		return dgBigVector (m_x * scale, m_y * scale, m_z * scale, m_w * scale);
	}

	// return dot product
	DG_INLINE dgFloat64 DotProduct3 (const dgBigVector& A) const
	{
		return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
	}

	// return cross product
	DG_INLINE dgBigVector CrossProduct (const dgBigVector& B) const
	{
		return dgBigVector (m_y * B.m_z - m_z * B.m_y, m_z * B.m_x - m_x * B.m_z, m_x * B.m_y - m_y * B.m_x, m_w);
	}

	DG_INLINE dgBigVector CrossProduct (const dgBigVector& A, const dgBigVector& B) const
	{
		dgFloat64 cofactor[3][3];
		dgFloat64 array[4][4];

		const dgBigVector& me = *this;
		for (dgInt32 i = 0; i < 4; i ++) {
			array[0][i] = me[i];
			array[1][i] = A[i];
			array[2][i] = B[i];
			array[3][i] = dgFloat32 (1.0f);
		}

		dgBigVector normal;
		dgFloat64  sign = dgFloat64 (-1.0f);
		for (dgInt32 i = 0; i < 4; i ++)  {

			for (dgInt32 j = 0; j < 3; j ++) {
				dgInt32 k0 = 0;
				for (dgInt32 k = 0; k < 4; k ++) {
					if (k != i) {
						cofactor[j][k0] = array[j][k];
						k0 ++;
					}
				}
			}
			dgFloat64 x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
			dgFloat64 y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
			dgFloat64 z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
			dgFloat64 det = x + y + z;

			normal[i] = sign * det;
			sign *= dgFloat64 (-1.0f);
		}

		return normal;
	}

	DG_INLINE dgBigVector GetInt () const
	{
		return dgBigVector (dgInt64 (floor (m_x)), dgInt64(floor (m_y)), dgInt64(floor (m_z)), dgInt64 (floor (m_w)));
	}

	DG_INLINE dgBigVector TestZero() const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		return dgBigVector ((a[0] == 0) ? dgFloat64 (-1.0f) : dgFloat64 (1.0f),
							(a[1] == 0) ? dgFloat64 (-1.0f) : dgFloat64 (1.0f),
							(a[2] == 0) ? dgFloat64 (-1.0f) : dgFloat64 (1.0f),
							(a[3] == 0) ? dgFloat64 (-1.0f) : dgFloat64 (1.0f));
	}


	DG_INLINE dgBigVector Floor () const
	{
		return dgBigVector (floor (m_x), floor (m_y), floor (m_z), floor (m_w));
	}

	DG_INLINE dgBigVector DotProduct (const dgBigVector &A) const
	{
		return dgBigVector (m_x * A.m_x + m_y * A.m_y + m_z * A.m_z + m_w * A.m_w);
	}

	DG_INLINE dgBigVector Reciproc () const
	{
		return dgBigVector (dgFloat64 (1.0f) / m_x, dgFloat64 (1.0f) / m_y, dgFloat64 (1.0f) / m_z, dgFloat64 (1.0f) / m_w);
	}

	DG_INLINE dgBigVector Sqrt () const
	{
		return dgBigVector (sqrt (m_x), sqrt (m_y), sqrt (m_z), sqrt (m_w));
	}

	DG_INLINE dgBigVector InvSqrt () const
	{
		return dgBigVector (dgFloat64 (1.0f) / sqrt (m_x), dgFloat64 (1.0f) / sqrt (m_y), dgFloat64 (1.0f) / sqrt (m_z), dgFloat64 (1.0f) / sqrt (m_w));
	}

	DG_INLINE dgBigVector InvMagSqrt () const
	{
		return dgBigVector (dgFloat64 (1.0f) / sqrt (DotProduct(*this).m_x));
	}

	DG_INLINE dgBigVector Normalize() const
	{
		dgAssert (m_w == dgFloat64 (0.0f));
		//const dgBigVector& me = *this;
		//return *this * dgBigVector (dgRsqrt(DotProduct(*this).m_x));
		return *this * InvMagSqrt();
	}

	dgBigVector Abs () const
	{
		return dgBigVector ((m_x > dgFloat64 (0.0f)) ? m_x : -m_x,
							(m_y > dgFloat64 (0.0f)) ? m_y : -m_y,
							(m_z > dgFloat64 (0.0f)) ? m_z : -m_z,
							(m_w > dgFloat64 (0.0f)) ? m_w : -m_w);
	}

	dgFloat64 GetMax () const
	{
		return dgMax(dgMax(m_x, m_y), dgMax(m_z, m_w));
	}

	dgBigVector GetMax (const dgBigVector& data) const
	{
		return dgBigVector ((m_x > data.m_x) ? m_x : data.m_x,
							(m_y > data.m_y) ? m_y : data.m_y,
							(m_z > data.m_z) ? m_z : data.m_z,
							(m_w > data.m_w) ? m_w : data.m_w);
					}

	dgBigVector GetMin (const dgBigVector& data) const
	{
		return dgBigVector ((m_x < data.m_x) ? m_x : data.m_x,
							(m_y < data.m_y) ? m_y : data.m_y,
							(m_z < data.m_z) ? m_z : data.m_z,
							(m_w < data.m_w) ? m_w : data.m_w);
	}

	// relational operators
	DG_INLINE dgBigVector operator== (const dgBigVector& data) const
	{
		return dgBigVector ((m_x == data.m_x) ? dgInt64 (-1) : dgInt64 (0),
							(m_y == data.m_y) ? dgInt64 (-1) : dgInt64 (0),
							(m_z == data.m_z) ? dgInt64 (-1) : dgInt64 (0),
							(m_w == data.m_w) ? dgInt64 (-1) : dgInt64 (0));
	}

	DG_INLINE dgBigVector operator> (const dgBigVector& data) const
	{
		return dgBigVector ((m_x > data.m_x) ? dgInt64 (-1) : dgInt64 (0),
							(m_y > data.m_y) ? dgInt64 (-1) : dgInt64 (0),
							(m_z > data.m_z) ? dgInt64 (-1) : dgInt64 (0),
							(m_w > data.m_w) ? dgInt64 (-1) : dgInt64 (0));
	}

	DG_INLINE dgBigVector operator< (const dgBigVector& data) const
	{
		return dgBigVector ((m_x < data.m_x) ? dgInt64 (-1) : dgInt64 (0),
							(m_y < data.m_y) ? dgInt64 (-1) : dgInt64 (0),
							(m_z < data.m_z) ? dgInt64 (-1) : dgInt64 (0),
							(m_w < data.m_w) ? dgInt64 (-1) : dgInt64 (0));
	}

	DG_INLINE dgBigVector operator>= (const dgBigVector& data) const
	{
		return dgBigVector ((m_x >= data.m_x) ? dgInt64 (-1) : dgInt64 (0), 
							(m_y >= data.m_y) ? dgInt64 (-1) : dgInt64 (0),
							(m_z >= data.m_z) ? dgInt64 (-1) : dgInt64 (0),
							(m_w >= data.m_w) ? dgInt64 (-1) : dgInt64 (0));
	}

	DG_INLINE dgBigVector operator<= (const dgBigVector& data) const
	{
		return dgBigVector ((m_x <= data.m_x) ? dgInt64 (-1) : dgInt64 (0),
							(m_y <= data.m_y) ? dgInt64 (-1) : dgInt64 (0),
							(m_z <= data.m_z) ? dgInt64 (-1) : dgInt64 (0),
							(m_w <= data.m_w) ? dgInt64 (-1) : dgInt64 (0));
	}


	// logical operations
	DG_INLINE dgBigVector operator& (const dgBigVector& data) const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		const dgInt64* const b = (dgInt64*)&data.m_x;
		return dgBigVector (a[0] & b[0], a[1] & b[1], a[2] & b[2], a[3] & b[3]); 
	}

	DG_INLINE dgBigVector operator| (const dgBigVector& data) const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		const dgInt64* const b = (dgInt64*)&data.m_x;
		return dgBigVector (a[0] | b[0], a[1] | b[1], a[2] | b[2], a[3] | b[3]); 
	}

	DG_INLINE dgBigVector operator^ (const dgBigVector& data) const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		const dgInt64* const b = (dgInt64*)&data.m_x;
		return dgBigVector (a[0] ^ b[0], a[1] ^ b[1], a[2] ^ b[2], a[3] ^ b[3]); 
	}

	DG_INLINE dgBigVector AndNot (const dgBigVector& data) const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		const dgInt64* const b = (dgInt64*)&data.m_x;
		return dgBigVector (a[0] & ~b[0], a[1] & ~b[1], a[2] & ~b[2], a[3] & ~b[3]); 
	}

	DG_INLINE dgInt32 GetSignMask() const
	{
		const dgInt64* const a = (dgInt64*)&m_x;
		return (((a[0]>>63) ? 1 : 0) | ((a[1]>>63) ? 2 : 0) | ((a[2]>>63) ? 4 : 0) | ((a[3]>>63) ? 8 : 0));
	} 

	DG_INLINE dgBigVector ShiftTripleRight () const
	{
		return dgBigVector (m_z, m_x, m_y, m_w); 
	}

	DG_INLINE dgBigVector ShiftTripleLeft () const
	{
		return dgBigVector (m_y, m_z, m_x, m_w); 
	}

	DG_INLINE dgBigVector ShiftRightLogical (int bits) const
	{
		return dgBigVector (dgInt64 (dgUnsigned64 (m_ix) >> bits), dgInt64 (dgUnsigned64 (m_iy) >> bits), dgInt64 (dgUnsigned64 (m_iz) >> bits), dgInt64 (dgUnsigned64 (m_iw) >> bits)); 
	}

	DG_INLINE static void Transpose4x4 (dgBigVector& dst0, dgBigVector& dst1, dgBigVector& dst2, dgBigVector& dst3, const dgBigVector& src0, const dgBigVector& src1, const dgBigVector& src2, const dgBigVector& src3)
	{
		dgBigVector tmp0 (src0);
		dgBigVector tmp1 (src1);
		dgBigVector tmp2 (src2);
		dgBigVector tmp3 (src3);

		dst0 = dgBigVector (tmp0.m_x, tmp1.m_x, tmp2.m_x, tmp3.m_x);
		dst1 = dgBigVector (tmp0.m_y, tmp1.m_y, tmp2.m_y, tmp3.m_y);
		dst2 = dgBigVector (tmp0.m_z, tmp1.m_z, tmp2.m_z, tmp3.m_z);
		dst3 = dgBigVector (tmp0.m_w, tmp1.m_w, tmp2.m_w, tmp3.m_w);
	}

	DG_CLASS_ALLOCATOR(allocator)

	union 
	{
		dgInt64 m_i[4];
		struct 
		{
			dgFloat64 m_x;
			dgFloat64 m_y;
			dgFloat64 m_z;
			dgFloat64 m_w;
		};
		struct 
		{
			dgInt64 m_ix;
			dgInt64 m_iy;
			dgInt64 m_iz;
			dgInt64 m_iw;
		};
	};

	static dgBigVector m_zero;
	static dgBigVector m_one;
	static dgBigVector m_wOne;
	static dgBigVector m_half;
	static dgBigVector m_two;
	static dgBigVector m_three;
	static dgBigVector m_negOne;
	static dgBigVector m_xMask;
	static dgBigVector m_yMask;
	static dgBigVector m_zMask;
	static dgBigVector m_wMask;
	static dgBigVector m_signMask;
	static dgBigVector m_triplexMask;
} DG_GCC_VECTOR_ALIGMENT;


DG_MSC_VECTOR_ALIGMENT
class dgSpatialVector
{
	public:
	DG_INLINE dgSpatialVector()
	{
	}

	DG_INLINE dgSpatialVector(const dgFloat32 a)
	{
		for (dgInt32 i = 0; i < 6; i ++) {
			m_d[i] = a;
		}
	}

	DG_INLINE dgSpatialVector(const dgVector& low, const dgVector& high)
	{
		for (dgInt32 i = 0; i < 3; i ++) {
			m_d[i] = low[i];
			m_d[i + 3] = high[i];
		}
	}

	DG_INLINE dgSpatialVector(const dgSpatialVector& src)
	{
		for (dgInt32 i = 0; i < 6; i++) {
			m_d[i] = src[i];
		}
	}

	DG_INLINE dgFloat64& operator[] (dgInt32 i)
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return m_d[i];
	}

	DG_INLINE const dgFloat64& operator[] (dgInt32 i) const
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return m_d[i];
	}

	DG_INLINE dgSpatialVector operator+ (const dgSpatialVector& A) const
	{
		dgSpatialVector tmp;
		for (dgInt32 i = 0; i < 6; i++) {
			tmp[i] = m_d[i] + A.m_d[i];
		}
		return tmp;
	}

	DG_INLINE dgSpatialVector operator* (const dgSpatialVector& A) const
	{
		dgSpatialVector tmp;
		for (dgInt32 i = 0; i < 6; i++) {
			tmp[i] = m_d[i] * A.m_d[i];
		}
		return tmp;
	}

	DG_INLINE dgFloat64 DotProduct(const dgSpatialVector& v) const
	{
		dgFloat64 ret = dgFloat64 (0.0f);
		for (dgInt32 i = 0; i < 6; i++) {
			ret += m_d[i] * v.m_d[i];
		}
		return ret;
	}

	DG_INLINE dgSpatialVector Scale(dgFloat64 s) const
	{
		dgSpatialVector tmp;
		for (dgInt32 i = 0; i < 6; i++) {
			tmp[i] = m_d[i] * s;
		}
		return tmp;
	}

	dgFloat64 m_d[6];
	static dgSpatialVector m_zero;
} DG_GCC_VECTOR_ALIGMENT;

#endif
#endif
