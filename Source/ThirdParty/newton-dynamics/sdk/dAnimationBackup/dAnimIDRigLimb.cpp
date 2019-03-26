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
#include "dAnimIDRigLimb.h"
#include "dAnimIDRigEffector.h"
#include "dAnimIDManager.h"


dAnimIDRigLimb::dAnimIDRigLimb(dAnimIDRigJoint* const parent, NewtonBody* const body)
	:dAnimIDRigJoint(parent)
	,dComplementaritySolver::dBilateralJoint()
	,m_newtonBody(body)
	,m_effector(NULL)
{
	m_proxyJoint = this;
	dAnimIDRigJoint::Init (body);
	dComplementaritySolver::dBilateralJoint::Init (dAnimIDRigJoint::GetProxyBody(), parent->GetProxyBody());
}

dAnimIDRigLimb::~dAnimIDRigLimb()
{
	if (m_effector) {
		delete m_effector;
	}
}

NewtonBody* dAnimIDRigLimb::GetNewtonBody() const 
{ 
	return m_newtonBody; 
}

void dAnimIDRigLimb::Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const
{
	if (m_effector) {
		m_effector->Debug(debugDisplay);
	}

	dAnimIDRigJoint::Debug(debugDisplay);
}

void dAnimIDRigLimb::Finalize()
{
	if (m_effector) {
		m_effector->m_referenceBody = NULL;
		for (dAnimIDRigLimb* parentLimb = GetParent()->GetAsRigLimb(); parentLimb; parentLimb = parentLimb->GetParent()->GetAsRigLimb()) {
			if (parentLimb->m_effector) {
				m_effector->m_referenceBody = parentLimb->GetNewtonBody();
				break;
			}
		}
		if (!m_effector->m_referenceBody) {
			m_effector->m_referenceBody = GetRoot()->GetNewtonBody();
		}
	}

	dAnimIDRigJoint::Finalize();
}

int dAnimIDRigLimb::GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray)
{
	int count = 0;
	if (m_effector && m_effector->IsActive()) {
		jointArray[count] = m_effector;
		dAnimAcyclicJoint* const owner1 = m_effector->GetOwner1();
		if (owner1->IsLoopNode()) {
			m_effector->GetOwner1()->SetIndex(-1);
		}
		count ++;
	}
	return dAnimIDRigJoint::GetKinematicLoops(&jointArray[count]) + count;
}

