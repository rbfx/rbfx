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


// dCustomPathFollow.cpp: implementation of the dCustomPathFollow class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomPathFollow.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


dCustomPathFollow::dCustomPathFollow (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent)
{
	// calculate the two local matrix of the pivot point
	CalculateLocalMatrix (pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
}

dCustomPathFollow::~dCustomPathFollow()
{
}


/*
void dCustomPathFollow::SetPathTarget (const dVector& posit, const dVector& tangent)
{
	m_pointOnPath = posit;
	m_pathTangent = tangent.Scale (1.0f / dSqrt (m_pathTangent % m_pathTangent));
}

void dCustomPathFollow::GetPathTarget (dVector& posit, dVector& tangent) const 
{
	posit = m_pointOnPath;
	tangent = m_pathTangent;
}
*/


void dCustomPathFollow::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector pathPosit(0.0f);
	dVector pathTangent(0.0f);
		
	// Get the global matrices of each rigid body.
	CalculateGlobalMatrix (matrix0, matrix1);

	GetPointAndTangentAtLocation(matrix0.m_posit, pathPosit, pathTangent);
	if (pathTangent.DotProduct3(matrix0.m_front) < 0.0f) {
		pathTangent = pathTangent.Scale (-1.0f);
	}
	matrix1 = dGrammSchmidt(pathTangent);
	matrix1.m_posit = pathPosit;
	matrix1.m_posit.m_w = 1.0f;

	// Restrict the movement on the pivot point along all tree the normal and bi normal of the path
	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix1.m_posit;
	dVector p00 (p0 + matrix0.m_front.Scale(50.0f));
	dVector p11 (p1 + matrix1.m_front.Scale(50.0f));

	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix1[1][0]);
	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix1[2][0]);
	NewtonUserJointAddLinearRow(m_joint, &p00[0], &p11[0], &matrix1[1][0]);
	NewtonUserJointAddLinearRow(m_joint, &p00[0], &p11[0], &matrix1[2][0]);
}



