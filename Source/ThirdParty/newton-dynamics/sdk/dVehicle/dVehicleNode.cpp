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


#include "dStdafxVehicle.h"
#include "dVehicleNode.h"

dVehicleNode::dVehicleNode(dVehicleNode* const parent, bool isLoop)
	:dContainersAlloc()
	,m_userData(NULL)
	,m_world(NULL)
	,m_parent(parent)
	,m_body()
	,m_solverIndex(-1)
	,m_isLoop(isLoop)
{
	if (parent) {
		parent->m_children.Append(this);
	}
}

dVehicleNode::~dVehicleNode()
{
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		delete child->GetInfo();
	}
}

void* dVehicleNode::GetUserData()
{
	return m_userData;
}

void dVehicleNode::SetUserData(void* const userData)
{
	m_userData = userData;
}


void dVehicleNode::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Debug(debugContext);
	}
}

void dVehicleNode::CalculateAABB(const NewtonCollision* const collision, const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	for (int i = 0; i < 3; i++) {
		dVector support(0.0f);
		dVector dir(0.0f);
		dir[i] = 1.0f;

		dVector localDir(matrix.UnrotateVector(dir));
		NewtonCollisionSupportVertex(collision, &localDir[0], &support[0]);
		support = matrix.TransformVector(support);
		maxP[i] = support[i];

		localDir = localDir.Scale(-1.0f);
		NewtonCollisionSupportVertex(collision, &localDir[0], &support[0]);
		support = matrix.TransformVector(support);
		minP[i] = support[i];
	}
}

void dVehicleNode::CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	minP = matrix.m_posit;
	maxP = matrix.m_posit;
}

void dVehicleNode::ApplyExternalForce()
{
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->ApplyExternalForce();
	}
}

void dVehicleNode::RigidBodyToStates()
{
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->RigidBodyToStates();
	}
}

void dVehicleNode::StatesToRigidBody(dFloat timestep)
{
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->StatesToRigidBody(timestep);
	}
}

void dVehicleNode::Integrate(dFloat timestep)
{
	m_body.IntegrateForce(timestep, m_body.GetForce(), m_body.GetTorque());
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Integrate(timestep);
	}
}

int dVehicleNode::GetKinematicLoops(dKinematicLoopJoint** const jointArray)
{
	int count = 0;
	for (dList<dVehicleNode*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		count += child->GetInfo()->GetKinematicLoops(&jointArray[count]);
	}
	return count;
}