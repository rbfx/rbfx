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

#ifndef _DG_PARALLEL_SOLVER_H_
#define _DG_PARALLEL_SOLVER_H_

#include "dgPhysicsStdafx.h"

class dgBodyInfo;
class dgJointInfo;
class dgBodyCluster;
class dgSkeletonContainer;

#define DG_WORK_GROUP_SIZE		8 

DG_MSC_VECTOR_ALIGMENT
class dgWorkGroupFloat
{
	public:
	DG_INLINE dgWorkGroupFloat()
	{
	}

	DG_INLINE dgWorkGroupFloat(const dgWorkGroupFloat& me)
		:m_low(me.m_low)
		,m_high(me.m_high)
	{
	}

	DG_INLINE dgWorkGroupFloat(const dgVector& v)
		:m_low(v)
		,m_high(v)
	{
	}

	DG_INLINE dgWorkGroupFloat(const dgVector& low, const dgVector& high)
		:m_low(low)
		,m_high(high)
	{
	}

	DG_INLINE dgInt32 GetInt(dgInt32 i) const
	{
		dgAssert (i >= 0);
		dgAssert(i < DG_WORK_GROUP_SIZE);
		#ifdef _NEWTON_USE_DOUBLE
		const dgInt64* const ptr = &m_low.m_i[0];
		#else
		const dgInt32* const ptr = &m_low.m_i[0];
		#endif
		return dgInt32 (ptr[i]);
	}

	DG_INLINE void SetInt(dgInt32 i, dgInt32 value)
	{
		dgAssert(i >= 0);
		dgAssert(i < DG_WORK_GROUP_SIZE);

		#ifdef _NEWTON_USE_DOUBLE
		dgInt64* const ptr = &m_low.m_i[0];
		#else
		dgInt32* const ptr = &m_low.m_i[0];
		#endif
		ptr[i] = value;
	}

	DG_INLINE dgFloat32& operator[] (dgInt32 i)
	{
		dgAssert(i >= 0);
		dgAssert(i < DG_WORK_GROUP_SIZE);
		dgFloat32* const ptr = &m_low[0];
		return ptr[i];
	}

	DG_INLINE const dgFloat32& operator[] (dgInt32 i) const
	{
		dgAssert(i >= 0);
		dgAssert(i < DG_WORK_GROUP_SIZE);
		const dgFloat32* const ptr = &m_low[0];
		return ptr[i];
	}

	DG_INLINE dgWorkGroupFloat operator+ (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low + A.m_low, m_high + A.m_high);
	}

	DG_INLINE dgWorkGroupFloat operator- (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low - A.m_low, m_high - A.m_high);
	}

	DG_INLINE dgWorkGroupFloat operator* (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low * A.m_low, m_high * A.m_high);
	}

	DG_INLINE dgWorkGroupFloat MulAdd(const dgWorkGroupFloat& A, const dgWorkGroupFloat& B) const
	{
		return dgWorkGroupFloat(m_low.MulAdd(A.m_low, B.m_low), m_high.MulAdd(A.m_high, B.m_high));
	}

	DG_INLINE dgWorkGroupFloat MulSub(const dgWorkGroupFloat& A, const dgWorkGroupFloat& B) const
	{
		return dgWorkGroupFloat(m_low.MulSub(A.m_low, B.m_low), m_high.MulSub(A.m_high, B.m_high));
	}

	DG_INLINE dgWorkGroupFloat operator> (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low > A.m_low, m_high > A.m_high);
	}

	DG_INLINE dgWorkGroupFloat operator< (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low < A.m_low, m_high < A.m_high);
	}

	DG_INLINE dgWorkGroupFloat operator| (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low | A.m_low, m_high | A.m_high);
	}

	DG_INLINE dgWorkGroupFloat AndNot (const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low.AndNot(A.m_low), m_high.AndNot(A.m_high));
	}

	DG_INLINE dgWorkGroupFloat GetMin(const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low.GetMin(A.m_low), m_high.GetMin(A.m_high));
	}

	DG_INLINE dgWorkGroupFloat GetMax(const dgWorkGroupFloat& A) const
	{
		return dgWorkGroupFloat(m_low.GetMax(A.m_low), m_high.GetMax(A.m_high));
	}

	DG_INLINE dgFloat32 AddHorizontal() const
	{
		return (m_low + m_high).AddHorizontal().GetScalar();
	}

	DG_INLINE dgFloat32 GetMax() const
	{
		return (m_low.GetMax(m_high)).GetMax();
	}

	dgVector m_low;
	dgVector m_high;
	static dgWorkGroupFloat m_one;
	static dgWorkGroupFloat m_zero;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT
class dgWorkGroupVector3
{
	public:
	dgWorkGroupFloat m_x;
	dgWorkGroupFloat m_y;
	dgWorkGroupFloat m_z;
} DG_GCC_VECTOR_ALIGMENT;


DG_MSC_VECTOR_ALIGMENT
class dgWorkGroupVector6
{
	public:
	dgWorkGroupVector3 m_linear;
	dgWorkGroupVector3 m_angular;
} DG_GCC_AVX_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT
class dgSolverSoaJacobianPair
{
	public:
	dgWorkGroupVector6 m_jacobianM0;
	dgWorkGroupVector6 m_jacobianM1;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT
class dgSolverSoaElement
{
	public:
	dgSolverSoaJacobianPair m_Jt;
	dgSolverSoaJacobianPair m_JMinv;

