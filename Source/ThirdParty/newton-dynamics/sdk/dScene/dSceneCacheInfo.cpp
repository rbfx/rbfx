/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneCacheInfo.h
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
#include "dSceneCacheInfo.h"
#include <tinyxml.h>


D_IMPLEMENT_CLASS_NODE(dSceneCacheInfo);

dSceneCacheInfo::dSceneCacheInfo(dScene* const world) 
	:dNodeInfo () 
	,m_internalCacheID(0)
{
	SetName ("chacheNode");
}

dSceneCacheInfo::dSceneCacheInfo()
	:dNodeInfo () 
	,m_internalCacheID (0)
{
	SetName ("cacheNode");
}

dSceneCacheInfo::~dSceneCacheInfo(void)
{
}

void dSceneCacheInfo::SetID(dCRCTYPE id)
{
	m_internalCacheID = id;
}

dCRCTYPE dSceneCacheInfo::GetID() const
{
	return m_internalCacheID;
}


void dSceneCacheInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);

//	char id[256];
//	itoa64____ (m_internalCacheID, id, 10);
	dString id (m_internalCacheID);
	rootNode->SetAttribute("cacheId", id.GetStr());
}

bool dSceneCacheInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	 char text[1024];
	sprintf (text, "%s", rootNode->Attribute("cacheId"));
	m_internalCacheID = dString((const char*)text).ToInteger64();;
	return true;
}







