/////////////////////////////////////////////////////////////////////////////
// Name:        dGeometryNodeInfo.h
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
#include "dScene.h"
#include "dGeometryNodeInfo.h"
#include "dTextureNodeInfo.h"
#include "dMaterialNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dGeometryNodeInfo);


dGeometryNodeInfo::dGeometryNodeInfo()
	:dNodeInfo (), m_matrix (dGetIdentityMatrix())
{
}

dGeometryNodeInfo::dGeometryNodeInfo(dScene* const world)
	:dNodeInfo (), m_matrix (dGetIdentityMatrix())
{
}


dGeometryNodeInfo::dGeometryNodeInfo(const dGeometryNodeInfo& me)
	:dNodeInfo (me), m_matrix (me.m_matrix)
{
}

dGeometryNodeInfo::~dGeometryNodeInfo(void)
{
}

const dMatrix& dGeometryNodeInfo::GetPivotMatrix () const
{
	return m_matrix;
}

void dGeometryNodeInfo::SetPivotMatrix (const dMatrix& matrix)
{
	m_matrix = matrix;
}

dCRCTYPE dGeometryNodeInfo::CalculateSignature() const
{
	dAssert (0);
	return 0;
}

void dGeometryNodeInfo::CalcutateAABB (dVector& p0, dVector& p1) const
{
	dAssert (0);
	p0 = dVector (1.0e10f, 1.0e10f, 1.0e10f, 0.0f);
	p1 = dVector (-1.0e10f, -1.0e10f, -1.0e10f, 0.0f);
}

void dGeometryNodeInfo::BakeTransform (const dMatrix& transform)
{
	dAssert (0);
/*
	dVector scale; 
	dMatrix stretchMatrix;

//	dMatrix matrix (m_matrix * transform);
//	matrix.PolarDecomposition (m_matrix, scale, stretchMatrix);
//	matrix = dMatrix (GetIdentityMatrix(), scale, stretchMatrix);
	dMatrix matrix (transform.Inverse4x4() * m_matrix * transform);
	matrix.PolarDecomposition (m_matrix, scale, stretchMatrix);
	matrix = dMatrix (GetIdentityMatrix(), scale, stretchMatrix) * transform;

	int pointCount = NewtonMeshGetPointCount (m_mesh); 
	int pointStride = NewtonMeshGetPointStrideInByte (m_mesh) / sizeof (dFloat);
	dFloat* const points = NewtonMeshGetPointArray (m_mesh); 
	matrix.TransformTriplex(points, pointStride * sizeof (dFloat), points, pointStride * sizeof (dFloat), pointCount);

	//dMatrix rotation (matrix.Inverse4x4().Transpose() * matrix);
	dMatrix rotation (matrix.Inverse4x4().Transpose());
	rotation.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	rotation.TransformTriplex(&points[3], pointStride * sizeof (dFloat), &points[3], pointStride * sizeof (dFloat), pointCount);
//	for (int i = 0; i < pointCount; i ++) {
//		dVector n (points[i * pointStride + 3], points[i * pointStride + 3 + 4], points[i * pointStride + 3 + 5], 0.0f);
//		n = n.Scale (1.0f / dSqrt (n % n));
//		points[i * pointStride + 3 + 3] = n[0];
//		points[i * pointStride + 3 + 4] = n[1];
//		points[i * pointStride + 3 + 5] = n[2];
//	}

	int vertexCount = NewtonMeshGetVertexCount (m_mesh); 
	int vertexStride = NewtonMeshGetVertexStrideInByte (m_mesh) / sizeof (dFloat);
	dFloat* const vertex = NewtonMeshGetVertexArray (m_mesh); 
	matrix.TransformTriplex(vertex, vertexStride * sizeof (dFloat), vertex, vertexStride * sizeof (dFloat), vertexCount);
*/
}

void dGeometryNodeInfo::Serialize (TiXmlElement* const rootNode) const 
{
	SerialiseBase(dNodeInfo, rootNode);

	char matrixText[32 * 16];

	TiXmlElement* matrix = new TiXmlElement ("pivotMatrix");
	rootNode->LinkEndChild(matrix);
	dFloatArrayToString (&m_matrix[0][0], 16, matrixText, sizeof (matrixText));
	matrix->SetAttribute("float16", matrixText);
}


bool dGeometryNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode)
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	
	TiXmlElement* const matrixNode = (TiXmlElement*) rootNode->FirstChild ("pivotMatrix");
	dStringToFloatArray (matrixNode->Attribute("float16"), &m_matrix[0][0], 16);
	return false;
}