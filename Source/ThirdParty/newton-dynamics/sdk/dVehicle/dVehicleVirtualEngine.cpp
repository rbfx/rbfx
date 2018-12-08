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
#include "dVehicleVirtualEngine.h"

dVehicleVirtualEngine::dEngineMetricInfo::dEngineMetricInfo(const dEngineInfo& info)
	:dEngineInfo(info)
{
	const dFloat horsePowerToWatts = 735.5f;
	const dFloat rpmToRadiansPerSecunds = 0.105f;
	const dFloat poundFootToNewtonMeters = 1.356f;

	m_clutchTorque *= poundFootToNewtonMeters;
	m_idleTorque *= poundFootToNewtonMeters;
	m_peakTorque *= poundFootToNewtonMeters;

	m_rpmAtPeakTorque *= rpmToRadiansPerSecunds;
	m_rpmAtPeakHorsePower *= rpmToRadiansPerSecunds;
	m_rpmAtRedLine *= rpmToRadiansPerSecunds;
	m_rpmAtIdleTorque *= rpmToRadiansPerSecunds;

	m_peakHorsePower *= horsePowerToWatts;
	m_peakPowerTorque = m_peakHorsePower / m_rpmAtPeakHorsePower;

	dAssert(m_rpmAtIdleTorque > 0.0f);
	dAssert(m_rpmAtIdleTorque < m_rpmAtPeakHorsePower);
	dAssert(m_rpmAtPeakTorque < m_rpmAtPeakHorsePower);
	dAssert(m_rpmAtPeakHorsePower < m_rpmAtRedLine);

	dAssert(m_idleTorque > 0.0f);
	dAssert(m_peakTorque > m_peakPowerTorque);
	dAssert((m_peakTorque * m_rpmAtPeakTorque) < m_peakHorsePower);
}

dFloat dVehicleVirtualEngine::dEngineMetricInfo::GetTorque (dFloat rpm) const
{
	dAssert(rpm >= -0.1f);
	const int maxIndex = sizeof (m_torqueCurve) / sizeof (m_torqueCurve[0]);
	rpm = dClamp(rpm, dFloat(0.0f), m_torqueCurve[maxIndex - 1].m_rpm);

	for (int i = 1; i < maxIndex; i++) {
		if (rpm <= m_torqueCurve[i].m_rpm) {
			dFloat rpm1 = m_torqueCurve[i].m_rpm;
			dFloat rpm0 = m_torqueCurve[i - 1].m_rpm;

			dFloat torque1 = m_torqueCurve[i].m_torque;
			dFloat torque0 = m_torqueCurve[i - 1].m_torque;

			dFloat torque = torque0 + (rpm - rpm0) * (torque1 - torque0) / (rpm1 - rpm0);
			return torque;
		}
	}

	return m_torqueCurve[maxIndex - 1].m_torque;
}

dVehicleVirtualEngine::dVehicleVirtualEngine(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential)
	:dVehicleEngineInterface(parent, info, differential)
	,m_metricInfo(info)
	,m_blockJoint()
	,m_crankJoint()
	,m_gearBox()
	,m_omega(0.0f)
{
	SetWorld(parent->GetWorld());

	dFloat inertia = 0.7f * m_info.m_mass * m_info.m_armatureRadius * m_info.m_armatureRadius;
	m_proxyBody.SetMass(m_info.m_mass);
	m_proxyBody.SetInertia(inertia, inertia, inertia);
	m_proxyBody.UpdateInertia();

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*) ((dVehicleNode*)m_parent)->GetAsVehicle();
	// set the tire joint
	m_blockJoint.Init(&m_proxyBody, chassis->GetProxyBody());

	m_gearBox.Init(&m_proxyBody, differential->GetProxyBody());
	m_gearBox.SetOwners(this, differential);

	m_crankJoint.Init(&m_proxyBody, chassis->m_groundNode.GetProxyBody());
	m_crankJoint.SetOwners(this, &chassis->m_groundNode);

	SetInfo(info);
}

dVehicleVirtualEngine::~dVehicleVirtualEngine()
{
}

