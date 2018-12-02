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
#include "dVehicleChassis.h"
#include "dVehicleSingleBody.h"
#include "dVehicleVirtualTire.h"
#include "dVehicleVirtualEngine.h"
#include "dVehicleVirtualDifferential.h"

dVehicleSingleBody::dVehicleSingleBody(dVehicleChassis* const chassis)
	:dVehicleInterface(chassis)
	,m_gravity(0.0f)
	,m_groundNode(NULL)
	,m_newtonBody(chassis->GetBody())
{
	dVector tmp;
	dComplementaritySolver::dBodyState* const chassisBody = GetBody();
	m_groundNode.SetWorld(m_world);
	m_groundNode.SetLoopNode(true);
	
	// set the inertia matrix;
	NewtonBodyGetMass(m_newtonBody, &tmp.m_w, &tmp.m_x, &tmp.m_y, &tmp.m_z);
	chassisBody->SetMass(tmp.m_w);
	chassisBody->SetInertia(tmp.m_x, tmp.m_y, tmp.m_z);

	dMatrix matrix (dGetIdentityMatrix());
	NewtonBodyGetCentreOfMass(m_newtonBody, &matrix.m_posit[0]);
	matrix.m_posit.m_w = 1.0f;
	chassisBody->SetLocalMatrix(matrix);
}

dVehicleSingleBody::~dVehicleSingleBody()
{
}

dVehicleTireInterface* dVehicleSingleBody::AddTire (const dMatrix& locationInGlobalSpace, const dVehicleTireInterface::dTireInfo& tireInfo, const dMatrix& localFrame)
{
	return new dVehicleVirtualTire(this, locationInGlobalSpace, tireInfo, localFrame);
}

dVehicleDifferentialInterface* dVehicleSingleBody::AddDifferential(dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire)
{
	return new dVehicleVirtualDifferential(this, leftTire, rightTire);
}

dVehicleEngineInterface* dVehicleSingleBody::AddEngine(const dVehicleEngineInterface::dEngineInfo& engineInfo, dVehicleDifferentialInterface* const differential)
{
	return new dVehicleVirtualEngine(this, engineInfo, differential);
}

dMatrix dVehicleSingleBody::GetMatrix () const
{
	return m_body.GetMatrix();
}

void dVehicleSingleBody::CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	NewtonCollision* const collision = NewtonBodyGetCollision(m_newtonBody);
	CalculateAABB(collision, matrix, minP, maxP);
}

void dVehicleSingleBody::RigidBodyToStates()
{
	dVector vector;
	dMatrix matrix;
	dComplementaritySolver::dBodyState* const chassisBody = GetBody();

	// get data from engine rigid body and copied to the vehicle chassis body
	NewtonBodyGetMatrix(m_newtonBody, &matrix[0][0]);
	chassisBody->SetMatrix(matrix);

	NewtonBodyGetVelocity(m_newtonBody, &vector[0]);
	chassisBody->SetVeloc(vector);

	NewtonBodyGetOmega(m_newtonBody, &vector[0]);
	chassisBody->SetOmega(vector);

	NewtonBodyGetForce(m_newtonBody, &vector[0]);
	chassisBody->SetForce(vector);

	NewtonBodyGetTorque(m_newtonBody, &vector[0]);
	chassisBody->SetTorque(vector);

	chassisBody->UpdateInertia();
	
	dVehicleInterface::RigidBodyToStates();
}

void dVehicleSingleBody::StatesToRigidBody(dFloat timestep)
{
	dComplementaritySolver::dBodyState* const chassisBody = GetBody();

	dVector force(chassisBody->GetForce());
	dVector torque(chassisBody->GetTorque());
	NewtonBodySetForce(m_newtonBody, &force[0]);
	NewtonBodySetTorque(m_newtonBody, &torque[0]);

	dVehicleInterface::StatesToRigidBody(timestep);
}

int dVehicleSingleBody::GetKinematicLoops(dKinematicLoopJoint** const jointArray)
{
	m_groundNode.SetIndex(-1);
	return dVehicleInterface::GetKinematicLoops(jointArray);
}

void dVehicleSingleBody::ApplyExternalForce()
{
	dVector force(0.0f);
	dVector torque;
	
	dComplementaritySolver::dBodyState* const chassisBody = GetBody();
	dComplementaritySolver::dBodyState* const groundBody = m_groundNode.GetBody();

	groundBody->SetForce(force);
	groundBody->SetTorque(force);

	NewtonBodyGetForce(m_newtonBody, &force[0]);
	chassisBody->SetForce(force);

	NewtonBodyGetTorque(m_newtonBody, &torque[0]);
	chassisBody->SetTorque(torque);

	//const dVector& updir = chassisBody->GetMatrix().m_up;
	//m_gravity = updir.Scale (chassisBody->GetInvMass() * updir.DotProduct3(force));
	m_gravity = force.Scale(chassisBody->GetInvMass());
	dVehicleInterface::ApplyExternalForce();
}

void dVehicleSingleBody::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dVehicleInterface::Debug(debugContext);

	const dMatrix& matrix = m_body.GetMatrix();

	// draw velocity
	dVector veloc (m_body.GetVelocity());
	veloc = veloc - matrix.m_up.Scale (veloc.DotProduct3(matrix.m_up));
	dFloat mag = dSqrt (veloc.DotProduct3(veloc));
	veloc = veloc.Scale (dLog (mag) / (mag + 0.1f));

	debugContext->SetColor(dVector(1.0f, 1.0f, 0.0f, 1.0f));
	dVector p0 (matrix.m_posit + matrix.m_up.Scale (1.0f));
	dVector p1 (p0 + veloc);
	debugContext->DrawLine(p0, p1);
	
	debugContext->SetColor(dVector(0.5f, 0.5f, 0.5f, 1.0f));
	dVector p2(p0 + matrix.m_front.Scale(2.0f));
	debugContext->DrawLine(p0, p2);

	//draw vehicle weight
	if (m_gravity.DotProduct3(m_gravity) > 0.1) {
		// for now weight is normalize to 2.0
		debugContext->SetColor(dVector(0.0f, 0.0f, 1.0f, 1.0f));
		dVector gravity (m_gravity.Normalize());
		dVector p3(p0 - gravity.Scale (2.0f));
		debugContext->DrawLine(p0, p3);
	}
}
