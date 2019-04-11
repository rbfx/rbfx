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
	:dNodeInfo ()
	,m_boneIndex(0)
	,m_lengh (0.0f) 
	,m_type(m_effector)
{
	SetName ("boneNode");
}

dBoneNodeInfo::~dBoneNodeInfo(void)
{
}

dFloat dBoneNodeInfo::GetLength() const
{
	return m_lengh;
}

void dBoneNodeInfo::SetLength(dFloat length)
{
	m_lengh = length;
}

int dBoneNodeInfo::GetId() const
{
	return m_boneIndex;
}

void dBoneNodeInfo::SetId(int id)
{
	m_boneIndex = id;
}

void dBoneNodeInfo::SetType(dBoneType type)
{
	m_type = type;
}

dBoneNodeInfo::dBoneType dBoneNodeInfo::GetType() const
{
	return m_type;
}

void dBoneNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);

	TiXmlElement* boneData = new TiXmlElement ("boneData");
	rootNode->LinkEndChild(boneData);

	boneData->SetAttribute("boneIndex", m_boneIndex);
	boneData->SetDoubleAttribute("length", m_lengh);
	boneData->SetAttribute("type", int (m_type));
}

bool dBoneNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	TiXmlElement* const boneData = (TiXmlElement*) rootNode->FirstChild ("boneData");

	boneData->Attribute("boneIndex", (int*)&m_boneIndex);
	dStringToFloatArray (boneData->Attribute("length"), &m_lengh, 1);
	int type;
	dStringToIntArray(boneData->Attribute("type"), &type, 1);
	m_type = dBoneType(type);
	return true;
}

