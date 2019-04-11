/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionCapsuleNodeInfo.h
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
#include "dCollisionCapsuleNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionCapsuleNodeInfo);

dCollisionCapsuleNodeInfo::dCollisionCapsuleNodeInfo(dScene* const world) 
	:dCollisionNodeInfo (), m_radius0 (0.5f), m_radius1 (0.5f), m_height(1.0f)
{
	SetName ("capsule collision");
}

dCollisionCapsuleNodeInfo::dCollisionCapsuleNodeInfo()
	:dCollisionNodeInfo (), m_radius0 (0.5f), m_radius1 (0.5f), m_height(1.0f)
{
	SetName ("capsule collision");
}

dCollisionCapsuleNodeInfo::~dCollisionCapsuleNodeInfo(void)
{
}

dCollisionCapsuleNodeInfo::dCollisionCapsuleNodeInfo(NewtonCollision* const cylinder)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(cylinder, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_CAPSULE);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("cylinder collision");

	m_radius0 = record.m_capsule.m_radio0;
	m_radius1 = record.m_capsule.m_radio1;
	m_height = record.m_capsule.m_height;

	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (cylinder, m_geometricInertia, m_geometricCenterAndVolume); 
}


void dCollisionCapsuleNodeInfo::SetRadius (dFloat radius0, dFloat radius1)
{
	m_radius0 = radius0;
	m_radius1 = radius1;
}
void dCollisionCapsuleNodeInfo::SetHeight (dFloat height)
{
	m_height = height;
}

dFloat dCollisionCapsuleNodeInfo::GetRadius0 () const
{
	return m_radius0;
}

dFloat dCollisionCapsuleNodeInfo::GetRadius1() const
{
	return m_radius1;
}


dFloat dCollisionCapsuleNodeInfo::GetHeight () const
{
	return m_height;
}



void dCollisionCapsuleNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);

	dVector scale;
	dMatrix stretchAxis;
	dMatrix transformMatrix;
	transform.PolarDecomposition (transformMatrix, scale, stretchAxis);
	m_radius0 *= scale.m_y;
	m_radius1 *= scale.m_y;
	m_height *= scale.m_x;
}


void dCollisionCapsuleNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* const shape = NewtonCreateCapsule(newton, m_radius0, m_radius1, m_height, 0, &m_matrix[0][0]);
	CalculateGeometryProperies (shape, inertia, centerOfMass);
	NewtonDestroyCollision (shape);
}


void dCollisionCapsuleNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = new TiXmlElement ("size");
	rootNode->LinkEndChild(dataNode);
	dataNode->SetDoubleAttribute("radius0", double (m_radius0));
	dataNode->SetDoubleAttribute("radius1", double (m_radius1));
	dataNode->SetDoubleAttribute("height", double (m_height));
}

bool dCollisionCapsuleNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = (TiXmlElement*) rootNode->FirstChild ("size");
	dStringToFloatArray (dataNode->Attribute("radius0"), &m_radius0, 1);
	dStringToFloatArray (dataNode->Attribute("radius1"), &m_radius1, 1);
	dStringToFloatArray (dataNode->Attribute("height"), &m_height, 1);
	return true;
}


NewtonCollision* dCollisionCapsuleNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionCapsuleNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	const dMatrix& offsetMatrix = GetTransform ();
	
	// create a newton collision shape from the node.
	return NewtonCreateCapsule(world, m_radius0, m_radius1, m_height, collisionID, &offsetMatrix[0][0]);
}