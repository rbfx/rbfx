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

#include "dgPhysicsStdafx.h"

#include "dgBody.h"
#include "dgWorld.h"
#include "dgConstraint.h"
#include "dgDynamicBody.h"
#include "dgSkeletonContainer.h"
#include "dgWorldDynamicUpdate.h"
#include "dgWorldDynamicsParallelSolver.h"

#define D_USE_SOA_SOLVER

dgWorkGroupFloat dgWorkGroupFloat::m_one(dgVector::m_one);
dgWorkGroupFloat dgWorkGroupFloat::m_zero(dgVector::m_zero);

class dgWorldDynamicUpdate::dgParallelClusterArray
{
	public:
	dgParallelClusterArray(const dgBodyCluster* const clusterArray, dgInt32 clustersCount, dgFloat32 timestep)
		:m_clusterArray(clusterArray)
		,m_clustersCount(clustersCount)
		,m_timestep(timestep)
		,m_atomicIndex(0)
	{
	}

	const dgBodyCluster* m_clusterArray;
	dgInt32 m_clustersCount;
	dgFloat32 m_timestep;
	dgInt32 m_atomicIndex;
};

void dgWorldDynamicUpdate::CalculateReactionForcesParallel(const dgBodyCluster* const clusterArray, dgInt32 clustersCount, dgFloat32 timestep)
{
	DG_TRACKTIME(__FUNCTION__);
	dgWorld* const world = (dgWorld*) this;

	dgBodyCluster cluster(MergeClusters(clusterArray, clustersCount));
	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[m_bodies];
	dgJointInfo* const jointArray = &world->m_jointsMemory[m_joints];

	if (world->GetCurrentPlugin()) {
		dgWorldPlugin* const plugin = world->GetCurrentPlugin()->GetInfo().m_plugin;
		plugin->CalculateJointForces(cluster, bodyArray, jointArray, timestep);
	} else {
		m_parallelSolver.CalculateJointForces(cluster, bodyArray, jointArray, timestep);
	}

	dgParallelClusterArray integrateCluster(clusterArray, clustersCount, timestep);
	const dgInt32 threadCounts = world->GetThreadCount();
	for (dgInt32 i = 0; i < threadCounts; i++) {
		world->QueueJob(IntegrateClustersParallelKernel, &integrateCluster, world, "dgWorldDynamicUpdate::IntegrateClustersParallelKernel");
	}
	world->SynchronizationBarrier();
}

void dgWorldDynamicUpdate::IntegrateClustersParallelKernel(void* const context, void* const worldPtr, dgInt32 threadID)
{
	dgWorld* const world = (dgWorld*)worldPtr;
	dgParallelClusterArray* const clusterArray = (dgParallelClusterArray*)context;
	world->IntegrateInslandParallel(clusterArray, threadID);
}

void dgWorldDynamicUpdate::IntegrateInslandParallel(dgParallelClusterArray* const clusterArray, dgInt32 threadID)
{
	dgWorld* const world = (dgWorld*) this;
	dgFloat32 timestep = clusterArray->m_timestep;
	const dgInt32 clustersCount = clusterArray->m_clustersCount;
	for (dgInt32 i = dgAtomicExchangeAndAdd(&clusterArray->m_atomicIndex, 1); i < clustersCount; i = dgAtomicExchangeAndAdd(&clusterArray->m_atomicIndex, 1)) {
		world->IntegrateVelocity(&clusterArray->m_clusterArray[i], DG_SOLVER_MAX_ERROR, timestep, 0);
	}
}

dgBodyCluster dgWorldDynamicUpdate::MergeClusters(const dgBodyCluster* const clusterArray, dgInt32 clustersCount) const
{
	DG_TRACKTIME(__FUNCTION__);
	dgBodyCluster cluster;
	dgWorld* const world = (dgWorld*) this;
	dgInt32 bodyCount = 0;
	dgInt32 jointsCount = 0;
	dgInt32 rowCount = 0;
	for (dgInt32 i = 0; i < clustersCount; i++) {
		const dgBodyCluster* const srcCluster = &clusterArray[i];
		bodyCount += srcCluster->m_bodyCount - 1;
		jointsCount += srcCluster->m_jointCount;
		rowCount += srcCluster->m_rowCount;
	}

	world->m_solverMemory.Init(world, rowCount, 2 * bodyCount);
	world->m_bodiesMemory.ResizeIfNecessary((m_bodies + bodyCount + 1) * sizeof(dgBodyInfo));
	world->m_jointsMemory.ResizeIfNecessary(m_joints + jointsCount + 32);

	dgBodyInfo* const bodyPtr = &world->m_bodiesMemory[0];
	dgJointInfo* const constraintPtr = &world->m_jointsMemory[0];

	dgBodyInfo* const bodyArray = &bodyPtr[m_bodies];
	dgJointInfo* const jointArray = &constraintPtr[m_joints];

	bodyArray[0].m_body = world->m_sentinelBody;
	dgAssert(world->m_sentinelBody->m_index == 0);

	dgInt32 rowsCount = 0;
	dgInt32 bodyIndex = 1;
	dgInt32 jointIndex = 0;
	for (dgInt32 i = 0; i < clustersCount; i++) {
		const dgBodyCluster* const srcCluster = &clusterArray[i];
		rowsCount += srcCluster->m_rowCount;

		dgBodyInfo* const srcBodyArray = &bodyPtr[srcCluster->m_bodyStart];
		const dgInt32 count = srcCluster->m_bodyCount;
		for (dgInt32 j = 1; j < count; j++) {
			dgBody* const body = srcBodyArray[j].m_body;
			bodyArray[bodyIndex].m_body = body;
			body->m_index = bodyIndex;
			bodyIndex++;
		}

		dgJointInfo* const clusterJointArray = &constraintPtr[srcCluster->m_jointStart];
		const dgInt32 joints = srcCluster->m_jointCount;
		for (dgInt32 j = 0; j < joints; j++) {
			jointArray[jointIndex] = clusterJointArray[j];
			dgJointInfo* const jointInfo = &jointArray[jointIndex];

			dgConstraint* const constraint = jointInfo->m_joint;
			constraint->m_index = jointIndex;
			const dgBody* const body0 = constraint->GetBody0();
			const dgBody* const body1 = constraint->GetBody1();

			dgInt32 m0 = (body0->GetInvMass().m_w != dgFloat32(0.0f)) ? body0->m_index : 0;
			dgInt32 m1 = (body1->GetInvMass().m_w != dgFloat32(0.0f)) ? body1->m_index : 0;
			jointInfo->m_m0 = m0;
			jointInfo->m_m1 = m1;
			jointIndex++;
		}
	}

	cluster.m_bodyStart = 0;
	cluster.m_jointStart = 0;
	cluster.m_bodyCount = bodyIndex;
	cluster.m_jointCount = jointsCount;
	cluster.m_rowCount = rowsCount;

	cluster.m_rowStart = 0;
	cluster.m_isContinueCollision = 0;
	cluster.m_hasSoftBodies = 0;

	return cluster;
}

void dgParallelBodySolver::InitWeights()
{
	DG_TRACKTIME(__FUNCTION__);
	const dgJointInfo* const jointArray = m_jointArray;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	dgBodyProxy* const weight = m_bodyProxyArray;
	memset(m_bodyProxyArray, 0, m_cluster->m_bodyCount * sizeof(dgBodyProxy));
	for (dgInt32 i = 0; i < jointCount; i++) {
		const dgJointInfo* const jointInfo = &jointArray[i];
		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		weight[m0].m_weight += dgFloat32(1.0f);
		weight[m1].m_weight += dgFloat32(1.0f);
	}
	m_bodyProxyArray[0].m_weight = dgFloat32(1.0f);

	dgFloat32 extraPasses = dgFloat32(0.0f);
	const dgInt32 bodyCount = m_cluster->m_bodyCount;

	dgSkeletonList& skeletonList = *m_world;
	const dgInt32 lru = skeletonList.m_lruMarker;
	skeletonList.m_lruMarker += 1;

	m_skeletonCount = 0;
	for (dgInt32 i = 1; i < bodyCount; i++) {
		extraPasses = dgMax(weight[i].m_weight, extraPasses);

		dgDynamicBody* const body = (dgDynamicBody*)m_bodyArray[i].m_body;
		dgSkeletonContainer* const container = body->GetSkeleton();
		if (container && (container->m_lru != lru)) {
			container->m_lru = lru;
			m_skeletonArray[m_skeletonCount] = container;
			m_skeletonCount ++;
		}
	}
	const dgInt32 conectivity = 7;
	m_solverPasses += 2 * dgInt32(extraPasses) / conectivity + 1;
}

