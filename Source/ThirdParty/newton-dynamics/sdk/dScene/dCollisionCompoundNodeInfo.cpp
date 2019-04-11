/////////////////////////////////////////////////////////////////////////////
// Name:        dCollisionCompoundNodeInfo.h
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
#include "dCollisionCompoundNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dCollisionCompoundNodeInfo);

dCollisionCompoundNodeInfo::dCollisionCompoundNodeInfo(dScene* const world) 
	:dCollisionNodeInfo ()
{
	SetName ("compound collision");
}

dCollisionCompoundNodeInfo::dCollisionCompoundNodeInfo()
	:dCollisionNodeInfo ()
{
	SetName ("compound collision");
}


dCollisionCompoundNodeInfo::dCollisionCompoundNodeInfo(NewtonCollision* const compound)
	:dCollisionNodeInfo () 
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(compound, &record);
	dAssert (record.m_collisionType == SERIALIZE_ID_COMPOUND);


	dMatrix& offsetMatrix = *((dMatrix*) record.m_offsetMatrix);
	SetName ("compound collision");

	SetTransform (offsetMatrix);
	SetShapeId (record.m_collisionMaterial.m_userId);

	CalculateGeometryProperies (compound, m_geometricInertia, m_geometricCenterAndVolume); 
}

dCollisionCompoundNodeInfo::~dCollisionCompoundNodeInfo(void)
{
}


void dCollisionCompoundNodeInfo::BakeTransform (const dMatrix& transform)
{
	dCollisionNodeInfo::BakeTransform (transform);
}


void dCollisionCompoundNodeInfo::CalculateInertiaGeometry (dScene* const world, dVector& inertia, dVector& centerOfMass) const
{
	dAssert (0);
/*
	NewtonWorld* const newton = world->GetNewtonWorld();
	NewtonCollision* box = NewtonCreateBox(newton, m_size.m_x, m_size.m_y, m_size.m_z, 0, &m_matrix[0][0]);
	CalculateGeometryProperies (box, inertia, centerOfMass);
	NewtonReleaseCollision (newton, box);
*/
}


void dCollisionCompoundNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dCollisionNodeInfo, rootNode);
	
}

bool dCollisionCompoundNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dCollisionNodeInfo, rootNode);
	return true;
}


NewtonCollision* dCollisionCompoundNodeInfo::CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (IsType (dCollisionCompoundNodeInfo::GetRttiType()));

	// get the collision node	
	int collisionID = GetShapeId ();
	
	NewtonCollision* const collision = NewtonCreateCompoundCollision(world, collisionID); 
	NewtonCompoundCollisionBeginAddRemove(collision);

	// create space to load all sub shapes
	for (void* link = scene->GetFirstChildLink(myNode); link; link = scene->GetNextChildLink (myNode, link)) {
		dScene::dTreeNode* childNode = scene->GetNodeFromLink(link);
		dNodeInfo* info = scene->GetInfoFromNode(childNode);
		if (info->IsType (dCollisionNodeInfo::GetRttiType())) {
			dCollisionNodeInfo* collInfo = (dCollisionNodeInfo*) scene->GetInfoFromNode(childNode);
			NewtonCollision* const convex = collInfo->CreateNewtonCollision (world, scene, childNode);
			if (convex) {
				NewtonCompoundCollisionAddSubCollision(collision, convex);
				NewtonDestroyCollision(convex);
			}
		}
	}
	NewtonCompoundCollisionEndAddRemove(collision);
	
	return collision;

}