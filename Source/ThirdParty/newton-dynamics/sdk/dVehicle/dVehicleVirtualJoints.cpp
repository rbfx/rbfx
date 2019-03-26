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
#include "dVehicleChassis.h"
#include "dVehicleSingleBody.h"
#include "dVehicleVirtualTire.h"
#include "dVehicleVirtualEngine.h"
#include "dVehicleVirtualJoints.h"

#define D_VEHICLE_STOP_TORQUE	10000.0f

// *******************************************************************
// tire
// *******************************************************************
dTireJoint::dTireJoint()
	:dComplementaritySolver::dBilateralJoint()
{
}

void dTireJoint::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dComplementaritySolver::dBodyState* const tire = m_state0;
	dComplementaritySolver::dBodyState* const chassis = m_state1;

	const dVector& omega = chassis->GetOmega();
	const dMatrix& tireMatrix = tire->GetMatrix();

	// lateral force
	AddLinearRowJacobian(constraintParams, tireMatrix.m_posit, tireMatrix.m_front, omega);

	// longitudinal force
	AddLinearRowJacobian(constraintParams, tireMatrix.m_posit, tireMatrix.m_up, omega);

	// angular constraints	
	AddAngularRowJacobian(constraintParams, tireMatrix.m_up, omega, 0.0f);
	AddAngularRowJacobian(constraintParams, tireMatrix.m_right, omega, 0.0f);

	// dry rolling friction (for now contact, but it should be a function of the tire angular velocity)
	//int index = constraintParams->m_count;
	//AddAngularRowJacobian(constraintParams, tire->m_matrix[0], 0.0f);
	//constraintParams->m_jointLowFriction[index] = -chassis->m_dryRollingFrictionTorque;
	//constraintParams->m_jointHighFriction[index] = chassis->m_dryRollingFrictionTorque;

	dFloat brakeTorque = m_tire->GetBrakeTorque();
//brakeTorque = 1000;
	if (brakeTorque > 1.0e-3f) {

		int index = constraintParams->m_count;
		AddAngularRowJacobian(constraintParams, tireMatrix.m_front, omega, 0.0f);

		const dVector& omega0 = m_state0->GetOmega();
		const dVector& omega1 = m_state1->GetOmega();
		const dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[index].m_jacobian_J01;
		const dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[index].m_jacobian_J10;
		const dVector relVeloc(omega0 * jacobian0.m_angular + omega1 * jacobian1.m_angular);
		dFloat relOmega = -(relVeloc.m_x + relVeloc.m_y + relVeloc.m_z);
		constraintParams->m_jointAccel[index] = relOmega * constraintParams->m_timestepInv;
		constraintParams->m_jointLowFrictionCoef[index] = -brakeTorque;
		constraintParams->m_jointHighFrictionCoef[index] = brakeTorque;
	}
	m_tire->SetBrakeTorque(0.0f);
}

// *******************************************************************
// differential attachment to chassis
// *******************************************************************
dDifferentialMount::dDifferentialMount()
	:dComplementaritySolver::dBilateralJoint()
	,m_slipeOn(false)
{
}

void dDifferentialMount::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dComplementaritySolver::dBodyState* const chassis = m_state1;
	dComplementaritySolver::dBodyState* const differential = m_state0;

	const dVector& omega = chassis->GetOmega();
	const dMatrix& matrix = differential->GetMatrix();

	// three rigid attachment to chassis
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_front, omega);
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_up, omega);
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_right, omega);

	// angular constraints	
	AddAngularRowJacobian(constraintParams, matrix.m_front, omega, 0.0f);
	if (m_slipeOn) {
		dAssert (0);
	}
}

// *******************************************************************
// tire contacts
// *******************************************************************
dTireContact::dTireContact()
//	:dAnimIDRigKinematicLoopJoint()
	:m_point(0.0f)
	,m_normal(0.0f)
	,m_lateralDir(0.0f)
	,m_longitudinalDir(0.0f)
	,m_penetration(0.0f)
	,m_staticFriction(1.0f)
	,m_kineticFriction(1.0f)
	,m_load(0.0f)
	,m_tireModel()
{
	dAssert(0);
/*
	m_jointFeebackForce[0] = 0.0f;
	m_jointFeebackForce[1] = 0.0f;
	m_jointFeebackForce[2] = 0.0f;
	memset(m_normalFilter, 0, sizeof(m_normalFilter));
	memset(m_isActiveFilter, 0, sizeof(m_isActiveFilter));
*/
}