void dgParallelBodySolver::InitBodyArray()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(InitBodyArrayKernel, this, NULL, "dgParallelBodySolver::InitBodyArray");
	}
	m_world->SynchronizationBarrier();
	m_bodyProxyArray->m_invWeight = dgFloat32(1.0f);
}

void dgParallelBodySolver::InitBodyArrayKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->InitBodyArray(threadID);
}

void dgParallelBodySolver::InitJacobianMatrixKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->InitJacobianMatrix(threadID);
}

void dgParallelBodySolver::CalculateJointsForceKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->CalculateJointsForce(threadID);
}

void dgParallelBodySolver::CalculateJointsAccelerationKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->CalculateJointsAcceleration(threadID);
}

void dgParallelBodySolver::IntegrateBodiesVelocityKernel(void* const context, void* const worldContext, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->IntegrateBodiesVelocity(threadID);
}

void dgParallelBodySolver::CalculateBodiesAccelerationKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->CalculateBodiesAcceleration(threadID);
}

void dgParallelBodySolver::UpdateForceFeedbackKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->UpdateForceFeedback(threadID);
}

void dgParallelBodySolver::UpdateKinematicFeedbackKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->UpdateKinematicFeedback(threadID);
}

void dgParallelBodySolver::TransposeMassMatrixKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->TransposeMassMatrix(threadID);
}

void dgParallelBodySolver::UpdateRowAccelerationKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->UpdateRowAcceleration(threadID);
}

