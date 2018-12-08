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

#include "dStdAfxMath.h"
#include "dMathDefines.h"
#include "dLinearAlgebra.h"


#define COMPLEMENTARITY_VEL_DAMP			dFloat(100.0f)
#define COMPLEMENTARITY_POS_DAMP			dFloat(1500.0f)
#define COMPLEMENTARITY_PSD_DAMP_TOL		dFloat(1.0e-4f)
#define COMPLEMENTARITY_STACK_ENTRIES		64


void dSymmetricBiconjugateGradientSolve::ScaleAdd (int size, dFloat64* const a, const dFloat64* const b, dFloat64 scale, const dFloat64* const c) const
{
	for (int i = 0; i < size; i ++) {
		a[i] = b[i] + scale * c[i];
	}
}

void dSymmetricBiconjugateGradientSolve::Sub (int size, dFloat64* const a, const dFloat64* const b, const dFloat64* const c) const
{
	for (int i = 0; i < size; i ++) {
		a[i] = b[i] - c[i];
	}
}

dFloat64 dSymmetricBiconjugateGradientSolve::DotProduct (int size, const dFloat64* const b, const dFloat64* const c) const
{
	dFloat64 product = dFloat64 (0.0f);
	for (int i = 0; i < size; i ++) {
		product += b[i] * c[i];
	}
	return product;
}

dFloat64 dSymmetricBiconjugateGradientSolve::Solve (int size, dFloat64 tolerance, dFloat64* const x, const dFloat64* const b) const
{
	dFloat64* const r0 = new dFloat64 [size];
	dFloat64* const p0 = new dFloat64 [size];
	dFloat64* const MinvR0 = new dFloat64 [size];
	dFloat64* const matrixP0 = new dFloat64 [size];

	MatrixTimeVector (matrixP0, x);
	Sub(size, r0, b, matrixP0);
	bool continueExecution = InversePrecoditionerTimeVector (p0, r0);

	int iter = 0;
	dFloat64 num = DotProduct (size, r0, p0);
	dFloat64 error2 = num;
	for (int j = 0; (j < size) && (error2 > tolerance) && continueExecution; j ++) {

		MatrixTimeVector (matrixP0, p0);
		dFloat64 den = DotProduct (size, p0, matrixP0);

		dAssert (fabs(den) > dFloat64 (0.0f));
		dFloat64 alpha = num / den;

		ScaleAdd (size, x, x, alpha, p0);
        if ((j % 50) != 49) {
		    ScaleAdd (size, r0, r0, -alpha, matrixP0);
        } else {
            MatrixTimeVector (matrixP0, x);
            Sub(size, r0, b, matrixP0);
        }

		continueExecution = InversePrecoditionerTimeVector (MinvR0, r0);

		dFloat64 num1 = DotProduct (size, r0, MinvR0);
		dFloat64 beta = num1 / num;
		ScaleAdd (size, p0, MinvR0, beta, p0);
		num = DotProduct (size, r0, MinvR0);
		iter ++;
		error2 = num;
		if (j > 10) {
			error2 = dFloat64 (0.0f);
			for (int i = 0; i < size; i ++) {
				error2 = dMax (error2, r0[i] * r0[i]);
			}
		}
	}

	delete[] matrixP0;
	delete[] MinvR0;
	delete[] p0;
	delete[] r0;

	dAssert (iter <= size);
	return num;
}


dComplementaritySolver::dBodyState::dBodyState()
	:m_matrix(dGetIdentityMatrix())
	,m_localFrame(dGetIdentityMatrix())
	,m_inertia(dGetZeroMatrix())
	,m_invInertia(dGetZeroMatrix())
	,m_localInertia (0.0f)
	,m_localInvInertia(0.0f)
	,m_veloc(0.0f)
	,m_omega(0.0f)
	,m_externalForce(0.0f)
	,m_externalTorque(0.0f)
	,m_globalCentreOfMass(0.0f)
	,m_mass(0.0f)
	,m_invMass(0.0f)
	,m_myIndex(0)
{
}


const dVector& dComplementaritySolver::dBodyState::GetOmega() const
{
	return m_omega;
}

const dVector& dComplementaritySolver::dBodyState::GetVelocity() const
{
	return m_veloc;
}

dVector dComplementaritySolver::dBodyState::CalculatePointVelocity (const dVector& point) const
{
	return m_veloc + m_omega.CrossProduct(point - m_globalCentreOfMass);
}

dFloat dComplementaritySolver::dBodyState::GetMass () const
{
	return m_mass;
}

dFloat dComplementaritySolver::dBodyState::GetInvMass () const
{
	return m_invMass;
}

void dComplementaritySolver::dBodyState::SetMass (dFloat mass)
{
	m_mass = mass;
	m_invMass = 1.0f / mass;
}

