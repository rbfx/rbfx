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

#ifndef _DG_SOLVER_H_
#define _DG_SOLVER_H_


#include "dgPhysicsStdafx.h"


#define DG_SOA_WORD_GROUP_SIZE	8 


DG_MSC_AVX_ALIGMENT
class dgSoaFloat
{
	public:
	#define PERMUTE_MASK(w, z, y, x)		_MM_SHUFFLE (w, z, y, x)

	DG_INLINE dgSoaFloat()
	{
	}

	DG_INLINE dgSoaFloat(const float val)
		:m_low(_mm_set1_ps (val))
		,m_high(_mm_set1_ps(val))
	{
	}

	DG_INLINE dgSoaFloat(const dgVector& type)
		:m_low(_mm_set_ps(dgFloat32(type[0]), dgFloat32(type[1]), dgFloat32(type[2]), dgFloat32(type[3])))
		,m_high(m_low)
	{
	}

	DG_INLINE dgSoaFloat(const dgSoaFloat& copy)
		:m_low(copy.m_low)
		,m_high(copy.m_high)
	{
	}

	DG_INLINE dgSoaFloat(const __m128 low, const __m128 high)
		:m_low(low)
		,m_high(high)
	{
	}

	DG_INLINE dgSoaFloat(const dgVector& low, const dgVector& high)
		:m_low(_mm_set_ps(dgFloat32(low[0]), dgFloat32(low[1]), dgFloat32(low[2]), dgFloat32(low[3])))
		,m_high(_mm_set_ps(dgFloat32(high[0]), dgFloat32(high[1]), dgFloat32(high[2]), dgFloat32(high[3])))
	{
	}

	DG_INLINE float& operator[] (dgInt32 i)
	{
		dgAssert(i < DG_SOA_WORD_GROUP_SIZE);
		dgAssert(i >= 0);
		return m_f[i];
	}

	DG_INLINE const float& operator[] (dgInt32 i) const
	{
		dgAssert(i < DG_SOA_WORD_GROUP_SIZE);
		dgAssert(i >= 0);
		return m_f[i];
	}

	DG_INLINE dgSoaFloat operator+ (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_add_ps (m_low, A.m_low), _mm_add_ps (m_high, A.m_high)); 
	}

	DG_INLINE dgSoaFloat operator- (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_sub_ps (m_low, A.m_low), _mm_sub_ps (m_high, A.m_high)); 
	}

	DG_INLINE dgSoaFloat operator* (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_mul_ps (m_low, A.m_low), _mm_mul_ps (m_high, A.m_high)); 
	}

	DG_INLINE dgSoaFloat MulAdd(const dgSoaFloat& A, const dgSoaFloat& B) const
	{
		return *this + A * B;
	}

	DG_INLINE dgSoaFloat MulSub(const dgSoaFloat& A, const dgSoaFloat& B) const
	{
		return *this - A * B;
	}
	
	DG_INLINE dgSoaFloat operator> (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_cmpgt_ps (m_low, A.m_low), _mm_cmpgt_ps (m_high, A.m_high));
	}

	DG_INLINE dgSoaFloat operator< (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_cmplt_ps (m_low, A.m_low), _mm_cmplt_ps (m_high, A.m_high));
	}

	DG_INLINE dgSoaFloat operator| (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_or_ps (m_low, A.m_low), _mm_or_ps (m_high, A.m_high));
	}

	DG_INLINE dgSoaFloat AndNot (const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_andnot_ps (m_low, A.m_low), _mm_andnot_ps (m_high, A.m_high));
	}

	DG_INLINE dgSoaFloat GetMin(const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_min_ps (m_low, A.m_low), _mm_min_ps (m_high, A.m_high));
	}

	DG_INLINE dgSoaFloat GetMax(const dgSoaFloat& A) const
	{
		return dgSoaFloat(_mm_max_ps (m_low, A.m_low), _mm_max_ps (m_high, A.m_high));
	}

	DG_INLINE float AddHorizontal() const
	{
		__m128 tmp0 (_mm_add_ps (m_low, m_high));
		__m128 tmp1 (_mm_hadd_ps (tmp0, tmp0));
		__m128 tmp2 (_mm_hadd_ps (tmp1, tmp1));
		return _mm_cvtss_f32 (tmp2);
	}

	DG_INLINE float GetMax() const
	{
		__m128 tmp0 (_mm_max_ps (m_low, m_high));
		__m128 tmp1 (_mm_max_ps (tmp0, _mm_shuffle_ps (tmp0, tmp0, PERMUTE_MASK(1, 0, 3, 2))));
		__m128 tmp2 (_mm_max_ps (tmp1, _mm_shuffle_ps (tmp1, tmp1, PERMUTE_MASK(2, 3, 0, 1))));
		return _mm_cvtss_f32 (tmp2);
	}

	union
	{
		struct
		{
			__m128 m_low;
			__m128 m_high;
		};
		int m_i[DG_SOA_WORD_GROUP_SIZE];
		float m_f[DG_SOA_WORD_GROUP_SIZE];
	};
} DG_GCC_AVX_ALIGMENT;

