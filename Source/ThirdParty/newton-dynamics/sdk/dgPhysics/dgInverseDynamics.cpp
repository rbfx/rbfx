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
#include "dgInverseDynamics.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"


class dgInverseDynamics::dgJointInfo
{
	public:
	dgBilateralConstraint* m_joint;
	dgInt16 m_pairStart;
	dgInt16 m_pairCount;
};

class dgInverseDynamics::dgNodePair
{
	public:
	dgInt16 m_m0;
	dgInt16 m_m1;
};

DG_MSC_VECTOR_ALIGMENT
class dgInverseDynamics::dgForcePair
{
	public:
	dgSpatialVector m_joint;
	dgSpatialVector m_body;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT 
class dgInverseDynamics::dgMatriData
{
	public:
	dgSpatialMatrix m_jt;
	dgSpatialMatrix m_mass;
	dgSpatialMatrix m_invMass;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT 
class dgInverseDynamics::dgBodyJointMatrixDataPair
{
	public:
	dgMatriData m_body;
	dgMatriData m_joint;
} DG_GCC_VECTOR_ALIGMENT;


class dgInverseDynamics::dgNode
{
	public:

	DG_CLASS_ALLOCATOR(allocator)
	dgNode(dgDynamicBody* const body)
		:m_body (body)
		,m_joint(NULL)
		,m_parent(NULL)
		,m_child(NULL)
		,m_sibling(NULL)
		,m_primaryStart(0)
		,m_auxiliaryStart(0)
		,m_dof(0)
		,m_ikdof(0)
		,m_index(0)
		,m_swapJacobianBodiesIndex(0)
	{
	}

	dgNode (dgBilateralConstraint* const joint, dgNode* const parent)
		:m_body ((dgDynamicBody*) ((joint->GetBody0() == parent->m_body) ? joint->GetBody1() : joint->GetBody0()))
		,m_joint (joint)
		,m_parent(parent)
		,m_child(NULL)
		,m_sibling(NULL)
		,m_primaryStart(0)
		,m_auxiliaryStart(0)
		,m_dof(0)
		,m_ikdof(0)
		,m_index(0)
		,m_swapJacobianBodiesIndex(joint->GetBody0() == parent->m_body)
	{
		dgAssert (m_parent);
		dgAssert (m_body->GetInvMass().m_w != dgFloat32 (0.0f));
		if (m_parent->m_child) {
			m_sibling = m_parent->m_child;
		}
		m_parent->m_child = this;
	}

	DG_INLINE ~dgNode()
	{
		dgNode* next;
		for (dgNode* ptr = m_child; ptr; ptr = next) {
			next = ptr->m_sibling;
			delete ptr;
		}
	}

	DG_INLINE void CalculateInertiaMatrix()
	{
		dgSpatialMatrix& bodyMass = m_data.m_body.m_mass;

        dgFloat32 mass = m_body->GetMass().m_w;
		dgAssert(mass < dgFloat32(1.0e10f));
		dgMatrix inertia(m_body->CalculateInertiaMatrix());
		for (dgInt32 i = 0; i < 3; i++) {
			bodyMass[i][i] = mass;
			for (dgInt32 j = 0; j < 3; j++) {
				bodyMass[i + 3][j + 3] = inertia[i][j];
			}
		}
	}

	DG_INLINE void GetJacobians(const dgJointInfo* const jointInfo, const dgLeftHandSide* const matrixRow, const dgRightHandSide* const rightHandSide)
	{
		dgAssert(m_parent);
		dgAssert(jointInfo->m_joint == m_joint);

		dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		dgSpatialMatrix& jointMass = m_data.m_joint.m_mass;

		dgSpatialVector zero (dgFloat32(0.0f));
		const dgInt32 start = jointInfo->m_pairStart;
		if (!m_swapJacobianBodiesIndex) {
			for (dgInt32 i = 0; i < m_dof; i++) {
				const dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[start + k];
				const dgLeftHandSide* const row = &matrixRow[start + k];
				jointMass[i] = zero;
				jointMass[i][i] = -rhs->m_diagDamp;
				bodyJt[i] = dgSpatialVector (row->m_Jt.m_jacobianM0.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM0.m_angular * dgVector::m_negOne);
				jointJ[i] = dgSpatialVector (row->m_Jt.m_jacobianM1.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM1.m_angular * dgVector::m_negOne);
			}
		} else {
			for (dgInt32 i = 0; i < m_dof; i++) {
				const dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[start + k];
				const dgLeftHandSide* const row = &matrixRow[start + k];
				jointMass[i] = zero;
				jointMass[i][i] = -rhs->m_diagDamp;
				bodyJt[i] = dgSpatialVector(row->m_Jt.m_jacobianM1.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM1.m_angular * dgVector::m_negOne);
				jointJ[i] = dgSpatialVector(row->m_Jt.m_jacobianM0.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM0.m_angular * dgVector::m_negOne);
			}
		}
	}

	DG_INLINE dgInt32 Factorize(const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const matrixRow, const dgRightHandSide* const rightHandSide)
	{
		dgSpatialMatrix& bodyMass = m_data.m_body.m_mass;
		dgSpatialMatrix& bodyInvMass = m_data.m_body.m_invMass;

		bodyMass = dgSpatialMatrix(dgFloat32(0.0f));
        if (m_body->GetInvMass().m_w != dgFloat32 (0.0f)) {
			CalculateInertiaMatrix();
		}

		dgInt32 boundedDof = 0;
		m_ordinals = m_ordinalInit;
		if (m_joint) {
			dgAssert (m_parent);
			const dgJointInfo* const jointInfo = &jointInfoArray[m_index];
			dgAssert(jointInfo->m_joint == m_joint);

			m_dof = 0;
			m_ikdof = 0;
			dgInt32 count = jointInfo->m_pairCount;
			const dgInt32 first = jointInfo->m_pairStart;

			for (dgInt32 i = 0; i < count; i++) {
				dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[k + first];
				if (rhs->m_normalForceIndex) {
					bool test0 = rhs->m_lowerBoundFrictionCoefficent <= dgFloat32 (-DG_LCP_MAX_VALUE);
					bool test1 = rhs->m_upperBoundFrictionCoefficent >= dgFloat32 (DG_LCP_MAX_VALUE);
					if (!(test0 ^ test1)) {
						dgAssert (!(test0 && test1));
						dgSwap(m_sourceJacobianIndex[i], m_sourceJacobianIndex[count - 1]);
						i--;
						count--;
						m_ikdof++;
					}
				}
			}

			for (dgInt32 i = 0; i < count; i++) {
				dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[k + first];
				if ((rhs->m_lowerBoundFrictionCoefficent <= dgFloat32 (-DG_LCP_MAX_VALUE)) && (rhs->m_upperBoundFrictionCoefficent >= dgFloat32 (DG_LCP_MAX_VALUE))) {
					m_dof ++;
				} else {
					dgSwap(m_sourceJacobianIndex[i], m_sourceJacobianIndex[count - 1]);
					i--;
					count--;
				}
			}
			dgAssert (m_dof > 0);
			dgAssert (m_dof <= 6);
			boundedDof = jointInfo->m_pairCount - m_dof - m_ikdof;
			GetJacobians(jointInfo, matrixRow, rightHandSide);
		}

		if (m_body->GetInvMass().m_w != dgFloat32(0.0f)) {
			for (dgNode* child = m_child; child; child = child->m_sibling) {
				CalculateBodyDiagonal(child);
			}
			bodyInvMass = bodyMass.Inverse(6);
		} else {
			bodyInvMass = dgSpatialMatrix(dgFloat32(0.0f));
		}

		if (m_joint) {
			dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
			dgAssert(m_parent);
			for (dgInt32 i = 0; i < m_dof; i++) {
				bodyJt[i] = bodyInvMass.VectorTimeMatrix(bodyJt[i]);
			}
			CalculateJointDiagonal();
			CalculateJacobianBlock();
		}
		return boundedDof;
	}

