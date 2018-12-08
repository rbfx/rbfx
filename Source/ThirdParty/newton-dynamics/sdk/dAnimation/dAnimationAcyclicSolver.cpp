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


#include "dAnimationStdAfx.h"
#include "dAnimationAcyclicJoint.h"
#include "dAnimationAcyclicSolver.h"
#include "dAnimationKinematicLoopJoint.h"

#define D_DIAG_DAMP		 dFloat(1.0e-6f)
#define D_DIAG_REGULARIZER	 dFloat(1.0e-4f)
#define D_MAX_FRICTION_BOUND (D_COMPLEMENTARITY_MAX_FRICTION_BOUND * dFloat(0.5f))

class dAnimationAcyclicSolver::dMatrixData
{
	public:
	dSpatialMatrix m_jt;
	dSpatialMatrix m_mass;
	dSpatialMatrix m_invMass;
};

class dAnimationAcyclicSolver::dNodePair
{
	public:
	int m_m0;
	int m_m1;
};

class dAnimationAcyclicSolver::dVectorPair
{
	public:
	dSpatialVector m_joint;
	dSpatialVector m_body;
};

class dAnimationAcyclicSolver::dBodyJointMatrixDataPair
{
	public:
	dMatrixData m_body;
	dMatrixData m_joint;
};

dAnimationAcyclicSolver::dAnimationAcyclicSolver()
	:dContainersAlloc()
{
	m_data = NULL;
	m_pairs = NULL;
	m_rootNode = NULL;
	m_nodesOrder = NULL;
	m_loopJoints = NULL;
	m_deltaForce = NULL;
	m_leftHandSide = NULL;
	m_massMatrix10 = NULL;
	m_massMatrix11 = NULL;
	m_rightHandSide = NULL;
	m_matrixRowsIndex = NULL;

	m_rowCount = 0;
	m_nodeCount = 0;
	m_maxNodeCount = 0;
	m_loopRowCount = 0;
	m_loopNodeCount = 0;
	m_loopJointCount = 0;
	m_auxiliaryRowCount = 0;
}

dAnimationAcyclicSolver::~dAnimationAcyclicSolver()
{
	if (m_nodesOrder) {
		delete[] m_nodesOrder; 
	}
}

int dAnimationAcyclicSolver::CalculateNodeCount () const
{
	int count = 0;
	int stack = 1;
	dAnimationAcyclicJoint* pool[256];

	pool[0] = m_rootNode;
	while (stack) {
		stack --;
		count ++;
		dAnimationAcyclicJoint* const root = pool[stack];
		for (dList<dAnimationAcyclicJoint*>::dListNode* node = root->m_children.GetFirst(); node; node = node->GetNext()) {
			pool[stack] = node->GetInfo();
			stack ++;
			dAssert (stack < sizeof (pool) / sizeof (pool[0]));
		}
	}
	return count;
}

void dAnimationAcyclicSolver::SortGraph(dAnimationAcyclicJoint* const root, int& index)
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = root->m_children.GetFirst(); child; child = child->GetNext()) {
		SortGraph(child->GetInfo(), index);
	}

	dAssert((m_nodeCount - index - 1) >= 0);
	m_nodesOrder[index] = root;
	root->SetIndex (index);
	index++;
	dAssert(index <= m_nodeCount);
}

void dAnimationAcyclicSolver::Finalize(dAnimationAcyclicJoint* const rootNode)
{
	m_rootNode = rootNode;
	if (!rootNode->m_children.GetCount()) {
		return ;
	}

	if (m_nodesOrder) {
		delete m_nodesOrder;
	}

	m_nodeCount = CalculateNodeCount ();
	m_maxNodeCount = m_nodeCount * 2 + 8;
	m_nodesOrder = new dAnimationAcyclicJoint*[m_maxNodeCount * sizeof (dAnimationAcyclicJoint*)];

	int index = 0;
	SortGraph(rootNode, index);
	dAssert(index == m_nodeCount);
}

void dAnimationAcyclicSolver::CalculateInertiaMatrix(dAnimationAcyclicJoint* const node) const
{
	dComplementaritySolver::dBodyState* const body = node->GetProxyBody();
	dSpatialMatrix& bodyMass = m_data[node->GetIndex()].m_body.m_mass;
	bodyMass = dSpatialMatrix(dFloat32(0.0f));

	if (body->GetInvMass() != dFloat32(0.0f)) {
		const dFloat mass = body->GetMass();
		const dMatrix& inertia = body->GetInertia();

		for (int i = 0; i < 3; i++) {
			bodyMass[i][i] = mass;
			for (int j = 0; j < 3; j++) {
				bodyMass[i + 3][j + 3] = inertia[i][j];
			}
		}
	}
}

void dAnimationAcyclicSolver::GetJacobians(dAnimationAcyclicJoint* const node)
{
	dAssert(node->m_parent);
	dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();

	const int index = node->GetIndex();
	dSpatialMatrix& bodyJt = m_data[index].m_body.m_jt;
	dSpatialMatrix& jointJ = m_data[index].m_joint.m_jt;
	dSpatialMatrix& jointMass = m_data[index].m_joint.m_mass;
	const int start = joint->m_start;
	const dSpatialVector zero(0.0f);
	const dVector negOne (-1.0f);
	for (int i = 0; i < joint->m_dof; i++) {
		const int k = joint->m_sourceJacobianIndex[i];
		const dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[start + k];
		const dComplementaritySolver::dJacobianPair* const row = &m_leftHandSide[start + k];

		jointMass[i] = zero;
		jointMass[i][i] = -rhs->m_diagDamp;
		bodyJt[i] = dSpatialVector(row->m_jacobian_J01.m_linear * negOne, row->m_jacobian_J01.m_angular * negOne);
		jointJ[i] = dSpatialVector(row->m_jacobian_J10.m_linear * negOne, row->m_jacobian_J10.m_angular * negOne);
	}
}

void dAnimationAcyclicSolver::CalculateBodyDiagonal(dAnimationAcyclicJoint* const child)
{
	dComplementaritySolver::dBilateralJoint* const joint = child->GetProxyJoint();
	dAssert(joint);

	dSpatialMatrix copy(dFloat(0.0f));
	const int dof = joint->m_dof;
	const int index = child->GetIndex();
	const dSpatialMatrix& jacobianMatrix = m_data[index].m_joint.m_jt;
	const dSpatialMatrix& childDiagonal = m_data[index].m_joint.m_mass;
	for (int i = 0; i < dof; i++) {
		const dSpatialVector& jacobian = jacobianMatrix[i];
		for (int j = 0; j < dof; j++) {
			dFloat val = childDiagonal[i][j];
			copy[j] = copy[j] + jacobian.Scale(val);
		}
	}
	
	const int parentIndes = child->GetParent()->GetIndex();
	dSpatialMatrix& bodyMass = m_data[parentIndes].m_body.m_mass;
	for (int i = 0; i < dof; i++) {
		const dSpatialVector& Jacobian = copy[i];
		const dSpatialVector& JacobianTranspose = jacobianMatrix[i];
		for (int j = 0; j < 6; j++) {
			dFloat val = -Jacobian[j];
			bodyMass[j] = bodyMass[j] + JacobianTranspose.Scale(val);
		}
	}
}

