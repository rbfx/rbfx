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

#ifndef __dgVectorSimd__
#define __dgVectorSimd__

#ifndef DG_SCALAR_VECTOR_CLASS

#ifdef _NEWTON_USE_DOUBLE
	#define dgVector dgBigVector
#else

class dgBigVector;
// *****************************************************************************************
//
// 4 x 1 single precision SSE vector class declaration
//
// *****************************************************************************************
DG_MSC_VECTOR_ALIGMENT
class dgVector
{
	#define PERMUTE_MASK(w, z, y, x)		_MM_SHUFFLE (w, z, y, x)
	public:
	DG_INLINE dgVector() 
	{
	}

	DG_INLINE dgVector(const __m128 type)
		:m_type (type)
	{
	}

	DG_INLINE dgVector(const __m128i type)
		:m_typeInt (type)
	{
	}

	DG_INLINE dgVector (const dgFloat32 a)
		:m_type(_mm_set_ps1(a)) 
	{
	}

	DG_INLINE dgVector (const dgFloat32* const ptr)
		:m_type(_mm_loadu_ps (ptr))
	{
	}

#ifndef	_NEWTON_USE_DOUBLE
	DG_INLINE dgVector(const dgFloat64* const ptr)
		:m_type(_mm_set_ps(dgFloat32(ptr[3]), dgFloat32(ptr[2]), dgFloat32(ptr[1]), dgFloat32(ptr[0])))
	{
	}
#endif

	DG_INLINE dgVector (const dgVector& copy)
		:m_type(copy.m_type)
	{
	}

	DG_INLINE dgVector (const dgBigVector& copy)
		:m_type(_mm_shuffle_ps (_mm_cvtpd_ps (((__m128d*)&copy)[0]), _mm_cvtpd_ps (((__m128d*)&copy)[1]), PERMUTE_MASK(1, 0, 1, 0)))
	{
		dgAssert (dgCheckVector ((*this)));
	}

	DG_INLINE dgVector (dgFloat32 x, dgFloat32 y, dgFloat32 z, dgFloat32 w)
		:m_type(_mm_set_ps(w, z, y, x))
	{
	}

	DG_INLINE dgVector (dgInt32 ix, dgInt32 iy, dgInt32 iz, dgInt32 iw)
		:m_type(_mm_set_ps(*(dgFloat32*)&iw, *(dgFloat32*)&iz, *(dgFloat32*)&iy, *(dgFloat32*)&ix))
	{
	}

	DG_INLINE dgFloat32 GetScalar () const
	{
		//return m_x;
		return _mm_cvtss_f32 (m_type);
	}

	DG_INLINE void Store (dgFloat32* const dst) const
	{
		_mm_storeu_ps(dst, m_type);
	}

