/////////////////////////////////////////////////////////////////////////////
// Name:        dLineNodeInfo.h
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
#include "dSceneRender.h"
#include "dLineNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dLineNodeInfo);

#define D_LINE_DEAFAULT_SUBDIVITION_COUNT 100

dLineNodeInfo::dLineNodeInfo()
	:dGeometryNodeInfo (NULL)
	,m_curve()
	,m_renderSegments(D_LINE_DEAFAULT_SUBDIVITION_COUNT)
{
	SetName ("line");
}

dLineNodeInfo::dLineNodeInfo(dScene* const world)
	:dGeometryNodeInfo (world)
	,m_curve()
	,m_renderSegments(D_LINE_DEAFAULT_SUBDIVITION_COUNT)
{
	SetName ("line");
}


dLineNodeInfo::dLineNodeInfo(const dLineNodeInfo& me)
	:dGeometryNodeInfo (me)
	,m_curve(me.m_curve)
	,m_renderSegments(me.m_renderSegments)
{
	dAssert (0);
}

dLineNodeInfo::~dLineNodeInfo(void)
{
}

const dBezierSpline& dLineNodeInfo::GetCurve() const
{
	return m_curve;
}

void dLineNodeInfo::BakeTransform (const dMatrix& transform)
{
/*
	dVector scale; 
	dMatrix stretchMatrix;

	dMatrix matrix (transform.Inverse4x4() * m_matrix * transform);
	matrix.PolarDecomposition (m_matrix, scale, stretchMatrix);
	matrix = transform * dMatrix (dGetIdentityMatrix(), scale, stretchMatrix);

	matrix.TransformTriplex (&m_curve.GetControlPointArray()[0].m_x, sizeof (dBigVector), &m_curve.GetControlPointArray()[0].m_x, sizeof (dBigVector), m_curve.GetControlPointCount());
*/
	transform.TransformTriplex(&m_curve.GetControlPointArray()[0].m_x, sizeof(dBigVector), &m_curve.GetControlPointArray()[0].m_x, sizeof(dBigVector), m_curve.GetControlPointCount());
}

void dLineNodeInfo::DrawWireFrame(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (myNode == scene->Find(GetNodeID()));
	dAssert (scene->GetInfoFromNode(myNode) == this);

	//	int displayList = render->GetCachedFlatShadedDisplayList(m_mesh);
	//	dAssert (displayList > 0);
	if (m_curve.GetControlPointArray()) {

		dVector savedColor (render->GetColor());

		//render->PushMatrix(&m_matrix[0][0]);
		//render->DrawDisplayList(displayList);

		render->BeginLine();
		render->SetColor(dVector (1.0f, 1.0f, 1.0f));

		dFloat scale = 1.0f / m_renderSegments;
		dBigVector p0 (m_curve.CurvePoint(0.0f));
		for (int i = 1; i <= m_renderSegments; i ++) {
			dFloat u = i * scale;
			dBigVector p1 (m_curve.CurvePoint(u));
			render->DrawLine (dVector(dFloat(p0.m_x), dFloat(p0.m_y), dFloat(p0.m_z), dFloat(p0.m_w)), dVector(dFloat(p1.m_x), dFloat(p1.m_y), dFloat(p1.m_z), dFloat(p1.m_w)));
			p0 = p1;
		}
		render->End();
		//render->PopMatrix();

		render->SetColor(savedColor);
	}
}

void dLineNodeInfo::DrawFlatShaded(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	DrawWireFrame (render, scene, myNode);
}