void dComplementaritySolver::dBodyState::SetInertia (dFloat Ixx, dFloat Iyy, dFloat Izz)
{
	m_localInertia[0] = Ixx;
	m_localInertia[1] =	Iyy;
	m_localInertia[2] =	Izz;
	m_localInvInertia[0] = 1.0f / Ixx;
	m_localInvInertia[1] = 1.0f / Iyy;
	m_localInvInertia[2] = 1.0f / Izz;
}

void dComplementaritySolver::dBodyState::GetInertia (dFloat& Ixx, dFloat& Iyy, dFloat& Izz) const
{
	Ixx = m_localInertia[0];
	Iyy = m_localInertia[1];
	Izz = m_localInertia[2];
}

const dMatrix& dComplementaritySolver::dBodyState::GetInertia() const
{
	return m_inertia;
}

const dMatrix& dComplementaritySolver::dBodyState::GetInvInertia() const
{
	return m_invInertia;
}

void dComplementaritySolver::dBodyState::SetMatrix (const dMatrix& matrix)
{
	m_matrix = matrix;
	m_globalCentreOfMass = m_matrix.TransformVector(m_localFrame.m_posit);
}

const dMatrix& dComplementaritySolver::dBodyState::GetMatrix () const
{
	return m_matrix;
}

void dComplementaritySolver::dBodyState::SetLocalMatrix (const dMatrix& matrix)
{
	m_localFrame = matrix;
	m_globalCentreOfMass = m_matrix.TransformVector(m_localFrame.m_posit);
}

const dMatrix& dComplementaritySolver::dBodyState::GetLocalMatrix () const
{
	return m_localFrame;
}

const dVector& dComplementaritySolver::dBodyState::GetCOM () const
{
	return m_localFrame.m_posit;
}

void dComplementaritySolver::dBodyState::SetVeloc (const dVector& veloc)
{
	m_veloc = veloc;
}

void dComplementaritySolver::dBodyState::SetOmega (const dVector& omega)
{
	m_omega = omega;
}

void dComplementaritySolver::dBodyState::SetForce (const dVector& force)
{
	m_externalForce = force;
}

void dComplementaritySolver::dBodyState::SetTorque (const dVector& torque)
{
	m_externalTorque = torque;
}

const dVector& dComplementaritySolver::dBodyState::GetForce () const
{
	return m_externalForce;
}

const dVector& dComplementaritySolver::dBodyState::GetTorque () const
{
	return m_externalTorque;
}

void dComplementaritySolver::dBodyState::UpdateInertia()
{
	dMatrix tmpMatrix (dGetZeroMatrix());

	tmpMatrix[0] = m_localInertia * dVector (m_matrix[0][0], m_matrix[1][0], m_matrix[2][0], 0.0f);
	tmpMatrix[1] = m_localInertia * dVector (m_matrix[0][1], m_matrix[1][1], m_matrix[2][1], 0.0f);
	tmpMatrix[2] = m_localInertia * dVector (m_matrix[0][2], m_matrix[1][2], m_matrix[2][2], 0.0f);
	m_inertia = tmpMatrix * m_matrix;

	tmpMatrix[0] = m_localInvInertia * dVector (m_matrix[0][0], m_matrix[1][0], m_matrix[2][0], 0.0f);
	tmpMatrix[1] = m_localInvInertia * dVector (m_matrix[0][1], m_matrix[1][1], m_matrix[2][1], 0.0f);
	tmpMatrix[2] = m_localInvInertia * dVector (m_matrix[0][2], m_matrix[1][2], m_matrix[2][2], 0.0f);
	m_invInertia = tmpMatrix * m_matrix;
}

void dComplementaritySolver::dBodyState::IntegrateForce (dFloat timestep, const dVector& force, const dVector& torque)
{
	dVector accel (force.Scale (m_invMass));
	dVector alpha (m_invInertia.RotateVector(torque));
	m_veloc += accel.Scale (timestep);
	m_omega += alpha.Scale (timestep);
}

