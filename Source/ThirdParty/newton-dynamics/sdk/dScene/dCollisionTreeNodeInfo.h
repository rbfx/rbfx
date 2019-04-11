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

#ifndef _D_COLLISION_TREE_NODE_H_
#define _D_COLLISION_TREE_NODE_H_


#include "dCollisionNodeInfo.h"

class dCollisionTreeNodeInfo: public dCollisionNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dCollisionTreeNodeInfo,dCollisionNodeInfo,DSCENE_API)

	dCollisionTreeNodeInfo();
	dCollisionTreeNodeInfo(dScene* const world);
	dCollisionTreeNodeInfo(NewtonCollision* const tree);
	virtual ~dCollisionTreeNodeInfo(void);

	virtual void BakeTransform (const dMatrix& transform);
	virtual void CalculateInertiaGeometry (dScene* const world, dVector& Inertia, dVector& centerOfMass) const; 

//	virtual dFloat RayCast (const dVector& p0, const dVector& p1) const;


	protected:
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);
	NewtonCollision* CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const;

	NewtonMesh* m_mesh;
};





#endif