	DG_INLINE void CalculateBodyDiagonal(dgNode* const child)
	{
		dgAssert(child->m_joint);
		
		dgSpatialMatrix copy (dgFloat32(0.0f));
		const dgInt32 dof = child->m_dof;
		const dgSpatialMatrix& jacobianMatrix = child->m_data.m_joint.m_jt;
		const dgSpatialMatrix& childDiagonal = child->m_data.m_joint.m_mass;
		for (dgInt32 i = 0; i < dof ; i++) {
			const dgSpatialVector& jacobian = jacobianMatrix[i];
			for (dgInt32 j = 0; j < dof ; j++) {
				dgAssert(dgAreEqual (dgFloat64(childDiagonal[i][j]), dgFloat64(childDiagonal[j][i]), dgFloat64(1.0e-5f)));
				dgFloat64 val = childDiagonal[i][j];
				copy[j] = copy[j] + jacobian.Scale(val);
			}
		}

		dgSpatialMatrix& bodyMass = m_data.m_body.m_mass;
		for (dgInt32 i = 0; i < dof; i++) {
			const dgSpatialVector& Jacobian = copy[i];
			const dgSpatialVector& JacobianTranspose = jacobianMatrix[i];
			for (dgInt32 j = 0; j < 6; j++) {
				dgFloat64 val = -Jacobian[j];
				bodyMass[j] = bodyMass[j] + JacobianTranspose.Scale(val);
			}
		}
	}

	DG_INLINE void CalculateJointDiagonal ()
	{
		const dgSpatialMatrix& bodyMass = m_data.m_body.m_mass;
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;

		dgSpatialMatrix tmp;
		for (dgInt32 i = 0; i < m_dof; i++) {
			tmp[i] = bodyMass.VectorTimeMatrix(bodyJt[i]);
		}

		dgSpatialMatrix& jointMass = m_data.m_joint.m_mass;
		for (dgInt32 i = 0; i < m_dof; i++) {
			dgFloat64 a = bodyJt[i].DotProduct(tmp[i]);
			jointMass[i][i] -= a;
			for (dgInt32 j = i + 1; j < m_dof; j++) {
				a = - bodyJt[i].DotProduct(tmp[j]);
				jointMass[i][j] = a;
				jointMass[j][i] = a;
			}
		}

		dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		jointInvMass = jointMass.Inverse(m_dof);
	}

	DG_INLINE void CalculateJacobianBlock()
	{
		dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;

		dgSpatialMatrix copy;
		for (dgInt32 i = 0; i < m_dof; i++) {
			copy[i] = jointJ[i];
			jointJ[i] = dgSpatialVector(dgFloat32(0.0f));
		}

		const dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		for (dgInt32 i = 0; i < m_dof; i++) {
			const dgSpatialVector& jacobian = copy[i];
			const dgSpatialVector& invDiagonalRow = jointInvMass[i];
			for (dgInt32 j = 0; j < m_dof; j++) {
				dgFloat64 val = invDiagonalRow[j];
				jointJ[j] = jointJ[j] + jacobian.Scale(val);
			}
		}
	}

	DG_INLINE void JointJacobianTimeMassForward (dgForcePair& force)
	{
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_joint[i] -= bodyJt[i].DotProduct(force.m_body);
		}
	}

	DG_INLINE void BodyJacobianTimeMassForward(const dgForcePair& force, dgForcePair& parentForce) const 
	{
		const dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			parentForce.m_body = parentForce.m_body + jointJ[i].Scale(-force.m_joint[i]);
		}
	}

	DG_INLINE void JointJacobianTimeSolutionBackward(dgForcePair& force, const dgForcePair& parentForce)
	{
		const dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		const dgSpatialVector& f = parentForce.m_body;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_joint[i] -= f.DotProduct(jointJ[i]);
		}
	}

	DG_INLINE void BodyJacobianTimeSolutionBackward(dgForcePair& force)
	{
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_body = force.m_body + bodyJt[i].Scale(-force.m_joint[i]);
		}
	}

	DG_INLINE void BodyDiagInvTimeSolution(dgForcePair& force)
	{
		const dgSpatialMatrix& bodyInvMass = m_data.m_body.m_invMass;
		force.m_body = bodyInvMass.VectorTimeMatrix(force.m_body);
	}

	DG_INLINE void JointDiagInvTimeSolution(dgForcePair& force)
	{
		const dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		force.m_joint = jointInvMass.VectorTimeMatrix(force.m_joint, m_dof);
	}

	DG_INLINE dgInt32 GetAuxiliaryRows(const dgJointInfo* const jointInfoArray, const dgRightHandSide* const rightHandSide) const
	{
		dgInt32 rowCount = 0;
		if (m_joint) {
			dgAssert(m_parent);
			const dgJointInfo* const jointInfo = &jointInfoArray[m_index];
			dgAssert(jointInfo->m_joint == m_joint);
			dgInt32 count = jointInfo->m_pairCount;
			const dgInt32 first = jointInfo->m_pairStart;
			for (dgInt32 i = 0; i < count; i++) {
				//const dgLeftHandSide* const row = &matrixRow[i + first];
				const dgRightHandSide* const rhs = &rightHandSide[i + first];
				if (!((rhs->m_lowerBoundFrictionCoefficent <= dgFloat32 (-DG_LCP_MAX_VALUE)) && (rhs->m_upperBoundFrictionCoefficent >= dgFloat32 (DG_LCP_MAX_VALUE)))) {
					rowCount++;
				}
			}
		}
		return rowCount;
	}
	
	dgBodyJointMatrixDataPair m_data;
	dgDynamicBody* m_body;
	dgBilateralConstraint* m_joint;
	dgNode* m_parent;
	dgNode* m_child;
	dgNode* m_sibling;
	union
	{
		dgInt64 m_ordinals;
		dgInt8 m_sourceJacobianIndex[8];
	};
	dgInt16 m_primaryStart;
	dgInt16 m_auxiliaryStart;

	dgInt8 m_dof;
	dgInt8 m_ikdof;
	dgInt8 m_index;
	dgInt8 m_swapJacobianBodiesIndex;
	static dgInt64 m_ordinalInit;
};

dgInt64 dgInverseDynamics::dgNode::m_ordinalInit = 0x050403020100ll;


dgInverseDynamics::dgInverseDynamics(dgWorld* const world)
	:m_world(world)
	,m_skeleton(NULL)
	,m_nodesOrder(NULL)
	,m_pairs(NULL)
	,m_deltaForce(NULL)
	,m_massMatrix11(NULL)
	,m_massMatrix10(NULL)
	,m_rowArray(NULL)
	,m_loopingJoints(world->GetAllocator())
	,m_nodeCount(1)
	,m_rowCount(0)
	,m_ikRowCount(0)
	,m_auxiliaryRowCount(0)
{
}