void dComplementaritySolver::dBodyState::IntegrateVelocity (dFloat timestep)
{
	const dFloat D_MAX_ANGLE_STEP = dFloat (45.0f * dDegreeToRad);
	const dFloat D_ANGULAR_TOL = dFloat (0.0125f * dDegreeToRad);

	m_globalCentreOfMass += m_veloc.Scale (timestep); 
	while ((m_omega.DotProduct3(m_omega) * timestep * timestep) > (D_MAX_ANGLE_STEP * D_MAX_ANGLE_STEP)) {
		m_omega = m_omega.Scale (dFloat (0.8f));
	}

	// this is correct
	dFloat omegaMag2 = m_omega.DotProduct3(m_omega);
	if (omegaMag2 > (D_ANGULAR_TOL * D_ANGULAR_TOL)) {
		dFloat invOmegaMag = 1.0f / dSqrt (omegaMag2);
		dVector omegaAxis (m_omega.Scale (invOmegaMag));
		dFloat omegaAngle = invOmegaMag * omegaMag2 * timestep;
		dQuaternion rotation (omegaAxis, omegaAngle);
		dQuaternion rotMatrix (m_matrix);
		rotMatrix = rotMatrix * rotation;
		rotMatrix.Scale( 1.0f / dSqrt (rotMatrix.DotProduct (rotMatrix)));
		m_matrix = dMatrix (rotMatrix, m_matrix.m_posit);
	}

	m_matrix.m_posit = m_globalCentreOfMass - m_matrix.RotateVector(m_localFrame.m_posit);

#ifdef _DEBUG
	int j0 = 1;
	int j1 = 2;
	for (int i = 0; i < 3; i ++) {
		dAssert (m_matrix[i][3] == 0.0f);
		dFloat val = m_matrix[i].DotProduct3(m_matrix[i]);
		dAssert (dAbs (val - 1.0f) < 1.0e-5f);
		dVector tmp (m_matrix[j0].CrossProduct(m_matrix[j1]));
		val = tmp.DotProduct3(m_matrix[i]);
		dAssert (dAbs (val - 1.0f) < 1.0e-5f);
		j0 = j1;
		j1 = i;
	}
#endif
}

void dComplementaritySolver::dBodyState::ApplyNetForceAndTorque (dFloat invTimestep, const dVector& veloc, const dVector& omega)
{
	dVector accel = (m_veloc - veloc).Scale(invTimestep);
	dVector alpha = (m_omega - omega).Scale(invTimestep);

	m_externalForce = accel.Scale(m_mass);
	alpha = m_matrix.UnrotateVector(alpha);
	m_externalTorque = m_matrix.RotateVector(alpha * m_localInertia);
}

void dComplementaritySolver::dBilateralJoint::Init(dBodyState* const state0, dBodyState* const state1)
{
	m_start = 0;
	m_count = 0;
	memset (m_rowIsMotor, 0, sizeof (m_rowIsMotor));
	memset (m_motorAcceleration, 0, sizeof (m_motorAcceleration));
	memset (m_jointFeebackForce, 0, sizeof (m_jointFeebackForce));

	m_state0 = state0;
	m_state1 = state1;
}

void dComplementaritySolver::dBilateralJoint::InitPointParam (dPointDerivativeParam& param, const dVector& pivot) const
{
	dAssert (m_state0);
	dAssert (m_state1);

	param.m_posit0 = pivot;
	param.m_r0 = pivot - m_state0->m_globalCentreOfMass;
	param.m_veloc0 = m_state0->m_omega.CrossProduct(param.m_r0);
	//param.m_centripetal0 = m_state0->m_omega.CrossProduct(param.m_veloc0);
	//param.m_veloc0 += m_state0->m_veloc;

	param.m_posit1 = pivot;
	param.m_r1 = pivot - m_state1->m_globalCentreOfMass;
	param.m_veloc1 = m_state1->m_omega.CrossProduct(param.m_r1);
	//param.m_centripetal1 = m_state1->m_omega.CrossProduct(param.m_veloc1);
	//param.m_veloc1 += m_state1->m_veloc;
}


void dComplementaritySolver::dBilateralJoint::CalculatePointDerivative (dParamInfo* const constraintParams, const dVector& dir, const dVector& dirOmega, const dPointDerivativeParam& param)
{
	int index = constraintParams->m_count;

	dAssert(dir.m_w == 0.0f);
	dJacobian &jacobian0 = constraintParams->m_jacobians[index].m_jacobian_J01; 
	jacobian0.m_linear = dir;
	jacobian0.m_angular = param.m_r0.CrossProduct(jacobian0.m_linear);

	dJacobian &jacobian1 = constraintParams->m_jacobians[index].m_jacobian_J10; 
	jacobian1.m_linear = dir.Scale (-1.0f);
	jacobian1.m_angular = param.m_r1.CrossProduct(jacobian1.m_linear);

//	dVector velocError (param.m_veloc1 - param.m_veloc0);
//	dVector positError (param.m_posit1 - param.m_posit0);
//	dVector centrError (param.m_centripetal1 - param.m_centripetal0);

//	dFloat relPosit = positError.DotProduct3(dir);
//	dFloat relVeloc = velocError.DotProduct3(dir);
//	dFloat relCentr = centrError.DotProduct3(dir); 
	
	dVector j01_linear(dirOmega.CrossProduct(jacobian0.m_linear));
	dVector j01_angular(dirOmega.CrossProduct(jacobian0.m_angular));
	dVector j10_linear(dirOmega.CrossProduct(jacobian1.m_linear));
	dVector j10_angular(dirOmega.CrossProduct(jacobian1.m_angular));

	const dVector& veloc0 = m_state0->m_veloc;
	const dVector& omega0 = m_state0->m_omega;
	const dVector& veloc1 = m_state1->m_veloc;
	const dVector& omega1 = m_state1->m_omega;
	const dVector accel(j01_linear * veloc0 + j01_angular * omega0 + j10_linear * veloc1 + j10_angular * omega1);

/*
	dFloat dt = constraintParams->m_timestep;
	dFloat ks = COMPLEMENTARITY_POS_DAMP;
	dFloat kd = COMPLEMENTARITY_VEL_DAMP;
	dFloat ksd = dt * ks;
	dFloat num = ks * relPosit + kd * relVeloc + ksd * relVeloc;
	dFloat den = dFloat (1.0f) + dt * kd + dt * ksd;
	dFloat accelError = num / den;

	m_rowIsMotor[index] = false;
	m_motorAcceleration[index] = 0.0f;
	constraintParams->m_jointAccel[index] = accelError + relCentr;
*/
	constraintParams->m_jointAccel[index] = - (accel.m_x + accel.m_y + accel.m_z);
	constraintParams->m_jointLowFrictionCoef[index] = D_COMPLEMENTARITY_MIN_FRICTION_BOUND;
	constraintParams->m_jointHighFrictionCoef[index] = D_COMPLEMENTARITY_MAX_FRICTION_BOUND;
	constraintParams->m_count = index + 1;
}

