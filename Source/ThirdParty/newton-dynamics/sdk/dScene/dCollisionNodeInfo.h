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

#ifndef _D_COLLISION_NODE_H_
#define _D_COLLISION_NODE_H_

#include "dNodeInfo.h"


class dCollisionNodeInfo: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dCollisionNodeInfo,dNodeInfo,DSCENE_API)

	dCollisionNodeInfo();
	dCollisionNodeInfo(dScene* const world);
	virtual ~dCollisionNodeInfo(void);

	virtual dMatrix GetTransform () const;
	virtual void SetTransform (const dMatrix& matrix);

	virtual void BakeTransform (const dMatrix& transform);
	virtual void CalculateInertiaGeometry (dScene* const world, dVector& Inertia, dVector& centerOfMass) const; 

	virtual void SetShapeId (int id); 
	virtual int GetShapeId () const; 

	virtual void SetInertiaGeometry (const dVector& inertia); 
	virtual const dVector& GetInertiaGeometry () const; 

	virtual void SetCenterOfMassAndVolume (const dVector& comAndVolume); 
	virtual const dVector& GetCenterMassAndVolume () const; 


//	virtual dFloat RayCast (const dVector& p0, const dVector& p1) const;


	virtual NewtonCollision* CreateNewtonCollision (NewtonWorld* const world, dScene* const scene, dScene::dTreeNode* const myNode) const;

	protected:
	virtual void CalculateGeometryProperies (NewtonCollision* shape, dVector& inertia, dVector& centerOfMass) const;

	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dMatrix m_matrix;
	dVector m_geometricInertia;
	dVector m_geometricCenterAndVolume;
	int m_shapeID;
};





#endif