	dgWorkGroupFloat m_force;
	dgWorkGroupFloat m_diagDamp;
	dgWorkGroupFloat m_invJinvMJt;
	dgWorkGroupFloat m_coordenateAccel;
	dgWorkGroupFloat m_normalForceIndex;
	dgWorkGroupFloat m_lowerBoundFrictionCoefficent;
	dgWorkGroupFloat m_upperBoundFrictionCoefficent;
} DG_GCC_VECTOR_ALIGMENT;

class dgParallelBodySolver
{
	public:
	class dgBodyProxy
	{
		public:
		dgFloat32 m_weight;
		dgFloat32 m_invWeight;
		dgInt32 m_lock;
	};

	~dgParallelBodySolver() {}
	dgParallelBodySolver(dgMemoryAllocator* const allocator);

	void CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep);

	private:
	void InitWeights();
	void InitBodyArray();
	void CalculateForces();
	void InitJacobianMatrix();
	void UpdateForceFeedback();
	void CalculateJointsForce();
	void IntegrateBodiesVelocity();
	void UpdateKinematicFeedback();
	void CalculateJointsAcceleration();
	void CalculateBodiesAcceleration();
	
	void InitBodyArray(dgInt32 threadID);
	void InitJacobianMatrix(dgInt32 threadID);
	void UpdateForceFeedback(dgInt32 threadID);
	void TransposeMassMatrix(dgInt32 threadID);
	void CalculateJointsForce(dgInt32 threadID);
	void UpdateRowAcceleration(dgInt32 threadID);
	void IntegrateBodiesVelocity(dgInt32 threadID);
	void UpdateKinematicFeedback(dgInt32 threadID);
	void CalculateJointsAcceleration(dgInt32 threadID);
	void CalculateBodiesAcceleration(dgInt32 threadID);
	
//	static void InitSkeletonsKernel(void* const context, void* const, dgInt32 threadID);
	static void InitBodyArrayKernel(void* const context, void* const, dgInt32 threadID);
	static void InitJacobianMatrixKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateForceFeedbackKernel(void* const context, void* const, dgInt32 threadID);
	static void TransposeMassMatrixKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateJointsForceKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateRowAccelerationKernel(void* const context, void* const, dgInt32 threadID);
	static void IntegrateBodiesVelocityKernel(void* const context, void* const, dgInt32 threadID);
	static void UpdateKinematicFeedbackKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateBodiesAccelerationKernel(void* const context, void* const, dgInt32 threadID);
	static void CalculateJointsAccelerationKernel(void* const context, void* const, dgInt32 threadID);

	static dgInt32 CompareJointInfos(const dgJointInfo* const infoA, const dgJointInfo* const infoB, void* notUsed);

	DG_INLINE void SortWorkGroup (dgInt32 base) const; 
	DG_INLINE void TransposeRow (dgSolverSoaElement* const row, const dgJointInfo* const jointInfoArray, dgInt32 index);
	DG_INLINE dgFloat32 CalculateJointForce(const dgJointInfo* const jointInfo, dgSolverSoaElement* const massMatrix, const dgJacobian* const internalForces) const;
	DG_INLINE void BuildJacobianMatrix(dgJointInfo* const jointInfo, dgLeftHandSide* const leftHandSide, dgRightHandSide* const righHandSide, dgJacobian* const internalForces);

	protected:
	dgWorld* m_world;
	const dgBodyCluster* m_cluster;
	dgBodyInfo* m_bodyArray;
	dgJointInfo* m_jointArray;
	dgBodyProxy* m_bodyProxyArray;
	dgFloat32 m_timestep;
	dgFloat32 m_invTimestep;
	dgFloat32 m_invStepRK;
	dgFloat32 m_timestepRK;
	dgFloat32 m_invTimestepRK;
	dgFloat32 m_firstPassCoef;
	dgFloat32 m_accelNorm[DG_MAX_THREADS_HIVE_COUNT];
	dgInt32 m_hasJointFeeback[DG_MAX_THREADS_HIVE_COUNT];

	dgInt32 m_jointCount;
	dgInt32 m_jacobianMatrixRowAtomicIndex;
	dgInt32 m_solverPasses;
	dgInt32 m_threadCounts;
	dgInt32 m_soaRowsCount;
	dgInt32* m_soaRowStart;
	dgInt32* m_bodyRowStart;

	private:
	dgArray<dgSolverSoaElement> m_massMatrix;
	friend class dgWorldDynamicUpdate;
};

DG_INLINE dgParallelBodySolver::dgParallelBodySolver(dgMemoryAllocator* const allocator)
	:m_world(NULL)
	,m_cluster(NULL)
	,m_bodyArray(NULL)
	,m_jointArray(NULL)
	,m_bodyProxyArray(NULL)
	,m_timestep(dgFloat32(0.0f))
	,m_invTimestep(dgFloat32(0.0f))
	,m_invStepRK(dgFloat32(0.0f))
	,m_timestepRK(dgFloat32(0.0f))
	,m_invTimestepRK(dgFloat32(0.0f))
	,m_firstPassCoef(dgFloat32(0.0f))
	,m_jointCount(0)
	,m_jacobianMatrixRowAtomicIndex(0)
	,m_solverPasses(0)
	,m_threadCounts(0)
	,m_soaRowsCount(0)
	,m_soaRowStart(NULL)
	,m_bodyRowStart(NULL)
	,m_massMatrix(allocator)
{
}

#endif

