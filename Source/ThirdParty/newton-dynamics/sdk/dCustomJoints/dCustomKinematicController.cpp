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


// dCustomKinematicController.cpp: implementation of the dCustomKinematicController class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomKinematicController.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_CUSTOM_JOINT(dCustomKinematicController)

dCustomKinematicController::dCustomKinematicController(NewtonBody* const body, const dVector& handleInGlobalSpace)
	:dCustomJoint(6, body, NULL)
{
	dMatrix matrix;
	dAssert (GetBody0() == body);
	NewtonBodyGetMatrix (body, &matrix[0][0]);
	matrix.m_posit = handleInGlobalSpace;
	Init (body, matrix);
}

dCustomKinematicController::dCustomKinematicController (NewtonBody* const body, const dMatrix& attachmentMatrixInGlobalSpace)
	:dCustomJoint(6, body, NULL)
{
	Init (body, attachmentMatrixInGlobalSpace);
}

dCustomKinematicController::dCustomKinematicController(NewtonInverseDynamics* const invDynSolver, void* const invDynNode, const dMatrix& handleInGlobalSpace)
	:dCustomJoint(invDynSolver, invDynNode)
{
	Init(GetBody0(), handleInGlobalSpace);
}


void dCustomKinematicController::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	dAssert (0);
}

void dCustomKinematicController::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dAssert (0);
}


void dCustomKinematicController::ResetAutoSleep ()
{
	NewtonBodySetAutoSleep(GetBody0(), 0);
}

dCustomKinematicController::~dCustomKinematicController()
{
	NewtonBodySetAutoSleep (m_body0, m_autoSleepState);
}

void dCustomKinematicController::Init (NewtonBody* const body, const dMatrix& matrix)
{
	CalculateLocalMatrix(matrix, m_localMatrix0, m_localMatrix1);

	m_autoSleepState = NewtonBodyGetAutoSleep(body) ? true : false;
	NewtonBodySetSleepState(body, 0);

	SetPickMode(1);
	SetLimitRotationVelocity(10.0f);
	SetTargetMatrix(matrix);
	SetMaxLinearFriction(1.0f);
	SetMaxAngularFriction(1.0f);

	// set as soft joint
	SetSolverModel(3);
}

void dCustomKinematicController::SetPickMode (int mode)
{
	m_pickingMode = char (dClamp (mode, 0, 2));
}

void dCustomKinematicController::SetLimitRotationVelocity(dFloat omegaCap)
{
	m_omegaCap = dMax (omegaCap, dFloat (1.0f));
}

void dCustomKinematicController::SetMaxLinearFriction(dFloat frictionForce)
{
	m_maxLinearFriction = dAbs (frictionForce);
}

void dCustomKinematicController::SetMaxAngularFriction(dFloat frictionTorque)
{
	m_maxAngularFriction = dAbs (frictionTorque);
}


void dCustomKinematicController::SetTargetRotation(const dQuaternion& rotation)
{
	NewtonBodySetSleepState(m_body0, 0);
	m_targetMatrix = dMatrix(rotation, m_targetMatrix.m_posit);
}

void dCustomKinematicController::SetTargetPosit(const dVector& posit)
{
	NewtonBodySetSleepState(m_body0, 0);
	m_targetMatrix.m_posit = posit;
	m_targetMatrix.m_posit.m_w = 1.0f;
}

void dCustomKinematicController::SetTargetMatrix(const dMatrix& matrix)
{
	NewtonBodySetSleepState(m_body0, 0);
	m_targetMatrix = matrix;
	m_targetMatrix.m_posit.m_w = 1.0f; 
}

dMatrix dCustomKinematicController::GetTargetMatrix () const
{
	return m_targetMatrix;
}

dMatrix dCustomKinematicController::GetBodyMatrix () const
{
	dMatrix matrix0;
	NewtonBodyGetMatrix(m_body0, &matrix0[0][0]);
	return m_localMatrix0 * matrix0;
}

void dCustomKinematicController::Debug(dDebugDisplay* const debugDisplay) const
{
	debugDisplay->DrawFrame(GetBodyMatrix());
	debugDisplay->DrawFrame(m_targetMatrix);
}

void dCustomKinematicController::SubmitConstraints (dFloat timestep, int threadIndex)
{
	// check if this is an impulsive time step
	dMatrix matrix0(GetBodyMatrix());
	dVector omega(0.0f);
	dVector com(0.0f);
	dVector pointVeloc(0.0f);
	const dFloat damp = 0.3f;
	dAssert (timestep > 0.0f);
	const dFloat invTimestep = 1.0f / timestep;

	// we not longer cap excessive angular velocities, it is left to the client application. 
	NewtonBodyGetOmega(m_body0, &omega[0]);

	//cap excessive angular velocities
	dFloat mag2 = omega.DotProduct3(omega);
	if (mag2 > (m_omegaCap * m_omegaCap)) {
		omega = omega.Normalize().Scale(m_omegaCap);
		NewtonBodySetOmega(m_body0, &omega[0]);
	}

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	dVector relPosit(m_targetMatrix.m_posit - matrix0.m_posit);
	NewtonBodyGetPointVelocity(m_body0, &m_targetMatrix.m_posit[0], &pointVeloc[0]);

	for (int i = 0; i < 3; i ++) {
		// Restrict the movement on the pivot point along all tree orthonormal direction
		dFloat speed = pointVeloc.DotProduct3(m_targetMatrix[i]);
		dFloat dist = relPosit.DotProduct3(m_targetMatrix[i]) * damp;
		dFloat relSpeed = dist * invTimestep - speed;
		dFloat relAccel = relSpeed * invTimestep;
		NewtonUserJointAddLinearRow(m_joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &m_targetMatrix[i][0]);
		NewtonUserJointSetRowAcceleration(m_joint, relAccel);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_maxLinearFriction);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxLinearFriction);
	}	

	if (m_pickingMode == 1) {
		dQuaternion rotation (matrix0.Inverse() * m_targetMatrix);
		if (dAbs (rotation.m_w) < 0.99998f) {
			dMatrix rot (dGrammSchmidt(dVector (rotation.m_x, rotation.m_y, rotation.m_z, dFloat32 (0.0f))));
			dFloat angle = 2.0f * dAcos(dClamp(rotation.m_w, dFloat(-1.0f), dFloat(1.0f)));

			dFloat speed = omega.DotProduct3(rot[0]);
			dFloat relSpeed = angle * invTimestep - speed;
			dFloat relAccel = relSpeed * invTimestep;
			NewtonUserJointAddAngularRow (m_joint, angle, &rot.m_front[0]);
			NewtonUserJointSetRowAcceleration(m_joint, relAccel);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &rot.m_up[0]);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &rot.m_right[0]);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

		} else {
			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_front[0]);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);
		}

	} else {
		for (int i = 0; i < 3; i ++) {
			dFloat relSpeed = -omega.DotProduct3(matrix0[i]);
			dFloat relAccel = relSpeed * invTimestep;
			NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0[i][0]);
			NewtonUserJointSetRowAcceleration(m_joint, relAccel);
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_maxAngularFriction);
		}
	}
}



