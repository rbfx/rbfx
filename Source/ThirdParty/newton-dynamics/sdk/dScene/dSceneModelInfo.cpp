/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneModelInfo.h
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
#include "dSceneModelInfo.h"
#include <tinyxml.h>


D_IMPLEMENT_CLASS_NODE(dSceneModelInfo);

dSceneModelInfo::dSceneModelInfo(dScene* const world) 
	:dNodeInfo () 
{
	SetName ("model");
}

dSceneModelInfo::dSceneModelInfo()
	:dNodeInfo () 
{
	SetName ("model");
}

dSceneModelInfo::~dSceneModelInfo(void)
{
}

void dSceneModelInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);
/*
	char tmp[1024];

	TiXmlElement* color = new TiXmlElement ("color");
	rootNode->LinkEndChild(color);
	dFloatArrayToString (&m_solidColor[0], 4, tmp, sizeof (tmp));
	color->SetAttribute("float4", tmp);

	TiXmlElement* matrix = new TiXmlElement ("transform");
	rootNode->LinkEndChild(matrix);
	dFloatArrayToString (&m_position[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("position", tmp);

	dFloatArrayToString (&m_euler[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("eulerAngles", tmp);

	dFloatArrayToString (&m_scale[0], 4, tmp, sizeof (tmp));
	matrix->SetAttribute("localScale", tmp);

	dFloatArrayToString (&m_eigenScaleAxis[0][0], 16, tmp, sizeof (tmp));
	matrix->SetAttribute("stretchAxis", tmp);
*/
}

bool dSceneModelInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);
/*
	TiXmlElement* tcolor = (TiXmlElement*) rootNode->FirstChild ("color");
	dStringToFloatArray (tcolor->Attribute("float4"), &m_solidColor[0], 4);

	TiXmlElement* transformNode = (TiXmlElement*) rootNode->FirstChild ("transform");
	dStringToFloatArray (transformNode->Attribute("position"), &m_position[0], 4);
	dStringToFloatArray (transformNode->Attribute("eulerAngles"), &m_euler[0], 4);
	dStringToFloatArray (transformNode->Attribute("localScale"), &m_scale[0], 4);
	dStringToFloatArray (transformNode->Attribute("stretchAxis"), &m_eigenScaleAxis[0][0], 16);
*/
	return true;
}