dgInverseDynamics::~dgInverseDynamics()
{
	dgList<dgLoopingJoint>::dgListNode* ptr1 = NULL;
	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr1) {
		ptr1 = ptr->GetNext();
		RemoveLoopJoint(ptr);
	}
	//dgAssert(m_loopingJoints.GetCount() == 0);

	dgMemoryAllocator* const allocator = m_world->GetAllocator();
	if (m_nodesOrder) {
		allocator->Free(m_nodesOrder);
	}

	delete m_skeleton;
}

dgInverseDynamics::dgNode* dgInverseDynamics::AddRoot(dgDynamicBody* const rootBody)
{
	dgAssert (!m_skeleton);
	m_skeleton = new (m_world->GetAllocator()) dgNode(rootBody);
	return m_skeleton;
}

dgInverseDynamics::dgNode* dgInverseDynamics::GetRoot () const
{
	return m_skeleton;
}

dgInverseDynamics::dgNode* dgInverseDynamics::GetParent (dgNode* const node) const
{
	return node->m_parent;
}

dgDynamicBody* dgInverseDynamics::GetBody(dgInverseDynamics::dgNode* const node) const
{
	return node->m_body;
}

dgBilateralConstraint* dgInverseDynamics::GetJoint(dgInverseDynamics::dgNode* const node) const
{
	return node->m_joint;
}

dgInverseDynamics::dgNode* dgInverseDynamics::GetFirstChild(dgInverseDynamics::dgNode* const parent) const
{
	return parent->m_child;
}

dgInverseDynamics::dgNode* dgInverseDynamics::GetNextSiblingChild(dgInverseDynamics::dgNode* const sibling) const
{
	return sibling->m_sibling;
}

dgWorld* dgInverseDynamics::GetWorld() const
{
	return m_world;
}


void dgInverseDynamics::SortGraph(dgNode* const root, dgInt32& index)
{
	for (dgNode* node = root->m_child; node; node = node->m_sibling) {
		SortGraph(node, index);
	}

	dgAssert((m_nodeCount - index - 1) >= 0);
	m_nodesOrder[index] = root;
	root->m_index = dgInt8(index);
	index++;
	dgAssert(index <= m_nodeCount);
}

dgInverseDynamics::dgNode* dgInverseDynamics::FindNode(dgDynamicBody* const body) const
{
	dgInt32 stack = 1;
	dgNode* stackPool[1024];

	stackPool[0] = m_skeleton;
	while (stack) {
		stack--;
		dgNode* const node = stackPool[stack];
		if (node->m_body == body) {
			return node;
		}

		for (dgNode* ptr = node->m_child; ptr; ptr = ptr->m_sibling) {
			stackPool[stack] = ptr;
			stack++;
			dgAssert(stack < dgInt32(sizeof (stackPool) / sizeof (stackPool[0])));
		}
	}
	return NULL;
}

dgInverseDynamics::dgNode* dgInverseDynamics::AddChild(dgBilateralConstraint* const joint, dgNode* const parent)
{
	dgAssert (m_skeleton->m_body);
	dgMemoryAllocator* const allocator = m_world->GetAllocator();
	dgNode* const node = new (allocator)dgNode(joint, parent);
	m_nodeCount ++;


	dgAssert (m_world->GetSentinelBody() != node->m_body);
	return node;
}

dgList<dgInverseDynamics::dgLoopingJoint>::dgListNode* dgInverseDynamics::FindLoopJointNode(dgBilateralConstraint* const joint) const
{
	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		if (ptr->GetInfo().m_joint == joint) {
			return ptr;
		}
	}
	return NULL;
}


bool dgInverseDynamics::AddEffector (dgBilateralConstraint* const joint)
{
	dgLoopingJoint cyclicEntry(joint, 0, 0, 0, 1);
	dgNode* const node0 = FindNode((dgDynamicBody*)joint->GetBody0());
	dgNode* const node1 = FindNode((dgDynamicBody*)joint->GetBody1());
	if (node0 || node1) {
		m_loopingJoints.Append(cyclicEntry);
	}
	return node0 || node1;
}

bool dgInverseDynamics::AddLoopJoint(dgBilateralConstraint* const joint)
{
	dgLoopingJoint cyclicEntry(joint, 0, 0, 0, 0);
	dgNode* const node0 = FindNode((dgDynamicBody*)joint->GetBody0());
	dgNode* const node1 = FindNode((dgDynamicBody*)joint->GetBody1());
	if (node0 || node1) {
		m_loopingJoints.Append(cyclicEntry);
	}
	return node0 || node1;
}


void dgInverseDynamics::RemoveLoopJoint(dgList<dgLoopingJoint>::dgListNode* const node)
{
	const dgLoopingJoint& loopJoint = node->GetInfo();
	if (loopJoint.m_isEffector) {
		dgBilateralConstraint* const joint = node->GetInfo().m_joint;
		m_loopingJoints.Remove(node);
		delete joint;
	}
}


void dgInverseDynamics::RemoveLoopJoint(dgBilateralConstraint* const joint)
{
	dgList<dgLoopingJoint>::dgListNode* const node = FindLoopJointNode(joint);
	if (node) {
		RemoveLoopJoint(node);
	}
}


void dgInverseDynamics::Finalize()
{
	dgAssert(m_nodeCount >= 1);

	if (m_skeleton) {
		const dgDynamicBody* const rootBody = m_skeleton->m_body;

		dgMemoryAllocator* const allocator = rootBody->GetWorld()->GetAllocator();
		m_nodesOrder = (dgNode**)allocator->Malloc(m_nodeCount * sizeof (dgNode*));

		dgInt32 index = 0;
		SortGraph(m_skeleton, index);
		dgAssert(index == m_nodeCount);

		dgInt32 loopJointsCount = 0;
		dgInt32 isEffector[1024];
		dgBilateralConstraint* loopJointArray[1024];
		for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
			const dgLoopingJoint& entry = ptr->GetInfo();
			isEffector[loopJointsCount] = entry.m_isEffector;
			loopJointArray[loopJointsCount] = entry.m_joint;
			loopJointsCount ++;
		}
		m_loopingJoints.RemoveAll();

		dgInt32 infoIndex = m_nodeCount - 1;
		dgInt32 loopBodyIndex = m_nodeCount;
		dgTree<dgInt32, dgDynamicBody*> filter (allocator);
		for (dgInt32 i = 0; i < loopJointsCount; i++) {
			dgBilateralConstraint* const joint = loopJointArray[i];
			dgDynamicBody* const body0 = (dgDynamicBody*)joint->GetBody0();
			dgDynamicBody* const body1 = (dgDynamicBody*)joint->GetBody1();

			dgAssert(body0->IsRTTIType(dgBody::m_dynamicBodyRTTI));
			dgAssert(body1->IsRTTIType(dgBody::m_dynamicBodyRTTI));

			dgNode* const node0 = FindNode(body0);
			dgNode* const node1 = FindNode(body1);
			dgAssert((node0 && !node1) || (node1 && !node0));
		
			if (node0) {
				filter.Insert(node0->m_index, node0->m_body);
			}
			if (node1) {
				filter.Insert(node1->m_index, node1->m_body);
			}

			dgTree<dgInt32, dgDynamicBody*>::dgTreeNode* index0 = filter.Find(body0);
			if (!index0) {
				index0 = filter.Insert(loopBodyIndex, body0);
				loopBodyIndex ++;
			}

			dgTree<dgInt32, dgDynamicBody*>::dgTreeNode* index1 = filter.Find(body1);
			if (!index1) {
				index1 = filter.Insert(loopBodyIndex, body1);
				loopBodyIndex++;
			}

			dgLoopingJoint loopJointEntry(joint, index0->GetInfo(), index1->GetInfo(), infoIndex, isEffector[i]);
			m_loopingJoints.Append(loopJointEntry);
			infoIndex ++;
		}
	}
}

