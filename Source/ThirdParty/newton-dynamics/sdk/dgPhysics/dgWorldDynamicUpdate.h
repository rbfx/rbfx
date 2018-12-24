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

#ifndef _DG_WORLD_DYNAMICS_H_
#define _DG_WORLD_DYNAMICS_H_


#include "dgPhysicsStdafx.h"
#include "dgWorldDynamicsParallelSolver.h"


#define	DG_BODY_LRU_STEP				2	
#define	DG_MAX_SKELETON_JOINT_COUNT		256
#define DG_MAX_CONTINUE_COLLISON_STEPS	8
#define	DG_SMALL_ISLAND_COUNT			2

#define	DG_FREEZZING_VELOCITY_DRAG		dgFloat32 (0.9f)
#define	DG_PSD_DAMP_TOL					dgFloat32 (1.0e-3f)
#define	DG_SOLVER_MAX_ERROR				(DG_FREEZE_MAG * dgFloat32 (0.5f))


// the solver is a RK order 4, but instead of weighting the intermediate derivative by the usual 1/6, 1/3, 1/3, 1/6 coefficients
// I am using 1/4, 1/4, 1/4, 1/4.
// This is correct.The weighting coefficients of any RK method comes from an arbitrary criteria
// solving for a set of linear equation on the coefficients. 
// The standard coefficients just happen to lead to an accurate result because they are optimal 
// with respect to a second order solution.For differential equations of higher order, then is not 
// clear if the traditional weigh factors are any better than any other set of weighting factors.
// A different set of coefficient generates results that are not much different than the optimal set, 
// but it allows for simpler calculation of the intermediate derivatives and also for less intermediate memory.
// For more detail on the derivation of the Runge Kutta coefficients you can go to:  
// http://pathfinder.scar.utoronto.ca/~dyer/csca57/book_P/node51.html


class dgBody;
class dgDynamicBody;
class dgWorldDynamicUpdateSyncDescriptor;


class dgClusterCallbackStruct
{
	public:
	dgWorld* m_world;
	dgInt32 m_count;
	dgInt32 m_strideInByte;
	void* m_bodyArray;
};

class dgBodyInfo
{
	public:
	dgBody* m_body;
};

class dgJointInfo
{
	public:
	union 
	{
		struct
		{
			dgBody* m_body;
			dgInt32 m_bodyCount;
			dgInt32 m_jointCount;
			dgInt32 m_setId;
			dgInt32 m_unUsed;
		};
		struct 
		{
			dgConstraint* m_joint;
			dgInt32 m_m0;
			dgInt32 m_m1;
			dgInt32 m_pairStart;
			dgInt32 m_pairCount;
		};
	};
	dgFloat32 m_preconditioner0;
	dgFloat32 m_preconditioner1;
};


class dgBodyJacobianPair
{
	public:
	dgInt32 m_bodyIndex;
	dgInt32 m_JointIndex;
};

class dgBodyCluster
{
	public:
	dgInt32 m_bodyCount;
	dgInt32 m_jointCount;
	dgInt32 m_rowCount;
	dgInt32 m_bodyStart;
	dgInt32 m_jointStart;	
	dgInt32 m_rowStart;
	dgInt16 m_hasSoftBodies;
	dgInt16 m_isContinueCollision;
};

template<class T>
class dgQueue
{
	public:
	dgQueue (T* const pool, dgInt32 size)
		:m_pool (pool)
	{
		m_mod = size;
		m_lastIndex = 0;
		m_firstIndex = 0;
	}

	void Insert (T info) 
	{
		m_pool[m_firstIndex] = info;
		m_firstIndex ++;
		if (m_firstIndex >= m_mod) {
			m_firstIndex = 0;
		}
		dgAssert (m_firstIndex != m_lastIndex);
	}

	T Remove () 
	{
		dgAssert (m_firstIndex != m_lastIndex);

		T element = m_pool[m_lastIndex];
		m_lastIndex ++;
		if (m_lastIndex >= m_mod) {
			m_lastIndex = 0;
		}
		
		return element;
	}

	void Reset ()
	{
		m_lastIndex = m_firstIndex;
	}

	bool IsEmpty () const 
	{
		return (m_firstIndex == m_lastIndex);
	}

	dgInt32 m_mod;
	dgInt32 m_firstIndex;
	dgInt32 m_lastIndex;
	T* m_pool;
};

DG_MSC_VECTOR_ALIGMENT
class dgLeftHandSide
{
	public:
	dgJacobianPair m_Jt;
	dgJacobianPair m_JMinv;
} DG_GCC_VECTOR_ALIGMENT;


class dgRightHandSide
{
	public:
	dgFloat32 m_force;
	dgFloat32 m_diagDamp;
	dgFloat32 m_invJinvMJt;
	dgFloat32 m_coordenateAccel;

