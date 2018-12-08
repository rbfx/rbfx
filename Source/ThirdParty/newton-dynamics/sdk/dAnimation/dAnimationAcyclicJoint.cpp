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

dAnimationAcyclicJoint::dAnimationAcyclicJoint(dAnimationAcyclicJoint* const parent)
	:dContainersAlloc()
	,m_proxyBody()
	,m_proxyJoint()
	,m_parent(parent)
	,m_userData(NULL)
	,m_world(NULL)
	,m_solverIndex(-1)
	,m_isLoop(false)
	,m_children()
{
	if (parent) {
		parent->m_children.Append(this);
	}
}

dAnimationAcyclicJoint::~dAnimationAcyclicJoint()
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		delete child->GetInfo();
	}
}

void* dAnimationAcyclicJoint::GetUserData()
{
	return m_userData;
}

void dAnimationAcyclicJoint::SetUserData(void* const userData)
{
	m_userData = userData;
}


void dAnimationAcyclicJoint::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Debug(debugContext);
	}
}

void dAnimationAcyclicJoint::ApplyExternalForce(dFloat timestep)
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->ApplyExternalForce(timestep);
	}
}

int dAnimationAcyclicJoint::GetKinematicLoops(dAnimationKinematicLoopJoint** const jointArray)
{
	int count = 0;
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		count += child->GetInfo()->GetKinematicLoops(&jointArray[count]);
	}
	return count;
}

void dAnimationAcyclicJoint::UpdateJointAcceleration()
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->UpdateJointAcceleration();
	}
}

void dAnimationAcyclicJoint::Finalize()
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Finalize();
	}
}