void dAnimationAcyclicSolver::CalculateJointDiagonal(dAnimationAcyclicJoint* const node)
{
	dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();

	const int index = node->GetIndex();
	const dSpatialMatrix& bodyMass = m_data[index].m_body.m_mass;
	const dSpatialMatrix& bodyJt = m_data[index].m_body.m_jt;

	dSpatialMatrix tmp;
	for (int i = 0; i < joint->m_dof; i++) {
		tmp[i] = bodyMass.VectorTimeMatrix(bodyJt[i]);
	}

	dSpatialMatrix& jointMass = m_data[index].m_joint.m_mass;
	for (int i = 0; i < joint->m_dof; i++) {
		dFloat a = bodyJt[i].DotProduct(tmp[i]);
		jointMass[i][i] -= a;
		for (int j = i + 1; j < joint->m_dof; j++) {
			a = -bodyJt[i].DotProduct(tmp[j]);
			jointMass[i][j] = a;
			jointMass[j][i] = a;
		}
	}

	dSpatialMatrix& jointInvMass = m_data[index].m_joint.m_invMass;
	jointInvMass = jointMass.Inverse(joint->m_dof);
}

void dAnimationAcyclicSolver::CalculateJacobianBlock(dAnimationAcyclicJoint* const node)
{
	dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const int index = node->GetIndex();
	dSpatialMatrix& jointJ = m_data[index].m_joint.m_jt;

	dSpatialMatrix copy;
	const dSpatialVector zero(0.0f);
	for (int i = 0; i < joint->m_dof; i++) {
		copy[i] = jointJ[i];
		jointJ[i] = zero;
	}

	const dSpatialMatrix& jointInvMass = m_data[index].m_joint.m_invMass;
	for (int i = 0; i < joint->m_dof; i++) {
		const dSpatialVector& jacobian = copy[i];
		const dSpatialVector& invDiagonalRow = jointInvMass[i];
		for (int j = 0; j < joint->m_dof; j++) {
			dFloat val = invDiagonalRow[j];
			jointJ[j] = jointJ[j] + jacobian.Scale(val);
		}
	}
}

void dAnimationAcyclicSolver::Factorize(dAnimationAcyclicJoint* const node)
{
	CalculateInertiaMatrix(node);
	dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	if (joint) {
		joint->m_ordinals = 0x050403020100ll;
		dAssert(node->m_parent);

		joint->m_dof = 0;
		int count = joint->m_count;
		int first = joint->m_start;
		for (int i = 0; i < count; i++) {
			int k = joint->m_sourceJacobianIndex[i];
			const dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[k + first];

			if ((rhs->m_jointLowFriction <= dFloat(-D_MAX_FRICTION_BOUND)) && (rhs->m_jointHighFriction >= dFloat(D_MAX_FRICTION_BOUND))) {
				joint->m_dof++;
			} else {
				dSwap(joint->m_sourceJacobianIndex[i], joint->m_sourceJacobianIndex[count - 1]);
				i--;
				count--;
			}
		}

		dAssert(joint->m_dof > 0);
		dAssert(joint->m_dof <= 6);
		GetJacobians(node);
	}

	const int nodeIndex = node->GetIndex();
	dComplementaritySolver::dBodyState* const body = node->GetProxyBody();

	const dSpatialMatrix& bodyMass = m_data[nodeIndex].m_body.m_mass;
	dSpatialMatrix& bodyInvMass = m_data[nodeIndex].m_body.m_invMass;
	if (body->GetInvMass() != dFloat32(0.0f)) {
		const dList<dAnimationAcyclicJoint*>& children = node->GetChildren();
		for (dList<dAnimationAcyclicJoint*>::dListNode* childNode = children.GetFirst(); childNode; childNode = childNode->GetNext()) {
			CalculateBodyDiagonal(childNode->GetInfo());
		}
		bodyInvMass = bodyMass.Inverse(6);
	} else {
		bodyInvMass = dSpatialMatrix(0.0f);
	}

	if (joint) {
		dSpatialMatrix& bodyJt = m_data[nodeIndex].m_body.m_jt;
		dAssert(node->m_parent);
		for (int i = 0; i < joint->m_dof; i++) {
			bodyJt[i] = bodyInvMass.VectorTimeMatrix(bodyJt[i]);
		}
		CalculateJointDiagonal(node);
		CalculateJacobianBlock(node);
	}
}