void dVehicleVirtualEngine::InitEngineTorqueCurve()
{
	m_metricInfo = dEngineMetricInfo(m_info);

	m_metricInfo.m_torqueCurve[0] = dEngineTorqueNode(0.0f, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[1] = dEngineTorqueNode(m_metricInfo.m_rpmAtIdleTorque, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[2] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakTorque, m_metricInfo.m_peakTorque);
	m_metricInfo.m_torqueCurve[3] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakHorsePower, m_metricInfo.m_peakPowerTorque);
	m_metricInfo.m_torqueCurve[4] = dEngineTorqueNode(m_metricInfo.m_rpmAtRedLine, m_metricInfo.m_idleTorque);
}

void dVehicleVirtualEngine::SetInfo(const dEngineInfo& info)
{
	dVehicleEngineInterface::SetInfo(info);
	InitEngineTorqueCurve();

//	m_controller->SetAerodynamicsDownforceCoefficient(info.m_aerodynamicDownforceFactor, info.m_aerodynamicDownForceSurfaceCoeficident, info.m_aerodynamicDownforceFactorAtTopSpeed);
}

dComplementaritySolver::dBilateralJoint* dVehicleVirtualEngine::GetProxyJoint()
{
	return &m_blockJoint;
}

void dVehicleVirtualEngine::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dVehicleEngineInterface::Debug(debugContext);
}

dFloat dVehicleVirtualEngine::GetRpm() const
{
	return -m_omega * 9.549f;
}

dFloat dVehicleVirtualEngine::GetRedLineRpm() const
{
	return m_metricInfo.m_rpmAtRedLine * 9.549f;
}

void dVehicleVirtualEngine::SetThrottle (dFloat throttle)
{
	dFloat torque = m_metricInfo.GetTorque(dAbs (m_omega));
	dFloat omega = m_metricInfo.m_rpmAtRedLine * dClamp(throttle, dFloat (0.0f), dFloat (1.0f));
	m_crankJoint.SetTorqueAndRpm(torque, omega);
}

void dVehicleVirtualEngine::SetGear (int gear)
{
	gear = dClamp (gear, int (m_reverseGear), m_metricInfo.m_gearsCount);
	dFloat ratio = m_metricInfo.m_gearRatios[gear];
	m_gearBox.SetGearRatio(ratio);
}

void dVehicleVirtualEngine::SetClutch (dFloat clutch) 
{
	clutch = dClamp (clutch, dFloat (0.0f), dFloat (1.0f));
	m_gearBox.SetClutchTorque(clutch * m_metricInfo.m_clutchTorque);
}

int dVehicleVirtualEngine::GetKinematicLoops(dAnimationKinematicLoopJoint** const jointArray)
{
	jointArray[0] = &m_crankJoint;
	jointArray[1] = &m_gearBox;

	int count = 2;
	return dVehicleEngineInterface::GetKinematicLoops(&jointArray[count]) + count;
}

void dVehicleVirtualEngine::ApplyExternalForce(dFloat timestep)
{
	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetProxyBody();

	dMatrix matrix(chassisBody->GetMatrix());
	matrix.m_posit = matrix.TransformVector(chassisBody->GetCOM());
	m_proxyBody.SetMatrix(matrix);

	m_proxyBody.SetVeloc(chassisBody->GetVelocity());
	m_proxyBody.SetOmega(chassisBody->GetOmega() + matrix.m_right.Scale (m_omega));
	m_proxyBody.SetTorque(dVector(0.0f));
	m_proxyBody.SetForce(chassisNode->m_gravity.Scale(m_proxyBody.GetMass()));

	dVehicleEngineInterface::ApplyExternalForce(timestep);
}

void dVehicleVirtualEngine::Integrate(dFloat timestep)
{
	dVehicleEngineInterface::Integrate(timestep);

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassis->GetProxyBody();

	const dMatrix chassisMatrix(chassisBody->GetMatrix());

	dVector omega(m_proxyBody.GetOmega());
	dVector chassisOmega(chassisBody->GetOmega());
	dVector localOmega(omega - chassisOmega);
	m_omega = chassisMatrix.m_right.DotProduct3(localOmega);

//dTrace (("eng(%f)\n", m_omega));
}