DG_MSC_AVX_ALIGMENT
class dgSoaVector3
{
	public:
	dgSoaFloat m_x;
	dgSoaFloat m_y;
	dgSoaFloat m_z;
} DG_GCC_AVX_ALIGMENT;


DG_MSC_AVX_ALIGMENT
class dgSoaVector6
{
	public:
	dgSoaVector3 m_linear;
	dgSoaVector3 m_angular;
} DG_GCC_AVX_ALIGMENT;

DG_MSC_AVX_ALIGMENT
class dgSoaJacobianPair
{
	public:
	dgSoaVector6 m_jacobianM0;
	dgSoaVector6 m_jacobianM1;
} DG_GCC_AVX_ALIGMENT;

DG_MSC_AVX_ALIGMENT
class dgSoaMatrixElement
{
	public:
	dgSoaJacobianPair m_Jt;
	dgSoaJacobianPair m_JMinv;

	dgSoaFloat m_force;
	dgSoaFloat m_diagDamp;
	dgSoaFloat m_invJinvMJt;
	dgSoaFloat m_coordenateAccel;
	dgSoaFloat m_normalForceIndex;
	dgSoaFloat m_lowerBoundFrictionCoefficent;
	dgSoaFloat m_upperBoundFrictionCoefficent;
} DG_GCC_AVX_ALIGMENT;

DG_MSC_AVX_ALIGMENT
class dgSolver: public dgParallelBodySolver
{
	public:
	dgSolver(dgWorld* const world, dgMemoryAllocator* const allocator);
	~dgSolver();
	void CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, float timestep);

	private:
	void InitWeights();
	void InitBodyArray();
	void CalculateForces();
	void InitJacobianMatrix();
	void CalculateBodyForce();
	void UpdateForceFeedback();
	void CalculateJointsForce();
	void IntegrateBodiesVelocity();
	void UpdateKinematicFeedback();
	void CalculateJointsAcceleration();
	void CalculateBodiesAcceleration();
	
	void InitBodyArray(dgInt32 threadID);
	void InitInternalForces(dgInt32 threadID);
	void InitJacobianMatrix(dgInt32 threadID);
	void CalculateBodyForce(dgInt32 threadID);
	void UpdateForceFeedback(dgInt32 threadID);
	void TransposeMassMatrix(dgInt32 threadID);
	void CalculateJointsForce(dgInt32 threadID);
	void UpdateRowAcceleration(dgInt32 threadID);
	void IntegrateBodiesVelocity(dgInt32 threadID);
	void UpdateKinematicFeedback(dgInt32 threadID);
	void CalculateJointsAcceleration(dgInt32 threadID);
	void CalculateBodiesAcceleration(dgInt32 threadID);

	static void InitBodyArrayKernel(void* const context, void* const, dgInt32 threadID);
	static void InitInternalForcesKernel(void* const context, void* const, dgInt32 threadID);
	static void InitJacobianMatrixKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateBodyForceKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateForceFeedbackKernel(void* const context, void* const, dgInt32 threadID);
	static void TransposeMassMatrixKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateJointsForceKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateRowAccelerationKernel(void* const context, void* const, dgInt32 threadID);
	static void IntegrateBodiesVelocityKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateKinematicFeedbackKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateBodiesAccelerationKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateJointsAccelerationKernel(void* const context, void* const, dgInt32 threadID);
	
	static dgInt32 CompareJointInfos(const dgJointInfo* const infoA, const dgJointInfo* const infoB, void* notUsed);
	static dgInt32 CompareBodyJointsPairs(const dgBodyJacobianPair* const pairA, const dgBodyJacobianPair* const pairB, void* notUsed);

	DG_INLINE void SortWorkGroup(dgInt32 base) const;
	DG_INLINE void TransposeRow (dgSoaMatrixElement* const row, const dgJointInfo* const jointInfoArray, dgInt32 index);
	DG_INLINE void BuildJacobianMatrix(dgJointInfo* const jointInfo, dgLeftHandSide* const leftHandSide, dgRightHandSide* const righHandSide);
	DG_INLINE float CalculateJointForce(const dgJointInfo* const jointInfo, dgSoaMatrixElement* const massMatrix, const dgSoaFloat* const internalForces) const;

	dgSoaFloat m_soaOne;
	dgSoaFloat m_soaZero;
	dgVector m_zero;
	dgVector m_negOne;
	dgArray<dgSoaMatrixElement> m_massMatrix;
} DG_GCC_AVX_ALIGMENT;


#endif