void dAnimationAcyclicSolver::CalculateLoopMassMatrixCoefficients()
{
	const int auxiliaryRowCount = m_auxiliaryRowCount + m_loopRowCount;
	const int primaryCount = m_rowCount - m_auxiliaryRowCount;

	const dVector zero (0.0f);
	for (int i = 0; i < auxiliaryRowCount; i++) {
		const int ii = m_matrixRowsIndex[primaryCount + i];

		const dComplementaritySolver::dJacobianPair* const row_i = &m_leftHandSide[ii];
		const dComplementaritySolver::dJacobianColum* const rhs_i = &m_rightHandSide[ii];
		dFloat* const matrixRow11 = &m_massMatrix11[auxiliaryRowCount * i];

		const int m0_i = m_pairs[primaryCount + i].m_m0;
		const int m1_i = m_pairs[primaryCount + i].m_m1;

		dAnimationAcyclicJoint* const node0 = m_nodesOrder[m0_i];
		dAnimationAcyclicJoint* const node1 = m_nodesOrder[m1_i];
		dComplementaritySolver::dBodyState* const state0 = node0->GetProxyBody();
		dComplementaritySolver::dBodyState* const state1 = node1->GetProxyBody();

		const dMatrix& invInertia0 = state0->GetInvInertia();
		const dMatrix& invInertia1 = state1->GetInvInertia();

		const dFloat invMass0 = state0->GetInvMass();
		const dFloat invMass1 = state1->GetInvMass();

		dComplementaritySolver::dJacobian J01invM0(row_i->m_jacobian_J01.m_linear.Scale(invMass0), invInertia0.RotateVector(row_i->m_jacobian_J01.m_angular));
		dComplementaritySolver::dJacobian J10invM1(row_i->m_jacobian_J10.m_linear.Scale(invMass1), invInertia1.RotateVector(row_i->m_jacobian_J10.m_angular));

		dVector element(J01invM0.m_linear * row_i->m_jacobian_J01.m_linear + J01invM0.m_angular * row_i->m_jacobian_J01.m_angular + 
						J10invM1.m_linear * row_i->m_jacobian_J10.m_linear + J10invM1.m_angular * row_i->m_jacobian_J10.m_angular);
		dFloat diagonal = element.m_x + element.m_y + element.m_z + rhs_i->m_diagDamp;

		matrixRow11[i] = diagonal + rhs_i->m_diagDamp;
		//diagDamp[i] = matrixRow11[i] * (DG_PSD_DAMP_TOL * dFloat(4.0f));
		for (int j = i + 1; j < auxiliaryRowCount; j++) {
			const int jj = m_matrixRowsIndex[primaryCount + j];
			const dComplementaritySolver::dJacobianPair* const row_j = &m_leftHandSide[jj];

			const int k = primaryCount + j;
			const int m0_j = m_pairs[k].m_m0;
			const int m1_j = m_pairs[k].m_m1;

			bool hasEffect = false;

			dVector acc(zero);
			if (m0_i == m0_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J01.m_linear + J01invM0.m_angular * row_j->m_jacobian_J01.m_angular;
			} else if (m0_i == m1_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J10.m_linear + J01invM0.m_angular * row_j->m_jacobian_J10.m_angular;
			}

			if (m1_i == m0_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J01.m_linear + J10invM1.m_angular * row_j->m_jacobian_J01.m_angular;
			} else if (m1_i == m1_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J10.m_linear + J10invM1.m_angular * row_j->m_jacobian_J10.m_angular;
			}

			if (hasEffect) {
				dFloat offDiagValue = acc.m_x + acc.m_y + acc.m_z;
				matrixRow11[j] = offDiagValue;
				m_massMatrix11[j * auxiliaryRowCount + i] = offDiagValue;
			}
		}

		dFloat* const matrixRow10 = &m_massMatrix10[primaryCount * i];
		for (int j = 0; j < primaryCount; j++) {
			const int jj = m_matrixRowsIndex[j];
			const dComplementaritySolver::dJacobianPair* const row_j = &m_leftHandSide[jj];

			const int m0_j = m_pairs[j].m_m0;
			const int m1_j = m_pairs[j].m_m1;

			dVector acc(zero);
			bool hasEffect = false;

			if (m0_i == m0_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J01.m_linear + J01invM0.m_angular * row_j->m_jacobian_J01.m_angular;
			} else if (m0_i == m1_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J10.m_linear + J01invM0.m_angular * row_j->m_jacobian_J10.m_angular;
			}

			if (m1_i == m0_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J01.m_linear + J10invM1.m_angular * row_j->m_jacobian_J01.m_angular;
			} else if (m1_i == m1_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J10.m_linear + J10invM1.m_angular * row_j->m_jacobian_J10.m_angular;				
			}

			if (hasEffect) {
				dFloat offDiagValue = acc.m_x + acc.m_y + acc.m_z;
				matrixRow10[j] = offDiagValue;
			}
		}
	}
}

void dAnimationAcyclicSolver::InitLoopMassMatrix()
{
	int primaryIndex = 0;
	int auxiliaryIndex = 0;
	const int primaryCount = m_rowCount - m_auxiliaryRowCount;
	const int nodeCount = m_nodeCount - 1;
	for (int i = 0; i < nodeCount; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		dAssert(joint);
		dAssert(i == node->GetIndex());

		const int m0 = node->GetIndex();
		const int m1 = node->m_parent->GetIndex();
		const int first = joint->m_start;
		const int primaryDof = joint->m_dof;
		for (int j = 0; j < primaryDof; j++) {
			const int index = joint->m_sourceJacobianIndex[j];
			m_pairs[primaryIndex].m_m0 = m0;
			m_pairs[primaryIndex].m_m1 = m1;
			m_matrixRowsIndex[primaryIndex] = first + index;
			primaryIndex++;
		}

		const int auxiliaryDof = joint->m_count - primaryDof;
		for (int j = 0; j < auxiliaryDof; j++) {
			const int index = joint->m_sourceJacobianIndex[primaryDof + j];
			m_pairs[auxiliaryIndex + primaryCount].m_m0 = m0;
			m_pairs[auxiliaryIndex + primaryCount].m_m1 = m1;
			m_matrixRowsIndex[auxiliaryIndex + primaryCount] = first + index;
			auxiliaryIndex++;
		}
	}

	const int loopJointCount = m_loopJointCount;
	for (int j = 0; j < loopJointCount; j++) {
		const dAnimationKinematicLoopJoint* const joint = m_loopJoints[j];
		const dAnimationAcyclicJoint* const node0 = joint->GetOwner0();
		const dAnimationAcyclicJoint* const node1 = joint->GetOwner1();
		const int m0 = node0->GetIndex();
		const int m1 = node1->GetIndex();
		const int first = joint->m_start;
		const int auxiliaryDof = joint->m_dof;
		for (int i = 0; i < auxiliaryDof; i++) {
			m_pairs[primaryCount + auxiliaryIndex].m_m0 = m0;
			m_pairs[primaryCount + auxiliaryIndex].m_m1 = m1;
			m_matrixRowsIndex[primaryCount + auxiliaryIndex] = first + i;
			auxiliaryIndex++;
		}
	}

	dAssert(primaryIndex == primaryCount);
	dAssert(auxiliaryIndex == m_auxiliaryRowCount + m_loopRowCount);

	const int row = m_rowCount - m_auxiliaryRowCount;
	const int col = m_auxiliaryRowCount + m_loopRowCount;

	memset(m_massMatrix10, 0, row * col * sizeof(dFloat));
	memset(m_massMatrix11, 0, col * col * sizeof(dFloat));

	CalculateLoopMassMatrixCoefficients();

	const dSpatialVector zero(0.0f);
	dVectorPair* const accelPair = dAlloca(dVectorPair, m_nodeCount);
	dVectorPair* const forcePair = dAlloca(dVectorPair, m_nodeCount);

	accelPair[m_nodeCount - 1].m_body = zero;
	accelPair[m_nodeCount - 1].m_joint = zero;
	for (int i = 0; i < auxiliaryIndex; i++) {
		int entry = 0;
		int startjoint = m_nodeCount;
		const dFloat* const matrixRow10 = &m_massMatrix10[i * primaryCount];

		for (int j = 0; j < nodeCount; j++) {
			dAnimationAcyclicJoint* const node = m_nodesOrder[j];
			const int index = node->GetIndex();
			const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();

			accelPair[index].m_body = zero;
			dSpatialVector& a = accelPair[index].m_joint;

			const int count = joint->m_dof;
			for (int k = 0; k < count; k++) {
				const dFloat value = matrixRow10[entry];
				a[k] = value;
				startjoint = (value == 0.0f) ? startjoint : dMin(startjoint, index);
				entry++;
			}
		}

		entry = 0;
		startjoint = (startjoint == m_nodeCount) ? 0 : startjoint;
		dAssert(startjoint < m_nodeCount);
		SolveForward(forcePair, accelPair, startjoint);
		SolveBackward(forcePair, forcePair);

		dFloat* const deltaForcePtr = &m_deltaForce[i * primaryCount];
		for (int j = 0; j < nodeCount; j++) {
			dAnimationAcyclicJoint* const node = m_nodesOrder[j];
			const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
			const int index = node->GetIndex();
			const dSpatialVector& f = forcePair[index].m_joint;
			const int count = joint->m_dof;
			for (int k = 0; k < count; k++) {
				deltaForcePtr[entry] = f[k];
				entry++;
			}
		}
	}

	short* const indexList = dAlloca(short, primaryCount);
	for (int i = 0; i < auxiliaryIndex; i++) {
		const dFloat* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		const dFloat* const deltaForcePtr = &m_deltaForce[i * primaryCount];
		dFloat* const matrixRow11 = &m_massMatrix11[i * auxiliaryIndex];

		int indexCount = 0;
		for (int k = 0; k < primaryCount; k++) {
			indexList[indexCount] = short(k);
			indexCount += (matrixRow10[k] != dFloat(0.0f)) ? 1 : 0;
		}

		dFloat diagonal = matrixRow11[i];
		for (int k = 0; k < indexCount; k++) {
			int index = indexList[k];
			diagonal += matrixRow10[index] * deltaForcePtr[index];
		}
		dAssert (diagonal > 0.0f);
		diagonal += diagonal * D_DIAG_REGULARIZER;
		matrixRow11[i] = dMax(diagonal, D_DIAG_DAMP);

		for (int j = i + 1; j < auxiliaryIndex; j++) {
			dFloat offDiagonal = matrixRow11[j];
			const dFloat* const row10 = &m_deltaForce[j * primaryCount];
			for (int k = 0; k < indexCount; k++) {
				int index = indexList[k];
				offDiagonal += matrixRow10[index] * row10[index];
			}
			matrixRow11[j] = offDiagonal;
			m_massMatrix11[j * auxiliaryIndex + i] = offDiagonal;
		}
	}

#ifdef _DEBUG
	dFloat* const tmp = dAlloca (dFloat, auxiliaryIndex * auxiliaryIndex);
	memcpy (tmp, m_massMatrix11, sizeof (dFloat) * auxiliaryIndex * auxiliaryIndex);
	dAssert (dCholeskyFactorization(auxiliaryIndex, tmp));
#endif
}

