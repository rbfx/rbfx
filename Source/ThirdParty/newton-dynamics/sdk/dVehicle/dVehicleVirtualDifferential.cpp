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
#include "dVehicleVirtualDifferential.h"


dVehicleVirtualDifferential::dVehicleVirtualDifferential(dVehicleNode* const parent, dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire)
	:dVehicleDifferentialInterface(parent)
	,m_differential()
	,m_leftAxle()
	,m_rightAxle()
	,m_leftTire(leftTire)
	,m_rightTire(rightTire)
	,m_diffOmega(0.0f)
	,m_shaftOmega(0.0f)
{
	dAssert(0);
/*
	SetWorld(parent->GetWorld());

	dFloat mass = 0.75f * 0.5f * (m_leftTire->GetInfo().m_mass + m_rightTire->GetInfo().m_mass);
	dFloat radius = m_leftTire->GetInfo().m_radio * 0.5f;
	dFloat inertia = 0.7f * mass * radius * radius;

	m_proxyBody.SetMass(mass);
	m_proxyBody.SetInertia(inertia, inertia, inertia);
	m_proxyBody.UpdateInertia();

	// set the tire joint
	m_differential.Init(&m_proxyBody, m_parent->GetProxyBody());
	m_leftAxle.Init(&m_proxyBody, m_leftTire->GetProxyBody());
	m_rightAxle.Init(&m_proxyBody, m_rightTire->GetProxyBody());

	m_leftAxle.SetOwners(this, m_leftTire);
	m_rightAxle.SetOwners(this, m_rightTire);

	m_leftAxle.m_diffSign = -1.0f;
	m_rightAxle.m_diffSign = 1.0f;
*/
}

dVehicleVirtualDifferential::~dVehicleVirtualDifferential()
{
}

dComplementaritySolver::dBilateralJoint* dVehicleVirtualDifferential::GetProxyJoint()
{
	return &m_differential;
}

void dVehicleVirtualDifferential::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dAssert(0);
//	dVehicleDifferentialInterface::Debug(debugContext);
}

int dVehicleVirtualDifferential::GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray)
{
	dAssert(0);
	return 0;
/*
	jointArray[0] = &m_leftAxle;
	jointArray[1] = &m_rightAxle;
	return dVehicleDifferentialInterface::GetKinematicLoops(&jointArray[2]) + 2;
*/
}

void dVehicleVirtualDifferential::ApplyExternalForce(dFloat timestep)
{
	dAssert(0);
/*
	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetProxyBody();

	dMatrix matrix(chassisBody->GetMatrix());
	matrix.m_posit = matrix.TransformVector(chassisBody->GetCOM());
	m_proxyBody.SetMatrix(matrix);

	m_proxyBody.SetVeloc(chassisBody->GetVelocity());
	m_proxyBody.SetOmega(chassisBody->GetOmega() + matrix.m_right.Scale (m_shaftOmega) + matrix.m_up.Scale (m_diffOmega));
	m_proxyBody.SetTorque(dVector(0.0f));
	m_proxyBody.SetForce(chassisNode->m_gravity.Scale(m_proxyBody.GetMass()));

	dVehicleDifferentialInterface::ApplyExternalForce(timestep);
*/
}

void dVehicleVirtualDifferential::Integrate(dFloat timestep)
{
	dAssert(0);
/*
	dVehicleDifferentialInterface::Integrate(timestep);

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassis->GetProxyBody();

	const dMatrix chassisMatrix(chassisBody->GetMatrix());

	dVector omega(m_proxyBody.GetOmega());
	dVector chassisOmega(chassisBody->GetOmega());
	dVector localOmega(omega - chassisOmega);
	m_diffOmega = chassisMatrix.m_up.DotProduct3(localOmega);
	m_shaftOmega = chassisMatrix.m_right.DotProduct3(localOmega);
*/
}
