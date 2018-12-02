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

//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomWheel.h"


IMPLEMENT_CUSTOM_JOINT(dCustomWheel);

dCustomWheel::dCustomWheel(const dMatrix& pinAndPivotFrame, NewtonBody* child, NewtonBody* parent)
	:dCustomSlidingContact(pinAndPivotFrame, child, parent)
	,m_steerAngle(0.0f)
	,m_steerSpeed(60.0f * dDegreeToRad)
	,m_currentSteerAngle(0.0f)
{
}

dCustomWheel::dCustomWheel(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent)
	:dCustomSlidingContact(pinAndPivotFrameChild, pinAndPivotFrameParent, child, parent)
	,m_steerAngle(0.0f)
	,m_steerSpeed(60.0f * dDegreeToRad)
	,m_currentSteerAngle(0.0f)
{
}

dCustomWheel::~dCustomWheel()
{
}

void dCustomWheel::Deserialize(NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &m_steerAngle, sizeof(dFloat));
	callback(userData, &m_steerSpeed, sizeof(dFloat));
}

void dCustomWheel::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomSlidingContact::Serialize(callback, userData);

	callback(userData, &m_steerAngle, sizeof(dFloat));
	callback(userData, &m_steerSpeed, sizeof(dFloat));
}

dFloat dCustomWheel::GetSteerAngle() const
{
	return m_currentSteerAngle;
}

dFloat dCustomWheel::GetTargetSteerAngle() const
{
	return m_steerAngle;
}

void dCustomWheel::SetTargetSteerAngle(dFloat angle)
{
	m_steerAngle = angle;
}

dFloat dCustomWheel::GetSteerRate() const
{
	return m_steerSpeed;
}

void dCustomWheel::SetSteerRate(dFloat rate)
{
	m_steerSpeed = dAbs (m_steerSpeed);
}

void dCustomWheel::Debug(dDebugDisplay* const debugDisplay) const
{
	dCustomSlidingContact::Debug(debugDisplay);
/*
	if (m_options.m_option2) {
		dMatrix matrix0;
		dMatrix matrix1;
		CalculateGlobalMatrix(matrix0, matrix1);

		const int subdiv = 12;
		dVector arch[subdiv + 1];
		const dFloat radius = debugDisplay->m_debugScale;

		if ((m_maxAngle > 1.0e-3f) || (m_minAngle < -1.0e-3f)) {
			// show pitch angle limits
			dVector point(dFloat(radius), dFloat(0.0f), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = m_minAngle;
			dFloat maxAngle = m_maxAngle;
			if ((maxAngle - minAngle) >= dPi * 2.0f) {
				minAngle = 0.0f;
				maxAngle = dPi * 2.0f;
			}

			dFloat angleStep = (maxAngle - minAngle) / subdiv;
			dFloat angle0 = minAngle;

			matrix1.m_posit = matrix0.m_posit;
			debugDisplay->SetColor(dVector(0.0f, 0.5f, 0.0f, 0.0f));
			for (int i = 0; i <= subdiv; i++) {
				arch[i] = matrix1.TransformVector(dYawMatrix(angle0).RotateVector(point));
				debugDisplay->DrawLine(matrix1.m_posit, arch[i]);
				angle0 += angleStep;
			}

			for (int i = 0; i < subdiv; i++) {
				debugDisplay->DrawLine(arch[i], arch[i + 1]);
			}
		}
	}
*/
}

void dCustomWheel::SubmitAnglarStructuralRows(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dVector wheelFramePlane(matrix0.m_up - matrix1.m_front.Scale(matrix0.m_up.DotProduct3(matrix1.m_front)));
	wheelFramePlane = wheelFramePlane.Normalize();
	dVector wheelPin(matrix1.m_front.CrossProduct(wheelFramePlane));
	dFloat planeAngle = CalculateAngle(wheelFramePlane, matrix0.m_up, wheelPin);

	NewtonUserJointAddAngularRow(m_joint, -planeAngle, &wheelPin[0]);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

	const dFloat invTimeStep = 1.0f / timestep;
	const dFloat tol = m_steerSpeed * timestep;
	const dFloat angle = CalculateAngle(matrix1.m_up, wheelFramePlane, matrix1.m_front);
	m_currentSteerAngle = angle;

	dFloat currentSpeed = 0.0f;
	if (angle > (m_steerAngle + tol)) {
		currentSpeed = -m_steerSpeed;
		dFloat predictAngle = angle + currentSpeed * timestep;
		if (predictAngle < m_steerAngle) {
			currentSpeed = 0.5f * (m_steerAngle - angle) * invTimeStep;
		}
	} else if (angle < (m_steerAngle - tol)) {
		currentSpeed = m_steerSpeed;
		dFloat predictAngle = angle + currentSpeed * timestep;
		if (predictAngle > m_steerAngle) {
			currentSpeed = 0.5f * (m_steerAngle - angle) * invTimeStep;
		}
	}
	NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_front[0]);
	dFloat accel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + currentSpeed * invTimeStep;
	NewtonUserJointSetRowAcceleration(m_joint, accel);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
}