DG_INLINE void dgParallelBodySolver::TransposeRow(dgSolverSoaElement* const row, const dgJointInfo* const jointInfoArray, dgInt32 index)
{
	const dgLeftHandSide* const leftHandSide = &m_world->m_solverMemory.m_leftHandSizeBuffer[0];
	const dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	if (jointInfoArray[0].m_pairCount == jointInfoArray[DG_WORK_GROUP_SIZE - 1].m_pairCount) {
		for (dgInt32 i = 0; i < DG_WORK_GROUP_SIZE; i++) {
			const dgJointInfo* const jointInfo = &jointInfoArray[i];
			const dgLeftHandSide* const lhs = &leftHandSide[jointInfo->m_pairStart + index];
			const dgRightHandSide* const rhs = &rightHandSide[jointInfo->m_pairStart + index];

			row->m_Jt.m_jacobianM0.m_linear.m_x[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_x;
			row->m_Jt.m_jacobianM0.m_linear.m_y[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_y;
			row->m_Jt.m_jacobianM0.m_linear.m_z[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_z;
			row->m_Jt.m_jacobianM0.m_angular.m_x[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_x;
			row->m_Jt.m_jacobianM0.m_angular.m_y[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_y;
			row->m_Jt.m_jacobianM0.m_angular.m_z[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_z;
			row->m_Jt.m_jacobianM1.m_linear.m_x[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_x;
			row->m_Jt.m_jacobianM1.m_linear.m_y[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_y;
			row->m_Jt.m_jacobianM1.m_linear.m_z[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_z;
			row->m_Jt.m_jacobianM1.m_angular.m_x[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_x;
			row->m_Jt.m_jacobianM1.m_angular.m_y[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_y;
			row->m_Jt.m_jacobianM1.m_angular.m_z[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_z;

			row->m_JMinv.m_jacobianM0.m_linear.m_x[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_x;
			row->m_JMinv.m_jacobianM0.m_linear.m_y[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_y;
			row->m_JMinv.m_jacobianM0.m_linear.m_z[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_z;
			row->m_JMinv.m_jacobianM0.m_angular.m_x[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_x;
			row->m_JMinv.m_jacobianM0.m_angular.m_y[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_y;
			row->m_JMinv.m_jacobianM0.m_angular.m_z[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_z;
			row->m_JMinv.m_jacobianM1.m_linear.m_x[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_x;
			row->m_JMinv.m_jacobianM1.m_linear.m_y[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_y;
			row->m_JMinv.m_jacobianM1.m_linear.m_z[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_z;
			row->m_JMinv.m_jacobianM1.m_angular.m_x[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_x;
			row->m_JMinv.m_jacobianM1.m_angular.m_y[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_y;
			row->m_JMinv.m_jacobianM1.m_angular.m_z[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_z;

			row->m_force[i] = rhs->m_force;
			row->m_diagDamp[i] = rhs->m_diagDamp;
			row->m_invJinvMJt[i] = rhs->m_invJinvMJt;
			row->m_coordenateAccel[i] = rhs->m_coordenateAccel;
			row->m_normalForceIndex.SetInt(i, rhs->m_normalForceIndex);
			row->m_lowerBoundFrictionCoefficent[i] = rhs->m_lowerBoundFrictionCoefficent;
			row->m_upperBoundFrictionCoefficent[i] = rhs->m_upperBoundFrictionCoefficent;
		}
	} else {
		memset(row, 0, sizeof (dgSolverSoaElement));
		for (dgInt32 i = 0; i < DG_WORK_GROUP_SIZE; i++) {
			if (index < jointInfoArray[i].m_pairCount) {
				const dgJointInfo* const jointInfo = &jointInfoArray[i];
				const dgLeftHandSide* const lhs = &leftHandSide[jointInfo->m_pairStart + index];
				const dgRightHandSide* const rhs = &rightHandSide[jointInfo->m_pairStart + index];

				row->m_Jt.m_jacobianM0.m_linear.m_x[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_x;
				row->m_Jt.m_jacobianM0.m_linear.m_y[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_y;
				row->m_Jt.m_jacobianM0.m_linear.m_z[i] = lhs->m_Jt.m_jacobianM0.m_linear.m_z;
				row->m_Jt.m_jacobianM0.m_angular.m_x[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_x;
				row->m_Jt.m_jacobianM0.m_angular.m_y[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_y;
				row->m_Jt.m_jacobianM0.m_angular.m_z[i] = lhs->m_Jt.m_jacobianM0.m_angular.m_z;
				row->m_Jt.m_jacobianM1.m_linear.m_x[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_x;
				row->m_Jt.m_jacobianM1.m_linear.m_y[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_y;
				row->m_Jt.m_jacobianM1.m_linear.m_z[i] = lhs->m_Jt.m_jacobianM1.m_linear.m_z;
				row->m_Jt.m_jacobianM1.m_angular.m_x[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_x;
				row->m_Jt.m_jacobianM1.m_angular.m_y[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_y;
				row->m_Jt.m_jacobianM1.m_angular.m_z[i] = lhs->m_Jt.m_jacobianM1.m_angular.m_z;

				row->m_JMinv.m_jacobianM0.m_linear.m_x[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_x;
				row->m_JMinv.m_jacobianM0.m_linear.m_y[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_y;
				row->m_JMinv.m_jacobianM0.m_linear.m_z[i] = lhs->m_JMinv.m_jacobianM0.m_linear.m_z;
				row->m_JMinv.m_jacobianM0.m_angular.m_x[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_x;
				row->m_JMinv.m_jacobianM0.m_angular.m_y[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_y;
				row->m_JMinv.m_jacobianM0.m_angular.m_z[i] = lhs->m_JMinv.m_jacobianM0.m_angular.m_z;
				row->m_JMinv.m_jacobianM1.m_linear.m_x[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_x;
				row->m_JMinv.m_jacobianM1.m_linear.m_y[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_y;
				row->m_JMinv.m_jacobianM1.m_linear.m_z[i] = lhs->m_JMinv.m_jacobianM1.m_linear.m_z;
				row->m_JMinv.m_jacobianM1.m_angular.m_x[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_x;
				row->m_JMinv.m_jacobianM1.m_angular.m_y[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_y;
				row->m_JMinv.m_jacobianM1.m_angular.m_z[i] = lhs->m_JMinv.m_jacobianM1.m_angular.m_z;

				row->m_force[i] = rhs->m_force;
				row->m_diagDamp[i] = rhs->m_diagDamp;
				row->m_invJinvMJt[i] = rhs->m_invJinvMJt;
				row->m_coordenateAccel[i] = rhs->m_coordenateAccel;
				row->m_normalForceIndex.SetInt(i, rhs->m_normalForceIndex);
				row->m_lowerBoundFrictionCoefficent[i] = rhs->m_lowerBoundFrictionCoefficent;
				row->m_upperBoundFrictionCoefficent[i] = rhs->m_upperBoundFrictionCoefficent;
			}
			else {
				row->m_normalForceIndex.SetInt(i, DG_INDEPENDENT_ROW);
			}
		}
	}
}

void dgParallelBodySolver::TransposeMassMatrix(dgInt32 threadID)
{
	const dgJointInfo* const jointInfoArray = m_jointArray;
	dgSolverSoaElement* const massMatrixArray = &m_massMatrix[0];

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		const dgInt32 index = i * DG_WORK_GROUP_SIZE;
		const dgInt32 rowCount = jointInfoArray[index].m_pairCount;
		const dgInt32 rowSoaStart = dgAtomicExchangeAndAdd(&m_soaRowsCount, rowCount);
		m_soaRowStart[i] = rowSoaStart;
		for (dgInt32 j = 0; j < rowCount; j++) {
			dgSolverSoaElement* const row = &massMatrixArray[rowSoaStart + j];
			TransposeRow(row, &jointInfoArray[index], j);
		}
	}
}

DG_INLINE void dgParallelBodySolver::BuildJacobianMatrix(dgJointInfo* const jointInfo, dgLeftHandSide* const leftHandSide, dgRightHandSide* const rightHandSide, dgJacobian* const internalForces)
{
	const dgInt32 m0 = jointInfo->m_m0;
	const dgInt32 m1 = jointInfo->m_m1;
	const dgInt32 index = jointInfo->m_pairStart;
	const dgInt32 count = jointInfo->m_pairCount;
	const dgDynamicBody* const body0 = (dgDynamicBody*)m_bodyArray[m0].m_body;
	const dgDynamicBody* const body1 = (dgDynamicBody*)m_bodyArray[m1].m_body;
	const bool isBilateral = jointInfo->m_joint->IsBilateral();

	const dgMatrix invInertia0 = body0->m_invWorldInertiaMatrix;
	const dgMatrix invInertia1 = body1->m_invWorldInertiaMatrix;
	const dgVector invMass0(body0->m_invMass[3]);
	const dgVector invMass1(body1->m_invMass[3]);

	dgWorkGroupFloat force0(dgVector::m_zero);
	if (body0->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
		force0 = dgWorkGroupFloat(body0->m_externalForce, body0->m_externalTorque);
	}

	dgWorkGroupFloat force1(dgVector::m_zero);
	if (body1->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
		force1 = dgWorkGroupFloat(body1->m_externalForce, body1->m_externalTorque);
	}

	jointInfo->m_preconditioner0 = dgFloat32(1.0f);
	jointInfo->m_preconditioner1 = dgFloat32(1.0f);
	if ((invMass0.GetScalar() > dgFloat32(0.0f)) && (invMass1.GetScalar() > dgFloat32(0.0f)) && !(body0->GetSkeleton() && body1->GetSkeleton())) {
		const dgFloat32 mass0 = body0->GetMass().m_w;
		const dgFloat32 mass1 = body1->GetMass().m_w;
		if (mass0 > (DG_DIAGONAL_PRECONDITIONER * mass1)) {
			jointInfo->m_preconditioner0 = mass0 / (mass1 * DG_DIAGONAL_PRECONDITIONER);
		} else if (mass1 > (DG_DIAGONAL_PRECONDITIONER * mass0)) {
			jointInfo->m_preconditioner1 = mass1 / (mass0 * DG_DIAGONAL_PRECONDITIONER);
		}
	}

	dgWorkGroupFloat forceAcc0(dgVector::m_zero);
	dgWorkGroupFloat forceAcc1(dgVector::m_zero);

	const dgWorkGroupFloat weight0(m_bodyProxyArray[m0].m_weight * jointInfo->m_preconditioner0);
	const dgWorkGroupFloat weight1(m_bodyProxyArray[m1].m_weight * jointInfo->m_preconditioner0);

	const dgFloat32 forceImpulseScale = dgFloat32(1.0f);
	const dgFloat32 preconditioner0 = jointInfo->m_preconditioner0;
	const dgFloat32 preconditioner1 = jointInfo->m_preconditioner1;

	for (dgInt32 i = 0; i < count; i++) {
		dgLeftHandSide* const lhs = &leftHandSide[index + i];
		dgRightHandSide* const rhs = &rightHandSide[index + i];

		lhs->m_JMinv.m_jacobianM0.m_linear = lhs->m_Jt.m_jacobianM0.m_linear * invMass0;
		lhs->m_JMinv.m_jacobianM0.m_angular = invInertia0.RotateVector(lhs->m_Jt.m_jacobianM0.m_angular);
		lhs->m_JMinv.m_jacobianM1.m_linear = lhs->m_Jt.m_jacobianM1.m_linear * invMass1;
		lhs->m_JMinv.m_jacobianM1.m_angular = invInertia1.RotateVector(lhs->m_Jt.m_jacobianM1.m_angular);

		const dgWorkGroupFloat& JMinvM0 = (dgWorkGroupFloat&)lhs->m_JMinv.m_jacobianM0;
		const dgWorkGroupFloat& JMinvM1 = (dgWorkGroupFloat&)lhs->m_JMinv.m_jacobianM1;
		dgWorkGroupFloat tmpAccel((JMinvM0 * force0).MulAdd(JMinvM1, force1));

		dgFloat32 extenalAcceleration = -tmpAccel.AddHorizontal();
		rhs->m_deltaAccel = extenalAcceleration * forceImpulseScale;
		rhs->m_coordenateAccel += extenalAcceleration * forceImpulseScale;
		dgAssert(rhs->m_jointFeebackForce);
		const dgFloat32 force = rhs->m_jointFeebackForce->m_force * forceImpulseScale;
		rhs->m_force = isBilateral ? dgClamp(force, rhs->m_lowerBoundFrictionCoefficent, rhs->m_upperBoundFrictionCoefficent) : force;
		rhs->m_maxImpact = dgFloat32(0.0f);

		const dgWorkGroupFloat& JtM0 = (dgWorkGroupFloat&)lhs->m_Jt.m_jacobianM0;
		const dgWorkGroupFloat& JtM1 = (dgWorkGroupFloat&)lhs->m_Jt.m_jacobianM1;

		dgWorkGroupFloat tmpDiag((weight0 * JMinvM0 * JtM0).MulAdd(weight1, JMinvM1 * JtM1));
		dgFloat32 diag = tmpDiag.AddHorizontal();
		dgAssert(diag > dgFloat32(0.0f));
		rhs->m_diagDamp = diag * rhs->m_stiffness;
		diag *= (dgFloat32(1.0f) + rhs->m_stiffness);
		rhs->m_invJinvMJt = dgFloat32(1.0f) / diag;

		dgWorkGroupFloat f0(rhs->m_force * preconditioner0);
		dgWorkGroupFloat f1(rhs->m_force * preconditioner1);
		forceAcc0 = forceAcc0.MulAdd(JtM0, f0);
		forceAcc1 = forceAcc1.MulAdd(JtM1, f1);
	}

	if (m0) {
		dgWorkGroupFloat& out = (dgWorkGroupFloat&)internalForces[m0];
		dgScopeSpinPause lock(&m_bodyProxyArray[m0].m_lock);
		out = out + forceAcc0;
	}
	if (m1) {
		dgWorkGroupFloat& out = (dgWorkGroupFloat&)internalForces[m1];
		dgScopeSpinPause lock(&m_bodyProxyArray[m1].m_lock);
		out = out + forceAcc1;
	}
}

DG_INLINE void dgParallelBodySolver::SortWorkGroup(dgInt32 base) const
{
	dgJointInfo* const jointArray = m_jointArray;
	for (dgInt32 i = 1; i < DG_WORK_GROUP_SIZE; i++) {
		dgInt32 index = base + i;
		const dgJointInfo tmp(jointArray[index]);
		for (; (index > base) && (jointArray[index - 1].m_pairCount < tmp.m_pairCount); index--) {
			jointArray[index] = jointArray[index - 1];
		}
		jointArray[index] = tmp;
	}
}

void dgParallelBodySolver::CalculateBodiesAcceleration()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(CalculateBodiesAccelerationKernel, this, NULL, "dgParallelBodySolver::CalculateBodiesAcceleration");
	}
	m_world->SynchronizationBarrier();
}

void dgParallelBodySolver::UpdateForceFeedback()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(UpdateForceFeedbackKernel, this, NULL, "dgParallelBodySolver::UpdateForceFeedback");
	}
	m_world->SynchronizationBarrier();
}

void dgParallelBodySolver::UpdateKinematicFeedback()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(UpdateKinematicFeedbackKernel, this, NULL, "dgParallelBodySolver::UpdateKinematicFeedback");
	}
	m_world->SynchronizationBarrier();
}

void dgParallelBodySolver::InitJacobianMatrix(dgInt32 threadID)
{
	dgLeftHandSide* const leftHandSide = &m_world->m_solverMemory.m_leftHandSizeBuffer[0];
	dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];

	dgContraintDescritor constraintParams;
	constraintParams.m_world = m_world;
	constraintParams.m_threadIndex = threadID;
	constraintParams.m_timestep = m_timestep;
	constraintParams.m_invTimestep = m_invTimestep;

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		dgJointInfo* const jointInfo = &m_jointArray[i];
		dgConstraint* const constraint = jointInfo->m_joint;
		dgAssert(jointInfo->m_m0 >= 0);
		dgAssert(jointInfo->m_m1 >= 0);
		dgAssert(jointInfo->m_m0 != jointInfo->m_m1);
		const dgInt32 rowBase = dgAtomicExchangeAndAdd(&m_jacobianMatrixRowAtomicIndex, jointInfo->m_pairCount);
		m_world->GetJacobianDerivatives(constraintParams, jointInfo, constraint, leftHandSide, rightHandSide, rowBase);
		BuildJacobianMatrix(jointInfo, leftHandSide, rightHandSide, internalForces);
	}
}

dgInt32 dgParallelBodySolver::CompareJointInfos(const dgJointInfo* const infoA, const dgJointInfo* const infoB, void* notUsed)
{
	const dgInt32 restingA = (infoA->m_joint->m_body0->m_resting & infoA->m_joint->m_body1->m_resting) ? 1 : 0;
	const dgInt32 restingB = (infoB->m_joint->m_body0->m_resting & infoB->m_joint->m_body1->m_resting) ? 1 : 0;

	const dgInt32 countA = (restingA << 24) + infoA->m_pairCount;
	const dgInt32 countB = (restingB << 24) + infoB->m_pairCount;

	if (countA < countB) {
		return 1;
	}
	if (countA > countB) {
		return -1;
	}
	return 0;
}

void dgParallelBodySolver::InitJacobianMatrix()
{
	m_jacobianMatrixRowAtomicIndex = 0;
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];
	memset(internalForces, 0, m_cluster->m_bodyCount * sizeof (dgJacobian));

	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(InitJacobianMatrixKernel, this, NULL, "dgParallelBodySolver::InitJacobianMatrix");
	}
	m_world->SynchronizationBarrier();

#ifdef D_USE_SOA_SOLVER
	dgJointInfo* const jointArray = m_jointArray;
//	dgSort(jointArray, m_cluster->m_jointCount, CompareJointInfos);
	dgParallelSort(*m_world, jointArray, m_cluster->m_jointCount, CompareJointInfos);

	const dgInt32 jointCount = m_jointCount * DG_WORK_GROUP_SIZE;
	for (dgInt32 i = m_cluster->m_jointCount; i < jointCount; i++) {
		memset(&jointArray[i], 0, sizeof(dgJointInfo));
	}

	dgInt32 size = 0;
	for (dgInt32 i = 0; i < jointCount; i += DG_WORK_GROUP_SIZE) {
		const dgConstraint* const joint1 = jointArray[i + DG_WORK_GROUP_SIZE - 1].m_joint;
		if (joint1) {
			if (!(joint1->m_body0->m_resting & joint1->m_body1->m_resting)) {
				const dgConstraint* const joint0 = jointArray[i].m_joint;
				if (joint0->m_body0->m_resting & joint0->m_body1->m_resting) {
					SortWorkGroup(i);
				}
			}
			for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j++) {
				dgConstraint* const joint = jointArray[i + j].m_joint;
				joint->m_index = i + j;
			}
		} else {
			SortWorkGroup(i);
			for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j ++) {
				dgConstraint* const joint = jointArray[i + j].m_joint;
				if (joint) {
					joint->m_index = i + j;
				}
			}
		}
		size += jointArray[i].m_pairCount;
	}
	m_massMatrix.ResizeIfNecessary(size);

	m_soaRowsCount = 0;
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(TransposeMassMatrixKernel, this, NULL, "dgParallelBodySolver::TransposeMassMatrix");
	}
	m_world->SynchronizationBarrier();
#endif
}

void dgParallelBodySolver::InitBodyArray(dgInt32 threadID)
{
	const dgBodyInfo* const bodyArray = m_bodyArray;
	dgBodyProxy* const bodyProxyArray = m_bodyProxyArray;

	const dgInt32 step = m_threadCounts;;
	const dgInt32 bodyCount = m_cluster->m_bodyCount;
	for (dgInt32 i = threadID; i < bodyCount; i += step) {
		const dgBodyInfo* const bodyInfo = &bodyArray[i];
		dgBody* const body = (dgDynamicBody*)bodyInfo->m_body;
		body->AddDampingAcceleration(m_timestep);
		body->CalcInvInertiaMatrix();

		body->m_accel = body->m_veloc;
		body->m_alpha = body->m_omega;

		const dgFloat32 w = bodyProxyArray[i].m_weight ? bodyProxyArray[i].m_weight : dgFloat32(1.0f);
		bodyProxyArray[i].m_weight = w;
		bodyProxyArray[i].m_invWeight = dgFloat32(1.0f) / w;
	}
}

void dgParallelBodySolver::CalculateJointsAcceleration(dgInt32 threadID)
{
	dgJointAccelerationDecriptor joindDesc;
	joindDesc.m_timeStep = m_timestepRK;
	joindDesc.m_invTimeStep = m_invTimestepRK;
	joindDesc.m_firstPassCoefFlag = m_firstPassCoef;
	dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	const dgLeftHandSide* const leftHandSide = &m_world->m_solverMemory.m_leftHandSizeBuffer[0];

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		dgJointInfo* const jointInfo = &m_jointArray[i];
		dgConstraint* const constraint = jointInfo->m_joint;
		const dgInt32 pairStart = jointInfo->m_pairStart;
		joindDesc.m_rowsCount = jointInfo->m_pairCount;
		joindDesc.m_leftHandSide = &leftHandSide[pairStart];
		joindDesc.m_rightHandSide = &rightHandSide[pairStart];

		constraint->JointAccelerations(&joindDesc);
	}
}

void dgParallelBodySolver::CalculateBodiesAcceleration(dgInt32 threadID)
{
	dgVector invTime(m_invTimestep);
	dgFloat32 maxAccNorm2 = DG_SOLVER_MAX_ERROR * DG_SOLVER_MAX_ERROR;

	const dgInt32 step = m_threadCounts;
	const dgInt32 bodyCount = m_cluster->m_bodyCount;
	for (dgInt32 i = threadID; i < bodyCount; i += step) {
		dgDynamicBody* const body = (dgDynamicBody*)m_bodyArray[i].m_body;
		m_world->CalculateNetAcceleration(body, invTime, maxAccNorm2);
	}
}

void dgParallelBodySolver::UpdateKinematicFeedback(dgInt32 threadID)
{
	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		dgJointInfo* const jointInfo = &m_jointArray[i];
		if (jointInfo->m_joint->m_updaFeedbackCallback) {
			jointInfo->m_joint->m_updaFeedbackCallback(*jointInfo->m_joint, m_timestep, threadID);
		}
	}
}

void dgParallelBodySolver::UpdateRowAcceleration(dgInt32 threadID)
{
	dgSolverSoaElement* const massMatrix = &m_massMatrix[0];
	const dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];

	const dgInt32* const soaRowStart = m_soaRowStart;
	const dgJointInfo* const jointInfoArray = m_jointArray;

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		const dgInt32 rowStart = soaRowStart[i];
		const dgJointInfo* const jointInfoBase = &jointInfoArray[i * DG_WORK_GROUP_SIZE];

		for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j++) {
			const dgJointInfo* const jointInfo = &jointInfoBase[j];
			if (jointInfo->m_joint) {
				dgInt32 const rowCount = jointInfo->m_pairCount;
				dgInt32 const rowStartBase = jointInfo->m_pairStart;
				for (dgInt32 k = 0; k < rowCount; k++) {
					dgSolverSoaElement* const row = &massMatrix[rowStart + k];
					row->m_coordenateAccel[j] = rightHandSide[k + rowStartBase].m_coordenateAccel;
				}
			}
		}
	}
}

void dgParallelBodySolver::UpdateForceFeedback(dgInt32 threadID)
{
	const dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	dgInt32 hasJointFeeback = 0;

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		dgJointInfo* const jointInfo = &m_jointArray[i];
		dgConstraint* const constraint = jointInfo->m_joint;
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 count = jointInfo->m_pairCount;

		for (dgInt32 j = 0; j < count; j++) {
			const dgRightHandSide* const rhs = &rightHandSide[j + first];
			dgAssert(dgCheckFloat(rhs->m_force));
			rhs->m_jointFeebackForce->m_force = rhs->m_force;
			rhs->m_jointFeebackForce->m_impact = rhs->m_maxImpact * m_timestepRK;
		}
		hasJointFeeback |= (constraint->m_updaFeedbackCallback ? 1 : 0);
	}
	m_hasJointFeeback[threadID] = hasJointFeeback;
}

void dgParallelBodySolver::IntegrateBodiesVelocity(dgInt32 threadID)
{
	dgVector speedFreeze2(m_world->m_freezeSpeed2 * dgFloat32(0.1f));
	dgVector freezeOmega2(m_world->m_freezeOmega2 * dgFloat32(0.1f));

	dgVector timestep4(m_timestepRK);
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];

	const dgInt32 step = m_threadCounts;
	const dgInt32 bodyCount = m_cluster->m_bodyCount - 1;
	for (dgInt32 j = threadID; j < bodyCount; j += step) {
		const dgInt32 i = j + 1;
		dgDynamicBody* const body = (dgDynamicBody*)m_bodyArray[i].m_body;
		dgAssert(body->m_index == i);

		if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
			const dgJacobian& forceAndTorque = internalForces[i];
			const dgVector force(body->m_externalForce + forceAndTorque.m_linear);
			const dgVector torque(body->m_externalTorque + forceAndTorque.m_angular);

			const dgVector velocStep((force.Scale(body->m_invMass.m_w)) * timestep4);
			const dgVector omegaStep((body->m_invWorldInertiaMatrix.RotateVector(torque)) * timestep4);

			if (!body->m_resting) {
				body->m_veloc += velocStep;
				body->m_omega += omegaStep;
			} else {
				const dgVector velocStep2(velocStep.DotProduct(velocStep));
				const dgVector omegaStep2(omegaStep.DotProduct(omegaStep));
				const dgVector test(((velocStep2 > speedFreeze2) | (omegaStep2 > speedFreeze2)) & dgVector::m_negOne);
				const dgInt32 equilibrium = test.GetSignMask() ? 0 : 1;
				body->m_resting &= equilibrium;
			}
			dgAssert(body->m_veloc.m_w == dgFloat32(0.0f));
			dgAssert(body->m_omega.m_w == dgFloat32(0.0f));
		}
	}
}

void dgParallelBodySolver::IntegrateBodiesVelocity()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(IntegrateBodiesVelocityKernel, this, NULL, "dgParallelBodySolver::IntegrateBodiesVelocity");
	}
	m_world->SynchronizationBarrier();
}

void dgParallelBodySolver::CalculateJointsForce()
{
	const dgInt32 bodyCount = m_cluster->m_bodyCount;
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];
	dgJacobian* const tempInternalForces = &m_world->m_solverMemory.m_internalForcesBuffer[bodyCount];

	memset(tempInternalForces, 0, bodyCount * sizeof(dgJacobian));
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(CalculateJointsForceKernel, this, NULL, "dgParallelBodySolver::CalculateJointsForce");
	}
	m_world->SynchronizationBarrier();
	memcpy(internalForces, tempInternalForces, bodyCount * sizeof(dgJacobian));
}

void dgParallelBodySolver::CalculateJointsAcceleration()
{
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(CalculateJointsAccelerationKernel, this, NULL, "dgParallelBodySolver::CalculateJointsAcceleration");
	}
	m_world->SynchronizationBarrier();
	m_firstPassCoef = dgFloat32(1.0f);

#ifdef D_USE_SOA_SOLVER
	for (dgInt32 i = 0; i < m_threadCounts; i++) {
		m_world->QueueJob(UpdateRowAccelerationKernel, this, NULL, "dgParallelBodySolver::UpdateRowAcceleration");
	}
	m_world->SynchronizationBarrier();
#endif
}

void dgParallelBodySolver::InitSkeletons(dgInt32 threadID)
{
	dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	const dgLeftHandSide* const leftHandSide = &m_world->m_solverMemory.m_leftHandSizeBuffer[0];

	const dgInt32 count = m_skeletonCount;
	const dgInt32 threadCounts = m_world->GetThreadCount();
	dgSkeletonContainer** const skeletonArray = &m_skeletonArray[0];

	for (dgInt32 i = threadID; i < count; i += threadCounts) {
		dgSkeletonContainer* const skeleton = skeletonArray[i];
		skeleton->InitMassMatrix(m_jointArray, leftHandSide, rightHandSide);
	}
}

void dgParallelBodySolver::UpdateSkeletons(dgInt32 threadID)
{
	const dgInt32 count = m_skeletonCount;
	const dgInt32 threadCounts = m_world->GetThreadCount();
	dgSkeletonContainer** const skeletonArray = &m_skeletonArray[0];
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];

	for (dgInt32 i = threadID; i < count; i += threadCounts) {
		dgSkeletonContainer* const skeleton = skeletonArray[i];
		skeleton->CalculateJointForce(m_jointArray, m_bodyArray, internalForces);
	}
}

void dgParallelBodySolver::InitSkeletonsKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->InitSkeletons(threadID);
}

void dgParallelBodySolver::UpdateSkeletonsKernel(void* const context, void* const, dgInt32 threadID)
{
	dgParallelBodySolver* const me = (dgParallelBodySolver*)context;
	me->UpdateSkeletons(threadID);
}

void dgParallelBodySolver::InitSkeletons()
{
	const dgInt32 threadCounts = m_world->GetThreadCount();
	for (dgInt32 i = 0; i < threadCounts; i++) {
		m_world->QueueJob(InitSkeletonsKernel, this, NULL, "dgParallelBodySolver::InitSkeletonsKernel");
	}
	m_world->SynchronizationBarrier();
}

void dgParallelBodySolver::UpdateSkeletons()
{
	const dgInt32 threadCounts = m_world->GetThreadCount();
	for (dgInt32 i = 0; i < threadCounts; i++) {
		m_world->QueueJob(UpdateSkeletonsKernel, this, NULL, "dgParallelBodySolver::UpdateSkeletons");
	}
	m_world->SynchronizationBarrier();
}


#ifdef D_USE_SOA_SOLVER

DG_INLINE dgFloat32 dgParallelBodySolver::CalculateJointForce(const dgJointInfo* const jointInfo, dgSolverSoaElement* const massMatrix, const dgJacobian* const internalForcesPtr) const
{
	dgWorkGroupVector6 forceM0;
	dgWorkGroupVector6 forceM1;
	dgWorkGroupFloat weight0;
	dgWorkGroupFloat weight1;
	dgWorkGroupFloat preconditioner0;
	dgWorkGroupFloat preconditioner1;
	dgWorkGroupFloat accNorm(dgWorkGroupFloat::m_zero);
	dgWorkGroupFloat normalForce[DG_CONSTRAINT_MAX_ROWS + 1];
	const dgBodyProxy* const bodyProxyArray = m_bodyProxyArray;
	const dgWorkGroupFloat* const internalForces = (dgWorkGroupFloat*)internalForcesPtr;

	for (dgInt32 i = 0; i < DG_WORK_GROUP_SIZE; i++) {
		const dgInt32 m0 = jointInfo[i].m_m0;
		const dgInt32 m1 = jointInfo[i].m_m1;

		forceM0.m_linear.m_x[i] = internalForces[m0][0];
		forceM0.m_linear.m_y[i] = internalForces[m0][1];
		forceM0.m_linear.m_z[i] = internalForces[m0][2];
		forceM0.m_angular.m_x[i] = internalForces[m0][4];
		forceM0.m_angular.m_y[i] = internalForces[m0][5];
		forceM0.m_angular.m_z[i] = internalForces[m0][6];

		forceM1.m_linear.m_x[i] = internalForces[m1][0];
		forceM1.m_linear.m_y[i] = internalForces[m1][1];
		forceM1.m_linear.m_z[i] = internalForces[m1][2];
		forceM1.m_angular.m_x[i] = internalForces[m1][4];
		forceM1.m_angular.m_y[i] = internalForces[m1][5];
		forceM1.m_angular.m_z[i] = internalForces[m1][6];

		weight0[i] = bodyProxyArray[m0].m_weight;
		weight1[i] = bodyProxyArray[m1].m_weight;

		preconditioner0[i] = jointInfo[i].m_preconditioner0;
		preconditioner1[i] = jointInfo[i].m_preconditioner1;
	}

	forceM0.m_linear.m_x = forceM0.m_linear.m_x * preconditioner0;
	forceM0.m_linear.m_y = forceM0.m_linear.m_y * preconditioner0;
	forceM0.m_linear.m_z = forceM0.m_linear.m_z * preconditioner0;
	forceM0.m_angular.m_x = forceM0.m_angular.m_x * preconditioner0;
	forceM0.m_angular.m_y = forceM0.m_angular.m_y * preconditioner0;
	forceM0.m_angular.m_z = forceM0.m_angular.m_z * preconditioner0;

	forceM1.m_linear.m_x = forceM1.m_linear.m_x * preconditioner1;
	forceM1.m_linear.m_y = forceM1.m_linear.m_y * preconditioner1;
	forceM1.m_linear.m_z = forceM1.m_linear.m_z * preconditioner1;
	forceM1.m_angular.m_x = forceM1.m_angular.m_x * preconditioner1;
	forceM1.m_angular.m_y = forceM1.m_angular.m_y * preconditioner1;
	forceM1.m_angular.m_z = forceM1.m_angular.m_z * preconditioner1;

	preconditioner0 = preconditioner0 * weight0;
	preconditioner1 = preconditioner1 * weight1;

	const dgInt32 rowsCount = jointInfo->m_pairCount;
	normalForce[0] = dgWorkGroupFloat::m_one;
	for (dgInt32 i = 0; i < rowsCount; i++) {
		dgSolverSoaElement* const row = &massMatrix[i];

		dgWorkGroupFloat a;
		a = row->m_coordenateAccel.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_x, forceM0.m_linear.m_x);
		a = a.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_y, forceM0.m_linear.m_y);
		a = a.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_z, forceM0.m_linear.m_z);
		a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_x, forceM0.m_angular.m_x);
		a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_y, forceM0.m_angular.m_y);
		a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_z, forceM0.m_angular.m_z);

		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_x, forceM1.m_linear.m_x);
		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_y, forceM1.m_linear.m_y);
		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_z, forceM1.m_linear.m_z);
		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_x, forceM1.m_angular.m_x);
		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_y, forceM1.m_angular.m_y);
		a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_z, forceM1.m_angular.m_z);
		a = a.MulSub(row->m_force, row->m_diagDamp);

		dgWorkGroupFloat f(row->m_force.MulAdd(row->m_invJinvMJt, a));

		dgWorkGroupFloat frictionNormal;
		for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j++) {
			dgAssert(row->m_normalForceIndex.GetInt(j) >= -1);
			dgAssert(row->m_normalForceIndex.GetInt(j) <= rowsCount);
			const dgInt32 frictionIndex = row->m_normalForceIndex.GetInt(j) + 1;
			frictionNormal[j] = normalForce[frictionIndex][j];
		}

		dgWorkGroupFloat lowerFrictionForce(frictionNormal * row->m_lowerBoundFrictionCoefficent);
		dgWorkGroupFloat upperFrictionForce(frictionNormal * row->m_upperBoundFrictionCoefficent);

		a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
		f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

		accNorm = accNorm.MulAdd(a, a);
		dgWorkGroupFloat deltaForce(f - row->m_force);

		row->m_force = f;
		normalForce[i + 1] = f;

		dgWorkGroupFloat deltaForce0(deltaForce * preconditioner0);
		dgWorkGroupFloat deltaForce1(deltaForce * preconditioner1);

		forceM0.m_linear.m_x = forceM0.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_x, deltaForce0);
		forceM0.m_linear.m_y = forceM0.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_y, deltaForce0);
		forceM0.m_linear.m_z = forceM0.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_z, deltaForce0);
		forceM0.m_angular.m_x = forceM0.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_x, deltaForce0);
		forceM0.m_angular.m_y = forceM0.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_y, deltaForce0);
		forceM0.m_angular.m_z = forceM0.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_z, deltaForce0);

		forceM1.m_linear.m_x = forceM1.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_x, deltaForce1);
		forceM1.m_linear.m_y = forceM1.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_y, deltaForce1);
		forceM1.m_linear.m_z = forceM1.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_z, deltaForce1);
		forceM1.m_angular.m_x = forceM1.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_x, deltaForce1);
		forceM1.m_angular.m_y = forceM1.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_y, deltaForce1);
		forceM1.m_angular.m_z = forceM1.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_z, deltaForce1);
	}

	const dgFloat32 tol = dgFloat32(0.5f);
	const dgFloat32 tol2 = tol * tol;
	dgWorkGroupFloat maxAccel(accNorm);
	for (dgInt32 i = 0; (i < 4) && (maxAccel.AddHorizontal() > tol2); i++) {
		maxAccel = dgWorkGroupFloat::m_zero;
		for (dgInt32 j = 0; j < rowsCount; j++) {
			dgSolverSoaElement* const row = &massMatrix[j];

			dgWorkGroupFloat a;
			a = row->m_coordenateAccel.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_x, forceM0.m_linear.m_x);
			a = a.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_y, forceM0.m_linear.m_y);
			a = a.MulSub(row->m_JMinv.m_jacobianM0.m_linear.m_z, forceM0.m_linear.m_z);
			a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_x, forceM0.m_angular.m_x);
			a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_y, forceM0.m_angular.m_y);
			a = a.MulSub(row->m_JMinv.m_jacobianM0.m_angular.m_z, forceM0.m_angular.m_z);

			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_x, forceM1.m_linear.m_x);
			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_y, forceM1.m_linear.m_y);
			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_linear.m_z, forceM1.m_linear.m_z);
			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_x, forceM1.m_angular.m_x);
			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_y, forceM1.m_angular.m_y);
			a = a.MulSub(row->m_JMinv.m_jacobianM1.m_angular.m_z, forceM1.m_angular.m_z);
			a = a.MulSub(row->m_force, row->m_diagDamp);

			dgWorkGroupFloat f(row->m_force.MulAdd(row->m_invJinvMJt, a));

			dgWorkGroupFloat frictionNormal;
			for (dgInt32 k = 0; k < DG_WORK_GROUP_SIZE; k++) {
				dgAssert(row->m_normalForceIndex.GetInt(k) >= -1);
				dgAssert(row->m_normalForceIndex.GetInt(k) <= rowsCount);
				const dgInt32 frictionIndex = row->m_normalForceIndex.GetInt(k) + 1;
				frictionNormal[k] = normalForce[frictionIndex][k];
			}

			dgWorkGroupFloat lowerFrictionForce(frictionNormal * row->m_lowerBoundFrictionCoefficent);
			dgWorkGroupFloat upperFrictionForce(frictionNormal * row->m_upperBoundFrictionCoefficent);

			a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
			f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

			maxAccel = maxAccel.MulAdd(a, a);
			dgWorkGroupFloat deltaForce(f - row->m_force);

			row->m_force = f;
			normalForce[j + 1] = f;

			dgWorkGroupFloat deltaForce0(deltaForce * preconditioner0);
			dgWorkGroupFloat deltaForce1(deltaForce * preconditioner1);

			forceM0.m_linear.m_x = forceM0.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_x, deltaForce0);
			forceM0.m_linear.m_y = forceM0.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_y, deltaForce0);
			forceM0.m_linear.m_z = forceM0.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_z, deltaForce0);
			forceM0.m_angular.m_x = forceM0.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_x, deltaForce0);
			forceM0.m_angular.m_y = forceM0.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_y, deltaForce0);
			forceM0.m_angular.m_z = forceM0.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_z, deltaForce0);

			forceM1.m_linear.m_x = forceM1.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_x, deltaForce1);
			forceM1.m_linear.m_y = forceM1.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_y, deltaForce1);
			forceM1.m_linear.m_z = forceM1.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_z, deltaForce1);
			forceM1.m_angular.m_x = forceM1.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_x, deltaForce1);
			forceM1.m_angular.m_y = forceM1.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_y, deltaForce1);
			forceM1.m_angular.m_z = forceM1.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_z, deltaForce1);
		}
	}

	return accNorm.AddHorizontal();
}

