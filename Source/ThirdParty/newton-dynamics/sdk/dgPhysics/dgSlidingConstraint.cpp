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
#include "dgSlidingConstraint.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgSlidingConstraint::dgSlidingConstraint ()
	:dgBilateralConstraint() 
{
	dgAssert ((((dgUnsigned64) &m_localMatrix0) & 15) == 0);

	m_localMatrix0 = dgGetIdentityMatrix();
	m_localMatrix1 = dgGetIdentityMatrix();

	m_maxDOF = 6;
	m_constId = m_sliderConstraint;
	m_posit = dgFloat32 (0.0f);
	m_jointAccelFnt = NULL;
}

dgSlidingConstraint::~dgSlidingConstraint ()
{
}

/*
dgSlidingConstraint* dgSlidingConstraint::Create(dgWorld* world)
{
	dgSlidingConstraint* constraint;

	dgSlidingConstraintArray& array = *world;
//	constraint = dgSlidingConstraintArray::GetPool().GetElement();
	constraint = array.GetElement();

	dgAssert ((((dgUnsigned64) &constraint->m_localMatrix0) & 15) == 0);

	constraint->Init ();
	constraint->m_maxDOF = 6;
	constraint->m_constId = dgSliderConstraintId;
	constraint->m_posit = dgFloat32 (0.0f);
	constraint->m_jointAccelFnt = NULL;
	return constraint;
}

void dgSlidingConstraint::Remove(dgWorld* world)
{
	dgSlidingConstraintArray& array = *world;
	dgBilateralConstraint::Remove (world);
//	dgSlidingConstraintArray::GetPool().RemoveElement (this);
	array.RemoveElement (this);
}
*/

void dgSlidingConstraint::SetJointParameterCallback (dgSlidingJointAcceleration callback)
{
	m_jointAccelFnt = callback;
}

dgFloat32 dgSlidingConstraint::GetJointPosit () const
{
	return m_posit;
}

dgFloat32 dgSlidingConstraint::GetJointVeloc () const
{
	dgAssert (m_body0);
	dgAssert (m_body1);
	dgVector dir (m_body0->GetMatrix().RotateVector (m_localMatrix0[0]));
	const dgVector& veloc0 = m_body0->GetVelocity();
	const dgVector& veloc1 = m_body1->GetVelocity();
	return dir.DotProduct(veloc0 - veloc1).GetScalar();
}

dgFloat32 dgSlidingConstraint::CalculateStopAccel (dgFloat32 distance, const dgJointCallbackParam* param) const
{
	dgFloat32 accel;
	dgFloat32 speed;
	dgFloat32 penetrationErr;

	accel = dgFloat32 (0.0f);
	if (m_posit > distance) {
		speed = GetJointVeloc ();
		if (speed < dgFloat32 (0.0f)) {
			speed = dgFloat32 (0.0f);
		}
		penetrationErr = (distance - m_posit); 
		accel = dgFloat32 (100.0f) * penetrationErr - speed * dgFloat32 (1.01f) / param->m_timestep;

	} else if (m_posit < distance) {
		speed = GetJointVeloc ();
		if (speed > dgFloat32 (0.0f)) {
			speed = dgFloat32 (0.0f);
		}
		penetrationErr = distance - m_posit; 
		dgAssert (penetrationErr >= dgFloat32 (0.0f));
		accel = dgFloat32 (100.0f) * penetrationErr - speed * dgFloat32 (1.01f) / param->m_timestep;
	} 
	return accel;
}

dgVector dgSlidingConstraint::GetJointForce () const
{
	dgMatrix matrix0;
	dgMatrix matrix1;

	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	return dgVector (matrix0.m_up.Scale (m_jointForce[0].m_force) + 
		             matrix0.m_right.Scale (m_jointForce[1].m_force) + 
					 matrix0.m_up.Scale (m_jointForce[2].m_force) +
					 matrix0.m_right.Scale (m_jointForce[3].m_force) +
					 matrix0.m_right.Scale (m_jointForce[4].m_force));
}


dgUnsigned32 dgSlidingConstraint::JacobianDerivative (dgContraintDescritor& params)
{
	dgMatrix matrix0;
	dgMatrix matrix1;

	//dgVector angle (CalculateGlobalMatrixAndAngle (matrix0, matrix1));
	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	m_posit = matrix0.m_front.DotProduct(matrix0.m_posit - matrix1.m_posit).GetScalar();
	matrix1.m_posit += matrix1.m_front.Scale (m_posit);

	dgAssert (dgAbs (dgFloat32 (1.0f) - matrix0.m_front.DotProduct(matrix0.m_front).GetScalar()) < dgFloat32 (1.0e-5f)); 
	dgAssert (dgAbs (dgFloat32 (1.0f) - matrix0.m_up.DotProduct(matrix0.m_up).GetScalar()) < dgFloat32 (1.0e-5f)); 
	dgAssert (dgAbs (dgFloat32 (1.0f) - matrix0.m_right.DotProduct(matrix0.m_right).GetScalar()) < dgFloat32 (1.0e-5f)); 

	const dgVector& dir1 = matrix0.m_up;
	const dgVector& dir2 = matrix0.m_right;

	dgVector p0 (matrix0.m_posit);
	dgVector p1 (matrix1.m_posit + matrix1.m_front.Scale (matrix1.m_front.DotProduct(p0 - matrix1.m_posit).GetScalar())); 

	dgVector q0 (p0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dgVector q1 (p1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

	dgVector r0 (p0 + matrix0.m_up.Scale(MIN_JOINT_PIN_LENGTH));
	dgVector r1 (p1 + matrix1.m_up.Scale(MIN_JOINT_PIN_LENGTH));

	dgPointParam pointDataP;
	dgPointParam pointDataQ;
	dgPointParam pointDataR;
	InitPointParam (pointDataP, m_stiffness, p0, p1);
	InitPointParam (pointDataQ, m_stiffness, q0, q1);
	InitPointParam (pointDataR, m_stiffness, r0, r1);

	CalculatePointDerivative (0, params, dir1, pointDataP, &m_jointForce[0]); 
	CalculatePointDerivative (1, params, dir2, pointDataP, &m_jointForce[1]); 
	CalculatePointDerivative (2, params, dir1, pointDataQ, &m_jointForce[2]); 
	CalculatePointDerivative (3, params, dir2, pointDataQ, &m_jointForce[3]); 
	CalculatePointDerivative (4, params, dir2, pointDataR, &m_jointForce[4]); 

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

			CalculatePointDerivative (5, params, matrix0.m_front, pointDataP, &m_jointForce[5]); 
			SetMotorAcceleration (5, axisParam.m_accel, params);
			ret = 6;
		}
	}

	return dgUnsigned32 (ret);
}



