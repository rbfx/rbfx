/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneNodeInfo.h
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
#include "dSceneRender.h"
#include "dSceneNodeInfo.h"
#include "dGeometryNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dSceneNodeInfo);

dSceneNodeInfo::dSceneNodeInfo(dScene* const world) 
	:dNodeInfo () 
	,m_position (0.0f, 0.0f, 0.0f, 1.0f)  // node location in global space
	,m_euler (0.0f, 0.0f, 0.0f, 1.0f)     // node orientation matrix R: x = pitch, y = yaw, z = roll, 
	,m_scale (1.0f, 1.0f, 1.0f, 1.0f)	// local scale: x = scale_x, y = scale_y, z = scale_z, 
	,m_eigenScaleAxis (dGetIdentityMatrix())
	,m_geometryTransform(dGetIdentityMatrix())
	,m_solidColor (0.75f, 0.75f, 0.0f, 0.0f)
	,m_editorMinOOBB (0.0f) 
	,m_editorMaxOOBB (0.0f)
{
	SetName ("sceneNode");
}

dSceneNodeInfo::dSceneNodeInfo()
	:dNodeInfo ()
	,m_position (0.0f, 0.0f, 0.0f, 1.0f)  // node location in global space
	,m_euler (0.0f, 0.0f, 0.0f, 1.0f)     // node orientation matrix R: x = pitch, y = yaw, z = roll, 
	,m_scale (1.0f, 1.0f, 1.0f, 1.0f)	// local scale: x = scale_x, y = scale_y, z = scale_z, 
	,m_eigenScaleAxis (dGetIdentityMatrix())
	,m_geometryTransform(dGetIdentityMatrix())
	,m_solidColor (0.75f, 0.75f, 0.0f, 0.0f)
	,m_editorMinOOBB (0.0f) 
	,m_editorMaxOOBB (0.0f)
{
	SetName ("sceneNode");
}

dSceneNodeInfo::~dSceneNodeInfo(void)
{
}

dMatrix dSceneNodeInfo::GetGeometryTransform () const
{
	return m_geometryTransform;
}

void dSceneNodeInfo::SetGeometryTransform (const dMatrix& matrix)
{
	m_geometryTransform = matrix;
}

dMatrix dSceneNodeInfo::GetTransform () const
{
//	return dMatrix (CalculateOrthoMatrix()), m_scale, m_eigenScaleAxis);
	return dMatrix (CalculateScaleMatrix() * CalculateOrthoMatrix());
}

void dSceneNodeInfo::SetTransform (const dMatrix& matrix)
{
	dMatrix transform;
	matrix.PolarDecomposition(transform, m_scale, m_eigenScaleAxis);
	m_position = matrix.m_posit;
    dVector tmp(0.0f);
    transform.GetEulerAngles (m_euler, tmp);
}

dMatrix dSceneNodeInfo::CalculateScaleMatrix() const
{
	return dMatrix (dGetIdentityMatrix(), m_scale, m_eigenScaleAxis);
}

dMatrix dSceneNodeInfo::CalculateOrthoMatrix() const
{
	dMatrix matrix (dPitchMatrix(m_euler.m_x) * dYawMatrix(m_euler.m_y) * dRollMatrix(m_euler.m_z));
	matrix.m_posit = m_position;
	return matrix;
}




dVector dSceneNodeInfo::GetPosition () const
{
	return m_position;
}

void dSceneNodeInfo::SetPosition (const dVector& position)
{
	m_position = position;
}

dVector dSceneNodeInfo::GetEulers () const
{
	return m_euler;	
}

void dSceneNodeInfo::SetEulers (const dVector& angles)
{
	m_euler = angles;
}

dVector dSceneNodeInfo::GetScale () const
{
	return m_scale;
}

void dSceneNodeInfo::SetScale (const dVector& scale)
{
	m_scale = scale;
}

dVector dSceneNodeInfo::GetColor () const
{
	return m_solidColor;
}

void dSceneNodeInfo::SetColor (const dVector& color)
{
	m_solidColor = color;
}