void dgInverseDynamics::InitMassMatrix(const dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgInt8* const memoryBuffer)
{
	dgInt32 rowCount = 0;
	dgInt32 ikRowCount = 0;
	dgInt32 primaryStart = 0;
	dgInt32 auxiliaryCount = 0;

	if (m_nodesOrder) {
		for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
			dgNode* const node = m_nodesOrder[i];
			dgAssert (node->m_index == i);
			node->m_auxiliaryStart = dgInt16 (auxiliaryCount);
			node->m_primaryStart = dgInt16 (primaryStart);
			auxiliaryCount += node->Factorize(jointInfoArray, matrixRow, rightHandSide);
			ikRowCount += node->m_ikdof;
			rowCount += (jointInfoArray[i].m_pairCount);
			primaryStart += node->m_dof;
		}
		m_nodesOrder[m_nodeCount - 1]->Factorize(jointInfoArray, matrixRow, rightHandSide);
	}
	m_rowCount = dgInt16 (rowCount);
	m_ikRowCount = dgInt16 (ikRowCount);
	m_auxiliaryRowCount = dgInt16 (auxiliaryCount);

	dgInt32 extraAuxiliaryRows = 0;
	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		const dgLoopingJoint& entry = ptr->GetInfo();
		extraAuxiliaryRows += jointInfoArray[entry.m_infoIndex].m_pairCount;
	}
	m_rowCount += dgInt16 (extraAuxiliaryRows);
	m_auxiliaryRowCount += dgInt16 (extraAuxiliaryRows);

	if (m_auxiliaryRowCount) {
		const dgInt32 primaryCount = m_rowCount - m_auxiliaryRowCount - m_ikRowCount;
		dgInt32 primaryIndex = 0;
		dgInt32 auxiliaryIndex = 0;

		m_rowArray = (dgLeftHandSide**) memoryBuffer;
		m_pairs = (dgNodePair*) &m_rowArray[m_rowCount];
		m_massMatrix11 = (dgFloat32*)&m_pairs[m_rowCount];
		m_massMatrix10 = (dgFloat32*)&m_massMatrix11[m_auxiliaryRowCount * m_auxiliaryRowCount];
		m_deltaForce = (dgFloat32*)&m_massMatrix10[m_auxiliaryRowCount * primaryCount];
		m_righHandSize = (dgRightHandSide**)&m_deltaForce[m_auxiliaryRowCount * primaryCount];

		for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
			dgAssert (m_nodesOrder[i]->m_index == i);
			const dgNode* const node = m_nodesOrder[i];
			const dgJointInfo* const jointInfo = &jointInfoArray[i];
			
			const dgInt16 m0 = dgInt16(node->m_swapJacobianBodiesIndex ? node->m_parent->m_index : i);
			const dgInt16 m1 = dgInt16(node->m_swapJacobianBodiesIndex ? i : node->m_parent->m_index);

			const dgInt32 primaryDof = node->m_dof;
			const dgInt32 first = jointInfo->m_pairStart;

			for (dgInt32 j = 0; j < primaryDof; j++) {
				const dgInt32 index = node->m_sourceJacobianIndex[j];
				m_rowArray[primaryIndex] = &matrixRow[first + index];
				m_righHandSize[primaryIndex] = &rightHandSide[first + index];
				m_pairs[primaryIndex].m_m0 = m0;
				m_pairs[primaryIndex].m_m1 = m1;
				primaryIndex++;
			}

			const dgInt32 auxiliaryDof = jointInfo->m_pairCount - primaryDof - node->m_ikdof;
			for (dgInt32 j = 0; j < auxiliaryDof; j++) {
				const dgInt32 index = node->m_sourceJacobianIndex[primaryDof + j];
				dgRightHandSide* const rhs = &rightHandSide[first + i];
				dgLeftHandSide* const row = &matrixRow[first + index];

				m_rowArray[auxiliaryIndex + primaryCount] = row;
				m_righHandSize[auxiliaryIndex + primaryCount] = rhs;

				m_pairs[auxiliaryIndex + primaryCount].m_m0 = m0;
				m_pairs[auxiliaryIndex + primaryCount].m_m1 = m1;
				auxiliaryIndex++;
			}
		}

		for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
			const dgLoopingJoint& entry = ptr->GetInfo();
			const dgJointInfo* const jointInfo = &jointInfoArray[entry.m_infoIndex];
			
			const dgInt16 m0 = entry.m_m0;
			const dgInt16 m1 = entry.m_m1;

			const dgInt32 first = jointInfo->m_pairStart;
			const dgInt32 auxiliaryDof = jointInfo->m_pairCount;

			for (dgInt32 i = 0; i < auxiliaryDof; i++) {
				dgRightHandSide* const rhs = &rightHandSide[first + i];
				dgLeftHandSide* const row = &matrixRow[first + i];

				m_rowArray[auxiliaryIndex + primaryCount] = row;
				m_righHandSize[auxiliaryIndex + primaryCount] = rhs;
				
				m_pairs[auxiliaryIndex + primaryCount].m_m0 = m0;
				m_pairs[auxiliaryIndex + primaryCount].m_m1 = m1;
				auxiliaryIndex++;
			}
		}

		dgFloat32* const diagDamp = dgAlloca(dgFloat32, m_auxiliaryRowCount);
		const dgInt32 auxiliaryBase = m_rowCount - m_auxiliaryRowCount - m_ikRowCount;
		for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
			const dgRightHandSide* const rhs_i = m_righHandSize[primaryCount + i];
			const dgLeftHandSide* const row_i = m_rowArray[primaryCount + i];
			dgFloat32* const matrixRow11 = &m_massMatrix11[m_auxiliaryRowCount * i];

			dgJacobian JMinvM0(row_i->m_JMinv.m_jacobianM0);
			dgJacobian JMinvM1(row_i->m_JMinv.m_jacobianM1);

			dgVector element(JMinvM0.m_linear * row_i->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_i->m_Jt.m_jacobianM0.m_angular +
							 JMinvM1.m_linear * row_i->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_i->m_Jt.m_jacobianM1.m_angular);
			element = element.AddHorizontal();

			// I know I am doubling the matrix regularize, but this makes the solution more robust.
			dgFloat32 diagonal = element.GetScalar() + rhs_i->m_diagDamp;
			matrixRow11[i] = diagonal + rhs_i->m_diagDamp;
			diagDamp[i] = matrixRow11[i] * (DG_PSD_DAMP_TOL * dgFloat32 (2.0f));

			const dgInt32 m0 = m_pairs[auxiliaryBase + i].m_m0;
			const dgInt32 m1 = m_pairs[auxiliaryBase + i].m_m1;
			for (dgInt32 j = i + 1; j < m_auxiliaryRowCount; j++) {
				const dgLeftHandSide* const row_j = m_rowArray[auxiliaryBase + j];

				const dgInt32 k = auxiliaryBase + j;
				dgVector acc(dgVector::m_zero);
				if (m0 == m_pairs[k].m_m0) {
					acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
				} else if (m0 == m_pairs[k].m_m1) {
					acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
				}

				if (m1 == m_pairs[k].m_m1) {
					acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
				} else if (m1 == m_pairs[k].m_m0) {
					acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
				}
				acc = acc.AddHorizontal();
				dgFloat32 offDiagValue = acc.GetScalar();
				matrixRow11[j] = offDiagValue;
				m_massMatrix11[j * m_auxiliaryRowCount + i] = offDiagValue;
			}

			dgFloat32* const matrixRow10 = &m_massMatrix10[primaryCount * i];
			for (dgInt32 j = 0; j < primaryCount; j++) {
				const dgLeftHandSide* const row_j = m_rowArray[j];

				dgVector acc(dgVector::m_zero);
				if (m0 == m_pairs[j].m_m0) {
					acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
				} else if (m0 == m_pairs[j].m_m1) {
					acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
				}

				if (m1 == m_pairs[j].m_m1) {
					acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
				} else if (m1 == m_pairs[j].m_m0) {
					acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
				}
				acc = acc.AddHorizontal();
				dgFloat32 val = acc.GetScalar();
				matrixRow10[j] = val;
			}
		}

		dgForcePair* const forcePair = dgAlloca(dgForcePair, m_nodeCount);
		dgForcePair* const accelPair = dgAlloca(dgForcePair, m_nodeCount);
		accelPair[m_nodeCount - 1].m_body = dgSpatialVector(dgFloat32(0.0f));
		accelPair[m_nodeCount - 1].m_joint = dgSpatialVector(dgFloat32(0.0f));

		for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
			dgFloat32* const matrixRow10 = &m_massMatrix10[i * primaryCount];

			dgInt32 entry = 0;
			for (dgInt32 j = 0; j < m_nodeCount - 1; j++) {
				const dgNode* const node = m_nodesOrder[j];
				const dgInt32 index = node->m_index;
				accelPair[index].m_body = dgSpatialVector(dgFloat32(0.0f));
				dgSpatialVector& a = accelPair[index].m_joint;

				const int count = node->m_dof;
				for (dgInt32 k = 0; k < count; k++) {
					a[k] = matrixRow10[entry];
					entry++;
				}
			}

			entry = 0;
			CalculateOpenLoopForce(forcePair, accelPair);
			dgFloat32* const deltaForcePtr = &m_deltaForce[i * primaryCount];
			for (dgInt32 j = 0; j < m_nodeCount - 1; j++) {
				const dgNode* const node = m_nodesOrder[j];
				const dgInt32 index = node->m_index;
				const dgSpatialVector& f = forcePair[index].m_joint;
				const int count = node->m_dof;
				for (dgInt32 k = 0; k < count; k++) {
					deltaForcePtr[entry] = dgFloat32 (f[k]);
					entry++;
				}
			}

			dgFloat32* const matrixRow11 = &m_massMatrix11[i * m_auxiliaryRowCount];
			dgFloat32 diagonal = matrixRow11[i];
			for (dgInt32 k = 0; k < primaryCount; k++) {
				diagonal += deltaForcePtr[k] * matrixRow10[k];
			}
			matrixRow11[i] = dgMax(diagonal, diagDamp[i]);

			for (dgInt32 j = i + 1; j < m_auxiliaryRowCount; j++) {
				dgFloat32 offDiagonal = dgFloat32(0.0f);
				const dgFloat32* const row10 = &m_massMatrix10[j * primaryCount];
				for (dgInt32 k = 0; k < primaryCount; k++) {
					offDiagonal += deltaForcePtr[k] * row10[k];
				}
				matrixRow11[j] += offDiagonal;
				m_massMatrix11[j * m_auxiliaryRowCount + i] += offDiagonal;
			}
		}

		dgCholeskyApplyRegularizer(m_auxiliaryRowCount, m_massMatrix11, diagDamp);
	}
}

