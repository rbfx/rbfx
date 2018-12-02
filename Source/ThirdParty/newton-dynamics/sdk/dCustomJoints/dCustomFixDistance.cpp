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


// dCustomBallAndSocket.cpp: implementation of the dCustomBallAndSocket class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomFixDistance.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_CUSTOM_JOINT(dCustomFixDistance);

dCustomFixDistance::dCustomFixDistance(const dVector& pivotInChildInGlobalSpace, const dVector& pivotInParentInGlobalSpace, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(3, child, parent)
{
	dVector dist(pivotInChildInGlobalSpace - pivotInParentInGlobalSpace);
	m_distance = dSqrt(dist.DotProduct3(dist));

	dMatrix childMatrix(dGetIdentityMatrix());
	dMatrix parentMatrix(dGetIdentityMatrix());

	childMatrix.m_posit = pivotInChildInGlobalSpace;
	parentMatrix.m_posit = pivotInParentInGlobalSpace;
	childMatrix.m_posit.m_w = 1.0f;
	parentMatrix.m_posit.m_w = 1.0f;

	dMatrix dummy;
	CalculateLocalMatrix(childMatrix, m_localMatrix0, dummy);
	CalculateLocalMatrix(parentMatrix, dummy, m_localMatrix1);
}

dCustomFixDistance::~dCustomFixDistance()
{
}

void dCustomFixDistance::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
}

void dCustomFixDistance::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize (callback, userData);
}


void dCustomFixDistance::SubmitConstraints(dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	dVector p0(matrix0.m_posit);
	dVector p1(matrix1.m_posit);

	dVector dir(p1 - p0);
	dFloat mag2 = dir.DotProduct3(dir);
	if (mag2 < 1.0e-3f) {
		NewtonUserJointAddLinearRow(m_joint, &p0[0], &p1[0], &matrix0.m_front[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

		NewtonUserJointAddLinearRow(m_joint, &p0[0], &p1[0], &matrix0.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

		NewtonUserJointAddLinearRow(m_joint, &p0[0], &p1[0], &matrix0.m_right[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	} else {
		dir = dir.Scale(1.0f / dSqrt(mag2));
		dMatrix matrix (dGrammSchmidt (dir));
		dFloat x = dSqrt (mag2) - m_distance;

		dVector com0(0.0f);
		dVector com1(0.0f);
		dVector veloc0(0.0f);
		dVector veloc1(0.0f);
		dMatrix body0Matrix;
		dMatrix body1Matrix;

		NewtonBody* const body0 = GetBody0();
		NewtonBody* const body1 = GetBody1();

		NewtonBodyGetCentreOfMass(body0, &com0[0]);
		NewtonBodyGetMatrix(body0, &body0Matrix[0][0]);
		NewtonBodyGetPointVelocity (body0, &p0[0], &veloc0[0]);

		NewtonBodyGetCentreOfMass(body1, &com1[0]);
		NewtonBodyGetMatrix(body1, &body1Matrix[0][0]);
		NewtonBodyGetPointVelocity (body1, &p1[0], &veloc1[0]);

		dFloat v((veloc0 - veloc1).DotProduct3(dir));
		dFloat a = (x - v * timestep) / (timestep * timestep);

		dVector r0 ((p0 - body0Matrix.TransformVector(com0)).CrossProduct(matrix.m_front));
		dVector r1 ((p1 - body1Matrix.TransformVector(com1)).CrossProduct(matrix.m_front));
		dFloat jacobian0[6];
		dFloat jacobian1[6];

		jacobian0[0] = matrix[0][0];
		jacobian0[1] = matrix[0][1];
		jacobian0[2] = matrix[0][2];
		jacobian0[3] = r0[0];
		jacobian0[4] = r0[1];
		jacobian0[5] = r0[2];

		jacobian1[0] = -matrix[0][0];
		jacobian1[1] = -matrix[0][1];
		jacobian1[2] = -matrix[0][2];
		jacobian1[3] = -r1[0];
		jacobian1[4] = -r1[1];
		jacobian1[5] = -r1[2];

		NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
		NewtonUserJointSetRowAcceleration(m_joint, a);
	}
}