void dComplementaritySolver::dBilateralJoint::AddAngularRowJacobian (dParamInfo* const constraintParams, const dVector& dir, const dVector& dirOmega, dFloat jointAngle)
{
	int index = constraintParams->m_count;
	dAssert(dir.m_w == 0.0f);

	dJacobian &jacobian0 = constraintParams->m_jacobians[index].m_jacobian_J01; 
	jacobian0.m_linear = dVector(0.0f);
	jacobian0.m_angular = dir;

	dJacobian &jacobian1 = constraintParams->m_jacobians[index].m_jacobian_J10; 
	jacobian1.m_linear = dVector(0.0f);
	jacobian1.m_angular = dir.Scale (-1.0f);

	const dVector& omega0 = m_state0->m_omega;
	const dVector& omega1 = m_state1->m_omega;

	dVector j01_angular(dirOmega.CrossProduct(jacobian0.m_angular));
	dVector j10_angular(dirOmega.CrossProduct(jacobian1.m_angular));
	const dVector accel(j01_angular * omega0 + j10_angular * omega1);
/*
	dFloat omegaError = (omega1 - omega0).DotProduct3(dir);
	//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
	dFloat dt = constraintParams->m_timestep;
	dFloat ks = COMPLEMENTARITY_POS_DAMP;
	dFloat kd = COMPLEMENTARITY_VEL_DAMP;
	dFloat ksd = dt * ks;
	dFloat num = ks * jointAngle + kd * omegaError + ksd * omegaError;
	dFloat den = dFloat (1.0f) + dt * kd + dt * ksd;
	dFloat alphaError = num / den;

	m_rowIsMotor[index] = false;
	m_motorAcceleration[index] = 0.0f;
	constraintParams->m_jointAccel[index] = alphaError;
*/
	constraintParams->m_jointAccel[index] = -(accel.m_x + accel.m_y + accel.m_z);
	constraintParams->m_jointLowFrictionCoef[index] = D_COMPLEMENTARITY_MIN_FRICTION_BOUND;
	constraintParams->m_jointHighFrictionCoef[index] = D_COMPLEMENTARITY_MAX_FRICTION_BOUND;
	constraintParams->m_count = index + 1;
}


/*
void dComplementaritySolver::dBilateralJoint::AddAngularRowJacobian (dParamInfo* const constraintParams, const dVector& dir0, const dVector& dir1, dFloat accelerationRatio)
{
	int index = constraintParams->m_count;
	dJacobian &jacobian0 = constraintParams->m_jacobians[index].m_jacobian_IM0; 

	jacobian0.m_linear[0] = 0.0f;
	jacobian0.m_linear[1] = 0.0f;
	jacobian0.m_linear[2] = 0.0f;
	jacobian0.m_linear[3] = 0.0f;
	jacobian0.m_angular[0] = dir0.m_x;
	jacobian0.m_angular[1] = dir0.m_y;
	jacobian0.m_angular[2] = dir0.m_z;
	jacobian0.m_angular[3] = 0.0f;

	dJacobian &jacobian1 = constraintParams->m_jacobians[index].m_jacobian_IM1; 
	jacobian1.m_linear[0] = 0.0f;
	jacobian1.m_linear[1] = 0.0f;
	jacobian1.m_linear[2] = 0.0f;
	jacobian1.m_linear[3] = 0.0f;
	jacobian1.m_angular[0] = dir1.m_x;
	jacobian1.m_angular[1] = dir1.m_y;
	jacobian1.m_angular[2] = dir1.m_z;
	jacobian1.m_angular[3] = 0.0f;

	m_rowIsMotor[index] = true;
	m_motorAcceleration[index] = accelerationRatio;
	constraintParams->m_jointAccel[index] = 0.0f;
	constraintParams->m_jointLowFriction[index] = D_COMPLEMENTARITY_MIN_FRICTION_BOUND;
	constraintParams->m_jointHighFriction[index] = D_COMPLEMENTARITY_MAX_FRICTION_BOUND;
	constraintParams->m_count = index + 1;
}
*/

