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


// dCustomPulley.cpp: implementation of the dCustomPulley class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomPulley.h"

IMPLEMENT_CUSTOM_JOINT(dCustomPulley);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dCustomPulley::dCustomPulley(dFloat gearRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(1, child, parent)
{
	m_gearRatio = gearRatio;

	// calculate the two local matrix of the pivot point
	dMatrix dommyMatrix;
	// calculate the local matrix for body body0
	dMatrix pinAndPivot0 (dGrammSchmidt (childPin));
	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);
	m_localMatrix0.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dGrammSchmidt (parentPin));
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);
	m_localMatrix1.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// set as kinematoc loop
	SetSolverModel(1);
}

dCustomPulley::~dCustomPulley()
{
}

void dCustomPulley::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void dCustomPulley::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
	callback (userData, &m_gearRatio, sizeof (dFloat));
}


void dCustomPulley::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector veloc0(0.0f);
	dVector veloc1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);

	// set the linear part of Jacobian 0 to translational pin vector	
	dVector dir0 (matrix0.m_front.Scale (1.0f/m_gearRatio));
	const dVector& dir1 = matrix1.m_front;
	jacobian0[0] = dir0.m_x;
	jacobian0[1] = dir0.m_y;	
	jacobian0[2] = dir0.m_z;
	jacobian0[3] = 0.0f;
	jacobian0[4] = 0.0f;
	jacobian0[5] = 0.0f;
	
	jacobian1[0] = dir1.m_x;
	jacobian1[1] = dir1.m_y;	
	jacobian1[2] = dir1.m_z;	
	jacobian1[3] = 0.0f;
	jacobian1[4] = 0.0f;
	jacobian1[5] = 0.0f;

	// calculate the angular velocity for both bodies
	NewtonBodyGetVelocity(m_body0, &veloc0[0]);
	NewtonBodyGetVelocity(m_body1, &veloc1[0]);

	// get angular velocity relative to the pin vector
	dFloat w0 = veloc0.DotProduct3(dir0);
	dFloat w1 = veloc1.DotProduct3(dir1);
	dFloat relVeloc = w0 + w1;

	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep : 1.0f;
	dFloat relAccel = -0.5f * relVeloc * invTimestep;

	// add a angular constraint
	NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);

	// set the desired angular acceleration between the two bodies
	NewtonUserJointSetRowAcceleration (m_joint, relAccel);
}


