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
#include "dCustom6dof.h"


IMPLEMENT_CUSTOM_JOINT(dCustom6dof);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dCustom6dof::dCustom6dof (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent)
	,m_minLinearLimits(0.0f)
	,m_maxLinearLimits(0.0f)
	,m_pitch()
	,m_yaw()
	,m_roll()
{
	m_options.m_value = 0x3f;
	CalculateLocalMatrix (pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
}

dCustom6dof::dCustom6dof (const dMatrix& pinAndPivotChildFrame, const dMatrix& pinAndPivotParentFrame,  NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent)
	,m_minLinearLimits(0.0f)
	,m_maxLinearLimits(0.0f)
	,m_pitch()
	,m_yaw()
	,m_roll()
{
	dMatrix dummy;
	m_options.m_value = 0x3f;
	CalculateLocalMatrix(pinAndPivotChildFrame, m_localMatrix0, dummy);
	CalculateLocalMatrix(pinAndPivotParentFrame, dummy, m_localMatrix1);
}

dCustom6dof::~dCustom6dof()
{
}

void dCustom6dof::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &m_minLinearLimits, sizeof(m_minLinearLimits));
	callback(userData, &m_maxLinearLimits, sizeof(m_maxLinearLimits));
	callback(userData, &m_yaw, sizeof(m_yaw));
	callback(userData, &m_roll, sizeof(m_roll));
	callback(userData, &m_pitch, sizeof(m_pitch));
}

void dCustom6dof::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
	callback(userData, &m_minLinearLimits, sizeof (m_minLinearLimits));
	callback(userData, &m_maxLinearLimits, sizeof (m_maxLinearLimits));
	callback(userData, &m_yaw, sizeof(m_yaw));
	callback(userData, &m_roll, sizeof(m_roll));
	callback(userData, &m_pitch, sizeof(m_pitch));
}

void dCustom6dof::DisableAxisX()
{
	dAssert(0);
//	m_xAxis = 0;
}

void dCustom6dof::DisableAxisY()
{
	dAssert(0);
//	m_yAxis = 0;
}

void dCustom6dof::DisableAxisZ()
{
	dAssert(0);
//	m_zAxis = 0;
}

void dCustom6dof::DisableRotationX()
{
	dAssert(0);
//	m_pitchAxis = 0;
}

void dCustom6dof::DisableRotationY()
{
	dAssert(0);
//	m_yawAxis = 0;
}

void dCustom6dof::DisableRotationZ()
{
	dAssert(0);
//	m_rollAxis = 0;
}

void dCustom6dof::SetLinearLimits (const dVector& minLinearLimits, const dVector& maxLinearLimits)
{
	for (int i = 0; i < 3; i ++) {
		m_minLinearLimits[i] =  (dAbs (minLinearLimits[i]) < dFloat (1.0e-3f)) ? dFloat(0.0f) : -dAbs (minLinearLimits[i]);
		m_maxLinearLimits[i] =  (dAbs (maxLinearLimits[i]) < dFloat (1.0e-3f)) ? dFloat(0.0f) :  dAbs (maxLinearLimits[i]);
	}
}

void dCustom6dof::GetLinearLimits (dVector& minLinearLimits, dVector& maxLinearLimits) const
{
	minLinearLimits = m_minLinearLimits;
	maxLinearLimits = m_maxLinearLimits;
}

void dCustom6dof::SetYawLimits(dFloat minAngle, dFloat maxAngle)
{
	m_yaw.m_maxAngle = dAbs(maxAngle);
	m_yaw.m_minAngle = -dAbs(minAngle);
}

void dCustom6dof::SetRollLimits(dFloat minAngle, dFloat maxAngle)
{
	m_roll.m_maxAngle = dAbs(maxAngle);
	m_roll.m_minAngle = -dAbs(minAngle);
}

void dCustom6dof::SetPitchLimits(dFloat minAngle, dFloat maxAngle)
{
	m_pitch.m_maxAngle = dAbs(maxAngle);
	m_pitch.m_minAngle = -dAbs(minAngle);
}

void dCustom6dof::GetYawLimits(dFloat& minAngle, dFloat& maxAngle) const
{
	maxAngle = m_yaw.m_maxAngle;
	minAngle = m_yaw.m_minAngle;
}

void dCustom6dof::GetRollLimits(dFloat& minAngle, dFloat& maxAngle) const
{
	maxAngle = m_roll.m_maxAngle;
	minAngle = m_roll.m_minAngle;
}

void dCustom6dof::GetPitchLimits(dFloat& minAngle, dFloat& maxAngle) const
{
	maxAngle = m_pitch.m_maxAngle;
	minAngle = m_pitch.m_minAngle;
}


