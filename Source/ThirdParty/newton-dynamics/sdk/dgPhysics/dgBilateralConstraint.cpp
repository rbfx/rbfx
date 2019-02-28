/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgConstraint.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define DG_VEL_DAMP				 (dgFloat32(100.0f))
#define DG_POS_DAMP				 (dgFloat32(1500.0f))


dgBilateralConstraint::dgBilateralConstraint ()
	:dgConstraint () 
	,m_destructor(NULL)
	,m_jointNode(NULL)
{
	m_maxDOF = 6;
	m_isBilateral = true;
	m_contactActive = true;
	m_solverModel = 0;
	m_rowIsIk = 0;
	m_rowIsMotor = 0;

	SetStiffness (dgFloat32 (1.0f));

	memset (m_jointForce, 0, sizeof (m_jointForce));
	memset (m_motorAcceleration, 0, sizeof (m_motorAcceleration));
	memset (m_inverseDynamicsAcceleration, 0, sizeof (m_inverseDynamicsAcceleration));
}

dgBilateralConstraint::~dgBilateralConstraint ()
{
	if (m_destructor) {
		m_destructor(*this);
	}

	if (m_jointNode) {
		dgAssert(m_body0);
		dgBilateralConstraintList* const jointList = m_body0->m_world;
		jointList->Remove(m_jointNode);
	}
}

void dgBilateralConstraint::AppendToJointList()
{
	dgAssert(m_body0);
	dgAssert(!m_jointNode);
	
	dgBilateralConstraintList* const jointList = m_body0->m_world;
	m_jointNode = jointList->Addtop(this);
}


void dgBilateralConstraint::ResetInverseDynamics()
{
	m_rowIsIk = 0;
}

dgInt32 dgBilateralConstraint::GetSolverModel() const
{
	return m_solverModel;
}

void dgBilateralConstraint::SetSolverModel(dgInt32 model)
{
	m_solverModel = dgClamp(model, 0, 2);
}


dgFloat32 dgBilateralConstraint::GetStiffness() const
{
	return m_stiffness;
}

void dgBilateralConstraint::SetStiffness(dgFloat32 stiffness)
{
	m_stiffness = dgClamp (stiffness, dgFloat32(0.0f), dgFloat32(1.0f));
}

void dgBilateralConstraint::SetDestructorCallback (OnConstraintDestroy destructor)
{
	m_destructor = destructor;
}

void dgBilateralConstraint::CalculateMatrixOffset (const dgVector& pivot, const dgVector& dir, dgMatrix& matrix0, dgMatrix& matrix1) const
{
	dgFloat32 length; 
	dgAssert (m_body0);
	dgAssert (m_body1);
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	const dgMatrix& body0_Matrix = m_body0->GetMatrix();

	length = dir.DotProduct(dir).GetScalar();
	length = dgSqrt (length);
	dgAssert (length > dgFloat32 (0.0f));
	matrix0 = dgMatrix (body0_Matrix.UnrotateVector (dir.Scale (dgFloat32 (1.0f) / length)));
	matrix0.m_posit = body0_Matrix.UntransformVector (pivot);

	matrix0.m_front.m_w = dgFloat32 (0.0f);
	matrix0.m_up.m_w    = dgFloat32 (0.0f);
	matrix0.m_right.m_w = dgFloat32 (0.0f);
	matrix0.m_posit.m_w = dgFloat32 (1.0f);

	const dgMatrix& body1_Matrix = m_body1->GetMatrix();
	matrix1 = matrix0 * body0_Matrix * body1_Matrix.Inverse(); 
}


void dgBilateralConstraint::SetPivotAndPinDir(const dgVector &pivot, const dgVector &pinDirection, dgMatrix& matrix0, dgMatrix& matrix1) const
{
	CalculateMatrixOffset (pivot, pinDirection, matrix0, matrix1);
}

