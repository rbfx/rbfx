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
	const dFloat kmhToMetersPerSecunds = 0.278f;
	const dFloat rpmToRadiansPerSecunds = 0.105f;
	const dFloat poundFootToNewtonMeters = 1.356f;

	m_idleTorque *= poundFootToNewtonMeters;
	m_peakTorque *= poundFootToNewtonMeters;

	m_rpmAtPeakTorque *= rpmToRadiansPerSecunds;
	m_rpmAtPeakHorsePower *= rpmToRadiansPerSecunds;
	m_rpmAtRedLine *= rpmToRadiansPerSecunds;
	m_rpmAtIdleTorque *= rpmToRadiansPerSecunds;

	m_peakHorsePower *= horsePowerToWatts;
//	m_vehicleTopSpeed *= kmhToMetersPerSecunds;
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
	const int maxIndex = sizeof (m_torqueCurve) / sizeof (m_torqueCurve[0]) - 1;
	rpm = dClamp(rpm, dFloat(0.0f), m_torqueCurve[maxIndex - 1].m_rpm);

	for (int i = 1; i < maxIndex; i++) {
		if (m_torqueCurve[i].m_rpm >= rpm) {
			dFloat rpm0 = m_torqueCurve[i - 1].m_rpm;
			dFloat rpm1 = m_torqueCurve[i].m_rpm;

			dFloat torque0 = m_torqueCurve[i - 1].m_torque;
			dFloat torque1 = m_torqueCurve[i].m_torque;
			dFloat torque = torque0 + (rpm - rpm0) * (torque1 - torque0) / (rpm1 - rpm0);
			return torque;
		}
	}

	return m_torqueCurve[maxIndex - 1].m_rpm;
}


dVehicleVirtualEngine::dVehicleVirtualEngine(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential)
	:dVehicleEngineInterface(parent, info, differential)
	,m_joint()
	,m_omega(0.0f)
	,m_metricInfo(info)
{
	SetWorld(parent->GetWorld());

	dFloat inertia = 0.7f * m_info.m_mass * m_info.m_armatureRadius * m_info.m_armatureRadius;
	m_body.SetMass(m_info.m_mass);
	m_body.SetInertia(inertia, inertia, inertia);
	m_body.UpdateInertia();

	// set the tire joint
	m_joint.Init(&m_body, m_parent->GetBody());
/*
	m_leftDifferential.Init(&m_body, m_leftTire->GetBody());
	m_rightDifferential.Init(&m_body, m_rightTire->GetBody());

	m_leftDifferential.SetOwners(this, m_leftTire);
	m_rightDifferential.SetOwners(this, m_rightTire);
*/
	SetInfo(info);
}

dVehicleVirtualEngine::~dVehicleVirtualEngine()
{
}

