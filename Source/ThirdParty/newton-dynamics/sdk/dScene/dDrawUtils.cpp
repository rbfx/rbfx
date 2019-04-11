/////////////////////////////////////////////////////////////////////////////
// Name:        dDrawUtils.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 07:45:05
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////


#include "dSceneStdafx.h"
#include "dDrawUtils.h"



int CreateCone (dVector* const points, dVector* const normals, int segments, dFloat radius, dFloat height, int maxPoints)
{
	dMatrix rotation (dPitchMatrix((-360.0f / segments) * dDegreeToRad));
	dVector p0 (0.0f, 0.0f, 0.0f, 0.0f);
	dVector p1 (0.0f, radius, 0.0f, 0.0f);
	dVector q1 (height, 0.0f, 0.0f, 0.0f);

	dVector n (radius, height, 0.0f, 0.0f);
	n = n.Scale (1.0f / dSqrt (n.DotProduct3(n)));

	int count = 0;
	for (int i = 0; (i < segments) && (count < maxPoints); i ++) {
		dVector p2 (rotation.RotateVector(p1));

		normals[count] = dVector (-1.0f, 0.0f, 0.0f, 0.0f);
		points[count * 3 + 0] = p0;
		points[count * 3 + 1] = p1;
		points[count * 3 + 2] = p2;
		count ++;

		normals[count] = dVector (n.m_x, n.m_y, n.m_z, 0.0f);
		points[count * 3 + 0] = p1;
		points[count * 3 + 1] = q1;
		points[count * 3 + 2] = p2;
		count ++;

		n = rotation.RotateVector(n);
		p1 = p2;
	}
	return count;
}

int CreateCylinder (dVector* const points, dVector* const normals, int segments, dFloat radius, dFloat height, int maxPoints)
{
	dMatrix rotation (dPitchMatrix((-360.0f / segments) * dDegreeToRad));
	dVector p0 (0.0f, 0.0f, 0.0f, 0.0f);
	dVector p1 (0.0f, radius, 0.0f, 0.0f);

	dVector q0 (height, 0.0f, 0.0f, 0.0f);
	dVector q1 (height, radius, 0.0f, 0.0f);
	dVector n (0.0f, 1.0f, 0.0f, 0.0f);

	int count = 0;
	for (int i = 0; (i < segments) && (count < maxPoints); i ++) {
		dVector p2 (rotation.RotateVector(p1));
		normals[count] = dVector (-1.0f, 0.0f, 0.0f, 0.0f);
		points[count * 3 + 0] = p0;
		points[count * 3 + 1] = p1;
		points[count * 3 + 2] = p2;
		count ++;

		dVector q2 (rotation.RotateVector(q1));
		normals[count] = dVector (1.0f, 0.0f, 0.0f, 0.0f);
		points[count * 3 + 0] = q0;
		points[count * 3 + 1] = q2;
		points[count * 3 + 2] = q1;
		count ++;

		normals[count] = n;
		points[count * 3 + 0] = p1;
		points[count * 3 + 1] = q1;
		points[count * 3 + 2] = p2;
		count ++;

		normals[count] = n;
		points[count * 3 + 0] = p2;
		points[count * 3 + 1] = q1;
		points[count * 3 + 2] = q2;
		count ++;

		n = rotation.RotateVector(n);
		p1 = p2;
		q1 = q2;
	}
	return count;
}

/*
static void SetMaterial (const dVector& color)
{
	dAssert (0);

	SetDefaultLight ();
	dVector ambient(0.0f);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &color.m_x);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &ambient.m_x);
	glMaterialfv(GL_FRONT, GL_AMBIENT, &ambient.m_x);
	glMaterialfv(GL_FRONT, GL_EMISSION, &ambient.m_x);
	glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_CULL_FACE);
	glCullFace (GL_BACK);
	glFrontFace (GL_CCW);
	glShadeModel (GL_SMOOTH);
}
*/

void Draw3DArrow (const dMatrix& location, int segments, dFloat radius, dFloat height, const dVector& color)
{

	dAssert (0);
/*
	SetMaterial (color);

	dVector points[128 * 3];  
	dVector normals[128];  
	int count = CreateCone (points, normals, segments, radius, height, 128);

	glPushMatrix();
	glMultMatrix(&location[0][0]);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < count; i ++) {
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 0].m_x, points[i * 3 + 0].m_y, points[i * 3 + 0].m_z); 
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 1].m_x, points[i * 3 + 1].m_y, points[i * 3 + 1].m_z); 
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 2].m_x, points[i * 3 + 2].m_y, points[i * 3 + 2].m_z); 
	}
	glEnd();
	glPopMatrix();
*/
}


void Draw3DCylinder (const dMatrix& location, int segments, dFloat radius, dFloat height, const dVector& color)
{
dAssert (0);
/*
	SetMaterial (color);

	dVector points[128 * 3];  
	dVector normals[128];  
	int count = CreateCylinder (points, normals, segments, radius, height, 128);

	glPushMatrix();
	glMultMatrix(&location[0][0]);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < count; i ++) {
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 0].m_x, points[i * 3 + 0].m_y, points[i * 3 + 0].m_z); 
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 1].m_x, points[i * 3 + 1].m_y, points[i * 3 + 1].m_z); 
		glNormal3f(normals[i].m_x, normals[i].m_y, normals[i].m_z);
		glVertex3f(points[i * 3 + 2].m_x, points[i * 3 + 2].m_y, points[i * 3 + 2].m_z); 
	}
	glEnd();
	glPopMatrix();
*/
}