void dgBilateralConstraint::SetPivotAndPinDir (const dgVector& pivot, const dgVector& pinDirection0, const dgVector& pinDirection1, dgMatrix& matrix0, dgMatrix& matrix1) const
{
	dgAssert (m_body0);
	dgAssert (m_body1);

	const dgMatrix& body0_Matrix = m_body0->GetMatrix();
	dgAssert (pinDirection0.m_w == dgFloat32 (0.0f));
	dgAssert ((pinDirection0.DotProduct(pinDirection0).GetScalar()) > dgFloat32 (0.0f));

	matrix0.m_front = pinDirection0.Scale (dgRsqrt (pinDirection0.DotProduct(pinDirection0).GetScalar()));
	matrix0.m_right = matrix0.m_front.CrossProduct(pinDirection1);
	matrix0.m_right = matrix0.m_right.Scale (dgRsqrt (matrix0.m_right.DotProduct(matrix0.m_right).GetScalar()));
	matrix0.m_up = matrix0.m_right.CrossProduct(matrix0.m_front); 
	matrix0.m_posit = pivot;
	
	matrix0.m_front.m_w = dgFloat32 (0.0f);
	matrix0.m_up.m_w    = dgFloat32 (0.0f);
	matrix0.m_right.m_w = dgFloat32 (0.0f);
	matrix0.m_posit.m_w = dgFloat32 (1.0f);
	 
	const dgMatrix& body1_Matrix = m_body1->GetMatrix();

	matrix1 = matrix0 * body1_Matrix.Inverse(); 
	matrix0 = matrix0 * body0_Matrix.Inverse();
}

dgVector dgBilateralConstraint::CalculateGlobalMatrixAndAngle (const dgMatrix& localMatrix0, const dgMatrix& localMatrix1, dgMatrix& globalMatrix0, dgMatrix& globalMatrix1) const
{
	dgAssert (m_body0);
	dgAssert (m_body1);
	const dgMatrix& body0Matrix = m_body0->GetMatrix();
	const dgMatrix& body1Matrix = m_body1->GetMatrix();

	globalMatrix0 = localMatrix0 * body0Matrix;
	globalMatrix1 = localMatrix1 * body1Matrix;

	dgMatrix relMatrix (globalMatrix1 * globalMatrix0.Inverse());

	dgAssert (dgAbs (dgFloat32 (1.0f) - (relMatrix.m_front.DotProduct(relMatrix.m_front).GetScalar())) < 1.0e-5f); 
	dgAssert (dgAbs (dgFloat32 (1.0f) - (relMatrix.m_up.DotProduct(relMatrix.m_up).GetScalar())) < 1.0e-5f); 
	dgAssert (dgAbs (dgFloat32 (1.0f) - (relMatrix.m_right.DotProduct(relMatrix.m_right).GetScalar())) < 1.0e-5f); 

	dgVector euler0;
	dgVector euler1;
	relMatrix.CalcPitchYawRoll (euler0, euler1);
	return euler0;
}

dgFloat32 dgBilateralConstraint::GetRowAcceleration (dgInt32 index, dgContraintDescritor& desc) const
{
	//return m_motorAcceleration[index];
	return desc.m_penetrationStiffness[index];
}

void dgBilateralConstraint::SetMotorAcceleration (dgInt32 index, dgFloat32 acceleration, dgContraintDescritor& desc)
{
	m_rowIsMotor |= (1 << index);
	m_motorAcceleration[index] = acceleration;
	desc.m_jointAccel[index] = acceleration;
	desc.m_penetrationStiffness[index] = acceleration; 
}


