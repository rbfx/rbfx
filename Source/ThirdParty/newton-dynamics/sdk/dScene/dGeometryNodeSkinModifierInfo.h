/////////////////////////////////////////////////////////////////////////////
// Name:        dGeometryNodeSkinModifierInfo.h
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
#include "dGeometryNodeModifierInfo.h"

class dGeometryNodeSkinModifierInfo: public dGeometryNodeModifierInfo
{
	public:
	struct dBoneVertexWeightData
	{
		int m_vertexIndex;
		dFloat m_weight;
		dScene::dTreeNode* m_boneNode;
	};

	struct dBoneWeightIndex
	{
		int m_index[4];
	};


	D_DEFINE_CLASS_NODE(dGeometryNodeSkinModifierInfo, dGeometryNodeModifierInfo, DSCENE_API)

	DSCENE_API dGeometryNodeSkinModifierInfo();
	DSCENE_API dGeometryNodeSkinModifierInfo(dScene* const world);
	DSCENE_API dGeometryNodeSkinModifierInfo(const dGeometryNodeSkinModifierInfo& me);
	DSCENE_API virtual ~dGeometryNodeSkinModifierInfo(void);

//	virtual const dMatrix& GetPivotMatrix () const;
//	virtual void SetPivotMatrix (const dMatrix& matrix);
//	virtual dFloat RayCast (const dVector& p0, const dVector& p1) const {return 1.0f;}

	DSCENE_API virtual void RemoveUnusedVertices(const int* const verteMap);
	DSCENE_API virtual void SkinMesh(dScene::dTreeNode* const myNode, dScene* const world, dBoneVertexWeightData* const skinData, int skinDataCount);  

	DSCENE_API virtual void BakeTransform (const dMatrix& matrix);

	DSCENE_API virtual void Serialize (TiXmlElement* const rootNode) const;
	DSCENE_API virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	int m_boneCount;
	int m_vertexCount;
	dMatrix m_shapeBindMatrix;
	dVector* m_vertexWeights;
	dMatrix* m_boneBindingMatrix;
	dBoneWeightIndex* m_boneWeightIndex;
};





#endif