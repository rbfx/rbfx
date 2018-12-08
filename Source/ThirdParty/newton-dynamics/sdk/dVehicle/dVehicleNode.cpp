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

dVehicleNode::dVehicleNode(dVehicleNode* const parent)
	:dAnimationAcyclicJoint(parent)
{
}

dVehicleNode::~dVehicleNode()
{
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


void dVehicleNode::RigidBodyToStates()
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = (dVehicleNode*)child->GetInfo();
		node->RigidBodyToStates();
	}
}

void dVehicleNode::StatesToRigidBody(dFloat timestep)
{
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = (dVehicleNode*)child->GetInfo();
		node->StatesToRigidBody(timestep);
	}
}

void dVehicleNode::Integrate(dFloat timestep)
{
	m_proxyBody.IntegrateForce(timestep, m_proxyBody.GetForce(), m_proxyBody.GetTorque());
	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = (dVehicleNode*)child->GetInfo();
		node->Integrate(timestep);
	}
}