void dAnimationAcyclicSolver::InitMassMatrix()
{
	int rowCount = 0;
	int auxiliaryRowCount = 0;

	dAssert (m_nodesOrder);
	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		Factorize(node);
		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		rowCount += joint->m_count;
		auxiliaryRowCount += joint->m_count - joint->m_dof;
	}
	Factorize(m_nodesOrder[m_nodeCount - 1]);

	m_rowCount = rowCount;
	m_auxiliaryRowCount = auxiliaryRowCount;

	int loopRowCount = 0;
	for (int j = 0; j < m_loopJointCount; j++) {
		const dAnimationKinematicLoopJoint* const joint = m_loopJoints[j];
		loopRowCount += joint->m_count;
	}

	m_loopRowCount = loopRowCount;
}

int dAnimationAcyclicSolver::BuildJacobianMatrix(dFloat timestep, dComplementaritySolver::dBilateralJoint* const joint)
{
	dComplementaritySolver::dParamInfo constraintParams;
	constraintParams.m_timestep = timestep;
	constraintParams.m_timestepInv = 1.0f / timestep;

	constraintParams.m_count = 0;
	memset (&constraintParams.m_normalIndex, 0, sizeof (constraintParams.m_normalIndex));
	joint->JacobianDerivative(&constraintParams);

	const int dofCount = constraintParams.m_count;

	// complete the derivative matrix for this joint
	const int index = joint->m_start;
	dComplementaritySolver::dBodyState* const state0 = joint->m_state0;
	dComplementaritySolver::dBodyState* const state1 = joint->m_state1;
	const dMatrix& invInertia0 = state0->GetInvInertia();
	const dMatrix& invInertia1 = state1->GetInvInertia();
	dFloat invMass0 = state0->GetInvMass();
	dFloat invMass1 = state1->GetInvMass();
	const dFloat weight = 0.9f;
	for (int i = 0; i < dofCount; i++) {
		dComplementaritySolver::dJacobianPair* const row = &m_leftHandSide[index + i];
		dComplementaritySolver::dJacobianColum* const col = &m_rightHandSide[index + i];

		*row = constraintParams.m_jacobians[i];

		dVector J01MinvJ01linear(row->m_jacobian_J01.m_linear.Scale(invMass0) * row->m_jacobian_J01.m_linear);
		dVector J10MinvJ10linear(row->m_jacobian_J10.m_linear.Scale(invMass1) * row->m_jacobian_J10.m_linear);
		dVector J01MinvJ01angular(invInertia0.RotateVector(row->m_jacobian_J01.m_angular) * row->m_jacobian_J01.m_angular);
		dVector J10MinvJ10angular(invInertia1.RotateVector(row->m_jacobian_J10.m_angular) * row->m_jacobian_J10.m_angular);
		dVector tmpDiag(J01MinvJ01linear + J10MinvJ10linear + J01MinvJ01angular + J10MinvJ10angular);

		dFloat diag = tmpDiag.m_x + tmpDiag.m_y + tmpDiag.m_z;
		col->m_diagDamp = diag * D_DIAG_DAMP;
		col->m_coordenateAccel = constraintParams.m_jointAccel[i];
		col->m_normalIndex = constraintParams.m_normalIndex[i];
		col->m_jointLowFriction = constraintParams.m_jointLowFrictionCoef[i];
		col->m_jointHighFriction = constraintParams.m_jointHighFrictionCoef[i];
		dFloat normal = col->m_normalIndex ? col[col->m_normalIndex].m_force : 1.0f;
		dFloat lowFriction = normal * col->m_jointLowFriction;
		dFloat highFriction = normal * col->m_jointHighFriction;
		col->m_force = dClamp(joint->m_jointFeebackForce[i] * weight, lowFriction, highFriction);
	}

	return dofCount;
}

