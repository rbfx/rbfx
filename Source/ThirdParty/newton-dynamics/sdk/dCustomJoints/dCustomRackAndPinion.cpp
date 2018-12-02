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

// CustomWormGear.cpp: implementation of the CustomWormGear class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomRackAndPinion.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_CUSTOM_JOINT(dCustomRackAndPinion);

dCustomRackAndPinion::dCustomRackAndPinion(dFloat gearRatio, const dVector& rotationalPin, const dVector& linearPin, NewtonBody* rotationalBody, NewtonBody* linearBody)
	:dCustomJoint(2, rotationalBody, linearBody)
{
	m_gearRatio = gearRatio;

	dMatrix dommyMatrix;

	dMatrix pinAndPivot0 (dGrammSchmidt (rotationalPin));
	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);
	m_localMatrix0.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dGrammSchmidt(linearPin));
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);
	m_localMatrix1.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// set as kinematoc loop
	SetSolverModel(1);
}

dCustomRackAndPinion::~dCustomRackAndPinion()
{
}

void dCustomRackAndPinion::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void dCustomRackAndPinion::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
	callback (userData, &m_gearRatio, sizeof (dFloat));
}


void dCustomRackAndPinion::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector omega0(0.0f);
	dVector veloc1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);
	
	// calculate the angular velocity for both bodies
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetVelocity(m_body1, &veloc1[0]);
	dVector dir0 (matrix0.m_front.Scale (m_gearRatio));
	const dVector& dir1 = matrix1.m_front;

	jacobian0[0] = dFloat(0.0f);
	jacobian0[1] = dFloat(0.0f);
	jacobian0[2] = dFloat(0.0f);
	jacobian0[3] = dir0.m_x;
	jacobian0[4] = dir0.m_y;
	jacobian0[5] = dir0.m_z;

	jacobian1[0] = dir1.m_x;
	jacobian1[1] = dir1.m_y;
	jacobian1[2] = dir1.m_z;
	jacobian1[3] = dFloat(0.0f);
	jacobian1[4] = dFloat(0.0f);
	jacobian1[5] = dFloat(0.0f);

	dFloat w0 = omega0.DotProduct3(dir0);
	dFloat w1 = veloc1.DotProduct3(dir1);
	dFloat relOmega = w0 + w1;
	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep : 1.0f;
	dFloat relAccel = -0.5f * relOmega * invTimestep;
	NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);
	NewtonUserJointSetRowAcceleration (m_joint, relAccel);
}