void dgBilateralConstraint::SetJacobianDerivative (dgInt32 index, dgContraintDescritor& desc, const dgFloat32* const jacobianA, const dgFloat32* const jacobianB, dgForceImpactPair* const jointForce)
{
	dgJacobian &jacobian0 = desc.m_jacobian[index].m_jacobianM0; 
	dgJacobian &jacobian1 = desc.m_jacobian[index].m_jacobianM1; 

	m_r0[index] = dgVector::m_zero;
	jacobian0.m_linear[0] = jacobianA[0];
	jacobian0.m_linear[1] = jacobianA[1];
	jacobian0.m_linear[2] = jacobianA[2];
	jacobian0.m_linear[3] = dgFloat32 (0.0f);
	jacobian0.m_angular[0] = jacobianA[3];
	jacobian0.m_angular[1] = jacobianA[4];
	jacobian0.m_angular[2] = jacobianA[5];
	jacobian0.m_angular[3] = dgFloat32 (0.0f);

	m_r1[index] = dgVector::m_zero;
	jacobian1.m_linear[0] = jacobianB[0];
	jacobian1.m_linear[1] = jacobianB[1];
	jacobian1.m_linear[2] = jacobianB[2];
	jacobian1.m_linear[3] = dgFloat32 (0.0f);
	jacobian1.m_angular[0] = jacobianB[3];
	jacobian1.m_angular[1] = jacobianB[4];
	jacobian1.m_angular[2] = jacobianB[5];
	jacobian1.m_angular[3] = dgFloat32 (0.0f);

	m_rowIsMotor |= (1 << index);
	m_motorAcceleration[index] = dgFloat32 (0.0f);

	desc.m_restitution[index] = dgFloat32 (0.0f);
	desc.m_jointAccel[index] = dgFloat32 (0.0f);
	desc.m_penetration[index] = dgFloat32 (0.0f);
	desc.m_penetrationStiffness[index] = dgFloat32 (0.0f);
	desc.m_jointStiffness[index] = dgFloat32 (1.0f);
	desc.m_forceBounds[index].m_jointForce = jointForce;
}

void dgBilateralConstraint::SetSpringDamperAcceleration (dgInt32 index, dgContraintDescritor& desc, dgFloat32 rowStiffness, dgFloat32 spring, dgFloat32 damper)
{
	if (desc.m_timestep > dgFloat32 (0.0f)) {

		dgAssert (m_body1);
		const dgJacobian &jacobian0 = desc.m_jacobian[index].m_jacobianM0; 
		const dgJacobian &jacobian1 = desc.m_jacobian[index].m_jacobianM1; 

		const dgVector& veloc0 = m_body0->m_veloc;
		const dgVector& omega0 = m_body0->m_omega;
		const dgVector& veloc1 = m_body1->m_veloc;
		const dgVector& omega1 = m_body1->m_omega;

		//dgFloat32 relPosit = (p1Global - p0Global) % jacobian0.m_linear + jointAngle;
		dgFloat32 relPosit = desc.m_penetration[index];
		dgFloat32 relVeloc = - (veloc0.DotProduct(jacobian0.m_linear) + veloc1.DotProduct(jacobian1.m_linear) + omega0.DotProduct(jacobian0.m_angular) + omega1.DotProduct(jacobian1.m_angular)).GetScalar();

		//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
		dgFloat32 dt = desc.m_timestep;
		dgFloat32 ks = dgAbs (spring);
		dgFloat32 kd = dgAbs (damper);
		dgFloat32 ksd = dt * ks;
		dgFloat32 num = ks * relPosit + kd * relVeloc + ksd * relVeloc;
		dgFloat32 den = dt * kd + dt * ksd;
		dgFloat32 accel = num / (dgFloat32 (1.0f) + den);
		rowStiffness = dgClamp(rowStiffness, dgFloat32(0.0f), dgFloat32(1.0f));
		desc.m_jointStiffness[index] = dgFloat32(1.0f) - rowStiffness / DG_PSD_DAMP_TOL;
		SetMotorAcceleration (index, accel, desc);
	}
}

dgFloat32 dgBilateralConstraint::CalculateMotorAcceleration (dgInt32 index, dgContraintDescritor& desc) const
{
	return desc.m_zeroRowAcceleration[index];
}

dgFloat32 dgBilateralConstraint::GetInverseDynamicAcceleration(dgInt32 index) const
{
	dgAssert (index >= 0);
	dgAssert (index < DG_BILATERAL_CONTRAINT_DOF);
	return m_inverseDynamicsAcceleration[index];
}

