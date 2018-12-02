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



// dCustomMotor.cpp: implementation of the dCustomMotor class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomMotor.h"

IMPLEMENT_CUSTOM_JOINT(dCustomMotor);
IMPLEMENT_CUSTOM_JOINT(dCustomMotor2);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dCustomMotor::dCustomMotor(int dof, const dVector& pin, NewtonBody* const body, NewtonBody* const referenceBody)
	:dCustomJoint(dof, body, NULL)
	,m_motorOmega(0.0f)
	,m_targetSpeed(0.0f)
	,m_motorTorque(1.0f)
	,m_referenceBody(referenceBody)
{
	dMatrix matrix;
	NewtonBodyGetMatrix(m_referenceBody, &matrix[0][0]);
	m_localReferencePin = matrix.UnrotateVector(pin);
	SetSolverModel(2);
}

dCustomMotor::dCustomMotor(const dVector& pin, NewtonBody* const body, NewtonBody* const referenceBody)
	:dCustomJoint(1, body, NULL)
	,m_motorOmega(0.0f)
	,m_targetSpeed(0.0f)
	,m_motorTorque(1.0f)
	,m_referenceBody(referenceBody)
{
	dMatrix matrix;
	NewtonBodyGetMatrix(m_referenceBody, &matrix[0][0]);
	m_localReferencePin = matrix.UnrotateVector(pin);

	// set as kinematic loop
	SetSolverModel(2);
}

dCustomMotor::~dCustomMotor()
{
}

void dCustomMotor::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize(callback, userData);
	callback(userData, &m_localReferencePin, sizeof(dFloat));
	callback(userData, &m_motorOmega, sizeof(dFloat));
	callback(userData, &m_targetSpeed, sizeof(dFloat));
	callback(userData, &m_motorTorque, sizeof(dFloat));

	int refeBodyID = NewtonBodyGetSerializedID(m_referenceBody);
	callback(userData, &refeBodyID, sizeof(int));
}

void dCustomMotor::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	int refeBodyID;
	callback(userData, &m_localReferencePin, sizeof(dFloat));
	callback(userData, &m_motorOmega, sizeof(dFloat));
	callback (userData, &m_targetSpeed, sizeof (dFloat));
	callback (userData, &m_motorTorque, sizeof (dFloat));
	callback(userData, &refeBodyID, sizeof(int));

	NewtonWorld* const world = NewtonBodyGetWorld(GetBody0());
	m_referenceBody = NewtonFindSerializedBody(world, refeBodyID);
}


dFloat dCustomMotor::GetSpeed() const
{
	return m_targetSpeed;
}

void dCustomMotor::SetSpeed(dFloat speed)
{
	m_targetSpeed = speed;
}

void dCustomMotor::SetTorque(dFloat torque)
{
	m_motorTorque = dAbs(torque);
}

void dCustomMotor::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix;
	dVector omega(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	NewtonBodyGetMatrix(m_referenceBody, &matrix[0][0]);

	// calculate the angular velocity for both bodies
	dVector pin (matrix.RotateVector(m_localReferencePin));

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = pin.m_x;
	jacobian0[4] = pin.m_y;
	jacobian0[5] = pin.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = -pin.m_x;
	jacobian1[4] = -pin.m_y;
	jacobian1[5] = -pin.m_z;

	NewtonBodyGetOmega(m_body0, &omega[0]);
	m_motorOmega = omega.DotProduct3(pin);

	dFloat accel = (m_targetSpeed - m_motorOmega) / timestep;
	NewtonUserJointAddAngularRow(m_joint, 0.0f, &pin[0]);
	NewtonUserJointSetRowAcceleration(m_joint, accel);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	NewtonUserJointSetRowMinimumFriction(m_joint, -m_motorTorque);
	NewtonUserJointSetRowMaximumFriction(m_joint, m_motorTorque);
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dCustomMotor2::dCustomMotor2(const dVector& pin0, const dVector& pin1, NewtonBody* const body, NewtonBody* const referenceBody0, NewtonBody* const referenceBody1)
	:dCustomMotor(2, pin0, body, referenceBody0)
	,m_motorOmega1(0.0f)
	,m_targetSpeed1(0.0f)
	,m_motorTorque1(1.0f)
	,m_referenceBody1(referenceBody1)
{
	dMatrix matrix;
	NewtonBodyGetMatrix(m_referenceBody1, &matrix[0][0]);
	m_localReferencePin1 = matrix.UnrotateVector(pin1);

	// set as kinematic loop
	SetSolverModel(2);
}


dCustomMotor2::~dCustomMotor2()
{
}

void dCustomMotor2::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomMotor::Serialize(callback, userData);
	callback(userData, &m_localReferencePin1, sizeof(dFloat));
	callback(userData, &m_motorOmega1, sizeof(dFloat));
	callback(userData, &m_targetSpeed1, sizeof(dFloat));
	callback(userData, &m_motorTorque1, sizeof(dFloat));
	int refeBodyID1 = NewtonBodyGetSerializedID(m_referenceBody1);
	callback(userData, &refeBodyID1, sizeof(int));
}

void dCustomMotor2::Deserialize(NewtonDeserializeCallback callback, void* const userData)
{
	int refeBodyID1;
	callback(userData, &m_localReferencePin1, sizeof(dFloat));
	callback(userData, &m_motorOmega1, sizeof(dFloat));
	callback(userData, &m_targetSpeed1, sizeof(dFloat));
	callback(userData, &m_motorTorque1, sizeof(dFloat));
	callback(userData, &refeBodyID1, sizeof(int));

	NewtonWorld* const world = NewtonBodyGetWorld(GetBody0());
	m_referenceBody1 = NewtonFindSerializedBody(world, refeBodyID1);
}


dFloat dCustomMotor2::GetSpeed1() const
{
	return m_targetSpeed1;
}

void dCustomMotor2::SetSpeed1(dFloat speed)
{
	m_targetSpeed1 = speed;
}

void dCustomMotor2::SetTorque1(dFloat torque)
{
	m_motorTorque1 = dAbs(torque);
}


void dCustomMotor2::SubmitConstraints(dFloat timestep, int threadIndex)
{
	dMatrix matrix;
	dVector omega(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	dCustomMotor::SubmitConstraints(timestep, threadIndex);

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	NewtonBodyGetMatrix(m_referenceBody1, &matrix[0][0]);

	// calculate the angular velocity for both bodies
	dVector pin(matrix.RotateVector(m_localReferencePin1));

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = pin.m_x;
	jacobian0[4] = pin.m_y;
	jacobian0[5] = pin.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = -pin.m_x;
	jacobian1[4] = -pin.m_y;
	jacobian1[5] = -pin.m_z;

	NewtonBodyGetOmega(m_body0, &omega[0]);
	m_motorOmega1 = omega.DotProduct3(pin);

	dFloat accel = (m_targetSpeed1 - m_motorOmega1) / timestep;
	NewtonUserJointAddAngularRow(m_joint, 0.0f, &pin[0]);
	NewtonUserJointSetRowAcceleration(m_joint, accel);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	NewtonUserJointSetRowMinimumFriction(m_joint, -m_motorTorque1);
	NewtonUserJointSetRowMaximumFriction(m_joint, m_motorTorque1);
}

