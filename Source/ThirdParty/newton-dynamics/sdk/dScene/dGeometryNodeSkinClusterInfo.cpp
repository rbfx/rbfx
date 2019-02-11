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


#include "dSceneStdafx.h"
#include "dScene.h"
#include "dBoneNodeInfo.h"
#include "dMeshNodeInfo.h"
#include "dSceneNodeInfo.h"
#include "dGeometryNodeModifierInfo.h"
#include "dGeometryNodeSkinClusterInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dGeometryNodeSkinClusterInfo);


dGeometryNodeSkinClusterInfo::dGeometryNodeSkinClusterInfo()
	:dNodeInfo()
	,m_basePoseMatrix()
	,m_vertexIndex()
	,m_vertexWeight()
{
	SetName ("Skin Cluster");
}

dGeometryNodeSkinClusterInfo::dGeometryNodeSkinClusterInfo(dScene* const world)
	:dNodeInfo()
	,m_basePoseMatrix()
	,m_vertexIndex()
	,m_vertexWeight()
{
	SetName ("Skin Modifier");
}

dGeometryNodeSkinClusterInfo::dGeometryNodeSkinClusterInfo(const dGeometryNodeSkinClusterInfo& me)
	:dNodeInfo(me)
	,m_basePoseMatrix(me.m_basePoseMatrix)
	,m_vertexIndex(me.m_vertexIndex)
	,m_vertexWeight(me.m_vertexWeight)
{
	dAssert (0);
	SetName ("Skin Cluster");
}

dGeometryNodeSkinClusterInfo::~dGeometryNodeSkinClusterInfo(void)
{
}

void dGeometryNodeSkinClusterInfo::BakeTransform(const dMatrix& transform)
{
	dMatrix inverse(transform.Inverse4x4());
	m_basePoseMatrix = inverse * m_basePoseMatrix * transform;
}

void dGeometryNodeSkinClusterInfo::RemoveUnusedVertices(const int* const vertexMap)
{
	dAssert(0);
}

void dGeometryNodeSkinClusterInfo::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dNodeInfo, rootNode);

	const int bufferSize = 1024 * 64 * sizeof(int);
	char* const buffer = new char [bufferSize];

	TiXmlElement* const geometryTransform = new TiXmlElement("basePoseTransform");
	rootNode->LinkEndChild(geometryTransform);
	dFloatArrayToString(&m_basePoseMatrix[0][0], 16, buffer, bufferSize);
	geometryTransform->SetAttribute("matrix", buffer);

	TiXmlElement* const vertexIndex = new TiXmlElement("vertexIndex");
	rootNode->LinkEndChild(vertexIndex);
	vertexIndex->SetAttribute("count", m_vertexIndex.GetSize());
	dIntArrayToString(&m_vertexIndex[0], m_vertexIndex.GetSize(), buffer, bufferSize);
	vertexIndex->SetAttribute("indices", buffer);

	TiXmlElement* const vertexWeight = new TiXmlElement("vertexWeight");
	rootNode->LinkEndChild(vertexWeight);
	vertexWeight->SetAttribute("count", m_vertexWeight.GetSize());
	dFloatArrayToString(&m_vertexWeight[0], m_vertexWeight.GetSize(), buffer, bufferSize);
	vertexWeight->SetAttribute("weights", buffer);

	delete[] buffer;
}

bool dGeometryNodeSkinClusterInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	TiXmlElement* const transform = (TiXmlElement*) rootNode->FirstChild ("basePoseTransform");
	dStringToFloatArray (transform->Attribute("matrix"), &m_basePoseMatrix[0][0], 16);

	int vertexCount; 
	int weightCount; 
	TiXmlElement* const vertexIndex = (TiXmlElement*) rootNode->FirstChild ("vertexIndex");
	TiXmlElement* const vertexWeight = (TiXmlElement*) rootNode->FirstChild ("vertexWeight");
	vertexIndex->Attribute("count", &vertexCount);
	vertexWeight->Attribute("count", &weightCount);
	dAssert (vertexCount == weightCount);
	m_vertexIndex.Resize(vertexCount);
	m_vertexWeight.Resize(weightCount);

	dStringToIntArray (vertexIndex->Attribute("indices"), &m_vertexIndex[0], vertexCount);
	dStringToFloatArray (vertexWeight->Attribute("weights"), &m_vertexWeight[0], weightCount);

	return true;
}

