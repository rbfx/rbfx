/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionSphereNodeInfo.h
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
#include "dCollisionSphereNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionSphereNodeInfo);

dCollisionSphereNodeInfo::dCollisionSphereNodeInfo(dScene* const world) 
	:dCollisionNodeInfo (), m_radius (1.0f)
{
	SetName ("sphere collision");
}

dCollisionSphereNodeInfo::dCollisionSphereNodeInfo()
	:dCollisionNodeInfo (), m_radius (1.0f)
{
	SetName ("sphere collision");
}

dCollisionSphereNodeInfo::dCollisionSphereNodeInfo(NewtonCollision* const  sphere)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(sphere, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_SPHERE);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("sphere collision");
	m_radius = record.m_sphere.m_radio; 
	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (sphere, m_geometricInertia, m_geometricCenterAndVolume); 
}

dCollisionSphereNodeInfo::~dCollisionSphereNodeInfo(void)
{
}

void dCollisionSphereNodeInfo::SetRadius (const dFloat radius)
{
	m_radius = radius;
}

dFloat dCollisionSphereNodeInfo::GetRadius () const
{
	return m_radius; 
}

void dCollisionSphereNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);
	
	dVector scale;
	dMatrix stretchAxis;
	dMatrix transformMatrix;
	transform.PolarDecomposition (transformMatrix, scale, stretchAxis);
	m_radius = m_radius * scale.m_x;
}


void dCollisionSphereNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* const shape = NewtonCreateSphere(newton, m_radius, 0, &m_matrix[0][0]);
	CalculateGeometryProperies (shape, inertia, centerOfMass);
	NewtonDestroyCollision(shape);

//	NewtonConvexCollisionCalculateInertialMatrix (box, &inertia[0], &centerOfMass[0]);	
//	centerOfMass.m_w = NewtonConvexCollisionCalculateVolume (box);
//	inertia.m_w = centerOfMass.m_w;
//	NewtonReleaseCollision (newton, box);
}


void dCollisionSphereNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);

	char tmp[1024];
	TiXmlElement* const dataNode = new TiXmlElement ("radius");
	rootNode->LinkEndChild(dataNode);
	dFloatArrayToString (&m_radius, 1, tmp, sizeof (tmp));
	dataNode->SetAttribute("float1", tmp);
}

bool dCollisionSphereNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = (TiXmlElement*) rootNode->FirstChild ("radius");
	dStringToFloatArray (dataNode->Attribute("float1"), &m_radius, 1);
	return true;
}


NewtonCollision* dCollisionSphereNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionSphereNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	const dMatrix& offsetMatrix = GetTransform ();

	// create a newton collision shape from the node.
	return NewtonCreateSphere(world, m_radius, collisionID, &offsetMatrix[0][0]);

}