void dComplementaritySolver::dBilateralJoint::AddLinearRowJacobian (dParamInfo* const constraintParams, const dVector& pivot, const dVector& dir, const dVector& dirOmega)
{
	dPointDerivativeParam pointData;
	InitPointParam (pointData, pivot);
	CalculatePointDerivative (constraintParams, dir, dirOmega, pointData);
}

void dComplementaritySolver::dBilateralJoint::JointAccelerations (dJointAccelerationDecriptor* const params)
{
	dJacobianColum* const jacobianColElements = params->m_colMatrix;
	dJacobianPair* const jacobianRowElements = params->m_rowMatrix;

	const dVector& bodyVeloc0 = m_state0->m_veloc;
	const dVector& bodyOmega0 = m_state0->m_omega;
	const dVector& bodyVeloc1 = m_state1->m_veloc;
	const dVector& bodyOmega1 = m_state1->m_omega;

	dFloat timestep = params->m_timeStep;
	dFloat kd = COMPLEMENTARITY_VEL_DAMP * dFloat (4.0f);
	dFloat ks = COMPLEMENTARITY_POS_DAMP * dFloat (0.25f);
	for (int k = 0; k < params->m_rowsCount; k ++) {
		if (m_rowIsMotor[k]) {
			jacobianColElements[k].m_coordenateAccel = m_motorAcceleration[k] + jacobianColElements[k].m_deltaAccel;
		} else {
			const dJacobianPair& Jt = jacobianRowElements[k];
			dVector relVeloc (Jt.m_jacobian_J01.m_linear * bodyVeloc0 +
							  Jt.m_jacobian_J01.m_angular * bodyOmega0 + 
							  Jt.m_jacobian_J10.m_linear * bodyVeloc1 +
							  Jt.m_jacobian_J10.m_angular * bodyOmega1);

			dFloat vRel = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;
			dFloat aRel = jacobianColElements[k].m_deltaAccel;
			dFloat ksd = timestep * ks;
			dFloat relPosit = 0.0f - vRel * timestep * params->m_firstPassCoefFlag;

			dFloat num = ks * relPosit - kd * vRel - ksd * vRel;
			dFloat den = dFloat (1.0f) + timestep * kd + timestep * ksd;
			dFloat aRelErr = num / den;
			jacobianColElements[k].m_coordenateAccel = aRelErr + aRel;
		}
	}
}

int dComplementaritySolver::dFrictionLessContactJoint::CompareContact (const dContact* const contactA, const dContact* const contactB, void* dommy)
{
	if (contactA->m_point[0] < contactB->m_point[0]) {
		return -1;
	} else if (contactA->m_point[0] > contactB->m_point[0]) {
		return 1;
	} else {
		return 0;
	}
}

int dComplementaritySolver::dFrictionLessContactJoint::ReduceContacts (int count, dContact* const contacts, dFloat tol)
{
	int mask[D_MAX_PLACEMENT_CONTACTS];
	int index = 0;
	int packContacts = 0;
	dFloat window = tol;
	dFloat window2 = window * window;
	memset (mask, 0, size_t (count));
	dSort (contacts, count, CompareContact, NULL);
	dAssert (count <= D_MAX_PLACEMENT_CONTACTS);
	for (int i = 0; i < count; i ++) {
		if (!mask[i]) {
			dFloat val = contacts[i].m_point[index] + window;
			for (int j = i + 1; (j < count) && (contacts[j].m_point[index] < val) ; j ++) {
				if (!mask[j]) {
					dVector dp (contacts[j].m_point - contacts[i].m_point);
					dFloat dist2 = dp.DotProduct3(dp);
					if (dist2 < window2) {
						mask[j] = 1;
						packContacts = 1;
					}
				}
			}
		}
	}

	if (packContacts) {
		int j = 0;
		for (int i = 0; i < count; i ++) {
			dAssert (i < D_MAX_PLACEMENT_CONTACTS);
			if (!mask[i]) {
				contacts[j] = contacts[i];
				j ++;
			}
		}
		count = j;
	}
	return count;
}

void dComplementaritySolver::dFrictionLessContactJoint::SetContacts (int count, dContact* const contacts, dFloat restitution)
{
	dFloat tol = 5.0e-3f;
	count = ReduceContacts(count, contacts, tol);
	while (count > D_MAX_PRAM_INFO_SIZE) {
		tol *= 2.0f; 
		count = ReduceContacts(count, contacts, tol);
	}

	m_count = count;
	m_restitution = restitution;
	memcpy (m_contacts, contacts, count * sizeof (dContact));
}

