/////////////////////////////////////////////////////////////////////////////
// Name:        dGeometryNodeModifierInfo.h
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
#include "dGeometryNodeModifierInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dGeometryNodeModifierInfo);


dGeometryNodeModifierInfo::dGeometryNodeModifierInfo()
	:dNodeInfo ()
{
}

dGeometryNodeModifierInfo::dGeometryNodeModifierInfo(dScene* const world)
	:dNodeInfo ()
{
}


dGeometryNodeModifierInfo::dGeometryNodeModifierInfo(const dGeometryNodeModifierInfo& me)
	:dNodeInfo (me)
{
}

dGeometryNodeModifierInfo::~dGeometryNodeModifierInfo(void)
{
}

void dGeometryNodeModifierInfo::RemoveUnusedVertices(const int* const verteMap)
{

}

/*
const dMatrix& dGeometryNodeModifierInfo::GetPivotMatrix () const
{
	return m_matrix;
}

void dGeometryNodeModifierInfo::SetPivotMatrix (const dMatrix& matrix)
{
	m_matrix = matrix;
}


void dGeometryNodeModifierInfo::BakeTransform (const dMatrix& transform)
{
	dAssert (0);
}
*/


void dGeometryNodeModifierInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);
}

bool dGeometryNodeModifierInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);
	return true;
}