bool dgInverseDynamics::SanityCheck(const dgForcePair* const force, const dgForcePair* const accel) const
{
	return true;
}

DG_INLINE void dgInverseDynamics::CalculateOpenLoopForce (dgForcePair* const force, const dgForcePair* const accel) const
{
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(node->m_joint);
		dgAssert(node->m_index == i);
		dgForcePair& f = force[i];
		const dgForcePair& a = accel[i];
		f.m_body = a.m_body;
		f.m_joint = a.m_joint; 
		for (dgNode* child = node->m_child; child; child = child->m_sibling) {
			dgAssert(child->m_joint);
			dgAssert(child->m_parent->m_index == i);
			child->BodyJacobianTimeMassForward(force[child->m_index], f);
		}
		node->JointJacobianTimeMassForward(f);
	}

	force[m_nodeCount - 1] = accel[m_nodeCount - 1];
	for (dgNode* child = m_nodesOrder[m_nodeCount - 1]->m_child; child; child = child->m_sibling) {
		child->BodyJacobianTimeMassForward(force[child->m_index], force[child->m_parent->m_index]);
	}

	m_nodesOrder[m_nodeCount - 1]->BodyDiagInvTimeSolution(force[m_nodeCount - 1]);
	for (dgInt32 i = m_nodeCount - 2; i >= 0; i--) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(node->m_index == i);
		dgForcePair& f = force[i];
		node->JointDiagInvTimeSolution(f);
		node->JointJacobianTimeSolutionBackward(f, force[node->m_parent->m_index]);
		node->BodyDiagInvTimeSolution(f);
		node->BodyJacobianTimeSolutionBackward(f);
	}
}

dgInt32 dgInverseDynamics::GetMemoryBufferSizeInBytes (const dgJointInfo* const jointInfoArray, const dgRightHandSide* const rightHandSide) const
{
	dgInt32 rowCount = 0;
	dgInt32 auxiliaryRowCount = 0;
	if (m_nodesOrder) {
		for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
			dgNode* const node = m_nodesOrder[i];
			rowCount += jointInfoArray[node->m_index].m_pairCount;
			auxiliaryRowCount += node->GetAuxiliaryRows(jointInfoArray, rightHandSide);
		}
	}

	dgInt32 extraAuxiliaryRows = 0;
	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		const dgLoopingJoint& entry = ptr->GetInfo();
		extraAuxiliaryRows += jointInfoArray[entry.m_infoIndex].m_pairCount;
	}

	const dgInt32 dofRows = rowCount;
	rowCount += extraAuxiliaryRows;
	auxiliaryRowCount += extraAuxiliaryRows;

	dgInt32 size = sizeof (dgLeftHandSide*) * rowCount;
	size += sizeof (dgRightHandSide*) * rowCount;
	size += sizeof (dgNodePair) * rowCount;
	size += sizeof (dgFloat32) * auxiliaryRowCount * auxiliaryRowCount;		// matrix11[auxiliaryRowCount * auxiliaryRowCount]
//	size += sizeof (dgFloat32) * auxiliaryRowCount * auxiliaryRowCount;		// matrixLowerTraingular [auxiliaryRowCount * auxiliaryRowCount]
	size += sizeof (dgFloat32) * auxiliaryRowCount * dofRows;
	size += sizeof (dgFloat32) * auxiliaryRowCount * dofRows;
	return (size + 1024) & -0x10;
}


