/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionConvexHullNodeInfo.h
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
#include "dDrawUtils.h"
#include "dCollisionConvexHullNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionConvexHullNodeInfo);

dCollisionConvexHullNodeInfo::dCollisionConvexHullNodeInfo(dScene* const world) 
	:dCollisionNodeInfo (), m_count(0), m_vertexCloud(NULL)
{
	SetName ("convexHull collision");
}

dCollisionConvexHullNodeInfo::dCollisionConvexHullNodeInfo()
	:dCollisionNodeInfo (), m_count(0), m_vertexCloud(NULL)
{
	SetName ("convexHull collision");
}

dCollisionConvexHullNodeInfo::dCollisionConvexHullNodeInfo(const dCollisionConvexHullNodeInfo& info)
	:dCollisionNodeInfo (info), m_count(info.m_count), m_vertexCloud(NULL)
{
	if (info.m_vertexCloud) {
		m_vertexCloud = new dVector [m_count];
		memcpy (m_vertexCloud, info.m_vertexCloud, m_count * sizeof (dVector));
	}
}

dCollisionConvexHullNodeInfo::dCollisionConvexHullNodeInfo(NewtonCollision* const hull)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(hull, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_CONVEXHULL);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("convexHull collision");

	m_count = record.m_convexHull.m_vertexCount;
	m_vertexCloud = new dVector [m_count];
	int stride = record.m_convexHull.m_vertexStrideInBytes / sizeof (dFloat);
	for (int i = 0; i < m_count; i ++) {
		m_vertexCloud[i] = dVector (record.m_convexHull.m_vertex[i * stride + 0], record.m_convexHull.m_vertex[i * stride + 1], record.m_convexHull.m_vertex[i * stride + 2], 0.0f);
	}
	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (hull, m_geometricInertia, m_geometricCenterAndVolume); 
}

dCollisionConvexHullNodeInfo::~dCollisionConvexHullNodeInfo(void)
{
	if (m_vertexCloud) {
		delete[] m_vertexCloud;
	}
}

int dCollisionConvexHullNodeInfo::GetVertexCount() const
{
	return m_count;
}

const dVector* dCollisionConvexHullNodeInfo::GetVertexCloud() const
{
	return m_vertexCloud;
}


void dCollisionConvexHullNodeInfo::SetFaceSelection (int count, const dFloat* points, int strideInBytes)
{
	if (m_vertexCloud) {
		delete[] m_vertexCloud;
	}
	m_count = count;
	int stride = strideInBytes / sizeof (dFloat);
	m_vertexCloud = new dVector [m_count];
	for (int i = 0; i < m_count; i ++) {
		int index = i * stride;
		m_vertexCloud[i] = dVector (points[index + 0], points[index + 1], points[index + 2], 0.0f);

	}
}


void dCollisionConvexHullNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);
	transform.TransformTriplex(&m_vertexCloud[0].m_x, sizeof (dVector), &m_vertexCloud[0].m_x, sizeof (dVector), m_count);;
}


void dCollisionConvexHullNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* const shape = NewtonCreateConvexHull(newton, m_count, &m_vertexCloud[0][0], sizeof (dVector), 0.0f, 0, &m_matrix[0][0]);

	CalculateGeometryProperies (shape, inertia, centerOfMass);
	NewtonDestroyCollision(shape);

//	NewtonConvexCollisionCalculateInertialMatrix (box, &inertia[0], &centerOfMass[0]);	
//	centerOfMass.m_w = NewtonConvexCollisionCalculateVolume (box);
//	inertia.m_w = centerOfMass.m_w;
//	NewtonReleaseCollision (newton, box);
}


void dCollisionConvexHullNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);

	if (m_vertexCloud) {
		char* buffer = new char[m_count * sizeof (dFloat) * 4 * 12];
		TiXmlElement* const dataNode = new TiXmlElement ("pointCloud");
		rootNode->LinkEndChild(dataNode);
		dFloatArrayToString (&m_vertexCloud[0][0], m_count * 4, buffer, m_count * sizeof (dFloat) * 4 * 12);
		dataNode->SetAttribute("count", m_count);
		dataNode->SetAttribute("float4", buffer);
		delete[] buffer; 
	}
}

bool dCollisionConvexHullNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = (TiXmlElement*) rootNode->FirstChild ("pointCloud");
	if (dataNode) {
		if (m_vertexCloud) {
			delete[] m_vertexCloud;
		}
		dataNode->Attribute("count", &m_count);
		m_vertexCloud = new dVector [m_count];
		dStringToFloatArray (dataNode->Attribute("float4"), &m_vertexCloud[0][0], 4 * m_count);
	}

	return true;
}


NewtonCollision* dCollisionConvexHullNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionConvexHullNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	const dMatrix& offsetMatrix = GetTransform ();

	// create a newton collision shape from the node.
	return NewtonCreateConvexHull(world, m_count, &m_vertexCloud[0].m_x, sizeof(dVector), 0.01f, collisionID, &offsetMatrix[0][0]);
}