void dVehicleVirtualEngine::InitEngineTorqueCurve()
{
	m_metricInfo = dEngineMetricInfo(m_info);

//	dAssert(m_metricInfo.m_vehicleTopSpeed >= 0.0f);
//	dAssert(m_metricInfo.m_vehicleTopSpeed < 100.0f);
//	m_metricInfo.m_crownGearRatio = 1.0f;
//	dWheelJoint* const tire = m_crownGearCalculator;
//	dAssert(tire);

	// drive train geometrical relations
	// G0 = m_differentialGearRatio
	// G1 = m_transmissionGearRatio
	// s = topSpeedMPS
	// r = tireRadio
	// wt = rpsAtTire
	// we = rpsAtPickPower
	// we = G1 * G0 * wt;
	// wt = e / r
	// we = G0 * G1 * s / r
	// G0 = r * we / (G1 * s)
	// using the top gear and the optimal engine torque for the calculations
//	dFloat topGearRatio = GetTopGear();
//	dFloat tireRadio = tire->m_radio;
//	m_metricInfo.m_crownGearRatio = tireRadio * m_metricInfo.m_rpmAtPeakHorsePower / (m_metricInfo.m_vehicleTopSpeed * topGearRatio);

	// bake crown gear with the engine power curve
	//m_metricInfo.m_idleTorque *= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_peakTorque *= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_peakPowerTorque *= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_rpmAtIdleTorque /= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_rpmAtPeakTorque /= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_rpmAtPeakHorsePower /= m_metricInfo.m_crownGearRatio;
	//m_metricInfo.m_rpmAtRedLine /= m_metricInfo.m_crownGearRatio;

	m_metricInfo.m_torqueCurve[0] = dEngineTorqueNode(0.0f, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[1] = dEngineTorqueNode(m_metricInfo.m_rpmAtIdleTorque, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[2] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakTorque, m_metricInfo.m_peakTorque);
	m_metricInfo.m_torqueCurve[3] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakHorsePower, m_metricInfo.m_peakPowerTorque);
	m_metricInfo.m_torqueCurve[4] = dEngineTorqueNode(m_metricInfo.m_rpmAtRedLine, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[5] = dEngineTorqueNode(m_metricInfo.m_rpmAtRedLine, m_metricInfo.m_idleTorque);
}


void dVehicleVirtualEngine::SetInfo(const dEngineInfo& info)
{
	dVehicleEngineInterface::SetInfo(info);

//	m_metricInfoCopy = info;
//	m_metricInfo.m_clutchFrictionTorque = dMax(dFloat(10.0f), dAbs(m_metricInfo.m_clutchFrictionTorque));
//	m_metricInfoCopy.m_clutchFrictionTorque = m_metricInfo.m_clutchFrictionTorque;
	InitEngineTorqueCurve();
/*
	dAssert(info.m_gearsCount < (int(sizeof(m_metricInfo.m_gearRatios) / sizeof(m_metricInfo.m_gearRatios[0])) - D_VEHICLE_FIRST_GEAR));
	m_metricInfo.m_gearsCount = info.m_gearsCount + D_VEHICLE_FIRST_GEAR;

	m_metricInfo.m_gearRatios[D_VEHICLE_NEUTRAL_GEAR] = 0.0f;
	m_metricInfo.m_gearRatios[D_VEHICLE_REVERSE_GEAR] = -dAbs(info.m_reverseGearRatio);
	for (int i = 0; i < (m_metricInfo.m_gearsCount - D_VEHICLE_FIRST_GEAR); i++) {
		m_metricInfo.m_gearRatios[i + D_VEHICLE_FIRST_GEAR] = dAbs(info.m_gearRatios[i]);
	}

	m_controller->SetAerodynamicsDownforceCoefficient(info.m_aerodynamicDownforceFactor, info.m_aerodynamicDownForceSurfaceCoeficident, info.m_aerodynamicDownforceFactorAtTopSpeed);
*/
}

dComplementaritySolver::dBilateralJoint* dVehicleVirtualEngine::GetJoint()
{
	return &m_joint;
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
	m_joint.SetTorqueAndRpm(torque, omega);
}

void dVehicleVirtualEngine::ApplyExternalForce()
{
	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetBody();

	const dMatrix& matrix(chassisBody->GetMatrix());
	m_body.SetMatrix(matrix);

	m_body.SetVeloc(chassisBody->GetVelocity());
	m_body.SetOmega(chassisBody->GetOmega() + matrix.m_right.Scale (m_omega));
	m_body.SetTorque(dVector(0.0f));
	m_body.SetForce(chassisNode->m_gravity.Scale(m_body.GetMass()));

	dVehicleEngineInterface::ApplyExternalForce();
}

void dVehicleVirtualEngine::Integrate(dFloat timestep)
{
	dVehicleEngineInterface::Integrate(timestep);

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassis->GetBody();

	const dMatrix chassisMatrix(chassisBody->GetMatrix());

	dVector omega(m_body.GetOmega());
	dVector chassisOmega(chassisBody->GetOmega());
	dVector localOmega(omega - chassisOmega);
	m_omega = chassisMatrix.m_right.DotProduct3(localOmega);
}
