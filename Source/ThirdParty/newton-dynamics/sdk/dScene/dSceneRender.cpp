/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneNodeInfo.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 08:02:08
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
#include "dSceneRender.h"

dSceneRender::dSceneRender(void)
{
}

dSceneRender::~dSceneRender(void)
{
}


dVector dSceneRender::GlobalToScreen (const dVector& global) const
{
	dMatrix modelview (GetModelViewMatrix());
	dMatrix projection (GetProjectionMatrix());

	dVector screen (projection.RotateVector4x4(modelview.TransformVector(global)));

//dMatrix matrix (modelview * projection);
//dMatrix invMatrix (matrix.Inverse4x4());
//dVector p2 (invMatrix.RotateVector4x4(screen));

	dAssert (screen.m_w > 0.0f);
	screen.m_w = 1.0f / screen.m_w;
	screen = screen.Scale(screen.m_w);

	dFloat width = dFloat  (GetViewPortWidth());
	dFloat height = dFloat (GetViewPortHeight());

	screen.m_x = 0.5f * (screen.m_x + 1.0f) * width;
	screen.m_y = height - 0.5f * (screen.m_y + 1.0f) * height;
	screen.m_z = 0.5f * (screen.m_z + 1.0f);
	screen.m_w = 1.0f;
	return screen;
}

dVector dSceneRender::ScreenToGlobal (const dVector& screen) const
{
	dFloat width = dFloat  (GetViewPortWidth());
	dFloat height = dFloat (GetViewPortHeight());
	dVector p0 (2.0f * screen.m_x / width - 1.0f, 2.0f * (height - screen.m_y) / height - 1.0f, 2.0f * screen.m_z - 1.0f, 1.0f);

	dMatrix modelview (GetModelViewMatrix());
	dMatrix projection (GetProjectionMatrix());
	dMatrix matrix (modelview * projection);
	dMatrix invMatrix (matrix.Inverse4x4());
	dVector p1 (invMatrix.RotateVector4x4(p0));

	p1.m_w = 1.0f / p1.m_w;
	p1 = p1.Scale(p1.m_w);
	p1.m_w = 1.0f;
	return p1;
}


void dSceneRender::DrawLine(const dVector& p0, const dVector& p1)
{
	SubmitVertex (p0);
	SubmitVertex (p1);
}

void dSceneRender::DrawTriangle(const dVector& p0, const dVector& p1, const dVector& p2)
{
	SubmitVertex (p0);
	SubmitVertex (p1);
	SubmitVertex (p2);
}


void dSceneRender::DrawCylinder(int segments, dFloat radius, dFloat heigh)
{
	dVector q0 ( heigh / 2.0f, radius, 0.0f, 0.0f);
	dVector q1 (-heigh / 2.0f, radius, 0.0f, 0.0f);
	dMatrix rotation (dPitchMatrix(2.0f * 3.1614f/segments));
	dVector cap0[1024];
	dVector cap1[1024];
	dAssert (segments < sizeof (cap0)/sizeof (cap0[0]));
	cap0[segments] = q0;
	cap1[segments] = q1;
	for (int i = 0; i < segments; i ++) {
		cap0[i] = q0;
		cap1[i] = q1;
		q0 = rotation.RotateVector(q0);
		q1 = rotation.RotateVector(q1);
	}
	dVector normal0 ( 1.0f, 0.0f, 0.0f, 0.0f);
	dVector normal1 (-1.0f, 0.0f, 0.0f, 0.0f);

	BeginTriangle();	
	for (int i = 2; i < segments; i ++) {
		SubmitNormal(normal0);
		DrawTriangle(cap0[0], cap0[i-1], cap0[i]);

		SubmitNormal(normal1);
		DrawTriangle(cap1[0], cap1[i], cap1[i - 1]);
	}

	for (int i = 0; i < segments; i ++) {
		dVector p0 (cap0[i]);
		dVector p1 (cap0[i + 1]);
		dVector p2 (cap1[i + 1]);
		dVector p3 (cap1[i]);

		dVector normal ((p1 - p0).CrossProduct(p2 - p0));
		normal = normal.Scale (1.0f / dSqrt(normal.DotProduct3(normal)));

		SubmitNormal(normal);
		DrawTriangle(p0, p2, p1);

		SubmitNormal(normal);
		DrawTriangle(p0, p3, p2);
	}
	End();	
}



void dSceneRender::DrawCone(int segments, dFloat radius, dFloat heigh)
{
	dVector q1 (-heigh / 2.0f, radius, 0.0f, 0.0f);
	dMatrix rotation (dPitchMatrix(2.0f * 3.1614f/segments));
	dVector cap[1024];
	dAssert (segments < sizeof (cap)/sizeof (cap[0]));
	cap[segments] = q1;
	for (int i = 0; i < segments; i ++) {
		cap[i] = q1;
		q1 = rotation.RotateVector(q1);
	}
	dVector normal1 (-1.0f, 0.0f, 0.0f, 0.0f);

	BeginTriangle();	
	for (int i = 2; i < segments; i ++) {
		SubmitNormal(normal1);
		DrawTriangle(cap[0], cap[i], cap[i - 1]);
	}

	dVector p0 ( heigh / 2.0f, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < segments; i ++) {
		dVector p1 (cap[i]);
		dVector p2 (cap[i + 1]);

		dVector normal ((p1 - p0).CrossProduct(p2 - p0));
		normal = normal.Scale (1.0f / dSqrt(normal.DotProduct3(normal)));

		SubmitNormal(normal);
		DrawTriangle(p0, p1, p2);
	}
	End();	


/*
	{
		dVector p1 (-heigh / 2.0f, radius, 0.0f, 0.0f);
		dMatrix rotation (dPitchMatrix(2.0f * 3.1614f/segments));
		dVector cap[1024];
		dAssert (segments < sizeof (cap)/sizeof (cap[0]));
		cap[segments] = p1;
		for (int i = 0; i < segments; i ++) {
			cap[i] = p1;
			p1 = rotation.RotateVector(p1);
		}
		dVector normal1 (-1.0f, 0.0f, 0.0f, 0.0f);

		BeginLine();	
		DrawLine(cap[0], cap[0] + normal1);


		dVector p0 ( heigh / 2.0f, 0.0f, 0.0f, 0.0f);
		for (int i = 0; i < segments; i ++) {
			dVector p1 (cap[i]);
			dVector p2 (cap[i + 1]);

			dVector normal ((p1 - p0) * (p2 - p0));
			normal = normal.Scale (1.0f / dSqrt(normal % normal));

			DrawLine(p0, p0 + normal);
		}
		End();
	}
*/
}


void dSceneRender::DrawArrow (int segments, dFloat radius, dFloat heigh, const dVector& stemColor, const dVector& tipColor)
{
	dMatrix matrix (dGetIdentityMatrix());
	matrix.m_posit.m_x = heigh / 2.0f;
	PushMatrix(matrix);
	SetColor(stemColor);
	DrawCylinder(segments, radius, heigh);

	dFloat tipLength = heigh / 3.0f;
	matrix.m_posit.m_x = (heigh + tipLength) / 2.0f;
	PushMatrix(matrix);
	SetColor(tipColor);
	DrawCone(segments, radius * 2.0f, tipLength);

	PopMatrix();
	PopMatrix();
}