int dAnimationAcyclicSolver::BuildJacobianMatrix(dFloat timestep)
{
	int rowCount = 0;
	for (int j = 0; j < m_nodeCount - 1; j++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[j];
		dAssert(node && node->GetProxyJoint());
		dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		joint->m_start = rowCount;
		joint->m_count = BuildJacobianMatrix(timestep, joint);
		rowCount += joint->m_count;
	}

	for (int i = 0; i < m_loopJointCount; i++) {
		dAnimationKinematicLoopJoint* const joint = m_loopJoints[i];
		joint->m_start = rowCount;
		joint->m_count = BuildJacobianMatrix(timestep, joint);
		rowCount += joint->m_count;
	}

	return rowCount;
}

void dAnimationAcyclicSolver::CalculateJointAccel(dVectorPair* const accel) const
{
	const dSpatialVector zero(0.0f);
	const int n = m_nodeCount - 1;
	dAssert(n == m_nodesOrder[n]->GetIndex());
	for (int i = 0; i < n; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dAssert(i == node->GetIndex());

		dVectorPair& a = accel[i];
		dAssert(node->GetProxyBody());
		dAssert(node->GetProxyJoint());

		//a.m_body = zero;
		a.m_joint = zero;

		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		const int first = joint->m_start;
		const int dof = joint->m_count;
		for (int j = 0; j < dof; j++) {
			const int k = joint->m_sourceJacobianIndex[j];
			dComplementaritySolver::dJacobianColum& col = m_rightHandSide[first + k];
			//a.m_joint[j] = col.m_coordenateAccel;
			a.m_joint[j] = -col.m_coordenateAccel;
		}
		
		dComplementaritySolver::dBodyState* const body = node->GetProxyBody();
		const dVector& force = body->GetForce(); 
		const dVector& torque = body->GetTorque(); 
		for (int j = 0; j < 3; j++) {
			a.m_body[j + 0] = force[j];
			a.m_body[j + 3] = torque[j];
		}
	}
	
	dAnimationAcyclicJoint* const rooNode = m_nodesOrder[n];
	dComplementaritySolver::dBodyState* const body = rooNode->GetProxyBody();	
	const dVector& force = body->GetForce();
	const dVector& torque = body->GetTorque();
	dVectorPair& a = accel[n];
	a.m_joint = zero;
	for (int j = 0; j < 3; j++) {
		a.m_body[j + 0] = force[j];
		a.m_body[j + 3] = torque[j];
	}
}

void dAnimationAcyclicSolver::BodyJacobianTimeMassForward(dAnimationAcyclicJoint* const node, const dVectorPair& force, dVectorPair& parentForce) const
{
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const dSpatialMatrix& jointJ = m_data[node->GetIndex()].m_joint.m_jt;
	for (int i = 0; i < joint->m_dof; i++) {
		parentForce.m_body = parentForce.m_body + jointJ[i].Scale(-force.m_joint[i]);
	}
}

void dAnimationAcyclicSolver::JointJacobianTimeMassForward(dAnimationAcyclicJoint* const node, dVectorPair& force) const
{
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const dSpatialMatrix& bodyJt = m_data[node->GetIndex()].m_body.m_jt;
	for (int i = 0; i < joint->m_dof; i++) {
		force.m_joint[i] -= bodyJt[i].DotProduct(force.m_body);
	}
}

void dAnimationAcyclicSolver::BodyDiagInvTimeSolution(dAnimationAcyclicJoint* const node, dVectorPair& force) const
{
	const dSpatialMatrix& bodyInvMass = m_data[node->GetIndex()].m_body.m_invMass;
	force.m_body = bodyInvMass.VectorTimeMatrix(force.m_body);
}

void dAnimationAcyclicSolver::JointDiagInvTimeSolution(dAnimationAcyclicJoint* const node, dVectorPair& force) const
{
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const dSpatialMatrix& jointInvMass = m_data[node->GetIndex()].m_joint.m_invMass;
	force.m_joint = jointInvMass.VectorTimeMatrix(force.m_joint, joint->m_dof);
}

void dAnimationAcyclicSolver::JointJacobianTimeSolutionBackward(dAnimationAcyclicJoint* const node, dVectorPair& force, const dVectorPair& parentForce) const
{
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const dSpatialMatrix& jointJ = m_data[node->GetIndex()].m_joint.m_jt;
	const dSpatialVector& f = parentForce.m_body;
	for (int i = 0; i < joint->m_dof; i++) {
		force.m_joint[i] -= f.DotProduct(jointJ[i]);
	}
}

void dAnimationAcyclicSolver::BodyJacobianTimeSolutionBackward(dAnimationAcyclicJoint* const node, dVectorPair& force) const
{
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	const dSpatialMatrix& bodyJt = m_data[node->GetIndex()].m_body.m_jt;
	for (int i = 0; i < joint->m_dof; i++) {
		force.m_body = force.m_body + bodyJt[i].Scale(-force.m_joint[i]);
	}
}

void dAnimationAcyclicSolver::SolveForward(dVectorPair* const force, const dVectorPair* const accel, int startNode) const
{
	dSpatialVector zero(0.0f);
	for (int i = 0; i < startNode; i++) {
		force[i].m_body = zero;
		force[i].m_joint = zero;
	}

	const int nodeCount = m_nodeCount - 1;
	for (int i = startNode; i < nodeCount; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dAssert(node->GetProxyJoint());
		dAssert(i == node->GetIndex());

		dVectorPair& f = force[i];
		const dVectorPair& a = accel[i];
		f.m_body = a.m_body;
		f.m_joint = a.m_joint;

		const dList<dAnimationAcyclicJoint*>& children = node->GetChildren();
		for (dList<dAnimationAcyclicJoint*>::dListNode* childNode = children.GetFirst(); childNode; childNode = childNode->GetNext()) {
			dAnimationAcyclicJoint* const child = childNode->GetInfo();
			dAssert(child->m_parent->GetIndex() == i);
			BodyJacobianTimeMassForward(child, force[child->GetIndex()], f);
		}
		JointJacobianTimeMassForward(node, f);
	}

	const int n = m_nodeCount - 1;
	dAssert(n == m_nodesOrder[n]->GetIndex());
	dAnimationAcyclicJoint* const rootNode = m_nodesOrder[n];
	force[n] = accel[n];
	const dList<dAnimationAcyclicJoint*>& children = rootNode->GetChildren();
	for (dList<dAnimationAcyclicJoint*>::dListNode* childNode = children.GetFirst(); childNode; childNode = childNode->GetNext()) {
		dAnimationAcyclicJoint* const child = childNode->GetInfo();
		dAssert(child->m_parent->GetIndex() == n);
		BodyJacobianTimeMassForward(child, force[child->GetIndex()], force[n]);
	}

	for (int i = startNode; i < nodeCount; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dVectorPair& f = force[i];
		BodyDiagInvTimeSolution(node, f);
		JointDiagInvTimeSolution(node, f);
	}
	BodyDiagInvTimeSolution(rootNode, force[n]);
}