void dgParallelBodySolver::CalculateJointsForce(dgInt32 threadID)
{
	const dgInt32* const soaRowStart = m_soaRowStart;
	const dgBodyInfo* const bodyArray = m_bodyArray;
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];
	dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	dgSolverSoaElement* const massMatrix = &m_massMatrix[0];
	dgFloat32 accNorm = dgFloat32(0.0f);

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_jointCount;
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		const dgInt32 rowStart = soaRowStart[i];
		dgJointInfo* const jointInfo = &m_jointArray[i * DG_WORK_GROUP_SIZE];

		bool isSleeping = true;
		dgFloat32 accel2 = dgFloat32 (0.0f);
		for (dgInt32 j = 0; (j < DG_WORK_GROUP_SIZE) && isSleeping; j++) {
			const dgInt32 m0 = jointInfo[j].m_m0;
			const dgInt32 m1 = jointInfo[j].m_m1;
			const dgBody* const body0 = bodyArray[m0].m_body;
			const dgBody* const body1 = bodyArray[m1].m_body;
			isSleeping &= body0->m_resting;
			isSleeping &= body1->m_resting;
		}
		if (!isSleeping) {
			accel2 = CalculateJointForce(jointInfo, &massMatrix[rowStart], internalForces);
			for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j++) {
				const dgJointInfo* const joint = &jointInfo[j];
				if (joint->m_joint) {
					dgInt32 const rowCount = joint->m_pairCount;
					dgInt32 const rowStartBase = joint->m_pairStart;
					for (dgInt32 k = 0; k < rowCount; k++) {
						const dgSolverSoaElement* const row = &massMatrix[rowStart + k];
						rightHandSide[k + rowStartBase].m_force = row->m_force[j];
						rightHandSide[k + rowStartBase].m_maxImpact = dgMax(dgAbs(row->m_force[j]), rightHandSide[k + rowStartBase].m_maxImpact);
					}
				}
			}
		}

		dgWorkGroupVector6 forceM0;
		dgWorkGroupVector6 forceM1;

		forceM0.m_linear.m_x = dgWorkGroupFloat::m_zero;
		forceM0.m_linear.m_y = dgWorkGroupFloat::m_zero;
		forceM0.m_linear.m_z = dgWorkGroupFloat::m_zero;
		forceM0.m_angular.m_x = dgWorkGroupFloat::m_zero;
		forceM0.m_angular.m_y = dgWorkGroupFloat::m_zero;
		forceM0.m_angular.m_z = dgWorkGroupFloat::m_zero;

		forceM1.m_linear.m_x = dgWorkGroupFloat::m_zero;
		forceM1.m_linear.m_y = dgWorkGroupFloat::m_zero;
		forceM1.m_linear.m_z = dgWorkGroupFloat::m_zero;
		forceM1.m_angular.m_x = dgWorkGroupFloat::m_zero;
		forceM1.m_angular.m_y = dgWorkGroupFloat::m_zero;
		forceM1.m_angular.m_z = dgWorkGroupFloat::m_zero;

		const dgInt32 rowsCount = jointInfo->m_pairCount;

		for (dgInt32 j = 0; j < rowsCount; j++) {
			dgSolverSoaElement* const row = &massMatrix[rowStart + j];

			dgWorkGroupFloat f(row->m_force);
			forceM0.m_linear.m_x = forceM0.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_x, f);
			forceM0.m_linear.m_y = forceM0.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_y, f);
			forceM0.m_linear.m_z = forceM0.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_linear.m_z, f);
			forceM0.m_angular.m_x = forceM0.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_x, f);
			forceM0.m_angular.m_y = forceM0.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_y, f);
			forceM0.m_angular.m_z = forceM0.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM0.m_angular.m_z, f);

			forceM1.m_linear.m_x = forceM1.m_linear.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_x, f);
			forceM1.m_linear.m_y = forceM1.m_linear.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_y, f);
			forceM1.m_linear.m_z = forceM1.m_linear.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_linear.m_z, f);
			forceM1.m_angular.m_x = forceM1.m_angular.m_x.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_x, f);
			forceM1.m_angular.m_y = forceM1.m_angular.m_y.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_y, f);
			forceM1.m_angular.m_z = forceM1.m_angular.m_z.MulAdd(row->m_Jt.m_jacobianM1.m_angular.m_z, f);
		}

		dgBodyProxy* const bodyProxyArray = m_bodyProxyArray;
		dgJacobian* const tempInternalForces = &m_world->m_solverMemory.m_internalForcesBuffer[m_cluster->m_bodyCount];
		for (dgInt32 j = 0; j < DG_WORK_GROUP_SIZE; j++) {
			const dgJointInfo* const joint = &jointInfo[j];
			if (joint->m_joint) {
				dgJacobian m_body0Force;
				dgJacobian m_body1Force;

				m_body0Force.m_linear = dgVector (forceM0.m_linear.m_x[j], forceM0.m_linear.m_y[j], forceM0.m_linear.m_z[j], dgFloat32 (0.0f));
				m_body0Force.m_angular = dgVector (forceM0.m_angular.m_x[j], forceM0.m_angular.m_y[j], forceM0.m_angular.m_z[j], dgFloat32 (0.0f));

				m_body1Force.m_linear = dgVector(forceM1.m_linear.m_x[j], forceM1.m_linear.m_y[j], forceM1.m_linear.m_z[j], dgFloat32(0.0f));
				m_body1Force.m_angular = dgVector(forceM1.m_angular.m_x[j], forceM1.m_angular.m_y[j], forceM1.m_angular.m_z[j], dgFloat32(0.0f));

				const dgInt32 m0 = jointInfo[j].m_m0;
				const dgInt32 m1 = jointInfo[j].m_m1;

				if (m0) {
					dgScopeSpinPause lock(&bodyProxyArray[m0].m_lock);
					tempInternalForces[m0].m_linear += m_body0Force.m_linear;
					tempInternalForces[m0].m_angular += m_body0Force.m_angular;
				}
				if (m1) {
					dgScopeSpinPause lock(&bodyProxyArray[m1].m_lock);
					tempInternalForces[m1].m_linear += m_body1Force.m_linear;
					tempInternalForces[m1].m_angular += m_body1Force.m_angular;
				}
			}
		}
		accNorm += accel2;
	}
	m_accelNorm[threadID] = accNorm;
}

