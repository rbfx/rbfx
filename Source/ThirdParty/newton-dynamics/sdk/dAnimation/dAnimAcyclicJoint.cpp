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
#include "dAnimAcyclicJoint.h"

dAnimAcyclicJoint::dAnimAcyclicJoint(dAnimAcyclicJoint* const parent)
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

dAnimAcyclicJoint::~dAnimAcyclicJoint()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		delete child->GetInfo();
	}
}

void* dAnimAcyclicJoint::GetUserData()
{
	return m_userData;
}

void dAnimAcyclicJoint::SetUserData(void* const userData)
{
	m_userData = userData;
}


void dAnimAcyclicJoint::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Debug(debugContext);
	}
}

void dAnimAcyclicJoint::ApplyExternalForce(dFloat timestep)
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->ApplyExternalForce(timestep);
	}
}

int dAnimAcyclicJoint::GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray)
{
	int count = 0;
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		count += child->GetInfo()->GetKinematicLoops(&jointArray[count]);
	}
	return count;
}

void dAnimAcyclicJoint::UpdateJointAcceleration()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->UpdateJointAcceleration();
	}
}

void dAnimAcyclicJoint::Finalize()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Finalize();
	}
}