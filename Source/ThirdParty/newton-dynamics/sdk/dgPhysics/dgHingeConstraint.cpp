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
#include "dgHingeConstraint.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgHingeConstraint::dgHingeConstraint ()
	:dgBilateralConstraint() 
{
	dgAssert ((((dgUnsigned64) &m_localMatrix0) & 15) == 0);
//	constraint->Init ();

	m_localMatrix0 = dgGetIdentityMatrix();
	m_localMatrix1 = dgGetIdentityMatrix();

	m_maxDOF = 6;
	m_jointAccelFnt = NULL;
	m_constId = m_hingeConstraint;
	m_angle = dgFloat32 (dgFloat32 (0.0f));
}

dgHingeConstraint::~dgHingeConstraint ()
{
}

/*
dgHingeConstraint* dgHingeConstraint::Create(dgWorld* world)
{
	dgHingeConstraint*	constraint;

	//constraint = dgHingeConstraintArray::GetPool().GetElement();
	dgHingeConstraintArray& array = *world;
	constraint = array.GetElement();

	dgAssert ((((dgUnsigned64) &constraint->m_localMatrix0) & 15) == 0);
	constraint->Init ();
	constraint->m_maxDOF = 6;
	constraint->m_constId = dgHingeConstraintId;

	constraint->m_angle = dgFloat32 (0.0f);
	constraint->m_jointAccelFnt = NULL;
	return constraint;
}

void dgHingeConstraint::Remove(dgWorld* world)
{
	dgHingeConstraintArray& array = *world;
	dgBilateralConstraint::Remove (world);
	//dgHingeConstraintArray::GetPool().RemoveElement (this);
	array.RemoveElement (this);
}
*/

void dgHingeConstraint::SetJointParameterCallback (dgHingeJointAcceleration callback)
{
	m_jointAccelFnt = callback;
}

dgFloat32 dgHingeConstraint::GetJointAngle () const
{
	return m_angle;
}

dgFloat32 dgHingeConstraint::GetJointOmega () const
{
	dgAssert (m_body0);
	dgAssert (m_body1);
	dgVector dir (m_body0->GetMatrix().RotateVector (m_localMatrix0[0]));
	const dgVector& omega0 = m_body0->GetOmega();
	const dgVector& omega1 = m_body1->GetOmega();
	return dir.DotProduct(omega0 - omega1).GetScalar();
}

dgFloat32 dgHingeConstraint::CalculateStopAlpha (dgFloat32 angle, const dgJointCallbackParam* param) const
{
	dgFloat32 alpha;
	dgFloat32 omega;
	dgFloat32 penetrationErr;

	alpha = dgFloat32 (0.0f);
	if (m_angle > angle) {
		omega = GetJointOmega ();
		if (omega < dgFloat32 (0.0f)) {
			omega = dgFloat32 (0.0f);
		}
		penetrationErr = (angle - m_angle); 
		alpha = dgFloat32 (100.0f) * penetrationErr - omega * dgFloat32 (1.01f) / param->m_timestep;

	} else if (m_angle < angle) {
		omega = GetJointOmega ();
		if (omega > dgFloat32 (0.0f)) {
			omega = dgFloat32 (0.0f);
		}

		penetrationErr = MIN_JOINT_PIN_LENGTH * (angle - m_angle); 
		alpha = dgFloat32 (100.0f) * penetrationErr - omega * dgFloat32 (1.01f) / param->m_timestep;
	} 
	return alpha;
}

dgVector dgHingeConstraint::GetJointForce () const
{
	dgMatrix matrix0;
	dgMatrix matrix1;

	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	return dgVector (matrix0.m_front.Scale (m_jointForce[0].m_force) + 
		             matrix0.m_up.Scale (m_jointForce[1].m_force) + 
					 matrix0.m_right.Scale (m_jointForce[2].m_force) +
					 matrix0.m_up.Scale (m_jointForce[3].m_force) +
					 matrix0.m_right.Scale (m_jointForce[4].m_force));
}


dgUnsigned32 dgHingeConstraint::JacobianDerivative (dgContraintDescritor& params)
{
	dgMatrix matrix0;
	dgMatrix matrix1;
	dgVector angle (CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1));

	m_angle = -angle.m_x;

	dgAssert (dgAbs (1.0f - matrix0.m_front.DotProduct(matrix0.m_front).GetScalar()) < dgFloat32 (1.0e-5f)); 
	dgAssert (dgAbs (1.0f - matrix0.m_up.DotProduct(matrix0.m_up).GetScalar()) < dgFloat32 (1.0e-5f)); 
	dgAssert (dgAbs (1.0f - matrix0.m_right.DotProduct(matrix0.m_right).GetScalar()) < dgFloat32 (1.0e-5f)); 

	const dgVector& dir0 = matrix0.m_front;
	const dgVector& dir1 = matrix0.m_up;
	const dgVector& dir2 = matrix0.m_right;

	const dgVector& p0 = matrix0.m_posit;
	const dgVector& p1 = matrix1.m_posit;
	dgVector q0 (p0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dgVector q1 (p1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

//	dgAssert (((p1 - p0) % (p1 - p0)) < 1.0e-2f);

	dgPointParam pointDataP;
	dgPointParam pointDataQ;
	InitPointParam (pointDataP, m_stiffness, p0, p1);
	InitPointParam (pointDataQ, m_stiffness, q0, q1);

	CalculatePointDerivative (0, params, dir0, pointDataP, &m_jointForce[0]); 
	CalculatePointDerivative (1, params, dir1, pointDataP, &m_jointForce[1]); 
	CalculatePointDerivative (2, params, dir2, pointDataP, &m_jointForce[2]); 
	CalculatePointDerivative (3, params, dir1, pointDataQ, &m_jointForce[3]); 
	CalculatePointDerivative (4, params, dir2, pointDataQ, &m_jointForce[4]); 

	dgInt32 ret = 5;
	if (m_jointAccelFnt) {
		dgJointCallbackParam axisParam;
		axisParam.m_accel = dgFloat32 (0.0f);
		axisParam.m_timestep = params.m_timestep;
		axisParam.m_minFriction = DG_MIN_BOUND;
		axisParam.m_maxFriction = DG_MAX_BOUND;

		if (m_jointAccelFnt (*this, &axisParam)) {
			if ((axisParam.m_minFriction > DG_MIN_BOUND) || (axisParam.m_maxFriction < DG_MAX_BOUND)) {
				params.m_forceBounds[5].m_low = axisParam.m_minFriction;
				params.m_forceBounds[5].m_upper = axisParam.m_maxFriction;
				params.m_forceBounds[5].m_normalIndex = DG_INDEPENDENT_ROW;
			}

			CalculateAngularDerivative (5, params, dir0, m_stiffness, dgFloat32 (0.0f), &m_jointForce[5]);
			SetMotorAcceleration (5, axisParam.m_accel, params);
			ret = 6;
		}
	}

	return dgUnsigned32 (ret);
}



