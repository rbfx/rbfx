/////////////////////////////////////////////////////////////////////////////
// Name:        dTextureNodeInfo.h
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
#include "dTextureNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dTextureNodeInfo);

dTextureNodeInfo::dTextureNodeInfo()
	:dNodeInfo (), m_id (0)
	,m_internalUsage(-1) 
	,m_path()
{

}

dTextureNodeInfo::dTextureNodeInfo(dScene* const world)
	:dNodeInfo (), m_id (0)
	,m_internalUsage(-1) 
	,m_path()
{
	SetName ("texture");
}

dTextureNodeInfo::dTextureNodeInfo(const char* const pathName)
	:dNodeInfo (), m_internalUsage(-1) 
	,m_path((char*)pathName)
{
	SetName ("texture");
}

dTextureNodeInfo::~dTextureNodeInfo(void)
{
	if (m_internalUsage != -1)  {
		dAssert (0);
	}
}

void dTextureNodeInfo::SetPathName (const char* const path)
{
	//const char* const ptr = dGetNameFromPath (path);
	//strncpy (m_path, ptr, sizeof (m_path));
	m_path = dGetNameFromPath (path);
	m_id = dCRC64 (m_path.GetStr());
}


void dTextureNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);
	rootNode->SetAttribute("path", m_path.GetStr());
}

bool dTextureNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	SetPathName (rootNode->Attribute ("path"));

	return true;
}

