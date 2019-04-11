/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionBoxNodeInfo.h
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
#include "dCollisionBoxNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionBoxNodeInfo);

dCollisionBoxNodeInfo::dCollisionBoxNodeInfo(dScene* const world) 
	:dCollisionNodeInfo (), m_size (1.0f, 1.0f, 1.0f, 0.0f)
{
	SetName ("box collision");
}

dCollisionBoxNodeInfo::dCollisionBoxNodeInfo()
	:dCollisionNodeInfo (), m_size (1.0f, 1.0f, 1.0f, 0.0f)
{
	SetName ("box collision");
}

dCollisionBoxNodeInfo::~dCollisionBoxNodeInfo(void)
{
}

dCollisionBoxNodeInfo::dCollisionBoxNodeInfo(NewtonCollision* const box)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(box, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_BOX);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("box collision");
	m_size = dVector (record.m_box.m_x, record.m_box.m_y, record.m_box.m_z); 
	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (box, m_geometricInertia, m_geometricCenterAndVolume); 
}

void dCollisionBoxNodeInfo::SetSize (const dVector& size)
{
	m_size = size;
	m_size.m_w = 0.0f;
}

const dVector& dCollisionBoxNodeInfo::GetSize () const
{
	return m_size; 
}

void dCollisionBoxNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);

	dVector scale;
	dMatrix stretchAxis;
	dMatrix transformMatrix;
	transform.PolarDecomposition (transformMatrix, scale, stretchAxis);
	m_size = m_size * scale;
}


void dCollisionBoxNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* const box = NewtonCreateBox(newton, m_size.m_x, m_size.m_y, m_size.m_z, 0, &m_matrix[0][0]);
	CalculateGeometryProperies (box, inertia, centerOfMass);
	NewtonDestroyCollision (box);
}


void dCollisionBoxNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);

	char tmp[1024];
	TiXmlElement* const dataNode = new TiXmlElement ("size");
	rootNode->LinkEndChild(dataNode);
	dFloatArrayToString (&m_size[0], 4, tmp, sizeof (tmp));
	dataNode->SetAttribute("float4", tmp);
}

bool dCollisionBoxNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);

	TiXmlElement* const dataNode = (TiXmlElement*) rootNode->FirstChild ("size");
	dStringToFloatArray (dataNode->Attribute("float4"), &m_size[0], 4);
	return true;
}



NewtonCollision* dCollisionBoxNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionBoxNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	const dMatrix& offsetMatrix = GetTransform ();
	
	// create a newton collision shape from the node.
	return NewtonCreateBox(world, m_size.m_x, m_size.m_y, m_size.m_z, collisionID, &offsetMatrix[0][0]);
}