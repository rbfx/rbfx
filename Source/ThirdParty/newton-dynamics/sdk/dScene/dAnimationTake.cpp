/////////////////////////////////////////////////////////////////////////////
// Name:        dAnimationTake.h
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
#include "dAnimationTake.h"
#include <tinyxml.h>


D_IMPLEMENT_CLASS_NODE(dAnimationTake);

dAnimationTake::dAnimationTake(dScene* const world) 
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationTake::dAnimationTake()
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationTake::~dAnimationTake(void)
{
}

void dAnimationTake::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dNodeInfo, rootNode);
}

bool dAnimationTake::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);
	return true;
}
