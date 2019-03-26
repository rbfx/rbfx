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


// dCustomHinge.cpp: implementation of the dCustomHinge class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomHinge.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_CUSTOM_JOINT(dCustomHinge);

dCustomHinge::dCustomHinge(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent)
	,m_curJointAngle()
	,m_minAngle(-45.0f * dDegreeToRad)
	,m_maxAngle(45.0f * dDegreeToRad)
	,m_friction(0.0f)
	,m_jointOmega(0.0f)
	,m_spring(0.0f)
	,m_damper(0.0f)
	,m_motorSpeed(0.0f)
	,m_springDamperRelaxation(0.9f)
	,m_limitReached(false)
{
	// calculate the two local matrix of the pivot point
	CalculateLocalMatrix (pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
}

dCustomHinge::dCustomHinge (const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent)
	,m_curJointAngle()
	,m_minAngle(-45.0f * dDegreeToRad)
	,m_maxAngle(45.0f * dDegreeToRad)
	,m_friction(0.0f)
	,m_jointOmega(0.0f)
	,m_spring(0.0f)
	,m_damper(0.0f)
	,m_motorSpeed(0.0f)
	,m_springDamperRelaxation(0.9f)
	,m_limitReached(false)
{
	dMatrix	dummy;
	CalculateLocalMatrix (pinAndPivotFrameChild, m_localMatrix0, dummy);
	CalculateLocalMatrix (pinAndPivotFrameParent, dummy, m_localMatrix1);
}

dCustomHinge::~dCustomHinge()
{
}

void dCustomHinge::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	m_limitReached = false;
	callback(userData, &m_curJointAngle, sizeof(dAngularIntegration));
	callback (userData, &m_minAngle, sizeof (dFloat));
	callback (userData, &m_maxAngle, sizeof (dFloat));
	callback (userData, &m_friction, sizeof (dFloat));
	callback (userData, &m_jointOmega, sizeof (dFloat));
	callback (userData, &m_spring, sizeof (dFloat));
	callback (userData, &m_damper, sizeof (dFloat));
	callback (userData, &m_motorSpeed, sizeof(dFloat));
	callback (userData, &m_springDamperRelaxation, sizeof (dFloat));
}

void dCustomHinge::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
	callback(userData, &m_curJointAngle, sizeof(dAngularIntegration));
	callback (userData, &m_minAngle, sizeof (dFloat));
	callback (userData, &m_maxAngle, sizeof (dFloat));
	callback (userData, &m_friction, sizeof (dFloat));
	callback (userData, &m_jointOmega, sizeof (dFloat));
	callback(userData, &m_spring, sizeof (dFloat));
	callback(userData, &m_damper, sizeof (dFloat));
	callback(userData, &m_motorSpeed, sizeof(dFloat));
	callback(userData, &m_springDamperRelaxation, sizeof (dFloat));
}

void dCustomHinge::EnableLimits(bool state)
{
	m_options.m_option0 = state;
}

void dCustomHinge::SetLimits(dFloat minAngle, dFloat maxAngle)
{
	m_minAngle = -dAbs (minAngle);
	m_maxAngle = dAbs (maxAngle);
}

void dCustomHinge::EnableMotor(bool state, dFloat motorSpeed)
{
	m_options.m_option2 = state;
	m_motorSpeed = motorSpeed;
}

void dCustomHinge::SetAsSpringDamper(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper)
{
	m_spring = spring;
	m_damper = damper;
	m_options.m_option1 = state;
	m_springDamperRelaxation = dClamp(springDamperRelaxation, dFloat(0.0f), dFloat(0.999f));
}

dFloat dCustomHinge::GetJointAngle () const
{
	return m_curJointAngle.GetAngle();
}

dVector dCustomHinge::GetPinAxis () const
{
	dMatrix matrix;
	NewtonBodyGetMatrix (m_body0, &matrix[0][0]);
	return matrix.RotateVector (m_localMatrix0.m_front);
}

dFloat dCustomHinge::GetJointOmega () const
{
	return m_jointOmega;
}

void dCustomHinge::SetFriction (dFloat frictionTorque)
{
	m_friction = dAbs (frictionTorque);
}

dFloat dCustomHinge::GetFriction () const
{
	return m_friction;
}

void dCustomHinge::Debug(dDebugDisplay* const debugDisplay) const
{
	dMatrix matrix0;
	dMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);
	debugDisplay->DrawFrame(matrix0);
	debugDisplay->DrawFrame(matrix1);

	if (m_options.m_option0) {

		const int subdiv = 12;
		dVector arch[subdiv + 1];
		const dFloat radius = debugDisplay->m_debugScale;

		if ((m_maxAngle > 1.0e-3f) || (m_minAngle < -1.0e-3f)) {
			// show pitch angle limits
			dVector point(dFloat(0.0f), dFloat(radius), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = m_minAngle;
			dFloat maxAngle = m_maxAngle;
			if ((maxAngle - minAngle) >= dPi * 2.0f) {
				minAngle = 0.0f;
				maxAngle = dPi * 2.0f;
			}

			dFloat angleStep = (maxAngle - minAngle) / subdiv;
			dFloat angle0 = minAngle;

			matrix1.m_posit = matrix0.m_posit;
			debugDisplay->SetColor(dVector(0.5f, 0.0f, 0.0f, 0.0f));
			for (int i = 0; i <= subdiv; i++) {
				arch[i] = matrix1.TransformVector(dPitchMatrix(angle0).RotateVector(point));
				debugDisplay->DrawLine(matrix1.m_posit, arch[i]);
				angle0 += angleStep;
			}

			for (int i = 0; i < subdiv; i++) {
				debugDisplay->DrawLine(arch[i], arch[i + 1]);
			}
		}
	}
}