void dgBilateralConstraint::CalculateAngularDerivative (dgInt32 index, dgContraintDescritor& desc, const dgVector& dir,	dgFloat32 stiffness, dgFloat32 jointAngle, dgForceImpactPair* const jointForce)
{
	dgAssert (jointForce);
	dgAssert (m_body0);
	dgAssert (dir.m_w == dgFloat32 (0.0f));

	dgJacobian &jacobian0 = desc.m_jacobian[index].m_jacobianM0; 
	m_r0[index] = dgVector::m_zero;
	jacobian0.m_linear = dgVector::m_zero;
	jacobian0.m_angular[0] = dir.m_x;
	jacobian0.m_angular[1] = dir.m_y;
	jacobian0.m_angular[2] = dir.m_z;
	jacobian0.m_angular[3] = dgFloat32 (0.0f);

	dgJacobian &jacobian1 = desc.m_jacobian[index].m_jacobianM1; 
	dgAssert (m_body1);
	m_r1[index] = dgVector::m_zero;
	jacobian1.m_linear = dgVector::m_zero;
	jacobian1.m_angular[0] = -dir.m_x;
	jacobian1.m_angular[1] = -dir.m_y;
	jacobian1.m_angular[2] = -dir.m_z;
	jacobian1.m_angular[3] = dgFloat32 (0.0f);

	const dgVector& omega0 = m_body0->GetOmega();
	const dgVector& omega1 = m_body1->GetOmega();
	dgFloat32 omegaError = dir.DotProduct(omega1 - omega0).GetScalar();

	m_rowIsMotor &= ~(1 << index);
	m_motorAcceleration[index] = dgFloat32 (0.0f);
	if (desc.m_timestep > dgFloat32 (0.0f)) {
		#ifdef _DEBUG
			dgVector accel(omega0 * omega0.CrossProduct(jacobian0.m_angular) + omega1 * omega1.CrossProduct(jacobian1.m_angular));
			dgFloat32 relCentr = -accel.AddHorizontal().GetScalar();
			// allow for some large error since this is affected bu numerical precision a lot
			dgAssert (dgAbs(relCentr) < dgFloat32 (4.0f)); 
		#endif

		//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
		dgFloat32 dt = desc.m_timestep;
		dgFloat32 ks = DG_POS_DAMP;
		dgFloat32 kd = DG_VEL_DAMP;
		dgFloat32 ksd = dt * ks;
		dgFloat32 num = ks * jointAngle + kd * omegaError + ksd * omegaError;
		dgFloat32 den = dgFloat32 (1.0f) + dt * kd + dt * ksd;
		dgFloat32 alphaError = num / den;
		//desc.m_zeroRowAcceleration[index] = (jointAngle * desc.m_invTimestep + omegaError) * desc.m_invTimestep;
		desc.m_zeroRowAcceleration[index] = omegaError * desc.m_invTimestep;

		desc.m_penetration[index] = jointAngle;
		desc.m_restitution[index] = dgFloat32 (0.0f);
		desc.m_jointStiffness[index] = stiffness;
		desc.m_jointAccel[index] = alphaError;
		desc.m_penetrationStiffness[index] = alphaError;
		desc.m_forceBounds[index].m_jointForce = jointForce;
	} else {
		desc.m_penetration[index] = dgFloat32 (0.0f);
		desc.m_restitution[index] = dgFloat32 (0.0f);
		desc.m_jointStiffness[index] = stiffness;
		desc.m_jointAccel[index] = omegaError;
		desc.m_penetrationStiffness[index] = omegaError;;
		desc.m_zeroRowAcceleration[index]  = dgFloat32 (0.0f);
		desc.m_forceBounds[index].m_jointForce = jointForce;
	}
}

