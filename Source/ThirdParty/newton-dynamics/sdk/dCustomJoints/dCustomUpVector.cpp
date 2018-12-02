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



// dCustomUpVector.cpp: implementation of the dCustomUpVector class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomUpVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


dCustomUpVector::dCustomUpVector(const dVector& pin, NewtonBody* child)
	:dCustomJoint(2, child, NULL)
{
	dMatrix pivot;

	NewtonBodyGetMatrix(child, &pivot[0][0]);

	dMatrix matrix (dGrammSchmidt(pin));
	matrix.m_posit = pivot.m_posit;

	CalculateLocalMatrix (matrix, m_localMatrix0, m_localMatrix1);
}

dCustomUpVector::~dCustomUpVector()
{
}


// bu animating the orientation of the pin vector the application can change the orientation of the picked object
void dCustomUpVector::SetPinDir (const dVector& pin)
{
	m_localMatrix1 = dGrammSchmidt(pin);
}


void dCustomUpVector::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);
  
	// if the body ha rotated by some amount, the there will be a plane of rotation
	dVector lateralDir (matrix0.m_front.CrossProduct(matrix1.m_front));
	dFloat mag = lateralDir.DotProduct3(lateralDir);
	if (mag > 1.0e-6f) {
		// if the side vector is not zero, it means the body has rotated
		mag = dSqrt (mag);
		lateralDir = lateralDir.Scale (1.0f / mag);
		dFloat angle = dAsin (mag);

		// add an angular constraint to correct the error angle
		NewtonUserJointAddAngularRow (m_joint, angle, &lateralDir[0]);

		// in theory only one correction is needed, but this produces instability as the body may move sideway.
		// a lateral correction prevent this from happening.
		dVector frontDir (lateralDir.CrossProduct(matrix1.m_front));
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &frontDir[0]);
 	} else {
		// if the angle error is very small then two angular correction along the plane axis do the trick
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
	}
}

