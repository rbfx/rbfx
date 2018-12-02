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


#include "dCustomJointLibraryStdAfx.h"
#include "dCustomGear.h"

IMPLEMENT_CUSTOM_JOINT(dCustomGear);
IMPLEMENT_CUSTOM_JOINT(dCustomGearAndSlide);
IMPLEMENT_CUSTOM_JOINT(dCustomDifferentialGear);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dCustomGear::dCustomGear(int dof, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(dof, child, parent)
	,m_gearRatio(1.0f)
{
	// set as kinematic loop
	//SetSolverModel(1);
}


dCustomGear::dCustomGear(dFloat gearRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(1, child, parent)
	,m_gearRatio (gearRatio)
{
	dMatrix dommyMatrix;
	// calculate the local matrix for body body0
	dMatrix pinAndPivot0 (dGrammSchmidt(childPin));

	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);
	m_localMatrix0.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dGrammSchmidt(parentPin));
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);
	m_localMatrix1.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// set as kinematic loop
	//SetSolverModel(1);
}


dCustomGear::~dCustomGear()
{
}

void dCustomGear::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	m_gearRatio = 1.0f;
	// set as kinematic loop
	//SetSolverModel(1);

	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void dCustomGear::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void dCustomGear::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);

	// calculate the angular velocity for both bodies
	dVector dir0 = matrix0.m_front.Scale (m_gearRatio);
	const dVector& dir1 = matrix1.m_front;

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = dir0.m_x;
	jacobian0[4] = dir0.m_y;
	jacobian0[5] = dir0.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = dir1.m_x;
	jacobian1[4] = dir1.m_y;
	jacobian1[5] = dir1.m_z;

	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetOmega(m_body1, &omega1[0]);

	dFloat w0 = omega0.DotProduct3(dir0);
	dFloat w1 = omega1.DotProduct3(dir1);
	dFloat relOmega = w0 + w1;
	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep : 1.0f;
	dFloat relAccel = -0.5f * relOmega * invTimestep;
	NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
	NewtonUserJointSetRowAcceleration(m_joint, relAccel);
}

dCustomGearAndSlide::dCustomGearAndSlide (dFloat gearRatio, dFloat slideRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const child, NewtonBody* const parent)
	:dCustomGear(2, child, parent)
	,m_slideRatio (slideRatio)
{
	m_gearRatio = gearRatio;

	dMatrix dommyMatrix;
	// calculate the local matrix for body body0
	dMatrix pinAndPivot0 (dGrammSchmidt(childPin));
	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dGrammSchmidt(parentPin));
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);

	// set as kinematoc loop
	//SetSolverModel(1);
}

dCustomGearAndSlide::~dCustomGearAndSlide()
{
}

void dCustomGearAndSlide::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	// set as kinematic loop
	//SetSolverModel(1);

	callback (userData, &m_slideRatio, sizeof (dFloat));
}

void dCustomGearAndSlide::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomGear::Serialize (callback, userData);
	callback (userData, &m_slideRatio, sizeof (dFloat));
}

void dCustomGearAndSlide::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector omega0;
	dVector veloc1;
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);

	// calculate the angular velocity for both bodies
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetVelocity(m_body1, &veloc1[0]);

	// get angular velocity relative to the pin vector
	dFloat w0 = omega0.DotProduct3(matrix0.m_front);
	dFloat w1 = veloc1.DotProduct3(matrix1.m_front);

	// establish the gear equation.
	dFloat relVeloc = w0 + w1;

	// calculate the relative angular acceleration by dividing by the time step

	// ideally relative acceleration should be zero, but is practice there will always 
	// be a small difference in velocity that need to be compensated. 
	// using the full acceleration will make the to over show a oscillate 
	// so use only fraction of the acceleration

	// check if this is an impulsive time step
	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep: 1.0f;
	dFloat relAccel = - 0.3f * relVeloc * invTimestep;

	// set the linear part of Jacobian 0 to zero	
	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;	 
	jacobian0[2] = 0.0f;

	dVector dir0(matrix0.m_front.Scale (m_slideRatio));
	dVector dir1(matrix1.m_front);

	// set the angular part of Jacobian 0 pin vector		
	jacobian0[3] = dir0[0];
	jacobian0[4] = dir0[1];
	jacobian0[5] = dir0[2];

	// set the linear part of Jacobian 1 to translational pin vector	
	jacobian1[0] = dir1[0];
	jacobian1[1] = dir1[1];
	jacobian1[2] = dir1[2];

	// set the rotational part of Jacobian 1 to zero
	jacobian1[3] = 	0.0f;
	jacobian1[4] = 	0.0f;
	jacobian1[5] = 	0.0f;

	// add a angular constraint
	NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);

	// set the desired angular acceleration between the two bodies
	NewtonUserJointSetRowAcceleration (m_joint, relAccel);

	// add the angular relation constraint form the base class
	dCustomGear::SubmitConstraints (timestep, threadIndex);
}


dCustomDifferentialGear::dCustomDifferentialGear(dFloat gearRatio, const dVector& childPin, const dVector& parentPin, const dVector& referencePin, NewtonBody* const child, NewtonBody* const parent, NewtonBody* const referenceBody)
	:dCustomGear(gearRatio, childPin, parentPin, child, parent)
	,m_referenceBody(referenceBody)
{
	dMatrix referenceMatrix;
	NewtonBodyGetMatrix(m_referenceBody, &referenceMatrix[0][0]);
	m_referencePin = referenceMatrix.UnrotateVector(referencePin);

	// set as kinematic loop
	//SetSolverModel(1);
}

void dCustomDifferentialGear::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	// remember to get referenceBody from the world 
	int refeBodyID;
	callback(userData, &m_referencePin, sizeof(dVector));
	callback(userData, &refeBodyID, sizeof(int));
	NewtonWorld* const world = NewtonBodyGetWorld(GetBody0());
	m_referenceBody = NewtonFindSerializedBody(world, refeBodyID);

	// set as kinematic loop
//	SetSolverModel(1);
}

void dCustomDifferentialGear::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomGear::Serialize(callback, userData);
	int refeBodyID = NewtonBodyGetSerializedID(m_referenceBody);
	callback(userData, &m_referencePin, sizeof(dVector));
	callback(userData, &refeBodyID, sizeof(int));
}


void dCustomDifferentialGear::SubmitConstraints(dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space.
	CalculateGlobalMatrix(matrix0, matrix1);

	dVector dir2(m_referencePin);
	if (m_referenceBody) {
		dMatrix referenceMatrix;
		NewtonBodyGetMatrix(m_referenceBody, &referenceMatrix[0][0]);
		dir2 = referenceMatrix.RotateVector(dir2);
	}

	// calculate the angular velocity for both bodies
	const dVector& dir0 = matrix0.m_front.Scale(m_gearRatio);
	dVector dir1 (matrix1.m_front + dir2);
//dTrace(("%f %f %f\n", dir1[0], dir1[1], dir1[2]));

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = dir0.m_x;
	jacobian0[4] = dir0.m_y;
	jacobian0[5] = dir0.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = dir1.m_x;
	jacobian1[4] = dir1.m_y;
	jacobian1[5] = dir1.m_z;

	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetOmega(m_body1, &omega1[0]);

	dFloat w0 = omega0.DotProduct3(dir0);
	dFloat w1 = omega1.DotProduct3(dir1);

	dFloat relOmega = w0 + w1;
	dFloat invTimestep = 1.0f / timestep;
	dFloat relAccel = -0.5f * relOmega * invTimestep;
	NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
	NewtonUserJointSetRowAcceleration(m_joint, relAccel);
}

