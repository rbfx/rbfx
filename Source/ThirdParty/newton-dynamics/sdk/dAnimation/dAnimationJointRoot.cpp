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
#include "dAnimationJointRoot.h"
#include "dAnimationLoopJoint.h"
#include "dAnimationModelManager.h"

dAnimationJointRoot::dAnimationJointRoot(NewtonBody* const body, const dMatrix& bindMarix)
	:dAnimationJoint(body, bindMarix, NULL)
	,m_staticBody()
	,m_solver()
	,m_loopJoints()
	,m_manager(NULL)
	,m_managerNode(NULL)
	,m_calculateLocalTransform(true)
{
}

dAnimationJointRoot::~dAnimationJointRoot()
{
	for (dAnimationLoopJointList::dListNode* ptr = m_loopJoints.GetFirst(); ptr; ptr = ptr->GetNext()) {
		delete ptr->GetInfo();
	}

	if (m_manager) {
		dAssert(m_managerNode);
		m_manager->RemoveModel(this);
	}
}

void dAnimationJointRoot::Finalize()
{
	CopyRigidBodyMassToStates();
	m_solver.Finalize(this);
}

void dAnimationJointRoot::UpdateTransforms(dFloat timestep) const
{
	if (m_calculateLocalTransform) {
		dMatrix parentMatrixPool[128];
		const dAnimationJoint* stackPool[128];

		int stack = 1;
		stackPool[0] = this;
		parentMatrixPool[0] = dGetIdentityMatrix();

		while (stack) {
			dMatrix matrix;
			stack--;

			dMatrix parentMatrix(parentMatrixPool[stack]);
			const dAnimationJoint* const bone = stackPool[stack];

			NewtonBodyGetMatrix(bone->GetBody(), &matrix[0][0]);
			m_manager->OnUpdateTransform(bone, matrix * parentMatrix * bone->GetBindMatrix());

			parentMatrix = matrix.Inverse();
			const dAnimationJointChildren& boneChildren = bone->GetChildren();
			for (dAnimationJointChildren::dListNode* ptrNode = boneChildren.GetFirst(); ptrNode; ptrNode = ptrNode->GetNext()) {
				parentMatrixPool[stack] = parentMatrix;
				stackPool[stack] = ptrNode->GetInfo();
				stack++;
			}
		}
	}
}