void dCustom6dof::CalculateJointAngles(const dMatrix& matrix0, const dMatrix& matrix1)
{
	dMatrix localMatrix(matrix0 * matrix1.Inverse());
	dVector euler0;
	dVector euler1;
	localMatrix.GetEulerAngles(euler0, euler1, m_pitchRollYaw);

#if 0
	dMatrix matrix1_ (dGetIdentityMatrix());
	dMatrix matrix0_ (dPitchMatrix(30.0f * dDegreeToRad) * dRollMatrix(100.0f * dDegreeToRad) * dYawMatrix(50.0f * dDegreeToRad) * matrix1_);
	dMatrix localMatrix_(matrix0_ * matrix1_.Inverse());
	localMatrix_.GetEulerAngles(euler0, euler1, m_pitchRollYaw);
#endif

	// deal with gimbals lock
	if (euler0.m_z > 89.5f * dDegreeToRad) {
//		dAssert(0);
	} else if (euler0.m_z < -89.5f * dDegreeToRad) {
//		dAssert(0);
	}

	// deal with roll angle flip
	dAngularIntegration deltaYaw(dAngularIntegration(euler0.m_y) - m_yaw.m_currentAngle);
	dAngularIntegration deltaPitch(dAngularIntegration(euler0.m_x) - m_pitch.m_currentAngle);
	if ((dAbs(deltaYaw.GetAngle()) > (0.5f * dPi)) && (dAbs(deltaPitch.GetAngle()) > (0.5f * dPi))) {
		euler0 = euler1;
	}

	m_yaw.m_currentAngle.Update(euler0.m_y);
	m_roll.m_currentAngle.Update(euler0.m_z);
	m_pitch.m_currentAngle.Update(euler0.m_x);
}