void dAnimationAcyclicSolver::SolveBackward(dVectorPair* const force, const dVectorPair* const accel) const
{
	for (int i = m_nodeCount - 2; i >= 0; i--) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dAssert(i == node->GetIndex());
		dVectorPair& f = force[i];
		JointJacobianTimeSolutionBackward(node, f, force[node->m_parent->GetIndex()]);
		BodyJacobianTimeSolutionBackward(node, f);
	}
}

void dAnimationAcyclicSolver::CalculateOpenLoopForce(dVectorPair* const force, const dVectorPair* const accel) const
{
	SolveForward(force, accel, 0);
	SolveBackward(force, accel);
}

void dAnimationAcyclicSolver::UpdateForces(const dVectorPair* const force) const
{
	dVector zero(0.0f);
	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dAssert(i == node->GetIndex());
		dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();

		dComplementaritySolver::dJacobian y0;
		dComplementaritySolver::dJacobian y1;

		const dSpatialVector& f = force[i].m_joint;
		const int first = joint->m_start;
		const int count = joint->m_dof;

		const dComplementaritySolver::dJacobianPair* const row = &m_leftHandSide[first];
		for (int j = 0; j < count; j++) {
			const int k = joint->m_sourceJacobianIndex[j];
			joint->m_jointFeebackForce[k] = f[j];
			dVector jointForce = f[j];
			y0.m_linear += row[k].m_jacobian_J01.m_linear * jointForce;
			y0.m_angular += row[k].m_jacobian_J01.m_angular * jointForce;
			y1.m_linear += row[k].m_jacobian_J10.m_linear * jointForce;
			y1.m_angular += row[k].m_jacobian_J10.m_angular * jointForce;
		}

		dAssert(node->GetProxyBody() == joint->m_state0);
		dAssert(node->m_parent->GetProxyBody() == joint->m_state1);
		dComplementaritySolver::dBodyState* const body0 = joint->m_state0;
		dComplementaritySolver::dBodyState* const body1 = joint->m_state1;

		body0->SetForce(body0->GetForce() + y0.m_linear);
		body0->SetTorque(body0->GetTorque() + y0.m_angular);
		body1->SetForce(body1->GetForce() + y1.m_linear);
		body1->SetTorque(body1->GetTorque() + y1.m_angular);
	}
}

void dAnimationAcyclicSolver::SolveAuxiliary(dVectorPair* const force, const dVectorPair* const accel) const
{
	const int n = m_loopRowCount + m_auxiliaryRowCount;
	dFloat* const f = dAlloca(dFloat, m_rowCount + n);
	dFloat* const u = dAlloca(dFloat, n);
	dFloat* const b = dAlloca(dFloat, n);
	dFloat* const low = dAlloca(dFloat, n);
	dFloat* const high = dAlloca(dFloat, n);
	int* const normalIndex = dAlloca(int, n);

	int primaryIndex = 0;
	int auxiliaryIndex = 0;
	const int primaryCount = m_rowCount - m_auxiliaryRowCount;
	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		const int first = joint->m_start;
		const int primaryDof = joint->m_dof;

		const dSpatialVector& accelSpatial = accel[i].m_joint;
		const dSpatialVector& forceSpatial = force[i].m_joint;

		for (int j = 0; j < primaryDof; j++) {
			f[primaryIndex] = dFloat(forceSpatial[j]);
			primaryIndex++;
		}

		const int auxiliaryDof = joint->m_count - primaryDof;
		for (int j = 0; j < auxiliaryDof; j++) {
			const int index = joint->m_sourceJacobianIndex[primaryDof + j];
			dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[first + index];

			f[auxiliaryIndex + primaryCount] = rhs->m_force;
			b[auxiliaryIndex] = -dFloat(accelSpatial[primaryDof + j]);

			dAssert(rhs->m_force >= rhs->m_jointLowFriction * dFloat(2.0f));
			dAssert(rhs->m_force <= rhs->m_jointHighFriction * dFloat(2.0f));
			u[auxiliaryIndex] = rhs->m_force;
			low[auxiliaryIndex] = rhs->m_jointLowFriction;
			high[auxiliaryIndex] = rhs->m_jointHighFriction;
			normalIndex[auxiliaryIndex] = rhs->m_normalIndex;
			dAssert (normalIndex[auxiliaryIndex] == 0);
			auxiliaryIndex++;
		}
	}

	for (int j = 0; j < m_loopJointCount; j++) {
		dAnimationKinematicLoopJoint* const joint = m_loopJoints[j];

		dAnimationAcyclicJoint* const node0 = joint->GetOwner0();
		dAnimationAcyclicJoint* const node1 = joint->GetOwner1();
		const int first = joint->m_start;
		const int auxiliaryDof = joint->m_count;

		dAssert (node0 == m_nodesOrder[node0->GetIndex()]);
		dAssert (node1 == m_nodesOrder[node1->GetIndex()]);

		dComplementaritySolver::dBodyState* const state0 = node0->GetProxyBody();
		dComplementaritySolver::dBodyState* const state1 = node1->GetProxyBody();
		dAssert (state0 == m_nodesOrder[node0->GetIndex()]->GetProxyBody());
		dAssert (state1 == m_nodesOrder[node1->GetIndex()]->GetProxyBody());

		const dVector& force0 = state0->GetForce();
		const dVector& torque0 = state0->GetTorque();

		const dVector& force1 = state1->GetForce();
		const dVector& torque1 = state1->GetTorque();

		const dMatrix& invInertia0 = state0->GetInvInertia();
		const dMatrix& invInertia1 = state1->GetInvInertia();

		const dFloat invMass0 = state0->GetInvMass();
		const dFloat invMass1 = state1->GetInvMass();

		const dComplementaritySolver::dJacobianPair* const row = &m_leftHandSide[first];
		const dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[first];
		for (int i = 0; i < auxiliaryDof; i++) {
			dVector acc (row[i].m_jacobian_J01.m_linear.Scale(invMass0) * force0 +
						 row[i].m_jacobian_J01.m_angular * invInertia0.RotateVector(torque0) +
						 row[i].m_jacobian_J10.m_linear.Scale(invMass1) * force1 +
						 row[i].m_jacobian_J10.m_angular * invInertia1.RotateVector(torque1));

			u[auxiliaryIndex] = rhs[i].m_force;
			f[auxiliaryIndex + primaryCount] = rhs[i].m_force;
			b[auxiliaryIndex] = rhs[i].m_coordenateAccel - acc.m_x - acc.m_y - acc.m_z;
			low[auxiliaryIndex] = rhs[i].m_jointLowFriction;
			high[auxiliaryIndex] = rhs[i].m_jointHighFriction;
			normalIndex[auxiliaryIndex] = rhs[i].m_normalIndex;
			auxiliaryIndex++;
		}
	}

	for (int i = 0; i < n; i++) {
		dFloat* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		dFloat r = dFloat(0.0f);
		for (int j = 0; j < primaryCount; j++) {
			r += matrixRow10[j] * f[j];
		}
		b[i] -= r;
	}

	dGaussSeidelLcpSor(n, m_massMatrix11, u, b, normalIndex, low, high, dFloat(0.001f), 30, dFloat(1.15f));

	for (int i = 0; i < n; i++) {
		const dFloat s = u[i];
		f[primaryCount + i] = s;
		const dFloat* const deltaForce = &m_deltaForce[i * primaryCount];
		for (int j = 0; j < primaryCount; j++) {
			f[j] += deltaForce[j] * s;
		}
	}

	for (int i = 0; i < primaryCount + n ; i++) {
		int index = m_matrixRowsIndex[i];
		dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[index];
		const dComplementaritySolver::dJacobianPair* const row = &m_leftHandSide[index];
		const int m0 = m_pairs[i].m_m0;
		const int m1 = m_pairs[i].m_m1;

		dComplementaritySolver::dBodyState* const state0 = m_nodesOrder[m0]->GetProxyBody();
		dComplementaritySolver::dBodyState* const state1 = m_nodesOrder[m1]->GetProxyBody();

		rhs->m_force = f[i];
		dVector jointForce(f[i]);
		state0->SetForce(state0->GetForce() + row->m_jacobian_J01.m_linear * jointForce);
		state0->SetTorque(state0->GetTorque() + row->m_jacobian_J01.m_angular * jointForce);
		state1->SetForce(state1->GetForce() + row->m_jacobian_J10.m_linear * jointForce);
		state1->SetTorque(state1->GetTorque() + row->m_jacobian_J10.m_angular * jointForce);
	}

	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		const int first = joint->m_start;
		const int count = joint->m_count;
		const dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[first];
		for (int j = 0; j < count; j++) {
			const int k = joint->m_sourceJacobianIndex[j];
			joint->m_jointFeebackForce[k] = rhs[j].m_force;
		}
	}

	for (int i = 0; i < m_loopJointCount; i++) {
		dComplementaritySolver::dBilateralJoint* const joint = m_loopJoints[i];
		const int first = joint->m_start;
		const int count = joint->m_count;
		const dComplementaritySolver::dJacobianColum* const rhs = &m_rightHandSide[first];
		for (int j = 0; j < count; j++) {
			joint->m_jointFeebackForce[j] = rhs[j].m_force;
		}
	}
}

