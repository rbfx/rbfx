/////////////////////////////////////////////////////////////////////////////
// Name:        dGeometryNodeSkinClusterInfo.h
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

#ifndef _D_GEOMETRY_NODE_SKIN_MODIFIER_INFO_H_
#define _D_GEOMETRY_NODE_SKIN_MODIFIER_INFO_H_

#include "dNodeInfo.h"

class dGeometryNodeSkinClusterInfo: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dGeometryNodeSkinClusterInfo, dNodeInfo, DSCENE_API)

	DSCENE_API dGeometryNodeSkinClusterInfo();
	DSCENE_API dGeometryNodeSkinClusterInfo(dScene* const world);
	DSCENE_API dGeometryNodeSkinClusterInfo(const dGeometryNodeSkinClusterInfo& me);
	DSCENE_API virtual ~dGeometryNodeSkinClusterInfo(void);

	DSCENE_API virtual void RemoveUnusedVertices(const int* const verteMap);
	DSCENE_API virtual void BakeTransform (const dMatrix& matrix);

	DSCENE_API virtual void Serialize (TiXmlElement* const rootNode) const;
	DSCENE_API virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dMatrix m_basePoseMatrix;
	dArray<int> m_vertexIndex;
	dArray<dFloat> m_vertexWeight;
};

#endif