DG_INLINE void dgInverseDynamics::CalculateJointAccel(dgJointInfo* const jointInfoArray, dgRightHandSide* const rightHandSide, dgForcePair* const accel) const
{
	dgSpatialVector zero (dgFloat32(0.0f));
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(i == node->m_index);

		dgForcePair& a = accel[i];
		dgAssert(node->m_body);
		a.m_body = zero;
		a.m_joint = zero;

		dgAssert(node->m_joint);
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_index];
		dgAssert(jointInfo->m_joint == node->m_joint);

		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 dof = jointInfo->m_pairCount;

		for (dgInt32 j = 0; j < dof; j++) {
			const dgInt32 k = node->m_sourceJacobianIndex[j];
			dgRightHandSide* const rhs = &rightHandSide[first + k];
			a.m_joint[j] = - rhs->m_penetrationStiffness;
		}
	}
	dgAssert((m_nodeCount - 1) == m_nodesOrder[m_nodeCount - 1]->m_index);
	accel[m_nodeCount - 1].m_body = zero;
	accel[m_nodeCount - 1].m_joint = zero;
}


DG_INLINE void dgInverseDynamics::CalculateInternalForces(dgJacobian* const internalForce, const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, const dgForcePair* const force) const
{
	dgVector zero(dgVector::m_zero);
	const dgInt32 bodyCount = m_nodeCount + 1;
	for (dgInt32 i = 0; i < bodyCount; i++) {
		internalForce[i].m_linear = zero; 
		internalForce[i].m_angular = zero; 
	}

	for (dgInt32 i = 0; i < (m_nodeCount - 1); i++) {
		dgNode* const node = m_nodesOrder[i];
		const dgJointInfo* const jointInfo = &jointInfoArray[i];

		dgJacobian y0;
		dgJacobian y1;
		y0.m_linear = zero;
		y0.m_angular = zero;
		y1.m_linear = zero;
		y1.m_angular = zero;
		dgAssert (node->m_index == i);
		dgAssert(jointInfo->m_joint == node->m_joint);

		const dgSpatialVector& f = force[i].m_joint;
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 count = node->m_dof;
		for (dgInt32 j = 0; j < count; j++) {
			const dgInt32 k = node->m_sourceJacobianIndex[j];
			dgRightHandSide* const rhs = &rightHandSide[first + k];
			const dgLeftHandSide* const row = &matrixRow[first + k];

			rhs->m_force += dgFloat32(f[j]);
			dgVector jointForce = dgFloat32(f[j]);
			y0.m_linear += row->m_Jt.m_jacobianM0.m_linear * jointForce;
			y0.m_angular += row->m_Jt.m_jacobianM0.m_angular * jointForce;
			y1.m_linear += row->m_Jt.m_jacobianM1.m_linear * jointForce;
			y1.m_angular += row->m_Jt.m_jacobianM1.m_angular * jointForce;
		}

		const dgInt32 m0 = node->m_swapJacobianBodiesIndex ? node->m_parent->m_index : i;
		const dgInt32 m1 = node->m_swapJacobianBodiesIndex ? i : node->m_parent->m_index;

		internalForce[m0].m_linear += y0.m_linear;
		internalForce[m0].m_angular += y0.m_angular;
		internalForce[m1].m_linear += y1.m_linear;
		internalForce[m1].m_angular += y1.m_angular;
	}
}

void dgInverseDynamics::CalculateCloseLoopsForces(dgJacobian* const externalForce, const dgJointInfo* const jointInfoArray, dgRightHandSide* const rightHandSide, const dgForcePair* const accel, dgForcePair* const force) const
{
	dgFloat32* const f = dgAlloca(dgFloat32, m_rowCount);
	dgFloat32* const u = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const b = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const low = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const high = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const massMatrix11 = dgAlloca(dgFloat32, m_auxiliaryRowCount * m_auxiliaryRowCount);

	dgInt32 primaryIndex = 0;
	dgInt32 auxiliaryIndex = 0;
	const dgInt32 primaryCount = m_rowCount - m_auxiliaryRowCount - m_ikRowCount;

	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		const dgNode* const node = m_nodesOrder[i];
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_index];
		dgAssert(jointInfo->m_joint == node->m_joint);

		const dgInt32 first = jointInfo->m_pairStart;

		const dgInt32 primaryDof = node->m_dof;
		const dgSpatialVector& accelSpatial = accel[i].m_joint;
		const dgSpatialVector& forceSpatial = force[i].m_joint;

		for (dgInt32 j = 0; j < primaryDof; j++) {
			f[primaryIndex] = dgFloat32(forceSpatial[j]);
			primaryIndex++;
		}

		const dgInt32 auxiliaryDof = jointInfo->m_pairCount - primaryDof - node->m_ikdof;
		for (dgInt32 j = 0; j < auxiliaryDof; j++) {
			const dgInt32 index = node->m_sourceJacobianIndex[primaryDof + j];
			dgRightHandSide* const rhs = &rightHandSide[first + index];
			f[auxiliaryIndex + primaryCount] = dgFloat32(0.0f);
			b[auxiliaryIndex] = -dgFloat32(accelSpatial[primaryDof + j]);
			low[auxiliaryIndex] = dgClamp(rhs->m_lowerBoundFrictionCoefficent, -DG_MAX_BOUND, dgFloat32(0.0f));
			high[auxiliaryIndex] = dgClamp(rhs->m_upperBoundFrictionCoefficent, dgFloat32(0.0f), DG_MAX_BOUND);
			auxiliaryIndex++;
		}
	}

	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		const dgLoopingJoint& entry = ptr->GetInfo();
		const dgJointInfo* const jointInfo = &jointInfoArray[entry.m_infoIndex];
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 auxiliaryDof = jointInfo->m_pairCount;

		for (dgInt32 i = 0; i < auxiliaryDof; i++) {
			dgRightHandSide* const rhs = &rightHandSide[first + i];
			f[auxiliaryIndex + primaryCount] = dgFloat32(0.0f);
			b[auxiliaryIndex] = rhs->m_penetrationStiffness;
			low[auxiliaryIndex] = dgClamp(rhs->m_lowerBoundFrictionCoefficent, -DG_MAX_BOUND, dgFloat32(0.0f));
			high[auxiliaryIndex] = dgClamp(rhs->m_upperBoundFrictionCoefficent, dgFloat32(0.0f), DG_MAX_BOUND);
			auxiliaryIndex++;
		}
	}

	memcpy(massMatrix11, m_massMatrix11, sizeof(dgFloat32) * m_auxiliaryRowCount * m_auxiliaryRowCount);
	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		dgFloat32* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		u[i] = dgFloat32(0.0f);
		dgFloat32 r = dgFloat32(0.0f);
		for (dgInt32 j = 0; j < primaryCount; j++) {
			r += matrixRow10[j] * f[j];
		}
		b[i] -= r;
	}

	dgSolveDantzigLCP(m_auxiliaryRowCount, massMatrix11, u, b, low, high);

	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		const dgFloat32 s = u[i];
		f[primaryCount + i] = s;
		const dgFloat32* const deltaForce = &m_deltaForce[i * primaryCount];
		for (dgInt32 j = 0; j < primaryCount; j++) {
			f[j] += deltaForce[j] * s;
		}
	}

	const dgInt32 rowCount = m_rowCount - m_ikRowCount;
	for (dgInt32 i = 0; i < rowCount; i++) {
		dgRightHandSide* const rhs = m_righHandSize[i];
		const dgLeftHandSide* const row = m_rowArray[i];
		const dgInt32 m0 = m_pairs[i].m_m0;
		const dgInt32 m1 = m_pairs[i].m_m1;

		rhs->m_force += f[i];
		dgVector jointForce(f[i]);
		externalForce[m0].m_linear += row->m_Jt.m_jacobianM0.m_linear * jointForce;
		externalForce[m0].m_angular += row->m_Jt.m_jacobianM0.m_angular * jointForce;
		externalForce[m1].m_linear += row->m_Jt.m_jacobianM1.m_linear * jointForce;
		externalForce[m1].m_angular += row->m_Jt.m_jacobianM1.m_angular * jointForce;
	}
}

