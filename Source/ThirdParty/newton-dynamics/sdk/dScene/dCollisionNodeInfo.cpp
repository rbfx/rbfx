/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionNodeInfo.h
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
#include "dCollisionNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionNodeInfo);

dCollisionNodeInfo::dCollisionNodeInfo(dScene* const world) 
	:dNodeInfo() 
	,m_matrix (dGetIdentityMatrix())
	,m_geometricInertia(0.0f)
	,m_geometricCenterAndVolume(0.0f)
	,m_shapeID(0)
{
	SetName ("collision");
}

dCollisionNodeInfo::dCollisionNodeInfo()
	:dNodeInfo() 
	,m_matrix (dGetIdentityMatrix()) 
	,m_geometricInertia(0.0f)
	,m_geometricCenterAndVolume(0.0f)
	,m_shapeID(0)
{
	SetName ("collision");
}

dCollisionNodeInfo::~dCollisionNodeInfo(void)
{
}


dMatrix dCollisionNodeInfo::GetTransform () const
{
//	dMatrix matrix (dPitchMatrix(m_euler.m_x) * dYawMatrix(m_euler.m_y) * dRollMatrix(m_euler.m_z));
//	matrix.m_posit = m_position;
//	return dMatrix (matrix, m_scale, m_eigenScaleAsis);
	return m_matrix;
}

void dCollisionNodeInfo::SetTransform (const dMatrix& matrix)
{
//	dMatrix transform;
//	matrix.PolarDecomposition(transform, m_scale, m_eigenScaleAsis);
//	m_position = matrix.m_posit;
//	m_euler = transform.GetXYZ_EulerAngles ();
	m_matrix = matrix;
}


void dCollisionNodeInfo::BakeTransform (const dMatrix& transform)
{
	SetTransform (transform.Inverse4x4() * GetTransform() * transform);
}

void dCollisionNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	inertia = dVector (0.0f);
	centerOfMass = dVector (0.0f);
}


void dCollisionNodeInfo::SetShapeId (int id)
{
	m_shapeID = id;
}

int dCollisionNodeInfo::GetShapeId () const
{
	return m_shapeID;
}

void dCollisionNodeInfo::SetInertiaGeometry (const dVector& inertia)
{
	m_geometricInertia = inertia;
}

const dVector& dCollisionNodeInfo::GetInertiaGeometry () const
{
	return m_geometricInertia;
}

void dCollisionNodeInfo::SetCenterOfMassAndVolume (const dVector& comAndVolume)
{
	m_geometricCenterAndVolume = comAndVolume;
}

const dVector& dCollisionNodeInfo::GetCenterMassAndVolume () const
{
	return m_geometricCenterAndVolume;
}


void dCollisionNodeInfo::CalculateGeometryProperies (NewtonCollision* shape, dVector& inertia, dVector& centerOfMass) const
{
	NewtonConvexCollisionCalculateInertialMatrix (shape, &inertia[0], &centerOfMass[0]);	
	centerOfMass.m_w = NewtonConvexCollisionCalculateVolume (shape);
	inertia.m_w = centerOfMass.m_w;
}


bool dCollisionNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	rootNode->Attribute("shapeId", &m_shapeID);

	TiXmlElement* childNode = (TiXmlElement*) rootNode->FirstChild ("offsetMatrix");

	dStringToFloatArray (childNode->Attribute("float16"), &m_matrix[0][0], 16);

	childNode = (TiXmlElement*) rootNode->FirstChild ("geometricInertia");
	dStringToFloatArray (childNode->Attribute("float4"), &m_geometricInertia[0], 4);

	childNode = (TiXmlElement*) rootNode->FirstChild ("geometricCenterAndVolume");
	dStringToFloatArray (childNode->Attribute("float4"), &m_geometricCenterAndVolume[0], 4);

	return true;
}



void dCollisionNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);

	rootNode->SetAttribute("shapeId", m_shapeID);

	char tmp[1024];
	TiXmlElement* dataNode = new TiXmlElement ("offsetMatrix");
	rootNode->LinkEndChild(dataNode);
	dFloatArrayToString (&m_matrix[0][0], 16, tmp, sizeof (tmp));
	dataNode->SetAttribute("float16", tmp);

	dataNode = new TiXmlElement ("geometricInertia");
	rootNode->LinkEndChild(dataNode);
	dFloatArrayToString (&m_geometricInertia[0], 4, tmp, sizeof (tmp));
	dataNode->SetAttribute("float4", tmp);

	dataNode = new TiXmlElement ("geometricCenterAndVolume");
	rootNode->LinkEndChild(dataNode);
	dFloatArrayToString (&m_geometricCenterAndVolume[0], 4, tmp, sizeof (tmp));
	dataNode->SetAttribute("float4", tmp);
}


NewtonCollision* dCollisionNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionNodeInfo::GetRttiType()));

	dAssert (0);
	return NULL;
/*
	// get the collision node	
	dScene::dTreeNode* shapeNode = scene->FindChildByType( bodyNode, dCollisionNodeInfo::GetRttiType());
	dAssert (shapeNode);
	dCollisionNodeInfo* collInfo = (dCollisionNodeInfo*) scene->GetInfoFromNode(shapeNode);

	dMatrix offsetMatrix (collInfo->GetTransform ());
	int collisionID = collInfo->GetShapeId ();

	// crate a newton collision shape from the node.
	NewtonCollision* collision = NULL;
	if (collInfo->GetTypeId() == dCollisionSphereNodeInfo::GetRttiType()) {
		dAssert (0);
	} else if (collInfo->GetTypeId() == dCollisionBoxNodeInfo::GetRttiType()) { 
		dCollisionBoxNodeInfo* box = (dCollisionBoxNodeInfo*) collInfo;
		dVector size (box->GetSize());
		collision = NewtonCreateBox(m_world, size.m_x, size.m_y, size.m_z, collisionID, &offsetMatrix[0][0]);
	} else if (collInfo->GetTypeId() == dCollisionConvexHullNodeInfo::GetRttiType()) { 
		dCollisionConvexHullNodeInfo* convex = (dCollisionConvexHullNodeInfo*) collInfo;
		collision = NewtonCreateConvexHull(m_world, convex->GetVertexCount(), &convex->GetVertexCloud()[0].m_x, sizeof(dVector), 0.01f, collisionID, &offsetMatrix[0][0]);
	} else {
		dAssert (0);
	}
	return collision;
*/
}