void dAnimationAcyclicSolver::DebugMassMatrix()
{
	dNodePair pairs[30];
	dFloat matrix[30 * 30];
	int matrixRowsIndex[30];

	int rows = 0;
	int auxiliaryCount = 0;
	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		dAssert(joint);
		dAssert(i == node->GetIndex());

		const int m0 = node->GetIndex();
		const int m1 = node->m_parent->GetIndex();
		const int first = joint->m_start;
		const int primaryDof = joint->m_dof;
		for (int j = 0; j < primaryDof; j++) {
			const int index = joint->m_sourceJacobianIndex[j];
			pairs[rows].m_m0 = m0;
			pairs[rows].m_m1 = m1;
			matrixRowsIndex[rows] = first + index;
			rows++;
		}
	}

	for (int i = 0; i < m_nodeCount - 1; i++) {
		dAnimationAcyclicJoint* const node = m_nodesOrder[i];
		const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
		dAssert(joint);
		dAssert(i == node->GetIndex());

		const int m0 = node->GetIndex();
		const int m1 = node->m_parent->GetIndex();
		const int first = joint->m_start;
		const int primaryDof = joint->m_dof;
		const int auxiliaryDof = joint->m_count - primaryDof;
		for (int j = 0; j < auxiliaryDof; j++) {
			const int index = joint->m_sourceJacobianIndex[primaryDof + j];
			pairs[rows].m_m0 = m0;
			pairs[rows].m_m1 = m1;
			matrixRowsIndex[rows] = first + index;
			rows++;
			auxiliaryCount ++;
		}
	}

	for (int j = 0; j < m_loopJointCount; j ++) {
		dAnimationKinematicLoopJoint* const joint = m_loopJoints[j];	
		const dAnimationAcyclicJoint* const node0 = joint->GetOwner0();
		const dAnimationAcyclicJoint* const node1 = joint->GetOwner1();
		const int m0 = node0->GetIndex();
		const int m1 = node1->GetIndex();
		const int first = joint->m_start;
		const int auxiliaryDof = joint->m_dof;
		for (int i = 0; i < auxiliaryDof; i++) {
			pairs[rows].m_m0 = m0;
			pairs[rows].m_m1 = m1;
			matrixRowsIndex[rows] = first + i;
			rows++;
			auxiliaryCount++;
		}
	}
	
	dFloat u[30];
	dFloat b[30];
	dFloat low[30];
	dFloat high[30];
	int normalIndex[30];
	const dVector zero(0.0f);
	memset (matrix, 0, sizeof (dFloat) * rows * rows);
	for (int i = 0; i < rows; i++) {
		const int ii = matrixRowsIndex[i];

		const dComplementaritySolver::dJacobianPair* const row_i = &m_leftHandSide[ii];
		const dComplementaritySolver::dJacobianColum* const rhs_i = &m_rightHandSide[ii];
		dFloat* const matrixRow11 = &matrix[rows * i];

		const int m0_i = pairs[i].m_m0;
		const int m1_i = pairs[i].m_m1;

		dAnimationAcyclicJoint* const node0 = m_nodesOrder[m0_i];
		dAnimationAcyclicJoint* const node1 = m_nodesOrder[m1_i];
		dComplementaritySolver::dBodyState* const state0 = node0->GetProxyBody();
		dComplementaritySolver::dBodyState* const state1 = node1->GetProxyBody();

		const dMatrix& invInertia0 = state0->GetInvInertia();
		const dMatrix& invInertia1 = state1->GetInvInertia();

		const dFloat invMass0 = state0->GetInvMass();
		const dFloat invMass1 = state1->GetInvMass();

		dComplementaritySolver::dJacobian J01invM0(row_i->m_jacobian_J01.m_linear.Scale(invMass0), invInertia0.RotateVector(row_i->m_jacobian_J01.m_angular));
		dComplementaritySolver::dJacobian J10invM1(row_i->m_jacobian_J10.m_linear.Scale(invMass1), invInertia1.RotateVector(row_i->m_jacobian_J10.m_angular));
		
		dVector force0 (state0->GetForce());
		dVector torque0 (state0->GetTorque());
		dVector force1(state1->GetForce());
		dVector torque1(state1->GetTorque());

		dVector accel (J01invM0.m_linear * force0 + J01invM0.m_angular * torque0 +
					   J10invM1.m_linear * force1 + J10invM1.m_angular * torque1);
		
		u[i] = 0.0f;
		b[i] = rhs_i->m_coordenateAccel - accel.m_x - accel.m_y - accel.m_z;
		low[i] = rhs_i->m_jointLowFriction;
		high[i] = rhs_i->m_jointHighFriction;
		normalIndex[i] = rhs_i->m_normalIndex;

		for (int j = 0; j < rows; j++) {
			const int jj = matrixRowsIndex[j];
			const dComplementaritySolver::dJacobianPair* const row_j = &m_leftHandSide[jj];

			const int k = j;
			const int m0_j = pairs[k].m_m0;
			const int m1_j = pairs[k].m_m1;

			bool hasEffect = false;

			dVector acc(zero);
			if (m0_i == m0_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J01.m_linear + J01invM0.m_angular * row_j->m_jacobian_J01.m_angular;
			} else if (m0_i == m1_j) {
				hasEffect = true;
				acc += J01invM0.m_linear * row_j->m_jacobian_J10.m_linear + J01invM0.m_angular * row_j->m_jacobian_J10.m_angular;
			}

			if (m1_i == m1_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J10.m_linear + J10invM1.m_angular * row_j->m_jacobian_J10.m_angular;
			} else if (m1_i == m0_j) {
				hasEffect = true;
				acc += J10invM1.m_linear * row_j->m_jacobian_J01.m_linear + J10invM1.m_angular * row_j->m_jacobian_J01.m_angular;
			}

			if (hasEffect) {
				dFloat offDiagValue = acc.m_x + acc.m_y + acc.m_z;
				matrixRow11[j] = offDiagValue;
			}
		}
	}
