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

#ifndef _D_SCENE_CACHE_INFO_H_
#define _D_SCENE_CACHE_INFO_H_

#include "dNodeInfo.h"

class dSceneCacheInfo: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dSceneCacheInfo,dNodeInfo,DSCENE_API)

	dSceneCacheInfo();
	dSceneCacheInfo(dScene* const world);
	virtual ~dSceneCacheInfo(void);

	virtual void SetID(dCRCTYPE id);
	virtual dCRCTYPE GetID() const;

	protected:
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);


	dCRCTYPE m_internalCacheID;
};





#endif