void dgBilateralConstraint::CalculatePointDerivative (dgInt32 index, dgContraintDescritor& desc, const dgVector& dir, const dgPointParam& param, dgForceImpactPair* const jointForce)
{
	dgAssert (jointForce);
	dgAssert (m_body0);
	dgAssert (m_body1);
	dgAssert (dir.m_w == dgFloat32 (0.0f));

	dgJacobian &jacobian0 = desc.m_jacobian[index].m_jacobianM0; 
	dgVector r0CrossDir (param.m_r0.CrossProduct(dir));
	m_r0[index] = param.m_r0;
	jacobian0.m_linear[0] = dir.m_x;
	jacobian0.m_linear[1] = dir.m_y;
	jacobian0.m_linear[2] = dir.m_z;
	jacobian0.m_linear[3] = dgFloat32 (0.0f);
	jacobian0.m_angular[0] = r0CrossDir.m_x;
	jacobian0.m_angular[1] = r0CrossDir.m_y;
	jacobian0.m_angular[2] = r0CrossDir.m_z;
	jacobian0.m_angular[3] = dgFloat32 (0.0f);

	dgJacobian &jacobian1 = desc.m_jacobian[index].m_jacobianM1; 
	dgVector r1CrossDir (dir.CrossProduct(param.m_r1));
	m_r1[index] = param.m_r1;
	jacobian1.m_linear[0] = -dir.m_x;
	jacobian1.m_linear[1] = -dir.m_y;
	jacobian1.m_linear[2] = -dir.m_z;
	jacobian1.m_linear[3] = dgFloat32 (0.0f);
	jacobian1.m_angular[0] = r1CrossDir.m_x;
	jacobian1.m_angular[1] = r1CrossDir.m_y;
	jacobian1.m_angular[2] = r1CrossDir.m_z;
	jacobian1.m_angular[3] = dgFloat32 (0.0f);

	m_rowIsMotor &= ~(1 << index);
	m_motorAcceleration[index] = dgFloat32 (0.0f);

	dgVector velocError (param.m_veloc1 - param.m_veloc0);
	dgFloat32 relVeloc = velocError.DotProduct(dir).GetScalar();
	if (desc.m_timestep > dgFloat32 (0.0f)) {

		dgVector positError (param.m_posit1 - param.m_posit0);
		dgFloat32 relPosit = positError.DotProduct(dir).GetScalar();

		const dgVector& bodyOmega0 = m_body0->m_omega;
		const dgVector& bodyOmega1 = m_body1->m_omega;

		const dgVector& centripetal0 (bodyOmega0.CrossProduct(bodyOmega0.CrossProduct(m_r0[index])));
		const dgVector& centripetal1 (bodyOmega1.CrossProduct(bodyOmega1.CrossProduct(m_r1[index])));
		const dgVector centripetalAccel (jacobian0.m_linear * centripetal0 + jacobian1.m_linear * centripetal1);
		dgFloat32 relCentr = -centripetalAccel.AddHorizontal().GetScalar();

		//desc.m_zeroRowAcceleration[index] = (relPosit * desc.m_invTimestep + relVeloc) * desc.m_invTimestep;
		desc.m_zeroRowAcceleration[index] = relVeloc * desc.m_invTimestep;

		//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
		const dgFloat32 dt = desc.m_timestep;
		const dgFloat32 ks = DG_POS_DAMP;
		const dgFloat32 kd = DG_VEL_DAMP;
		const dgFloat32 ksd = dt * ks;
		const dgFloat32 num = ks * relPosit + kd * relVeloc + ksd * relVeloc;
		const dgFloat32 den = dgFloat32 (1.0f) + dt * kd + dt * ksd;
		const dgFloat32 accelError = num / den;

		desc.m_penetration[index] = relPosit;
		desc.m_jointStiffness[index] = param.m_stiffness;
		desc.m_jointAccel[index] = accelError + relCentr;
		desc.m_penetrationStiffness[index] = accelError + relCentr;
		desc.m_restitution[index] = dgFloat32 (0.0f);
		desc.m_forceBounds[index].m_jointForce = jointForce;
	} else {
		desc.m_penetration[index] = dgFloat32 (0.0f);
		desc.m_jointStiffness[index] = param.m_stiffness;
		desc.m_jointAccel[index] = relVeloc;
		desc.m_penetrationStiffness[index] = relVeloc;
		desc.m_restitution[index] = dgFloat32 (0.0f);
		desc.m_zeroRowAcceleration[index]  = dgFloat32 (0.0f);
		desc.m_forceBounds[index].m_jointForce = jointForce;
	}
}