void dCustom6dof::Debug(dDebugDisplay* const debugDisplay) const
{
	dMatrix matrix0;
	dMatrix matrix1;

	dCustomJoint::Debug(debugDisplay);

	const int subdiv = 12;
	dVector arch[subdiv + 1];
	const dFloat radius = debugDisplay->m_debugScale;

	CalculateGlobalMatrix(matrix0, matrix1);

	{
		// show yaw angle limits
		if ((m_yaw.m_maxAngle > 1.0e-3f) || (m_yaw.m_minAngle < -1.0e-3f)) {
			dVector point(dFloat(radius), dFloat(0.0f), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = dClamp(m_yaw.m_minAngle, dFloat(-180.0f) * dDegreeToRad, dFloat(0.0f) * dDegreeToRad);
			dFloat maxAngle = dClamp(m_yaw.m_maxAngle, dFloat(0.0f) * dDegreeToRad, dFloat(180.0f) * dDegreeToRad);

			dFloat angleStep = (maxAngle - minAngle) / subdiv;
			dFloat angle0 = minAngle;

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

	matrix1 = dYawMatrix(GetYaw()) * matrix1;
	{
		// show roll angle limits
		if ((m_roll.m_maxAngle > 1.0e-3f) || (m_roll.m_minAngle < -1.0e-3f)) {
			dVector point(dFloat(radius), dFloat(0.0f), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = dClamp(m_roll.m_minAngle, dFloat(-180.0f) * dDegreeToRad, dFloat(0.0f) * dDegreeToRad);
			dFloat maxAngle = dClamp(m_roll.m_maxAngle, dFloat(0.0f) * dDegreeToRad, dFloat(180.0f) * dDegreeToRad);

			dFloat angleStep = (maxAngle - minAngle) / subdiv;
			dFloat angle0 = minAngle;
			
			debugDisplay->SetColor(dVector(0.0f, 0.0f, 0.5f, 0.0f));
			for (int i = 0; i <= subdiv; i++) {
				arch[i] = matrix1.TransformVector(dRollMatrix(angle0).RotateVector(point));
				debugDisplay->DrawLine(matrix1.m_posit, arch[i]);
				angle0 += angleStep;
			}

			for (int i = 0; i < subdiv; i++) {
				debugDisplay->DrawLine(arch[i], arch[i + 1]);
			}
		}
	}

	matrix1 = dRollMatrix(GetRoll()) * matrix1;
	{
		if ((m_pitch.m_maxAngle > 1.0e-3f) || (m_pitch.m_minAngle < -1.0e-3f)) {
			// show pitch angle limits
			dVector point(dFloat(0.0f), dFloat(radius), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = dClamp(m_pitch.m_minAngle, dFloat(-180.0f) * dDegreeToRad, dFloat(0.0f) * dDegreeToRad) + dFloat(0.0f) * dPi;
			dFloat maxAngle = dClamp(m_pitch.m_maxAngle, dFloat(0.0f) * dDegreeToRad, dFloat(180.0f) * dDegreeToRad) + dFloat(0.0f) * dPi;

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


void dCustom6dof::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector veloc0;
	dVector veloc1;
	dVector omega0;
	dVector omega1;

	const dFloat invtimestep = 1.0f / timestep;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);

	dAssert(m_body0);
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetPointVelocity(m_body0, &matrix0.m_posit[0], &veloc0[0]);
	if (m_body1) {
		NewtonBodyGetOmega(m_body1, &omega1[0]);
		NewtonBodyGetPointVelocity(m_body1, &matrix1.m_posit[0], &veloc1[0]);
	}

	// add the linear limits
	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix1.m_posit;
	const dVector dp(p0 - p1);
	const dVector veloc(veloc0 - veloc1);
	for (int i = 0; i < 3; i ++) {
		if (m_options.m_value & (1 << (i + 3))) {
			if ((m_minLinearLimits[i] == 0.0f) && (m_maxLinearLimits[i] == 0.0f)) {
				const dVector& dir = matrix1[i];
				dVector prejectPoint(p0 - dir.Scale(dir.DotProduct3(dp)));
				NewtonUserJointAddLinearRow(m_joint, &p0[0], &prejectPoint[0], &dir[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			} else {
				dFloat posit = dp.DotProduct3(matrix1[i]);
				dFloat speed = veloc.DotProduct3(matrix1[i]);
				dFloat x = posit + speed * timestep;
				if (x < m_minLinearLimits[i]) {
					NewtonUserJointAddLinearRow(m_joint, &matrix1.m_posit[0], &matrix1.m_posit[0], &matrix1[i][0]);
					NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
					NewtonUserJointSetRowMinimumFriction(m_joint, 0.0f);

					speed = 0.5f * (m_minLinearLimits[i] - posit) * invtimestep;
					const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
					NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

				} else if (x > m_maxLinearLimits[i]) {
					NewtonUserJointAddLinearRow(m_joint, &matrix1.m_posit[0], &matrix1.m_posit[0], &matrix1[i][0]);
					NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
					NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);

					speed = 0.5f * (m_maxLinearLimits[i] - posit) * invtimestep;
					const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
					NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
				}
			}
		}
	}

	// update joint angle
	CalculateJointAngles(matrix0, matrix1);
	const dVector omega(omega0 - omega1);

	if (m_options.m_option3) {
		const dFloat pitchAngle = GetPitch();
		if ((m_pitch.m_minAngle == 0.0f) && (m_pitch.m_maxAngle == 0.0f)) {
			NewtonUserJointAddAngularRow(m_joint, -pitchAngle, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		} else {
			dFloat jointOmega = omega.DotProduct3(matrix0.m_front);
			dFloat projectAngle = pitchAngle + jointOmega * timestep;
			if (projectAngle < m_pitch.m_minAngle) {
				NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMinimumFriction(m_joint, 0.0f);

				const dFloat speed = 0.5f * (m_pitch.m_minAngle - pitchAngle) * invtimestep;
				const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
				NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
			} else if (projectAngle > m_pitch.m_maxAngle) {
				NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);

				const dFloat speed = 0.5f * (m_pitch.m_maxAngle - pitchAngle) * invtimestep;
				const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
				NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
			}
		}
	}

	if (m_options.m_option4) {
		dFloat yawAngle = GetYaw();
		if ((m_yaw.m_minAngle == 0.0f) && (m_yaw.m_maxAngle == 0.0f)) {
			NewtonUserJointAddAngularRow(m_joint, -yawAngle, &matrix1.m_up[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		} else {
			if (yawAngle > m_yaw.m_maxAngle) {
				NewtonUserJointAddAngularRow(m_joint, m_yaw.m_maxAngle - yawAngle, &matrix1.m_up[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);
			} else if (yawAngle < m_yaw.m_minAngle) {
				NewtonUserJointAddAngularRow(m_joint, m_yaw.m_minAngle - yawAngle, &matrix1.m_up[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMinimumFriction(m_joint, 0.0f);
			}
		}
	}

	if (m_options.m_option5) {
		matrix1 = dYawMatrix(GetYaw()) * matrix1;
		dFloat rollAngle = GetRoll();
		if ((m_roll.m_minAngle == 0.0f) && (m_roll.m_maxAngle == 0.0f)) {
			NewtonUserJointAddAngularRow(m_joint, -rollAngle, &matrix1.m_right[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		} else {
			if (rollAngle > m_roll.m_maxAngle) {
				NewtonUserJointAddAngularRow(m_joint, m_roll.m_maxAngle - rollAngle, &matrix1.m_right[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);
			} else if (rollAngle < m_roll.m_minAngle) {
				NewtonUserJointAddAngularRow(m_joint, m_roll.m_minAngle - rollAngle, &matrix1.m_right[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointSetRowMinimumFriction(m_joint, 0.0f);
			}
		}
	}
}

