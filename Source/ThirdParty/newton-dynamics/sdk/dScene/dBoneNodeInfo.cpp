/////////////////////////////////////////////////////////////////////////////
// Name:        dBoneNodeInfo.h
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
#include "dBoneNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dBoneNodeInfo);

dBoneNodeInfo::dBoneNodeInfo()
	:dSceneNodeInfo (), m_lengh (0.0f) 
{
	SetName ("boneNode");
}

dBoneNodeInfo::~dBoneNodeInfo(void)
{
}



void dBoneNodeInfo::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dSceneNodeInfo, rootNode);

	TiXmlElement* boneData = new TiXmlElement ("boneData");
	rootNode->LinkEndChild(boneData);

	boneData->SetDoubleAttribute("length", m_lengh);
}


bool dBoneNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dSceneNodeInfo, rootNode);

	TiXmlElement* const boneData = (TiXmlElement*) rootNode->FirstChild ("boneData");
	dStringToFloatArray (boneData->Attribute("length"), &m_lengh, 1);
	return true;
}