/*
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < rows; j++) {
			dTrace(("%f ", matrix[i * rows + j]));
		}
		dTrace (("\n"));
	}

	dTrace(("\n"));
	for (int i = 0; i < rows; i++) {
		dTrace(("%f ", b[i]));
	}
	dTrace(("\n"));
*/
//	dGaussSeidelLcpSor(rows, matrix, u, b, normalIndex, low, high, dFloat(1.0e-3f), 1000, dFloat(1.15f));
	dSolvePartitionDantzigLCP(rows, matrix, u, b, low, high, rows - auxiliaryCount);

	for (int i = 0; i < rows; i++) {
		dTrace(("%f ", u[i]));
	}
//	dTrace(("\n"));

}

void dAnimationAcyclicSolver::Update(dFloat timestep)
{
	if (!m_rootNode->m_children.GetCount()) {
		return;
	}

	dAnimationKinematicLoopJoint* kinematicLoop[128];
	m_loopJoints = kinematicLoop;

	m_rootNode->ApplyExternalForce(timestep);

	m_loopJointCount = m_rootNode->GetKinematicLoops(m_loopJoints);

	int loopDof = 0;
	m_loopNodeCount = 0;
	for (int i = 0; i < m_loopJointCount; i ++) {
		dAnimationKinematicLoopJoint* const loop = m_loopJoints[i];
		dAssert (loop->IsActive());
		loopDof += loop->GetMaxDof();
		dAnimationAcyclicJoint* const node0 = loop->GetOwner0();
		dAnimationAcyclicJoint* const node1 = loop->GetOwner1();

		if (node0->IsLoopNode() && (node0->GetIndex() == -1)) {
			node0->SetIndex(m_loopNodeCount + m_nodeCount);
			m_nodesOrder[m_nodeCount + m_loopNodeCount] = node0;
			dAssert ((m_nodeCount + m_loopNodeCount) < m_maxNodeCount);
			m_loopNodeCount ++;
		}
		if (node1->IsLoopNode() && (node1->GetIndex() == -1)) {
			node1->SetIndex(m_loopNodeCount + m_nodeCount);
			m_nodesOrder[m_nodeCount + m_loopNodeCount] = node1;
			dAssert ((m_nodeCount + m_loopNodeCount) < m_maxNodeCount);
			m_loopNodeCount++;
		}
	}

	int totalJoint = m_nodeCount + m_loopNodeCount;
	m_data = dAlloca(dBodyJointMatrixDataPair, m_nodeCount);
	m_leftHandSide = dAlloca(dComplementaritySolver::dJacobianPair, totalJoint * 6 + loopDof);
	m_rightHandSide = dAlloca(dComplementaritySolver::dJacobianColum, totalJoint * 6 + loopDof);

	BuildJacobianMatrix(timestep);

	InitMassMatrix();
	m_pairs = dAlloca(dNodePair, m_rowCount + m_loopRowCount);
	m_matrixRowsIndex = dAlloca(int, m_rowCount + m_loopRowCount);

	const int row = m_rowCount - m_auxiliaryRowCount;
	const int col = m_auxiliaryRowCount + m_loopRowCount;

	m_deltaForce = dAlloca(dFloat, row * col);
	m_massMatrix10 = dAlloca(dFloat, row * col);
	m_massMatrix11 = dAlloca(dFloat, col * col);

	if (m_auxiliaryRowCount || m_loopRowCount) {
		InitLoopMassMatrix();
	}

	dVectorPair* const force = dAlloca(dVectorPair, m_nodeCount);
	dVectorPair* const accel = dAlloca(dVectorPair, m_nodeCount);
	CalculateJointAccel(accel);
	CalculateOpenLoopForce(force, accel);

	if (m_auxiliaryRowCount || m_loopRowCount) {
		SolveAuxiliary(force, accel);
	} else {
		UpdateForces(force);
	}

/*
//DebugMassMatrix();
for (int i = 0; i < m_nodeCount - 1; i++) {
	dAnimationAcyclicJoint* const node = m_nodesOrder[i];
	const dComplementaritySolver::dBilateralJoint* const joint = node->GetProxyJoint();
	for (int j = 0; j < joint->m_dof; j++) {
		dTrace(("%f ", joint->m_jointFeebackForce[j]));
	}
}
for (int i = 0; i < m_loopJointCount; i++) {
	dAnimationKinematicLoopJoint* const joint = m_loopJoints[i];
	for (int j = 0; j < joint->m_dof; j++) {
		dTrace(("%f ", joint->m_jointFeebackForce[j]));
	}
}
dTrace(("\n"));
*/
}
