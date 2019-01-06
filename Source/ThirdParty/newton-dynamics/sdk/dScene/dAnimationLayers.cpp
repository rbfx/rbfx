/////////////////////////////////////////////////////////////////////////////
// Name:        dAnimationLayers.h
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
#include "dAnimationLayers.h"
#include <tinyxml.h>


D_IMPLEMENT_CLASS_NODE(dAnimationLayers);

dAnimationLayers::dAnimationLayers(dScene* const world) 
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationLayers::dAnimationLayers()
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationLayers::~dAnimationLayers(void)
{
}

void dAnimationLayers::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dNodeInfo, rootNode);

//	dString id (m_internalCacheID);
//	rootNode->SetAttribute("animationLayers", id.GetStr());
}

bool dAnimationLayers::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

//	char text[1024];
//	sprintf (text, "%s", rootNode->Attribute("animationLayers"));
//	m_internalCacheID = dString((const char*)text).ToInteger64();;
	return true;
}
