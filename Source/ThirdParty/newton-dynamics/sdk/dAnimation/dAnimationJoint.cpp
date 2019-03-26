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
#include "dAnimationJoint.h"
#include "dAnimationModelManager.h"

dAnimationJoint::dAnimationJoint(NewtonBody* const body, const dMatrix& bindMarix, dAnimationJoint* const parent)
	:dCustomAlloc()
	,m_bindMatrix(bindMarix)
	,m_userData(NULL)
	,m_body(body)
	,m_joint(NULL)
	,m_parent(parent)
	,m_proxyJoint(NULL)
	,m_children()
{
	if (m_parent) {
		m_parent->m_children.Append(this);
	}
}

dAnimationJoint::~dAnimationJoint()
{
	for (dAnimationJointChildren::dListNode* node = m_children.GetFirst(); node; node = node->GetNext()) {
		delete node->GetInfo();
	}
}

void dAnimationJoint::PostUpdate(dAnimationModelManager* const manager, dFloat timestep) const
{
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
		manager->OnUpdateTransform(bone, matrix * parentMatrix * bone->GetBindMatrix());

		parentMatrix = matrix.Inverse();
		for (dAnimationJointChildren::dListNode* ptrNode = bone->m_children.GetFirst(); ptrNode; ptrNode = ptrNode->GetNext()) {
			parentMatrixPool[stack] = parentMatrix;
			stackPool[stack] = ptrNode->GetInfo();
			stack++;
		}
	}
}