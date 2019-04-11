/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionConeNodeInfo.h
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
#include "dCollisionConeNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionConeNodeInfo);

dCollisionConeNodeInfo::dCollisionConeNodeInfo(dScene* const world) 
	:dCollisionNodeInfo (), m_radius (0.5f), m_height(1.0f)
{
	SetName ("cone collision");
}

dCollisionConeNodeInfo::dCollisionConeNodeInfo()
	:dCollisionNodeInfo (), m_radius (0.5f), m_height(1.0f)
{
	SetName ("cone collision");
}

dCollisionConeNodeInfo::~dCollisionConeNodeInfo(void)
{
}

dCollisionConeNodeInfo::dCollisionConeNodeInfo(NewtonCollision* const cylinder)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(cylinder, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_CONE);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("cone collision");

	m_radius = record.m_cone.m_radio;
	m_height = record.m_cone.m_height;

	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (cylinder, m_geometricInertia, m_geometricCenterAndVolume); 
}


void dCollisionConeNodeInfo::SetRadius (dFloat radius)
{
	m_radius = radius;
}
void dCollisionConeNodeInfo::SetHeight (dFloat height)
{
	m_height = height;
}

dFloat dCollisionConeNodeInfo::GetRadius () const
{
	return m_radius;
}

dFloat dCollisionConeNodeInfo::GetHeight () const
{
	return m_height;
}



void dCollisionConeNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);

	dVector scale;
	dMatrix stretchAxis;
	dMatrix transformMatrix;
	transform.PolarDecomposition (transformMatrix, scale, stretchAxis);
	m_radius *= scale.m_y;
	m_height *= scale.m_x;
}


void dCollisionConeNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* const shape= NewtonCreateCone(newton, m_radius, m_height, 0, &m_matrix[0][0]);
	CalculateGeometryProperies (shape, inertia, centerOfMass);
	NewtonDestroyCollision(shape);
}


void dCollisionConeNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = new TiXmlElement ("size");
	rootNode->LinkEndChild(dataNode);
	dataNode->SetDoubleAttribute("radius", double (m_radius));
	dataNode->SetDoubleAttribute("height", double (m_height));
}

bool dCollisionConeNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = (TiXmlElement*) rootNode->FirstChild ("size");

	dStringToFloatArray (dataNode->Attribute("radius"), &m_radius, 1);
	dStringToFloatArray (dataNode->Attribute("height"), &m_height, 1);
	return true;
}


NewtonCollision* dCollisionConeNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionConeNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	const dMatrix& offsetMatrix = GetTransform ();
	
	// create a newton collision shape from the node.
	return NewtonCreateCone(world, m_radius, m_height, collisionID, &offsetMatrix[0][0]);
}