void dTireContact::ResetContact ()
{
	dAssert(0);
/*
	if (m_isActive == false) {
		m_jointFeebackForce[0] = 0.0f;
		m_jointFeebackForce[1] = 0.0f;
		m_jointFeebackForce[2] = 0.0f;
		memset(m_normalFilter, 0, sizeof(m_normalFilter));
		memset(m_isActiveFilter, 0, sizeof(m_isActiveFilter));
	}
	m_load = 0.0f;
	m_isActive = false;
	for (int i = sizeof(m_isActiveFilter) / sizeof(m_isActiveFilter[0]) - 1; i > 0; i--) {
		m_normalFilter[i] = m_normalFilter[i - 1];
		m_isActiveFilter[i] = m_isActiveFilter[i - 1];
	}
*/
}

void dTireContact::SetContact(const dVector& posit, const dVector& normal, const dVector& longitudinalDir, dFloat penetration, dFloat staticFriction, dFloat kineticFriction)
{
	dAssert(0);
/*
	m_point = posit;
	m_normal = normal;
	m_longitudinalDir = longitudinalDir;
	m_lateralDir = m_longitudinalDir.CrossProduct(m_normal);

	m_isActive = true;
	m_isActiveFilter[0] = true;
	m_normalFilter[0] = m_jointFeebackForce[0];

	dFloat load = 0.0f;
	for (int i = 0; i < sizeof(m_isActiveFilter) / sizeof(m_isActiveFilter[0]); i++) {
		load += m_normalFilter[i];
	}
	m_load = load * (1.0f / (sizeof(m_isActiveFilter) / sizeof(m_isActiveFilter[0])));

	m_staticFriction = staticFriction;
	m_kineticFriction = kineticFriction;
	m_penetration = dClamp (penetration, dFloat(-D_TIRE_MAX_ELASTIC_DEFORMATION), dFloat(D_TIRE_MAX_ELASTIC_DEFORMATION));
*/
}

void dTireContact::TireForces(dFloat longitudinalSlip, dFloat lateralSlip, dFloat frictionCoef)
{
	dAssert(0);
/*
	dVehicleTireInterface* const tire = ((dVehicleNode*)GetOwner0())->GetAsTire();
	dAssert (tire);

	const dVehicleTireInterface::dTireInfo& tireInfo = tire->GetInfo();

	dFloat v = dAbs (lateralSlip);
	dFloat u = dAbs (longitudinalSlip);
	dFloat f = dMax (frictionCoef * m_load, dFloat (1.0f));

	dFloat invden = 1.0f / (1.0f + u);

	dFloat y = tireInfo.m_corneringStiffness * v * invden;
	dFloat x = tireInfo.m_longitudinalStiffness * u * invden;
	dFloat g = dSqrt (x * x + y * y);

	dFloat r = g / f;
	if (g < (3.0f * f)) {
		f = g * (1.0f - (1.0f / 3.0f) * r  + (1.0f / 27.0f) * r * r);
	}
	r = f / (g + 1.0e-3f);
	m_tireModel.m_longitodinalSlip = u * invden;
	m_tireModel.m_lateralSlip = v * invden * (1.0f / D_TIRE_MAX_LATERAL_SLIP);
	m_tireModel.m_alingMoment = 0.0f;
	m_tireModel.m_lateralForce = y * r;
	m_tireModel.m_longitunalForce = x * r;
	dAssert(m_tireModel.m_lateralForce >= 0.0f);
	dAssert(m_tireModel.m_longitunalForce >= 0.0f);
*/
}

void dTireContact::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
/*
FILE* file_xxx = fopen("xxxxx.csv", "wb");
fprintf(file_xxx, "tireforce\n");
m_load = 1000.0f;
for (int i = 0; i < 100; i++) {
	float u = 0.01f * i;
	TireForces(0.0f, u, 1.0f);
	//fprintf(file_xxx, "%f,\n", m_tireModel.m_longitunalForce);
	fprintf(file_xxx, "%f,\n", m_tireModel.m_lateralForce);
}
fclose(file_xxx);
*/


	dAssert(0);