	dgFloat32 m_lowerBoundFrictionCoefficent;
	dgFloat32 m_upperBoundFrictionCoefficent;
	dgFloat32 m_gyroAccel;
	dgFloat32 m_maxImpact;

	dgFloat32 m_deltaAccel;
	dgFloat32 m_restitution;
	dgFloat32 m_penetration;
	dgFloat32 m_stiffness;

	//	dgFloat32 m_jinvMJt;
	dgForceImpactPair* m_jointFeebackForce;
	dgFloat32 m_penetrationStiffness;
	dgInt32 m_normalForceIndex;
};

class dgJacobianMemory
{
	public:
	void Init (dgWorld* const world, dgInt32 rowsCount, dgInt32 bodyCount);

	dgJacobian* m_internalForcesBuffer;
	dgLeftHandSide* m_leftHandSizeBuffer;
	dgRightHandSide* m_righHandSizeBuffer;
};

class dgWorldDynamicUpdate
{
	public:
	class dgParallelClusterArray;

	dgWorldDynamicUpdate(dgMemoryAllocator* const allocator);
	~dgWorldDynamicUpdate() {}
	void UpdateDynamics (dgFloat32 timestep);
	dgBody* GetClusterBody (const void* const cluster, dgInt32 index) const;

	dgJacobianMemory& GetSolverMemory() { return m_solverMemory; }
	virtual dgInt32 GetJacobianDerivatives (dgContraintDescritor& constraintParamOut, dgJointInfo* const jointInfo, dgConstraint* const constraint, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgInt32 rowCount) const;
	virtual void CalculateNetAcceleration (dgBody* const body, const dgVector& invTimeStep, const dgVector& accNorm) const;

	private:
	static DG_INLINE dgInt32 CompareKey(dgInt32 highA, dgInt32 lowA, dgInt32 highB, dgInt32 lowB);
	static dgInt32 CompareJointInfos(const dgJointInfo* const infoA, const dgJointInfo* const infoB, void* notUsed);
	static dgInt32 CompareClusterInfos (const dgBodyCluster* const clusterA, const dgBodyCluster* const clusterB, void* notUsed);

	void BuildClusters(dgFloat32 timestep);

	dgBodyCluster MergeClusters(const dgBodyCluster* const clusterArray, dgInt32 clustersCount) const;
	dgInt32 SortClusters(const dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 threadID) const;
	
	static dgInt32 CompareBodyJacobianPair(const dgBodyJacobianPair* const infoA, const dgBodyJacobianPair* const infoB, void* notUsed);
	static void IntegrateClustersParallelKernel (void* const context, void* const worldContext, dgInt32 threadID);
	static void CalculateClusterReactionForcesKernel (void* const context, void* const worldContext, dgInt32 threadID);

	void BuildJacobianMatrix (dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const;
	void ResolveClusterForces (dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const;
	void IntegrateReactionsForces(const dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const;
	void BuildJacobianMatrix (const dgBodyInfo* const bodyInfo, dgJointInfo* const jointInfo, dgJacobian* const internalForces, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgFloat32 forceImpulseScale) const;
	void CalculateClusterReactionForces(const dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const;

	void IntegrateInslandParallel(dgParallelClusterArray* const clusters, dgInt32 threadID);
	void CalculateReactionForcesParallel(const dgBodyCluster* const clusters, dgInt32 clustersCount, dgFloat32 timestep);
		
	dgFloat32 CalculateJointForce(const dgJointInfo* const jointInfo, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide) const;
	dgFloat32 CalculateJointForce_3_13(const dgJointInfo* const jointInfo, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide) const;
	dgJacobian IntegrateForceAndToque(dgDynamicBody* const body, const dgVector& force, const dgVector& torque, const dgVector& timestep) const ;

	void IntegrateExternalForce(const dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 threadID) const;
	void IntegrateVelocity (const dgBodyCluster* const cluster, dgFloat32 accelTolerance, dgFloat32 timestep, dgInt32 threadID) const;
	void CalculateClusterContacts (dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 currLru, dgInt32 threadID) const;
	
	dgInt32 m_bodies;
	dgInt32 m_joints;
	dgInt32 m_clusters;
	dgInt32 m_markLru;
	dgInt32 m_softBodiesCount;
	dgJacobianMemory m_solverMemory;
	dgInt32 m_softBodyCriticalSectionLock;
	dgBodyCluster* m_clusterData;
	
	dgParallelBodySolver m_parallelSolver;
	static dgVector m_velocTol;

	friend class dgWorld;
	friend class dgJacobianMemory;
	friend class dgSkeletonContainer;
	friend class dgParallelBodySolver;
	friend class dgSolverWorlkerThreads;
};

#endif

