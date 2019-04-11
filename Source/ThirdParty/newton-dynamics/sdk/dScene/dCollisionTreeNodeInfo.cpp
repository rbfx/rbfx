/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionTreeNodeInfo.h
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
#include "dCollisionTreeNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionTreeNodeInfo);

dCollisionTreeNodeInfo::dCollisionTreeNodeInfo(dScene* const world) 
	:dCollisionNodeInfo ()
{
	SetName ("Tree collision");
	m_mesh = NewtonMeshCreate (world->GetNewtonWorld());
}

dCollisionTreeNodeInfo::dCollisionTreeNodeInfo()
	:dCollisionNodeInfo ()
{
	SetName ("Tree collision");
	m_mesh = NULL;
}


dCollisionTreeNodeInfo::dCollisionTreeNodeInfo(NewtonCollision* const tree)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(tree, &record);

	m_mesh = NewtonMeshCreateFromCollision(tree);

	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	dAssert (record.m_collisionType == SERIALIZE_ID_TREE);
	SetName ("Tree collision");
	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (tree, m_geometricInertia, m_geometricCenterAndVolume); 
}


dCollisionTreeNodeInfo::~dCollisionTreeNodeInfo(void)
{
	if (m_mesh) {
		NewtonMeshDestroy(m_mesh);
	}
}



void dCollisionTreeNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);
}


void dCollisionTreeNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	inertia = dVector (0.0f);
	centerOfMass = dVector (0.0f);
}


void dCollisionTreeNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);
	SerializeMesh (m_mesh, rootNode);;
}

bool dCollisionTreeNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);
	DeserializeMesh (m_mesh, rootNode); 
	return true;
}


NewtonCollision* dCollisionTreeNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionTreeNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	//const dMatrix& offsetMatrix = GetTransform ();
	
	// create a newton collision shape from the node.
	return NewtonCreateTreeCollisionFromMesh(world, m_mesh, collisionID);

}