void dgBilateralConstraint::JointAccelerations(dgJointAccelerationDecriptor* const params)
{
	const dgVector& bodyVeloc0 = m_body0->m_veloc;
	const dgVector& bodyOmega0 = m_body0->m_omega;
	const dgVector& bodyVeloc1 = m_body1->m_veloc;
	const dgVector& bodyOmega1 = m_body1->m_omega;

// remember the impulse branch 
//dgAssert (params->m_timeStep > dgFloat32 (0.0f));

	dgRightHandSide* const rhs = params->m_rightHandSide;
	const dgLeftHandSide* const row = params->m_leftHandSide;
	if (params->m_timeStep > dgFloat32 (0.0f)) {
		//const dgFloat32 ks = DG_POS_DAMP * dgFloat32 (0.25f);
		//const dgFloat32 kd = DG_VEL_DAMP * dgFloat32 (4.0f);
		const dgFloat32 ks = DG_POS_DAMP * dgFloat32 (0.5f);
		const dgFloat32 kd = DG_VEL_DAMP * dgFloat32 (4.0f);
		const dgFloat32 dt = params->m_timeStep;
		for (dgInt32 k = 0; k < params->m_rowsCount; k ++) {
			if (m_rowIsMotor & (1 << k)) {
   				rhs[k].m_coordenateAccel = m_motorAcceleration[k] + rhs[k].m_deltaAccel;
			} else {
				const dgJacobianPair& Jt = row[k].m_Jt;

				//calculate internal centripetal each sub step 
				const dgVector& centripetal0(bodyOmega0.CrossProduct(bodyOmega0.CrossProduct(m_r0[k])));
				const dgVector& centripetal1(bodyOmega1.CrossProduct(bodyOmega1.CrossProduct(m_r1[k])));
				const dgVector centripetalAccel(Jt.m_jacobianM0.m_linear * centripetal0 + Jt.m_jacobianM1.m_linear * centripetal1);
				dgFloat32 relCentr = -centripetalAccel.AddHorizontal().GetScalar();

				dgVector relVeloc (Jt.m_jacobianM0.m_linear * bodyVeloc0 + Jt.m_jacobianM0.m_angular * bodyOmega0 +
								   Jt.m_jacobianM1.m_linear * bodyVeloc1 + Jt.m_jacobianM1.m_angular * bodyOmega1);
				dgFloat32 vRel = relVeloc.AddHorizontal().GetScalar();

				//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
				//alphaError = num / den;
				//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
				//dgFloat32 dt = desc.m_timestep;
				//dgFloat32 ks = DG_POS_DAMP;
				//dgFloat32 kd = DG_VEL_DAMP;
				//dgFloat32 ksd = dt * ks;
				//dgFloat32 num = ks * relPosit + kd * relVeloc + ksd * relVeloc;
				//dgFloat32 den = dgFloat32 (1.0f) + dt * kd + dt * ksd;
				//accelError = num / den;

				dgFloat32 relPosit = rhs[k].m_penetration - vRel * dt * params->m_firstPassCoefFlag;
				rhs[k].m_penetration = relPosit;

				dgFloat32 ksd = dt * ks;
				dgFloat32 num = ks * relPosit - kd * vRel - ksd * vRel;
				dgFloat32 den = dgFloat32(1.0f) + dt * kd + dt * ksd;
				dgFloat32 aRelErr = num / den;
				rhs[k].m_coordenateAccel = aRelErr + relCentr;
			}
		}
	} else {

		for (dgInt32 k = 0; k < params->m_rowsCount; k ++) {
			if (m_rowIsMotor & (1 << k)) {
				rhs[k].m_coordenateAccel = m_motorAcceleration[k] + rhs[k].m_deltaAccel;
			} else {
				const dgJacobianPair& Jt = row[k].m_Jt;
				dgVector relVeloc (Jt.m_jacobianM0.m_linear * bodyVeloc0 + Jt.m_jacobianM0.m_angular * bodyOmega0 +
								   Jt.m_jacobianM1.m_linear * bodyVeloc1 + Jt.m_jacobianM1.m_angular * bodyOmega1);

				dgFloat32 vRel = relVeloc.m_x + relVeloc.m_y + relVeloc.m_z;
				rhs[k].m_coordenateAccel = rhs[k].m_deltaAccel - vRel;
			}
		}
	}
}