#else

void dgParallelBodySolver::CalculateJointsForce(dgInt32 threadID)
{
	dgVector accNorm(dgVector::m_zero);
	const dgBodyInfo* const bodyArray = m_bodyArray;
	dgBodyProxy* const bodyProxyArray = m_bodyProxyArray;
	dgJacobian* const internalForces = &m_world->m_solverMemory.m_internalForcesBuffer[0];
	dgJacobian* const tempInternalForces = &m_world->m_solverMemory.m_internalForcesBuffer[m_cluster->m_bodyCount];

	dgRightHandSide* const rightHandSide = &m_world->m_solverMemory.m_righHandSizeBuffer[0];
	const dgLeftHandSide* const leftHandSide = &m_world->m_solverMemory.m_leftHandSizeBuffer[0];

	const dgInt32 step = m_threadCounts;
	const dgInt32 jointCount = m_cluster->m_jointCount;
	dgFloat32 normalForce[DG_CONSTRAINT_MAX_ROWS + 1];
	
	for (dgInt32 i = threadID; i < jointCount; i += step) {
		dgJointInfo* const jointInfo = &m_jointArray[i];
		dgFloat32 accel2 = dgFloat32(0.0f);

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		const dgBody* const body0 = bodyArray[m0].m_body;
		const dgBody* const body1 = bodyArray[m1].m_body;
		dgInt32 isSleeping = body0->m_resting & body1->m_resting;
		if (!isSleeping) {

			dgVector preconditioner0 (jointInfo->m_preconditioner0);
			dgVector preconditioner1 (jointInfo->m_preconditioner1);

			dgVector forceM0 (internalForces[m0].m_linear * preconditioner0);
			dgVector torqueM0 (internalForces[m0].m_angular * preconditioner0);
			dgVector forceM1 (internalForces[m1].m_linear * preconditioner1);
			dgVector torqueM1 (internalForces[m1].m_angular * preconditioner1);

			preconditioner0 = preconditioner0.Scale (bodyProxyArray[m0].m_weight);
			preconditioner1 = preconditioner1.Scale (bodyProxyArray[m1].m_weight);

			normalForce[0] = dgFloat32 (1.0f);
			const dgInt32 rowsCount = jointInfo->m_pairCount;
			const dgInt32 rowStart = jointInfo->m_pairStart;
			for (dgInt32 j = 0; j < rowsCount; j++) {
				dgRightHandSide* const rhs = &rightHandSide[rowStart + j];
				const dgLeftHandSide* const lhs = &leftHandSide[rowStart + j];
				dgVector a(lhs->m_JMinv.m_jacobianM0.m_linear * forceM0);
				a = a.MulAdd(lhs->m_JMinv.m_jacobianM0.m_angular, torqueM0);
				a = a.MulAdd(lhs->m_JMinv.m_jacobianM1.m_linear, forceM1);
				a = a.MulAdd(lhs->m_JMinv.m_jacobianM1.m_angular, torqueM1);
				//a = dgVector(rhs->m_coordenateAccel + rhs->m_gyroAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();
				a = dgVector(rhs->m_coordenateAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();
				dgVector f(rhs->m_force + rhs->m_invJinvMJt * a.GetScalar());

				dgAssert(rhs->m_normalForceIndex >= -1);
				dgAssert(rhs->m_normalForceIndex <= rowsCount);

				const dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;
				const dgFloat32 frictionNormal = normalForce[frictionIndex];
				const dgVector lowerFrictionForce(frictionNormal * rhs->m_lowerBoundFrictionCoefficent);
				const dgVector upperFrictionForce(frictionNormal * rhs->m_upperBoundFrictionCoefficent);

				a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
				f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

				accNorm = accNorm.MulAdd(a, a);
				dgVector deltaForce(f - dgVector(rhs->m_force));

				rhs->m_force = f.GetScalar();
				normalForce[j + 1] = f.GetScalar();

				dgVector deltaForce0(deltaForce * preconditioner0);
				dgVector deltaForce1(deltaForce * preconditioner1);

				forceM0 = forceM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_linear, deltaForce0);
				torqueM0 = torqueM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_angular, deltaForce0);
				forceM1 = forceM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_linear, deltaForce1);
				torqueM1 = torqueM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_angular, deltaForce1);
			}
	
			const dgFloat32 tol = dgFloat32(0.5f);
			const dgFloat32 tol2 = tol * tol;
			dgVector maxAccel(accNorm);

			for (dgInt32 k = 0; (k < 4) && (maxAccel.GetScalar() > tol2); k++) {
				maxAccel = dgVector::m_zero;
				for (dgInt32 j = 0; j < rowsCount; j++) {
					dgRightHandSide* const rhs = &rightHandSide[rowStart + j];
					const dgLeftHandSide* const lhs = &leftHandSide[rowStart + j];

					dgVector a(lhs->m_JMinv.m_jacobianM0.m_linear * forceM0);
					a = a.MulAdd(lhs->m_JMinv.m_jacobianM0.m_angular, torqueM0);
					a = a.MulAdd(lhs->m_JMinv.m_jacobianM1.m_linear, forceM1);
					a = a.MulAdd(lhs->m_JMinv.m_jacobianM1.m_angular, torqueM1);
					//a = dgVector(rhs->m_coordenateAccel + rhs->m_gyroAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();
					a = dgVector(rhs->m_coordenateAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();
					dgVector f(rhs->m_force + rhs->m_invJinvMJt * a.GetScalar());

					dgAssert(rhs->m_normalForceIndex >= -1);
					dgAssert(rhs->m_normalForceIndex <= rowsCount);

					const dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;
					const dgFloat32 frictionNormal = normalForce[frictionIndex];
					const dgVector lowerFrictionForce(frictionNormal * rhs->m_lowerBoundFrictionCoefficent);
					const dgVector upperFrictionForce(frictionNormal * rhs->m_upperBoundFrictionCoefficent);

					a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
					f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

					maxAccel = maxAccel.MulAdd(a, a);
					dgVector deltaForce(f - rhs->m_force);

					rhs->m_force = f.GetScalar();
					normalForce[j + 1] = f.GetScalar();

					dgVector deltaForce0(deltaForce * preconditioner0);
					dgVector deltaForce1(deltaForce * preconditioner1);
					forceM0 = forceM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_linear, deltaForce0);
					torqueM0 = torqueM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_angular, deltaForce0);
					forceM1 = forceM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_linear, deltaForce1);
					torqueM1 = torqueM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_angular, deltaForce1);
				}
			}

		}

		dgVector forceM0(dgVector::m_zero);
		dgVector torqueM0(dgVector::m_zero);
		dgVector forceM1(dgVector::m_zero);
		dgVector torqueM1(dgVector::m_zero);

		const dgInt32 rowsCount = jointInfo->m_pairCount;
		const dgInt32 rowStart = jointInfo->m_pairStart;

		for (dgInt32 j = 0; j < rowsCount; j++) {
			const dgRightHandSide* const rhs = &rightHandSide[rowStart + j];
			const dgLeftHandSide* const lhs = &leftHandSide[rowStart + j];

			dgVector f (rhs->m_force);
			forceM0 = forceM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_linear, f);
			torqueM0 = torqueM0.MulAdd(lhs->m_Jt.m_jacobianM0.m_angular, f);
			forceM1 = forceM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_linear, f);
			torqueM1 = torqueM1.MulAdd(lhs->m_Jt.m_jacobianM1.m_angular, f);
		}

		if (m0) {
			dgScopeSpinPause lock(&bodyProxyArray[m0].m_lock);
			tempInternalForces[m0].m_linear += forceM0;
			tempInternalForces[m0].m_angular += torqueM0;
		}
		if (m1) {
			dgScopeSpinPause lock(&bodyProxyArray[m1].m_lock);
			tempInternalForces[m1].m_linear += forceM1;
			tempInternalForces[m1].m_angular += torqueM1;
		}

		accNorm += accel2;
	}
	m_accelNorm[threadID] = accNorm.GetScalar();
}
#endif