/*
	dVector omega(0.0f);

	// normal constraint
	const dVector& veloc0 = m_state0->GetVelocity();
	const dVector& omega0 = m_state0->GetOmega();
	const dVector& veloc1 = m_state1->GetVelocity();
	const dVector& omega1 = m_state1->GetOmega();

	int n = 0;
	{
		// normal force row
		AddLinearRowJacobian(constraintParams, m_point, m_normal, omega);
		const dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[n].m_jacobian_J01;
		const dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[n].m_jacobian_J10;
		const dVector relVeloc(veloc0 * jacobian0.m_linear + omega0 * jacobian0.m_angular + veloc1 * jacobian1.m_linear + omega1 * jacobian1.m_angular);
		dFloat relSpeed = -(relVeloc.m_x + relVeloc.m_y + relVeloc.m_z);
		relSpeed += D_TIRE_MAX_ELASTIC_NORMAL_STIFFNESS * m_penetration;
		constraintParams->m_jointLowFrictionCoef[n] = 0.0f;
		constraintParams->m_jointAccel[n] = relSpeed * constraintParams->m_timestepInv;
		n++;
	}

	int lateralIndex = 0;
	int longitudinaIndex = 0;
	dFloat lateralSlip = 0.4f;
	dFloat longitudialSlip = 0.4f;
	dFloat frictionCoef = m_staticFriction;
	{
		// longitudinal force row
		AddLinearRowJacobian(constraintParams, m_point, m_longitudinalDir, omega);
		const dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[n].m_jacobian_J01;
		const dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[n].m_jacobian_J10;

		dVector linearVeloc(veloc0 * jacobian0.m_linear + veloc1 * jacobian1.m_linear + omega1 * jacobian1.m_angular);
		dFloat omegaSpeed = omega0.DotProduct3(jacobian0.m_angular);
		dFloat linearSpeed = linearVeloc.m_x + linearVeloc.m_y + linearVeloc.m_z;
		dFloat relSpeed = omegaSpeed + linearSpeed;

		omegaSpeed = dAbs (omegaSpeed);
		linearSpeed = dAbs (linearSpeed);
		if (!((omegaSpeed < 0.2f) && (linearSpeed < 0.2f))) {
			if (relSpeed < 0.0f) {
				dFloat speedDen = dMax (linearSpeed, dFloat(0.01f));
				longitudialSlip = dClamp(dAbs (relSpeed / speedDen), dFloat(0.0f), dFloat(20.0f));
				dFloat slipLimit = 2.0f * dMax (linearSpeed, dFloat(0.0f));
				if ((omegaSpeed - slipLimit) > 0.0f) {
					frictionCoef = m_kineticFriction;
				}
			} else {
				dFloat speedDen = dMax(omegaSpeed, dFloat(0.01f));
				longitudialSlip = dClamp (dAbs (relSpeed / speedDen), dFloat(0.0f), dFloat(4.0f));
				dFloat slipLimit = 2.0f * dMax(omegaSpeed, dFloat(0.0f));
				if ((linearSpeed - slipLimit) > 0.0f) {
					frictionCoef = m_kineticFriction;
				} 
			}
		}

		constraintParams->m_jointAccel[n] = -relSpeed * constraintParams->m_timestepInv;
		longitudinaIndex = n;
		n++;
	}

	{
		// lateral force row
		AddLinearRowJacobian(constraintParams, m_point, m_lateralDir, omega);
		const dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[n].m_jacobian_J01;
		const dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[n].m_jacobian_J10;

		const dVector relVeloc(veloc0 * jacobian0.m_linear + omega0 * jacobian0.m_angular + veloc1 * jacobian1.m_linear + omega1 * jacobian1.m_angular);
		dFloat lateralSpeed = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;
		dFloat longitudinalSpeed = dAbs (m_longitudinalDir.DotProduct3(relVeloc)) + 0.01f;
		lateralSlip = lateralSpeed / longitudinalSpeed;

		dFloat maxLateralSpeed = D_TIRE_MAX_LATERAL_SLIP * longitudinalSpeed;
		if (lateralSpeed > maxLateralSpeed) {
			frictionCoef = m_kineticFriction;
		} else if (lateralSpeed < -maxLateralSpeed) {
			frictionCoef = m_kineticFriction;
		}
		
		constraintParams->m_jointAccel[n] = -lateralSpeed * constraintParams->m_timestepInv;
		lateralIndex = n;
		n ++;
	}

	TireForces(longitudialSlip, lateralSlip, frictionCoef);

	constraintParams->m_jointLowFrictionCoef[lateralIndex] = -m_tireModel.m_lateralForce;
	constraintParams->m_jointHighFrictionCoef[lateralIndex] = m_tireModel.m_lateralForce;
	constraintParams->m_jointLowFrictionCoef[longitudinaIndex] = -m_tireModel.m_longitunalForce;
	constraintParams->m_jointHighFrictionCoef[longitudinaIndex] = m_tireModel.m_longitunalForce;

//dTrace (("(%d = %f) ", GetOwner0()->GetIndex(), longitudialSlip));
//dTrace (("(%d = %f %f %f) ", GetOwner0()->GetIndex(), lateralSlip, m_tireModel.m_lateralForce, m_load));
//dTrace (("(%d = %f %f %f) ", GetOwner0()->GetIndex(), longitudialSlip, m_tireModel.m_longitunalForce, m_load));
//if (GetOwner0()->GetIndex() == 3)
//dTrace (("\n"));

	m_dof = n;
	m_count = n;
	constraintParams->m_count = n;
*/
}