void dComplementaritySolver::dFrictionLessContactJoint::JacobianDerivative (dParamInfo* const constraintParams)
{
	for (int i = 0; i < m_count; i ++) {
		dAssert(0);
/*
		dPointDerivativeParam pointData;
		InitPointParam (pointData, m_contacts[i].m_point);
		CalculatePointDerivative (constraintParams, m_contacts[i].m_normal, pointData);

		dVector velocError (pointData.m_veloc1 - pointData.m_veloc0);

		//dFloat restitution = 0.05f;
		dFloat relVelocErr = velocError.DotProduct3(m_contacts[i].m_normal);
		dFloat penetration = 0.0f;
		dFloat penetrationStiffness = 0.0f;
		dFloat penetrationVeloc = penetration * penetrationStiffness;

		if (relVelocErr > dFloat(1.0e-3f)) {
			relVelocErr *= (m_restitution + dFloat (1.0f));
		}

		constraintParams->m_jointLowFriction[i] = dFloat (0.0f);
		constraintParams->m_jointAccel[i] = dMax (dFloat (-4.0f), relVelocErr + penetrationVeloc) * constraintParams->m_timestepInv;
*/
	}
}


void dComplementaritySolver::dFrictionLessContactJoint::JointAccelerations (dJointAccelerationDecriptor* const params)
{
	dJacobianPair* const rowMatrix = params->m_rowMatrix;
	dJacobianColum* const jacobianColElements = params->m_colMatrix;

	const dVector& bodyVeloc0 = m_state0->GetVelocity();
	const dVector& bodyOmega0 = m_state0->GetOmega();
	const dVector& bodyVeloc1 = m_state1->GetVelocity();
	const dVector& bodyOmega1 = m_state1->GetOmega();

	int count = params->m_rowsCount;

	dAssert (params->m_timeStep > dFloat (0.0f));
	for (int k = 0; k < count; k ++) {
		const dJacobianPair& Jt = rowMatrix[k];
		dJacobianColum& element = jacobianColElements[k];

		dVector relVeloc (Jt.m_jacobian_J01.m_linear * bodyVeloc0 + Jt.m_jacobian_J01.m_angular * bodyOmega0 + Jt.m_jacobian_J10.m_linear * bodyVeloc1 + Jt.m_jacobian_J10.m_angular * bodyOmega1);

		dFloat vRel = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;
		dFloat aRel = element.m_deltaAccel;
		//dFloat restitution = (vRel <= 0.0f) ? 1.05f : 1.0f;
		dFloat restitution = (vRel <= 0.0f) ? (dFloat (1.0f) + m_restitution) : dFloat(1.0f);
		
		vRel *= restitution;
		vRel = dMin (dFloat (4.0f), vRel);
		element.m_coordenateAccel = (aRel - vRel * params->m_invTimeStep);
	}
}

int dComplementaritySolver::BuildJacobianMatrix (int jointCount, dBilateralJoint** const jointArray, dFloat timestep, dJacobianPair* const jacobianArray, dJacobianColum* const jacobianColumnArray, int maxRowCount)
{
	int rowCount = 0;

	dParamInfo constraintParams;
	constraintParams.m_timestep = timestep;
	constraintParams.m_timestepInv = 1.0f / timestep;

	// calculate Jacobian derivative for each active joint	
	for (int j = 0; j < jointCount; j ++) {
		dBilateralJoint* const joint = jointArray[j];
		constraintParams.m_count = 0;
		joint->JacobianDerivative (&constraintParams); 

		int dofCount = constraintParams.m_count;
		joint->m_count = dofCount;
		joint->m_start = rowCount;

		// complete the derivative matrix for this joint
		int index = joint->m_start;
		dBodyState* const state0 = joint->m_state0;
		dBodyState* const state1 = joint->m_state1;

		const dMatrix& invInertia0 = state0->m_invInertia;
		const dMatrix& invInertia1 = state1->m_invInertia;

		dFloat invMass0 = state0->m_invMass;
		dFloat invMass1 = state1->m_invMass;
		dFloat weight = 0.9f;

		for (int i = 0; i < dofCount; i ++) {
			dJacobianPair* const row = &jacobianArray[index];
			dJacobianColum* const col = &jacobianColumnArray[index];
			jacobianArray[rowCount] = constraintParams.m_jacobians[i]; 

			dVector JMinvIM0linear (row->m_jacobian_J01.m_linear.Scale (invMass0));
			dVector JMinvIM1linear (row->m_jacobian_J10.m_linear.Scale (invMass1));
			dVector JMinvIM0angular = invInertia0.UnrotateVector(row->m_jacobian_J01.m_angular);
			dVector JMinvIM1angular = invInertia1.UnrotateVector(row->m_jacobian_J10.m_angular);

			dVector tmpDiag (JMinvIM0linear * row->m_jacobian_J01.m_linear + JMinvIM0angular * row->m_jacobian_J01.m_angular + JMinvIM1linear * row->m_jacobian_J10.m_linear + JMinvIM1angular * row->m_jacobian_J10.m_angular);
			dVector tmpAccel (JMinvIM0linear * state0->m_externalForce + JMinvIM0angular * state0->m_externalTorque + JMinvIM1linear * state1->m_externalForce + JMinvIM1angular * state1->m_externalTorque);
			dFloat extenalAcceleration = -(tmpAccel[0] + tmpAccel[1] + tmpAccel[2]);

			col->m_diagDamp = 1.0f;
			col->m_coordenateAccel = constraintParams.m_jointAccel[i];
dAssert (0);
//			col->m_jointLowFriction = constraintParams.m_jointLowFriction[i];
//			col->m_jointHighFriction = constraintParams.m_jointHighFriction[i];

			col->m_deltaAccel = extenalAcceleration;
			col->m_coordenateAccel += extenalAcceleration;

			col->m_force = joint->m_jointFeebackForce[i] * weight;

			dFloat stiffness = COMPLEMENTARITY_PSD_DAMP_TOL * col->m_diagDamp;
			dFloat diag = (tmpDiag[0] + tmpDiag[1] + tmpDiag[2]);
			dAssert (diag > dFloat (0.0f));
			col->m_diagDamp = diag * stiffness;

			diag *= (dFloat(1.0f) + stiffness);
			col->m_invDJMinvJt = dFloat(1.0f) / diag;
			index ++;
			rowCount ++;
			dAssert (rowCount < maxRowCount);
		}
	}
	return rowCount;
}