void dCustomHinge::SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	NewtonUserJointAddAngularRow(m_joint, -m_curJointAngle.GetAngle(), &matrix0.m_front[0]);
	NewtonUserJointSetRowSpringDamperAcceleration(m_joint, m_springDamperRelaxation, m_spring, m_damper);
}

void dCustomHinge::SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	m_limitReached = true;
	if ((m_minAngle > -1.e-4f) && (m_maxAngle < 1.e-4f)) {
		//dFloat angle = CalculateAngle (matrix1.m_up, matrix0.m_up, matrix1.m_front);
		dFloat angle = m_curJointAngle.GetAngle();
		NewtonUserJointAddAngularRow(m_joint, -angle, &matrix1.m_front[0]);
		NewtonUserJointSetRowStiffness(m_joint, 1.0f);
	} else {
		dFloat angle = m_curJointAngle.GetAngle() + m_jointOmega * timestep;
		if (angle < m_minAngle) {
			NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);

			const dFloat invtimestep = 1.0f / timestep;
			const dFloat speed = 0.5f * (m_minAngle - m_curJointAngle.GetAngle()) * invtimestep;
			const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
			NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
		} else if (angle > m_maxAngle) {
			NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);

			const dFloat invtimestep = 1.0f / timestep;
			const dFloat speed = 0.5f * (m_maxAngle - m_curJointAngle.GetAngle()) * invtimestep;
			const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
			NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
		} else if (m_friction != 0.0f) {
			m_limitReached = false;
			NewtonUserJointAddAngularRow(m_joint, 0, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			NewtonUserJointSetRowAcceleration(m_joint, -m_jointOmega / timestep);
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);
		}
	}
}

void dCustomHinge::SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dFloat angle = m_curJointAngle.GetAngle() + m_jointOmega * timestep;
	if (angle < m_minAngle) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_minAngle - m_curJointAngle.GetAngle()) * invtimestep;
		const dFloat springAccel = NewtonCalculateSpringDamperAcceleration(timestep, m_spring, m_curJointAngle.GetAngle(), m_damper, m_jointOmega);
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep + springAccel;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

	} else if (angle > m_maxAngle) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_maxAngle - m_curJointAngle.GetAngle()) * invtimestep;
		const dFloat springAccel = NewtonCalculateSpringDamperAcceleration(timestep, m_spring, m_curJointAngle.GetAngle(), m_damper, m_jointOmega);
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep + springAccel;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

	} else {
		SubmitConstraintSpringDamper(matrix0, matrix1, timestep);
	}
}

void dCustomHinge::SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& euler, dFloat timestep)
{
	// two rows to restrict rotation around around the parent coordinate system
	NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up), &matrix1.m_up[0]);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
}

void dCustomHinge::SubmitConstraints(dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	// Restrict the movement on the pivot point along all two orthonormal axis direction perpendicular to the motion
	SubmitLinearRows(0x07, matrix0, matrix1);

	dMatrix localMatrix(matrix0 * matrix1.Inverse());
	dVector euler0;
	dVector euler1;
	localMatrix.GetEulerAngles(euler0, euler1, m_pitchRollYaw);
//	localMatrix.GetEulerAngles(euler0, euler1, m_pitchYawRoll);

	// the joint angle can be determined by getting the angle between any two non parallel vectors
	m_curJointAngle.Update(euler0.m_x);

	// save the current joint Omega
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	if (m_body1) {
		NewtonBodyGetOmega(m_body1, &omega1[0]);
	}
	m_jointOmega = matrix0.m_front.DotProduct3(omega0 - omega1);

	// submit the angular rows.
	SubmitAngularRow(matrix0, matrix1, euler0, timestep);

	if (m_options.m_option2) {
		// the joint is motor
		dFloat accel = (m_motorSpeed - m_jointOmega) / timestep;
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
		NewtonUserJointSetRowAcceleration(m_joint, accel);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);
	} else {
		// the joint is not motor
		if (m_options.m_option0) {
			if (m_options.m_option1) {
				SubmitConstraintLimitSpringDamper(matrix0, matrix1, timestep);
			} else {
				SubmitConstraintLimits(matrix0, matrix1, timestep);
			}
		} else if (m_options.m_option1) {
			SubmitConstraintSpringDamper(matrix0, matrix1, timestep);
		} else if (m_friction != 0.0f) {
			NewtonUserJointAddAngularRow(m_joint, 0, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			NewtonUserJointSetRowAcceleration(m_joint, -m_jointOmega / timestep);
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);
		}
	}
}
