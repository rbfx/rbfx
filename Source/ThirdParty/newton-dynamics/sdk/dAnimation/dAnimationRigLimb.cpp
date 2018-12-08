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
#include "dAnimationRigLimb.h"
#include "dAnimationRigEffector.h"
#include "dAnimationCharacterRigManager.h"


dAnimationRigLimb::dAnimationRigLimb(dAnimationRigJoint* const parent, NewtonBody* const body)
	:dAnimationRigJoint(parent)
	,dComplementaritySolver::dBilateralJoint()
	,m_newtonBody(body)
	,m_effector(NULL)
{
	m_proxyJoint = this;
	dAnimationRigJoint::Init (body);
	dComplementaritySolver::dBilateralJoint::Init (dAnimationRigJoint::GetProxyBody(), parent->GetProxyBody());
}

dAnimationRigLimb::~dAnimationRigLimb()
{
	if (m_effector) {
		delete m_effector;
	}
}

NewtonBody* dAnimationRigLimb::GetNewtonBody() const 
{ 
	return m_newtonBody; 
}

void dAnimationRigLimb::Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const
{
	if (m_effector) {
		m_effector->Debug(debugDisplay);
	}

	dAnimationRigJoint::Debug(debugDisplay);
}

void dAnimationRigLimb::Finalize()
{
	if (m_effector) {
		m_effector->m_referenceBody = NULL;
		for (dAnimationRigLimb* parentLimb = GetParent()->GetAsRigLimb(); parentLimb; parentLimb = parentLimb->GetParent()->GetAsRigLimb()) {
			if (parentLimb->m_effector) {
				m_effector->m_referenceBody = parentLimb->GetNewtonBody();
				break;
			}
		}
		if (!m_effector->m_referenceBody) {
			m_effector->m_referenceBody = GetRoot()->GetNewtonBody();
		}
	}

	dAnimationRigJoint::Finalize();
}

int dAnimationRigLimb::GetKinematicLoops(dAnimationKinematicLoopJoint** const jointArray)
{
	int count = 0;
	if (m_effector && m_effector->IsActive()) {
		jointArray[count] = m_effector;
		dAnimationAcyclicJoint* const owner1 = m_effector->GetOwner1();
		if (owner1->IsLoopNode()) {
			m_effector->GetOwner1()->SetIndex(-1);
		}
		count ++;
	}
	return dAnimationRigJoint::GetKinematicLoops(&jointArray[count]) + count;
}