void dTireContact::Debug(dCustomJoint::dDebugDisplay* const debugContext, dFloat scale) const
{
	dAssert(0);
/*
	dVehicleVirtualTire* const tire = (dVehicleVirtualTire*)((dVehicleNode*)GetOwner0())->GetAsTire();
	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)((dVehicleNode*)GetOwner0()->GetParent())->GetAsVehicle();

	dAssert (tire);
	dAssert (chassis);

	const dMatrix& tireMatrix = tire->GetProxyBody()->GetMatrix();
	const dMatrix& chassisMatrix = chassis->GetProxyBody()->GetMatrix();

	dVector localPosit (chassisMatrix.UntransformVector(tireMatrix.m_posit));
	dVector origin (m_point + m_normal.Scale (1.0f/32.0f)); 
	if (localPosit.m_z > 0.0f) {
		origin += m_lateralDir.Scale (1.0f/4.0f);
	} else {
		origin -= m_lateralDir.Scale (1.0f/4.0f);
	}

	scale *= 4.0f;

	// show tire load
	debugContext->SetColor(dVector(0.0f, 0.0f, 1.0f, 1.0f));
	dVector p1(origin + m_normal.Scale (scale * m_jointFeebackForce[0]));
	debugContext->DrawLine(origin, p1);

	scale *= 1.0f;
	// show tire longitudinal force
	debugContext->SetColor(dVector(0.0f, 1.0f, 0.0f, 1.0f));
	dVector p2(origin + m_longitudinalDir.Scale(scale * m_jointFeebackForce[1]));
	debugContext->DrawLine(origin, p2);

	// show tire lateral force
	debugContext->SetColor(dVector(1.0f, 0.0f, 0.0f, 1.0f));
	dVector p3(origin + m_lateralDir.Scale(scale * m_jointFeebackForce[2]));

	debugContext->DrawLine(origin, p3);
*/
}

// *******************************************************************
// engine block 
// *******************************************************************
dEngineBlockJoint::dEngineBlockJoint()
	:dComplementaritySolver::dBilateralJoint()
{
}

void dEngineBlockJoint::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dComplementaritySolver::dBodyState* const engine = m_state0;
	dComplementaritySolver::dBodyState* const chassis = m_state1;

	const dVector& omega = chassis->GetOmega();
	const dMatrix& matrix = engine->GetMatrix();

	// three rigid attachment to chassis
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_front, omega);
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_up, omega);
	AddLinearRowJacobian(constraintParams, matrix.m_posit, matrix.m_right, omega);

	// angular constraints	
	AddAngularRowJacobian(constraintParams, matrix.m_front, omega, 0.0f);
	AddAngularRowJacobian(constraintParams, matrix.m_up, omega, 0.0f);
}

// *******************************************************************
// engine crank
// *******************************************************************
dEngineCrankJoint::dEngineCrankJoint()
//	:dAnimIDRigKinematicLoopJoint()
	:m_targetRpm(0.0f)
	,m_targetTorque(D_VEHICLE_STOP_TORQUE)
{
	dAssert(0);
//	m_isActive = true;
}

void dEngineCrankJoint::SetTorqueAndRpm(dFloat torque, dFloat rpm)
{
	m_targetRpm = -dAbs(rpm);
	m_targetTorque = dAbs(torque);
}

void dEngineCrankJoint::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dAssert(0);
/*
	dComplementaritySolver::dBodyState* const engine = m_state0;
	dComplementaritySolver::dBodyState* const chassis = GetOwner0()->GetProxyBody();

	const dVector& omega = chassis->GetOmega();
	const dMatrix& matrix = engine->GetMatrix();
	AddAngularRowJacobian(constraintParams, matrix.m_right, omega, 0.0f);

	const dVector& omega0 = m_state0->GetOmega();
	const dVector& omega1 = m_state1->GetOmega();
	const dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians->m_jacobian_J01;
	const dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians->m_jacobian_J10;
	const dVector relVeloc(omega0 * jacobian0.m_angular + omega1 * jacobian1.m_angular);
	dFloat w = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;
	dFloat relOmega = m_targetRpm - w;
	constraintParams->m_jointAccel[0] = relOmega * constraintParams->m_timestepInv;

	bool test = (m_targetRpm > -0.1f) && (w > -1.0f);
	dFloat targetTorque = test ? D_VEHICLE_STOP_TORQUE : m_targetTorque;
	constraintParams->m_jointLowFrictionCoef[0] = -targetTorque;
	constraintParams->m_jointHighFrictionCoef[0] = targetTorque;

	m_dof = 1;
	m_count = 1;
	constraintParams->m_count = 1;
*/
}