void dSceneNodeInfo::BakeTransform (const dMatrix& transform)
{
	dMatrix invert (transform.Inverse4x4());
	SetTransform (invert * GetTransform() * transform);
	SetGeometryTransform(invert * GetGeometryTransform() * transform);
}

void dSceneNodeInfo::UpdateOOBB (dScene* const scene, dScene::dTreeNode* const myNode)
{
	dAssert (scene->GetInfoFromNode(myNode) == this);

	m_editorMinOOBB = dVector (0.0f);
	m_editorMaxOOBB = dVector (0.0f);
	for (void* link = scene->GetFirstChildLink(myNode); link; link = scene->GetNextChildLink(myNode, link)) {
		dScene::dTreeNode* const node = scene->GetNodeFromLink(link);
		dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*)scene->GetInfoFromNode(node);
		if (geometryInfo->IsType(dGeometryNodeInfo::GetRttiType())) {
			dVector p0; 
			dVector p1;
dAssert (0);
			geometryInfo->CalcutateAABB (p0, p1);
			m_editorMinOOBB[0] = dMin(p0[0], m_editorMinOOBB[0]);
			m_editorMinOOBB[1] = dMin(p0[1], m_editorMinOOBB[1]);
			m_editorMinOOBB[2] = dMin(p0[2], m_editorMinOOBB[2]);

			m_editorMaxOOBB[0] = dMax(p1[0], m_editorMaxOOBB[0]);
			m_editorMaxOOBB[1] = dMax(p1[1], m_editorMaxOOBB[1]);
			m_editorMaxOOBB[2] = dMax(p1[2], m_editorMaxOOBB[2]);
		}
	}
}


dFloat dSceneNodeInfo::RayCast (const dVector& localP0, const dVector& localP1) const
{
	return dBoxRayCast (localP0, localP1, m_editorMinOOBB, m_editorMaxOOBB);
}


void dSceneNodeInfo::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dNodeInfo, rootNode);

	char tmp[1024];

	TiXmlElement* const color = new TiXmlElement ("color");
	rootNode->LinkEndChild(color);
	dFloatArrayToString (&m_solidColor[0], 4, tmp, sizeof (tmp));
	color->SetAttribute("float4", tmp);

	TiXmlElement* const matrix = new TiXmlElement ("transform");
	rootNode->LinkEndChild(matrix);
	dFloatArrayToString (&m_position[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("position", tmp);

	dFloatArrayToString (&m_euler[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("eulerAngles", tmp);

	dFloatArrayToString (&m_scale[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("localScale", tmp);

	dFloatArrayToString (&m_eigenScaleAxis[0][0], 16, tmp, sizeof (tmp));
	matrix->SetAttribute("stretchAxis", tmp);

	TiXmlElement* const geometryTransform = new TiXmlElement ("geometryTransform");
	rootNode->LinkEndChild(geometryTransform);
	dFloatArrayToString (&m_geometryTransform[0][0], 16, tmp, sizeof (tmp));
	geometryTransform->SetAttribute("matrix", tmp);
}

bool dSceneNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	TiXmlElement* const tcolor = (TiXmlElement*) rootNode->FirstChild ("color");
	dStringToFloatArray (tcolor->Attribute("float4"), &m_solidColor[0], 4);

	TiXmlElement* const transformNode = (TiXmlElement*) rootNode->FirstChild ("transform");
	dStringToFloatArray (transformNode->Attribute("position"), &m_position[0], 4);
	dStringToFloatArray (transformNode->Attribute("eulerAngles"), &m_euler[0], 4);
	dStringToFloatArray (transformNode->Attribute("localScale"), &m_scale[0], 4);
	dStringToFloatArray (transformNode->Attribute("stretchAxis"), &m_eigenScaleAxis[0][0], 16);

	TiXmlElement* const geometryTransform = (TiXmlElement*) rootNode->FirstChild ("geometryTransform");
	if (geometryTransform ) {
		dStringToFloatArray (geometryTransform ->Attribute("matrix"), &m_geometryTransform[0][0], 16);
	}

	return true;
}