void dgParallelBodySolver::CalculateForces()
{
	const dgInt32 passes = m_solverPasses;
	m_firstPassCoef = dgFloat32(0.0f);
	const dgInt32 threadCounts = m_world->GetThreadCount();

	InitSkeletons();
	for (dgInt32 step = 0; step < 4; step++) {
		CalculateJointsAcceleration();
		dgFloat32 accNorm = DG_SOLVER_MAX_ERROR * dgFloat32(2.0f);
		for (dgInt32 k = 0; (k < passes) && (accNorm > DG_SOLVER_MAX_ERROR); k++) {
			CalculateJointsForce();
			accNorm = dgFloat32(0.0f);
			for (dgInt32 i = 0; i < threadCounts; i++) {
				accNorm = dgMax(accNorm, m_accelNorm[i]);
			}
		}
		UpdateSkeletons();
		IntegrateBodiesVelocity();
	}

	UpdateForceFeedback();

	dgInt32 hasJointFeeback = 0;
	for (dgInt32 i = 0; i < DG_MAX_THREADS_HIVE_COUNT; i++) {
		hasJointFeeback |= m_hasJointFeeback[i];
	}
	CalculateBodiesAcceleration();

	if (hasJointFeeback) {
		UpdateKinematicFeedback();
	}
}

void dgParallelBodySolver::CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep)
{
	m_cluster = &cluster;
	m_bodyArray = bodyArray;
	m_jointArray = jointArray;
	m_timestep = timestep;
	m_invTimestep = (timestep > dgFloat32(0.0f)) ? dgFloat32(1.0f) / timestep : dgFloat32(0.0f);

	m_invStepRK = dgFloat32(0.25f);
	m_timestepRK = m_timestep * m_invStepRK;
	m_invTimestepRK = m_invTimestep * dgFloat32(4.0f);

	m_solverPasses = m_world->GetSolverIterations();
	m_threadCounts = m_world->GetThreadCount();
	m_jointCount = ((m_cluster->m_jointCount + DG_WORK_GROUP_SIZE - 1) & -dgInt32(DG_WORK_GROUP_SIZE - 1)) / DG_WORK_GROUP_SIZE;

	m_soaRowStart = dgAlloca(dgInt32, m_jointCount);
	m_bodyProxyArray = dgAlloca(dgBodyProxy, cluster.m_bodyCount);

	InitWeights();
	InitBodyArray();
	InitJacobianMatrix();
	CalculateForces();
}