// *******************************************************************
// engine crank
// *******************************************************************
dGearBoxJoint::dGearBoxJoint()
//	:dAnimIDRigKinematicLoopJoint()
	:m_gearRatio(0.0f)
	,m_crowndGear(1.0f)
	,m_clutchTorque(1000.0f)
{
	dAssert(0);
//	m_isActive = true;
}

void dGearBoxJoint::SetGearRatio(dFloat ratio)
{
	m_gearRatio = ratio;
}

void dGearBoxJoint::SetClutchTorque(dFloat clutchTorque)
{
	m_clutchTorque = clutchTorque;
}

void dGearBoxJoint::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dAssert(0);
/*
	m_dof = 0;
	m_count = 0;
	constraintParams->m_count = 0;
	if (dAbs(m_gearRatio) > 1.0e-3f) {

		dVehicleVirtualEngine* const engineNode = (dVehicleVirtualEngine*)GetOwner0();
		dComplementaritySolver::dBodyState* const chassis = engineNode->GetProxyBody();
		const dVehicleEngineInterface::dEngineInfo& info = engineNode->GetInfo();
	
		const dVector& omega = chassis->GetOmega(); 
		const dMatrix& matrix = m_state0->GetMatrix();
		AddAngularRowJacobian(constraintParams, matrix.m_right, omega, 0.0f);

		dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[0].m_jacobian_J01;
		dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[0].m_jacobian_J10;

		dFloat gain = info.m_crownGear * m_gearRatio;
		jacobian1.m_angular = jacobian1.m_angular.Scale(gain);

		const dVector& omega0 = m_state0->GetOmega();
		const dVector& omega1 = m_state1->GetOmega();

		const dVector relVeloc(omega0 * jacobian0.m_angular + omega1 * jacobian1.m_angular);
		dFloat w = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;

		constraintParams->m_jointAccel[0] = -w * constraintParams->m_timestepInv;
		constraintParams->m_jointLowFrictionCoef[0] = -m_clutchTorque;
		constraintParams->m_jointHighFrictionCoef[0] = m_clutchTorque;

//constraintParams->m_jointLowFrictionCoef[0] = -50;
//constraintParams->m_jointHighFrictionCoef[0] = 50;

		m_dof = 1;
		m_count = 1;
		constraintParams->m_count = 1;
	}
*/
}

// *******************************************************************
// tire axle
// *******************************************************************
dTireAxleJoint::dTireAxleJoint()
//	:dAnimIDRigKinematicLoopJoint()
	:m_diffSign(1.0f)
{
	dAssert(0);
//	m_isActive = true;
}

void dTireAxleJoint::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	dAssert(0);
/*
	dVehicleVirtualEngine* const engineNode = (dVehicleVirtualEngine*)GetOwner0();
	dComplementaritySolver::dBodyState* const chassis = engineNode->GetProxyBody();

	const dVector& omega = chassis->GetOmega();
	const dMatrix& diffMatrix = m_state0->GetMatrix();
	const dMatrix& tireMatrix = m_state1->GetMatrix();
	AddAngularRowJacobian(constraintParams, tireMatrix.m_front, omega, 0.0f);

	dComplementaritySolver::dJacobian &jacobian0 = constraintParams->m_jacobians[0].m_jacobian_J01;
	dComplementaritySolver::dJacobian &jacobian1 = constraintParams->m_jacobians[0].m_jacobian_J10;

	jacobian0.m_angular = diffMatrix.m_right + diffMatrix.m_up.Scale (m_diffSign);

	const dVector& omega0 = m_state0->GetOmega();
	const dVector& omega1 = m_state1->GetOmega();

	const dVector relVeloc(omega0 * jacobian0.m_angular + omega1 * jacobian1.m_angular);
	dFloat w = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;

	constraintParams->m_jointAccel[0] = -w * constraintParams->m_timestepInv;

	m_dof = 1;
	m_count = 1;
	constraintParams->m_count = 1;
*/
}