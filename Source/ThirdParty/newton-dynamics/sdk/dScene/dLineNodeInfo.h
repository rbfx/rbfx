/////////////////////////////////////////////////////////////////////////////
// Name:        dLineNodeInfo.h
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

#ifndef _D_LINE_NODE_INFO_H_
#define _D_LINE_NODE_INFO_H_

#include "dNodeInfo.h"
#include "dGeometryNodeInfo.h"

class dLineNodeInfo: public dGeometryNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dLineNodeInfo,dGeometryNodeInfo,DSCENE_API)

	dLineNodeInfo();
	dLineNodeInfo(dScene* const world);
	dLineNodeInfo(const dLineNodeInfo& me);
	virtual ~dLineNodeInfo(void);

	virtual const dBezierSpline& GetCurve() const;

	virtual void BakeTransform (const dMatrix& matrix);
	virtual dCRCTYPE CalculateSignature() const;
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	virtual void DrawWireFrame(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const;
	virtual void DrawFlatShaded(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const;
	dBezierSpline m_curve;
	int m_renderSegments;
};





#endif