DG_INLINE void dgInverseDynamics::CalculateRowJacobianDerivatives (dgInt32 index, const dgVector& invMass0, const dgVector& invMass1, const dgMatrix& invInertia0, const dgMatrix& invInertia1, const dgContraintDescritor& constraintParams, dgLeftHandSide* const row, dgRightHandSide* const rhs, int rowIsIK) const
{
	dgAssert(constraintParams.m_forceBounds[index].m_jointForce);
	row->m_Jt = constraintParams.m_jacobian[index];

	rhs->m_diagDamp = dgFloat32(0.0f);
	rhs->m_stiffness = DG_PSD_DAMP_TOL * (dgFloat32(1.0f) - constraintParams.m_jointStiffness[index]) + dgFloat32(1.0e-6f);
	dgAssert(rhs->m_stiffness >= dgFloat32(0.0f));
	dgAssert(constraintParams.m_jointStiffness[index] <= dgFloat32(1.0f));
	dgAssert((dgFloat32(1.0f) - constraintParams.m_jointStiffness[index]) >= dgFloat32(0.0f));
	rhs->m_coordenateAccel = constraintParams.m_jointAccel[index];
	rhs->m_restitution = constraintParams.m_restitution[index];
	rhs->m_penetration = constraintParams.m_penetration[index];
	rhs->m_penetrationStiffness = constraintParams.m_penetrationStiffness[index];
	rhs->m_lowerBoundFrictionCoefficent = constraintParams.m_forceBounds[index].m_low;
	rhs->m_upperBoundFrictionCoefficent = constraintParams.m_forceBounds[index].m_upper;
	rhs->m_jointFeebackForce = constraintParams.m_forceBounds[index].m_jointForce;
	// use this to indicate if this row is an inverse IK
	rhs->m_normalForceIndex = rowIsIK;

	dgAssert(row->m_Jt.m_jacobianM0.m_linear.m_w == dgFloat32(0.0f));
	dgAssert(row->m_Jt.m_jacobianM0.m_angular.m_w == dgFloat32(0.0f));
	dgAssert(row->m_Jt.m_jacobianM1.m_linear.m_w == dgFloat32(0.0f));
	dgAssert(row->m_Jt.m_jacobianM1.m_angular.m_w == dgFloat32(0.0f));

	row->m_JMinv.m_jacobianM0.m_linear = row->m_Jt.m_jacobianM0.m_linear * invMass0;
	row->m_JMinv.m_jacobianM0.m_angular = invInertia0.RotateVector(row->m_Jt.m_jacobianM0.m_angular);
	row->m_JMinv.m_jacobianM1.m_linear = row->m_Jt.m_jacobianM1.m_linear * invMass1;
	row->m_JMinv.m_jacobianM1.m_angular = invInertia1.RotateVector(row->m_Jt.m_jacobianM1.m_angular);
	dgVector tmpDiag(row->m_JMinv.m_jacobianM0.m_linear * row->m_Jt.m_jacobianM0.m_linear +
					 row->m_JMinv.m_jacobianM0.m_angular * row->m_Jt.m_jacobianM0.m_angular +
					 row->m_JMinv.m_jacobianM1.m_linear * row->m_Jt.m_jacobianM1.m_linear +
					 row->m_JMinv.m_jacobianM1.m_angular * row->m_Jt.m_jacobianM1.m_angular);

	dgAssert(tmpDiag.m_w == dgFloat32(0.0f));
	dgFloat32 diag = (tmpDiag.AddHorizontal()).GetScalar();
	dgAssert(diag > dgFloat32(0.0f));
	rhs->m_diagDamp = diag * rhs->m_stiffness;
	diag *= (dgFloat32(1.0f) + rhs->m_stiffness);
	rhs->m_invJinvMJt = dgFloat32(1.0f) / diag;
	rhs->m_force = dgFloat32(0.0f);
}

DG_INLINE dgInt32 dgInverseDynamics::GetJacobianDerivatives (dgBilateralConstraint* const constraint, dgContraintDescritor& constraintParams) const
{
	constraint->m_rowIsIk = 0;
	constraint->m_rowIsMotor = 0;
	for (dgInt32 i = 0; i < 6; i++) {
		constraint->m_inverseDynamicsAcceleration[i] = dgFloat32(0.0f);
		constraintParams.m_forceBounds[i].m_low = DG_MIN_BOUND;
		constraintParams.m_forceBounds[i].m_upper = DG_MAX_BOUND;
		constraintParams.m_forceBounds[i].m_jointForce = NULL;
		constraintParams.m_forceBounds[i].m_normalIndex = DG_INDEPENDENT_ROW;
	}
	return constraint->JacobianDerivative(constraintParams);
}