void dComplementaritySolver::CalculateReactionsForces (int bodyCount, dBodyState** const bodyArray, int jointCount, dBilateralJoint** const jointArray, dFloat timestepSrc, dJacobianPair* const jacobianArray, dJacobianColum* const jacobianColumnArray)
{
	dJacobian stateVeloc[COMPLEMENTARITY_STACK_ENTRIES];
	dJacobian internalForces [COMPLEMENTARITY_STACK_ENTRIES];

	int stateIndex = 0;
	dVector zero(dFloat (0.0f), dFloat (0.0f), dFloat (0.0f), dFloat (0.0f));
	for (int i = 0; i < bodyCount; i ++) {
		dBodyState* const state = bodyArray[i];
		stateVeloc[stateIndex].m_linear = state->m_veloc;
		stateVeloc[stateIndex].m_angular = state->m_omega;

		internalForces[stateIndex].m_linear = zero;
		internalForces[stateIndex].m_angular = zero;

		state->m_myIndex = stateIndex;
		stateIndex ++;
		dAssert (stateIndex < int (sizeof (stateVeloc)/sizeof (stateVeloc[0])));
	}

	for (int i = 0; i < jointCount; i ++) {
		dJacobian y0;
		dJacobian y1;
		y0.m_linear = zero;
		y0.m_angular = zero;
		y1.m_linear = zero;
		y1.m_angular = zero;
		dBilateralJoint* const constraint = jointArray[i];
		int first = constraint->m_start;
		int count = constraint->m_count;
		for (int j = 0; j < count; j ++) { 
			dJacobianPair* const row = &jacobianArray[j + first];
			const dJacobianColum* const col = &jacobianColumnArray[j + first];
			dFloat val = col->m_force; 
			y0.m_linear += row->m_jacobian_J01.m_linear.Scale(val);
			y0.m_angular += row->m_jacobian_J01.m_angular.Scale(val);
			y1.m_linear += row->m_jacobian_J10.m_linear.Scale(val);
			y1.m_angular += row->m_jacobian_J10.m_angular.Scale(val);
		}
		int m0 = constraint->m_state0->m_myIndex;
		int m1 = constraint->m_state1->m_myIndex;
		internalForces[m0].m_linear += y0.m_linear;
		internalForces[m0].m_angular += y0.m_angular;
		internalForces[m1].m_linear += y1.m_linear;
		internalForces[m1].m_angular += y1.m_angular;
	}


	dFloat invTimestepSrc = dFloat (1.0f) / timestepSrc;
	dFloat invStep = dFloat (0.25f);
	dFloat timestep = timestepSrc * invStep;
	dFloat invTimestep = invTimestepSrc * dFloat (4.0f);

	int maxPasses = 5;
	dFloat firstPassCoef = dFloat (0.0f);
	dFloat maxAccNorm = dFloat (1.0e-2f);

	for (int step = 0; step < 4; step ++) {
		dJointAccelerationDecriptor joindDesc;
		joindDesc.m_timeStep = timestep;
		joindDesc.m_invTimeStep = invTimestep;
		joindDesc.m_firstPassCoefFlag = firstPassCoef;

		for (int i = 0; i < jointCount; i ++) {
			dBilateralJoint* const constraint = jointArray[i];
			joindDesc.m_rowsCount = constraint->m_count;
			joindDesc.m_rowMatrix = &jacobianArray[constraint->m_start];
			joindDesc.m_colMatrix = &jacobianColumnArray[constraint->m_start];
			constraint->JointAccelerations (&joindDesc);
		}
		firstPassCoef = dFloat (1.0f);

		dFloat accNorm = dFloat (1.0e10f);
		for (int passes = 0; (passes < maxPasses) && (accNorm > maxAccNorm); passes ++) {
			accNorm = dFloat (0.0f);
			for (int i = 0; i < jointCount; i ++) {

				dBilateralJoint* const constraint = jointArray[i];
				int index = constraint->m_start;
				int rowsCount = constraint->m_count;
				int m0 = constraint->m_state0->m_myIndex;
				int m1 = constraint->m_state1->m_myIndex;

				dVector linearM0 (internalForces[m0].m_linear);
				dVector angularM0 (internalForces[m0].m_angular);
				dVector linearM1 (internalForces[m1].m_linear);
				dVector angularM1 (internalForces[m1].m_angular);

				dBodyState* const state0 = constraint->m_state0;
				dBodyState* const state1 = constraint->m_state1;
				const dMatrix& invInertia0 = state0->m_invInertia;
				const dMatrix& invInertia1 = state1->m_invInertia;
				dFloat invMass0 = state0->m_invMass;
				dFloat invMass1 = state1->m_invMass;

				for (int k = 0; k < rowsCount; k ++) {
					dJacobianPair* const row = &jacobianArray[index];
					dJacobianColum* const col = &jacobianColumnArray[index];

					dVector JMinvIM0linear (row->m_jacobian_J01.m_linear.Scale (invMass0));
					dVector JMinvIM1linear (row->m_jacobian_J10.m_linear.Scale (invMass1));
					dVector JMinvIM0angular = invInertia0.UnrotateVector(row->m_jacobian_J01.m_angular);
					dVector JMinvIM1angular = invInertia1.UnrotateVector(row->m_jacobian_J10.m_angular);
					dVector acc (JMinvIM0linear * linearM0 + JMinvIM0angular * angularM0 + JMinvIM1linear * linearM1 + JMinvIM1angular * angularM1);

					dFloat a = col->m_coordenateAccel - acc.m_x - acc.m_y - acc.m_z - col->m_force * col->m_diagDamp;
					dFloat f = col->m_force + col->m_invDJMinvJt * a;

					dFloat lowerFrictionForce = col->m_jointLowFriction;
					dFloat upperFrictionForce = col->m_jointHighFriction;

					if (f > upperFrictionForce) {
						a = dFloat (0.0f);
						f = upperFrictionForce;
					} else if (f < lowerFrictionForce) {
						a = dFloat (0.0f);
						f = lowerFrictionForce;
					}

					accNorm = dMax (accNorm, dAbs (a));
					dFloat prevValue = f - col->m_force;
					col->m_force = f;

					linearM0 += row->m_jacobian_J01.m_linear.Scale (prevValue);
					angularM0 += row->m_jacobian_J01.m_angular.Scale (prevValue);
					linearM1 += row->m_jacobian_J10.m_linear.Scale (prevValue);
					angularM1 += row->m_jacobian_J10.m_angular.Scale (prevValue);
					index ++;
				}
				internalForces[m0].m_linear = linearM0;
				internalForces[m0].m_angular = angularM0;
				internalForces[m1].m_linear = linearM1;
				internalForces[m1].m_angular = angularM1;
			}
		}

		for (int i = 0; i < bodyCount; i ++) {
			dBodyState* const state = bodyArray[i];
			//int index = state->m_myIndex;
			dAssert (state->m_myIndex == i);
			dVector force (state->m_externalForce + internalForces[i].m_linear);
			dVector torque (state->m_externalTorque + internalForces[i].m_angular);
			state->IntegrateForce(timestep, force, torque);
		}
	}

	for (int i = 0; i < jointCount; i ++) {
		dBilateralJoint* const constraint = jointArray[i];
		int first = constraint->m_start;
		int count = constraint->m_count;
		for (int j = 0; j < count; j ++) { 
			const dJacobianColum* const col = &jacobianColumnArray[j + first];
			dFloat val = col->m_force; 
			constraint->m_jointFeebackForce[j] = val;
		}
	}

	for (int i = 0; i < jointCount; i ++) {
		dBilateralJoint* const constraint = jointArray[i];
		constraint->UpdateSolverForces (jacobianArray);
	}

	for (int i = 0; i < bodyCount; i ++) {
		dBodyState* const state = bodyArray[i];
		dAssert (state->m_myIndex == i);
		state->ApplyNetForceAndTorque (invTimestepSrc, stateVeloc[i].m_linear, stateVeloc[i].m_angular);
	}
}

