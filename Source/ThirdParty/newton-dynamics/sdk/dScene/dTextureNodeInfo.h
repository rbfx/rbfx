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

#ifndef _D_TEXTURE_NODE_H_
#define _D_TEXTURE_NODE_H_

#include "dNodeInfo.h"

#define D_MAX_TEXTURE_PATH_NAME_LENGTH	128

class dTextureNodeInfo: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dTextureNodeInfo,dNodeInfo,DSCENE_API)

	dTextureNodeInfo(); 
	dTextureNodeInfo(dScene* const world); 
	dTextureNodeInfo(const char* const pathName);
	virtual ~dTextureNodeInfo(void);

	virtual dCRCTYPE GetId () const {return m_id;}
	virtual const char* GetPathName () const {return m_path.GetStr();}
	virtual void SetPathName (const char* const path);

	dCRCTYPE GetInternalId() const {return m_internalUsage;}
	void SetInternalId(dCRCTYPE id) {m_internalUsage = id;}


	protected:
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dCRCTYPE m_id;
	dCRCTYPE m_internalUsage;
	dString m_path;
};





#endif