	DG_INLINE dgVector BroadcastX () const
	{
		return _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(0, 0, 0, 0));
	}

	DG_INLINE dgVector BroadcastY () const
	{
		return _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(1, 1, 1, 1));
	}

	DG_INLINE dgVector BroadcastZ () const
	{
		return _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(2, 2, 2, 2));
	}

	DG_INLINE dgVector BroadcastW () const
	{
		return _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(3, 3, 3, 3));
	}

	DG_INLINE dgVector Scale (dgFloat32 s) const
	{
		return _mm_mul_ps (m_type, _mm_set_ps1(s));
	}

	DG_INLINE dgFloat32& operator[] (dgInt32 i)
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return m_f[i];
	}

	DG_INLINE const dgFloat32& operator[] (dgInt32 i) const
	{
		dgAssert (i < 4);
		dgAssert (i >= 0);
		return m_f[i];
	}

	DG_INLINE dgVector operator+ (const dgVector& A) const
	{
		return _mm_add_ps (m_type, A.m_type);	
	}

	DG_INLINE dgVector operator- (const dgVector& A) const 
	{
		return _mm_sub_ps (m_type, A.m_type);	
	}

	DG_INLINE dgVector operator* (const dgVector& A) const
	{
		return _mm_mul_ps(m_type, A.m_type);
	}

	DG_INLINE dgVector& operator+= (const dgVector& A)
	{
		return (*this = _mm_add_ps (m_type, A.m_type));
	}

	DG_INLINE dgVector& operator-= (const dgVector& A)
	{
		return (*this = _mm_sub_ps (m_type, A.m_type));
	}

	DG_INLINE dgVector& operator*= (const dgVector& A)
	{
		return (*this = _mm_mul_ps(m_type, A.m_type));
	}

	// return cross product
	DG_INLINE dgVector CrossProduct (const dgVector& B) const
	{
		return _mm_sub_ps (_mm_mul_ps (_mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(3, 0, 2, 1)), _mm_shuffle_ps (B.m_type, B.m_type, PERMUTE_MASK(3, 1, 0, 2))),
			   _mm_mul_ps (_mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(3, 1, 0, 2)), _mm_shuffle_ps (B.m_type, B.m_type, PERMUTE_MASK(3, 0, 2, 1))));
	}

	// return dot product
	DG_INLINE dgFloat32 DotProduct3(const dgVector& A) const
	{
		dgVector tmp(A & m_triplexMask);
		dgAssert((m_w * tmp.m_w) == dgFloat32(0.0f));
		return (*this * tmp).AddHorizontal().GetScalar();
	}

	DG_INLINE dgVector DotProduct(const dgVector& A) const
	{
		return (*this * A).AddHorizontal();
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
			dgFloat32  x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
			dgFloat32  y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
			dgFloat32  z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
			dgFloat32  det = x + y + z;

			normal[i] = sign * det;
			sign *= dgFloat32 (-1.0f);
		}

		return normal;
	}

	DG_INLINE dgVector Reciproc () const
	{
		return _mm_div_ps (m_one.m_type, m_type);
	}

	DG_INLINE dgVector MulAdd(const dgVector& A, const dgVector& B) const
	{
		return _mm_add_ps(m_type, _mm_mul_ps(A.m_type, B.m_type));
	}

	DG_INLINE dgVector MulSub(const dgVector& A, const dgVector& B) const
	{
		return _mm_sub_ps(m_type, _mm_mul_ps(A.m_type, B.m_type));
	}

	DG_INLINE dgVector AddHorizontal () const
	{
		__m128 tmp (_mm_hadd_ps (m_type, m_type));
		return _mm_hadd_ps (tmp, tmp);
	}

	DG_INLINE dgVector Abs () const
	{
		return _mm_and_ps (m_type, m_signMask.m_type);
	}

	dgFloat32 GetMax () const
	{
		__m128 tmp (_mm_max_ps (m_type, _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(1, 0, 3, 2))));
		return dgVector (_mm_max_ps (tmp, _mm_shuffle_ps (tmp, tmp, PERMUTE_MASK(2, 3, 0, 1)))).GetScalar();
	}

	dgVector GetMax (const dgVector& data) const
	{
		return _mm_max_ps (m_type, data.m_type);
	}

	dgVector GetMin (const dgVector& data) const
	{
		return _mm_min_ps (m_type, data.m_type);
	}

	DG_INLINE dgVector GetInt () const
	{
		return dgVector(_mm_cvtps_epi32(Floor().m_type));
	}

	DG_INLINE dgVector TestZero() const
	{
		//return dgVector (_mm_cmpeq_epi32 (m_typeInt, m_zero.m_typeInt)) & m_negOne;
		return m_negOne & (*this == m_zero);
	}

	DG_INLINE dgVector Floor () const
	{
		dgVector truncated (_mm_cvtepi32_ps (_mm_cvttps_epi32 (m_type)));
		dgVector ret (truncated - (dgVector::m_one & (*this < truncated)));
		dgAssert (ret.m_f[0] == dgFloor(m_f[0]));
		dgAssert (ret.m_f[1] == dgFloor(m_f[1]));
		dgAssert (ret.m_f[2] == dgFloor(m_f[2]));
		dgAssert (ret.m_f[3] == dgFloor(m_f[3]));
		return ret;
	}

	DG_INLINE dgVector Sqrt () const
	{
		return dgVector (_mm_sqrt_ps(m_type));
	}

	DG_INLINE dgVector InvSqrt () const
	{
		dgVector tmp0 (_mm_rsqrt_ps(m_type));
		return m_half * tmp0 * (m_three - *this * tmp0 * tmp0);
	}

	DG_INLINE dgVector InvMagSqrt () const
	{
		return DotProduct(*this).InvSqrt();
	}

	DG_INLINE dgVector Normalize () const
	{
		dgAssert (m_w == dgFloat32 (0.0f));
		// somehow this function changes the behavior with 3.13
		//return Scale(dgFloat32 (1.0f) / dgSqrt(DotProduct(*this).GetScalar()));
		return *this * InvMagSqrt ();
	}

	// relational operators
	DG_INLINE dgVector operator> (const dgVector& data) const
	{
		return _mm_cmpgt_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator== (const dgVector& data) const
	{
		return _mm_cmpeq_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator< (const dgVector& data) const
	{
		return _mm_cmplt_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator>= (const dgVector& data) const
	{
		return _mm_cmpge_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator<= (const dgVector& data) const
	{
		return _mm_cmple_ps (m_type, data.m_type);	
	}

	// logical operations
	DG_INLINE dgVector operator& (const dgVector& data) const
	{
		return _mm_and_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator| (const dgVector& data) const
	{
		return _mm_or_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector operator^ (const dgVector& data) const
	{
		return _mm_xor_ps (m_type, data.m_type);	
	}

	DG_INLINE dgVector AndNot (const dgVector& data) const
	{
		return _mm_andnot_ps (data.m_type, m_type);	
	}

	DG_INLINE dgInt32 GetSignMask() const
	{
		return _mm_movemask_ps(m_type);
	} 

	DG_INLINE dgVector ShiftTripleRight () const
	{
		return _mm_shuffle_ps(m_type, m_type, PERMUTE_MASK(3, 1, 0, 2));
	}

	DG_INLINE dgVector ShiftTripleLeft () const
	{
		return _mm_shuffle_ps (m_type, m_type, PERMUTE_MASK(3, 0, 2, 1));
	}

	DG_INLINE dgVector ShiftRightLogical (int bits) const
	{
		return dgVector (_mm_srli_epi32(m_typeInt, bits)); 
	}

	DG_INLINE static void Transpose4x4 (dgVector& dst0, dgVector& dst1, dgVector& dst2, dgVector& dst3, const dgVector& src0, const dgVector& src1, const dgVector& src2, const dgVector& src3)
	{
		__m128 tmp0 (_mm_unpacklo_ps (src0.m_type, src1.m_type));
		__m128 tmp1 (_mm_unpacklo_ps (src2.m_type, src3.m_type));
		__m128 tmp2 (_mm_unpackhi_ps (src0.m_type, src1.m_type));
		__m128 tmp3 (_mm_unpackhi_ps (src2.m_type, src3.m_type));

		dst0 = dgVector (_mm_movelh_ps (tmp0, tmp1));
		dst1 = dgVector (_mm_movehl_ps (tmp1, tmp0));
		dst2 = dgVector (_mm_movelh_ps (tmp2, tmp3));
		dst3 = dgVector (_mm_movehl_ps (tmp3, tmp2));
	}

#ifdef _DEBUG
	DG_INLINE void Trace(char* const name) const
	{
		dgTrace(("%s %f %f %f %f\n", name, m_x, m_y, m_z, m_w));
	}
#else 
	DG_INLINE void Trace(char* const name) const {}
#endif

	DG_CLASS_ALLOCATOR(allocator)

	union {
		dgFloat32 m_f[4];
		dgInt32 m_i[4];
		__m128 m_type;
		__m128i m_typeInt;
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
	static dgVector m_two;
	static dgVector m_half;
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


// *****************************************************************************************
//
// 4 x 1 double precision SSE2 vector class declaration
//
// *****************************************************************************************
DG_MSC_VECTOR_ALIGMENT
class dgBigVector
{
	#define PERMUT_MASK_DOUBLE(y, x)	_MM_SHUFFLE2 (y, x)

	public:
	DG_INLINE dgBigVector()
	{
	}

	DG_INLINE dgBigVector(const dgBigVector& copy)
		:m_typeLow(copy.m_typeLow)
		,m_typeHigh(copy.m_typeHigh)
	{
	}

	DG_INLINE dgBigVector(const __m128d typeLow, const __m128d typeHigh)
		:m_typeLow(typeLow)
		,m_typeHigh(typeHigh)
	{
	}

	DG_INLINE dgBigVector(const __m128i typeLow, const __m128i typeHigh)
		:m_typeIntLow(typeLow)
		,m_typeIntHigh(typeHigh)
	{
	}

	DG_INLINE dgBigVector(const dgFloat64 a)
		:m_typeLow(_mm_set1_pd(a))
		,m_typeHigh(_mm_set1_pd(a))
	{
	}

#ifdef _NEWTON_USE_DOUBLE
	DG_INLINE dgBigVector (const dgFloat32* const ptr)
		:m_typeLow(_mm_loadu_pd(ptr))
//		,m_typeHigh(_mm_set_pd(ptr[3], ptr[2]))
		,m_typeHigh(_mm_loadu_pd(&ptr[2]))
	{
	}
#else

	DG_INLINE dgBigVector(const dgVector& v)
		:m_typeLow(_mm_cvtps_pd (v.m_type))
		,m_typeHigh(_mm_cvtps_pd (_mm_shuffle_ps (v.m_type, v.m_type, PERMUTE_MASK(3, 2, 3, 2))))
	{
		dgAssert(dgCheckVector((*this)));
	}

	DG_INLINE dgBigVector(const dgFloat64* const ptr)
		:m_typeLow(_mm_loadu_pd(ptr))
		//,m_typeHigh(_mm_set_pd(dgFloat64(0.0f), ptr[2]))
		,m_typeHigh(_mm_loadu_pd(&ptr[2]))
	{
	}
#endif


	DG_INLINE dgBigVector(dgFloat64 x, dgFloat64 y, dgFloat64 z, dgFloat64 w)
		:m_typeLow(_mm_set_pd(y, x))
		,m_typeHigh(_mm_set_pd(w, z))
	{
	}

	DG_INLINE dgBigVector(dgInt32 ix, dgInt32 iy, dgInt32 iz, dgInt32 iw)
		:m_ix(dgInt64(ix)), m_iy(dgInt64(iy)), m_iz(dgInt64(iz)), m_iw(dgInt64(iw))
	{
	}

	DG_INLINE dgBigVector(dgInt64 ix, dgInt64 iy, dgInt64 iz, dgInt64 iw)
		:m_ix(ix), m_iy(iy), m_iz(iz), m_iw(iw)
	{
	}


	DG_INLINE dgFloat64& operator[] (dgInt32 i)
	{
		dgAssert(i < 4);
		dgAssert(i >= 0);
		return m_f[i];
	}

	DG_INLINE const dgFloat64& operator[] (dgInt32 i) const
	{
		dgAssert(i < 4);
		dgAssert(i >= 0);
		return m_f[i];
	}

	DG_INLINE dgFloat64 GetScalar() const
	{
		return m_x;
	}

	DG_INLINE dgBigVector operator+ (const dgBigVector& A) const
	{
		return dgBigVector(_mm_add_pd(m_typeLow, A.m_typeLow), _mm_add_pd(m_typeHigh, A.m_typeHigh));
	}

	DG_INLINE dgBigVector operator- (const dgBigVector& A) const
	{
		return dgBigVector(_mm_sub_pd(m_typeLow, A.m_typeLow), _mm_sub_pd(m_typeHigh, A.m_typeHigh));
	}

	DG_INLINE dgBigVector operator* (const dgBigVector& A) const
	{
		return dgBigVector(_mm_mul_pd(m_typeLow, A.m_typeLow), _mm_mul_pd(m_typeHigh, A.m_typeHigh));
	}

	DG_INLINE dgBigVector& operator+= (const dgBigVector& A)
	{
		m_typeLow = _mm_add_pd(m_typeLow, A.m_typeLow);
		m_typeHigh = _mm_add_pd(m_typeHigh, A.m_typeHigh);
		return *this;
	}

	DG_INLINE dgBigVector& operator-= (const dgBigVector& A)
	{
		m_typeLow = _mm_sub_pd(m_typeLow, A.m_typeLow);
		m_typeHigh = _mm_sub_pd(m_typeHigh, A.m_typeHigh);
		return *this;
	}

	DG_INLINE dgBigVector& operator*= (const dgBigVector& A)
	{
		m_typeLow = _mm_mul_pd(m_typeLow, A.m_typeLow);
		m_typeHigh = _mm_mul_pd(m_typeHigh, A.m_typeHigh);
		return *this;
	}

	DG_INLINE dgBigVector MulAdd(const dgBigVector& A, const dgBigVector& B) const
	{
		return *this + A * B;
	}

	DG_INLINE dgBigVector MulSub(const dgBigVector& A, const dgBigVector& B) const
	{
		return *this - A * B;
	}

	// return cross product
	DG_INLINE dgBigVector CrossProduct(const dgBigVector& B) const
	{
		//dgBigVector tmp0(ShiftTripleLeft());
		//dgBigVector tmp1(B.ShiftTripleRight());
		//dgBigVector tmp2(ShiftTripleRight());
		//dgBigVector tmp3(B.ShiftTripleLeft());
		//return tmp0 * tmp1 - tmp2 * tmp3;
		return dgBigVector(m_y * B.m_z - m_z * B.m_y, m_z * B.m_x - m_x * B.m_z, m_x * B.m_y - m_y * B.m_x, m_w);
	}

	DG_INLINE dgBigVector AddHorizontal() const
	{
		__m128d tmp0(_mm_add_pd(m_typeHigh, m_typeLow));
		__m128d tmp1(_mm_hadd_pd(tmp0, tmp0));
		return dgBigVector(tmp1, tmp1);
	}

	DG_INLINE dgBigVector BroadcastX() const
	{
		return dgBigVector(m_x);
	}

	DG_INLINE dgBigVector BroadcastY() const
	{
		return dgBigVector(m_y);
	}

	DG_INLINE dgBigVector BroadcastZ() const
	{
		return dgBigVector(m_z);
	}

	DG_INLINE dgBigVector BroadcastW() const
	{
		return dgBigVector(m_w);
	}

	DG_INLINE dgBigVector Scale(dgFloat64 s) const
	{
		__m128d tmp0(_mm_set1_pd(s));
		return dgBigVector(_mm_mul_pd(m_typeLow, tmp0), _mm_mul_pd(m_typeHigh, tmp0));
	}

	DG_INLINE dgBigVector Abs() const
	{
		return dgBigVector(_mm_and_pd(m_typeLow, m_signMask.m_typeLow), _mm_and_pd(m_typeHigh, m_signMask.m_typeLow));
	}

	DG_INLINE dgBigVector Reciproc() const
	{
		return dgBigVector(_mm_div_pd(m_one.m_typeLow, m_typeLow), _mm_div_pd(m_one.m_typeHigh, m_typeHigh));
	}

	DG_INLINE dgBigVector Sqrt() const
	{
		return dgBigVector(_mm_sqrt_pd(m_typeLow), _mm_sqrt_pd(m_typeHigh));
	}

	DG_INLINE dgBigVector InvSqrt() const
	{
		return Sqrt().Reciproc();
	}

	DG_INLINE dgBigVector Normalize() const
	{
		dgAssert (m_w == dgFloat32 (0.0f));
		dgFloat64 mag2 = DotProduct(*this).GetScalar();
		return Scale(dgFloat64 (1.0f) / sqrt (mag2));
	}

	dgFloat64 GetMax() const
	{
		__m128d tmp(_mm_max_pd(m_typeLow, m_typeHigh));
		return dgBigVector(_mm_max_pd(tmp, _mm_shuffle_pd(tmp, tmp, PERMUT_MASK_DOUBLE(0, 1))), tmp).GetScalar();
	}

	dgBigVector GetMax(const dgBigVector& data) const
	{
		return dgBigVector(_mm_max_pd(m_typeLow, data.m_typeLow), _mm_max_pd(m_typeHigh, data.m_typeHigh));
	}

	dgBigVector GetMin(const dgBigVector& data) const
	{
		return dgBigVector(_mm_min_pd(m_typeLow, data.m_typeLow), _mm_min_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector GetInt() const
	{
		dgBigVector temp(Floor());
		dgInt64 x = _mm_cvtsd_si32(temp.m_typeLow);
		dgInt64 y = _mm_cvtsd_si32(_mm_shuffle_pd(temp.m_typeLow, temp.m_typeLow, PERMUT_MASK_DOUBLE(1, 1)));
		dgInt64 z = _mm_cvtsd_si32(temp.m_typeHigh);
		dgInt64 w = _mm_cvtsd_si32(_mm_shuffle_pd(temp.m_typeHigh, temp.m_typeHigh, PERMUT_MASK_DOUBLE(1, 1)));
//		return dgBigVector(_mm_set_pd(*(dgFloat32*)&y, *(dgFloat32*)&x), _mm_set_pd(*(dgFloat32*)&w, *(dgFloat32*)&z));
		return dgBigVector(y, x, w, z);
	}

	// relational operators
	DG_INLINE dgBigVector operator> (const dgBigVector& data) const
	{
		return dgBigVector(_mm_cmpgt_pd(m_typeLow, data.m_typeLow), _mm_cmpgt_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator== (const dgBigVector& data) const
	{
		return dgBigVector(_mm_cmpeq_pd(m_typeLow, data.m_typeLow), _mm_cmpeq_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator< (const dgBigVector& data) const
	{
		return dgBigVector(_mm_cmplt_pd(m_typeLow, data.m_typeLow), _mm_cmplt_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator>= (const dgBigVector& data) const
	{
		return dgBigVector(_mm_cmpge_pd(m_typeLow, data.m_typeLow), _mm_cmpge_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator<= (const dgBigVector& data) const
	{
		return dgBigVector(_mm_cmple_pd(m_typeLow, data.m_typeLow), _mm_cmple_pd(m_typeHigh, data.m_typeHigh));
	}

	// logical operations
	DG_INLINE dgBigVector operator& (const dgBigVector& data) const
	{
		return dgBigVector(_mm_and_pd(m_typeLow, data.m_typeLow), _mm_and_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator| (const dgBigVector& data) const
	{
		return dgBigVector(_mm_or_pd(m_typeLow, data.m_typeLow), _mm_or_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector operator^ (const dgBigVector& data) const
	{
		return dgBigVector(_mm_xor_pd(m_typeLow, data.m_typeLow), _mm_xor_pd(m_typeHigh, data.m_typeHigh));
	}

	DG_INLINE dgBigVector AndNot(const dgBigVector& data) const
	{
		return dgBigVector(_mm_andnot_pd(data.m_typeLow, m_typeLow), _mm_andnot_pd(data.m_typeHigh, m_typeHigh));
	}

	DG_INLINE dgBigVector ShiftTripleRight() const
	{
		return dgBigVector(_mm_shuffle_pd(m_typeHigh, m_typeLow, PERMUT_MASK_DOUBLE(0, 0)), _mm_shuffle_pd(m_typeLow, m_typeHigh, PERMUT_MASK_DOUBLE(1, 1)));
	}

	DG_INLINE dgBigVector ShiftTripleLeft() const
	{
		return dgBigVector(_mm_shuffle_pd(m_typeLow, m_typeHigh, PERMUT_MASK_DOUBLE(0, 1)), _mm_shuffle_pd(m_typeLow, m_typeHigh, PERMUT_MASK_DOUBLE(1, 0)));
	}

	DG_INLINE dgBigVector ShiftRightLogical(int bits) const
	{
		//return dgBigVector(dgInt64(dgUnsigned64(m_ix) >> bits), dgInt64(dgUnsigned64(m_iy) >> bits), dgInt64(dgUnsigned64(m_iz) >> bits), dgInt64(dgUnsigned64(m_iw) >> bits));
		return dgBigVector(_mm_srli_epi64(m_typeIntLow, bits), _mm_srli_epi64(m_typeIntHigh, bits));
	}

	DG_INLINE dgInt32 GetSignMask() const
	{
		return _mm_movemask_pd(m_typeLow) | (_mm_movemask_pd(m_typeHigh) << 2);
	}

	DG_INLINE dgBigVector Floor() const
	{
		return dgBigVector(floor(m_x), floor(m_y), floor(m_z), floor(m_w));
	}

	DG_INLINE dgBigVector TestZero() const
	{
		return m_negOne & (*this == m_zero);
	}

	DG_INLINE static void Transpose4x4(dgBigVector& dst0, dgBigVector& dst1, dgBigVector& dst2, dgBigVector& dst3,
		const dgBigVector& src0, const dgBigVector& src1, const dgBigVector& src2, const dgBigVector& src3)
	{
		dgBigVector tmp0(src0);
		dgBigVector tmp1(src1);
		dgBigVector tmp2(src2);
		dgBigVector tmp3(src3);

		dst0 = dgBigVector(tmp0.m_x, tmp1.m_x, tmp2.m_x, tmp3.m_x);
		dst1 = dgBigVector(tmp0.m_y, tmp1.m_y, tmp2.m_y, tmp3.m_y);
		dst2 = dgBigVector(tmp0.m_z, tmp1.m_z, tmp2.m_z, tmp3.m_z);
		dst3 = dgBigVector(tmp0.m_w, tmp1.m_w, tmp2.m_w, tmp3.m_w);
	}

	DG_INLINE dgFloat64 DotProduct3(const dgBigVector& A) const
	{
		return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
	}

	// return dot 4d dot product
	DG_INLINE dgBigVector DotProduct(const dgBigVector &A) const
	{
		return (*this * A).AddHorizontal();
	}

	DG_INLINE dgBigVector CrossProduct(const dgBigVector& A, const dgBigVector& B) const
	{
		dgFloat64 cofactor[3][3];
		dgFloat64 array[4][4];

		const dgBigVector& me = *this;
		for (dgInt32 i = 0; i < 4; i++) {
			array[0][i] = me[i];
			array[1][i] = A[i];
			array[2][i] = B[i];
			array[3][i] = dgFloat64(1.0f);
		}

		dgBigVector normal;
		dgFloat64 sign = dgFloat64(-1.0f);
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
			dgFloat64 x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
			dgFloat64 y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
			dgFloat64 z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
			dgFloat64 det = x + y + z;

			normal[i] = sign * det;
			sign *= dgFloat64(-1.0f);
		}

		return normal;
	}

	DG_CLASS_ALLOCATOR(allocator)

	union
	{
		dgFloat64 m_f[4];
		dgInt64 m_i[4];
		struct
		{
			__m128d m_typeLow;
			__m128d m_typeHigh;
		};
		struct
		{
			__m128i m_typeIntLow;
			__m128i m_typeIntHigh;
		};
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
	static dgBigVector m_two;
	static dgBigVector m_half;
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
		:m_d0(_mm_set1_pd(a))
		,m_d1(_mm_set1_pd(a))
		,m_d2(_mm_set1_pd(a))
	{
	}

#ifdef _NEWTON_USE_DOUBLE
#define PURMUT_MASK2(y, x)		_MM_SHUFFLE2(x, y)
	DG_INLINE dgSpatialVector(const dgVector& low, const dgVector& high)
		:m_d0(low.m_typeLow)
		,m_d1(_mm_shuffle_pd(low.m_typeHigh, high.m_typeLow, PURMUT_MASK2(0, 0)))
		,m_d2(_mm_shuffle_pd(high.m_typeLow, high.m_typeHigh, PURMUT_MASK2(1, 0)))
	{
	}
#else 
	DG_INLINE dgSpatialVector(const dgVector& low, const dgVector& high)
		:m_d0(_mm_cvtps_pd(low.m_type))
		,m_d1(_mm_cvtps_pd(_mm_unpackhi_ps(low.m_type, _mm_shuffle_ps(low.m_type, high.m_type, PERMUTE_MASK(0, 0, 0, 2)))))
		,m_d2(_mm_cvtps_pd(_mm_shuffle_ps(high.m_type, high.m_type, PERMUTE_MASK(3, 3, 2, 1))))
	{
	}
#endif

	DG_INLINE dgSpatialVector(const dgSpatialVector& copy)
		:m_d0(copy.m_d0)
		,m_d1(copy.m_d1)
		,m_d2(copy.m_d2)
	{
	}

	DG_INLINE dgSpatialVector(const __m128d d0, const __m128d d1, const __m128d d2)
		:m_d0(d0)
		,m_d1(d1)
		,m_d2(d2)
	{
	}

	DG_INLINE dgFloat64& operator[] (dgInt32 i)
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return ((dgFloat64*)&m_d0)[i];
	}

	DG_INLINE const dgFloat64& operator[] (dgInt32 i) const
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return ((dgFloat64*)&m_d0)[i];
	}

	DG_INLINE dgSpatialVector operator+ (const dgSpatialVector& A) const
	{
		return dgSpatialVector(_mm_add_pd(m_d0, A.m_d0), _mm_add_pd(m_d1, A.m_d1), _mm_add_pd(m_d2, A.m_d2));
	}

	DG_INLINE dgSpatialVector operator*(const dgSpatialVector& A) const
	{
		return dgSpatialVector(_mm_mul_pd(m_d0, A.m_d0), _mm_mul_pd(m_d1, A.m_d1), _mm_mul_pd(m_d2, A.m_d2));
	}

	DG_INLINE dgFloat64 DotProduct(const dgSpatialVector& v) const
	{
		dgSpatialVector tmp(*this * v);
		__m128d tmp2(_mm_add_pd(tmp.m_d0, _mm_add_pd(tmp.m_d1, tmp.m_d2)));
		__m128d dot(_mm_hadd_pd(tmp2, tmp2));
		dgFloat64 ret;
		_mm_store_sd(&ret, dot);
		return ret;
	}

	DG_INLINE dgSpatialVector Scale(dgFloat64 s) const
	{
		__m128d tmp(_mm_set1_pd(s));
		return dgSpatialVector(_mm_mul_pd(m_d0, tmp), _mm_mul_pd(m_d1, tmp), _mm_mul_pd(m_d2, tmp));
	}

	__m128d m_d0;
	__m128d m_d1;
	__m128d m_d2;
	static dgSpatialVector m_zero;
} DG_GCC_VECTOR_ALIGMENT;

#endif
#endif