dgInt32 dgInverseDynamics::GetJacobianDerivatives(dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgFloat32 timestep, dgInt32 threadIndex) const
{
	dgContraintDescritor constraintParams;
	constraintParams.m_timestep = timestep;
	constraintParams.m_threadIndex = threadIndex;
	constraintParams.m_invTimestep = dgFloat32 (1.0f / timestep);

	dgDynamicBody** const bodyArray = dgAlloca (dgDynamicBody*, m_nodeCount + 1); 
	for (dgInt32 i = 0; i < m_nodeCount; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(i == node->m_index);
		bodyArray[i] = node->m_body;
		node->m_body->CalcInvInertiaMatrix();
	}
	bodyArray[m_nodeCount] = m_world->GetSentinelBody();
	dgInt32 rowCount = 0;	

	for (dgInt32 j = 0; j < m_nodeCount - 1; j++) {
		dgNode* const node = m_nodesOrder[j];
		dgAssert(j == node->m_index);
		dgJointInfo* const jointInfo = &jointInfoArray[j];
		jointInfo->m_joint = node->m_joint;
		dgBilateralConstraint* const constraint = jointInfo->m_joint;
		dgInt32 dof = GetJacobianDerivatives (constraint, constraintParams);

		jointInfo->m_pairCount = dgInt16(dof);
		jointInfo->m_pairStart = dgInt16(rowCount);
	
		const dgInt32 m0 = node->m_swapJacobianBodiesIndex ? node->m_parent->m_index : j;
		const dgInt32 m1 = node->m_swapJacobianBodiesIndex ? j : node->m_parent->m_index;
		const dgDynamicBody* const body0 = bodyArray[m0];
		const dgDynamicBody* const body1 = bodyArray[m1];
		dgAssert((body0 == constraint->GetBody0()) || (body0 == constraint->GetBody1()));
		dgAssert((body1 == constraint->GetBody0()) || (body1 == constraint->GetBody1()));

		const dgVector invMass0(body0->m_invMass[3]);
		const dgVector invMass1(body1->m_invMass[3]);
		const dgMatrix& invInertia0 = body0->m_invWorldInertiaMatrix;
		const dgMatrix& invInertia1 = body1->m_invWorldInertiaMatrix;
		const dgInt32 rowIsIk = constraint->m_rowIsIk;

		for (dgInt32 i = 0; i < dof; i++) {
			dgRightHandSide* const rhs = &rightHandSide[rowCount];
			dgLeftHandSide* const row = &matrixRow[rowCount];
			dgAssert(constraintParams.m_forceBounds[i].m_jointForce);
			CalculateRowJacobianDerivatives (i, invMass0, invMass1, invInertia0, invInertia1, constraintParams, row, rhs, rowIsIk & (1 << i));
			rowCount++;
		}
	}

	dgInt32 jointInfoCount = m_nodeCount - 1;
	for (dgList<dgLoopingJoint>::dgListNode* ptr = m_loopingJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		const dgLoopingJoint& entry = ptr->GetInfo();
		dgAssert (entry.m_infoIndex == jointInfoCount);
		dgJointInfo* const jointInfo = &jointInfoArray[jointInfoCount];
		jointInfoCount ++;

		jointInfo->m_joint = entry.m_joint;
		dgBilateralConstraint* const constraint = jointInfo->m_joint;
		dgInt32 dof = GetJacobianDerivatives (constraint, constraintParams);

		jointInfo->m_pairCount = dgInt16(dof);
		jointInfo->m_pairStart = dgInt16(rowCount);

		const dgInt32 m0 = entry.m_m0;
		const dgInt32 m1 = entry.m_m1;

		const dgDynamicBody* const body0 = bodyArray[m0];
		const dgDynamicBody* const body1 = bodyArray[m1];
		dgAssert ((body0 == constraint->GetBody0()) || (body0 == constraint->GetBody1()));
		dgAssert ((body1 == constraint->GetBody0()) || (body1 == constraint->GetBody1()));

		const dgVector invMass0(body0->m_invMass[3]);
		const dgMatrix& invInertia0 = body0->m_invWorldInertiaMatrix;
		const dgVector invMass1(body1->m_invMass[3]);
		const dgMatrix& invInertia1 = body1->m_invWorldInertiaMatrix;
		const dgInt32 rowIsIk = constraint->m_rowIsIk;

		for (dgInt32 i = 0; i < dof; i++) {
			dgRightHandSide* const rhs = &rightHandSide[rowCount];
			dgLeftHandSide* const row = &matrixRow[rowCount];
			dgAssert(constraintParams.m_forceBounds[i].m_jointForce);
			CalculateRowJacobianDerivatives(i, invMass0, invMass1, invInertia0, invInertia1, constraintParams, row, rhs, rowIsIk & (1 << i));
			rowCount++;
		}
	}

	return rowCount;
}


void dgInverseDynamics::CalculateMotorsAccelerations (const dgJacobian* const externalForce, const dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgFloat32 timestep) const
{
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(i == node->m_index);
		const dgJointInfo* const jointInfo = &jointInfoArray[i];
		dgBilateralConstraint* const joint = node->m_joint;

		const dgInt32 m0 = node->m_swapJacobianBodiesIndex ? node->m_parent->m_index : i;
		const dgInt32 m1 = node->m_swapJacobianBodiesIndex ? i : node->m_parent->m_index;
			
		const dgJacobian& force0 = externalForce[m0];
		const dgJacobian& force1 = externalForce[m1];

		const dgInt32 dof = jointInfo->m_pairCount;
		const dgInt32 first = jointInfo->m_pairStart;

		for (dgInt32 j = node->m_dof; j < dof; j++) {
			dgInt32 k = node->m_sourceJacobianIndex[j];
			dgLeftHandSide* const row = &matrixRow[first + k];
			dgVector accel(force0.m_linear * row->m_JMinv.m_jacobianM0.m_linear +
						   force0.m_angular * row->m_JMinv.m_jacobianM0.m_angular +
						   force1.m_linear * row->m_JMinv.m_jacobianM1.m_linear +
						   force1.m_angular * row->m_JMinv.m_jacobianM1.m_angular);
			dgFloat32 a = accel.AddHorizontal().GetScalar();
			joint->m_inverseDynamicsAcceleration[k] = a;
//dgTrace (("%f ", a));
		}
	}
//dgTrace (("\n"));
}

void dgInverseDynamics::Update (dgFloat32 timestep, dgInt32 threadIndex)
{
	if (m_skeleton) {
		dgJointInfo* const jointInfoArray = dgAlloca (dgJointInfo, m_nodeCount + m_loopingJoints.GetCount());
		dgRightHandSide* const righHandSide = dgAlloca (dgRightHandSide, 6 * (m_nodeCount + m_loopingJoints.GetCount()));
		dgLeftHandSide* const matrixRow = dgAlloca (dgLeftHandSide, 6 * (m_nodeCount + m_loopingJoints.GetCount()));
		GetJacobianDerivatives(jointInfoArray, matrixRow, righHandSide, timestep, threadIndex);

		dgInt32 memorySizeInBytes = GetMemoryBufferSizeInBytes(jointInfoArray, righHandSide);
		dgInt8* const memoryBuffer = dgAlloca(dgInt8, memorySizeInBytes);
		dgForcePair* const accel = dgAlloca(dgForcePair, m_nodeCount);
		dgForcePair* const force = dgAlloca(dgForcePair, m_nodeCount);
		dgJacobian* const internalForce = dgAlloca(dgJacobian, m_nodeCount + 1);

		dgAssert((dgInt64(accel) & 0x0f) == 0);
		dgAssert((dgInt64(force) & 0x0f) == 0);
		dgAssert((dgInt64(matrixRow) & 0x0f) == 0);
		dgAssert((dgInt64(memoryBuffer) & 0x0f) == 0);
		dgAssert((dgInt64(internalForce) & 0x0f) == 0);
		dgAssert((dgInt64(jointInfoArray) & 0x0f) == 0);

		InitMassMatrix(jointInfoArray, matrixRow, righHandSide, memoryBuffer);
		CalculateJointAccel(jointInfoArray, righHandSide, accel);
		CalculateOpenLoopForce(force, accel);

/*
for (int i = 0; i < m_nodeCount - 1; i++) {
	dgNode* const node = m_nodesOrder[i];
	dgBilateralConstraint* const joint = node->m_joint;

	const dgSpatialVector& f = force[i].m_joint;
	for (int j = 0; j < node->m_dof; j++) {
		dgTrace(("%f ", f[j]));
	}
}
dgTrace(("\n"));
*/

		CalculateInternalForces(internalForce, jointInfoArray, matrixRow, righHandSide, force);
		if (m_auxiliaryRowCount) {
			CalculateCloseLoopsForces(internalForce, jointInfoArray, righHandSide, accel, force);
		}

/*
for (int i = 0; i < m_nodeCount - 1; i++) {
	dgNode* const node = m_nodesOrder[i];
	dgBilateralConstraint* const joint = node->m_joint;

	const dgSpatialVector& f = force[i].m_joint;
	for (int j = 0; j < node->m_dof; j++) {
		dgTrace(("%f ", f[j]));
	}
}
dgTrace(("\n"));
*/

		CalculateMotorsAccelerations (internalForce, jointInfoArray, matrixRow, timestep);
	}
}