dCRCTYPE dLineNodeInfo::CalculateSignature() const
{
	dCRCTYPE signature = 0;
	dAssert (0);
#if 0
	int vertexCount = NewtonMeshGetVertexCount (m_mesh); 
	int vertexStride = NewtonMeshGetVertexStrideInByte(m_mesh);
	signature = dCRC64 (NewtonMeshGetVertexArray (m_mesh), vertexStride * vertexCount, signature);

	// for now just compare the vertex array, do no forget to add more text using the face winding and material indexed  


	// save the polygon array
	int faceCount = NewtonMeshGetTotalFaceCount (mesh); 
	int indexCount = NewtonMeshGetTotalIndexCount (mesh); 

	int* const faceArray = new int [faceCount];
	void** const indexArray = new void* [indexCount];
	int* const materialIndexArray = new int [faceCount];
	int* const remapedIndexArray = new int [indexCount];

	NewtonMeshGetFaces (mesh, faceArray, materialIndexArray, indexArray); 

	// save the faces vertex Count
	dIntArrayToString (faceArray, faceCount, buffer, bufferSizeInBytes);
	TiXmlElement* const polygons = new TiXmlElement ("polygons");
	rootNode->LinkEndChild(polygons);
	polygons->SetAttribute("count", faceCount);
	polygons->SetAttribute("faceIndexCount", buffer);

	dIntArrayToString (materialIndexArray, faceCount, buffer, bufferSizeInBytes);
	TiXmlElement* const faceMaterial = new TiXmlElement ("faceMaterial");
	polygons->LinkEndChild(faceMaterial);
	faceMaterial->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetVertexIndex (mesh, indexArray[i]);
		remapedIndexArray[i] = vertexIndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const positionIndex = new TiXmlElement ("position");
	polygons->LinkEndChild(positionIndex);
	positionIndex->SetAttribute("index", buffer);


	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = normalIndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const normalIndex = new TiXmlElement ("normal");
	polygons->LinkEndChild(normalIndex);
	normalIndex->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = uv0IndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const uv0Index = new TiXmlElement ("uv0");
	polygons->LinkEndChild(uv0Index);
	uv0Index->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = uv1IndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const uv1Index = new TiXmlElement ("uv1");
	polygons->LinkEndChild(uv1Index);
	uv1Index->SetAttribute("index", buffer);
#endif
	return signature;
}


void dLineNodeInfo::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dGeometryNodeInfo, rootNode);

	TiXmlElement* const bezierCurve = new TiXmlElement ("dBezierSpline");
	rootNode->LinkEndChild (bezierCurve);

	bezierCurve->SetAttribute("degree", m_curve.GetDegree());

	int pointCount = m_curve.GetControlPointCount();
	const dBigVector* const controlPoints = m_curve.GetControlPointArray();
	int bufferSizeInBytes = pointCount * sizeof (dVector) * 3;
	char* const buffer = new char[bufferSizeInBytes];
	dFloatArrayToString (&controlPoints[0][0], pointCount * sizeof (dBigVector) / sizeof (dFloat64), buffer, bufferSizeInBytes);

	TiXmlElement* const ctrlPoints = new TiXmlElement ("controlPoints");
	bezierCurve->LinkEndChild (ctrlPoints);
	ctrlPoints->SetAttribute("float4", pointCount);
	ctrlPoints->SetAttribute("floats", buffer);

	int knotCount = m_curve.GetKnotCount();
	const dFloat64* const knotPoints = m_curve.GetKnotArray();
	int buffer1SizeInBytes = knotCount * sizeof (dFloat64) * 3;
	char* const buffer1 = new char[buffer1SizeInBytes];
	dFloatArrayToString (knotPoints, knotCount, buffer1, buffer1SizeInBytes);

	TiXmlElement* const knotVector = new TiXmlElement ("knotVector");
	bezierCurve->LinkEndChild (knotVector);
	knotVector->SetAttribute("float", knotCount);
	knotVector->SetAttribute("floats", buffer1);

	delete[] buffer;
	delete[] buffer1;
}


bool dLineNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dGeometryNodeInfo, rootNode);
	TiXmlElement* const bezierCurve = (TiXmlElement*) rootNode->FirstChild ("dBezierSpline");

	int degree;
	bezierCurve->Attribute("degree", &degree);

	TiXmlElement* const ctrlPoints = (TiXmlElement*) bezierCurve->FirstChild ("controlPoints");
	int positionCount;
	ctrlPoints->Attribute("float4", &positionCount);
	dBigVector* const positions = new dBigVector[positionCount];
	dStringToFloatArray (ctrlPoints->Attribute("floats"), &positions[0][0], 4 * positionCount);
	
	TiXmlElement* const knots = (TiXmlElement*) bezierCurve->FirstChild ("knotVector");
	int knotCount;
	knots->Attribute("float", &knotCount);
	dFloat64* const knotVector = new dFloat64[knotCount];
	dStringToFloatArray (knots->Attribute("floats"), knotVector, knotCount);

	dAssert (positionCount == (knotCount- 2 * (degree-1)));
	m_curve.CreateFromKnotVectorAndControlPoints (degree, knotCount- 2 * degree, &knotVector[degree], positions);
	delete[] knotVector;	
	delete[] positions;	

	return true;
}


